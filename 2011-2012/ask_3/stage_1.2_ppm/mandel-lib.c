/*
 * mandel-lib.c
 *
 * A library with useful functions
 * for computing the Mandelbrot Set and handling a 256-color xterm.
 *
 */

#include <stdio.h>
#include <unistd.h>
#include <assert.h>
#include <string.h>
#include <math.h>
#include <stdlib.h>

#include "mandel-lib.h"


/*******************************************
 *                                         *
 * Functions to compute the Mandelbrot set *
 *                                         *
 *******************************************/

/*
 * This function takes a (x,y) point on the complex plane
 * and uses the escape time algorithm to return a color value
 * used to draw the Mandelbrot Set.
 */
double mandel_iterations_at_point(double x, double y, int max)
{
	double x0 = x;
	double y0 = y;
	int iter = 0;

	while ( (x * x + y * y <= 4) && iter < max) {
		double xt = x * x - y * y + x0;
		double yt = 2 * x * y + y0;

		x = xt;
		y = yt;

		++iter;
	}

	return iter+1-log(log(sqrt(x*x+y*y)))/log(2);
	//return iter;
}

/*
 * Insist until all count bytes beginning at
 * address buff have been written to file descriptor fd.
 */
ssize_t insist_write(int fd, const char *buf, size_t count)
{
	ssize_t ret;
	size_t orig_count = count;

	while (count > 0) {
		ret = write(fd, buf, count);
		if (ret < 0)
			return ret;
		buf += ret;
		count -= ret;
	}

	return orig_count;
}
