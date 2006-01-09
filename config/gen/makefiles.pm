# Copyright: 2001-2005 The Perl Foundation.  All Rights Reserved.
# $Id$

=head1 NAME

config/gen/makefiles.pm - Build files

=head1 DESCRIPTION

Generates the various F<Makefile>s and other files needed to build Parrot.

=cut

package gen::makefiles;

use strict;
use vars qw($description $result @args);

use base qw(Parrot::Configure::Step::Base);

use Parrot::Configure::Step ':gen';

$description = "Generating build files...";

@args = ();

sub runstep
{
    my ($self, $conf) = @_;

    $self->makefiles($conf);
    $self->cflags($conf);
    genfile('config/gen/makefiles/libparrot_def.in', 'libparrot.def');
}

sub cflags
{
    my ($self, $conf) = @_;

    genfile(
        'config/gen/makefiles/CFLAGS.in' => 'CFLAGS',
        commentType                      => '#'
    );

    open(CFLAGS, ">> CFLAGS") or die "open >> CFLAGS: $!";

    # Why is this here?  I'd think this information belongs
    # in the CFLAGS.in file. -- A.D.  March 12, 2004
    if ($conf->data->get('cpuarch') =~ /sun4|sparc64/) {

        # CFLAGS entries must be left-aligned.
        print CFLAGS <<"EOF";
src/jit_cpu.c -{-Wcast-align}        # lots of noise!
src/nci.c     -{-Wstrict-prototypes} # lots of noise!
EOF
    }

    close CFLAGS;
}

sub makefiles
{
    my ($self, $conf) = @_;

    genfile(
        'config/gen/makefiles/root.in' => 'Makefile',
        commentType                    => '#',
        replace_slashes                => 1,
        conditioned_lines              => 1
    );
    genfile(
        'config/gen/makefiles/dynclasses_pl.in' => 'tools/build/dynclasses.pl',
        commentType                             => '#',
        replace_slashes                         => 0,
        conditioned_lines                       => 1
    );
	genfile(
        'config/gen/makefiles/dynoplibs_pl.in' => 'tools/build/dynoplibs.pl',
        commentType                             => '#',
        replace_slashes                         => 0,
        conditioned_lines                       => 1
    );
    genfile(
        'config/gen/makefiles/pge.in' => 'compilers/pge/Makefile',
        commentType                   => '#',
        replace_slashes               => 1
    );
    genfile(
        'config/gen/makefiles/tge.in' => 'compilers/tge/Makefile',
        commentType                   => '#',
        replace_slashes               => 1
    );
    genfile(
        'config/gen/makefiles/dynclasses.in' => 'src/dynclasses/Makefile',
        commentType                          => '#',
        replace_slashes                      => 1,
        conditioned_lines                    => 1
    );
    genfile(
        'config/gen/makefiles/dynoplibs.in' => 'src/dynoplibs/Makefile',
        commentType                         => '#',
        replace_slashes                     => 1
    );
    genfile(
        'config/gen/makefiles/editor.in' => 'editor/Makefile',
        commentType                      => '#',
        replace_slashes                  => 1
    );
    genfile(
        'config/gen/makefiles/languages.in' => 'languages/Makefile',
        commentType                         => '#',
        replace_slashes                     => 1
    );
    genfile(
        'config/gen/makefiles/amber.in' => 'languages/amber/Makefile',
        commentType                     => '#',
        replace_slashes                 => 1,
        conditioned_lines               => 1
    );
    genfile(
        'languages/bc/config/makefiles/root.in' => 'languages/bc/Makefile',
        commentType                             => '#',
        replace_slashes                         => 1
    );
    genfile(
        'config/gen/makefiles/befunge.in' => 'languages/befunge/Makefile',
        commentType                       => '#',
        replace_slashes                   => 1
    );
    genfile(
        'config/gen/makefiles/bf.in' => 'languages/bf/Makefile',
        commentType                  => '#',
        replace_slashes              => 1
    );
    genfile(
        'config/gen/makefiles/cola.in' => 'languages/cola/Makefile',
        commentType                    => '#',
        replace_slashes                => 1
    );
    genfile(
        'config/gen/makefiles/jako.in' => 'languages/jako/Makefile',
        commentType                    => '#',
        replace_slashes                => 1
    );
    genfile(
        'config/gen/makefiles/lisp.in' => 'languages/lisp/Makefile',
        commentType                    => '#',
        replace_slashes                => 1
    );
    genfile(
        'config/gen/makefiles/lua.in' => 'languages/lua/Makefile',
        commentType                   => '#',
        replace_slashes               => 1,
        conditioned_lines             => 1
    );
    genfile(
        'config/gen/makefiles/miniperl.in' => 'languages/miniperl/Makefile',
        commentType                        => '#',
        replace_slashes                    => 1
    );
    genfile(
        'languages/m4/config/makefiles/root.in' => 'languages/m4/Makefile',
        commentType                  => '#',
        replace_slashes              => 1,
        conditioned_lines            => 1
    );
    genfile(
        'config/gen/makefiles/ook.in' => 'languages/ook/Makefile',
        commentType                   => '#',
        replace_slashes               => 1
    );
    genfile(
        'config/gen/makefiles/parrot_compiler.in' => 'languages/parrot_compiler/Makefile',
        commentType                               => '#',
        replace_slashes                           => 1
    );
    genfile(
        'config/gen/makefiles/punie.in' => 'languages/punie/Makefile',
        commentType                     => '#',
        replace_slashes                 => 1
    );
    genfile(
        'config/gen/makefiles/regex.in' => 'languages/regex/Makefile',
        commentType                     => '#',
        replace_slashes                 => 1
    );
    genfile(
        'config/gen/makefiles/scheme.in' => 'languages/scheme/Makefile',
        commentType                      => '#',
        replace_slashes                  => 1
    );
    genfile(
        'languages/tcl/config/root.in' => 'languages/tcl/Makefile',
        commentType                    => '#',
        replace_slashes                => 1,
        conditioned_lines              => 1
    );
    genfile(
        'languages/tcl/config/examples.in' => 'languages/tcl/examples/Makefile',
        commentType                        => '#',
        replace_slashes                    => 1
    );
    genfile(
        'config/gen/makefiles/urm.in' => 'languages/urm/Makefile',
        commentType                   => '#',
        replace_slashes               => 1
    );
    genfile(
        'config/gen/makefiles/Zcode.in' => 'languages/Zcode/Makefile',
        commentType                     => '#',
        replace_slashes                 => 1
    );

    if ($conf->data->get('has_perldoc')) {

        # set up docs/Makefile, partly based on the .ops in the root dir

        opendir OPS, "src/ops" or die "opendir ops: $!";
        my @ops = sort grep { !/^\./ && /\.ops$/ } readdir OPS;
        closedir OPS;

        my $pod = join " ", map { my $t = $_; $t =~ s/\.ops$/.pod/; "ops/$t" } @ops;

        $conf->data->set(pod => $pod);

        genfile(
            'config/gen/makefiles/docs.in', 'docs/Makefile',
            commentType       => '#',
            replace_slashes   => 1,
            conditioned_lines => 1
        );

        $conf->data->set(pod => undef);

        open MAKEFILE, ">> docs/Makefile" or die "open >> docs/Makefile: $!";

        my $slash       = $conf->data->get('slash');
        my $new_perldoc = $conf->data->get('new_perldoc');

        foreach my $ops (@ops) {
            my $pod = $ops;
            $pod =~ s/\.ops$/.pod/;
            print MAKEFILE "ops$slash$pod: ..${slash}src${slash}ops${slash}$ops\n";
            if ($new_perldoc == 1) {
                print MAKEFILE "\tperldoc -ud ops${slash}$pod ..${slash}src${slash}ops${slash}$ops\n";
                print MAKEFILE "\t\$(CHMOD) 0644 ops${slash}$pod\n\n";
            } else {
                print MAKEFILE "\tperldoc -u ..${slash}ops${slash}$ops > ops${slash}$pod\n";
                print MAKEFILE "\t\$(CHMOD) 0644 ..${slash}ops${slash}$pod\n\n";
            }
        }

    } else {
        print "\nNo Perldoc, not generating a docs makefile.\n";
    }
}

1;
