#include "tms66x.h"
#include "bits.h"
#include "value_filter.h"
#include "fetch_packet.h"
#include <idp.hpp>
#include <bytes.hpp>

/* 32bit旧的指令参考请以TMS320C674x为主，TMS320C66X为辅，因为后者有很多错误*/

struct tmsinsn_t
{
	uint16 itype;
	uchar src1;
	uchar src2;
	uchar dst;
};

enum TMSC66X_INS_FMT
{
	Dunit_1,    //0:  1 or 2 Sources Instruction Format
	Dunit_2,    //1:  Extended .D Unit 1 or 2 Sources Instruction Format
	Dunit_3,    //2:  ADDAB/ADDAH/ADDAW Long-Immediate Operations
	Dunit_4,    //3:  Load/Store Basic Operations
	Dunit_5,    //4:  Load/Store Long-Immediate Operations
	Dunit_6,    //5:  Load/Store Doubleword Instruction Format
	Dunit_7,    //6:  Load/Store Nonaligned Doubleword Instruction Format
	Lunit_1,    //7:  1 or 2 Sources Instruction Format
	Lunit_2,    //8:  Unary Instruction Format
	Lunit_3,    //9:  1 or 2 Sources, Nonconditional Instruction Format
	Munit_1,    //10: Extended M-Unit with Compound Operations
	Munit_2,    //11: Extended .M-Unit Unary Instruction Format
	Munit_3,    //12: Extended .M Unit 1 or 2 Sources, Nonconditional Instruction Format
    Munit_4,    //13: MPY Instruction Format
	Sunit_1,    //14: 1 or 2 Sources Instruction Format
	Sunit_2,    //15: ADDDP/ADDSP and SUBDP/SUBSP Instruction Format
	Sunit_3,    //16: ADDK Instruction Format
	Sunit_4,    //17: ADDKPC Instruction Format
	Sunit_5,    //18: Extended .S Unit 1 or 2 Sources Instruction Format
	Sunit_6,    //19: Branch Using a Displacement Instruction Format
	Sunit_7,    //20: Branch Using a Register Instruction Format
	Sunit_8,    //21: Branch Using a Pointer Instruction Format
	Sunit_9,    //22: BDEC/BPOS Instruction Format
	Sunit_10,   //23: Branch Using a Displacement with NOP Instruction Format
    Sunit_11,   //24: Branch Using a Register with NOP Instruction Format
    Sunit_12,   //25: Call Nonconditional, Immediate with Implied NOP 5 Instruction Format
    Sunit_13,   //26: Move Constant Instruction Format
    Sunit_14,   //27: Extended .S Unit 1 or 2 Sources, Nonconditional Instruction Format
    Sunit_15,   //28: Unary Instruction Format
	Sunit_16,   //29: Field Operations
	Nunit_1,    //30: DINT and RINT, SWE and SWENR Instruction Format
	Nunit_2,    //31: IDLE and NOP Instruction Format
	Nunit_3,    //32: Loop Buffer, Nonconditional Instruction Format
    Nunit_4,    //33: Loop Buffer Instruction Format
};

#define DUNIT_START 0
#define DUNIT_END 6
#define LUNIT_START 7
#define LUNIT_END 9
#define MUNIT_START 10
#define MUNIT_END 13
#define SUNIT_START 14
#define SUNIT_END 29
#define NUNIT_START 30
#define NUNIT_END 33
//以下格式请参考C674x，C66x很多错误
static filter_table_t g_tms66x_ins[] =
{
	{ ".........................10000..", Dunit_1 },    //0:  1 or 2 Sources Instruction Format
	{ "....................10....1100..", Dunit_2 },    //1:  Extended .D Unit 1 or 2 Sources Instruction Format
    { "0001........................11..", Dunit_3 },    //2:  ADDAB/ADDAH/ADDAW Long-Immediate Operations
	{ "............................01..", Dunit_4 },    //3:  Load/Store Basic Operations
    { "............................11..", Dunit_5 },    //4:  Load/Store Long-Immediate Operations
	{ ".......................1....01..", Dunit_6 },    //5:  Load/Store Doubleword Instruction Format
	{ ".......................1....01..", Dunit_7 },    //6:  Load/Store Nonaligned Doubleword Instruction Format

	{ "...........................110..", Lunit_1 },    //7:  1 or 2 Sources Instruction Format
    { "....................0011010110..", Lunit_2 },    //8:  Unary Instruction Format
	{ "0001.......................110..", Lunit_3 },    //9:  1 or 2 Sources, Nonconditional Instruction Format

	{ "....................0.....1100..", Munit_1 },    //10: Extended M-Unit with Compound Operations
    { "....................0000111100..", Munit_2 },    //11: Extended .M-Unit Unary Instruction Format
	{ "0001................0.....1100..", Munit_3 },    //12: Extended .M Unit 1 or 2 Sources, Nonconditional Instruction Format
    { ".........................00000..", Munit_4 },    //13: MPY Instruction Format
	
    { "..........................1000..", Sunit_1 },    //14: 1 or 2 Sources Instruction Format
    { "...........................110..", Sunit_2 },    //15: ADDDP/ADDSP and SUBDP/SUBSP Instruction Format
    { ".........................10100..", Sunit_3 },    //16: ADDK Instruction Format
    { "...................00001011000..", Sunit_4 },    //17: ADDKPC Instruction Format
    { "....................11....1100..", Sunit_5 },    //18: Extended .S Unit 1 or 2 Sources Instruction Format
    { ".........................00100..", Sunit_6 },    //19: Branch Using a Displacement Instruction Format
    { "....00000.....00000.0011011000..", Sunit_7 },    //20: Branch Using a Register Instruction Format
    { "....0000000...00000000001110001.", Sunit_8 },    //21: Branch Using a Pointer Instruction Format
    { "....................0000001000..", Sunit_9 },    //22: BDEC/BPOS Instruction Format
    { "...................00001001000..", Sunit_10 },   //23: Branch Using a Displacement with NOP Instruction Format
    { "....00001.....00....00110110001.", Sunit_11 },   //24: Branch Using a Register with NOP Instruction Format
    { "0001.....................00100..", Sunit_12 },   //25: Call Nonconditional, Immediate with Implied NOP 5 Instruction Format
    { "..........................1010..", Sunit_13 },   //26: Move Constant Instruction Format
    { "0001................11....1100..", Sunit_14 },   //27: Extended .S Unit 1 or 2 Sources, Nonconditional Instruction Format
    { "....................1111001000..", Sunit_15 },   //28: Unary Instruction Format
    { "..........................0010..", Sunit_16 },   //29: Field Operations

    { "0001..........0....00000000000..", Nunit_1 },    //30: DINT and RINT, SWE and SWENR Instruction Format
	{ "0000..........0....00000000000..", Nunit_2 },    //31: IDLE and NOP Instruction Format
    { "0000..........1....00000000000..", Nunit_3 },    //32: Loop Buffer, Nonconditional Instruction Format
	{ "..............1....00000000000..", Nunit_4 },    //33: Loop Buffer Instruction Format
};

enum TMSC66X_OPCODE_TYPE
{
	t_none,
	t_sint,
	t_xsint,
	t_uint,
	t_xuint,
	t_slong,
	t_xslong,
	t_ulong,
	t_xulong,
	t_scst5,
	t_ucst5,
	t_slsb16,
	t_xslsb16,
	t_ulsb16,
	t_xulsb16,
	t_smsb16,
	t_xsmsb16,
	t_umsb16,
	t_xumsb16,
	t_irp,
	t_cregr,
	t_cregw,
	t_ucst1,
	t_dp,
	t_xdp,
	t_sp,
	t_xsp,
	t_ucst15,
	t_scst7,
	t_ucst3,
	t_b14,
	t_dint,
	t_i2,
	t_xi2,
	t_i4,
	t_xi4,
	t_s2,
	t_xs2,
	t_u2,
	t_xu2,
	t_s4,
	t_xs4,
	t_u4,
	t_xu4,
	t_scst10,
	t_scst12,
	t_scst21,
	t_a3,
	t_bv2,
	t_bv4,
	t_ds2,
	t_sllong,
	t_ullong,
	t_dws4,
	t_dwu4,
    t_int,
    t_xint,
    //new in c66x
    t_xop,
    t_dst
};

static void make_imm(op_t* op, int32 v)
{
	op->type = o_imm;
	op->value = v;
	op->dtype = dt_dword;
}

static void make_spmask(op_t* op, int32 v)
{
	op->type = o_spmask;
	op->dtype = dt_dword;
	op->reg = v;
}

static void make_stgcyc(op_t* op, int32 v)
{
	op->type = o_stgcyc;
	op->dtype = dt_dword;
	op->reg = v;
}

static void make_reg(op_t* op, int32 v, bool isother)
{
    if (isother)
        v += rB0;
    op->type = o_reg;
    op->reg = v;
    op->dtype = dt_dword;
}

static void make_regpair(op_t* op, int32 v, bool isother)
{
    v &= ~1;
    if (isother)
        v += rB0;
    op->type = o_regpair;
    op->reg = v;
    op->dtype = dt_qword;
}

static void make_near(op_t* op, int32 v, fetch_packet_t *fp)
{
    op->type = o_near;
    op->dtype = dt_code;
    if (fp != NULL && fp->fph_vaild)
        op->addr = fp->start + (v << 1);
    else
        op->addr = fp->start + (v << 2);
}

struct tms_reginfo_t
{
    int mask;
    int idx;
    int reg;
};

static const tms_reginfo_t ctrls[] =
{
  { 0x21F, 0x00, rAMR    }, // Addressing mode register
  { 0x21F, 0x01, rCSR    }, // Control status register
//  { 0x21F, 0x02, rIFR    }, // Interrupt flag register
  { 0x21F, 0x02, rISR    }, // Interrupt set register
  { 0x21F, 0x03, rICR    }, // Interrupt clear register
  { 0x21F, 0x04, rIER    }, // Interrupt enable register
  { 0x21F, 0x05, rISTP   }, // Interrupt service table pointer register
  { 0x21F, 0x06, rIRP    }, // Interrupt return pointer register
  { 0x21F, 0x07, rNRP    }, // Nonmaskable interrupt or exception return pointer
  { 0x3FF, 0x0A, rTSCL   }, // Time-stamp counter (low 32 bits) register
  { 0x3FF, 0x0B, rTSCH   }, // Time-stamp counter (high 32 bits) register
  { 0x3FF, 0x0D, rILC    }, // Inner loop count register
  { 0x3FF, 0x0E, rRILC   }, // Reload inner loop count register
  { 0x3FF, 0x0F, rREP    }, // Restricted entry point address register
  { 0x3FF, 0x10, rPCE1   }, // Program counter, E1 phase
  { 0x3FF, 0x11, rDNUM   }, // DSP core number register
  { 0x3FF, 0x12, rFADCR  }, // Floating-point adder configuration register
  { 0x3FF, 0x13, rFAUCR  }, // Floating-point auxiliary configuration register
  { 0x3FF, 0x14, rFMCR   }, // Floating-point multiplier configuration register
  { 0x3FF, 0x15, rSSR    }, // Saturation status register
  { 0x3FF, 0x16, rGPLYA  }, // GMPY A-side polynomial register
  { 0x3FF, 0x17, rGPLYB  }, // GMPY B-side polynomial register
  { 0x3FF, 0x18, rGFPGFR }, // Galois field multiply control register
  { 0x3FF, 0x1A, rTSR    }, // Task state register
  { 0x3FF, 0x1B, rITSR   }, // Interrupt task state register
  { 0x3FF, 0x1C, rNTSR   }, // NMI/Exception task state register
  { 0x3FF, 0x1D, rECR    }, // Exception clear register
//  { 0x3FF, 0x1D, rEFR    }, // Exception flag register
  { 0x3FF, 0x1F, rIERR   }, // Internal exception report register
};

//返回控制寄存器的值
static int find_crreg(int idx)
{
    for (int i = 0; i < qnumber(ctrls); i++)
        if (ctrls[i].idx == (idx & ctrls[i].mask))
            return ctrls[i].reg;
    return -1;
}

static int make_op(
    const insn_t& insn,
    op_t& x,
    uint32 code,
    uchar optype,
    int32 v,
    bool isother,
    fetch_packet_t* fp)
{
    switch (optype)
    {
    case t_none:
        break;
    case t_s2:
    case t_u2:
    case t_i2:
    case t_i4:
    case t_s4:
    case t_u4:
    case t_ds2:
    case t_sint:
    case t_uint:
    case t_int:
    case t_bv2:
    case t_bv4:
        isother = false;
        // no break
    case t_xs2:
    case t_xu2:
    case t_xi2:
    case t_xi4:
    case t_xu4:
    case t_xs4:
    case t_xsint:
    case t_xuint:
    case t_xint:
    case t_xop:
        make_reg(&x, v, isother);
        break;
    case t_slsb16:
    case t_ulsb16:
    case t_smsb16:
    case t_umsb16:
        isother = false;
        // no break
    case t_xslsb16:
    case t_xulsb16:
    case t_xsmsb16:
    case t_xumsb16:
        make_reg(&x, v, isother);
        break;
    case t_dint:
    case t_slong:
    case t_ulong:
    case t_sllong:
    case t_ullong:
    case t_dws4:
    case t_dwu4:
        isother = false;
        // no break
    case t_xslong:
    case t_xulong:
        make_regpair(&x, v, isother);
        break;
    case t_sp:
        isother = false;
        // no break
    case t_xsp:
        make_reg(&x, v, isother);
        x.dtype = dt_float;
        break;
    case t_dp:
        isother = false;
        // no break
    case t_xdp:
        make_regpair(&x, v, isother);
        x.dtype = dt_double;
        break;
    case t_ucst1:
        make_imm(&x, v & 1);
        break;
    case t_scst5:
        v = check_scts(v, 5);
    case t_ucst5:
        make_imm(&x, v);
        break;
    case t_ucst15:
        make_imm(&x, bits_ucst(code, 8, 15));
        break;
    case t_ucst3:
        make_imm(&x, bits_ucst(code, 13, 3));
        break;
    case t_scst7:
        make_near(&x, bits_scst(code, 16, 7), NULL);
        break;
    case t_scst10:
        make_near(&x, bits_scst(code, 13, 10), NULL);
        break;
    case t_scst12:
        make_near(&x, bits_scst(code, 16, 12), fp);
        break;
    case t_scst21:
        make_near(&x, bits_scst(code, 7, 21), NULL);
        break;
    case t_irp:
        x.type = o_reg;
        x.dtype = dt_word;
        if (v == 6)
            x.reg = rIRP;
        else if (v == 7)
            x.reg = rNRP;
        else
            return 0;
        break;
    case t_cregr: // read control reg
    {
        int idx = bits_ucst(code, 18, 5);
        int reg = find_crreg(idx);
        if (reg == -1)
            return 0;
        if (reg == rISR)
            reg = rIFR;
        if (reg == rECR)
            reg = rEFR;
        make_reg(&x, reg, false);
    }
    break;
    case t_cregw:
    {
        int idx = bits_ucst(code, 23, 5);
        int reg = find_crreg(idx);
        if (reg == -1)
            return 0;
        make_reg(&x, reg, false);
    }
    break;
    case t_b14:
        make_reg(&x, rB14 + bits_ucst(code, 7, 1), false);
        break;
    case t_a3:
        make_reg(&x, rA3, isother);
        break;
    default:
        INTERR(257);
    }
    return true;
}

//--------------------------------------------------------------------------
static int table_insns(
	insn_t& insn,
	uint32 code,
	const tmsinsn_t* tinsn,
	bool isother,
    fetch_packet_t* fp)
{
	// +------------------------------------------...
	// |31    29|28|27    23|22   18|17        13|...
	// |  creg  |z |  dst   |  src2 |  src1/cst  |...
	// +------------------------------------------...

	if (tinsn->itype == TMS6_null)
		return 0;
	insn.itype = tinsn->itype;
	if (isother)
		insn.cflags |= aux_xp;  // xpath is used
	op_t* xptr = &insn.Op1;
	if (!make_op(insn, *xptr, code, tinsn->src1, bits_ucst(code, 13, 5), bits_check(code, 1), fp))
		return 0;
	if (xptr->type != o_void)
		xptr++;
	if (!make_op(insn, *xptr, code, tinsn->src2, bits_ucst(code, 18, 5), bits_ucst(code, 12, 1) != bits_ucst(code, 1, 1), fp))
		return 0;
	if (xptr->type != o_void)
		xptr++;
	if (!make_op(insn, *xptr, code, tinsn->dst, bits_ucst(code, 23, 5), bits_check(code, 1), fp))
		return 0;
    insn.size = 4;
	return insn.size;
}

static int d_unit_ins(insn_t* insn, int ctype, uint32_t code, fetch_packet_t* fp)
{

}

static const tmsinsn_t lunit_ops[128] =
{                                                                // bits 11..5
  { TMS6_pack2,  t_i2,          t_xi2,          t_i2            }, // 000 0000
  { TMS6_dptrunc,t_none,        t_dp,           t_sint          }, // 000 0001
  { TMS6_add,    t_scst5,       t_xsint,        t_sint          }, // 000 0010
  { TMS6_add,    t_sint,        t_xsint,        t_sint          }, // 000 0011
  { TMS6_sub2,   t_i2,          t_xi2,          t_i2            }, // 000 0100
  { TMS6_add2,   t_i2,          t_xi2,          t_i2            }, // 000 0101
  { TMS6_sub,    t_scst5,       t_xsint,        t_sint          }, // 000 0110
  { TMS6_sub,    t_sint,        t_xsint,        t_sint          }, // 000 0111
  { TMS6_dpint,  t_none,        t_dp,           t_sint          }, // 000 1000
  { TMS6_dpsp,   t_none,        t_dp,           t_sp            }, // 000 1001
  { TMS6_spint,  t_none,        t_sp,           t_sint          }, // 000 1010
  { TMS6_sptrunc,t_none,        t_xsp,          t_sint          }, // 000 1011
  { TMS6_null,   t_none,        t_none,         t_none          }, // 000 1100
  { TMS6_null,   t_none,        t_none,         t_none          }, // 000 1101
  { TMS6_ssub,   t_scst5,       t_xsint,        t_sint          }, // 000 1110
  { TMS6_ssub,   t_sint,        t_xsint,        t_sint          }, // 000 1111
  { TMS6_addsp,  t_sp,          t_xsp,          t_sp            }, // 001 0000
  { TMS6_subsp,  t_sp,          t_xsp,          t_sp            }, // 001 0001
  { TMS6_sadd,   t_scst5,       t_xsint,        t_sint          }, // 001 0010
  { TMS6_sadd,   t_sint,        t_xsint,        t_sint          }, // 001 0011
  { TMS6_null,   t_none,        t_none,         t_none          }, // 001 0100
  { TMS6_subsp,  t_xsp,         t_sp,           t_sp            }, // 001 0101
  { TMS6_null,   t_none,        t_none,         t_none          }, // 001 0110
  { TMS6_sub,    t_xsint,       t_sint,         t_sint          }, // 001 0111
  { TMS6_adddp,  t_dp,          t_xdp,          t_dp            }, // 001 1000
  { TMS6_subdp,  t_dp,          t_xdp,          t_dp            }, // 001 1001
  { TMS6_null,   t_none,        t_xsint,        t_sint          }, // 001 1010 * why parse args?
  { TMS6_packlh2,t_i2,          t_xi2,          t_i2            }, // 001 1011 *
  { TMS6_packhl2,t_i2,          t_xi2,          t_i2            }, // 001 1100
  { TMS6_subdp,  t_xdp,         t_dp,           t_dp            }, // 001 1101
  { TMS6_packh2, t_i2,          t_xi2,          t_i2            }, // 001 1110
  { TMS6_ssub,   t_xsint,       t_sint,         t_sint          }, // 001 1111
  { TMS6_add,    t_scst5,       t_slong,        t_slong         }, // 010 0000
  { TMS6_add,    t_xsint,       t_slong,        t_slong         }, // 010 0001
  { TMS6_null,   t_none,        t_none,         t_none          }, // 010 0010
  { TMS6_add,    t_sint,        t_xsint,        t_slong         }, // 010 0011
  { TMS6_sub,    t_scst5,       t_slong,        t_slong         }, // 010 0100
  { TMS6_null,   t_none,        t_none,         t_none          }, // 010 0101
  { TMS6_null,   t_none,        t_none,         t_none          }, // 010 0110
  { TMS6_sub,    t_sint,        t_xsint,        t_slong         }, // 010 0111
  { TMS6_null,   t_none,        t_none,         t_none          }, // 010 1000
  { TMS6_addu,   t_xuint,       t_ulong,        t_ulong         }, // 010 1001
  { TMS6_null,   t_none,        t_none,         t_none          }, // 010 1010
  { TMS6_addu,   t_uint,        t_xuint,        t_ulong         }, // 010 1011
  { TMS6_ssub,   t_scst5,       t_slong,        t_slong         }, // 010 1100
  { TMS6_null,   t_none,        t_none,         t_none          }, // 010 1101
  { TMS6_null,   t_none,        t_none,         t_none          }, // 010 1110
  { TMS6_subu,   t_uint,        t_xuint,        t_ulong         }, // 010 1111
  { TMS6_sadd,   t_scst5,       t_slong,        t_slong         }, // 011 0000
  { TMS6_sadd,   t_xsint,       t_slong,        t_slong         }, // 011 0001
  { TMS6_null,   t_none,        t_none,         t_none          }, // 011 0010
  { TMS6_null,   t_none,        t_none,         t_none          }, // 011 0011
  { TMS6_null,   t_none,        t_none,         t_none          }, // 011 0100
  { TMS6_null,   t_none,        t_none,         t_none          }, // 011 0101
  { TMS6_null,   t_none,        t_none,         t_none          }, // 011 0110
  { TMS6_sub,    t_xsint,       t_sint,         t_slong         }, // 011 0111
  { TMS6_abs,    t_none,        t_slong,        t_slong         }, // 011 1000
  { TMS6_intdp,  t_none,        t_xsint,        t_dp            }, // 011 1001
  { TMS6_null,   t_none,        t_none,         t_none          }, // 011 1010
  { TMS6_intdpu, t_none,        t_xuint,        t_dp            }, // 011 1011
  { TMS6_null,   t_none,        t_none,         t_none          }, // 011 1100
  { TMS6_null,   t_none,        t_none,         t_none          }, // 011 1101
  { TMS6_null,   t_none,        t_none,         t_none          }, // 011 1110
  { TMS6_subu,   t_xuint,       t_uint,         t_ulong         }, // 011 1111
  { TMS6_sat,    t_none,        t_slong,        t_sint          }, // 100 0000
  { TMS6_min2,   t_s2,          t_xs2,          t_s2            }, // 100 0001
  { TMS6_max2,   t_s2,          t_xs2,          t_s2            }, // 100 0010
  { TMS6_maxu4,  t_u4,          t_xu4,          t_u4            }, // 100 0011
  { TMS6_cmpgt,  t_scst5,       t_slong,        t_uint          }, // 100 0100
  { TMS6_cmpgt,  t_xsint,       t_slong,        t_uint          }, // 100 0101
  { TMS6_cmpgt,  t_scst5,       t_xsint,        t_uint          }, // 100 0110
  { TMS6_cmpgt,  t_sint,        t_xsint,        t_uint          }, // 100 0111
  { TMS6_minu4,  t_u4,          t_xu4,          t_u4            }, // 100 1000
  { TMS6_intspu, t_none,        t_xuint,        t_sp            }, // 100 1010
  { TMS6_intsp,  t_none,        t_xsint,        t_sp            }, // 100 1010
  { TMS6_subc,   t_uint,        t_xuint,        t_uint          }, // 100 1011
  // 1: on the C62x and C67x, op1 is ucst4 (the TI docs are misleading); on the
  //    C64x/C64x+ and C66x, op1 is ucst5; since the MSB should always be clear
  //    on the former we can treat it as a 5-bit field
  { TMS6_cmpgtu, t_ucst5,       t_ulong,        t_uint          }, // 100 1100 // see [1]
  { TMS6_cmpgtu, t_xuint,       t_ulong,        t_uint          }, // 100 1101
  { TMS6_cmpgtu, t_ucst5,       t_xuint,        t_uint          }, // 100 1110 // see [1]
  { TMS6_cmpgtu, t_uint,        t_xuint,        t_uint          }, // 100 1111
  { TMS6_cmpeq,  t_scst5,       t_slong,        t_uint          }, // 101 0000
  { TMS6_cmpeq,  t_xsint,       t_slong,        t_uint          }, // 101 0001
  { TMS6_cmpeq,  t_scst5,       t_xsint,        t_uint          }, // 101 0010
  { TMS6_cmpeq,  t_sint,        t_xsint,        t_uint          }, // 101 0011
  { TMS6_cmplt,  t_scst5,       t_slong,        t_uint          }, // 101 0100
  { TMS6_cmplt,  t_xsint,       t_slong,        t_uint          }, // 101 0101
  { TMS6_cmplt,  t_scst5,       t_xsint,        t_uint          }, // 101 0110
  { TMS6_cmplt,  t_sint,        t_xsint,        t_uint          }, // 101 0111
  { TMS6_null,   t_none,        t_none,         t_none          }, // 101 1000
  { TMS6_null,   t_none,        t_none,         t_none          }, // 101 1001
  { TMS6_subabs4,t_u4,          t_xu4,          t_u4            }, // 101 1010
  { TMS6_null,   t_none,        t_none,         t_none          }, // 101 1011
  { TMS6_cmpltu, t_ucst5,       t_ulong,        t_uint          }, // 101 1100 // see [1]
  { TMS6_cmpltu, t_xuint,       t_ulong,        t_uint          }, // 101 1101
  { TMS6_cmpltu, t_ucst5,       t_xuint,        t_uint          }, // 101 1110 // see [1]
  { TMS6_cmpltu, t_uint,        t_xuint,        t_uint          }, // 101 1111
  { TMS6_norm,   t_none,        t_slong,        t_uint          }, // 110 0000
  { TMS6_shlmb,  t_u4,          t_xu4,          t_u4            }, // 110 0001
  { TMS6_shrmb,  t_u4,          t_xu4,          t_u4            }, // 110 0010
  { TMS6_norm,   t_none,        t_xsint,        t_uint          }, // 110 0011
  { TMS6_ssub2,  t_s2,          t_xs2,          t_s2            }, // 110 0100
  { TMS6_add4,   t_i4,          t_xi4,          t_i4            }, // 110 0101
  { TMS6_sub4,   t_i4,          t_xi4,          t_i4            }, // 110 0101
  { TMS6_null,   t_none,        t_none,         t_none          }, // 110 0111
  { TMS6_packl4, t_i4,          t_xi4,          t_i4            }, // 110 1000
  { TMS6_packh4, t_i4,          t_xi4,          t_i4            }, // 110 1001
  // op1 is actually scst5, but because only the LSB is meaningful, we decode
  // it as ucst1 so that the result would be correct for the assembler
  { TMS6_lmbd,   t_ucst1,       t_xuint,        t_uint          }, // 110 1010
  { TMS6_lmbd,   t_uint,        t_xuint,        t_uint          }, // 110 1011
  { TMS6_null,   t_none,        t_none,         t_none          }, // 110 1100
  { TMS6_null,   t_none,        t_none,         t_none          }, // 110 1101
  { TMS6_xor,    t_scst5,       t_xuint,        t_uint          }, // 110 1110
  { TMS6_xor,    t_uint,        t_xuint,        t_uint          }, // 110 1111
  { TMS6_addsp,  t_sp,          t_xsp,          t_sp            }, // 111 0000
  { TMS6_subsp,  t_sp,          t_xsp,          t_sp            }, // 111 0001
  { TMS6_adddp,  t_dp,          t_xdp,          t_dp            }, // 111 0010
  { TMS6_subdp,  t_dp,          t_xdp,          t_dp            }, // 111 0011
  { TMS6_null,   t_none,        t_none,         t_none          }, // 111 0100
  { TMS6_subsp,  t_xsp,         t_sp,           t_sp            }, // 111 0101
  { TMS6_null,   t_none,        t_none,         t_none          }, // 111 0110
  { TMS6_subdp,  t_xdp,         t_dp,           t_dp            }, // 111 0111
  { TMS6_null,   t_none,        t_none,         t_none          }, // 111 1000
  { TMS6_null,   t_none,        t_none,         t_none          }, // 111 1001
  { TMS6_and,    t_scst5,       t_xuint,        t_uint          }, // 111 1010
  { TMS6_and,    t_uint,        t_xuint,        t_uint          }, // 111 1011
  { TMS6_andn,   t_uint,        t_xuint,        t_uint          }, // 111 1100
  { TMS6_null,   t_none,        t_none,         t_none          }, // 111 1101
  { TMS6_or,     t_scst5,       t_xuint,        t_uint          }, // 111 1110
  { TMS6_or,     t_uint,        t_xuint,        t_uint          }, // 111 1111
};
static const tmsinsn_t lunit_unary_ops[32] =
{
  { TMS6_abs,    t_none,        t_xsint,        t_sint          }, // 0 0000
  { TMS6_swap4,  t_none,        t_xu4,          t_u4            }, // 0 0001
  { TMS6_unpklu4,t_none,        t_xsint,        t_sint          }, // 0 0010
  { TMS6_unpkhu4,t_none,        t_xsint,        t_sint          }, // 0 0011
  { TMS6_abs2,   t_none,        t_xs2,          t_s2            }, // 0 0100
  { TMS6_mvk,    t_none,        t_scst5,        t_sint          }, // 0 0101
  { TMS6_null,   t_none,        t_none,         t_none          }, // 0 0110
  { TMS6_null,   t_none,        t_none,         t_none          }, // 0 0111
  { TMS6_null,   t_none,        t_none,         t_none          }, // 0 1000
  { TMS6_null,   t_none,        t_none,         t_none          }, // 0 1001
  { TMS6_crot270,t_none,        t_xop,         t_none          }, // 0 1010
  { TMS6_null,   t_none,        t_none,         t_none          }, // 0 1011
  { TMS6_null,   t_none,        t_none,         t_none          }, // 0 1100
  { TMS6_null,   t_none,        t_none,         t_none          }, // 0 1101
  { TMS6_null,   t_none,        t_none,         t_none          }, // 0 1110
  { TMS6_null,   t_none,        t_none,         t_none          }, // 0 1111
  { TMS6_null,   t_none,        t_none,         t_none          }, // 1 0000
  { TMS6_null,   t_none,        t_none,         t_none          }, // 1 0001
  { TMS6_null,   t_none,        t_none,         t_none          }, // 1 0010
  { TMS6_null,   t_none,        t_none,         t_none          }, // 1 0011
  { TMS6_null,   t_none,        t_none,         t_none          }, // 1 0100
  { TMS6_null,   t_none,        t_none,         t_none          }, // 1 0101
  { TMS6_null,   t_none,        t_none,         t_none          }, // 1 0110
  { TMS6_null,   t_none,        t_none,         t_none          }, // 1 0111
  { TMS6_null,   t_none,        t_none,         t_none          }, // 1 1000
  { TMS6_null,   t_none,        t_none,         t_none          }, // 1 1001
  { TMS6_null,   t_none,        t_none,         t_none          }, // 1 1010
  { TMS6_null,   t_none,        t_none,         t_none          }, // 1 1011
  { TMS6_null,   t_none,        t_none,         t_none          }, // 1 1100
  { TMS6_null,   t_none,        t_none,         t_none          }, // 1 1101
  { TMS6_null,   t_none,        t_none,         t_none          }, // 1 1110
  { TMS6_null,   t_none,        t_none,         t_none          }, // 1 1111
};
static int l_unit_ins(insn_t* insn, int ctype, uint32_t code, fetch_packet_t* fp)
{
	char op;
	const tmsinsn_t *table = NULL;

    insn->funit = bits_check(code, 1) ? FU_L2 : FU_L1;
    switch (ctype)
    {
    case Lunit_1:
    case Lunit_3:
        op = bits_ucst(code, 5, 7);
        table = &lunit_ops[op];
        break;
    case Lunit_2:
        op = bits_ucst(code, 13, 5);
        table = &lunit_unary_ops[op];
        break;
    default:
        return 0;
    }

    return table_insns(*insn, code, table, bits_check(code, 12), fp);
}


static const tmsinsn_t munit_mpy_ops[32] =
{                                                              // bits 11..7
  { TMS6_null,    t_none,       t_none,         t_none          }, // 0 0000
  { TMS6_mpyh,    t_smsb16,     t_xsmsb16,      t_sint          }, // 0 0001
  { TMS6_smpyh,   t_smsb16,     t_xsmsb16,      t_sint          }, // 0 0010
  { TMS6_mpyhsu,  t_smsb16,     t_xumsb16,      t_sint          }, // 0 0011
  { TMS6_mpyi,    t_sint,       t_xsint,        t_sint          }, // 0 0100
  { TMS6_mpyhus,  t_umsb16,     t_xsmsb16,      t_sint          }, // 0 0101
  { TMS6_mpyi,    t_scst5,      t_xsint,        t_sint          }, // 0 0110
  { TMS6_mpyhu,   t_umsb16,     t_xumsb16,      t_uint          }, // 0 0111
  { TMS6_mpyid,   t_sint,       t_xsint,        t_dint          }, // 0 1000
  { TMS6_mpyhl,   t_smsb16,     t_xslsb16,      t_sint          }, // 0 1001
  { TMS6_smpyhl,  t_smsb16,     t_xslsb16,      t_sint          }, // 0 1010
  { TMS6_mpyhslu, t_smsb16,     t_xulsb16,      t_sint          }, // 0 1011
  { TMS6_mpyid,   t_scst5,      t_xsint,        t_dint          }, // 0 1100
  { TMS6_mpyhuls, t_umsb16,     t_xslsb16,      t_sint          }, // 0 1101
  { TMS6_mpydp,   t_dp,         t_dp,           t_dp            }, // 0 1110
  { TMS6_mpyhlu,  t_umsb16,     t_xulsb16,      t_uint          }, // 0 1111
  { TMS6_mpy32,   t_sint,       t_xsint,        t_sint          }, // 1 0000
  { TMS6_mpylh,   t_slsb16,     t_xsmsb16,      t_sint          }, // 1 0001
  { TMS6_smpylh,  t_slsb16,     t_xsmsb16,      t_sint          }, // 1 0010
  { TMS6_mpylshu, t_slsb16,     t_xumsb16,      t_sint          }, // 1 0011
  { TMS6_mpy32,   t_sint,       t_xsint,        t_dint          }, // 1 0100
  { TMS6_mpyluhs, t_ulsb16,     t_xsmsb16,      t_sint          }, // 1 0101
  { TMS6_mpy32su, t_sint,       t_xuint,        t_dint          }, // 1 0000
  { TMS6_mpylhu,  t_ulsb16,     t_xumsb16,      t_uint          }, // 1 0111
  { TMS6_mpy,     t_scst5,      t_xslsb16,      t_sint          }, // 1 1000
  { TMS6_mpy,     t_slsb16,     t_xslsb16,      t_sint          }, // 1 1001
  { TMS6_smpy,    t_slsb16,     t_xslsb16,      t_sint          }, // 1 1010
  { TMS6_mpysu,   t_slsb16,     t_xulsb16,      t_sint          }, // 1 1011
  { TMS6_mpysp,   t_sp,         t_xsp,          t_sp            }, // 1 1100
  { TMS6_mpyus,   t_ulsb16,     t_xslsb16,      t_sint          }, // 1 1101
  { TMS6_mpysu,   t_scst5,      t_xulsb16,      t_sint          }, // 1 1110
  { TMS6_mpyu,    t_ulsb16,     t_xulsb16,      t_uint          }, // 1 1111
};
static const tmsinsn_t munit_ops[32] =
{                                                              // bits 11..7
  { TMS6_smpy2,    t_s2,        t_xs2,          t_ullong        }, // 0 0000
  {     -1,        t_none,      t_xu4,          t_u4            }, // 0 0001    todo:check
  { TMS6_mpysu4,   t_s4,        t_xu4,          t_dws4          }, // 0 0010
  { TMS6_dotpnrsu2,t_s2,        t_xu2,          t_sint          }, // 0 0011
  { TMS6_dotpn2,   t_s2,        t_xs2,          t_sint          }, // 0 0100
  { TMS6_dotp2,    t_s2,        t_xs2,          t_sllong        }, // 0 0101
  { TMS6_dotprsu2, t_s2,        t_xu2,          t_sint          }, // 0 0110
  { TMS6_null,     t_none,      t_none,         t_none          }, // 0 0111
  { TMS6_gmpy4,    t_u4,        t_xu4,          t_u4,           }, // 0 1000
  { TMS6_avg2,     t_s2,        t_xs2,          t_s2            }, // 0 1001
  { TMS6_mpyli,    t_sint,      t_xsint,        t_sllong        }, // 0 1010
  // TI docs say mpysp2dp operands are "sp, xsp, sp", but the TI assembler
  // wants a regpair for operand 3, so the docs must be wrong
  { TMS6_mpysp2dp, t_sp,        t_xsp,          t_dp            }, // 0 1011
  { TMS6_mpy32us,  t_uint,      t_xsint,        t_dint          }, // 0 1100
  { TMS6_null,     t_none,      t_none,         t_none          }, // 0 1101
  { TMS6_rotl,     t_uint,      t_xuint,        t_uint          }, // 0 1110
  { TMS6_null,     t_none,      t_none,         t_none          }, // 0 1111
  { TMS6_null,     t_none,      t_none,         t_none          }, // 1 0000
  { TMS6_or,       t_scst5,     t_xuint,        t_uint          }, // 1 0001
  { TMS6_sub2,     t_i2,        t_xi2,          t_i2            }, // 1 0010
  { TMS6_and,      t_scst5,     t_xuint,        t_uint          }, // 1 0011
  { TMS6_null,     t_none,      t_none,         t_none          }, // 1 0100
  { TMS6_add,      t_scst5,     t_xsint,        t_sint          }, // 1 0101
  { TMS6_null,     t_none,      t_none,         t_none          }, // 1 0110
  { TMS6_xor,      t_scst5,     t_xuint,        t_uint          }, // 1 0111
  { TMS6_saddus2,  t_u2,        t_xs2,          t_u2            }, // 1 1000
  { TMS6_saddu4,   t_u4,        t_xu4,          t_u4            }, // 1 1001
  { TMS6_sub,      t_sint,      t_xsint,        t_sint          }, // 1 1010
  { TMS6_shr2,     t_uint,      t_xs2,          t_s2            }, // 1 1011
  { TMS6_shlmb,    t_u4,        t_xu4,          t_u4            }, // 1 1100
  { TMS6_dmv,      t_sint,      t_xsint,        t_dint          }, // 1 1101
  { TMS6_max2,     t_s2,        t_xs2,          t_s2            }, // 1 1110
  { TMS6_pack2,    t_i2,        t_xi2,          t_i2            }, // 1 1111
};
static int m_unit_ins(insn_t* insn, int ctype, uint32_t code, fetch_packet_t* fp)
{
    char op;
    const tmsinsn_t* table = NULL;

    insn->funit = bits_check(code, 1) ? FU_M2 : FU_M1;
    switch (ctype)
    {
    case Munit_1:
    case Munit_2:
    case Munit_3:
        op = bits_ucst(code, 6, 5);
        table = &munit_ops[op];
        break;
    case Munit_4:
        op = bits_ucst(code, 7, 5);
        table = &munit_mpy_ops[op];
        break;
    default:
        return 0;
    }
    return table_insns(*insn, code, table, bits_check(code, 12), fp);
}

static const tmsinsn_t sunit_ops[64] =
{                                                               // bits 11..6
  { TMS6_bdec,   t_scst10,      t_none,         t_uint          }, // 00 0000
  { TMS6_add2,   t_i2,          t_xi2,          t_i2            }, // 00 0001
  { TMS6_spdp,   t_none,        t_xsp,          t_dp            }, // 00 0010
  { TMS6_b,      t_none,        t_irp,          t_none          }, // 00 0011
  { TMS6_bnop,   t_none,        t_scst12,       t_ucst3         }, // 00 0100
  { TMS6_addkpc, t_scst7,       t_ucst3,        t_uint          }, // 00 0101
  { TMS6_add,    t_scst5,       t_xsint,        t_sint          }, // 00 0110
  { TMS6_add,    t_sint,        t_xsint,        t_sint          }, // 00 0111
  { TMS6_packhl2,t_i2,          t_xi2,          t_i2            }, // 00 1000
  { TMS6_packh2, t_i2,          t_xi2,          t_i2            }, // 00 1000
  { TMS6_xor,    t_scst5,       t_xuint,        t_uint          }, // 00 1010
  { TMS6_xor,    t_uint,        t_xuint,        t_uint          }, // 00 1011
  { TMS6_null,   t_none,        t_none,         t_none          }, // 00 1100
  { TMS6_b,      t_none,        t_xuint,        t_none          }, // 00 1101
  { TMS6_mvc,    t_none,        t_xuint,        t_cregw         }, // 00 1110
  { TMS6_mvc,    t_none,        t_cregr,        t_uint          }, // 00 1111
  { TMS6_packlh2,t_i2,          t_xi2,          t_i2            }, // 01 0000
  { TMS6_sub2,   t_sint,        t_xsint,        t_sint          }, // 01 0001
  { TMS6_shl,    t_ucst5,       t_xsint,        t_slong         }, // 01 0010
  { TMS6_shl,    t_uint,        t_xsint,        t_slong         }, // 01 0011
  { TMS6_cmpgt2, t_s2,          t_xs2,          t_bv2           }, // 01 0100
  { TMS6_cmpgtu4,t_u4,          t_xu4,          t_bv4           }, // 01 0101
  { TMS6_sub,    t_scst5,       t_xsint,        t_sint          }, // 01 0110
  { TMS6_sub,    t_sint,        t_xsint,        t_sint          }, // 01 0111
  { TMS6_shr2,   t_ucst5,       t_xs2,          t_s2            }, // 01 1000
  { TMS6_shru2,  t_ucst5,       t_xu2,          t_u2            }, // 01 1001
  { TMS6_or,     t_scst5,       t_xuint,        t_uint          }, // 01 1010
  { TMS6_or,     t_uint,        t_xuint,        t_uint          }, // 01 1011
  { TMS6_cmpeq4, t_s4,          t_xs4,          t_bv4           }, // 01 1100
  { TMS6_cmpeq2, t_s2,          t_xs2,          t_bv2           }, // 01 1101
  { TMS6_and,    t_scst5,       t_xuint,        t_uint          }, // 01 1110
  { TMS6_and,    t_uint,        t_xuint,        t_uint          }, // 01 1111
  { TMS6_sadd,   t_sint,        t_xsint,        t_sint          }, // 10 0000
  { TMS6_null,   t_none,        t_none,         t_none          }, // 10 0001
  { TMS6_sshl,   t_ucst5,       t_xsint,        t_sint          }, // 10 0010
  { TMS6_sshl,   t_uint,        t_xsint,        t_sint          }, // 10 0011
  { TMS6_shru,   t_ucst5,       t_ulong,        t_ulong         }, // 10 0100
  { TMS6_shru,   t_uint,        t_ulong,        t_ulong         }, // 10 0101
  { TMS6_shru,   t_ucst5,       t_xuint,        t_uint          }, // 10 0110
  { TMS6_shru,   t_uint,        t_xuint,        t_uint          }, // 10 0111
  { TMS6_cmpeqdp,t_dp,          t_xdp,          t_sint          }, // 10 1000
  { TMS6_cmpgtdp,t_dp,          t_xdp,          t_sint          }, // 10 1001
  { TMS6_cmpltdp,t_dp,          t_xdp,          t_sint          }, // 10 1010
  { TMS6_extu,   t_uint,        t_xuint,        t_uint          }, // 10 1011
  { TMS6_absdp,  t_dp,          t_none,         t_dp            }, // 10 1100
  { TMS6_rcpdp,  t_dp,          t_none,         t_dp            }, // 10 1101
  { TMS6_rsqrdp, t_dp,          t_none,         t_dp            }, // 10 1110
  { TMS6_ext,    t_uint,        t_xsint,        t_sint          }, // 10 1111
  { TMS6_shl,    t_ucst5,       t_slong,        t_slong         }, // 11 0000
  { TMS6_shl,    t_uint,        t_slong,        t_slong         }, // 11 0001
  { TMS6_shl,    t_ucst5,       t_xsint,        t_sint          }, // 11 0010
  { TMS6_shl,    t_uint,        t_xsint,        t_sint          }, // 11 0011
  { TMS6_shr,    t_ucst5,       t_slong,        t_slong         }, // 11 0100
  { TMS6_shr,    t_uint,        t_slong,        t_slong         }, // 11 0101
  { TMS6_shr,    t_ucst5,       t_xsint,        t_sint          }, // 11 0110
  { TMS6_shr,    t_uint,        t_xsint,        t_sint          }, // 11 0111
  { TMS6_cmpeqsp,t_sp,          t_xsp,          t_sint          }, // 11 1000
  { TMS6_cmpgtsp,t_sp,          t_xsp,          t_sint          }, // 11 1001
  { TMS6_cmpltsp,t_sp,          t_xsp,          t_sint          }, // 11 1010
  { TMS6_set,    t_uint,        t_xuint,        t_uint          }, // 11 1011
  { TMS6_abssp,  t_none,        t_xsp,          t_sp            }, // 11 1100
  { TMS6_rcpsp,  t_none,        t_xsp,          t_sp            }, // 11 1101
  { TMS6_rsqrsp, t_none,        t_xsp,          t_sp            }, // 11 1110
  { TMS6_clr,    t_uint,        t_xuint,        t_uint          }, // 11 1111
};
static const tmsinsn_t sunit_extend_ops[16] = 
{
                                        //bit 9..6
    { TMS6_sadd2, t_s2, t_xs2, t_s2 },      //0000
    { TMS6_saddsu2, t_u2, t_xs2, t_u2 },    //0001
    { TMS6_spack2, t_int, t_xint, t_s2 },   //0010
    { TMS6_saddu4, t_u4, t_xu4, t_u4 },     //0011
    { TMS6_spacku4, t_s2, t_xs2, t_u4 },    //0100
    { TMS6_null, t_none, t_none, t_none },  //0101
    { TMS6_andn, t_uint, t_xuint, t_uint }, //0110
    { TMS6_shr2, t_uint, t_xs2, t_s2 },     //0111
    { TMS6_shru2, t_uint, t_xu2, t_u2 },    //1000
    { TMS6_shlmb, t_u4, t_xu4, t_u4 },      //1001
    { TMS6_shrmb, t_u4, t_xu4, t_u4 },      //1010
    { TMS6_dmv, t_sint, t_xsint, t_dint },  //1011
    { TMS6_min2, t_s2, t_xs2, t_s2 },       //1100
    { TMS6_max2, t_s2, t_xs2, t_s2 },       //1101
    { TMS6_null, t_none, t_none, t_none },  //1110
    { TMS6_pack2, t_i2, t_xi2, t_i2 },      //1111
}
static int s_unit_ins(insn_t* insn, int ctype, uint32_t code, fetch_packet_t* fp)
{
    char op;
    const tmsinsn_t* table = NULL;

    //unit set
    insn->funit = bits_check(code, 1) ? FU_S2 : FU_S1;
    //cross path set
    if (bits_check(code, 12)) insn->cflags |= aux_xp;
    switch (ctype)
    {
    case Sunit_1:
        op = bits_ucst(code, 6, 6);
        table = &sunit_ops[op];
        break;
    case Sunit_2:
        op = bits_ucst(code, 5, 7); //addsp...格式与L单元相近
        table = &lunit_ops[op];
        break;
    case Sunit_3:
        insn->itype = TMS6_addk;
        make_imm(&insn->Op1, bits_ucst(code, 7, 16));
        make_reg(&insn->Op2, bits_ucst(code, 23, 5), bits_check(code, 1));
        return insn->size;
    case Sunit_4:
        insn->funit = FU_S2;
        insn->itype = TMS6_addkpc;
        make_imm(&insn->Op1, bits_scst(code, 16, 7));
        make_imm(&insn->Op2, bits_ucst(code, 13, 3));
        make_reg(&insn->Op3, bits_ucst(code, 23, 5), bits_check(code, 1));
        return insn->size;
    case Sunit_5:
        op = bits_ucst(code, 6, 4);
        table = &sunit_extend_ops[op];
        break;
    case Sunit_6:
        insn->itype = TMS6_b;
        make_near(&insn->Op1, bits_scst(code, 7, 21), NULL);
        return insn->size;
    case Sunit_7:
        insn->itype = TMS6_b;
        insn->funit = FU_S2;
        make_reg(&insn->Op1, bits_ucst(code, 18, 5), bits_check(code, 12));
        return insn->size;
    case Sunit_8:
        insn->itype = bits_check(code, 18) ? TMS6_bnrp: TMS6_birp;
        insn->funit = FU_S2;
        return insn->size;
    case Sunit_9:
        insn->itype = bits_check(code, 12) ? TMS6_bdec : TMS6_bpos;
        make_near(&insn->Op1, bits_scst(code, 13, 10), NULL);
        make_reg(&insn->Op2, bits_ucst(code, 23, 5), bits_check(code, 1));
        return insn->size;
    case Sunit_10:
        insn->itype = TMS6_bnop;
        make_near(&insn->Op1, bits_scst(code, 16, 12), fp);
        make_imm(&insn->Op2, bits_ucst(code, 13, 3));
        return insn->size;
    case Sunit_11:
        insn->itype = TMS6_bnop;
        insn->funit = FU_S2;
        make_reg(&insn->Op1, bits_ucst(code, 18, 5), bits_check(code, 12));
        make_imm(&insn->Op2, bits_ucst(code, 13, 3));
        return insn->size;
    case Sunit_12:
        insn->itype = TMS6_callp;
        make_near(&insn->Op1, bits_scst(code, 7, 21), NULL);
        return insn->size;
    case Sunit_13:
        insn->itype = bits_check(code, 6) ? TMS6_mvkh : TMS6_mvkl;
        make_imm(&insn->Op1, bits_scst(code, 7, 16));
        make_reg(&insn->Op2, bits_ucst(code, 23, 5), bits_check(code, 1));
        return insn->size;
    case Sunit_14:  //op in 6-9
        break;
    case Sunit_15:  //op in 13-17
        break;
    case Sunit_16:  //op in 6-7
        //todo: check op table
        op = bits_ucst(code, 6, 2);
        switch (op)
        {
        case 3:
            insn->itype = TMS6_clr;
            break;
        case 2:
            insn->itype = TMS6_set;
            break;
        case 1:
            insn->itype = TMS6_ext;
            break;
        case 0:
            insn->itype = TMS6_extu;
            break;
        }
        make_imm(&insn->Op1, bits_ucst(code, 13, 5));
        make_imm(&insn->Op2, bits_ucst(code, 8, 5));
        make_reg(&insn->Op3, bits_ucst(code, 23, 5), bits_check(code, 1));
        make_reg(&insn->Op4, bits_ucst(code, 18, 5), bits_check(code, 1));
        insn->Op1.src2 = insn->Op4.reg;
        return insn->size;
    default:
        return 0;
    }
    return table_insns(*insn, code, table, bits_check(code, 12), fp);
}

static int n_unit_ins(insn_t* insn, int ctype, uint32_t code, fetch_packet_t* fp)
{
	char op = bits_ucst(code, 13, 4);

	if (ctype == Nunit_3)
	{
		switch (op)
		{
		case 8://spmask-op is 1000, error in doc
		case 9:
			make_spmask(&insn->Op1, bits_ucst(code, 18, 8));
			insn->itype = op == 8 ? TMS6_spmask : TMS6_spmaskr;
			return insn->size;
		case 10:
			make_stgcyc(&insn->Op1, bits_ucst(code, 22, 6));
			insn->itype = TMS6_spkernel;
			return insn->size;
		case 11:
			insn->itype = TMS6_spkernelr;
			return insn->size;
		}
	}
	else if (ctype == Nunit_2)
	{
		if (op == 15)
		{
			insn->itype = TMS6_idle;
			return insn->size;
		}
		else if (op <= 8)
		{
			insn->itype = TMS6_nop;
			make_imm(&insn->Op1, op+1);
			return insn->size;
		}
	}
	else if (ctype == Nunit_1)
	{
		switch (op)
		{
		case 0://SWE
			insn->itype = TMS6_swe;
			return insn->size;
		case 1://SWENR
			insn->itype = TMS6_swenr;
			return insn->size;
		case 2://DINT
			insn->itype = TMS6_dint;
			return insn->size;
		case 3://rINT
			insn->itype = TMS6_rint;
			return insn->size;
		case 4://MFENCE
			insn->itype = TMS6_rint;
			return insn->size;
		}
    }
    else if (ctype == Nunit_4)
    {
        make_imm(&insn->Op1, bits_ucst(code, 23, 5));
        if (op == 12)	insn->itype = TMS6_sploop;
        if (op == 13)	insn->itype = TMS6_sploopd;
        if (op == 15)	insn->itype = TMS6_sploopw;
        return insn->size;
    }
	return 0;
}

int idaapi ana32(insn_t* insn, fetch_packet_t* fp)
{
	if ((insn->ea & 3) != 0)	//4 bytes align
		return 0;

    //insn size set
	uint32_t code = insn->get_next_dword();
	int ins_ctype = filter(g_tms66x_ins, sizeof(g_tms66x_ins) / sizeof(filter_table_t), code);
	int ret = 0;
	if (ins_ctype >= DUNIT_START && ins_ctype <= DUNIT_END)
		ret = d_unit_ins(insn, ins_ctype, code, fp);
	else if (ins_ctype >= LUNIT_START && ins_ctype <= LUNIT_END)
		ret = l_unit_ins(insn, ins_ctype, code, fp);
	else if (ins_ctype >= MUNIT_START && ins_ctype <= MUNIT_END)
		ret = m_unit_ins(insn, ins_ctype, code, fp);
	else if (ins_ctype >= SUNIT_START && ins_ctype <= SUNIT_END)
		ret = s_unit_ins(insn, ins_ctype, code, fp);
	else if (ins_ctype >= NUNIT_START && ins_ctype <= NUNIT_END)
		ret = n_unit_ins(insn, ins_ctype, code, fp);

    if (ret > 0)
        insn->cond = bits_ucst(code, 28, 4);//cond set

	if (ret > 0 && bits_check(code, 0))
		insn->cflags |= aux_para;   //parallel set
	return ret;
}