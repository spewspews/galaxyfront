#include <u.h>
#include <libc.h>
#include <bio.h>
#include <draw.h>
#include <thread.h>
#include <mouse.h>
#include <cursor.h>
#include <keyboard.h>
#include "galaxy.h"

typedef struct Quad Quad;
typedef struct QB QB;

struct QB {
	union {
		Quad *q;
		Body *b;
	};
	int t;
};

struct Quad {
	QB c[4];
	double x, y, mass;
};

struct {
	ulong c;
	Image *i;
} cols[] = {
	DWhite, nil,
	DRed, nil,
	DGreen, nil,
	DCyan, nil,
	DMagenta, nil,
	DYellow, nil,
	DPaleyellow, nil,
	DDarkyellow, nil,
	DDarkgreen, nil,
	DPalegreen, nil,
	DPalebluegreen, nil,
	DPaleblue, nil,
	DPalegreygreen, nil,
	DYellowgreen, nil,
	DGreyblue, nil,
	DPalegreyblue, nil,
};

Cursor crosscursor = {
	{-7, -7},
	{0x03, 0xC0, 0x03, 0xC0, 0x03, 0xC0, 0x03, 0xC0,
	 0x03, 0xC0, 0x03, 0xC0, 0xFF, 0xFF, 0xFF, 0xFF,
	 0xFF, 0xFF, 0xFF, 0xFF, 0x03, 0xC0, 0x03, 0xC0,
	 0x03, 0xC0, 0x03, 0xC0, 0x03, 0xC0, 0x03, 0xC0, },
	{0x00, 0x00, 0x01, 0x80, 0x01, 0x80, 0x01, 0x80,
	 0x01, 0x80, 0x01, 0x80, 0x01, 0x80, 0x7F, 0xFE,
	 0x7F, 0xFE, 0x01, 0x80, 0x01, 0x80, 0x01, 0x80,
	 0x01, 0x80, 0x01, 0x80, 0x01, 0x80, 0x00, 0x00, }
};
Cursor pausecursor={
	0, 0,
	0x01, 0x80, 0x03, 0xC0, 0x07, 0xE0, 0x07, 0xe0,
	0x07, 0xe0, 0x07, 0xe0, 0x03, 0xc0, 0x0F, 0xF0,
	0x1F, 0xF8, 0x1F, 0xF8, 0x1F, 0xF8, 0x1F, 0xF8,
	0x0F, 0xF0, 0x1F, 0xF8, 0x3F, 0xFC, 0x3F, 0xFC,

	0x01, 0x80, 0x03, 0xC0, 0x07, 0xE0, 0x04, 0x20,
	0x04, 0x20, 0x06, 0x60, 0x02, 0x40, 0x0C, 0x30,
	0x10, 0x08, 0x14, 0x08, 0x14, 0x28, 0x12, 0x28,
	0x0A, 0x50, 0x16, 0x68, 0x20, 0x04, 0x3F, 0xFC,
};


enum {
	STK = 8192,
	LIMSCALE = 2,
	EMPTY = 0,
	QUAD,
	BODY,
	MOVE = 0,
	ZOOM,
	SPEED,
	SAVE,
	LOAD,
	EXIT,
	MEND,
	MKBODY = 0,
	ORIG,
	DT,
	SCALE,
	COSM,
	NOCMD,
};

Cursor *cursor;
Mousectl *mc;
Keyboardctl kc;
double
	G = 6.674,
	θ = 1,
	dt = .1,
	DENS = 10,
	ε = 500,
	LIM = 10,
	Λ,
	dt²;
QB space;
char *file;
int showv, showa, throttle, paused;

char *menustr[] = {
	[SAVE]	"save",
	[LOAD]	"load",
	[ZOOM]	"zoom",
	[SPEED]	"speed",
	[MOVE]	"move",
	[EXIT]	"exit",
	[MEND]	nil
};
Menu menu = {
	.item menustr
};

struct {
	Quad *a;
	int l, max;
} quads;

#define CHECKLIM(b, f) \
	if(((f) = fabs((b)->x)) > LIM/2)	\
		LIM = (f)*LIMSCALE;	\
	if(((f) = fabs((b)->y)) > LIM/2)	\
		LIM = (f)*LIMSCALE

void
pause(int p, int pri)
{
	static int paused, ppri;

	switch(p) {
	default:
		sysfatal("invalid pause value %d:", p);
		break;
	case 0:
		if(pri > ppri)
			ppri = pri;
		if(paused)
			break;
		paused = 1;
		qlock(&glxy);
		break;
	case 1:
		if(!paused || pri < ppri)
			break;
		paused = ppri = 0;
		qunlock(&glxy);
		break;
	}
}

void
growquads(void)
{
	quads.max *= 2;
	quads.a = realloc(quads.a, sizeof(Quad) * quads.max);
	if(quads.a == nil)
		sysfatal("could not realloc quads: %r");
}

Quad*
quad(Body *b)
{
	Quad *q;

	if(quads.l == quads.max)
		return nil;

	q = quads.a + quads.l++;
	memset(q->c, 0, sizeof(QB)*4);
	q->x = b->x;
	q->y = b->y;
	q->mass = b->mass;
	return q;
}

Image*
randcol(void)
{
	int r;

	r = nrand(nelem(cols));
	if(cols[r].i == nil)
		cols[r].i = allocimage(display, Rect(0,0,1,1), screen->chan, 1, cols[r].c);
	return cols[r].i;
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
	v.x = b->vx/scale*10;
	v.y = b->vy/scale*10;
	if(v.x != 0 || v.y != 0)
		line(screen, pos, addpt(pos, v), Enddisc, Endarrow, 0, b->col, ZP);
	flushimage(display, 1);
}

void
drawglxy(void)
{
	Point pos, vof;
	Body *b;
	int s;

	draw(screen, screen->r, display->black, 0, ZP);
	for(b = glxy.a; b < glxy.a + glxy.l; b++) {
		pos.x = b->x / scale + orig.x;
		pos.y = b->y / scale + orig.y;
		s = b->size/scale;
		fillellipse(screen, pos, s, s, b->col, ZP);
		if(showv) {
			vof.x = b->vx/scale*10;
			vof.y = b->vy/scale*10;
			if(vof.x != 0 || vof.y != 0)
				line(screen, pos, addpt(pos, vof), Enddisc, Endarrow, 0, b->col, ZP);
		}
		if(showa) {
			vof.x = b->ax;
			vof.y = b->ay;
			if(vof.x != 0 || vof.y != 0)
				line(screen, pos, addpt(pos, vof), Enddisc, Endarrow, 0, b->col, ZP);
		}
	}
	flushimage(display, 1);
}

void
setsize(Body *b)
{
	Point pos, d;
	double h;

	pos.x = b->x / scale + orig.x;
	pos.y = b->y / scale + orig.y;
	d = subpt(mc->xy, pos);
	h = hypot(d.x, d.y);
	b->size = h == 0 ? 1 : h;
	b->size *= scale;
	b->mass = DENS*3*b->size*b->size; /* π is exactly 3 */
}

void
dosize(Body *b)
{
	Point p;

	p = mc->xy;
	for(;;) {
		setsize(b);
		drawglxy();
		drawbody(b);
		readmouse(mc);
		if(mc->buttons != 3)
			break;
	}
	moveto(mc, p);
}

void
setvel(Body *b)
{
	Point pos, d;

	pos.x = b->x / scale + orig.x;
	pos.y = b->y / scale + orig.y;
	d = subpt(mc->xy, pos);
	b->vx = (double)d.x*scale/10;
	b->vy = (double)d.y*scale/10;
}

void
dovel(Body *b)
{
	Point p;
	p = mc->xy;
	for(;;) {
		setvel(b);
		drawglxy();
		drawbody(b);
		readmouse(mc);
		if(mc->buttons != 5)
			break;
	}
	moveto(mc, p);
}

void
setpos(Body *b)
{
	b->x = (mc->xy.x - orig.x) * scale;
	b->y = (mc->xy.y - orig.y) * scale;
}

void
dobody(void)
{
	double f;
	Body *b;

	pause(0, 0);
	b = body();
	setpos(b);
	setvel(b);
	setsize(b);
	b->col = randcol();
	for(;;) {
		drawglxy();
		drawbody(b);
		readmouse(mc);
		if(!(mc->buttons & 1))
			break;
		if(mc->buttons == 3)
			dosize(b);
		else if(mc->buttons == 5)
			dovel(b);
		else
			setpos(b);
	}
	CHECKLIM(b, f);
	center(1);
	pause(1, 0);
}

char*
getinput(char *info, char *sug)
{
	static char buf[1024];
	static Channel *rchan;
	char *input;
	int r;

	if(rchan == nil)
		rchan = chancreate(sizeof(Rune), 20);

	if(sug != nil)
		strecpy(buf, buf+1024, sug);
	else
		buf[0] = '\0';

	kc.c = rchan;
	r = enter(info, buf, sizeof(buf), mc, &kc, nil);
	kc.c = nil;
	if(r < 0)
		sysfatal("save: could not get filename: %r");

	input = strdup(buf);
	if(input == nil)
		sysfatal("getinput: could not save input: %r");
	return input;
}

char *cmds[] = {
	[MKBODY]	"MKBODY",
	[ORIG]	"ORIG",
	[DT]	"DT",
	[SCALE]	"SCALE",
	[COSM]	"COSM",
};

int
getcmd(char *l)
{
	int cmd;

	for(cmd = 0; cmd < nelem(cmds); cmd++) {
		if(strcmp(l, cmds[cmd]) == 0)
			return cmd;
	}
	return NOCMD;
}

void
load(int fd)
{
	static Biobuf bin;
	char *line;
	double f;
	int cmd, len;
	Body *b;

	glxy.l = 0;
	orig = divpt(subpt(screen->r.max, screen->r.min), 2);
	orig = addpt(orig, screen->r.min);
	Binit(&bin, fd, OREAD);
	for(;;) {
		line = Brdline(&bin, ' ');
		len = Blinelen(&bin);
		if(line == nil) {
			if(len == 0)
				break;
			sysfatal("load: read error: %r");
		}

		line[len-1] = '\0';
		cmd = getcmd(line);

		line = Brdline(&bin, '\n');
		if(line == nil)
			sysfatal("load: read error: %r");
		len = Blinelen(&bin);
		line[len-1] = '\0';

		switch(cmd) {
		case NOCMD:
			sysfatal("load: no such command");
		case MKBODY:
			b = body();
			b->x = strtod(line, &line);
			b->y = strtod(line, &line);
			b->vx = strtod(line, &line);
			b->vy = strtod(line, &line);
			b->mass = strtod(line, nil);
			b->size = sqrt(b->mass/(3*DENS)); /* π is exactly 3 */
			b->col = randcol();
			CHECKLIM(b, f);
			break;
		case ORIG:
			orig.x = strtol(line, &line, 10);
			orig.y = strtol(line, nil, 10);
			break;
		case DT:
			dt = strtod(line, nil);
			dt² = dt*dt;
			break;
		case SCALE:
			scale = strtod(line, nil);
			break;
		case COSM:
			Λ = strtod(line, nil);
			break;
		}
	}
	Bterm(&bin);
	center(0);
}

void
save(void)
{
	int fd;
	static Biobuf bout;
	Body *b;

	if(file == nil)
		return;
	fd = create(file, OWRITE, 0666);
	if(fd < 0)
		sysfatal("bodyprint: could not create file %s: %r", file);
	Binit(&bout, fd, OWRITE);

	
	Bprint(&bout, "ORIG %d %d\n", orig.x, orig.y);
	Bprint(&bout, "SCALE %g\n", scale);
	Bprint(&bout, "DT%g\n", dt);
	Bprint(&bout, "COSM%g\n", Λ);
	for(b = glxy.a; b < glxy.a + glxy.l; b++) {
		Bprint(&bout, "MKBODY %g %g ", b->x, b->y);
		Bprint(&bout, "%g %g ", b->vx, b->vy);
		Bprint(&bout, "%g\n", b->size);
	}
	Bterm(&bout);
	close(fd);
}

void
move(void)
{
	Point od;
	setcursor(mc, &crosscursor);
	for(;;) {
		for(;;) {
			readmouse(mc);
			if(mc->buttons & 1)
				break;
			if(mc->buttons & 4) {
				setcursor(mc, cursor);
				return;
			}
		}
		od = subpt(orig, mc->xy);
		for(;;) {
			readmouse(mc);
			if(!(mc->buttons & 1))
				break;
			orig = addpt(od, mc->xy);
			drawglxy();
		}
	}
}

void
domenu(void)
{
	int fd;
	char *s;
	double z;

	pause(0, 0);
	switch(menuhit(3, mc, &menu, nil)) {
	case SAVE:
		s = getinput("Enter file:", file);
		if(s == nil || *s == '\0')
			break;
		free(file);
		file = s;
		save();
		break;
	case LOAD:
		s = getinput("Enter file:", file);
		if(s == nil || *s == '\0')
			break;
		free(file);
		file = s;
		fd = open(file, OREAD);
		if(fd < 0)
			sysfatal("domenu: could not open file %s: %r", file);
		load(fd);
		close(fd);
		break;
	case ZOOM:
		s = getinput("Zoom factor:", nil);
		if(s == nil || *s == '\0')
			break;
		z = strtod(s, nil);
		free(s);
		if(z <= 0)
			break;
		scale /= z;
		break;
	case SPEED:
		s = getinput("Speed factor:", nil);
		if(s == nil || *s == '\0')
			break;
		z = strtod(s, nil);
		free(s);
		if(z <= 0)
			break;
		dt *= z;
		dt² = dt*dt;
		break;
	case MOVE:
		move();
		break;
	case EXIT:
		threadexitsall(nil);
		break;
	}
	drawglxy();
	pause(1, 0);
}

void
mousethread(void*)
{
	threadsetname("mouse");
	for(;;) {
		readmouse(mc);
		switch(mc->buttons) {
		case 1:
			if(paused) {
				paused ^= 1;
				cursor = nil;
				setcursor(mc, cursor);
				pause(1, 1);
				break;
			}
			dobody();
			break;
		case 4:
			domenu();
			break;
		}
	}
}

void
resizethread(void*)
{
	threadsetname("resize");
	for(;;) {
		recv(mc->resizec, nil);
		pause(0, 0);
		if(getwindow(display, Refnone) < 0)
			sysfatal("resize failed: %r");
		drawglxy();
		pause(1, 0);
	}
}

void
kbdthread(void*)
{
	Keyboardctl *realkc;
	Rune r;

	threadsetname("keyboard");
	if(realkc = initkeyboard(nil), realkc == nil)
		sysfatal("kbdthread: could not initkeyboard: %r");

	for(;;) {
		recv(realkc->c, &r);
		if(r == Kdel)
			threadexitsall(nil);
		if(kc.c != nil)
			send(kc.c, &r);
		else switch(r) {
		case 'v':
			showv ^= 1;
			break;
		case 'a':
			showa ^= 1;
			break;
		case ' ':
			paused ^= 1;
			if(paused) {
				cursor = &pausecursor;
				pause(0, 1);
			} else {
				cursor = nil;
				pause(1, 1);
			}
			setcursor(mc, cursor);
		}
	}
}

int
quadins(Body *nb, double size)
{
	QB *qb;
	Quad *q;
	Body *b;
	double mass, qx, qy;
	uint qxy;

	if(space.t == EMPTY) {
		space.t = BODY;
		space.b = nb;
		return 0;
	}

	qb = &space;
	qx = 0.0;
	qy = 0.0;
	for(;;) {
		if(qb->t == BODY) {
			b = qb->b;
			qxy = b->x < qx ? 0 : 1;
			qxy |= b->y < qy ? 0 : 2;
			qb->t = QUAD;
			if((qb->q = quad(b)) == nil)
				return -1;
			qb->q->c[qxy].t = BODY;
			qb->q->c[qxy].b = b;
		}

		q = qb->q;
		mass = q->mass + nb->mass;
		q->x = (q->x*q->mass + nb->x*nb->mass) / mass;
		q->y = (q->y*q->mass + nb->y*nb->mass) / mass;
		q->mass = mass;

		qxy = nb->x < qx ? 0 : 1;
		qxy |= nb->y < qy ? 0 : 2;
		if(q->c[qxy].t == EMPTY) {
			q->c[qxy].t = BODY;
			q->c[qxy].b = nb;
			return 0;
		}

		qb = &q->c[qxy];
		size /= 2;
		qx += qxy&1 ? size/2 : -size/2;
		qy += qxy&2 ? size/2 : -size/2;
	}
}

void
quadcalc(QB qb, Body *b, double size)
{
	double fx÷❨m₁m₂❩, fy÷❨m₁m₂❩, dx, dy, h, G÷h³;

	for(;;) switch(qb.t) {
	default:
		sysfatal("quadcalc: No such type");
	case EMPTY:
		return;
	case BODY:
		if(qb.b == b)
			return;
		dx = qb.b->x - b->x;
		dy = qb.b->y - b->y;
		h = hypot(hypot(dx, dy), ε);
		G÷h³ = G / (h*h*h);
		fx÷❨m₁m₂❩ = dx * G÷h³;
		fy÷❨m₁m₂❩ = dy * G÷h³;
		b->newax += fx÷❨m₁m₂❩ * qb.b->mass;
		b->neway += fy÷❨m₁m₂❩ * qb.b->mass;
		return;
	case QUAD:
		dx = qb.q->x - b->x;
		dy = qb.q->y - b->y;
		h = hypot(dx, dy);
		if(h != 0.0 && size/h < θ) {
			h = hypot(h, ε);
			G÷h³ = G / (h*h*h);
			fx÷❨m₁m₂❩ = dx * G÷h³;
			fy÷❨m₁m₂❩ = dy * G÷h³;
			b->newax += fx÷❨m₁m₂❩ * qb.q->mass;
			b->neway += fy÷❨m₁m₂❩ * qb.q->mass;
			return;
		}
		size /= 2;
		quadcalc(qb.q->c[0], b, size);
		quadcalc(qb.q->c[1], b, size);
		quadcalc(qb.q->c[2], b, size);
		qb = qb.q->c[3];
		break;	/* quadcalc(q->q[3], b, size); */
	}
}

void
calcforces(Body *b)
{
	double h;
	b->ax = b->newax;
	b->ay = b->neway;
	h = hypot(b->x, b->y);
	if(h != 0.0) {
		b->newax = Λ*b->x/h;
		b->neway = Λ*b->y/h;
	} else
		b->newax = b->neway = 0;
	quadcalc(space, b, LIM);	
}

/* verlet barnes-hut */
void
simulate(void*)
{
	Body *b;
	double f;

	threadsetname("simulate");

	for(;;) {
		qlock(&glxy);
		if(throttle)
			sleep(throttle);
		drawglxy();
Again:
		space.t = EMPTY;
		quads.l = 0;
		for(b = glxy.a; b < glxy.a + glxy.l; b++) {
			if(quadins(b, LIM) == -1) {
				growquads();
				goto Again;
			}
		}
		for(b = glxy.a; b < glxy.a + glxy.l; b++)
			calcforces(b);
		for(b = glxy.a; b < glxy.a + glxy.l; b++) {
			b->x += dt*b->vx + dt²*b->ax/2;
			b->y += dt*b->vy + dt²*b->ay/2;
			b->vx += dt*(b->ax + b->newax)/2;
			b->vy += dt*(b->ay + b->neway)/2;
			CHECKLIM(b, f);
		}
		qunlock(&glxy);
	}
}

void
mkglxy(void)
{

	glxy.a = calloc(5, sizeof(Body));
	if(glxy.a == nil)
		sysfatal("could not allocate glxy: %r");
	glxy.l = 0;
	glxy.max = 5;
}

void
mkquads(void)
{
	quads.a = calloc(5, sizeof(Body));
	if(quads.a == nil)
		sysfatal("could not allocate quads: %r");
	quads.l = 0;
	quads.max = 5;
}

void
usage(void)
{
	fprint(2, "Usage: %s [-t throttle] [-G gravity] [-ε smooth] [file]\n", argv0);
	threadexitsall("usage");
}

void
threadmain(int argc, char **argv)
{
	int doload;

	doload = 0;
	ARGBEGIN {
	default:
		usage();
		break;
	case 't':
		throttle = strtol(EARGF(usage()), nil, 0);
		break;
	case 'G':
		G = strtod(EARGF(usage()), nil);
		break;
	case L'ε':
		ε = strtod(EARGF(usage()), nil);
		break;
	case 'i':
		doload++;
		break;
	} ARGEND

	if(argc > 1)
		usage();

	if(argc == 1) {
		if(doload++)
			usage();
		file = strdup(argv[0]);
		if(file == nil)
			sysfatal("threadmain: could not save file name: %r");
		close(0);
		if(open(file, OREAD) != 0)
			sysfatal("threadmain: could not open file: %r");
	}

	if(initdraw(nil, nil, "Galaxy") < 0)
		sysfatal("initdraw failed: %r");
	if(mc = initmouse(nil, screen), mc == nil)
		sysfatal("initmouse failed: %r");

	dt² = dt*dt;
	orig = divpt(subpt(screen->r.max, screen->r.min), 2);
	orig = addpt(orig, screen->r.min);
	mkglxy();
	mkquads();
	if(doload) {
		load(0);
		close(0);
	}
	threadcreate(mousethread, nil, STK);
	threadcreate(resizethread, nil, STK);
	threadcreate(kbdthread, nil, STK);
	proccreate(simulate, nil, STK);
	threadexits(nil);
}
