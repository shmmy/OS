/*
 * monitorsem.c
 *
 * References from
 * A. Silberschatz, P. Galvin, and G. Gangne. Operating System Concept. John Wiley & Sons
 * 
 */

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <sys/types.h>

/* The functions are intentionally left blank :-) */
/* the function names are left the same for compatibility */

#include "monitorsem.h"

Monitor 
void monitorsem_init(struct monitorsem *sem, int val)
{
	freeBlock = n;
	nextIn = 0;
	nextOut = 0;
}

void monitorsem_wait(struct monitorsem *sem)
{
    if(freeBlock == n) //Nothing to read
	reader.wait;
	item a = buffer[nextOut];
	freeBlock++;
	nextOut = (nextOut + 1) % n;
	writer.signal;
	return item;
}

void monitorsem_signal(struct monitorsem *sem)
{
    
}

void monitorsem_destroy(struct monitorsem *sem)
{
    
}
