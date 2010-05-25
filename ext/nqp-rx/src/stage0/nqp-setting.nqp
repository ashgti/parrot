#! nqp

# This file automatically generated by build/gen_setting.pl in the nqp-rx project.

# From src/setting/ResizablePMCArray.pm

#! nqp

=begin

ResizablePMCArray Methods

These methods extend Parrot's ResizablePMCArray type to include
more methods typical of Perl 6 lists and arrays.

=end

module ResizablePMCArray {

    =begin item join
    Return all elements joined by $sep.
    =end item

    method join ($separator) {
        pir::join($separator, self);
    }

    =begin item map
    Return an array with the results of applying C<&code> to
    each element of the invocant.  Note that NQP doesn't have
    a flattening list context, so the number of elements returned
    is exactly the same as the original.
    =end item

    method map (&code) {
        my @mapped;
        for self { @mapped.push( &code($_) ); }
        @mapped;
    }

    =begin item reverse
    Return a reversed copy of the invocant.
    =end item

    method reverse () {
        my @reversed;
        for self { @reversed.unshift($_); }
        @reversed;
    }
}


our sub join ($separator, *@values) { @values.join($separator); }
our sub map (&code, *@values) { @values.map(&code); }
our sub list (*@values) { @values; }

# vim: ft=perl6
# From src/setting/Hash.pm

#! nqp

=begin

Hash methods and functions

These methods extend Parrot's Hash type to include more
methods typical of Perl 6 hashes.

=end

module Hash {

    =begin item delete
    Delete C<$key> from the hash.
    =end item

    method delete($key) {
        Q:PIR {
            $P1 = find_lex '$key'
            delete self[$P1]
        }
    }


    =begin item exists
    Returns true if C<$key> exists in the hash.
    =end item

    method exists($key) {
        Q:PIR {
            $P1 = find_lex '$key'
            $I0 = exists self[$P1]
            %r  = box $I0
        }
    }


    =begin item keys
    Returns a list of all of the keys in the hash.
    =end item

    method keys () {
        my @keys;
        for self { @keys.push($_.key); }
        @keys;
    }


    =begin item kv
    Return a list of key, value, key, value, ...
    =end item

    method kv () {
        my @kv;
        for self { @kv.push($_.key); @kv.push($_.value); }
        @kv;
    }


    =begin item values
    Returns a list of all of the values in the hash.
    =end item

    method values () {
        my @values;
        for self { @values.push($_.value); }
        @values;
    }

}


=begin item hash
Construct a hash from named arguments.
=end item

our sub hash(*%h) { %h }

# vim: ft=perl6
# From src/setting/Regex.pm

#! nqp

=begin

Regex methods and functions

=end

=begin item match
Match C<$text> against C<$regex>.  If the C<$global> flag is
given, then return an array of all non-overlapping matches.
=end item

our sub match ($text, $regex, :$global?) {
    my $match := $text ~~ $regex;
    my @matches;
    if $global {
        while $match {
            @matches.push($match);
            $match := $match.CURSOR.parse($text, :rule($regex), :c($match.to));
        }
    }
    elsif $match {
        @matches.push($match);
    }
    @matches;
}


=begin item subst
Substitute an match of C<$regex> in C<$text> with C<$replacement>,
returning the substituted string.  If C<$global> is given, then
perform the replacement on all matches of C<$text>.
=end item

our sub subst ($text, $regex, $repl, :$global?) {
    my @matches := match($text, $regex, $global);

    my $is_code := pir::isa($repl, 'Sub');
    my $offset  := 0;
    my $result  := pir::new__Ps('StringBuilder');

    for @matches -> $match {
        pir::push($result, pir::substr($text, $offset, $match.from - $offset))
            if $match.from > $offset;
        pir::push($result, $is_code ?? $repl($match) !! $repl);
        $offset := $match.to;
    }

    my $chars := pir::length($text);
    pir::push($result, pir::substr($text, $offset, $chars))
        if $chars > $offset;

    ~$result;
}

# vim: ft=perl6
# From src/setting/IO.pm

#! nqp

=begin

IO Methods and Functions

=end

=begin item slurp
Returns the contents of C<$filename> as a single string.
=end

our sub slurp ($filename) {
    my $handle := pir::open__Pss($file, 'r');
    my $contents := $handle.readall;
    $handle.close();
    $contents;
}


=begin item spew
Write the string value of C<$contents> to C<$filename>.
=end item

our sub spew($filename, $contents) {
    my $handle := pir::open__Pss($filename, 'w');
    $handle.print($contents);
    $handle.close();
}

# vim: ft=perl6

# vim: set ft=perl6 nomodifiable :
