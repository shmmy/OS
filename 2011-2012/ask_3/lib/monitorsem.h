
/*
 * monitorsem.h
 *
 * An attempt to implement
 * semaphores over monitors.
 *
 * Vasilis Gerakaris 	<vgerak@gmail.com>
 * Grigoris Liras 		<gregliras@gmail.com>
 */

#ifndef MONITORSEM_H__
#define MONITORSEM_H__

struct monitorsem {
	/*
	 * Two file descriptors:
	 * one for the read and one for the write end of a monitor
	 */
	int rfd;
	int wfd;
};

/*
 * Function prototypes
 */
void monitorsem_init(struct monitorsem *sem, int val);
void monitorsem_wait(struct monitorsem *sem);
void monitorsem_signal(struct monitorsem *sem);
void monitorsem_destroy(struct monitorsem *sem);

#endif /* PIPESEM_H__ */
