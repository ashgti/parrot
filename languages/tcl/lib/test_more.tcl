#
# Analog to perl's Test::More
#
# We can't currently run tcltest.tcl, but this gives us a mock version that
# lets us at least run tcl test files; it also gives us TAP output for our
# parrot-focused tests.

proc skip_all {} {
    puts 1..0
}

proc plan {number} {
    if {$number eq "no_plan"} {
        global very_bad_global_variable_test_num
        puts 1..$very_bad_global_variable_test_num
    } {
        puts 1..$number
    }
}

set very_bad_global_variable_test_num 0

# The special argument, if passed, should be a two element list, e.g.
# {TODO "message"}

proc is {value expected {description ""} {special {}}}  {
    global very_bad_global_variable_test_num
    incr  very_bad_global_variable_test_num

    set num $very_bad_global_variable_test_num
    set type ""

    if {[llength $special] == 2} {
        set type [string toupper [lindex $special 0]]
        if {$type eq "TODO"} {
            global env
            set normal [array get env PARTCL_DEVTEST]
            if {$normal ne {}} {
                set type ""
                set special ""
            }
        }
        set special_reason [concat {*}[lindex $special 1]]
        set description [regsub -all # $description {\#}]
        set description " - $description # $type $special_reason"
    } else {
        if  {$description ne ""} {
            set description " - $description"
        }
    }

    if {$value eq $expected} {
        puts "ok $num$description"
        return 1
    } {
        puts "not ok $num$description"

        if {$type ne "TODO"} {
            set formatted_value [join [split $value "\n"] "\n# "]
            set formatted_expected [join [split $expected "\n"] "\n# "]
            diag "\n#     Failed test #$very_bad_global_variable_test_num\n#      got : '$formatted_value'\n# expected : '$formatted_expected'"
        }
        return 0
    }
}

proc eval_is {code expected {description ""} {special {}}}  {
    global very_bad_global_variable_test_num
    # The one case where skip actually means "don't do that"
    if {[llength $special] == 2} {
        set boolean [string compare -nocase [lindex $special 0] skip]
        if {! $boolean} {
            global very_bad_global_variable_test_num
            incr  very_bad_global_variable_test_num
            puts "ok $very_bad_global_variable_test_num # $special"
            return 1
        }
    }
    catch {uplevel #0 $code} actual
    is $actual $expected $description $special
}

proc ok {value {description ""} {special {}}}  {
    is $value 1 $description $special
}

proc not_ok {value {description ""} {special {}}} {
    set value [expr !$value]
    ok $value $description $special
}

proc pass {{description ""} {special ""}} {
  ok 1 $description $special
}

proc like {value regexp {description ""}} {
   if ([regexp $regexp $value]) {
     pass $description
   } else {
     is "STRING: $value" "REGEXP: $regexp" $description
   }
}

proc isnt {code expected {description ""} {special {}}}  {
    set code     "\"\[eval {$code}\]\""
    set expected "\"$expected\""

    cmp_ok $code ne $expected $description $special
}

proc cmp_ok {left op right {description ""} {special {}}} {
    ok "expr {$left $op $right}" $description $special
}

proc fail {{description ""} {special ""}} {
  is "something else: $description" {something} $description $special
}

proc diag {diagnostic} {
  puts stderr "# $diagnostic"
}

# A placeholder that simulates the real tcltest's exported test proc.
proc test {num description args} {
    global skip_tests
    global todo_tests
    global abort_after
    if {![info exists skip_tests]} {
        # get listing of all the tests we can't run.
        source lib/skipped_tests.tcl
    }

    set full_desc "$num $description"

    set should_skip [dict filter $skip_tests script {K V} {
        set val [lsearch -exact $V $num]
        expr {$val != -1}
    }]

    set skip_reason [dict keys $should_skip]

    set should_todo [dict filter $todo_tests script {K V} {
        set matched no
        foreach element $V {
          if {[string match $element $num]} {
	    set matched yes
	    break
	  }
	}
        set matched
    }]

    set todo_reason [dict keys $should_todo]

    if {[string length $skip_reason]} {
        pass $full_desc [list SKIP $skip_reason]
    } elseif {[llength $args] == 2} {
        if {[string length $todo_reason]} {
          eval_is [lindex $args 0] [lindex $args 1] $full_desc "TODO {$todo_reason}"
	} else {
          eval_is [lindex $args 0] [lindex $args 1] $full_desc
	}
    } elseif {[llength $args] == 3} {
        # XXX : we're just skipping the constraint here...
        if {[string length $todo_reason]} {
          eval_is [lindex $args 1] [lindex $args 2] $full_desc "TODO {$todo_reason}"
	} else {
          eval_is [lindex $args 1] [lindex $args 2] $full_desc
	}
    } else {
        # Skip test if too many or two few args.
        pass $full_desc [list SKIP {can't deal with this version of test yet}]
    }

    if {! [catch {set abort $abort_after($num)}]} {
       plan no_plan
       exit 0
    }
}

# Hacks to allow compilation of tests - used as a crutch until we
# support tcltest.

# Constraints are like skip conditions that
# can be specified by a particular invocation to test. Since we're ingoring
# all the option params to test, we'll ignore this one too, and execute tests
# when we shouldn't.

proc testConstraint     {args} {return 0}
proc temporaryDirectory {args} {return .}
proc makeFile           {args} {return 0}
proc removeFile         {args} {return 0}
proc bytestring         {args} {return 0}
proc customMatch        {args} {return 0}
proc testinfocmdcount   {args} {return 0}
proc interpreter        {args} {return 0}
proc safeInterp         {args} {return 0}
proc child              {args} {return 0}
proc child-trusted      {args} {return 0}
proc makeDirectory      {args} {return 0}
proc removeDirectory    {args} {return 0}
proc testobj            {args} {return 0}
proc testsetplatform    {args} {return 0}
proc testevalex         {cmd}  { uplevel {*}$cmd }
proc cleanupTests       {args} {return 0}
proc PowerSet           {args} {return 0}

set auto_path {}

namespace eval tcl {
    set OptDescN 0
}

namespace eval tcltest {
    set verbose 0
    set testSingleFile 0
    set temporaryDirectory .
    proc temporaryDirectory {args} {return .}
    proc testConstraint     {args} {return 0}
    proc test {args} {return [::test {*}$args]}
    proc cleanupTests       {args} {return 0}
}
