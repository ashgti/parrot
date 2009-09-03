#!/usr/bin/perl

# Copyright (C) 2009, Parrot Foundation.
# $Id$


no warnings;
use v5.10.0;
use warnings;
use strict;

use Data::Dumper;

=head1 NAME

tools/dev/pprof2cg.pl

=head1 DESCRIPTION

Convert the output of Parrot's profiling runcore to a Callgrind-compatible
format.

=head1 USAGE

Generate a profile by passing C<-Rprofiling> to parrot, for example C<./parrot
-Rprofiling perl6.pbc hello.p6>.  Once execution completes, parrot will print a
message specifying the location of profile.  The profile will usually be named
parrot.pprof.XXXX, where XXXX is the PID of the parrot process.

To generate a Callgrind-compatible profile, run this script with the pprof
filename as the first argument.  The output file will be in parrot.out.XXXX,
where XXXX again is the PID of the original parrot process.

=cut


my @ctx_stack = ();

main(\@ARGV);

sub main {
    my $argv = shift;
    my $stats = {};
    my $filename = $argv->[0];
    $stats->{'global_stats'}{'filename'} = $filename;
    open FH, "<$filename" or die "couldn't open $filename for reading";
    while (<FH>) {
        my $line = $_;
        process_line($line, $stats);
    }
    #print_stats($stats);
    write_cg_profile($stats);
}


sub process_line {

    my $line  = shift;
    my $stats = shift;

    for ($line) {
        if (/^#/) {
            #comments are always ignored
        }
        elsif (/^VERSION:(\d+)$/) {
            my $version = $1;
            if ($version != 1) {
                die "profile was generated by an incompatible version of the profiling runcore.";
            }
        }
        elsif (/^CLI:(.*)$/) {
            $stats->{'global_stats'}{'cli'} = $1;
        }
        #context switch
        elsif (/^CS:(.*)$/) {

            my $cs_hash      = split_vars($1);
            my $is_first     = $#ctx_stack == -1;
            my $is_redundant = !$is_first && ($ctx_stack[0]{'ctx'} eq $cs_hash->{'ctx'});
            my $is_call      = !scalar(grep {$_->{'ctx'} eq $cs_hash->{'ctx'}} @ctx_stack);

            if ($is_first) {
                #KCachegrind starts on the "main" function
                $cs_hash->{'ns'} = 'main';
                $ctx_stack[0] = $cs_hash;
            }
            elsif ($is_redundant) {
                #don't do anything
            }
            elsif ($is_call) {
                $ctx_stack[0]{'op_num'}++;
                my $extra = {
                    op_name => "CALL",
                    target  => $cs_hash->{'ns'}
                };
                store_stats($stats, $ctx_stack[0], 0, $extra );
            unshift @ctx_stack, $cs_hash;
            }
            else {
                shift @ctx_stack while ($ctx_stack[0]->{'ctx'} ne $cs_hash->{'ctx'});
            }
            #print Dumper(\@ctx_stack);
        }
        elsif (/^OP:(.*)$/) {
            my $op_hash = split_vars($1);

            if (exists $ctx_stack[0]{'line'} && $op_hash->{'line'} == $ctx_stack[0]{'line'}) {
                $ctx_stack[0]{'op_num'}++;
            }
            else {
                $ctx_stack[0]{'op_num'} = 0;
            }

            $ctx_stack[0]{'line'} = $op_hash->{'line'};
            my $extra = { op_name => $op_hash->{'op'} };
            store_stats($stats, $ctx_stack[0], $op_hash->{'time'}, $extra);

            $extra->{'no_hits'} = 1;
            for my $frame (@ctx_stack[1 .. $#ctx_stack]) {
                store_stats($stats, $frame, $op_hash->{'time'}, $extra);
            }
        }
    }
}

sub print_stats {
    my $stats = shift;

    for my $file (grep {$_ ne 'global_stats'} sort keys %$stats) {
        for my $ns (sort keys %{ $stats->{$file} }) {
            for my $line_num (sort {$a<=>$b} keys %{ $stats->{$file}{$ns} }) {
                for my $op_numbr (0 .. $#{$stats->{$file}{$ns}{$line_num}}) {

                    print "$file  $ns  line:$line_num  op:$op_numbr ";

                    for my $attr (sort keys %{ $stats->{$file}{$ns}{$line_num}[$op_numbr] }) {
                        print "{ $attr => $stats->{$file}{$ns}{$line_num}[$op_numbr]{$attr} } ";
                    }
                    say "";
                }
            }
            say "";
        }
    }
}

sub split_vars{
    my $href;
    my $str = shift;
    while ($str =~ /\G { ([^:]+) : (.*?) } /cxg) {
        $href->{$1} = $2;
    }
    return $href;
}

sub store_stats {
    my $stats = shift;
    my $locator   = shift;
    my $time      = shift;
    my $extra     = shift;

    my $file   = $locator->{'file'};
    my $ns     = $locator->{'ns'};
    my $line   = $locator->{'line'};
    my $op_num = $locator->{'op_num'};

    if (exists $stats->{'global_stats'}{'total_time'}) {
        $stats->{'global_stats'}{'total_time'} += $time;
    }
    else {
        $stats->{'global_stats'}{'total_time'} = $time;
    }

    if (exists $stats->{$file}{$ns}{$line}[$op_num]) {
        $stats->{$file}{$ns}{$line}[$op_num]{'hits'}++
            unless exists $extra->{no_hits};
        $stats->{$file}{$ns}{$line}[$op_num]{'time'} += $time;
    }
    else {
        $stats->{$file}{$ns}{$line}[$op_num]{'hits'} = 1;
        $stats->{$file}{$ns}{$line}[$op_num]{'time'} = $time;
        for my $key (keys %{$extra}) {
            $stats->{$file}{$ns}{$line}[$op_num]{$key} = $extra->{$key};
        }
    }
}


sub write_cg_profile {

    my $stats = shift;
    my $filename = $stats->{'global_stats'}{'filename'};
    $filename =~ s/\.pprof\./.out./;

    open(OUT_FH, ">$filename") or die "couldn't open parrot.out for writing";

    say OUT_FH <<"HEADER";
version: 1
creator: 3.4.1-Debian
pid: 5751
cmd: $stats->{'global_stats'}{'cli'}

part: 1
desc: I1 cache:
desc: D1 cache:
desc: L2 cache:
desc: Timerange: Basic block 0 - $stats->{'global_stats'}{'total_time'}
desc: Trigger: Program termination
positions: line
events: Ir
summary: $stats->{'global_stats'}{'total_time'}

HEADER

    for my $file (grep {$_ ne 'global_stats'} keys %$stats) {

        say OUT_FH "fl=$file";

        for my $ns (keys %{ $stats->{$file} }) {
            say OUT_FH "\nfn=$ns";

            for my $line (sort keys %{ $stats->{$file}{$ns} }) {

                my $curr_op  = 0;
                my $op_count = scalar(@{$stats->{$file}{$ns}{$line}});
                my $op_time  = 0;

                while ($curr_op < $op_count && $stats->{$file}{$ns}{$line}[$curr_op]{'op_name'} ne 'CALL') {
                    $op_time += $stats->{$file}{$ns}{$line}[$curr_op]{'time'};
                    $curr_op++;
                }
                say OUT_FH "$line $op_time";

                if ($curr_op < $op_count && $stats->{$file}{$ns}{$line}[$curr_op]{'op_name'} eq 'CALL') {
                    my $call_target = $stats->{$file}{$ns}{$line}[$curr_op]{'target'};
                    my $call_count  = $stats->{$file}{$ns}{$line}[$curr_op]{'hits'};
                    my $call_cost   = $stats->{$file}{$ns}{$line}[$curr_op]{'time'};

                    say OUT_FH "cfn=$call_target";
                    say OUT_FH "calls=$call_count $call_cost";
                }

                if ($curr_op < $op_count) {
                    $op_time = 0;
                    while ($curr_op < $op_count) {
                        $op_time += $stats->{$file}{$ns}{$line}[$curr_op]{'time'};
                        $curr_op++;
                    }
                    say OUT_FH "$line $op_time";
                }
            }
        }
    }

    say OUT_FH "totals: $stats->{'global_stats'}{'total_time'}";
    close OUT_FH;
}
