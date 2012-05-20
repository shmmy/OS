/*
 * mandel-lib.h
 *
 * A library with useful functions
 * for computing the Mandelbrot Set and handling a 256-color xterm.
 *
 */

#ifndef MANDEL_LIB_H__
#define MANDEL_LIB_H__

/* Function prototypes */
double mandel_iterations_at_point(double x, double y, int max);
ssize_t insist_write(int fd, const char *buf, size_t count);

#endif /* MANDEL_LIB_H__ */
