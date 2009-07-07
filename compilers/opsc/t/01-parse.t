#! ../../parrot

.include 't/common.pir'

.sub 'main' :main

    .include 'test_more.pir'
    load_bytecode 'opsc.pbc'

    plan(5)

    test_parse_basic_op()
    test_parse_many_ops()
.end

.sub "test_parse_basic_op"
    .local string buf
    .local pmc res

    buf = <<"END"
inline op noop() {
}
END
    
    "_parse_buffer"(buf)
    ok(1, "Simple noop parsed")

    buf = <<"END"
inline op noop() {
    foo
}
END
    
    "_parse_buffer"(buf)
    ok(1, "noop body parsed")

    buf = <<"END"
inline op noop() {
    foo {
        bar{};
    }
}
END
    
    "_parse_buffer"(buf)
    ok(1, "noop nested body parsed")


.end

.sub "test_parse_many_ops"
    .local string buf
    .local pmc res

    buf = <<"END"

=item noop

asdfs

=cut

inline op noop() {
}

=item halt

asdsad

=cut

inline op halt() {
}

=head2 

ads

=cut

inline op rule_the_world() {
}


END
    
    res = "_parse_buffer"(buf)
    ok(1, "Multiple ops parsed")

    $I0 = res['ops';'op']
    is($I0, 3, "...and we have 3 ops")

.end



# Don't forget to update plan!

# Local Variables:
#   mode: cperl
#   cperl-indent-level: 4
#   fill-column: 100
# End:
# vim: expandtab shiftwidth=4 ft=pir:
