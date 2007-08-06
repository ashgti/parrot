=head1 NAME

Parrot::Pmc2c::PMCEmitter - PMC to C Code Generation

=head1 SYNOPSIS

    use Parrot::Pmc2c::PMCEmitter;

=head1 DESCRIPTION

C<Parrot::Pmc2c::PMCEmitter> is used by F<tools/build/pmc2c.pl> to generate C code from PMC files.

=head2 Functions

=over

=cut

package Parrot::Pmc2c::PMC;
use strict;
use warnings;
use Data::Dumper;
use Parrot::Pmc2c::Emitter;
use Parrot::Pmc2c::Method;
use Parrot::Pmc2c::MethodEmitter;
use Parrot::Pmc2c::UtilFunctions
    qw( gen_ret dont_edit count_newlines dynext_load_code c_code_coda );
use Text::Balanced 'extract_bracketed';
use Parrot::Pmc2c::PCCMETHOD;
use Parrot::Pmc2c::PMC::RO;
use Parrot::Pmc2c::PMC::ParrotClass;

sub prep_for_emit {
    my ( $this, $pmc, $vtable_dump ) = @_;

    $pmc->vtable($vtable_dump);
    $pmc->init();
    $pmc;
}

sub generate {
    my ( $self ) = @_;
    my $emitter = $self->{emitter} = Parrot::Pmc2c::Emitter->new( $self->filename(".c") );
    $self->generate_c_file;
    $emitter->write_to_file;

    $emitter = $self->{emitter} = Parrot::Pmc2c::Emitter->new( $self->filename(".h") );
    $self->generate_h_file;
    $emitter->write_to_file;
}


=item C<generate_c_file()>

Generates the C implementation file code for the PMC.

=cut

sub generate_c_file {
    my ( $self ) = @_;
    my $c = $self->{emitter};

    $c->emit( dont_edit( $self->filename ) );
    $c->emit( "#define PARROT_IN_EXTENSION\n" ) if ( $self->is_dynamic );
    $c->emit( $self->preamble );
    $self->gen_includes;
    $self->gen_methods;
    my $ro = $self->ro;
    if ( $ro ) {
        $ro->{emitter} = $self->{emitter};
        $ro->gen_methods;
    }
    $c->emit( $self->init_func );
    $c->emit( $self->postamble );

    return 1;
}

=item C<generate_h_file()>

Generates the C header file code for the PMC.

=cut

sub generate_h_file {
    my ( $self ) = @_;
    my $h = $self->{emitter};
    my $name = uc $self->name;

    $h->emit( dont_edit( $self->filename ) );
    $h->emit(<<"EOH");

#ifndef PARROT_PMC_${name}_H_GUARD
#define PARROT_PMC_${name}_H_GUARD

EOH

    $h->emit( "#define PARROT_IN_EXTENSION\n" ) if ( $self->is_dynamic );
    $h->emit($self->hdecls);
    $h->emit($self->{ro}->hdecls)             if ( $self->{ro} );
    $h->emit(<<"EOH");

#endif /* PARROT_PMC_${name}_H_GUARD */

EOH
    $h->emit(c_code_coda());
    return 1;
}

=item C<hdecls()>

Returns the C code function declarations for all the methods for inclusion
in the PMC's C header file.

TODO include MMD variants.

=cut

sub hdecls {
    my ( $self ) = @_;

    my $hout;
    my $name = $self->name;

    # generate decls for all vtable methods in this PMC
    foreach my $vt_method_name ( @{ $self->vtable->names } ) {
        if ( $self->implements_vtable( $vt_method_name) ) {
            $hout .= $self->get_method( $vt_method_name )->generate_headers($self);
        }
    }

    # generate decls for all nci methods in this PMC
    foreach my $method ( @{ $self->{methods} } ) {
        next if $method->is_vtable;
        $hout .= $method->generate_headers($self);
    }


    # class init decl
    $hout .= 'PARROT_DYNEXT_EXPORT ' if ( $self->is_dynamic );
    $hout .= "void Parrot_${name}_class_init(PARROT_INTERP, int, int);\n";
    $self->{hdecls} .= $hout;
    $self->{hdecls};
}

=back

=head2 Instance Methods

=over

=item C<init()>

Initializes the instance.

=cut

sub init {
    my ($self) = @_;

    $self->fixup_singleton if $self->singleton;
    #!( singleton or abstract ) everything else gets readonly version of methods too.
    $self->ro(Parrot::Pmc2c::PMC::RO->new($self)) unless ( $self->abstract or $self->singleton );
}

sub fixup_singleton {
    my ($self) = @_;

    # Since singletons are shared between interpreters, we need to make special effort to use
    # the right namespace for method lookups.
    #
    # Note that this trick won't work if the singleton inherits from something else
    # (because the MRO will still be shared).
    unless ( $self->implements_vtable('pmc_namespace')
            or $self->super_method('pmc_namespace') ne 'default' ) {
        my $body = Parrot::Pmc2c::Emitter->text("  return INTERP->vtables[SELF->vtable->base_type]->_namespace;\n");
        $self->add_method( Parrot::Pmc2c::Method->new( {
            name        => 'pmc_namespace',
            parent_name => $self->name,
            parameters  => '',
            body        => $body,
            type        => Parrot::Pmc2c::Method::VTABLE,
            mmds        => [],
            return_type => 'PMC*',
            attrs       => {},
        }));
    }
}

=item C<gen_includes()>

Returns the C C<#include> for the header file of each of the PMC's superclasses.

=cut

sub gen_includes {
    my ( $self ) = @_;
    my $c = $self->{emitter};

    $c->emit(<<"EOC");
#include "parrot/parrot.h"
#include "parrot/extend.h"
#include "parrot/dynext.h"
EOC

    $c->emit(qq{#include "pmc_fixedintegerarray.h"\n}) if ( $self->flag('need_fia_header') );

    foreach my $parent_name ( $self->name, @{ $self->parents } ) {
        $c->emit('#include "pmc_' . lc $parent_name . ".h\"\n");
    }
    foreach my $mixin_name ( @{ $self->mixins } ) {
        $c->emit('#include "pmc_' . lc $mixin_name . ".h\"\n");
    }
    $c->emit('#include "' . lc $self->name . ".str\"\n") unless $self->is_dynamic;
}


=item C<proto($type,$parameters)>

Determines the prototype (argument signature) for a method body
(see F<src/call_list>).

=cut

my %calltype = (
    "char"     => "c",
    "short"    => "s",
    "char"     => "c",
    "short"    => "s",
    "int"      => "i",
    "INTVAL"   => "I",
    "float"    => "f",
    "FLOATVAL" => "N",
    "double"   => "d",
    "STRING*"  => "S",
    "char*"    => "t",
    "PMC*"     => "P",
    "short*"   => "2",
    "int*"     => "3",
    "long*"    => "4",
    "void"     => "v",
    "void*"    => "b",
    "void**"   => "B",

    #"BIGNUM*" => "???" # RT#43731
);

sub proto {
    my ( $type, $parameters ) = @_;

    # reduce to a comma separated set of types
    $parameters =~ s/\w+(,|$)/,/g;
    $parameters =~ s/ //g;

    # type method(interp, self, parameters...)
    my $ret = $calltype{ $type or "void" };
    $ret .= "JO" . join( '', map { $calltype{$_} or "?" } split( /,/, $parameters ) );

    # RT#43733
    # scan src/call_list.txt if the generated signature is available

    # RT#43735 report errors for "?"
    # --leo

    return $ret;
}

sub pre_method_gen {
}

=item C<gen_methods()>

Returns the C code for the pmc methods.

=cut

sub gen_methods {
    my ( $self ) = @_;

    # vtable methods
    foreach my $method ( @{ $self->vtable->methods } ) {
        my $vt_method_name = $method->name;
        next if $vt_method_name eq 'class_init';
        if ( $self->implements_vtable($vt_method_name) ) {
            $self->get_method($vt_method_name)->generate_body($self);
        }
    }

    # non-vtable methods
    foreach my $method ( @{ $self->methods } ) {
        next if $method->is_vtable;
        $method->generate_body($self);
    }
}


# RT#43737 quick hack - to get MMD variants
sub get_super_mmds {
    my ( $self, $vt_method_name, $right, $mmd_prefix ) = @_;
    my @mmds;

    my $super_mmd_rights = $self->{super_mmd_rights}{$vt_method_name};
    if ($super_mmd_rights)
    {
        while ( my ($super_pmc_name, $mmd_rights) = each %{$super_mmd_rights} ) {
            for my $x ( @{ $mmd_rights} ) {
                next if $x eq "DEFAULT";
                my $right = "enum_class_$x";
                my $super_name = "Parrot_${super_pmc_name}_${vt_method_name}_$x";
                push @mmds, [ $mmd_prefix, 0, $right, $super_name ];
            }
        }
    }
    return @mmds;
}

=item C<find_mmd_methods()>

Returns three values:

The first is an arrayref of <[ mmd_number, left, right, implementation_func]>
suitable for initializing the MMD list.

The second is a arrayref listing dynamic PMCs which will need to be looked up.

The third is a list of C<[index, dynamic PMC]> pairs of right entries
in the MMD table that will need to be resolved at runtime.

=cut

sub find_mmd_methods {
    my $self      = shift;
    my $pmcname = $self->name;
    my ( @mmds, @init_mmds, %init_mmds );

    foreach my $vt_method ( @{ $self->vtable->methods } ) {
        my $vt_method_name = $vt_method->name;
        next unless $vt_method->is_mmd;


        my $implementor;
        if ( !$self->implements_vtable($vt_method_name) ) {
            my $class = $self->{super}{$vt_method_name};
            next if $class =~ /^[A-Z]/
                 or $class eq 'default'
                 or $class eq 'delegate';
            $implementor = $class;
        }
        else {
            $implementor = $pmcname;
        }

        my ( $mmd_method_name, $func, $left, $right);
        $mmd_method_name = "Parrot_${implementor}_$vt_method_name";
        $func = $vt_method->mmd_prefix;
        # dynamic PMCs need the runtime type which is passed in entry to class_init
        $left  = 0;                 # set to 'entry' below in initialization loop.
        $right = $vt_method->right;


        if ( exists $self->{super}{$vt_method_name} ) {
            push @mmds, $self->get_super_mmds( $vt_method_name, $right, $func );
        }

        push @mmds, [ $func, $left, $right, $mmd_method_name ];

        my $pmc_method = $self->get_method($vt_method_name);
        if ( $pmc_method ) {
            foreach my $mmd ( @{ $pmc_method->mmds } ) {
                my $right = $mmd->right;
                if ( $self->is_dynamic( $right ) ) {
                    $right = 0;
                    push @init_mmds, [ $#mmds + 1, $mmd->right ];
                    $init_mmds{ $mmd->right } = 1;
                }
                else {
                    $right = "enum_class_$right";
                }
                $mmd_method_name = "Parrot_" . $self->name . "_" . $mmd->name;
                push @mmds, [ $func, $left, $right, $mmd_method_name ];
            }
            #$self->{mmds} = @mmds;    # RT#43739
        }
    }
    return ( \@mmds, \@init_mmds, [ keys %init_mmds ] );
}

sub build_full_c_vt_method_name {
    my ($self, $vt_method_name) = @_;

    my $implementor;
    if ( $self->implements_vtable($vt_method_name) ) {
        return $self->get_method($vt_method_name)->
            full_method_name($self->name . $self->{variant});
    }
    elsif ( $self->{super}{$vt_method_name} ) {
        $implementor = $self->{super}{$vt_method_name};
    }
    else {
        $implementor = "default"
    }
    return "Parrot_${implementor}_$vt_method_name";
}


=item C<vtable_decl($name)>

Returns the C code for the declaration of a vtable temporary named
C<$name> with the functions for this class.

=cut

sub vtable_decl {
    my ( $self, $temp_struct_name, $enum_name ) = @_;

    # gen vtable flags
    my $vtbl_flag = 0;
    $vtbl_flag .= '|VTABLE_PMC_NEEDS_EXT'     if $self->flag('need_ext');
    $vtbl_flag .= '|VTABLE_PMC_IS_SINGLETON'  if $self->flag('singleton');
    $vtbl_flag .= '|VTABLE_IS_SHARED_FLAG'    if $self->flag('is_shared');
    $vtbl_flag .= '|VTABLE_IS_READONLY_FLAG'  if $self->flag('is_ro');
    $vtbl_flag .= '|VTABLE_HAS_READONLY_FLAG' if $self->flag('has_ro');

    my @vt_methods;
    foreach my $vt_method ( @{ $self->vtable->methods } ) {
        next if $vt_method->is_mmd;
        push @vt_methods, $self->build_full_c_vt_method_name($vt_method->name);
    }

    my $methlist = join( ",\n        ", @vt_methods );

    my $cout = <<ENDOFCODE;
    const VTABLE $temp_struct_name = {
        NULL,   /* namespace */
        $enum_name, /* base_type */
        NULL,   /* whoami */
        $vtbl_flag, /* flags */
        NULL,   /* does_str */
        NULL,   /* isa_str */
        NULL,   /* class */
        NULL,   /* mro */
        NULL,   /* ro_variant_vtable */
        $methlist
    };
ENDOFCODE
    return $cout;
}

=item C<init_func()>

Returns the C code for the PMC's initialization method, or an empty
string if the PMC has a C<no_init> flag.

=cut

sub init_func {
    my ($self) = @_;
    return "" if $self->noinit;

    my $cout = "";
    my $classname = $self->name;
    my ( $mmds, $init_mmds, $dyn_mmds ) = $self->find_mmd_methods();
    my $enum_name = $self->is_dynamic ? -1 : "enum_class_$classname";
    my $vtable_decl = $self->vtable_decl('temp_base_vtable', $enum_name);

    my $mmd_list =
        join( ",\n        ", map { "{ $_->[0], $_->[1], $_->[2], (funcptr_t) $_->[3] }" } @$mmds );
    my $isa = join( " ", $classname, @{ $self->parents } );
    $isa =~ s/\s?default$//;
    my $does = join( " ", keys( %{ $self->{flags}{does} } ) );
    my $class_init_code = "";
    $class_init_code =  $self->get_method('class_init')->body if $self->has_method('class_init');
    $class_init_code =~ s/INTERP/interp/g;
    $class_init_code =~ s/^/        /mg;  #fix indenting

    my %extra_vt;

    if ( $self->{ro} ) {
        $extra_vt{ro} = $self->{ro};
    }

    $cout .= <<"EOC";
void
Parrot_${classname}_class_init(PARROT_INTERP, int entry, int pass)
{
$vtable_decl
EOC

    for my $k ( keys %extra_vt ) {
        $cout .= $extra_vt{$k}->vtable_decl("temp_${k}_vtable", $enum_name);
    }

    my $const = ( $self->{flags}{dynpmc} ) ? " " : " const ";
    if ( scalar @$mmds ) {
        $cout .= <<"EOC";

   $const MMD_init _temp_mmd_init[] = {
        $mmd_list
    };
    /* Dynamic PMCs need the runtime type which is passed in entry to class_init. */
EOC
    }

    $cout .= <<"EOC";
    if (pass == 0) {
EOC
    $cout .= <<"EOC";
        /* create vtable - clone it - we have to set a few items */
        VTABLE *vt_clone = Parrot_clone_vtable(interp, &temp_base_vtable);
EOC
    for my $k ( keys %extra_vt ) {
        $cout .= <<"EOC";
        VTABLE *vt_${k}_clone = Parrot_clone_vtable(interp, &temp_${k}_vtable);
EOC
    }

    # init vtable slot
    if ( $self->is_dynamic ) {
        $cout .= <<"EOC";
        vt_clone->base_type = entry;
        vt_clone->whoami = string_make(interp, "$classname", @{[length($classname)]}, "ascii",
            PObj_constant_FLAG|PObj_external_FLAG);
        vt_clone->isa_str = string_make(interp, "$isa", @{[length($isa)]}, "ascii",
            PObj_constant_FLAG|PObj_external_FLAG);
        vt_clone->does_str = string_make(interp, "$does", @{[length($does)]}, "ascii",
            PObj_constant_FLAG|PObj_external_FLAG);
EOC
    }
    else {
        $cout .= <<"EOC";
        vt_clone->whoami = CONST_STRING(interp, "$classname");
        vt_clone->isa_str = CONST_STRING(interp, "$isa");
        vt_clone->does_str = CONST_STRING(interp, "$does");
EOC
    }
    for my $k ( keys %extra_vt ) {
        $cout .= <<"EOC";
        vt_${k}_clone->base_type = entry;
        vt_${k}_clone->whoami = vt_clone->whoami;
        vt_${k}_clone->isa_str = vt_clone->isa_str;
        vt_${k}_clone->does_str = vt_clone->does_str;
EOC
    }

    if ( $extra_vt{ro} ) {
        $cout .= <<"EOC";
        vt_clone->ro_variant_vtable = vt_ro_clone;
        vt_ro_clone->ro_variant_vtable = vt_clone;
EOC
    }

    $cout .= <<"EOC";
        interp->vtables[entry] = vt_clone;
EOC
    $cout .= <<"EOC";
    }
    else { /* pass */
EOC

    # To make use of the .HLL directive, register any mapping...
    if ( $self->{flags}{hll} && $self->{flags}{maps} ) {

        my $hll  = $self->{flags}{hll};
        my $maps = ( keys %{ $self->{flags}{maps} } )[0];
        $cout .= <<"EOC";

        {
            /* Register this PMC as a HLL mapping */
            INTVAL pmc_id = Parrot_get_HLL_id( interp, const_string(interp, "$hll")
            );
            if (pmc_id > 0)
                Parrot_register_HLL_type( interp, pmc_id, enum_class_$maps, entry);
        } /* Register */
EOC
    }

    $cout .= <<"EOC";
        /* setup MRO and _namespace */
        Parrot_create_mro(interp, entry);
        /* create PMC Proxy object */
        Parrot_create_pmc_proxy(interp, entry);
EOC

    # declare each nci method for this class
    foreach my $method ( @{ $self->{methods} } ) {
        next unless $method->type eq Parrot::Pmc2c::Method::NON_VTABLE;
        my $proto = proto( $method->return_type, $method->parameters );
        my $method_name = $method->name;
        my $symbol_name = defined $method->symbol ? $method->symbol : $method->name;
        if ( exists $method->{PCCMETHOD} ) {
            $cout .= <<"EOC";
        register_raw_nci_method_in_ns(interp, entry,
            F2DPTR(Parrot_${classname}_${method_name}), "$symbol_name");
EOC
        }
        else {
            $cout .= <<"EOC";
        register_nci_method(interp, entry,
                F2DPTR(Parrot_${classname}_${method_name}), "$symbol_name", "$proto");
EOC
        }
        if ( $method->{attrs}{write} ) {
            $cout .= <<"EOC";
        Parrot_mark_method_writes(interp, entry, "$symbol_name");
EOC
        }
    }

    # include any class specific init code from the .pmc file
    $cout .= <<"EOC";
        /* class_init */
EOC
    $cout .= <<"EOC" if $class_init_code;
        {
$class_init_code
        }
EOC

    $cout .= <<"EOC";
        {
EOC

    # declare auxiliary variables for dyncpmc IDs
    foreach my $dynpmc (@$dyn_mmds) {
        next if $dynpmc eq $classname;
        $cout .= <<"EOC";
            int my_enum_class_$dynpmc = pmc_type(interp, string_from_literal(interp, "$dynpmc"));
EOC
    }

    # init MMD "right" slots with the dynpmc types
    foreach my $entry (@$init_mmds) {
        if ( $entry->[1] eq $classname ) {
            $cout .= <<"EOC";
            _temp_mmd_init[$entry->[0]].right = entry;
EOC
        }
        else {
            $cout .= <<"EOC";
            _temp_mmd_init[$entry->[0]].right = my_enum_class_$entry->[1];
EOC
        }
    }

    # just to be safe
    foreach my $dynpmc (@$dyn_mmds) {
        next if $dynpmc eq $classname;
        $cout .= <<"EOC";
            PARROT_ASSERT(my_enum_class_$dynpmc != enum_class_default);
EOC
    }
    if ( scalar @$mmds ) {
        $cout .= <<"EOC";
#define N_MMD_INIT (sizeof(_temp_mmd_init)/sizeof(_temp_mmd_init[0]))
            Parrot_mmd_register_table(interp, entry,
                _temp_mmd_init, N_MMD_INIT);
EOC
    }

    $cout .= <<"EOC";
        }
    } /* pass */
} /* Parrot_${classname}_class_init */
EOC
    if ( $self->is_dynamic) {
        $cout .= dynext_load_code( $classname, $classname => {} );
    }

    $cout;
}

sub is_vtable_method {
    my ( $self, $vt_method_name ) = @_;
    return 1 if $self->vtable->has_method($vt_method_name);
    return 0;
}

sub vtable {
    my ( $self, $value) = @_;
    $self->{vtable} = $value if $value;
    return $self->{vtable};
}

1;

# Local Variables:
#   mode: cperl
#   cperl-indent-level: 4
#   fill-column: 100
# End:
# vim: expandtab shiftwidth=4:

