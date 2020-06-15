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
Image *brushcolor;

void resized(void);

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
mkcheckerboard(int w, int h)
{
	Image *i, *dark, *light;
	int x, y;

	i = eallocimage(display, Rect(0,0,2*w,2*h), screen->chan, 1, DNofill);
	dark = eallocimage(display, Rect(0,0,w,h), screen->chan, 0, 0x333333FF);
	light = eallocimage(display, Rect(0,0,w,h), screen->chan, 0, 0xEEEEEEFF);

	for(y = 0; y < i->r.max.y; y += h)
		for(x = 0; x < i->r.max.x; x += w)
			draw(i, Rect(x,y,x+w,y+h), (x/w + y/h) % 2 == 0? light: dark, nil, ZP);

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
	Layer *l;

	if(c == nil)
		return;
	for(l = c->layers.next; l != &c->layers; l = l->next)
		drawlayer(c, l);
	draw(screen, rectaddpt(c->image->r, toscreen(c->p)), c->image, nil, ZP);
}

void
redraw(void)
{
	lockdisplay(display);
	draw(screen, screen->r, pal[PCBlack], nil, ZP);
	draw(screen, curcanvas == nil? screen->r: rectaddpt(curcanvas->image->r, toscreen(curcanvas->p)), background, nil, ZP);
	drawcanvas(curcanvas);
	flushimage(display, 1);
	unlockdisplay(display);
}

static char*
genrmbmenuitem(int idx)
{
	enum {
		NEW,
		NEWLAYER,
		SETCOLOR,
		SAVE,
		SEP,
		NITEMS
	};
	Layer *l;

	switch(idx){
	case NEW: return "new";
	case NEWLAYER: return "new layer";
	case SETCOLOR: return "set color";
	case SAVE: return "save";
	case SEP: return "";
	}

	if(curcanvas == nil)
		return nil;

	idx -= NITEMS;
	l = curcanvas->layers.next;
	while(l != &curcanvas->layers && idx--)
		l = l->next;
	if(l == &curcanvas->layers)
		return nil;
	return l->name;
}

void
rmb(Mousectl *mc, Keyboardctl *kc)
{
	enum {
		NEW,
		NEWLAYER,
		SETCOLOR,
		SAVE,
		SEP,
		NITEMS
	};
	static Menu menu = { .gen = genrmbmenuitem };
	char buf[256], chanstr[9+1], *s;
	int w, h, idx, fd;
	ulong chan;
	Point2 cpos;
	Layer *l;

	idx = menuhit(3, mc, &menu, nil);
	if(idx < 0)
		return;

	switch(idx){
	case NEW:
		if(curcanvas != nil)
			break;
		snprint(buf, sizeof buf, "%d %d %s", Dx(screen->r), Dy(screen->r), chantostr(chanstr, screen->chan));
		enter("w h chan", buf, sizeof buf, mc, kc, nil);
		w = strtol(buf, &s, 10);
		h = strtol(s, &s, 10);
		chan = strtochan(s);
		cpos = Pt2(Dx(screen->r)/2 - w/2,Dy(screen->r)/2 - h/2,1);
		curcanvas = newcanvas("default", cpos, Rect(0,0,w,h), chan);
		break;
	case NEWLAYER:
		if(curcanvas == nil)
			break;
		buf[0] = 0;
		while(strlen(buf) == 0 || getlayer(curcanvas, buf) != nil)
			enter("layer name", buf, sizeof buf, mc, kc, nil);
		addlayer(curcanvas, buf);
		break;
	case SETCOLOR:
		buf[0] = 0;
		while(strlen(buf) == 0)
			enter("hex value", buf, sizeof buf, mc, kc, nil);
		brushcolor = eallocimage(display, Rect(0,0,1,1), screen->chan, 1, strtoul(buf, nil, 16));
		break;
	case SAVE:
		if(curcanvas == nil)
			break;
		buf[0] = 0;
		while(strlen(buf) == 0)
			enter("file", buf, sizeof buf, mc, kc, nil);
		fd = create(buf, OWRITE, 0666);
		if(fd < 0)
			break;
		drawcanvas(curcanvas);
		writeimage(fd, curcanvas->image, 1);
		close(fd);
		break;	
	case SEP:
		return;
	}

	idx -= NITEMS;
	if(idx >= 0){
		l = curcanvas->layers.next;
		while(l != &curcanvas->layers && idx--)
			l = l->next;
		curcanvas->curlayer = l;
	}
}

void
mmb(Mousectl *mc, Keyboardctl *)
{
	Point2 oldp, p;
	Mouse m;

	if(curcanvas == nil)
		return;

	for(;;){
		m = mc->Mouse;
		if(readmouse(mc) < 0)
			break;
		if((mc->buttons & 7) != 2)
			break;
		oldp = fromscreen(m.xy);
		p = fromscreen(mc->xy);
		curcanvas->p = addpt2(curcanvas->p, subpt2(p, oldp));
		redraw();
	}
}

void
lmb(Mousectl *mc, Keyboardctl *)
{
	Rectangle r;
	Point2 mpos;
	Point p, oldp;
	Mouse m;

	if(curcanvas == nil || curcanvas->curlayer == nil)
		return;

	r = Rect(0,0,1,1);
	mpos = fromscreen(mc->xy);
	mpos = rframexform(mpos, *curcanvas);
	p = Pt(mpos.x,mpos.y);

	draw(curcanvas->curlayer->image, rectaddpt(r, p), brushcolor, nil, ZP);
	redraw();
	for(;;){
		oldp = p;
		m = mc->Mouse;
		if(readmouse(mc) < 0)
			break;
		if(((mc->buttons ^ m.buttons) & 7) != 0)
			break;
		mpos = fromscreen(mc->xy);
		mpos = rframexform(mpos, *curcanvas);
		p = Pt(mpos.x,mpos.y);
		if(eqpt(p, oldp))
			continue;
		//draw(curcanvas->curlayer->image, rectaddpt(r, p), brushcolor, nil, ZP);
		line(curcanvas->curlayer->image, oldp, p, Enddisc, Enddisc, 0, brushcolor, ZP);
		redraw();
	}
}

void
mouse(Mousectl *mc, Keyboardctl *kc)
{
	if((mc->buttons&1) != 0)
		lmb(mc, kc);
	if((mc->buttons&2) != 0)
		mmb(mc, kc);
	if((mc->buttons&4) != 0)
		rmb(mc, kc);
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

	worldrf.p = Pt2(screen->r.min.x,screen->r.min.y,1);
	worldrf.bx = Vec2(1,0);
	worldrf.by = Vec2(0,1);
	initpalette();
	background = mkcheckerboard(4, 4);
	brushcolor = pal[PCBlack];

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

void
resized(void)
{
	lockdisplay(display);
	if(getwindow(display, Refnone) < 0)
		sysfatal("resize failed");
	unlockdisplay(display);
	worldrf.p = Pt2(screen->r.min.x,screen->r.min.y,1);
	redraw();
}
