# nqp

say("# regex { 'foo'*! }");
my $past :=
  PAST::Regex.new(
    PAST::Regex.new(
      PAST::Regex.new('foo', :pasttype('literal')),
      :pasttype('quant')
    ),
    PAST::Regex.new(:pasttype('pass')),
  );
say(PAST::Compiler.compile($past, :target('pir')));

say("# regex { 'foo'+! }");
my $past :=
  PAST::Regex.new(
    PAST::Regex.new(
      PAST::Regex.new('foo', :pasttype('literal')),
      :pasttype('quant'),
      :min(1)
    ),
    PAST::Regex.new(:pasttype('pass')),
  );
say(PAST::Compiler.compile($past, :target('pir')));

say("# regex { 'foo'?! }");
my $past :=
  PAST::Regex.new(
    PAST::Regex.new(
      PAST::Regex.new('foo', :pasttype('literal')),
      :pasttype('quant'),
      :max(1)
    ),
    PAST::Regex.new(:pasttype('pass')),
  );
say(PAST::Compiler.compile($past, :target('pir')));

say("# regex { 'foo' **! 5..9 }");
my $past :=
  PAST::Regex.new(
    PAST::Regex.new(
      PAST::Regex.new('foo', :pasttype('literal')),
      :pasttype('quant'),
      :min(5),
      :max(9)
    ),
    PAST::Regex.new(:pasttype('pass')),
  );
say(PAST::Compiler.compile($past, :target('pir')));


