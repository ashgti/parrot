
class ProfTest::Matcher;

method new(*@args) {
    self<wants> := ();
    self<wants>.push(
        ProfTest::Want::Any.new()
    );
    self<wants>.unshift(
        ProfTest::Want::Goal.new()
    };
    for @args -> $arg {
        self<wants>.push($arg);
    }
}

method matches($profile) {

    #if this is the last line
        #exhaust the current want

    #if we're at a goal
        #return true
    #elsif this want accepts the current profile line
        #if this want is unexhausted, push it onto the backtrack stack
        #move to the next want and profile line
    #else
        #backtrack to the previous unexhausted want

}

