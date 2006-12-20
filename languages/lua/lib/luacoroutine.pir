# Copyright (C) 2005-2006, The Perl Foundation.
# $Id$

=head1 NAME

lib/luacoroutine.pir - Lua Coroutine Library

=head1 DESCRIPTION

The operations related to coroutines comprise a sub-library of the basic
library and come inside the table C<coroutine>.

See "Lua 5.1 Reference Manual", section 5.2 "Coroutine Manipulation".

=head2 Functions

=over 4

=cut

.HLL 'Lua', 'lua_group'

.sub 'init_coroutine' :load :anon

    load_bytecode 'languages/lua/lib/luabasic.pbc'
    load_bytecode 'Parrot/Coroutine.pbc'

#    print "init Lua Coroutine\n"

    .local pmc _lua__GLOBAL
    _lua__GLOBAL = global '_G'
    $P1 = new .LuaString

    .local pmc _coroutine
    _coroutine = new .LuaTable
    $P1 = 'coroutine'
    _lua__GLOBAL[$P1] = _coroutine

    .const .Sub _coroutine_create = '_coroutine_create'
    $P1 = 'create'
    _coroutine[$P1] = _coroutine_create

    .const .Sub _coroutine_resume = '_coroutine_resume'
    $P1 = 'resume'
    _coroutine[$P1] = _coroutine_resume

    .const .Sub _coroutine_running = '_coroutine_running'
    $P1 = 'running'
    _coroutine[$P1] = _coroutine_running

    .const .Sub _coroutine_status = '_coroutine_status'
    $P1 = 'status'
    _coroutine[$P1] = _coroutine_status

    .const .Sub _coroutine_wrap = '_coroutine_wrap'
    $P1 = 'wrap'
    _coroutine[$P1] = _coroutine_wrap

    .const .Sub _coroutine_yield = '_coroutine_yield'
    $P1 = 'yield'
    _coroutine[$P1] = _coroutine_yield

    $P0 = new .ResizablePMCArray
    global '_COROUTINE_STACK' = $P0

.end

=item C<coroutine.create (f)>

Creates a new coroutine, with body C<f>. C<f> must be a Lua function.
Returns this new coroutine, an object with type C<"thread">.

=cut

.sub '_coroutine_create' :anon
    .param pmc f :optional
    .local pmc ret
    checktype(f, 'function')
    $I0 = isa f, 'LuaClosure'
    if $I0 goto L1
    argerror('Lua function expected')
L1:
    ret = new .LuaThread, f
    .return (ret)
.end

=item C<coroutine.resume (co [, val1, ..., valn])>

Starts or continues the execution of coroutine C<co>. The first time you
resume a coroutine, it starts running its body. The values C<val1, ..., valn>
are passed as the arguments to the body function. If the coroutine has yielded,
C<resume> restarts it; the values C<val1, ..., valn> are passed as the results
from the yield.

If the coroutine runs without any errors, C<resume> returns B<true> plus any
values passed to yield (if the coroutine yields) or any values returned by
the body function (if the coroutine terminates). If there is any error,
C<resume> returns B<false> plus the error message.

=cut

.sub '_coroutine_resume' :anon
    .param pmc co :optional
    .param pmc argv :slurpy
    .local pmc ret
    .local pmc status
    .local pmc co_stack
    co_stack = global '_COROUTINE_STACK'
    new status, .LuaBoolean
    checktype(co, 'thread')
    push co_stack, co
    $P0 = getattribute co, 'co'
    $P1 = getattribute $P0, 'state'
    if $P1 goto L1
    status = 0
    .const .LuaString msg = "cannot resume dead coroutine"
    $P0 = pop co_stack
    .return (status, msg)
L1:
    push_eh _handler
    (ret :slurpy) = $P0.'resume'(argv :flat)
    status = 1
    .return (status, ret :flat)
_handler:
    .local pmc e
    .local string s
    .local pmc msg
    .get_results (e, s)
    status = 0
    new msg, .LuaString
    msg = s
    $P0 = pop co_stack
    .return (status, msg)
.end


=item C<coroutine.running ()>

Returns the running coroutine, or B<nil> when called by the main thread.

=cut

.sub '_coroutine_running' :anon
    .local pmc co_stack
    .local pmc ret
    co_stack = global '_COROUTINE_STACK'
    $I0 = elements co_stack
    if $I0 goto L1
    new ret, .LuaNil
    goto L2
L1:
    ret = pop co_stack
    push co_stack, ret 
L2:    
    .return (ret)
.end


=item C<coroutine.status (co)>

Returns the status of coroutine C<co>, as a string: C<"running">, if the
coroutine is running (that is, it called C<status>); C<"suspended">, if the
coroutine is suspended in a call to yield, or if it has not started running
yet; C<"normal"> if the coroutine is active but not running (that is, it has
resumed another coroutine); and C<"dead"> if the coroutine has finished its
body function, or if it has stopped with an error.

STILL INCOMPLETE.

=cut

.sub '_coroutine_status' :anon
    .param pmc co :optional
    .local pmc ret
    checktype(co, 'thread')
    new ret, .LuaString
    $P0 = getattribute co, 'co'
    $P1 = getattribute $P0, 'state'
    if $P1 goto L1
    ret = 'dead'
    goto L2
L1:
    ret = 'suspended'
L2:
    .return (ret)
.end

=item C<coroutine.wrap (f)>

Creates a new coroutine, with body C<f>. C<f> must be a Lua function.
Returns a function that resumes the coroutine each time it is called. Any
arguments passed to the function behave as the extra arguments to C<resume>.
Returns the same values returned by C<resume>, except the first boolean. In
case of error, propagates the error.

NOT YET IMPLEMENTED.

=cut

.sub '_coroutine_wrap' :anon
    .param pmc f :optional
    .local pmc ret
    ret = _coroutine_wrap(f)
    not_implemented()
.end

=item C<coroutine.yield ([val1, ..., valn])>

Suspends the execution of the calling coroutine. The coroutine cannot be
running a C function, a metamethod, or an iterator.
Any arguments to C<yield> are passed as extra results to C<resume>.

=cut

.sub '_coroutine_yield' :anon
    .param pmc argv :slurpy
    .local pmc ret
    .local pmc co
    .local pmc co_stack
    co_stack = global '_COROUTINE_STACK'
    co = pop co_stack 
    $P0 = getattribute co, 'co'
    (ret :slurpy) = $P0.'yield'(argv :flat)
    .return (ret :flat)
.end

=back

=head1 AUTHORS

Francois Perrad.

Bob Rogers.

=cut

