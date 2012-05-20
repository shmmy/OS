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
void fork_procs(struct tree_node *me,int ffd)
{
    /*
     * initial process is A.
     */
    int i;
    int status;
    pid_t pid;
    pid_t children_pids[2];
    int pipes[4];
    int answers[2];
    int result;
    int iocheck;
    change_pname(me->name);
    /* loop to fork for all my children */
    if(me->nr_children>0)
    {
        //i am a father
        if(pipe(pipes) == -1)
        {
            perror("pipe");
            exit(EXIT_FAILURE);
        }
        if(pipe(pipes+2) == -1)
        {
            perror("pipe");
            exit(EXIT_FAILURE);
        }
    }


    //{{{ Fork all children recursively
    for (i=0;i<me->nr_children;i++)
    {
        pid = fork();
        if (pid < 0)
        {
            perror("fork_procs: fork");
            exit(1);
        }
        else if (pid == 0)
        {
            /* Child */
            close(pipes[2*i]); //child closes read
            me = me->children+i;
            fork_procs(me,pipes[2*i+1]);
            exit(1);
        }
        else
        {
            close(pipes[2*i+1]); //father closes write
            children_pids[i]=pid;
        }
    }
    //}}}

    //{{{ Read from both children descriptors
    for (i=0;i<(me->nr_children);i++)
    {
        pid = *(children_pids+i);
        //here is a bug \./
        //it's squashed now _._
        iocheck = read(pipes[2*i],answers+i,sizeof(int));
        if(iocheck==-1)
        {
            perror("read error");
            exit(1);
        }
        printf("%s: \t I read %d\n",me->name,answers[i]);
    }
    //}}}

    // Waiting
    for (i=0;i<(me->nr_children);i++)
    {
        pid = wait(&status);
        explain_wait_status(pid,status);
    }

    // Generating result
    switch(*(me->name))
    {
        case '+':
            result=answers[0]+answers[1];
            break;
        case '*':
            result=answers[0]*answers[1];
            break;
        default:
            sscanf(me->name,"%d",&result);
            break;
    }
    printf("%s said %d\n",me->name,result);
    iocheck = write(ffd,&result,sizeof(int));
    if (iocheck ==-1)
    {
        perror("write error");
        exit(1);
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
    pid_t pid;
    int status;
    int initpipe[2];
    int final_answer;
    int iocheck;
    if(pipe(initpipe) == -1)
    {
        perror("main:pipe");
        exit(EXIT_FAILURE);
    }

    struct tree_node * root = get_tree_from_file(argv[1]);
    printf("\n Expression tree to calculate: \n");
    printf("= = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = =\n");
    print_tree(root);
    printf("= = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = =\n\n");

    /* Fork root of process tree */
    pid = fork();
    if (pid < 0)
    {
        perror("main: fork");
        exit(1);
    }
    if (pid == 0)
    {
        /* Child */
        close(initpipe[0]);
        fork_procs(root,initpipe[1]);
        exit(1);
    }

    /*
     * Father
     */
    close(initpipe[1]);
    iocheck = read(initpipe[0],&final_answer,sizeof(int));
    if(iocheck==-1)
    {
        perror("main:read");
        exit(EXIT_FAILURE);
    }

    waitpid(pid,&status,WUNTRACED);
    explain_wait_status(pid,status);

    printf("\033[1;31mThe answer is %d\033[0m\n",final_answer);
    return 0;
}
