//
// Created by ben on 03/01/17.
//

#include "../h/host.h"
#include "../h/syscall.h"

void read_elf_binary(const char *path, void** start, uint32_t *length){
    int64_t ilen;
    int fd;
    fd = host_open(path, O_RDONLY, 0);
    if (fd == -1)   {
        printf("Could not open file: %s\n" , path);
        host_exit();
    }
    ilen = host_lseek(fd, 0, SEEK_END); // find the size.
    if (ilen == -1){
        printf("lseek failed\n");
        host_exit();
    }
    else
        *length = (uint64_t) ilen;

    host_lseek(fd, 0, SEEK_SET); // go to the beginning
    *start = (void *)syscall_mmap(0,
                  *length,
                  PROT_READ|PROT_WRITE,
                  MAP_PRIVATE,
                  fd,
                  0);
    if (*start == (void*) -1){
        printf("could not map region\n");
        host_exit();
    }
    printf("Mapped %d bytes!\n", *length);
}