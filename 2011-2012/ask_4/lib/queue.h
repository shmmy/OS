/* -.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.

* File Name : queue.h

* Purpose :

* Creation Date : 21-01-2012

* Last Modified : Thu 09 Feb 2012 11:40:12 AM EET

* Created By : Greg Liras <gregliras@gmail.com>

_._._._._._._._._._._._._._._._._._._._._.*/
#ifndef QUEUE_H
#define QUEUE_H 
#include <sys/types.h>
#include <stdlib.h>
#include <stdio.h>

typedef struct __queue queue;
typedef int sid_t;
struct __queue {
    pid_t pid;
    int id;
    queue * prev;
    queue * next;
};

void init_q(queue *head);
void print_q(queue *q,int len);
void insert_q(pid_t pid,sid_t id, queue *q);
queue *find_q(sid_t p,queue *q,int len);
queue *find_q_with_pid(pid_t p,queue *q,int len);
queue *remove_q(queue *q);
queue *next_q(queue *q);
#endif
