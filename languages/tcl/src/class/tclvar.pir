.include "languages/tcl/src/returncodes.pir"
.include "languages/tcl/src/macros.pir"

.namespace [ "TclVar" ]

.HLL "Tcl", "tcl_group"

.cloneable()

=head2 __class_init

Define the attributes required for the class.

=cut

.sub __class_init :load
  $P0 = getclass "String"
  $P1 = subclass $P0, "TclVar"
.end

.sub compile :method
    .param int register_num

    .local string pir_code
    .local string template
    template = <<"END_PIR"
# src/class/tclvar.pir :: compile
.local pmc read
read = find_global '_Tcl', '__read'
$P%i = read("%s")
END_PIR

     $S0 = self
    .sprintf2(pir_code, template, register_num, $S0)

    .return (register_num,pir_code)
.end
