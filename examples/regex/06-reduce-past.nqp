# nqp

say('# regex xyz { "foo" {*} "bar" }');
my $past :=
  PAST::Regex.new(
    PAST::Regex.new('foo', :pasttype('literal')),
    PAST::Regex.new(:pasttype('reduce'), :name('xyz')),
    PAST::Regex.new('bar', :pasttype('literal')),
    PAST::Regex.new(:pasttype('pass'), :name('xyz')),
    :pasttype('concat')
  );

say(PAST::Compiler.compile($past, :target('pir')));

