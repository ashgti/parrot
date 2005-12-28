# $Id$

.sub bench :main
.include "pmctypes.pasm"
.include "mmd.pasm"
   .local pmc my_mul
   my_mul = global "my_mul"
   mmdvtregister .MMD_MULTIPLY, .Integer, .Integer, my_mul
   .local int i
   .local pmc r
   .local pmc a
   .local pmc b
   a = new Integer
   b = new Integer
   r = new Integer
   a = 7
   b = 6
   i = 1
loop:
   r = a * b
   inc i
   if i <= 500000 goto loop
   print r
   print "\n"
   end
.end


.sub my_mul
   .param pmc left
   .param pmc right
   .param pmc dest
   $I0 = left
   $I1 = right
   $I2 = $I0 * $I1
   dest = $I2
   .return (dest)
.end
