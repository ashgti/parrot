#! nqp

class Ops::Trans is Hash;

=begin

Base class for Runcode transformations.

=end

method prefix() { 'Parrot_' };

# We can't use yada-yada-yada
method suffix() { die("...") }

# Prepare internal structures from Ops::File.ops.
method prepare_ops($emitter, $ops_file) { die('...') }

# 
method emit_c_header_part($fh) { die('...') }

# Called from Ops::Op.
method body_prelude() { '' }



# vim: expandtab shiftwidth=4 ft=perl6:
