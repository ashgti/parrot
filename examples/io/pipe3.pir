.sub main :main
    .param pmc argv
    .local int argc
    .local pmc pipew
    pipew = open '/bin/cat -n', "|-"
    $S0 = pop pipew	# pop buf layer
    print pipew, "hello\n"
    print pipew, "world\n"
    close pipew
.end


# Local Variables:
#   mode: pir
#   fill-column: 100
# End:
# vim: expandtab shiftwidth=4:
