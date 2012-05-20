/*
 * pipesem.c
 */

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <sys/types.h>

/* The functions are intentionally left blank :-) */

#include "pipesem.h"

void pipesem_init(struct pipesem *sem, int val)
{
    if (pipe(&sem->rfd) == -1)
    {
        perror("pipe creation error");
        exit(EXIT_FAILURE);
    }
    int token = 1;
    int iocheck;
    while (val > 0)
    {
        iocheck= write(sem->wfd, &token, sizeof(int));
        if (iocheck == -1)
        {
            perror("write in pipe error");
            exit(1);
        }
        val-- ;
    }
}

void pipesem_wait(struct pipesem *sem)
{
    int iocheck;
    int token = 0;
    iocheck = read(sem->rfd, &token, sizeof(int));
    if (iocheck == -1)
    {
        perror("read from pipe error");
        exit(1);
    }
}

void pipesem_signal(struct pipesem *sem)
{
    int iocheck,token;
    iocheck = write(sem->wfd, &token, sizeof(int));
    if (iocheck == -1)
    {
        perror("write in pipe error");
        exit(1);
    }
}

void pipesem_destroy(struct pipesem *sem)
{
    close(sem->wfd);
    close(sem->rfd);
}
