.loadlib 'io_ops'

.sub 'foo' 
    .param pmc bar
    say bar
.end

.sub 'main' :main
    $I0 = 123.0
    $S0 = $I0
    $S0 .= 'bar'

    $P0 = getstdin
readstuff:
    $S1 = $P0.'readline_interactive'('Please enter "foo": ')
    ne $S1, 'foo', readstuff
    'foo'($S0)
    .return(0)
.end
