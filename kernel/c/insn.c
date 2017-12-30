/*
 * x86 instruction analysis
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 * Copyright (C) IBM Corporation, 2002, 2004, 2009
 */

#include "../h/inat.h"
#include "../h/insn.h"
#include "../../libc/stdlib.h"

#define __get_next(t, insn)	\
	({ t r = *(t*)insn->next_byte; insn->next_byte += sizeof(t); r; })

#define __peek_nbyte_next(t, insn, n)	\
	({ t r = *(t*)((insn)->next_byte + n); r; })

#define get_next(t, insn)	\
	({ __get_next(t, insn); })

#define peek_nbyte_next(t, insn, n)	\
	({ __peek_nbyte_next(t, insn, n); })

#define peek_next(t, insn)	peek_nbyte_next(t, insn, 0)

/**
 * insn_init() - initialize struct insn
 * @insn:	&struct insn to be initialized
 * @kaddr:	address (in kernel memory) of instruction (or copy thereof)
 * @x86_64:	!0 for 64-bit kernel or 64-bit app
 */
void insn_init(struct insn *insn, const void *kaddr, int buf_len, int x86_64)
{
    /*
     * Instructions longer than MAX_INSN_SIZE (15 bytes) are invalid
     * even if the input buffer is long enough to hold them.
     */
    if (buf_len > MAX_INSN_SIZE)
        buf_len = MAX_INSN_SIZE;

    memset(insn, 0, sizeof(*insn));
    insn->next_byte = kaddr;
    insn->opnd_bytes = 4;
    insn->addr_bytes = 8;
    insn->vector_bytes = 16;
}

/**
 * insn_get_modrm - collect ModRM byte, if any
 * @insn:       &struct insn containing instruction
 *
 * Populates @insn->modrm and updates @insn->next_byte to point past the
 * ModRM byte, if any.  If necessary, first collects the preceding bytes
 * (prefixes and opcode(s)).  No effect if @insn->modrm.got is already 1.
 */
void insn_get_modrm(struct insn *insn)
{
    struct insn_field *modrm = &insn->modrm;
    insn_byte_t pfx_id, mod;
    if (modrm->got)
        return;
    if (!insn->opcode.got)
        insn_get_opcode(insn);

    if (inat_has_modrm(insn->attr)) {
        mod = get_next(insn_byte_t, insn);
        modrm->value = mod;
        modrm->nbytes = 1;
        if (inat_is_group(insn->attr)) {
            pfx_id = insn_last_prefix_id(insn);
            insn->attr = inat_get_group_attribute(mod, pfx_id,
                                                  insn->attr);
            if (insn_is_avx(insn) && !inat_accept_vex(insn->attr))
                insn->attr = 0; /* This is bad */
        }
    }

    if (inat_is_force64(insn->attr))
        insn->opnd_bytes = 8;
    modrm->got = 1;

    return;
}

/**
 * insn_get_prefixes - scan x86 instruction prefix bytes
 * @insn:	&struct insn containing instruction
 *
 * Populates the @insn->prefixes bitmap, and updates @insn->next_byte
 * to point to the (first) opcode.  No effect if @insn->prefixes.got
 * is already set.
 */
void insn_get_prefixes(struct insn *insn)
{
    struct insn_field *prefixes = &insn->prefixes;
    insn_attr_t attr;
    insn_byte_t b, lb;
    int i, nb;

    if (prefixes->got)
        return;

    nb = 0;
    lb = 0;
    b = peek_next(insn_byte_t, insn);
    attr = inat_get_opcode_attribute(b);
    while (inat_is_legacy_prefix(attr)) {
        /* Skip if same prefix */
        for (i = 0; i < nb; i++)
            if (prefixes->bytes[i] == b)
                goto found;
        if (nb == 4)
            /* Invalid instruction */
            break;
        prefixes->bytes[nb++] = b;
        if (inat_is_address_size_prefix(attr)) {
            /* address size switches 2/4 or 4/8 */
            insn->addr_bytes ^= 12;
        } else if (inat_is_operand_size_prefix(attr)) {
            /* oprand size switches 2/4 */
            insn->opnd_bytes ^= 6;
        }
        found:
        prefixes->nbytes++;
        insn->next_byte++;
        lb = b;
        b = peek_next(insn_byte_t, insn);
        attr = inat_get_opcode_attribute(b);
    }
    /* Set the last prefix */
    if (lb && lb != insn->prefixes.bytes[3]) {
        if (insn->prefixes.bytes[3]) {
            /* Swap the last prefix */
            b = insn->prefixes.bytes[3];
            for (i = 0; i < nb; i++)
                if (prefixes->bytes[i] == lb)
                    prefixes->bytes[i] = b;
        }
        insn->prefixes.bytes[3] = lb;
    }

    /* Decode REX prefix */
    b = peek_next(insn_byte_t, insn);
    attr = inat_get_opcode_attribute(b);
    if (inat_is_rex_prefix(attr)) {
        insn->rex_prefix.value = b;
        insn->rex_prefix.nbytes = 1;
        insn->next_byte++;
        if (X86_REX_W(b))
            /* REX.W overrides opnd_size */
            insn->opnd_bytes = 8;
    }
    insn->rex_prefix.got = 1;

    /* Decode VEX prefix */
    b = peek_next(insn_byte_t, insn);
    attr = inat_get_opcode_attribute(b);
    if (inat_is_vex_prefix(attr)) {
        insn_byte_t b2 = peek_nbyte_next(insn_byte_t, insn, 1);
        insn->vex_prefix.bytes[0] = b;
        insn->vex_prefix.bytes[1] = b2;
        if (inat_is_evex_prefix(attr)) {
            //4
            b2 = peek_nbyte_next(insn_byte_t, insn, 2);
            insn->vex_prefix.bytes[2] = b2;
            insn_byte_t b3 = peek_nbyte_next(insn_byte_t, insn, 3);
            insn->vex_prefix.bytes[3] = b2;
            insn->vex_prefix.nbytes = 4;
            insn->next_byte += 4;
            if (X86_EVEX_W(b2)){
                /* EVEX.W overrides opnd_size */
                insn->opnd_bytes = 8;
            }
            if (X86_EVEX_L(b3)){
                insn->vector_bytes = 32;
            }else if (X86_EVEX_L_BT(b3)){
                insn->vector_bytes = 64;
            }
        }else{
            if (inat_is_vex3_prefix(attr)) {
                //3
                b2 = peek_nbyte_next(insn_byte_t, insn, 2);
                insn->vex_prefix.bytes[2] = b2;
                insn->vex_prefix.nbytes = 3;
                insn->next_byte += 3;
            } else {
                //
                /*
                 * For VEX2, fake VEX3-like byte#2.
                 * Makes it easier to decode vex.W, vex.vvvv,
                 * vex.L and vex.pp. Masking with 0x7f sets vex.W == 0.
                 */
                insn->vex_prefix.bytes[2] = b2 & 0x7f;
                insn->vex_prefix.nbytes = 2;
                insn->next_byte += 2;
            }

            if (X86_VEX_W(b2))
                /* VEX.W overrides opnd_size */
                insn->opnd_bytes = 8;
            if (X86_VEX_L(b2)){
                /* VEX.L overrides opnd_size */
                insn->vector_bytes = 32;
            }
        }
    }
    insn->vex_prefix.got = 1;

    prefixes->got = 1;

    return;
}

/**
 * insn_get_opcode - collect opcode(s)
 * @insn:	&struct insn containing instruction
 *
 * Populates @insn->opcode, updates @insn->next_byte to point past the
 * opcode byte(s), and set @insn->attr (except for groups).
 * If necessary, first collects any preceding (prefix) bytes.
 * Sets @insn->opcode.value = opcode1.  No effect if @insn->opcode.got
 * is already 1.
 */
void insn_get_opcode(struct insn *insn)
{
    struct insn_field *opcode = &insn->opcode;
    insn_byte_t op;
    int pfx_id;
    if (opcode->got)
        return;
    if (!insn->prefixes.got)
        insn_get_prefixes(insn);

    /* Get first opcode */
    op = get_next(insn_byte_t, insn);
    opcode->bytes[0] = op;
    opcode->nbytes = 1;

    /* Check if there is VEX prefix or not */
    if (insn_is_avx(insn)) {
        insn_byte_t m, p;
        m = insn_vex_m_bits(insn);
        p = insn_vex_p_bits(insn);
        insn->attr = inat_get_avx_attribute(op, m, p);
        if ((inat_must_evex(insn->attr) && !insn_is_evex(insn)) ||
            (!inat_accept_vex(insn->attr) &&
             !inat_is_group(insn->attr)))
            insn->attr = 0;	/* This instruction is bad */
        goto end;	/* VEX has only 1 byte for opcode */
    }

    insn->attr = inat_get_opcode_attribute(op);
    while (inat_is_escape(insn->attr)) {
        /* Get escaped opcode */
        op = get_next(insn_byte_t, insn);
        opcode->bytes[opcode->nbytes++] = op;
        pfx_id = insn_last_prefix_id(insn);
        insn->attr = inat_get_escape_attribute(op, pfx_id, insn->attr);
    }
    if (inat_must_vex(insn->attr))
        insn->attr = 0;	/* This instruction is bad */
    end:
    opcode->got = 1;

    insn_get_modrm(insn);

    return;
}
