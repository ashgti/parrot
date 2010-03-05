#! nqp

class Ops::Emitter is Hash;

=begin

Emitter.

=end

method new(:$ops_file!, :$trans!, :$script!, :$file, :%flags) {
    self<ops_file>  := $ops_file;
    self<trans>     := $trans;
    self<script>    := $script;
    self<file>      := $file;
    self<flags>     := %flags;

    # Preparing various bits.
    my $suffix := $trans.suffix();

    my $base := %flags<core> ?? 'core' !! $file; #FIXME drop .ops
    my $base_ops_stub := $base ~ '_ops' ~ $suffix;
    my $base_ops_h    := $base_ops_stub ~ '.h';


    self<include> := %flags<dir> ~ "parrot/oplib/$base_ops_h";
    self<header>  := "include/{self<include>}";


    self;
};

method ops_file()   { self<ops_file> };
method trans()      { self<trans> };
method script()     { self<script> };
method file()       { self<file> };
method flags()      { self<flags> };


method print_c_header_file() {

}


# vim: expandtab shiftwidth=4 ft=perl6:
