#!/usr/bin/env parrot-nqp

INIT {
    # Load the Test::More library
    pir::load_language('parrot');
    pir::compreg__PS('parrot').import('Test::More');

    pir::load_bytecode('dumper.pbc');
}


sub get_profile_array($pprof) {

    my @pprof_lines := pir::split("\n", $pprof);
    my @pprof := ();

    grammar pprof_line {
        rule TOP { ^^ [ <fixed_line> | <variable_line> ] $$ }
        
        rule fixed_line      { <fixed_line_type> ':' <fixed_line_data> }
        rule fixed_line_type { [ 'VERSION' | 'CLI' | 'END_OF_RUNLOOP' ] }
        rule fixed_line_data { \N* }

        rule variable_line      { <variable_line_type> ':' <variable_line_data>* }
        rule variable_line_type { [ 'CS' | 'OP' ] }
        rule variable_line_data { '{x{' <field_name> ':' <field_data> '}x}' }
        rule field_name         { <.ident> }
        rule field_data         { <[a..zA..Z0..9_\-]>* }
    }

    for @pprof_lines -> $line {
        my $line_match := pprof_line.parse($line);
        #pir::say($line);
        _dumper($line_match);
        @pprof.push($line_match);
    }
    @pprof;
}

sub get_profile_from_pir($pir, $canonical? = 1) {

    my $tmp_pir := '/tmp/test.pir';
    my $tmp_pprof := '/tmp/test.pprof';
    my $fh := pir::new__p_sc('FileHandle');
    $fh.open($tmp_pir, "w");
    $fh.puts($pir);
    $fh.close();

    my %env := pir::new__p_sc('Env');
    %env{'PARROT_PROFILING_FILENAME'} := $tmp_pprof;
    if $canonical {
        %env{'PARROT_PROFILING_CANONICAL_OUTPUT'} := 1;
    }
    
    my %config := get_config();
    my $parrot_exe := %config<prefix> ~ %config<slash> ~ %config<test_prog>;

    my $cli := "$parrot_exe --hash-seed=1234 --runcore profiling $tmp_pir";  

    my $pipe := pir::new__p_sc('FileHandle');
    $pipe.open($cli, "rp");
    $pipe.readall();
    my $exit_status := $pipe.exit_status();

    my $pprof_fh := pir::new__p_sc('FileHandle');
    $pprof_fh.readall($tmp_pprof);
}

sub get_config() {
    return Q:PIR {
        .include 'iglobals.pasm'
        .local pmc i
        i = getinterp
        %r = i[.IGLOBALS_CONFIG_HASH]
    };
}
        
