//
// Created by ben on 27/12/16.
//
#include "../h/utils.h"
#include "../../intelxed/kit/include/xed-interface.h"

void disassemble_address(uint64_t addr, size_t num_inst){
    uint64_t inst_start = addr;
    #define BUFLEN  100
    char buffer[BUFLEN];
    xed_bool_t ok;
    xed_error_enum_t xed_error;
    xed_decoded_inst_t xedd;

    for (size_t i = 0; i < num_inst; i++){
        xed_decoded_inst_zero(&xedd);

        xed_decoded_inst_set_mode(&xedd, XED_MACHINE_MODE_LONG_64, XED_ADDRESS_WIDTH_64b);
        xed_error = xed_decode(&xedd,
                               XED_REINTERPRET_CAST(const xed_uint8_t*, inst_start),
        15);

        if (xed_error == XED_ERROR_NONE) {
            ok = xed_format_context(XED_SYNTAX_ATT, &xedd, buffer, BUFLEN, 0, 0, 0);
            if (ok) {
                printf("  %x: %s\n", inst_start, buffer);
            }else
                printf("Error disassembling instruction at %p\n", inst_start);
            inst_start += xed_decoded_inst_get_length (&xedd);
        }else{
            printf("Error decoding instruction at %p\n", inst_start);
            return;
        }
    }
}