%{

use constant TRUE => "TRUE";
use constant FALSE => 0;

sub tokenize {
    my $data = shift;

    my @tokens;
    my @types;
    my $state = 0;
    my $depth = 0;
    for my $c (split(//, $data)) {
        if ($state == 0) {
            if ($c eq '\\') {
                $state = 1;
            } elsif ($c eq '{') {
                push @tokens, '{';
                push @types, 'CODE';
                $state = 2;
                $depth = 1;
            } else {
                if ($c =~ /\d/) {
                    push @tokens, $c;
                    push @types, 'NUM';
                } elsif ($c =~ /\w/) {
                    push @tokens, $c;
                    push @types, 'CHAR';
                } elsif ($c =~ /\s/) {
                    # Skip whitespace, unless backslashed
                } else {
                    push @tokens, $c;
                    push @types, $c;
                }
            }
        } elsif ($state == 1) {
            push @tokens, $c;
            push @types, 'CHAR';
            $state = 0;
        } else {
            if ($c eq '{') {
                ++$depth;
            } elsif ($c eq '}') {
                if (--$depth == 0) {
                    $state = 0;
                }
            }
            $tokens[-1] .= $c;
        }
    }

#    for (0..$#tokens) { print STDERR "  $tokens[$_] $types[$_]\n"; }

    return \@tokens, \@types;
}

sub op {
    Regex::Ops::Tree->op(@_);
}

$::paren = 0;

%}

%left '|'
%left SEQUENCE
%nonassoc '(' '[' '<' '>' CHAR '.' RANGE '*' '+'
%nonassoc '?'

%%

rules : rules rule
  { return [ @{ $_[1] }, $_[2] ] }
      |
  { return []; }
;

rule : '&' rulename '=' regex
   { my ($name, $tree) = @_[2,4];
     # This is really not the place for this...
     if ($tree->{name} eq 'scan' && $name ne 'default') {
         $tree = $tree->{args}->[0];
     }
     my $op = op('rule' => [ $name, $tree, 1 + $::paren ]);
     $::paren = 0; # FIXME!
     return $op;
   }
     | regex
   {
     my $op = op('rule' => [ 'default', $_[1], 1 + $::paren ]);
     $::paren = 0; # FIXME!
     return $op;
   }
;

regex : '^' regex1
   { return $_[2]; }
      | regex1
   { return op('scan' => [ $_[1] ]); }
;

regex1 : expr
   { return $_[1]; }
      | expr '$'
   { return op('seq' => [ $_[1], op('atend') ]); }
;

expr : expr '|' expr
   { return op('alternate' => [ $_[1], $_[3] ]); }
     | expr expr %prec SEQUENCE
   { return op('seq' => [ $_[1], $_[2] ]); }
     | CHAR
   { return op('match' => [ ord($_[1]) ]); }
     | charclass
   { return op('classpieces' => [ $_[1] ]); }
     | expr '*'
   { return op('multi_match' => [ 0, -1, TRUE, $_[1] ]); }
     | expr '*' '?'
   { return op('multi_match' => [ 0, -1, FALSE, $_[1] ]); }
     | expr '+'
   { return op('multi_match' => [ 1, -1, TRUE, $_[1] ]); }
     | expr '+' '?'
   { return op('multi_match' => [ 1, -1, FALSE, $_[1] ]); }
     | expr '?'
   { return op('multi_match' => [ 0, 1, TRUE, $_[1] ]); }
     | expr '?' '?'
   { return op('multi_match' => [ 0, 1, FALSE, $_[1] ]); }
     | expr range %prec RANGE
   { return op('multi_match' => [ $_[2]->{min}, $_[2]->{max}, TRUE, $_[1] ]); }
     | expr range '?'
   { return op('multi_match' => [ $_[2]->{min}, $_[2]->{max}, FALSE, $_[1] ]); }
     | '(' { ++$::paren } expr ')'
   { return op('group' => [ $_[3], $_[2] ]) }
     | '(' '?' ':' expr ')'
   { return $_[4]; }
     | '<' rulename '>'
   { return op('call' => [ $_[2], 0 ]) }
     | '<' '?' rulename '>'
   { return op('group', [ op('call' => [ $_[3], 1 ]), ++$::paren ]) }
     | CODE
   { return op('code' => [ $_[1] ]) }
;

rulename : rulename CHAR
   { return $_[1] . $_[2]; }
     |
   { return '' }
;

range : '<' number '.' '.' number '>'
   { return { min => $_[2], max => $_[5] }; }
      | '<' '.' '.' number '>'
   { return { min => 0, max => $_[4] }; }
      | '<' number '>'
   { return { min => $_[2], max => $_[2] }; }
      | '<' number '.' '.' '>'
   { return { min => $_[2], max => -1 }; }
;

number : number NUM { return $_[1] * 10 + $_[2]; }
       | NUM { return $_[1]; }
;

charclass : '[' '^' classpieces ']' { return [ 'neg', $_[3] ]; }
          | '[' '-' classpieces ']' { push @{$_[3]}, '-'; return $_[3]; }
          | '[' classpieces ']' { return $_[2]; }
          | '.' { return [ 'neg', [] ]; }
;

classpieces : classpieces classpiece { push @{$_[1]}, $_[2]; return $_[1]; }
            | { return []; }
;

classpiece : CHAR '-' CHAR
    { return [ $_[1], $_[3] ] }
           | CHAR
    { return $_[1]; }
           | NUM '-' NUM
    { return [ $_[1], $_[3] ] }
           | NUM
    { return $_[1]; }
;

%%
