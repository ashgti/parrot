use Parrot::Vtable;
my %vtable = parse_vtable();

while (<DATA>) {
    next if /^#/ or /^$/;
    my @params = split;
    my $op = $params[1];
    my $vtable_entry = $params[2] || $op;
    die "Can't find $vtable_entry in vtable, line $.\n"
        unless exists $vtable{$vtable_entry};
    print "AUTO_OP $params[1] (".(join ", ", ("p")x$params[0]).") {\n";
    print "\t(\$2->vtable->$vtable_entry";
    print multimethod($vtable_entry);
    print ")(interpreter, ";
    if ($params[0] == 3) {
        # Three-address function
        print '$2,$3,$1';
    } elsif ($params[0] == 2) {
        # Unary function
        print '$2,$1';
    }
    print ");\n}\n";
}

sub multimethod {
    my $type = $vtable{$_[0]}{meth_type};
    return ""               if $type eq "unique";
    return '[$3->vtable->num_type]' if $type eq "num";
    return '[$3->vtable->string_type]' if $type eq "str";
    die "Coding error - undefined type $type\n";
}


__DATA__
# Three-address functions
3 add
3 sub subtract
3 mul multiply
3 div divide
3 mod modulus
3 concat concatenate
3 and logical_and
3 or logical_or
# Unary functions
2 not logical_not
