#! perl -w

use Test::More tests => 4;

use File::Find qw(find);
use ExtUtils::Manifest;

my $ignore_re = qr,(?:   \.cvsignore $ ),x;

ok (-e $ExtUtils::Manifest::MANIFEST, 'MANIFEST exists');

$ExtUtils::Manifest::Quiet = 1;
ok (!ExtUtils::Manifest::manicheck (), 'manicheck ()');

sub scan_cvs;
sub list_diff (\@\@);
sub read_manifest;

SKIP: {
  skip ('No CVS version', 1) unless -e 'CVS';

  local @cvs_entries;
  find { wanted => \&scan_cvs }, '.';

  @cvs_entries = grep { s,^./,, && !m/$ignore_re/ } @cvs_entries;

  my @manifest = sort keys %{ExtUtils::Manifest::maniread()};

  my ($cvs_miss, $mani_miss);

  ($cvs_miss, $mani_miss) = list_diff @cvs_entries, @manifest;

  local $" = "\n\t";

  ok (!@$cvs_miss, 'all files in MANIFEST are in CVS')
    or diag ("Missing files in CVS:\n\t@$cvs_miss");

  ok (!@$mani_miss, 'all files in CVS are in MANIFEST')
    or diag ("Missing files in Manifest:\n\t@$mani_miss");
}

sub scan_cvs {
  my $file = $_;

  if ( $file eq 'CVS') {
    local *CVS;

    open CVS, 'CVS/Entries';

    while (<CVS>) {
      chop; 
      next if m/^D/; # directories are will be further scanned

      my (undef, $entry, $rev) = split '/';
      push @cvs_entries, "$File::Find::dir/$entry" if $rev !~ m/^-/;
    }

    close CVS;

    $File::Find::prune = 1;
  }
}

sub list_diff (\@\@) {
  my ($a, $b) = @_;

  my %elem;
  grep { $elem{$_}++ } @$a;
  grep { $elem{$_}-- } @$b;

  return ( [ sort grep { $elem{$_} < 0 } keys %elem ],
	   [ sort grep { $elem{$_} > 0 } keys %elem ] );
}

