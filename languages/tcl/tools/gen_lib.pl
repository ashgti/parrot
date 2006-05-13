#! perl -w

use strict;
use lib qw(lib);

my $template = shift;

my $header = <<EOH;
# This file automatically generated by $0. Edit
# $template instead.

EOH

open(TEMPLATE,$template) or die;
local $/ = undef;
my $contents = $header . <TEMPLATE>;
close(TEMPLATE);

# commands that are in Tcl's :: namespace directly
my $command_dir = "runtime/builtin";
opendir(CMDDIR,$command_dir);
my @cmd_files = readdir(CMDDIR);
closedir(CMDDIR);

# subroutines that generate the PIR for Tcl commands
# (including templates)
my $builtins_dir = "src/builtin";
opendir(CMDDIR,$builtins_dir);
my @blt_files = readdir(CMDDIR);
closedir(CMDDIR);

my @cmd_includes = map {"$command_dir/$_"} grep {m/\.pir$/}  @cmd_files;
my @blt_includes = map {"$builtins_dir/$_"} grep {m/\.pir$/} @blt_files;

# remove extensions from @cmd_files and @blt_files
# (this uses $_ as an alias -- somewhat subtle and evil)
my @commands = grep {s/\.(pir|tmt)$//} @cmd_files, @blt_files;

my $lib_dir = "runtime";
opendir(LIBDIR,$lib_dir) or die;
my @libs = map {"$lib_dir/$_"} grep {m/\.pir$/} grep {$_ ne "tcllib.pir"} readdir(LIBDIR);
closedir(LIBDIR);

my $src_dir = "src";
opendir(SRCDIR,$src_dir) or die;
my @srcs = map {"$src_dir/$_"} grep {! m/^(tclsh|macros|returncodes).pir$/} grep {m/\.pir$/} readdir(SRCDIR);
closedir(SRCDIR);

my $includes;
foreach my $file (@cmd_includes, @blt_includes, @libs, @srcs) {
  $includes .= "  .include 'languages/tcl/$file\'\n";
}

my $fallbacks;

# For every builtin with an inline'd version and no interpreted version,
# create a shim for the interpreted version that automatically calls 
# the inline'd version, compiles the result and invokes it.

my @fallbacks = @blt_files;
foreach my $cmd (@cmd_files) {
  @fallbacks = grep { $_ ne $cmd } @fallbacks;
}

foreach my $fallback (@fallbacks) {
$fallbacks .= <<"END_PIR"
# fallback for interpreter: call the inlined version

.sub "&$fallback"
  .param pmc argv :slurpy
  .local pmc inlined, pir_compiler, invokable
  .get_from_HLL(inlined, '_tcl';'builtins', '$fallback')
  .local string pir_code
  .local int register_num
  (register_num,pir_code) = inlined(0,argv)
  .get_from_HLL(pir_compiler, '_tcl', 'pir_compiler')
  invokable = pir_compiler(register_num,pir_code)
  .return invokable()
.end
END_PIR
}

$contents =~ s/\${INCLUDES}/$includes/;
$contents =~ s/\${HEADER}/This file automatically generated by $0, do not edit/;
$contents =~ s/\${FALLBACKS}/$fallbacks/;
$contents =~s/\${XXX.*}//g;

print $contents;
