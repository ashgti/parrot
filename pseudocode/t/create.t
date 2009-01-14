use Test;
use ParrotString;
plan 3;

my $str = Parrot_string_new_init("abcdef", 4, ParrotCharset::ASCII, ParrotEncoding::Byte);
ok($str.charset ~~ ParrotCharset::ASCII, "Charset set properly");
is(Parrot_string_grapheme_length($str), 4, "String length correct");
is(Parrot_string_byte_length($str), 4, "String length correct");
