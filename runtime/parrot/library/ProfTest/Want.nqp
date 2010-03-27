
class ProfTest::Want is Hash;

method new() { die('...'); }

method accepts() { 1; }

method hashify_variable_data($data) {
    my %h := {};
    for $data -> $match {
        %h{ $match<field_name> } := $match<field_data>;
    }
    %h;
}

method exhaust($x? = 1) {
    self<exhausted> := $x;
}

method exhausted() { self<exhausted>; }

method goal() { 0; }


class ProfTest::Want::Goal;

method new() { }

method goal() { 1; }



class ProfTest::Want::Any;

method new(@except?) {
    self<except>     := @except;
    self<cursor_pos> := 0;
    self<exhausted>  := 0;
}

method accepts($prof_line) {
    my $line_type := $prof_line<variable_line> ?? 
        $prof_line<variable_line><line_type> !! 
        $prof_line<variable_line><line_type> ;

    for self<except> -> $except_type {
        if $except_type eq $line_type {
            return 0;
        }
    }
    return 1;
}



class ProfTest::Want::Version is ProfTest::Want;

method new($version?) {
    self<version>   := $version;
    self<exhausted> := 0;
}

method accepts($prof_line) {
    if $prof_line<fixed_line> &&
        $prof_line<fixed_line><line_type> eq 'VERSION' {
        self.exhaust;
        return 1;
    }
}



class ProfTest::Want::CLI is ProfTest::Want;

method new() { self<exhausted> := 1; }

method accepts($prof_line) {
    if $prof_line<fixed_line> &&
        $prof_line<fixed_line><line_type> eq 'CLI' {
        self.exhaust;
        return 1;
    }
}



class ProfTest::Want::EndOfRunloop is ProfTest::Want;

method new() { }

method accepts($prof_line) {
    if $prof_line<fixed_line> &&
        $prof_line<fixed_line><line_type> eq 'END_OF_RUNLOOP' {
        self.exhaust;
        return 1;
    }
}



class ProfTest::Want::Op is ProfTest::Want;

method new($name, $line?) {
    self<name> := $name;
    self.exhaust;
    if $line {
        self<line> := $line;
    }
}

method accepts($prof_line) {
    if $prof_line<variable_line> && $prof_line<variable_line><line_type> eq 'OP' {
        my %variable_data := self.hashify_variable_data($prof_line<variable_line><variable_data>);
        if self<name> ne %variable_data<op> {
            return 0;
        }
        if self<line> && self<line> != %variable_data<line> {
            return 0;
        }

    }
    self.exhaust;
    return 1;
}



class ProfTest::Want::CS is ProfTest::Want;

method new($ns?, :$slurp_until?) {
    self<ns> := $ns;
    self<slurp_until> := $slurp_until;
    self<found_cs> := 0;
    self.exhaust;
}

method accepts($prof_line) {
    if self<found_cs> && self<slurp_until> {
        return $prof_line<variable_line><line_type> ne self<slurp_until>;
    }
    elsif $prof_line<variable_line> && $prof_line<variable_line><line_type> eq 'CS' {
        if !self<ns> {
            self<found_cs>  := 1;
            self<exhausted> := !?self<slurp_until>;
            return 1;
        }
        my %h := self.hashify_variable_data($prof_line<variable_line><variable_data>);
        if %h<ns> eq self<ns> {
            self<found_cs> := 1;
            return 1;
        }
    }
    return 0;
}
