# Copyright (C) 2005-2008, Parrot Foundation.
# $Id$

=head1 NAME

examples/pir/mandel.pir - Print the Mandelbrot set

=head1 SYNOPSIS

    % ./parrot examples/pir/mandel.pir

=head1 DESCRIPTION

This prints an ASCII-art representation of the Mandelbrot set.

Translated from C code by Glenn Rhoads into Parrot assembler
by Leon Brocard <acme@astray.com>.
Translated from PASM to PIR by Bernhard Schmalhofer.

The C code is:

    main() {
        int x, y, k;
        char *b = " .:,;!/>)|&IH%*#";
        float r, i, z, Z, t, c, C;
        for (y=30; puts(""), C = y*0.1 - 1.5, y--;) {
            for (x=0; c = x*0.04 - 2, z=0, Z=0, x++ < 75;) {
                for (r=c, i=C, k=0; t = z*z - Z*Z + r, Z = 2*z*Z + i, z=t, k<112; k++)
                    if (z*z + Z*Z > 10) break;
                printf ("%c", b[k%16]);
            }
        }
    }

=cut

.sub 'main' :main

        .local string b
        .local int    x, y, k
        .local num    r, i, z, Z, t, c, C

        b = " .:,;!/>)|&IH%*#"
        y = 30

YREDO:  #  C = y*0.1 - 1.5
        C = y * 0.1
        C -= 1.5

        x = 0

XREDO:  # c = x*0.04 - 2
        c = x * 0.04
        c -= 2
        z = 0
        Z = 0

        r = c
        i = C

        k = 0

KREDO:  # t = z*z - Z*Z + r
        $N1 = z * z
        $N2 = Z * Z
        t = $N1 - $N2
        t += r

        # Z = 2*z*Z + i
        Z = 2 * Z
        Z = z * Z
        Z += i

        # z = t
        z = t

        # if (z*z + Z*Z > 10) break;
        $N1 = z * z
        $N2 = Z * Z
        $N1 += $N2
        if $N1 > 10 goto PRINT

        inc k
        if k < 112 goto KREDO

PRINT:  $I1 = k % 16
        $S1 = substr b, $I1, 1
        print $S1

        inc x
        if x < 75 goto XREDO

        print "\n"
        dec y
        if y > 0 goto YREDO
.end

# Local Variables:
#   mode: pir
#   fill-column: 100
# End:
# vim: expandtab shiftwidth=4 ft=pir:
