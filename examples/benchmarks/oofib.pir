
.pcc_sub _main
    .param pmc argv
    .sym int argc
    argc = argv
    .sym pmc N
    N = new PerlInt
    N = 28
    if argc <= 1 goto noarg
    $S0 = argv[1]
    N = $S0
noarg:
    .sym float start
    time start

    .local pmc A
    .local pmc B
    .local pmc b

    A = newclass "A"
    B = subclass  A, "B"

    find_type $I0, "B"
    b = new  $I0

    .sym pmc r
    r = b."fib"(N)

    .sym float fin
    time fin
    print "fib("
    print N
    print ") = "
    print r
    print " "
    sub fin, start
    print fin
    print "s\n"
    end
.end

.namespace ["A"]

.sub fib method
    .param pmc n
    if n >= 2 goto rec
    .pcc_begin_return
    .return n
    .pcc_end_return
rec:
    .sym pmc n1
    .sym pmc n2
    .sym pmc r1
    .sym pmc r2
    n1 = new PerlInt
    n2 = new PerlInt
    n1 = n - 1
    n2 = n - 2
    r1 = self."fibA"(n1)
    r2 = self."fibB"(n2)
    n = new PerlInt
    n = r1 + r2
    .pcc_begin_return
    .return n
    .pcc_end_return
.end

.sub fibA method
    .param pmc n
    if n >= 2 goto rec
    .pcc_begin_return
    .return n
    .pcc_end_return
rec:
    .sym pmc n1
    .sym pmc n2
    .sym pmc r1
    .sym pmc r2
    n1 = new PerlInt
    n2 = new PerlInt
    n1 = n - 1
    n2 = n - 2
    r1 = self."fib"(n1)
    r2 = self."fibB"(n2)
    n = new PerlInt
    n = r1 + r2
    .pcc_begin_return
    .return n
    .pcc_end_return
.end

.namespace ["B"]

.sub fibB method
    .param pmc n
    if n >= 2 goto rec
    .pcc_begin_return
    .return n
    .pcc_end_return
rec:
    .sym pmc n1
    .sym pmc n2
    .sym pmc r1
    .sym pmc r2
    n1 = new PerlInt
    n2 = new PerlInt
    n1 = n - 1
    n2 = n - 2
    r1 = self."fib"(n1)
    r2 = self."fibA"(n2)
    n = new PerlInt
    n = r1 + r2
    .pcc_begin_return
    .return n
    .pcc_end_return
.end
