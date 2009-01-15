use Test;
use ParrotString;
plan 8;

my $str = Parrot_string_new_init("flurble", 4, ParrotCharset::ASCII, ParrotEncoding::Byte);
ok($str.charset ~~ ParrotCharset::ASCII, "Charset set properly");
is(Parrot_string_grapheme_length($str), 4, "String length correct");
is(Parrot_string_byte_length($str), 4, "String length correct");
is(Parrot_string_index($str, 1), ord("l"), "String indexing");

$str = Parrot_string_new_init("\xce\xb3\xce\xb5\xce\xb9\xce\xac \xcf\x83\xce\xbf\xcf\x85 \xce\xba\xcf\x8c\xcf\x83\xce\xbc\xce\xbf\xcf\x82", 28, ParrotCharset::Unicode, ParrotEncoding::UTF8);
ok($str.charset ~~ ParrotCharset::Unicode, "We're unicode");
is(Parrot_string_byte_length($str), 28, "String byte length correct");
is(Parrot_string_length($str), 15, "UTF8 char length correct");
is(Parrot_string_index($str, 3), 0x3ac, "UTF8 string indexing");
