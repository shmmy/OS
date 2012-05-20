#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <string.h>
#include <assert.h>

#include <sys/wait.h>
#include <sys/types.h>

#include "proc-common.h"
#include "request.h"
#include "queue.h"

/* Compile-time parameters. */
#define SCHED_TQ_SEC 2                /* time quantum */
#define TASK_NAME_SZ 60               /* maximum size for a task's name */
#define SHELL_EXECUTABLE_NAME "shell" /* executable for shell */

#define HIGH_STATE 1
#define LOW_STATE 0

static queue *current_proc;
static queue *high_proc;
static queue *low_proc;
static int *current_tasks;
static int high_tasks;
static int low_tasks;
static int current_state;
static sid_t super_id;

/* Print a list of all tasks currently being scheduled.  */
static void
sched_print_tasks( void )
{
    queue *buf;
    printf( "high priority\n" );
    buf = high_proc;
    print_q( buf, high_tasks );
    printf( "low priority\n" );
    buf = low_proc;
    print_q( buf, low_tasks );
}

/* Send SIGKILL to a task determined by the value of its
 * scheduler-specific id.
 */
static int
sched_kill_task_by_id(int id)
{
    if ( id == 0 )
    {
        fprintf(stderr,"Cannot kill shell\n");
        return -1;
    }
    fprintf(stderr,"DIE DIE DIE!!!!! %d\n",id);
    queue *buf = find_q( id, low_proc, low_tasks ); 
    if ( buf != NULL )
    {
        return ( kill( buf->pid, SIGKILL ) );
    }
    else
    {
        queue *buf = find_q( id, high_proc, high_tasks ); 
        if ( buf != NULL )
            return ( kill( buf->pid, SIGTERM ) );
    }
    return -1;
}


static void
state_check( void )
{
    if ( high_tasks > 0 && current_state == LOW_STATE )
    {
        fprintf(stderr,"high tasks only\n");
        low_proc = current_proc;
        current_tasks = &high_tasks;
        current_proc = high_proc;
        current_state = HIGH_STATE;
    }
    else if ( high_tasks == 0 && current_state == HIGH_STATE )
    {
        fprintf(stderr,"low tasks allowed\n");
        high_proc = current_proc;
        //comment this out later
        current_tasks = &low_tasks;
        current_proc = low_proc;
        current_state = LOW_STATE;
    }
}

/* Create a new task.  */
static void
sched_create_task( char *executable )
{
    pid_t p;
    p = fork();
    if ( p < 0 ) {
        perror( "scheduler: fork" );
        exit( 1 );
    }

    if ( p == 0 ) {
        /* Child */
        char *newargv[] = { executable, NULL, NULL, NULL };
        char *newenviron[] = { NULL };
        raise( SIGSTOP );
        execve( executable, newargv, newenviron );
        //kill( getpid(), SIGKILL );
        exit( -1 );
        
    }
    else {
        //this inserts the new proc in the right order
        //before the running proc (if on low)
        //or before low_proc (if running high)
        if ( current_state == LOW_STATE )
            insert_q( p, super_id++, current_proc );
        else
            insert_q( p, super_id++, low_proc );
        low_tasks++;
    }
    //assert(0 && "Please fill me!");
}

/* this will lower the priority of a task */
static void
sched_low_task_by_id( int id )
{
    pid_t pid_to_be;

    kill( current_proc -> pid, SIGSTOP );
    if ( high_proc -> id == id )
    {
        pid_to_be = high_proc->pid;
        high_proc = remove_q( high_proc );
            if ( current_proc -> id == id )
            {
                current_proc = high_proc;
            }
        high_tasks--;
        insert_q( pid_to_be, id, low_proc );
        low_tasks++;
    }
    else
    {
        queue *buf = find_q ( id, high_proc, high_tasks );
        if ( buf != NULL )
        {
            pid_to_be = buf->pid;
            if ( current_proc -> id == id )
            {
                current_proc = remove_q( buf );
            }
            else
            {
                remove_q( buf );
            }
            high_tasks--;
            insert_q( pid_to_be, id, low_proc );
            low_tasks++;
        }
        else
        {
            fprintf( stderr, "\t\tProcess not found in high priority queue\n" );
            fflush( stderr );
        }
    }
    state_check();
    kill( current_proc -> pid, SIGCONT );
}

/* this will get the priority of a task set to high */
static void
sched_high_task_by_id( int id )
{
    pid_t pid_to_be;
        kill( current_proc -> pid, SIGSTOP );
    if ( low_proc -> id == id )
    {
        pid_to_be = low_proc -> pid;
        low_proc = remove_q( low_proc );
            if ( current_proc -> id == id )
            {
                current_proc = low_proc;
            }
        low_tasks--;
        insert_q( pid_to_be, id, high_proc );
        high_tasks++;
    }
    else
    {
        queue *buf = find_q ( id, low_proc, low_tasks );
        if ( buf != NULL )
        {
            pid_to_be = buf -> pid;
            if ( current_proc -> id == id )
            {
                current_proc = remove_q( buf );
            }
            else
            {
                remove_q( buf );
            }
            low_tasks--;
            insert_q( pid_to_be, id, high_proc );
            high_tasks++;
        }
        else
        {
            fprintf( stderr, "\t\tProcess not found in low priority queue\n" );
            fflush( stderr );
        }
    }
    state_check();
    kill( current_proc -> pid, SIGCONT );
}

/* Process requests by the shell.  */
static int
process_request( struct request_struct *rq )
{
    switch ( rq->request_no ) {
        case REQ_PRINT_TASKS:
            sched_print_tasks();
            return 0;
        case REQ_KILL_TASK:
            return sched_kill_task_by_id(rq->task_arg);
        case REQ_EXEC_TASK:
            sched_create_task(rq->exec_task_arg);
            return 0;
        case REQ_HIGH_TASK:
            sched_high_task_by_id(rq->task_arg);
            return 0;
        case REQ_LOW_TASK:
            sched_low_task_by_id(rq->task_arg);
            return 0;
        default:
            return -ENOSYS;
    }
}

/* SIGALRM handler: Gets called whenever an a\t\tlarm goes off.
 * The time quantum of the currently executing process has expired,
 * so send it a SIGSTOP. The SIGCHLD handler will take care of
 * activating the next in line.
 */
static void
sigalrm_handler( int signum )
{
    /*
     * Stop currently running process
     * This will spawn SIGCHLD when the
     * child is stopped
     */
    //state_check();
    if ( ( (*current_tasks) > 0 ) && current_proc  )
    {
        fprintf( stderr, "\t\tSTOP\t\tid:%d\n", current_proc->id );
        fflush( stderr );
        kill( current_proc->pid, SIGSTOP );
    }
    alarm( SCHED_TQ_SEC );
}

/* SIGCHLD handler: Gets called whenever a process is stopped,
 * terminated due to a signal, or exits gracefully.
 *
 * If the currently executing task has been stopped,
 * it means its time quantum has expired and a new one has
 * to be activated.
 */
static void
sigchld_handler( int signum )
{
    int status;
    pid_t p;
    queue * buf;

    if ( *current_tasks > 0 && current_proc )
    {
        while( ( p = waitpid( -1, &status, WUNTRACED | WCONTINUED | WNOHANG ) ) > 0 )
        {
            //explain_wait_status(p,status);
            if ( WIFCONTINUED( status ) )
            {
                //fprintf(stderr,"CONTINUED nothing to do\n");
                return;
            }
            else if ( WIFSTOPPED( status ) )
            {
                //go on only if the current_proc got stopped
                if ( p == current_proc->pid )
                {
                    current_proc = next_q( current_proc );
                    kill( current_proc->pid, SIGCONT );
                    fprintf( stderr, "\t\tNEXT\t\tid:%d\n", current_proc -> id);
                    alarm( SCHED_TQ_SEC );
                    return;
                }
                else
                {
                    fprintf( stderr, "\t\tSIGSTOP irrelevant\n" );
                    fflush( stderr );
                    return;
                }
            }
            else if ( WIFEXITED( status ) )
            {
                fprintf( stderr, "\t\tDEAD\t\tid:%d\n", current_proc -> id );
                fflush( stderr );
                if ( current_proc -> pid == p )
                {
                    current_proc = remove_q( current_proc );
                    *current_tasks = *current_tasks-1;
                    if ( current_state  == HIGH_STATE )
                        high_proc = current_proc;
                    else
                        low_proc = current_proc;
                    state_check();
                }
                else 
                {
                    buf = find_q_with_pid( p, high_proc, high_tasks );
                    if ( buf != NULL )
                    {
                        remove_q( buf );
                        high_tasks--;
                    }
                    else
                    {
                        buf = find_q_with_pid( p, low_proc, low_tasks );
                        if ( buf != NULL )
                        {
                            remove_q( buf );
                            low_tasks--;
                        }
                    }
                    state_check();
                }
                if ( *current_tasks )
                {
                    kill( current_proc->pid, SIGCONT );
                    fprintf( stderr, "\t\tNEXT\t\tid:%d\n", current_proc -> id);
                    alarm( SCHED_TQ_SEC );
                    return;
                }
                else
                {
                    fprintf( stderr, "\t\tboth queues are empty\n" );
                    fflush( stderr );
                    exit( 0 );
                }

            }
            else if ( WIFSIGNALED( status ) )
            {
                if ( p == current_proc->pid)
                {
                    current_proc = remove_q( current_proc );
                    if ( current_state  == HIGH_STATE )
                        high_proc = current_proc;
                    else
                        low_proc = current_proc;
                    *current_tasks = *current_tasks-1;
                    state_check();
                    alarm( SCHED_TQ_SEC );
                }
                else
                {
                    buf = find_q_with_pid( p, high_proc, high_tasks );
                    if ( buf != NULL )
                    {
                        remove_q( buf );
                        high_tasks--;
                    }
                    else
                    {
                        buf = find_q_with_pid( p, low_proc, low_tasks );
                        if ( buf != NULL )
                        {
                            remove_q( buf );
                            low_tasks--;
                        }
                    }
                    state_check();
                }
                if ( *current_tasks )
                {
                    kill( current_proc->pid, SIGCONT );
                    fprintf( stderr, "\t\tNEXT\t\tid:%d\n", current_proc -> id);
                    return;
                    //fprintf(stderr,"i just died in your arms tonight\n");
                }
                else
                {
                    fprintf( stderr,"\t\tboth queues are empty\n" );
                    fflush( stderr );
                    exit( 0 );
                }

            }
        }
    }
}
/* Disable delivery of SIGALRM and SIGCHLD. */
static void
signals_disable( void )
{
    sigset_t sigset;
    sigemptyset( &sigset );
    sigaddset( &sigset, SIGALRM );
    sigaddset( &sigset, SIGCHLD );
    if ( sigprocmask(SIG_BLOCK, &sigset, NULL ) < 0 ) {
        perror( "signals_disable: sigprocmask" );
        exit( 1 );
    }
}

/* Enable delivery of SIGALRM and SIGCHLD.  */
static void
signals_enable( void )
{
    sigset_t sigset;

    sigemptyset( &sigset );
    sigaddset( &sigset, SIGALRM );
    sigaddset( &sigset, SIGCHLD );
    if ( sigprocmask(SIG_UNBLOCK, &sigset, NULL ) < 0 ) {
        perror( "signals_enable: sigprocmask" );
        exit( 1 );
    }
}


/* Install two signal handlers.
 * One for SIGCHLD, one for SIGALRM.
 * Make sure both signals are masked when one of them is running.
 */
static void
install_signal_handlers( void )
{
    sigset_t sigset;
    struct sigaction sa;

    sa.sa_handler = sigchld_handler;
    sa.sa_flags = SA_RESTART;
    sigemptyset( &sigset );
    sigaddset( &sigset, SIGCHLD );
    sigaddset( &sigset, SIGALRM );
    sa.sa_mask = sigset;
    if ( sigaction(SIGCHLD, &sa, NULL ) < 0 ) {
        perror( "sigaction: sigchld" );
        exit( 1 );
    }

    sa.sa_handler = sigalrm_handler;
    if ( sigaction( SIGALRM, &sa, NULL ) < 0 ) {
        perror( "sigaction: sigalrm" );
        exit( 1 );
    }

    /*
     * Ignore SIGPIPE, so that write()s to pipes
     * with no reader do not result in us being killed,
     * and write() returns EPIPE instead.
     */
    if ( signal(SIGPIPE, SIG_IGN) == SIG_ERR ) {
        perror( "signal: sigpipe" );
        exit( 1 );
    }
}

static void
do_shell( char *executable, int wfd, int rfd )
{
    char arg1[10], arg2[10];
    char *newargv[] = { executable, NULL, NULL, NULL };
    char *newenviron[] = { NULL };

    sprintf( arg1, "%05d", wfd );
    sprintf( arg2, "%05d", rfd );
    newargv[1] = arg1;
    newargv[2] = arg2;

    raise( SIGSTOP );
    execve( executable, newargv, newenviron );

    /* execve() only returns on error */
    perror( "scheduler: child: execve" );
    exit( 1 );
}

/* Create a new shell task.
 *
 * The shell gets special treatment:
 * two pipes are created for communication and passed
 * as command-line arguments to the executable.
 */
static void
sched_create_shell( char *executable, int *request_fd, int *return_fd )
{
    pid_t p;
    int pfds_rq[2], pfds_ret[2];

    if ( pipe( pfds_rq ) < 0 || pipe( pfds_ret ) < 0 ) {
        perror( "pipe" );
        exit( 1 );
    }

    p = fork();
    if ( p < 0 ) {
        perror( "scheduler: fork" );
        exit( 1 );
    }

    if ( p == 0 ) {
        /* Child */
        close( pfds_rq[0] );
        close( pfds_ret[1] );
        do_shell( executable, pfds_rq[1], pfds_ret[0] );
        assert( 0 );
    }
    /* Parent */
    insert_q( p, super_id++, low_proc );
    low_tasks++;
    close( pfds_rq[1] );
    close( pfds_ret[0] );
    *request_fd = pfds_rq[0];
    *return_fd = pfds_ret[1];
}

static void
shell_request_loop( int request_fd, int return_fd )
{
    int ret;
    struct request_struct rq;

    /*
     * Keep receiving requests from the shell.
     */
    for ( ;; ) {
        if ( read( request_fd, &rq, sizeof( rq ) ) != sizeof( rq ) ) {
            perror( "scheduler: read from shell" );
            fprintf( stderr, "Scheduler: giving up on shell request processing.\n" );
            fflush( stderr );
            break;
        }

        signals_disable();
        ret = process_request( &rq );
        signals_enable();

        if ( write( return_fd, &ret, sizeof( ret ) ) != sizeof( ret ) ) {
            perror( "scheduler: write to shell" );
            fprintf( stderr, "Scheduler: giving up on shell request processing.\n" );
            fflush( stderr );
            break;
        }
    }
}

int main(void)
{
    int nproc;
    /* Two file descriptors for communication with the shell */
    static int request_fd, return_fd;

    low_tasks=0;
    super_id = 0;
    current_tasks = &low_tasks;
    current_state = LOW_STATE;
    high_tasks=0;
    nproc = 1;
    low_proc = ( queue * ) malloc( sizeof( queue ) );
    if ( !low_proc )
    {
        perror( "main: init, bad alloc" );
        exit( 1 );
    }
    high_proc = ( queue * ) malloc( sizeof( queue ) );
    if ( !high_proc )
    {
        perror( "main: init, bad alloc" );
        exit( 1 );
    }
    current_proc = low_proc;
    init_q( low_proc );
    init_q( high_proc );
    /* Create the shell. */
    sched_create_shell( SHELL_EXECUTABLE_NAME, &request_fd, &return_fd );
    /* TODO: add the shell to the scheduler's tasks */

    /*
     * For each of argv[1] to argv[argc - 1],
     * create a new child process, add it to the process list.
     */


    /* Wait for all children to raise SIGSTOP before exec()ing. */
    wait_for_ready_children( nproc );

    ///* Install SIGALRM and SIGCHLD handlers. */
    install_signal_handlers();

    fprintf( stderr, "\t\thandlers installed, now running\n" );
    fflush( stderr );
    kill( low_proc->pid, SIGCONT );
    fprintf( stderr, "\t\tNEXT\t\tid:%d\n", low_proc -> id);
    alarm( SCHED_TQ_SEC );

    //if (nproc == 0) {
    //    fprintf(stderr, "Scheduler: No tasks. Exiting...\n");
    //    exit(1);
    //}

    shell_request_loop( request_fd, return_fd );

    /* Now that the shell is gone, just loop forever
     * until we exit from inside a signal handler.
     */
    while ( pause() )
        ;

    /* Unreachable */
    fprintf( stderr, "Internal error: Reached unreachable point\n" );
    fflush( stderr );
    return 1;
}
