#include <u.h>
#include <libc.h>
#include <draw.h>
#include "galaxy.h"

void
mkglxy(void)
{

	glxy.a = calloc(5, sizeof(Body));
	if(glxy.a == nil)
		sysfatal("could not allocate glxy: %r");
	glxy.l = 0;
	glxy.max = 5;
}

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
drawbody(Body *b)
{
	Point pos, v;
	int s;

	pos.x = b->x / scale + orig.x;
	pos.y = b->y / scale + orig.y;
	s = b->size/scale;
	fillellipse(screen, pos, s, s, b->col, ZP);
	v.x = b->v.x/scale*10;
	v.y = b->v.y/scale*10;
	if(v.x != 0 || v.y != 0)
		line(screen, pos, addpt(pos, v), Enddisc, Endarrow, 0, b->col, ZP);
	flushimage(display, 1);
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
	DEBUG(calcs = 0;)
	quadcalc(space, b, LIM);
	DEBUG(avgcalcs += calcs;)
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

int
Bfmt(Fmt *f)
{
	Body *b;
	int r;

	b = va_arg(f->args, Body*);

	r = fmtprint(f, "MKBODY %g %g ", b->x, b->y);
	if(r < 0)
		return -1;

	r = fmtprint(f, "%g %g ", b->v.x, b->v.y);
	if(r < 0)
		return -1;

	return fmtprint(f, "%g", b->mass);
}
