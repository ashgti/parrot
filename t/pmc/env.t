#! perl -w

use Parrot::Test tests => 5;
use Test::More;
use Parrot::Config;

$ENV{"PARROT_TMP"} = "riding a ponie";
output_like(<<'CODE', <<OUT, "getenv");
    new P0, .Env
    set S0, P0["PARROT_TMP"]
    print S0
    end
CODE
/riding a ponie/i
OUT

output_like(<<'CODE', <<OUT, "setenv/getenv");
    new P0, .Env
    set P0["PARROT_TMP"], "hello polly"
    set S0, P0["PARROT_TMP"]
    print S0
    end
CODE
/hello polly/i
OUT

output_is(<<'CODE', <<OUT, "envs are all the same");
    new P0, .Env
    set P0["PARROT_TMP"], "hello polly"
    set S0, P0["PARROT_TMP"]
    new P1, .Env
    set S1, P1["PARROT_TMP"]
    eq S0, S1, ok
    print "not ok\n"
    end
ok:
    print "ok\n"
    end
CODE
ok
OUT

output_is(<<'CODE', <<OUT, "gone/delete");
    new P0, .Env
    set P0["PARROT_TMP"], "hello polly"
    exists I0, P0["PARROT_TMP"]
    if I0, ok1
    print "not "
ok1:
    print "ok 1\n"
    delete P0["PARROT_TMP"]
    set S0, P0["PARROT_TMP"]
    unless S0, ok2
    print "not "
ok2:
    print "ok 2\n"
    end
CODE
ok 1
ok 2
OUT

SKIP: {
    # won't work on our unsetenv implementation
    skip("no native unsetenv", 1) unless $PConfig{"unsetenv"};
output_is(<<'CODE', <<OUT, "exists/delete");
    new P0, .Env
    set P0["PARROT_TMP"], "hello polly"
    exists I0, P0["PARROT_TMP"]
    if I0, ok1
    print "not "
ok1:
    print "ok 1\n"
    delete P0["PARROT_TMP"]
    exists I0, P0["PARROT_TMP"]
    unless I0, ok2
    print "not "
ok2:
    print "ok 2\n"
    end
CODE
ok 1
ok 2
OUT
}
