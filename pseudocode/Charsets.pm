class ParrotCharset::Base {
    method normalize($str, $nf) { return } # Normalization is a Unicode thing
}

class ParrotCharset::Unicode { 

    sub is_combining($x) {
        return 0x0300 <= $x <= 0x36f    || 0x0483 <= $x <= 0x486    ||
               0x0488 <= $x <= 0x489    || 0x135F <= $x <= 0x135f   ||
               0x1DC0 <= $x <= 0x1dc3   || 0x20D0 <= $x <= 0x20eb   ||
               0x3099 <= $x <= 0x309a   || 0xFE20 <= $x <= 0xfe23   ||
               0x1D165 <= $x <= 0x1d169 || 0x1D16D <= $x <= 0x1d172 ||
               0x1D17B <= $x <= 0x1d182 || 0x1D185 <= $x <= 0x1d18b ||
               0x1D1AA <= $x <= 0x1d1ad || 0x1D242 <= $x <= 0x1D244
    }

    method to_unicode($c) { return $c }
    method normalize($str, $nf) {
        return if $str.normalization and $str.normalization ~~ $nf;
        # You might think that normalization requires a complicated
        # dispatch based on what form we currently have and what form we
        # want, but it's easier than that: the forms are hierarchical;
        # to go to NFC you have to go through NFD, and to go to NFG you
        # have to go through NFC. So...
        if (!$str.normalization or $str.normalization !~~ Normalization::NFD) {
            # Everyone starts in fully decomposed form (NFD)
            # This is code-heavy and we'll do it in ICU in Parrot.
            # Here, we just pretend we've already done it.
            $str.normalization = ParrotNormalization::NFD;
            return if $nf ~~ ParrotNormalization::NFD;
        }

        # Then from there convert to fully composed form (NFC)
        # Equally just pretend we've already done it.
        $str.normalization = ParrotNormalization::NFC;
        return if $str.normalization and $str.normalization ~~ $nf;

        # Now we do the NFC->NFG conversion
        my @nfc = $str.buffer;
        $str.normalization = ParrotNormalization::NFG.new(); # Beware!
        my @nfg;
        my $len = $str.strlen;
        my $offset = 0;
        my $count = 0;
        while $offset < $len {
            # Find end of current grapheme sequence
            my $e = 0+$offset; 
            $e++ while $e+1 <= $len && is_combining(@nfc[$e+1]);
            # Current grapheme sequence runs from $offset to $e.
            if ($e - $offset) == 0 { # Simple case first
                push @nfg, @nfc[$offset];
           } else {
                push @nfg, $str.normalization.get_grapheme_table_entry(@nfg[$offset..$e], $e-$offset)
           }
           $offset = $e + 1;
           $count++;
        }
        $str.buffer = @nfg;
        $str.strlen = $count;
        return if $str.normalization ~~ $nf;
        die "We wanted "~$nf~" and we ended up with "~$str.normalization;
    }
};
class ParrotCharset::ASCII   {
    method to_unicode($c) { return $c }
};
class ParrotCharset::Binary  {  };
class ParrotCharset::SJIS    {  };
class ParrotCharset::EUCJP   {  };

