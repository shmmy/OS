/* -.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.

* File Name : queue.c

* Purpose :

* Creation Date : 21-01-2012

* Last Modified : Sat 18 Feb 2012 06:36:17 PM EET

* Created By : Greg Liras <gregliras@gmail.com>

_._._._._._._._._._._._._._._._._._._._._.*/
#include "queue.h"

void insert_q( pid_t pid, sid_t id, queue *q )
{
    /*
     * insert a new pid in the queue
     * before q 
     * this way it is appended at the end
     * for round robin implementation
     */
    if( q->pid != 0 )
    {
        queue *nq = ( queue *) malloc ( sizeof( queue ) );
        queue *prq = q -> prev;
        if( nq == NULL )
        {
            perror("queue:insert, bad allocation");
            exit(EXIT_FAILURE);
        }
        nq -> next = q;
        nq -> prev = prq;
        prq -> next = nq;
        q -> prev = nq;
        nq -> pid = pid;
        nq -> id = id;
    }
    else
    {
        init_q( q );
        q->pid = pid;
        q->id = id;
    }
}

inline
queue *next_q( queue *q )
{
    /*
     * returns the next element
     * in the queue
     */
    return q->next;
}

queue *remove_q( queue *q )
{
    /*
     * deletes an element and
     * returns the next one
     */
    //p -> next = q -> next;
    if ( q == q -> next )
    {
        init_q( q );
        return q;
    }
    queue *nq = q -> next;
    queue *pq = q -> prev;
    nq -> prev = pq;
    pq -> next = nq;

    free( q );
    return nq;
}

void init_q( queue *head )
{
    /*
     * initialize the queue
     */
    head -> pid = 0;
    head -> id = -1;
    head -> prev = head;
    head -> next = head;
}

void print_q(queue *q,int len)
{
    int i;
    for ( i = 0 ; i < len ; ++i )
    {
        fprintf( stdout, "sid:\t%d pid:\t%d prog\n",q->id,q->pid);
        fflush( stdout );
        q=next_q(q);
    }
}

queue *find_q( sid_t id,queue *q, int len )
{
    queue *buf = q;
    int i;
    for( i = 0 ; i < len ; ++i )
    {
        if ( buf->id == id )
        {
            return buf;
        }
        buf = next_q( buf );
    }
    return NULL;
}

queue *find_q_with_pid( pid_t pid,queue *q, int len )
{
    queue *buf = q;
    int i;
    for( i = 0 ; i < len ; ++i )
    {
        if ( buf->pid == pid )
        {
            return buf;
        }
        buf = next_q( buf );
    }
    return NULL;
}
