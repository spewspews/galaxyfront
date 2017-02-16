#include <u.h>
#include <libc.h>
#include <bio.h>
#include <draw.h>
#include "galaxy.h"

Vector o, d;
double
	sp = 50, sprand,
	m = 100, mrand,
	v, vrand,
	r, rrand,
	c;
int new;

void
usage(void)
{
	fprint(2, "Usage: %s [-s spacing[±rand]]\n\t[-m mass[±rand]] [-v vel[±rand]]\n\t[-r angvel[±rand]] [-d xdir,ydir]\n\t[-o xoff,yoff] [-f file]\n\t[-c] [-i] size\n", argv0);
	exits("usage");
}

Vector
polar(double ang, double mag)
{
	Vector v;

	v.x = cos(ang)*mag;
	v.y = sin(ang)*mag;
	return v;
}

void quadcalc(QB, Body*, double){}
Image *randcol(void){ return nil; }

Vector
getvec(char *str)
{
	Vector v;

	v.x = strtod(str, &str);
	if(*str != ',')
		usage();
	v.y = strtod(str+1, nil);
	return v;
}

double
getvals(char *str, double *rand)
{
	Rune r;
	double val;
	int i;

	val = strtod(str, &str);
	i = chartorune(&r, str);
	if(r == L'±')
		*rand = strtod(str+i, nil);
	else
		*rand = 0;
	return val;
}

#define RAND(r)	((r)*(frand()*2 - 1))

void
mkbodies(double lim)
{
	Body *b;
	double x, y;

	for(x = -lim/2; x < lim/2; x += sp)
	for(y = -lim/2; y < lim/2; y += sp) {
		b = body();
		b->x = x + RAND(sprand);
		b->y = y + RAND(sprand);
		b->v = polar(frand()*π2, v+RAND(vrand));
		b->v.x += d.x;
		b->v.y += d.y;
		b->mass = m + RAND(mrand);
	}
	center();
}

void
main(int argc, char **argv)
{
	static Biobuf bout;
	Body *b;
	double lim;
	int fd;

	srand(time(nil));
	fmtinstall('B', Bfmt);
	glxyinit();

	ARGBEGIN {
	case 'f':
		fd = open(EARGF(usage()), OREAD);
		if(fd < 0)
			sysfatal("Could not open file %s: %r", *argv);
		readglxy(fd);
		close(fd);
		break;
	case 'i':
		readglxy(0);
		break;
	case 's':
		sp = getvals(EARGF(usage()), &sprand);
		break;
	case 'm':
		m = getvals(EARGF(usage()), &mrand);
		break;
	case 'v':
		v = getvals(EARGF(usage()), &vrand);
		break;
	case 'r':
		r = getvals(EARGF(usage()), &rrand);
		break;
	case 'c':
		c++;
		break;
	case 'o':
		o = getvec(EARGF(usage()));
		break;
	case 'd':
		d = getvec(EARGF(usage()));
		break;
	} ARGEND

	if(argc != 1)
		usage();

	new = glxy.l;
	lim = strtod(*argv, nil);
	mkbodies(lim);

	Binit(&bout, 1, OWRITE);
	for(b = glxy.a; b < glxy.a + new; b++)
		Bprint(&bout, "%B\n", b);

	for(b = glxy.a+new; b < glxy.a+glxy.l; b++) {
		if(c)
		if(hypot(b->x, b->y) > lim/2)
			continue;
		b->x += o.x;
		b->y += o.y;
		Bprint(&bout, "%B\n", b);
	}
	Bterm(&bout);

	exits(nil);
}
