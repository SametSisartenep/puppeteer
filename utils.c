#include <u.h>
#include <libc.h>
#include <draw.h>

double
fclamp(double n, double min, double max)
{
	return n < min? min: n > max? max: n;
}

int
alphachan(ulong chan)
{
	for(; chan; chan >>= 8)
		if(TYPE(chan) == CAlpha)
			return 1;
	return 0;
}
