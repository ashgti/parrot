class ParrotEncoding::Base::Fixed {
    our $.width;
    method setup($str) { }
    method string_length($str) { return $str.strlen / $str.encoding.width; }

    method string_char_iterate($str, $callback, $parameter) {
        for (0..self.string_length($str)-1) { 
            $callback(self.char_at_index($str,$_), $parameter); 
        }
    }

    # We assume in the base case that grapheme==char, which is true for
    # legacy, non-Unicode fixed width formats. Unicode fixed width
    # formats that care about graphemes can override.
   
    method grapheme_at_index($str, $index) { 
        return [ self.char_at_index($str, $index) ]; 
    }
    method string_grapheme_iterate($str, $callback, $parameter) {
        for (0..self.string_length($str)-1) { 
            $callback($str.encoding.grapheme_at_index($str,$_), $parameter); 
        }
    }

    method chopn_inplace($str, $n) { $str.strlen -= $n * $.width }
}

class ParrotEncoding::Base::Variable {
    method setup($str) { }
    method string_length($str) {
        # This code written funny to be a bit more C-like
        my $data = 0; 
        my $callback = sub ($char, $data is rw) { $data++ };
        $str.encoding.string_char_iterate($str, $callback, $data);
        return $data;
    }
}

class ParrotEncoding::UTF8 is ParrotEncoding::Base::Variable {  
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
    method string_grapheme_iterate ($str, $callback, $parameter) {
        if ($str.charset !~~ ParrotCharset::Unicode) {
            # Although why you'd store non-Unicode in UTF8 is beyond me
            my $to_unicode = sub ($c, $subcallback) {
                $subcallback.[0].( [ $str.charset.to_unicode($c) ], $subcallback.[1]);
            };
            self.string_char_iterate($str, $to_unicode, [ $callback, $parameter ]);
        }
        # Collect characters into graphemes in a roughly O(n) way...
        my $index = 0;
        while ($index < $str.bufused-1) {
            my $c = char_at_byteoffset($str, $index);

            # If we're the last character, do the callback and give up
            if ($index >= $str.bufused) { $callback([$c], $parameter); return; }

            # At this point we know there is at least one following character.
            # How many of them are combining?
            my @grapheme = ( $c );
            my $next_char;
            my $nc_index = $index;
            my $end_of_grapheme_sequence = $index;
            while ($nc_index <= $str.bufused and
                   $next_char = char_at_byteoffset($str, $nc_index)
                   and ParrotCharset::Unicode::is_combining($next_char)) {
                   $end_of_grapheme_sequence = $nc_index;
                   push @grapheme, @([ $next_char ]); # Work around horrible push/copy bug
           }
           $callback([@grapheme], $parameter);
           $index = $end_of_grapheme_sequence;
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

class ParrotEncoding::ParrotNative is ParrotEncoding::Base::Fixed {
    our $.width = 1;

    method setup($str) { $str.normalization = ParrotNormalization::NFG.new(); }
    method append_grapheme ($str, $g) {
        my $item;
        if (@($g) > 1) {
            $item = $str.normalization.get_grapheme_table_entry(@($g));
        } else {
            ($item) = @($g);
        }
        $str.buffer.push($item);
        $str.bufused++;
        $str.strlen++;
    }

    method string_char_iterate ($str, $callback, $parameter) {
        for (0..$str.bufused-1) { 
            my $grapheme = grapheme_at_index($str, $_);
            for (@( $grapheme )) {
                $callback($str.buffer.[$_], $parameter); 
            }
        }
    }

    method char_at_index($str, $index) { 
        # We need to look inside each grapheme, since NFG stores individual
        # graphemes and graphemes are composed of multiple characters - 
        # this could be improved with caching later but we will 
        ...
    }

    method grapheme_at_index($str, $index) {
        if (!$str.normalization) { 
            $str.charset.normalize($str, ParrotNormalization::NFG);
        }
        my $c = $str.buffer[$index];
        if $c >= 0 { return [ $c ]; }
        return $str.normalization.grapheme_table.[-$c];
        # We are allowed to be pally with the normalization internals
        # because NFG is specific to ParrotEncoding.
    }
};

class ParrotEncoding::Byte is ParrotEncoding::Base::Fixed {
    our $.width = 1;
    method char_at_index($str, $index) { return $str.buffer[$index]; }
};
