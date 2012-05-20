/*
 * mandel.c
 *
 * A program to draw the Mandelbrot Set on a 256-color xterm.
 *
 */

#include <stdio.h>
#include <unistd.h>
#include <assert.h>
#include <string.h>
#include <math.h>
#include <stdlib.h>

#include <sys/types.h>
#include <sys/wait.h>

#include "mandel-lib.h"
#include "pipesem.h"

#define MANDEL_MAX_ITERATION 100000
#define PROCS 30

/***************************
 * Compile-time parameters *
 ***************************/

/*
 * Output at the terminal is is x_chars wide by y_chars long
 */
int y_chars = 50;
int x_chars = 90;

/*
 * The part of the complex plane to be drawn:
 * upper left corner is (xmin, ymax), lower right corner is (xmax, ymin)
 */
double xmin = -1.8, xmax = 1.0;
double ymin = -1.0, ymax = 1.0;

/*
 * Every character in the final output is
 * xstep x ystep units wide on the complex plane.
 */
double xstep;
double ystep;

/*
 * This function computes a line of output
 * as an array of x_char color values.
 */
void compute_mandel_line(int line, int color_val[])
{
    /*
     * x and y traverse the complex plane.
     */
    double x, y;

    int n;
    int val;

    /* Find out the y value corresponding to this line */
    y = ymax - ystep * line;

    /* and iterate for all points on this line */
    for (x = xmin, n = 0; x <= xmax; x+= xstep, n++) {

        /* Compute the point's color value */
        val = mandel_iterations_at_point(x, y, MANDEL_MAX_ITERATION);
        if (val > 255)
            val = 255;

        /* And store it in the color_val[] array */
        val = xterm_color(val);
        color_val[n] = val;
    }
}

/*
 * This function outputs an array of x_char color values
 * to a 256-color xterm.
 */
void output_mandel_line(int fd, int color_val[])
{
    int i;

    char point ='@';
    char newline='\n';

    for (i = 0; i < x_chars; i++) {
        /* Set the current color, then output the point */
        set_xterm_color(fd, color_val[i]);
        if (write(fd, &point, 1) != 1) {
            perror("compute_and_output_mandel_line: write point");
            exit(1);
        }
    }

    /* Now that the line is done, output a newline character */
    if (write(fd, &newline, 1) != 1) {
        perror("compute_and_output_mandel_line: write newline");
        exit(1);
    }
}

void compute_and_output_mandel_line(int fd, int line)
{
    /*
     * A temporary array, used to hold color values for the line being drawn
     */
    int color_val[x_chars];

    compute_mandel_line(line, color_val);
    output_mandel_line(fd, color_val);
}

static void
sigint_handler(int signum)
{
    reset_xterm_color(1);
    exit(1);
}
static void
install_signal_handler(void)
{
    sigset_t sigset;
    struct sigaction sa;

    sa.sa_handler = sigint_handler;
    sa.sa_flags = SA_RESTART ;
    sigemptyset(&sigset);
    sigaddset(&sigset, SIGINT);
    sa.sa_mask = sigset;
    if (sigaction(SIGINT, &sa, NULL) < 0) {
        perror("sigaction: sigint");
        exit(1);
    }
}
void child(int ch_id,struct pipesem *mysem,struct pipesem *othersem)
{
    int line;
    int color_val[x_chars];
    for (line = ch_id; line < y_chars; line+=PROCS) {
        //this can be parallel
        compute_mandel_line(line, color_val);
        //this has to be serial
        pipesem_wait(mysem);
        output_mandel_line(1, color_val);
        pipesem_signal(othersem);
    }
    pipesem_destroy(mysem);
}

int main(void)
{
    int i;
    pid_t p;
    int status;
    struct pipesem sem[PROCS];
    //struct pipesem semC;
    //struct pipesem sem2,sem3;
    //pipesem_init(&sem0,0);
    //pipesem_init(&sem1,0);
    //pipesem_init(&sem2,0);
    //pipesem_init(&sem3,0);
    //pipesem_init(&semC,0);

    xstep = (xmax - xmin) / x_chars;
    ystep = (ymax - ymin) / y_chars;

    /*
     * draw the Mandelbrot Set, one line at a time.
     * Output is sent to file descriptor '1', i.e., standard output.
     */
    /*
     * setup SIGINT handler
     */
    install_signal_handler();
    /* Create a child */
    for(i=0;i<PROCS;i++)
    {
        pipesem_init(&sem[i],0);
    }
    for(i=0;i<PROCS;i++)
    {
        p = fork();
        if (p < 0) {
            perror("parent: fork");
            exit(1);
        }
        if (p == 0) {
            child(i,&sem[i],&sem[(i+1)%PROCS]);
            exit(1);
        }
    }

    pipesem_signal(sem);

    for(i=0;i<PROCS;i++)
    {
        wait(&status);
    }

    reset_xterm_color(1);
    return 0;
}
