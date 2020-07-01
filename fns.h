/* alloc */
void *emalloc(ulong);
void *erealloc(void*, ulong);
Image *eallocimage(Display*, Rectangle, ulong, int, ulong);

/* canvas */
Canvas *newcanvas(char*, Point2, Rectangle, ulong);
void rmcanvas(Canvas*);
Layer *addlayer(Canvas*, char*);
Layer *getlayer(Canvas*, char*);

/* layer */
Layer *newlayer(char*, Rectangle, ulong);
void rmlayer(Layer*);

/* color */
Color *newcolor(ulong);
void rmcolor(Color*);

/* utils */
int clamp(int, int, int);
int alphachan(ulong);
