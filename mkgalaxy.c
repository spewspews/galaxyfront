#include <u.h>
#include <libc.h>
#include "galaxy.h"

double
	d = 50, drand,
	m = 10, mrand
	v, vrand,
	r, rrand,
	Λ, c;

void
usage(void)
{
	fprint(2, "Usage: %s [-d dist[±rand]] [-m mass[±rand]] [-v vel[±rand]] [-r rotvel[±rand]] [-Λ cosm] [-s scale] [-c] nbody\n", argv0);
	exits("usage");
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

void calcforces(Body*){}

void
main(int argc, char **argv)
{
	double dvar;
	long i, j, ij, n, l, sq, space;

	srand(time(nil));

	ARGBEGIN {
	case 'd':
		d = getvals(EARGF(usage()), drand);
		break;
	case 'm':
		m = getvals(EARGF(usage()), mrand);
		break;
	case 'v':
		v = getvals(EARGF(usage()), vrand);
		break;
	case 'r':
		r = getvals(EARGF(usage()), rrand);
		break;
	case L'Λ':
		Λ = strtod(EARGF(usage()), nil);
		break;
	case 's':
		scale = strtod(EARGF(usage()), nil);
		break;
	case 'c':
		c++;
		break;
	} ARGEND

	if(argc != 1)
		usage();
	n = strtol(argv[0], nil, 0);

	exits(nil);
}
