# Copyright: 2001-2003 The Perl Foundation.  All Rights Reserved.
# $Id$

=head1 NAME

config/inter/pmc.pm - PMC Files

=head1 DESCRIPTION

Asks the user to select which PMC files to include.

=cut

package inter::pmc;

use strict;
use vars qw($description @args %PMC_PARENTS);

use base qw(Parrot::Configure::Step::Base);

use Parrot::Configure::Step ':inter';

$description = 'Determining what pmc files should be compiled in';

@args = qw(ask pmc);

# Return the (lowercased) name of the immediate parent of the given
# (lowercased) pmc name.
sub pmc_parent
{
    my ($pmc) = @_;

    return $PMC_PARENTS{$pmc} if defined $PMC_PARENTS{$pmc};

    local $/;
    open(PMC, "src/pmc/$pmc.pmc")
        or die "open src/pmc/$pmc.pmc failed: $!";
    local $_ = <PMC>;
    close PMC;

    # Throw out everything but the pmclass declaration
    s/^.*?pmclass//s;
    s/\{.*$//s;

    return $PMC_PARENTS{$pmc} = lc($1) if /extends\s+(\w+)/;
    return $PMC_PARENTS{$pmc} = "default";
}

# Return an array of all
sub pmc_parents
{
    my ($pmc) = @_;
    my @parents = ($pmc);
    push @parents, pmc_parent($parents[-1]) until $parents[-1] eq 'default';
    shift(@parents);
    push(@parents, 'perlhash') if ($pmc eq 'OrderedHash');
    return @parents;
}

sub get_pmc_order
{
    open IN, 'src/pmc/pmc.num' or die "Can't read src/pmc/pmc.num";
    my %order;
    while (<IN>) {
        next if (/^#/);
        if (/(\w+\.\w+)\s+(\d+)/) {
            $order{$1} = $2;
        }
    }
    close IN;
    return \%order;
}

sub sort_pmcs
{
    my @pmcs      = @_;
    my $pmc_order = get_pmc_order();
    my $n         = scalar keys(%{$pmc_order});
    my @sorted_pmcs;
    for (@pmcs) {
        if (exists $pmc_order->{$_}) {
            $sorted_pmcs[$pmc_order->{$_}] = $_;

            #if (exists $pmc_order->{"const$_"}) {
            #	$sorted_pmcs[$pmc_order->{"const$_"}] = "const$_";
            #}
        } else {
            $sorted_pmcs[$n++] = $_;
        }
    }
    ## print "***\n", join(' ', @sorted_pmcs), "\n";
    @sorted_pmcs;
}

sub runstep
{
    my ($self, $conf) = @_;

    my @pmc = (
        sort
            map { m{\./src/pmc/(.*)} } glob "./src/pmc/*.pmc"
    );

    @pmc = sort_pmcs(@pmc);

    my $pmc_list = $conf->options->get('pmc')
        || join(' ', grep { defined $_ } @pmc);

    if ($conf->options->get('ask')) {
        print <<"END";


The following PMC files are available:
  @pmc
END
        {
            $pmc_list = prompt('Which PMC files would you like?', $pmc_list);
        }
    }

    # XXX:leo do we really need an interactive step for this
    # user could deactivate vital PMCs like PerlHash or SArray
    # so there would be tests needed, that check for vital classes

    # names of class files for src/pmc/Makefile
    (my $TEMP_pmc_o   = $pmc_list) =~ s/\.pmc/\$(O)/g;
    (my $TEMP_pmc_str = $pmc_list) =~ s/\.pmc/\.str/g;

    # calls to pmc2c.pl for src/pmc/Makefile
    my $TEMP_pmc_build = <<"E_NOTE";

# the following part of the Makefile was built by 'config/inter/pmc.pm'

E_NOTE

    foreach my $pmc (split(/\s+/, $pmc_list)) {
        $pmc =~ s/\.pmc$//;
        next if ($pmc =~ /^const/);

        # make each pmc depend upon its parent.
        my $parent = pmc_parent($pmc) . ".pmc";
        $parent = "perlhash.pmc src/pmc/$parent" if ($pmc eq 'orderedhash');
        my $parent_dumps = '';
        $parent_dumps .= "src/pmc/$_.dump " foreach reverse((pmc_parents($pmc)));
        $parent_dumps = "src/pmc/perlhash.dump $parent_dumps"
            if ($pmc eq 'orderedhash');
        my $parent_headers = '';
        $parent_headers .= "src/pmc/pmc_$_.h " foreach (pmc_parents($pmc));
        $parent_headers = "src/pmc/pmc_perlhash.h $parent_headers"
            if ($pmc eq 'orderedhash');
        $TEMP_pmc_build .= <<END
src/pmc/$pmc.c : src/pmc/$pmc.dump
	\$(PMC2CC) src/pmc/$pmc.pmc

src/pmc/$pmc.dump : vtable.dump $parent_dumps lib/Parrot/Pmc2c.pm \\
		src/pmc/$pmc.pmc
	\$(PMC2CD) src/pmc/$pmc.pmc 

src/pmc/pmc_$pmc.h: src/pmc/$pmc.c

src/pmc/$pmc\$(O): src/pmc/$pmc.str \$(NONGEN_HEADERS) \\
        $parent_headers

END
    }

    # src/pmc/$pmc\$(O): \$(NONGEN_HEADERS) $parent_headers src/pmc/pmc_$pmc.h

    # build list of libraries for link line in Makefile
    my $slash = $conf->data->get('slash');
    (my $TEMP_pmc_classes_o   = $TEMP_pmc_o)   =~ s/^| / src${slash}pmc${slash}/g;
    (my $TEMP_pmc_classes_str = $TEMP_pmc_str) =~ s/^| / src${slash}pmc${slash}/g;
    (my $TEMP_pmc_classes_pmc = $pmc_list)     =~ s/^| / src${slash}pmc${slash}/g;

    # Gather the actual names (with MixedCase) of all of the
    # non-abstract built-in PMCs.
    my @names;
    PMC: foreach my $pmc_file (split(/\s+/, $pmc_list)) {
        next if ($pmc_file =~ /^const/);
        my $name;
        open(PMC, "src/pmc/$pmc_file") or die "open src/pmc/$pmc_file: $!";
        my $const;
        while (<PMC>) {
            if (/^pmclass (\w+)(.*)/) {
                $name = $1;
                my $decl = $2;
                $decl .= <PMC> until ($decl =~ s/\{.*//);
                $const = 1 if $decl =~ /\bconst_too\b/;
                next PMC if $decl =~ /\babstract\b/;
                next PMC if $decl =~ /\bextension\b/;
                last;
            }
        }
        close PMC;
        die "No pmclass declaration found in $pmc_file"
            if !defined $name;

        # please note that normal and Const PMCs must be in this
        # order
        push @names, $name;
        push @names, "Const$name" if $const;
    }

    $conf->data->set(
        pmc                  => $pmc_list,
        pmc_names            => join(" ", @names),
        TEMP_pmc_o           => $TEMP_pmc_o,
        TEMP_pmc_build       => $TEMP_pmc_build,
        TEMP_pmc_classes_o   => $TEMP_pmc_classes_o,
        TEMP_pmc_classes_str => $TEMP_pmc_classes_str,
        TEMP_pmc_classes_pmc => $TEMP_pmc_classes_pmc,
    );

    return $self;
}

1;
