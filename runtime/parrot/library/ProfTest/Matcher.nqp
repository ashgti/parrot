
class ProfTest::Matcher;

method new(*@args) {
    self<wants> := ();
    self<wants>.push(
        ProfTest::Want::Any.new()
    );
    self<wants>.unshift(
        ProfTest::Want::Goal.new()
    );
    for @args -> $arg {
        self<wants>.push($arg);
    }
}

method matches($profile) {

    my @backtracks := ();
    my $line_idx   := 0;
    my $want_idx   := 0;

    my $curr_line := $profile.profile_array[$line_idx];
    my $curr_want := self<wants>[$want_idx];

    while (11) {
        if $curr_want.goal {
            return 1;
        }
        elsif ($curr_want.accepts($curr_line)) {

            @backtracks.push( [$line_idx+1, $want_idx] );
            $line_idx++;
            $want_idx++;
            $curr_line := $profile.profile_array[$line_idx];
            $curr_want := self<wants>[$want_idx];
        }
        else {
            if !@backtracks {
                return 0;
            }
            ($curr_want, $curr_line) := @backtracks.pop;
            $curr_line := $profile.profile_array[$line_idx];
            $curr_want := self<wants>[$want_idx];
        }
    }
}

