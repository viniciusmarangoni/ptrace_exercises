#include <stdio.h>
#include <sys/types.h>
#include <sys/ptrace.h>
#include <sys/wait.h>
#include <sys/reg.h> // EIP
#include <string.h> // strsignal
#include <stdlib.h>
#include <string.h>

int main(int argc, char *argv[]){
    int status;
    long instruction_pointer;
    long instruction_bytecode;
    pid_t target_pid;
    
    if(argc != 2){
        printf("Usage: ./%s <pid_to_attach>\n", argv[0]);
        exit(1);
    }

    target_pid = atoi(argv[1]);
    ptrace(PTRACE_ATTACH, target_pid, 0, 0);

    wait(&status);

    // Note that when attaching in remote process we should expect to a SIGSTOP
    // When using PTRACE_TRACEME, the SIGSTOP is bein raised by execve
    if(WIFSTOPPED(status) && WSTOPSIG(status) == SIGSTOP){
        printf("Attached!\n");

        instruction_pointer = ptrace(PTRACE_PEEKUSER, target_pid, sizeof(long int) * EIP, 0);
        instruction_bytecode = ptrace(PTRACE_PEEKTEXT, target_pid, instruction_pointer, 0);

        printf("EIP: 0x%08x\n", instruction_pointer);
        printf("Bytecode: %08x\n\n", instruction_bytecode);
    }

    printf("Press enter to detach...\n");
    getchar();
    ptrace(PTRACE_DETACH, target_pid, 0, 0);

    return 0;
}
