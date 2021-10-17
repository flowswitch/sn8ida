#include "sn8.hpp"
#include <srarea.hpp>

inline ushort ua_next_full_byte(void)
{
  return (ushort)get_full_byte(cmd.ea + cmd.size++);
}

ea_t GetBank(void)
{
	sel_t v = getSR(cmd.ea, BANK);
  	if (v==BADSEL) 
  		v = 0;
	return (v & 3)<<8;
}

void ops_membit(int code)
{
	cmd.Op1.type = o_mem;
	cmd.Op1.dtyp = dt_byte;
	cmd.Op1.addr = code & 0x0FF;
	if ((code & 0x2000)==0)
		cmd.Op1.addr |= GetBank();
	cmd.Op1.specval = (code>>8) & 7;
	cmd.Op1.is_bit = 1;
}

void ops_mem8(int code)
{
	cmd.Op1.type = o_mem;
	cmd.Op1.dtyp = dt_byte;
	cmd.Op1.addr = GetBank() | (code & 0x0FF);
}

void ops_a_mem8(int code)
{
	cmd.Op1.type = o_reg;
	cmd.Op1.reg = rA;

	cmd.Op2.type = o_mem;
	cmd.Op2.dtyp = dt_byte;
	cmd.Op2.addr = GetBank() | (code & 0x0FF);
}

void ops_a_b0mem8(int code)
{
	cmd.Op1.type = o_reg;
	cmd.Op1.reg = rA;

	cmd.Op2.type = o_mem;
	cmd.Op2.dtyp = dt_byte;
	cmd.Op2.addr = code & 0x0FF;
}

void ops_mem8_a(int code)
{
	cmd.Op1.type = o_mem;
	cmd.Op1.dtyp = dt_byte;
	cmd.Op1.addr = GetBank() | (code & 0x0FF);

	cmd.Op2.type = o_reg;
	cmd.Op2.reg = rA;
}

void ops_b0mem8_a(int code)
{
	cmd.Op1.type = o_mem;
	cmd.Op1.dtyp = dt_byte;
	cmd.Op1.addr = code & 0x0FF;

	cmd.Op2.type = o_reg;
	cmd.Op2.reg = rA;
}

void ops_a_imm8(int code)
{
	cmd.Op1.type = o_reg;
	cmd.Op1.reg = rA;

	cmd.Op2.type = o_imm;
	cmd.Op2.dtyp = dt_byte;
	cmd.Op2.value = code & 0x0FF;
}

int idaapi ana(void)
{
  	int code = ua_next_full_byte();
  	if (code & 0x08000) //jmp/call
  	{
  		cmd.itype = (code & 0x4000) ? SN8_call : SN8_jmp;
  		cmd.Op1.type = o_near;
  		cmd.Op1.addr = code & 0x3FFF;
        	cmd.Op1.dtyp = dt_code;
	}
	else switch(code>>11)
	{
		case 6: //B0MOV mem3, imm8
			cmd.itype = SN8_mov;
        		cmd.Op1.type = o_mem;
        		cmd.Op1.dtyp = dt_byte;
        		cmd.Op1.addr = 0x80 + ((code>>8) & 7);

          		cmd.Op2.type  = o_imm;
          		cmd.Op2.value = code & 0x0FF;
        		cmd.Op2.dtyp  = dt_byte;
        		break;
		case 8: //BCLR mem8.bit3
		case 9: //BSET
		case 0xC: //B0BCLR
		case 0xD: //B0BSET
			cmd.itype = (code & 0x0800) ? SN8_bset : SN8_bclr;
			ops_membit(code);
			cmd.Op2.type = o_imm;
			cmd.Op2.dtyp = dt_byte;
			cmd.Op2.value = (code>>11) & 1;
			break;
		case 0xA: //BTS0
		case 0xE: //B0BTS0
		case 0xB: //BTS1
		case 0xF: //B0BTS1
			cmd.itype = SN8_if2;
			ops_membit(code);
			if ((cmd.Op1.addr==SN8_PFLAG) && (cmd.Op1.bit<=SN8_C))
			{
				cmd.itype = SN8_if1;
				cmd.Op1.type = o_phrase;
				switch(cmd.Op1.bit)
				{
					case SN8_Z:
						cmd.Op1.phrase = PH_EQ;
						break;
					case SN8_DC:
						cmd.Op1.phrase = PH_DC;
						break;
					case SN8_C:
						cmd.Op1.phrase = PH_C;
						break;
				}
				cmd.Op1.phrase+=(code>>11) & 1;
				cmd.Op1.is_bit = 0;
			}
			else
			{
				cmd.Op2.type = o_imm;
				cmd.Op2.value = ((code>>11) & 1) ^ 1; //this is SKIP condition, so invert
				cmd.Op2.dtyp = dt_byte;
				cmd.Op2.pfx = PFX_EQ;
			}
			break;
		default:
			switch(code>>8)
			{
				case 0x02: //B0XCH A, mem8
					cmd.itype = SN8_xch;
					ops_a_b0mem8(code);
					break;					
				case 0x03: //B0ADD
					cmd.itype = SN8_add;
					ops_b0mem8_a(code);
					break;
				case 0x06: //CMPRS imm8
					cmd.itype = SN8_if2;
					ops_a_imm8(code);
					cmd.Op2.pfx=PFX_NE;
					break;
				case 0x07: //CMPRS mem8
					cmd.itype = SN8_if2;
					ops_a_mem8(code);
					cmd.Op2.pfx=PFX_NE;
					break;
				case 0x08: //RRC
					cmd.itype = SN8_rrc;
					ops_a_mem8(code);
					break;
				case 0x09: //RRCM
					cmd.itype = SN8_rrcm;
					ops_mem8(code);
					break;
				case 0x0A: //RLC
					cmd.itype = SN8_rlc;
					ops_a_mem8(code);
					break;
				case 0x0B: //RLCM
					cmd.itype = SN8_rlcm;
					ops_mem8(code);
					break;
				case 0x10: //ADC A, mem8
					cmd.itype = SN8_adc;
					ops_a_mem8(code);
					break;
				case 0x11: //ADC mem8, A
					cmd.itype = SN8_adc;
					ops_mem8_a(code);
					break;
				case 0x12: //ADD A, mem8
					cmd.itype = SN8_add;
					ops_a_mem8(code);
					break;
				case 0x13: //ADD mem8, A
					cmd.itype = SN8_add;
					ops_mem8_a(code);
					break;
				case 0x14: //ADD A, imm8
					cmd.itype = SN8_add;
					ops_a_imm8(code);
					break;
				case 0x15: //INCS A, mem8
					cmd.itype = SN8_incs;
					ops_a_mem8(code);
					break;
				case 0x16: //INCMS
					cmd.itype = SN8_if1;
					ops_mem8(code);
					cmd.Op1.sfx = SFX_INC;
					break;
				case 0x17: //SWAP A, mem8
					cmd.itype = SN8_swap;
					ops_a_mem8(code);
					break;
				case 0x18: //OR A, mem8
					cmd.itype = SN8_or;
					ops_a_mem8(code);
					break;
				case 0x19: //OR mem8, A
					cmd.itype = SN8_or;
					ops_mem8_a(code);
					break;
				case 0x1A: //OR A, imm8
					cmd.itype = SN8_or;
					ops_a_imm8(code);
					break;
				case 0x1B: //XOR A, mem8
					cmd.itype = SN8_xor;
					ops_a_mem8(code);
					break;
				case 0x1C: //XOR mem8, A
					cmd.itype = SN8_xor;
					ops_mem8_a(code);
					break;
				case 0x1D: //XOR A, imm8
					cmd.itype = SN8_xor;
					ops_a_imm8(code);
					break;
				case 0x1E: //MOV A, mem8
					cmd.itype = SN8_mov;
					ops_a_mem8(code);
					break;
				case 0x1F: //MOV mem8, A
					cmd.itype = SN8_mov;
					ops_mem8_a(code);
					break;
				case 0x20: //SBC A, mem8
					cmd.itype = SN8_sbc;
					ops_a_mem8(code);
					break;
				case 0x21: //SBC mem8, A
					cmd.itype = SN8_sbc;
					ops_mem8_a(code);
					break;
				case 0x22: //SUB A, mem8
					cmd.itype = SN8_sub;
					ops_a_mem8(code);
					break;
				case 0x23: //SUB mem8, A
					cmd.itype = SN8_sub;
					ops_mem8_a(code);
					break;
				case 0x24: //SUB A, imm8
					cmd.itype = SN8_sub;
					ops_a_imm8(code);
					break;
				case 0x25: //DECS
					cmd.itype = SN8_decs;
					ops_a_mem8(code);
					break;
				case 0x26: //DECMS
					cmd.itype = SN8_if1;
					ops_mem8(code);
					cmd.Op1.sfx = SFX_DEC;
					break;
				case 0x27: //SWAPM
					cmd.itype = SN8_swapm;
					ops_mem8(code);
					break;
				case 0x28: //AND A, mem8
					cmd.itype = SN8_and;
					ops_a_mem8(code);
					break;
				case 0x29: //AND mem8, A
					cmd.itype = SN8_and;
					ops_mem8_a(code);
					break;
				case 0x2A: //AND A, imm8
					cmd.itype = SN8_and;
					ops_a_imm8(code);
					break;
				case 0x2B: //CLR mem8
					cmd.itype = SN8_clr;
					ops_mem8(code);
					cmd.Op2.type = o_imm;
					cmd.Op2.dtyp = dt_byte;
					cmd.Op2.value = 0;
					break;
				case 0x2C: //XCH A, mem8
					cmd.itype = SN8_xch;
					ops_a_mem8(code);
					break;
				case 0x2D: //MOV A, imm8
					cmd.itype = SN8_mov;
					ops_a_imm8(code);
					break;
				case 0x2E: //B0MOV A, B0:mem8
					cmd.itype = SN8_mov;
					ops_a_b0mem8(code);
					break;
				case 0x2F: //B0MOV B0:mem8, A
					cmd.itype = SN8_mov;
					ops_b0mem8_a(code);
					break;
				case 0x38: //MUL A, mem8
					cmd.itype = SN8_mul;
					ops_a_mem8(code);
					break;
				default:
					switch(code)
					{
						case 0x0000: //NOP
							cmd.itype = SN8_nop;
							break;
						case 0x0400: //PUSH
							cmd.itype = SN8_push;
							break;
						case 0x0500: //POP
							cmd.itype = SN8_pop;
							break;
						case 0x0E00: //RET
							cmd.itype = SN8_ret;
							break;
						case 0x0F00: //RETI
							cmd.itype = SN8_reti;
							break;
						case 0x0C00: //DAA
							cmd.itype = SN8_daa;
							cmd.Op1.type = o_reg;
							cmd.Op1.reg = rA;
							break;
						case 0x0D00: //MOVC
							cmd.itype = SN8_movc;
							cmd.Op1.type = o_reg;
							cmd.Op1.reg = rA;
							cmd.Op2.type = o_phrase;
							cmd.Op2.phrase = PH_XYZ;
							break;
						default:
							cmd.itype = SN8_null;
							return 0;
					}
			}
	}
	cmd.size = 1;
	return 1;
}
