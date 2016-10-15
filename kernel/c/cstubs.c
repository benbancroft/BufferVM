//
// Created by ben on 14/10/16.
//

#include "../../libc/stdlib.h"
#include "../h/host.h"

void *realloc(void * ptr, size_t size){

}

void* malloc(size_t nbytes){

}

void free(void *ap){

}

int write(int file, const void *buffer, size_t count){
    host_write(file, buffer, count);
}

size_t read(int fd, void *buffer, size_t length){
    //host_read(file, buffer, count);
}