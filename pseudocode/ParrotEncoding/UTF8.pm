class ParrotEncoding::UTF8 is ParrotEncoding::Base::Variable {  
    sub _skip($c) {
        if $c <= 191 { return 1 }
        if 191 < $c < 224 { return 2 }
        return 3
    }
    sub _bytes_needed($c) {
        if $c < 0x80 { return 1 }
        if $c < 0x0800 { return 2 }
        return 3;
    }
    sub char_at_byteoffset ($str, $offset is rw) { # Private helper
        if ($offset > $str.strlen) { Parrot_debug_string($str); die "BUG: Asked for a byte "~$offset~" that's not there" };
        my $c = $str.buffer.[$offset++];
        if 191 < $c < 224 {
            if ($offset + 1 > $str.strlen) { die "BUG: UTF8 encoding ran off end of string" }
            $c = (($c +& 31) +< 6) +| ( $str.buffer.[$offset++] +& 63 );
        } elsif $c >= 224 {
            if ($offset + 2 > $str.strlen) { die "BUG: UTF8 encoding ran off end of string" }
            $c = (($c +& 15) +< 12) 
                +| (( $str.buffer.[$offset++] +& 63 ) +< 6);
            $c  +|= $str.buffer.[$offset++] +& 63;
        }
        return $c;
    }

    method append_char($str, $c) {
        $str.bufused += _bytes_needed($c);
        $str.strlen  += _bytes_needed($c);
        if ($c < 0x80) {
            push $str.buffer, $c;
        } elsif ($c < 0x0800) {
            push $str.buffer, $c +> 6 +| 0xc0;
            push $str.buffer, $c +& 0x3f +| 0x80;
        } else {
            push $str.buffer, $c +> 12 +| 0xe0;
            push $str.buffer, $c +> 6 +& 0x3f +| 0x80;
            push $str.buffer, $c +& 0x3f +| 0x80;
        }
    }

    method append_grapheme($str, $g) { 
        for (@($g)) { self.append_char($str, $_) }
    }

    method string_char_iterate ($str, $callback, $parameter) {
        my $index = 0;
        while ($index < $str.bufused) {
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
        while ($index < $str.bufused) {
            my $c = char_at_byteoffset($str, $index);

            # If we're the last character, do the callback and give up
            if ($index >= $str.bufused) { $callback([$c], $parameter); return; }

            # At this point we know there is at least one following character.
            # How many of them are combining?
            my @grapheme = ( $c );
            my $next_char;
            my $nc_index = $index;
            my $end_of_grapheme_sequence = $index;
            while ($nc_index < $str.bufused and
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
