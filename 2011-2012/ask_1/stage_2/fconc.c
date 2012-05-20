/* -.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.

 * File Name : fconc.c

 * Last Modified : Thu 24 Nov 2011 04:45:11 PM EET

 * Created By : Greg Liras <gregliras@gmail.com>

 * Created By : Vasilis Gerakaris <vgerak@gmail.com>

 _._._._._._._._._._._._._._._._._._._._._.*/

#include "fconc.h"

int main(int argc, char ** argv)
{
    int OUT;
    int TMP;
    int i;
    const char * output;
    int duplicate = 0;
    int W_FLAGS = O_CREAT | O_WRONLY | O_TRUNC;
    int C_PERMS = S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH ;

    if (argc == 1)
    {
        perror("Use at least 1 file name when calling fconc\n");
        exit(EX_USAGE);
    }
    if (argc == 2)
    {
        //No need to chance anything
        exit(0);
    }
    else
    {
        output = argv[argc-1];
    }

    for (i=1; i<(argc-1); i++)
    {
        if (strcmp (argv[i], output) ==0 )
        {
            duplicate = 1;
            break;
        }
    }

    if (duplicate)
    {

        TMP = open("/tmp/fconc.out.tmp",W_FLAGS,C_PERMS);
        if (TMP < 0)
        {
            perror("Error opening tmp file, is another instance running?\n");
            exit(EX_TEMPFAIL);
        }

        lock_file(TMP,F_WRLCK);
        for(i=1; i <(argc-1); i++)
        {
            write_file(TMP,argv[i]);
        }
        unlock_file(TMP);
        if ( close(TMP) < 0)
        {
            perror("Could  not close tmp file");
            exit(EX_IOERR);
        }
        OUT = open(output,W_FLAGS,C_PERMS);
        if (OUT < 0)
        {
            perror("Error handling output file\n");
            exit(EX_IOERR);
        }

        lock_file(OUT,F_WRLCK);
        write_file(OUT,"/tmp/fconc.out.tmp");
        unlock_file(OUT);
        if ( close (OUT) < 0 )
        {
            perror("Could not close output file");
            exit(EX_IOERR);
        }
        if (unlink("/tmp/fconc.out.tmp") != 0)
        {
            perror("Error deleting temporary file, please remove /tmp/fconc.out.tmp\n");
            exit(EX__BASE);
        }
    }

    else
    {
        OUT = open(output,W_FLAGS,C_PERMS);
        if (OUT < 0)
        {
            perror("Error handling output file\n");
            exit(EX_IOERR);
        }
        lock_file(OUT,F_WRLCK);
        for (i=1;i<(argc-1);i++)
        {
            write_file(OUT,argv[i]);
        }
        unlock_file(OUT);
        if ( close(OUT) < 0 )
        {
            perror("Could not close out file");
            exit(EX_IOERR);
        }
    }
    exit(EXIT_SUCCESS);
}


void doWrite(int fd,const char *buff,int len)
{
    int written = 0;
    int current = 0;

    do
    {
        if ( (current = write(fd,buff+written,len-written)) < 0 )
        {
            perror("Error in writing\n");
            exit(EX_IOERR);
        }
        written+=current;
    } while(written < len );
}

void write_file(int fd,const char *infile)
{
    int A;
    char buffer[BUFFER_SIZE];
    int chars_read=0;
    struct flock lock;

    A = open(infile,O_RDONLY);
    if (A ==-1)
    {
        char error_message[BUFFER_SIZE];
        sprintf(error_message,"%s",infile);
        perror(error_message);
        exit(EX_NOINPUT);
    }
    fcntl(A,F_GETLK,lock);  //get lock info on A
    lock.l_type = F_RDLCK;  //set lock to read lock
    fcntl(A,F_SETLK,lock);  //set lock on A
    //time to read
    while( (chars_read = read(A,buffer,BUFFER_SIZE)) > 0)
    {
        //and write
        doWrite(fd,buffer,chars_read);
    }
    if ( chars_read == -1 )
    {
        perror("Read Error\n");
        exit(EX_IOERR);
    }
    lock.l_type = F_UNLCK;  //set lock to unlock
    fcntl(A,F_SETLK,lock);  //set lock on A
    //ok close
    if ( close(A) == - 1 )
    {
        perror("Close Error\n");
        exit(EX_IOERR);
    }
}

void lock_file(int fd,int lock_type)
{
    struct flock lock;
    lock.l_type = lock_type;  //set lock to lock_type
    lock.l_whence = SEEK_SET;
    lock.l_start = 0;
    lock.l_len = 0;
    if ( fcntl(fd,F_SETLKW,&lock) == -1)
    {
        if ( errno == EINTR )
        {
            perror("Interrupted before being able to set lock");
            exit(EX_IOERR);
        }
        else if ( errno == EDEADLK )
        {
            perror("DEADLOCK met");
            exit(EX_IOERR);
        }
        else
        {
            perror("Error while trying to set lock on file");
            exit(EX_IOERR);
        }
    }
}

void unlock_file(int fd)
{
    lock_file(fd,F_UNLCK); //set lock on file to UNLOCK
}
