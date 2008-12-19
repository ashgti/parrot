use v6;

=begin pod

=head1 TITLE

shapes.p6 - Exercise basic OpenGL 1.1/GLUT 3 APIs by drawing animated shapes

=head1 SYNOPSIS

    $ cd parrot-home
    $ make perl6
    $ cd parrot-home/runtime/parrot/library
    $ ../../../perl6 ../../../examples/opengl/shapes.p6

=head1 DESCRIPTION

NOTE: The conversion from F<shapes.pir> is still in progress; much is not
working yet.

This example is slightly more complex than F<triangle.p6>, and exercises more
of the OpenGL 1.1 and GLUT 3 APIs.  It is also a better behaved application,
correctly responding to reshape events, pausing on minimize, and so on.  It is
a fairly direct translation of f<shapes.pir> to Perl 6.

To quit the example, press C<Q> or the C<ESCAPE> key, or close the window using
your window manager (using the X in the corner of the window title bar, for
example).  To pause or restart the animation, press any other ASCII key.

=end pod


use OpenGL;
use NCI::call_toolkit_init;

# .include 'datatypes.pasm'

# .include 'opengl_defines.pasm'
our $GLUT_RGBA              = 0x0000;
our $GLUT_DOUBLE            = 0x0002;
our $GLUT_DEPTH             = 0x0010;
our $GLUT_STENCIL           = 0x0020;

our $GL_DEPTH_BUFFER_BIT    = 0x0100;
our $GL_STENCIL_BUFFER_BIT  = 0x0400;
our $GL_COLOR_BUFFER_BIT    = 0x4000;

our $GL_FALSE               = 0x0000;
our $GL_TRUE                = 0x0001;
our $GL_TRIANGLES           = 0x0004;
our $GL_EQUAL               = 0x0202;
our $GL_ALWAYS              = 0x0207;
our $GL_SRC_ALPHA           = 0x0302;
our $GL_ONE_MINUS_SRC_ALPHA = 0x0303;
our $GL_FRONT               = 0x0404;
our $GL_BACK                = 0x0405;
our $GL_DEPTH_TEST          = 0x0B71;
our $GL_STENCIL_TEST        = 0x0B90;
our $GL_NORMALIZE           = 0x0BA1;
our $GL_BLEND               = 0x0BE2;
our $GL_MODELVIEW           = 0x1700;
our $GL_PROJECTION          = 0x1701;
our $GL_KEEP                = 0x1E00;
our $GL_REPLACE             = 0x1E01;


our $mode        = $GLUT_DOUBLE +| $GLUT_RGBA +| $GLUT_DEPTH +| $GLUT_STENCIL;
our $glut_window = init_glut($mode, 'Test', @*ARGS);

our $aspect      = 1.0;
our $frames      = 0;
our $paused      = 0;

our $time_prev   = time();
our $time_curr   = $time_prev;
our $time_sim    = 0.0;
our $time_sim_dt = 0.0;

glutIdleFunc(     &idle     );
glutDisplayFunc(  &draw     );
# glutReshapeFunc(  &reshape  );
glutKeyboardFunc( &keyboard );

glutMainLoop();


sub init_glut($display_mode, $window_title, @args is rw) {
    # Import OpenGL and GLUT functions
    OpenGL::_export_all_functions();

    # Set larger default window size
    glutInitWindowSize(500, 500);

    # Initialize GLUT, fixup command line args
    @args = NCI::call_toolkit_init(&glutInit, @args);

    # Set display mode, create GLUT window, return window handle
    glutInitDisplayMode($GLUT_DOUBLE +| $GLUT_RGBA +| $GLUT_DEPTH +| $GLUT_STENCIL);

    return glutCreateWindow($window_title);
}

sub idle {
    $time_prev = $time_curr;
    $time_curr = time();

    my $dt = $paused ?? 0 !! $time_curr - $time_prev;

    $time_sim_dt = $dt;
    # XXXX: Broken because of Parrot RT #60036
    # $time_sim   += $dt;
    $time_sim    = $dt + $time_sim;

    glutPostRedisplay() unless $paused;
}

sub reshape($width, $height is copy) {
    $height = $height || 1;
    $aspect = $width / $height;

    # say "HELLO1";
    glViewport(0, 0, $width, $height);
    say "HELLO2";
}

sub keyboard($key, $x, $y) {
    # For ESCAPE, 'Q', and 'q', exit program
    if ($key == 27 | 81 | 113) {
        glutDestroyWindow($glut_window);

        say "FPS: { $frames / ($time_sim || .001) }";
    }
    # For all other keys, just toggle pause
    else {
        $paused = !$paused;
    }
}

sub draw {
    glClear($GL_COLOR_BUFFER_BIT +| $GL_DEPTH_BUFFER_BIT +| $GL_STENCIL_BUFFER_BIT);

    set_3d_view();
    update_particle_effect();
    draw_reflected_scene();
    draw_main_scene();
    set_2d_view();

    glutSwapBuffers();
    $frames++;
}

sub set_3d_view {
    # Simple 60 degree FOV perspective view
    glMatrixMode($GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(60, $aspect, 1, 100);

    # Look at origin from (0,2,4), with +Y up
    glMatrixMode($GL_MODELVIEW);
    glLoadIdentity();
    gluLookAt(0, 2, 4, 0, 0, 0, 0, 1, 0);

    # Rotate view around origin, to see objects from all angles
    my $angle = ($time_sim * -24) % 360;
    glRotatef($angle, 0, 1, 0);
}

sub draw_reflected_scene {
    # First, make a stencil of the floor, so that reflected scene
    # doesn't "leak out" of the reflective area

    # Turn off everything we don't need
    glDisable($GL_DEPTH_TEST);
    glColorMask($GL_FALSE, $GL_FALSE, $GL_FALSE, $GL_FALSE);

    # Set stencil for just the reflecting area
    glEnable($GL_STENCIL_TEST);
    glStencilOp($GL_REPLACE, $GL_REPLACE, $GL_REPLACE);
    glStencilFunc($GL_ALWAYS, 1, 0xffffffff);

    # Draw the floor as the reflector
    draw_floor();

    # Now only draw where stencil is set
    glStencilOp($GL_KEEP, $GL_KEEP, $GL_KEEP);
    glStencilFunc($GL_EQUAL, 1, 0xffffffff);

    # Turn back on the stuff we turned off
    glEnable($GL_DEPTH_TEST);
    glColorMask($GL_TRUE, $GL_TRUE, $GL_TRUE, $GL_TRUE);

    # Flip reflection through the reflector
    glPushMatrix();
    glScalef(1, -1, 1);

    # Account for the reversed normals
    glEnable($GL_NORMALIZE);
    glCullFace($GL_FRONT);

    # Lights need to be reassigned after reflection
    set_lights();

    # Draw the reflected objects
    draw_objects();

    # Switch back to normal facing
    glDisable($GL_NORMALIZE);
    glCullFace($GL_BACK);
    glPopMatrix();

    # Done with stencil
    glDisable($GL_STENCIL_TEST);
}

sub draw_main_scene {
    #Draw floor blended over reflected scene
    glEnable($GL_BLEND);
    glBlendFunc($GL_SRC_ALPHA, $GL_ONE_MINUS_SRC_ALPHA);

    draw_floor();

    # Done with blending
    glDisable($GL_BLEND);

    # Set lights for upright view
    set_lights();

    # Draw objects (now in upright orientation)
    draw_objects();
}

sub draw_floor {
    # Partially transparent grey (so that reflection shows through)
    glColor4f(.7, .7, .7, .7);

    # Rotate quadric from +Z up to +Y up
    glPushMatrix();
    glRotatef(90, 1, 0, 0);

    # Annulus floor (shapes sit in various spots above it)
#     my $glu_quadric = gluNewQuadric();
#     gluDisk($glu_quadric, 1, 2, 128, 1);
#     gluDeleteQuadric($glu_quadric);

    glPopMatrix();
}

sub set_lights {
}

sub draw_objects {
    draw_rgb_triangle();
    draw_lit_teapot();
    draw_particle_effect();
}

sub draw_rgb_triangle {
    # Unlit spinning RGB triangle at -Z

    my $angle = ($time_sim * 45) % 360;

    glPushMatrix();
    glTranslatef(0, 0.04, -1.5);
    glRotatef($angle, 0, 1, 0);

    glBegin($GL_TRIANGLES);
    glColor3f(1, 0, 0); glVertex3f(-.5, 0, 0);
    glColor3f(0, 1, 0); glVertex3f( .5, 0, 0);
    glColor3f(0, 0, 1); glVertex3f(0  , 1, 0);
    glEnd();

    glPopMatrix();
}

sub draw_lit_teapot {
}

sub update_particle_effect {
}

sub draw_particle_effect {
}

sub set_2d_view {
    glMatrixMode($GL_PROJECTION);
    glLoadIdentity();

    glMatrixMode($GL_MODELVIEW);
    glLoadIdentity();
}


=begin pod

.sub main :main
    # Init miscellaneous globals
    init_globals()

    # Create particle effect
    init_particle_effect()
.end

.sub init_globals
    # Create structure definition for float4 structure
    .local pmc float4
    float4 = new 'ResizablePMCArray'
    push float4, .DATATYPE_FLOAT
    push float4, 0
    push float4, 0
    push float4, .DATATYPE_FLOAT
    push float4, 0
    push float4, 0
    push float4, .DATATYPE_FLOAT
    push float4, 0
    push float4, 0
    push float4, .DATATYPE_FLOAT
    push float4, 0
    push float4, 0
    set_global 'float4', float4
.end

.sub init_particle_effect
    .local pmc pfx_pos, pfx_vel
    pfx_pos = new 'ResizablePMCArray'
    pfx_vel = new 'ResizablePMCArray'

    new_particle(0, pfx_pos, pfx_vel)

    set_global 'pfx_pos', pfx_pos
    set_global 'pfx_vel', pfx_vel
.end

.sub set_lights
    glEnable(.GL_LIGHT0)

    # Light above origin
    .local pmc float4, position
    float4   = get_global 'float4'
    position = new 'ManagedStruct', float4
    position[0] = 0.0
    position[1] = 2.0
    position[2] = 0.0
    position[3] = 1.0

    glLightfv(.GL_LIGHT0, .GL_POSITION, position)
.end

.sub draw_lit_teapot
    # Lit cyan teapot at +X

    glPushMatrix()
    glTranslatef(1.5, .4, 0)
    glRotatef(90, 0, 1, 0)

    glEnable(.GL_LIGHTING)

    .local pmc float4, color
    float4 = get_global 'float4'
    color  = new 'ManagedStruct', float4
    color[0] = 0.0
    color[1] = 0.8
    color[2] = 0.8
    color[3] = 1.0
    glMaterialfv(.GL_FRONT, .GL_AMBIENT_AND_DIFFUSE, color)

    color[0] = 1.0
    color[1] = 1.0
    color[2] = 1.0
    color[3] = 1.0
    glMaterialfv(.GL_FRONT, .GL_SPECULAR,  color)
    glMaterialf (.GL_FRONT, .GL_SHININESS, 64)

    glutSolidTeapot(.5)

    glDisable(.GL_LIGHTING)

    glPopMatrix()
.end

.sub new_particle
    .param int particle_num
    .param pmc pfx_pos
    .param pmc pfx_vel

    .local num x, y, z, vx, vy, vz
    x  = 4.0
    y  = 0.0
    z  = 0.0

    vx = 0.0
    vy = 0.135
    vz = 0.0

    .local pmc random
    .local num rand
    random = new 'Random'
    rand  = random
    rand *= .1
    x    += rand
    rand  = random
    rand *= .1
    y    += rand
    rand  = random
    rand *= .1
    z    += rand

    rand  = random
    rand -= .5
    rand *= .01
    vx   += rand
    rand  = random
    rand -= .5
    rand *= .01
    vy   += rand
    rand  = random
    rand -= .5
    rand *= .01
    vz   += rand

    .local pmc pos
    pos = new 'FixedFloatArray'
    pos = 3
    pos[0] = x
    pos[1] = y
    pos[2] = z

    .local pmc vel
    vel = new 'FixedFloatArray'
    vel = 3
    vel[0] = vx
    vel[1] = vy
    vel[2] = vz

    pfx_pos[particle_num] = pos
    pfx_vel[particle_num] = vel
.end

.sub update_particle_effect
    .param num dt

    # "Black hole" particle effect at +Z

    # Speed up time a little; this effect is *slow*
    dt *= 30.0

    # Global particle state arrays
    .local pmc pfx_pos, pfx_vel
    pfx_pos = get_global 'pfx_pos'
    pfx_vel = get_global 'pfx_vel'

    # Add a particle at random
    .local int count
    count = pfx_pos
    if count > 1000 goto update_particles
    .local pmc random
    .local num rand
    random = new 'Random'
    rand   = random
    rand  *= 4
    if rand > dt goto update_particles
    new_particle(count, pfx_pos, pfx_vel)

    # Update all particles
  update_particles:
    dec count
    if count < 0 goto update_particles_end

    # Update particle states
    update_particle(pfx_pos, pfx_vel, count, dt)

    goto update_particles
  update_particles_end:
.end

.sub update_particle
    .param pmc pfx_pos
    .param pmc pfx_vel
    .param int particle_num
    .param num dt

    .local pmc pos, vel
    pos = pfx_pos[particle_num]
    vel = pfx_vel[particle_num]

    # Constants
    .const num G           = -.075   # Gravitational force constant
    .const num Cd          = -.00033 # Coefficient of drag
    .const num event_grav  = -.3     # Gravity at "event horizon"
    .const num min_dist2   =  .001   # Minimum distance**2 before calc blows up
    .const num escape_dist = 30      # Distance at which "escape" occurs

    # Particle states
    .local num x, y, z, vx, vy, vz
    x  = pos[0]
    y  = pos[1]
    z  = pos[2]
    vx = vel[0]
    vy = vel[1]
    vz = vel[2]

    # Calculate distance and distance squared
    .local num x2, y2, z2, dist2, dist
    x2     = x * x
    y2     = y * y
    z2     = z * z
    dist2  = x2 + y2
    dist2 += z2
    if dist2 >= min_dist2 goto dist2_ok
    dist2  = min_dist2
  dist2_ok:
    dist   = sqrt dist2

    # If distance is too great, particle has "escaped"; regenerate it
    if dist < escape_dist goto dist_ok
    new_particle(particle_num, pfx_pos, pfx_vel)
    .return ()
  dist_ok:

    # Compute gravity force
    .local num grav
    grav = G / dist2

    # If gravity is too strong, it has "passed the event horizon"; regenerate it
    if grav > event_grav goto grav_ok
    new_particle(particle_num, pfx_pos, pfx_vel)
    .return ()
  grav_ok:

    # Calculate gravity vector (always directed toward center of "hole")
    .local num gx, gy, gz
    gx  = x / dist
    gy  = y / dist
    gz  = z / dist
    gx *= grav
    gy *= grav
    gz *= grav

    # Calculate drag vector (always directed opposite of velocity)
    # NOTE: Using drag proportional to velocity, instead of velocity squared
    .local num dragx, dragy, dragz
    dragx = Cd * vx
    dragy = Cd * vy
    dragz = Cd * vz

    # Acceleration is gravity + drag
    .local num ax, ay, az
    ax = gx + dragx
    ay = gy + dragy
    az = gz + dragz

    # Update velocity and position with simple Euler integration
    .local num dvx, dvy, dvz
    .local num  dx,  dy,  dz
    dvx  = ax * dt
    dvy  = ay * dt
    dvz  = az * dt
    vx  += dvx
    vy  += dvy
    vz  += dvz
    dx   = vx * dt
    dy   = vy * dt
    dz   = vz * dt
    x   += dx
    y   += dy
    z   += dz

    # Save new values back to particle state
    vel[0] = vx
    vel[1] = vy
    vel[2] = vz
    pos[0] = x
    pos[1] = y
    pos[2] = z
.end

.sub draw_particle_effect
    # "Black hole" particle effect at +Z

    # Make it visually interesting
    glPushMatrix()
    glTranslatef(0, .3, 1.5)
    glRotatef(-20, 0, 0, 1)
    glRotatef( 90, 1, 0, 0)
    glScalef(.15, .15, .15)

    # OpenGL state for "glowing transparent particles"
    glEnable(.GL_BLEND)
    glBlendFunc(.GL_SRC_ALPHA, .GL_ONE)
    glDepthMask(.GL_FALSE)
    glPointSize(4)

    # XXXX: Disabled for now because it's broken on many systems;
    #       will replace later with textured quads
    # glEnable(.GL_POINT_SMOOTH)

#     # Show plane of effect
#     glColor4f(1, 1, 1, .2)
#     glBegin(.GL_QUADS)
#     glVertex3f(-2, -2, 0)
#     glVertex3f( 2, -2, 0)
#     glVertex3f( 2,  2, 0)
#     glVertex3f(-2,  2, 0)
#     glEnd()

    # Start drawing particles
    glColor4f(1, 1, 1, .5)
    glBegin(.GL_POINTS)

    # Global particle state arrays
    .local pmc pfx_pos, pfx_vel
    pfx_pos = get_global 'pfx_pos'
    pfx_vel = get_global 'pfx_vel'

    # Loop over all particles, updating and drawing them
    .local int count
    count = pfx_pos
  draw_particle_loop:
    dec count
    if count < 0 goto draw_particle_loop_end

    # Draw particle
    .local pmc pos
    .local num x, y, z
    pos = pfx_pos[count]
    x   = pos[0]
    y   = pos[1]
    z   = pos[2]
    glVertex3f(x, y, z)

    goto draw_particle_loop

    # Finished with all particles
  draw_particle_loop_end:
    glEnd()

    # Done, return to normal OpenGL state
    glDepthMask(.GL_TRUE)
    glDisable(.GL_BLEND)

    glPopMatrix()
.end


=end pod

# Local Variables:
#   mode: pir
#   fill-column: 100
# End:
# vim: expandtab shiftwidth=4 ft=perl6:
