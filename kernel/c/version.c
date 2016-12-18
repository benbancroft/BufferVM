//
// Created by ben on 07/12/16.
//

#include "../../common/version.h"
#include "../h/kernel.h"
#include "../../common/paging.h"
#include "../h/host.h"

bool on_same_page(void *addr1, void *addr2) {
    return P2ALIGN((uint64_t) addr1, PAGE_SIZE) == P2ALIGN((uint64_t) addr2, PAGE_SIZE);
}

void *set_version(uint64_t *addr, size_t length, uint64_t version) {
    uint8_t *version_buf = (uint8_t *)user_version_start + (uint64_t)normalise_version_ptr(addr);

    for (size_t i = 0; i < length; i++)
        version_buf[i] = (uint8_t)version;

    return set_version_ptr(version, addr);
}

uint8_t get_version(uint64_t *addr) {
    return *((uint8_t *)user_version_start + (uint64_t)normalise_version_ptr(addr));
}

bool check_version(void *addr, uint64_t *rip) {
    //printf("Checking version of VA %p at RIP: %p\n", addr, rip);
    uint64_t ptr_ver = get_version_ptr(addr);
    void *norm_addr = normalise_version_ptr(addr);
    for (size_t i = 0; i < 10; i++){
        void *off_ptr_nor = normalise_version_ptr(addr+i);
        if (!on_same_page(norm_addr, off_ptr_nor) && is_vpage_present((uint64_t)off_ptr_nor, NULL)){
            //here we would check to see if rip size was bigger than i
            //if not, we would return true
            return (true);
        }
        else if (get_version(addr+i) != ptr_ver){
            host_print_var((uint64_t )addr);
            host_print_var((uint64_t )normalise_version_ptr(addr));
            printf ("VADDR: %p, RIP: %p PVer: %d byte: %d\n", addr, rip, ptr_ver, i);
            return (false);
        }
    }
    return (true);
}
