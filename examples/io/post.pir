#!parrot
# Copyright (C) 2010, Parrot Foundation.

.include 'iglobals.pasm'

.sub 'send_archive_to_smolder' :main
    .local pmc config
    $P0 = getinterp
    config = $P0[.IGLOBALS_CONFIG_HASH]
    .local pmc contents
    contents = new 'ResizablePMCArray' # by couple
    push contents, 'architecture'
    $S0 = config['cpuarch']
    push contents, $S0
    push contents, 'platform'
    $S0 = config['osname']
    push contents, $S0
    push contents, 'revision'
    $S0 = config['revision']
    push contents, $S0
    push contents, 'username'
    push contents, 'parrot-autobot'
    push contents, 'password'
    push contents, 'squ@wk'
    push contents, 'comments'
    push contents, "EXPERIMENTAL LWP.pir"
    push contents, 'report_file'
    $P0 = new 'FixedStringArray'
    set $P0, 6
    $P0[0] = 'parrot_test_run.tar.gz' # filename
    $P0[1] = 'parrot_test_run.tar.gz' # usename
    $P0[2] = 'Content-Type'
    $P0[3] = 'application/x-tar'
    $P0[4] = 'Content-Encoding'
    $P0[5] = 'gzip'
    push contents, $P0
    load_bytecode 'LWP.pir'
    .const string url = 'http://smolder.plusthree.com/app/projects/process_add_report/8'
    .local pmc ua, response
    ua = new ['LWP';'UserAgent']
    ua.'show_progress'(1)
    response = ua.'post'(url, contents :flat, 'form-data' :named('Content-Type'), 'TE, close' :named('Connection'), 'deflate,gzip;q=0.3' :named('TE'))
.end

# Local Variables:
#   mode: pir
#   fill-column: 100
# End:
# vim: expandtab shiftwidth=4 ft=pir:
