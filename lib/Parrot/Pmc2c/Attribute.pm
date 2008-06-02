# Copyright (C) 2008, The Perl Foundation.
# $Id$

package Parrot::Pmc2c::Attribute;

use strict;
use warnings;

sub new {
    my ( $class, $self_hash ) = @_;
    my $self = {
        (
            name      => "",
            type      => "",
            modifiers => [],
            %{ $self_hash || {} }
        )
    };

    bless $self, $class;
}

# getters/setters
sub name {
    my ( $self, $value ) = @_;
    $self->{name}        = $value if $value;
    return $self->{name};
}

=head1 C<generate_declaration>

Generate and emit the C code for an attribute declaration.

=cut

sub generate_declaration {
    my ( $self, $pmc ) = @_;
    my $h              = $pmc->{emitter};
    my $decl           = '    ' . $self->{type} . ' ' . $self->{name} . ";\n";

    $h->emit($decl);

    return 1;
}

=head1 C<generate_start>

Generate and emit the C code for the start of an attribute struct.

=cut

sub generate_start {
    my ( $self, $pmc ) = @_;
    my $h              = $pmc->{emitter};
    my $name           = $pmc->{name};

    $h->emit(<<"EOH");

/* $name PMC's underlying struct. */
typedef struct Parrot_${name} {
EOH

    return 1;
}

=head1 C<generate_end>

Generate and emit the C code for the end of an attribute struct.

=cut

sub generate_end {
    my ( $self, $pmc ) = @_;
    my $h              = $pmc->{emitter};
    my $name           = $pmc->{name};
    my $ucname         = uc($name);

    $h->emit(<<"EOH");
} Parrot_${name};

/* Macro to access underlying structure of a $name PMC. */
#define PARROT_${ucname}(o) ((Parrot_${name} *) PMC_data(o))

EOH

    return 1;
}

=head1 C<generate_accessor>

Generate and emit the C code for an attribute get/set accessor pair.

=cut

sub generate_accessor {
    my ( $self, $pmc ) = @_;

    my $h              = $pmc->{emitter};
    my $pmcname        = $pmc->{name};
    my $attrtype       = $self->{type};
    my $attrname       = $self->{name};
    my $decl           = <<"EOA";

/* Generated macro accessors for '$attrname' attribute of $pmcname PMC. */
#define GETATTR_${pmcname}_${attrname}(interp, pmc, dest) \\
    { \\
        if (PObj_is_object_TEST(pmc)) { \\
EOA

    if ($attrtype eq "INTVAL") {
        $decl .= <<"EOA";
            PMC *attr_value = VTABLE_get_attr_str(interp, \\
                              pmc, const_string(interp, "$attrname")); \\
            (dest) = VTABLE_get_integer(interp, attr_value); \\
EOA
    }
    elsif ($attrtype eq "FLOATVAL") {
        $decl .= <<"EOA";
            PMC *attr_value = VTABLE_get_attr_str(interp, \\
                              pmc, const_string(interp, "$attrname")); \\
            (dest) = VTABLE_get_number(interp, attr_value); \\
EOA
    }
    elsif ($attrtype =~ "STRING") {
        $decl .= <<"EOA";
            PMC *attr_value = VTABLE_get_attr_str(interp, \\
                              pmc, const_string(interp, "$attrname")); \\
            (dest) = VTABLE_get_string(interp, attr_value); \\
EOA
    }
    elsif ($attrtype =~ /PMC/) {
        $decl .= <<"EOA";
            (dest) = VTABLE_get_attr_str(interp, \\
                              pmc, const_string(interp, "$attrname")); \\
EOA
    }

    else {
        $decl .= <<"EOA";
            real_exception(interp, NULL, INVALID_OPERATION, \\
                            "Attributes of type '$attrtype' cannot be " \\
                            "subclassed from a high-level PMC."); \\
EOA
    }

    $decl .= <<"EOA";
        } \\
        else \\
            (dest) = ((Parrot_${pmcname} *)PMC_data(pmc))->$attrname; \\
        }

#define SETATTR_${pmcname}_${attrname}(interp, pmc, value) \\
    { \\
        if (PObj_is_object_TEST(pmc)) { \\
EOA

    if ($attrtype eq "INTVAL") {
        $decl .= <<"EOA";
            PMC *attr_value = new_pmc(interp, enum_class_Integer); \\
            VTABLE_set_integer(interp, attr_value, value); \\
            VTABLE_set_attr_str(interp, pmc, \\
                              const_string(interp, "$attrname"), attr_value); \\
EOA
    }
    elsif ($attrtype eq "FLOATVAL") {
        $decl .= <<"EOA";
            PMC *attr_value = new_pmc(interp, enum_class_Float); \\
            VTABLE_set_number(interp, attr_value, value); \\
            VTABLE_set_attr_str(interp, pmc, \\
                              const_string(interp, "$attrname"), attr_value); \\
EOA
    }
    elsif ($attrtype =~ "STRING") {
        $decl .= <<"EOA";
            PMC *attr_value = new_pmc(interp, enum_class_String); \\
            VTABLE_set_string_native(interp, attr_value, value); \\
            VTABLE_set_attr_str(interp, pmc, \\
                              const_string(interp, "$attrname"), attr_value); \\
EOA
    }
    elsif ($attrtype =~ /PMC/) {
        $decl .= <<"EOA";
            VTABLE_set_attr_str(interp, pmc, \\
                              const_string(interp, "$attrname"), value); \\
EOA
    }

    else {
        $decl .= <<"EOA";
            real_exception(interp, NULL, INVALID_OPERATION, \\
                            "Attributes of type '$attrtype' cannot be " \\
                            "subclassed from a high-level PMC."); \\
EOA
    }

    $decl .= <<"EOA";
        } \\
        else \\
            ((Parrot_${pmcname} *)PMC_data(pmc))->$attrname = (value); \\
    }

EOA

    $h->emit($decl);

    return 1;
}


1;

# Local Variables:
#   mode: cperl
#   cperl-indent-level: 4
#   fill-column: 100
# End:
# vim: expandtab shiftwidth=4:
