#!./parrot nqp.pbc

# Iterpolation and quoting. Cherry-picked from Perl 6 spectest.

plan(9);

# interpolating into double quotes results in a Str
my $a := 1;
ok("foo $a bar" == "foo 1 bar", "Scalar interpolated in double-quotes");

$a++;
ok(q<ok $a> == "ok 2", "Scalar interpolated in q<>");

# Multi-line
$a++;
my $b := $a++;
ok(qq<
foo $b
bar $a
> == '
foo 3
bar 4
', 'Multi-line qq<> works');

# Word quoting.
my @list := qw{ foo bar baz };
ok( +@list == 3, 'qw produced 3 words');

@list := <foo bar>;
ok( +@list == 2, '<> produced 2 words');
ok( @list[0] == 'foo', 'First is "foo"');
ok( @list[1] == 'bar', 'Second is "bar"');

my $str := 'foo';
my $matched := / foo /($str);
ok($matched, 'Can positive match regexp');
$matched := / bar /($str);
ok(!$matched, 'Can negative match regexp');
