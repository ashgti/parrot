use Test;
use ParrotString;
plan 5;

# The standard NFG example...
my $str = Parrot_string_new_init("ABC \xd0\xb8\xcc\x8f", 8, ParrotCharset::Unicode, ParrotEncoding::UTF8);
my $str2 = Parrot_string_new_init("", 0, ParrotCharset::Unicode, ParrotEncoding::ParrotNative);
Parrot_string_grapheme_copy($str, $str2);
is(Parrot_string_grapheme_length($str2), 5, "Four UTF8 bytes = one grapheme");
is(Parrot_string_length($str2), 6, "One grapheme is actually two chars");
my $str3 = Parrot_string_new_init("", 0, ParrotCharset::Unicode, ParrotEncoding::UTF8);

Parrot_string_grapheme_copy($str2, $str3);
ok(Parrot_string_byte_equal($str, $str3), "Round-tripping UTF8" );
ok(Parrot_string_character_equal($str, $str3), "Character equivalence for UTF8" );
ok(Parrot_string_character_equal($str2, $str3), "Character equivalence between UTF8 and NFG" );
