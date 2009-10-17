# Copyright (C) 2009, Parrot Foundation.
# $Id$

=head1 NAME

config/gen/libjit.pm - LibJIT Code Generation

=head1 DESCRIPTION

Populate F<config/gen/frame_builder_libjit_h.in> and
F<configu/gen/frame_builder_libjit_c.in> with system appropriate
type information and automatically generated parrot function and
vtable jit wrappers.

=cut

package gen::libjit;

use strict;
use warnings;

use base 'Parrot::Configure::Step';

use Parrot::Configure::Utils ':gen';


sub _init {
    my $self = shift;
    my %data = (
	description => 'Generate LibJIT specific code',
	result => '',
    targets => {
        frame_builder_h => 'src/frame_builder_libjit.h',
        frame_builder_c => 'src/frame_builder_libjit.c',
    },
	templates => {
	    frame_builder_h => 'config/gen/libjit/frame_builder_libjit_h.in',
	    frame_builder_c => 'config/gen/libjit/frame_builder_libjit_c.in',
	},
        wrapped_vtables => {
            get_integer        => [ ()           => 'INTVAL' ],
            set_integer_native => [ ('INTVAL')   => 'void' ],
            get_pointer        => [ ()           => 'void_ptr' ],
            set_pointer        => [ ('void_ptr') => 'void' ],
        },
        wrapped_funcs => {
            get_nci_I => [ qw(void_ptr void_ptr sys_int) => 'INTVAL' ],
            get_nci_N => [ qw(void_ptr void_ptr sys_int) => 'FLOATVAL' ],
            get_nci_S => [ qw(void_ptr void_ptr sys_int) => 'void_ptr' ],
            get_nci_P => [ qw(void_ptr void_ptr sys_int) => 'void_ptr' ],
            get_nci_p => [ qw(void_ptr void_ptr sys_int) => 'void_ptr' ],

            set_nci_I => [ qw(void_ptr void_ptr INTVAL)   => 'void' ],
            set_nci_N => [ qw(void_ptr void_ptr FLOATVAL) => 'void' ],
            set_nci_S => [ qw(void_ptr void_ptr void_ptr) => 'void' ],
            set_nci_P => [ qw(void_ptr void_ptr void_ptr) => 'void' ],

            Parrot_str_new          => [ qw(void_ptr void_ptr UINTVAL) => 'void_ptr' ],
            Parrot_str_to_cstring   => [ qw(void_ptr void_ptr)         => 'void_ptr' ],
            Parrot_str_free_cstring => [ ('void_ptr')                  => 'void' ],

            Parrot_init_arg_nci => [ qw(void_ptr void_ptr void_ptr) => 'void_ptr' ],
            pmc_new_noinit      => [ qw(void_ptr INTVAL)            => 'void_ptr' ],
        },
    );
    return \%data;
}

sub runstep {
    my ($self, $conf) = @_;

    my ($libjit_iv, $libjit_uv) = @{
        my $iv = $conf->data->get('iv') || '';
        {
            short       => [ 'jit_type_sys_short'   , 'jit_type_sys_ushort' ],
            int         => [ 'jit_type_sys_int'     , 'jit_type_sys_uint' ],
            long        => [ 'jit_type_sys_long'    , 'jit_type_sys_ulong' ],
            'long long' => [ 'jit_type_sys_longlong', 'jit_type_sys_ulonglong' ],
        }->{$iv}
            or die "Couldn't determine libjity type for intval of type '$iv'";
    };

    my $libjit_nv = do {
        my $nv = $conf->data->get('nv') || '';
        {
            float         => 'jit_type_sys_float',
            double        => 'jit_type_sys_double',
            'long double' => 'jit_type_sys_long_double',
        }->{$nv}
            or die "Couldn't determine libjity type for floatval of type '$nv'";
    };

    $conf->data->set( libjit_iv => $libjit_iv,
		      libjit_uv => $libjit_uv,
		      libjit_nv => $libjit_nv, );

    my @vtable_wrappers   = map {gen_vtable_wrapper($self, $_)}   keys %{$self->{wrapped_vtables}};
    my @function_wrappers = map {gen_function_wrapper($self, $_)} keys %{$self->{wrapped_funcs}};

    $conf->data->set( TEMP_vtable_wrap_decls => (join "\n", map {$_->{decl}} @vtable_wrappers),
                      TEMP_vtable_wrap_defns => (join "\n", map {$_->{defn}} @vtable_wrappers),
                      TEMP_func_wrap_decls   => (join "\n", map {$_->{decl}} @function_wrappers),
                      TEMP_func_wrap_defns   => (join "\n", map {$_->{defn}} @function_wrappers) );

    foreach my $t (keys %{$self->{targets}}) {
        $conf->genfile($self->{templates}{$t}, $self->{targets}{$t});
        $conf->append_configure_log($t);
    }

    return 1;
}

sub gen_vtable_wrapper {
    my ($self, $entry_name) = @_;

    my $entry_sig = $self->{wrapped_vtables}{$entry_name};
    $_ = jit_prefix_type($_) for @$entry_sig;

    my $ret_t = pop @$entry_sig;
    my $arg_t = join ", ", @$entry_sig;

    my $n_args      = scalar @$entry_sig;
    my $arg_decls_t = join ", ", map {'jit_value_t'}     1..$n_args;
    my $arg_decls_v = join ", ", map {"jit_value_t v$_"} 1..$n_args;
    my $arg_v       = join ", ", map {"v$_"}             1..$n_args;

    my $_arg_decls_t = $n_args ? ", $arg_decls_t" : "";
    my $_arg_decls_v = $n_args ? ", $arg_decls_v" : "";
    my $_arg_t = $n_args ? ", $arg_t" : "";
    my $_arg_v = $n_args ? ", $arg_v" : "";

    return { decl => <<DECL, defn => <<DEFN };
jit_value_t
jit__vtable_$entry_name(jit_function_t, jit_value_t, jit_value_t $_arg_decls_t);
DECL
jit_value_t
jit__vtable_$entry_name(jit_function_t f, jit_value_t interp, jit_value_t self $_arg_decls_v) {
    jit_type_t sig;
    jit_value_t vtable, method;
    jit_type_t  arg_t[] = { jit_type_void_ptr, jit_type_void_ptr $_arg_t };
    jit_value_t arg_v[] = { interp, self $_arg_v };

    sig = jit_type_create_signature(jit_abi_cdecl, $ret_t, arg_t, $n_args + 2, 1);

    vtable = jit_insn_load_relative(f, self,   offsetof(PMC, vtable),         jit_type_void_ptr);
    method = jit_insn_load_relative(f, vtable, offsetof(VTABLE, $entry_name), jit_type_void_ptr);

    return jit_insn_call_indirect(f, method, sig, arg_v, $n_args + 2, 0);
}
DEFN
}

sub gen_function_wrapper {
    my ($self, $func_name) = @_;

    my $func_sig = $self->{wrapped_funcs}{$func_name};
    $_ = jit_prefix_type($_) for @$func_sig;

    my $ret_t = pop @$func_sig;
    my $arg_t = join ", ", @$func_sig;

    my $n_args      = scalar @$func_sig;
    my $arg_decls_t = join ", ",  map {'jit_value_t'}     1..$n_args;
    my $arg_decls_v = join ", ",  map {"jit_value_t v$_"} 1..$n_args;
    my $arg_v       = join ", ", map {"v$_"}             1..$n_args;

    return { decl => <<DECL, defn => <<DEFN };
jit_value_t
jit__$func_name(jit_function_t, $arg_decls_t);
DECL
jit_value_t
jit__$func_name(jit_function_t f, $arg_decls_v) {
    jit_type_t sig;
    jit_type_t  arg_t[] = { $arg_t };
    jit_value_t arg_v[] = { $arg_v };

    sig = jit_type_create_signature(jit_abi_cdecl, $ret_t, arg_t, $n_args, 1);

    return jit_insn_call_native(f, "$func_name", (void *)&$func_name, sig, arg_v, $n_args, 0);
}
DEFN
}

sub jit_prefix_type {
    my $type = shift;
    if ($type !~ /[A-Z]/) {
        return "jit_type_$_";
    } else {
        return "JIT_TYPE_$_";
    }
}

1;

# Local Variables:
#   mode: cperl
#   cperl-indent-level: 4
#   fill-column: 100
# End:
# vim: expandtab shiftwidth=4:
