/*
Copyright (C) 2001-2009, Parrot Foundation.
$Id$

=head1 Run Cores

During execution, the runcore is like the heart of Parrot. The runcore
controls calling the various opcodes with the correct data, and making
sure that program flow moves properly. Some runcores, such as the
I<precomputed C goto runcore> are optimized for speed and don't perform
many tasks beyond finding and dispatching opcodes. Other runcores,
such as the I<GC-Debug>, I<debug> and I<profiling> runcores help with
typical software maintenance and analysis tasks. We'll talk about all
of these throughout the chapter.

Runcores must pass execution to each opcode in the incoming bytecode
stream. This is called I<dispatching> the opcodes. Because the different
runcores are structured in different ways, the opcodes themselves must
be formated differently. The opcode compiler compiles opcodes into a
number of separate formats, depending on what runcores are included in
the compiled Parrot. Because of this, understanding opcodes first
requires an understanding of the Parrot runcores.

Parrot has multiple runcores. Some are useful for particular maintenance
tasks, some are only available as optimizations in certain compilers,
some are intended for general use, and some are just interesing flights
of fancy with no practical benefits. Here we list the various runcores,
their uses, and their benefits.

=head2 Slow Core

The slow core is a basic runcore design that treats each opcode as a
separate function at the C level. Each function is called, and returns
the address of the next opcode to be called by the core. The slow core
performs bounds checking to ensure that the next opcode to be called is
properly in bounds, and not somewhere random in memory. Because of this
modular approach where opcodes are treated as separate executable
entities many other runcores, especially diagnostic and maintenance
cores are based on this design. The program counter C<pc> is the current
index into the bytecode stream. Here is a pseudocode representation for
how the slow core works:

  while(1) {
      pc = NEXT_OPCODE;
      if(pc < LOW_BOUND || pc > HIGH_BOUND)
          throw exception;
      DISPATCH_OPCODE(pc);
      UPDATE_INTERPRETER();
  }

=head2 Fast Core

The fast core is a bare-bones core that doesn't do any of the
bounds-checking or context updating that the slow core does. The fast
core is the way Parrot should run, and is used to find and debug places
where execution strays outside of its normal bounds. In pseudocode, the
fast core is very much like the slow core except it doesn't do the bounds
checking between each instruction, and doesn't update the interpreter's
current context for each dispatch.

  while(1) {
      pc = NEXT_OPCODE;
      DISPATCH_OPCODE(pc);
  }

=head2 Switch Core

As its name implies, the switch core uses a gigantic C C<switch / case>
structure to execute opcodes. Here's a brief example of how this
architecture works:

  for( ; ; current_opcode++) {
      switch(*current_opcode) {
          case opcode_1:
              ...
          case opcode_2:
              ...
          case opcode_3:
              ...
      }
  }

This is quite a fast architecture for dispatching opcodes because it all
happens within a single function. The only operations performed between
opcodes is a jump back to the top of the loop, incrementing the opcode
pointer, dereferencing the opcode pointer, and then a jump to the C<case>
statement for the next opcode.

=head2 Computed Goto Core

I<Computed Goto> is a feature of some C compilers where a label is
treated as a piece of data that can be stored as a C<void *> pointer. Each
opcode becomes simply a label in a very large function, and pointers to the
labels are stored in a large array. Calling an opcode is as easy as taking
that opcode's number as the index of the label array, and calling the
associated label. Sound complicated? It is a little, especially to C
programmers who are not used to using labels, much less treating them as
first class data items.

Notice that computed goto is a feature only available in some compilers
such as GCC, and will not be available in every distribution of Parrot,
depending what compilers were used to build it.

As was mentioned earlier, not all compilers support computed goto, which
means that this core will not be built on platforms that don't support it.
However, it's still an interesting topic to study so we will look at it
briefly here. For compilers that support it, computed goto labels are
C<void **> values. In the computed goto core, all the labels represent
different opcodes, so they are stored in an array:

  void *my_labels[] = {
      &&label1,
      &&label2,
      &&label3
  };

  label1:
      ...
  label2:
      ...
  label3:
      ...

Jumping to one of these labels is done with a command like this:

  goto *my_labels[opcode_number];

Actually, opcodes are pointed to by an C<opcode_t *> pointer, and all
opcodes are stored sequentially in memory, so the actual jump in the
computed goto core must increment the pointer and then jump to the new
version. In C it looks something like this:

  goto *my_labels[*(current_opcode += 1)];

Each opcode is an index into the array of labels, and at the end of each
opcode an instruction like this is performed to move to the next opcode
in series, or else some kind of control flow occurs that moves it to a
non-sequential location:

  goto *my_lables[*(current_opcode = destination)];

These are simplifications on what really happens in this core, because
the actual code has been optimized quite a bit from what has been
presented here. However, as we shall see with the precomputed goto core,
it isn't optimized as aggressively as is possible.

=head2 Precomputed Goto Core

The precomputed goto core is an amazingly fast optimized core that uses
the same computed goto feature, but performs the array dereferencing
before the core even starts. The compiled bytecode is fed into a
preprocessor that converts the bytecode instruction numbers into label
pointer values. In the computed goto core, you have this
operation to move to the next opcode:

  goto *my_labels[*(current_opcode += 1)];

This single line of code is deceptively complex. A number of machine code
operations must be performed to complete this step: The value of
C<current_opcode> must be incremented to the next value, that value must
be dereferenced to find the opcode value. In C, arrays are pointers, so
C<my_labels> gets dereferenced and an offset is taken from it to find
the stored label reference. That label reference is then dereferenced, and
the jump is performed.

That's a lot of steps to execute before we can jump to the next opcode.
What if each opcode value was replaced with the value of the jump
label beforehand? If C<current_opcode> points to a label pointer directly,
we don't need to perform an additional dereference on the array at all. We
can replace that entire mess above with this line:

  goto **(current_opcode += 1);

That's far fewer machine instructions to execute before we can move to the
next opcode, which means faster throughput. Remember that whatever dispatch
mechanism is used will be called after every single opcode, and some large
programs may have millions of opcodes! Every single machine instruction
that can be cut out of the dispatch mechanism could increase the execution
speed of Parrot in a significant and noticable way. B<The dispatch mechanism
used by the various runcores is hardly the largest performance bottleneck in
Parrot anyway, but we like to use faster cores to shave every little bit of
speed out of the system>.

The caveat of course is that the predereferenced computed goto core is only
available with compilers that support computed goto, such as GCC. Parrot
will not have access to this core if it is built with a different compiler.

=head2 Tracing Core

To come.

=head2 Profiling Core

The profiling core analyzes the performance of Parrot, and helps to
determine where bottlenecks and trouble spots are in the programs that
run on top of Parrot. When Parrot calls a PIR subroutine it sets up the
environment, allocates storage for the passed parameters and the return
values, passes the parameters, and calls a new runcore to execute it. To
calculate the amount of time that each subroutine takes, we need to
measure the amount of time spent in each runcore from the time the core
begins to the time the core executes. The profiling core does exactly
this, acting very similarly to a slow core but also measuring the amount
of time it takes for the core to complete. The tracing core actually
keeps track of a few additional values, including the number of GC cycles
run while in the subroutine, the number of each opcode called and the
number of calls to each subroutine made. All this information is helpfully
printed to the STDERR output for later analysis.

=head2 GC Debug Core

Parrot's garbage collector has been known as a weakness in the system
for several years. In fact, the garbage collector and memory management
subsystem was one of the last systems to be improved and rewritten before
the release of version 1.0. It's not that garbage collection isn't
important, but instead that it was so hard to do earlier in the project.

Early on when the GC was such a weakness, and later when the GC was under
active development, it was useful to have an operational mode that would
really exercise the GC and find bugs that otherwise could hide by sheer
chance. The GC debug runcore was this tool. The core executes a complete
collection iteration between every single opcode. The throughput
performance is terrible, but that's not the point: it's almost guaranteed
to find problems in the memory system if they exist.

=head2 Debug Core

The debug core works like a normal software debugger, such as GDB. The
debug core executes each opcode, and then prompts the user to enter a
command. These commands can be used to continue execution, step to the
next opcode, or examine and manipulate data from the executing program.


=head2 Functions

=over 4

=cut

*/

#include "parrot/runcore_api.h"
#include "parrot/embed.h"
#include "parrot/runcore_trace.h"

#ifdef HAVE_COMPUTED_GOTO
#  include "parrot/oplib/core_ops_cg.h"
#endif

/* HEADERIZER HFILE: include/parrot/runcore_api.h */

/* HEADERIZER BEGIN: static */
/* Don't modify between HEADERIZER BEGIN / HEADERIZER END.  Your changes will be lost. */

PARROT_WARN_UNUSED_RESULT
PARROT_CAN_RETURN_NULL
static opcode_t * runops_trace_core(PARROT_INTERP, ARGIN(opcode_t *pc))
        __attribute__nonnull__(1)
        __attribute__nonnull__(2);

#define ASSERT_ARGS_runops_trace_core __attribute__unused__ int _ASSERT_ARGS_CHECK = \
       PARROT_ASSERT_ARG(interp) \
    || PARROT_ASSERT_ARG(pc)
/* Don't modify between HEADERIZER BEGIN / HEADERIZER END.  Your changes will be lost. */
/* HEADERIZER END: static */

#ifdef code_start
#  undef code_start
#endif
#ifdef code_end
#  undef code_end
#endif

#define  code_start interp->code->base.data
#define  code_end (interp->code->base.data + interp->code->base.size)

/*

=item C<opcode_t * runops_fast_core(PARROT_INTERP, opcode_t *pc)>

Runs the Parrot operations starting at C<pc> until there are no more
operations.  This performs no bounds checking, profiling, or tracing.

=cut

*/

PARROT_WARN_UNUSED_RESULT
PARROT_CAN_RETURN_NULL
opcode_t *
runops_fast_core(PARROT_INTERP, ARGIN(opcode_t *pc))
{
    ASSERT_ARGS(runops_fast_core)

    /* disable pc */
    Parrot_pcc_set_pc(interp, CURRENT_CONTEXT(interp), NULL);

    while (pc) {
        if (pc < code_start || pc >= code_end)
            Parrot_ex_throw_from_c_args(interp, NULL, 1,
                "attempt to access code outside of current code segment");

        DO_OP(pc, interp);
    }

    return pc;
}


/*

=item C<opcode_t * runops_cgoto_core(PARROT_INTERP, opcode_t *pc)>

Runs the Parrot operations starting at C<pc> until there are no more
operations, using the computed C<goto> core, performing no bounds checking,
profiling, or tracing.

If computed C<goto> is not available then Parrot exits with exit code 1.

=cut

*/

PARROT_WARN_UNUSED_RESULT
PARROT_CAN_RETURN_NULL
opcode_t *
runops_cgoto_core(PARROT_INTERP, ARGIN(opcode_t *pc))
{
    ASSERT_ARGS(runops_cgoto_core)

    /* disable pc */
    Parrot_pcc_set_pc(interp, CURRENT_CONTEXT(interp), NULL);

#ifdef HAVE_COMPUTED_GOTO
    pc = cg_core(pc, interp);
    return pc;
#else
    UNUSED(pc);
    Parrot_io_eprintf(interp,
            "Computed goto unavailable in this configuration.\n");
    Parrot_exit(interp, 1);
#endif
}


/*

=item C<static opcode_t * runops_trace_core(PARROT_INTERP, opcode_t *pc)>

Runs the Parrot operations starting at C<pc> until there are no more
operations, using the tracing interpreter.

=cut

*/

PARROT_WARN_UNUSED_RESULT
PARROT_CAN_RETURN_NULL
static opcode_t *
runops_trace_core(PARROT_INTERP, ARGIN(opcode_t *pc))
{
    ASSERT_ARGS(runops_trace_core)

    static size_t  gc_mark_runs, gc_collect_runs;
    Interp        *debugger;

    gc_mark_runs    = Parrot_gc_count_mark_runs(interp);
    gc_collect_runs = Parrot_gc_count_collect_runs(interp);
    if (interp->pdb) {
        debugger = interp->pdb->debugger;
        PARROT_ASSERT(debugger);
    }
    else {
        PMC *pio;

        /*
         * using a distinct interpreter for tracing should be ok
         * - just in case, make it easy to switch
         */
#if 0
        debugger = interp:
#else
        Parrot_debugger_init(interp);
        PARROT_ASSERT(interp->pdb);
        debugger = interp->pdb->debugger;
#endif
        PARROT_ASSERT(debugger);

        /* set the top of the stack so GC can trace it for GC-able pointers
         * see trace_system_areas() in src/cpu_dep.c */
        debugger->lo_var_ptr = interp->lo_var_ptr;

        pio = Parrot_io_STDERR(debugger);

        if (Parrot_io_is_tty(debugger, pio))
            Parrot_io_setlinebuf(debugger, pio);
        else {
            /* this is essential (100 x faster!)  and should probably
             * be in init/open code */
            Parrot_io_setbuf(debugger, pio, 8192);
        }
    }

    trace_op(interp, code_start, code_end, pc);
    while (pc) {
        size_t runs;
        if (pc < code_start || pc >= code_end)
            Parrot_ex_throw_from_c_args(interp, NULL, 1,
                "attempt to access code outside of current code segment");

        Parrot_pcc_set_pc(interp, CURRENT_CONTEXT(interp), pc);

        DO_OP(pc, interp);
        trace_op(interp, code_start, code_end, pc);

        runs = Parrot_gc_count_mark_runs(interp);
        if (gc_mark_runs != runs) {
            gc_mark_runs  = runs;
            Parrot_io_eprintf(debugger, "       GC mark\n");
        }

        runs = Parrot_gc_count_collect_runs(interp);
        if (gc_collect_runs != runs) {
            gc_collect_runs  = runs;
            Parrot_io_eprintf(debugger, "       GC collect\n");
        }
    }

    Parrot_io_flush(debugger, Parrot_io_STDERR(debugger));

    return pc;
}


/*

=item C<opcode_t * runops_slow_core(PARROT_INTERP, opcode_t *pc)>

Runs the Parrot operations starting at C<pc> until there are no more
operations, with tracing and bounds checking enabled.

=cut

*/

PARROT_WARN_UNUSED_RESULT
PARROT_CAN_RETURN_NULL
opcode_t *
runops_slow_core(PARROT_INTERP, ARGIN(opcode_t *pc))
{
    ASSERT_ARGS(runops_slow_core)

    if (Interp_trace_TEST(interp, PARROT_TRACE_OPS_FLAG))
        return runops_trace_core(interp, pc);
#if 0
    if (interp->debugger && interp->debugger->pdb)
        return Parrot_debug(interp, interp->debugger, pc);
#endif

    while (pc) {
        if (pc < code_start || pc >= code_end)
            Parrot_ex_throw_from_c_args(interp, NULL, 1,
                "attempt to access code outside of current code segment");

        Parrot_pcc_set_pc(interp, CURRENT_CONTEXT(interp), pc);

        DO_OP(pc, interp);
    }

    return pc;
}


/*

=item C<opcode_t * runops_gc_debug_core(PARROT_INTERP, opcode_t *pc)>

Runs the Parrot operations starting at C<pc> until there are no more
operations, performing a full GC run before each op.  This is very slow, but
it's also a very quick way to find GC problems.

=cut

*/

PARROT_WARN_UNUSED_RESULT
PARROT_CAN_RETURN_NULL
opcode_t *
runops_gc_debug_core(PARROT_INTERP, ARGIN(opcode_t *pc))
{
    ASSERT_ARGS(runops_gc_debug_core)
    while (pc) {
        if (pc < code_start || pc >= code_end)
            Parrot_ex_throw_from_c_args(interp, NULL, 1,
                "attempt to access code outside of current code segment");

        Parrot_gc_mark_and_sweep(interp, GC_TRACE_FULL);
        Parrot_pcc_set_pc(interp, CURRENT_CONTEXT(interp), pc);

        DO_OP(pc, interp);
    }

    return pc;
}

#undef code_start
#undef code_end


/*

=item C<opcode_t * runops_profile_core(PARROT_INTERP, opcode_t *pc)>

Runs the Parrot operations starting at C<pc> until there are no more
operations, with tracing, bounds checking, and profiling enabled.

=cut

*/

PARROT_WARN_UNUSED_RESULT
PARROT_CAN_RETURN_NULL
opcode_t *
runops_profile_core(PARROT_INTERP, ARGIN(opcode_t *pc))
{
    ASSERT_ARGS(runops_profile_core)
    RunProfile * const profile = interp->profile;
    const opcode_t     old_op  = profile->cur_op;

    /* if reentering the runloop, remember old op and calc time 'til now */
    if (old_op)
        profile->data[old_op].time +=
            Parrot_floatval_time() - profile->starttime;

    while (pc) {/* && pc >= code_start && pc < code_end) */
        opcode_t cur_op;

        Parrot_pcc_set_pc(interp, CURRENT_CONTEXT(interp), pc);
        profile->cur_op                   = cur_op = *pc + PARROT_PROF_EXTRA;
        profile->starttime                = Parrot_floatval_time();
        profile->data[cur_op].numcalls++;

        DO_OP(pc, interp);

        /* profile->cur_op may be different, if exception was thrown */
        profile->data[profile->cur_op].time +=
            Parrot_floatval_time() - profile->starttime;
    }

    if (old_op) {
        /* old opcode continues */
        profile->starttime = Parrot_floatval_time();
        profile->cur_op    = old_op;
    }

    return pc;
}

/*

=item C<opcode_t * runops_debugger_core(PARROT_INTERP, opcode_t *pc)>

Used by the debugger, under construction

=cut

*/

PARROT_WARN_UNUSED_RESULT
PARROT_CAN_RETURN_NULL
opcode_t *
runops_debugger_core(PARROT_INTERP, ARGIN(opcode_t *pc))
{
    ASSERT_ARGS(runops_debugger_core)
    /*fprintf(stderr, "Enter runops_debugger_core\n");*/

    PARROT_ASSERT(interp->pdb);

    if (interp->pdb->state & PDB_ENTER) {
        Parrot_debugger_start(interp, pc);
    }

    while (pc) {
        if (pc < interp->code->base.data || pc >= interp->code->base.data + interp->code->base.size)
            Parrot_ex_throw_from_c_args(interp, NULL, 1,
                    "attempt to access code outside of current code segment");

        if (interp->pdb->state & PDB_GCDEBUG)
            Parrot_gc_mark_and_sweep(interp, 0);

        if (interp->pdb->state & PDB_TRACING) {
            trace_op(interp,
                    interp->code->base.data,
                    interp->code->base.data +
                    interp->code->base.size,
                    pc);
        }

        Parrot_pcc_set_pc(interp, CURRENT_CONTEXT(interp), pc);
        DO_OP(pc, interp);

        if (interp->pdb->state & PDB_STOPPED) {
            Parrot_debugger_start(interp, pc);
        }
        else
        {
            if (PDB_break(interp)) {
                Parrot_debugger_start(interp, pc);
                continue;
            }

            if (interp->pdb->tracing) {
                if (--interp->pdb->tracing == 0) {
                    Parrot_debugger_start(interp, pc);
                }
            }
        }
    }

    return pc;
}

/*

=back

*/

/*
 * Local variables:
 *   c-file-style: "parrot"
 * End:
 * vim: expandtab shiftwidth=4:
 */
