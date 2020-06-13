typedef struct Layer Layer;
typedef struct Canvas Canvas;

struct Layer
{
	RFrame;
	Image *image;
	Layer *prev, *next;
};

struct Canvas
{
	RFrame;
	Image *image;
	Layer layers;
};
