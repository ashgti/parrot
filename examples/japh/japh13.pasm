# 2 threads, synced by sleeping time only
    find_global P5, "_th"
    new P2, .ParrotThread
    find_method P0, P2, "thread3"
    new P6, .PerlString
    set P6, "Js nte artHce\n"
    set I3, 2
    invoke
    new P2, .ParrotThread
    set P6, "utaohrPro akr"
    invoke
    end
.pcc_sub _th:
    set S5, P6
l:
    substr S0, S5, I0, 1
    print S0
    inc I0
    sleep 0.1
    if S0, l
    invoke P1
