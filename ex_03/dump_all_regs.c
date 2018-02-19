#include <stdio.h>        // printf
#include <sys/ptrace.h>   // ptrace
#include <sys/types.h>    // pid_t
#include <sys/reg.h>      // ORIG_EAX
#include <sys/wait.h>     // wait
#include <sys/user.h>   // struct user_regs_struct
#include <unistd.h>       // fork
#include <sys/syscall.h> // SYS_execve

int main(){
    pid_t child_pid;

    child_pid = fork();

    if(child_pid == 0){
        ptrace(PTRACE_TRACEME, 0, NULL, NULL);
        sleep(2);
        execl("/bin/ls", "ls", NULL);
    }
    else{
        pid_t child_pid_from_wait;
        int status;
        long syscall_number;
        struct user_regs_struct regs;
        int insyscall = 0;

        while(1){
            child_pid_from_wait = wait(&status);

            if(WIFEXITED(status)){
                break;
            }
            else if(WIFSTOPPED(status)){
                if(WSTOPSIG(status) == SIGTRAP){
                    syscall_number = ptrace(PTRACE_PEEKUSER, child_pid, sizeof(long int) * ORIG_EAX, NULL);


                    if(syscall_number == SYS_write){
                        if(insyscall == 0){
                            insyscall = 1;
                            
                            printf("-----[Entering SYS_write...]-----\n");
                            /*
                               PTRACE_GETREGS is not available to all architectures. I think it doesn't exists in ARM64
                               All registers will be copied to regs variable
                            */
                            ptrace(PTRACE_GETREGS, child_pid, NULL, &regs); // "addr" parameter is ignored.

                            printf("ORIG_EAX: %ld\n", regs.orig_eax);
                            printf("EAX: %ld\n", regs.eax);
                            printf("EBX: %ld\n", regs.ebx);
                            printf("ECX: %ld\n", regs.ecx);
                            printf("EDX: %ld\n", regs.edx);
                            printf("ESP: %p\n", regs.esp);
                            printf("EBP: %p\n", regs.ebp);
                        }
                        else{
                            int ret;
                            insyscall = 0;

                            printf("-----[Returning from SYS_write...]-----\n");

                            ret = ptrace(PTRACE_PEEKUSER, child_pid, sizeof(long int) * EAX, NULL);

                            printf("Write returned %ld\n", ret);
                        }
                    }

                    ptrace(PTRACE_SYSCALL, child_pid, NULL, NULL);
                }
            }
        }
    }

    return 0;
}
