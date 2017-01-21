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

int load_elf64_pg_hdrs(int fd, elf64_hdr_t *elf_hdr, void** elf_start, elf_info_t *elf_info, bool is_interp){

    uint64_t start_addr;
    uint64_t min_addr = UINT64_MAX;

    uint64_t section_max_maddr = 0;
    uint64_t section_max_faddr = 0;

    uint64_t max_faddr = 0;
    uint64_t max_maddr = 0;

    uint64_t load_addr = 0;

    bool found_entry = false;

    host_lseek(fd, elf_hdr->e_phoff, SEEK_SET);

    for(size_t i = 0; i < elf_hdr->e_phnum; i++) {
        elf64_phdr_t phdr;
        uint64_t v_addr, map_addr;

        host_read(fd, (char *)&phdr, sizeof (elf64_phdr_t));

        v_addr = phdr.p_vaddr - PAGE_OFFSET(phdr.p_vaddr);
        uint64_t size = phdr.p_filesz + PAGE_OFFSET(phdr.p_vaddr);
        uint64_t file_offset =  phdr.p_offset - PAGE_OFFSET(phdr.p_vaddr);
        v_addr = v_addr & PAGE_MASK;
        size = PAGE_ALIGN(size);
        map_addr = load_addr + v_addr;

        //TODO - size > file_offset

        if (/*load_only && */phdr.p_type != PT_LOAD) {
            continue;
        }

        uint64_t prot = 0, flags = 0;

        if (size) {

            //Readable.
            if (phdr.p_flags & PF_R)
                prot |= PROT_READ;

            //Writeable.
            if (phdr.p_flags & PF_W)
                prot |= PROT_WRITE;

            // Executable.
            if (phdr.p_flags & PF_X)
                prot |= PROT_EXEC;

            if (elf_hdr->e_type == ET_EXEC || found_entry)
                flags |= MAP_FIXED;
            /*else if (elf_hdr->e_type == ET_DYN) {
                v_addr = NULL;
            }*/

            prot |= PROT_EXEC | PROT_WRITE | PROT_READ;

            //if (phdr.p_vaddr == 0) continue;

            start_addr = syscall_mmap(map_addr,
                                      size, prot,
                                      MAP_PRIVATE | flags,
                                      fd,
                                      file_offset);
            //Will handle error codes too - debugging point
            printf("Error %p\n", -(uint64_t) start_addr);
            ASSERT(!IS_ERR_VALUE(start_addr));
            ASSERT(!(flags & MAP_FIXED) || start_addr == map_addr);

        } else {
            start_addr = v_addr;
        }

        if (is_interp && !found_entry && elf_hdr->e_type == ET_DYN) {
            load_addr = start_addr - (v_addr & PAGE_MASK);
            printf("Entry f: %p\n", *elf_start);
            found_entry = true;
        } else if (!is_interp && !found_entry){
            elf_info->load_addr = phdr.p_vaddr - phdr.p_offset;
            found_entry = true;
        }

        section_max_maddr = load_addr + phdr.p_vaddr + phdr.p_memsz;
        section_max_faddr = load_addr + phdr.p_vaddr + phdr.p_filesz;

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

    if (is_interp && elf_start) *elf_start = (void*)load_addr;

    return 0;
}

void load_elf64_interpreter(int fd, char *path, void **elf_entry, elf_info_t *elf_info){
    printf("Interp: %s\n", path);

    elf_info_t interp_elf_info;
    int user_interp_fd = read_binary(path);
    load_elf_binary(user_interp_fd, elf_entry, &interp_elf_info, true, 0);
    *elf_entry = *elf_entry + (uint64_t)interp_elf_info.entry_addr;
}

int load_elf64(int fd, void **elf_entry, elf_info_t *elf_info, bool is_interp){
    elf64_hdr_t elf_hdr;
    int ret;

    host_read(fd, (char *)&elf_hdr, sizeof (elf64_hdr_t));

    printf("Entry point user: %p\n", elf_hdr.e_entry);
    printf("Num program headers: %d\n", elf_hdr.e_phnum);

    elf_info->entry_addr = (void*)elf_hdr.e_entry;
    if (elf_entry) *elf_entry = elf_info->entry_addr;

    host_lseek(fd, elf_hdr.e_phoff, SEEK_SET);

    for(size_t i = 0; i < elf_hdr.e_phnum; i++) {
        elf64_phdr_t phdr;
        uint64_t end_addr;

        host_read(fd, (char *)&phdr, sizeof (elf64_phdr_t));

        if (phdr.p_type == PT_INTERP) {

            char interp_path[phdr.p_filesz];
            host_lseek(fd, phdr.p_offset, SEEK_SET);
            host_read(fd, interp_path, phdr.p_filesz);

            load_elf64_interpreter(fd, interp_path, elf_entry, elf_info);

            //load base elf image after interpreter
            if ((ret = load_elf64_pg_hdrs(fd, &elf_hdr, NULL, elf_info, false)))
                return ret;

            elf_info->entry_addr = (void *) elf_hdr.e_entry;
            elf_info->phdr_num = elf_hdr.e_phnum;
            elf_info->phdr_off = elf_hdr.e_phoff;

            return 0;
        }
    }

    if ((ret = load_elf64_pg_hdrs(fd, &elf_hdr, elf_entry, elf_info, is_interp)))
        return ret;

    elf_info->entry_addr = (void *) elf_hdr.e_entry;
    elf_info->phdr_num = elf_hdr.e_phnum;
    elf_info->phdr_off = elf_hdr.e_phoff;

    return 0;
}

int load_elf_binary(int fd, void **elf_entry, elf_info_t *elf_info, bool is_interp, char *mem_offset){
    if (is_elf(fd)){
        if (is_binary_64bit(fd)){
            printf("64-bit bin\n");
            return load_elf64(fd, elf_entry, elf_info, is_interp);
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
