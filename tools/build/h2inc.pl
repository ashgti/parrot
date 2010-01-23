# Copyright (C) 2009, Parrot Foundation.
# $Id$

=head1 NAME

tools/build/h2inc.pl

=head1 DESCRIPTION

Generate C<.pasm> and C<.pm> files with constants based on C<.h> files.

=cut

use strict;
use warnings;

my $usage = "Usage: $0 <input_file> <output_file>\n";

my $in_file  = shift or die $usage;
my $out_file = shift or die $usage;
die $usage if @ARGV;

open my $in_fh, '<', $in_file or die "Can't open $in_file: $!\n";
my $directive = parse_file($in_file, $in_fh, $out_file);
close $in_fh;
die "invalid output file: '$out_file' for input '$in_file'" unless $directive;

my @defs = perform_directive($directive);
my $target  = $directive->{file};
my $gen;
if ($target =~ /\.pm$/) {
    $gen = join "\n", &const_to_perl(@defs);
    $gen .= "\n1;";
}
else {
    $gen = join "\n", &const_to_parrot(@defs);
}

open my $out_fh, '>', $out_file or die "Can't open $out_file: $!\n";
print $out_fh <<"EOF";
# DO NOT EDIT THIS FILE.
#
# This file is generated automatically from
# $in_file by $0
#
# Any changes made here will be lost.
#
$gen
EOF
close $out_fh;

=head1 SUBROUTINES

=head2 C<const_to_parrot()>

=over 4

=item * Arguments

    $gen = join "\n", const_to_parrot(@defs);

List.

=item * Return Value

String.

=back

=cut

sub const_to_parrot {

    my $keylen = (sort { $a <=> $b } map { length($_->[0]) } @_ )[-1] ;
    my $vallen = (sort { $a <=> $b } map { length($_->[1]) } @_ )[-1] ;

    map {sprintf ".macro_const %-${keylen}s %${vallen}s", $_->[0], $_->[1]} @_;
}

=head2 C<const_to_perl()>

=over 4

=item * Arguments

    $gen = join "\n", const_to_perl(@defs);

List.

=item * Return Value

String.

=back

=cut

sub const_to_perl {

    my $keylen = (sort { $a <=> $b } map { length($_->[0]) } @_ )[-1] ;

    map {sprintf "use constant %-${keylen}s => %s;", $_->[0], $_->[1]} @_;
}

=head2 C<transform_name()>

=over 4

=item * Arguments

    transform_name( sub { $prefix . $_[0] }, @_ );

List of two or more elements, the first element of which is a subroutine
reference.

=item * Return Value

List which is a mapping of the transformations executed by the first argument
upon the remaining arguments.

=back

=cut

sub transform_name {
    my $action = shift;

    return map { [ $action->( $_->[0] ), $_->[1] ] } @_;
}

=head2 C<prepend_prefix()>

=over 4

=item * Arguments

    @defs = prepend_prefix $d->{prefix}, @{ $d->{defs} };

List of two or more elements, the first element of which is a string.

=item * Return Value

List.

=back

=cut

sub prepend_prefix {
    my $prefix = shift;

    transform_name( sub { $prefix . $_[0] }, @_ );
}

=head2 C<perform_directive()>

=over 4

=item * Arguments

    @defs = perform_directive($directive);

Single hash reference (which is the return value from a successful run of
C<parse_file()>.

=item * Return Value

List.

=back

=cut

sub perform_directive {
    my ($d) = @_;

    my @defs = prepend_prefix $d->{prefix}, @{ $d->{defs} };
    if ( my $subst = $d->{subst} ) {
        @defs = transform_name( sub { local $_ = shift; eval $subst; $_ }, @defs );
    }
    @defs;
}

=head2 C<parse_file()>

=over 4

=item * Arguments

    $directive = parse_file($in_file, $in_fh, $out_file);

List of 3 elements: string holding name of incoming file; read handle to that
file; string holding name of outgoing file.

=item * Return Value

If successful, returns a hash reference.

=back

=cut

sub parse_file {
    my ( $in_file, $fh, $out_file) = @_;

    my ( @directives, %values, $last_val, $cur, $or_continues );
    while ( my $line = <$fh> ) {
        if (
            $line =~ m!
            &gen_from_(enum|def) \( ( [^)]* ) \)
            (?: \s+ prefix \( (\w+) \) )?
            (?: \s+ subst \( (s/.*?/.*?/[eig]?) \) )?
            !x
            )
        {
            $cur and die "Missing '&end_gen' in $in_file\n";
            my $file;
            foreach (split ' ', $2) {
                $file = $_ if $out_file =~ /$_$/;
            }
            $cur = {
                type   => $1,
                file   => $file,
                prefix => defined $3 ? $3 : '',
                defined $4 ? ( subst => $4 ) : (),
            };
            $last_val = -1;
        }
        elsif ( $line =~ /&end_gen\b/ ) {
            $cur or die "Missing &gen_from_(enum|def) in $in_file\n";
            return $cur if defined $cur->{file};
            $cur = undef;
        }

        $cur or next;

        if ( $cur->{type} eq 'def' && $line =~ /^\s*#define\s+(\w+)\s+(-?\w+|"[^"]*")/ ) {
            push @{ $cur->{defs} }, [ $1, $2 ];
        }
        elsif ( $cur->{type} eq 'enum' ) {
            # Special case: enum value is or'd combination of other values
            if ( $or_continues ) {
                $or_continues = 0;
                my $last_def = $cur->{defs}->[-1];
                my ($k, $v) = @{$last_def};
                my @or_values = grep {defined $_} $line =~ /^\s*(-?\w+)(?:\s*\|\s*(-?\w+))*/;
                for my $or (@or_values) {
                    if ( defined $values{$or} ) {
                        $v |= $values{$or};
                    }
                    elsif ( $or =~ /^0/ ) {
                        $v |= oct $or;
                    }
                }
                if ($line =~ /\|\s*$/) {
                    $or_continues = 1;
                }
                $values{$k} = $last_val = $v;
                $cur->{defs}->[-1]->[1] = $v;
            }
            elsif ( $line =~ /^\s*(\w+)\s*=\s*(-?\w+)\s*\|/ ) {
                my ( $k, $v ) = ( $1, $2 );
                my @or_values = ($v, $line =~ /\|\s*(-?\w+)/g);
                $v = 0;
                for my $or (@or_values) {
                    if ( defined $values{$or} ) {
                        $v |= $values{$or};
                    }
                    elsif ( $or =~ /^0/ ) {
                        $v |= oct $or;
                    }
                }
                if ($line =~ /\|\s*$/) {
                    $or_continues = 1;
                }
                $values{$k} = $last_val = $v;
                push @{ $cur->{defs} }, [ $k, $v ];
            }
            elsif ( $line =~ /^\s*(\w+)\s*=\s*(-?\w+)/ ) {
                my ( $k, $v ) = ( $1, $2 );
                if ( defined $values{$v} ) {
                    $v = $values{$v};
                }
                elsif ( $v =~ /^0/ ) {
                    $v = oct $v;
                }
                $values{$k} = $last_val = $v;
                push @{ $cur->{defs} }, [ $k, $v ];
            }
            elsif ( $line =~ m!^\s*(\w+)\s*(?:,\s*)?(?:/\*|$)! ) {
                my $k = $1;
                my $v = $values{$k} = ++$last_val;
                push @{ $cur->{defs} }, [ $k, $v ];
            }
        }
    }
    $cur and die "Missing '&end_gen' in $in_file\n";

    return;
}

1;

# Local Variables:
#   mode: cperl
#   cperl-indent-level: 4
#   fill-column: 100
# End:
# vim: expandtab shiftwidth=4:
