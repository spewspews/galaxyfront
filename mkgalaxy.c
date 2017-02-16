#include <u.h>
#include <libc.h>
#include <bio.h>
#include <draw.h>
#include "galaxy.h"

double
	d = 50, drand,
	m = 10, mrand,
	v, vrand,
	r, rrand,
	c;

void
usage(void)
{
	fprint(2, "Usage: %s [-d dist[±rand]] [-m mass[±rand]] [-v vel[±rand]] [-r rotvel[±rand]] [-c] nbody\n", argv0);
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
circle(long){}

void
square(long n)
{
	Body *b;
	long sq;
	int i, j;

	sq = sqrt(n);
	for(i = 0; i < sq; i++)
	for(j = 0; j < sq; j++) {
		b = body();
		b->x = i*(d + RAND(drand));
		b->y = j*(d + RAND(drand));
		b->v = polar(frand()*π2, v+RAND(vrand));
		b->mass = m + RAND(mrand);
	}
	center();
}

void
main(int argc, char **argv)
{
	static Biobuf bout;
	Body *b;
	long n;

	srand(time(nil));

	ARGBEGIN {
	case 'd':
		d = getvals(EARGF(usage()), &drand);
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
	} ARGEND

	if(argc != 1)
		usage();

	fmtinstall('B', Bfmt);
	mkglxy();

	n = strtol(argv[0], nil, 0);

	if(c)
		circle(n);
	else
		square(n);

	Binit(&bout, 1, OWRITE);
	for(b = glxy.a; b < glxy.a + glxy.l; b++)
		Bprint(&bout, "%B\n", b);
	Bterm(&bout);

	exits(nil);
}
