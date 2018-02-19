#include <sys/ptrace.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/reg.h>
#include <sys/wait.h>
#include <sys/syscall.h>
#include <stdlib.h>
#include <string.h>

void get_data(pid_t child_pid, long r_buff_addr, char *l_string, long r_buff_size);
void put_data(pid_t child_pid, long r_buff_addr, char *l_string, long r_buff_size);
void reverse_str(char *str, int length);

int main(){
    int status;
    int insyscall = 0;
    long syscall_number;
    pid_t child_pid;

    child_pid = fork();

    if(child_pid == 0){
        // I'm the child
        ptrace(PTRACE_TRACEME, 0, 0, 0); // last 3 params ignored

        execl("./hello_world2", "hello_world2", NULL);
    }
    else{
        while(1){   
            wait(&status);

            if(WIFEXITED(status)){
                break;
            }
            
            if(WIFSTOPPED(status)){
                if(WSTOPSIG(status) == SIGTRAP){
                    printf("Received SIGTRAP from the child!\n");

                    syscall_number = ptrace(PTRACE_PEEKUSER, child_pid, sizeof(long int) * ORIG_EAX, 0);
                
                    if(syscall_number == SYS_write){
                        printf("Got sys_write\n");

                        if(insyscall == 0){
                            insyscall = 1;

                            // long r_fd = ptrace(PTRACE_PEEKUSER, child_pid, sizeof(long int) * EBX, 0); // We dont need this
                            long r_buff_addr = ptrace(PTRACE_PEEKUSER, child_pid, sizeof(long int) * ECX, 0);
                            long r_buff_size = ptrace(PTRACE_PEEKUSER, child_pid, sizeof(long int) * EDX, 0);

                            char *l_string = (char *) malloc((r_buff_size + 1) * sizeof(char));

                            if(l_string == NULL){
                                printf("Error allocating buffer!\n");
                                exit(1);
                            }

                            get_data(child_pid, r_buff_addr, l_string, r_buff_size);

                            printf("RETRIEVED STRING: %s\n", l_string);

                            printf("Below you can find the bytes retrieved:\n");
                            
                            for(int i = 0; i < r_buff_size; i++){
                                printf("\\x%02x", l_string[i]);
                            }
                            printf("\n");

                            printf("Reversing string...\n");

                            reverse_str(l_string, r_buff_size);
                            
                            printf("Result: %s\n", l_string);
                            printf("Putting data in remote process...\n");

                            put_data(child_pid, r_buff_addr, l_string, r_buff_size);
                            
                            free(l_string);
                        }
                        else{
                            insyscall = 0;
                            printf("Just returning from write syscall...\n");
                        }
                    }
                }
            }
            
            ptrace(PTRACE_SYSCALL, child_pid, 0, 0);            
        }
    }
}

// This function will retrieve the data from a remote buffer and store it in our local buffer
void get_data(pid_t child_pid, long r_buff_addr, char *l_string, long r_buff_size){
    char *aux_laddr;
    int i, j;

    /*
       This is an interesting trick to retrieve the data.
       In an union, all variables "points" to the same memory area/
       So, if you put a value at "val", you can access this value as a long accessing "val" variable
       or access this value as a char array, accessing "chars" variable.
       
       Example:
       data.val = 0x41414141
       
       printf("%ld\n", data.val); // Will print 1094795585 (0x41414141 in decimal)
       printf("%c\n", data.chars[0]); // Will print "A", since we are accessing the first byte of 0x41414141

       The trick in this case is that ptrace only allows retrieve of data by word size.
       ptrace will always return a "long", so we have to understand this "long" value as 4 bytes of an char array.
    */
    union u{
        long val;
        char chars[sizeof(long)];
    } data;

    // We will perform some pointer arithmetic to facilitate the copy of data to our destination array
    aux_laddr = l_string;

    i = 0;
    
    /*
       First we will get the amount of data that is multiple of sizeof(long)
       Imagine some scenarios:
           - We want to get an array of 4 bytes (long)
           - We want to get an array of 6 bytes (long + 2 bytes)
           - We want to get an array of 2 bytes (long - 2 bytes)
    */
    j = r_buff_size / (sizeof(long));

    while(i < j){
        /*
           The following instruction will return a long (that we understand as 4 chars of the array).
           It will get this 4 bytes from the beggining of the address pointed by the third argument.
           Note that we are retrieving the array 4 by 4 bytes. This calculation is represented 
           by "r_buff_addr + (i * sizeof(long)".
        */
        data.val = ptrace(PTRACE_PEEKDATA, child_pid, r_buff_addr + (i * sizeof(long)), 0);

        // Remember that data.chars points to the same memory area of data.value,
        // what means that they have the same value
        memcpy(aux_laddr, data.chars, sizeof(long));

        // Pointer arithmetic
        aux_laddr += sizeof(long);
        
        i++;
    }

    j = r_buff_size % sizeof(long);

    if(j != 0){
        /* 
           r_buff_size is not multiple of sizeof(long). Remains some bytes to copy
           if not entered in "while", i is 0. If entered, i will point to the last piece of data,
           since the last increment was what caused the "break" in while

           We will retrieve sizeof(long) amount of data, but will copy only j bytes to our array
        */
        data.val = ptrace(PTRACE_PEEKDATA, child_pid, r_buff_addr + (i * sizeof(long)), 0);

        memcpy(aux_laddr, data.chars, j);
    }

    l_string[r_buff_size] = '\0';
}

// This function is very similar to get_data. All interesting tricks are there.
// Read this one just to learn the syntax of PTRACE_POKEDATA
void put_data(pid_t child_pid, long r_buff_addr, char *l_string, long r_buff_size){
    char *aux_laddr;
    int i, j;

    union u{
        long val;
        char chars[sizeof(long)];
    } data;

    aux_laddr = l_string;

    i = 0;
    j = r_buff_size / (sizeof(long));

    while(i < j){
        // We have to put the data in remote address 4 by 4 bytes
        memcpy(data.chars, aux_laddr, sizeof(long));

        // Now the last argument of ptrace is being used
        ptrace(PTRACE_POKEDATA, child_pid, r_buff_addr + (i * sizeof(long)), data.val);

        aux_laddr += sizeof(long);
        
        i++;
    }

    j = r_buff_size % sizeof(long);

    if(j != 0){
        memcpy(data.chars, aux_laddr, j);
        ptrace(PTRACE_PEEKDATA, child_pid, r_buff_addr + (i * sizeof(long)), data.val);
    }
}

// Iterates until the half of the string, swapping the left char with the right
void reverse_str(char *str, int length){
    int half = length/2;
    length--;
    char aux;

    for(int i = 0; i < half; i++){
        aux = str[length-i];
        str[length-i] = str[i];
        str[i] = aux;
    }
}
