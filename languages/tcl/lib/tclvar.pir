.include "languages/tcl/lib/returncodes.pir"

.namespace [ "TclVar" ]

.HLL "Tcl", "tcl_group"

.cloneable()

=head2 __class_init

Define the attributes required for the class.

=cut

.sub __class_init :load
  $P0 = getclass "TclConst"
  $P1 = subclass $P0, "TclVar"
.end

.sub compile :method
    .param int register_num

    .local string pir_code
    pir_code = ""
    .local pmc args
 
    args = new .Array
    args = 4
    args[0] = register_num
    $S0 = self
    args[1] = $S0

    $S1 = sprintf ".local pmc read\nread=find_global \"_Tcl\", \"__read\"\n$P%i = read(\"%s\")\n", args
    pir_code .= $S1 
 
    .return (register_num,pir_code)
.end
