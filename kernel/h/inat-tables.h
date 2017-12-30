//
// Created by ben on 25/01/17.
//

#ifndef PROJECT_INAT_TABLES_H
#define PROJECT_INAT_TABLES_H

extern const insn_attr_t inat_primary_table[INAT_OPCODE_TABLE_SIZE];
extern const insn_attr_t * const inat_escape_tables[INAT_ESC_MAX + 1][INAT_LSTPFX_MAX + 1];
extern const insn_attr_t * const inat_group_tables[INAT_GRP_MAX + 1][INAT_LSTPFX_MAX + 1];
extern const insn_attr_t * const inat_avx_tables[X86_VEX_M_MAX + 1][INAT_LSTPFX_MAX + 1];

#endif //PROJECT_INAT_TABLES_H
