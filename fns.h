/* alloc */
void *emalloc(ulong);
void *erealloc(void*, ulong);
Image *eallocimage(Display*, Rectangle, ulong, int, ulong);

/* canvas */
Canvas *newcanvas(char*, Point2, Rectangle, ulong);
void rmcanvas(Canvas*);
Layer *addlayer(Canvas*, char*);

/* layer */
Layer *newlayer(char*, Rectangle, ulong);
void rmlayer(Layer*);

/* utils */
double fclamp(double, double, double);
int alphachan(ulong);
