#
# life.pasm
#
# Play conway's (no, not *him*. The other conway) game
# of life
#
	# First the generation count
	set I2, 500
	# Note the time
	time N5
	# If true, we don't print
	set I12, 1
	set S0,  "               "
	set S1,  "               "
	set S2,  "      *        "
	set S3,  "      **       "
        set S4,  "     * *       "
        set S5,  "               "
	set S6,  "         *     "
	set S7,  "         *     "
	set S8,  "         *     "
	set S9,  "     *         "
	set S10, "     *         "
	set S11, "     *         "
	set S12, "               "
	set S13, "               "
	set S14, "               "
	set S15, ""
	concat S15, S0
	concat S15, S1
	concat S15, S2
	concat S15, S3
	concat S15, S4
	concat S15, S5
	concat S15, S6
	concat S15, S7
	concat S15, S8
	concat S15, S9
	concat S15, S10
	concat S15, S11
	concat S15, S12
	concat S15, S13
	concat S15, S14
	bsr dump
	set I0, 0
loop:	ge I0, I2, getout
	inc I0

	bsr generate

	bsr dump
	branch loop

getout:	time N6
	sub N7, N6, N5
	print I2
	print " generations in "
	print N7
 	print " seconds. "
	set N8, I2
	div N1, N8, N7
	print N1
	print " generations/sec\n"

	interpinfo I1, 1
	print "A total of "
	print I1
	print " bytes were allocated\n"

	interpinfo I1, 2
	print "A total of "
	print I1
	print " DOD runs were made\n"

	interpinfo I1, 3
	print "A total of "
	print I1
	print " collection runs were made\n"

	interpinfo I1, 10
	print "Copying a total of "
	print I1
	print " bytes\n"

	interpinfo I1, 5
	print "There are "
	print I1
	print " active Buffer structs\n"

	interpinfo I1, 7
	print "There are "
	print I1
	print " total Buffer structs\n"


	end

# S15 has the incoming string, S0 is scratch, S1 is scratch, S2 is scratch
#
# I0 is the length of the string
# I1 is the current cell we're checking
# I2 is the count for that cell
# I3 is the offset to the neighbor
generate:
	pushi
	length I0, S15
	set S1, ""
	set I1, 0
genloop:
	set I2, 0
NW:
	set I3, -16
	add I3, I3, I0
	add I3, I3, I1
	mod I3, I3, I0
	substr S0, S15, I3, 1
	ne S0, "*", North
	inc I2
North:
	set I3, -15
	add I3, I3, I0
	add I3, I3, I1
	mod I3, I3, I0
	substr S0, S15, I3, 1
	ne S0, "*", NE
	inc I2
NE:
	set I3, -14
	add I3, I3, I0
	add I3, I3, I1
	mod I3, I3, I0
	substr S0, S15, I3, 1
	ne S0, "*", West
	inc I2
West:
	set I3, -1
	add I3, I3, I0
	add I3, I3, I1
	mod I3, I3, I0
	substr S0, S15, I3, 1
	ne S0, "*", East
	inc I2
East:
	set I3, 1
	add I3, I3, I0
	add I3, I3, I1
	mod I3, I3, I0
	substr S0, S15, I3, 1
	ne S0, "*", SW
	inc I2
SW:
	set I3, 14
	add I3, I3, I0
	add I3, I3, I1
	mod I3, I3, I0
	substr S0, S15, I3, 1
	ne S0, "*", South
	inc I2
South:
	set I3, 15
	add I3, I3, I0
	add I3, I3, I1
	mod I3, I3, I0
	substr S0, S15, I3, 1
	ne S0, "*", SE
	inc I2
SE:
	set I3, 16
	add I3, I3, I0
	add I3, I3, I1
	mod I3, I3, I0
	substr S0, S15, I3, 1
	ne S0, "*", check
	inc I2
check:
	substr S0, S15, I1, 1
	eq S0, "*", check_alive

# If eq 3, put a star in else a space
check_dead:
	eq I2, 3, star
	branch space

check_alive:
	lt I2, 2, space
	gt I2, 3, space
	branch star

space:
	concat S1, " "
	branch iter_done
star:
	concat S1, "*"
iter_done:
	inc I1
	lt I1, I0, genloop
done:
	set S15, S1
	popi
	ret

# S15 has the incoming string, S0 is scratch
dump:
	if I12, dumpend
	print "\f"
	save I0
	save I1
	print "\n\n\n\n\n\n\n\n\n\n\n"
	print "------------- generation "
	print I0
	print " -------------\n"
	set I0, 0
	set I1, 14
printloop:
	substr S0, S15, I0, 15
	print S0
	print "\n"
	add I0, I0, 15
	dec I1
	ge I1, 0 printloop
	restore I1
	restore I0
	sleep 1
dumpend:
	ret
