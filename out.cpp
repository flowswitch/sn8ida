#include "sn8.hpp"
#include <frame.hpp>
#include <srarea.hpp>
#include <struct.hpp>

bool conditional_insn(void)
{
	if ( isFlow(uFlag) )
  	{
    		int code = get_full_byte(cmd.ea-1);

    		switch(code & 0xFF00)
    		{
    			case 0x0600:
    			case 0x0700:
    			case 0x1500:
    			case 0x1600:
    			case 0x2500:
    			case 0x2600:
    				return true;
		}
		if ((code & 0xD000)==0x5000)
			return true;
	}
	return false;
}

//----------------------------------------------------------------------
static void out_bad_address(ea_t addr)
{
  out_tagon(COLOR_ERROR);
  OutLong(addr, 16);
  out_tagoff(COLOR_ERROR);
  QueueSet(Q_noName, cmd.ea);
}

//----------------------------------------------------------------------
ea_t calc_code_mem(ea_t ea)
{
  return toEA(cmd.cs, ea);
}

//----------------------------------------------------------------------
ea_t calc_data_mem(ea_t ea)
{
  return dataseg + map_port(ea);
}

//----------------------------------------------------------------------
int calc_outf(op_t &x)
{
	return OOFW_8;
}

//----------------------------------------------------------------------
inline void outreg(int r)
{
  out_register(ph.regNames[r]);
}

//----------------------------------------------------------------------
//----------------------------------------------------------------------
bool idaapi outop(op_t &x)
{
	ea_t ea;
	if (x.type==o_void)
		return 0;
	//output prefix
	switch(x.specflag3)
	{
		case PFX_ASSIGN:
			out_line("=", COLOR_INSN);
			break;
		case PFX_EQ:
			out_line("==", COLOR_INSN);
			break;
		case PFX_NE:
			out_line("<>", COLOR_INSN);
			break;
		default:
			if ((x.n!=0) && (!InstrIsSet(cmd.itype, CF_SFX)))
			{
    				out_symbol(',');
    				OutChar(' ');
			}
			break;
	}
  	switch ( x.type )
  	{
    		case o_void:
      			return 0;
    		case o_reg:
      			outreg(x.reg);
      			break;
    		case o_imm:
      			OutValue(x, OOFW_8);
      			break;
    		case o_mem:
      		{
        		ea = calc_data_mem(x.addr);
        		const char *name = find_sym(x.addr);
        		if (name == NULL) 
        		{
        			if (!out_name_expr(x, ea, x.addr))
          				out_bad_address(x.addr);
        		}
        		else
        		{
        			out_addr_tag(ea);
        			out_line(name, COLOR_IMPNAME);
			}
			if (x.specflag2) // .bit
			{
				out_symbol('.');
        			const char *bit = find_bit(x.addr, x.specval);
        			if (bit != NULL)
        			{
          				out_line(bit, COLOR_REG);
        			}
				else
				{
					out_long(x.specval, 10);
				}
			}
      		}
      		break;

    		case o_near:
      		{
        		ea = calc_code_mem(x.addr);
        		if (!out_name_expr(x, ea, x.addr))
          			out_bad_address(x.addr);
      		}
      		break;

      		case o_phrase:
      			switch(x.phrase)
      			{
      				case PH_XYZ:
      					out_line("@YZ", COLOR_REG);
      					break;
				case PH_EQ:
					out_line("EQ", COLOR_INSN);
					break;
				case PH_NE:
					out_line("NE", COLOR_INSN);
					break;
      			}
      			break;

    		default:
      			error("interr: out");
      			break;
  	}
  	//output suffix
  	switch(x.specflag4)
  	{
  		case SFX_INC:
  			out_line("++", COLOR_INSN);
  			break;
  		case SFX_DEC:
  			out_line("--", COLOR_INSN);
  			break;
  	}
  	return 1;
}

//----------------------------------------------------------------------
void idaapi out(void)
{
	char buf[MAXSTR];
  	init_output_buffer(buf, sizeof(buf));

  	if (conditional_insn()) 
  	{
  		OutChar(' ');
	}

	if (!InstrIsSet(cmd.itype, CF_SFX))
	{
  		OutMnem();
  		out_one_operand(0);
	}
	else
	{
		out_one_operand(0);
		OutChar(' ');
		OutMnem(0);
	}

  	if ( cmd.Op2.type != o_void )
  	{
    		out_one_operand(1);
    		if ( cmd.Op3.type != o_void )
    		{
      			out_one_operand(2);
    		}
  	}

  	if (isVoid(cmd.ea, uFlag, 0)) 
  		OutImmChar(cmd.Op1);
  	if (isVoid(cmd.ea, uFlag, 1)) 
  		OutImmChar(cmd.Op2);

  	term_output_buffer();
  	gl_comm = 1;
  	MakeLine(buf);
}

//--------------------------------------------------------------------------
void idaapi assumes(ea_t ea)         // function to produce assume directives
{
}

//--------------------------------------------------------------------------
void idaapi segstart(ea_t ea)
{
}

//--------------------------------------------------------------------------
void idaapi segend(ea_t) 
{
}

//--------------------------------------------------------------------------
void idaapi header(void)
{
  gen_cmt_line("Processor       : %-8.8s", inf.procName);
  gen_cmt_line("Target assembler: %s", ash.name);
  if ( ash.header != NULL )
    for ( const char *const *ptr=ash.header; *ptr != NULL; ptr++ )
      printf_line(0,COLSTR("%s",SCOLOR_ASMDIR),*ptr);
  MakeNull();
}

//--------------------------------------------------------------------------
void idaapi footer(void)
{
  char name[MAXSTR];
  get_colored_name(BADADDR, inf.beginEA, name, sizeof(name));
  const char *end = ash.end;
  if ( end == NULL )
    printf_line(inf.indent,COLSTR("%s end %s",SCOLOR_AUTOCMT), ash.cmnt, name);
  else
    printf_line(inf.indent,COLSTR("%s",SCOLOR_ASMDIR)
                  " "
                  COLSTR("%s %s",SCOLOR_AUTOCMT), ash.end, ash.cmnt, name);
}

//--------------------------------------------------------------------------
void idaapi data(ea_t ea)
{
  gl_name = 1;

  // the kernel's standard routine which outputs the data knows nothing
  // about "equ" directives. So we do the following:
  //    - try to output an "equ" directive
  //    - if we succeed, then ok
  //    - otherwise let the standard data output routine, intel_data()
  //        do all the job

    intel_data(ea);
}
