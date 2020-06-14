#include <u.h>
#include <libc.h>
#include <draw.h>
#include <geometry.h>
#include "dat.h"
#include "fns.h"

Layer*
newlayer(char *name, Canvas *c)
{
	Layer *l;

	l = emalloc(sizeof(Layer));
	l->p = Pt2(0,0,1);
	l->bx = c->bx;
	l->by = c->by;
	l->name = strdup(name);
	l->image = eallocimage(display, c->image->r, c->image->chan, 0, 0);
	l->prev = c->layers.prev;
	l->next = &c->layers;
	c->layers.prev->next = l;
	c->layers.prev = l;
	if(c->curlayer == nil)
		c->curlayer = l;
	return l;
}

void
rmlayer(Layer *l)
{
	l->prev->next = l->next;
	l->next->prev = l->prev;
	freeimage(l->image);
	free(l->name);
	free(l);
}
