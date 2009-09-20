#! perl
# Copyright (C) 2007-2009, Parrot Foundation.
# $Id$

use strict;
use warnings;
use 5.008;

use Cwd;
use Getopt::Long;
use File::Spec::Functions;

use Test::More tests => 33;

=head1 NAME

tools/install/smoke_languages.pl - checks some languages in install directory

=head1 SYNOPSIS

parrot in install tree

    % cd /usr/local/parrot-$version
    % perl tools/install/smoke_languages.pl

parrot in build tree

    % perl tools/install/smoke_languages.pl --bindir=.

test installation in DESTDIR:

    % cd /usr/src/parrot
    % mkdir .inst
    % make install DESTDIR=.inst
    % perl tools/install/smoke_languages.pl DESTDIR=.inst

=head1 DESCRIPTION

Checks that most of things run (or just start) into the install directory,
try to detect missing parts.

=head1 OPTIONS

=over

=item --bindir=/usr/bin

Override default value : 'bin'

=back

=cut

my ($bindir, $DESTDIR);
my $opts = GetOptions(
    'bindir=s'  => \$bindir,
    'DESTDIR=s' => \$DESTDIR,
);

$bindir = 'bin' unless $bindir;

chdir($DESTDIR) if ($DESTDIR);
my $pwd = getcwd();

my $filename;
my $exe;
my $out;
my $FH;
my $parrot = catfile($pwd, $bindir, 'parrot');

$out = `$parrot -V`;
$out =~ m/version (\S+) built/;
my $version = $1;

my $langdir = ($bindir eq 'bin')
            ? ($^O eq 'MSWin32') ? 'lib/parrot/languages' : "lib/parrot/$version/languages"
            : 'languages';

#
# some languages
#

SKIP:
{
skip("abc", 1) unless (-d "$pwd/$langdir/abc");
chdir("$pwd/$langdir/abc");
$filename = 'test.bc';
open $FH, '>', $filename
        or die "Can't open $filename ($!).\n";
print $FH "1 + 2\n";
close $FH;
$out = `$parrot abc.pbc $filename`;
ok($out eq "3\n", "check abc");
unlink($filename);
}

SKIP:
{
skip("Befunge", 1) unless (-d "$pwd/$langdir/befunge");
chdir("$pwd/$langdir/befunge");
$filename = 'test.bef';
open $FH, '>', $filename
        or die "Can't open $filename ($!).\n";
print $FH <<'CODE';
<                   p 04   "v"
  ^ >
I                       @   _v
                            !,
  _   2! |                  :
    . \-  %2/36 `21  $   <  ^<   "<- then everything is ok!" +37
  !      #
  3      >
         <       v  ,  _ ^# -8 : g20 "f you can see a 4 here ->" 8 4
                 > :8- ^
CODE
close $FH;
$out = `$parrot befunge.pbc $filename`;
ok($out eq "If you can see a 4 here ->4 <- then everything is ok!\n", "check befunge");
unlink($filename);
}

SKIP:
{
skip("bf", 3) unless (-d "$pwd/$langdir/bf");
chdir("$pwd/$langdir/bf");
$out = `$parrot bf.pbc`;
ok($out =~ /^usage/, "check bf");
$out = `$parrot bfc.pbc`;
ok($out =~ /^usage/, "check bfc");
$out = `$parrot bfco.pbc`;
ok($out =~ /^usage/, "check bfco");
}

SKIP:
{
if (-d "$pwd/$langdir/blizkost") {
    chdir("$pwd/$langdir/blizkost");
}
elsif (-d "$pwd/$langdir/perl5") {
    chdir("$pwd/$langdir/perl5");
}
else {
    skip("blizkost", 1)
}
$filename = 'test.pl';
open $FH, '>', $filename
        or die "Can't open $filename ($!).\n";
print $FH "print qq{Hello, World!\n};\n";
close $FH;
$out = `$parrot perl5.pbc $filename`;
ok($out eq "Hello, World!\n" || $out eq "Hello, World!\r\n", "check blizkost");
unlink($filename);
}

SKIP:
{
skip("Cardinal", 1) unless (-d "$pwd/$langdir/cardinal");
chdir("$pwd/$langdir/cardinal");
$out = `$parrot cardinal.pbc -e "print 'hello world';"`;
ok($out eq "hello world", "check cardinal");
}

SKIP:
{
skip("ChitChat", 1) unless (-d "$pwd/$langdir/chitchat");
chdir("$pwd/$langdir/chitchat");
$filename = 'test.smalltalk';
open $FH, '>', $filename
        or die "Can't open $filename ($!).\n";
print $FH "Transcript show: 'Hello, world!'.";
close $FH;
$out = `$parrot chitchat.pbc $filename`;
ok($out eq "Hello, world!\n", "check chitchat");
unlink($filename);
}

SKIP:
{
skip(".NET", 1) unless (-d "$pwd/$langdir/dotnet");
chdir("$pwd/$langdir/dotnet");
$out = `$parrot dotnet/net2pbc.pbc`;
ok($out =~ /^Usage/, "check dotnet");
}

SKIP:
{
skip("EcmaScript", 1) unless (-d "$pwd/$langdir/ecmascript");
chdir("$pwd/$langdir/ecmascript");
$filename = 'test.js';
open $FH, '>', $filename
        or die "Can't open $filename ($!).\n";
print $FH "print(\"Hello World from JS\");";
close $FH;
$out = `$parrot js.pbc $filename`;
ok($out eq "Hello World from JS\n", "check ecmascript");
unlink($filename);
}

SKIP:
{
skip("fun", 1) unless (-d "$pwd/$langdir/fun");
chdir("$pwd/$langdir/fun");
$filename = 'test.fun';
open $FH, '>', $filename
        or die "Can't open $filename ($!).\n";
print $FH "\"Hello World!\".";
close $FH;
$out = `$parrot fun.pbc $filename`;
ok($out eq "Hello World!\n", "check fun");
unlink($filename);
}

SKIP:
{
skip("gil", 1) unless (-d "$pwd/$langdir/gil");
chdir("$pwd/$langdir/gil");
$filename = 'test.gil';
open $FH, '>', $filename
        or die "Can't open $filename ($!).\n";
print $FH q{say('Hello, world!');};
close $FH;
$out = `$parrot gil.pbc $filename`;
ok($out eq "Hello, world!\n", "check gil");
unlink($filename);
}

SKIP:
{
skip("HQ9Plus", 1) unless (-d "$pwd/$langdir/hq9plus");
chdir("$pwd/$langdir/hq9plus");
$filename = 'test.HQ9Plus';
open $FH, '>', $filename
        or die "Can't open $filename ($!).\n";
print $FH "H";
close $FH;
$out = `$parrot hq9plus.pbc $filename`;
ok($out eq "Hello, world!\n", "check HQ9Plus");
unlink($filename);
}

SKIP:
{
skip("Lisp", 1) unless (-d "$pwd/$langdir/lisp");
chdir("$pwd/$langdir/lisp");
$filename = 'test.l';
open $FH, '>', $filename
        or die "Can't open $filename ($!).\n";
print $FH "( print \"Hello, World!\" )\n";
close $FH;
$out = `$parrot lisp.pbc $filename`;
ok($out eq "Hello, World!\n", "check lisp");
unlink($filename);
}

SKIP:
{
skip("LOLCODE", 1) unless (-d "$pwd/$langdir/lolcode");
chdir("$pwd/$langdir/lolcode");
$filename = 'test.lolcode';
open $FH, '>', $filename
        or die "Can't open $filename ($!).\n";
print $FH <<'CODE';
HAI 1.2
    VISIBLE "HAI WORLD!"
KTHXBYE
CODE
close $FH;
$out = `$parrot lolcode.pbc $filename`;
ok($out eq "HAI WORLD!\n", "check lolcode");
unlink($filename);
}

SKIP:
{
skip("Lua", 1) unless (-d "$pwd/$langdir/lua");
chdir("$pwd/$langdir/lua");
$out = `$parrot lua.pbc -e "print(nil)"`;
ok($out eq "nil\n", "check lua");
}

SKIP:
{
skip("m4", 1) unless (-d "$pwd/$langdir/m4");
chdir("$pwd/$langdir/m4");
$out = `$parrot m4.pbc`;
ok($out =~ /^Usage/, "check m4");
}

SKIP:
{
skip("Markdown", 1) unless (-d "$pwd/$langdir/markdown");
chdir("$pwd/$langdir/markdown");
$filename = 'test.text';
open $FH, '>', $filename
        or die "Can't open $filename ($!).\n";
print $FH "Hello, World!\n\n";
close $FH;
$out = `$parrot markdown.pbc $filename`;
ok($out eq "<p>Hello, World!</p>\n", "check markdown");
unlink($filename);
}

SKIP:
{
skip("matrixy", 1) unless (-d "$pwd/$langdir/matrixy");
chdir("$pwd/$langdir/matrixy");
$filename = 'test.oct';
open $FH, '>', $filename
        or die "Can't open $filename ($!).\n";
print $FH "printf(\"Hello, world!\n\");";
close $FH;
$out = `$parrot matrixy.pbc $filename`;
ok($out eq "Hello, world!\n", "check matrixy");
unlink($filename);
}

SKIP:
{
skip("Paraplegic", 1) unless (-d "$pwd/$langdir/paraplegic");
chdir("$pwd/$langdir/paraplegic");
$filename = 'test.apl';
open $FH, '>', $filename
        or die "Can't open $filename ($!).\n";
print $FH "\"Hello world!\"";
close $FH;
$out = `$parrot APL.pbc $filename`;
ok($out eq "Hello world!\n", "check Paraplegic");
unlink($filename);
}

SKIP:
{
skip("Pheme", 1) unless (-d "$pwd/$langdir/pheme");
chdir("$pwd/$langdir/pheme");
$filename = 'test.l';
open $FH, '>', $filename
        or die "Can't open $filename ($!).\n";
print $FH "( write \"Hello, World!\\n\" )\n";
close $FH;
$out = `$parrot pheme.pbc $filename`;
ok($out eq "Hello, World!\n", "check pheme");
unlink($filename);
}

SKIP:
{
skip("Pipp", 1) unless (-d "$pwd/$langdir/pipp");
chdir("$pwd/$langdir/pipp");
$filename = 'test.php';
open $FH, '>', $filename
        or die "Can't open $filename ($!).\n";
print $FH "<?php echo \"Hello, World!\\n\"; ?>";
close $FH;
$out = `$parrot pipp.pbc $filename`;
ok($out eq "Hello, World!\n", "check pipp");
unlink($filename);
}

SKIP:
{
skip("porcupine", 1) unless (-d "$pwd/$langdir/porcupine");
chdir("$pwd/$langdir/porcupine");
$filename = 'test.pas';
open $FH, '>', $filename
        or die "Can't open $filename ($!).\n";
print $FH <<'CODE';
program hello;
begin
    writeln('Hello, world!');
end.
CODE
close $FH;
$out = `$parrot porcupine.pbc $filename`;
ok($out eq "Hello, world!\n", "check porcupine");
unlink($filename);
}

SKIP:
{
skip("primitivearc", 1) unless (-d "$pwd/$langdir/primitivearc");
chdir("$pwd/$langdir/primitivearc");
$filename = 'test.arc';
open $FH, '>', $filename
        or die "Can't open $filename ($!).\n";
print $FH q{"Hello, world!\n"};
close $FH;
$out = `$parrot primitivearc.pbc $filename`;
ok($out eq "Hello, world!\n\n", "check primitivearc");
unlink($filename);
}

SKIP:
{
skip("Punie", 1) unless (-d "$pwd/$langdir/punie");
chdir("$pwd/$langdir/punie");
$filename = 'test.p1';
open $FH, '>', $filename
        or die "Can't open $filename ($!).\n";
print $FH "print \"Hello, World!\";\n";
close $FH;
$out = `$parrot punie.pbc $filename`;
ok($out eq "Hello, World!", "check punie");
unlink($filename);
}

SKIP:
{
skip("Pynie", 1) unless (-d "$pwd/$langdir/pynie");
chdir("$pwd/$langdir/pynie");
$filename = 'test.py';
open $FH, '>', $filename
        or die "Can't open $filename ($!).\n";
print $FH "print('Hello, World!')\n";
close $FH;
$out = `$parrot pynie.pbc $filename`;
ok($out eq "Hello, World!\n", "check pynie");
unlink($filename);
}

SKIP:
{
skip("Rakudo", 2) unless (-d "$pwd/$langdir/rakudo");
chdir("$pwd/$langdir/rakudo");
$out = `$parrot perl6.pbc -e "say 'hello world'"`;
ok($out eq "hello world\n", "check rakudo");

skip("perl6 executable", 1) if ($bindir eq '.');
$exe = catfile($bindir, 'perl6');
$out = `$exe -v`;
ok($out =~ /Rakudo/, "check perl6");
}

SKIP:
{
skip("Shakespeare", 1) unless (-d "$pwd/$langdir/shakespeare");
chdir("$pwd/$langdir/shakespeare");
$filename = 'test.spl';
open $FH, '>', $filename
        or die "Can't open $filename ($!).\n";
print $FH <<'CODE';
The Infamous Hello World Program.

Romeo, a young man with a remarkable patience.
Juliet, a likewise young woman of remarkable grace.
Ophelia, a remarkable woman much in dispute with Hamlet.
Hamlet, the flatterer of Andersen Insulting A/S.


                    Act I: Hamlet's insults and flattery.

                    Scene I: The insulting of Romeo.

[Enter Hamlet and Romeo]

Hamlet:
 You lying stupid fatherless big smelly half-witted coward!
 You are as stupid as the difference between a handsome rich brave
 hero and thyself! Speak your mind!

 You are as brave as the sum of your fat little stuffed misused dusty
 old rotten codpiece and a beautiful fair warm peaceful sunny summer's
 day. You are as healthy as the difference between the sum of the
 sweetest reddest rose and my father and yourself! Speak your mind!

 You are as cowardly as the sum of yourself and the difference
 between a big mighty proud kingdom and a horse. Speak your mind.

 Speak your mind!

[Exit Romeo]

                    Scene II: The praising of Juliet.

[Enter Juliet]

Hamlet:
 Thou art as sweet as the sum of the sum of Romeo and his horse and his
 black cat! Speak thy mind!

[Exit Juliet]

                    Scene III: The praising of Ophelia.

[Enter Ophelia]

Hamlet:
 Thou art as lovely as the product of a large rural town and my amazing
 bottomless embroidered purse. Speak thy mind!

 Thou art as loving as the product of the bluest clearest sweetest sky
 and the sum of a squirrel and a white horse. Thou art as beautiful as
 the difference between Juliet and thyself. Speak thy mind!

[Exeunt Ophelia and Hamlet]


                    Act II: Behind Hamlet's back.

                    Scene I: Romeo and Juliet's conversation.

[Enter Romeo and Juliet]

Romeo:
 Speak your mind. You are as worried as the sum of yourself and the
 difference between my small smooth hamster and my nose. Speak your
 mind!

Juliet:
 Speak YOUR mind! You are as bad as Hamlet! You are as small as the
 difference between the square of the difference between my little pony
 and your big hairy hound and the cube of your sorry little
 codpiece. Speak your mind!

[Exit Romeo]

                    Scene II: Juliet and Ophelia's conversation.

[Enter Ophelia]

Juliet:
 Thou art as good as the quotient between Romeo and the sum of a small
 furry animal and a leech. Speak your mind!

Ophelia:
 Thou art as disgusting as the quotient between Romeo and twice the
 difference between a mistletoe and an oozing infected blister! Speak
 your mind!

[Exeunt]

CODE
close $FH;
$out = `$parrot shakespeare.pbc $filename`;
ok($out eq "Hello World!\n", "check shakespeare");
unlink($filename);
}

SKIP:
{
skip("steme", 1) unless (-d "$pwd/$langdir/steme");
chdir("$pwd/$langdir/steme");
$filename = 'test.scm';
open $FH, '>', $filename
        or die "Can't open $filename ($!).\n";
print $FH "( say \"Hello, World!\" )\n";
close $FH;
$out = `$parrot steme.pbc $filename`;
ok($out eq "Hello, World!\n", "check steme");
unlink($filename);
}

SKIP:
{
skip("Squaak", 1) unless (-d "$pwd/$langdir/squaak");
chdir("$pwd/$langdir/squaak");
$filename = 'test.squaak';
open $FH, '>', $filename
        or die "Can't open $filename ($!).\n";
print $FH "print(\"Hello, World!\")\n";
close $FH;
$out = `$parrot squaak.pbc $filename`;
ok($out eq "Hello, World!\n", "check squaak");
unlink($filename);
}

SKIP:
{
skip("unlambda", 1) unless (-d "$pwd/$langdir/unlambda");
chdir("$pwd/$langdir/unlambda");
$filename = 'test.unl';
open $FH, '>', $filename
        or die "Can't open $filename ($!).\n";
print $FH <<'CODE';
# hello world
`r```````````.H.e.l.l.o. .w.o.r.l.di
CODE
close $FH;
$out = `$parrot unl.pbc $filename`;
ok($out eq "Hello world\n", "check unlambda");
unlink($filename);
}

SKIP:
{
skip("WMLScript", 1) unless (-d "$pwd/$langdir/wmlscript");
skip("WMLScript, not wmlsc", 1) unless (`wmlsc -h` =~ /wmlsc/);
chdir("$pwd/$langdir/wmlscript");

$filename = 'test.wmls';
open $FH, '>', $filename
        or die "Can't open $filename ($!).\n";
print $FH <<'CODE';
extern function main()
{
    Console.println("Hello World!");
}
CODE
close $FH;
`wmlsc $filename`;
$out = `$parrot wmlsi.pbc ${filename}c main`;
ok($out eq "Hello World!\n", "check wmlscript");
unlink($filename);
unlink($filename . 'c');
}

# Local Variables:
#   mode: cperl
#   cperl-indent-level: 4
#   fill-column: 100
# End:
# vim: expandtab shiftwidth=4:
