=head1 TITLE

anim_image_dblbuf.pir - animate an image in a doublebuffered Parrot SDL window

=head1 SYNOPSIS

To run this file, run the following command from the Parrot root directory:

	$ ./parrot examples/sdl/anim_image_dblbuf.pir
	Drew 540 frames in 2.200484 seconds (245.400580 fps)
	$ 

=cut

.sub _main non_prototyped, :main
	load_bytecode "library/SDL/App.pir"
	load_bytecode "library/SDL/Color.pir"
	load_bytecode "library/SDL/Rect.pir"
	load_bytecode "library/SDL/Image.pir"
	load_bytecode "library/SDL/Sprite.pir"

	.local pmc args
	args             = new PerlHash
	args[ 'height' ] =        480
	args[ 'width'  ] =        640
	args[ 'bpp'    ] =          0
	args[ 'flags'  ] = 1073741825

	.local pmc app
	.local int app_type

	find_type app_type, 'SDL::App'
	app = new app_type, args

	.local pmc main_screen
	main_screen = app.'surface'()

	.local int rect_type
	find_type  rect_type, 'SDL::Rect'

	args[ 'height' ] = 100
	args[ 'width'  ] = 100
	args[ 'x'      ] =   0
	args[ 'y'      ] = 190

	.local pmc dest_rect
	dest_rect = new rect_type, args

	args['width'] = 101
	.local pmc prev_rect
	prev_rect = new rect_type, args

	args[ 'height' ] =  56
	args[ 'width'  ] = 100
	args[ 'x'      ] =   0
	args[ 'y'      ] =   0

	.local pmc source_rect
	source_rect = new rect_type, args

	.local int color_type
	find_type  color_type, 'SDL::Color'

	args[ 'r' ] = 0
	args[ 'g' ] = 0
	args[ 'b' ] = 0

	.local pmc black
	black = new color_type, args

	.local int sprite_type
	find_type  sprite_type, 'SDL::Sprite'

	.local pmc image
	.local int image_type
	.local pmc filename

	find_type image_type, 'SDL::Image'
	filename = new PerlString
	filename = 'examples/sdl/parrot_small.png'
	image    = new image_type, filename

	args = new PerlHash
	args[ 'surface'  ] = image
	args[ 'source_x' ] =     0
	args[ 'source_y' ] =     0
	args[ 'dest_x'   ] =     0
	args[ 'dest_y'   ] =   190
	args[ 'bgcolor'  ] = black

	.local pmc sprite
	sprite = new sprite_type, args

	.local float start_time
	time start_time

	_animate_on_x_axis( main_screen, sprite,   0, 540,  2)
	sleep 1
	_animate_on_x_axis( main_screen, sprite, 540,   0, -2)

	.local float end_time
	time end_time

	.local float total_time
	total_time = end_time - start_time
	dec total_time

	.local float fps
	fps = 540/total_time

	print "Drew 540 frames in "
	print total_time
	print " seconds ("
	print fps
	print " fps)\n"

	sleep 1

	app.'quit'()
	end
.end

.sub _animate_on_x_axis
	.param pmc screen
	.param pmc sprite
	.param int start_pos
	.param int end_pos
	.param int step_size
	# PMCs:
	#	destination SDL::Surface and SDL::Sprite to animate
	# Ints:
	#	starting and x coordinates and number of pixels to move at a time

	.local int x_pos
	x_pos = start_pos

_loop:
		add x_pos, step_size
		sprite.'x'( x_pos )
		sprite.'draw'( screen )
		screen.'flip'()
		if x_pos != end_pos goto _loop
.end

=head1 AUTHOR

chromatic, E<lt>chromatic at wgz dot orgE<gt>.

=head1 COPYRIGHT

Copyright (c) 2004, The Perl Foundation.

=cut
