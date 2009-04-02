
# $Id$

=head1 NAME

SDL::EventHandler - base class for application-specific SDL event handlers

=head1 SYNOPSIS

    # load the event class and this library
    load_bytecode 'SDL/Event.pir'
    load_bytecode 'SDL/EventHandler.pir'

    # subclass this class
    .local pmc parent_class
    .local pmc class_type

    get_class parent_class, 'SDL::EventHandler'
    subclass class_type, parent_class, 'My::Event::Handler'

    # define your overridden methods
    .sub key_down_down :method
        .param pmc event
        .param pmc event_args

        # ...

    .end

    # create an event handler object
    .local pmc event_handler
    event_handler = new 'My::Event::Handler'

    # create and populate some event_arguments
    .local pmc event_args

    new event_args, 'Hash'
    event_args[ 'main_surface' ] = main_surface
    event_args[ 'sprite_list'  ] = sprites

    # create a new event object
    .local pmc event
    event = new 'SDL::Event'

    # ... and process events
    event.'process_events'( event_handler, handler_args )

=head1 DESCRIPTION

SDL::EventHandler is a parent class for all event handlers in SDL Parrot
programs.  Subclass this class and override the methods that correspond to the
events you want to handle.

=head1 METHODS

SDL::EventHandler provides the following methods:

=over 4

=cut

.namespace [ 'SDL::EventHandler' ]

.sub _initialize :load
    .local pmc   handler_class

    newclass     handler_class, 'SDL::EventHandler'
    addattribute handler_class, 'args'

    .return()
.end

# not documented now; could set default args here
# maybe nice, maybe not

.sub 'init' :method
    .param pmc args

    setattribute self, 'args', args
.end

=item key_down( event, event_args )

Dispatches a key down event to the appropriate C<key_down_I<keyname>> method,
if one exists.  Otherwise, does nothing.

Override this if you want to change the way dispatching happens or to do
something different for I<all> key down events.  In general, you will probably
want to override the C<key_down_*> methods instead.

XXX The I<*> above i.e. the actually key_name isn't really documented.
But:

  $ perldoc SDL::Event

might be helpful (for now), if you remove I<SDLK_> and lowercase the
remainder - sorry.

  .sub key_down_q         # 'q' key
  .sub key_down_down      # <down> arrow key
  .sub key_down_kp_plus   # <keypad-plus> key
  ...

=cut

.sub key_down :method
    .param pmc event
    .param pmc event_args

    .local string key_name
    key_name = event.'event_keyname'()

    .local string key_method_name
    key_method_name = 'key_down_'

    concat key_method_name, key_name

    .local int can_handle
    can can_handle, self, key_method_name

    eq can_handle, 0, _return
    self.key_method_name( event_args )

_return:

.end

=item key_up( event, event_args )

Dispatches a key up event to the appropriate C<key_upI<keyname>> method, if one
exists.  Otherwise, does nothing.

Override this if you want to change the way dispatching happens or to do
something different for I<all> key up events.  In general, you will probably
want to override the C<key_up_*> methods instead.

=back

=cut

.sub key_up :method
    .param pmc event
    .param pmc event_args

    .local string key_name
    key_name = event.'event_keyname'()

    .local string key_method_name
    key_method_name = 'key_up_'

    concat key_method_name, key_name

    .local int can_handle
    can can_handle, self, key_method_name

    eq can_handle, 0, _return
    self.key_method_name( event_args )

_return:

.end

=head2 Key Event Methods

Key event methods have names of the form C<key_I<eventtype>_I<keyname>>.  That
is, to handle a key down event for the Escape key, override the method
C<key_down_escape>.  Key names follow the SDL naming convention, except that
key names are in all lowercase.

Unless you override C<key_down> or C<key_up> and do something different, all of
these methods will receive one argument, the C<event_args> hash passed to
C<SDL::Event::wait_event()>.  Use this hash to store and to retrieve data
between events, particularly your main surface and any sprites or other
surfaces.

=head2 Other Event Methods

In addition, you can override the following methods to handle their event
types.

=over 4

=item * active_event

=item * mouse_motion

=item * mouse_button_down

=item * mouse_button_up

Synopsis for mouse event handler:

  .sub mouse_button_up :method
    .param pmc event
    .param pmc args

    .local int b, x, y
    event = event.'event'( 'MouseButton' )
    b = event['state']    # 1 = left, 2 = middle, 3 = right
    x = event['x']
    y = event['y']
    ...

=item * joy_axis_motion

=item * joy_ball_motion

=item * joy_hat_motion

=item * joy_button_down

=item * joy_button_up

=item * quit

=item * sys_wm_event

=item * video_resize

=item * video_expose

=item * event_reserved_a

=item * event_reserved_b

=item * event_reserved_2

=item * event_reserved_3

=item * event_reserved_4

=item * event_reserved_5

=item * event_reserved_6

=item * event_reserved_7

=back

By default, these methods do nothing.  They all take two arguments, the
SDL::Event object representing the incoming event and the C<event_args> hash
storing data between events.

At the very least, you should override C<quit()>.

=cut

.sub active_event :method
    .param pmc event
    .param pmc event_args

.end

.sub mouse_motion :method
    .param pmc event
    .param pmc event_args

.end

.sub mouse_button_down :method
    .param pmc event
    .param pmc event_args

.end

.sub mouse_button_up :method
    .param pmc event
    .param pmc event_args
.end

.sub joy_axis_motion :method
    .param pmc event
    .param pmc event_args

.end

.sub joy_ball_motion :method
    .param pmc event
    .param pmc event_args

.end

.sub joy_hat_motion :method
    .param pmc event
    .param pmc event_args

.end

.sub joy_button_down :method
    .param pmc event
    .param pmc event_args

.end

.sub joy_button_up :method
    .param pmc event
    .param pmc event_args

.end

.sub quit :method
    .param pmc event
    .param pmc event_args

.end

.sub sys_wm_event :method
    .param pmc event
    .param pmc event_args

.end

.sub event_reserved_a :method
    .param pmc event
    .param pmc event_args

.end

.sub event_reserved_b :method
    .param pmc event
    .param pmc event_args

.end

.sub video_resize :method
    .param pmc event
    .param pmc event_args

.end

.sub video_expose :method
    .param pmc event
    .param pmc event_args

.end

.sub event_reserved_2 :method
    .param pmc event
    .param pmc event_args

.end

.sub event_reserved_3 :method
    .param pmc event
    .param pmc event_args

.end

.sub event_reserved_4 :method
    .param pmc event
    .param pmc event_args

.end

.sub event_reserved_5 :method
    .param pmc event
    .param pmc event_args

.end

.sub event_reserved_6 :method
    .param pmc event
    .param pmc event_args

.end

.sub event_reserved_7 :method
    .param pmc event
    .param pmc event_args

.end

=head1 AUTHOR

Written and maintained by chromatic, E<lt>chromatic at wgz dot orgE<gt>.
Designed by Allison Randal.  Please send patches, feedback, and suggestions to
the Perl 6 Internals mailing list.

=head1 COPYRIGHT

Copyright (C) 2004-2008, Parrot Foundation.

=cut

# Local Variables:
#   mode: pir
#   fill-column: 100
# End:
# vim: expandtab shiftwidth=4 ft=pir:
