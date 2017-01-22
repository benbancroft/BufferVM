#include <stdio.h>
#include <malloc.h>
#include <string.h>

#include "buffervm.h"

int main(int argc, char *argv[], char *envp[]){
    printf("hello world from glibc!\n");

    printf("\nargv:");
    while(argc-- > 0)
        printf(" %s",*argv++);
    printf("\n");

    printf("envp:");
    while(*envp)
        printf(" %s",*envp++);
    printf("\n\n");

    char *hello = "this is a string literal\n";
    size_t size = strlen(hello)+1;

    char *mem = malloc(size);
    mem = set_version((void *)mem, size, 5);

    memcpy(mem, hello, size);

    printf("here is some memory: %p %s\n", mem, mem);


    return 0;
}
