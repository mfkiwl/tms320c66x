#include "tms66x.h"
#include "fetch_packet.h"

int data_id; //for SET_MODULE_DATA

//--------------------------------------------------------------------------
// A4 B4 A6 B6 A8 B8 - argv
// B3 - return address
// B14 - data page pointer
// B15 - stack pointer
static const char* const RegNames[] =
{
  "A0", "A1",  "A2", "A3",  "A4",  "A5",  "A6",  "A7",
  "A8", "A9", "A10", "A11", "A12", "A13", "A14", "A15",
  "A16", "A17", "A18", "A19", "A20", "A21", "A22", "A23",
  "A24", "A25", "A26", "A27", "A28", "A29", "A30", "A31",
  "B0", "B1", "B2",  "B3",  "B4",  "B5",  "B6",  "B7",
  "B8", "B9", "B10", "B11", "B12", "B13", "B14", "B15",
  "B16", "B17", "B18", "B19", "B20", "B21", "B22", "B23",
  "B24", "B25", "B26", "B27", "B28", "B29", "B30", "B31",
  "AMR",
  "CSR",
  "IFR",
  "ISR",
  "ICR",
  "IER",
  "ISTP",
  "IRP",
  "NRP",
  "ACR",  // undocumented, info from Jeff Bailey <jeff_bailey@infinitek.com>
  "ADR",  // undocumented, info from Jeff Bailey <jeff_bailey@infinitek.com>
  "PCE1",
  "FADCR",
  "FAUCR",
  "FMCR",
  "TSCL",
  "TSCH",
  "ILC",
  "RILC",
  "REP",
  "DNUM",
  "SSR",
  "GPLYA",
  "GPLYB",
  "GFPGFR",
  "TSR",
  "ITSR",
  "NTSR",
  "ECR",
  "EFR",
  "IERR",
  "CS", "DS"
};

//----------------------------------------------------------------------
// This old-style callback only returns the processor module object.
static ssize_t idaapi notify(void*, int msgid, va_list)
{
    if (msgid == processor_t::ev_get_procmod)
        return size_t(SET_MODULE_DATA(tms66x_t));
    return 0;
}

//--------------------------------------------------------------------------
ssize_t idaapi tms66x_t::on_event(ssize_t msgid, va_list va)
{
    int code = 0;
    switch (msgid)
    {
    case processor_t::ev_init:
        hook_event_listener(HT_IDB, this, &LPH);
        break;

    case processor_t::ev_term:
        unhook_event_listener(HT_IDB, this);
        clr_module_data(data_id);
        break;

    case processor_t::ev_out_header:
    {
        outctx_t* ctx = va_arg(va, outctx_t*);
        header(ctx);
        return 1;
    }

    case processor_t::ev_out_footer:
    {
        outctx_t* ctx = va_arg(va, outctx_t*);
        footer(ctx);
        return 1;
    }

    case processor_t::ev_ana_insn:
    {
        static fetch_packet_t s_fp = { 0 };
        insn_t* insn = va_arg(va, insn_t*);

        if (is_in_fetch_packet(insn->ea, &s_fp) == false)
        {
            update_fetch_packet(insn->ea, &s_fp);
        }

        int ins_itype = get_ins_type(insn->ea, &s_fp);
        if (ins_itype == OPCODE_TYPE_16_BIT)
        {
            return ana16(insn, &s_fp);
        }
        else if (ins_itype == OPCODE_TYPE_HEADER)
        {
            insn->itype = TMS6_null;
            insn->cflags |= aux_fph;
            insn->size = 4;
            return 4;
        }
        else if (ins_itype == OPCODE_TYPE_32_BIT)
        {
            return ana32(insn, &s_fp);
        }
        return 0;
    }

    case processor_t::ev_emu_insn:
    {
        const insn_t* insn = va_arg(va, const insn_t*);
        return emu(insn) ? 1 : -1;
    }

    case processor_t::ev_out_insn:
    {
        outctx_t* ctx = va_arg(va, outctx_t*);
        out_insn(ctx);
        return 1;
    }

    case processor_t::ev_out_operand:
    {
        outctx_t* ctx = va_arg(va, outctx_t*);
        const op_t* op = va_arg(va, const op_t*);
        return out_opnd(*ctx, *op) ? 1 : -1;
    }

    default:
        break;
    }
    return code;
}

//-----------------------------------------------------------------------
//           TMS320C6x COFF Assembler (这是什么东西，有必要指定吗)
//-----------------------------------------------------------------------
static const asm_t dspasm =
{
  AS_COLON | ASH_HEXF0 | ASD_DECF0 | ASB_BINF0 | ASO_OCTF5,
  0,
  "TMS320C6x COFF Assembler",
  0,
  NULL,         // header lines
  NULL,         // org
  ".end",

  ";",          // comment string
  '"',          // string delimiter
  '\'',         // char delimiter
  "\\\"'",      // special symbols in char and string constants

  ".string",    // ascii string directive
  ".char",      // byte directive
  ".short",     // word directive
  ".long",      // double words
  NULL,         // no qwords
  NULL,         // oword  (16 bytes)
  NULL,         // float  (4 bytes)
  NULL,         // double (8 bytes)
  NULL,         // tbyte  (10/12 bytes)
  NULL,         // packed decimal real
  NULL,         // arrays (#h,#d,#v,#s(...)
  ".space %s",  // uninited arrays
  ".set",       // equ
  NULL,         // 'seg' prefix (example: push seg seg001)
  "$",          // current IP (instruction pointer)
  NULL,         // func_header
  NULL,         // func_footer
  ".def",       // "public" name keyword
  NULL,         // "weak"   name keyword
  ".ref",       // "extrn"  name keyword
  ".usect",     // "comm" (communal variable)
  NULL,         // get_type_name
  ".align",     // "align" keyword
  '(', ')',     // lbrace, rbrace
  NULL,    // mod
  "&",     // and
  "|",     // or
  "^",     // xor
  "!",     // not
  "<<",    // shl
  ">>",    // shr
  NULL,    // sizeof
};


static const asm_t* const asms[] = { &dspasm, NULL };
//-----------------------------------------------------------------------
#define FAMILY "TMS320C6 series:"
static const char* const shnames[] = { "TMSC66x", NULL };
static const char* const lnames[] =
{
  FAMILY"Texas Instruments TMS320C66x",
  NULL
};

//--------------------------------------------------------------------------
static const uchar retcode_1[] = { 0x62, 0x63, 0x0C, 0x00 };

static const bytes_t retcodes[] =
{
  { sizeof(retcode_1), retcode_1 },
  { 0, NULL }
};

//-----------------------------------------------------------------------
//      Processor Definition
//-----------------------------------------------------------------------
processor_t LPH =
{
  IDP_INTERFACE_VERSION,  // version
  PLFM_TMSC6 + 0x8000,    // id
                          // flag
    PR_USE32
  | PR_DEFSEG32
  | PR_DELAYED
  | PR_ALIGN_INSN,        // allow align instructions
                          // flag2
  0,
  8,                      // 8 bits in a byte for code segments
  8,                      // 8 bits in a byte for other segments

  shnames,
  lnames,

  asms,

  notify,

  RegNames,             // Register names
  qnumber(RegNames),    // Number of registers

  rVcs,                 // first
  rVds,                 // last
  0,                    // size of a segment register
  rVcs, rVds,

  NULL,                 // No known code start sequences
  retcodes,

  TMS6_null,
  TMS6_last,
  Instructions,         // instruc
  0,                    // int tbyte_size;
  { 2, 4, 8, 12 },      // char real_width[4];
  TMS6_null,            // Icode of return instruction. It is ok to give any of possible return instructions
};
