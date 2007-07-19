/*
Copyright (C) 2001-2007, The Perl Foundation.
$Id$

=head1 NAME

src/spf_render.c - Parrot sprintf

=head1 DESCRIPTION

Implements the main function that drives the C<Parrot_sprintf> family
and its utility functions.

=head2 Utility Functions

*/

#define IN_SPF_SYSTEM

#include "parrot/parrot.h"
#include "parrot/string_funcs.h"
#include "spf_render.str"
#include <assert.h>

/* HEADERIZER HFILE: include/parrot/misc.h */

/* HEADERIZER BEGIN: static */

static void gen_sprintf_call(
    NOTNULL(char *out),
    NOTNULL(SpfInfo info),
    int thingy )
        __attribute__nonnull__(1)
        __attribute__nonnull__(2);

PARROT_CANNOT_RETURN_NULL
static STRING * handle_flags( PARROT_INTERP,
    SpfInfo info,
    NOTNULL(STRING *str),
    INTVAL is_int_type,
    NULLOK(STRING* prefix) )
        __attribute__nonnull__(1)
        __attribute__nonnull__(3);

PARROT_CANNOT_RETURN_NULL
static STRING* str_append_w_flags( PARROT_INTERP,
    NOTNULL(STRING* dest),
    SpfInfo info,
    NOTNULL(STRING* src),
    NULLOK(STRING *prefix) )
        __attribute__nonnull__(1)
        __attribute__nonnull__(2)
        __attribute__nonnull__(4);

/* HEADERIZER END: static */


/* Per Dan's orders, we will not use sprintf if snprintf isn't
 * around for us.
 */
#ifdef _MSC_VER
#  define snprintf _snprintf
#endif

/*

FUNCDOC: handle_flags

Handles C<+>, C<->, C<0>, C<#>, space, width, and prec.

*/

PARROT_CANNOT_RETURN_NULL
static STRING *
handle_flags(PARROT_INTERP,
             SpfInfo info, NOTNULL(STRING *str), INTVAL is_int_type, NULLOK(STRING* prefix))
{
    UINTVAL len = string_length(interp, str);

    if (is_int_type) {
        if (info->flags & FLAG_PREC && info->prec == 0 &&
                len == 1 &&
                string_ord(interp, str, 0) == '0') {
            string_chopn_inplace(interp, str, len);
            len = 0;
        }
        /* +, space */
        if (!len || string_ord(interp, str, 0) != '-') {
            if (info->flags & FLAG_PLUS) {
                STRING * const cs = CONST_STRING(interp, "+");
                str = string_concat(interp, cs , str, 0);
                len++;
            }
            else if (info->flags & FLAG_SPACE) {
                STRING * const cs = CONST_STRING(interp, " ");
                str = string_concat(interp, cs , str, 0);
                len++;
            }
        }

        /* # 0x ... */
        if ((info->flags & FLAG_SHARP) && prefix) {
            str = string_concat(interp, prefix, str, 0);
            len += string_length(interp, prefix);
        }
        /* XXX sharp + fill ??? */

#if 0
        /* precision - only for floats, which is handled elsewhere */
        if (info->flags & FLAG_PREC) {
            info->flags |= FLAG_WIDTH;
            if (string_ord(interp, str, 0) == '-' ||
                    string_ord(interp, str, 0) == '+') {
                info->width = info->prec + 1;
            }
            else {
                info->width = info->prec;
            }
        }
#endif
    }
    else {
        /* string precision */
        if (info->flags & FLAG_PREC && info->prec == 0) {
            string_chopn_inplace(interp, str, len);
            len = 0;
        }
        else
            if (info->flags & FLAG_PREC && info->prec < len) {
                string_chopn_inplace(interp, str, -(INTVAL)(info->prec));
                len = info->prec;
            }
    }

    if ((info->flags & FLAG_WIDTH) && info->width > len) {
        STRING * const filler =
            ((info->flags & FLAG_ZERO) && !(info->flags & FLAG_MINUS))
                ? CONST_STRING(interp, "0")
                : CONST_STRING(interp, " ");
        STRING * const fill = string_repeat(interp, filler, info->width - len, NULL);

        if (info->flags & FLAG_MINUS) { /* left-align */
            str = string_concat(interp, str, fill, 0);
        }
        else {                  /* right-align */
            /* signed and zero padded */
            if (info->flags & FLAG_ZERO
                && (string_ord(interp, str,0) == '-' ||
                    string_ord(interp, str,0) == '+')) {
                STRING *temp = NULL;
                string_substr(interp, str, 1, len-1, &temp, 0);
                string_chopn_inplace(interp, str, -1);
                str = string_append(interp, str, fill);
                str = string_append(interp, str, temp);
            }
            else {
                str = string_concat(interp, fill, str, 0);
            }
        }
    }
    return str;
}

PARROT_CANNOT_RETURN_NULL
static STRING*
str_append_w_flags(PARROT_INTERP,
        NOTNULL(STRING* dest), SpfInfo info, NOTNULL(STRING* src), NULLOK(STRING *prefix))
{
    src = handle_flags(interp, info, src, 1, prefix);
    dest = string_append(interp, dest, src);
    return dest;
}

/*

FUNCDOC: gen_sprintf_call

Turn the info structure back into an sprintf format. Far from being
pointless, this is used to call C<snprintf()> when we're confronted with
a float.

*/

static void
gen_sprintf_call(NOTNULL(char *out), NOTNULL(SpfInfo info), int thingy)
{
    int i = 0;

    out[i++] = '%';
    if (info->flags) {
        if (info->flags & FLAG_MINUS) {
            out[i++] = '-';
        }
        if (info->flags & FLAG_PLUS) {
            out[i++] = '+';
        }
        if (info->flags & FLAG_ZERO) {
            out[i++] = '0';
        }
        if (info->flags & FLAG_SPACE) {
            out[i++] = ' ';
        }
        if (info->flags & FLAG_SHARP) {
            out[i++] = '#';
        }
    }

    if (info->flags & FLAG_WIDTH) {
        if (info->width > PARROT_SPRINTF_BUFFER_SIZE - 1) {
            info->width = PARROT_SPRINTF_BUFFER_SIZE;
        }
        i += sprintf(out + i, "%u", (unsigned)info->width);
    }

    if (info->flags & FLAG_PREC) {
        if (info->prec > PARROT_SPRINTF_MAX_PREC) {
            info->prec = PARROT_SPRINTF_MAX_PREC;
        }

        out[i++] = '.';
        i += sprintf(out + i, "%u", (unsigned)info->prec);
    }
    if (thingy == 'd' || thingy == 'i' ||thingy == 'u') {
        /* theu?int isa HUGEU?INTVAL aka long long
         * the 'll' modifier is specced in susv3 - hopefully all our
         * compilers support it too */
        out[i++] = 'l';
        out[i++] = 'l';
    }
    out[i++] = thingy;
    out[i] = 0;
}

/*

FUNCDOC: Parrot_sprintf_format

This is the engine that does all the formatting.

*/

PARROT_WARN_UNUSED_RESULT
PARROT_CANNOT_RETURN_NULL
STRING *
Parrot_sprintf_format(PARROT_INTERP,
        NOTNULL(STRING *pat), NOTNULL(SPRINTF_OBJ *obj))
{
    INTVAL i, len, old;
    /*
     * start with some allocated buffer
     * this speeds up tracing mandel.pasm by a factor of 3
     */
    STRING *targ = string_make_empty(interp, enum_stringrep_one, 128);

    /* ts is used almost universally as an intermediate target;
     * tc is used as a temporary buffer by uint_to_string and
     * as a target by gen_sprintf_call.
     */
    STRING *substr = NULL;
    char tc[PARROT_SPRINTF_BUFFER_SIZE];


    for (i = old = len = 0; i < (INTVAL) string_length(interp, pat); i++) {
        if (string_ord(interp, pat, i) == '%') {        /* % */
            if (len) {
                string_substr(interp, pat, old, len, &substr, 1);
                /* XXX This shouldn't modify targ the pointer */
                targ = string_append(interp, targ, substr);
            }
            len = 0;
            old = i;
            if (string_ord(interp, pat, i + 1) == '%') {
                /* skip this one, make next the first char
                 * of literal sequence, starting at old */
                i++;
                old++;
                len++;
                continue;
            }
            else {
                /* hoo boy, here we go... */

                HUGEINTVAL sharedint = 0;

                /* Storage for flags, etc. */
                struct spfinfo_t info = { 0, 0, 0, 0, 0 };

                /* Reset temporaries */
                tc[0] = '\0';

/*  This can be really hard to understand, so I'll try to explain beforehand.
 *  A rough grammar for a printf format is:
 *
 *  grammar Parrot::PrintF_Format {
 *      rule format {
 *          <other_stuff> ( <field> <other_stuff> )*
 *      }
 *
 *      rule other_stuff {
 *          [<[^\%]> | \%\%]*:
 *      }
 *
 *      rule field {
 *          \%
 *          <flags>?
 *          <width>?
 *          [\.<prec>]?
 *          <size>?
 *          <term>
 *      }
 *
 *      rule flags {
 *          <[
 *              +       # prefix with a + if necessary
 *              -       # left-align
 *              0       # zero-pad
 *              <sp>    # space-pad
 *              \#      # 0, 0x on octal, hex; force decimal point on float
 *          ]>+
 *      }
 *
 *      rule width {
 *          [\d|\*]+    # minimum width
 *      }
 *
 *      rule prec {
 *          [\d|\*]+    # width on integers;
 *                      # number of digits after decimal on floats;
 *                      # maximum width on strings
 *      }
 *
 *      rule size {
 *          <[
 *              h       # short (or float)
 *              l       # long
 *              H       # HUGEwhateverVAL (long [long]?, [long]? double)
 *              v       # whateverVAL
 *              O       # opcode_t
 *              P       # really a PMC
 *              S       # Parrot string (only with %s)
 *          ]>
 *      }
 *
 *      rule term {
 *          <[
 *              c       # char
 *              d       # integer
 *              i       # integer
 *              o       # octal
 *              x       # hex
 *              X       # hex with capital X (if #)
 *              b       # binary
 *              B       # binary with capital B (if #)
 *              u       # unsigned integer
 *              p       # pointer
 *
 *              e       # 1e1
 *              E       # 1E1
 *              f       # 1.0
 *              g       # 1, 0.1, 1e1
 *              G       # 1, 0.1, 1E1
 *
 *              s       # string
 *          ]>
 *      }
 *  }
 *
 *      Complication: once upon a time, %P existed.  Now you should
 *      use %Ps, %Pd or %Pf, but we still need to support the old form.
 *      The same is true of %S--%Ss is the best form, but %S is still
 *      supported.
 *
 *  The implementation of Parrot_vsprintf is surprisingly similar to this
 *  regex, even though the two were developed semi-independently.
 *  Parrot_vsprintf keeps track of what it expects to see next (the
 *  'phase')--flags, width, precision, size, or field type (term).  If it
 *  doesn't find a character that fits whatever it's expecting, it sets
 *  info.phase to the next thing and tries it.  The first four phases just
 *  set flags--the last does all the work.
 */

                for (i++; i < (INTVAL) string_length(interp, pat)
                     && info.phase != PHASE_DONE; i++) {
                    const INTVAL ch = string_ord(interp, pat, i);

                    switch (info.phase) {
                    /*@fallthrough@ */ case PHASE_FLAGS:
                        switch (ch) {
                        case '-':
                            info.flags |= FLAG_MINUS;
                            continue;

                        case '+':
                            info.flags |= FLAG_PLUS;
                            continue;

                        case '0':
                            info.flags |= FLAG_ZERO;
                            continue;

                        case ' ':
                            info.flags |= FLAG_SPACE;
                            continue;

                        case '#':
                            info.flags |= FLAG_SHARP;
                            continue;

                        default:
                            info.phase = PHASE_WIDTH;
                        }


                    /*@fallthrough@ */ case PHASE_WIDTH:
                        switch (ch) {
                        case '0':
                        case '1':
                        case '2':
                        case '3':
                        case '4':
                        case '5':
                        case '6':
                        case '7':
                        case '8':
                        case '9':
                            info.flags |= FLAG_WIDTH;
                            info.width *= 10;
                            info.width += ch - '0';
                            continue;

                        case '*':
                            info.flags |= FLAG_WIDTH;
                            info.width = (UINTVAL)obj->getint(interp,
                                                      SIZE_XVAL, obj);
                            /* fall through */

                        case '.':
                            info.phase = PHASE_PREC;
                            continue;

                        default:
                            info.phase = PHASE_PREC;
                        }


                    /*@fallthrough@ */ case PHASE_PREC:
                        switch (ch) {
                        case '0':
                        case '1':
                        case '2':
                        case '3':
                        case '4':
                        case '5':
                        case '6':
                        case '7':
                        case '8':
                        case '9':
                            info.flags |= FLAG_PREC;
                            info.prec *= 10;
                            info.prec += ch - '0';
                            continue;

                        case '*':
                            info.flags |= FLAG_PREC;
                            info.prec = (UINTVAL)obj->getint(interp,
                                                     SIZE_XVAL, obj);
                            info.phase = PHASE_TYPE;
                            continue;

                        default:
                            info.phase = PHASE_TYPE;
                        }

                    /*@fallthrough@ */ case PHASE_TYPE:
                        switch (ch) {
                        case 'h':
                            info.type = SIZE_SHORT;
                            continue;

                        case 'l':
                            info.type = SIZE_LONG;
                            continue;

                        case 'L':
                        case 'H':
                            info.type = SIZE_HUGE;
                            continue;

                        case 'v':
                            info.type = SIZE_XVAL;
                            continue;

                        case 'O':
                            info.type = SIZE_OPCODE;
                            continue;

                        case 'P':
                            info.type = SIZE_PMC;
                            continue;

                        case 'S':
                            info.type = SIZE_PSTR;
                            continue;

                        default:
                            info.phase = PHASE_TERM;
                        }


                    /*@fallthrough@ */ case PHASE_TERM:
                        switch (ch) {
                            /* INTEGERS */
                        case 'c':
                            {
                            STRING * const ts = string_chr(interp,
                                 (UINTVAL)obj->getint(interp, info.type, obj));
                            targ = str_append_w_flags(interp, targ,
                                    &info, ts, NULL);
                            }
                            break;

                        case 'o':
                            {
                            const UHUGEINTVAL theuint = obj->getuint(interp, info.type, obj);
                            STRING * const ts = uint_to_str(interp, tc, theuint, 8, 0);
                            STRING * const prefix = CONST_STRING(interp, "0");
                            /* unsigned conversion - no plus */
                            info.flags &= ~FLAG_PLUS;
                            targ = str_append_w_flags(interp, targ,
                                    &info, ts, prefix);
                            }
                            break;

                        case 'x':
                            {
                            const UHUGEINTVAL theuint = obj->getuint(interp, info.type, obj);
                            STRING * const ts = uint_to_str(interp, tc, theuint, 16, 0);
                            STRING * const prefix = CONST_STRING(interp, "0x");
                            /* unsigned conversion - no plus */
                            info.flags &= ~FLAG_PLUS;
                            targ = str_append_w_flags(interp, targ,
                                    &info, ts, prefix);
                            }
                            break;

                        case 'X':
                            {
                            STRING * const prefix = CONST_STRING(interp, "0X");
                            const UHUGEINTVAL theuint =
                                obj->getuint(interp, info.type, obj);
                            STRING * const ts = uint_to_str(interp, tc, theuint, 16, 0);
                            string_upcase_inplace(interp, ts);
                            /* unsigned conversion - no plus */
                            info.flags &= ~FLAG_PLUS;
                            targ = str_append_w_flags(interp, targ,
                                    &info, ts, prefix);
                            }
                            break;

                        case 'b':
                            {
                            STRING * const prefix = CONST_STRING(interp, "0b");
                            const UHUGEINTVAL theuint =
                                obj->getuint(interp, info.type, obj);
                            STRING * const ts = uint_to_str(interp, tc, theuint, 2, 0);
                            /* unsigned conversion - no plus */
                            info.flags &= ~FLAG_PLUS;
                            targ = str_append_w_flags(interp, targ,
                                    &info, ts, prefix);
                            }
                            break;

                        case 'B':
                            {
                            STRING * const prefix = CONST_STRING(interp, "0B");
                            const HUGEINTVAL theint = obj->getint(interp, info.type, obj);
                            STRING * const ts = int_to_str(interp, tc, theint, 2);
                            /* unsigned conversion - no plus */
                            info.flags &= ~FLAG_PLUS;
                            targ = str_append_w_flags(interp, targ,
                                    &info, ts, prefix);
                            }
                            break;

                        case 'u':
                            {
                            const UHUGEINTVAL theuint =
                                obj->getuint(interp, info.type, obj);
                            sharedint = theuint;
                            }
                            goto do_sprintf;
                        case 'd':
                        case 'i':
                            /* EVIL: Work around bug in glibc that makes %0lld sometimes output an
                             * empty string. */
                            if (!(info.flags & FLAG_WIDTH))
                                info.flags &= ~FLAG_ZERO;
                            sharedint = obj->getint(interp, info.type, obj);
do_sprintf:
                            {
                            STRING *ts;
                            gen_sprintf_call(tc, &info, ch);
                            ts = cstr2pstr(tc);
                            {
                                char * const tempstr =
                                    string_to_cstring(interp, ts);

#ifdef PARROT_HAS_SNPRINTF
                                snprintf(tc, PARROT_SPRINTF_BUFFER_SIZE,
                                         tempstr, sharedint);
#else
                                /* the buffer is 4096, so no problem here */
                                sprintf(tc, tempstr, sharedint);
#endif
                                string_cstring_free(tempstr);
                            }
                            targ = string_append(interp, targ,
                                                 cstr2pstr(tc));
                            }
                            break;

                        case 'p':
                            {
                            STRING * const prefix = CONST_STRING(interp, "0x");
                            const void * const ptr = obj->getptr(interp, info.type, obj);
                            STRING * const ts = uint_to_str(interp, tc,
                                       (HUGEINTVAL) (size_t) ptr, 16, 0);

                            targ = str_append_w_flags(interp, targ,
                                    &info, ts, prefix);
                            }
                            break;

                            /* FLOATS - We cheat on these and use snprintf. */
                        case 'e':
                        case 'E':
                        case 'f':
                        case 'g':
                        case 'G':
                            {
                            STRING *ts;
                            const HUGEFLOATVAL thefloat = obj->getfloat(interp, info.type, obj);
                            /* turn -0.0 into 0.0 */
                            /* WTF if( thefloat == 0.0 ) { thefloat = 0.0; } */
                            gen_sprintf_call(tc, &info, ch);
                            ts = cstr2pstr(tc);
                            /* XXX lost precision if %Hg or whatever
                               */
                            {
                                char * const tempstr =
                                    string_to_cstring(interp, ts);

#ifdef PARROT_HAS_SNPRINTF
                                snprintf(tc, PARROT_SPRINTF_BUFFER_SIZE,
                                         tempstr,
                                         (double)thefloat);
#else
                                /* the buffer is 4096, so no problem here */
                                sprintf(tc, tempstr, (double)thefloat);
#endif
                                string_cstring_free(tempstr);
                            }

#ifdef WIN32
                            /* Microsoft defaults to three digits for exponents,
                             * even when fewer digits would suffice.  For the sake
                             * of portability, we will here attempt to hide that.
                             */

                            if (ch == 'g' || ch == 'G' || ch == 'e' || ch == 'E') {
                                const size_t tclen = strlen(tc);
                                size_t j;
                                for (j = 0; j < tclen; j++) {
                                    if ((tc[j] == 'e' || tc[j] == 'E')
                                        && (tc[j+1] == '+' || tc[j+1] == '-')
                                        && tc[j+2] == '0'
                                        && isdigit((unsigned char)tc[j+3])
                                        && isdigit((unsigned char)tc[j+4]))
                                    {
                                        mem_sys_memmove(&tc[j+2], &tc[j+3], strlen(&tc[j+2]));

                                        /* now fix the length if we just broke it */
                                        if ((info.flags & FLAG_WIDTH) && strlen(tc) < info.width) {
                                            if (info.flags & FLAG_MINUS)
                                                strcat(tc, " ");
                                            else {
                                                mem_sys_memmove(&tc[1], &tc[0], strlen(tc) + 1);
                                                tc[0] = (info.flags & FLAG_ZERO) ? '0' : ' ';
                                            }
                                        }

                                        /* only one fix required per string */
                                        break;
                                    }
                                }
                            }
#endif /* WIN32 */

                            targ = string_append(interp, targ,
                                                 cstr2pstr(tc));
                            }
                            break;

                            /* STRINGS */
                        case 'r':        /* Python repr */
                            /* XXX the right fix is to add a getrepr entry *
                             * to SPRINTF_OBJ, but for now, getstring_pmc  *
                             * is inlined and modified to call get_repr    */
                            if (obj->getstring == pmc_core.getstring) {
                                STRING *ts;
                                STRING *string;
                                PMC * const tmp =
                                    VTABLE_get_pmc_keyed_int(interp,
                                                             ((PMC *)obj->data),
                                                             (obj->index));

                                obj->index++;
                                string = (VTABLE_get_repr(interp, tmp));

                                ts = handle_flags(interp, &info, string,
                                    0, NULL);

                                targ = string_append(interp, targ, ts);

                                break;
                            }

                        case 's':
                          CASE_s:
                            {
                            STRING * const string = obj->getstring(interp, info.type, obj);

                            STRING * const ts = handle_flags(interp, &info, string,
                                    0, NULL);

                            targ = string_append(interp, targ, ts);
                            }
                            break;

                        default:
                            /* fake the old %P and %S commands */
                            if (info.type == SIZE_PMC
                                || info.type == SIZE_PSTR) {
                                i--;
                                goto CASE_s;
                                /* case 's' will see the SIZE_PMC or SIZE_PSTR
                                 * and assume it was %Ps (or %Ss).  Genius,
                                 * no?
                                 */
                            }
                            else {
                                real_exception(interp, NULL, INVALID_CHARACTER,
                                                   "'%c' is not a valid "
                                                   "sprintf format",
                                                   ch);
                            }
                        }

                        info.phase = PHASE_DONE;
                        break;

                    case PHASE_DONE:
                        /* This is the terminating condition of the surrounding
                         * loop, so...
                         */
                        PANIC(interp, "We can't be here");
                    }
                }
            }

            old = i;
            i--;
        }
        else {
            len++;
        }
    }
    if (len) {
        string_substr(interp, pat, old, len, &substr, 1);
        targ = string_append(interp, targ, substr);
    }

    return targ;
}

/*

=head1 SEE ALSO

F<src/misc.h>, F<src/misc.c>, F<src/spf_vtable.c>.

*/


/*
 * Local variables:
 *   c-file-style: "parrot"
 * End:
 * vim: expandtab shiftwidth=4:
 */
