#!./parrot nqp.pbc

# Iterpolation. Cherry-picked from Perl 6 spectest.

plan(4);

# interpolating into double quotes results in a Str
my $a := 1;
say("ok $a");

$a++;
say(qq<ok $a>);

# Multi-line
$a++;
my $b := $a++;
say(qq<
ok $b
ok $a
>);
