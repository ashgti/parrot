## $Id$

=head1 TITLE

Str - JS Str class and related functions

=head1 DESCRIPTION

This file sets up the C<JSStr> PMC type (from F<src/pmc/jsstr.pmc>)
as the JS C<Str> class.

=head1 Methods

=over 4

=cut

.namespace ['Str']

.include 'cclass.pasm'

.sub 'onload' :anon :init :load
    .local pmc jsmeta, strproto
    jsmeta = get_hll_global ['JSObject'], '$!JSMETA'
    strproto = jsmeta.'new_class'('Str', 'parent'=>'JS6Str Any')
    strproto.'!IMMUTABLE'()
    jsmeta.'register'('JS6Str', 'parent'=>strproto, 'protoobject'=>strproto)
    jsmeta.'register'('String', 'parent'=>strproto, 'protoobject'=>strproto)

    $P0 = get_hll_namespace ['Str']
    '!EXPORT'('sprintf,reverse', 'from'=>$P0)
.end


## special method to cast Parrot String into Rakudo Str.
.namespace ['String']
.sub 'Scalar' :method
    $P0 = new 'Str'
    assign $P0, self
    copy self, $P0
    .return (self)
.end


.namespace ['Str']
.sub 'ACCEPTS' :method
    .param string topic
    .tailcall 'infix:eq'(topic, self)
.end


.sub 'reverse' :method :multi('String')
    .local pmc retv

    retv = self.'split'('')
    retv = retv.'reverse'()
    retv = retv.'join'('')

    .return(retv)
.end



=item js()

Returns a js representation of the Str.

=cut

.sub 'js' :method
    .local string str, result
    str = self
    result = '"'
    .local int pos
    pos = 0
    .local pmc arr
    arr = new 'ResizablePMCArray'
  loop:
    .local string ch
    ch = substr str, pos, 1
    if ch == '' goto done
    if ch == ' ' goto loop_ch
    ##  check for special escapes
    $I0 = index  "$ @ % & { \b \n \r \t \\ \"", ch
    if $I0 < 0 goto loop_nonprint
    ch = substr  "\\$\\@\\%\\&\\{\\b\\n\\r\\t\\\\\\\"", $I0, 2
    goto loop_ch
  loop_nonprint:
    $I0 = is_cclass .CCLASS_PRINTING, ch, 0
    if $I0 goto loop_ch
    $I0 = ord ch
    arr[0] = $I0
    ch = sprintf '\x[%x]', arr
  loop_ch:
    result .= ch
    inc pos
    goto loop
  done:
    result .= '"'
    .return (result)
.end


=item sprintf( *@args )

=cut

.sub 'sprintf' :method
    .param pmc args            :slurpy
    args.'!flatten'()
    $P0 = new 'Str'
    sprintf $P0, self, args
    .return ($P0)
.end


=item WHICH()

Returns the identify value.

=cut

.sub 'WHICH' :method
    $S0 = self
    .return ($S0)
.end


=back

=head1 Functions

=over 4

=cut

.namespace []

.include 'cclass.pasm'


=item infix:===

Overridden for Str.

=cut

.namespace []
.sub 'infix:===' :multi(String,String)
    .param string a
    .param string b
    .tailcall 'infix:eq'(a, b)
.end


=back

=head2 TODO Functions

=over 4

=item p5chop

 our Char multi P5emul::Str::p5chop ( Str  $string is rw )
 our Char multi P5emul::Str::p5chop ( Str *@strings = ($+_) is rw )

Trims the last character from C<$string>, and returns it. Called with a
list, it chops each item in turn, and returns the last character
chopped.

=item p5chomp

 our Int multi P5emul::Str::p5chomp ( Str  $string is rw )
 our Int multi P5emul::Str::p5chomp ( Str *@strings = ($+_) is rw )

Related to C<p5chop>, only removes trailing chars that match C</\n/>. In
either case, it returns the number of chars removed.

=item chomp

 our Str method Str::chomp ( Str $string: )

Returns string with newline removed from the end.  An arbitrary
terminator can be removed if the input filehandle has marked the
string for where the "newline" begins.  (Presumably this is stored
as a property of the string.)  Otherwise a standard newline is removed.

Note: Most users should just let their I/O handles autochomp instead.
(Autochomping is the default.)

=item length

This word is banned in JS.  You must specify units.

=item index

Needs to be in terms of StrPos, not Int.

=item pack

=item pos

=item quotemeta

=item rindex

Needs to be in terms of StrPos, not Int.

=item sprintf

=item unpack

=item vec

Should replace vec with declared arrays of bit, uint2, uint4, etc.

=item words

 our List multi Str::words ( Rule $matcher = /\S+/,  Str $input = $+_, Int $limit = inf )
 our List multi Str::words ( Str $input : Rule $matcher = /\S+/, Int $limit = inf )

=back

=cut

# Local Variables:
#   mode: pir
#   fill-column: 100
# End:
# vim: expandtab shiftwidth=4 ft=pir:
