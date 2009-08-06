#!/usr/bin/perl

no warnings;
use v5.10.0;
use warnings;
use strict;

use Data::Dumper;

#what about throw and rethrow?
my @call_ops   = (qw{invoke invokecc callmethod callmethodcc});
my @return_ops = (qw{yield returncc tailcall tailcallmethod});
my @prof_stack = ();
my %stats;

my ($file, $func, $line, $op_seq, $op) = ('', '', 0, 0, 0);
my ($call, $return) = (0,0);
my $prev_func = '';
my $total_time = 0;
my $func_num = 0;
my ($prev_line, $new_line, $op_num) = (0,0,0);

sub maybe_say($@) {
    #say $@;
}

while (<>) {
    if (/^F:(.*)$/) {
        $file = $1;
        maybe_say "found file $file";
        $stats{$file} = {} unless exists $stats{$file};
    }
    elsif (/S:(.*)$/) {
        $prev_func = $func;
        $func = $1;
        maybe_say "found func $func";

        $stats{$file}{$func} = {} unless exists $stats{$file}{$func};

        if ($return) {
            #pop the current func and op number off the stack
            #the func is there mainly for debugging
            unless (@prof_stack) {
                die "tried to pop off an empty stack when returning from $func";
            }
            for my $frame (@prof_stack) {
                my ($file, $func, $line, $op_num) = @{$frame};
                maybe_say "($file,$func,$line,$op_num)";
            }
            ($file, $func, $line, $op_num) = @{pop @prof_stack};
            maybe_say "popped func $func, op $op_num off the stack";
            $stats{$file}{$func}{$line}{$op_num}{func_name} = $prev_func;
        }
    }
    elsif (/^(\d+):(\d+):(\d+):(.*)$/) {
        my ($line, $op_time, $rec_depth, $op) = ($1, $2, $3, $4);

        $new_line = ($line != $prev_line);
        $call     = ($op ~~ @call_ops);
        $total_time += $op_time;
        
        $prev_line = $line;

        if ($new_line && !$return) {
            $op_num = 0;
        }
        else {
            $op_num++;
        }

        maybe_say "$func line #$line, op #$op_num is $op";

        if ($new_line) {
            $stats{$file}{$func}{$line} = {line_calls_func => 0}
                unless exists $stats{$file}{$func}{$line};
        }

        if (!exists $stats{$file}{$func}{$line}{$op_num}) {
            $stats{$file}{$func}{$line}{$op_num} = {
                time => $op_time,
                hits => 1,
                op   => $op,
            };
        }
        else {
            $stats{$file}{$func}{$line}{$op_num}{time} += $op_time;
            $stats{$file}{$func}{$line}{$op_num}{hits}++;
        }

        maybe_say "calling a func" if $call;
        maybe_say "returning from a func" if $return;

        if ($call) {

            #inject a fake op representing the function call
            $op_num++;
            if (!exists $stats{$file}{$func}{$line}{$op_num}) {
                $stats{$file}{$func}{$line}{$op_num} = {
                    time      => 0,
                    hits      => 1,
                    op        => "FUNCTION_CALL",
                    func_name => "unknown_function$func_num",
                };
                $func_num++;
            }
            else {
                $stats{$file}{$func}{$line}{$op_num}{hits}++;
            }

            maybe_say "pushed func $func, op $op_num onto the stack";
            push @prof_stack, [$file, $func, $line, $op_num];
            for my $frame (@prof_stack) {
                my ($file, $func, $line, $op_num) = @{$frame};
                maybe_say "($file,$func,$line,$op_num)";
            }
            $stats{$file}{$func}{$line}{line_calls_func} = 1;
        }
        else {
            for my $fun (@prof_stack) {
                my ($file, $func, $line, $op_seq, $op) = @$fun;
                $stats{$file}{$func}{$line}{$op_seq}{time} += $op_time;
            }
        }
        $return = ($op ~~ @return_ops);
    }
}

#print Dumper(%stats);

open(OUT_FH, ">parrot.out") or die "couldn't open parrot.out for writing";

say OUT_FH <<"HEADER";
version: 1
creator: 3.4.1-Debian
pid: 5751
cmd:  ./callgrind_test

part: 1
desc: I1 cache:
desc: D1 cache:
desc: L2 cache:
desc: Timerange: Basic block 0 - $total_time
desc: Trigger: Program termination
positions: line
events: Ir
summary: $total_time

HEADER

for $file (keys %stats) {

    say OUT_FH "fl=$file"; 

    for $func (keys %{ $stats{$file} }) {
        say OUT_FH "\nfn=$func";

        for $line (sort keys %{ $stats{$file}{$func} }) {

            if ($stats{$file}{$func}{$line}{line_calls_func}) {

                my $line_time = 0;
                my $func_op_num = 0;

                for $op_num (sort grep {$_ ne 'line_calls_func'} keys %{ $stats{$file}{$func}{$line} }) {

                    if ($stats{$file}{$func}{$line}{$op_num}{op} eq "FUNCTION_CALL") {
                        $func_op_num = $op_num;
                    }
                    else {
                        $line_time += $stats{$file}{$func}{$line}{$op_num}{time}
                            unless $stats{$file}{$func}{$line}{$op_num}{op} ~~ @call_ops;
                    }
                }
                say OUT_FH "$line $line_time";

                my $func_name = $stats{$file}{$func}{$line}{$func_op_num}{func_name};
                my $hits      = $stats{$file}{$func}{$line}{$func_op_num}{hits};
                $line_time = $stats{$file}{$func}{$line}{$func_op_num-1}{time};
                say OUT_FH "cfn=$func_name";
                say OUT_FH "calls=$hits $line_time";

                my $func_time = $stats{$file}{$func}{$line}{$func_op_num}{time};

                say OUT_FH "$line $func_time";
            }
            else {
                #aggregate all lines
                my $line_time = 0;
                for $op_num (sort grep {$_ ne 'line_calls_func'} keys %{ $stats{$file}{$func}{$line} }) {
                    $line_time += $stats{$file}{$func}{$line}{$op_num}{time};
                }
                say OUT_FH "$line $line_time";
            }
        }
    }
}

say OUT_FH "totals: $total_time";
