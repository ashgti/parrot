#!/usr/bin/perl -w
#
# This is a minimal and incomplete python bytecode to PIR translator
# It's purpose is just to investigate missing pythonic features
# in Parrot and how to translate Python stack-oriented bytecode to PIR.
#

use strict;
use Getopt::Std;

my ($DIS, @dis, @source, $file, %opt, $DEFVAR, $cur_func, $lambda_count,
   %main_names, %namespace, %may_be_none);
$DIS = 'python mydis.py';
$DEFVAR = 'PerlInt';

getopts('dnD', \%opt);
$file = $ARGV[0];
$lambda_count = 0;

my %builtin_ops = (
    abs => 'o',
    isinstance => 's',
    ord => 's',
);

my %builtins = (
    AssertionError => 1,
    bool => 1,
    complex => 1,
    callable => 1,
    chr => 1,
    dict => 1,
    divmod => 1,
    enumerate => 1,
    float => 1,
    hash => 1,
    hex => 1,
    id => 1,
    iter => 1,
    filter => 1,
    list => 1,
    long => 1,
    int => 1,
    map => 1,
    max => 'v',
    min => 'v',
    range => 1,
    xrange => 1,
    reduce => 1,
    str => 1,
    tuple => 1,
);

my %vtables = (
    __iter__ => '__get_iter',
    __repr__ => '__get_repr',
    __str__ => '__get_string',
    __cmp__ => 41,		# MMD_CMP
);

# the new way type system
my %type_map = (
    bool  => 'Py_bool',
    complex  => 'Py_complex',
    float => 'Py_float',
    int   => 'Py_int',
    long  => 'Py_long',
    str   => 'Py_str',

    dict  => 'Py_dict',
    list  => 'Py_list',
    tuple => 'Py_tuple',

    iter  => 'Py_iter',
    xrange => 'Py_xrange',

    object => 'Py_object',
    type => 'Py_type',
);

my %nci_methods = (
    'append' => 'append',
    'fromkeys' => 'fromkeys',
    'locase' => 'locase',
    'next' => 'next',
    'sort' => 'sort',
);

my %rev_type_map;

while (my ($k, $v) =  each (%type_map)) {
    $rev_type_map{$v} = $k;
};

get_dis($DIS, $file);
get_source($file);
exit if $opt{D};
gen_code();

sub nci_method {
    my $m = shift;
    return 1 if $vtables{$m};
    return 1 if $nci_methods{$m};
    return 1 if $m =~ /^\d+$/;	# MMD nr
    return 0;
}

sub type_map {
    my $t = $_[0];
    return $type_map{$t} if $type_map{$t} ;
    return $t;
}
sub parse_dis
{
    my @d = @_;
    my ($dir1, $dir2);
    for (@d) {
	if (/^\[/) {
	    if ($dir1) {
		$dir2 = $_;
	    }
	    else {
		$dir1 = $_;
	    }
	}
    }
    $dir1 =~ s/^\[//;
    $dir2 =~ s/^\[//;
    $dir1 =~ s/\[$//;
    $dir2 =~ s/\[$//;
    my @dir1 = split(/,/, $dir1);
    my @dir2 = split(/,/, $dir2);
    my (%dir1, %dir2);
    @dir1{@dir1} = (1) x scalar @dir1;
    @dir2{@dir2} = (1) x scalar @dir2;
    my @diff;
    foreach (keys(%dir2)) {
	push @diff, $_ unless $dir1{$_};
    }
    print "diff @diff\n" if $opt{d};
    @diff;
}

sub get_dis {
    my ($cmd, $f) = @_;
    @dis = qx($cmd $f);
    print @dis if $opt{d};
}

sub get_source {
    my ($f) = @_;
    open IN, $f or die "can't read $f: $!";
    @source = <IN>;
    close(IN);
}

my ($code_l, %params, %lexicals, %names, %def_args, %arg_count,
    @code, %globals, %classes, @loops, %def_arg_names, %func_info);

sub decode_line {
    my $l = shift;
    my ($pc, $line ,$opcode, $arg, $rest);
    if ($l =~ /Disassembly of <?([\w:]+)>?/) {
	push @code, [ 0, 0, "New_func", 0, $1, undef ];
	return;
    }
    if ($l =~ />>\s+(\d+)/) {
	push @code, [ 0, $1, "Label", $1, "", undef ];
	$l =~ s/>>//;
    }
    my $source = undef;
    if ($l =~ /^\s*	     # intial space
	(?:(\d+)\s+)?   # optional line
	(\d+)\s+        # PC
	([\w+]+)\s+      # opcode e.g. SLICE+3
	(?:(\d+)(?:\s+\((.*)\))?)? # oparg rest
	/x) {
	($line, $pc, $opcode, $arg, $rest) = ($1, $2, $3, $4, $5);
	$opcode =~ s/\+/_plus_/;
	## print STDERR "Op: '$opcode'\n";
	if ($line) {
	    $source = $source[$line-1];
	    if ($source =~ /def (\w+)\s*\((.*)\)/) {
		my ($f, $args) = ($1, $2);
		my @args = split(/,/, $args);
		my $n = @args;
		$arg_count{$f} = $n;
		for (my $i = 0; $i < $n; $i++) {
		    my ($a, $def) = split(/=/, $args[$i]);
		    $a =~ s/\s//g;
		    $a = qq!"$a"!;   # quote argument
		    $def_arg_names{$f}{$a} = $i;
		    # print STDERR "def $f($a = $i)\n";
		}
		push @code, [$line, $pc, "ARG_count", $n, $f, $source];
	    }
	    elsif ($source =~ /lambda\s(.*?):/) {
		my $f = "lambda_$lambda_count";
		print "#xxxxxxxxx $f\n";
		my $args = $1;
		++$lambda_count;
		my @args = split(/,/, $args);
		my $n = @args;
		$arg_count{$f} = $n;
		for (my $i = 0; $i < $n; $i++) {
		    my ($a, $def) = split(/=/, $args[$i]);
		    $a =~ s/\s//g;
		    $a = qq!"$a"!;   # quote argument
		    $def_arg_names{$f}{$a} = $i;
		    # print STDERR "def $f($a = $i)\n";
		}
		push @code, [$line, $pc, "ARG_count", $n, $f, $source];
	    }
	}
	$arg = '' unless defined $arg;
	$rest = '' unless defined $rest;
    }
    else {  # program output from import - really ugly
	push @code, [0,0, undef, 0, $l, ''];
	return;
    }
    push @code, [$line, $pc, $opcode, $arg, $rest, $source];
}

sub XXX {
   my ($n, $c, $cmt) = @_;
   print "#Unknown '$cmt'\n";
}

sub Label {
   my ($n, $c) = @_;
   print <<EOC;
pc_$n:
EOC
}

sub New_func {
    my ($n, $arg, $cmt) = @_;
    my $nst = "";
    my $ns = $namespace{$arg};
    my $real_name = $arg;
    my $meth = '';
    if ($ns) {
	$meth = ', method';
	$nst = qq!.namespace [$ns]!;
	if ($vtables{$arg}) {
	    $real_name = $vtables{$arg};
	}
    }
    print <<EOC;
.end		# $cur_func
.namespace [""]

$nst
.sub $real_name prototyped$meth $cmt
EOC
    my (@params, $k, $v, $params);
    while ( ($k, $v) = each(%{$def_arg_names{$arg}})) {
	$k =~ s/[\*"]//g;
	$params[$v] = $k;
    }
    my $self;
    if ($meth ne '') {
	$self = shift @params;
	#shift @{$def_args{$arg}};
	$arg_count{$arg}--;
    }

    $params = join("\n\t", map {".param pmc $_"} @params);
    print <<EOC;
	$params
EOC
    if ($self && $self ne 'self') {
	print <<EOC;
	.local pmc $self
	$self = self
EOC
    }
    if ($func_info{$arg}{flags} & 0x20) {  # GENERATOR flag
	for (my $i = 0; $i < @params; ++$i) {
	    my $p = $params[$i];
	    print <<EOC;
	$p = find_lex -1, $i
EOC
	}
    }
    # XXX classes are store in this pad because of name clash
    #     namespace <=> classname
    # TODO mange class namespace
    print <<EOC;
	# new_pad 0
	.local pmc None
	None = new .None
EOC
    %globals = ();
    $names{None} = 'None';
    $globals{None} = 'None';
    if ($def_args{$arg}) {
	my ($i, $n, $defs);
	$n = $arg_count{$arg};
	$defs = @{$def_args{$arg}};
	print "# @{$def_args{$arg}}\n";
	# XXX this is wrong, the default args should be evaluated once
	#     if this value depends on a global and that changes, this fails
	for ($i = $n; @{$def_args{$arg}}; $i--) {
	    my $reg = 4 + $i;
	    my $d = pop @{$def_args{$arg}};
	    my $arg_name = pop @params;
	    $may_be_none{$arg_name} = 1;
	    print <<EOC;
	if argcP >= $i goto arg_ok
	    find_global $arg_name, "${arg}_$d"
EOC
	}
	print <<EOC;
arg_ok:
EOC
    }
    $cur_func = $arg;
    %params = ();
    %lexicals = ();
    %names = ();
}

sub ARG_count {
    my ($n, $c, $cmt) = @_;
    print <<EOC;
	# $c($n) $cmt
EOC
}

my (@stack, $temp, $make_f, %pir_functions);

sub gen_code {
    $cur_func = 'test::main';
    print <<EOC;
.sub $cur_func \@MAIN
    .param pmc sys::argv
    new_pad 0
    \$P0 = getinterp
    \$P0."recursion_limit"(998)
    .local pmc __name__
    __name__ = new $DEFVAR
    __name__ = '__main__'
    global '__name__' = __name__
    .local pmc None
    None = new .None
EOC
    $globals{'__name__'} = '__name__';
    $code_l = 0;
    my $in_info = 0;
    my $cur_f;
    for (@dis) {
	if (/Information of <?([\w:]+)>?/) {
	    $in_info = 1;
	    $cur_f = $1;
	}
	elsif ($in_info) {
	    if (/^#/) {
		if (/# getargs\s+\(\[(.*)\], (.*?), (.*?)\)/) {
		    my ($args, $ar, $kw) = ($1, $2, $3);
		    $args =~ s/[\s']//g;
		    $ar =~ s/'//g;
		    $kw =~ s/'//g;
		    print "# $cur_f: args='$args' ar='$ar' kw='$kw'\n";
		    $func_info{$cur_f}{'args'} = $args;
		    $func_info{$cur_f}{'ar'} = $ar;
		    $func_info{$cur_f}{'kw'} = $kw;
		    #if ($cur_f =~ /^Build::(\w+)/) {
		    # $classes{$cur_f} = 1;
		    # }
		}
		elsif (/# flags\s+(\S*)/) {
		    my $f = eval($1);
		    $func_info{$cur_f}{flags} = $f;
		    print "# $cur_f; flags=$f\n";
		}
		elsif (/# varnames\s+\((.*)\)/) {
		    my $vars = $1;
		    $vars =~ s/[\s']//g;
		    $func_info{$cur_f}{varnames} = $vars;
		    print "# $cur_f; vars=$vars\n";
		}
	    }
	    else {
		$in_info = 0;
	    }
	}
	else {
	    next if /^\s*$/;
	    decode_line($_);
	}
    }
    while ($code_l < @code) {
	my $l = $code[$code_l++];
	my ($opcode, $arg, $rest, $src) = ($l->[2], $l->[3], $l->[4], $l->[5]);
	next unless $opcode;
	my $cmt = "";
	print "## $src" if  $src;

	if ($rest =~ /(<code> \w+)/) {
	    $rest = "$1 ..>";
	}
	$cmt = "\t\t# $opcode\t$arg $rest" unless $opt{n};
	gen_pir($opcode, $arg, $rest, $cmt);
    }
    print ".end\t\t# $cur_func\n";
}

sub gen_pir {
    my ($opcode, $arg, $rest, $cmt) = @_;
    no strict "refs";
    &$opcode($arg, $rest, $cmt);
}

sub temp {
    my $t = $_[0];
    "\$$t" . ++$temp;
}

sub is_num {
    my $c = $_[0];
    my ($pointfloat, $expfloat, $frac, $exp);
    $exp = qr/[eE][-+]?\d+/;
    $frac = qr/\.\d+/;
    $pointfloat = qr/(?:(?:\d+)?$frac)|\d+\./o;
    $expfloat = qr/(?:\d+|$pointfloat)$exp/o;
    return 1 if ($c =~ /$pointfloat|$expfloat/o);
    return 0;
}

sub is_imag {
    my $c = $_[0];
    return 1 if ($c =~ /^[+-]?\d+[jJ]$/);
    return 1 if ($c =~ s/[jJ]$// && is_num($c));
    return 0;
}

sub typ {
    my $c = $_[0];
    my $t = 'P';
    if ($c =~ /<code>/) {
	$t = 'c';
    }
    elsif ($c =~ /^[+-]?\d+$/) {	# int
	$t = 'I';
    }
    elsif ($c =~ /^[+-]?\d+[lL]$/) {	# bigint
	$t = 'B';
    }
    elsif ($c =~ /^'.*'$/) {	# string consts are single quoted by dis
	$t = 'S';
    }
    elsif ($c =~ /^u'.*'$/) {	# unicode-string TODO r raw
	$t = 'U';
    }
    elsif (is_num($c)) {        # num
	$t = 'N';
    }
    elsif ($c =~ /^\(/) {
	$t = 'u'; # unimp
    }
    $t;
}


sub promote {
    my $v = $_[0];
    my $n = $v->[1];
    if ($v->[2] ne 'P') {
	if ($v->[2] eq 'c') {
	    if ($v->[1] =~ /<code> (lambda_\d+)/) {
		$n = $1;
		return $n;
	    }
	}
	$n = temp('P');
	print <<"EOC";
	$n = new $DEFVAR
	$n = $v->[1]
EOC
    }
    $n;
}

sub LOAD_CONST {
    my ($n, $c, $cmt) = @_;
    my $typ = typ($c);
    if ($typ eq 'P') {
	if ($c =~ /^[_a-zA-Z]/ && !$names{$c}) {	# True, False ...
	    print <<EOC;
	.local pmc $c $cmt
	$c = new .$c
EOC
	    $names{$c} = 1;
	}
	else {
	    my $typ = $DEFVAR;
	    if (is_imag($c)) {
		$typ = '.Complex';
		$c = qq!"$c"!;
	    }
	    my $pmc = temp('P');
	    print <<EOC;
	$pmc = new $typ $cmt
	$pmc = $c
EOC
	    push @stack, [$n, $pmc, 'P'];
	    return;
	}
    }
    elsif ($typ eq 'B') {   # bigint
	my $typ = $DEFVAR;
        my $pmc = temp('P');
	$c =~ s/[lL]$//;
	print <<EOC;
	$pmc = new .BigInt $cmt
	$pmc = "$c"
EOC
	push @stack, [$n, $pmc, 'P'];
	return;
    }
    elsif ($typ =~ /[US]/) {   # strings
	# parrot has double quoted escapes
	$c =~ s/"/\\"/g;	# XXX unescape
	my $u = defined $1 ? $1 : "";
	if ($c =~ /^(u|U)?'(.*)'/) {
	    my $u = defined $1 ? "u:" : "";
	    my $s = $2;
	    $c =~ s/.*/$u"$s"/;
	}
	print <<EOC;
	\t$cmt
EOC
    }
    else {
	print <<EOC;
	\t$cmt
EOC
    }
    push @stack, [$n, $c, $typ];
}
sub LOAD_LOCALS {
    my ($n, $c, $cmt) = @_;
    # TODO $cmt
    my $pad = temp('P');
    print <<EOC;
	peek_pad $pad      # ???
EOC
    push @stack, [-1, $pad, 'P'];
}

sub STORE_NAME {
    my ($n, $c, $cmt) = @_;
    if ($make_f) {
	$make_f = 0;
	print_stack();
	print "# make_f \t$cmt\n";
	return;
    }
    my $tos = pop @stack;
    my $p = $tos->[1];
    if ($names{$c}) {
	my $pmc = $names{$c};
	print <<EOC;
	set $pmc, $p $cmt
EOC
	$p = $pmc;
    }
    else {
	$p = promote($tos);
	if ($cur_func eq 'test::main') {
	    $main_names{$c} = $p;
	    print <<EOC;
	store_lex -1, "$c", $p $cmt
EOC
	}
	else {
	    print <<EOC;
	store_lex -1, $n, $p $cmt
EOC
	}
    }
    $names{$c} = $p;
    $lexicals{$c} = $p;
}

sub STORE_GLOBAL {
    my ($n, $c, $cmt) = @_;
    my $tos = pop @stack;
    my $p = $tos->[1];
    my $t;
    if ($t=$globals{$c}) {
	if ($t ne $p) {
	    print <<EOC;
	assign $t, $p
EOC
	}
    }
    else {
	$p = promote($tos);
	print <<EOC;
	global "$c" = $p $cmt
EOC
    }
    $globals{$c} = $p;
}


sub is_opcode {
    my $f = shift;
    return $builtin_ops{$f};
}

sub LOAD_GLOBAL {
    my ($n, $c, $cmt) = @_;
    if (is_opcode($c) || $builtins{$c}) {
	return LOAD_NAME(@_);
    }
    my $p;
    if (($p = $globals{$c})) {
	print <<EOC;
	# $p = global "$c" $cmt
EOC
    }
    elsif ($main_names{$c}) {
	$p = temp('P');
	print <<EOC;
	$p = find_lex "$c" $cmt
EOC
    }
    else {
	$p = temp('P');
	$globals{$c} = $p;
	print <<"EOC";
	$p = global "$c" $cmt
EOC
    }
    push @stack, [$c, $p, 'P'];
    # print_stack();
}


sub LOAD_NAME() {
    my ($n, $c, $cmt) = @_;
    my ($o);
    my $p;
    if (($o = is_opcode($c))) {
	print <<EOC;
	# builtin $c $cmt $o
EOC
	push @stack, [$c, $c, $o];
	return;
    }
    # params TODO
    if ($names{$c}) {
	$p = $names{$c};
	print <<"EOC";
	# lexical $n '$c' := $p $cmt
EOC
    }
    elsif ($globals{$c}) {
	$p = $globals{$c};
	print <<"EOC";
	# $c = global "$c" $cmt
EOC
    }
    else {
	my $type = 'pmc';
	$p = $c;
	if ($type_map{$c}) {
	    $c = $p = $type_map{$c};
	    $type = 'NCI';
	}
	$globals{$c} = $c;
	print <<"EOC";
	.local $type $c $cmt
	$c = global "$c"
EOC
    }
    push @stack, [$c, $p, 'P'];
}

sub PRINT_ITEM
{
    my ($n, $c, $cmt) = @_;
    my $tos = pop @stack;
    print <<"EOC";
	print_item $tos->[1] $cmt
EOC
}

sub PRINT_NEWLINE
{
    my ($n, $c, $cmt) = @_;
    print <<"EOC";
	print_newline $cmt
EOC
}

sub RETURN_VALUE
{
    my ($n, $c, $cmt) = @_;
    my $tos = promote(pop @stack);
    unless ($cur_func eq 'test::main') {
	print <<EOC;
    	.pcc_begin_return $cmt
	.return $tos
	.pcc_end_return
EOC
    }
    else {
	print <<EOC;
	# $cmt
EOC
    }
}

sub YIELD_VALUE
{
    my ($n, $c, $cmt) = @_;
    my $tos = promote(pop @stack);
    print <<EOC;
    	.pcc_begin_yield $cmt
	.return $tos
	.pcc_end_yield
EOC
}

sub MAKE_FUNCTION
{
    my ($n, $c, $cmt) = @_;
    my $tos = pop @stack;
    my $f;
    $tos->[1] =~ /<code> (\S+)/;
    $f = $1;
    print "\t\t$cmt $f\n";
    if ($n) {
	for (my $i=0; $i < $n; ++$i) {
	    my $arg = pop @stack;
	    my $g = promote($arg);
	    # TODO should better be namespace of func
	    # but can't create namespace yet
	    my $gn = "def_arg_" . ($n-$i-1);
	    print <<EOC;
	# $gn $g
	store_global "${f}_$gn", $g
EOC
	    unshift @{$def_args{$f}}, $gn;
	}
    }
    if ($cur_func =~ /Build::(\w+)/) {   ## XXX && $f ne '__new__'
	$namespace{$f} = $classes{$1};
	if ($vtables{$f}) {
	    print <<EOC;
	# $namespace{$f} => $vtables{$f}
EOC
	    #$namespace{$f} => $vtables{$f}
	}
	else {
	    print <<EOC;
	# $namespace{$f}
	# addattribute P5, "$f"
EOC
	}
    }
    else {
	$pir_functions{$f} = 1;
    }
    $make_f = 1 unless $f =~ /lambda/;
}

sub binary
{
    my ($op, $cmt) = @_;
    my $r = pop @stack;
    my $l = pop @stack;
    my ($t, $n);
    {
	my $nl = promote($l);
	$n = temp($t = 'P');
	my $nr = $r->[1];
	$nr = promote($r) if $r->[2] eq 'S';
	print <<"EOC";
	$n = new $DEFVAR $cmt
	$n = $nl $op $nr
EOC
    }
    push @stack, [-1, $n, $t];
}

sub binary_word
{
    my ($op, $cmt) = @_;
    my $r = pop @stack;
    my $l = pop @stack;
    my ($t, $n);
    {
	my $nl = promote($l);
	$n = temp($t = 'P');
	my $nr = $r->[1];
#	$nr = promote($r) if $r->[2] eq 'S';
	$nr = promote($r);
	print <<"EOC";
	$n = new $DEFVAR $cmt
	$op $n, $nl, $nr
EOC
    }
    push @stack, [-1, $n, $t];
}

sub BINARY_AND
{
    my ($n, $c, $cmt) = @_;
    binary('&', $cmt);
}

sub BINARY_OR
{
    my ($n, $c, $cmt) = @_;
    binary('|', $cmt);
}

sub BINARY_XOR
{
    my ($n, $c, $cmt) = @_;
    binary_word('xor', $cmt);
}

sub BINARY_LSHIFT
{
    my ($n, $c, $cmt) = @_;
    binary_word('shl', $cmt);
}

sub BINARY_RSHIFT
{
    my ($n, $c, $cmt) = @_;
    binary_word('shr', $cmt);
}

sub BINARY_ADD
{
    my ($n, $c, $cmt) = @_;
    binary('+', $cmt);
}

sub BINARY_SUBTRACT
{
    my ($n, $c, $cmt) = @_;
    binary('-', $cmt);
}

sub BINARY_MODULO
{
    my ($n, $c, $cmt) = @_;
    binary('%', $cmt);
}
sub BINARY_MULTIPLY
{
    my ($n, $c, $cmt) = @_;
    binary('*', $cmt);
}
sub BINARY_FLOOR_DIVIDE
{
    my ($n, $c, $cmt) = @_;
    binary('//', $cmt);
}
sub BINARY_DIVIDE
{
    my ($n, $c, $cmt) = @_;
    binary('/', $cmt);
}
sub BINARY_POWER
{
    my ($op, $cmt) = @_;
    my $r = pop @stack;
    my $l = pop @stack;
    my ($t, $n);
    if ($r->[2] eq 'I' && $l->[2] eq 'I') {
	$n = temp($t = 'N');
	print <<"EOC";
	$n = pow $l->[1], $r->[1] $cmt
EOC
    }
    else {
	my $nl = temp('N');
	my $nr = temp('N');
	$n = temp($t = 'N');
	print <<"EOC";
	$nl = $l->[1]
	$nr = $r->[1]
	$n = pow $nl, $nr $cmt
EOC
    }
    push @stack, [-1, $n, $t];
}
sub inplace
{
    my ($op, $cmt) = @_;
    my $r = pop @stack;
    my $l = pop @stack;
    print <<"EOC";
	$l->[1] $op= $r->[1] $cmt
EOC
    push @stack, [-1, $l->[1], $l->[2]];
}
sub inplace_word
{
    my ($op, $cmt) = @_;
    my $r = pop @stack;
    my $l = pop @stack;
    print <<"EOC";
	$op $l->[1], $r->[1] $cmt
EOC
    push @stack, [-1, $l->[1], $l->[2]];
}
sub INPLACE_MODULO
{
    my ($n, $c, $cmt) = @_;
    inplace_word('mod', $cmt);
}
sub INPLACE_ADD
{
    my ($n, $c, $cmt) = @_;
    inplace('+', $cmt);
}
sub INPLACE_SUBTRACT
{
    my ($n, $c, $cmt) = @_;
    inplace('-', $cmt);
}
sub JUMP_FORWARD()
{
    my ($n, $c, $cmt) = @_;
    my $targ = "pc_xxx";
    if ($c =~ /to (\d+)/) {
	$targ = "pc_$1";
    }
    print <<EOC;
	goto $targ $cmt
EOC
}

sub JUMP_ABSOLUTE()
{
    my ($n, $c, $cmt) = @_;
    my $targ = "pc_$n";
    print <<EOC;
	goto $targ $cmt
EOC
}

sub JUMP_IF_FALSE
{
    my ($n, $c, $cmt) = @_;
    if (!@stack) {
	print "#XXX\t\t$cmt - stack empty\n";
	return;
    }
    my $tos = pop @stack;
    my $targ = "pc_xxx";
    if ($c =~ /to (\d+)/) {
	$targ = "pc_$1";
    }
    print <<EOC;
	unless $tos->[1] goto $targ $cmt
EOC
}

sub JUMP_IF_TRUE
{
    my ($n, $c, $cmt) = @_;
    my $tos = pop @stack;
    my $targ = "pc_xxx";
    if ($c =~ /to (\d+)/) {
	$targ = "pc_$1";
    }
    print <<EOC;
	if $tos->[1] goto $targ $cmt
EOC
}

sub UNARY_NOT
{
    my ($n, $c, $cmt) = @_;
    my ($opcode, $arg, $rest) = ($code[$code_l]->[2],
	$code[$code_l]->[3],$code[$code_l]->[4]);

    if ($opcode eq 'JUMP_IF_FALSE') {
	print "\t\t$cmt\n";
	$code_l++;
	JUMP_IF_TRUE($arg, $rest, "\t# JUMP_IF_FALSE");
    }
    else {
	my $tos = pop @stack;
	my $n = temp($tos->[2]);
	print <<EOC;
	$n = not $tos->[1] $cmt
EOC
	push @stack, [-1, $n, $tos->[2]];

    }
}

sub UNARY_POSITIVE
{
    my (undef, $c, $cmt) = @_;
    my $t = pop @stack;
    my $p = $t->[1];
    print <<EOC;
    \t $cmt
EOC
    push @stack, [-1, $p, 'P'];
}

sub UNARY_NEGATIVE
{
    my (undef, $c, $cmt) = @_;
    my $t = pop @stack;
    my $n = temp('P');
    my $p = $t->[1];
    print <<EOC;
	$n = new $DEFVAR
	neg $n, $p $cmt
EOC
    push @stack, [-1, $n, 'P'];
}

sub except_compare
{
    my ($l, $r) = @_;
    my $cmp = temp('I');
    if ($l && $l->[1]) {
	$l = $l->[1];
    }
    else {
	$l = 'P5';
    }
    push @stack, [-1, 'P5', 'P'];   # simulate the DUP_TOP
    print <<EOC;
	# except compare '$l' <=> $r->[1]
	$cmp = iseq $l, $r->[1]
EOC
    push @stack, [-1, $cmp, 'P'];
}

sub COMPARE_OP
{
    my ($n, $c, $cmt) = @_;
    my $r = pop @stack;
    my $l = pop @stack;
    if ($c =~ /exception match/) {
	return except_compare($l, $r);
    }
    my %rev_map = (
	'==' => 'ne',
	'!=' => 'eq',
	'>' => 'le',
	'>=' => 'lt',
	'<' => 'ge',
	'<=' => 'gt',
	'is' => 'ne_addr',
	'is not' => 'eq_addr',
    );
    my %op_map = (
	'==' => 'eq',
	'!=' => 'ne',
	'>' => 'gt',
	'>=' => 'ge',
	'<' => 'lt',
	'<=' => 'le',
	'is' => 'eq_addr',
	'is not' => 'ne_addr',
    );
    my $op = $rev_map{$c};
    my ($opcode, $rest) = ($code[$code_l]->[2],$code[$code_l]->[4]);
    my $targ = "pc_xxx";
    my $label = '';
    if (!defined $op) {
	goto plain;
    }
    if ($opcode eq 'Label') {
	$label = "pc_" . $code[$code_l]->[3] . ":";
	$code_l++;
	($opcode, $rest) = ($code[$code_l]->[2],$code[$code_l]->[4]);
    }
    if ($opcode eq 'JUMP_IF_FALSE') {
	print "\t\t$cmt\n";
	$code_l++;
	$cmt ="\t\t# $opcode\t $rest";
	if ($rest =~ /to (\d+)/) {
	    $targ = "pc_$1";
	}
    }
    elsif ($opcode eq 'JUMP_IF_TRUE') {
	print "\t\t$cmt\n";
	$code_l++;
	$cmt ="\t\t# $opcode\t $rest";
	if ($rest =~ /to (\d+)/) {
	    $targ = "pc_$1";
	}
	$op = $op_map{$c};
    }
    elsif ($opcode eq 'UNARY_NOT' && $code[$code_l+1]->[2] eq 'JUMP_IF_FALSE') {
	$code_l++;
	print "\t\t\t# UNARY_NOT\n\t\t\t# JUMP_IF_FALSE\n";
        ($opcode, $rest) = ($code[$code_l]->[2],$code[$code_l]->[4]);
	if ($rest =~ /to (\d+)/) {
	    $targ = "pc_$1";
	}
	$cmt ="\t\t# $opcode\t $rest";
	$code_l++;
	$op = $op_map{$c};
    }
    else {
plain:
	$code_l-- if ($label ne '');
	# plain compare, no branch
	my %is_map = (
	    '==' => 'iseq',
	    '!=' => 'isne',
	    '>' => 'isgt',
	    '>=' => 'isge',
	    '<' => 'islt',
	    '<=' => 'isle',
	    'is' => 'issame',
	    'is not' => 'issame',
	    'in'   => 'exists'
	);
	my $res = temp('I');
	my $pres = temp('P');
	$op = $is_map{$c};
	my $isnot = '';
	if ($c eq 'is not' || $c eq 'not in') {
	    $isnot = qq!\n\t$res = not $res!;
	}
	if ($op eq 'exists') {
	    my $lk = $l->[1];
	    print <<EOC;
	$res = exists $r->[1]\[$lk\]
EOC
	}
	else {
	    my $lp = promote($l);
	    my $rp = promote($r);
	    print <<EOC;
	$res = $op $lp, $rp $cmt$isnot
EOC
	}
	print <<EOC;
	$pres = new .Boolean
	$pres = $res # ugly
EOC
	push @stack, [-1, $pres, 'P'];
	return;

    }
    # XXX the label may be wrong, if the JUMP_IF_x got rewritten
    if ($r->[2] eq 'I' && $l->[2] eq 'I') {
	print <<"EOC";
	$op $l->[1], $r->[1], $targ $cmt
$label
EOC
    }
    else {
	my $nl = promote($l);
	my $nr = $r->[1];
	$nr = promote($r) if $op =~ /addr/;
	print <<"EOC";
      	$op $nl, $nr, $targ $cmt
$label
EOC
    }
}
sub print_stack {
    for $_ (@stack) {
	print "# STACK $_->[0] $_->[1] $_->[2]\n";
    }
}


sub ret_val {
    my $a = shift;
    my %rets = (
	'id'       => 'I',
    );
    return $rets{$a} if defined $rets{$a};
    return 'P';
}
sub OPC_isinstance
{
    my ($n, $c, $cmt) = @_;
    my $i = temp('I');
    my $cl = pop @stack;
    my $ob = promote(pop @stack);
    pop @stack;	# functions
    my $s = temp('S');
    my $b = temp('P');
    # TODO make op or function
    print <<EOC;
        $s = classname $cl->[1]
	$i = isa $ob, $s
	$b = new Boolean
	$b = $i
EOC
    push @stack, [-1, $b, 'P'];
}

sub OPC_ord
{
    my ($n, $c, $cmt) = @_;
    my $i = temp('I');
    my $p = pop @stack;
    pop @stack;	# functions
    my $s = temp('S');
    print <<EOC;
	$s = $p->[1]
	$i = ord $s
EOC
    push @stack, [-1, $i, 'S'];
}

sub CALL_FUNCTION_VAR
{
    my ($n, $c, $cmt) = @_;
    $n++;	# its for sure not that simple
    # we have a tuple argumen
    my $tupl = $stack[-1];
    print <<EOC;
    # tuple $tupl->[1] n = $tupl->[0]
EOC
    $n = $tupl->[0];
    UNPACK_SEQUENCE($n, '', "\t\t #unpack");
    CALL_FUNCTION($n, $c, $cmt);
}
sub CALL_FUNCTION
{
    my ($n, $c, $cmt) = @_;
    my @args;
    if ($make_f) {
	$make_f = 0;
	print <<EOC;
	# make_f \t$cmt
EOC
	# pop @stack;
	return;
    }
    my $func;
    my $nfix =  ($n & 0xff);
    my $nk =  2*($n >> 8);
    my $name = $stack[-1 - $nfix-$nk]->[0];
    print "\t\t $cmt $name\n";
    if ($builtin_ops{$name} && $builtin_ops{$name} eq 's') {
	no strict "refs";
	my $opcode = "OPC_$name";
	&$opcode($n, $name, $cmt);
	return;
    }

    if ($func_info{$name} && $func_info{$name}{'ar'} ne 'None') {
	my $fix_args = $func_info{$name}{'args'};
	my @fargs = split /,/, $fix_args;
	my $nf = scalar @fargs;
	if ($func_info{$name}{'ar'} ne 'None') {
	    $nfix -= $nf;
	    BUILD_TUPLE($nfix, '', "\t\t #call_args");
	    my $t = pop @stack;
	    unshift @args, $t->[1];
	}
	for (my $i = 0; $i < $nf; $i++) {
	    my $arg = pop @stack;
	    unshift @args, promote($arg);
	}
    }
    else {
	# arguments = $n & 0xff
	# named args: = ($n >> 8) *2
	for (my $i = 0; $i < $nfix; $i++) {
	    my $arg = pop @stack;
	    unshift @args, promote($arg);
	}
	my ($i, $j, $arg_name);
	my $pushed_args = scalar @args;
	#
	# that's wrong, works only for all or none named arguments
	#
	for ($i = 0; $i < $nk; $i+=2,) {
	    my $val = pop @stack;
	    my $arg = pop @stack;
	    my $arg_name = $arg->[1];
	    $j = $def_arg_names{$name}{$arg_name};
	    print <<EOC;
	# func $name named arg $j name $arg_name val $val->[1]
EOC
	    $args[$pushed_args + $j] = promote($val);
	}
	$n = $nfix + $nk/2;
    }
    my $tos = pop @stack;
    my $args = join ', ', @args;
    my $t;
    $func = $tos->[1];
    # create argument tuple
    if ($builtins{$name} && $builtins{$name} eq 'v') {
	my $ar = temp('P');
	print <<"EOC";
	$ar = new .FixedPMCArray
	$ar = $n
EOC
	$cmt .= "   $name";
	for (my $i = 0; $i < $n; $i++) {
	    print <<"EOC";
	$ar\[$i\] = $args[$i]
EOC
	}
	$args = $ar;
    }
    my $rett = 'P';
    if ($tos->[2] eq 'o') {	# builtin opcode
	$t = temp('P');
	print <<EOC;
	$t = new $DEFVAR   # builtin opcode
	$t = $func $args   $cmt
EOC
    }
    elsif ($name =~/^obj (\S+) attr (\w+)/) {  # convert to meth call syntax
	my ($obj, $attr) = ($1, $2);
	my $ret_type = ret_val($attr);
	my $ret_string = "";
	if ($ret_type ne 'None') {
	    $t = temp($rett = $ret_type);
	    $ret_string = "$t = ";
	}
	if (!nci_method($attr)) {	# a method function
	    print <<EOC;
	P2 = $1   # obj '$obj' attr '$attr'
	${ret_string}$func($args)  $cmt
EOC
	}
	else {
	    print <<EOC;
	.local NCI meth\:\:$attr
	meth\:\:$attr = $func # avoid savetop
	P2 = $1
	${ret_string}meth\:\:$attr($args)  $cmt
EOC
	}
    }
    else {
	my $ret_type = ret_val($func);
	my $ret_string = "";
	if ($ret_type ne 'None') {
	    $t = temp($rett = $ret_type);
	    $ret_string = "$t = ";
	}
	if ($builtins{$name}) {
	    print <<EOC;
	.local NCI the::internal
	the::internal = $func # avoid savetop
EOC
	    $func = 'the::internal';
	}
	print <<EOC;
	$ret_string$func($args)  $cmt
EOC
    }
    my $opcode = $code[$code_l]->[2];
    if ($opcode eq 'POP_TOP') {
	print "# POP_TOP\n";
	$code_l++;
    }
    else {
	if (!$t) {
	    $t = temp('P');
	    print <<EOC;
	$t = P5
EOC
	}
	push @stack, [$name, $t, $rett];
    }
}

sub POP_TOP
{
    my ($n, $c, $cmt) = @_;
    print "\t\t$cmt\n";
    #pop @stack;
}
my @fast;
sub LOAD_FAST
{
    my ($n, $c, $cmt) = @_;
    my $p;
    if ($p=$lexicals{$c}) {
	$n = $fast[$n]->[0];
	print <<EOC;
	\t # '$c' := $p $cmt
EOC
	$c = $p;
    }
    else {
	my $p = 5 + keys %params;
	$params{$c} = $c;
	$lexicals{$c} = $c;
	$names{$c} = $c;
	print <<EOC;
        \t $cmt
EOC
    }
    push @stack, [$n, $c, 'P'];
}

sub STORE_FAST
{
    my ($n, $c, $cmt) = @_;
    my $tos = pop @stack;
    $fast[$n] = $tos;
    my $p;
    if ($p = $lexicals{$c}) {
	if ($p eq $tos->[1]) {
	    print <<"EOC";
	\t $cmt
EOC
	}
	else {
	    if ($may_be_none{$c}) {
		delete $may_be_none{$c};
		print <<"EOC";
	    ne_addr $c, None, temp_$code_l
	    $c = new $DEFVAR
temp_$code_l:
EOC
	    }
	    print <<"EOC";
	# assign $c, $tos->[1] $cmt
	set $p, $tos->[1] $cmt
EOC
	    $lexicals{$c} = $p;
	}
    }
    else {
	$lexicals{$c} = promote($tos);
	print <<"EOC";
        \t $cmt
EOC
    }
}

sub UNARY_CONVERT
{
    my ($n, $c, $cmt) = @_;
    my $tos = pop @stack;
    my $p = promote($tos);
    my $s = temp('P');
    print <<EOC;
	$s = $p."__get_repr"() $cmt
EOC
    push @stack, [-1, $s, 'P'];
}

sub BUILD_TUPLE
{
    my ($n, $c, $cmt, $type) = @_;
    $type = "FixedPMCArray" unless defined $type;
    my ($opcode, $rest) = ($code[$code_l]->[2],$code[$code_l]->[4]);
    if ($opcode eq 'UNPACK_SEQUENCE') {
	$code_l++;
	print "\t\t$cmt + UNPACK_SEQUENCE\n";
	# have to reverse n stack elems
	my ($i, @rev);
	for ($i = 0; $i < $n; $i++) {
	    push @rev, pop @stack;
	}
	push @stack, @rev;

	return;
    }
    my $ar = temp('P');
    print <<EOC;
	$ar = new $type $cmt
	$ar = $n
EOC
    for (my $i = $n-1; $i >= 0; $i--) {
	my $p = pop @stack;
	print <<EOC;
	$ar\[$i\] = $p->[1]
EOC
    }
    push @stack, [$n, $ar, 'P'];
}

sub BUILD_LIST
{
    BUILD_TUPLE(@_,"ResizablePMCArray")
}
sub BUILD_MAP
{
    my ($n, $c, $cmt) = @_;
    my $ar = temp('P');
    print <<EOC;
	$ar = new PerlHash $cmt
EOC
    push @stack, ["hash", $ar, 'P'];
}
sub RAISE_VARARGS
{
    my ($n, $c, $cmt) = @_;
    my $throw;
    if ($n == 0) {
	$throw = 'rethrow P5';
    }
    elsif ($n == 1) {
	my $x = (pop @stack)->[1];
	$throw = "throw $x $cmt";
    }
    else {
	for (my $i = $n-1; $i > 0; $i--) {
	    my $p = pop @stack;
	    print <<EOC;
	# arg $p->[1]
EOC
	}
	my $x = (pop @stack)->[1];
	print <<EOC;
	$x\["_message"\] = "Foo"
EOC
	$throw = "throw $x # TODO create, args";
    }
    print <<EOC;
	$throw $cmt
EOC
}
sub GET_ITER
{
    my ($n, $c, $cmt) = @_;
    my $it = temp('P');
    my $tos = pop @stack;
    my $var = promote($tos);
    print <<EOC;
	$it = iter $var $cmt
EOC
    push @stack, [$tos->[0], $it, 'P']
}
sub FOR_ITER
{
    my ($n, $c, $cmt) = @_;
    my $targ = "pc_xxx";
    my $tos = pop @stack;
    my $iter = $tos->[1];
    if ($c =~ /to (\d+)/) {
	$targ = "pc_$1";
    }
    my $var = temp('P');
    my $name = $tos->[0];
    print <<EOC;
	unless $iter goto $targ # $tos->[0]
	$var = shift $iter $cmt
EOC
    push @stack, [-1, $var, 'P']
}


sub UNPACK_SEQUENCE
{
    my ($n, $c, $cmt) = @_;
    my $tos = pop @stack;
    my $seq = $tos->[1];
    my ($p, $i);
    for ($i = $n-1; $i >= 0; $i--) {
	$p = temp('P');
	print <<EOC;
	$p = $seq\[$i\] $cmt
EOC
	push @stack, [-1, $p, 'P'];
    }
}

sub DUP_TOP
{
    my ($n, $c, $cmt) = @_;
    my $tos = $stack[-1];
    print <<EOC;
	$cmt
EOC
    push @stack, $tos;
}

sub DUP_TOPX
{
    my ($n, $c, $cmt) = @_;
    foreach (1..$n) {
	my $thing = $stack[-$n];
	push @stack, $thing;
    print <<EOC;
	$cmt
EOC
    }
}

sub ROT_THREE
{
    my ($n, $c, $cmt) = @_;
    print "\t\t$cmt\n";
    my $v = pop @stack;
    my $w = pop @stack;
    my $x = pop @stack;
    push @stack, $v;
    push @stack, $x;
    push @stack, $w;
}

sub ROT_TWO
{
    my ($n, $c, $cmt) = @_;
    print "\t\t$cmt\n";
    my $v = pop @stack;
    my $w = pop @stack;
    push @stack, $w;
    push @stack, $v;
}

sub STORE_SUBSCR
{
    my ($n, $c, $cmt) = @_;
    my $x = pop @stack;
    my $v = pop @stack;
    my $w = pop @stack;
    my $key = $x->[1];
    if ($v->[0] eq 'hash') {
	if ($key =~ /^\d+$/) {
	    $key = qq!"$key"!;
	}
	elsif ($v->[2] eq 'I') {
	    # ok ?
	}
    }
    print <<EOC
	$v->[1]\[$key\] = $w->[1] $cmt
EOC
}

sub BINARY_SUBSCR
{
    my ($n, $c, $cmt) = @_;
    my $w = pop @stack;
    my $v = pop @stack;
    my $x = temp('P');
    my $agg = promote($v);
    print <<EOC;
	$x = $agg\[$w->[1]\] $cmt
EOC
    push @stack, [-1, $x, 'P'];
}
# exceptions
sub SETUP_EXCEPT
{
    my ($n, $c, $cmt) = @_;
    my $targ = "pc_xxx";
    if ($c =~ /to (\d+)/) {
	$targ = "pc_$1";
    }
    my $eh = temp('P');
    print <<EOC;
	newsub $eh, .Exception_Handler, $targ $cmt
	set_eh $eh
EOC
}
sub SETUP_FINALLY
{
    my ($n, $c, $cmt) = @_;
    SETUP_EXCEPT($n, $c, $cmt);
}
sub END_FINALLY
{
    my ($n, $c, $cmt) = @_;
    print <<EOC;
	throw P5 $cmt
EOC
}

sub SETUP_LOOP
{
    my ($n, $c, $cmt) = @_;
    my $targ = "pc_xxx";
    if ($c =~ /to (\d+)/) {
	$targ = "pc_$1";
    }
    push @loops, $targ;
    my $eh = temp('P');
    print <<EOC;
	newsub $eh, .Exception_Handler, $targ $cmt
	set_eh $eh
EOC
}
sub POP_BLOCK
{
    my ($n, $c, $cmt) = @_;
    if (@loops) {
	my $pc = pop @loops;
	print <<EOC;
	# $pc  $cmt
	clear_eh
EOC
    }
    else {
	print <<EOC;
	\t\t$cmt
EOC
    }
}

sub BREAK_LOOP
{
    my ($n, $c, $cmt) = @_;
    my $pc = pop @loops;
    print <<EOC;
	goto $pc $cmt
EOC
}

sub BUILD_CLASS
{
    my ($n, $c, $cmt) = @_;
    my $parent_tuple = pop @stack;
    my $tos = pop @stack;
    my $cl = temp('P');
    my $name = $tos->[1];
    $n = $name;
    $n =~ s/["]//g;
    my $mangle = qq!"py::$n"!;
    $classes{$n} = $mangle;
    print <<EOC;
	$cl = subclass $parent_tuple->[1], $mangle $cmt
	global $name = $cl
	Build::$n($cl)
EOC
    push @stack, ["class $tos->[1]", $cl, 'P'];
}
sub LOAD_ATTR
{
    my ($n, $c, $cmt) = @_;
    my $tos = pop @stack;  # object
    my $attr = temp('P');
    my $obj = promote $tos;
    my $o;
    if ($builtins{$obj}) { # postponed LOAD_ like dict
	$o = temp('P');
	my $args = "";
	if ($builtins{$obj} eq 'v') {
	    my $arg = temp('P');
	    print <<EOC;
	$arg = new FixedPMCArray
EOC
	    $args = $arg;
	}
	print <<EOC;
	$o = $obj($args) 		# postponed LOAD_
EOC
	$obj = $o;
    }
    my $a = $c;
    if ($vtables{$c}) {
	$c = $vtables{$c};
    }
    my $cc;
    if ($c =~ /^\d+$/) {	# MMD
	$cc = "-$c";
    }
    else {
	$cc = qq!"$c"!;
    }
    print <<EOC;
	$attr = getattribute $obj, $cc $cmt
EOC
    push @stack, ["obj $obj attr $a", $attr, 'P'];
}

sub STORE_ATTR
{
    my ($n, $c, $cmt) = @_;
    my $obj = pop @stack;  # object
    my $val = promote(pop @stack);
    if ($vtables{$c}) {
	$c = $vtables{$c};
    }
    my $cc;
    if ($c =~ /^\d+$/) {	# MMD
	$cc = "-$c";
    }
    else {
	$cc = qq!"$c"!;
    }
    print <<EOC;
	setattribute $obj->[1], $cc, $val $cmt
EOC
}

sub Slice
{
    my ($n, $c, $cmt, $sl_n) = @_;
    my ($v, $w, $vv, $ww);
    $vv = 0;
    $ww =  "";
    if ($sl_n & 2) {
	$w = pop @stack;
	$ww = $w->[1];
	if ($w->[2] eq 'P') {
	   $ww = temp('I');
	   print <<EOC;
	$ww = $w->[1]
EOC
       }
    }
    if ($sl_n & 1) {
	$v = pop @stack;
	$vv = $v->[1];
	if ($v->[2] eq 'P') {
	   $vv = temp('I');
	   print <<EOC;
	$vv = $v->[1]
EOC
       }
    }
    my $ag = promote(pop @stack);
    my $a = temp('P');
    print <<EOC;
	\t $cmt
	$a = slice $ag\[ $vv .. $ww ], 1
EOC
    push @stack, [-1, $a, 'P'];
}

sub SLICE_plus_0 {
    return Slice(@_, 0);
}
sub SLICE_plus_1 {
    return Slice(@_, 1);
}
sub SLICE_plus_2 {
    return Slice(@_, 2);
}
sub SLICE_plus_3 {
    return Slice(@_, 3);
}

sub Store_Slice
{
    my ($n, $c, $cmt, $sl_n) = @_;
    my ($v, $w, $vv, $ww);
    $vv = 0;
    $ww =  "";
    if ($sl_n & 2) {
	$w = pop @stack;
	$ww = $w->[1];
	if ($w->[2] eq 'P') {
	   $ww = temp('I');
	   print <<EOC;
	$ww = $w->[1]
EOC
       }
    }
    if ($sl_n & 1) {
	$v = pop @stack;
	$vv = $v->[1];
	if ($v->[2] eq 'P') {
	   $vv = temp('I');
	   print <<EOC;
	$vv = $v->[1]
EOC
       }
    }
    my $dest =  (pop @stack)->[1];
    my $ag = (pop @stack)->[1];
    print <<EOC;
	\t $cmt
	set $dest\[ $vv .. $ww ], $ag
EOC
    #push @stack, [-1, $dest, 'P'];
}
sub STORE_SLICE_plus_0 {
    return Store_Slice(@_, 0);
}
sub STORE_SLICE_plus_1 {
    return Store_Slice(@_, 1);
}
sub STORE_SLICE_plus_2 {
    return Store_Slice(@_, 2);
}
sub STORE_SLICE_plus_3 {
    return Store_Slice(@_, 3);
}

sub DELETE_SLICE_plus_0 {
    my ($n, $c, $cmt) = @_;
    my $agg = pop @stack;
    print <<EOC;
	$agg->[1] = 0
EOC
}

sub Del_Slice
{
    my ($n, $c, $cmt, $sl_n) = @_;
    my ($v, $w, $vv, $ww);
    $vv = 0;
    $ww =  "";
    if ($sl_n & 2) {
	$w = pop @stack;
	$ww = $w->[1];
	if ($w->[2] eq 'P') {
	   $ww = temp('I');
	   print <<EOC;
	$ww = $w->[1]
EOC
       }
    }
    if ($sl_n & 1) {
	$v = pop @stack;
	$vv = $v->[1];
	if ($v->[2] eq 'P') {
	   $vv = temp('I');
	   print <<EOC;
	$vv = $v->[1]
EOC
       }
    }
    my $dest =  (pop @stack)->[1];
    print <<EOC;
	\t $cmt
	delete $dest\[ $vv .. $ww ]
EOC
    #push @stack, [-1, $dest, 'P'];
}
sub DELETE_SLICE_plus_1 {
    return Del_Slice(@_, 1);
}
sub DELETE_SLICE_plus_2 {
    return Del_Slice(@_, 2);
}
sub DELETE_SLICE_plus_3 {
    return Del_Slice(@_, 3);
}
sub DELETE_FAST {
    my ($n, $c, $cmt) = @_;
    print <<EOC;
	\t $cmt
EOC
}

sub IMPORT_NAME {
    my ($n, $c, $cmt) = @_;
    pop @stack;
    print <<EOC;
	\t $cmt XXX
EOC
}
sub IMPORT_FROM {
    my ($n, $c, $cmt) = @_;
    print <<EOC;
	\t $cmt XXX
EOC
    # push @stack, [-1, 'time', 'P'];
    $code_l++;
}

# vim: sw=4 tw=70:
