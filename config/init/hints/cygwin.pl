# cygwin's perl is compiled with -lutil, which for some reason is not 
# in the standard installation, so we get rid of it
my $libs = Configure::Data->get('libs');
$libs =~ s/-lutil\b//g;

my @define = split ',', (Configure::Data->get('define') || '');
push @define, 'inet_aton' unless grep { /^inet_[ap]ton$/ } @define;

# A note about building shared libraries:  Perl5 uses the 'ld2' tool, which
# is installed as part of the perl5 installation.  So far, it appears
# parrot can get by with simply using gcc -shared, so we override the
# perl5 Configure defaults and use 'gcc -shared' instead of 'ld2'.
# If this later causes problems, it might be worth revisiting.
# A. Dougherty 9/9/2002
Configure::Data->set(
  ld => 'gcc',
  ld_shared => '-shared',
  libs => $libs,
);

unless($_[2]) {
  $_[2]='inet_aton';
}
elsif($_[2] !~ /inet_[ap]ton/) {
  $_[2]=join(',', 'inet_aton', $_[2]);
}