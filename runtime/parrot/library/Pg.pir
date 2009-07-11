# Copyright (C) 2006-2008, Parrot Foundation.
# $Id$

=head1 NAME

Pg.pir - OO interface to libpq

=head1 SYNOPSIS

  .local pmc pg, con, res
  pg = get_class 'Pg'
  con = pg.'connectdb'('dbname = db')
  res = con.'exec'('SELECT * from tab')
  n = res.'ntuples'()
  m = res.'nfields'()
  ...
  val = res.'getvalue'(i, j)


=head1 ABSTRACT

The Pg library provides an OO interface to libpq functions - the C
interface to PostgreSQL.

See "Chapter 28. libpq - C Library" for details or the tests
in F<t/library/pg.t>.

=head1 AUTHOR

Leopold "leo" Toetsch <lt(at)toetsch.at>

=head1 DESCRIPTION

Pg.pir is a thin wrapper around F<postgres.pir> - the NCI functions of
the C<libpq> library. It's roughly divided into 3 parts, represented
by 'Pg', 'Pg;Conn', and 'Pg;Result' classes.

=cut

## TODO generate includes from libpq-fe.h
## .include 'postgres.pasm'

.const int CONNECTION_OK = 0

.sub __load :load
    .local pmc cl
    load_bytecode 'postgres.pbc'
    cl = newclass 'Pg'       # Pg connection constructor

    # XXX the hasa 'con' is suboptimal
    cl = newclass ['Pg'; 'Conn']  # Pg connection class
    addattribute cl, 'con'

    cl = newclass ['Pg'; 'Result']  # Pg Result class
    addattribute cl, 'res'
.end

=head2 Connection Construction

=over

=item con = Pg::connectdb('var=val var=val ...')

A class method that returns a new connection object.

=back

=cut

.namespace ['Pg']

.sub connectdb :method
    .param string args
    .local pmc con, connectdb, o_con
    connectdb = get_global 'PQconnectdb'
    con = connectdb(args)
    $P0 = get_class ['Pg';'Conn']

    .local pmc init_data
    init_data        = new 'Hash'
    init_data['con'] = con
    o_con            = new $P0, init_data

    # verify success
    .local int ok
    ok = o_con.'status'()
    if ok == CONNECTION_OK goto is_ok

    con              = new 'Undef'
    init_data['con'] = con

    o_con = new $P0, init_data
is_ok:
    .return (o_con)
.end

.namespace ['Pg'; 'Conn']

=head2 Connection Methods

=over

=item __init(con)

Object initializer. Takes a C<PGconn> structure.

=cut

.sub init_pmc :vtable :method
    .param pmc init

    .local pmc con
    con = init['con']
    setattribute self, 'con', con
.end

.sub get_bool :vtable :method
    .local pmc con
    con = getattribute self, 'con'

    $I0 = isa con, 'Undef'
    $I1 = not $I0

    .return ($I1)
.end

=item c = res.'PGconn'()

Return the raw PGconn structure. You probably don't need this
function except for calling PQ* code directly.

=cut

.sub PGconn :method
    .local pmc con
    con = getattribute self, 'con'
    .return (con)
.end

=item $I0 = con.'status'()

Return the connection status.

=cut

.sub 'status' :method
    .local pmc con, st
    con = getattribute self, 'con'
    st = get_root_global ['parrot';'Pg'], 'PQstatus'
    $I0 = st(con)
    .return ($I0)
.end

=item con.'finish'()

Finish the connection. The connection attribute is set to .undef
thereafter and inaccessible then.

=cut

.sub 'finish' :method
    .local pmc con, finish
    con = getattribute self, 'con'
    # XXX this really is looking ugly
    # XXX and what happens if Pg is loaded from another HLL?
    finish = get_root_global ['parrot';'Pg'], 'PQfinish'
    finish(con)
    con = new 'Undef'
    setattribute self, 'con', con
.end

=item res = con.'exec'(str)

Execute the SQL command and return a Pg;Result object.

=cut

# result creation helper
.sub mk_res
    .param pmc res
    .local pmc o_res
    $P0 = get_class ['Pg';'Result']
    o_res = new $P0
    setattribute o_res, 'res', res
    .return (o_res)
.end

.sub 'exec' :method
    .param string cmd
    .local pmc con, exec, res
    con = getattribute self, 'con'
    exec = get_root_global ['parrot';'Pg'], 'PQexec'
    res = exec(con, cmd)
    .tailcall mk_res(res)
.end

.include "datatypes.pasm"

=item res = con.'execParams'(str, val, ...)

Execute the SQL command and return a Pg;Result object. All values
are considered being text - there's no provision to use binary data.

=cut

# helper to create the char* array
.sub mk_struct
    .param pmc values
    .local int i, n
    .local pmc str, vals
    n = elements values
    str = new 'OrderedHash'
    push str, .DATATYPE_CSTR
    push str, n
    push str, 0
    vals = new 'ManagedStruct'
    assign vals, str
    i = 0
loop:
    if i >= n goto done
    $S0 = values[i]
    vals[0; i] = $S0
    inc i
    goto loop
done:
    .return (n, vals)
.end

.sub 'execParams' :method
    .param string cmd
    .param pmc values  :slurpy
    .local pmc con, exec, res, nil, vals
    .local int n
    con = getattribute self, 'con'
    exec = get_root_global ['parrot';'Pg'], 'PQexecParams'
    nil = new 'ManagedStruct'
    (n, vals) = mk_struct(values)
    # we don't handle binary
    res = exec(con, cmd, n, nil, vals, nil, nil, 0)
    .tailcall mk_res(res)
.end

=item res = con.'prepare'(name, query, nparams)

Prepare a query for execution with B<execPrepared>

=cut

.sub 'prepare' :method
    .param string name
    .param string query
    .param int nparams
    .local pmc con, f, res, nil
    con = getattribute self, 'con'
    f = get_root_global ['parrot';'Pg'], 'PQprepare'
    nil = new 'ManagedStruct'
    res = f(con, name, query, nparams, nil)
    .tailcall mk_res(res)
.end

=item res = con.'execPrepared'(name, val, ...)

Execute a prepared query.

=cut

.sub 'execPrepared' :method
    .param string name
    .param pmc values :slurpy
    .local pmc con, f, res, nil, vals
    .local int n
    con = getattribute self, 'con'
    f = get_root_global ['parrot';'Pg'], 'PQexecPrepared'
    nil = new 'ManagedStruct'
    (n, vals) = mk_struct(values)
    res = f(con, name, n, vals, nil, nil, 0)
    .tailcall mk_res(res)
.end

=item $P0 = con.'setNoticeReceiver'(cb, arg)

Install a notice receiver callback. The callback will be called as

  .sub 'notice'
    .param pmc arg
    .param pmc res


=cut

.sub 'setNoticeReceiver' :method
    .param pmc the_sub
    .param pmc arg
    .local pmc con, cb, f
    cb = new_callback the_sub, arg, "vUp"
    f = get_root_global ['parrot';'Pg'], 'PQsetNoticeReceiver'
    con = getattribute self, 'con'
    $P0 = f(con, cb, arg)
    .return ($P0)
.end

=back

=cut

.namespace ['Pg'; 'Result']

=head2 Result Methods

=over

=item __finalize()

Object finalizer. Calls self.'clear'().

=cut

.sub __finalize :method
    self.'clear'()
.end

=item r = res.'PGresult'()

Return the raw PGresult structure. You probably don't need this
function except for calling PQ* code directly.

=cut

.sub PGresult :method
    .local pmc res
    res = getattribute self, 'res'
    return (res)
.end

=item $I0 = res.'resultStatus'()

Return the status of the result.

=cut

.sub 'resultStatus' :method
    .local pmc res, st
    res = getattribute self, 'res'
    st = get_root_global ['parrot';'Pg'], 'PQresultStatus'
    $I0 = st(res)
    .return ($I0)
.end

=item res.'clear'()

Clear the result structure. You don't have to explicitly call this
method. If a result object is no longer alive, the GC will call
__finalize(), which wil clear the object.

=cut

.sub 'clear' :method
    .local pmc res, clear
    res = getattribute self, 'res'
    if null res goto done
    clear = get_root_global ['parrot';'Pg'], 'PQclear'
    clear(res)
    null res
    setattribute self, 'res', res
done:
.end

=item $I0 = res.'ntuples'()

Return the amount of tuples in the result.

=item $I0 = res.'nfields'()

Return the amount of fields in the result.

=item $S0 = res.'fname'(c)

Return the name of the c-th field in the result.

=item $I0 = res.'fnumber'(col_name)

Return the number of the field or -1.

=cut

.sub 'ntuples' :method
    .local pmc res, nt
    res = getattribute self, 'res'
    nt = get_root_global ['parrot';'Pg'], 'PQntuples'
    $I0 = nt(res)
    .return ($I0)
.end

.sub 'nfields' :method
    .local pmc res, nf
    res = getattribute self, 'res'
    nf = get_root_global ['parrot';'Pg'], 'PQnfields'
    $I0 = nf(res)
    .return ($I0)
.end

.sub 'fname' :method
    .param int c
    .local pmc res, f
    res = getattribute self, 'res'
    f = get_root_global ['parrot';'Pg'], 'PQfname'
    $S0 = f(res, c)
    .return ($S0)
.end

.sub 'fnumber' :method
    .param string c
    .local pmc res, f
    res = getattribute self, 'res'
    f = get_root_global ['parrot';'Pg'], 'PQfnumber'
    $I0 = f(res, c)
    .return ($I0)
.end

=item v = res.'getvalue'(r, c)

Return result value from row r and column c.

=item $I0 = res.'getisnull'(r, c)

Return true if the result value at (r,c) is NULL.

=cut

.sub 'getvalue' :method
    .param int r
    .param int c
    .local pmc res, gv
    res = getattribute self, 'res'
    gv = get_root_global ['parrot';'Pg'], 'PQgetvalue'
    $S0 = gv(res, r, c)
    .return ($S0)
.end

.sub 'getisnull' :method
    .param int r
    .param int c
    .local pmc res, f
    res = getattribute self, 'res'
    f = get_root_global ['parrot';'Pg'], 'PQgetisnull'
    $I0 = f(res, r, c)
    .return ($I0)
.end

=back

=cut

# Local Variables:
#   mode: pir
#   fill-column: 100
# End:
# vim: expandtab shiftwidth=4 ft=pir:
