#! nqp
# Copyright (C) 2010, Parrot Foundation.
# $Id$

class Ops::Emitter is Hash;

=begin

Emitter.

=end

method new(:$ops_file!, :$trans!, :$script!, :$file, :%flags!) {
    self<ops_file>  := $ops_file;
    self<trans>     := $trans;
    self<script>    := $script;
    self<file>      := $file;
    self<flags>     := %flags;

    # Preparing various bits.
    my $suffix := $trans.suffix();

    my $base := %flags<core> ?? 'core' !! $file; #FIXME drop .ops
    my $base_ops_stub := $base ~ '_ops' ~ $suffix;
    my $base_ops_h    := $base_ops_stub ~ '.h';

    self<base>    := $base;
    self<suffix>  := $suffix;
    self<bs>      := $base ~ $suffix ~ '_';

    self<include> := "parrot/oplib/$base_ops_h";
    self<header>  := (~%flags<dir>) ~ "include/" ~ self<include>;
    self<source>  := (~%flags<dir>) ~ "src/ops/$base_ops_stub.c";

    self<sym_export> := %flags<dynamic>
                        ?? 'PARROT_DYNEXT_EXPORT'
                        !! '';

    self<init_func>  := join('_',
        'Parrot', 'DynOp', $base ~ $suffix, |$ops_file.version );

    # Prepare ops
    $trans.prepare_ops(self, $ops_file);

    self;
};

method ops_file()   { self<ops_file> };
method trans()      { self<trans> };
method script()     { self<script> };
method file()       { self<file> };
method flags()      { self<flags> };
method sym_export() { self<sym_export> };
method init_func()  { self<init_func> };

method base()       { self<base> };
method suffix()     { self<suffix> };
method bs()         { self<bs> };

method print_c_header_file() {
    my $fh := pir::open__PSs(self<header>, 'w') || die("Can't open filehandle");
    self.emit_c_header_file($fh);
    $fh.close();
    return self<header>;
}

method emit_c_header_file($fh) {

    self._emit_guard_prefix($fh);

    self._emit_preamble($fh);

    # Emit runcore specific part.
    self.trans.emit_c_header_part($fh);

    self._emit_guard_suffix($fh);

    self._emit_coda($fh);
}

method print_c_source_file() {
    # Build file in memeory
    my $fh := pir::new__Ps('StringHandle');
    $fh.open('dummy.c', 'w');
    self.emit_c_source_file($fh);
    $fh.close();

    # ... and write it to disk
    my $final := pir::open__PSs(self<source>, 'w') || die("Can't open filehandle");
    $final.print($fh.readall());
    $final.close();
    return self<source>;
}

method emit_c_source_file($fh) {
    self._emit_source_preamble($fh);
    self._emit_op_lib_descriptor($fh);

    self.trans.emit_source_part(self, $fh);
    self.trans.emit_op_lookup(self, $fh);

    self._emit_init_func($fh);
    self._emit_dymanic_lib_load($fh);
    self._emit_coda($fh);
}

method _emit_source_preamble($fh) {

    self._emit_preamble($fh);
    $fh.print(qq|
#include "{self<include>}"
#include "pmc/pmc_parrotlibrary.h"
#include "pmc/pmc_callcontext.h"

{self.trans.defines(self)}

|);

    $fh.print(self.ops_file.preamble);
}

method _emit_op_lib_descriptor($fh) {

    my $core_type := self.trans.core_type;
    $fh.print(q|
/*
** op lib descriptor:
*/

static op_lib_t | ~ self.bs ~ q|op_lib = {| ~ qq|
  "{self.base}",               /* name */
  "{self.suffix}",             /* suffix */
  $core_type,                       /* core_type = PARROT_XX_CORE */
  0,                                /* flags */
  {self.ops_file.version_major},    /* major_version */
  {self.ops_file.version_minor},    /* minor_version */
  {self.ops_file.version_patch},    /* patch_version */
  {+self.ops_file.ops},             /* op_count */
  {self.trans.op_info(self)},       /* op_info_table */
  {self.trans.op_func(self)},       /* op_func_table */
  {self.trans.getop(self)}          /* op_code() */ | ~ q|
};
|);
}

method _emit_init_func($fh) {

    my $init1    := self.trans.init_func_init1;
    my $dispatch := self.trans.init_func_disaptch;

    # TODO There is a bug in NQP about \{
    $fh.print(q|
op_lib_t *
| ~ self.init_func ~ q|(PARROT_INTERP, long init) {
    /* initialize and return op_lib ptr */
    if (init == 1) {
| ~ $init1 ~ q|
        return &| ~ self.bs ~q|op_lib;
    }
    /* set op_lib to the passed ptr (in init) */
    else if (init) {
| ~ $dispatch ~ q|
    }
    /* deinit - free resources */
    else {
        hop_deinit(interp);
    }
    return NULL;
}

|);
}

method _emit_dymanic_lib_load($fh) {

    if ! self.flags<dynamic> {
        return;
    }

    my $load_func := join('_',
            q{Parrot}, q{lib}, self.base, q{ops} ~ self.suffix, q{load}, );
    $fh.print(qq|
/*
 * dynamic lib load function - called once
 */
{self.sym_export} PMC*
$load_func(PARROT_INTERP);

{self.sym_export} PMC*
$load_func(PARROT_INTERP)
| ~ q|
{
    PMC *const lib = Parrot_pmc_new(interp, enum_class_ParrotLibrary);
    ((Parrot_ParrotLibrary_attributes*)PMC_data(lib))->oplib_init = (void *) | ~ self.init_func ~q|;
    dynop_register(interp, lib);
    return lib;
}
|);
}

# given a headerfile name like "include/parrot/oplib/core_ops.h", this
# returns a string like "PARROT_OPLIB_CORE_OPS_H_GUARD"
method _generate_guard_macro_name() {
    my $fn   := self<header>;
    $fn := subst($fn, /.h$/, '');
    #my @path = File::Spec->splitdir($fn);
    my @path := split('/', $fn);
    @path.shift if @path[0]~'/' eq self<flags><dir>;
    @path.shift if @path[0] eq 'include';
    @path.shift if @path[0] eq 'parrot';
    uc( join( '_', 'parrot', |@path, 'h', 'guard' ) );
}


method _emit_guard_prefix($fh) {
    my $guardname := self._generate_guard_macro_name();
    $fh.print(qq/
#ifndef $guardname
#define $guardname

/);
}

method _emit_guard_suffix($fh) {
    my $guardname := self._generate_guard_macro_name();
    $fh.print(qq|

#endif /* $guardname */
|);
}


method _emit_coda($fh) {
    $fh.print(q|

/*
 * Local variables:
 *   c-file-style: "parrot"
 *   buffer-read-only: t
 * End:
 * vim: expandtab shiftwidth=4:
 */
|);
}

method _emit_preamble($fh) {

    $fh.print(qq|
/* ex: set ro:
 * !!!!!!!   DO NOT EDIT THIS FILE   !!!!!!!
 *
 * This file is generated automatically from '{self<file>}' (and possibly other
 * .ops files). by {self<script>}.
 *
 * Any changes made here will be lost!
 *
 */
|);

    if self.flags<dynamic> {
        $fh.print("#define PARROT_IN_EXTENSION\n");
    }

    $fh.print(qq|
#include "parrot/parrot.h"
#include "parrot/oplib.h"

{self.sym_export} op_lib_t *{self.init_func}(PARROT_INTERP, long init);

|);
}

# vim: expandtab shiftwidth=4 ft=perl6:
