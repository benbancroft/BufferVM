//
// Created by ben on 09/10/16.
//

#ifndef BUFFERVM_ELF_H
#define BUFFERVM_ELF_H

#include <gelf.h>

#include "vm.h"
#include "../common/paging.h"

typedef struct elf_info {
    void *entry_addr;
    uint64_t min_page_addr;
    uint64_t max_page_addr;
} elf_info_t;


void relocate(Elf32_Shdr *shdr, const Elf32_Sym *syms, const char *strings, const char *src, char *dst);

void *find_sym(const char *name, Elf32_Shdr *shdr, const char *strings, const char *src, char *dst);

elf_info_t image_load(int fd, bool user, struct vm_t *vm);

/* image_load */
int read_binary(char *name);

#endif //BUFFERVM_ELF_H
