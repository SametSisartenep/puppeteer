#include <u.h>
#include <libc.h>
#include <draw.h>
#include <geometry.h>
#include "dat.h"
#include "fns.h"

Color*
newcolor(ulong col)
{
	Color *c;

	c = emalloc(sizeof(Color));
	c->v = col;
	c->i = eallocimage(display, Rect(0,0,1,1), screen->chan, 1, col);
	return c;
}

void
rmcolor(Color *c)
{
	freeimage(c->i);
	free(c);
}
