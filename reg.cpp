#include <ctype.h>
#include "sn8.hpp"
#include <diskio.hpp>
#include <srarea.hpp>

//--------------------------------------------------------------------------

netnode helper;
ea_t dataseg = BADADDR;

//--------------------------------------------------------------------------
static const char *register_names[] =
{
  "A",
  "BANK",
  "cs","ds",       // virtual registers for code and data segments
};

//--------------------------------------------------------------------------
static const uchar retcode_0[] = { 0x00, 0x0E };  // ret
static const uchar retcode_1[] = { 0x00, 0x0F };  // reti

static bytes_t retcodes[] =
{
 { sizeof(retcode_0), retcode_0 },
 { sizeof(retcode_1), retcode_1 },
 { 0, NULL }
};

//-----------------------------------------------------------------------
//      Microchip's MPALC
//-----------------------------------------------------------------------
static const asm_t mpalc =
{
  ASH_HEXF2|ASD_DECF3|ASB_BINF5|ASO_OCTF5|AS_N2CHR|AS_NCMAS|AS_ONEDUP,
  0,
  "Generic ASM",
  0,
  NULL,         // header lines
  NULL,         // no bad instructions
  "org",        // org
  "end",        // end

  ";",          // comment string
  '"',          // string delimiter
  '\'',         // char delimiter
  "'\"",        // special symbols in char and string constants

  "data",       // ascii string directive
  "byte",       // byte directive
  "data",       // word directive
  NULL,         // double words
  NULL,         // qwords
  NULL,         // oword  (16 bytes)
  NULL,         // float  (4 bytes)
  NULL,         // double (8 bytes)
  NULL,         // tbyte  (10/12 bytes)
  NULL,         // packed decimal real
  NULL,         // arrays (#h,#d,#v,#s(...)
  "res %s",     // uninited arrays
  "equ",        // equ
  NULL,         // 'seg' prefix (example: push seg seg001)
  NULL,         // Pointer to checkarg_preline() function.
  NULL,         // char *(*checkarg_atomprefix)(char *operand,void *res); // if !NULL, is called before each atom
  NULL,         // const char **checkarg_operations;
  NULL,         // translation to use in character and string constants.
  "$",          // current IP (instruction pointer)
  NULL,         // func_header
  NULL,         // func_footer
  NULL,         // "public" name keyword
  NULL,         // "weak"   name keyword
  NULL,         // "extrn"  name keyword
  NULL,         // "comm" (communal variable)
  NULL,         // get_type_name
  NULL,         // "align" keyword
  '(', ')',     // lbrace, rbrace
  "%",          // mod
  "&",          // and
  "|",          // or
  "^",          // xor
  "~",          // not
  "<<",         // shl
  ">>",         // shr
  NULL,         // sizeof
};

static const asm_t *const asms[] = { &mpalc, NULL };

//--------------------------------------------------------------------------
struct portmap_t
{
  ea_t from;
  ea_t to;
};

static qvector<portmap_t> map;

static void free_mappings(void)
{
  map.clear();
}

static void add_mapping(ea_t from, ea_t to)
{
  if ( from != to )
  {
    deb(IDA_DEBUG_IDP, "add_mapping %a -> %a\n", from, to);
    portmap_t &p = map.push_back();
    p.from = from;
    p.to = to;
  }
}

ea_t map_port(ea_t from)
{
  for ( int i=0; i < map.size(); i++ )
    if ( map[i].from == from )
      return map[i].to;
  return from;
}

//--------------------------------------------------------------------------
static ioport_t *ports = NULL;
static size_t numports = 0;
char device[MAXSTR] = "";
static const char *cfgname = "sn8.cfg";

inline void get_cfg_filename(char *buf, size_t bufsize)
{
  qstrncpy(buf, cfgname, bufsize);
}

// create the mapping table
static void create_mappings(void)
{
  free_mappings();
  for ( int i=0; i < numports; i++ )
  {
    const char *name = ports[i].name;
    ea_t nameea = get_name_ea(BADADDR, name);
    if ( nameea != BADADDR && nameea > dataseg)
      add_mapping(ports[i].address, nameea-dataseg);
  }
}

//----------------------------------------------------------------------
static ea_t AddSegment(ea_t start, size_t size, ea_t base, const char *name, uchar type)
{
  segment_t s;
  s.startEA = start;
  s.endEA   = start + size;
  s.sel     = allocate_selector(base >> 4);
  s.type    = type;
  s.align   = saRelByte;
  s.comb    = scPub;
  add_segm_ex(&s, name, NULL, ADDSEG_NOSREG|ADDSEG_OR_DIE);
  return s.startEA;
}

//----------------------------------------------------------------------
// special handling for 16-bit PICs
// for CODE segments use addresses as-is
// for DATA segments, start from dataseg base
//lint -esym(528,handle_area)
static bool handle_area(ea_t start, ea_t end, const char *name, const char *aclass)
{
  if ( strcmp(aclass, "CODE") == 0 )
  {
    AddSegment(start, end-start, 0, name, SEG_CODE);
  }
  else if ( strcmp(aclass, "DATA") == 0 )
  {
    if ( dataseg == BADADDR )
      dataseg = freechunk(0, 0x1000, -0xF);
    uchar type = stristr(name, "FSR") != NULL ? SEG_IMEM : SEG_DATA;
    AddSegment(dataseg + start, end-start, dataseg, name, type);
  }
  else
  {
    return false;
  }
  return true;
}

#define NO_GET_CFG_PATH
#define AREA_PROCESSING handle_area

#include "../iocommon.cpp"
static void load_symbols_without_infotype(int /*respect_args*/)
{
  free_ioports(ports, numports);
  ports = read_ioports(&numports, cfgname, device, sizeof(device), callback);
  create_mappings();
}

static void load_symbols(int respect_args)
{
  if ( display_infotype_dialog(IORESP_ALL, &respect_args, cfgname) )
    load_symbols_without_infotype(respect_args);
}

const char *find_sym(ea_t address)
{
  const ioport_t *port = find_ioport(ports, numports, address);
  return port ? port->name : NULL;
}

const ioport_bit_t *find_bits(ea_t address)
{
  const ioport_t *port = find_ioport(ports, numports, address);
  return port ? (*port->bits) : NULL;
}

const char *find_bit(ea_t address, int bit)
{
  address = map_port(address);
  const ioport_bit_t *b = find_ioport_bit(ports, numports, address, bit);
  return b ? b->name : NULL;
}

//----------------------------------------------------------------------
static void apply_symbols(void)
{
  free_mappings();
  if ( dataseg != BADADDR )
  {
    for ( int i=0; i < numports; i++ )
    {
      ea_t ea = calc_data_mem(ports[i].address);
      segment_t *s = getseg(ea);
      if ( s == NULL || s->type != SEG_IMEM )
        continue;
      doByte(ea, 1);
      const char *name = ports[i].name;
      if ( !set_name(ea, name, SN_NOWARN) )
        set_cmt(ea, name, 0);
    }
    for ( segment_t *d = getseg(dataseg); d != NULL; d = get_next_seg(d->startEA) )
    {
      if ( d->type != SEG_IMEM )
        continue;
      ea_t ea = d->startEA;
      ea_t dataend = d->endEA;
      while ( 1 )
      {
        ea = next_unknown(ea, dataend);
        if ( ea == BADADDR )
          break;
        ea_t end = nextthat(ea, dataend, f_isHead, NULL);
        if ( end == BADADDR )
          end = dataend;
        doByte(ea, end-ea);
      }
    }
    create_mappings();
  }
}

//------------------------------------------------------------------
static void setup_device(int lrespect_info)
{
  if ( choose_ioport_device(cfgname, device, sizeof(device), parse_area_line0) )
  {
    // we don't pass IORESP_PORT because that would rename bytes in the code segment
    // we'll handle port renaming ourselves
    if ( display_infotype_dialog(IORESP_ALL, &lrespect_info, cfgname) )
    {
      set_device_name(device, lrespect_info & ~IORESP_PORT);
      if ( (lrespect_info & IORESP_PORT) != 0 )
         apply_symbols();
    }
  }
}

//----------------------------------------------------------------------
static ea_t AdditionalSegment(size_t size, ea_t offset, const char *name)
{
  ea_t start = freechunk(0, size, -0xF);
  return AddSegment(start, size, start - offset, name, SEG_IMEM) - offset;
}

//----------------------------------------------------------------------
static int idaapi notify(processor_t::idp_notify msgid, ...)
{
  va_list va;
  va_start(va, msgid);

// A well behaving processor module should call invoke_callbacks()
// in his notify() function. If this function returns 0, then
// the processor module should process the notification itself
// Otherwise the code should be returned to the caller:

  int code = invoke_callbacks(HT_IDP, msgid, va);
  if ( code ) return code;

  switch ( msgid )
  {
    case processor_t::init:
      helper.create("$ sn8");
      helper.supval(0, device, sizeof(device));
    default:
      break;

    case processor_t::term:
      free_mappings();
      free_ioports(ports, numports);
      break;

    case processor_t::newfile:   // new file loaded
      {
        segment_t *s0 = get_first_seg();
        if ( s0 != NULL )
        {
            set_segm_name(s0, "CODE");
            dataseg = AdditionalSegment(0x200, 0, "DATA");
            setup_device(IORESP_ALL);
          segment_t *s1 = getseg(dataseg);
          SetDefaultRegisterValue(s0, BANK, 0);
          SetDefaultRegisterValue(s1, BANK, 0);
        }
      }
      break;

    case processor_t::oldfile:   // old file loaded
      dataseg  = helper.altval(0);
      load_symbols_without_infotype(IORESP_PORT);
      break;

    case processor_t::closebase:
    case processor_t::savebase:
      helper.altset(0,  dataseg);
      helper.supset(0,  device);
      break;

    case processor_t::newprc:    // new processor type
      break;

    case processor_t::newasm:    // new assembler type
      break;

    case processor_t::newseg:    // new segment
      break;

  }
  va_end(va);
  return 1;
}

//--------------------------------------------------------------------------
static void idaapi choose_device(TView *[],int)
{
  if ( choose_ioport_device(cfgname, device, sizeof(device), NULL) )
  {
    load_symbols(IORESP_ALL);
    apply_symbols();
  }
}

//-----------------------------------------------------------------------
static const char *const shnames[] =
{
  "SN8",
  NULL
};
static const char *const lnames[] =
{ 
  "Sonix SN8xxx",
  NULL
};

//-----------------------------------------------------------------------
//      Processor Definition
//-----------------------------------------------------------------------
processor_t LPH =
{
  IDP_INTERFACE_VERSION,        // version
  0x8001,                     // id
  PRN_HEX | PR_SEGS | PR_SGROTHER | PR_RNAMESOK,
  16,                           // 12/14/16 bits in a byte for code segments
  8,                            // 8 bits in a byte for other segments

  shnames,
  lnames,

  asms,

  notify,

  header,
  footer,

  segstart,
  segend,

  assumes,              // generate "assume" directives

  ana,                  // analyze instruction
  emu,                  // emulate instruction

  out,                  // generate text representation of instruction
  outop,                // generate ...                    operand
  intel_data,                 // generate ...                    data
  NULL,                 // compare operands
  NULL,                 // can have type

  qnumber(register_names), // Number of registers
  register_names,       // Register names
  NULL,                 // get abstract register

  0,                    // Number of register files
  NULL,                 // Register file names
  NULL,                 // Register descriptions
  NULL,                 // Pointer to CPU registers

  BANK,                 // first
  rVds,               // last
  0,                    // size of a segment register
  rVcs, rVds,

  NULL,                 // No known code start sequences
  retcodes,

  SN8_null,
  SN8_last,
  Instructions,

  NULL,                 // int  (*is_far_jump)(int icode);
  NULL,                 // Translation function for offsets
  0,                    // int tbyte_size;  -- doesn't exist
  NULL,                 // int (*realcvt)(void *m, ushort *e, ushort swt);
  { 0, 0, 0, 0 },       // char real_width[4];
                        // number of symbols after decimal point
                        // 2byte float (0-does not exist)
                        // normal float
                        // normal double
                        // long double
  NULL,                 // int (*is_switch)(switch_info_t *si);
  NULL,                 // int32 (*gen_map_file)(FILE *fp);
  NULL,                 // ea_t (*extract_address)(ea_t ea,const char *string,int x);
  NULL,          // Check whether the operand is relative to stack pointer
  NULL,    // create frame of newly created function
  NULL, // Get size of function return address in bytes
  NULL,                 // void (*gen_stkvar_def)(char *buf,const member_t *mptr,int32 v);
  NULL,           // Generate text representation of an item in a special segment
  SN8_ret,           // Icode of return instruction. It is ok to give any of possible return instructions
  NULL,      // const char *(*set_idp_options)(const char *keyword,int value_type,const void *value);
  NULL,                 // int (*is_align_insn)(ea_t ea);
  NULL,                 // mvm_t *mvm;
};


