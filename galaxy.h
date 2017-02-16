typedef struct QB QB;
typedef struct Body Body;
typedef struct Quad Quad;
typedef struct Vector Vector;

struct QB {
	union {
		Quad *q;
		Body *b;
	};
	int t;
};

struct Vector {
	double x, y;
};

struct Body {
	Vector;
	Vector v, a, newa;
	double size, mass;
	Image *col;
};

struct Quad {
	Vector;
	QB c[4];
	double mass;
};

#pragma varargck type "B" Body*

struct {
	QLock;
	Body *a;
	int l, max;
} glxy;

struct {
	Quad *a;
	int l, max;
} quads;

#define π2 6.28318530718e0

enum {
	EMPTY,
	QUAD,
	BODY,
};

Point orig;
double G, θ, scale, ε, Λ, LIM;
Body ZB;
QB space;

Body *body(void);
void drawbody(Body*);
void calcforces(Body*);
Vector center(void);
void mkglxy(void);
int Bfmt(Fmt*);

void quadcalc(QB, Body*, double);
int quadins(Body*, double);
void growquads(void);
void mkquads(void);

int debug;
int insdepth;
int calcs;
double avgcalcs;

#define DEBUG(e) if(debug) {e}
