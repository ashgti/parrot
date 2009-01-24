use Test;
use ParrotString;
plan 11;

my $str = Parrot_string_new_init("flurble", 4, ParrotCharset::ASCII, ParrotEncoding::Byte);
ok($str.charset ~~ ParrotCharset::ASCII, "Charset set properly");
is(Parrot_string_grapheme_length($str), 4, "Grapheme length correct");
is(Parrot_string_byte_length($str), 4, "String length correct");
is(Parrot_string_index($str, 1), ord("l"), "String indexing");
Parrot_string_chopn_inplace($str, 2);
is(Parrot_string_grapheme_length($str), 2, "Grapheme length correct post-chop");
is(Parrot_string_byte_length($str), 2, "String length correct post-chop");

$str = Parrot_string_new_init("\xce\xb3\xce\xb5\xce\xb9\xce\xac \xcf\x83\xce\xbf\xcf\x85 \xce\xba\xcf\x8c\xcf\x83\xce\xbc\xce\xbf\xcf\x82", 28, ParrotCharset::Unicode, ParrotEncoding::UTF8);
ok($str.charset ~~ ParrotCharset::Unicode, "We're unicode");
is(Parrot_string_byte_length($str), 28, "String byte length correct");
is(Parrot_string_length($str), 15, "UTF8 char length correct");
is(Parrot_string_index($str, 3), 0x3ac, "UTF8 string indexing");

# The standard NFG example...
$str = Parrot_string_new_init("ABC \xd0\xb8\xcc\x8f", 8, ParrotCharset::Unicode, ParrotEncoding::UTF8);
my $str2 = Parrot_string_new_init("", 0, ParrotCharset::Unicode, ParrotEncoding::ParrotNative);
Parrot_string_grapheme_copy($str, $str2);
is(Parrot_string_grapheme_length($str2), 5, "Four UTF8 bytes = one grapheme");
