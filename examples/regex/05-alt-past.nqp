# nqp


say("# regex { 'foo' || 'bar' || 'baz' }");
my $past :=
  PAST::Regex.new(
    PAST::Regex.new(
      PAST::Regex.new('foo', :pasttype('literal')),
      PAST::Regex.new('bar', :pasttype('literal')),
      PAST::Regex.new('baz', :pasttype('literal')),
      :pasttype('alt')
    ),
    PAST::Regex.new(:pasttype('pass')),
  );

say(PAST::Compiler.compile($past, :target('pir')));
for $past.peek { say($_); }

say("# regex { ['foo' | 'bar' | 'baz'] 'xyz' }");
my $past :=
  PAST::Regex.new(
    PAST::Regex.new(
      PAST::Regex.new('foo', :pasttype('literal')),
      PAST::Regex.new('bar', :pasttype('literal')),
      PAST::Regex.new('baz', :pasttype('literal')),
      :pasttype('alt_longest')
    ),
    PAST::Regex.new('xyz', :pasttype('literal')),
    PAST::Regex.new(:pasttype('pass')),
  );

say(PAST::Compiler.compile($past, :target('pir')));
for $past.peek { say($_); }

