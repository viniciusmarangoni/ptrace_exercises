#include <stdio.h>        // printf
#include <sys/ptrace.h>   // ptrace
#include <sys/types.h>    // pid_t
#include <sys/reg.h>      // ORIG_EAX
#include <sys/wait.h>     // wait
#include <unistd.h>       // fork
#include <sys/syscall.h> // SYS_execve

int main(){
    pid_t child_pid;

    child_pid = fork();

    if(child_pid == 0){
        ptrace(PTRACE_TRACEME, 0, NULL, NULL); // Last three paramaters are ignored
        sleep(2);
        execl("/bin/ls", "ls", NULL);
    }
    else{
        pid_t child_pid_from_wait;
        int status;
        long syscall_number;

        long params[3];
        int insyscall = 0;

        while(1){
            child_pid_from_wait = wait(&status);

            if(WIFEXITED(status)){
                break;
            }
            else if(WIFSTOPPED(status)){
                if(WSTOPSIG(status) == SIGTRAP){
                    syscall_number = ptrace(PTRACE_PEEKUSER, child_pid, sizeof(long int) * ORIG_EAX, NULL); // The last parameter is ignored


                    /*
                       The interesting things starts here. We want to track what is happening BEFORE the syscall is executed and AFTER the syscall is executed
                       To control this, we set the inssyscall variable to 0 in its declaration. When the first syscall with the "SYS_write" identifier is called, we
                       set this variable to 1. This means that the syscall will be executed and when we catch another SIGTRAP with the same syscall, it represents 
                       the "return" of the syscall.
                    */
                    if(syscall_number == SYS_write){
                        if(insyscall == 0){
                            insyscall = 1;
                            
                            printf("-----[Entering SYS_write...]-----\n");
                        
                            params[0] = ptrace(PTRACE_PEEKUSER, child_pid, sizeof(long int) * EBX, NULL);
                            params[1] = ptrace(PTRACE_PEEKUSER, child_pid, sizeof(long int) * ECX, NULL);
                            params[2] = ptrace(PTRACE_PEEKUSER, child_pid, sizeof(long int) * EDX, NULL);
                            
                            printf("Write called with %ld, %ld, %ld\n\n", params[0], params[1], params[2]);
                        }
                        else{
                            int ret;
                            insyscall = 0;

                            printf("-----[Returning from SYS_write...]-----\n");

                            /*
                               The most interesting thing here is that in line 35 we get the value of ORIG_EAX. The EAX register stores the syscall number.
                               But let's remember that, after the syscall is executed, its return value will be stored in EAX too, overriding the syscall number.
                               So, when we want to get the syscall number, we get ORIG_EAX. We the syscall is "returning", to get its return value we have to get
                               the index of EAX instead of ORIG_EAX

                               References: 
                               https://stackoverflow.com/questions/6468896/why-is-orig-eax-provided-in-addition-to-eax
                               
                            */
                            ret = ptrace(PTRACE_PEEKUSER, child_pid, sizeof(long int) * EAX, NULL);

                            printf("Write returned %ld\n", ret);
                        }
                    }

                    /*
                       PTRACE_SYSCALL will let the child execute until the next entry or exit of a syscall
                    */
                    ptrace(PTRACE_SYSCALL, child_pid, NULL, NULL); // If "data" parameter is nonzero, this signal number is delivered to the tracee. If it's zero, no signal is delivered. The "addr" param is ignored.
                }
            }
        }
    }

    return 0;
}
