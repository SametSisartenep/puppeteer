/* alloc */
void *emalloc(ulong);
void *erealloc(void*, ulong);
Image *eallocimage(Display*, Rectangle, ulong, int, ulong);

/* canvas */
Canvas *newcanvas(Point2, Rectangle, ulong);

/* layer */
Layer *newlayer(Canvas*);
void rmlayer(Layer*);
