</$objtype/mkfile

BIN=/$objtype/bin
TARG=puppeteer
OFILES=\
	utils.$O\
	hud.$O\
	alloc.$O\
	color.$O\
	layer.$O\
	canvas.$O\
	main.$O\

HFILES=\
	dat.h\
	fns.h\
	libgeometry/geometry.h\

LIB=\
	libgeometry/libgeometry.a$O\

CFLAGS=$CFLAGS -Ilibgeometry

</sys/src/cmd/mkone

libgeometry/libgeometry.a$O:
	cd libgeometry
	mk install

clean nuke:V:
	rm -f *.[$OS] [$OS].??* $TARG
	@{cd libgeometry; mk $target}
