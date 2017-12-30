//
// Created by ben on 07/12/16.
//

#include "../../common/version.h"
#include "../h/kernel.h"
#include "../../common/paging.h"
#include "../../intelxed/kit/include/xed-interface.h"
#include "../h/insn.h"
#include "../h/utils.h"
#include "../h/host.h"
#include "../h/vma.h"

bool on_same_page(void *addr1, void *addr2) {
    return P2ALIGN((uint64_t) addr1, PAGE_SIZE) == P2ALIGN((uint64_t) addr2, PAGE_SIZE);
}

void *set_version(uint64_t *addr, size_t length, uint64_t version) {
    uint8_t *version_buf = (uint8_t *)user_version_start + (uint64_t)normalise_version_ptr(addr);

    uint64_t start_page = PAGE_ALIGN_DOWN((uint64_t)version_buf);
    uint64_t end_page = PAGE_ALIGN_DOWN((uint64_t)version_buf+length);
    size_t pages = PAGE_DIFFERENCE(end_page, start_page);

    map_physical_pages(start_page, -1, PDE64_NO_EXE | PDE64_WRITEABLE,
                       pages, MAP_ZERO_PAGES | MAP_NO_OVERWRITE, 0);

    for (size_t i = 0; i < length; i++)
        version_buf[i] = (uint8_t)version;

    return set_version_ptr(version, addr);
}

uint8_t get_version(uint64_t *addr) {
    uint8_t *version_buf = (uint8_t *)user_version_start + (uint64_t)normalise_version_ptr(addr);

    uint64_t start_page = PAGE_ALIGN_DOWN((uint64_t)version_buf);

    map_physical_pages(start_page, -1, PDE64_NO_EXE | PDE64_WRITEABLE,
                       1, MAP_ZERO_PAGES | MAP_NO_OVERWRITE, 0);

    return (*version_buf);
}

bool check_version_instruction(uint64_t *addr, uint64_t *rip, size_t offset, uint64_t ptr_ver) {
    xed_error_enum_t xed_error;
    xed_bool_t ok;
    xed_decoded_inst_t xedd;
    uint64_t inst_len;

    xed_decoded_inst_zero(&xedd);

    xed_decoded_inst_set_mode(&xedd, XED_MACHINE_MODE_LONG_64, XED_ADDRESS_WIDTH_64b);
    xed_error = xed_decode(&xedd,
                           XED_REINTERPRET_CAST(const xed_uint8_t*, rip),
                           15);

    ASSERT(on_same_page(rip, rip+15));

    if (xed_error == XED_ERROR_NONE) {
        inst_len = xed_decoded_inst_get_operand_width(&xedd) / 8;

        /*struct insn insn;
        insn_init(&insn, (void *)rip, 15, 1);
        insn_get_opcode(&insn);
        insn_attr_t flags = insn.attr;
        bool is_byte = !inat_is_force64(flags) && flags & INAT_BYTEOP;
        bool is_vector = flags & INAT_VEXOK;

        if (is_vector){
            printf("pc %p, xed: %d, mine: %d opcode %x\n", rip, inst_len, is_vector ? insn.vector_bytes : (is_byte ? 1 : insn.opnd_bytes), insn.opcode.value);
            disassemble_address((uint64_t)rip, 1);
        }
        ASSERT(is_vector || inst_len == (is_byte ? 1 : insn.opnd_bytes));*/

        if (inst_len <= offset)
            return (true);
    } else{

        printf ("Failed on getting instruction length - using my method\n");

        struct insn insn;
        insn_init(&insn, (void *)rip, 15, 1);
        insn_get_opcode(&insn);
        insn_attr_t flags = insn.attr;
        bool is_byte = !inat_is_force64(flags) && flags & INAT_BYTEOP;
        bool is_vector = flags & INAT_VEXOK;
        size_t length = is_vector ? insn.vector_bytes : (is_byte ? 1 : insn.opnd_bytes);

        printf("pc %p, length: %d opcode %x\n", rip, length, insn.opcode.value);

        for (size_t i = 0; i < 15; i++) {
            printf(" %x", *((char*)rip+i) & 0xFF);
        }
        putchar('\n');

        if (length <= offset)
            return (true);
    }
    printf ("VADDR: %p, RIP: %p PVer: %d byte: %d\n", addr, rip, ptr_ver, offset);

    vm_area_t *vma = vma_find((uint64_t)normalise_version_ptr(addr));
    vma_print_node(vma, false);

    disassemble_address((uint64_t)normalise_version_ptr(addr), 10);

    return (false);
}

bool check_version(void *addr, uint64_t *rip) {

    vm_area_t *vma = vma_find((uint64_t)normalise_version_ptr(addr));
    ASSERT(vma->page_prot & VMA_IS_VERSIONED);

    //printf("Checking version of VA %p at RIP: %p\n", addr, rip);
    uint64_t ptr_ver = get_version_ptr(addr);
    void *norm_addr = normalise_version_ptr(addr);
    for (size_t i = 0; i < 10; i++){
        void *off_ptr_nor = normalise_version_ptr(addr+i);
        if (!on_same_page(norm_addr, off_ptr_nor) && is_vpage_present((uint64_t)off_ptr_nor, NULL)){
            //check to see if rip size was bigger than i
            //if so, this would have likely failed on execution, but we caught it here
            return check_version_instruction(addr, rip, i, ptr_ver);
        }
        else if (get_version(addr+i) != ptr_ver){
            return check_version_instruction(addr, rip, i, ptr_ver);
        }
    }
    return (true);
}
