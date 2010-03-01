# nqp
class Ops::Compiler is HLL::Compiler;

INIT {
    Ops::Compiler.language('Ops');
    Ops::Compiler.parsegrammar(Ops::Compiler::Grammar);
    Ops::Compiler.parseactions(Ops::Compiler::Actions);
}

=begin

=item C<expand_args(@args)>

Given an argument list, returns a list of all the possible argument
combinations.

=end
sub expand_args(@args) {

    return list() unless +@args;

    my $arg := @args.shift;

    my @var := list($arg<type>);
    if $arg<variant> {
        @var.push($arg<variant>);
    }

    my @list := expand_args(@args);
    unless +@list {
        return @var;
    }

    my @results;
    for @list -> $l {
        for @var -> $v {
            # NQP can't handle it automagically. So wrap $l into list.
            my @l := pir::does__IPS($l, 'array') ?? $l !! list($l);
            @results.push(list($v, |@l));
        }
    }

    @results;
}

# vim: ft=perl6 expandtab shiftwidth=4:
