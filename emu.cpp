#include "sn8.hpp"
#include <srarea.hpp>
#include <frame.hpp>

static int flow;

//------------------------------------------------------------------------
// is bank register?
bool is_bank(void)
{
	return (cmd.Op1.type==o_mem) && (cmd.Op1.addr==SN8_RBANK);
}

//----------------------------------------------------------------------
static void destroy_if_unnamed_array(ea_t ea)
{
  flags_t lF = get_flags_novalue(ea);
  if ( isTail(lF) && segtype(ea) == SEG_IMEM )
  {
    ea_t head = prev_not_tail(ea);
    if ( !has_user_name(get_flags_novalue(head)) )
    {
      do_unknown(head, DOUNK_SIMPLE);
      doByte(head, ea-head);
      ea_t end = nextthat(ea, inf.maxEA, f_isHead, NULL);
      if ( end == BADADDR ) end = getseg(ea)->endEA;
      doByte(ea+1, end-ea-1);
    }
  }
}

//----------------------------------------------------------------------
// propagate the bank/pclath register value to the destination
static void propagate_sreg(ea_t ea, int reg)
{
  if ( isLoaded(ea) )
  {
    sel_t v = getSR(cmd.ea, reg);
    splitSRarea1(ea, reg, v, SR_auto);
  }
}

//----------------------------------------------------------------------
static void process_operand(op_t &x,int ,int isload)
{
  switch ( x.type )
  {
    case o_reg:
    case o_phrase:
      return;
    case o_imm:
  	doImmd(cmd.ea);
      if ( op_adds_xrefs(uFlag, x.n) )
        ua_add_off_drefs2(x, dr_O, calc_outf(x));
      break;
    case o_near:
      {
        cref_t ftype = fl_JN;
        ea_t ea = calc_code_mem(x.addr);
        if ( InstrIsSet(cmd.itype, CF_CALL) )
        {
          if ( !func_does_return(ea) )
            flow = false;
          ftype = fl_CN;
        }
        ua_add_cref(x.offb, ea, ftype);
        propagate_sreg(ea, BANK);
      }
      break;
    case o_mem:
      {
        ea_t ea = calc_data_mem(x.addr);
        destroy_if_unnamed_array(ea);
        ua_add_dref(x.offb, ea, isload ? dr_R : dr_W);
        ua_dodata2(x.offb, ea, x.dtyp);
        doVar(ea);
      }
      break;
    default:
      warning("interr: emu2 %a", cmd.ea);
  }
}

int idaapi emu(void)
{
	uint32 Feature = cmd.get_canon_feature();
  	int flag1 = is_forced_operand(cmd.ea, 0);
  	int flag2 = is_forced_operand(cmd.ea, 1);

  	flow = (Feature & CF_STOP) == 0;

  	if (Feature & CF_USE1) 
  		process_operand(cmd.Op1, flag1, 1);
  	if (Feature & CF_USE2) 
  		process_operand(cmd.Op2, flag2, 1);
  	if (Feature & CF_CHG1) 
  		process_operand(cmd.Op1, flag1, 0);
  	if (Feature & CF_CHG2) 
  		process_operand(cmd.Op2, flag2, 0);

  	if (is_bank())
  	{
  		if ((cmd.itype==SN8_mov) && (cmd.Op2.type==o_imm))
  		{
  			msg("%a: bank change to %d\n", cmd.ea, cmd.Op2.value & 3);
    			splitSRarea1(get_item_end(cmd.ea), BANK, cmd.Op2.value & 3, SR_auto);
  		}
  	}

	if (!flow) 
		flow = conditional_insn();
  	if (segtype(cmd.ea) == SEG_XTRN) 
  		flow = 0;
  	if (flow) 
  		ua_add_cref(0,cmd.ea+cmd.size,fl_F);
  	return 1;
}
