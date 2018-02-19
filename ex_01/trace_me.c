#include <stdio.h>		// printf
#include <sys/ptrace.h> // ptrace
#include <sys/types.h>  // pid_t
#include <sys/wait.h>   // wait
#include <sys/reg.h>    // ORIG_EAX
#include <unistd.h>     // execl
#include <string.h>     // strsignal

/*
  long ptrace(enum __ptrace_request request, pid_t pid,
              void *addr, void *data);
*/

int main(){
    pid_t child_pid;
    long orig_eax;

    child_pid = fork();  // Will fork the program. If I'm the child, child_pid will be 0. Otherwise child_pid will be the pid of the child

    if(child_pid == 0){
        /// I'm the child!


        /*
             PTRACE_TRACEME indicates that I will be traced by my father.
             The last three arguments are ignored
        */
        ptrace(PTRACE_TRACEME, 0, NULL, NULL);

        // Sleep for 2 seconds before execute exec
        sleep(3);

        /*
           This will execute the exec system call. Let's remember an explanation about the impact ov execve:
           If the PTRACE_O_TRACEEXEC option is not in effect, all successful calls to execve(2) by the traced process will cause it to be sent a SIGTRAP signal, giving the parent a chance to gain control before the new program begins execution

           Every time a signal is sent, the tracer will be able to control the tracee.
           Since execl will raise a SIGTRAP, our tracer will start manipulating the tracee // read about this SIGTRAP in "man execve"
        */
        execl("/bin/ls", "ls", NULL);
    }
    else{
        pid_t child_pid_from_wait;
        int status;
        
        child_pid_from_wait = wait(&status); // wait() will wait for signals of a child. It returns the child_pid and put the status in the "status" variable

        if(WIFSTOPPED(status)){ // returns true if the child process was stopped by delivery of a signal; this is possible only if the call was done using WUNTRACED or when the child is being traced (see ptrace(2)).
            int signal_number;
            
            signal_number = WSTOPSIG(status);
            printf("I received the signal [%s] number [%d] from the pid %d\n", strsignal(signal_number), signal_number, child_pid_from_wait);
        }

        /*
           The following line required me some research to understand.
           PTRACE_PEEKUSER will retrieve data from USER area.
           sizeof(long int) * ORIG_EAX     This is being performed to calculate the index of eax register inside de struct user
           See notes.txt to understand better
           The last parameter is ignored
           ptrace function will return the value of the register

           In order to understand better, you have to read the manual of ptrace AND THE NOTES
        */
        orig_eax = ptrace(PTRACE_PEEKUSER, child_pid, sizeof(long int) * ORIG_EAX, NULL);

        printf("The child made a syscall %ld\n", orig_eax);
        ptrace(PTRACE_CONT, child_pid, NULL, NULL);
    }
    
    return 0;
}
