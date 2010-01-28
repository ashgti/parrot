# Copyright (C) 2009, Parrot Foundation.
# $Id$

=head1 NAME

distutils - Parrot Distribution Utilities

=head2 DESCRIPTION

This module is greatly inspired by Python Distribution Utilities
(L<http://docs.python.org/distutils/>).

Its goal is to make Parrot modules and extensions easily available
to a wider audience with very little overhead for build/release/install mechanics.

All the rules needed (dynops, dynpmc, pbc_to_exe, nqp, ...) are coded in this module distutils.
A module author just must write a script C<setup.pir> (or C<setup.nqp> in future).

A setup script can be as simple as this:

    pir::load_bytecode('distutils.pir');
    setup( @steps,
        ... many key/values here ...
    );

Distutils could work with Plumage (L<https://trac.parrot.org/parrot/wiki/ModuleEcosystem>).
Plumage handles setup.pir commands.

Distutils could generate a skeleton of Plumage metadata.

=head3 Commands / Steps / Targets

The default are :

=over 4

=item build

Build the library. (it is the default command)

=item test

Run the test suite.

=item install

Install the library.

=item uninstall

Uninstall the library.

=item clean

Basic cleaning up.

=item update

Update from the repository.

=item plumage

Output a skeleton for Plumage

=item sdist, sdist_gztar, sdist_bztar, sdist_zip, sdist_rpm, manifest

Create a source distribution or a source RPM package

=item bdist, bdist_rpm, bdist_wininst, spec, control, ebuild

Create a binary RPM package or Windows Installer.

=item help

Print a help message.

=back

The behaviour is driven by the data supplied to the function C<setup>.
So, below each step is described with the list of key/values handled.

An API allows to write customized step :
C<register_step>, C<register_step_before>, C<register_step_after>, C<run_step>.

Customized step could reuse Configuration Helpers & OS Utilities.

=head3 Invocations

Typical invocations are:

    $ parrot setup.pir
    $ parrot setup.pir test
    $ sudo parrot setup.pir install

=head2 PERL DEPENDENCIES

=over 4

=item prove (in step 'test')

core module Test-Harness

=item prove --archive (in step 'smoke')

module TAP-Harness-Archive

=item pod2html

core module Pod-Html

=item chmod (in step 'install')

core module ExtUtils::Command, see TT #1322

=back

=head2 PARROT DEPENDENCIES

=over 4

=item glob (in step 'manifest' & 'sdist')

PGE::Glob

Limitation: currently, OS.'readdir' is dummy with MSVC.

=back

=head2 SYSTEM DEPENDENCIES

=over 4

=item smoke

curl

=item sdist_gztar

Some coreutils : tar, gzip

=item sdist_bztar

bzip2

=item sdist_zip

zip

=item spec, sdist_rpm, bdist_rpm

rpmbuild

=item bdist_wininst

Inno Setup

=back

=head2 EXAMPLES

L<http://github.com/fperrad/parrot-MT19937/blob/master/setup.pir>

L<http://github.com/fperrad/markdown/blob/master/setup.pir>

L<http://github.com/fperrad/xml/blob/master/setup.pir>

L<http://github.com/fperrad/wmlscript/blob/master/setup.pir>

L<http://github.com/fperrad/lua-batteries/blob/master/setup.pir>

L<https://trac.parrot.org/languages/browser/bf/trunk/setup.pir>

L<https://trac.parrot.org/languages/browser/chitchat/trunk/setup.pir>

L<https://trac.parrot.org/languages/browser/forth/trunk/setup.pir>

L<https://trac.parrot.org/languages/browser/lolcode/trunk/setup.pir>

L<https://trac.parrot.org/languages/browser/pheme/trunk/setup.pir>

L<http://github.com/leto/kea/blob/master/setup.pir>

L<http://github.com/vadrer/tcl-bridge/blob/master/setup.pir>

L<http://github.com/tene/gil/blob/master/setup.pir>

L<http://github.com/tene/steme/blob/master/setup.pir>

L<http://github.com/TiMBuS/fun/blob/master/setup.pir>

L<http://code.google.com/p/decnum-dynpmcs/source/browse/trunk/setup.pir>

L<http://github.com/Whiteknight/parrot-linear-algebra/blob/master/setup.pir>

L<http://bitbucket.org/riffraff/shakespeare-parrot/src/tip/setup.pir>

=cut

.sub '__onload' :load :init :anon
    $P0 = new 'Hash'
    set_global '%step', $P0

    .const 'Sub' _build_dynpmc = '_build_dynpmc'
    register_step('build', _build_dynpmc)
    .const 'Sub' _build_dynops = '_build_dynops'
    register_step_after('build', _build_dynops)
    .const 'Sub' _build_pir_pge = '_build_pir_pge'
    register_step_after('build', _build_pir_pge)
    .const 'Sub' _build_pir_tge = '_build_pir_tge'
    register_step_after('build', _build_pir_tge)
    .const 'Sub' _build_pir_nqp = '_build_pir_nqp'
    register_step_after('build', _build_pir_nqp)
    .const 'Sub' _build_pir_nqp_rx = '_build_pir_nqp_rx'
    register_step_after('build', _build_pir_nqp_rx)
    .const 'Sub' _build_pbc_pir = '_build_pbc_pir'
    register_step_after('build', _build_pbc_pir)
    .const 'Sub' _build_pbc_pbc = '_build_pbc_pbc'
    register_step_after('build', _build_pbc_pbc)
    .const 'Sub' _build_exe_pbc = '_build_exe_pbc'
    register_step_after('build', _build_exe_pbc)
    .const 'Sub' _build_installable_pbc = '_build_installable_pbc'
    register_step_after('build', _build_installable_pbc)
    .const 'Sub' _build_html_pod = '_build_html_pod'
    register_step_after('build', _build_html_pod)

    .const 'Sub' _clean_dynpmc = '_clean_dynpmc'
    register_step('clean', _clean_dynpmc)
    .const 'Sub' _clean_dynops = '_clean_dynops'
    register_step_after('clean', _clean_dynops)
    .const 'Sub' _clean_pir_pge = '_clean_pir_pge'
    register_step_after('clean', _clean_pir_pge)
    .const 'Sub' _clean_pir_tge = '_clean_pir_tge'
    register_step_after('clean', _clean_pir_tge)
    .const 'Sub' _clean_pir_nqp = '_clean_pir_nqp'
    register_step_after('clean', _clean_pir_nqp)
    .const 'Sub' _clean_pir_nqp_rx = '_clean_pir_nqp_rx'
    register_step_after('clean', _clean_pir_nqp_rx)
    .const 'Sub' _clean_pbc_pir = '_clean_pbc_pir'
    register_step_after('clean', _clean_pbc_pir)
    .const 'Sub' _clean_pbc_pbc = '_clean_pbc_pbc'
    register_step_after('clean', _clean_pbc_pbc)
    .const 'Sub' _clean_exe_pbc = '_clean_exe_pbc'
    register_step_after('clean', _clean_exe_pbc)
    .const 'Sub' _clean_installable_pbc = '_clean_installable_pbc'
    register_step_after('clean', _clean_installable_pbc)
    .const 'Sub' _clean_html_pod = '_clean_html_pod'
    register_step_after('clean', _clean_html_pod)
    .const 'Sub' _clean_gztar = '_clean_gztar'
    register_step_after('clean', _clean_gztar)
    .const 'Sub' _clean_bztar = '_clean_bztar'
    register_step_after('clean', _clean_bztar)
    .const 'Sub' _clean_zip = '_clean_zip'
    register_step_after('clean', _clean_zip)
    .const 'Sub' _clean_smoke = '_clean_smoke'
    register_step_after('clean', _clean_smoke)

    .const 'Sub' _update = '_update'
    register_step('update', _update)
    .const 'Sub' _patch = '_patch'
    register_step('patch', _patch)

    .const 'Sub' _install = '_install'
    register_step('install', _install)

    .const 'Sub' _test = '_test'
    register_step('test', _test)

    .const 'Sub' _smoke = '_smoke'
    register_step('smoke', _smoke)

    .const 'Sub' _uninstall = '_uninstall'
    register_step('uninstall', _uninstall)

    .const 'Sub' _usage = '_usage'
    register_step('usage', _usage)
    register_step('help', _usage)

    .const 'Sub' _plumage = '_plumage'
    register_step('plumage', _plumage)

    .const 'Sub' _sdist = '_sdist'
    register_step('sdist', _sdist)
    .const 'Sub' _sdist_gztar = '_sdist_gztar'
    register_step('sdist_gztar', _sdist_gztar)
    .const 'Sub' _sdist_bztar = '_sdist_bztar'
    register_step('sdist_bztar', _sdist_bztar)
    .const 'Sub' _sdist_zip = '_sdist_zip'
    register_step('sdist_zip', _sdist_zip)
    .const 'Sub' _manifest = '_manifest'
    register_step('manifest', _manifest)
    .const 'Sub' _sdist_rpm = '_sdist_rpm'
    register_step('sdist_rpm', _sdist_rpm)
    .const 'Sub' _spec_rpm = '_spec_rpm'
    register_step('spec', _spec_rpm)
    .const 'Sub' _control_deb = '_control_deb'
    register_step('control', _control_deb)
    .const 'Sub' _ebuild_gentoo = '_ebuild_gentoo'
    register_step('ebuild', _ebuild_gentoo)

    .const 'Sub' _bdist = '_bdist'
    register_step('bdist', _bdist)
    .const 'Sub' _bdist_rpm = '_bdist_rpm'
    register_step('bdist_rpm', _bdist_rpm)

    $P0 = get_config()
    $S0 = $P0['osname']
    unless $S0 == 'MSWin32' goto L1
    .const 'Sub' _bdist_wininst = '_bdist_wininst'
    register_step('bdist_wininst', _bdist_wininst)
    .const 'Sub' _clean_wininst = '_clean_wininst'
    register_step_after('clean', _clean_wininst)
  L1:
.end

=head2 Functions

=over 4

=item setup

Entry point.

=cut

.sub 'setup'
    .param pmc args :slurpy
    .param pmc kv :slurpy :named
    .local pmc steps
    steps = new 'ResizableStringArray'
    $P0 = iter args
  L1:
    unless $P0 goto L2
    $S0 = shift $P0
    $S1 = substr $S0, 0, 2
    unless $S1 == '--' goto L3
    $S1 = substr $S0, 2
    $S2 = shift $P0
    print $S1
    print "="
    say $S2
    kv[$S1] = $S2
    goto L1
  L3:
    push steps, $S0
    goto L1
  L2:

    push_eh _handler
    $P0 = iter steps
    if $P0 goto L11
    # default step
    run_step('build', 'build' :named('__target__'), kv :flat :named)
    goto L12
  L11:
    unless $P0 goto L12
    $S0 = shift $P0
    $I0 = run_step($S0, $S0 :named('__target__'), kv :flat :named)
    if $I0 goto L11
    print "unknown target : "
    say $S0
    run_step('usage')
  L12:
    pop_eh
    end
  _handler:
    .local pmc ex
    .get_results (ex)
    rethrow ex
.end

=item run_step

Call a step by its name.

=cut

.sub 'run_step'
    .param string name
    .param pmc kv :slurpy :named
    $P0 = get_global '%step'
    $I0 = exists $P0[name]
    if $I0 goto L1
    .return (0)
  L1:
    $P1 = $P0[name]
    $P2 = iter $P1
  L2:
    unless $P2 goto L3
    $P3 = shift $P2
    $P3(kv :flat :named)
    goto L2
  L3:
    .return (1)
.end

=item register_step

Register a step by its name

=cut

.sub 'register_step'
    .param string name
    .param pmc step
    $P0 = get_global '%step'
    $P1 = new 'ResizablePMCArray'
    unshift $P1, step
    $P0[name] = $P1
.end

=item register_step_after

Append a step

=cut

.sub 'register_step_after'
    .param string name
    .param pmc step
    $P0 = get_global '%step'
    $P1 = $P0[name]
    push $P1, step
.end

=item register_step_before

Insert a step

=cut

.sub 'register_step_before'
    .param string name
    .param pmc step
    $P0 = get_global '%step'
    $P1 = $P0[name]
    unshift $P1, step
.end

=back

=head2 Steps

=head3 Step usage

Display a helpful message

=over 4

=item usage

Overload the default message

=back

=cut

.sub '_usage' :anon
    .param pmc kv :slurpy :named
    .local string msg
    msg = <<'USAGE'
usage: parrot setup.pir [target|--key value]*

    Default targets are :

        build:          Build the library.

        test:           Run the test suite.

        install:        Install the library.

        uninstall:      Uninstall the library.

        clean:          Basic cleaning up.

        update:         Update from the repository.

        plumage:        Output a skeleton for Plumage

        sdist:          Create a source distribution

        bdist:          Create a binary distribution

        help:           Print this help message.
USAGE
    $S0 = get_value('usage', msg :named('default'), kv :flat :named)
    say $S0
.end

=head3 Step build

=over 4

=item pbc_pir

hash

the key is the PBC pathname

the value is an array of pathname or a single pathname

the first item of the array is the PIR source

the others items of the array are just the dependencies

=cut

.sub '_build_pbc_pir' :anon
    .param pmc kv :slurpy :named
    $I0 = exists kv['pbc_pir']
    unless $I0 goto L1
    $P0 = kv['pbc_pir']
    build_pbc_pir($P0)
  L1:
.end

.sub 'build_pbc_pir'
    .param pmc hash
    $P0 = iter hash
  L1:
    unless $P0 goto L2
    .local string pbc, src
    pbc = shift $P0
    .local pmc depends
    depends = hash[pbc]
    $I0 = does depends, 'array'
    unless $I0 goto L3
    $I0 = newer(pbc, depends)
    if $I0 goto L1
    src = shift depends
    goto L4
  L3:
    src = depends
    $I0 = newer(pbc, src)
    if $I0 goto L1
  L4:
    .local string cmd
    cmd = get_parrot()
    cmd .= " -o "
    cmd .= pbc
    cmd .= " "
    cmd .= src
    system(cmd, 1 :named('verbose'))
    goto L1
  L2:
.end

=item pir_pge

hash

the key is the PIR pathname

the value is an array of PGE pathname or a single PGE pathname

=item pir_pge_flags

=cut

.sub '_build_pir_pge' :anon
    .param pmc kv :slurpy :named
    $I0 = exists kv['pir_pge']
    unless $I0 goto L1
    $P0 = kv['pir_pge']
    $S0 = get_value('pir_pge_flags', '' :named('default'), kv :flat :named)
    build_pir_pge($P0, $S0)
  L1:
.end

.sub 'build_pir_pge'
    .param pmc hash
    .param string flags
    $P0 = iter hash
  L1:
    unless $P0 goto L2
    .local string pir, src
    pir = shift $P0
    .local pmc srcs
    srcs = hash[pir]
    $I0 = does srcs, 'array'
    unless $I0 goto L3
    $I0 = newer(pir, srcs)
    if $I0 goto L1
    src = join ' ', srcs
    goto L4
  L3:
    src = srcs
    $I0 = newer(pir, src)
    if $I0 goto L1
  L4:
    .local string cmd
    cmd = get_parrot()
    cmd .= " "
    $S0 = get_library('PGE/Perl6Grammar.pbc')
    cmd .= $S0
    cmd .= " --output="
    cmd .= pir
    cmd .= " "
    cmd .= flags
    cmd .= " "
    cmd .= src
    system(cmd, 1 :named('verbose'))
    goto L1
  L2:
.end

=item pir_tge

hash

the key is the PIR pathname

the value is the TGE pathname

=cut

.sub '_build_pir_tge' :anon
    .param pmc kv :slurpy :named
    $I0 = exists kv['pir_tge']
    unless $I0 goto L1
    $P0 = kv['pir_tge']
    build_pir_tge($P0)
  L1:
.end

.sub 'build_pir_tge'
    .param pmc hash
    $P0 = iter hash
  L1:
    unless $P0 goto L2
    .local string pir, tge
    pir = shift $P0
    tge = hash[pir]
    $I0 = newer(pir, tge)
    if $I0 goto L1
    .local string cmd
    cmd = get_parrot()
    cmd .= " "
    $S0 = get_compiler('tge/tgc.pir')
    cmd .= $S0
    cmd .= " --output="
    cmd .= pir
    cmd .= " "
    cmd .= tge
    system(cmd, 1 :named('verbose'))
    goto L1
  L2:
.end

=item pir_nqp

hash

the key is the PIR pathname

the value is the NQP pathname

=item pir_nqp_flags

=cut

.sub '_build_pir_nqp' :anon
    .param pmc kv :slurpy :named
    $I0 = exists kv['pir_nqp']
    unless $I0 goto L1
    $P0 = kv['pir_nqp']
    $S0 = get_value('pir_nqp_flags', '' :named('default'), kv :flat :named)
    build_pir_nqp($P0, $S0)
  L1:
.end

.sub 'build_pir_nqp'
    .param pmc hash
    .param string flags
    $P0 = iter hash
  L1:
    unless $P0 goto L2
    .local string pir, nqp
    pir = shift $P0
    nqp = hash[pir]
    $I0 = newer(pir, nqp)
    if $I0 goto L1
    .local string cmd
    cmd = get_parrot()
    cmd .= " "
    $S0 = get_compiler('nqp/nqp.pbc')
    cmd .= $S0
    cmd .= " --target=pir --output="
    cmd .= pir
    cmd .= " "
    cmd .= flags
    cmd .= " "
    cmd .= nqp
    system(cmd, 1 :named('verbose'))
    goto L1
  L2:
.end

=item pir_nqp-rx

hash

the key is the PIR pathname

the value is the NQP pathname

=cut

.sub '_build_pir_nqp_rx' :anon
    .param pmc kv :slurpy :named
    $I0 = exists kv['pir_nqp-rx']
    unless $I0 goto L1
    $P0 = kv['pir_nqp-rx']
    build_pir_nqp_rx($P0)
  L1:
.end

.sub 'build_pir_nqp_rx'
    .param pmc hash
    $P0 = iter hash
  L1:
    unless $P0 goto L2
    .local string pir, nqp
    pir = shift $P0
    nqp = hash[pir]
    $I0 = newer(pir, nqp)
    if $I0 goto L1
    .local string cmd
    cmd = get_nqp()
    cmd .= " --target=pir --output="
    cmd .= pir
    cmd .= " "
    cmd .= nqp
    system(cmd, 1 :named('verbose'))
    goto L1
  L2:
.end

=item pbc_pbc

hash

the key is the PBC pathname

the value is an array of PBC pathname

=cut

.sub '_build_pbc_pbc' :anon
    .param pmc kv :slurpy :named
    $I0 = exists kv['pbc_pbc']
    unless $I0 goto L1
    $P0 = kv['pbc_pbc']
    build_pbc_pbc($P0)
  L1:
.end

.sub 'build_pbc_pbc'
    .param pmc hash
    $P0 = iter hash
  L1:
    unless $P0 goto L2
    .local string pbc, src
    pbc = shift $P0
    .local pmc srcs
    srcs = hash[pbc]
    $I0 = newer(pbc, srcs)
    if $I0 goto L1
    src = join ' ', srcs
    .local string cmd
    cmd = get_executable('pbc_merge')
    cmd .= " -o "
    cmd .= pbc
    cmd .= " "
    cmd .= src
    system(cmd, 1 :named('verbose'))
    goto L1
  L2:
.end

=item exe_pbc

hash

the key is the executable pathname

the value is the PBC pathname

=cut

.sub '_build_exe_pbc' :anon
    .param pmc kv :slurpy :named
    $I0 = exists kv['exe_pbc']
    unless $I0 goto L1
    $P0 = kv['exe_pbc']
    build_exe_pbc($P0)
  L1:
.end

.sub 'build_exe_pbc'
    .param pmc hash
    .local string exe
    exe = get_exe()
    $P0 = iter hash
  L1:
    unless $P0 goto L2
    .local string bin, pbc
    bin = shift $P0
    pbc = hash[bin]
    $I0 = length pbc
    $I0 -= 4
    $S0 = substr pbc, 0, $I0
    $S1 = $S0 . exe
    $I0 = newer($S1, pbc)
    if $I0 goto L1
    .local string cmd
    cmd = get_executable('pbc_to_exe')
    cmd .= " "
    cmd .= pbc
    system(cmd, 1 :named('verbose'))
    goto L1
  L2:
.end

=item installable_pbc

hash

the key is the executable pathname

the value is the PBC pathname

=cut

.sub '_build_installable_pbc' :anon
    .param pmc kv :slurpy :named
    $I0 = exists kv['installable_pbc']
    unless $I0 goto L1
    $P0 = kv['installable_pbc']
    build_installable_pbc($P0)
  L1:
.end

.sub 'build_installable_pbc'
    .param pmc hash
    .local string exe
    exe = get_exe()
    .local int has_strip
    has_strip = _has_strip()
    $P0 = iter hash
  L1:
    unless $P0 goto L2
    .local string bin, pbc
    bin = shift $P0
    pbc = hash[bin]
    $I0 = length pbc
    $I0 -= 4
    $S0 = substr pbc, 0, $I0
    $S1 = "installable_" . $S0
    $S1 .= exe
    $I0 = newer($S1, pbc)
    if $I0 goto L1
    .local string cmd
    cmd = get_executable('pbc_to_exe')
    cmd .= " "
    cmd .= pbc
    cmd .= " --install"
    system(cmd, 1 :named('verbose'))
    unless has_strip goto L1
    cmd = "strip " . $S1
    system(cmd, 1 :named('verbose'))
    goto L1
  L2:
.end

.sub '_has_strip' :anon
    .param string cflags        :optional
    .param int has_cflags       :opt_flag
    .local pmc config
    config = get_config()
    $S0 = config['gccversion']
    unless $S0 goto L1
    unless has_cflags goto L2
    $I0 = index cflags, '-g'
    unless $I0 < 0 goto L1
  L2:
    $S0 = config['cflags']
    $I0 = index $S0, '-g'
    unless $I0 < 0 goto L1
    .return (1)
  L1:
    .return (0)
.end

=item dynops

hash

the key is the name

the value is the OPS pathname

=item dynops_cflags

=item dynops_ldflags

=cut

.sub '_build_dynops' :anon
    .param pmc kv :slurpy :named
    $I0 = exists kv['dynops']
    unless $I0 goto L1
    .local string cflags, ldflags
    cflags = get_value('dynops_cflags', '' :named('default'), kv :flat :named)
    ldflags = get_value('dynops_ldflags', '' :named('default'), kv :flat :named)
    $P0 = kv['dynops']
    build_dynops($P0, cflags, ldflags)
  L1:
.end

.sub 'build_dynops'
    .param pmc hash
    .param string cflags
    .param string ldflags
    mkpath('dynext', 1 :named('verbose'))
    .local pmc cores
    cores = get_cores()
    .local string load_ext
    load_ext = get_load_ext()
    $P0 = iter hash
  L1:
    unless $P0 goto L2
    .local string ops, src
    ops = shift $P0
    src = hash[ops]
    $P1 = iter cores
  L3:
    unless $P1 goto L4
    .local string core, suffix
    core = shift $P1
    suffix = cores[core]
    $S0 = _mk_path_dynops(ops, suffix, load_ext)
    $I0 = newer($S0, src)
    if $I0 goto L3
    __build_dynops(src, ops, core, suffix, cflags, ldflags)
    goto L3
  L4:
    goto L1
  L2:
.end

.sub '__build_dynops' :anon
    .param string src
    .param string ops
    .param string core
    .param string suffix
    .param string cflags
    .param string ldflags
    .local pmc config
    config = get_config()
    .local string cmd
    cmd = config['perl']
    cmd .= " "
    $S0 = get_tool('build/ops2c.pl')
    cmd .= $S0
    cmd .= " "
    cmd .= core
    cmd .= " --dynamic "
    cmd .= src
    system(cmd, 1 :named('verbose'))

    $S0 = config['o']
    $S1 = _mk_path_gen_dynops(src, ops, suffix, $S0)
    $S2 = _mk_path_gen_dynops(src, ops, suffix, '.c')
    __compile_cc($S1, $S2, cflags)

    .local string dynext
    $S0 = config['load_ext']
    dynext = _mk_path_dynops(ops, suffix, $S0)
    cmd = config['ld']
    cmd .= " "
    $S0 = config['ld_out']
    cmd .= $S0
    cmd .= dynext
    cmd .= " "
    $S0 = config['o']
    $S0 = _mk_path_gen_dynops(src, ops, suffix, $S0)
    cmd .= $S0
    cmd .= " "
    $S0 = get_ldflags()
    cmd .= $S0
    cmd .= " "
    $S0 = config['ld_load_flags']
    cmd .= $S0
    cmd .= " "
    $I0 = config['parrot_is_shared']
    unless $I0 goto L1
    $S0 = config['inst_libparrot_ldflags']
    cmd .= $S0
    cmd .= " "
  L1:
    cmd .= ldflags
    system(cmd, 1 :named('verbose'))

    $I0 = _has_strip(cflags)
    unless $I0 goto L2
    cmd = "strip " . dynext
    system(cmd, 1 :named('verbose'))
  L2:
.end

.sub '__compile_cc'
    .param string obj
    .param string src
    .param string cflags
    .local pmc config
    config = get_config()
    .local string cmd
    cmd = config['cc']
    cmd .= " -c "
    $S0 = config['cc_o_out']
    cmd .= $S0
    cmd .= " "
    cmd .= obj
    cmd .= " -I"
    $S0 = get_incdir()
    cmd .= $S0
    cmd .= " -I"
    cmd .= $S0
    cmd .= "/pmc -I"
    $S0 = cwd()
    cmd .= $S0
    cmd .= " "
    $S0 = get_cflags()
    cmd .= $S0
    cmd .= " "
    cmd .= cflags
    cmd .= " "
    cmd .= src
    system(cmd, 1 :named('verbose'))
.end

.sub '_mk_path_dynops' :anon
    .param string ops
    .param string suffix
    .param string load_ext
    $S0 = "dynext/" . ops
    $S0 .= suffix
    $S0 .= load_ext
    .return ($S0)
.end

.sub '_mk_path_gen_dynops' :anon
    .param string src
    .param string ops
    .param string suffix
    .param string ext
    $S0 = dirname(src)
    $S0 .= "/"
    $S0 .= ops
    $S0 .= suffix
    $S0 .= ext
    .return ($S0)
.end

.sub 'get_cores'
    $P0 = new 'Hash'
    $P0['C'] = ''
#    $P0['CGP'] = '_cgp'
#    $P0['CGoto'] = '_cg'
    $P0['CSwitch'] = '_switch'
    .return ($P0)
.end

=item dynpmc

hash

the key is the group name

the value is an array of PMC pathname

=item dynpmc_cflags

=item dynpmc_ldflags

=cut

.sub '_build_dynpmc' :anon
    .param pmc kv :slurpy :named
    $I0 = exists kv['dynpmc']
    unless $I0 goto L1
    .local string cflags, ldflags
    cflags = get_value('dynpmc_cflags', '' :named('default'), kv :flat :named)
    ldflags = get_value('dynpmc_ldflags', '' :named('default'), kv :flat :named)
    $P0 = kv['dynpmc']
    build_dynpmc($P0, cflags, ldflags)
  L1:
.end

.sub 'build_dynpmc'
    .param pmc hash
    .param string cflags
    .param string ldflags
    mkpath('dynext', 1 :named('verbose'))
    .local string load_ext, obj
    load_ext = get_load_ext()
    obj = get_obj()
    $P0 = iter hash
  L1:
    unless $P0 goto L2
    .local string group
    group = shift $P0
    .local pmc srcs
    srcs = hash[group]
    $P1 = iter srcs
  L3:
    unless $P1 goto L4
    .local string src
    src = shift $P1
    $S0 = _mk_path_gen_dynpmc(src, obj)
    $I0 = newer($S0, src)
    if $I0 goto L3
    __build_dynpmc(src, cflags)
    goto L3
  L4:
    if group == '' goto L1
    $S0 = _mk_path_dynpmc(group, load_ext)
    $I0 = newer($S0, srcs)
    if $I0 goto L1
    __build_dynpmc_group(srcs, group, cflags, ldflags)
    goto L1
  L2:
.end

.sub '__build_dynpmc' :anon
    .param string src
    .param string cflags
    .local pmc config
    config = get_config()
    .local string pmc2c
    pmc2c = config['perl']
    pmc2c .= " "
    $S0 = get_tool('build/pmc2c.pl')
    pmc2c .= $S0
    .local string pmc2c_includes
    pmc2c_includes = "--include "
    $S0 = get_srcdir()
    pmc2c_includes .= $S0
    pmc2c_includes .= " --include "
    pmc2c_includes .= $S0
    pmc2c_includes .= "/pmc"
    .local string current_dir
    current_dir = cwd()
    $S0 = dirname(src)
    chdir($S0)

    .local string cmd
    cmd = clone pmc2c
    cmd .= " --dump "
    cmd .= pmc2c_includes
    cmd .= " "
#    cmd .= src
    $S0 = basename(src)
    cmd .= $S0
    system(cmd, 1 :named('verbose'))

    cmd = clone pmc2c
    cmd .= " --c "
    cmd .= pmc2c_includes
    cmd .= " "
#    cmd .= src
    $S0 = basename(src)
    cmd .= $S0
    system(cmd, 1 :named('verbose'))

    chdir(current_dir)

    $S0 = config['o']
    $S1 = _mk_path_gen_dynpmc(src, $S0)
    $S2 = _mk_path_gen_dynpmc(src, '.c')
    __compile_cc($S1, $S2, cflags)
.end

.sub '__build_dynpmc_group' :anon
    .param pmc srcs
    .param string group
    .param string cflags
    .param string ldflags
    .local pmc config
    config = get_config()
    .local string src, obj
    src = srcs[0]
    obj = config['o']
    .local string current_dir
    current_dir = cwd()
    $S0 = dirname(src)
    chdir($S0)

    .local string cmd
    cmd = config['perl']
    cmd .= " "
    $S0 = get_tool('build/pmc2c.pl')
    cmd .= $S0
    cmd .= " --library "
#    $S0 = dirname(src)
#    cmd .= $S0
#    cmd .= "/"
    cmd .= group
    cmd .= " --c "
#    $S0 = join " ", srcs
#    cmd .= $S0
    $P0 = iter srcs
  L1:
    unless $P0 goto L2
    src = shift $P0
    $S0 = basename(src)
    cmd .= $S0
    cmd .= " "
    goto L1
  L2:
    system(cmd, 1 :named('verbose'))

    chdir(current_dir)

    $S1 = _mk_path_gen_dynpmc_group(src, group, obj)
    $S2 = _mk_path_gen_dynpmc_group(src, group, '.c')
    __compile_cc($S1, $S2, cflags)

    .local string dynext
    $S0 = config['load_ext']
    dynext = _mk_path_dynpmc(group, $S0)
    cmd = config['ld']
    cmd .= " "
    $S0 = config['ld_out']
    cmd .= $S0
    cmd .= dynext
    cmd .= " "
    $S0 = _mk_path_gen_dynpmc_group(src, group, obj)
    cmd .= $S0
    cmd .= " "
    $P0 = iter srcs
  L3:
    unless $P0 goto L4
    src = shift $P0
    $S0 = _mk_path_gen_dynpmc(src, obj)
    cmd .= $S0
    cmd .= " "
    goto L3
  L4:
    $S0 = get_ldflags()
    cmd .= $S0
    cmd .= " "
    $S0 = config['ld_load_flags']
    cmd .= $S0
    cmd .= " "
    $I0 = config['parrot_is_shared']
    unless $I0 goto L5
    $S0 = config['inst_libparrot_ldflags']
    cmd .= $S0
    cmd .= " "
  L5:
    cmd .= ldflags
    system(cmd, 1 :named('verbose'))

    $I0 = _has_strip(cflags)
    unless $I0 goto L6
    cmd = "strip " . dynext
    system(cmd, 1 :named('verbose'))
  L6:
.end

.sub '_mk_path_dynpmc' :anon
    .param string group
    .param string load_ext
    $S0 = "dynext/" . group
    $S0 .= load_ext
    .return ($S0)
.end

.sub '_mk_path_gen_dynpmc' :anon
    .param string src
    .param string ext
    $I0 = length src
    $I0 -= 4
    $S0 = substr src, 0, $I0
    $S0 .= ext
    unless ext == '.h' goto L1
    $S1 = dirname($S0)
    $S2 = basename($S0)
    $S0 = $S1 . "/pmc_"
    $S0 .= $S2
  L1:
    .return ($S0)
.end

.sub '_mk_path_gen_dynpmc_group' :anon
    .param string src
    .param string group
    .param string ext
    $S0 = dirname(src)
    $S0 .= "/"
    $S0 .= group
    $S0 .= ext
    .return ($S0)
.end

=item html_pod

hash

the key is the HTML pathname

the value is the POD pathname

=cut

.sub '_build_html_pod' :anon
    .param pmc kv :slurpy :named
    $I0 = exists kv['html_pod']
    unless $I0 goto L1
    $P0 = kv['html_pod']
    build_html_pod($P0)
  L1:
.end

.sub 'build_html_pod'
    .param pmc hash
    $P0 = iter hash
  L1:
    unless $P0 goto L2
    .local string html, pod
    html = shift $P0
    pod = hash[html]
    $I0 = newer(html, pod)
    if $I0 goto L1
    .local string cmd
    cmd = "pod2html --infile "
    cmd .= pod
    cmd .= " --outfile "
    cmd .= html
    system(cmd, 1 :named('verbose'))
    unlink("pod2htmd.tmp")
    unlink("pod2htmi.tmp")
    goto L1
  L2:
.end

=back

=head3 Step clean

=over 4

=item pbc_pir

=cut

.sub '_clean_pbc_pir' :anon
    .param pmc kv :slurpy :named
    $I0 = exists kv['pbc_pir']
    unless $I0 goto L1
    $P0 = kv['pbc_pir']
    clean_key($P0)
  L1:
.end

.sub 'clean_key'
    .param pmc hash
    $P0 = iter hash
  L1:
     unless $P0 goto L2
     $S0 = shift $P0
     unlink($S0, 1 :named('verbose'))
     goto L1
  L2:
.end

=item pir_pge

=cut

.sub '_clean_pir_pge' :anon
    .param pmc kv :slurpy :named
    $I0 = exists kv['pir_pge']
    unless $I0 goto L1
    $P0 = kv['pir_pge']
    clean_key($P0)
  L1:
.end

=item pir_tge

=cut

.sub '_clean_pir_tge' :anon
    .param pmc kv :slurpy :named
    $I0 = exists kv['pir_tge']
    unless $I0 goto L1
    $P0 = kv['pir_tge']
    clean_key($P0)
  L1:
.end

=item pir_nqp

=cut

.sub '_clean_pir_nqp' :anon
    .param pmc kv :slurpy :named
    $I0 = exists kv['pir_nqp']
    unless $I0 goto L1
    $P0 = kv['pir_nqp']
    clean_key($P0)
  L1:
.end

=item pir_nqp-rx

=cut

.sub '_clean_pir_nqp_rx' :anon
    .param pmc kv :slurpy :named
    $I0 = exists kv['pir_nqp-rx']
    unless $I0 goto L1
    $P0 = kv['pir_nqp-rx']
    clean_key($P0)
  L1:
.end

=item pbc_pbc

=cut

.sub '_clean_pbc_pbc' :anon
    .param pmc kv :slurpy :named
    $I0 = exists kv['pbc_pbc']
    unless $I0 goto L1
    $P0 = kv['pbc_pbc']
    clean_key($P0)
  L1:
.end

=item exe_pbc

=cut

.sub '_clean_exe_pbc' :anon
    .param pmc kv :slurpy :named
    $I0 = exists kv['exe_pbc']
    unless $I0 goto L1
    $P0 = kv['exe_pbc']
    clean_exe_pbc($P0)
  L1:
.end

.sub 'clean_exe_pbc'
    .param pmc hash
    .local string bin, exe, obj, pbc
    exe = get_exe()
    obj = get_obj()
    $P0 = iter hash
  L1:
    unless $P0 goto L2
    bin = shift $P0
    pbc = hash[bin]
    $I0 = length pbc
    $I0 -= 4
    $S0 = substr pbc, 0, $I0
    $S1 = $S0 . exe
    unlink($S1, 1 :named('verbose'))
    $S1 = $S0 . '.c'
    unlink($S1, 1 :named('verbose'))
    $S1 = $S0 . obj
    unlink($S1, 1 :named('verbose'))
    goto L1
  L2:
.end

=item installable_pbc

=cut

.sub '_clean_installable_pbc' :anon
    .param pmc kv :slurpy :named
    $I0 = exists kv['installable_pbc']
    unless $I0 goto L1
    $P0 = kv['installable_pbc']
    clean_installable_pbc($P0)
  L1:
.end

.sub 'clean_installable_pbc'
    .param pmc hash
    .local string bin, exe, obj, pbc
    exe = get_exe()
    obj = get_obj()
    $P0 = iter hash
  L1:
    unless $P0 goto L2
    bin = shift $P0
    pbc = hash[bin]
    $I0 = length pbc
    $I0 -= 4
    $S0 = substr pbc, 0, $I0
    $S1 = 'installable_' . $S0
    $S1 .= exe
    unlink($S1, 1 :named('verbose'))
    $S1 = $S0 . '.c'
    unlink($S1, 1 :named('verbose'))
    $S1 = $S0 . obj
    unlink($S1, 1 :named('verbose'))
    goto L1
  L2:
.end

=item dynops

=cut

.sub '_clean_dynops' :anon
    .param pmc kv :slurpy :named
    $I0 = exists kv['dynops']
    unless $I0 goto L1
    $P0 = kv['dynops']
    clean_dynops($P0)
  L1:
.end

.sub 'clean_dynops'
    .param pmc hash
    .local pmc cores
    cores = get_cores()
    .local string load_ext, obj
    load_ext = get_load_ext()
    obj = get_obj()
    $P0 = iter hash
  L1:
    unless $P0 goto L2
    .local string ops, src
    ops = shift $P0
    src = hash[ops]
    $P1 = iter cores
  L3:
    unless $P1 goto L4
    .local string core, suffix
    core = shift $P1
    suffix = cores[core]
    $S0 = _mk_path_dynops(ops, suffix, load_ext)
    unlink($S0, 1 :named('verbose'))
    $S0 = _mk_path_gen_dynops(src, ops, suffix, '.c')
    unlink($S0, 1 :named('verbose'))
    $S0 = _mk_path_gen_dynops(src, ops, suffix, '.h')
    unlink($S0, 1 :named('verbose'))
    $S0 = _mk_path_gen_dynops(src, ops, suffix, obj)
    unlink($S0, 1 :named('verbose'))
    goto L3
  L4:
    goto L1
  L2:
.end

=item dynpmc

=cut

.sub '_clean_dynpmc' :anon
    .param pmc kv :slurpy :named
    $I0 = exists kv['dynpmc']
    unless $I0 goto L1
    $P0 = kv['dynpmc']
    clean_dynpmc($P0)
  L1:
.end

.sub 'clean_dynpmc'
    .param pmc hash
    .local string load_ext, obj
    load_ext = get_load_ext()
    obj = get_obj()
    $P0 = iter hash
  L1:
    unless $P0 goto L2
    .local string group
    group = shift $P0
    .local pmc srcs
    srcs = hash[group]
    $S0 = _mk_path_dynpmc(group, load_ext)
    unlink($S0, 1 :named('verbose'))
    $P1 = iter srcs
  L3:
    unless $P1 goto L4
    .local string src
    src = shift $P1
    $S0 = _mk_path_gen_dynpmc(src, '.c')
    unlink($S0, 1 :named('verbose'))
    $S0 = _mk_path_gen_dynpmc(src, '.h')
    unlink($S0, 1 :named('verbose'))
    $S0 = _mk_path_gen_dynpmc(src, '.dump')
    unlink($S0, 1 :named('verbose'))
    $S0 = _mk_path_gen_dynpmc(src, obj)
    unlink($S0, 1 :named('verbose'))
    goto L3
  L4:
    src = srcs[0]
    $S0 = _mk_path_gen_dynpmc_group(src, group, '.c')
    unlink($S0, 1 :named('verbose'))
    $S0 = _mk_path_gen_dynpmc_group(src, group, '.h')
    unlink($S0, 1 :named('verbose'))
    $S0 = _mk_path_gen_dynpmc_group(src, group, obj)
    unlink($S0, 1 :named('verbose'))
    goto L1
  L2:
.end

=item html_pod

=cut

.sub '_clean_html_pod' :anon
    .param pmc kv :slurpy :named
    $I0 = exists kv['html_pod']
    unless $I0 goto L1
    $P0 = kv['html_pod']
    clean_key($P0)
  L1:
.end

=back

=head3 Step update

The following Version Control System are handled :

=over 4

=cut

.sub '_update' :anon
    .param pmc kv :slurpy :named
    $S0 = get_vcs()
    unless $S0 == 'cvs' goto L1
    .tailcall _update_cvs(kv :flat :named)
  L1:
    unless $S0 == 'git' goto L2
    .tailcall _update_git(kv :flat :named)
  L2:
    unless $S0 == 'hg' goto L3
    .tailcall _update_hg(kv :flat :named)
  L3:
    unless $S0 == 'svn' goto L4
    .tailcall _update_svn(kv :flat :named)
  L4:
    die "Don't known how to update."
.end

=item CVS

=cut

.sub '_update_cvs' :anon
    .param pmc kv :slurpy :named
    system('cvs update', 1 :named('verbose'))
.end

=item Git

=cut

.sub '_update_git' :anon
    .param pmc kv :slurpy :named
    system('git pull', 1 :named('verbose'))
.end

=item Mercurial

=cut

.sub '_update_hg' :anon
    .param pmc kv :slurpy :named
    system('hg pull --update', 1 :named('verbose'))
.end

=item SVN

=cut

.sub '_update_svn' :anon
    .param pmc kv :slurpy :named
    system('svn update', 1 :named('verbose'))
.end

=back

=head3 Step patch

The following Version Control System are handled :

=over 4

=cut

.sub '_patch' :anon
    .param pmc kv :slurpy :named
    $S0 = get_vcs()
    unless $S0 == 'cvs' goto L1
    .tailcall _patch_cvs(kv :flat :named)
  L1:
    unless $S0 == 'git' goto L2
    .tailcall _patch_git(kv :flat :named)
  L2:
    unless $S0 == 'hg' goto L3
    .tailcall _patch_hg(kv :flat :named)
  L3:
    unless $S0 == 'svn' goto L4
    .tailcall _patch_svn(kv :flat :named)
  L4:
    die "Don't known how to create a patch."
.end

=item CVS

=cut

.sub '_patch_cvs' :anon
    .param pmc kv :slurpy :named
    system('cvs diff', 1 :named('verbose'))
.end

=item Git

=cut

.sub '_patch_git' :anon
    .param pmc kv :slurpy :named
    system('git diff', 1 :named('verbose'))
.end

=item Mercurial

=cut

.sub '_patch_hg' :anon
    .param pmc kv :slurpy :named
    system('hg diff', 1 :named('verbose'))
.end

=item SVN

=cut

.sub '_patch_svn' :anon
    .param pmc kv :slurpy :named
    system('svn diff', 1 :named('verbose'))
.end

=back

=head3 Step test

If t/harness exists, run : t/harness

Else run : prove t/*.t

=cut

.sub '_test' :anon
    .param pmc kv :slurpy :named
    run_step('build', kv :flat :named)
    $I0 = file_exists('t/harness')
    unless $I0 goto L1
    .tailcall _test_harness(kv :flat :named)
  L1:
    .tailcall _test_prove(kv :flat :named)
.end

=over 4

=item harness_exec

the default value is with perl

=item harness_files

the default value is "t/*.t"

=cut

.sub '_test_harness' :anon
    .param pmc kv :slurpy :named
    .local string cmd
    $I0 = exists kv['harness_exec']
    unless $I0 goto L1
    cmd = kv['harness_exec']
    goto L2
  L1:
    cmd = "perl -I"
    $S0 = get_libdir()
    cmd .= $S0
    cmd .= "/tools/lib"
  L2:
    cmd .= " t/harness "
    $S0 = get_value('harness_files', "t/*.t" :named('default'), kv :flat :named)
    cmd .= $S0
    system(cmd, 1 :named('verbose'))
.end

=item prove_exec

option --exec of prove

=item prove_files

the default value is "t/*.t"

=back

=cut

.sub '_test_prove' :anon
    .param pmc kv :slurpy :named
    .local string cmd
    cmd = "prove"
    $S0 = get_executable('parrot-tapir')
    $I0 = file_exists($S0)
    unless $I0 goto L0
    cmd = $S0
  L0:
    $I0 = exists kv['prove_exec']
    unless $I0 goto L1
    cmd .= " --exec="
    $S0 = kv['prove_exec']
    $I0 = index $S0, ' '
    if $I0 < 0 goto L2
    cmd .= "\""
  L2:
    cmd .= $S0
    if $I0 < 0 goto L1
    cmd .= "\""
  L1:
    cmd .= " "
    $S0 = get_value('prove_files', "t/*.t" :named('default'), kv :flat :named)
    cmd .= $S0
    system(cmd, 1 :named('verbose'))
.end

=head3 Step smoke

Unless t/harness exists, run : prove --archive t/*.t

=cut

.sub '_smoke' :anon
    .param pmc kv :slurpy :named
    run_step('build', kv :flat :named)
    $I0 = file_exists('t/harness')
    if $I0 goto L1
    .tailcall _smoke_prove(kv :flat :named)
  L1:
    die "Don't known how to smoke with t/harness."
.end

.sub '_clean_smoke' :anon
    .param pmc kv :slurpy :named
    $S0 = get_value('prove_archive', "report.tar.gz" :named('default'), kv :flat :named)
    unlink($S0, 1 :named('verbose'))
    unlink('meta.yml', 1 :named('verbose'))
.end

=over 4

=item prove_exec

option --exec of prove

=item prove_files

the default value is "t/*.t"

=item prove_archive

option --archive of prove

the default value is report.tar.gz

=item smolder_url

a string

=item smolder_tags

a string

=item smolder_comments

a string

=item smolder_extra_properties

a hash

=back

=cut

.sub '_smoke_prove' :anon
    .param pmc kv :slurpy :named
    .local string cmd
    cmd = "prove"
    $I0 = exists kv['prove_exec']
    unless $I0 goto L1
    cmd .= " --exec="
    $S0 = kv['prove_exec']
    $I0 = index $S0, ' '
    if $I0 < 0 goto L2
    cmd .= "\""
  L2:
    cmd .= $S0
    if $I0 < 0 goto L1
    cmd .= "\""
  L1:
    cmd .= " "
    $S0 = get_value('prove_files', "t/*.t" :named('default'), kv :flat :named)
    cmd .= $S0
    cmd .= " --archive="
    .local string archive
    archive = get_value('prove_archive', "report.tar.gz" :named('default'), kv :flat :named)
    cmd .= archive
    system(cmd, 1 :named('verbose'), 1 :named('ignore_error'))

    $I0 = exists kv['smolder_extra_properties']
    unless $I0 goto L4
    system('perl -MExtUtils::Command -e rm_rf tmp')
    cmd = "mkdir tmp && cd tmp && tar xzf ../"
    cmd .= archive
    system(cmd, 1 :named('verbose'))

    $P0 = kv['smolder_extra_properties']
    $S0 = mk_extra_properties($P0)
    say "append extra properties"
    append('tmp/meta.yml', $S0)

    unlink(archive)
    cmd = "cd tmp && tar czf ../"
    cmd .= archive
    cmd .= " *"
    system(cmd, 1 :named('verbose'))
    system('perl -MExtUtils::Command -e rm_rf tmp')
  L4:

    $I0 = exists kv['smolder_url']
    unless $I0 goto L5
    .local pmc config
    config = get_config()
    cmd = "curl -F architecture="
    $S0 = config['cpuarch']
    cmd .= $S0
    cmd .= " -F platform="
    $S0 = config['osname']
    cmd .= $S0
    cmd .= " -F revision="
    $S0 = config['revision']
    cmd .= $S0
    $I0 = exists kv['smolder_tags']
    unless $I0 goto L6
    cmd .= " -F tags=\""
    $S0 = kv['smolder_tags']
    cmd .= $S0
    cmd .= "\""
  L6:
    $I0 = exists kv['smolder_comments']
    unless $I0 goto L7
    cmd .= " -F comments=\""
    $S0 = kv['smolder_comments']
    cmd .= $S0
    cmd .= "\""
  L7:
    cmd .= " -F report_file=@"
    cmd .= archive
    cmd .= " "
    $S0 = kv['smolder_url']
    cmd .= $S0
    system(cmd, 1 :named('verbose'))
  L5:
.end

.sub 'mk_extra_properties' :anon
    .param pmc hash
    $S0 = "extra_properties:\n"
    $P0 = iter hash
  L1:
    unless $P0 goto L2
    .local string key, value
    key = shift $P0
    value = hash[key]
    if value == '' goto L1
    $S0 .= "  "
    $S0 .= key
    $S0 .= ": "
    $S0 .= value
    $S0 .= "\n"
    goto L1
  L2:
    .return ($S0)
.end

=head3 Step install

=over 4

=item inst_bin ???

array of pathname or a single pathname

=item inst_dynext ???

array of pathname or a single pathname

=item inst_inc

array of pathname or a single pathname

=item inst_lang

array of pathname or a single pathname

=item inst_lib

array of pathname or a single pathname

=item installable_pbc

=item dynops

=item dynpmc

=item root

=cut

.sub '_install' :anon
    .param pmc kv :slurpy :named
    .local string root
    root = get_value('root', '' :named('default'), kv :flat :named)

    $P0 = get_install_files(kv :flat :named)
    $P1 = iter $P0
  L1:
    unless $P1 goto L2
    $S0 = shift $P1
    $S1 = $P0[$S0]
    $S2 = root . $S0
    install($S1, $S2, 1 :named('verbose'))
    goto L1
  L2:

    $P0 = get_install_xfiles(kv :flat :named)
    $P1 = iter $P0
  L3:
    unless $P1 goto L4
    $S0 = shift $P1
    $S1 = $P0[$S0]
    $S2 = root . $S0
    install($S1, $S2, 1 :named('exe'), 1 :named('verbose'))
    goto L3
  L4:
.end

.sub 'get_install_files' :anon
    .param pmc kv :slurpy :named
    .local pmc files
    files = new 'Hash'
    $I0 = exists kv['inst_bin']
    unless $I0 goto L1
    $P0 = kv['inst_bin']
    get_install_bin(files, $P0)
  L1:
    $I0 = exists kv['inst_dynext']
    unless $I0 goto L2
    $P0 = kv['inst_dynext']
    get_install_lib(files, "dynext", $P0)
  L2:
    $I0 = exists kv['inst_inc']
    unless $I0 goto L3
    $P0 = kv['inst_inc']
    get_install_lib(files, "include", $P0)
  L3:
    $I0 = exists kv['inst_lang']
    unless $I0 goto L4
    $P0 = kv['inst_lang']
    get_install_lib(files, "languages", $P0)
  L4:
    $I0 = exists kv['inst_lib']
    unless $I0 goto L5
    $P0 = kv['inst_lib']
    get_install_lib(files, "library", $P0)
  L5:
    .return (files)
.end

.sub 'get_install_bin' :anon
    .param pmc files
    .param pmc array
    $S1 = get_bindir()
    $S1 .= "/"
    $I0 = does array, 'array'
    if $I0 goto L1
    $S0 = array
    $S2 = $S1 . $S0
    files[$S2] = $S0
    goto L2
  L1:
    $P0 = iter array
  L3:
    unless $P0 goto L2
    $S0 = shift $P0
    $S2 = $S1 . $S0
    files[$S2] = $S0
    goto L3
  L2:
.end

.sub 'get_install_lib' :anon
    .param pmc files
    .param string dirname
    .param pmc array
    $S1 = get_libdir()
    $S1 .= "/"
    $S1 .= dirname
    $S1 .= "/"
    $I0 = does array, 'array'
    if $I0 goto L1
    $S0 = array
    $S2 = $S1 . $S0
    files[$S2] = $S0
    goto L2
  L1:
    $P0 = iter array
  L3:
    unless $P0 goto L2
    $S0 = shift $P0
    $S2 = $S1 . $S0
    files[$S2] = $S0
    goto L3
  L2:
.end

.sub 'get_install_xfiles' :anon
    .param pmc kv :slurpy :named
    .local pmc files
    files = new 'Hash'
    $I0 = exists kv['installable_pbc']
    unless $I0 goto L1
    $P0 = kv['installable_pbc']
    get_install_installable_pbc(files, $P0)
  L1:
    $I0 = exists kv['dynops']
    unless $I0 goto L2
    $P0 = kv['dynops']
    get_install_dynops(files, $P0)
  L2:
    $I0 = exists kv['dynpmc']
    unless $I0 goto L3
    $P0 = kv['dynpmc']
    get_install_dynpmc(files, $P0)
  L3:
    .return (files)
.end

.sub 'get_install_installable_pbc' :anon
    .param pmc files
    .param pmc hash
    .local string bin, bindir, pbc, exe
    bindir = get_bindir()
    exe = get_exe()
    $P0 = iter hash
  L1:
    unless $P0 goto L2
    bin = shift $P0
    pbc = hash[bin]
    $I0 = length pbc
    $I0 -= 4
    $S0 = substr pbc, 0, $I0
    $S1 = 'installable_' . $S0
    $S1 .= exe
    $S2 = bindir . '/'
    $S2 .= bin
    $S2 .= exe
    files[$S2] = $S1
    goto L1
  L2:
.end

.sub 'get_install_dynops' :anon
    .param pmc files
    .param pmc hash
    .local string libdir, load_ext, ops, suffix
    libdir = get_libdir()
    load_ext = get_load_ext()
    .local pmc cores
    cores = get_cores()
    $P0 = iter hash
  L1:
    unless $P0 goto L2
    ops = shift $P0
    $P1 = iter cores
  L3:
    unless $P1 goto L4
    $S0 = shift $P1
    suffix = cores[$S0]
    $S1 = _mk_path_dynops(ops, suffix, load_ext)
    $S2 = libdir . "/"
    $S2 .= $S1
    files[$S2] = $S1
    goto L3
  L4:
    goto L1
  L2:
.end

.sub 'get_install_dynpmc' :anon
    .param pmc files
    .param pmc hash
    .local string libdir, load_ext
    libdir = get_libdir()
    load_ext = get_load_ext()
    $P0 = iter hash
  L1:
    unless $P0 goto L2
    $S0 = shift $P0
    $S1 = _mk_path_dynpmc($S0, load_ext)
    $S2 = libdir . "/"
    $S2 .= $S1
    files[$S2] = $S1
    goto L1
  L2:
.end

=back

=head3 Step uninstall

Same options as install.

=cut

.sub '_uninstall' :anon
    .param pmc kv :slurpy :named
    .local string root
    root = get_value('root', '' :named('default'), kv :flat :named)

    $P0 = get_install_files(kv :flat :named)
    $P1 = iter $P0
  L1:
    unless $P1 goto L2
    $S0 = shift $P1
    $S0 = root . $S0
    unlink($S0, 1 :named('verbose'))
    goto L1
  L2:

    $P0 = get_install_xfiles(kv :flat :named)
    $P1 = iter $P0
  L3:
    unless $P1 goto L4
    $S0 = shift $P1
    $S0 = root . $S0
    unlink($S0, 1 :named('verbose'))
    goto L3
  L4:
.end

=head3 Step plumage

=over 4

=item name

=item abstract

=item authority

=item version

=item license_type

=item license_uri

=item copyright_holder

=item packager

=item description

=item keywords

=item checkout_uri

=item browser_uri

=item project_uri

=back

=cut

.sub '_plumage' :anon
    .param pmc kv :slurpy :named
    $S0 = get_plumage(kv :flat :named)
    $S1 = mk_plumage(kv :flat :named)
    $I0 = file_exists($S0)
    unless $I0 goto L1
    $S0 = kv['__target__']
    unless $S0 == 'plumage' goto L2
    print $S1
    goto L2
  L1:
    $S2 = dirname($S0)
    mkpath($S2, 1 :named('verbose'))
    spew($S0, $S1, 1 :named('verbose'))
  L2:
.end

.sub 'get_plumage' :anon
    .param pmc kv :slurpy :named
    $S0 = get_name(kv :flat :named)
    $S0 .= '.json'
    $S0 = 'ports/plumage/' . $S0
    .return ($S0)
.end

.sub 'mk_plumage' :anon
    .param pmc kv :slurpy :named

    .local string name
    name = get_name(kv :flat :named)

    .local string abstract
    $S0 = get_value('abstract', kv :flat :named)
    abstract = _json_escape($S0)

    .local string authority
    authority = get_value('authority', kv :flat :named)

    .local string version
    version = get_version(kv :flat :named)

    .local string license_type
    license_type = get_value('license_type', kv :flat :named)

    .local string license_uri
    license_uri = get_value('license_uri', kv :flat :named)

    .local string copyright_holder
    $S0 = get_value('copyright_holder', kv :flat :named)
    copyright_holder = _json_escape($S0)

    .local string packager
    $S0 = get_value('packager', "distutils" :named('default'), kv :flat :named)
    packager = _json_escape($S0)

    .local string description
    $S0 = get_value('description', kv :flat :named)
    description = _json_escape($S0)

    .local string keywords
    keywords = ''
    $I0 = exists kv['keywords']
    unless $I0 goto L10
    $P1 = kv['keywords']
    $S1 = join "\", \"", $P1
    keywords = "\"" . $S1
    keywords .= "\""
  L10:

    .local string vcs
    vcs = get_vcs()

    .local string checkout_uri
    checkout_uri = get_value('checkout_uri', kv :flat :named)

    .local string browser_uri
    browser_uri = get_value('browser_uri', kv :flat :named)

    .local string project_uri
    project_uri =get_value('project_uri', kv :flat :named)

    $P0 = new 'FixedStringArray'
    set $P0, 16
    $P0[0] = name
    $P0[1] = abstract
    $P0[2] = authority
    $P0[3] = version
    $P0[4] = license_type
    $P0[5] = license_uri
    $P0[6] = copyright_holder
    $P0[7] = packager
    $P0[8] = keywords
    $P0[9] = description
    $P0[10] = name
    $P0[11] = vcs
    $P0[12] = vcs
    $P0[13] = checkout_uri
    $P0[14] = browser_uri
    $P0[15] = project_uri

    $S0 = <<'TEMPLATE'
{
    "meta-spec"    : {
        "version"  : 1,
        "uri"      : "https://trac.parrot.org/parrot/wiki/ModuleEcosystem"
    },
    "general"      : {
        "name"     : "%s",
        "abstract" : "%s",
        "authority": "%s",
        "version"  : "%s",
        "license"  : {
            "type" : "%s",
            "uri"  : "%s"
        },
        "copyright_holder" : "%s",
        "generated_by"     : "%s",
        "keywords"         : [%s],
        "description"      : "%s"
    },
    "instructions" : {
        "fetch"    : {
            "type" : "repository"
        },
        "update"   : {
            "type" : "parrot_setup"
        },
        "build"    : {
            "type" : "parrot_setup"
        },
        "test"     : {
            "type" : "parrot_setup"
        },
        "smoke"    : {
            "type" : "parrot_setup"
        },
        "install"  : {
            "type" : "parrot_setup"
        },
        "uninstall": {
            "type" : "parrot_setup"
        },
        "clean"    : {
            "type" : "parrot_setup"
        }
    },
    "dependency-info"  : {
        "provides"     : ["%s"],
        "requires"     : {
            "fetch"    : ["%s"],
            "build"    : [],
            "test"     : ["perl5"],
            "install"  : [],
            "runtime"  : []
        }
    },
    "resources"            : {
        "repository"       : {
             "type"        : "%s",
             "checkout_uri": "%s",
             "browser_uri" : "%s",
             "project_uri" : "%s"
        }
    }
}
TEMPLATE
    $S0 = sprintf $S0, $P0
    .return ($S0)
.end

.sub '_json_escape' :anon
    .param string str
    $P0 = split '"', str
    str = join '\"', $P0
    $P0 = split "\n", str
    str = join "\\n", $P0
    .return (str)
.end

=head3 Step manifest

=over 4

=item manifest_includes

array of pathname or a single pathname

=item manifest_excludes

array of pathname or a single pathname

=item doc_files

array of pathname or a single pathname

=item pbc_pir, pir_pge, pir_tge, pir_nqp, pir_nqp-rx, pbc_pbc, exe_pbc,
installable_pbc, dynops, dynpmc, html_pod

=item inst_bin, inst_dynext, inst_inc, inst_lang, inst_lib

=item harness_files, prove_files

=back

=cut

.sub '_manifest' :anon
    .param pmc kv :slurpy :named

    $S0 = ''
    $I0 = file_exists('MANIFEST')
    unless $I0 goto L1
    $S0 = slurp('MANIFEST')
  L1:

    $S1 = mk_manifest(kv :flat :named)
    unless $S0 != $S1 goto L2
    spew('MANIFEST', $S1, 1 :named('verbose'))
  L2:
.end

.sub 'mk_manifest' :anon
    .param pmc kv :slurpy :named
    .local pmc needed, generated
    needed = new 'Hash'
    generated = new 'Hash'

    $P0 = split ' ', 'pbc_pir pir_pge pir_tge pir_nqp pir_nqp-rx pbc_pbc exe_pbc installable_pbc dynops dynpmc html_pod'
  L1:
    unless $P0 goto L2
    $S0 = shift $P0
    $I0 = exists kv[$S0]
    unless $I0 goto L1
    $P1 = kv[$S0]
    _manifest_add_hash(needed, generated, $P1)
    goto L1
  L2:

    $P0 = split ' ', 'inst_bin inst_dynext inst_inc inst_lang inst_lib doc_files'
  L3:
    unless $P0 goto L4
    $S0 = shift $P0
    $I0 = exists kv[$S0]
    unless $I0 goto L3
    $P1 = kv[$S0]
    _manifest_add_array(needed, $P1)
    goto L3
  L4:

    $P0 = split ' ', 'harness_files prove_files'
    $I1 = 0
  L5:
    unless $P0 goto L6
    $S0 = shift $P0
    $I0 = exists kv[$S0]
    unless $I0 goto L5
    $I1 = 1
    $S1 = kv[$S0]
    _manifest_add_glob(needed, $S1)
    goto L5
  L6:
    if $I1 goto L7
    _manifest_add_glob(needed, 't/*.t')
  L7:

    $P0 = split ' ', 'setup.pir setup.nqp t/harness'
  L8:
    unless $P0 goto L9
    $S0 = shift $P0
    $I0 = file_exists($S0)
    unless $I0 goto L8
    needed[$S0] = 1
    goto L8
  L9:

    $S0 = get_license_file()
    if $S0 == '' goto L10
    needed[$S0] = 1
  L10:

    $P0 = iter generated
  L11:
    unless $P0 goto L12
    $S0 = shift $P0
    delete needed[$S0]
    goto L11
  L12:

    $I0 = exists kv['manifest_includes']
    unless $I0 goto L13
    $P1 = kv['manifest_includes']
    _manifest_add_array(needed, $P1)
  L13:

    $I0 = exists kv['manifest_excludes']
    unless $I0 goto L14
    $P1 = kv['manifest_excludes']
    _manifest_del_array(needed, $P1)
  L14:

    $P1 = iter needed
    $I0 = elements $P1
    inc $I0
    $P0 = new 'FixedPMCArray'
    # currently, FixedStringArray hasn't the method sort.
    # see TT #1356
    set $P0, $I0
    $I0 = 0
    $P0[$I0] = 'MANIFEST'
  L21:
    unless $P1 goto L22
    $S0 = shift $P1
    inc $I0
    $P0[$I0] = $S0
    goto L21
  L22:

    $P0.'sort'()
    $S0 = join "\n", $P0
    $S0 .= "\n"
    .return ($S0)
.end

.sub '_manifest_add_hash' :anon
    .param pmc needed
    .param pmc generated
    .param pmc hash
    $P0 = iter hash
  L1:
    unless $P0 goto L2
    .local string key
    key = shift $P0
    generated[key] = 1
    .local pmc depends
    depends = $P0[key]
    $I0 = does depends, 'array'
    unless $I0 goto L3
    $P1 = iter depends
  L4:
    unless $P1 goto L1
    $S0 = shift $P1
    if $S0 == '' goto L4
    needed[$S0] = 1
    goto L4
  L3:
    $S0 = depends
    needed[$S0] = 1
    goto L1
  L2:
.end

.sub '_manifest_add_array' :anon
    .param pmc needed
    .param pmc array
    $I0 = does array, 'array'
    unless $I0 goto L1
    $P0 = iter array
  L2:
    unless $P0 goto L3
    $S0 = shift $P0
    needed[$S0] = 1
    goto L2
  L1:
    $S0 = array
    needed[$S0] = 1
  L3:
.end

.sub '_manifest_del_array' :anon
    .param pmc needed
    .param pmc array
    $I0 = does array, 'array'
    unless $I0 goto L1
    $P0 = iter array
  L2:
    unless $P0 goto L3
    $S0 = shift $P0
    delete needed[$S0]
    goto L2
  L1:
    $S0 = array
    delete needed[$S0]
  L3:
.end

.sub '_manifest_add_glob' :anon
    .param pmc needed
    .param string str
    $P0 = glob(str)
  L1:
    unless $P0 goto L2
    $S0 = shift $P0
    needed[$S0] = 1
    goto L1
  L2:
.end

=head3 Step sdist

On Windows calls sdist_zip, otherwise sdist_gztar

=cut

.sub '_sdist' :anon
    .param pmc kv :slurpy :named
    $P0 = get_config()
    $S0 = $P0['osname']
    unless $S0 == 'MSWin32' goto L1
    .tailcall run_step('sdist_zip', kv :flat :named)
  L1:
    .tailcall run_step('sdist_gztar', kv :flat :named)
.end

=head3 Step sdist_gztar

=cut

.sub '_sdist_gztar' :anon
    .param pmc kv :slurpy :named
    run_step('manifest', kv :flat :named)

    $S0 = slurp('MANIFEST')
    $P0 = split "\n", $S0
    $S0 = pop $P0
    $S0 = get_tarname('.tar.gz', kv :flat :named)
    $I0 = newer($S0, $P0)
    if $I0 goto L1
    $S0 = get_tarname('', kv :flat :named)
    copy_sdist($S0, $P0)

    .local string cmd
    cmd = 'tar -cvf ' . $S0
    cmd .= '.tar '
    cmd .= $S0
    system(cmd, 1 :named('verbose'))

    rmtree($S0)

    cmd = 'gzip --best ' . $S0
    cmd .= '.tar'
    system(cmd, 1 :named('verbose'))
  L1:
.end

.sub '_clean_gztar' :anon
    .param pmc kv :slurpy :named

    $S0 = get_tarname('.tar.gz', kv :flat :named)
    unlink($S0, 1 :named('verbose'))
    unlink('MANIFEST', 1 :named('verbose'))
.end

.sub 'copy_sdist' :anon
    .param string dirname
    .param pmc files
    mkdir(dirname)
    $S1 = dirname . "/"
    $P0 = iter files
  L1:
    unless $P0 goto L2
    $S0 = shift $P0
    $S2 = $S1 . $S0
    install($S0, $S2)
    goto L1
  L2:
.end

.sub 'get_tarname' :anon
    .param string ext
    .param pmc kv :slurpy :named

    $S0 = 'parrot-'
    $S1 = get_name(kv :flat :named)
    $S0 .= $S1
    $S0 .= '-'
    $S1 = get_version(kv :flat :named)
    $S0 .= $S1
    $S0 .= ext
    .return ($S0)
.end

=head3 Step sdist_bztar

=cut

.sub '_sdist_bztar' :anon
    .param pmc kv :slurpy :named
    run_step('manifest', kv :flat :named)

    $S0 = slurp('MANIFEST')
    $P0 = split "\n", $S0
    $S0 = pop $P0
    $S0 = get_tarname('.tar.bz2', kv :flat :named)
    $I0 = newer($S0, $P0)
    if $I0 goto L1
    $S0 = get_tarname('', kv :flat :named)
    copy_sdist($S0, $P0)

    .local string cmd
    cmd = 'tar -cvf ' . $S0
    cmd .= '.tar '
    cmd .= $S0
    system(cmd, 1 :named('verbose'))

    rmtree($S0)

    cmd = 'bzip2 ' . $S0
    cmd .= '.tar'
    system(cmd, 1 :named('verbose'))
  L1:
.end

.sub '_clean_bztar' :anon
    .param pmc kv :slurpy :named

    $S0 = get_tarname('.tar.bz2', kv :flat :named)
    unlink($S0, 1 :named('verbose'))
    unlink('MANIFEST', 1 :named('verbose'))
.end

=head3 Step sdist_zip

=cut

.sub '_sdist_zip' :anon
    .param pmc kv :slurpy :named
    run_step('manifest', kv :flat :named)

    $S0 = slurp('MANIFEST')
    $P0 = split "\n", $S0
    $S0 = pop $P0
    $S0 = get_tarname('.zip', kv :flat :named)
    $I0 = newer($S0, $P0)
    if $I0 goto L1
    .local string cmd
    $S0 = get_tarname('', kv :flat :named)
    copy_sdist($S0, $P0)

    cmd = 'zip -9 -r '
    cmd .= $S0
    cmd .= '.zip '
    cmd .= $S0
    system(cmd, 1 :named('verbose'))

    rmtree($S0)
  L1:
.end

.sub '_clean_zip' :anon
    .param pmc kv :slurpy :named

    $S0 = get_tarname('.zip', kv :flat :named)
    unlink($S0, 1 :named('verbose'))
    unlink('MANIFEST', 1 :named('verbose'))
.end

=head3 Step sdist_rpm

=cut

.sub '_sdist_rpm' :anon
    .param pmc kv :slurpy :named
    run_step('sdist_gztar', kv :flat :named)

    .local string rpm_base
    rpm_base = get_value('rpm_base', 'rpm' :named('default'), kv :flat :named)

    $S1 = get_tarname('.tar.gz', kv :flat :named)
    $S2 = rpm_base . "/SOURCES/"
    $S2 .= $S1
    install($S1, $S2, 1 :named('verbose'))

    run_step('spec', kv :flat :named)

    .local string cmd
    cmd = "rpmbuild --define '_topdir "
    $S0 = cwd()
    cmd .= $S0
    cmd .= "/"
    cmd .= rpm_base
    cmd .= "' -bs -v "
    $S0 = get_spec(kv :flat :named)
    cmd .= $S0
    system(cmd, 1 :named('verbose'))
.end

=head3 Step bdist

On Windows calls bdist_wininst, otherwise bdist_rpm

=cut

.sub '_bdist' :anon
    .param pmc kv :slurpy :named
    $P0 = get_config()
    $S0 = $P0['osname']
    unless $S0 == 'MSWin32' goto L1
    .tailcall run_step('bdist_wininst', kv :flat :named)
  L1:
    .tailcall run_step('bdist_rpm', kv :flat :named)
.end

=head3 Step spec

=over 4

=item rpm_base

the default value is ports/rpm

=item name

=item version

=item release

=item abstract

=item license_type

=item project_uri

=item description

=item packager

=item doc_files

=item installable_pbc, dynops, dynpmc

=item inst_bin, inst_dynext, inst_inc, inst_lang, inst_lib

=back

=cut

.sub '_spec_rpm' :anon
    .param pmc kv :slurpy :named
    $S0 = get_spec(kv :flat :named)
    $S1 = mk_spec(kv :flat :named)
    $I0 = file_exists($S0)
    unless $I0 goto L1
    $S0 = kv['__target__']
    unless $S0 == 'spec' goto L2
    print $S1
    goto L2
  L1:
    $S2 = dirname($S0)
    mkpath($S2, 1 :named('verbose'))
    spew($S0, $S1, 1 :named('verbose'))
    .local string cmd
    $S2 = dirname($S2)
    cmd = "rpmbuild --define '_topdir " . $S2
    cmd .= "' --nobuild "
    cmd .= $S0
    system(cmd, 1 :named('verbose'), 1 :named('ignore_error'))
  L2:
.end

.sub 'get_spec' :anon
    .param pmc kv :slurpy :named
    $S0 = get_value('rpm_base', 'rpm' :named('default'), kv :flat :named)
    $S0 .= "/SPECS/parrot-"
    $S1 = get_name(kv :flat :named)
    $S0 .= $S1
    $S0 .= '.spec'
    .return ($S0)
.end

.sub 'mk_spec' :anon
    .param pmc kv :slurpy :named

    .local pmc config
    config = get_config()

    .local string parrot_version
    $S1 = config['VERSION']
    $S2 = config['DEVEL']
    parrot_version = $S1 . $S2

    .local string name
    name = get_name(kv :flat :named)

    .local string version
    version = get_version(kv :flat :named)

    .local string release
    release = get_value('release', '1' :named('default'), kv :flat :named)

    .local string abstract
    abstract = get_value('abstract', kv :flat :named)

    .local string license_type
    license_type = get_value('license_type', kv :flat :named)

    .local string project_uri
    project_uri =get_value('project_uri', kv :flat :named)

    .local string tarball
    tarball = get_tarname('.tar.gz', kv :flat :named)

    .local string description
    description = get_value('description', kv :flat :named)

    .local string packager
    packager = get_value('packager', "you <you@you.org>" :named('default'), kv :flat :named)

    $P0 = new 'FixedStringArray'
    set $P0, 9
    $P0[0] = parrot_version
    $P0[1] = name
    $P0[2] = version
    $P0[3] = release
    $P0[4] = abstract
    $P0[5] = license_type
    $P0[6] = project_uri
    $P0[7] = tarball
    $P0[8] = description

    $S0 = <<'TEMPLATE'
%%define parrot_version %s

Name:           parrot-%s
Version:        %s
Release:        %s
Summary:        %s
License:        %s
Group:          Development/Libraries
URL:            %s
Source0:        %s
BuildRoot:      %%{_tmppath}/%%{name}-%%{version}-%%{release}
#BuildRequires:  parrot           = %%parrot_version
#BuildRequires:  parrot-devel     = %%parrot_version

%%description
%s

%%prep
%%setup -n %%{name}-%%{version}

%%build
parrot setup.pir

%%install
rm -rf $RPM_BUILD_ROOT
parrot setup.pir --root $RPM_BUILD_ROOT install

%%check
parrot setup.pir test

%%clean
rm -rf $RPM_BUILD_ROOT

%%files
%%defattr(-,root,root,-)
TEMPLATE
    .local string spec
    spec = sprintf $S0, $P0

    $S0 = mk_deb_docs(kv :flat :named)
    if $S0 == '' goto L1
    spec .= "%doc "
    $P1 = split "\n", $S0
    $S0 = pop $P1
    $S0 = join "\n%doc ", $P1
    spec .= $S0
    spec .= "\n"
  L1:

    $S0 = mk_deb_install(kv :flat :named)
    $P1 = split parrot_version, $S0
    $S0 = join "%{parrot_version}", $P1
    spec .= $S0

    spec .= "\n%changelog\n* "
    $I0 = time
    $S0 = localtime $I0
    $I0 = length $S0
    dec $I0
    $S0 = substr $S0, 0, $I0
    $S1 = substr $S0, 0, 11
    spec .= $S1
    $S1 = substr $S0, 20
    spec .= $S1
    spec .= " "
    spec .= packager
    spec .= "\n- created by distutils\n"
    .return (spec)
.end

=head3 Step bdist_rpm

=cut

.sub '_bdist_rpm' :anon
    .param pmc kv :slurpy :named
    run_step('sdist_gztar', kv :flat :named)

    .local string rpm_base
    rpm_base = get_value('rpm_base', 'rpm' :named('default'), kv :flat :named)

    $S1 = get_tarname('.tar.gz', kv :flat :named)
    $S2 = rpm_base . "/SOURCES/"
    $S2 .= $S1
    install($S1, $S2, 1 :named('verbose'))

    run_step('spec', kv :flat :named)

    .local string cmd
    cmd = "rpmbuild --define '_topdir "
    $S0 = cwd()
    cmd .= $S0
    cmd .= "/"
    cmd .= rpm_base
    cmd .= "' -bb -v "
    $S0 = get_spec(kv :flat :named)
    cmd .= $S0
    system(cmd, 1 :named('verbose'))
.end

=head3 Step control

See L<http://www.debian.org/doc/maint-guide/>.

=over 4

=item control_dir

the default value is ports/debian

=item name

=item packager

=item project_uri

=item abstract

=item description

=item release

=item copyright_holder

=item doc_files

=item installable_pbc, dynops, dynpmc

=item inst_bin, inst_dynext, inst_inc, inst_lang, inst_lib

=back

=cut

.sub '_control_deb' :anon
    .param pmc kv :slurpy :named

    $S0 = get_deb('control', kv :flat :named)
    $S1 = dirname($S0)
    mkpath($S1, 1 :named('verbose'))
    $S1 = mk_deb_control(kv :flat :named)
    $I0 = file_exists($S0)
    unless $I0 goto L1
    $S0 = kv['__target__']
    unless $S0 == 'control' goto L2
    print $S1
    goto L3
  L1:
    spew($S0, $S1, 1 :named('verbose'))
  L2:

    $S0 = get_deb('changelog', kv :flat :named)
    $I0 = file_exists($S0)
    if $I0 goto L4
    $S1 = mk_deb_changelog(kv :flat :named)
    spew($S0, $S1, 1 :named('verbose'))
  L4:

    $S0 = get_deb('copyright', kv :flat :named)
    $I0 = file_exists($S0)
    if $I0 goto L5
    $S1 = mk_deb_copyright(kv :flat :named)
    spew($S0, $S1, 1 :named('verbose'))
  L5:

    $S0 = get_deb('rules', kv :flat :named)
    $S1 = mk_deb_rules(kv :flat :named)
    spew($S0, $S1, 1 :named('verbose'))

    $S0 = get_deb_ext('.docs', kv :flat :named)
    $S1 = mk_deb_docs(kv :flat :named)
    spew($S0, $S1, 1 :named('verbose'))

    $S0 = get_deb_ext('.install', kv :flat :named)
    $S1 = mk_deb_install(kv :flat :named)
    spew($S0, $S1, 1 :named('verbose'))
  L3:
.end

.sub 'get_deb' :anon
    .param string filename
    .param pmc kv :slurpy :named
    $S0 = get_value('control_dir', 'ports/debian' :named('default'), kv :flat :named)
    $S0 .= "/"
    $S0 .= filename
    .return ($S0)
.end

.sub 'get_deb_ext' :anon
    .param string ext
    .param pmc kv :slurpy :named
    $S0 = get_value('control_dir', 'ports/debian' :named('default'), kv :flat :named)
    $S0 .= "/parrot-"
    $S1 = get_name(kv :flat :named)
    $S0 .= $S1
    $S0 .= ext
    .return ($S0)
.end

.sub 'mk_deb_control' :anon
    .param pmc kv :slurpy :named

    .local pmc config
    config = get_config()

    .local string parrot_version
    $S1 = config['VERSION']
    $S2 = config['DEVEL']
    parrot_version = $S1 . $S2

    .local string name
    name = get_name(kv :flat :named)

    .local string packager
    packager = get_value('packager', "you <you@you.org>" :named('default'), kv :flat :named)

    .local string project_uri
    project_uri =get_value('project_uri', kv :flat :named)

    .local string abstract
    abstract = get_value('abstract', kv :flat :named)

    .local string description
    $S0 = get_value('description', kv :flat :named)
    $P0 = split "\n", $S0
    description = join "\n ", $P0

    $P0 = new 'FixedStringArray'
    set $P0, 8
    $P0[0] = name
    $P0[1] = packager
    $P0[2] = parrot_version
    $P0[3] = project_uri
    $P0[4] = name
    $P0[5] = parrot_version
    $P0[6] = abstract
    $P0[7] = description

    $S0 = <<'TEMPLATE'
Source: parrot-%s
Section: interpreters
Priority: optional
Maintainer: %s
Build-Depends: debhelper (>= 7), parrot-devel (= %s)
Standards-Version: 3.8.3
Homepage: %s

Package: parrot-%s
Architecture: any
Depends: parrot-minimal (= %s)
Description: %s
 %s
TEMPLATE
    $S0 = sprintf $S0, $P0
    .return ($S0)
.end

.sub 'mk_deb_changelog' :anon
    .param pmc kv :slurpy :named

    .local string name
    name = get_name(kv :flat :named)

    .local string version
    version = get_version(kv :flat :named)

    .local string release
    release = get_value('release', '1' :named('default'), kv :flat :named)

    .local string packager
    packager = get_value('packager', "you <you@you.org>" :named('default'), kv :flat :named)

    .local string timestamp
    timestamp = get_timestamp()

    $P0 = new 'FixedStringArray'
    set $P0, 5
    $P0[0] = name
    $P0[1] = version
    $P0[2] = release
    $P0[3] = packager
    $P0[4] = timestamp

    $S0 = <<'TEMPLATE'
parrot-%s (%s-%s) unstable; urgency=low

  * Created by distutils.

 -- %s  %s
TEMPLATE
    $S0 = sprintf $S0, $P0
    .return ($S0)
.end

.sub 'mk_deb_copyright' :anon
    .param pmc kv :slurpy :named

    .local string packager
    packager = get_value('packager', "you <you@you.org>" :named('default'), kv :flat :named)

    .local string timestamp
    timestamp = get_timestamp()

    .local string project_uri
    project_uri =get_value('project_uri', kv :flat :named)

    .local string copyright_holder
    copyright_holder = get_value('copyright_holder', kv :flat :named)

    .local string license
    license = ''
    $S0 = get_license_file()
    if $S0 == '' goto L1
    $S0 = slurp($S0)
    $P0 = split "\n", $S0
    license = join "\n    ", $P0
  L1:

    $P0 = new 'FixedStringArray'
    set $P0, 5
    $P0[0] = packager
    $P0[1] = timestamp
    $P0[2] = project_uri
    $P0[3] = copyright_holder
    $P0[4] = license

    $S0 = <<'TEMPLATE'
This package was debianized by %s on
%s.

It was downloaded from %s

Copyright for the code is held by: %s

License:

    %s
TEMPLATE
    $S0 = sprintf $S0, $P0
    .return ($S0)
.end

.sub 'mk_deb_rules' :anon
    .param pmc kv :slurpy :named

    $S0 = <<'TEMPLATE'
#!/usr/bin/make -f
# -*- makefile -*-
# Uncomment this to turn on verbose mode.
#export DH_VERBOSE=1

configure: configure-stamp
configure-stamp:
	dh_testdir
	touch configure-stamp

build: build-stamp
build-stamp: configure-stamp
	dh_testdir
	parrot setup.pir build
	touch $@

clean:
	dh_testdir
	dh_testroot
	rm -f build-stamp configure-stamp
	parrot setup.pir clean
	dh_clean

install: build
	dh_testdir
	dh_testroot
	dh_prep
	dh_installdirs
	parrot setup.pir --root $(CURDIR)/debian/tmp install
	dh_install --sourcedir=$(CURDIR)/debian/tmp --list-missing

# Build architecture-independent files here.
binary-indep: install
# We have nothing to do by default.

# Build architecture-dependent files here.
binary-arch: build install
	dh_testdir
	dh_testroot
	dh_installchangelogs
	dh_installdocs
	dh_installexamples
	dh_installman
	dh_link
	dh_strip
	dh_compress
	dh_fixperms
	dh_installdeb
	dh_shlibdeps
	dh_gencontrol
	dh_md5sums
	dh_builddeb

binary: binary-indep binary-arch
.PHONY: build clean binary-indep binary-arch binary install configure

TEMPLATE
    .return ($S0)
.end

.sub 'mk_deb_docs' :anon
    .param pmc kv :slurpy :named
    $S0 = ''
    $I0 = exists kv['doc_files']
    unless $I0 goto L1
    $P0 = kv['doc_files']
    $I0 = does $P0, 'array'
    if $I0 goto L2
    $S0 = $P0
    goto L1
  L2:
    $S0 = join "\n", $P0
  L1:
    $S0 .= "\n"
    .return ($S0)
.end

.sub 'mk_deb_install' :anon
    .param pmc kv :slurpy :named
    $P0 = new 'ResizablePMCArray'
    # currently, ResizableStringArray hasn't the method sort.
    # see TT #1356
    $P1 = get_install_files(kv :flat :named)
    $P2 = iter $P1
  L1:
    unless $P2 goto L2
    $S0 = shift $P2
    push $P0, $S0
    goto L1
  L2:
    $P1 = get_install_xfiles(kv :flat :named)
    $P2 = iter $P1
  L3:
    unless $P2 goto L4
    $S0 = shift $P2
    push $P0, $S0
    goto L3
  L4:
    $P0.'sort'()
    $S0 = join "\n", $P0
    $S0 .= "\n"
    .return ($S0)
.end

.sub 'get_timestamp' :anon
    $P0 = open 'date --rfc-2822', 'rp'
    $S0 = $P0.'readline'()
    $P0.'close'()
    $S0 = chopn $S0, 1
    .return ($S0)
.end

=head3 Step ebuild

See L<http://devmanual.gentoo.org/>.

=over 4

=item description

=item project_uri

=item license_type

=item doc_files

=back

=cut

.sub '_ebuild_gentoo' :anon
    .param pmc kv :slurpy :named
    $S0 = get_ebuild(kv :flat :named)
    $S1 = mk_ebuild(kv :flat :named)
    $I0 = file_exists($S0)
    unless $I0 goto L1
    $S0 = kv['__target__']
    unless $S0 == 'ebuild' goto L2
    print $S1
    goto L2
  L1:
    $S2 = dirname($S0)
    mkpath($S2, 1 :named('verbose'))
    spew($S0, $S1, 1 :named('verbose'))
  L2:
.end

.sub 'get_ebuild' :anon
    .param pmc kv :slurpy :named
    $S0 = "ports/gentoo/parrot-"
    $S1 = get_name(kv :flat :named)
    $S0 .= $S1
    $S0 .= "-"
    $S1 = get_version(kv :flat :named)
    $S0 .= $S1
    $S0 .= '.ebuild'
    .return ($S0)
.end

.sub 'mk_ebuild' :anon
    .param pmc kv :slurpy :named

    .local string description
    $S0 = get_value('description', kv :flat :named)
    description = _json_escape($S0)

    .local string project_uri
    project_uri =get_value('project_uri', kv :flat :named)

    .local string license_type
    license_type = get_value('license_type', kv :flat :named)

    .local string doc
    doc = ''
    $I0 = exists kv['doc_files']
    unless $I0 goto L1
    doc = "    dodoc "
    $P0 = kv['doc_files']
    $I0 = does $P0, 'array'
    if $I0 goto L2
    $S0 = $P0
    goto L3
  L2:
    $S0 = join " ", $P0
  L3:
    doc .= $S0
    doc .= " || die"
  L1:

    $P0 = new 'FixedStringArray'
    set $P0, 4
    $P0[0] = description
    $P0[1] = project_uri
    $P0[2] = license_type
    $P0[3] = doc

    $S0 = <<'TEMPLATE'

DESCRIPTION="%s"
HOMEPAGE="%s"
SRC_URI="./${P}.tar.gz"

LICENSE="%s"
SLOT="0"
KEYWORDS="~arch"
IUSE=""

#DEPEND=""
#RDEPEND=""

src_compile() {
    parrot setup.pir build || die "build failed"
}

src_install() {
    parrot setup.pir --root ${D} install || die "install failed"
%s
}

src_test() {
    parrot setup.pir test || die "test failed"
}
TEMPLATE
    $S0 = sprintf $S0, $P0
    .return ($S0)
.end

=head3 Step bdist_wininst

Build an installer with Inno Setup.
See L<http://www.jrsoftware.org/>.

=over 4

=item name

=item version

=item copyright_holder

=item project_uri

=item installable_pbc, dynops, dynpmc

=item inst_bin, inst_dynext, inst_inc, inst_lang, inst_lib

=item doc_files

=back

=cut

.sub '_bdist_wininst' :anon
    .param pmc kv :slurpy :named
    run_step('build', kv :flat :named)

    $I0 = exists kv['installable_pbc']
    unless $I0 goto L1
    $P0 = kv['installable_pbc']
    .local string exe, bin, pbc
    exe = get_exe()
    $P1 = iter $P0
  L2:
    unless $P1 goto L1
    bin = shift $P1
    pbc = $P0[bin]
    $I0 = length pbc
    $I0 -= 4
    $S0 = substr pbc, 0, $I0
    $S1 = 'installable_' . $S0
    $S1 .= exe
    $S2 = bin . exe
    $I0 = newer($S2, $S1)
    if $I0 goto L2
    cp($S1, $S2, 1 :named('verbose'))
    goto L2
  L1:

    $S0 = mk_inno_script(kv :flat :named)
    spew('inno.iss', $S0)

    system("iscc inno.iss", 1 :named('verbose'))
.end

.sub 'mk_inno_script' :anon
    .param pmc kv :slurpy :named
    .local pmc config
    config = get_config()

    .local string prefix
    prefix = config['prefix']

    .local string parrot_version
    $S1 = config['VERSION']
    $S2 = config['DEVEL']
    parrot_version = $S1 . $S2

    .local string name
    name = get_Name(kv :flat :named)

    .local string version
    version = get_version(kv :flat :named)

    .local string copyright_holder
    copyright_holder = get_value('copyright_holder', kv :flat :named)

    .local string project_uri
    project_uri =get_value('project_uri', kv :flat :named)

    .local string license
    license = "; no LicenseFile"
    $S0 = get_license_file()
    if $S0 == '' goto L3
    license = "LicenseFile=" . $S0
  L3:

    .local string setupname
    setupname = get_setupname('', kv :flat :named)

    $P0 = new 'FixedStringArray'
    set $P0, 9
    $P0[0] = name
    $P0[1] = parrot_version
    $P0[2] = name
    $P0[3] = version
    $P0[4] = copyright_holder
    $P0[5] = project_uri
    $P0[6] = prefix
    $P0[7] = license
    $P0[8] = setupname

    $S0 = <<'TEMPLATE'
; generated by distutils.pir for the Inno Setup Script Compiler.

[Setup]
AppName=Parrot-%s
AppVerName=Parrot-%s-%s-%s
AppPublisher=%s
AppPublisherURL=%s
DefaultDirName={sd}%s
DefaultGroupName=Parrot
AllowNoIcons=yes
%s
OutputDir=.\\
OutputBaseFilename=%s
Compression=lzma
SolidCompression=yes
Uninstallable=no

[Files]
TEMPLATE
    .local string script
    script = sprintf $S0, $P0

    $I0 = exists kv['dynops']
    if $I0 goto L21
    $I0 = exists kv['dynpmc']
    if $I0 goto L21
    goto L22
  L21:
    script .= "Source: \".\\dynext\\*.dll\"; DestDir: \"{app}\\lib\\parrot\\dynext\"; Flags:\n"
  L22:

    $I0 = exists kv['installable_pbc']
    unless $I0 goto L23
    $P1 = kv['installable_pbc']
    $P2 = iter $P1
  L24:
    unless $P2 goto L23
    $S0 = shift $P2
    script .= "Source: \".\\"
    script .= $S0
    script .= ".exe\"; DestDir: \"{app}\\bin\"; Flags:\n"
    goto L24
  L23:

    $P1 = get_install_files(kv :flat :named)
    $P2 = iter $P1
  L31:
    unless $P2 goto L32
    $S0 = shift $P2
    $S1 = $P1[$S0]
    $S0 = _mk_inno_line($S1, $S0)
    script .= $S0
    goto L31
  L32:

    $I0 = exists kv['doc_files']
    unless $I0 goto L41
    name = get_name(kv :flat :named)
    $P1 = kv['doc_files']
    $I0 = does $P1, 'array'
    if $I0 goto L42
    $S0 = $P1
    $S0 = _mk_inno_line_doc($S0, name)
    script .= $S0
    goto L41
  L42:
    $P2 = iter $P1
  L43:
    unless $P2 goto L41
    $S0 = shift $P2
    $S0 = _mk_inno_line_doc($S0, name)
    script .= $S0
    goto L43
  L41:

    script .= "\n"
    .return (script)
.end

.sub '_mk_inno_line' :anon
    .param string src
    .param string dest
    .local string line
    line = "Source: \".\\"
    $S0 = _escape_path_win32(src)
    line .= $S0
    line .= "\"; DestDir: \"{app}\\"
    $S0 = dirname(dest)
    $I0 = index $S0, '/', 1
    inc $I0
    $S0 = substr $S0, $I0
    $S0 = _escape_path_win32($S0)
    line .= $S0
    line .= "\"; Flags:\n"
    .return (line)
.end

.sub '_mk_inno_line_doc' :anon
    .param string src
    .param string dest
    .local string line
    line = "Source: \".\\"
    $S0 = _escape_path_win32(src)
    line .= $S0
    line .= "\"; DestDir: \"{app}\\share\\doc\\parrot\\"
    $S0 = dest
    line .= $S0
    line .= "\"; Flags:\n"
    .return (line)
.end

.sub '_escape_path_win32' :anon
    .param string path
    $P0 = split "/", path
    $S0 = join "\\", $P0
    .return ($S0)
.end

.sub 'get_setupname' :anon
    .param string ext
    .param pmc kv :slurpy :named
    .local pmc config
    config = get_config()

    $S0 = 'setup-parrot-'
    $S1 = config['VERSION']
    $S0 .= $S1
    $S1 = config['DEVEL']
    $S0 .= $S1
    $S0 .= '-'
    $S1 = get_name(kv :flat :named)
    $S0 .= $S1
    $S0 .= '-'
    $S1 = get_version(kv :flat :named)
    $S0 .= $S1
    $S0 .= ext
    .return ($S0)
.end

.sub '_clean_wininst' :anon
    .param pmc kv :slurpy :named

    $I0 = exists kv['installable_pbc']
    unless $I0 goto L1
    $P0 = kv['installable_pbc']
    .local string exe, bin
    exe = get_exe()
    $P1 = iter $P0
  L2:
    unless $P1 goto L1
    bin = shift $P1
    $S0 = bin . exe
    unlink($S0, 1 :named('verbose'))
    goto L2
  L1:

    unlink('inno.iss', 1 :named('verbose'))
    $S0 = get_setupname('.exe', kv :flat :named)
    unlink($S0, 1 :named('verbose'))
.end

=head2 Configuration Helpers

=over 4

=item get_config

Return the whole config

=cut

.include 'iglobals.pasm'

.sub 'get_config'
    $P0 = getinterp
    $P1 = $P0[.IGLOBALS_CONFIG_HASH]
    .return ($P1)
.end

=item get_bindir

=cut

.sub 'get_bindir'
    $P0 = get_config()
    $S0 = $P0['bindir']
    .return ($S0)
.end

=item get_cflags

=cut

.sub 'get_cflags'
    $P0 = get_config()
    .local string flags
    flags = $P0['ccflags']
    flags .= " "
    $S0 = $P0['cc_shared']
    flags .= $S0
    flags .= " "
    $S0 = $P0['cc_debug']
    flags .= $S0
    flags .= " "
    $S0 = $P0['ccwarn']
    flags .= $S0
    flags .= " "
    $S0 = $P0['cc_hasjit']
    flags .= $S0
    flags .= " "
    $S0 = $P0['cg_flag']
    flags .= $S0
    .return (flags)
.end

=item get_compiler

=cut

.sub 'get_compiler'
    .param string name
    $P0 = get_config()
    $I0 = $P0['installed']
    unless $I0 goto L1
    $S0 = $P0['libdir']
    $S1 = $P0['versiondir']
    $S0 .= $S1
    $S0 .= '/languages/'
    goto L2
  L1:
    $S0 = $P0['prefix']
    $S0 .= '/compilers/'
  L2:
    $S0 .= name
    .return ($S0)
.end

=item get_exe

=cut

.sub 'get_exe'
    $P0 = get_config()
    $S0 = $P0['exe']
    .return ($S0)
.end

=item get_executable

=cut

.sub 'get_executable'
    .param string name
    $P0 = get_config()
    $I0 = $P0['installed']
    unless $I0 goto L1
    $S0 = $P0['bindir']
    goto L2
  L1:
    $S0 = $P0['prefix']
  L2:
    $S0 .= '/'
    $S0 .= name
    $S1 = $P0['exe']
    $S0 .= $S1
    .return ($S0)
.end

=item get_incdir

=cut

.sub 'get_incdir'
    $P0 = get_config()
    $S0 = $P0['includedir']
    $S1 = $P0['versiondir']
    $S0 .= $S1
    .return ($S0)
.end

=item get_libdir

=cut

.sub 'get_libdir'
    $P0 = get_config()
    $S0 = $P0['libdir']
    $S1 = $P0['versiondir']
    $S0 .= $S1
    .return ($S0)
.end

=item get_library

=cut

.sub 'get_library'
    .param string name
    $P0 = get_config()
    $I0 = $P0['installed']
    unless $I0 goto L1
    $S0 = $P0['libdir']
    $S1 = $P0['versiondir']
    $S0 .= $S1
    $S0 .= '/library/'
    goto L2
  L1:
    $S0 = $P0['prefix']
    $S0 .= '/runtime/parrot/library/'
  L2:
    $S0 .= name
    .return ($S0)
.end

=item get_ldflags

=cut

.sub 'get_ldflags'
    $P0 = get_config()
    .local string flags
    flags = $P0['ldflags']
    flags .= " "
    $S0 = $P0['ld_debug']
    flags .= $S0
    flags .= " "
    $S0 = $P0['rpath_blib']
    flags .= $S0
    flags .= " "
    $S0 = $P0['linkflags']
    flags .= $S0
    .return (flags)
.end

=item get_load_ext

=cut

.sub 'get_load_ext'
    $P0 = get_config()
    $S0 = $P0['load_ext']
    .return ($S0)
.end

=item get_obj

=cut

.sub 'get_obj'
    $P0 = get_config()
    $S0 = $P0['o']
    .return ($S0)
.end

=item get_parrot

=cut

.sub 'get_parrot'
    .tailcall get_executable('parrot')
.end

=item get_nqp

=cut

.sub 'get_nqp'
    .tailcall get_executable('parrot-nqp')
.end

=item get_srcdir

=cut

.sub 'get_srcdir'
    $P0 = get_config()
    $S0 = $P0['srcdir']
    $S1 = $P0['versiondir']
    $S0 .= $S1
    .return ($S0)
.end

=item get_tool

=cut

.sub 'get_tool'
    .param string name
    $P0 = get_config()
    $I0 = $P0['installed']
    unless $I0 goto L1
    $S0 = $P0['libdir']
    $S1 = $P0['versiondir']
    $S0 .= $S1
    goto L2
  L1:
    $S0 = $P0['prefix']
  L2:
    $S0 .= '/tools/'
    $S0 .= name
    .return ($S0)
.end

=item get_vcs

=cut

.sub 'get_vcs'
    .local string vcs
    vcs = 'VCS'
    $I0 = file_exists('CVS')
    unless $I0 goto L1
    vcs = 'cvs'
    goto L9
  L1:
    $I0 = file_exists('.git')
    unless $I0 goto L2
    vcs = 'git'
    goto L9
  L2:
    $I0 = file_exists('.hg')
    unless $I0 goto L3
    vcs = 'hg'
    goto L9
  L3:
    $I0 = file_exists('.svn')
    unless $I0 goto L4
    vcs = 'svn'
    goto L9
  L4:
  L9:
    .return (vcs)
.end

=item get_license_file

=cut

.sub 'get_license_file'
    $P0 = split ' ', "LICENSE COPYING COPYRIGHT"
  L1:
    unless $P0 goto L2
    $S0 = shift $P0
    $I0 = file_exists($S0)
    unless $I0 goto L1
    .return ($S0)
  L2:
    .return ('')
.end

=item get_Name

=cut

.sub 'get_Name'
    .param string name          :named('name') :optional
    .param int has_name         :opt_flag
    .param pmc extra            :slurpy :named
    unless has_name goto L1
    .return (name)
  L1:
    $S0 = cwd()
    $S0 = basename($S0)
    .return ($S0)
.end

=item get_name

=cut

.sub 'get_name'
    .param string name          :named('name') :optional
    .param int has_name         :opt_flag
    .param pmc extra            :slurpy :named
    unless has_name goto L1
    $S0 = downcase name
    .return ($S0)
  L1:
    $S0 = cwd()
    $S0 = basename($S0)
    $S0 = downcase $S0
    .return ($S0)
.end

=item get_version

=cut

.sub 'get_version'
    .param pmc kv :slurpy :named
    .tailcall get_value('version', 'HEAD' :named('default'), kv :flat :named)
.end

=item get_value

=cut

.sub 'get_value'
    .param string key
    .param string default       :named('default') :optional
    .param int has_default      :opt_flag
    .param pmc kv :slurpy :named
    $I0 = exists kv[key]
    unless $I0 goto L1
    $S0 = kv[key]
    .return ($S0)
  L1:
    unless has_default goto L2
    .return (default)
  L2:
    $S0 = upcase key
    .return ($S0)
.end

=item probe_include

=cut

.sub 'probe_include'
    .param string include
    .param int verbose          :named('verbose') :optional
    .param string cflags        :named('cflags') :optional
    .param int has_cflags       :opt_flag

    $S0 = <<'SOURCE_C'
#include <%s>
#include <stdio.h>

int
main(int argc, char* argv[])
{
    printf("%s OK\n");
    return 0;
}
SOURCE_C
    $P0 = new 'FixedStringArray'
    set $P0, 2
    $P0[0] = include
    $P0[1] = include
    $S0 = sprintf $S0, $P0
    spew('probe.c', $S0)

    .local string probe
    $S0 = get_exe()
    probe = "probe" . $S0
    .local pmc config
    config = get_config()
    .local string cmd
    cmd = config['cc']
    cmd .= " "
    $S0 = get_cflags()
    cmd .= $S0
    unless has_cflags goto L1
    cmd .= " "
    cmd .= cflags
  L1:
    cmd .= " probe.c -o "
    cmd .= probe
    system(cmd, verbose :named('verbose'), 1 :named('ignore_error'))

    cmd = "./" . probe
    $I0 = system(cmd, verbose :named('verbose'), 1 :named('ignore_error'))

    unlink('probe.c', verbose :named('verbose'))
    unlink(probe, verbose :named('verbose'))
    .return ($I0)
.end

=back

=head2 OS Utilities

=over 4

=item system

=cut

.sub 'system'
    .param string cmd
    .param int verbose          :named('verbose') :optional
    .param int has_verbose      :opt_flag
    .param int ignore_error     :named('ignore_error') :optional
    .param int has_ignore_error :opt_flag
    unless has_verbose goto L1
    unless verbose goto L1
    say cmd
  L1:
    $I0 = spawnw cmd
    unless $I0 goto L2
    unless has_ignore_error goto L3
    if ignore_error goto L2
  L3:
    $S0 = "exit status: "
    $S1 = $I0
    $S0 .= $S1
    $S0 .= "\ncommand: "
    $S0 .= cmd
    $S0 .= "\n"
    die $S0
  L2:
    .return ($I0)
.end

.include 'stat.pasm'

.sub 'file_exists'
    .param string filename
    $I0 = stat filename, .STAT_EXISTS
    .return ($I0)
.end

=item newer

=cut

.sub 'newer' :multi(string, pmc)
    .param string target
    .param pmc depends
    $I0 = does depends, 'array'
    if $I0 goto L0
    $S0 = depends
    .tailcall newer(target, $S0)
  L0:
    $I0 = stat target, .STAT_EXISTS
    if $I0 goto L1
    .return (0)
  L1:
    $I0 = stat target, .STAT_MODIFYTIME
    $P0 = iter depends
  L2:
    unless $P0 goto L3
    $S0 = shift $P0
    if $S0 == '' goto L2
    $I1 = stat $S0, .STAT_MODIFYTIME
    if $I1 < $I0 goto L2
    .return (0)
  L3:
    .return (1)
.end

.sub 'newer' :multi(string, string)
    .param string target
    .param string depend
    $I0 = stat target, .STAT_EXISTS
    if $I0 goto L1
    .return (0)
  L1:
    $I1 = stat target, .STAT_MODIFYTIME
    $I2 = stat depend, .STAT_MODIFYTIME
    $I0 = $I1 > $I2
    .return ($I0)
.end

=item mkpath

=cut

.sub 'mkpath'
    .param string pathname
    .param int verbose          :named('verbose') :optional
    $I1 = 1
  L1:
    $I1 = index pathname, '/', $I1
    if $I1 < 0 goto L2
    $S0 = substr pathname, 0, $I1
    inc $I1
    $I0 = stat $S0, .STAT_EXISTS
    if $I0 goto L1
    mkdir($S0, verbose :named('verbose'))
    goto L1
  L2:
    $I0 = stat pathname, .STAT_EXISTS
    if $I0 goto L3
    mkdir(pathname, verbose :named('verbose'))
  L3:
.end

=item mkdir

=cut

.sub 'mkdir'
    .param string dirname
    .param int verbose          :named('verbose') :optional
    .param int has_verbose      :opt_flag
    unless has_verbose goto L1
    unless verbose goto L1
    print "mkdir "
    say dirname
  L1:
    $P0 = new 'OS'
    $I1 = 0o775
    push_eh _handler
    $P0.'mkdir'(dirname, $I1)
    pop_eh
    .return ()
  _handler:
    .local pmc e
    .get_results (e)
    $S0 = "Can't mkdir '"
    $S0 .= dirname
    $S0 .= "' ("
    $S1 = err
    $S0 .= $S1
    $S0 .= ")\n"
    e = $S0
    rethrow e
.end

=item install

=cut

.sub 'install'
    .param string src
    .param string dst
    .param int exe              :named('exe') :optional
    .param int has_exe          :opt_flag
    .param int verbose          :named('verbose') :optional
    # mkpath
    $I1 = 1
  L1:
    $I1 = index dst, '/', $I1
    if $I1 < 0 goto L2
    $S0 = substr dst, 0, $I1
    inc $I1
    $I0 = stat $S0, .STAT_EXISTS
    if $I0 goto L1
    mkdir($S0, verbose :named('verbose'))
    goto L1
  L2:
    $I0 = newer(dst, src)
    if $I0 goto L3
    cp(src, dst, verbose :named('verbose'))
    unless has_exe goto L3
    unless exe goto L3
    $P0 = getinterp
    $P0 = $P0[.IGLOBALS_CONFIG_HASH]
    $I0 = $P0['cygwin']
    if $I0 goto L4
    $I0 = $P0['hpux']
    if $I0 goto L4
    goto L3
  L4:
    chmod(dst, 0o755, verbose :named('verbose'))
  L3:
.end

=item cp

=cut

.sub 'cp'
    .param string src
    .param string dst
    .param int verbose          :named('verbose') :optional
    .param int has_verbose      :opt_flag
    unless has_verbose goto L1
    unless verbose goto L1
    print "cp "
    print src
    print " "
    say dst
  L1:
    $P0 = new 'FileHandle'
    push_eh _handler1
    $S0 = $P0.'readall'(src)
    pop_eh
    push_eh _handler2
    $P0.'open'(dst, 'w')
    pop_eh
    $P0.'puts'($S0)
    $P0.'close'()
    .return ()
  _handler1:
    .local pmc e
    .get_results (e)
    $S0 = "Can't open '"
    $S0 .= src
    $S0 .= "' ("
    $S1 = err
    $S0 .= $S1
    $S0 .= ")\n"
    e = $S0
    rethrow e
  _handler2:
    .local pmc e
    .get_results (e)
    $S0 = "Can't open '"
    $S0 .= dst
    $S0 .= "' ("
    $S1 = err
    $S0 .= $S1
    $S0 .= ")\n"
    e = $S0
    rethrow e
.end

=item chmod

=cut

.sub 'chmod'
    .param string filename
    .param int mode
    .param int verbose          :named('verbose') :optional
    # see TT #1322
    $P0 = get_config()
    .local string cmd
    cmd = $P0['perl']
    cmd .= " -MExtUtils::Command -e ExtUtils::Command::chmod "
    $P1 = new 'FixedIntegerArray'
    set $P1, 1
    $P1[0] = mode
    $S0 = sprintf '0%o', $P1
    cmd .= $S0
    cmd .= " "
    cmd .= filename
    system(cmd, verbose :named('verbose'))
.end

=item unlink

=cut

.sub 'unlink' :multi(string)
    .param string filename
    .param int verbose          :named('verbose') :optional
    .param int has_verbose      :opt_flag
    $I0 = stat filename, .STAT_EXISTS
    unless $I0 goto L1
    $I0 = stat filename, .STAT_ISREG
    unless $I0 goto L1
    unless has_verbose goto L2
    unless verbose goto L2
    print "unlink "
    say filename
  L2:
    new $P0, 'OS'
    push_eh _handler
    $P0.'rm'(filename)
    pop_eh
  L1:
    .return ()
  _handler:
    .local pmc e
    .get_results (e)
    $S0 = "Can't remove '"
    $S0 .= filename
    $S0 .= "' ("
    $S1 = err
    $S0 .= $S1
    $S0 .= ")\n"
    e = $S0
    rethrow e
.end

.sub 'unlink' :multi(pmc)
    .param pmc list
    .param int verbose          :named('verbose') :optional
    $I0 = does list, 'array'
    if $I0 goto L1
    $S0 = list
    unlink($S0, verbose :named('verbose'))
    goto L2
  L1:
    $P0 = iter list
  L3:
    unless $P0 goto L2
    $S0 = shift $P0
    unlink($S0, verbose :named('verbose'))
    goto L3
  L2:
.end

=item rmtree

=cut

.sub 'rmtree'
    .param string path
    .param int verbose          :named('verbose') :optional
    .param int has_verbose      :opt_flag
    $I0 = stat path, .STAT_EXISTS
    unless $I0 goto L1
    $I0 = stat path, .STAT_ISDIR
    unless $I0 goto L1
    unless has_verbose goto L2
    unless verbose goto L2
    print "rmtree "
    say path
  L2:
    new $P0, 'OS'
    $P1 = $P0.'readdir'(path)
    push_eh _handler
  L3:
    unless $P1 goto L4
    $S0 = shift $P1
    if $S0 == '.' goto L3
    if $S0 == '..' goto L3
    $S1 = path . '/'
    $S1 .= $S0
    $I0 = stat $S1, .STAT_ISDIR
    unless $I0 goto L5
    rmtree($S1)
    goto L3
  L5:
    $P0.'rm'($S1)
    goto L3
  L4:
    push_eh _handler
    $S1 = path
    $P0.'rm'($S1)
    pop_eh
  L1:
    .return ()
  _handler:
    .local pmc e
    .get_results (e)
    $S0 = "Can't remove '"
    $S0 .= $S1
    $S0 .= "' ("
    $S1 = err
    $S0 .= $S1
    $S0 .= ")\n"
    e = $S0
    rethrow e
.end

=item basename

=cut

.sub 'basename'
    .param string path
    $I0 = 0
  L1:
    $I1 = index path, '/', $I0
    if $I1 < 0 goto L2
    $I0 = $I1 + 1
    goto L1
  L2:
    $S0 = substr path, $I0
    .return ($S0)
.end

=item dirname

=cut

.sub 'dirname'
    .param string path
    unless path goto L3
    $I0 = 0
  L1:
    $I1 = index path, '/', $I0
    if $I1 < 0 goto L2
    $I0 = $I1 + 1
    goto L1
  L2:
    dec $I0
    unless $I0 > 0 goto L3
    $S0 = substr path, 0, $I0
    .return ($S0)
  L3:
    .return ('.')
.end

=item cwd

=cut

.sub 'cwd'
    new $P0, 'OS'
    $S0 = $P0.'cwd'()
    $P0 = split "\\", $S0
    $S0 = join "/", $P0
    .return ($S0)
.end

=item chdir

=cut

.sub 'chdir'
    .param string dirname
    .param int verbose          :named('verbose') :optional
    .param int has_verbose      :opt_flag
    unless has_verbose goto L1
    unless verbose goto L1
    print "cd "
    say dirname
  L1:
    new $P0, 'OS'
    push_eh _handler
    $P0.'chdir'(dirname)
    pop_eh
    .return ()
  _handler:
    .local pmc e
    .get_results (e)
    $S0 = "Can't chdir '"
    $S0 .= dirname
    $S0 .= "' ("
    $S1 = err
    $S0 .= $S1
    $S0 .= ")\n"
    e = $S0
    rethrow e
.end

=item chomp

=cut

.include 'cclass.pasm'

.sub 'chomp'
    .param string str
    $I0 = index str, "\r"
    if $I0 < 0 goto L1
    str = substr str, 0, $I0
  L1:
    $I1 = index str, "\n"
    if $I1 < 0 goto L2
    str = substr str, 0, $I1
  L2:
    .return (str)
.end

=item glob

=cut

.sub 'glob'
    .param string patterns
    $P0 = new 'ResizableStringArray'
    $P1 = split ' ', patterns
  L1:
    unless $P1 goto L2
    .local string pattern
    pattern = shift $P1
    $I0 = index pattern, '*'
    unless $I0 < 0 goto L3
    $I0 = index pattern, '?'
    unless $I0 < 0 goto L3
    $I0 = index pattern, '['
    unless $I0 < 0 goto L3
    $I0 = stat pattern, .STAT_EXISTS
    unless $I0 goto L1
    push $P0, pattern
    goto L1
  L3:
    .local pmc matcher
    load_bytecode 'PGE/Glob.pbc'
    $P2 = compreg 'PGE::Glob'
    matcher = $P2.'compile'(pattern)
    $S0 = dirname(pattern)
    $P3 = glob($S0)
    $P4 = new 'OS'
  L4:
    unless $P3 goto L1
    .local string dir
    dir = shift $P3
    $I0 = stat dir, .STAT_ISDIR
    unless $I0 goto L4
    $S0 = basename(dir)
    $P5 = $P4.'readdir'(dir)
  L5:
    unless $P5 goto L4
    $S0 = shift $P5
    if $S0 == '.' goto L5
    if $S0 == '..' goto L5
    if dir == '.' goto L6
    $S1 = dir . '/'
    $S0 = $S1 . $S0
  L6:
    $P6 = matcher($S0)
    unless $P6 goto L5
    push $P0, $S0
    goto L5
  L2:
    .return ($P0)
.end

=item getenv

=cut

.sub 'getenv'
    .param string name
    new $P0, 'Env'
    $S0 = $P0[name]
    .return ($S0)
.end

=item setenv

=cut

.sub 'setenv'
    .param string name
    .param string value
    .param int verbose          :named('verbose') :optional
    .param int has_verbose      :opt_flag
    unless has_verbose goto L1
    unless verbose goto L1
    print "setenv "
    print name
    print " = "
    say value
  L1:
    new $P0, 'Env'
    $P0[name] = value
.end

=item slurp

=cut

.sub 'slurp'
    .param string filename
    $P0 = new 'FileHandle'
    push_eh _handler
    $S0 = $P0.'readall'(filename)
    pop_eh
    .return ($S0)
  _handler:
    .local pmc e
    .get_results (e)
    $S0 = "Can't open '"
    $S0 .= filename
    $S0 .= "' ("
    $S1 = err
    $S0 .= $S1
    $S0 .= ")\n"
    e = $S0
    rethrow e
.end

=item spew

=cut

.sub 'spew'
    .param string filename
    .param string content
    .param int verbose          :named('verbose') :optional
    .param int has_verbose      :opt_flag
    unless has_verbose goto L1
    unless verbose goto L1
    print "creat "
    say filename
  L1:
    $P0 = new 'FileHandle'
    push_eh _handler
    $P0.'open'(filename, 'w')
    pop_eh
    $P0.'puts'(content)
    $P0.'close'()
    .return ()
  _handler:
    .local pmc e
    .get_results (e)
    $S0 = "Can't open '"
    $S0 .= filename
    $S0 .= "' ("
    $S1 = err
    $S0 .= $S1
    $S0 .= ")\n"
    e = $S0
    rethrow e
.end

=item append

=cut

.sub 'append'
    .param string filename
    .param string content
    .param int verbose          :named('verbose') :optional
    .param int has_verbose      :opt_flag
    unless has_verbose goto L1
    unless verbose goto L1
    print "append "
    say filename
  L1:
    $P0 = new 'FileHandle'
    push_eh _handler
    $P0.'open'(filename, 'a')
    pop_eh
    $P0.'puts'(content)
    $P0.'close'()
    .return ()
  _handler:
    .local pmc e
    .get_results (e)
    $S0 = "Can't open '"
    $S0 .= filename
    $S0 .= "' ("
    $S1 = err
    $S0 .= $S1
    $S0 .= ")\n"
    e = $S0
    rethrow e
.end

=back

=head1 AUTHOR

Francois Perrad

=cut

# Local Variables:
#   mode: pir
#   fill-column: 100
# End:
# vim: expandtab shiftwidth=4 ft=pir:
