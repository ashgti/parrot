
use strict;

package pirVisitor;
{

    sub new {
        my $proto = shift;
        my $class = ref($proto) || $proto;
        my $self  = {};
        bless $self, $class;
        my ($fh) = @_;
        $self->{fh}       = $fh;
        $self->{prologue} = q{.namespace [ "Lua" ]
.HLL "Lua", "lua_group"

.include "languages/lua/lib/luaaux.pir"
.include "languages/lua/lib/luabasic.pir"
.include "languages/lua/lib/luacoroutine.pir"
.include "languages/lua/lib/luapackage.pir"
.include "languages/lua/lib/luastring.pir"
.include "languages/lua/lib/luatable.pir"
.include "languages/lua/lib/luamath.pir"
.include "languages/lua/lib/luaio.pir"
.include "languages/lua/lib/luaos.pir"
.include "languages/lua/lib/luadebug.pir"

.sub __start :main
#  print "start Lua\n"
  init_basic()
  init_coroutine()
  init_package()
  init_string()
  init_table()
  init_math()
  init_io()
  init_os()
  init_debug()
  
#
#  PBC loader does not support LuaFunction
#  
  
#  load_bytecode "languages/lua/lib/luabasic.pbc"
#  load_bytecode "languages/lua/lib/luacoroutine.pbc"
#  load_bytecode "languages/lua/lib/luapackage.pbc"
#  load_bytecode "languages/lua/lib/luastring.pbc"
#  load_bytecode "languages/lua/lib/luatable.pbc"
#  load_bytecode "languages/lua/lib/luamath.pbc"
#  load_bytecode "languages/lua/lib/luaio.pbc"
#  load_bytecode "languages/lua/lib/luaos.pbc"
#  load_bytecode "languages/lua/lib/luadebug.pbc"
  _main()
.end

};
        return $self;
    }

    sub visitUnaryOp {
        my $self = shift;
        my ($op) = @_;
        my $FH   = $self->{fh};
        print {$FH}
            "  $op->{result}->{symbol} = $op->{op} $op->{arg1}->{symbol}\n";
    }

    sub visitBinaryOp {
        my $self = shift;
        my ($op) = @_;
        my $FH   = $self->{fh};
        if ( $op->{result} == $op->{arg1} ) {
            print {$FH}
                "  $op->{op} $op->{result}->{symbol}, $op->{arg2}->{symbol}\n";
        }
        else {
            print {$FH}
                "  $op->{result}->{symbol} = $op->{op} $op->{arg1}->{symbol}, $op->{arg2}->{symbol}\n";
        }
    }

    sub visitRelationalOp {
        my $self = shift;
        my ($op) = @_;
        my $FH   = $self->{fh};
        print {$FH}
            "  $op->{result}->{symbol} = $op->{op} $op->{arg1}->{symbol}, $op->{arg2}->{symbol}\n";
    }

    sub visitAssignOp {
        my $self = shift;
        my ($op) = @_;
        my $FH   = $self->{fh};
        print {$FH} "  $op->{result}->{symbol} = $op->{arg1}->{symbol}\n";
    }

    sub visitKeyedGetOp {
        my $self = shift;
        my ($op) = @_;
        my $FH   = $self->{fh};
        print {$FH}
            "  $op->{result}->{symbol} = $op->{arg1}->{symbol}\[$op->{arg2}->{symbol}\]\n";
    }

    sub visitKeyedSetOp {
        my $self = shift;
        my ($op) = @_;
        my $FH   = $self->{fh};
        print {$FH}
            "  $op->{result}->{symbol}\[$op->{arg1}->{symbol}\] = $op->{arg2}->{symbol}\n";
    }

    sub visitIncrOp {
        my $self = shift;
        my ($op) = @_;
        my $FH   = $self->{fh};
        print {$FH} "  inc $op->{result}->{symbol}\n";
    }

    sub visitFindGlobalOp {
        my $self = shift;
        my ($op) = @_;
        my $FH   = $self->{fh};
        print {$FH}
            "  $op->{result}->{symbol} = find_global \"$op->{arg1}\"\n";
    }

    sub visitFindLexOp {
        my $self = shift;
        my ($op) = @_;
        my $FH   = $self->{fh};
        print {$FH}
            "  $op->{result}->{symbol} = find_lex \"$op->{arg1}->{symbol}\"\n";
    }

    sub visitStoreLexOp {
        my $self = shift;
        my ($op) = @_;
        my $FH   = $self->{fh};
        print {$FH}
            "  store_lex \"$op->{arg1}->{symbol}\", $op->{arg2}->{symbol}\n";
    }

    sub visitCloneOp {
        my $self = shift;
        my ($op) = @_;
        my $FH   = $self->{fh};
        print {$FH}
            "  $op->{result}->{symbol} = clone $op->{arg1}->{symbol}\n";
    }

    sub visitNewOp {
        my $self = shift;
        my ($op) = @_;
        my $FH   = $self->{fh};
        if ( exists $op->{arg2} ) {
            print {$FH}
                "  new $op->{result}->{symbol}, $op->{arg1}, $op->{arg2}\n";
        }
        else {
            print {$FH} "  new $op->{result}->{symbol}, $op->{arg1}\n";
        }
    }

    sub visitNewClosureOp {
        my $self = shift;
        my ($op) = @_;
        my $FH   = $self->{fh};
        print {$FH}
            "  $op->{result}->{symbol} = newclosure $op->{arg1}->{symbol}\n";
    }

    sub visitNoOp {
        my $self = shift;
        my ($op) = @_;
        my $FH   = $self->{fh};

        #    print {$FH} "  noop\n";
    }

    sub visitToBoolOp {
        my $self = shift;
        my ($op) = @_;
        my $FH   = $self->{fh};
        print {$FH} "  $op->{result}->{symbol} = $op->{arg1}->{symbol}\n";
    }

    sub visitCallOp {
        my $self = shift;
        my ($op) = @_;
        my $FH   = $self->{fh};
        print {$FH} "  ";
        if ( exists $op->{result} and scalar( @{ $op->{result} } ) ) {
            print {$FH} "(";
            my $first = 1;
            foreach ( @{ $op->{result} } ) {
                print {$FH} ", " unless ($first);
                print {$FH} "$_->{symbol}";
                if ( exists $_->{pragma} and $_->{pragma} eq 'multi' ) {
                    print {$FH} " :slurpy";
                }
                $first = 0;
            }
            print {$FH} ") = ";
        }
        print {$FH} "$op->{arg1}->{symbol}(";
        my $first = 1;
        foreach ( @{ $op->{arg2} } ) {
            print {$FH} ", " unless ($first);
            print {$FH} "$_->{symbol}";
            if ( exists $_->{pragma} and $_->{pragma} eq 'multi' ) {
                print {$FH} " :flat";
            }
            $first = 0;
        }
        print {$FH} ")\n";
    }

    sub visitCallMethOp {
        my $self = shift;
        my ($op) = @_;
        my $FH   = $self->{fh};
        print {$FH} "  ";
        if ( exists $op->{result} and scalar( @{ $op->{result} } ) ) {
            print {$FH} "(";
            my $first = 1;
            foreach ( @{ $op->{result} } ) {
                print {$FH} ", " unless ($first);
                print {$FH} "$_->{symbol}";
                if ( exists $_->{pragma} and $_->{pragma} eq 'multi' ) {
                    print {$FH} " :slurpy";
                }
                $first = 0;
            }
            print {$FH} ") = ";
        }
        my @args = @{ $op->{arg2} };
        my $obj  = shift @args;
        print {$FH} "$obj->{symbol}.$op->{arg1}(";
        my $first = 1;
        foreach (@args) {
            print {$FH} ", " unless ($first);
            print {$FH} "$_->{symbol}";
            if ( exists $_->{pragma} and $_->{pragma} eq 'multi' ) {
                print {$FH} " :flat";
            }
            $first = 0;
        }
        print {$FH} ")\n";
    }

    sub visitBranchIfOp {
        my $self = shift;
        my ($op) = @_;
        my $FH   = $self->{fh};
        if ( exists $op->{op} ) {
            print {$FH}
                "  if $op->{arg1}->{symbol} $op->{op} $op->{arg2}->{symbol} goto $op->{result}->{symbol}\n";
        }
        else {
            print {$FH}
                "  if $op->{arg1}->{symbol} goto $op->{result}->{symbol}\n";
        }
    }

    sub visitBranchUnlessOp {
        my $self = shift;
        my ($op) = @_;
        my $FH   = $self->{fh};
        if ( exists $op->{op} ) {
            print {$FH}
                "  unless $op->{arg1}->{symbol} $op->{op} $op->{arg2}->{symbol} goto $op->{result}->{symbol}\n";
        }
        else {
            print {$FH}
                "  unless $op->{arg1}->{symbol} goto $op->{result}->{symbol}\n";
        }
    }

    sub visitBranchUnlessNullOp {
        my $self = shift;
        my ($op) = @_;
        my $FH   = $self->{fh};
        print {$FH}
            "  unless_null $op->{arg1}->{symbol}, $op->{result}->{symbol}\n";
    }

    sub visitBranchOp {
        my $self = shift;
        my ($op) = @_;
        my $FH   = $self->{fh};
        print {$FH} "  goto $op->{result}->{symbol}\n";
    }

    sub visitLabelOp {
        my $self = shift;
        my ($op) = @_;
        my $FH   = $self->{fh};
        print {$FH} "\n";
        print {$FH} "$op->{arg1}->{symbol}:\n";
    }

    sub visitSubDir {
        my $self  = shift;
        my ($dir) = @_;
        my $FH    = $self->{fh};
        print {$FH} "\n";
        print {$FH} ".sub $dir->{result}->{symbol} :anon";
        if ( exists $dir->{outer} ) {
            print {$FH} " :outer($dir->{outer})";
        }
        print {$FH} "\n";
    }

    sub visitEndDir {
        my $self  = shift;
        my ($dir) = @_;
        my $FH    = $self->{fh};
        print {$FH} ".end\n";
        print {$FH} "\n";
    }

    sub visitParamDir {
        my $self  = shift;
        my ($dir) = @_;
        my $FH    = $self->{fh};
        if ( exists $dir->{pragma} ) {
            print {$FH}
                "  .param $dir->{result}->{type} $dir->{result}->{symbol} $dir->{pragma}\n";
        }
        else {
            print {$FH}
                "  .param $dir->{result}->{type} $dir->{result}->{symbol} :optional\n";
        }
    }

    sub visitReturnDir {
        my $self  = shift;
        my ($dir) = @_;
        my $FH    = $self->{fh};
        print {$FH} "  .return (";
        my $first = 1;
        foreach ( @{ $dir->{result} } ) {
            print {$FH} ", " unless ($first);
            print {$FH} "$_->{symbol}";
            if ( exists $_->{pragma} and $_->{pragma} eq 'multi' ) {
                print {$FH} " :flat";
            }
            $first = 0;
        }
        print {$FH} ")\n";
    }

    sub visitLocalDir {
        my $self  = shift;
        my ($dir) = @_;
        my $FH    = $self->{fh};
        print {$FH}
            "  .local $dir->{result}->{type} $dir->{result}->{symbol}\n";
    }

    sub visitLexDir {
        my $self  = shift;
        my ($dir) = @_;
        my $FH    = $self->{fh};
        print {$FH}
            "  .lex \"$dir->{arg1}->{symbol}\", $dir->{arg1}->{symbol}\n";
    }

    sub visitConstDir {
        my $self  = shift;
        my ($dir) = @_;
        my $FH    = $self->{fh};
        print {$FH}
            "  .const .$dir->{type} $dir->{result}->{symbol} = \"$dir->{arg1}\"\n";
    }

}
1;

