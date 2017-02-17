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
double G, θ, scale, ε, Λ, LIM, dt, dt²;
Body ZB;
QB space;

Image *randcol(void);

Body *body(void);
void drawbody(Body*);
void calcforces(Body*);
Vector center(void);
void glxyinit(void);
void readglxy(int);
void writeglxy(int);
int Bfmt(Fmt*);

void quadcalc(QB, Body*, double);
int quadins(Body*, double);
void growquads(void);
void quadsinit(void);

int stats;
int quaddepth;
int calcs;
double avgcalcs;

#define STATS(e) if(stats) {e}

#define CHECKLIM(b, f) \
	if(((f) = fabs((b)->x)) > LIM/2)	\
		LIM = (f)*2;	\
	if(((f) = fabs((b)->y)) > LIM/2)	\
		LIM = (f)*2
