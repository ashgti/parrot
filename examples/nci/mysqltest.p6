# Copyright (C) 2008, The Perl Foundation.
# $Id$

=begin pod

=head1 TITLE

mysqltest.p6 - Test NCI with mysqlclient

=head1 SYNOPSIS

A test for Mysql.pir module.

Compile Mysql.pbc before usage.

This is an initial version, be careful and not expect too much.

Connection data and query are hard coded, edit the file to change.

=end pod

########################################################################
use v6;
use Mysql;

say 'mysql client version: ', Mysql::get_client_info();

my $mysql = Mysql::new();

# host, user, password, database
$mysql.connect('localhost', 'parrot', 'baDworD', 'parrot');

$mysql.query('select * from hello;');

my $r = $mysql.result();

my $row = $r.fetch_row();
my $elems = $row.elems();
while (defined $row) {
    my $i = 0;
    while ($i < $elems) {
        print "'", $row[$i], "'";
        ++$i;
        if ($i < $elems) { print ", "; }
    }
    say '';
    $row := $r.fetch_row();
}

#End
