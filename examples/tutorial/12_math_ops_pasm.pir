=head1 Math Operations (continued)

Some math operations have an opcode name, but no simple operator syntax in PIR,
so you use the opcode name directly.

=cut

.sub main :main
    $N0 = abs -5.0  # the absolute value of -5.0 is 5.0
    print $N0
    print "\n"

    $I1 = fact 5    # the factorial of 5 is 120
    inc $I1         # 120 incremented by 1 is 121
    print $I1
    print "\n"
.end

# Local Variables:
#   mode: pir
#   fill-column: 100
# End:
# vim: expandtab shiftwidth=4 ft=pir:

