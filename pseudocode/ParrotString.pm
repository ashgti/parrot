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

class ParrotEncoding::UTF8   {  
    sub _skip($c) {
        if $c <= 191 { return 1 }
        if 191 < $c < 224 { return 2 }
        return 3
    }
    sub char_at_byteoffset ($str, $offset is rw) { # Private helper
        my $c = $str.buffer.[$offset++];
        if 191 < $c < 224 {
            # XXX Guard
            $c = (($c +& 31) +< 6) +| ( $str.buffer.[$offset++] +& 63 );
        } elsif $c >= 224 {
            # XXX Guard
            $c = (($c +& 15) +< 12) 
                +| (( $str.buffer.[$offset++] +& 63 ) +< 6);
            $c  +|= $str.buffer.[$offset++] +& 63;
        }
        return $c;
    }
    method string_char_iterate ($str, $callback, $parameter) {
        my $index = 0;
        while ($index < $str.bufused-1) {
            $callback(char_at_byteoffset($str, $index), $parameter);
        }
    }

    # We're not going to cache this because if it's worth caching it's
    # worth converting to a Parrot native string rather than keeping
    # UTF8. We'll keep it dumb and working and people can optimise later
    method char_at_index($str, $index) { 
        my $i = $index + 0; # work around Rakudo assignment weirdness
        my $offset = 0;
        while $i-- > 0 { $offset += _skip($str.buffer[$offset])  }
        return char_at_byteoffset($str, $offset);
    }
};
class ParrotEncoding::UTF16  {  };
class ParrotEncoding::UTF32  {  };
class ParrotEncoding::EBCDIC {  };
class ParrotEncoding::Byte   {
    method string_char_iterate ($str, $callback, $parameter) {
        for (0..$str.bufused-1) { $callback($str.buffer.[$_], $parameter); }
    }

    method string_grapheme_iterate($str, $callback, $parameter) {
        for (0..$str.bufused-1) { $callback($str.buffer.[$_], $parameter); }
    }

    method char_at_index($str, $index) { return $str.buffer.[$index] }
};


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
    $news.buffer   = map { ord $_ }, split("", $s);
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
    $str.encoding.string_char_iterate($str, $callback, $data);
    return $data;
}

sub Parrot_string_grapheme_length($str) {
    # This code written funny to be a bit more C-like
    my $data = 0; my $callback = sub ($char, $data is rw) { $data++ };
    $str.encoding.string_grapheme_iterate($str, $callback, $data);
    return $data;
}

sub Parrot_string_byte_length($str) { return $str.strlen }

sub Parrot_string_index($str, $index) { return $str.encoding.char_at_index($str, $index) }
sub Parrot_string_grapheme_index($str, $index) { ... }
sub Parrot_string_find_substr($str, $substr) { ... }

sub Parrot_string_copy($src, $dst) { ... }
sub Parrot_string_grapheme_copy ($src, $dst) { ... } 
sub Parrot_string_repeat($src, $reps) { ... }
sub Parrot_string_substr($src, $offset, $len) { ... }
sub Parrot_string_grapheme_substr($src, $offset, $len) { ... }
sub Parrot_string_replace($src, $offset, $len, $replacement) { ... }
sub Parrot_string_grapheme_replace($src, $offset, $len, $replacement) { ... }
sub Parrot_string_chopn($src, $count) { ... }
sub Parrot_string_chopn_inplace($src, $count) { ... }

sub Parrot_string_grapheme_chopn($src, $count) { 
    return Parrot_string_replace($src, Parrot_string_grapheme_length($src) - $count, $count, undef);
}
