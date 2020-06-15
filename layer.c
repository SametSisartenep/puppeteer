#include <u.h>
#include <libc.h>
#include <draw.h>
#include <geometry.h>
#include "dat.h"
#include "fns.h"

Layer*
newlayer(char *name, Rectangle r, ulong chan)
{
	Layer *l;

	l = emalloc(sizeof(Layer));
	l->p = Pt2(0,0,1);
	l->bx = Vec2(1,0);
	l->by = Vec2(0,1);
	l->name = strdup(name);
	l->image = eallocimage(display, r, chan, 0, alphachan(chan)? DTransparent: DWhite);
	l->prev = l->next = nil;
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
