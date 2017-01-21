#include <stdio.h>
#include <malloc.h>
#include <string.h>

int main(){
    printf("hello world from glibc!\n");

    char *hello = "this is a string literal\n";
    size_t size = strlen(hello)+1;

    char *mem = malloc(size);
    memcpy(mem, hello, size);

    printf("here is some memory: %p %s\n", mem, mem);


    return 0;
}
