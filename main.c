#include <u.h>
#include <libc.h>
#include <thread.h>
#include <draw.h>
#include <mouse.h>
#include <keyboard.h>
#include <geometry.h>
#include "dat.h"
#include "fns.h"

enum {
	SMPOS,
	NSTAT
};

RFrame worldrf;
Image *background;
Canvas *curcanvas;
Image *brushcolor;
Image *pal[NCOLOR];
int zoom = 1;

char stats[NSTAT][256];

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

/*
 * A draw operation that touches only the area contained in bot but not in top.
 * mp and sp get aligned with bot.min.
 */
static void
gendrawdiff(Image *dst, Rectangle bot, Rectangle top, 
	Image *src, Point sp, Image *mask, Point mp, int op)
{
	Rectangle r;
	Point origin;
	Point delta;

	if(Dx(bot)*Dy(bot) == 0)
		return;

	/* no points in bot - top */
	if(rectinrect(bot, top))
		return;

	/* bot - top ≡ bot */
	if(Dx(top)*Dy(top)==0 || rectXrect(bot, top)==0){
		gendrawop(dst, bot, src, sp, mask, mp, op);
		return;
	}

	origin = bot.min;
	/* split bot into rectangles that don't intersect top */
	/* left side */
	if(bot.min.x < top.min.x){
		r = Rect(bot.min.x, bot.min.y, top.min.x, bot.max.y);
		delta = subpt(r.min, origin);
		gendrawop(dst, r, src, addpt(sp, delta), mask, addpt(mp, delta), op);
		bot.min.x = top.min.x;
	}

	/* right side */
	if(bot.max.x > top.max.x){
		r = Rect(top.max.x, bot.min.y, bot.max.x, bot.max.y);
		delta = subpt(r.min, origin);
		gendrawop(dst, r, src, addpt(sp, delta), mask, addpt(mp, delta), op);
		bot.max.x = top.max.x;
	}

	/* top */
	if(bot.min.y < top.min.y){
		r = Rect(bot.min.x, bot.min.y, bot.max.x, top.min.y);
		delta = subpt(r.min, origin);
		gendrawop(dst, r, src, addpt(sp, delta), mask, addpt(mp, delta), op);
		bot.min.y = top.min.y;
	}

	/* bottom */
	if(bot.max.y > top.max.y){
		r = Rect(bot.min.x, top.max.y, bot.max.x, bot.max.y);
		delta = subpt(r.min, origin);
		gendrawop(dst, r, src, addpt(sp, delta), mask, addpt(mp, delta), op);
		bot.max.y = top.max.y;
	}
}

void
zoomdraw(Image *d, Rectangle r, Rectangle top, Image *b, Image *s, Point sp, int f)
{
	Rectangle dr;
	Image *t;
	Point a;
	int w;

	a = ZP;
	if(r.min.x < d->r.min.x){
		sp.x += (d->r.min.x - r.min.x)/f;
		a.x = (d->r.min.x - r.min.x)%f;
		r.min.x = d->r.min.x;
	}
	if(r.min.y < d->r.min.y){
		sp.y += (d->r.min.y - r.min.y)/f;
		a.y = (d->r.min.y - r.min.y)%f;
		r.min.y = d->r.min.y;
	}
	rectclip(&r, d->r);
	w = s->r.max.x - sp.x;
	if(w > Dx(r))
		w = Dx(r);
	dr = r;
	dr.max.x = dr.min.x+w;
	if(!alphachan(s->chan))
		b = nil;
	if(f <= 1){
		if(b) gendrawdiff(d, dr, top, b, sp, nil, ZP, SoverD);
		gendrawdiff(d, dr, top, s, sp, nil, ZP, SoverD);
		return;
	}
	if((t = allocimage(display, dr, s->chan, 0, 0)) == nil)
		return;
	for(; dr.min.y < r.max.y; dr.min.y++){
		dr.max.y = dr.min.y+1;
		draw(t, dr, s, nil, sp);
		if(++a.y == f){
			a.y = 0;
			sp.y++;
		}
	}
	dr = r;
	for(sp=dr.min; dr.min.x < r.max.x; sp.x++){
		dr.max.x = dr.min.x+1;
		if(b) gendrawdiff(d, dr, top, b, sp, nil, ZP, SoverD);
		gendrawdiff(d, dr, top, t, sp, nil, ZP, SoverD);
		for(dr.min.x++; ++a.x < f && dr.min.x < r.max.x; dr.min.x++){
			dr.max.x = dr.min.x+1;
			gendrawdiff(d, dr, top, d, Pt(dr.min.x-1, dr.min.y), nil, ZP, SoverD);
		}
		a.x = 0;
	}
	freeimage(t);
}


void
drawstats(void)
{
	int i;
	Point o;

	for(i = 0, o = Pt(10,10); i < nelem(stats); i++, o.y += font->height)
		stringnbg(screen, addpt(screen->r.min, o), pal[PCWhite], ZP, font, stats[i], sizeof stats[i], pal[PCBlack], ZP);
}

void
drawlayer(Layer *l, Canvas *c)
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
		drawlayer(l, c);
	zoomdraw(screen, rectaddpt(Rpt(mulpt(c->image->r.min, zoom),mulpt(c->image->r.max, zoom)), toscreen(c->p)), ZR, nil, c->image, ZP, zoom);
}

void
redraw(void)
{
	lockdisplay(display);
	gendrawdiff(screen, screen->r, curcanvas == nil? ZR: rectaddpt(Rpt(mulpt(curcanvas->image->r.min, zoom),mulpt(curcanvas->image->r.max, zoom)), toscreen(curcanvas->p)),  pal[PCBlack], ZP, nil, ZP, S);
	gendrawdiff(screen, curcanvas == nil? screen->r: rectaddpt(Rpt(mulpt(curcanvas->image->r.min, zoom),mulpt(curcanvas->image->r.max, zoom)), toscreen(curcanvas->p)), ZR, background, ZP, nil, ZP, S);
	drawcanvas(curcanvas);
	drawstats();
	flushimage(display, 1);
	unlockdisplay(display);
}

static void
pickerproc(void *arg)
{
	int *pfd;
	char mntdesc[64];

	pfd = arg;
	close(pfd[1]);
	dup(pfd[0], 0);
	dup(pfd[0], 1);
	close(pfd[0]);

	snprint(mntdesc, sizeof mntdesc, "-pid %d -dx %d -dy %d", getpid(), 384, 320);
	newwindow(mntdesc);

	procexecl(nil, "/bin/picker", "picker", "-e", nil);
	threadexits("procexecl: %r");
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
	int w, h, idx, fd, n, pfd[2];
	ulong chan;
	Point2 cpos;
	Layer *l;

	buf[0] = 0;

	idx = menuhit(3, mc, &menu, nil);
	if(idx < 0)
		return;

	switch(idx){
	case NEW:
		if(curcanvas != nil)
			return;
		snprint(buf, sizeof buf, "%d %d %s", Dx(screen->r), Dy(screen->r), chantostr(chanstr, screen->chan));
		if(enter("w h chan", buf, sizeof buf, mc, kc, nil) <= 0)
			return;
		w = strtol(buf, &s, 10);
		h = strtol(s, &s, 10);
		chan = strtochan(s);
		cpos = Pt2(Dx(screen->r)/2 - w/2,Dy(screen->r)/2 - h/2,1);
		curcanvas = newcanvas("default", cpos, Rect(0,0,w,h), chan);
		addlayer(curcanvas, "layer #1");
		break;
	case NEWLAYER:
		if(curcanvas == nil)
			return;
		snprint(buf, sizeof buf, "%s", curcanvas->layers.prev->name);
		do{
			if(enter("layer name", buf, sizeof buf, mc, kc, nil) <= 0)
				return;
		}while(getlayer(curcanvas, buf) != nil);
		addlayer(curcanvas, buf);
		break;
	case SETCOLOR:
		if(pipe(pfd) < 0)
			sysfatal("pipe: %r");
		procrfork(pickerproc, pfd, 4096, RFFDG|RFNAMEG);
		close(pfd[0]);
		fprint(pfd[1], "0 %08ux\n", 0x000000ff);
		n = read(pfd[1], buf, sizeof(buf)-1);
		close(pfd[1]);
		if(n < 0)
			return;
		buf[n] = 0;
		s = buf;
		while(*s && *s++ != '\t')
			;
		brushcolor = eallocimage(display, Rect(0,0,1,1), screen->chan, 1, strtoul(s, nil, 16));
		break;
	case SAVE:
		if(curcanvas == nil)
			return;
		if(enter("file", buf, sizeof buf, mc, kc, nil) <= 0)
			return;
		fd = create(buf, OWRITE, 0666);
		if(fd < 0)
			return;
		lockdisplay(display);
		drawcanvas(curcanvas);
		unlockdisplay(display);
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
		line(curcanvas->curlayer->image, oldp, p, Endsquare, Endsquare, 0, brushcolor, ZP);
		redraw();
	}
}

void
mouse(Mousectl *mc, Keyboardctl *kc)
{
	if(curcanvas == nil)
		snprint(stats[SMPOS], sizeof stats[SMPOS], "%v", fromscreen(mc->xy));
	else
		snprint(stats[SMPOS], sizeof stats[SMPOS], "%v", rframexform(fromscreen(mc->xy), *curcanvas));

	if((mc->buttons&1) != 0)
		lmb(mc, kc);
	if((mc->buttons&2) != 0)
		mmb(mc, kc);
	if((mc->buttons&4) != 0)
		rmb(mc, kc);
	if((mc->buttons&8) != 0){
		zoom = clamp(++zoom, 1, MAXZOOM);
		worldrf.bx = Vec2(zoom,0);
		worldrf.by = Vec2(0,zoom);
	}
	if((mc->buttons&16) != 0){
		zoom = clamp(--zoom, 1, MAXZOOM);
		worldrf.bx = Vec2(zoom,0);
		worldrf.by = Vec2(0,zoom);
	}
}

void
key(Rune r)
{
	switch(r){
	case Kdel:
	case 'q':
		threadexitsall(nil);
	case '+':
		zoom = clamp(++zoom, 1, MAXZOOM);
		worldrf.bx = Vec2(zoom,0);
		worldrf.by = Vec2(0,zoom);
		break;
	case '-':
		zoom = clamp(--zoom, 1, MAXZOOM);
		worldrf.bx = Vec2(zoom,0);
		worldrf.by = Vec2(0,zoom);
		break;
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

	GEOMfmtinstall();
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
