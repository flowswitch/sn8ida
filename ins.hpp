#ifndef __INSTRS_HPP
#define __INSTRS_HPP

extern instruc_t Instructions[];

enum nameNum ENUM_SIZE(uint16)
{

SN8_null = 0,      // Unknown Operation

// data transfer

SN8_mov,
SN8_xch,
SN8_movc,

// arithmetic

SN8_adc,
SN8_add,
SN8_sbc,
SN8_sub,
SN8_daa,
SN8_mul,

// logic

SN8_and,
SN8_or,
SN8_xor,

// processing

SN8_swap,
SN8_swapm,
SN8_rlc,
SN8_rlcm,
SN8_rrc,
SN8_rrcm,
SN8_clr,
SN8_bclr,
SN8_bset,

// branch

SN8_cmprs,
SN8_incs,
SN8_incms,
SN8_decs,
SN8_decms,
SN8_bts,
SN8_jmp,
SN8_call,

// misc

SN8_ret,
SN8_reti,
SN8_nop,
SN8_push,
SN8_pop,

// pseudo

SN8_if1,
SN8_if2,
SN8_if3,

SN8_last,

    };

#endif
