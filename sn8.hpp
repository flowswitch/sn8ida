#ifndef _SN8_HPP
#define _SN8_HPP

#include "../idaidp.hpp"
#include <diskio.hpp>
#include "ins.hpp"

//suffix insn - output menmonic after 1st op
#define CF_SFX		0x80000000

#define SN8_PFLAG	0x086
#define SN8_Z		0
#define SN8_DC		1
#define SN8_C		2

#define SN8_RBANK	0x087

// o_phrase
#define PH_XYZ	1
#define PH_EQ	2
#define PH_NE	3
#define PH_C	4
#define PH_NC	5
#define PH_DC	6
#define PH_NDC	7

//specflag1 - BANK0 address
#define is_bank0	specflag1

//specflag2 - .bit in specval
#define is_bit	specflag2
#define bit	specval

//specflag3 - prefix
#define pfx	specflag3
#define PFX_ASSIGN	1
#define PFX_EQ		2
#define PFX_NE		3

//specflag4 - suffix
#define sfx	specflag4
#define SFX_INC		1
#define SFX_DEC		2
//------------------------------------------------------------------

extern ea_t dataseg;
extern instruc_t Instructions[];

enum regnum_t ENUM_SIZE(uint16)
{
  rA,
  BANK,
  rVcs, rVds,    // virtual registers for code and data segments
};

void idaapi header(void);
void idaapi footer(void);

void idaapi segstart(ea_t ea);
void idaapi segend(ea_t ea);
void idaapi assumes(ea_t ea);         // function to produce assume directives

void idaapi out(void);

int  idaapi ana(void);
int  idaapi emu(void);
bool idaapi outop(op_t &op);
void idaapi data(ea_t ea);


const char *find_sym(ea_t address);
const ioport_bit_t *find_bits(ea_t address);
const char *find_bit(ea_t address, int bit);
ea_t calc_code_mem(ea_t ea);
ea_t calc_data_mem(ea_t ea);
int calc_outf(op_t &x);
ea_t map_port(ea_t from);

bool conditional_insn(void);

#endif
