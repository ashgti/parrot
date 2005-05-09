=head1 Title

PGE::Hs - Match and display PGE rules as Haskell expressions

=head1 SYNOPSIS

    .sub _main
        load_bytecode "PGE.pbc"
        $P0 = find_global "PGE::Hs", "match"
        $S0 = $P0("Hello\n", "H(.)llo(.)")
        print $S0   # Just ((0, 2), [(0, 6), (1, 2), (5, 6)])
    .end

=cut

.namespace [ "PGE::Hs" ]

.sub "__onload" 
.end

.sub "match" method
    .param string x
    .param string pattern
    .local pmc rulesub
    .local pmc match
    .local pmc p6rule_compile
    .local pmc capt
    .local string out

    find_global p6rule_compile, "PGE", "p6rule"
    null rulesub

    rulesub = p6rule_compile(pattern)
    match = rulesub(x)
  match_result:
    unless match goto match_fail
    out = "Just ((0, "
    $I0 = match
    capt = getattribute match, "PGE::Match\x0@:capt"
    isnull capt, match_null
    $I0 = elements capt
    $S0 = $I0
    concat out, $S0
    goto match_dump
  match_null:
    concat out, "0"
  match_dump:
    concat out, "), ["
    $S0 = match."dump_hs"()
    concat out, $S0
    concat out, "])\n"
    goto end_match
  match_fail:
    out = "Nothing\n"
    goto end_match
  end_match:
    .return (out)
.end

.namespace [ "PGE::Match" ]

.sub "dump_hs" method
    .local pmc capt
    .local int spi, spc
    .local pmc iter
    .local string out

    out = ""

    unless argcS < 3 goto start
    unless argcS < 2 goto start
  start:
    $I0 = self
    unless $I0 goto subpats
    concat out, "("
    $I0 = self."from"()
    $S0 = $I0
    concat out, $S0
    concat out, ", "
    $I0 = self."to"()
    $S0 = $I0
    concat out, $S0
    concat out, ")"

  subpats:
    $I0 = self
    capt = getattribute self, "PGE::Match\x0@:capt"
    isnull capt, subrules
    spi = 0
    spc = elements capt
  subpats_1:
    unless spi < spc goto subrules
    $S0 = spi
    $I0 = defined capt[spi]
    unless $I0 goto subpats_2
    $P0 = capt[spi]
    bsr dumper
  subpats_2:
    inc spi
    goto subpats_1

  subrules:
    capt = getattribute self, "PGE::Match\x0%:capt"
    isnull capt, end
    iter = new Iterator, capt
    iter = 0
  subrules_1:
    unless iter goto end
    $S0 = shift iter
    $I0 = defined capt[$S0]
    unless $I0 goto subrules_1
    $P0 = capt[$S0]
    bsr dumper
    goto subrules_1

  dumper:
    $I0 = 0
    $I1 = elements $P0
    unless $I0 < $I1 goto dumper_1
    $P1 = getprop "isarray", $P0
    if $P1 goto dumper_2
    $P1 = $P0[-1]
    concat out, ", "
    $S0 = $P1."dump_hs"()
    concat out, $S0
  dumper_1:
    ret
  dumper_2:
    unless $I0 < $I1 goto dumper_1
    $P1 = $P0[$I0]
    concat out, ", "
    $S0 = $P1."dump_hs"()
    concat out, $S0
    inc $I0
    goto dumper_2
  end:
    .return (out)
.end
