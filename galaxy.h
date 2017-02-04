typedef struct QB QB;
typedef struct Body Body;
typedef struct Quad Quad;

struct QB {
	union {
		Quad *q;
		Body *b;
	};
	int t;
};

struct Body {
	double x, y;
	double vx, vy;
	double ax, ay;
	double newax, neway;
	double size, mass;
	Image *col;
};

struct Quad {
	QB c[4];
	double x, y, mass;
};

struct {
	QLock;
	Body *a;
	int l, max;
} glxy;

struct {
	Quad *a;
	int l, max;
} quads;

enum {
	EMPTY,
	QUAD,
	BODY,
};

Point orig;
double G, θ, scale, ε;
Body ZB;
QB space;

Body *body(void);
void center(int);

void quadcalc(QB, Body*, double);
int quadins(Body*, double);
void growquads(void);
