use Test;
use ParrotString;
plan 2;

my $str = Parrot_string_new_init("abcdef", 4, ParrotCharset::ASCII, ParrotEncoding::Byte);
ok($str.charset ~~ ParrotCharset::ASCII, "Charset set properly");
is(Parrot_string_length($str), 4, "String length correct");
