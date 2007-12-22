#! perl
# Copyright (C) 2001-2007, The Perl Foundation.
# $Id$

use strict;
use warnings;
use Carp qw( confess );

# While I work on files individually to make sure headerizing and
# rewriting doesn't stomp on them.
my @experimental_files = qw(
    builtin.c
    byteorder.c
    charset.c
    datatypes.c
    exceptions.c
    gc/resources.c
    hash.c
    hll.c
    objects.c
    stacks.c
    string.c
    string_primitives.c
    sub.c
    tsq.c
    utils.c
);
my %experimental_files = map { ("src/$_", 1) } @experimental_files;

=head1 NAME

tools/build/headerizer.pl - Generates the function header parts of .h
files from .c files

=head1 SYNOPSIS

Update the headers in F<include/parrot> with the function declarations in
the F<*.pmc> or F<*.c> files that correspond to the F<*.o> files passed
on the command line.

    % perl tools/build/headerizer.pl OBJFILES

=head1 DESCRIPTION

=head1 TODO

* Tell if there are funcs without docs

* Generate docs from funcs

* Test the POD of the stuff we're parsing.

* Somehow handle static functions in the source file

=head1 NOTES

* the .c files MUST have a /* HEADERIZER HFILE: foo/bar.h */ directive in them

* Support for multiple .c files pointing at the same .h file

* Does NOT remove all blocks in the .h file, so if a .c file
disappears, it's block is "orphaned" and will remain there.

=head1 COMMAND-LINE OPTIONS

=over 4

=item C<--verbose>

Verbose status along the way.

=back

=head1 COMMAND-LINE ARGUMENTS

=over 4

=item C<OBJFILES>

One or more object file names.

=back

=cut

use Getopt::Long;
use lib qw( lib );
use Parrot::Config;

my %warnings;
my %opt;

my %valid_macros = map { ( $_, 1 ) } qw(
    PARROT_API
    PARROT_INLINE
    PARROT_CAN_RETURN_NULL
    PARROT_CANNOT_RETURN_NULL
    PARROT_IGNORABLE_RESULT
    PARROT_WARN_UNUSED_RESULT
    PARROT_PURE_FUNCTION
    PARROT_CONST_FUNCTION
    PARROT_DOES_NOT_RETURN
    PARROT_MALLOC
);

main();

=head1 FUNCTIONS

=head2 extract_function_declarations( $source_file_text )

Rips apart a C file to get the function declarations.

=cut

sub extract_function_declarations {
    my $text = shift;

    # Drop all text after HEADERIZER STOP
    $text =~ s[/\*\s*HEADERIZER STOP.+][]s;

    # Strip blocks of comments
    $text =~ s[^/\*.*?\*/][]mxsg;

    # Strip # compiler directives (Thanks, Audrey!)
    $text =~ s[^#(\\\n|.)*][]mg;

    # Strip code blocks
    $text =~ s[^{.+?^}][]msg;

    # Split on paragraphs
    my @funcs = split /\n{2,}/, $text;

    # If it doesn't start in the left column, it's not a func
    @funcs = grep /^\S/, @funcs;

    # Typedefs, enums and externs are no good
    @funcs = grep { !/^(typedef|enum|extern)\b/ } @funcs;

    # Structs are OK if they're not alone on the line
    @funcs = grep { !/^struct.+;\n/ } @funcs;

    # Structs are OK if they're not being defined
    @funcs = grep { !/^(static\s+)?struct.+{\n/ } @funcs;

    # Ignore magic function name YY_DECL
    @funcs = grep { !/YY_DECL/ } @funcs;

    # Ignore anything with magic words HEADERIZER SKIP
    @funcs = grep !m{/\*\s*HEADERIZER SKIP\s*\*/}, @funcs;

    # Variables are of no use to us
    @funcs = grep !/=/, @funcs;

    # Get rid of any blocks at the end
    s/\s*{.*//s for @funcs;

    # Toast anything non-whitespace
    @funcs = grep /\S/, @funcs;

    # If it's got a semicolon, it's not a function header
    @funcs = grep !/;/, @funcs;

    chomp @funcs;

    return @funcs;
}

=head2 extract_function_declaration_and_update_source( $cfile_name )

Extract all the function declarations from the C file specified by
I<$cfile_name>, and update the comment blocks within.

=cut

sub extract_function_declarations_and_update_source {
    my $cfile_name = shift;

    open( my $fhin, '<', $cfile_name ) or die "Can't open $cfile_name: $!";
    my $text = join( '', <$fhin> );
    close $fhin;

    my @func_declarations = extract_function_declarations( $text );

    for my $decl ( @func_declarations ) {
        my $specs = function_components_from_declaration( $cfile_name, $decl );
        my $name = $specs->{name};
        my $return_type = $specs->{return_type};
        my $heading = "$return_type $name";
        $heading = "static $heading" if $specs->{is_static};

        $text =~ s/=item C<[^>]*\b$name\b[^>]*>\n/=item C<$heading>\n/sm or
            warn "Couldn't replace $name\n";
    }
    open( my $fhout, '>', $cfile_name ) or die "Can't create $cfile_name: $!";
    print {$fhout} $text;
    close $fhout;

    return @func_declarations;
}

sub function_components_from_declaration {
    my $file  = shift;
    my $proto = shift;

    my @lines = split( /\n/, $proto );
    chomp @lines;

    my @macros;
    my $parrot_api;
    my $parrot_inline;

    while ( @lines && ( $lines[0] =~ /^PARROT_/ ) ) {
        my $macro = shift @lines;
        if ( $macro eq 'PARROT_API' ) {
            $parrot_api = 1;
        }
        elsif ( $macro eq 'PARROT_INLINE' ) {
            $parrot_inline = 1;
        }
        push( @macros, $macro );
    }

    my $return_type = shift @lines;
    my $args = join( ' ', @lines );

    $args =~ s/\s+/ /g;
    $args =~ s{([^(]+)\s*\((.+)\);?}{$2}
        or die qq{Couldn't handle "$proto"};

    my $name = $1;
    $args = $2;

    die "Can't have both PARROT_API and PARROT_INLINE on $name\n" if $parrot_inline && $parrot_api;

    my @args = split( /\s*,\s*/, $args );
    for (@args) {
        /\S+\s+\S+/
            || ( $_ eq '...' )
            || ( $_ eq 'void' )
            || ( $_ =~ /(PARROT|NULLOK|SHIM)_INTERP/ )
            or die "Bad args in $proto";
    }

    my $is_static = 0;
    $is_static = $2 if $return_type =~ s/^((static)\s+)?//i;

    die "$file $name: Impossible to have both static and PARROT_API" if $parrot_api && $is_static;

    my %macros;
    for my $macro (@macros) {
        $macros{$macro} = 1;
        if ( not $valid_macros{$macro} ) {
            squawk( $file, $name, "Invalid macro $macro" );
        }
    }
    if ( $return_type =~ /\*/ ) {
        if ( !$macros{PARROT_CAN_RETURN_NULL} && !$macros{PARROT_CANNOT_RETURN_NULL} ) {
            squawk( $file, $name,
                "Returns a pointer, but no PARROT_CAN(NOT)_RETURN_NULL macro found." );
        }
        elsif ( $macros{PARROT_CAN_RETURN_NULL} && $macros{PARROT_CANNOT_RETURN_NULL} ) {
            squawk( $file, $name,
                "Can't have both PARROT_CAN_RETURN_NULL and PARROT_CANNOT_RETURN_NULL together." );
        }
    }

    return {
        file        => $file,
        name        => $name,
        args        => \@args,
        macros      => \@macros,
        is_static   => $is_static,
        is_inline   => $parrot_inline,
        is_api      => $parrot_api,
        return_type => $return_type,
    };
}

sub attrs_from_args {
    my $func = shift;
    my @args = @_;

    my @attrs = ();

    my $n = 0;
    for my $arg (@args) {
        ++$n;
        if ( $arg =~ m{(ARGIN|ARGOUT|ARGINOUT|NOTNULL)\(} || $arg eq 'PARROT_INTERP' ) {
            push( @attrs, "__attribute__nonnull__($n)" );
        }
        if ( ( $arg =~ m{\*} ) && ( $arg !~ /\b(SHIM|((ARGIN|ARGOUT|ARGINOUT)(_NULLOK)?))/ ) ) {
            my $name = $func->{name};
            my $file = $func->{file};
            squawk( $file, $name, qq{"$arg" isn't protected with an ARGIN, ARGOUT or ARGINOUT (or a _NULLOK variant)} );
        }
    }

    return @attrs;
}

sub make_function_decls {
    my @funcs = @_;

    my @decls;
    foreach my $func (@funcs) {
        my $multiline = 0;

        my $decl = sprintf( "%s %s(", $func->{return_type}, $func->{name} );
        $decl = "static $decl" if $func->{is_static};

        my @args = @{ $func->{args} };
        my @attrs = attrs_from_args( $func, @args );

        for my $arg (@args) {
            if ( $arg =~ m{SHIM\((.+)\)} ) {
                $arg = $1;
                if ( $func->{is_static} || ( $arg =~ /\*/ ) ) {
                    $arg = "SHIM($arg)";
                }
                else {
                    $arg = "NULLOK($arg)";
                }
            }
        }

        my $argline = join( ", ", @args );
        if ( length( $decl . $argline ) <= 75 ) {
            $decl = "$decl$argline)";
        }
        else {
            if ( $args[0] =~ /^((SHIM|PARROT)_INTERP|Interp)\b/ ) {
                $decl .= ( shift @args );
                $decl .= "," if @args;
            }
            $argline   = join( ",", map { "\n\t$_" } @args );
            $decl      = "$decl$argline)";
            $multiline = 1;
        }

        my $attrs = join( "", map { "\n\t\t$_" } @attrs );
        if ($attrs) {
            $decl .= $attrs;
            $multiline = 1;
        }
        my @macros = @{ $func->{macros} };
        $multiline = 1 if @macros;

        $decl .= $multiline ? ";\n" : ";";
        $decl = join( "\n", @macros, $decl );
        $decl =~ s/\t/    /g;
        push( @decls, $decl );
    }

    return @decls;
}

sub squawk {
    my $file  = shift;
    my $func  = shift;
    my $error = shift;

    push( @{ $warnings{$file}->{$func} }, $error );
}

sub read_file {
    my $filename = shift;

    open my $fh, '<', $filename or die "couldn't read '$filename': $!";
    my $text = do { local $/ = undef; <$fh> };
    close $fh;

    return $text;
}

sub write_file {
    my $filename = shift;
    my $text     = shift;

    open my $fh, '>', $filename or die "couldn't write '$filename': $!";
    print {$fh} $text;
    close $fh;
}

sub replace_headerized_declarations {
    my $source_code = shift;
    my $cfile       = shift;
    my $hfile       = shift;
    my @funcs       = @_;

    # Allow a way to not headerize statics
    if ( $source_code =~ m{/\*\s*HEADERIZER NONE:\s*$cfile\s*\*/} ) {
        return $source_code;
    }

    @funcs = sort api_first_then_alpha @funcs;

    my @function_decls = make_function_decls(@funcs);

    my $function_decls = join( "\n", @function_decls );
    my $STARTMARKER    = qr#/\* HEADERIZER BEGIN: $cfile \*/\n#;
    my $ENDMARKER      = qr#/\* HEADERIZER END: $cfile \*/\n?#;
    $source_code =~ s#($STARTMARKER)(?:.*?)($ENDMARKER)#$1\n$function_decls\n$2#s
        or die "Need begin/end HEADERIZER markers for $cfile in $hfile\n";

    return $source_code;
}

sub api_first_then_alpha {
    return ( ( $b->{is_api} || 0 ) <=> ( $a->{is_api} || 0 ) )
        || ( lc $a->{name} cmp lc $b->{name} );
}

sub main {
    GetOptions( 'verbose' => \$opt{verbose}, ) or exit(1);

    my %ofiles;
    ++$ofiles{$_} for @ARGV;
    my @ofiles = sort keys %ofiles;
    for (@ofiles) {
        print "$_ is specified more than once.\n" if $ofiles{$_} > 1;
    }
    my %cfiles;
    my %cfiles_with_statics;

    # Walk the object files and find corresponding source (either .c or .pmc)
    for my $ofile (@ofiles) {
        next if $ofile =~ m/^\Qsrc$PConfig{slash}ops\E/;

        $ofile =~ s/\\/\//g;

        my $cfile = $ofile;
        $cfile =~ s/\Q$PConfig{o}\E$/.c/ or die "$cfile doesn't look like an object file";

        my $pmcfile = $ofile;
        $pmcfile =~ s/\Q$PConfig{o}\E$/.pmc/;

        my $csource = read_file($cfile);
        die "can't find HEADERIZER HFILE directive in '$cfile'"
            unless $csource =~ m{ /\* \s+ HEADERIZER\ HFILE: \s+ ([^*]+?) \s+ \*/ }sx;
        my $hfile = $1;
        if ( ( $hfile ne 'none' ) && ( not -f $hfile ) ) {
            die "'$hfile' not found (referenced from '$cfile')";
        }

        my @decls;
        if ( -f $pmcfile || !$experimental_files{$cfile} ) {
            @decls = extract_function_declarations( $csource );
        }
        else {
            @decls = extract_function_declarations_and_update_source( $cfile );
        }

        for my $decl (@decls) {
            my $components = function_components_from_declaration( $cfile, $decl );
            push( @{ $cfiles{$hfile}->{$cfile} }, $components ) unless $hfile eq 'none';
            push( @{ $cfiles_with_statics{$cfile} }, $components ) if $components->{is_static};
        }
    }    # for @cfiles

    # Update all the .h files
    for my $hfile ( sort keys %cfiles ) {
        my $cfiles = $cfiles{$hfile};

        my $header = read_file($hfile);

        for my $cfile ( sort keys %{$cfiles} ) {
            my @funcs = @{ $cfiles->{$cfile} };
            @funcs = grep { not $_->{is_static} } @funcs;    # skip statics
            $header = replace_headerized_declarations( $header, $cfile, $hfile, @funcs );
        }

        write_file( $hfile, $header );
    }

    # Update all the .c files in place
    for my $cfile ( sort keys %cfiles_with_statics ) {
        my @funcs = @{ $cfiles_with_statics{$cfile} };
        @funcs = grep { $_->{is_static} } @funcs;

        my $source = read_file($cfile);
        $source = replace_headerized_declarations( $source, 'static', $cfile, @funcs );

        write_file( $cfile, $source );
    }

    print "Headerization complete.\n";
    if ( keys %warnings ) {
        my $nwarnings     = 0;
        my $nwarningfuncs = 0;
        my $nwarningfiles = 0;
        for my $file ( sort keys %warnings ) {
            ++$nwarningfiles;
            print "$file\n";
            my $funcs = $warnings{$file};
            for my $func ( sort keys %{$funcs} ) {
                ++$nwarningfuncs;
                for my $error ( @{ $funcs->{$func} } ) {
                    print "    $func: $error\n";
                    ++$nwarnings;
                }
            }
        }

        print "$nwarnings warnings in $nwarningfuncs funcs in $nwarningfiles C files\n";
    }

    return;
}

=head1 NAME

headerizer.pl

=head1 SYNOPSIS

  $ tools/build/headerizer.pl [object files]

Generates C function declarations based on the function definitions in
the C source code.

=head1 DIRECTIVES

The headerizer works off of directives in the source and header files.

One source file's public declarations can only go into one header file.
However, one header file can have declarations from multiple source files.
In other words, headers-to-source is one-to-many.

=over 4

=item HEADERIZER BEGIN: F<source-filename> / HEADERIZER END: F<source-filename>

Marks the beginning and end of a block of declarations in a header file.

    # In file foo.h
    /* HEADERIZER BEGIN: src/foo.c */
    /* HEADERIZER END: src/foo.c */

    /* HEADERIZER BEGIN: src/bar.c */
    /* HEADERIZER END: src/bar.c */

=item HEADERIZER HFILE: F<header-filename>

Tells the headerizer where the declarations for the functions should go

    # In file foo.c
    /* HEADERIZER HFILE: foo.h */

    # In file bar.c
    /* HEADERIZER HFILE: foo.h */

=cut

# Local Variables:
#   mode: cperl
#   cperl-indent-level: 4
#   fill-column: 100
# End:
# vim: expandtab shiftwidth=4:
