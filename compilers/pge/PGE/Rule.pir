=head1 NAME

PGE::Rule - base class for grammars and built-in rules

=head1 DESCRIPTION

This implements the base classes for forming grammars, and provides
a number of built-in rules.

=cut

.namespace [ "PGE::Rule" ]

.include "cclass.pasm"
.include "interpinfo.pasm"

.sub "__onload" 
    .local pmc base
    .local pmc p6rule
    base = getclass "PGE::Match"
    $P0 = subclass base, "PGE::Rule"
    .return ()
.end

=head2 Built-in rules

=item C<null()>

Match a null string (always returns true on first match).

=cut

.sub null
    .param pmc mob
    .local pmc target, mfrom, mpos
    $P0 = find_global "PGE::Match", "newfrom"
    (mob, target, mfrom, mpos) = $P0(mob)
    assign mpos, mfrom
    .return (mob)    
.end

=item C<fail()>

Force a backtrack.  (Taken from A05.)

=cut

.sub fail
    .param pmc mob
    $P0 = find_global "PGE::Match", "newfrom"
    mob = $P0(mob)
    .return (mob)
.end

=item C<upper()>

Match a single uppercase character.

=cut

.sub "upper"
    .param pmc mob
    .local string target
    .local pmc mfrom, mpos
    $P0 = find_global "PGE::Match", "newfrom"
    (mob, target, mfrom, mpos) = $P0(mob)
    $I0 = mfrom
    $I1 = is_cclass .CCLASS_UPPERCASE, target, $I0
    unless $I1 goto end
    mpos = mfrom + 1
  end:
    .return (mob)
.end

=item C<lower()>

Match a single lowercase character.

=cut

.sub "lower"
    .param pmc mob
    .local string target
    .local pmc mfrom, mpos
    $P0 = find_global "PGE::Match", "newfrom"
    (mob, target, mfrom, mpos) = $P0(mob)
    $I0 = mfrom
    $I1 = is_cclass .CCLASS_LOWERCASE, target, $I0
    unless $I1 goto end
    mpos = mfrom + 1
  end:
    .return (mob)
.end

=item C<alpha()>

Match a single alphabetic character.

=cut

.sub "alpha"
    .param pmc mob
    .local string target
    .local pmc mfrom, mpos
    $P0 = find_global "PGE::Match", "newfrom"
    (mob, target, mfrom, mpos) = $P0(mob)
    $I0 = mfrom
    $I1 = is_cclass .CCLASS_ALPHABETIC, target, $I0
    unless $I1 goto end
    mpos = mfrom + 1
  end:
    .return (mob)
.end

=item C<digit()>

Match a single digit.

=cut

.sub "digit"
    .param pmc mob
    .local string target
    .local pmc mfrom, mpos
    $P0 = find_global "PGE::Match", "newfrom"
    (mob, target, mfrom, mpos) = $P0(mob)
    $I0 = mfrom
    $I1 = is_cclass .CCLASS_NUMERIC, target, $I0
    unless $I1 goto end
    mpos = mfrom + 1
  end:
    .return (mob)
.end

=item C<xdigit()>

Match a single alphanumeric character.

=cut

.sub "xdigit"
    .param pmc mob
    .local string target
    .local pmc mfrom, mpos
    $P0 = find_global "PGE::Match", "newfrom"
    (mob, target, mfrom, mpos) = $P0(mob)
    $I0 = mfrom
    $I1 = is_cclass .CCLASS_HEXADECIMAL, target, $I0
    unless $I1 goto end
    mpos = mfrom + 1
  end:
    .return (mob)
.end

=item C<space()>

Match a single whitespace character.

=cut

.sub "space"
    .param pmc mob
    .local string target
    .local pmc mfrom, mpos
    $P0 = find_global "PGE::Match", "newfrom"
    (mob, target, mfrom, mpos) = $P0(mob)
    $I0 = mfrom
    $I1 = is_cclass .CCLASS_WHITESPACE, target, $I0
    unless $I1 goto end
    mpos = mfrom + 1
  end:
    .return (mob)
.end

=item C<print()>

Match a single printable character.

=cut

.sub "print"
    .param pmc mob
    .local string target
    .local pmc mfrom, mpos
    $P0 = find_global "PGE::Match", "newfrom"
    (mob, target, mfrom, mpos) = $P0(mob)
    $I0 = mfrom
    $I1 = is_cclass .CCLASS_PRINTING, target, $I0
    unless $I1 goto end
    mpos = mfrom + 1
  end:
    .return (mob)
.end

=item C<graph()>

Match a single "graphical" character.

=cut

.sub "graph"
    .param pmc mob
    .local string target
    .local pmc mfrom, mpos
    $P0 = find_global "PGE::Match", "newfrom"
    (mob, target, mfrom, mpos) = $P0(mob)
    $I0 = mfrom
    $I1 = is_cclass .CCLASS_GRAPHICAL, target, $I0
    unless $I1 goto end
    mpos = mfrom + 1
  end:
    .return (mob)
.end

=item C<blank()>

Match a single "blank" character.

=cut

.sub "blank"
    .param pmc mob
    .local string target
    .local pmc mfrom, mpos
    $P0 = find_global "PGE::Match", "newfrom"
    (mob, target, mfrom, mpos) = $P0(mob)
    $I0 = mfrom
    $I1 = is_cclass .CCLASS_BLANK, target, $I0
    unless $I1 goto end
    mpos = mfrom + 1
  end:
    .return (mob)
.end

=item C<cntrl()>

Match a single "control" character.

=cut

.sub "cntrl"
    .param pmc mob
    .local string target
    .local pmc mfrom, mpos
    $P0 = find_global "PGE::Match", "newfrom"
    (mob, target, mfrom, mpos) = $P0(mob)
    $I0 = mfrom
    $I1 = is_cclass .CCLASS_CONTROL, target, $I0
    unless $I1 goto end
    mpos = mfrom + 1
  end:
    .return (mob)
.end

=item C<punct()>

Match a single punctuation character.

=cut

.sub "punct"
    .param pmc mob
    .local string target
    .local pmc mfrom, mpos
    $P0 = find_global "PGE::Match", "newfrom"
    (mob, target, mfrom, mpos) = $P0(mob)
    $I0 = mfrom
    $I1 = is_cclass .CCLASS_PUNCTUATION, target, $I0
    unless $I1 goto end
    mpos = mfrom + 1
  end:
    .return (mob)
.end

=item C<alnum()>

Match a single alphanumeric character.

=cut

.sub "alnum"
    .param pmc mob
    .local string target
    .local pmc mfrom, mpos
    $P0 = find_global "PGE::Match", "newfrom"
    (mob, target, mfrom, mpos) = $P0(mob)
    $I0 = mfrom
    $I1 = is_cclass .CCLASS_ALPHANUMERIC, target, $I0
    unless $I1 goto end
    mpos = mfrom + 1
  end:
    .return (mob)
.end

=item C<sp()>

Match a single space character.  (Taken from E05.)

=cut

.sub "sp"
    .param pmc mob
    .local string target
    .local pmc mfrom, mpos
    $P0 = find_global "PGE::Match", "newfrom"
    (mob, target, mfrom, mpos) = $P0(mob)
    $I0 = mfrom
    $S0 = substr target, $I0, 1
    if $S0 != " " goto end
    mpos = mfrom + 1
  end:
    .return (mob)
.end

=item C<lt()>

Match a single left angle bracket.  (Taken from E05.)

=cut

.sub "lt"
    .param pmc mob
    .local string target
    .local pmc mfrom, mpos
    $P0 = find_global "PGE::Match", "newfrom"
    (mob, target, mfrom, mpos) = $P0(mob)
    $I0 = mfrom
    $S0 = substr target, $I0, 1
    if $S0 != "<" goto end
    mpos = mfrom + 1
  end:
    .return (mob)
.end

=item C<gt()>

Match a single right angle bracket. (Taken from E05.)

=cut

.sub "gt"
    .param pmc mob
    .local string target
    .local pmc mfrom, mpos
    $P0 = find_global "PGE::Match", "newfrom"
    (mob, target, mfrom, mpos) = $P0(mob)
    $I0 = mfrom
    $S0 = substr target, $I0, 1
    if $S0 != ">" goto end
    mpos = mfrom + 1
  end:
    .return (mob)
.end

=item C<dot()>

Match a single dot ('.').  (Taken from E05.)

=cut

.sub "dot"
    .param pmc mob
    .local string target
    .local pmc mfrom, mpos
    $P0 = find_global "PGE::Match", "newfrom"
    (mob, target, mfrom, mpos) = $P0(mob)
    $I0 = mfrom
    $S0 = substr target, $I0, 1
    if $S0 != "." goto end
    mpos = mfrom + 1
  end:
    .return (mob)
.end

=item C<ws()>

Match whitespace between tokens.

=cut

.sub "ws"
    .param pmc mob
    .local string target
    .local pmc mfrom, mpos
    .local int rep, pos, lastpos
    .const .Sub corou = "ws_corou"
    $P0 = find_global "PGE::Match", "newfrom"
    (mob, target, mfrom, mpos) = $P0(mob)
    lastpos = length target
    pos = mfrom
    if pos >= lastpos goto end
    if pos < 1 goto ws_succ
    $I0 = is_cclass .CCLASS_WORD, target, pos
    if $I0 == 0 goto ws_succ
    $I1 = pos - 1
    $I0 = is_cclass .CCLASS_WORD, target, $I1
    if $I0 == 0 goto ws_succ
    .return (mob)
  ws_succ:
    $I0 = find_not_cclass .CCLASS_WHITESPACE, target, pos, lastpos
    mpos = $I0
    if $I0 == pos goto end
    $P0 = corou
    $P0 = clone $P0
    setattribute mob, "PGE::Match\x0&:corou", $P0
    $P0(mob, mpos)
  end:
    .return (mob)
.end
.sub "ws_corou" :anon
    .param pmc mob
    .param pmc mpos
  loop:
    .yield (mob)
    dec mpos
    goto loop
.end


=head1 AUTHOR

Patrick Michaud (pmichaud@pobox.com) is the author and maintainer.
Patches and suggestions should be sent to the Perl 6 compiler list
(perl6-compiler@perl.org).

=cut
