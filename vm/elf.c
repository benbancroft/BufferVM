//
// Created by ben on 09/10/16.
//

#include <stdio.h>
#include <err.h>
#include <fcntl.h>
#include <memory.h>
#include <sysexits.h>
#include <sys/mman.h>
#include <inttypes.h>

#include "elf.h"

/*void *resolve(const char* sym)
{
    static void *handle = NULL;
    if (handle == NULL) {
        handle = dlopen("libc.so", RTLD_NOW);
    }
    return dlsym(handle, sym);
}*/

void relocate(Elf32_Shdr *shdr, const Elf32_Sym *syms, const char *strings, const char *src, char *dst) {
    Elf32_Rel *rel = (Elf32_Rel *) (src + shdr->sh_offset);
    size_t j;
    for (j = 0; j < shdr->sh_size / sizeof(Elf32_Rel); j += 1) {
        //const char *sym = strings + syms[ELF32_R_SYM(rel[j].r_info)].st_name;
        switch (ELF32_R_TYPE(rel[j].r_info)) {
            case R_386_JMP_SLOT:
            case R_386_GLOB_DAT:
                //TODO
                //*(Elf32_Word*)(dst + rel[j].r_offset) = (Elf32_Word)resolve(sym);
                break;
        }
    }
}

void *find_sym(const char *name, Elf32_Shdr *shdr, const char *strings, const char *src, char *dst) {
    Elf32_Sym *syms = (Elf32_Sym *) (src + shdr->sh_offset);
    size_t i;
    for (i = 0; i < shdr->sh_size / sizeof(Elf32_Sym); i += 1) {
        if (strcmp(name, strings + syms[i].st_name) == 0) {
            return dst + syms[i].st_value;
        }
    }
    return NULL;
}

elf_info_t image_load(int fd, bool user, struct vm_t *vm) {
    Elf *e;
    size_t n;
    GElf_Ehdr ehdr;

    elf_info_t elf_info = { NULL, 0, 0 };

    if (elf_version(EV_CURRENT) == EV_NONE)
        errx(EX_SOFTWARE, "ELF library initialization failed: %s", elf_errmsg(-1));

    if ((e = elf_begin(fd, ELF_C_READ, NULL)) == NULL)
        errx(EX_SOFTWARE, "elf_begin () failed: %s.", elf_errmsg(-1));

    if (elf_kind(e) != ELF_K_ELF)
        errx(EX_DATAERR, "Not an ELF object.");

    if (gelf_getehdr(e, &ehdr) == NULL)
        errx(EX_SOFTWARE, "getehdr () failed: %s.", elf_errmsg(-1));

    elf_info.entry_addr = (void *) ehdr.e_entry;

    if (elf_getphdrnum(e, &n) != 0)
        errx(EX_DATAERR, "elf_getphdrnum () failed: %s.", elf_errmsg(-1));

    GElf_Phdr phdr;
    uint64_t start;
    uint64_t taddr;
    uint64_t min_addr = UINT64_MAX;
    uint64_t max_addr = 0;
    uint64_t section_max_addr;

    for (size_t i = 0; i < n; i++) {

        if (gelf_getphdr(e, i, &phdr) != &phdr)
            errx(EX_SOFTWARE, "getphdr () failed: %s.", elf_errmsg(-1));

        if (phdr.p_type != PT_LOAD) {
            continue;
        }
        if (phdr.p_filesz > phdr.p_memsz) {
            fprintf(stderr, "image_load:: p_filesz > p_memsz\n");
            //TODO
            //munmap(exec, size);
            return elf_info;
        }
        if (!phdr.p_filesz) {
            continue;
        }

        // p_filesz can be smaller than p_memsz,
        // the difference is zeroe'd out.
        start = (uint64_t) elf_rawfile(e, NULL) + phdr.p_offset;
        taddr = phdr.p_vaddr;

        uint64_t flags = 0;

        //Writeable.
        if (phdr.p_flags & PF_W)
            flags |= PDE64_WRITEABLE;

        // Executable.
        if (!(phdr.p_flags & PF_X))
            flags |= PDE64_NO_EXE;

        load_address_space(taddr, phdr.p_memsz, (char *) start, phdr.p_filesz, flags | user ? PDE64_USER : 0, vm->mem);

        section_max_addr = taddr + phdr.p_memsz;

        if (taddr < min_addr)
            min_addr = taddr;

        if (section_max_addr > max_addr)
            max_addr = section_max_addr;

        printf("Loaded header at %p of size: %" PRIu64 "\n", (void *) taddr, (uint64_t) phdr.p_memsz);
    }

    // Align to page size
    elf_info.min_page_addr = (uint64_t) P2ALIGN(min_addr, PAGE_SIZE);
    elf_info.max_page_addr = (uint64_t) P2ROUNDUP(max_addr, PAGE_SIZE);

    /*GElf_Shdr shdr;
    size_t shstrndx, sz;

    if (elf_getshdrstrndx(e, &shstrndx) != 0)
        errx(EX_SOFTWARE, "elf_getshdrstrndx () failed: %s.", elf_errmsg(-1));

    Elf_Scn *scn = NULL;
    Elf_Data *data = NULL;
    char *name;

    while ((scn = elf_nextscn(e, scn)) != NULL) {
        if (gelf_getshdr(scn , &shdr) != &shdr)
            errx(EX_SOFTWARE , "getshdr () failed: %s.", elf_errmsg ( -1));

        if (shdr.sh_type == SHT_DYNSYM) {
            //syms = (Elf32_Sym*)(elf_start + shdr[i].sh_offset);

            if ((data = elf_getdata(scn, data)) != NULL)
            {
                printf("Test: %i\n", data->d_off);
            }else{
                printf("fail\n");
            }


            //strings = elf_start + shdr[shdr[i].sh_link].sh_offset;
            //entry = find_sym("_start", shdr + i, strings, elf_start, exec);
            //printf("Found entry: %p", entry);
            //break;
        }else{
            printf("fail: %i\n", shdr.sh_type);
        }

        if ((name = elf_strptr(e, shstrndx , shdr.sh_name )) == NULL)
            errx(EX_SOFTWARE , "elf_strptr () failed: %s.", elf_errmsg ( -1));

        printf("Section %-4.4jd %s\n", (uintmax_t) elf_ndxscn(scn), name);
    }*/

    /*shdr = (Elf32_Shdr *)(elf_start + hdr->e_shoff);

    for(i=0; i < hdr->e_shnum; ++i) {
        if (shdr[i].sh_type == SHT_DYNSYM) {
            syms = (Elf32_Sym*)(elf_start + shdr[i].sh_offset);
            strings = elf_start + shdr[shdr[i].sh_link].sh_offset;
            entry = find_sym("_start", shdr + i, strings, elf_start, exec);
		printf("Found entry: %p", entry);
            break;
        }
    }*/

    //TODO - dynamic stoof
    /*for(i=0; i < hdr->e_shnum; ++i) {
        if (shdr[i].sh_type == SHT_REL) {
            relocate(shdr + i, syms, strings, elf_start, exec);
        }
    }*/

    elf_end(e);

    return elf_info;

}

/* image_load */

int read_binary(char *name) {
    int file;

    //Open file
    file = open(name, O_RDONLY, 0);
    if (file < 0) {
        fprintf(stderr, "Unable to open file %s\n", name);
        return 0;
    }

    return file;
}
