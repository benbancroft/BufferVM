//
// Created by ben on 14/10/16.
//

#include <stdio.h>
#include <stdlib.h>

#include "../h/host.h"

void *realloc(void * ptr, size_t size){

}

void* malloc(size_t nbytes){

}

void free(void *ap){

}

ssize_t write(int file, const void *buffer, size_t count){
    return host_write(file, buffer, count);
}

ssize_t read(int fd, void *buffer, size_t length){
    return host_read(fd, buffer, length);
}

int fprintf(FILE * stream, const char *format, ...){
    return (printf("fprintf unimplemented!\n"));
}

void abort(void){
    host_exit();
}

FILE *stderr = NULL;
