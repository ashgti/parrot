.sub main :main

   $S0 = foo("Zaphod")
   say $S0

.end

.sub foo
    .param string name

    .local string greeting
    greeting = "Hello, " . name

    .return (greeting)
.end

# Local Variables:
#   mode: pir
#   fill-column: 100
# End:
# vim: expandtab shiftwidth=4:

