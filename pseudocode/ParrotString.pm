use v6;
enum ParrotStringFlags <StringCOW>;
class ParrotString {
    has @.buffer is rw;
    has $.strlen is rw;
    has $.flags is rw;
    has $.bufused is rw;
    has $.hashval is rw;
    has $.encoding is rw;
    has $.charset  is rw;
    has $.normalization is rw;
};

use Charsets;
use Normalizations;
use Encodings;

## COW stuff
sub Parrot_string_new_COW($src) { ... }
sub Parrot_string_reuse_COW($src, $dst) { ... }
sub Parrot_string_write_COW($src) {
    if !($src.flags +| StringCOW) { return $src; }
    our @buffercopy = $src.buffer;
    $src.buffer = @buffercopy;
    $src.flags +&= +^ StringCOW;
    return $src;
}
sub Parrot_string_set($src, $dst) { 
    return Parrot_string_reuse_COW($src, $dst); # Isn't it?
} 

## String operations

sub Parrot_string_concat($first, $second, $flags) {
    if (!$first && !$second) { #
    }
    ...
}

sub Parrot_string_append($first, $second) { ... }

sub Parrot_string_new_init($s, $len, $charset, $encoding) {
    my $news = ParrotString.new();
    $news.charset  = $charset;
    $news.encoding = $encoding;
    $news.encoding.setup($news);
    $news.buffer   = map { ord $_ }, split("", $s);
    $news.bufused = $news.strlen = $len || $s.chars;
    return $news;
}

sub Parrot_string_resize($str, $bytes) {
    if ($bytes > 0) {
       $str.strlen += $bytes;
       # "Realloc"
       for 1..$bytes { push $str.buffer, 0 }
    } else {
       if ($str.strlen - $bytes) < $str.buflen { die "Parrot_string_resize would truncate"; } 
       for 1..$bytes { pop $str.buffer }
       $str.strlen -= $bytes;
    }
}

sub Parrot_string_length($str) {
    return $str.encoding.string_length($str); # Let fixies optimize themselves
}

sub Parrot_string_grapheme_length($str) {
    # This code written funny to be a bit more C-like
    my $data = 0; my $callback = sub ($char, $data is rw) { $data++ };
    $str.encoding.string_grapheme_iterate($str, $callback, $data);
    return $data;
}

sub Parrot_string_byte_length($str) { return $str.strlen }

sub Parrot_string_index($str, $index) { return $str.encoding.char_at_index($str, $index) }
sub Parrot_string_grapheme_index($str, $index) { return $str.encoding.grapheme_at_index($str, $index) }
sub Parrot_string_find_substr($str, $substr) { ... }

sub Parrot_string_copy($src, $dst) { ... }
sub Parrot_string_grapheme_copy ($src, $dst) { 
     if ($src.encoding ~~ $dst.encoding and $src.charset ~~ $dst.charset) {
        return Parrot_string_append($src, $dst);
     }
     my $append_to = sub ($g, $dst) { $dst.encoding.append_grapheme($dst, $g) };
     $src.encoding.string_grapheme_iterate($src, $append_to, $dst);
     return $src;
}
sub Parrot_string_repeat($src, $reps) { ... }
sub Parrot_string_substr($src, $offset, $len) { ... }
sub Parrot_string_grapheme_substr($src, $offset, $len) { ... }
sub Parrot_string_replace($src, $offset, $len, $replacement) { ... }
sub Parrot_string_grapheme_replace($src, $offset, $len, $replacement) { ... }
sub Parrot_string_chopn($src, $count) { ... }
sub Parrot_string_chopn_inplace($str, $count) {
    return $str.encoding.chopn_inplace($str, $count);
}

sub Parrot_string_grapheme_chopn($src, $count) { 
    return Parrot_string_replace($src, Parrot_string_grapheme_length($src) - $count, $count, undef);
}

sub Parrot_debug_string($src) {
    say "String charset: "~$src.charset;
    say "String encoding: "~$src.encoding;
    say "String normalization: "~$src.normalization;
    say "String buffer used: "~$src.bufused;
    say "String length: "~$src.strlen;
    say "String buffer contents: ";
    for ( $src.buffer) { print " ["~$_~"]"; }
    say "";
}

sub Parrot_string_byte_equal($one, $two) {
    if ($one.strlen != $two.strlen) { return 0; }
    for (0 .. $one.strlen-1) {
        if ($one.buffer.[$_] != $two.buffer.[$_]) { 
            return 0 
        }
    }
    return 1;
}
sub Parrot_string_character_equal($one, $two) {
    my $l = Parrot_string_length($one);
    return 0 if $l != Parrot_string_length($two);
    for (0.. $l-1) -> $char {
        my $savechar = 0 + $char; # Work around rakudo weirdness
        my $a = Parrot_string_index($one, $char);
        my $b = Parrot_string_index($two, $savechar);
        return 0 if $a != $b;
    }
    return 1;
}
