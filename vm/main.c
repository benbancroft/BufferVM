#include <zconf.h>
#include "memory.h"
#include "elf.h"
#include "vm.h"


int main(int argc, char **argv, char **envp) {
    if (argc < 3) {
        fprintf(stderr, "%s kernel.elf binary\n", argv[0]);
        return 1;
    }


    int kernel_binary_fd, prog_binary_fd;
    if (!(kernel_binary_fd = read_binary(argv[1]))) {
        return 1;
    }
    if (!(prog_binary_fd = read_binary(argv[2]))) {
        return 1;
    }

    struct vm_t vm;
    struct vcpu_t vcpu;

    vm_init(&vm, 0xFFF00000);
    vcpu_init(&vm, &vcpu);

    run(&vm, &vcpu, kernel_binary_fd, prog_binary_fd);

    close(kernel_binary_fd);
    close(prog_binary_fd);

    return 0;
}