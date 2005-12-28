=head1 NAME

SDL::Event - Parrot extension representing SDL Events

=head1 SYNOPSIS

	# load this library
	load_bytecode 'library/SDL/Event.pir'

	# create a new SDL::Event object
	.local pmc event
	.local int event_type

	find_type event_type, 'SDL::Event'
	event = new event_type

	# ... create a new event_handler and its hander_args

	# start the event loop
	event.'process_events'( event_handler, handler_args )

	# or handle one event at a time in your own loop
	event.'handle_event'( event_handler, handler_args )

=head1 DESCRIPTION

SDL::Event encapsulates event information for SDL.  This may change internally
as we migrate to Parrot events, but most of the interesting information happens
in SDL::EventHandler instead, so go read that first.

You may discover that you don't really care about this class beyond what you've
already learned from the SYNOPSIS already.

=head1 METHODS

The SDL::Event object has the following methods:

=over 4

=cut

.namespace [ 'SDL::Event' ]

.sub _initialize :load
	.local pmc   event_class

	newclass     event_class, 'SDL::Event'
	addattribute event_class, 'event'

	.local pmc initializer
	new initializer, .PerlString
	set initializer, 'BUILD'
	setprop event_class, 'BUILD', initializer

.end

=item BUILD()

Initializes the internal attributes of this object.  Trust me, you need to do
this, at least for now.

The name of this method may change, per discussion on p6i.  It may even go
away.

=cut

.sub BUILD method

	.local pmc  fetch_layout
	find_global fetch_layout, 'SDL::NCI', 'fetch_layout'

	.local pmc layout
	layout = fetch_layout( 'Event::Generic' )

	.local pmc event
	new event, .ManagedStruct, layout

	.local int offset
	classoffset offset, self, 'SDL::Event'
	setattribute self, offset, event

.end

=item event()

Returns the underlying C<SDL_Event> structure.  You probably don't need to use
this directly, unless you're working with raw SDL calls.

=cut

.sub event method
	.param string name :optional
	.param int argcS   :opt_flag 
	
        .local int offset
        classoffset offset, self, 'SDL::Event'

        .local pmc event
        getattribute event, self, offset

	if argcS == 0 goto END

	.local pmc  fetch_layout
	find_global fetch_layout, 'SDL::NCI', 'fetch_layout'

	.local pmc layout
	.local string ename
	
	ename = 'Event::'
	concat ename, name

	layout = fetch_layout( ename )
	#new event, .ManagedStruct, layout
	assign event, layout

	.local int offset
	classoffset offset, self, 'SDL::Event'
	setattribute self, offset, event

END:
	.pcc_begin_return
		.return event
	.pcc_end_return

.end

=item event_type( event_type )

Given the number of an incoming event type, returns a string name representing
that event.  The possible event names live in the C<SDL_Event.h> library;
they're all lower cased here.  For example, a key down event will have the name
C<key_down>.

If you pass an unknown event type identifier, you'll receive a string
containing C<unknown>.

B<Note:>  Arguably, this should operate on the current event, instead of
requiring an argument.  This may change in the future.

=cut

.sub event_type method
	.param int incoming_type

	.local pmc event_types
	event_types = new OrderedHash

	event_types[  0 ] = 'no_event'
	event_types[  1 ] = 'active_event'
	event_types[  2 ] = 'key_down'
	event_types[  3 ] = 'key_up'
	event_types[  4 ] = 'mouse_motion'
	event_types[  5 ] = 'mouse_button_down'
	event_types[  6 ] = 'mouse_button_up'
	event_types[  7 ] = 'joy_axis_motion'
	event_types[  8 ] = 'joy_ball_motion'
	event_types[  9 ] = 'joy_hat_motion'
	event_types[ 10 ] = 'joy_button_down'
	event_types[ 11 ] = 'joy_button_up'
	event_types[ 12 ] = 'quit'
	event_types[ 13 ] = 'sys_wm_event'
	event_types[ 14 ] = 'event_reserved_a'
	event_types[ 15 ] = 'event_reserved_b'
	event_types[ 16 ] = 'video_resize'
	event_types[ 17 ] = 'video_expose'
	event_types[ 18 ] = 'event_reserved_2'
	event_types[ 19 ] = 'event_reserved_3'
	event_types[ 20 ] = 'event_reserved_4'
	event_types[ 21 ] = 'event_reserved_5'
	event_types[ 22 ] = 'event_reserved_6'
	event_types[ 23 ] = 'event_reserved_7'

	.local string type_name
	.local int known_type

	type_name         = 'unknown'
	exists known_type, event_types[ incoming_type ]
	eq known_type, 0, return

	type_name         = event_types[ incoming_type ]

return:

	.pcc_begin_return
		.return type_name
	.pcc_end_return

.end

=item event_keyname()

Returns a string representing the name of the key for the current event.  Note
that the list of available keys is rather limited at the moment.  There's also
no error checking that the current event is even a key event.

On the other hand, you probably should define an event handler instead of using
these methods directly.

If this method does not recognize the current key, it will return the string
C<unknown> instead.

=cut

.sub event_keyname method

	.local pmc event
	event = self.'event'( 'Keyboard' )

	.local int key_sym
	key_sym = event['sym']

	.local pmc key_names
	find_global key_names, 'SDL::Constants', 'key_names'

	.local string key_name

	key_name = key_names[ key_sym ]
	length $I0, key_name
	if $I0 > 0 goto return
	key_name = 'unknown'
return:

	.pcc_begin_return
		.return key_name
	.pcc_end_return

.end

=item process_events( event_handler, handler_args, [ check_interval ] )

Given an C<event_handler> object (either subclassing C<SDL::EventHandler> or
implementing its necessary methods) and a C<PerlHash> of arguments to pass to
all event handlers, loops around SDL's event loop and calls appropriate methods
in C<event_handler> corresponding to what happens.

That is, if SDL reports a key down, this will call the C<key_down> method of
C<event_handler>, passing C<handler_args>.

If you pass a numeric value as C<check_interval>, this will poll for events
every C<check_interval> seconds.  This makes it possible to check for events in
your main thread, without breaking Parrot's own event loop (used for timers,
for example).  This is an optional parameter and can be any number of seconds
or fractions thereof.

For now, this will return if it detects a quit event.  This may not be what you
want and it may change in the future.  It won't return otherwise.  Arguably, it
should, somehow.

=cut

.sub process_events method
	.param pmc event_handler
	.param pmc handler_args
	.param num check_interval :optional
	.param int argcN          :opt_flag

	.local int polling

	.local int continue
	.local pmc GetEvent
	.local pmc event

	if argcN == 0 goto use_wait

	if check_interval < 0.0001 goto use_wait
	polling  = 1
	GetEvent = find_global 'SDL::NCI', 'PollEvent'

	branch loop

use_wait:
	polling  = 0
	GetEvent = find_global 'SDL::NCI', 'WaitEvent'

loop:
	event    = self.'event'( 'Generic' )
	continue = GetEvent( event )

	if continue goto dispatch
	unless polling goto loop
	
	# give parrot a chance to process its own events
	sleep check_interval
	branch loop

dispatch:
	continue = self.'dispatch_event'( event, event_handler, handler_args )
	if continue goto loop

.end

=item handle_event( event_handler, handler_args )

Given an C<event_handler> object (either subclass L<SDL::EventHandler> or
reimplement the appropriate methods yourself) and a C<PerlHash> PMC of data to
pass to all handler calls, handles I<one> event, if any are present.  If there
are no events pending, this will return directly.

This returns nothing interesting.  Perhaps it should return true or false
depending on whether it handled an event.  Perhaps not.

Use this method inside your own loop structure.

=cut

.sub handle_event method
	.param pmc event_handler
	.param pmc handler_args

	.local pmc event
	event = self.'event'()

	.local pmc  PollEvent
	find_global PollEvent, 'SDL::NCI', 'PollEvent'

	.local int event_waiting
	event_waiting = PollEvent( event )

	eq event_waiting, 0, return

	self.'dispatch_event'( event, event_handler, handler_args )

return:

.end

=item dispatch_event( event, event_handler, handler_args )

Dispatches an event.  You shouldn't call this directly.  Instead, call
C<handle_event()> or C<process_events()>.

=cut

.sub dispatch_event method
	.param pmc event
	.param pmc event_handler
	.param pmc handler_args

	.local int incoming_type
	incoming_type = event[ 'type' ]

	.local string event_type
	event_type = self.'event_type'( incoming_type )

	.local int continue
	continue = 0

	eq event_type, 'quit', return
	continue = 1

	.local int can_handle
	can can_handle, event_handler, event_type

	eq can_handle, 0, return

	# this is a method call using a method name in the event_type string!
	event_handler.event_type( self, handler_args )

return:

	.pcc_begin_return
		.return continue
	.pcc_end_return
.end

=back

=head1 AUTHOR

Written and maintained by chromatic, E<lt>chromatic at wgz dot orgE<gt>, with
some design from Allison Randal.  Please send patches, feedback, and
suggestions to the Perl 6 Internals mailing list.

=head1 COPYRIGHT

Copyright (c) 2004, The Perl Foundation.

=cut
