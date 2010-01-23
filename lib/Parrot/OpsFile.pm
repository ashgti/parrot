#! perl
# Copyright (C) 2001-2009, Parrot Foundation.
# $Id$

=head1 NAME

Parrot::OpsFile - Ops To C Code Generation

=head1 SYNOPSIS

  use Parrot::OpsFile;

=head1 DESCRIPTION

C<Parrot::OpsFile> takes one or more files of op functions and
creates real C code for them.

This class is used by F<tools/build/ops2c.pl>,
F<tools/build/ops2pm.pl> and F<tools/build/pbc2c.pl>.

=head2 Op Functions

For ops that have trivial bodies (such as just a call to some other
function and a C<return> statement), opcode functions are in the format:

    inline op opname (args) :flags {
        ... body of function ...
    }

Note that currently the C<inline> op type is ignored.

Alternately, for opcode functions that have more internal complexity the
format is:

    op opname (args) :flags {
        ... body of function ...
    }

There may be more than one C<return>.

In both cases the closing brace B<must> be on its own line.

When specifying multiple flags, each flag gets its own prefixing colon.

=head2 Op Arguments

Op arguments are a comma-separated list of direction and type pairs.

Argument direction is one of:

    in            the argument passes a value into the op
    out           the argument passes a value out of the op
    inout         the argument passes a value into and out of the op
    inconst       the argument passes a constant value into the op
    invar         the argument passes a variable value into the op

Argument direction is used to determine the life times of symbols and
their related register allocations. When an argument is passed into an
op a register is read from, when it's passed out of an op a register is
written to.

Argument type is one of:

    INT       the argument is an integer
    NUM       the argument is an numeric
    STR       the argument is an string
    PMC       the argument is an PMC
    KEY       the argument is an aggregate PMC key
    INTKEY    the argument is an aggregate PMC integer key
    LABEL     the argument is an integer branch offset or address

The size of the return offset is determined from the op function's
signature.

=head2 Op Flags

The flags are of two types:

=over 4

=item 1 class

The classification of ops is intended to facilitate the selection of
suitable ops for a Parrot safe mode.

=item 2 behavior

The presence (or absence) of certain flags will change how the op behaves. For
example, the lack of the C<flow> flag will cause the op to be implicitly
terminated with C<goto NEXT()>. (See next section).

The :deprecated flag will generate a diagnostic to standard error at
runtime when a deprecated opcode is invoked and
C<PARROT_WARNINGS_DEPRECATED_FLAG> has been set.

=back

=head2 Op Body (Macro Substitutions)

In the following macro descriptions, C<PC> and C<PC'> are the current
and next position within the Parrot code.

=over 4

=item C<goto OFFSET(X)>

Transforms to C<PC' = PC + X>. This is used for branches.

=item C<goto NEXT()>

Transforms to C<PC' = PC + S>, where C<S> is the size of an op.

=item C<goto ADDRESS(X)>

Transforms to C<PC' = X>. This is used for absolute jumps.

=item C<expr OFFSET(X)>

Transforms to C<PC + X>. This is used to give a relative address.

=item C<expr NEXT()>

Transforms to C<PC + S>, the position of the next op.

=item C<expr ADDRESS(X)>

Transforms to C<X>, an absolute address.

=item C<OP_SIZE>

Transforms to C<S>, the size of an op.

=item C<HALT()>

Transforms to C<PC' = 0>. Halts run loop, and resets the current
position to the start of the Parrot code, without resuming.

=item C<restart OFFSET(X)>

Transforms to C<PC' = 0> and restarts at C<PC + X>.

=item C<restart NEXT()>

Transforms to C<PC' = 0> and restarts at C<PC + S>.

=item C<$n>

Transforms to the op function's nth argument. C<$0> is the opcode itself.

=back

Note that, for ease of parsing, if the argument to one of the above
notations in a ops file contains parentheses, then double the enclosing
parentheses and add a space around the argument, like so:

    goto OFFSET(( (void*)interp->happy_place ))

=head2 Class Methods

=over 4

=cut

package Parrot::OpsFile;

use strict;
use warnings;

use base qw( Exporter );

use Parrot::Op;
use Parrot::Config;

our %op_body;
our @EXPORT = qw( %op_body );

# private sub  _trim()
#
# Trim leading and trailing spaces.

sub _trim {
    my $value = shift;

    $value =~ s/^\s+//;
    $value =~ s/\s+$//;

    return $value;
}

=item C<new(@files)>

Returns a new instance initialized by calling C<read_ops()> on each of
the specified op files.

=cut

sub new {
    my ( $class, $files, $nolines ) = @_;

    my $self = bless { PREAMBLE => '' }, $class;

    $self->read_ops( $_, $nolines ) for @{$files};

    # FILE holds a space separated list of opsfile name
    if ( $self->{FILE} ) {
        $self->{FILE} =~ s/, $//;
        $self->{FILE} =~ s/, $//;
    }

    return $self;
}

=back

=head2 Instance Methods

=over 4

=item C<read_ops($file,$nolines)>

Reads in the specified .ops file, gathering information about the ops.

=cut

sub read_ops {
    my ( $self, $file, $nolines ) = @_;

    my $ops_file = "src/" . $file;

    die "Parrot::OpFunc::init(): No file specified!\n" unless defined $file;

    $self->{FILE} .= $file . ', ';

    my $orig = $file;

    open my $OPS, '<', $file or die "Can't open $file, $!/$^E";

    $self->version( $PConfig{VERSION} );

    if ( !( $file =~ s/\.ops$/.c/ ) ) {
        $file .= ".c";
    }

    #
    # Read through the file, creating and storing Parrot::Op objects:
    #

    my $count = 0;
    my ( $name, $footer );
    my $type;
    my $body;
    my $short_name;
    my $args;
    my @args;
    my @argdirs;
    my $seen_pod;
    my $seen_op;
    my $in_preamble;
    my $line;
    my $flags;
    my @labels;

    while (<$OPS>) {
        $seen_pod    = 1 if m|^=|;
        $in_preamble = 1 if s|^BEGIN_OPS_PREAMBLE||;

        unless ( $seen_op or m|^(inline\s+)?op\s+| ) {

            if (m|^END_OPS_PREAMBLE|) {
                $_ = '';
                $in_preamble = 0;
            }
            elsif ($in_preamble) {
                $self->{PREAMBLE} .= $_;
            }

            next;
        }

        die "No 'VERSION = ...;' line found before beginning of ops in file '$orig'!\n"
            unless defined $self->version;

        #
        # Handle start-of-op:
        #
        # We create a new Parrot::Op instance based on the type, name and args.
        # We query the Parrot::Op for the op size, etc., which we use later.
        #
        # Either of these two forms work:
        #
        #   inline op  name (args) {
        #   op         name (args) {
        #
        # The args are a comma-separated list of items from this table of argument
        # types (even if no formal args are specified, there will be a single 'o'
        # entry):
        #
        #   op   The opcode
        #
        #   i    Integer register index
        #   n    Number register index
        #   p    PMC register index
        #   s    String register index
        #
        #   ic   Integer constant (in-line)
        #   nc   Number constant index
        #   pc   PMC constant index
        #   sc   String constant index
        #   kc   Key constant index
        #   kic  Integer Key constant index (in-line)
        #

        my $op_sig_RE = qr/
            ^
            (inline\s+)?  # optional keywords
            op
            \s+
            ([a-zA-Z]\w*) # op name
            \s*
            \((.*)\)      # argument signature
            \s*
            ((?: \:\w+\s*)*)         # :flags
            \s*
            {
            $
        /x;

        if ($_ =~ $op_sig_RE) {
            if ($seen_op) {
                die "$ops_file [$.]: Cannot define an op within an op definition!\n";
            }

            $type       = defined($1) ? 'inline' : 'function';
            $short_name = $2;
            $args       = _trim( lc $3 );
            $flags      = $4 ? _trim( lc $4 ) : "";
            @args       = split( /\s*,\s*/, $args );
            @argdirs    = ();
            @labels     = ();
            $body       = '';
            $seen_op    = 1;
            $line       = $. + 1;

            $flags = { map { $_ => undef } (split(/[ :]+/, $flags)) };

            my @temp;

            for my $arg (@args) {
                my ( $use, $type ) =
                    $arg =~ m/^(in|out|inout|inconst|invar)
                    \s+
                    (INT|NUM|STR|PMC|KEY|INTKEY|LABEL)$/ix;

                die "Unrecognized arg format '$arg' in '$_'!"
                    unless defined($use) and defined($type);

                # remember it's a label, then turn it to an int
                if ( $type =~ /^LABEL$/i ) {
                    $type = 'i';
                    push @labels, 1;
                }
                else {
                    push @labels, 0;
                }

                if ( $type =~ /^INTKEY$/i ) {
                    $type = 'ki';
                }
                else {
                    $type = lc substr( $type, 0, 1 );
                }

                if ( $use eq 'in' ) {
                    push @temp,    "$type|${type}c";
                    push @argdirs, 'i';
                }
                elsif ( $use eq 'invar' ) {
                    push @temp,    $type;
                    push @argdirs, 'i';
                }
                elsif ( $use eq 'inconst' ) {
                    push @temp,    "${type}c";
                    push @argdirs, 'i';
                }
                elsif ( $use eq 'inout' ) {
                    push @temp,    $type;
                    push @argdirs, 'io';
                }
                else {
                    push @temp,    $type;
                    push @argdirs, 'o';
                }
            }

            @args = @temp;

            next;
        }

        #
        # Handle end-of-op:
        #
        # We stash the accumulated body of source code in the Parrot::Op, push the
        # Parrot::Op onto our op array, and forget the op so we can start the next
        # one.
        #

        if (/^}\s*$/) {
            $count += $self->make_op(
                $count, $type, $short_name, $body,  \@args, \@argdirs,
                $line,  $orig, \@labels,    $flags, $nolines
            );

            $seen_op = 0;

            next;
        }

        #
        # Accumulate the code into the op's body:
        #

        if ($seen_op) {
            $body .= $_;
        }
        else {
            die "Parrot::OpsFile: Unrecognized line: '$_'!\n";
        }
    }

    if ($seen_op) {
        die "Parrot::OpsFile: File ended with incomplete op definition!\n";
    }

    close $OPS or die "Could not close ops file '$file' ($!)!";

    return;
}

# Extends a string containing an or expression "0" .. "A" .. "A|B" etc.
sub or_flag {
    my ( $flag, $value ) = @_;

    if ( $$flag eq '0' ) {
        $$flag = $value;
    }
    else {
        $$flag .= "|$value";
    }
}

=item C<make_op($code,
$type, $short_name, $body, $args, $argdirs, $line, $file, $labels, $flags, $nolines)>

Returns a new C<Parrot::Op> instance for the specified arguments.

=cut

sub make_op {
    my (
        $self,    $code, $type, $short_name, $body,  $args,
        $argdirs, $line, $file, $labels,     $flags, $nolines
    ) = @_;
    my $counter  = 0;
    my $absolute = 0;
    my $branch   = 0;
    my $pop      = 0;
    my $next     = 0;
    my $restart  = 0;

    if (exists($$flags{deprecated})) {
        $body = <<"END_CODE" . $body;
INTVAL unused = PARROT_WARNINGS_test(interp,PARROT_WARNINGS_DEPRECATED_FLAG) &&
  fprintf(stderr,"Warning: instruction '$short_name' is deprecated\\n");
END_CODE
}
    unless (exists($$flags{flow})) {
        $body .= "\ngoto NEXT();";
    }

    foreach my $variant ( expand_args(@$args) ) {
        my (@fixedargs) = split( /,/, $variant );
        my $op =
            Parrot::Op->new( $code++, $type, $short_name, [@fixedargs], [@$argdirs], [@$labels],
            $flags );
        my $op_size = $op->size;
        my $jumps   = "0";

        #
        # Macro substitutions:
        #
        # We convert the following notations:
        #
        #   .ops file          Op body  Meaning       Comment
        #   -----------------  -------  ------------  ----------------------------------
        #   goto OFFSET(X)     {{+=X}}  PC' = PC + X  Used for branches
        #   goto NEXT()        {{+=S}}  PC' = PC + S  Where S is op size
        #   goto ADDRESS(X)    {{=X}}   PC' = X       Used for absolute jumps
        #   expr OFFSET(X)     {{^+X}}  PC + X        Relative address
        #   expr NEXT()        {{^+S}}  PC + S        Where S is op size
        #   expr ADDRESS(X)    {{^X}}   X             Absolute address
        #   OP_SIZE            {{^S}}   S             op size
        #
        #   HALT()             {{=0}}   PC' = 0       Halts run_ops loop, no resume
        #
        #   restart OFFSET(X)  {{=0,+=X}}   PC' = 0   Restarts at PC + X
        #   restart NEXT()     {{=0,+=S}}   PC' = 0   Restarts at PC + S
        #
        #   $X                 {{@X}}   Argument X    $0 is opcode, $1 is first arg
        #
        # For ease of parsing, if the argument to one of the above
        # notations in a .ops file contains parentheses, then double the
        # enclosing parentheses and add a space around the argument,
        # like so:
        #
        #    goto OFFSET(( (void*)interp->happy_place ))
        #
        # Later transformations turn the Op body notations into C code, based
        # on the mode of operation (function calls, switch statements, gotos
        # with labels, etc.).
        #

        $absolute ||= $body =~ s/\bgoto\s+ADDRESS\(\( (.*?) \)\)/{{=$1}}/mg;
                      $body =~ s/\bexpr\s+ADDRESS\(\( (.*?) \)\)/{{^$1}}/mg;
        $absolute ||= $body =~ s/\bgoto\s+ADDRESS\((.*?)\)/{{=$1}}/mg;
                      $body =~ s/\bexpr\s+ADDRESS\((.*?)\)/{{^$1}}/mg;

        $branch   ||= $short_name =~ /runinterp/;
        $branch   ||= $body =~ s/\bgoto\s+OFFSET\(\( (.*?) \)\)/{{+=$1}}/mg;
                      $body =~ s/\bexpr\s+OFFSET\(\( (.*?) \)\)/{{^+$1}}/mg;
        $branch   ||= $body =~ s/\bgoto\s+OFFSET\((.*?)\)/{{+=$1}}/mg;
                      $body =~ s/\bexpr\s+OFFSET\((.*?)\)/{{^+$1}}/mg;

        $next     ||= $short_name =~ /runinterp/;
        $next     ||= $body =~ s/\bexpr\s+NEXT\(\)/{{^+$op_size}}/mg;
                      $body =~ s/\bgoto\s+NEXT\(\)/{{+=$op_size}}/mg;

        $body =~ s/\bHALT\(\)/{{=0}}/mg;
        $body =~ s/\bOP_SIZE\b/{{^$op_size}}/mg;

        if ( $body =~ s/\brestart\s+OFFSET\((.*?)\)/{{=0,+=$1}}/mg ) {
            $branch  = 1;
            $restart = 1;
        }
        elsif ( $body =~ s/\brestart\s+NEXT\(\)/{{=0,+=$op_size}}/mg ) {
            $restart = 1;
            $next    = 1;
        }
        elsif ( $body =~ s/\brestart\s+ADDRESS\((.*?)\)/{{=$1}}/mg ) {
            $next    = 0;
            $restart = 1;
        }

        $body =~ s/\$(\d+)/{{\@$1}}/mg;

        # We can only reference as many parameters as we declare
        my $max_arg_num = @$args;
        my @found_args = ($body =~ m/{{@(\d+)}}/g);
        foreach my $arg (@found_args) {
            die "opcode '$short_name' uses '\$$arg' but only has $max_arg_num parameters.\n" if $arg > $max_arg_num;
        }


        my $file_escaped = $file;
        $file_escaped =~ s|(\\)|$1$1|g;    # escape backslashes
        $op->body( $nolines ? $body : qq{#line $line "$file_escaped"\n$body} );

        # Constants here are defined in include/parrot/op.h
        or_flag( \$jumps, "PARROT_JUMP_ADDRESS"  ) if $absolute;
        or_flag( \$jumps, "PARROT_JUMP_RELATIVE" ) if $branch;
        or_flag( \$jumps, "PARROT_JUMP_ENEXT"    ) if $next;
        or_flag( \$jumps, "PARROT_JUMP_RESTART"  ) if $restart;

        # I'm assuming the op branches to the value in the last argument.
        if ( ($jumps)
            && ( $fixedargs[ @fixedargs - 1 ] )
            && ( $fixedargs[ @fixedargs - 1 ] eq 'i' ) ) {
            or_flag( \$jumps, "PARROT_JUMP_GNEXT" );
        }

        $op->jump($jumps);
        $self->push_op($op);
        $counter++;
    }

    return $counter;
}

=item C<expand_args(@args)>

Given an argument list, returns a list of all the possible argument
combinations.

=cut

sub expand_args {
    my (@args) = @_;

    return "" if ( !scalar(@args) );

    my $arg = shift(@args);
    my @var = split( /\|/, $arg );

    if ( !scalar(@args) ) {
        return @var;
    }
    else {
        my @list = expand_args(@args);
        my @results;

        foreach my $l (@list) {
            foreach my $v (@var) {
                push( @results, "$v,$l" );
            }
        }

        return @results;
    }
}

=item C<ops()>

Returns the C<Parrot::Op> instances found in the file(s).

=cut

sub ops {
    my ($self) = @_;

    return @{ $self->{OPS} };
}

=item C<op($index)>

Returns the op at C<$index>.

=cut

sub op {
    my ( $self, $index ) = @_;

    return $self->{OPS}[$index];
}

=item C<preamble()>

=item C<preamble($trans)>

Returns any lines found prior to first op definition.

If C<$trans> (an C<Parrot::OpTrans> subclass) is supplied then
substitutions are made.

=cut

sub preamble {
    my ( $self, $trans ) = @_;

    local $_ = $self->{PREAMBLE};

    if ($trans) {
        s/goto\s+OFFSET\((.*)\)/{{+=$1}}/mg;

        #s/goto\s+NEXT\(\)/{{+=$op_size}}/mg;   #not supported--dependent on op size
        s/goto\s+ADDRESS\((.*)\)/{{=$1}}/mg;
        s/HALT\(\)/{{=0}}/mg;

        $_ = Parrot::Op->rewrite_body( $_, $trans, 'preamble' );
    }

    return $_;
}

=item C<version($major, $minor, $patch)>

=item C<version($version)>

=item C<version()>

Sets/gets the version number.

=cut

sub version {
    my $self = shift;

    if ( @_ == 1 ) {
        $self->{VERSION} = shift;
    }
    elsif ( @_ == 3 ) {
        $self->{VERSION} = join( '.', @_ );
    }
    elsif ( @_ == 0 ) {
        if (wantarray) {
            return split( /\./, $self->{VERSION} );
        }
        else {
            return $self->{VERSION};
        }
    }
    else {
        die "Parrot::OpsFile::version(): Illegal argument count" . scalar(@_) . "!";
    }
}

=item C<major_version()>

Returns the major version number.

=cut

sub major_version {
    my $self = shift;

    $self->{VERSION} =~ m/^(\d+)\./;

    return $1;
}

=item C<minor_version()>

Returns the minor version number.

=cut

sub minor_version {
    my $self = shift;

    $self->{VERSION} =~ m/^\d+\.(\d+)\./;

    return $1;
}

=item C<patch_version()>

Returns the patch version number.

=cut

sub patch_version {
    my $self = shift;

    $self->{VERSION} =~ m/^\d+\.\d+\.(\d+)/;

    return $1;
}

=item C<push_op($op)>

Adds C<$op> to the end of the op list.

=cut

sub push_op {
    my ( $self, $op ) = @_;

    push @{ $self->{OPS} }, $op;
}

=back

=head1 SEE ALSO

=over 4

=item C<Parrot::Op>

=item C<Parrot::OpTrans>

=item F<tools/build/ops2c.pl>

=item F<tools/build/ops2pm.pl>

=item F<tools/build/pbc2c.pl>

=back

=cut

1;

# Local Variables:
#   mode: cperl
#   cperl-indent-level: 4
#   fill-column: 100
# End:
# vim: expandtab shiftwidth=4:
