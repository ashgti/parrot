#! perl -w

use Parrot::Test tests => 2;

TODO: {
    local $TODO="t/src doesn't work on Windows" if $^O =~ /Win32/;
    $TODO=$TODO;  #warnings

    c_output_is(<<'CODE', <<'OUTPUT', "hello world");
        #include <stdio.h>

        int main(int argc, char* argv[]) {
                printf("Hello, World!\n");
              exit(0);
        }
CODE
Hello, World!
OUTPUT

    c_output_is(<<'CODE', <<'OUTPUT', "direct internal_exception call");
        #include <parrot/parrot.h>
        #include <parrot/exceptions.h>

        int main(int argc, char* argv[]) {
                internal_exception(0, "Blow'd Up(tm)\n");
        }
CODE
Blow'd Up(tm)
OUTPUT
}
1;
