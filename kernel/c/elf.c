//
// Created by ben on 03/01/17.
//

#include "../h/host.h"
#include "../h/syscall.h"
#include "../../common/elf.h"
#include "../../common/paging.h"
#include "../h/utils.h"

bool is_elf(int fd){
    unsigned char e_ident[EI_NIDENT];
    host_read(fd, (char *)&e_ident, sizeof (e_ident));
    host_lseek(fd, 0, SEEK_SET); // go to the beginning

    return memcmp(e_ident, "\177ELF", 4) == 0;
}

bool is_binary_64bit(int fd){
    elf64_hdr_t elf_hdr;
    host_read(fd, (char *)&elf_hdr, sizeof (elf64_hdr_t));
    host_lseek(fd, 0, SEEK_SET); // go to the beginning

    return elf_hdr.e_machine == EM_X86_64;
}

int pad_zero(uint64_t elf_bss)
{
    uint64_t nbyte;

    nbyte = elf_bss & ~PAGE_MASK;
    if (nbyte) {
        nbyte = PAGE_SIZE - nbyte;
        if (memset((void *)elf_bss, 0, nbyte))
            return -EFAULT;
    }
    return 0;
}

int load_elf64(int fd, elf_info_t *elf_info){
    elf64_hdr_t elf_hdr;

    uint64_t start_addr;
    uint64_t min_addr = UINT64_MAX;

    uint64_t section_max_maddr = 0;
    uint64_t section_max_faddr = 0;

    uint64_t max_faddr = 0;
    uint64_t max_maddr = 0;

    host_read(fd, (char *)&elf_hdr, sizeof (elf64_hdr_t));

    printf("Entry point user: %p\n", elf_hdr.e_entry);
    printf("Num program headers: %d\n", elf_hdr.e_phnum);

    elf_info->entry_addr = (void *) elf_hdr.e_entry;

    host_lseek(fd, elf_hdr.e_phoff, SEEK_SET);

    for(size_t i = 0; i < elf_hdr.e_phnum; i++) {
        elf64_phdr_t phdr;
        uint64_t end_addr;

        host_read(fd, (char *)&phdr, sizeof (elf64_phdr_t));

        if (phdr.p_type != PT_LOAD) {
            continue;
        }

        uint64_t flags = 0;

        //Readable.
        if (phdr.p_flags & PF_R)
            flags |= PROT_READ;

        //Writeable.
        if (phdr.p_flags & PF_W)
            flags |= PROT_WRITE;

        // Executable.
        if (phdr.p_flags & PF_X)
            flags |= PROT_EXEC;

        uint64_t size = phdr.p_filesz + PAGE_OFFSET(phdr.p_vaddr);
        uint64_t file_offset =  phdr.p_offset - PAGE_OFFSET(phdr.p_vaddr);

        start_addr = syscall_mmap(phdr.p_vaddr - PAGE_OFFSET(phdr.p_vaddr),
                                  size, flags | MAP_FIXED,
                                  MAP_PRIVATE,
                                  fd,
                                  file_offset);
        //Will handle error codes too - debugging point
        ASSERT(start_addr == phdr.p_vaddr - PAGE_OFFSET(phdr.p_vaddr));

        section_max_maddr = phdr.p_vaddr + phdr.p_memsz;
        section_max_faddr = phdr.p_vaddr + phdr.p_filesz;

        if (start_addr < min_addr)
            min_addr = start_addr;

        //mem
        if (section_max_maddr > max_maddr)
            max_maddr = section_max_maddr;

        //file
        if (section_max_faddr > max_faddr)
            max_faddr = section_max_faddr;

        printf("Loaded header at %p of size: %d file offset %d\n", (void *)phdr.p_vaddr, phdr.p_memsz, phdr.p_filesz);
    }

    if (!pad_zero(max_faddr)) {
        printf("Zeroed end of bss\n");
    }

    // Align to page size
    elf_info->min_page_addr = (uint64_t) P2ALIGN(min_addr, PAGE_SIZE);
    elf_info->max_page_addr = (uint64_t) P2ROUNDUP(max_maddr, PAGE_SIZE);

    return 0;
}

int load_elf_binary(int fd, elf_info_t *elf_info, char *mem_offset){
    if (is_elf(fd)){
        if (is_binary_64bit(fd)){
            printf("64-bit bin\n");
            return load_elf64(fd, elf_info);
        }else{
            printf("Un-supported binary? 32-bit not currently supported?\n");

            return 1;
        }
    } else{
        printf("Not a valid elf file\n");

        return 1;
    }
}

int read_binary(char *name) {
    int file;

    //Open file
    file = host_open(name, O_RDONLY, 0);
    if (file < 0) {
        printf("Unable to open file %s\n", name);
        return 0;
    }

    return file;
}
