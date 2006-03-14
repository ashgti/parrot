# all timings Athlon 800, gcc 2.95.2

# parrot SVN-HEAD
# perl 5.8.0
# python 2.3.3

# perl oo2.pl                                 4.0
# python oo2.py                               2.9 (first time)
# python oo2.py                               2.4

# parrot -C oo2.pasm            -g           -O3
#   with reuse regsave mem                    6.15
# anchor P1                                   6.7
# Dan's new object layout                     5.1

# parrot -j oo2.pasm            -g           -O3
#   with reuse regsave mem                    6.1
# anchor P1                                   6.5
# Dan's new object layout                     4.9

# parrot -C oo2-prop.pasm                     2.8
# parrot -j oo2-prop.pasm                     2.6

    newclass P1, "Foo"
    find_global P2, "init"
    store_global "Foo", "__init", P2
    addattribute P1, ".i"
    addattribute P1, ".j"

    set I10, 0
    set I11, 500000
    find_type I12, "Foo"
loop:
    new P3, I12
    inc I10
    lt I10, I11, loop

    new P3, I12
    classoffset I0, P3, "Foo"
    getattribute P2, P3, I0
    print P2
    print "\n"
    typeof I0, P3
    end

.pcc_sub init:
.include "interpinfo.pasm"
    interpinfo P2, .INTERPINFO_CURRENT_OBJECT
    classoffset I0, P2, "Foo"
    new P10, .PerlInt
    set P10, 10
    setattribute P2, I0, P10
    inc I0
    new P10, .PerlInt
    set P10, 20
    setattribute P2, I0, P10
    returncc
