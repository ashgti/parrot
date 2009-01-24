use ParrotEncoding::Base;

class ParrotEncoding::UTF16  {  };
class ParrotEncoding::UTF32  {  };
class ParrotEncoding::EBCDIC {  };
use ParrotEncoding::UTF8;
use ParrotEncoding::ParrotNative;

class ParrotEncoding::Byte is ParrotEncoding::Base::Fixed {
    our $.width = 1;
    method char_at_index($str, $index) { return $str.buffer[$index]; }
};
