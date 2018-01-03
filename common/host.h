//
// Created by ben on 31/12/17.
//

#ifndef COMMON_HOST_H
#define COMMON_HOST_H

HOSTCALL_DEF(exit)
HOSTCALL_DEF(write)
HOSTCALL_DEF(read)
HOSTCALL_DEF(open)
HOSTCALL_DEF(openat)
HOSTCALL_DEF(close)
HOSTCALL_DEF(dup)
HOSTCALL_DEF(fstat)
HOSTCALL_DEF(stat)
HOSTCALL_DEF(mmap)
HOSTCALL_DEF(lseek)
HOSTCALL_DEF(access)
HOSTCALL_DEF(writev)
HOSTCALL_DEF(uname)
HOSTCALL_DEF(set_vma_heap)
HOSTCALL_DEF(map_vma)
HOSTCALL_DEF(unmap_vma)
HOSTCALL_DEF2(allocate_pages)
HOSTCALL_DEF2(map_physical_pages)
HOSTCALL_DEF2(is_vpage_present)
HOSTCALL_DEF2(get_phys_addr)
HOSTCALL_DEF_CUSTOM(unmap_physical_page)

#endif //COMMON_HOST_H
