#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "proc-common.h"
#include "tree.h"

#define SLEEP_PROC_SEC  10
#define SLEEP_TREE_SEC  3

/*
 * Create this process tree:
 * A-+-B---D
 *   `-C
 */
void fork_procs(struct tree_node *me)
{
    /*
     * initial process is A.
     */
    int i;
    pid_t pid;
    int status;

    change_pname(me->name);
    /* loop to fork for all my children */

    for (i=0;i<me->nr_children;i++)
    {
        pid = fork();
        if (pid < 0) {
            perror("fork_procs: fork");
            exit(1);
        }
        if (pid == 0) {
            /* Child */
            me=me->children+i;
            fork_procs(me);
            exit(1);
        }
    }
    if (me->nr_children==0)
    {
        printf("%s: Sleeping...\n",me->name);
        sleep(SLEEP_PROC_SEC);
    }

    /* ... */
    for(i=0;i<me->nr_children;i++)
    {
		pid = waitpid(-1, &status, WUNTRACED);
		explain_wait_status(pid, status);
    }

    printf("%s: Exiting...\n",me->name);
    exit(0);
}

/*
 * The initial process forks the root of the process tree,
 * waits for the process tree to be completely created,
 * then takes a photo of it using show_pstree().
 *
 * How to wait for the process tree to be ready?
 * In ask2-{fork, tree}:
 *      wait for a few seconds, hope for the best.
 * In ask2-signals:
 *      use wait_for_ready_children() to wait until
 *      the first process raises SIGSTOP.
 */
int main(int argc,char **argv)
{
    if(argc!=2)
    {
        printf("Usage:%s <input.tree> \n",argv[0]);
        exit(1);
    }
    pid_t pid=0;
    int status=0;
    struct tree_node * root = get_tree_from_file(argv[1]);

    /* Fork root of process tree */
    pid = fork();
    if (pid < 0) {
        perror("main: fork");
        exit(1);
    }
    if (pid == 0) {
        /* Child */
        fork_procs(root);
        exit(1);
    }

    /*
     * Father
     */

    /* for ask2-{fork, tree} */
    sleep(SLEEP_TREE_SEC);

    /* Print the process tree root at pid */
    show_pstree(pid);


    /* Wait for the root of the process tree to terminate */
    pid = wait(&status);
    explain_wait_status(pid, status);

    return 0;
}
