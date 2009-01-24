class ParrotEncoding::ParrotNative is ParrotEncoding::Base::Fixed {
    our $.width = 1;

    method setup($str) { $str.normalization = ParrotNormalization::NFG.new(); }
    method append_grapheme ($str, $g) {
        if (@($g) > 1) {
            my $item;
            $item = $str.normalization.get_grapheme_table_entry(@($g));
            $str.buffer.push($item);
        } else {
            $str.buffer.push(@( $g ));
        }
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
        return $str.normalization.grapheme_table.[-$c - 1];
        # We are allowed to be pally with the normalization internals
        # because NFG is specific to ParrotEncoding.
    }
};
