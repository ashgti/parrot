=head1 NAME

PAST::Code - A code block

=head1 DESCRIPTION

PAST::Code represents a block of code in the AST. It is a subclass of
Node.

=cut

.namespace [ "PAST::Code" ]

.sub "__onload" :load
    .local pmc base
    $P0 = getclass 'Node'
    base = subclass $P0, 'PAST::Code'
    .return ()
.end


# Local Variables:
#   mode: pir
#   fill-column: 100
# End:
# vim: expandtab shiftwidth=4:
