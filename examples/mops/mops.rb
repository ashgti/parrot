#!/usr/bin/ruby
#
# mops.rb
#
# A Ruby implementation of the mops.pasm example program,
# for speed comparisons.
#
# Copyright (C) 2001 The Parrot Team. All rights reserved.
# This program is free software. It is subject to the same
# license as The Parrot Interpreter.
#
# $Id$
#

i2 = 0                       # set    I2, 0
i3 = 1                       # set    I3, 1
i4 = 100000000               # set    I4, 100000000
                             #
puts "Iterations:    #{i4}"  # print  "Iterations:    "
                             # print  I4
                             # print  "\n"
                             #
i1 = 2                       # set    I1, 2
i5 = i4 * i1                 # mul    I5, I4, I1
                             #
puts "Estimated ops: #{i5}"  # print  "Estimated ops: "
                             # print  I5
                             # print  "\n"
                             #
n1 = Time.now()              # time N1
                             #
while i4 != 0                # REDO:
  i4 = i4 - i3               # sub    I4, I4, I3
end                          # if     I4, REDO
                             #
                             # DONE:
n5 = Time.now()              # time   N5
                             #
n2 = n5 - n1                 # sub    N2, N5, N1
                             #
puts "Elapsed time:  #{n2}"  # print  "Elapsed time:  "
                             # print  N2
                             # print  "\n"
                             #
n1 = i5                      # iton   N1, I5
n1 = n1 / n2                 # div    N1, N1, N2
n2 = 1000000.0               # set    N2, 1000000.0
n1 = n1 / n2                 # div    N1, N1, N2
                             #
puts "M op/s:        #{n1}"  # print  "M op/s:        "
                             # print  N1
                             # print  "\n"
                             #
exit(0)                      # end

