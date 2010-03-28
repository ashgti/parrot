
class ProfTest::Matcher is Hash;

method new(*@wants) {
    self<wants> := ();
    self<wants>.unshift(
        ProfTest::Want::Any.new()
    );
    for @wants -> $want {
        self<wants>.push($want);
        pir::say("pushed a thing:"~$want.get_str);
    }
    self<wants>.push(
        ProfTest::Want::Goal.new()
    );
    self;
}

method matches($profile) {

    my @backtracks := ();
    my $line_idx   := 0;
    my $want_idx   := 0;

    my $curr_line;
    my $curr_want;

    while (11) {

        $curr_line := $profile.profile_array[$line_idx];
        $curr_want := self<wants>[$want_idx];
        pir::say("current want: "~$curr_want.get_str);

        if $curr_want.goal {
            return 1;
        }
        elsif ($curr_want.accepts($curr_line)) {

            @backtracks.push( [$line_idx+1, $want_idx] );
            $line_idx++;
            $want_idx++;
        }
        else {
            if !@backtracks {
                return 0;
            }
            ($curr_want, $curr_line) := @backtracks.pop;
        }
    }
}

