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
#include <fcntl.h>
#include <sys/stat.h>

#include <sys/types.h>
#include <sys/wait.h>

#include "mandel-lib.h"
#include "pipesem.h"

#define MANDEL_MAX_ITERATION 256
#define PROCS 2
#define RES 1024

/***************************
 * Compile-time parameters *
 ***************************/

/*
 * Output at the terminal is is x_chars wide by y_chars long
 */
int y_chars = RES;
int x_chars = 1.4*RES;
int start;
int end;


/*
 * For output to image file
 */
FILE * image;
int image_fd;
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
    double val;

    /* Find out the y value corresponding to this line */
    y = ymax - ystep * line;

    /* and iterate for all points on this line */
    for (x = xmin, n = 0; x <= xmax; x+= xstep, n++) {

        /* Compute the point's color value */
        val = mandel_iterations_at_point(x, y, MANDEL_MAX_ITERATION);
        val*=10;
        if (val > 255)
            val = 255;

        ///* And store it in the color_val[] array */
        //val = xterm_color(val);
        color_val[n] = val;
    }
}

void output_mandel_line_to_ppm(int color_val[])
{
    int i;
    int iocheck;
    unsigned char rgb[3];
    char rgb_trio[20];
    char nl = '\n';
    for(i=0;i<x_chars;i++)
    {
        if(color_val[i]==255)
        {
            rgb[2]=0;
            rgb[1]=0;
            rgb[0]=0;
        }
        else
        {
            rgb[2]=color_val[i];
            rgb[1]=color_val[i]/4;
            rgb[0]=color_val[i]/2;
        }
        snprintf(rgb_trio,20,"%d\t %d\t %d\t",rgb[0],rgb[1],rgb[2]); 
        iocheck = insist_write(image_fd,rgb_trio,strlen(rgb_trio));
        if(iocheck == -1)
        {
            perror("child:output_mandel:for");
            exit(1);
        }
    }
    iocheck = insist_write(image_fd,&nl,1);
    if(iocheck == -1)
    {
        perror("child:output_mandel");
        exit(1);
    }
}

void child(int ch_id,struct pipesem *mysem,struct pipesem *othersem)
{
    int line;
    int color_val[x_chars];
    for (line = start+ch_id; line < end; line+=PROCS) {
        //this can be parallel
        compute_mandel_line(line, color_val);
        //this has to be serial
        pipesem_wait(mysem);
        //output_mandel_line(1, color_val);
        output_mandel_line_to_ppm(color_val);
        pipesem_signal(othersem);
    }
    pipesem_destroy(mysem);
}

int main(int argc,char **argv)
{
    const char *filename;
    if(argc != 3)
    {
        start=0;
        end=y_chars;
        filename="mandel.ppm";

    }
    else
    {
        sscanf(argv[1],"%d",&start);
        sscanf(argv[2],"%d",&end);
        filename=argv[1];
    }

    int i;
    pid_t p;
    int status;
    struct pipesem sem[PROCS];
    image_fd = open(filename,O_CREAT | O_WRONLY | O_TRUNC,\
            S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH );
    
    if(start==0)
    {
        int iXmax = x_chars;
        int iYmax = y_chars;
        const int MaxColorComponentValue=255; 
        char *comment="# mandelbrot set";
        char header[200];
        int iocheck;
        snprintf(header,200,"P3\n%s\n%d\t %d\n%d\n",comment,iXmax,iYmax,MaxColorComponentValue);
        iocheck = insist_write(image_fd,header,strlen(header));
        if (iocheck == -1 )
        {
            perror("main:write");
            exit(1);
        }
    }


    xstep = (xmax - xmin) / x_chars;
    ystep = (ymax - ymin) / y_chars;

    /*
     * draw the Mandelbrot Set, one line at a time.
     * Output is sent to file descriptor '1', i.e., standard output.
     */
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

    close(image_fd);
    return 0;
}
