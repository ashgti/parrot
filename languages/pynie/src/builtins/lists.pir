=head1 NAME

src/builtins/lists.pir - List operations

=head1 Functions

=over 4

=cut

.namespace

.sub 'listmaker'
    .param pmc args            :slurpy
    unless null args goto have_args
    args = new .ResizablePMCArray
  have_args:
    .return (args)
.end

=back

=head1 AUTHOR

Patrick Michaud <pmichaud@pobox.com> is the current author and
maintainer.  Patches and suggestions can be sent to
<parrot-porters@perl.org> or <perl-compiler@perl.org> .

=cut

# Local Variables:
#   mode: pir
#   fill-column: 100
# End:
# vim: expandtab shiftwidth=4:
