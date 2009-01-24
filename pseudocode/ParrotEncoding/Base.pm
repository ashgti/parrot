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
