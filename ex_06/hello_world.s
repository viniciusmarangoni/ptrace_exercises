.data                               # in this section remains the static data like global variables and static local variables
msg:                                # this defines the symbol msg as the name of the below string
    .ascii "Hello World\n"          # .ascii defnes a string. .asciiz defines a string terminated by null char
    .set len, . -msg                # this command defines a symbol named "len" that has the size of the string in bytes
    
.text
    .global _start

_start:
    # write a text in the screen
    mov $0x04, %eax                 # write syscall number
    mov $0x01, %ebx                 # specify the file descriptor (1 is stdout)
    mov $msg, %ecx                  # specify the pointer to the message
    mov $len, %edx                  # specify the number of bytes to write
    int $0x80

    # now let's exit
    mov $0x01, %eax                 # exit syscall number
    xor %ebx, %ebx                  # this zeroes the ebx register
    int $0x80
