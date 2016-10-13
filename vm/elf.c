//
// Created by ben on 09/10/16.
//

#include <stdio.h>
#include <err.h>
#include <fcntl.h>
#include <memory.h>
#include <sysexits.h>
#include <sys/mman.h>

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
    int j;
    for (j = 0; j < shdr->sh_size / sizeof(Elf32_Rel); j += 1) {
        const char *sym = strings + syms[ELF32_R_SYM(rel[j].r_info)].st_name;
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
    int i;
    for (i = 0; i < shdr->sh_size / sizeof(Elf32_Sym); i += 1) {
        if (strcmp(name, strings + syms[i].st_name) == 0) {
            return dst + syms[i].st_value;
        }
    }
    return NULL;
}

void *image_load(int fd, struct vm_t *vm) {
    Elf *e;
    char *id, bytes[5];
    size_t n;
    GElf_Ehdr ehdr;

    void *entry = NULL;

    if (elf_version(EV_CURRENT) == EV_NONE)
        errx(EX_SOFTWARE, "ELF library initialization failed: %s", elf_errmsg(-1));

    if ((e = elf_begin(fd, ELF_C_READ, NULL)) == NULL)
        errx(EX_SOFTWARE, "elf_begin () failed: %s.", elf_errmsg(-1));

    if (elf_kind(e) != ELF_K_ELF)
        errx(EX_DATAERR, "Not an ELF object.");

    if (gelf_getehdr(e, &ehdr) == NULL)
        errx(EX_SOFTWARE, "getehdr () failed: %s.", elf_errmsg(-1));

    entry = ehdr.e_entry;

    if (elf_getphdrnum(e, &n) != 0)
        errx(EX_DATAERR, "elf_getphdrnum () failed: %s.", elf_errmsg(-1));

    GElf_Phdr phdr;
    uint64_t start;
    uint64_t taddr;

    for (int i = 0; i < n; i++) {

        if (gelf_getphdr(e, i, &phdr) != &phdr)
            errx(EX_SOFTWARE, "getphdr () failed: %s.", elf_errmsg(-1));

        if (phdr.p_type != PT_LOAD) {
            continue;
        }
        if (phdr.p_filesz > phdr.p_memsz) {
            fprintf(stderr, "image_load:: p_filesz > p_memsz\n");
            //TODO
            //munmap(exec, size);
            return 0;
        }
        if (!phdr.p_filesz) {
            continue;
        }

        // p_filesz can be smaller than p_memsz,
        // the difference is zeroe'd out.
        start = elf_rawfile(e, NULL) + phdr.p_offset;
        taddr = phdr.p_vaddr;

        //char *segment = mmap(taddr, phdr[i].p_memsz, PROT_READ | PROT_WRITE | PROT_EXEC, MAP_PRIVATE | MAP_ANONYMOUS, 0, 0);;

        int flags = PROT_READ | PROT_WRITE | PROT_EXEC;

        if (!(phdr.p_flags & PF_W))
            //Read only.
            flags = PDE64_RW;

        if (!(phdr.p_flags & PF_X))
            // Executable.
            flags = PDE64_NO_EXE;

        load_address_space(taddr, phdr.p_memsz, start, phdr.p_filesz, flags, vm);

        printf("Loaded header at %p of size: %d\n", taddr, phdr.p_memsz);
    }

    GElf_Shdr shdr;
    size_t shstrndx, sz;

    if (elf_getshdrstrndx(e, &shstrndx) != 0)
        errx(EX_SOFTWARE, "elf_getshdrstrndx () failed: %s.", elf_errmsg(-1));

    Elf_Scn *scn = NULL;
    Elf_Data *data = NULL;
    char *name;

    /*while ((scn = elf_nextscn(e, scn)) != NULL) {
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

    return entry;

}

/* image_load */

int read_binary(char *name, char **buffer) {
    FILE *file;

    unsigned long file_len;

    //Open file
    file = open(name, O_RDONLY, 0);
    if (file < 0) {
        fprintf(stderr, "Unable to open file %s\n", name);
        return 0;
    }

    return file;
}