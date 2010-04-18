# Copyright (C) 2010, Parrot Foundation.
# $Id$

=head1 NAME

TAP/Harness

=head2 DESCRIPTION

Simplified port of TAP::Harness (version 3.21)
and TAP::Harness::Archive (version 0.14)

See L<http://search.cpan.org/~andya/Test-Harness/>
end L<http://search.cpan.org/~wonko/TAP-Harness-Archive/>.

=cut

.namespace ['TAP';'Harness']

.sub '' :init :load :anon
    load_bytecode 'TAP/Parser.pbc'
    load_bytecode 'TAP/Formatter.pbc'
    $P0 = subclass ['TAP';'Base'], ['TAP';'Harness']
    $P0.'add_attribute'('formatter')
    $P0.'add_attribute'('exec')
    $P0.'add_attribute'('opts')

    $P0 = new 'Hash'
    $P1 = split ' ', 'parser_args made_parser before_runtests after_runtests after_test'
  L1:
    unless $P1 goto L2
    $S0 = shift $P1
    $P0[$S0] = 1
    goto L1
  L2:
    set_global ['TAP';'Harness'], 'LEGAL_CALLBACK', $P0
.end

.sub 'init' :vtable :init
    $P0 = get_global ['TAP';'Harness'], 'LEGAL_CALLBACK'
    setattribute self, 'ok_callbacks', $P0
.end

.sub 'process_args' :method
    .param pmc opts
    setattribute self, 'opts', opts
    $I0 = exists opts['exec']
    unless $I0 goto L1
    $S0 = opts['exec']
    $P0 = box $S0
    setattribute self, 'exec', $P0
  L1:
.end

.sub 'formatter' :method
    .param pmc formatter
    setattribute self, 'formatter', formatter
.end

.sub 'runtests' :method
    .param pmc tests
    $P0 = getattribute self, 'formatter'
    unless null $P0 goto L1
    $P0 = new ['TAP';'Formatter';'Console']
    setattribute self, 'formatter', $P0
    $P1 = getattribute self, 'opts'
    $P0.'process_args'($P1)
  L1:
    .local pmc aggregate
    aggregate = new ['TAP';'Parser';'Aggregator']
    self.'_make_callback'('before_runtests', aggregate)
    aggregate.'start'()
    self.'aggregate_tests'(aggregate, tests)
    aggregate.'stop'()
    self.'summary'(aggregate)
    self.'_make_callback'('after_runtests', aggregate)
    .return (aggregate)
.end

.sub 'aggregate_tests' :method
    .param pmc aggregate
    .param pmc tests
    $P0 = getattribute self, 'formatter'
    $P0.'prepare'(tests)
    .local string exec
    exec = ''
    $P0 = getattribute self, 'exec'
    if null $P0 goto L1
    exec = $P0
  L1:
    $P0 = iter tests
  L2:
    unless $P0 goto L3
    $S0 = shift $P0
    .local pmc parser, session
    (parser, session) = self.'make_parser'($S0)
    unless exec == '' goto L4
    parser.'file'($S0)
    goto L5
  L4:
    parser.'exec'(exec, $S0)
  L5:
    .local pmc coro
    $P1 = get_hll_global ['TAP';'Parser'], 'next'
    coro = newclosure $P1
  L6:
    .local pmc result
    result = coro(parser)
    if null result goto L7
    session.'result'(result)
    $I0 = isa result, ['TAP';'Parser';'Result';'Bailout']
    unless $I0 goto L6
    self.'_bailout'(result)
  L7:
    self.'finish_parser'(parser, session)
    self.'_after_test'(aggregate, $S0, parser)
    goto L2
  L3:
.end

.sub '_after_test' :method
    .param pmc aggregate
    .param pmc desc
    .param pmc parser
    self.'_make_callback'('after_test', parser)
    aggregate.'add'(desc, parser)
.end

.sub '_bailout' :method
    .param pmc result
    $S0 = ".\n"
    $P0 = result.'bailout'()
    if null $P0 goto L1
    $S0 = $P0
  L1:
    $S0 = "FAILED--Further testing stopped" . $S0
    die $S0
.end

.sub 'summary' :method
    .param pmc aggregate
    $P0 = getattribute self, 'formatter'
    $P0.'summary'(aggregate)
.end

.sub 'make_parser' :method
    .param string desc
    .local pmc parser, session
    parser = new ['TAP';'Parser']
    self.'_open_spool'(parser, desc)
    self.'_make_callback'('made_parser', parser)
    $P0 = getattribute self, 'formatter'
    session = $P0.'open_test'(desc, parser)
    .return (parser, session)
.end

.sub 'finish_parser' :method
    .param pmc parser
    .param pmc session
    session.'close_test'()
    self.'_close_spool'(parser)
    .return (parser)
.end

.sub '_open_spool' :method
    .param pmc parser
    .param string test
    $P0 = new 'Env'
    $I0 = exists $P0['PARROT_TEST_HARNESS_DUMP_TAP']
    unless $I0 goto L1
    .local string spool
    spool = $P0['PARROT_TEST_HARNESS_DUMP_TAP']
    spool .= '/'
    spool .= test
    $S0 = dirname(spool)
    mkpath($S0)
    $P0 = new 'FileHandle'
    $P0.'open'(spool, 'w')
    parser.'spool'($P0)
  L1:
.end

.sub '_close_spool' :method
    .param pmc parser
    $P0 = parser.'delete_spool'()
    if null $P0 goto L1
    close $P0
  L1:
.end


.namespace ['TAP';'Harness';'Archive']

.sub '' :init :load :anon
    load_bytecode 'osutils.pbc'
    $P0 = subclass ['TAP';'Harness'], ['TAP';'Harness';'Archive']
    $P0.'add_attribute'('archive_file')
    $P0.'add_attribute'('archive_extra_files')
    $P0.'add_attribute'('archive_extra_props')
.end

.sub 'archive' :method
    .param pmc archive
    setattribute self, 'archive_file', archive
.end

.sub 'extra_files' :method
    .param pmc extra_files
    $I0 = does extra_files, 'array'
    if $I0 goto L1
    die "extra_files must be an array!"
  L1:
    setattribute self, 'archive_extra_files', extra_files
.end

.sub 'extra_props' :method
    .param pmc extra_props
    $I0 = does extra_props, 'hash'
    if $I0 goto L1
    die "extra_props must be an hash!"
  L1:
    setattribute self, 'archive_extra_props', extra_props
.end

.sub 'runtests' :method
    .param pmc files
    $P0 = getattribute self, 'archive_file'
    unless null $P0 goto L1
    die "You must provide the name of the archive to create!"
  L1:
    .local string archive, dir
    archive = $P0
    dir = tempdir()
    .local pmc env
    env = new 'Env'
    env['PARROT_TEST_HARNESS_DUMP_TAP'] = dir
    .local pmc aggregate
    $P0 = get_hll_global ['TAP';'Harness'], 'runtests'
    aggregate = $P0(self, files)
    .local string current_dir, cmd
    current_dir = cwd()
    chdir(dir)
    $S0 = self.'_mk_meta'(aggregate)
    spew('meta.yml', $S0)
    $I0 = length archive
    $I0 -= 3
    $S0 = substr archive, 0, $I0
    cmd = "tar -cf " . current_dir
    cmd .= "/"
    cmd .= $S0
    cmd .= " *"
    system(cmd)
    chdir(current_dir)
    cmd = "gzip --best " . $S0
    system(cmd)
    rmtree(dir)
    .return (aggregate)
.end

.sub '_mk_meta' :method
    .param pmc aggregate
    $S0 = "---"
    $S0 .= "\nfile_attributes:"
    $P0 = aggregate.'descriptions'()
    $P1 = iter $P0
  L1:
    unless $P1 goto L2
    $S1 = shift $P1
    .local pmc parser
    parser = aggregate.'parsers'($S1)
    $S0 .= "\n  -"
    $S0 .= "\n    description: "
    $S0 .= $S1
    $N0 = parser.'start_time'()
    $S0 .= "\n    start_time: "
    $S1 = $N0
    $S0 .= $S1
    $N0 = parser.'end_time'()
    $S0 .= "\n    stop_time: "
    $S1 = $N0
    $S0 .= $S1
    goto L1
  L2:
    $S0 .= "\nfile_order:"
    $P1 = iter $P0
  L3:
    unless $P1 goto L4
    $S1 = shift $P1
    $S0 .= "\n  - "
    $S0 .= $S1
    goto L3
  L4:
    $I0 = aggregate.'start_time'()
    $S0 .= "\nstart_time: "
    $S1 = $I0
    $S0 .= $S1
    $I0 = aggregate.'end_time'()
    $S0 .= "\nstop_time: "
    $S1 = $I0
    $S0 .= $S1
    $P0 = getattribute self, 'archive_extra_props'
    if null $P0 goto L5
    $S0 .= "\nextra_properties:"
    $P1 = iter $P0
  L6:
    unless $P1 goto L5
    .local string key, value
    key = shift $P1
    value = $P0[key]
    if value == '' goto L6
    $S0 .= "\n  "
    $S0 .= key
    $S0 .= ": "
    $S0 .= value
    goto L6
  L5:
    $S0 .= "\n"
    .return ($S0)
.end


=head1 AUTHOR

Francois Perrad

=cut

# Local Variables:
#   mode: pir
#   fill-column: 100
# End:
# vim: expandtab shiftwidth=4 ft=pir:
