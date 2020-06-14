#include <u.h>
#include <libc.h>
#include <draw.h>
#include <geometry.h>
#include "dat.h"
#include "fns.h"

static int
alphachan(ulong chan)
{
	for(; chan; chan >>= 8)
		if(TYPE(chan) == CAlpha)
			return 1;
	return 0;
}

Canvas*
newcanvas(Point2 p, Rectangle r, ulong chan)
{
	Canvas *c;

	c = emalloc(sizeof(Canvas));
	c->p = p;
	c->bx = Vec2(1,0);
	c->by = Vec2(0,1);
	c->image = eallocimage(display, r, chan, 0, alphachan(chan)? DTransparent: DNofill);
	memset(&c->layers, 0, sizeof(Layer));
	c->layers.next = &c->layers;
	c->layers.prev = &c->layers;
	return c;
}
