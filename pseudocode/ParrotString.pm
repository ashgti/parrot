use v6;
enum ParrotStringFlags <StringCOW>;
class ParrotString {
    has @.buffer is rw;
    has $.strlen is rw;
    has $.flags is rw;
    has $.bufused is rw;
    has $.hashval is rw;
    has ParrotString::Encoding      $.encoding is rw;
    has ParrotString::Charset       $.charset  is rw;
    has ParrotString::Normalization $.normalization is rw;
};

class ParrotCharset::Unicode {  };
class ParrotCharset::ASCII   {  };
class ParrotCharset::Binary  {  };
class ParrotCharset::SJIS    {  };
class ParrotCharset::EUCJP   {  };

class ParrotEncoding::UTF8   {  };
class ParrotEncoding::UTF16  {  };
class ParrotEncoding::UTF32  {  };
class ParrotEncoding::EBCDIC {  };
class ParrotEncoding::Byte   {
    method string_iterate ($str, $callback, $parameter) {
        for (0..$str.bufused-1) {
            $callback($str.buffer.[$_], $parameter);
        }
    }
};

sub Parrot_string_grapheme_copy ($src, $dst) { ... } 

sub Parrot_string_set($src, $dst) { 
    return Parrot_string_reuse_COW($src, $dst); # Isn't it?
} 

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
    $news.buffer   = split("", $s);
    $news.bufused = $news.strlen = $len || length($s);
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
    # This code written funny to be a bit more C-like
    my $data = 0; my $callback = sub ($char, $data is rw) { $data++ };
    $str.encoding.string_iterate($str, $callback, $data);
    return $data;
}
