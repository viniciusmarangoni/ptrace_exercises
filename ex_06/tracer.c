#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ptrace.h>
#include <sys/wait.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <sys/reg.h>

int main(){
    int status;
    long instruction_pointer;
    long instruction_bytecode;
    pid_t child_pid;

    child_pid = fork();

    if(child_pid == 0){
        // I'm the child
        ptrace(PTRACE_TRACEME, 0, 0, 0); // Last three params are ignored
        execl("./hello_world", "hello_world", NULL);
    }
    else{
        while(1){
            wait(&status);

            if(WIFEXITED(status)){
                break;
            }
            
            if(WIFSTOPPED(status) && WSTOPSIG(status) == SIGTRAP){
                instruction_pointer = ptrace(PTRACE_PEEKUSER, child_pid, sizeof(long int) * EIP, 0);
                instruction_bytecode = ptrace(PTRACE_PEEKTEXT, child_pid, instruction_pointer, 0);

                printf("EIP: 0x%08lx\n", instruction_pointer);
                printf("Bytecode: %08lx\n\n", instruction_bytecode);
            }

            ptrace(PTRACE_SINGLESTEP, child_pid, 0, 0);
        }
    }

    return 0;
}
