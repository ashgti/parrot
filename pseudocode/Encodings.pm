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
class ParrotEncoding::ParrotNative {

    method string_char_iterate ($str, $callback, $parameter) {
        for (0..$str.bufused-1) { $callback($str.buffer.[$_], $parameter); }
    }

    method string_grapheme_iterate($str, $callback, $parameter) {
        for (0..$str.bufused-1) { $callback($str.buffer.[$_], $parameter); }
    }

    method char_at_index($str, $index) { return $str.buffer.[$index] }

    method grapheme_at_index($str, $index) {
        if (!$str.normalization) { 
            $str.charset.normalize($str, ParrotNormalization::NFG);
            return $str.buffer.[$index]
        }
        return $str.normalization.grapheme_at_index($str, $index);
    }
};
class ParrotEncoding::Byte is ParrotEncoding::ParrotNative; # Just a bit thinner

