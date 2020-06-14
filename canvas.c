#include <u.h>
#include <libc.h>
#include <draw.h>
#include <geometry.h>
#include "dat.h"
#include "fns.h"

Canvas*
newcanvas(char *name, Point2 p, Rectangle r, ulong chan)
{
	Canvas *c;

	c = emalloc(sizeof(Canvas));
	c->p = p;
	c->bx = Vec2(1,0);
	c->by = Vec2(0,1);
	c->name = strdup(name);
	c->image = eallocimage(display, r, chan, 0, alphachan(chan)? DTransparent: DNofill);
	memset(&c->layers, 0, sizeof(Layer));
	c->layers.next = &c->layers;
	c->layers.prev = &c->layers;
	c->curlayer = nil;
	return c;
}

void
rmcanvas(Canvas *c)
{
	Layer *l;

	for(l = c->layers.next; l != &c->layers; l = l->next)
		rmlayer(l);
	freeimage(c->image);
	free(c->name);
	free(c);
}
