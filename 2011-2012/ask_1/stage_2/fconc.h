/* -.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.

 * File Name : fconc.h

 * Last Modified : Thu 24 Nov 2011 04:47:01 PM EET

 * Created By : Greg Liras <gregliras@gmail.com>

 * Created By : Vasilis Gerakaris <vgerak@gmail.com>

 _._._._._._._._._._._._._._._._._._._._._.*/

#ifndef FCONC_H
#define FCONC_H

#ifndef BUFFER_SIZE
#define BUFFER_SIZE 1024
#endif //BUFFER_SIZE

#include <unistd.h>
#include <fcntl.h>
#include <sys/errno.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <stdio.h>
#include <sysexits.h>
#include <string.h>

void doWrite(int fd, const char *buff, int len);
void write_file(int fd, const char *infile);
void lock_file(int fd,int lock_type);
void unlock_file(int fd);
#endif //FCONC_H
