use Parrot::Test tests => 10;
use Parrot::Config;

print STDERR $PConfig{jitcpuarch}, " JIT CPU\n";
print STDERR $PConfig{so}, " SO extension\n";

SKIP: {
if ($PConfig{jitcpuarch} eq 'i386' && -e "libnci" . $PConfig{so}) {
    $ENV{LD_LIBRARY_PATH} = '.';
}
else {
    skip('needs jit/i386 and libnci'.$PConfig{so},
         Test::Builder->expected_tests());
}

sub gen_test($) {
  local $_ = shift;
  s/\.so/$PConfig{so}/;
  $_;
}

output_is(gen_test(<<'CODE'), <<'OUTPUT', "nci_d_d");
  loadlib P1, "libnci.so"
  print "loaded\n"
  dlfunc P0, P1, "nci_dd", "dd"
  print "dlfunced\n"
  set I0, 1	# prototype used - unchecked
  set I1, 0	# items on stack - unchecked
  set N5, 4.0
  invoke
  ne N5, 8.0, nok_1
  print "ok 1\n"
  ne I0, 0, nok_2	# test return value convention
  ne I1, 0, nok_2
  ne I2, 0, nok_2
  ne I3, 0, nok_2
  ne I4, 1, nok_2
  print "ok 2\n"
  end
nok_1: print "nok 1\n"
  print N5
  print "\n"
  end
nok_2: print "nok 2\n"
  end
CODE
loaded
dlfunced
ok 1
ok 2
OUTPUT

output_is(gen_test(<<'CODE'), <<'OUTPUT', "nci_f_ff");
  loadlib P1, "libnci.so"
  print "loaded\n"
  dlfunc P0, P1, "nci_fff", "fff"
  print "dlfunced\n"
  set I0, 1	# prototype used - unchecked
  set I1, 0	# items on stack - unchecked
  set N5, 12.0
  set N6, 3.0
  invoke
  ne N5, 4.0, nok_1
  print "ok 1\n"
  ne I0, 0, nok_2	# test return value convention
  ne I1, 0, nok_2
  ne I2, 0, nok_2
  ne I3, 0, nok_2
  ne I4, 1, nok_2
  print "ok 2\n"
  end
nok_1: print "nok 1\n"
  print N5
  print "\n"
  end
nok_2: print "nok 2\n"
  end
CODE
loaded
dlfunced
ok 1
ok 2
OUTPUT

output_is(gen_test(<<'CODE'), <<'OUTPUT', "nci_i_sc");
  loadlib P1, "libnci.so"
  print "loaded\n"
  dlfunc P0, P1, "nci_isc", "isc"
  print "dlfunced\n"
  set I0, 1	# prototype used - unchecked
  set I1, 0	# items on stack - unchecked
  set I5, 2
  set I6, 3
  invoke
  ne I5, 6, nok_1
  print "ok 1\n"
  ne I0, 0, nok_2	# test return value convention
  ne I1, 1, nok_2
  ne I2, 0, nok_2
  ne I3, 0, nok_2
  ne I4, 0, nok_2
  print "ok 2\n"
  end
nok_1: print "nok 1\n"
  print I5
  print "\n"
  end
nok_2: print "nok 2\n"
  end
CODE
loaded
dlfunced
ok 1
ok 2
OUTPUT


output_is(gen_test(<<'CODE'), <<'OUTPUT', "nci_s_sc");
  loadlib P1, "libnci.so"
  print "loaded\n"
  dlfunc P0, P1, "nci_ssc", "ssc"
  print "dlfunced\n"
  set I0, 1	# prototype used - unchecked
  set I1, 0	# items on stack - unchecked
  set I5, 2
  set I6, 3
  invoke
  ne I5, 6, nok_1
  print "ok 1\n"
  ne I0, 0, nok_2	# test return value convention
  ne I1, 1, nok_2
  ne I2, 0, nok_2
  ne I3, 0, nok_2
  ne I4, 0, nok_2
  print "ok 2\n"
  end
nok_1: print "nok 1\n"
  print I5
  print "\n"
  end
nok_2: print "nok 2\n"
  end
CODE
loaded
dlfunced
ok 1
ok 2
OUTPUT

output_is(gen_test(<<'CODE'), <<'OUTPUT', "nci_c_sc");
  loadlib P1, "libnci.so"
  print "loaded\n"
  dlfunc P0, P1, "nci_csc", "csc"
  print "dlfunced\n"
  set I0, 1	# prototype used - unchecked
  set I1, 0	# items on stack - unchecked
  set I5, 64
  set I6, 2
  invoke
  ne I5, -128, nok_1
  print "ok 1\n"
  ne I0, 0, nok_2	# test return value convention
  ne I1, 1, nok_2
  ne I2, 0, nok_2
  ne I3, 0, nok_2
  ne I4, 0, nok_2
  print "ok 2\n"
  end
nok_1: print "nok 1\n"
  print I5
  print "\n"
  end
nok_2: print "nok 2\n"
  end
CODE
loaded
dlfunced
ok 1
ok 2
OUTPUT


output_is(gen_test(<<'CODE'), <<'OUTPUT', "nci_i_t");
  loadlib P1, "libnci.so"
  printerr "loaded\n"
  dlfunc P0, P1, "nci_it", "it"
  printerr "dlfunced\n"
  set I0, 1	# prototype used - unchecked
  set I1, 0	# items on stack - unchecked
  set S5, "ko\n"
  invoke
  ne I5, 2, nok_1
  ne I0, 0, nok_2	# test return value convention
  ne I1, 1, nok_2
  ne I2, 0, nok_2
  ne I3, 0, nok_2
  ne I4, 0, nok_2
  printerr "ok 2\n"
  end
nok_1: printerr "nok 1\n"
  printerr I5
  printerr "\n"
  end
nok_2: printerr "nok 2\n"
  end
CODE
loaded
dlfunced
ok
ok 2
OUTPUT

output_is(gen_test(<<'CODE'), <<'OUTPUT', "nci_t_t");
  loadlib P1, "libnci.so"
  print "loaded\n"
  dlfunc P0, P1, "nci_tt", "tt"
  print "dlfunced\n"
  set I0, 1	# prototype used - unchecked
  set I1, 0	# items on stack - unchecked
  set S5, "ko\n"
  invoke
  print S5
  ne I0, 0, nok_2	# test return value convention
  ne I1, 0, nok_2
  ne I2, 1, nok_2
  ne I3, 0, nok_2
  ne I4, 0, nok_2
  print "ok 2\n"
  end
nok_1: print "nok 1\n"
  print I5
  print "\n"
  end
nok_2: print "nok 2\n"
  end
CODE
loaded
dlfunced
ok worked
ok 2
OUTPUT

output_is(gen_test(<<'CODE'), <<'OUTPUT', "nci_d_d - stress test");
  loadlib P1, "libnci.so"
  print "loaded\n"
  set I10, 1000000
  print "dlfunced\n"
loop:
  dlfunc P0, P1, "nci_dd", "dd"
  set I0, 1	# prototype used - unchecked
  set I1, 0	# items on stack - unchecked
  set N5, 4.0
  invoke
  ne N5, 8.0, nok_1
  dec I10
  gt I10, 0, loop
  print "ok 1\n"
  ne I0, 0, nok_2	# test return value convention
  ne I1, 0, nok_2
  ne I2, 0, nok_2
  ne I3, 0, nok_2
  ne I4, 1, nok_2
  print "ok 2\n"
  end
nok_1: print "nok 1\n"
  print N5
  print "\n"
  end
nok_2: print "nok 2\n"
  end
CODE
loaded
dlfunced
ok 1
ok 2
OUTPUT

output_is(gen_test(<<'CODE'), <<'OUTPUT', "nci_d_d - clone");
  loadlib P1, "libnci.so"
  print "loaded\n"
  dlfunc P0, P1, "nci_dd", "dd"
  print "dlfunced\n"
  clone P2, P0
  print "ok 1\n"
  set I0, 1	# prototype used - unchecked
  set I1, 0	# items on stack - unchecked
  set N5, 4.0
  invoke
  ne N5, 8.0, nok_1
  print "ok 2\n"
  set I0, 1
  set I1, 0
  set N5, 4.0
  set P0, P2
  invoke
  ne N5, 8.0, nok_1
  print "ok 3\n"
  ne I0, 0, nok_2	# test return value convention
  ne I1, 0, nok_2
  ne I2, 0, nok_2
  ne I3, 0, nok_2
  ne I4, 1, nok_2
  print "ok 4\n"
  end
nok_1: print "nok 1\n"
  print N5
  print "\n"
  end
nok_2: print "nok 2\n"
  end
CODE
loaded
dlfunced
ok 1
ok 2
ok 3
ok 4
OUTPUT

output_is(gen_test(<<'CODE'), <<'OUTPUT', "nci_i_iii");
  loadlib P1, "libnci.so"
  dlfunc P0, P1, "nci_iiii", "iiii"
  set I0, 1	# prototype used - unchecked
  set I1, 0	# items on stack - unchecked
  set I5, 10
  set I6, 20
  set I7, 30
  invoke
  end
  end
nok_2: print "nok 2\n"
  end
CODE
10 20 30
OUTPUT

} # SKIP

1;
