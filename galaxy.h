typedef struct Body Body;
struct Body {
	double x, y;
	double vx, vy;
	double ax, ay;
	double newax, neway;
	double size, mass;
	Image *col;
};

struct {
	QLock;
	Body *a;
	int l, max;
} glxy;
Point orig;
double scale = 10;
Body ZB;

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

void calcforces(Body*);

void
center(int rec)
{
	Body *b;
	double gcx, gcy, gcvx, gcvy, mass;

	gcx = gcy = gcvx = gcvy = mass = 0;
	for(b = glxy.a; b < glxy.a+glxy.l; b++) {
		gcx += b->x * b->mass;
		gcy += b->y * b->mass;
		gcvx += b->vx * b->mass;
		gcvy += b->vy * b->mass;
		mass += b->mass;
	}
	gcx /= mass;
	gcy /= mass;
	gcvx /= mass;
	gcvy /= mass;
	for(b = glxy.a; b < glxy.a+glxy.l; b++) {
		b->x -= gcx;
		b->y -= gcy;
		b->vx -= gcvx;
		b->vy -= gcvy;
		calcforces(b);
	}
	if(rec) {
		orig.x += gcx / scale;
		orig.y += gcy /scale;
	}
}
