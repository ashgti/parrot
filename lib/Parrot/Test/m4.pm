# $Id$

use strict;
use File::Basename;

package Parrot::Test::m4;


=head1

Provide language specific testing routines here...

This is currently alarmingly similar to the generated subs in Parrot::Test.
Perhaps someone can do a better job of delegation here.

=cut

sub new 
{
  return bless {};
}

sub output_is() 
{
  my ( $self, $code, $output, $desc ) = @_;
  
  $count = $self->{builder}->current_test + 1;

  $desc ||= $language;
  
  my $lang_f    = Parrot::Test::per_test( '.m4', $count );
  my $out_f     = Parrot::Test::per_test( '.parrot_out', $count );
  my $parrotdir = dirname( $self->{parrot} );

  $TEST_PROG_ARGS = $ENV{TEST_PROG_ARGS} || '';
  my $args = $TEST_PROG_ARGS;

  # flatten filenames (don't use directories)
  $lang_f = ( File::Spec->splitpath($lang_f) )[2];
  $out_f  =  ( File::Spec->splitpath($out_f) )[2];
  # but, always put the test in a tempdir, so we're not cluttering
  $lang_f = File::Spec->catfile( File::Spec->tmpdir(), $lang_f );
  $out_f  = File::Spec->catfile( File::Spec->tmpdir(), $out_f );
  Parrot::Test::generate_pbc_for( $code, $parrotdir, $count, $lang_f );

  my $cmd;
  my $exit_code = 0;
  my $pass = 0;

  $cmd = "(cd $self->{relpath} &&  $self->{parrot} languages/m4/m4.pbc ${args} ${lang_f})";

  # For some reason, if you redirect both STDERR and STDOUT here, 
  # you get a 38M file of garbage. We'll temporarily assume everything
  # works and ignore stderr.
  $exit_code = Parrot::Test::_run_command( $cmd, STDOUT => $out_f );
  
  unless ($pass) 
  {
    my $file = Parrot::Test::slurp_file( $out_f );
    $pass = $self->{builder}->is_eq( Parrot::Test::slurp_file($out_f), $output, $desc );
    $self->{builder}->diag( "'$cmd' failed with exit code $exit_code" )
      if $exit_code and not $pass;
  }

  unless ( $ENV{POSTMORTEM} ) 
  {
    unlink( $lang_f );
    unlink( $out_f );
  }

  return $pass;
}

1;
