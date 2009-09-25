# Copyright (C) 2004-2008, Parrot Foundation.
# $Id$

package Parrot::Pmc2c::Parser;

use strict;
use warnings;

use base qw( Exporter );

our @EXPORT_OK = qw( parse_pmc extract_balanced );
use Parrot::Pmc2c::PMC ();
use Parrot::Pmc2c::Attribute ();
use Parrot::Pmc2c::Method ();
use Parrot::Pmc2c::Emitter ();
use Parrot::Pmc2c::UtilFunctions qw(count_newlines filename slurp);
use Text::Balanced 'extract_bracketed';
use File::Basename qw(basename);

=head1 NAME

Parrot::Pmc2c::Parser - PMC Parser

=head1 SYNOPSIS

    use Parrot::Pmc2c::Parser;

=head1 DESCRIPTION

Parrot::Pmc2c::Parser parses a sudo C syntax into a perl hash that is then dumped.


=head2 C<parse_pmc()>

    $parsed_pmc_hash = parse_pmc($pmc2cMain, $filename);

B<Purpose:>  Parse PMC code and return a hash ref of pmc attributes.

B<Arguments:>  List of two arguments:

=over 4

=item *

The pmc2cMain object

=item *

Filename of the pmc to parse.

=back

B<Return Values:>  Reference to a Parrot::Pmc2c::PMC object

B<Comments:>  Called by C<dump_pmc()>.

=cut

sub parse_pmc {
    my ( $pmc2cMain, $filename ) = @_;

    #slurp file contents
    $filename = $pmc2cMain->find_file( filename( $filename, '.pmc' ), 1 );
    my $code  = slurp($filename);

    my ( $preamble, $pmcname, $flags, $parents, $pmcbody, $post, $chewed_lines ) =
        parse_top_level($code);

    my $filebase = basename($filename);
    $filebase =~ s/\.pmc$//;
    die("PMC filename $filebase.pmc does not match pmclass name $pmcname!\n")
        unless lc($filebase) eq lc($pmcname);
    my $pmc = Parrot::Pmc2c::PMC->create($pmcname);
    $pmc->preamble( Parrot::Pmc2c::Emitter->text( $preamble, $filename, 1 ) );
    $pmc->name($pmcname);
    $pmc->set_filename($filename);
    $pmc->set_flags($flags);
    $pmc->set_parents($parents);

    # the +1 puts us on the current line
    my $lineno = count_newlines($preamble) + $chewed_lines + 1;
    my $class_init;

    ($lineno, $pmcbody)    = find_attrs(  $pmc, $pmcbody, $lineno, $filename, $pmc2cMain);
    ($lineno, $class_init) = find_methods($pmc, $pmcbody, $lineno, $filename);

    $pmc->postamble( Parrot::Pmc2c::Emitter->text( $post, $filename, $lineno ) );

    # ensure class_init is the last method in the method list
    $pmc->add_method($class_init) if $class_init;
    $pmc->vtable( $pmc2cMain->read_dump("vtable.pmc") );
    $pmc->pre_method_gen();

    return $pmc;
}

sub find_attrs {
    my ($pmc, $pmcbody, $lineno, $filename, $pmc2cMain) = @_;

    #prepend parent ATTRs to this PMC's ATTR list, if possible
    my $got_attrs_from = '';
    foreach my $parent ( @{ $pmc->{parents} } ) {

        my $parent_dump = $pmc2cMain->read_dump( lc($parent) . '.dump' );

        if ( $got_attrs_from ne '' && $parent_dump->{has_attribute} ) {
            die "$filename is trying to extend $got_attrs_from and $parent, ".
                "but both these PMCs have ATTRs.";
        }

        if ( $parent_dump->{has_attribute} ) {
            $got_attrs_from = $parent;
            foreach my $parent_attrs ( @{ $parent_dump->{attributes} } ) {
                $pmc->add_attribute($parent_attrs);
            }
        }
    }

    # backreferences here are all +1 because below the qr is wrapped in quotes
    my $attr_re = qr{
        ^
        (?:
          [;\n\s]*            # blank spaces and spurious semicolons
          (?:/\*.*?\*/)?      # C comments
        )*

        # attribute marker
        ATTR

        # type
        \s+
        (   U?INTVAL
          | FLOATVAL
          | STRING\s+\*
          | PMC\s+\*
          | (?:struct\s+)?\w+\s+\*+
          | (?:unsigned\s+)?char\s+\*+
          | \w*
        )

        # name
        \s*
        (
            \w+
          | \(\*\w*\)\(.*?\)
        )

        # Array size
        (\[\d+\])?

        # modifiers
        \s*
        ((?::\w+\s*)*)

        # declaration terminator
        ;

    # optional comment
    \s*
    (/\*.*?\*/)?
    }sx;

    while ($pmcbody =~ s/($attr_re)//o) {
        my ($type, $name, $array_size, @modifiers, $comment);
        $type = $2;
        $name = $3;
        $array_size = $4 || '';
        @modifiers = split /\s/, $5;
        $comment = $6;

        $lineno += count_newlines($1);

        $pmc->add_attribute(Parrot::Pmc2c::Attribute->new(
            {
                name       => $name,
                type       => $type,
                array_size => $array_size,
                modifiers  => \@modifiers,
            }
        ));
    }

    return ($lineno, $pmcbody);
}

sub find_methods {
    my ($pmc, $pmcbody, $lineno, $filename) = @_;
    my $class_init                          = 0;

    # backreferences here are all +1 because below the qr is wrapped in quotes
    my $signature_re = qr{
        ^
        (?:
          [;\n\s]*            # blank spaces and spurious semicolons
          (?:/\*.*?\*/)?      # C comments
        )*

        ((?:PARROT_\w+\s+)+)? # decorators

        # vtable, method, or multi marker
        (?:(VTABLE|METHOD|MULTI)\s+)?

        ((?:\w+\s*?\**\s*)?\w+) # method name (includes return type)
        \s*
        \((                     # parameters
            (?:\w+\s*\*?        # type (pointer optional)
            \s*
            \w+                 # argument name
            (?:
              \s+:\w+           # attribute
              (?:\("[^\)]+"\))? # with optional parameter
            )*
            ,?\s*               # probably a comma and whitespace
            )*                  # zero or more of these bad boys
        )\)
        \s*
        ((?::(\w+)\s*)*)        # method attrs
        \s*
    }sx;

    while ( $pmcbody =~ s/($signature_re)//o ) {
        my ( $decorators, $marker, $methodname, $parameters, $rawattrs ) =
            ( $2, $3, $4, $5, $6 );
        my $attrs = defined $rawattrs ? parse_method_attrs($rawattrs) : {};
        $lineno  += count_newlines($1);

        my $returntype = '';

        if ($methodname =~ /(.*\s+\*?)(\w+)/) {
            ($returntype, $methodname) = ($1, $2);
        }

        ( my $methodblock, $pmcbody ) = extract_balanced($pmcbody);
        my $block_lines = count_newlines($methodblock);

        $methodblock = strip_outer_brackets($methodblock);

        # remove pmclass 4 space indent
        $methodblock =~ s/^[ ]{4}//mg;

        # trim trailing ws from last line
        $methodblock =~ s/\n[\t ]+$/\n/g;

        $decorators ||= '';
        $decorators   =~ s/^\s*(.*?)\s*$/$1/s;
        $decorators   = [ split /\s+/ => $decorators ];

        $returntype = 'void' if (defined $marker && $marker eq 'METHOD');

        my $method = Parrot::Pmc2c::Method->new(
            {
                name        => $methodname,
                parent_name => $pmc->name,
                body        => Parrot::Pmc2c::Emitter->text( $methodblock, $filename, $lineno ),
                return_type => $returntype,
                parameters  => $parameters,
                type        => Parrot::Pmc2c::Method::VTABLE,
                attrs       => $attrs,
                decorators  => $decorators,
            }
        );

        # METHOD needs FixedIntegerArray header
        if ( $marker and $marker =~ /METHOD/ ) {
            Parrot::Pmc2c::PCCMETHOD::rewrite_pccmethod( $method, $pmc );
            $pmc->set_flag('need_fia_header');
        }

        if ( $marker and $marker =~ /MULTI/ ) {
            Parrot::Pmc2c::MULTI::rewrite_multi_sub( $method, $pmc );
        }

        # PCCINVOKE needs FixedIntegerArray header
        $pmc->set_flag('need_fia_header') if $methodblock =~ /PCCINVOKE/;

        # the class_init method is added last after all other methods
        if ( $methodname eq 'class_init' ) {
            $class_init = $method;
        }
        else {

            # Name-mangle NCI and multi methods to avoid conflict with vtable methods.
            if ( $marker) {
                if ( $marker =~ /MULTI/ ) {
                    $method->type(Parrot::Pmc2c::Method::MULTI);
                    $method->symbol($methodname);
                }
                elsif ( $marker !~ /VTABLE/ ) {
                    $method->type(Parrot::Pmc2c::Method::NON_VTABLE);
                    $method->name("nci_$methodname");
                    $method->symbol($methodname);
                }
            }

            # To be deprecated
            parse_mmds( $method, $filename, $lineno )
                if $methodblock =~ /\bMMD_(\w+):/;

            $pmc->add_method($method);
        }

        $lineno += $block_lines;
    }

    # include the remainder in the line count, minus the last one
    # (the last one is included in the postamble directly)
    chomp $pmcbody;
    $lineno += count_newlines($pmcbody);

    return ($lineno, $class_init);
}

sub parse_mmds {
    my ( $method, $filename, $lineno ) = @_;
    my $mmd_methods         = [];
    my $body_text           = $method->body;
    my $default_body        = $body_text;
    my $default_body_lineno = $lineno;

    # now split into MMD if necessary:
    while ( $body_text =~ s/(\bMMD_(\w+):\s*)// ) {

        $lineno       += count_newlines($1);
        my $right_type = $2;

        $method->add_mmd_rights($right_type);

        ( my $mmd_part, $body_text ) = extract_bracketed_body_text( $body_text, '{' );

        die "Empty MMD body near '$body_text'" unless $mmd_part;
        my $mmd_part_lines = count_newlines($mmd_part);

        # remove whitespace at end of last line
        $mmd_part =~ s/\n\s*$/\n/s;

        if ( $right_type eq 'DEFAULT' ) {
            $default_body        = $mmd_part;
            $default_body_lineno = $lineno;
        }
        else {
            my $mmd_method = Parrot::Pmc2c::Method->new(
                {
                    name        => $method->name . "_$right_type",
                    parent_name => $method->parent_name,
                    body        => Parrot::Pmc2c::Emitter->text( $mmd_part, $filename, $lineno ),
                    return_type => $method->return_type,
                    parameters  => $method->parameters,
                    type        => Parrot::Pmc2c::Method::VTABLE,
                    attrs       => $method->attrs,
                    right       => $right_type,
                }
            );

            push @{$mmd_methods}, $mmd_method;
        }

        $lineno += $mmd_part_lines;
    }
    $method->mmds($mmd_methods);
    $method->body( Parrot::Pmc2c::Emitter->text( $default_body, $filename, $default_body_lineno ) );
}

sub strip_outer_brackets {
    my ($method_body) = @_;
    die "First character in $method_body is not a {"
        unless substr( $method_body, 0,  1 ) eq '{';

    die "Last character in $method_body is not a }"
        unless substr( $method_body, -1, 1 ) eq '}';

    return substr $method_body, 1, -1;
}

sub extract_bracketed_body_text {
    my ( $body_text, $bracketed ) = @_;
    my ( $extracted, $remaining ) = extract_bracketed( $body_text, $bracketed );
    return ( strip_outer_brackets($extracted), $remaining );
}

=head2 C<parse_top_level()>

    my ($preamble, $pmcname, $flags, $parents, $pmcbody, $post, $chewed_lines)
        = parse_top_level(\$code);

B<Purpose:>  Extract a pmc signature from the code ref.

B<Argument:>  PMC file contents slurped by C<parse_pmc()>.

B<Return Values:>  List of seven elements:

=over 4

=item *

the code found before the pmc signature;

=item *

the name of the pmc

=item *

a hash ref containing the flags associated with the pmc (such as
C<extends> and C<provides>).

=item *

the list of parents this pmc extends

=item *

the body of the pmc

=item *

the code found after the pmc body

=item *

number of newlines in the pmc signature that need to be added to the
running total of lines in the file

=back

B<Comments:>  Called internally by C<parse_pmc()>.

=cut

sub parse_top_level {
    my $code = shift;

    my $top_level_re = qr{
        ^                 # beginning of line
        (.*?)             # preamble
        ^
        (
            \s*
            pmclass       # pmclass keyword
            \s+           # whitespace
            ([\w]*)       # pmc name
            ((?:\s+\w+)*)   # pmc attributes
            \s*           # whitespace
        )
        \{                # pmc body beginning marker
    }smx;
    $code =~ s[$top_level_re][{]smx or die "No pmclass found\n";
    my ( $preamble, $pmc_signature, $pmcname, $attributes ) = ( $1, $2, $3, $4 );

    my $chewed_lines         = count_newlines($pmc_signature);
    my ( $flags, $parents )  = parse_flags( $attributes, $pmcname );
    my ( $body, $postamble ) = extract_balanced($code);

    # trim out the { }
    $body = strip_outer_brackets($body);

    return ( $preamble, $pmcname, $flags, $parents, $body, $postamble, $chewed_lines );
}

our %has_value  = map { $_ => 1 } qw(does group hll);
our %has_values = map { $_ => 1 } qw(provides extends maps lib);

=head2 C<parse_flags()>

    my ($flags, $parents) = parse_flags($attributes, $pmcname);

B<Purpose:>  Extract a pmc signature from the code ref.

B<Argument:>  PMC file contents slurped by C<parse_pmc()>.

B<Return Values:>  List of two elements:

=over 4

=item *

a hash ref containing the flags associated with the pmc (such as
C<extends> and C<provides>).

=item *

the list of parents this pmc extends

=back

B<Comments:>  Called internally by C<parse_top_level()>.

=cut

sub parse_flags {
    my ( $data, $pmcname ) = @_;

    my ( $flags, @parents );

    my @words = $data =~ /(\w+)/g;

    while ( @words ) {
        my $name = shift @words;
        if ( $has_value{$name} || $has_values{$name} ) {
            my $value = shift @words;
            die "Parser error: no value for '$name'" unless $value;

            if ( $name eq 'extends' ) {
                push @parents, $value;
            }
            elsif ( $has_values{$name} ) {
                $flags->{$name}{$value} = 1;
            }
            else {
                $flags->{$name} = $value;
            }
        }
        else {
            $flags->{$name} = 1;
        }
    }

    # setup some defaults
    if ( $pmcname ne 'default' ) {
        push @parents, 'default' unless @parents;
        $flags->{provides}{scalar} = 1 unless $flags->{provides};
    }

    return ( $flags, \@parents );
}

=head2 C<extract_balanced()>

    ($pmcbody, $post) = extract_balanced($code);

B<Purpose:>  Remove a balanced C<{}> construct from the beginning of C<$code>.
Return it and the remaining code.

B<Argument:>  The code ref which was the first argument provided to
C<parse_pmc()>.

B<Return Values:>  List of two elements:

=over 4

=item *

String beginning with C<{> and ending with C<}>.  In between is found C code
where the comments hold strings of Perl comments written in POD.

=item *

String holding the balance of the code.  Same style as first element, but
without the braces.

=back

B<Comments:>  Called twice within C<parse_pmc()>.  Will die with error message
C<Badly balanced> if not balanced.

=cut

sub extract_balanced {
    my $code       = shift;
    my $unbalanced = 0;

    die 'Unexpected whitespace, expecting' if $code =~ /^\s+/;
    die 'bad block open: ', substr( $code, 0, 40 ), '...' unless $code =~ /^\{/;

    # create a copy and remove strings and comments so that
    # unbalanced {} can be used in them in PMCs, being careful to
    # preserve string length.
    local $_ = $code;
    s[
        ( ' (?: \\. | [^'] )* '     # remove ' strings
        | " (?: \\. | [^"] )* "     # remove " strings
        | /\* .*? \*/ )             # remove C comments
    ]
    [ "-" x length $1 ]sexg;

    while (/ (\{) | (\}) /gx) {
        if ($1) {
            $unbalanced++;
        }
        else {    # $2
            $unbalanced--;
            return ( substr( $code, 0, pos, "" ), $code ) if not $unbalanced;
        }
    }

    die "Badly balanced PMC source\n" if $unbalanced;
    return;
}

=head2 C<parse_method_attrs()>

    $attrs = parse_method_attrs($method_attributes);

B<Purpose:>  Parse a list of method attributes and return a hash ref of them.

B<Arguments:>  String captured from regular expression.

B<Return Values:>  Reference to hash of attribute values.

B<Comments:>  Called within C<parse_pmc()>.

=cut

sub parse_method_attrs {
    my $flags = shift;

    my %result;
    ++$result{$1} while $flags =~ /:(\w+)/g;

    return \%result;
}

1;

# Local Variables:
#   mode: cperl
#   cperl-indent-level: 4
#   fill-column: 100
# End:
# vim: expandtab shiftwidth=4:
