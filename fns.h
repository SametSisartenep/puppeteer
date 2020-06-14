/* alloc */
void *emalloc(ulong);
void *erealloc(void*, ulong);
Image *eallocimage(Display*, Rectangle, ulong, int, ulong);

/* canvas */
Canvas *newcanvas(char*, Point2, Rectangle, ulong);
void rmcanvas(Canvas*);

/* layer */
Layer *newlayer(char*, Canvas*);
void rmlayer(Layer*);

/* utils */
double fclamp(double, double, double);
int alphachan(ulong);
