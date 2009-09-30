# nqp

# regex { 'foo' 'bar' }
say("# regex { 'foo' 'bar' }");
my $past :=
  PAST::Regex.new(
    PAST::Regex.new('foo', :pasttype('literal')),
    PAST::Regex.new('bar', :pasttype('literal')),
    PAST::Regex.new(:pasttype('pass')),
    :pasttype('concat')
  );

say(PAST::Compiler.compile($past, :target('pir')));
for $past.peek { say($_); }

