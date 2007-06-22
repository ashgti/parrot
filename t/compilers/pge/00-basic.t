#!./parrot
# Copyright (C) 2006-2007, The Perl Foundation.

=head1 NAME

t/compilers/pge/00-grammar.t - test some simple grammars

=head1 SYNOPSIS

% prove t/compilers/pge/00-grammar.t

=head1 DESCRIPTION

Test the basic testing library.

=cut

.sub main :main
    load_bytecode 'Test/Builder.pir'

    .local pmc test

    test = new 'Test::Builder'
    test.'plan'(1)
    test.'ok'(1,'Basic passing')
.end

# vi: ft=pir
