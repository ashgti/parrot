#!/usr/bin/perl

# This script (optionally) runs a coverage test, then generates html reports.
# It requires gcc and gcov to be installed.

use File::Basename;
use File::Find;
use POSIX qw(strftime);

use strict;

my $SRCDIR  = "./"; # with trailing /
my $HTMLDIR = "parrot_coverage";
my $DEBUG   = 1;

if ($ARGV[0] =~ /recompile/) {

    #### clean up remnants of prior biulds
    File::Find::find({wanted => sub {
                          /\.(bb|bba|bbf|da|gcov)$/ &&
                            unlink($File::Find::name)
                        }}, $SRCDIR);

    #### build parrot with coverage support
    system("perl Configure.pl --defaults --define cc=\"gcc -fprofile-arcs -ftest-coverage\"");
    system("make");

    #### Now run the tests
    system("make test");
}

#### And generate the reports.

my @dafiles;
File::Find::find({wanted => sub {
                  /\.da$/ && push @dafiles, $File::Find::name }}, $SRCDIR);

my (%file_line_coverage, %file_branch_coverage, %file_call_coverage);
my (%function_line_coverage, %function_branch_coverage, %function_call_coverage);
my (%totals, %real_filename);

foreach my $da_file (@dafiles) {
    my $dirname   = dirname($da_file) || ".";
    my $filename  = basename($da_file);

    my $cmd = "cd $dirname; gcov -f -b $filename";
    print "Running $cmd..\n" if $DEBUG;
    open (GCOVSUMMARY, "$cmd|") || die "Error invoking '$cmd': $!";
    my $tmp;
    my %generated_files;
    while (<GCOVSUMMARY>) {
        if (/^Creating (.*)\./) {
            my $path = "$dirname/$1"; $path =~ s/\Q$SRCDIR\E//g;
            $generated_files{$path} = $tmp;
            $tmp = '';
        } else {
            $tmp .= $_;
        }
    }
    close(GCOVSUMARY);

    foreach my $gcov_file (keys %generated_files) {
        my $source_file = $gcov_file;
        $source_file =~ s/\.gcov$//g;

        print "Processing $gcov_file ($source_file)\n";

        foreach (split "\n", $generated_files{$gcov_file}) {
            my ($percent, $total_lines, $real_filename) = /\s*([^%]+)% of (\d+) source lines executed in file (.*)/;
            if ($total_lines) {
                my $covered_lines = int(($percent/100) * $total_lines);
                $totals{lines} += $total_lines;
                $totals{covered_lines} += $covered_lines;
                $file_line_coverage{$source_file} = $percent;
                $real_filename{$source_file} = $real_filename;
                next;
            }

            my ($percent, $total_lines, $function) = /\s*([^%]+)% of (\d+) source lines executed in function (.*)/;
            if ($total_lines) {
                $function_line_coverage{$source_file}{$function} = $percent;
                next;
            }

            my ($percent, $total_branches) = /\s*([^%]+)% of (\d+) branches taken at least once in file/;
            if ($total_branches) {
                my $covered_branches = int(($percent/100) * $total_branches);
                $totals{branches} += $total_branches;
                $totals{covered_branches} += $covered_branches;
                $file_branch_coverage{$source_file} = $percent;
                next;
            }

            my ($percent, $total_branches, $function) = /\s*([^%]+)% of (\d+) branches taken at least once in function (.*)/;
            if ($total_branches) {
                $function_branch_coverage{$source_file}{$function} = $percent;
                next;
            }

            my ($percent, $total_calls, $function) = /\s*([^%]+)% of (\d+) calls executed in function (.*)/;
            if ($total_calls) {
                $function_call_coverage{$source_file}{$function} = $percent;
                next;
            }

            my ($percent, $total_calls) = /\s*([^%]+)% of (\d+) calls executed in file/;
            if ($total_calls) {
                my $covered_calls = int(($percent/100) * $total_calls);
                $totals{calls} += $total_calls;
                $totals{covered_calls} += $covered_calls;
                $file_call_coverage{$source_file} = $percent;
                next;
            }
        }

        filter_gcov($gcov_file);
    }
}

write_file_coverage_summary();
write_function_coverage_summary();
write_index();

exit(0);


sub write_index {
    print "Writing $HTMLDIR/index.html..\n" if $DEBUG;
    open (OUT, ">$HTMLDIR/index.html") ||
      die "Can't open $HTMLDIR/index.html for writing: $!\n";

    $totals{line_coverage} = sprintf("%.2f", ($totals{covered_lines} / $totals{lines} * 100));
    $totals{branch_coverage} = sprintf("%.2f", ($totals{covered_branches} / $totals{branches} * 100));
    $totals{call_coverage} = sprintf("%.2f", ($totals{covered_calls} / $totals{calls} * 100));
    
    print OUT page_header("Parrot Test Coverage");
    print OUT qq(
            <ul>
              <li><a href="file_summary.html">File Summary</a>
              <li><a href="function_summary.html">Function Summary</a>
              <li>Overall Summary:<br>

            <table border="1">
              <tbody>
                 <tr>
                 <th></th><th>Lines</th><th>Branches</th><th>Calls</th>
                 </tr>
                 <tr>
                 <td>Totals:</td>
                 <td>$totals{covered_lines} of $totals{lines} ($totals{line_coverage} %)</td>
                 <td>$totals{covered_branches} of $totals{branches} ($totals{branch_coverage} %)</td>
                 <td>$totals{covered_calls} of $totals{calls} ($totals{call_coverage} %)</td>
                 </tr>
              </tbody>
            </table>
            </ul>
    );
    print OUT page_footer();
}


sub write_file_coverage_summary {

    print "Writing $HTMLDIR/file_summary.html..\n" if $DEBUG;
    open (OUT, ">$HTMLDIR/file_summary.html") ||
      die "Can't open $HTMLDIR/file_summary.html for writing: $!\n";

    print OUT page_header("File Coverage Summary");
    print OUT qq(
            <i>You may click on a percentage to see line-by-line detail</i>
            <table border="1">
              <tbody>
                <tr>
                  <th>File</th>
                  <th>Line Coverage</th>
                  <th>Branch Coverage</th>
                  <th>Call Coverage</th>
    );

    foreach my $source_file (sort keys %file_line_coverage) {
        my $outfile_base = $source_file;
        $outfile_base =~ s/\//_/g;

        print OUT qq(
           <tr>
             <td>$source_file</td>
             <td><a href="$outfile_base.lines.html">@{[$file_line_coverage{$source_file} ? "$file_line_coverage{$source_file} %" : "n/a" ]}</a></td>
             <td><a href="$outfile_base.branches.html">@{[$file_branch_coverage{$source_file} ? "$file_branch_coverage{$source_file} %" : "n/a" ]}</a></td>
             <td><a href="$outfile_base.calls.html">@{[$file_call_coverage{$source_file} ? "$file_call_coverage{$source_file} %" : "n/a" ]}</a></td>
             <td>[<a href="function_summary.html#$source_file">function detail</a>]</td>
          </tr>
       );
    }

    print OUT qq(
            </tbody>
          </table>
    );
    print OUT page_footer();

    close(OUT);
}



sub write_function_coverage_summary {

    print "Writing $HTMLDIR/function_summary.html..\n" if $DEBUG;
    open (OUT, ">$HTMLDIR/function_summary.html") ||
      die "Can't open $HTMLDIR/function_summary.html for writing: $!\n";

    print OUT page_header("Function Coverage Summary");
    print OUT qq(
            <i>You may click on a percentage to see line-by-line detail</i>
    );

    foreach my $source_file (sort keys %file_line_coverage) {

        print OUT qq(
            <hr noshade>
            <a name="$source_file"></a>
            <b>File: $source_file</b><br>
            <table border="1">
              <tbody>
                <tr>
                  <th>Function</th>
                  <th>Line Coverage</th>
                  <th>Branch Coverage</th>
                  <th>Call Coverage</th>
    );

        my $outfile_base = $source_file; 
        $outfile_base =~ s/\//_/g;

        foreach my $function (sort keys %{$function_line_coverage{$source_file}}) {

            print OUT qq(
           <tr>
             <td>$function</td>
             <td><a href="$outfile_base.lines.html#$function">@{[$function_line_coverage{$source_file}{$function} ? "$function_line_coverage{$source_file}{$function} %" : "n/a" ]}</a></td>
             <td><a href="$outfile_base.branches.html#$function">@{[$function_branch_coverage{$source_file}{$function} ? "$function_branch_coverage{$source_file}{$function} %" : "n/a" ]}</a></td>
             <td><a href="$outfile_base.calls.html#$function">@{[$function_call_coverage{$source_file}{$function} ? "$function_call_coverage{$source_file}{$function} %" : "n/a" ]}</a></td>
           </tr>
            );
        }
        print OUT qq(
            </tbody>
            </table>
        );
    }

    print OUT page_footer();

    close(OUT);
}


sub filter_gcov {
    my ($infile) = @_;

    my $source_file = $infile;
    $source_file =~ s/\.gcov$//g;

    my $outfile_base = $source_file; 
    $outfile_base =~ s/\//_/g;
    $outfile_base = "$HTMLDIR/$outfile_base";

    my $outfile = "$outfile_base.lines.html";
    print "Writing $outfile..\n" if $DEBUG;
    open (IN, "<$infile") || die "Can't read $infile: $!\n";
    open (OUT, ">$outfile") || die "Can't write $outfile: $!\n";

    print OUT page_header("Line Coverage for $source_file");
    print OUT "<pre>";

    # filter out any branch or call coverage lines.
    do_filter(sub { /^(call|branch)/ } );

    print OUT "</pre>";
    print OUT page_footer();

    close(OUT);
    close(IN);


    my $outfile = "$outfile_base.branches.html";
    print "Writing $outfile..\n" if $DEBUG;
    open (IN, "<$infile") || die "Can't read $infile: $!\n";
    open (OUT, ">$outfile") || die "Can't write $outfile: $!\n";

    print OUT page_header("Branch Coverage for $source_file");
    print OUT "<pre>";

    # filter out any call coverage lines.
    do_filter(sub { /^call/ } );

    print OUT "</pre>";
    print OUT page_footer();

    close(OUT);
    close(IN);


    my $outfile = "$outfile_base.calls.html";
    print "Writing $outfile..\n" if $DEBUG;
    open (IN, "<$infile") || die "Can't read $infile: $!\n";
    open (OUT, ">$outfile") || die "Can't write $outfile: $!\n";

    print OUT page_header("Call Coverage for $source_file");
    print OUT "<pre>";

    # filter out any branch coverage lines.
    do_filter(sub { /^branch/ } );

    print OUT "</pre>";
    print OUT page_footer();

    close(OUT);
    close(IN);


    return;

    sub do_filter {
        my ($skip_func) = @_;

        while (<IN>) {
            s/&/&amp;/g;
            s/</&lt;/g;
            s/>/&gt;/g;

            next if (&{$skip_func}($_));

            my $atag;
            if (/^\s*([^\(\s]+)\(/) {
                $atag="<a name=\"$1\"></a>";
            }

            my ($initial) = substr($_, 0, 16);
            if ($initial =~ /^\s*\d+\s*$/) {
                print OUT qq($atag<font color="green">$_</font>)
            } elsif ($_ =~ /branch \d+ taken = 0%/) {
                print OUT qq($atag<font color="red">$_</font>)
            } elsif ($_ =~ /call \d+ returns = 0%/) {
                print OUT qq($atag<font color="red">$_</font>)
            } elsif ($_ =~ /^call \d+ never executed/) {
                print OUT qq($atag<font color="red">$_</font>)
            } elsif ($_ =~ /^branch \d+ never executed/) {
                print OUT qq($atag<font color="red">$_</font>)
            } elsif ($initial =~ /\#\#\#/) {
                print OUT qq($atag<font color="red">$_</font>)
            } else {
                print OUT "$atag$_";
            }
        }
    }
}


sub page_header {
    my ($title) = @_;

    qq(
        <html>
          <head>
             <title>$title</title>
          </head>
          <body bgcolor="white">
            <h1>$title</h1>
            <hr noshade>
    );
}


sub page_footer {
    "<hr noshade><i>Last Updated: @{[ scalar(localtime) . strftime(' (%Z)', localtime(time)) ]} </i>
     </body></html>";
}
