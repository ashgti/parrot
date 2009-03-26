#!/usr/bin/env parrot
# $Id$

=head1 NAME

config.pir - Print a Parrot configuration item

=head1 VERSION

version 0.01

=head1 SYNOPSIS

  ./parrot parrot-config.pir VERSION
  ./parrot parrot-config.pir ccflags
  ./parrot parrot-config.pir --dump

=head1 DESCRIPTION

Print out configuration items.

=head1 AUTHOR

Leopold Toetsch E<lt>lt@toetsch.atE<gt>.

=head1 COPYRIGHT

Copyright (C) 2004-2009, Parrot Foundation.

=cut

.sub _main :main
    .param pmc argv
    .local int argc
    argc = argv
    if argc < 2 goto usage
    .local pmc interp, conf_hash
    .local string key
    .include "iglobals.pasm"
    interp = getinterp
    conf_hash = interp[.IGLOBALS_CONFIG_HASH]
    .local int i
    i = 1
loop:
    key = argv[i]
    if key == '--help' goto usage
    if key == '--dump' goto dump
    $I0 = defined conf_hash[key]
    if $I0 goto ok2
    print " no such key: '"
    print key
    print "'\n"
    end
ok2:
    $S0 = conf_hash[key]
    print $S0
    inc i
    if i < argc goto loop
    print "\n"
    end
dump:
   .local pmc iterator
    iterator = iter conf_hash
iter_loop:
    unless iterator goto iter_end
    shift $S0, iterator
    print $S0
    print " => '"
    $S1 = conf_hash[$S0]
    print $S1
    say "'"
    goto iter_loop
iter_end:
    end
usage:
    $S0 = argv[0]
    printerr $S0
    printerr " [ <config-key> | --dump | --help ]\n"
    exit 1
.end

# Local Variables:
#   mode: pir
#   fill-column: 100
# End:
# vim: expandtab shiftwidth=4 ft=pir:
