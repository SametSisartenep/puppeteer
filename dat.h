typedef struct Layer Layer;
typedef struct Canvas Canvas;

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
