# Copyright (C) 2001-2008, The Perl Foundation.
# $Id$

=head1 NAME

config/auto/pmc.pm - PMC Files

=head1 DESCRIPTION

Asks the user to select which PMC files to include.

=cut

package auto::pmc;

use strict;
use warnings;

use base qw(Parrot::Configure::Step);

use File::Basename qw/basename/;
use File::Spec::Functions qw/catfile/;

use Parrot::Configure::Utils ':auto';

sub _init {
    my $self = shift;
    return {
        description => 'Which pmc files should be compiled in',
        result      => '',
        PMC_PARENTS => {},
        srcpmc      => [ sort map { basename($_) } glob "./src/pmc/*.pmc" ],
    };
}

# Return the (lowercased) name of the immediate parent of the given
# (lowercased) pmc name.
sub pmc_parent {
    my ($self, $pmc) = @_;

    return $self->{PMC_PARENTS}{$pmc} if defined $self->{PMC_PARENTS}{$pmc};

    local $/;
    open( my $PMC, '<', "src/pmc/$pmc.pmc" )
        or die "open src/pmc/$pmc.pmc failed: $!";
    local $_ = <$PMC>;
    close $PMC;

    # Throw out everything but the pmclass declaration
    s/^.*?pmclass//s;
    s/\{.*$//s;

    return $self->{PMC_PARENTS}{$pmc} = lc($1) if m/extends\s+(\w+)/;
    return $self->{PMC_PARENTS}{$pmc} = 'default';
}

# Return an array of all
sub pmc_parents {
    my ($self, $pmc) = @_;

    my @parents = ($pmc);
    push @parents, $self->pmc_parent( $parents[-1] )
        until $parents[-1] eq 'default';

    shift @parents;
    return @parents;
}

sub get_pmc_order {
    open my $IN, '<', 'src/pmc/pmc.num' or die "Can't read src/pmc/pmc.num";
    my %order;
    while (<$IN>) {
        next if /^#/;

        if (/(\w+\.\w+)\s+(\d+)/) {
            $order{$1} = $2;
        }
    }

    close $IN;

    return \%order;
}

sub sort_pmcs {
    my @pmcs      = @_;
    my $pmc_order = get_pmc_order();
    my $n         = keys %$pmc_order;
    my @sorted_pmcs;

    for my $pmc (@pmcs) {
        if ( exists $pmc_order->{$pmc} ) {
            $sorted_pmcs[ $pmc_order->{$pmc} ] = $pmc;
        }
        else {
            $sorted_pmcs[ $n++ ] = $pmc;
        }
    }

    return @sorted_pmcs;
}

sub contains_pccmethod {
    my $file = shift;
    open( my $fh, '<', $file ) or die "Can't read '$file': $!\n";

    local $_;
    while (<$fh>) {
        next unless /METHOD/;
        return 1;
    }

    return;
}

# Given a PMC file name, get a list of all the includes it specifies
sub get_includes {
    my $file = shift;
    open( my $fh, '<', $file ) or die "Can't read '$file': $!\n";

    my @retval;
    local $_;
    while (<$fh>) {
        next unless /^\s*#include\s+["<](.*)[">]\s+$/;
        my $include = $1;
        if ($include =~ m{^(\.|parrot/)}) { # main parrot include dir
          $include = "include/" . $include;
        } elsif ($include =~ m/^pmc_|\.str$/) { # local pmc header
          $include = "src/pmc/" . $include;
        } # else it's probably a system header, don't depend on it.
        push @retval, $include;
    }

    return join(' ', @retval);
}


sub runstep {
    my ( $self, $conf ) = @_;

    my @pmc = sort_pmcs( @{ $self->{srcpmc} } );

    my $pmc_list = $conf->options->get('pmc')
        || join( ' ', grep { defined $_ } @pmc );

    # names of class files for src/pmc/Makefile
    ( my $TEMP_pmc_o   = $pmc_list ) =~ s/\.pmc/\$(O)/g;
    ( my $TEMP_pmc_str = $pmc_list ) =~ s/\.pmc/\.str/g;

    # calls to pmc2c.pl for src/pmc/Makefile
    my $TEMP_pmc_build = <<"E_NOTE";

# the following part of the Makefile was built by 'config/auto/pmc.pm'

E_NOTE

    $TEMP_pmc_build .= <<END;
PMC2C_FILES = \\
    lib/Parrot/Pmc2c/Pmc2cMain.pm \\
    lib/Parrot/Pmc2c/Parser.pm \\
    lib/Parrot/Pmc2c/Dumper.pm \\
    lib/Parrot/Pmc2c/PMC.pm \\
    lib/Parrot/Pmc2c/Method.pm \\
    lib/Parrot/Pmc2c/PCCMETHOD.pm \\
    lib/Parrot/Pmc2c/PMCEmitter.pm \\
    lib/Parrot/Pmc2c/MethodEmitter.pm \\
    lib/Parrot/Pmc2c/Library.pm \\
    lib/Parrot/Pmc2c/UtilFunctions.pm \\
    lib/Parrot/Pmc2c/PMC/default.pm \\
    lib/Parrot/Pmc2c/PMC/delegate.pm \\
    lib/Parrot/Pmc2c/PMC/deleg_pmc.pm \\
    lib/Parrot/Pmc2c/PMC/Null.pm \\
    lib/Parrot/Pmc2c/PMC/Ref.pm \\
    lib/Parrot/Pmc2c/PMC/SharedRef.pm \\
    lib/Parrot/Pmc2c/PMC/STMRef.pm \\
    lib/Parrot/Pmc2c/PMC/RO.pm
END

    for my $pmc ( split( /\s+/, $pmc_list ) ) {
        $pmc =~ s/\.pmc$//;
        next if $pmc =~ /^const/;

        # make each pmc depend upon its parent.
        my $parent_dumps = '';
        $parent_dumps .= "src/pmc/$_.dump "
            foreach reverse( ( $self->pmc_parents($pmc) ) );
        my $parent_headers = '';
        $parent_headers .= "src/pmc/pmc_$_.h "
            for $self->pmc_parents($pmc);

        # add dependencies that result from METHOD usage.
        my $pmc_fname = catfile('src', 'pmc', "$pmc.pmc");
        my $pccmethod_depend = '';
        if (contains_pccmethod($pmc_fname)) {
            $pccmethod_depend = 'lib/Parrot/Pmc2c/PCCMETHOD.pm';
            if ($pmc ne 'fixedintegerarray') {
                $pccmethod_depend .= ' src/pmc/pmc_fixedintegerarray.h';
            }
        }
        my $include_headers = get_includes($pmc_fname);

        $TEMP_pmc_build .= <<END
src/pmc/$pmc.c : src/pmc/$pmc.dump
\t\$(PMC2CC) src/pmc/$pmc.pmc

src/pmc/$pmc.dump : vtable.dump $parent_dumps src/pmc/$pmc.pmc \$(PMC2C_FILES) $pccmethod_depend
\t\$(PMC2CD) src/pmc/$pmc.pmc

src/pmc/pmc_$pmc.h: src/pmc/$pmc.c

src/pmc/$pmc\$(O): src/pmc/$pmc.str \$(NONGEN_HEADERS) \\
    $parent_headers $include_headers

END
    }

    # src/pmc/$pmc\$(O): \$(NONGEN_HEADERS) $parent_headers src/pmc/pmc_$pmc.h

    # build list of libraries for link line in Makefile
    my $slash = $conf->data->get('slash');
    ( my $TEMP_pmc_classes_o   = $TEMP_pmc_o )   =~ s/^| / src${slash}pmc${slash}/g;
    ( my $TEMP_pmc_classes_str = $TEMP_pmc_str ) =~ s/^| / src${slash}pmc${slash}/g;
    ( my $TEMP_pmc_classes_pmc = $pmc_list )     =~ s/^| / src${slash}pmc${slash}/g;

    # Gather the actual names (with MixedCase) of all of the
    # non-abstract built-in PMCs.
    my @names;
PMC: for my $pmc_file ( split( /\s+/, $pmc_list ) ) {
        next if $pmc_file =~ /^const/;
        my $name;
        open my $PMC, "<", "src/pmc/$pmc_file"
            or die "open src/pmc/$pmc_file: $!";
        my $const;
        while (<$PMC>) {
            if (/^pmclass (\w+)(.*)/) {
                $name    = $1;
                my $decl = $2;
                $decl .= <$PMC> until $decl =~ s/\{.*//;

                $const = 1 if $decl =~ /\bconst_too\b/;
                next PMC   if $decl =~ /\babstract\b/;
                next PMC   if $decl =~ /\bextension\b/;

                last;
            }
        }

        close $PMC;

        die "No pmclass declaration found in $pmc_file"
            unless defined $name;

        # please note that normal and Const PMCs must be in this order
        push @names, $name;
        push @names, "Const$name" if $const;
    }

    $conf->data->set(
        pmc                  => $pmc_list,
        pmc_names            => join( ' ', @names ),
        TEMP_pmc_o           => $TEMP_pmc_o,
        TEMP_pmc_build       => $TEMP_pmc_build,
        TEMP_pmc_classes_o   => $TEMP_pmc_classes_o,
        TEMP_pmc_classes_str => $TEMP_pmc_classes_str,
        TEMP_pmc_classes_pmc => $TEMP_pmc_classes_pmc,
    );

    return 1;
}

1;

# Local Variables:
#   mode: cperl
#   cperl-indent-level: 4
#   fill-column: 100
# End:
# vim: expandtab shiftwidth=4:
