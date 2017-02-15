#include <u.h>
#include <libc.h>
#include <draw.h>
#include "galaxy.h"

Body*
body(void)
{
	Body *b;

	if(glxy.l == glxy.max) {
		glxy.max *= 2;
		glxy.a = realloc(glxy.a, sizeof(Body) * glxy.max);
		if(glxy.a == nil)
			sysfatal("could not realloc glxy: %r");
	}
	b = glxy.a + glxy.l++;
	*b = ZB;
	return b;
}

void
calcforces(Body *b)
{
	double h;
	b->a.x = b->newa.x;
	b->a.y = b->newa.y;
	h = hypot(b->x, b->y);
	if(h != 0.0) {
		b->newa.x = Λ/b->mass * b->x/h;
		b->newa.y = Λ/b->mass * b->y/h;
	} else
		b->newa.x = b->newa.y = 0;
	quadcalc(space, b, LIM);
}

Vector
center(void)
{
	Body *b;
	Vector gc, gcv;
	double mass;

	gc.x = gc.y = gcv.x = gcv.y = mass = 0;
	for(b = glxy.a; b < glxy.a+glxy.l; b++) {
		gc.x += b->x * b->mass;
		gc.y += b->y * b->mass;
		gcv.x += b->v.x * b->mass;
		gcv.y += b->v.y * b->mass;
		mass += b->mass;
	}
	gc.x /= mass;
	gc.y /= mass;
	gcv.x /= mass;
	gcv.y /= mass;
	for(b = glxy.a; b < glxy.a+glxy.l; b++) {
		b->x -= gc.x;
		b->y -= gc.y;
		b->v.x -= gcv.x;
		b->v.y -= gcv.y;
	}
	return gc;
}
