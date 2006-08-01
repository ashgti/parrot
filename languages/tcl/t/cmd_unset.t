#!../../parrot tcl.pbc

source lib/test_more.tcl
plan 13

eval_is {unset a} \
  {can't unset "a": no such variable} \
  {unset nothing}

eval_is {
 set a 2
 unset a
} {} {unset something}

eval_is {
 set a 2
 unset a
 set a
} {can't read "a": no such variable} \
  {unset something, use it again}

eval_is {
 set a(2) 2
 unset a(2)
 puts $a(2)
} {can't read "a(2)": no such element in array} \
  {set/unset array element}

eval_is {
 set b(2) 2
 unset b
 set b(2)
} {can't read "b(2)": no such variable} \
  {set/unset array}

eval_is {
 set c(1) 1
 unset c(2)
} {can't unset "c(2)": no such element in array} \
  {unset missing array element}

eval_is {unset d(2)} \
  {can't unset "d(2)": no such variable} \
  {unset element in missing array}

is [unset] {} {unset - no args}

eval_is {
 set -nocomplain 2
 unset -nocomplain
 set -nocomplain
} 2 {unset -nocomplain}

eval_is {
 set -nocomplain 2
 unset -nocomplain -nocomplain
 set -nocomplain
} {can't read "-nocomplain": no such variable} \
  {unset -nocomplain -nocomplain}

eval_is {unset -nocomplain foo} {} {unset -nocomplain foo}

eval_is {
  set -- 2
  unset -nocomplain -- foo
  set --
} 2 {unset -nocomplain -- foo}

eval_is {
  set foo 2
  set bar 3
  unset foo bar
  list [catch {puts $foo}] [catch {puts $bar}]
} {1 1} {unset multiple variables}
