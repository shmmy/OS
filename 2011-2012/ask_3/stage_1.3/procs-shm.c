/*
 * procs-shm.c
 *
 * A program to create three processes,
 * working with a shared memory area.
 *
 * Vangelis Koukis <vkoukis@cslab.ece.ntua.gr>
 * Operating Systems
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

#include "proc-common.h"
#include "pipesem.h"

#define K 5 //for general case B: n = n-K

/*
 * This is a pointer to a shared memory area.
 * It holds an integer value, that is manipulated
 * concurrently by all children processes.
 */
int *shared_memory;
/* ... */

/* Proc A: n = n + 1 */
void proc_A(struct pipesem *semA, struct pipesem *semB, struct pipesem *semC)
{
	volatile int *n = &shared_memory[0];
	/* ... */

	for (;;) {
		/* ... */
                pipesem_wait(semA);
                printf("A up!\n");
		*n = *n + 1;
                pipesem_signal(semB);
		/* ... */
	}

	exit(0);
}

/* Proc B: n = n - 2 */
void proc_B(struct pipesem *semA, struct pipesem *semB, struct pipesem *semC)
{
	volatile int *n = &shared_memory[0];
	/* ... */
        int j;

	for (;;) {
		/* ... */
                pipesem_wait(semB);
                /* General case B: n = n-K */
                for (j = 1; j < K; j++) {
                    pipesem_signal(semA);
                    pipesem_wait(semB);
                }
                printf("B up!\n");
                *n = *n - K;
                pipesem_signal(semC);
                /* ... */
        }

	exit(0);
}

/* Proc C: print n */
void proc_C(struct pipesem *semA, struct pipesem *semB, struct pipesem *semC)
{
	int val;

	volatile int *n = &shared_memory[0];
	/* ... */

	for (;;) {
		/* ... */
                pipesem_wait(semC);
                printf("C up!\n");
		val = *n;
		printf("Proc C: n = %d\n", val);
		if (val != 1) {
			printf("     ...Aaaaaargh!\n");
		}
                pipesem_signal(semA);
		/* ... */
	}
	exit(0);
}

/*
 * Use a NULL-terminated array of pointers to functions.
 * Each child process gets to call a different pointer.
 */
typedef void proc_fn_t(struct pipesem *semA,struct pipesem *semB, struct pipesem *semC);
static proc_fn_t *proc_funcs[] = {proc_A, proc_B, proc_C, NULL};

int main(void)
{
	int i;
	int status;
	pid_t p;
	proc_fn_t *proc_fn;
        struct pipesem semA, semB, semC;

	/* ... */
        pipesem_init(&semA,1);
        pipesem_init(&semB,0);
        pipesem_init(&semC,0);

	/* Create a shared memory area */
	shared_memory = create_shared_memory_area(sizeof(int));
	*shared_memory = 1;

	for(i = 0; (proc_fn = proc_funcs[i]) != NULL; i++) {
		printf("%lu fork()\n", (unsigned long)getpid());
		p = fork();
		if (p < 0) {
			perror("parent: fork");
			exit(1);
		}
		if (p != 0) {
			/* Father */
			continue;
		}
		/* Child */
		proc_fn(&semA, &semB, &semC);
		assert(0);
	}

	/* Parent waits for all children to terminate */
	for (; i >0; i--)
		wait(&status);

	return 0;
}
