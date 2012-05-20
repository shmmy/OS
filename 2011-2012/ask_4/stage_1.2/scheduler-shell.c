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

queue *current_proc;
int tasks;
sid_t super_id;

/* Print a list of all tasks currently being scheduled.  */
static void
sched_print_tasks(void)
{
    print_q( current_proc, tasks );
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
    queue *buf = find_q( id, current_proc, tasks ); 
    if ( buf != NULL )
        return ( kill( buf->pid, SIGKILL ) );
    return -1;
}


/* Create a new task.  */
static void
sched_create_task(char *executable)
{   
    pid_t p;
    p = fork();
    if (p < 0) {
        perror("scheduler: fork");
        exit(1);
    }

    if (p == 0) {
        /* Child */
        char *newargv[] = { executable, NULL, NULL, NULL };
        char *newenviron[] = { NULL };
        raise(SIGSTOP);
        execve(executable, newargv, newenviron);
        perror( "execve failed!!" );
        exit( EXIT_FAILURE );
    }
    else {
        insert_q( p, super_id++, current_proc );
        tasks++;
    }
    //assert(0 && "Please fill me!");
}

/* Process requests by the shell.  */
static int
process_request(struct request_struct *rq)
{
    switch (rq->request_no) {
        case REQ_PRINT_TASKS:
            sched_print_tasks();
            return 0;

        case REQ_KILL_TASK:
            return sched_kill_task_by_id(rq->task_arg);

        case REQ_EXEC_TASK:
            sched_create_task(rq->exec_task_arg);
            return 0;

        default:
            return -ENOSYS;
    }
}

/* SIGALRM handler: Gets called whenever an alarm goes off.
 * The time quantum of the currently executing process has expired,
 * so send it a SIGSTOP. The SIGCHLD handler will take care of
 * activating the next in line.
 */
static void
sigalrm_handler(int signum)
{
    /*
     * Stop currently running process
     * This will spawn SIGCHLD when the
     * child is stopped
     */
    if ( tasks > 0 && current_proc )
    {
        kill( current_proc->pid, SIGSTOP );
        fprintf( stderr, "\t\tSTOP\t\tid:%d\n", current_proc->id );
        fflush( stderr );
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
sigchld_handler(int signum)
{
    int status;
    pid_t p;
    queue * buf;

    if ( tasks > 0 && current_proc )
    {
        while( ( p = waitpid( -1, &status, WUNTRACED | WCONTINUED | WNOHANG ) ) > 0 )
        {
            //explain_wait_status(p,status);
            if ( WIFCONTINUED( status ) )
            {
                //fprintf(stderr,"CONTINUED nothing to do\n");
                fprintf( stderr, "\t\tSIGSTOP irrelevant\n" );
                fflush( stderr );
                return;
            }

            else if ( WIFSTOPPED( status ) && ( p == current_proc -> pid ) )
            {
                current_proc = next_q( current_proc );
                if ( current_proc )
                {
                    kill( current_proc->pid, SIGCONT );
                    //fprintf(stderr,"NEEEEEEEEEEXT\n");
                    fprintf( stderr, "\t\tNEXT\t\tid:%d\n", current_proc -> id);
                    fflush( stderr );
                    alarm( SCHED_TQ_SEC );
                }
                else
                {
                    fprintf(stderr,"empty queue\n");
                    fflush(stderr);
                    alarm( SCHED_TQ_SEC );
                }

            }
            else if ( WIFEXITED( status ) || WIFSIGNALED( status )  )
            {
                fprintf( stderr, "\t\tDEAD\t\tid:%d\n", current_proc -> id );
                fflush( stderr );
                if ( p == current_proc->pid)
                {
                    current_proc = remove_q(current_proc);
                    alarm( SCHED_TQ_SEC );
                }
                else
                {
                    buf = find_q_with_pid( p, current_proc, tasks );
                    if ( buf )
                        buf = remove_q( buf );
                }
                tasks--;
                if ( tasks )
                {
                    kill( current_proc->pid, SIGCONT );
                    fprintf( stderr, "\t\tNEXT\t\tid:%d\n", current_proc -> id);
                    fflush( stderr );
                }
                else
                {
                    fprintf(stderr,"empty queue, exiting\n");
                    fflush(stderr);
                    exit( EXIT_SUCCESS );

                }

            }
        }
    }
}

/* Disable delivery of SIGALRM and SIGCHLD. */
static void
signals_disable(void)
{
    sigset_t sigset;

    sigemptyset(&sigset);
    sigaddset(&sigset, SIGALRM);
    sigaddset(&sigset, SIGCHLD);
    if (sigprocmask(SIG_BLOCK, &sigset, NULL) < 0) {
        perror("signals_disable: sigprocmask");
        exit(1);
    }
}

/* Enable delivery of SIGALRM and SIGCHLD.  */
static void
signals_enable(void)
{
    sigset_t sigset;

    sigemptyset(&sigset);
    sigaddset(&sigset, SIGALRM);
    sigaddset(&sigset, SIGCHLD);
    if (sigprocmask(SIG_UNBLOCK, &sigset, NULL) < 0) {
        perror("signals_enable: sigprocmask");
        exit(1);
    }
}


/* Install two signal handlers.
 * One for SIGCHLD, one for SIGALRM.
 * Make sure both signals are masked when one of them is running.
 */
static void
install_signal_handlers(void)
{
    sigset_t sigset;
    struct sigaction sa;

    sa.sa_handler = sigchld_handler;
    sa.sa_flags = SA_RESTART;
    sigemptyset(&sigset);
    sigaddset(&sigset, SIGCHLD);
    sigaddset(&sigset, SIGALRM);
    sa.sa_mask = sigset;
    if (sigaction(SIGCHLD, &sa, NULL) < 0) {
        perror("sigaction: sigchld");
        exit(1);
    }

    sa.sa_handler = sigalrm_handler;
    if (sigaction(SIGALRM, &sa, NULL) < 0) {
        perror("sigaction: sigalrm");
        exit(1);
    }

    /*
     * Ignore SIGPIPE, so that write()s to pipes
     * with no reader do not result in us being killed,
     * and write() returns EPIPE instead.
     */
    if (signal(SIGPIPE, SIG_IGN) < 0) {
        perror("signal: sigpipe");
        exit(1);
    }
}

static void
do_shell(char *executable, int wfd, int rfd)
{
    char arg1[10], arg2[10];
    char *newargv[] = { executable, NULL, NULL, NULL };
    char *newenviron[] = { NULL };

    sprintf(arg1, "%05d", wfd);
    sprintf(arg2, "%05d", rfd);
    newargv[1] = arg1;
    newargv[2] = arg2;

    raise(SIGSTOP);
    execve(executable, newargv, newenviron);

    /* execve() only returns on error */
    perror("scheduler: child: execve");
    exit(1);
}

/* Create a new shell task.
 *
 * The shell gets special treatment:
 * two pipes are created for communication and passed
 * as command-line arguments to the executable.
 */
static void
sched_create_shell(char *executable, int *request_fd, int *return_fd)
{
    pid_t p;
    int pfds_rq[2], pfds_ret[2];

    if (pipe(pfds_rq) < 0 || pipe(pfds_ret) < 0) {
        perror("pipe");
        exit(1);
    }

    p = fork();
    if (p < 0) {
        perror("scheduler: fork");
        exit(1);
    }

    if (p == 0) {
        /* Child */
        close(pfds_rq[0]);
        close(pfds_ret[1]);
        do_shell(executable, pfds_rq[1], pfds_ret[0]);
        assert(0);
    }
    /* Parent */
    insert_q(p, super_id++, current_proc);
    tasks++;
    close(pfds_rq[1]);
    close(pfds_ret[0]);
    *request_fd = pfds_rq[0];
    *return_fd = pfds_ret[1];
}

static void
shell_request_loop(int request_fd, int return_fd)
{
    int ret;
    struct request_struct rq;

    /*
     * Keep receiving requests from the shell.
     */
    for (;;) {
        if (read(request_fd, &rq, sizeof(rq)) != sizeof(rq)) {
            perror("scheduler: read from shell");
            fprintf(stderr, "Scheduler: giving up on shell request processing.\n");
            break;
        }

        signals_disable();
        ret = process_request(&rq);
        signals_enable();

        if (write(return_fd, &ret, sizeof(ret)) != sizeof(ret)) {
            perror("scheduler: write to shell");
            fprintf(stderr, "Scheduler: giving up on shell request processing.\n");
            break;
        }
    }
}

int main(int argc, char *argv[])
{
    int nproc;
    /* Two file descriptors for communication with the shell */
    static int request_fd, return_fd;

    tasks=0;
    super_id = 0;
    nproc = 1; /* number of proccesses goes here */
    current_proc = ( queue * ) malloc( sizeof(queue) );
    if ( !current_proc )
    {
        perror( "main: init, bad alloc");
        exit(1);
    }
    init_q(current_proc);
    /* Create the shell. */
    sched_create_shell(SHELL_EXECUTABLE_NAME, &request_fd, &return_fd);
    /* TODO: add the shell to the scheduler's tasks */

    /*
     * For each of argv[1] to argv[argc - 1],
     * create a new child process, add it to the process list.
     */


    /* Wait for all children to raise SIGSTOP before exec()ing. */
    wait_for_ready_children(nproc);

    ///* Install SIGALRM and SIGCHLD handlers. */
    install_signal_handlers();

    //if (nproc == 0) {
    //    fprintf(stderr, "Scheduler: No tasks. Exiting...\n");
    //    exit(1);
    //}
    kill(current_proc->pid,SIGCONT);
    alarm( SCHED_TQ_SEC );

    shell_request_loop(request_fd, return_fd);

    /* Now that the shell is gone, just loop forever
     * until we exit from inside a signal handler.
     */
    while (pause())
        ;

    /* Unreachable */
    fprintf(stderr, "Internal error: Reached unreachable point\n");
    return 1;
}
