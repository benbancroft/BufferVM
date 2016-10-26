//
// Created by ben on 09/10/16.
//

#ifndef BUFFERVM_ELF_H
#define BUFFERVM_ELF_H

#include <gelf.h>

#include "vm.h"
#include "../common/paging.h"

void relocate(Elf32_Shdr *shdr, const Elf32_Sym *syms, const char *strings, const char *src, char *dst);

void *find_sym(const char *name, Elf32_Shdr *shdr, const char *strings, const char *src, char *dst);

void *image_load(int fd, bool user, struct vm_t *vm);

/* image_load */
int read_binary(char *name);

#endif //BUFFERVM_ELF_H
