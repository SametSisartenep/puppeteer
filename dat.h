/* palette colors */
enum {
	PCBlack,
	PCWhite,
	NCOLOR
};

enum {
	MAXZOOM = 8
};

typedef struct Layer Layer;
typedef struct Canvas Canvas;
typedef struct HUD HUD;
typedef struct HUDWidget HUDWidget;

struct Layer
{
	RFrame;
	char *name;
	Image *image;
	Layer *prev, *next;
};

struct Canvas
{
	RFrame;
	char *name;
	Image *image;
	Layer layers;
	Layer *curlayer;
};

struct HUD
{
	Point2 p;
	HUDWidget *widgets;

	void (*addwidget)(HUD*, Point2, char*, ...);
};

struct HUDWidget
{
	Point2 p;
	char *fmt;
	void *args;
	int nargs;
	HUDWidget *next;
};
