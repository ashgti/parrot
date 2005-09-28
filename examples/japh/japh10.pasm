# $Id$

# 2 threads are alternately printing the JaPH
# synchronization is done via a TQueue
    find_global P5, "_th1"              # locate thread function
    new P2, .ParrotThread               # create a new thread
    find_method P0, P2, "thread3"       # a shared thread's entry
    new P7, .TQueue                     # create a Queue object
    new P8, .PerlInt                    # and a PerlInt
    push P7, P8                         # push the PerlInt onto queue
    new P6, .PerlString                 # create new string
    set P6, "Js nte artHce\n"
    set_args "(0,0,0)", P5, P6, P7
    get_results "()"
    invokecc P0                         # _th1.run(P5,P6,P7)
    new P2, .ParrotThread               # same for a second thread
    find_global P5, "_th2"
    set P6, "utaohrPro akr"             # set string to 2nd thread's
    set_args "(0,0,0)", P5, P6, P7
    get_results "()"
    invokecc P0                         # ... data, run 2nd thread too
    end                                 # Parrot joins both

  .pcc_sub _th1:                        # 1st thread function
    get_params "(0,0,0)", P5, P6, P7
  w1: sleep 0.001                       # wait a bit and schedule
    defined I1, P7                      # check if queue entry is ...
    unless I1, w1                       # ... defined, yes: it's ours
    set S5, P6                          # get string param
    substr S9, S5, I10, 1               # extract next char
    print S9                            # and print it

    getstdout P2			# flush output, its line-buffered
    set_args "(0)", P2
    callmethodcc P2, "flush"

    inc I10                             # increment char pointer
    shift P8, P7                        # pull item off from queue
    if S9, w1                           # then wait again, if todo
    set_returns "()"
    returncc                            # done with string

  .pcc_sub _th2:                        # 2nd thread function
    get_params "(0,0,0)", P5, P6, P7
  w2: sleep 0.001
    defined I1, P7                      # if queue entry is defined
    if I1, w2                           # then wait
    set S5, P6
    substr S9, S5, I10, 1               # if not print next char
    print S9
    getstdout P2
    set_args "(0)", P2
    callmethodcc P2, "flush"
    inc I10
    new P8, .PerlInt                    # and put a defined entry
    push P7, P8                         # onto the queue so that
    if S9, w2                           # the other thread will run
    set_returns "()"
    returncc

