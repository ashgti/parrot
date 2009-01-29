=head1 Title

PGE::Hs - Match and display PGE rules as Haskell expressions

=head1 SYNOPSIS

(You need to run C<make PGE-Hs.pbc> in F<compilers/pge> first.)

    .sub _main
        load_bytecode "PGE.pbc"
        load_bytecode "PGE/Hs.pir"
        $P0 = get_hll_global ["PGE";"Hs"], "match"
        $S0 = $P0("Hello", "(...)*$")
        print $S0   # PGE_Match 2 5 [PGE_Array [PGE_Match 2 5 [] []]] []
    .end

=head1 DESCRIPTION

The Haskell-side data structure is defined thus:

    data MatchPGE
        = PGE_Match Int Int [MatchPGE] [(String, MatchPGE)]
        | PGE_Array [MatchPGE]
        | PGE_String String
        | PGE_Fail
        deriving (Show, Eq, Ord, Read)

This F<PGE-Hs.pbc> is built separately (not by default).  The reason is
because it's intended to be bundled with Pugs, so as to make Pugs usable
with vanilla Parrot from 0.2.0 on, using either an external F<parrot>
executable, or a linked F<libparrot>.

In external F<parrot> mode, Parrot's include path looks into the
F<.pbc> files inside the library tree first, then look into the current
directory, F<.>.  Hence this file includes, rather than loads, the
F<PGE.pbc> library, because if Pugs is shipped with its own copy
of F<PGE.pbc>, Parrot would ignore that file and prefer to load
the one in the Parrot tree instead.

Granted, it is possible to pass in Pugs's own library path into an
environment variable (maybe C<PARROT_LIBS>?), but as this was not in
the 0.3.0 release, I (audreyt) decided to take the easy route. :-)

=head1 CAVEATS

This is an initial sketch.  The dump format may change, and the
whole thing may be taken out or refactored away at any moment.

=cut

.namespace [ "PGE";"Hs" ]

.const string PGE_FAIL = "PGE_Fail"

.sub "__onload" :load
    .local pmc load
    $P0 = get_class ['PGE';'Match']
.end

.sub "add_rule"
    .param string name
    .param string pattern
    .param pmc adverbs :slurpy :named
    .local pmc p6rule_compile, rulesub

    p6rule_compile = compreg 'PGE::Perl6Regex'
    null rulesub

    # adverbs['grammar'] = 'PGE::Grammar'
    # adverbs['name'] = name
    rulesub = p6rule_compile(pattern, adverbs :named :flat)

    $I0 = exists adverbs["grammar"]
    if $I0 goto done
    $P0 = get_class ["PGE";"Grammar"]
    if null $P0 goto done
    addmethod $P0, name, rulesub

  done:
    .return (name)
.end

.sub "match"
    .param string x
    .param string pattern
    .param pmc adverbs :slurpy :named
    .local string out, tmps
    .local pmc rulesub
    .local pmc match
    .local pmc p6rule_compile
    .local pmc capt

    p6rule_compile = compreg 'PGE::Perl6Regex'
    null rulesub

    push_eh match_error
    rulesub = p6rule_compile(pattern, adverbs :named :flat)
    match = rulesub(x, 'grammar' => 'PGE::Grammar')

  match_result:
    unless match goto match_fail
    tmps = match."dump_hs"()
    out .= tmps
    goto end_match

  match_fail:
    out = PGE_FAIL
    goto end_match

  match_error:
    get_results "0, 0", match, out

  end_match:
    out .= "\n"

    .return (out)
.end

.sub unescape
    .param string str
    .local string ret, tmp
    .local int i, j

    ret = ""
    j = length str
    if j == 0 goto END
    i = 0

LOOP:
    substr tmp, str, i, 1
    inc i
    if i >= j goto FIN

    eq tmp, "\\", ESC
    concat ret, tmp
    goto LOOP

ESC:
    substr tmp, str, i, 1
    inc i
    eq tmp, "n", LF
    concat ret, tmp
    goto UNESC
LF:
    concat ret, "\n"
UNESC:
    if i >= j goto END
    goto LOOP

FIN:
    concat ret, tmp
END:
    .return(ret)
.end

.namespace [ "PGE";"Match" ]

.sub "dump_hs" :method
    .local string out
    .local int spi, spc
    .local int ari, arc
    .local int tmpi, cond
    .local string tmps, key
    .local pmc capt, iter, subelm, elm

    out = ""

  start:
    out .= "PGE_Match "
    tmpi = self."from"()
    tmps = tmpi
    out .= tmps
    out .= " "
    tmpi = self."to"()
    tmps = tmpi
    out .= tmps
    out .= " ["

  subpats:
    capt = self.'list'()
    unless capt goto subrules
    spi = 0
    spc = elements capt
    goto subpats_body
  subpats_loop:
    unless spi < spc goto subrules
    out .= ", "
  subpats_body:
    cond = defined capt[spi]
    unless cond goto subpats_fail
    elm = capt[spi]
    bsr dumper
    inc spi
    goto subpats_loop
  subpats_fail:
    out .= PGE_FAIL
    inc spi
    goto subpats_loop

  subrules:
    out .= "] ["
    capt = self.'hash'()
    if_null capt, end
    iter = new 'Iterator', capt
    iter = 0
    unless iter goto end
  subrules_body:
    key = shift iter
    cond = defined capt[key]
    unless cond goto subrules_fail
    elm = capt[key]
    out .= '("'
    tmps = escape key
    out .= tmps
    out .= '", '
    bsr dumper
    out .= ")"
    unless iter goto end
    out .= ", "
    goto subrules_body
  subrules_fail:
    out .= PGE_FAIL
    key = shift iter
    unless iter goto end
    goto subrules_body

  dumper:
    $I0 = does elm, "array"
    if $I0 goto dumper_array
    $I0 = can elm, "dump_hs"
    unless $I0 goto dumper_string
    tmps = elm."dump_hs"()
    out .= tmps
    ret
  dumper_string:
    $S0 = elm
    tmps = escape $S0
    out .= 'PGE_String "'
    out .= tmps
    out .= '"'
    ret
  dumper_fail:
    out .= PGE_FAIL
    ret
  dumper_done:
    out .= "]"
    ret
  dumper_array:
    ari = 0
    arc = elements elm
    out .= "PGE_Array ["
    unless ari < arc goto dumper_done
    goto dumper_array_body
  dumper_array_loop:
    unless ari < arc goto dumper_done
    out .= ", "
  dumper_array_body:
    subelm = elm[ari]
    tmps = subelm."dump_hs"()
    out .= tmps
    inc ari
    goto dumper_array_loop

  end:
    out .= "]"
    .return (out)
.end

# Local Variables:
#   mode: pir
#   fill-column: 100
# End:
# vim: expandtab shiftwidth=4 ft=pir:
