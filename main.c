#include <u.h>
#include <libc.h>
#include <thread.h>
#include <draw.h>
#include <mouse.h>
#include <keyboard.h>
#include <geometry.h>
#include "dat.h"
#include "fns.h"

/* palette colors */
enum {
	PCBlack,
	PCWhite,
	NCOLOR
};

RFrame worldrf;
Image *pal[NCOLOR];
Image *background;
Canvas *curcanvas;

Point
toscreen(Point2 p)
{
	p = invrframexform(p, worldrf);
	return Pt(p.x,p.y);
}

Point2
fromscreen(Point p)
{
	return rframexform(Pt2(p.x,p.y,1), worldrf);
}

void
initpalette(void)
{
	pal[PCBlack] = eallocimage(display, Rect(0,0,1,1), screen->chan, 1, DBlack);
	pal[PCWhite] = eallocimage(display, Rect(0,0,1,1), screen->chan, 1, DWhite);
}

Image*
gencheckerboard(int w, int h)
{
	Image *i, *dark, *light;
	int x, y;

	i = eallocimage(display, Rect(0,0,2*w,2*h), screen->chan, 1, DNofill);
	dark = eallocimage(display, Rect(0,0,w,h), screen->chan, 0, DBlack&~0x7f);
	light = eallocimage(display, Rect(0,0,w,h), screen->chan, 0, DBlack&~0xdf);

	for(y = 0; y < h; y += h)
		for(x = 0; x < w; x += w)
			draw(i, Rect(x,y,x+w,y+h), (x+y) % 2? light: dark, ZP);

	freeimage(dark);
	freeimage(light);
	return i;
}

void
drawlayer(Canvas *c, Layer *l)
{
	draw(c->image, c->image->r, l->image, nil, ZP);
}

void
drawcanvas(Canvas *c)
{

}

void
redraw(void)
{
	lockdisplay(display);
	draw(screen, screen->r, pal[PCBlack], nil, ZP);
	flushimage(display, 1);
	unlockdisplay(display);
}

void
resized(void)
{
	lockdisplay(display);
	if(getwindow(display, Refnone) < 0)
		sysfatal("resize failed");
	unlockdisplay(display);
	worldrf.p = Pt2(screen->r.min.x,screen->r.max.y,1);
	redraw();
}

void
mouse(Mousectl *mc, Keyboardctl *)
{

}

void
key(Rune r)
{
	switch(r){
	case Kdel:
	case 'q':
		threadexitsall(nil);
	}
}

void
usage(void)
{
	fprint(2, "usage: %s\n", argv0);
	exits("usage");
}

void
threadmain(int argc, char *argv[])
{
	Mousectl *mc;
	Keyboardctl *kc;
	Rune r;

	ARGBEGIN{
	default: usage();
	}ARGEND;
	if(argc > 0)
		usage();

	if(initdraw(nil, nil, nil) < 0)
		sysfatal("initdraw: %r");
	if((mc = initmouse(nil, screen)) == nil)
		sysfatal("initmouse: %r");
	if((kc = initkeyboard(nil)) == nil)
		sysfatal("initkeyboard: %r");

	worldrf.p = Pt2(screen->r.min.x,screen->r.max.y,1);
	worldrf.bx = Vec2(1, 0);
	worldrf.by = Vec2(0,-1);
	initpalette();

	display->locking = 1;
	unlockdisplay(display);
	redraw();

	for(;;){
		enum { MOUSE, RESIZE, KEYBOARD };
		Alt a[] = {
			{mc->c, &mc->Mouse, CHANRCV},
			{mc->resizec, nil, CHANRCV},
			{kc->c, &r, CHANRCV},
			{nil, nil, CHANEND}
		};

		switch(alt(a)){
		case MOUSE:
			mouse(mc, kc);
			break;
		case RESIZE:
			resized();
			break;
		case KEYBOARD:
			key(r);
			break;
		}

		redraw();
	}
}
