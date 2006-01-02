# Copyright: 2001-2005 The Perl Foundation.  All Rights Reserved.
# $Id$

=head1 NAME

config/auto/gc.pm - Garbage Collection

=head1 DESCRIPTION

Checks whether the C<--gc> command-line option was passed to F<Configure.pl>
and sets the memory allocator accordingly.

C<--gc> can take the values:

=over

=item C<gc>

The default. Use the memory allocator in F<src/recources.c>.

=item C<libc>

Use the C library C<malloc>.

=item C<malloc>

Use the malloc in F<src/res_lea.c>.

=item C<malloc-trace>

Use the malloc in F<src/res_lea.c> with tracing enabled.

=back

=cut

package auto::gc;

use strict;
use vars qw($description $result @args);

use base qw(Parrot::Configure::Step::Base);

use Parrot::Configure::Step ':auto';

$description = "Determining what allocator to use...";

# valid libc/malloc/malloc-trace/gc
@args = qw(gc verbose);

sub runstep
{
    my ($self, $conf) = @_;

    my $gc = $conf->options->get('gc');

    if (!defined($gc)) {

        # default is GC in resources.c
        $gc = 'gc';
    } elsif ($gc eq 'libc') {

        # tests mallinfo after allocation of 128 bytes
        if ($conf->data->get('i_malloc')) {
            $conf->data->set(malloc_header => 'malloc.h');
        } else {
            $conf->data->set(malloc_header => 'stdlib.h');
        }

=for nothing

    cc_gen('config/auto/gc/test_c.in');
    eval { cc_build(); };
    my $test = 0;
    unless ($@) {
      $test = cc_run();
    }
    cc_clean();
    # used size should be somewhere here
    unless ($test >= 128 && $test < 155) {
      # if not, use own copy of malloc
      $gc = 'malloc';
    }

=cut

    }

    if ($gc =~ /^malloc(?:-trace)?$/) {
        $conf->data->set(
            TEMP_gc_c => <<"EOF",
\$(SRC_DIR)/$gc\$(O):	\$(GENERAL_H_FILES) \$(SRC_DIR)/$gc.c
\$(SRC_DIR)/res_lea\$(O):	\$(GENERAL_H_FILES) \$(SRC_DIR)/res_lea.c
EOF
            TEMP_gc_o => "\$(SRC_DIR)\/$gc\$(O) \$(SRC_DIR)/res_lea\$(O)",
            gc_flag   => '-DGC_IS_MALLOC',
        );
    } elsif ($gc eq 'libc') {
        $conf->data->set(
            TEMP_gc_c => <<"EOF",
\$(SRC_DIR)/res_lea\$(O):	\$(GENERAL_H_FILES) \$(SRC_DIR)/res_lea.c
EOF
            TEMP_gc_o => "\$(SRC_DIR)/res_lea\$(O)",
            gc_flag   => '-DGC_IS_MALLOC',
        );
    } else {
        $gc = 'gc';
        $conf->data->set(
            TEMP_gc_c => <<"EOF",
\$(SRC_DIR)/resources\$(O):	\$(GENERAL_H_FILES) \$(SRC_DIR)/resources.c
EOF
            TEMP_gc_o => "\$(SRC_DIR)/resources\$(O)",
            gc_flag   => '',
        );
    }
    print(" ($gc) ") if $conf->options->get('verbose');
}

1;
