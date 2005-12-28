=head1 NAME

SDL::Color - Parrot class representing colors in Parrot SDL

=head1 SYNOPSIS

	load_bytecode 'library/SDL/Color.pir'

	# create a new SDL::Color object
	.local pmc color
	.local int color_type

	find_type color_type, 'SDL::Color'
	color = new color_type

	# set the color values; this one's blue
	.local pmc color_args
	color_args = new PerlHash

	color_args[ 'r' ] =   0
	color_args[ 'g' ] =   0
	color_args[ 'b' ] = 255

	color.'_new'( color_args )

	# fetch the color value to pass directly to SDL functions
	# (you should never need to do this if the rest of the library works right)
	.local int color_value
	color_value = color.'color'()

=head1 DESCRIPTION

SDL::Color provides a very thin object layer over colors as used in SDL.  For
the most part, it's just a constructor and a convenient way to pass things
around.  In the future, it may grow more complex if you do odd things that
require depth conversions.  They make my head hurt now though.

This object represents a single color of, we'll say, 24-bit depth -- eight bits
per red, green, and blue component.  This may not match your requirements
exactly.  Per my understanding of SDL, though, it'll do the conversions for you
automatically.  Patches welcome.

=head1 METHODS

SDL::Color objects have the following methods:

=over 4

=cut

.namespace [ 'SDL::Color' ]

.sub _initialize  :load

	.local pmc color_class

	newclass color_class, 'SDL::Color'

	addattribute color_class, 'color'
	addattribute color_class, 'r'
	addattribute color_class, 'g'
	addattribute color_class, 'b'

	.local pmc initializer
	new initializer, .PerlString
	initializer = '_new'
	setprop      color_class, 'BUILD', initializer

.end

=item _new( color_args )

Initialize the new object with the necessary arguments,  The single argument,
C<color_args>, should be a C<PerlHash> PMC containing the following keys:

=over 4

=item C<r>

An integer from 0 to 255 representing the amount of red in the color, where 0
is none and 255 is the maximum possible.

=item C<g>

An integer from 0 to 255 representing the amount of green in the color, where 0
is none and 255 is the maximum possible.

=item C<b>

An integer from 0 to 255 representing the amount of blue in the color, where 0
is none and 255 is the maximum possible.

=back

The name of this method I<may> change, pending better ideas as discussed on
p6i.

=cut

.sub _new method
	.param pmc args

	.local int arg_color
	.local pmc color
	.local pmc red
	.local pmc green
	.local pmc blue

	.local int offset

	color     = new PerlInt
	red       = new PerlInt
	green     = new PerlInt
	blue      = new PerlInt

	arg_color = args['r']
	red       = arg_color
	arg_color = arg_color << 16
	add color, arg_color

	arg_color = args['g']
	green     = arg_color
	arg_color = arg_color <<  8
	add color, arg_color

	arg_color = args['b']
	blue      = arg_color
	add color, arg_color

	classoffset offset, self, 'SDL::Color'
	setattribute self, offset, color

	inc offset
	setattribute self, offset, red
	inc offset
	setattribute self, offset, green
	inc offset
	setattribute self, offset, blue

.end

=item color()

Returns an integer representing the actual color value passed to the underlying
SDL functions.  You should never need to use this directly, unless you need to
call those functions directly.

=cut

.sub color method

	.local int color
	.local int offset
	.local pmc color_value

	classoffset offset, self, 'SDL::Color'
	getattribute color_value, self, offset

	set color, color_value

	.pcc_begin_return
		.return color
	.pcc_end_return
.end

=item color_for_surface( surface )

Returns an integer representing the current color appropriate for the given
SDL::Surface C<surface>, 

As far as I understand things right now, you don't need to call this directly
unless you're manipulating raw pixels and need the boost of speed from
preconverting colors.  In that case, you probably I<do> need the speed boost.

The name of this method may change.

=cut

.sub color_for_surface method
	.param pmc surface

	.local pmc component
	.local int color

	.local int loss
	.local int shift_bits

	.local int offset
	classoffset offset, self, 'SDL::Color'

	color = 0

	# red
	inc offset
	getattribute component, self, offset

	.local int red
	red = component

	(loss, shift_bits) = surface.'convert_red'()
	shr red,   loss
	shl red,   shift_bits
	add color, red

	# green
	inc offset
	getattribute component, self, offset
	(loss, shift_bits) = surface.'convert_green'()

	.local int green
	green = component

	shr green, loss
	shl green, shift_bits
	add color, green

	# blue
	inc offset
	getattribute component, self, offset
	(loss, shift_bits) = surface.'convert_blue'()

	.local int blue
	blue = component

	shr blue,  loss
	shl blue,  shift_bits
	add color, blue

	.pcc_begin_return
		.return color
	.pcc_end_return
.end

=back

=head1 AUTHOR

Written and maintained by chromatic, E<lt>chromatic at wgz dot orgE<gt>, with
suggestions from Jens Rieks.  Please send patches, feedback, and suggestions to
the Perl 6 Internals mailing list.

=head1 COPYRIGHT

Copyright (c) 2004, The Perl Foundation.

=cut
