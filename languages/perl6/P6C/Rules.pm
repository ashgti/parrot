package P6C::Rules;

=head1 B<P6C::Rules>

Implementation of rule definitions and rule calls.

=cut

use vars qw($DEFAULT_RULE_SIGNATURE $DEFAULT_RULE_ARGUMENT_CONTEXT);

sub _parse_default_sig {
    ($DEFAULT_RULE_SIGNATURE, $DEFAULT_RULE_ARGUMENT_CONTEXT) =
      P6C::Parser::parse_sig(<<'END', no_named => 1);
int $rx_mode,
IntList @rx_stack,
str $rx_input,
int $rx_pos,
PerlArray *@_
END
}

sub default_arg_context {
    _parse_default_sig() unless defined $DEFAULT_RULE_ARGUMENT_CONTEXT;
    return $DEFAULT_RULE_ARGUMENT_CONTEXT;
}

sub default_return_context {
    return new P6C::Context type => ['int','int'];
}

sub default_signature {
    _parse_default_sig() unless defined $DEFAULT_RULE_ARGUMENT_CONTEXT;
    return $DEFAULT_RULE_SIGNATURE;
}

# Regex rules always take four additional parameters for tracking the
# regex engine's state.
sub adjust_rule {
    my ($func) = @_;

    return if $ENV{ORIGINAL_REGEXES};

    if (! defined($func->params)) {
        $func->params(new P6C::signature);
    }

    my $mk_param = sub {
        my ($name, $type) = @_;
        return new P6C::sigparam type => $type,
                                 zone => 'required_positional',
                                 var => new P6C::variable type => $type,
                                                          name => $name;
    };

    unshift @{ $func->params->positional },
      ($mk_param->('=mode', 'int'),
       $mk_param->('=rx_stack', 'IntList'),
       $mk_param->('=rx_input', 'str'),
       $mk_param->('=rx_pos', 'int'));
}

sub rule_vars {
    # FIXME! This is copied from adjust_rule, except only variables
    # are created.
    my $mk_param = sub {
        my ($name, $type) = @_;
        return new P6C::variable type => $type, name => $name;
    };

    return ($mk_param->('=mode', 'int'),
            $mk_param->('=rx_stack', 'IntList'),
            $mk_param->('=rx_input', 'str'),
            $mk_param->('=rx_pos', 'int'));
}

sub default_args {
    my $args = new P6C::ValueList
      vals => [ new P6C::variable(name => '$rx_mode', type => 'int'),
                new P6C::variable(name => '@rx_stack', type => 'IntList'),
                new P6C::variable(name => '$rx_input', type => 'str'),
                new P6C::variable(name => '$rx_pos', type => 'int'),
                new P6C::variable(name => '@_', type => 'PerlArray')
              ];
}

sub adjust_call {
    my ($call) = @_;
    return if $ENV{ORIGINAL_REGEXES};
    my $args = $call->args;

    my $LIT = 'P6C::IMCC::ExtRegex::literal';
    my @argvals = ( ($LIT->new(type => 'int')), # mode
                    ($LIT->new(type => 'IntList')), # stack -- HACK!! FIXME!!
                    ($LIT->new(type => 'str')), # input
                    ($LIT->new(type => 'int')) ); # pos
    $DB::single = 1 unless $args->isa('P6C::ValueList');
    die unless $args->isa('P6C::ValueList');
#    if ($args->isa('P6C::ValueList')) {
        unshift @{ $args->vals }, @argvals;
#    } else {
#        $call->args(new P6C::ValueList vals => [ @argvals, $args ]);
#    }
}

# Rules return their status result and the updated rx_pos
sub adjust_rule_return {
    my ($func) = @_;
    return if $func->{rx_hasparams};
    $func->{rx_hasparams} = 1;
    P6C::IMCC::set_function_return([ 'int', 'int' ]);
}

1;
