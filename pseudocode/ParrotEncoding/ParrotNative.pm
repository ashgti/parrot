class ParrotEncoding::ParrotNative {
    our $.width = 1;

    method setup($str) { $str.normalization = ParrotNormalization::NFG.new(); }
    method string_grapheme_iterate($str, $callback, $parameter) {
        for (0..$str.bufused-1) {
            $callback(self.grapheme_at_index($str,$_), $parameter); 
        }
    }
    method string_length($str) { 
        # We're not really all that fixed - borrow this from the
        # variable one. You could optimize this by checking if there's
        # any entries in the grapheme table.
        my $data = 0; 
        my $callback = sub ($char, $data is rw) { $data++ };
        $str.encoding.string_char_iterate($str, $callback, $data);
        return $data;
    }
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
            my $grapheme = self.grapheme_at_index($str, $_);
            for (@( $grapheme )) {
                $callback($_, $parameter); 
            }
        }
    }

    method char_at_index($str, $index) { 
        # We need to look inside each grapheme, since NFG stores individual
        # graphemes and graphemes are composed of multiple characters - 
        # this could be improved with caching later but we will 
        # do it the slow stupid way for now 
        my $i = $index;
        my $grapheme_index = 0;
        my $c;
        while ($i >= 0) {
            my $g = self.grapheme_at_index($str, $grapheme_index++);
            #say "i is "~$i~" and grapheme is "~$g.perl;
            for (@( $g )) {
                #say " Char is "~$_~" and i is "~$i;
                return $_ if $i == 0;
                $i--;
            }
        }
        Parrot_debug_string($str);
        die "char_at_index walked off the end of the string";
    }

    method grapheme_at_index($str, $index) {
        if (!$str.normalization) { 
            $str.charset.normalize($str, ParrotNormalization::NFG);
        }
        die $index~" Read off end of buffer" if $index > $str.buffer - 1;
        my $c = $str.buffer[$index];
        if $c >= 0 { return [ $c ]; }
        return $str.normalization.grapheme_table.[-$c - 1];
        # We are allowed to be pally with the normalization internals
        # because NFG is specific to ParrotEncoding.
    }
};
