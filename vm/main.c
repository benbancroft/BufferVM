#include <zconf.h>
#include "memory.h"
#include "elf.h"
#include "vm.h"


int main(int argc, char **argv, char **envp) {
    if (argc < 2) {
        fprintf(stderr, "%s binary.bin\n", argv[0]);
        return 1;
    }


    //int (*ptr)(int, char **, char**);
    char *code;
    int binary_fd;
    /*if (!(binary_fd = read_binary(argv[1], &code))) {
        return 1;
    }*/

    struct vm_t vm;
    struct vcpu_t vcpu;

    vm_init(&vm, 0x100000);
    vcpu_init(&vm, &vcpu);

    //char *entry_point = image_load(binary_fd, &vm);

    //close(binary_fd);

    //printf("Entry point: %p, physical addr: %p\n", entry_point, get_physaddr(entry_point, &vm));

    //printf("Main addr: %p\n", get_physaddr(0x804881e, &vm));

    run(&vm, &vcpu, NULL/*get_physaddr(entry_point, &vm)*/);
    //run_long_mode(&vm, &vcpu);
    //run_long_mode(&vm, &vcpu);

    //run_vm(&vm, ptr);

    return 0;
}