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
    if (!(binary_fd = read_binary(argv[1], &code))) {
        return 1;
    }

    struct vm_t vm;
    struct vcpu_t vcpu;

    vm_init(&vm, 0xFFF00000);
    vcpu_init(&vm, &vcpu);

    //printf("Main addr: %p\n", get_physaddr(0x804881e, &vm));

    run(&vm, &vcpu, binary_fd);
    //run_long_mode(&vm, &vcpu);
    //run_long_mode(&vm, &vcpu);

    //run_vm(&vm, ptr);

    close(binary_fd);

    return 0;
}