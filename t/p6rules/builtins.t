use Parrot::Test tests => 8;
use Parrot::Test::PGE;

p6rule_is  ("abc", '<alpha>', '<alpha>');
p6rule_isnt("123", '<alpha>', 'not <alpha>');
p6rule_is  ("123", '<digit>', 'not <digit>');
p6rule_isnt("abc", '<digit>', '<digit>');

p6rule_is  ("abc", '<digit>*', '<digit>* with none');
p6rule_is  ("abc 123", '<digit>+', '<digit>+ with digits');

p6rule_isnt("234", '<ident>',  'not <ident>');
p6rule_isnt("2ab", '^<ident>', 'not ^<ident>');
