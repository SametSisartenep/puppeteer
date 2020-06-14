#include <u.h>
#include <libc.h>
#include <draw.h>
#include <geometry.h>
#include "dat.h"
#include "fns.h"

Layer*
newlayer(Canvas *c)
{
	Layer *l;

	l = emalloc(sizeof(Layer));
	l->p = Pt2(0,0,1);
	l->bx = c->bx;
	l->by = c->by;
	l->image = eallocimage(display, c->image->r, c->image->chan, 0, DNofill);
	l->prev = c->layers.prev;
	l->next = &c->layers;
	c->layers.prev->next = l;
	c->layers.prev = l;
	return l;
}

void
rmlayer(Layer *l)
{
	l->prev->next = l->next;
	l->next->prev = l->prev;
	freeimage(l->image);
	free(l);
}
