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
    t_duint,
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
    t_op1,
    t_op2,
    t_xop, 
    t_xop1,
    t_xop2,
    t_dst,
    t_dwdst,
    t_dwop1,
    t_xdwop,
    t_xdwop1,
    t_xdwop2,
    t_qwop1,
    t_qwop2,
    t_qwdst
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

static void make_regpair(op_t* op, int32 v, bool isother, bool is_quad)
{
    v &= ~1;
    if (isother)
        v += rB0;
    op->type = is_quad?o_regqpair:o_regpair;
    op->reg = v;
    op->dtype = dt_qword;
}

static void make_near(op_t* op, uint32_t fp_start, int32 v, int shift = 1)
{
    op->type = o_near;
    op->dtype = dt_code;
    op->addr = fp_start + (v << shift);
}

static void make_displ(op_t* op, int32 base, int32 offset, bool is_other)
{
    make_reg(op, base, is_other);
    op->type = o_displ;
    op->addr = offset;
}

static void make_phrase(op_t* op, int32 base, int32 offset, bool is_other_b, bool is_other_o)
{
    make_reg(op, offset, is_other_o);
    op->secreg = op->reg;
    make_reg(op, base, is_other_b);
    op->type = o_phrase;
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
    bool is_Aside,
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
    case t_xop1:
    case t_xop2:
    case t_dst:
    case t_op1:
    case t_op2:
    case t_slsb16:
    case t_ulsb16:
    case t_smsb16:
    case t_umsb16:
    case t_xslsb16:
    case t_xulsb16:
    case t_xsmsb16:
    case t_xumsb16:
        make_reg(&x, v, is_Aside);
        break;
    case t_dint:
    case t_duint:
    case t_slong:
    case t_ulong:
    case t_sllong:
    case t_ullong:
    case t_dws4:
    case t_dwu4:
    case t_xslong:
    case t_xulong:
    case t_dwdst:
    case t_dwop1:
    case t_xdwop:
    case t_xdwop1:
    case t_xdwop2:
        make_regpair(&x, v, is_Aside, false);
        break;
    case t_qwop1:
    case t_qwop2:
    case t_qwdst:
        make_regpair(&x, v, is_Aside, true);
        break;
    case t_sp:
    case t_xsp:
        make_reg(&x, v, is_Aside);
        x.dtype = dt_float;
        break;
    case t_dp:
    case t_xdp:
        make_regpair(&x, v, is_Aside, false);
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
        make_near(&x, fp->start, bits_scst(code, 16, 7), 2);
        break;
    case t_scst10:
        make_near(&x, fp->start, bits_scst(code, 13, 10), 2);
        break;
    case t_scst12:
        make_near(&x, fp->start, bits_scst(code, 16, 12), fp->fph_vaild ? 1 : 2);
        break;
    case t_scst21:
        make_near(&x, fp->start, bits_scst(code, 7, 21), 2);
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
        make_reg(&x, rA3, is_Aside);
        break;
    default:
        msg("[+]DEBUG: unknown opcode type %d\n", optype);
        return 0;
    }
    return 1;
}

//指令默认为 ins src1, src2, dst
//如果src2在src1之前则两者互换
static void swap_op1_and_op2(insn_t* insn)
{
    op_t tmp = insn->Op1;
    insn->Op1 = insn->Op2;
    insn->Op2 = tmp;
    insn->Op1.n = 0;
    insn->Op2.n = 1;
}

//当出现伪指令时使用
static void swap_op2_and_op3(insn_t* insn)
{
    if ((insn->cflags & aux_pseudo) == 0)
    {
        op_t tmp = insn->Op3;
        insn->Op3 = insn->Op2;
        insn->Op2 = tmp;
        insn->Op2.n = 1;
        insn->Op3.n = 2;
    }
}

static void make_pseudo(insn_t* insn)
{
    switch (insn->itype)
    {
    case TMS6_add:
    case TMS6_or:
        if (insn->Op1.type == o_imm && insn->Op1.value == 0)
        {
            insn->itype = TMS6_mv;
        SHIFT_OPS:
            insn->Op1 = insn->Op2;
            insn->Op2 = insn->Op3;
            insn->Op1.n = 0;
            insn->Op2.n = 1;
            insn->Op3.type = o_void;
            insn->cflags |= aux_pseudo;
        }
        break;
    case TMS6_sub:
        //sub 0, reg1, reg2
        if (insn->Op1.type == o_imm
            && insn->Op1.value == 0
            && insn->funit != FU_D1
            && insn->funit != FU_D2)
        {
            insn->itype = TMS6_neg;
            goto SHIFT_OPS;
        }
        //sub reg1, reg2, regp3 (reg1==reg2)
        if (insn->Op1.type == o_reg
            && insn->Op2.type == o_reg
            && insn->Op3.type == o_regpair  //regpair in C67x
            && insn->Op1.reg == insn->Op2.reg)
        {
            insn->itype = TMS6_zero;
            insn->Op1.reg = insn->Op3.reg;
            insn->Op2.type = o_void;
            insn->Op3.type = o_void;
            insn->cflags |= aux_pseudo;
        }
        break;
    case TMS6_xor:
        if (insn->Op1.type == o_imm && insn->Op1.value == uval_t(-1))
        {
            insn->itype = TMS6_not;
            goto SHIFT_OPS;
        }
        break;
    case TMS6_packlh2:
        if (insn->Op1.type == o_reg
            && insn->Op2.type == o_reg
            && insn->Op1.reg == insn->Op2.reg)
        {
            insn->itype = TMS6_swap2;
            swap_op2_and_op3(insn);
            insn->Op3.type = o_void;
            insn->cflags |= aux_pseudo;
        }
        break;
    //case TMS6_mvk:
    //    if (insn->Op1.type == o_imm && insn->Op1.value == 0)
    //    {
    //        insn->itype = TMS6_zero;
    //        swap_op1_and_op2(insn);
    //        insn->Op2.type = o_void;
    //        insn->cflags |= aux_pseudo;
    //    }
    //    break;
    }
}

//--------------------------------------------------------------------------
static int table_insns(
	insn_t& insn,
	uint32 code,
	const tmsinsn_t* tinsn,
	bool cross_path,
    fetch_packet_t* fp,
    bool src2_first)
{
	// +------------------------------------------...
	// |31    29|28|27    23|22   18|17        13|...
	// |  creg  |z |  dst   |  src2 |  src1/cst  |...
	// +------------------------------------------...

	if (tinsn->itype == TMS6_null)
		return 0;
	insn.itype = tinsn->itype;
	if (cross_path)
		insn.cflags |= aux_xp;  // xpath is used
	op_t* xptr = &insn.Op1;
	if (!make_op(insn, *xptr, code, tinsn->src1, bits_ucst(code, 13, 5), bits_check(code, 1), fp))
		return 0;
	if (xptr->type != o_void)
		xptr++;
	if (!make_op(insn, *xptr, code, tinsn->src2, bits_ucst(code, 18, 5), (uint32_t)cross_path != bits_ucst(code, 1, 1), fp))
		return 0;
	if (xptr->type != o_void)
		xptr++;
	if (!make_op(insn, *xptr, code, tinsn->dst, bits_ucst(code, 23, 5), bits_check(code, 1), fp))
		return 0;
    
    make_pseudo(&insn);

    //有3个操作数且src2在src1前的情况，交换op1和op2
    if (src2_first && insn.Op1.type != o_void && insn.Op2.type != o_void && insn.Op3.type != o_void)
        swap_op1_and_op2(&insn);
    
	return insn.size;
}

/*
* 符合该格式的有Dunit_2、Munit_1、Sunit_5
*    | 31 29 | 28 | 27 23 | 22 18 | 17 13 | 12 | 11 6 | 5 4 3 2 | 1 0 |
*    |  creg |  z |  dst  |  src2 |  src1 |  x |  op  | 1 1 0 0 | s p |
*    |   3   |  1 |   5   |   5   |   5   |  1 |   6  |         | 1 1 |
*/
static tmsinsn_t dms_ops[64] =
{
                                               //BIT 11....6
    { TMS6_mpy2, t_s2, t_xs2, t_ullong },           //000000
    { TMS6_smpy2, t_s2, t_xs2, t_sllong },          //000001
    { TMS6_dotpsu4, t_s4, t_xu4, t_int },           //000010    same as dotpsu4h
    { TMS6_null, t_none, t_none, t_none },          //000011
    { TMS6_mpyu4, t_u4, t_xu4, t_dwu4 },            //000100
    { TMS6_mpysu4, t_s4, t_xu4, t_dws4 },           //000101    same as dotpsu4h
    { TMS6_dotpu4, t_s4, t_xu4, t_int },            //000110    same as dotp4h
    { TMS6_dotpnrsu2, t_s2, t_xu2, t_int },         //000111    same as dotp4h
    { TMS6_mpyu2, t_op1, t_xop2, t_dwdst },         //001000
    { TMS6_dotpn2, t_s2, t_xs2, t_sllong },         //001001
    { TMS6_null, t_none, t_none, t_none },          //001010
    { TMS6_null, t_none, t_none, t_none },          //001011
    { TMS6_dotp2, t_s2, t_xs2, t_int },             //001100
    { TMS6_dotprsu2, t_s2, t_xu2, t_int },          //001101
    { TMS6_mpyilr, t_int, t_xint, t_int },          //001110
    { TMS6_fmpydp, t_dwop1, t_xdwop2, t_dwdst},     //001111
    { TMS6_mpyhir, t_int, t_xint, t_int },          //010000
    { TMS6_gmpy4, t_s4, t_xu4, t_u4 },              //010001
    { TMS6_avgu4, t_u4, t_xu4, t_u4 },              //010010
    { TMS6_avg2, t_s2, t_xs2, t_s2 },               //010011
    { TMS6_mpyhi, t_int, t_xint, t_sllong },        //010100
    { TMS6_mpyil, t_int, t_xint, t_sllong },        //010101
    { TMS6_mpyspdp, t_sp, t_xdp, t_dp },            //010110
    { TMS6_mpysp2dp, t_sp, t_xsp, t_dp },           //010111
    { TMS6_mpy32u, t_uint, t_xuint, t_duint },      //011000
    { TMS6_mpy32us, t_uint, t_xuint, t_dint },      //011001
    { TMS6_sshvr, t_int, t_xint, t_int },           //011010
    { TMS6_null, t_none, t_none, t_none },          //011011
    { TMS6_sshvl, t_int, t_xint, t_int },           //011100
    { TMS6_rotl, t_uint, t_xuint, t_uint },         //011101
    { TMS6_rotl, t_ucst5, t_xuint, t_uint },        //011110
    { TMS6_null, t_none, t_none, t_none },          //011111
    { TMS6_andn, t_uint, t_xuint, t_uint },         //100000
    { TMS6_null, t_none, t_none, t_none },          //100001
    { TMS6_or, t_uint, t_xuint, t_uint },           //100010
    { TMS6_mv, t_none, t_sint, t_sint },            //100011
    { TMS6_add2, t_i2, t_xi2, t_i2 },               //100100
    { TMS6_sub2, t_i2, t_xi2, t_i2 },               //100101
    { TMS6_and, t_uint, t_xuint, t_uint },          //100110
    { TMS6_and, t_scst5, t_xuint, t_uint },         //100111
    { TMS6_null, t_none, t_none, t_none },          //101000
    { TMS6_null, t_none, t_none, t_none },          //101001
    { TMS6_add, t_sint, t_xsint, t_sint },          //101010
    { TMS6_add, t_scst5, t_xsint, t_sint },         //101011
    { TMS6_sub, t_sint, t_xsint, t_sint },          //101100
    { TMS6_null, t_none, t_none, t_none },          //101101
    { TMS6_xor, t_uint, t_xuint, t_uint },          //101110
    { TMS6_not, t_none, t_xuint, t_uint },          //101111
    { TMS6_sadd2, t_s2, t_xs2, t_s2 },              //110000
    { TMS6_saddsu2, t_u2, t_xs2, t_u2 },            //110001
    { TMS6_spack2, t_int, t_xint, t_s2 },           //110010
    { TMS6_saddu4, t_u4, t_xu4, t_u4 },             //110011
    { TMS6_spacku4, t_s2, t_xs2, t_u4 },            //110100
    { TMS6_sub, t_sint, t_xsint, t_sint },          //110101
    { TMS6_andn, t_uint, t_xuint, t_uint },         //110110
    { TMS6_shr2, t_uint, t_xs2, t_s2 },             //110111
    { TMS6_shru2, t_uint, t_xu2, t_u2 },            //111000
    { TMS6_shlmb, t_u4, t_xu4, t_u4 },              //111001
    { TMS6_shrmb, t_u4, t_xu4, t_u4 },              //111010
    { TMS6_dmv, t_sint, t_xsint, t_dint },          //111011
    { TMS6_min2, t_s2, t_xs2, t_s2 },               //111100
    { TMS6_max2, t_s2, t_xs2, t_s2 },               //111101
    { TMS6_fsubdp, t_dwop1, t_xdwop2, t_dwdst },    //111110
    { TMS6_pack2, t_i2, t_xi2, t_i2 },              //111111
};

/*
* 符合该格式的有Sunit_1, op不包含111100(Sunit15)
*    | 31 29 | 28 | 27 23 | 22 18 | 17 13 | 12 | 11 6 | 5 4 3 2 | 1 0 |
*    |  creg |  z |  dst  |  src2 |  src1 |  x |  op  | 1 0 0 0 | s p |
*    |   3   |  1 |   5   |   5   |   5   |  1 |   6  |         | 1 1 |
*/
static const tmsinsn_t sunit1_ops[64] =
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
  { TMS6_fadddp, t_dwop1,       t_xdwop2,       t_dwdst         }, // 00 1100
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
  { TMS6_dadd,   t_scst5,       t_xdwop2,       t_dwdst         }, // 10 0001
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
  { TMS6_null,   t_none,        t_none,         t_none          }, // 11 1100
  { TMS6_rcpsp,  t_none,        t_xsp,          t_sp            }, // 11 1101
  { TMS6_rsqrsp, t_none,        t_xsp,          t_sp            }, // 11 1110
  { TMS6_clr,    t_uint,        t_xuint,        t_uint          }, // 11 1111
};

/*
* 符合该格式的有Sunit_15
*    | 31 29 | 28 | 27 23 | 22 18 | 17 13 | 12 | 11                 2 | 1 0 |
*    |  creg |  z |  dst  |  src2 |   op  |  x |  1 1 1 1 0 0 1 0 0 0 | s p |
*    |   3   |  1 |   5   |   5   |   5   |  1 |                      | 1 1 |
*/
static const tmsinsn_t sunit15_ops[32] =
{
                                          //BIT 17...13
    { TMS6_abssp,  t_none,  t_xsp, t_sp },      //00000
    { TMS6_intspu, t_none, t_xop, t_dst },      //00001
    { TMS6_unpklu4, t_none, t_xu4, t_u2 },      //00010
    { TMS6_unpkhu4, t_none, t_xu4, t_u2 },      //00011
    { TMS6_spint, t_none, t_xop, t_dst },       //00100
    { TMS6_intsp, t_none, t_xop, t_dst },       //00101
    { TMS6_unpkhu2, t_none, t_xop, t_dwdst },   //00110
    { TMS6_unpkh2, t_none, t_xop, t_dwdst },    //00111
    { TMS6_unpkbu4, t_none, t_xop, t_dwdst },   //01000
    { TMS6_null, t_none, t_none, t_none },      //01001
    { TMS6_null, t_none, t_none, t_none },      //01010
    { TMS6_null, t_none, t_none, t_none },      //01011
    { TMS6_null, t_none, t_none, t_none },      //01100
    { TMS6_null, t_none, t_none, t_none },      //01101
    { TMS6_null, t_none, t_none, t_none },      //01110
    { TMS6_null, t_none, t_none, t_none },      //01111
    { TMS6_dspinth, t_none, t_xdwop, t_dwdst }, //10000
    { TMS6_dintspu, t_none, t_xop, t_dwdst },   //10001
    { TMS6_null, t_none, t_none, t_none },      //10010
    { TMS6_dinthspu,t_none, t_xop, t_dwdst },   //10011
    { TMS6_dspint, t_none, t_xdwop, t_dwdst },  //10100
    { TMS6_dinthsp, t_none, t_xop, t_dwdst },   //10101
    { TMS6_null, t_none, t_none, t_none },      //10110
    { TMS6_null, t_none, t_none, t_none },      //10111
    { TMS6_null, t_none, t_none, t_none },      //11000
    { TMS6_null, t_none, t_none, t_none },      //11001
    { TMS6_null, t_none, t_none, t_none },      //11010
    { TMS6_null, t_none, t_none, t_none },      //11011
    { TMS6_null, t_none, t_none, t_none },      //11100
    { TMS6_null, t_none, t_none, t_none },      //11101
    { TMS6_null, t_none, t_none, t_none },      //11110
    { TMS6_null, t_none, t_none, t_none },      //11111
};

/*
* 符合该格式的有Dunit_1
*    | 31 29 | 28 | 27 23 | 22 18 | 17 13 | 12  7 | 6 5 4 3 2 | 1 0 |
*    |  creg |  z |  dst  |  src2 |  src1 |   op  | 1 0 0 0 0 | s p |
*    |   3   |  1 |   5   |   5   |   5   |   6   |           | 1 1 |
*/
static const tmsinsn_t dunit1_ops[64] =
{                                                               // bits 12..7
  { TMS6_mvk,   t_scst5,        t_none,         t_sint          }, // 00 0000
  { TMS6_null,  t_none,         t_none,         t_none          }, // 00 0001
  { TMS6_null,  t_none,         t_none,         t_none          }, // 00 0010
  { TMS6_null,  t_none,         t_none,         t_none          }, // 00 0011
  { TMS6_null,  t_none,         t_none,         t_none          }, // 00 0100
  { TMS6_null,  t_none,         t_none,         t_none          }, // 00 0101
  { TMS6_null,  t_none,         t_none,         t_none          }, // 00 0110
  { TMS6_null,  t_none,         t_none,         t_none          }, // 00 0111
  { TMS6_null,  t_none,         t_none,         t_none          }, // 00 1000
  { TMS6_null,  t_none,         t_none,         t_none          }, // 00 1001
  { TMS6_null,  t_none,         t_none,         t_none          }, // 00 1010
  { TMS6_null,  t_none,         t_none,         t_none          }, // 00 1011
  { TMS6_null,  t_none,         t_none,         t_none          }, // 00 1100
  { TMS6_null,  t_none,         t_none,         t_none          }, // 00 1101
  { TMS6_null,  t_none,         t_none,         t_none          }, // 00 1110
  { TMS6_null,  t_none,         t_none,         t_none          }, // 00 1111
  { TMS6_add,   t_sint,         t_sint,         t_sint          }, // 01 0000
  { TMS6_sub,   t_sint,         t_sint,         t_sint          }, // 01 0001
  { TMS6_add,   t_ucst5,        t_sint,         t_sint          }, // 01 0010
  { TMS6_sub,   t_ucst5,        t_sint,         t_sint          }, // 01 0011
  { TMS6_null,  t_none,         t_none,         t_none          }, // 01 0100
  { TMS6_null,  t_none,         t_none,         t_none          }, // 01 0101
  { TMS6_null,  t_none,         t_none,         t_none          }, // 01 0110
  { TMS6_null,  t_none,         t_none,         t_none          }, // 01 0111
  { TMS6_null,  t_none,         t_none,         t_none          }, // 01 1000
  { TMS6_null,  t_none,         t_none,         t_none          }, // 01 1001
  { TMS6_null,  t_none,         t_none,         t_none          }, // 01 1010
  { TMS6_null,  t_none,         t_none,         t_none          }, // 01 1011
  { TMS6_null,  t_none,         t_none,         t_none          }, // 01 1100
  { TMS6_null,  t_none,         t_none,         t_none          }, // 01 1101
  { TMS6_null,  t_none,         t_none,         t_none          }, // 01 1110
  { TMS6_null,  t_none,         t_none,         t_none          }, // 01 1111
  { TMS6_null,  t_none,         t_none,         t_none          }, // 10 0000
  { TMS6_null,  t_none,         t_none,         t_none          }, // 10 0001
  { TMS6_null,  t_none,         t_none,         t_none          }, // 10 0010
  { TMS6_null,  t_none,         t_none,         t_none          }, // 10 0011
  { TMS6_null,  t_none,         t_none,         t_none          }, // 10 0100
  { TMS6_null,  t_none,         t_none,         t_none          }, // 10 0101
  { TMS6_null,  t_none,         t_none,         t_none          }, // 10 0110
  { TMS6_null,  t_none,         t_none,         t_none          }, // 10 0111
  { TMS6_null,  t_none,         t_none,         t_none          }, // 10 1000
  { TMS6_null,  t_none,         t_none,         t_none          }, // 10 1001
  { TMS6_null,  t_none,         t_none,         t_none          }, // 10 1010
  { TMS6_null,  t_none,         t_none,         t_none          }, // 10 1011
  { TMS6_null,  t_none,         t_none,         t_none          }, // 10 1100
  { TMS6_null,  t_none,         t_none,         t_none          }, // 10 1101
  { TMS6_null,  t_none,         t_none,         t_none          }, // 10 1110
  { TMS6_null,  t_none,         t_none,         t_none          }, // 10 1111
  { TMS6_addab, t_sint,         t_sint,         t_sint          }, // 11 0000
  { TMS6_subab, t_sint,         t_sint,         t_sint          }, // 11 0001
  { TMS6_addab, t_ucst5,        t_sint,         t_sint          }, // 11 0010
  { TMS6_subab, t_ucst5,        t_sint,         t_sint          }, // 11 0011
  { TMS6_addah, t_sint,         t_sint,         t_sint          }, // 11 0100
  { TMS6_subah, t_sint,         t_sint,         t_sint          }, // 11 0101
  { TMS6_addah, t_ucst5,        t_sint,         t_sint          }, // 11 0110
  { TMS6_subah, t_ucst5,        t_sint,         t_sint          }, // 11 0111
  { TMS6_addaw, t_sint,         t_sint,         t_sint          }, // 11 1000
  { TMS6_subaw, t_sint,         t_sint,         t_sint          }, // 11 1001
  { TMS6_addaw, t_ucst5,        t_sint,         t_sint          }, // 11 1010
  { TMS6_subaw, t_ucst5,        t_sint,         t_sint          }, // 11 1011
  { TMS6_addad, t_sint,         t_sint,         t_sint          }, // 11 1100
  { TMS6_addad, t_ucst5,        t_sint,         t_sint          }, // 11 1101
  { TMS6_null,  t_none,         t_none,         t_none          }, // 11 1110
  { TMS6_null,  t_none,         t_none,         t_none          }, // 11 1111
};

/*
* 符合该格式的有Munit_2
*    | 31 29 | 28 | 27 23 | 22 18 | 17 13 | 12 |  11        6 | 5 4 3 2 | 1 0 |
*    |  creg |  z |  dst  |  src2 |   op  |  x |  0 0 0 0 1 1 | 1 1 0 0 | s p |
*    |   3   |  1 |   5   |   5   |   5   |  1 |              |         | 1 1 |
*/
static const tmsinsn_t muint2_ops[32] =
{
                                            //BIT 17...13
    { TMS6_null, t_none, t_none, t_none },      //00000
    { TMS6_null, t_none, t_none, t_none },      //00001
    { TMS6_null, t_none, t_none, t_none },      //00010
    { TMS6_null, t_none, t_none, t_none },      //00011
    { TMS6_null, t_none, t_none, t_none },      //00100
    { TMS6_null, t_none, t_none, t_none },      //00101
    { TMS6_null, t_none, t_none, t_none },      //00110
    { TMS6_null, t_none, t_none, t_none },      //00111
    { TMS6_null, t_none, t_none, t_none },      //01000
    { TMS6_null, t_none, t_none, t_none },      //01001
    { TMS6_null, t_none, t_none, t_none },      //01010
    { TMS6_null, t_none, t_none, t_none },      //01011
    { TMS6_null, t_none, t_none, t_none },      //01100
    { TMS6_null, t_none, t_none, t_none },      //01101
    { TMS6_null, t_none, t_none, t_none },      //01110
    { TMS6_null, t_none, t_none, t_none },      //01111
    { TMS6_dxpnd4,t_none, t_xop, t_dwdst},      //10000
    { TMS6_dxpnd2,t_none, t_xop, t_dwdst},      //10001
    { TMS6_null, t_none, t_none, t_none },      //10010
    { TMS6_null, t_none, t_none, t_none },      //10011
    { TMS6_null, t_none, t_none, t_none },      //10100
    { TMS6_null, t_none, t_none, t_none },      //10101
    { TMS6_null, t_none, t_none, t_none },      //10110
    { TMS6_null, t_none, t_none, t_none },      //10111
    { TMS6_null, t_none, t_none, t_none },      //11000
    { TMS6_xpnd2, t_none, t_xuint, t_uint },    //11001
    { TMS6_mvd, t_none, t_xint, t_int },        //11010
    { TMS6_null, t_none, t_none, t_none },      //11011
    { TMS6_shfl, t_none, t_xuint, t_uint },     //11100
    { TMS6_deal, t_none, t_xuint, t_uint },     //11101
    { TMS6_bitc4, t_none, t_xu4, t_u4 },        //11110
    { TMS6_bitr, t_none, t_xuint, t_uint },     //11111
};

/*
* 符合该格式的有Munit_4
*    | 31 29 | 28 | 27 23 | 22 18 | 17 13 | 12 |  11  7 | 6 5 4 3 2 | 1 0 |
*    |  creg |  z |  dst  |  src2 |  sr1  |  x |   op   | 0 0 0 0 0 | s p |
*    |   3   |  1 |   5   |   5   |   5   |  1 |    5   |           | 1 1 |
*/
static const tmsinsn_t muint4_ops[32] =
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

/*
* 符合该格式的有Lunit_1、Sunit_2
*    | 31 29 | 28 | 27 23 | 22 18 | 17 13 | 12 |  11  5 | 4 3 2 | 1 0 |
*    |  creg |  z |  dst  |  src2 |  src1 |  x |   op   | 1 1 0 | s p |
*    |   3   |  1 |   5   |   5   |   5   |  1 |    7   |       | 1 1 |
*/
static const tmsinsn_t lsunit_ops[128] =
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
  { TMS6_dadd,   t_scst5,       t_xdwop2,       t_dwdst         }, // 010 0010
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
  { TMS6_faddsp, t_op1,         t_xop2,         t_dst           }, // 011 1100
  { TMS6_fsubsp, t_op1,         t_xop2,         t_dst           }, // 011 1101
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
  { TMS6_fadddp, t_dwop1,       t_xdwop2,       t_dwdst         }, // 101 1000
  { TMS6_fsubdp, t_dwop1,       t_xdwop2,       t_dwdst         }, // 101 1001
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
  { TMS6_dmv,    t_op1,         t_xop2,         t_dwdst         }, // 110 1100
  { TMS6_null,   t_none,        t_none,         t_none          }, // 110 1101
  { TMS6_xor,    t_scst5,       t_xuint,        t_uint          }, // 110 1110
  { TMS6_xor,    t_uint,        t_xuint,        t_uint          }, // 110 1111
  { TMS6_addsp,  t_sp,          t_xsp,          t_sp            }, // 111 0000
  { TMS6_subsp,  t_sp,          t_xsp,          t_sp            }, // 111 0001
  { TMS6_adddp,  t_dp,          t_xdp,          t_dp            }, // 111 0010
  { TMS6_subdp,  t_dp,          t_xdp,          t_dp            }, // 111 0011
  { TMS6_faddsp, t_op1,         t_xop2,         t_dst           }, // 111 0100
  { TMS6_subsp,  t_xsp,         t_sp,           t_sp            }, // 111 0101
  { TMS6_fsubsp, t_op1,         t_xop2,         t_dst           }, // 111 0110
  { TMS6_subdp,  t_xdp,         t_dp,           t_dp            }, // 111 0111
  { TMS6_land,   t_op1,         t_xop2,         t_dst           }, // 111 1000
  { TMS6_landn,  t_op1,         t_xop2,         t_dst           }, // 111 1001
  { TMS6_and,    t_scst5,       t_xuint,        t_uint          }, // 111 1010
  { TMS6_and,    t_uint,        t_xuint,        t_uint          }, // 111 1011
  { TMS6_andn,   t_uint,        t_xuint,        t_uint          }, // 111 1100
  { TMS6_lor,    t_op1,         t_xop2,         t_dst           }, // 111 1101
  { TMS6_or,     t_scst5,       t_xuint,        t_uint          }, // 111 1110
  { TMS6_or,     t_uint,        t_xuint,        t_uint          }, // 111 1111
};

/*
* 符合该格式的有Lunit_2
*    | 31 29 | 28 | 27 23 | 22 18 | 17 13 | 12 |  11           5 | 4 3 2 | 1 0 |
*    |  creg |  z |  dst  |  src2 |   op  |  x |   0 0 1 1 0 1 0 | 1 1 0 | s p |
*    |   3   |  1 |   5   |   5   |   5   |  1 |                 |       | 1 1 |
*/
static const tmsinsn_t lunit2_ops[32] =
{
  { TMS6_abs,    t_none,        t_xsint,        t_sint          }, // 0 0000
  { TMS6_swap4,  t_none,        t_xu4,          t_u4            }, // 0 0001
  { TMS6_unpklu4,t_none,        t_xsint,        t_sint          }, // 0 0010
  { TMS6_unpkhu4,t_none,        t_xsint,        t_sint          }, // 0 0011
  { TMS6_abs2,   t_none,        t_xs2,          t_s2            }, // 0 0100
  { TMS6_mvk,    t_none,        t_scst5,        t_sint          }, // 0 0101
  { TMS6_unpkhu2,t_none,        t_xop,          t_dwdst         }, // 0 0110
  { TMS6_unpkh2, t_none,        t_xop,          t_dwdst         }, // 0 0111
  { TMS6_unpkbu4,t_none,        t_xop,          t_dwdst         }, // 0 1000
  { TMS6_crot90, t_none,        t_xop,          t_dst           }, // 0 1001
  { TMS6_crot270,t_none,        t_xop,          t_dst           }, // 0 1010
  { TMS6_null,   t_none,        t_none,         t_none          }, // 0 1011
  { TMS6_null,   t_none,        t_none,         t_none          }, // 0 1100
  { TMS6_null,   t_none,        t_none,         t_none          }, // 0 1101
  { TMS6_null,   t_none,        t_none,         t_none          }, // 0 1110
  { TMS6_null,   t_none,        t_none,         t_none          }, // 0 1111
  { TMS6_dspinth,t_none,        t_xdwop,        t_dst           }, // 1 0000
  { TMS6_null,   t_none,        t_none,         t_none          }, // 1 0001
  { TMS6_dinthsp,t_none,        t_xop,          t_dwdst         }, // 1 0010
  { TMS6_dinthspu,t_none,       t_xop,          t_dwdst         }, // 1 0011
  { TMS6_dspint, t_none,        t_xdwop,        t_dwdst         }, // 1 0100
  { TMS6_null,   t_none,        t_none,         t_none          }, // 1 0101
  { TMS6_dintspu,t_none,        t_xop,          t_dwdst         }, // 1 0110
  { TMS6_null,   t_none,        t_none,         t_none          }, // 1 0111
  { TMS6_null,   t_none,        t_none,         t_none          }, // 1 1000
  { TMS6_dcrot90,t_none,        t_xdwop,        t_dwdst         }, // 1 1001
  { TMS6_dcrot270,t_none,       t_xdwop,        t_dwdst         }, // 1 1010
  { TMS6_null,   t_none,        t_none,         t_none          }, // 1 1011
  { TMS6_null,   t_none,        t_none,         t_none          }, // 1 1100
  { TMS6_null,   t_none,        t_none,         t_none          }, // 1 1101
  { TMS6_null,   t_none,        t_none,         t_none          }, // 1 1110
  { TMS6_null,   t_none,        t_none,         t_none          }, // 1 1111
};

/*
* 符合该格式的有Dunit_3、Lunit_3、Munit_3、Sunit_12、Sunit_14、Nunit_1
*    | 31   28 | 27                                        2 | 1 0 |
*    | 0 0 0 1 | ........................................... | s p |
*    |         |                                             | 1 1 |
*/
struct tmsinsn_indexed_t
{
    uint16 itype;
    uchar src1;
    uchar src2;
    uchar dst;
    uint32 index;
    uint32 mask;
    funit_t unit;
};
static const tmsinsn_indexed_t nopreds[] =
{                                                  // bits 11..2
  { TMS6_callp,    t_scst21,    t_a3,           t_none,  0x004, 0x01F, FU_S1 },
  { TMS6_rpack2,   t_sint,      t_xsint,        t_s2,    0x3BC, 0x3FF, FU_S1 },
  { TMS6_dadd,     t_dwop1,     t_xdwop2,       t_dwdst, 0x78,  0x3FF, FU_S1 },
  { TMS6_dadd2,    t_dwop1,     t_xdwop2,       t_dwdst, 0x18,  0x3FF, FU_S1 },
  { TMS6_daddsp,   t_dwop1,     t_xdwop2,       t_dwdst, 0x1E6, 0x3FF, FU_S1 },
  { TMS6_dcmpeq2,  t_dwop1,     t_xdwop2,       t_dst,   0x1D8, 0x3FF, FU_S1 },
  { TMS6_dcmpeq4,  t_dwop1,     t_xdwop2,       t_dst,   0x1C8, 0x3FF, FU_S1 },
  { TMS6_dcmpgt2,  t_dwop1,     t_xdwop2,       t_dst,   0x148, 0x3FF, FU_S1 },
  { TMS6_dcmpgtu4, t_dwop1,     t_xdwop2,       t_dst,   0x158, 0x3FF, FU_S1 },
  { TMS6_dmvd,     t_op1,       t_xop2,         t_dwdst, 0x33C, 0x3FF, FU_S1 },
  { TMS6_dpackh2,  t_dwop1,     t_xdwop2,       t_dwdst, 0x98,  0x3FF, FU_S1 },
  { TMS6_dpackhl2, t_dwop1,     t_xdwop2,       t_dwdst, 0x88,  0x3FF, FU_S1 },
  { TMS6_dpackl2,  t_dwop1,     t_xdwop2,       t_dwdst, 0x3FC, 0x3FF, FU_S1 },
  { TMS6_dpacklh2, t_dwop1,     t_xdwop2,       t_dwdst, 0x108, 0x3FF, FU_S1 },
  { TMS6_dsadd,    t_dwop1,     t_xdwop2,       t_dwdst, 0x208, 0x3FF, FU_S1 },
  { TMS6_dsadd2,   t_dwop1,     t_xdwop2,       t_dwdst, 0x30C, 0x3FF, FU_S1 },
  { TMS6_dshl,     t_op2,       t_xdwop1,       t_dwdst, 0x338, 0x3FF, FU_S1 },
  { TMS6_dshl,     t_ucst5,     t_xdwop1,       t_dwdst, 0x328, 0x3FF, FU_S1 },
  { TMS6_dshl2,    t_op2,       t_xdwop1,       t_dwdst, 0x128, 0x3FF, FU_S1 },
  { TMS6_dshl2,    t_ucst5,     t_xdwop1,       t_dwdst, 0x68,  0x3FF, FU_S1 },
  { TMS6_dshr,     t_op2,       t_xdwop1,       t_dwdst, 0x378, 0x3FF, FU_S1 },
  { TMS6_dshr,     t_ucst5,     t_xdwop1,       t_dwdst, 0x368, 0x3FF, FU_S1 },
  { TMS6_dshr2,    t_op2,       t_xdwop1,       t_dwdst, 0x37C, 0x3FF, FU_S1 },
  { TMS6_dshr2,    t_ucst5,     t_xdwop1,       t_dwdst, 0x188, 0x3FF, FU_S1 },
  { TMS6_dshru,    t_op2,       t_xdwop1,       t_dwdst, 0x278, 0x3FF, FU_S1 },
  { TMS6_dshru,    t_ucst5,     t_xdwop1,       t_dwdst, 0x268, 0x3FF, FU_S1 },
  { TMS6_dshru2,   t_op2,       t_xdwop1,       t_dwdst, 0x38C, 0x3FF, FU_S1 },
  { TMS6_dshru2,   t_ucst5,     t_xdwop1,       t_dwdst, 0x198, 0x3FF, FU_S1 },
  { TMS6_dspacku4, t_dwop1,     t_xdwop1,       t_dwdst, 0x34C, 0x3FF, FU_S1 },
  { TMS6_dsub,     t_dwop1,     t_xdwop1,       t_dwdst, 0x178, 0x3FF, FU_S1 },
  { TMS6_dsub2,    t_dwop1,     t_xdwop1,       t_dwdst, 0x118, 0x3FF, FU_S1 },
  { TMS6_dsubsp,   t_dwop1,     t_xdwop1,       t_dwdst, 0x2D8, 0x3FF, FU_S1 },
  { TMS6_shl2,     t_op2,       t_xop1,         t_dst,   0x138, 0x3FF, FU_S1 },
  { TMS6_shl2,     t_ucst5,     t_xop1,         t_dst,   0x38,  0x3FF, FU_S1 },
  { TMS6_or,       t_dwop1,     t_xdwop2,       t_dwdst, 0x1B8, 0x3FF, FU_S1 },
  { TMS6_and,      t_dwop1,     t_xdwop2,       t_dwdst, 0x1F8, 0x3FF, FU_S1 },
  { TMS6_xor,      t_dwop1,     t_xdwop2,       t_dwdst, 0xB8,  0x3FF, FU_S1 },
  { TMS6_xor,      t_scst5,     t_xdwop2,       t_dwdst, 0xA8,  0x3FF, FU_S1 },

  { TMS6_addab,    t_b14,       t_ucst15,       t_uint,  0x00F, 0x01F, FU_D1 },
  { TMS6_addad,    t_b14,       t_ucst15,       t_uint,  0x010, 0x01F, FU_D1 },
  { TMS6_addah,    t_b14,       t_ucst15,       t_uint,  0x017, 0x01F, FU_D1 },
  { TMS6_addaw,    t_b14,       t_ucst15,       t_uint,  0x01F, 0x01F, FU_D1 },

  { TMS6_addsub,   t_sint,      t_xsint,        t_dint,  0x066, 0x3FF, FU_L1 },
  { TMS6_saddsub,  t_sint,      t_xsint,        t_dint,  0x076, 0x3FF, FU_L1 },
  { TMS6_dpack2,   t_sint,      t_xsint,        t_dint,  0x1A6, 0x3FF, FU_L1 },
  { TMS6_shfl3,    t_sint,      t_xsint,        t_dint,  0x1B6, 0x3FF, FU_L1 },
  { TMS6_addsub2,  t_sint,      t_xsint,        t_dint,  0x06E, 0x3FF, FU_L1 },
  { TMS6_saddsub2, t_sint,      t_xsint,        t_dint,  0x07E, 0x3FF, FU_L1 },
  { TMS6_dpackx2,  t_sint,      t_xsint,        t_dint,  0x19E, 0x3FF, FU_L1 },
  { TMS6_dadd,     t_dwop1,     t_xdwop2,       t_dwdst, 0x1E,  0x3FF, FU_L1 },
  { TMS6_dadd,     t_dwop1,     t_xdwop2,       t_dwdst, 0x2E,  0x3FF, FU_L1 },
  { TMS6_daddsp,   t_dwop1,     t_xdwop2,       t_dwdst, 0x2C8, 0x3FF, FU_L1 },
  { TMS6_dapys2,   t_dwop1,     t_xdwop2,       t_dwdst, 0x1C6, 0x3FF, FU_L1 },
  { TMS6_dmax2,    t_dwop1,     t_xdwop2,       t_dwdst, 0x216, 0x3FF, FU_L1 },
  { TMS6_dmaxu4,   t_dwop1,     t_xdwop2,       t_dwdst, 0x21E, 0x3FF, FU_L1 },
  { TMS6_dmin2,    t_dwop1,     t_xdwop2,       t_dwdst, 0x20E, 0x3FF, FU_L1 },
  { TMS6_dminu4,   t_dwop1,     t_xdwop2,       t_dwdst, 0x246, 0x3FF, FU_L1 },
  { TMS6_dmvd,     t_op1,       t_xop2,         t_dwdst, 0x366, 0x3FF, FU_L1 },
  { TMS6_dpackh2,  t_dwop1,     t_xdwop2,       t_dwdst, 0xF6,  0x3FF, FU_L1 },
  { TMS6_dpackh4,  t_dwop1,     t_xdwop2,       t_dwdst, 0x34E, 0x3FF, FU_L1 },
  { TMS6_dpackhl2, t_dwop1,     t_xdwop2,       t_dwdst, 0xE6,  0x3FF, FU_L1 },
  { TMS6_dpackl2,  t_dwop1,     t_xdwop2,       t_dwdst, 0x6,   0x3FF, FU_L1 },
  { TMS6_dpackl4,  t_dwop1,     t_xdwop2,       t_dwdst, 0x346, 0x3FF, FU_L1 },
  { TMS6_dpacklh2, t_dwop1,     t_xdwop2,       t_dwdst, 0xDE,  0x3FF, FU_L1 },
  { TMS6_dpacklh4, t_op1,       t_xop2,         t_dst,   0x356, 0x3FF, FU_L1 },
  { TMS6_dsadd,    t_dwop1,     t_xdwop2,       t_dwdst, 0x9E,  0x3FF, FU_L1 },
  { TMS6_dsadd2,   t_dwop1,     t_xdwop2,       t_dwdst, 0xA4,  0x3FF, FU_L1 },
  { TMS6_dssub,    t_dwop1,     t_xdwop2,       t_dwdst, 0x33E, 0x3FF, FU_L1 },
  { TMS6_dssub2,   t_dwop1,     t_xdwop2,       t_dwdst, 0x326, 0x3FF, FU_L1 },
  { TMS6_dsub,     t_dwop1,     t_xdwop2,       t_dwdst, 0x3E,  0x3FF, FU_L1 },
  { TMS6_dsub2,    t_dwop1,     t_xdwop2,       t_dwdst, 0x26,  0x3FF, FU_L1 },
  { TMS6_dsubsp,   t_dwop1,     t_xdwop2,       t_dwdst, 0x1EE, 0x3FF, FU_L1 },
  { TMS6_or,       t_dwop1,     t_xdwop2,       t_dwdst, 0x3FE, 0x3FF, FU_L1 },
  { TMS6_and,      t_dwop1,     t_xdwop2,       t_dwdst, 0x3DE, 0x3FF, FU_L1 },
  { TMS6_xor,      t_dwop1,     t_xdwop2,       t_dwdst, 0x37E, 0x3FF, FU_L1 },
  { TMS6_xor,      t_scst5,     t_xdwop2,       t_dwdst, 0x376, 0x3FF, FU_L1 },

  { TMS6_cmpy,     t_s2,        t_xs2,          t_dint,  0x0AC, 0x3FF, FU_M1 },
  { TMS6_cmpyr,    t_s2,        t_xs2,          t_s2,    0x0BC, 0x3FF, FU_M1 },
  { TMS6_cmpyr1,   t_s2,        t_xs2,          t_s2,    0x0CC, 0x3FF, FU_M1 },
  { TMS6_mpy2ir,   t_sint,      t_xsint,        t_dint,  0x0FC, 0x3FF, FU_M1 },
  { TMS6_ddotpl2r, t_dint,      t_xs2,          t_s2,    0x14C, 0x3FF, FU_M1 },
  { TMS6_ddotph2r, t_dint,      t_xs2,          t_s2,    0x15C, 0x3FF, FU_M1 },
  { TMS6_ddotpl2,  t_dint,      t_xs2,          t_dint,  0x16C, 0x3FF, FU_M1 },
  { TMS6_ddotph2,  t_dint,      t_xs2,          t_dint,  0x17C, 0x3FF, FU_M1 },
  { TMS6_ddotp4,   t_ds2,       t_xs2,          t_dint,  0x18C, 0x3FF, FU_M1 },
  { TMS6_smpy32,   t_sint,      t_xsint,        t_sint,  0x19C, 0x3FF, FU_M1 },
  { TMS6_xormpy,   t_uint,      t_xuint,        t_uint,  0x1BC, 0x3FF, FU_M1 },
  { TMS6_gmpy,     t_uint,      t_xuint,        t_uint,  0x1FC, 0x3FF, FU_M1 },
  { TMS6_ccmatmpy, t_qwop2,     t_xdwop1,       t_qwdst, 0xA0,  0x3FF, FU_M1 },
  { TMS6_ccmatmpyr1,t_qwop2,    t_xdwop1,       t_qwdst, 0xE0,  0x3FF, FU_M1 },
  { TMS6_ccmpy32r1,t_dwop1,     t_xdwop2,       t_dwdst, 0x120, 0x3FF, FU_M1 },
  { TMS6_cmatmpy,  t_qwop2,     t_xdwop1,       t_qwdst, 0x80,  0x3FF, FU_M1 },
  { TMS6_cmatmpyr1,t_qwop2,     t_xdwop1,       t_qwdst, 0xC0,  0x3FF, FU_M1 },
  { TMS6_cmpy32r1, t_dwop1,     t_xdwop2,       t_dwdst, 0x1000,0x3FF, FU_M1 },
  { TMS6_cmpysp,   t_dwop1,     t_xdwop2,       t_qwdst, 0x3C0, 0x3FF, FU_M1 },
  { TMS6_davg2,    t_dwop1,     t_xdwop2,       t_dwdst, 0x13C, 0x3FF, FU_M1 },
  { TMS6_davgnr2,  t_dwop1,     t_xdwop2,       t_dwdst, 0x11C, 0x3FF, FU_M1 },
  { TMS6_davgnru4, t_dwop1,     t_xdwop2,       t_dwdst, 0x10C, 0x3FF, FU_M1 },
  { TMS6_davgu4,   t_dwop1,     t_xdwop2,       t_dwdst, 0x12C, 0x3FF, FU_M1 },
  { TMS6_dccmpy,   t_dwop1,     t_xdwop2,       t_dwdst, 0x3C,  0x3FF, FU_M1 },
  { TMS6_dccmpyr1, t_dwop1,     t_xdwop2,       t_dwdst, 0xEC,  0x3FF, FU_M1 },
  { TMS6_dcmpy,    t_dwop1,     t_xdwop2,       t_dwdst, 0x2C,  0x3FF, FU_M1 },
  { TMS6_dcmpyr1,  t_dwop1,     t_xdwop2,       t_dwdst, 0xDC,  0x3FF, FU_M1 },
  { TMS6_ddotp4h,  t_dwop1,     t_xdwop2,       t_dwdst, 0x8C,  0x3FF, FU_M1 },
  { TMS6_ddotpsu4h,t_dwop1,     t_xdwop2,       t_dwdst, 0x9C,  0x3FF, FU_M1 },
  { TMS6_dmpy2,    t_dwop1,     t_xdwop2,       t_qwdst, 0xC,   0x3FF, FU_M1 },
  { TMS6_dmpysp,   t_dwop1,     t_xdwop2,       t_dwdst, 0x380, 0x3FF, FU_M1 },
  { TMS6_dmpysu4,  t_dwop1,     t_xdwop2,       t_qwdst, 0x18C, 0x3FF, FU_M1 },
  { TMS6_dmpyu2,   t_dwop1,     t_xdwop2,       t_qwdst, 0x20,  0x3FF, FU_M1 },
  { TMS6_dmpyu4,   t_dwop1,     t_xdwop2,       t_qwdst, 0x4C,  0x3FF, FU_M1 },
  { TMS6_dsmpy2,   t_dwop1,     t_xdwop2,       t_qwdst, 0x1C,  0x3FF, FU_M1 },
  { TMS6_qmpy32,   t_qwop1,     t_qwop2,        t_qwdst, 0x200, 0x3FF, FU_M1 },
  { TMS6_qmpysp,   t_qwop1,     t_qwop2,        t_qwdst, 0x3A0, 0x3FF, FU_M1 },
  { TMS6_qsmpy32r1,t_qwop1,     t_qwop2,        t_qwdst, 0x1AC, 0x3FF, FU_M1 },

  { TMS6_swe,      t_none,      t_none,         t_none,  0x0000000, 0x3FFFFFF, FU_NONE },
  { TMS6_dint,     t_none,      t_none,         t_none,  0x0001000, 0x3FFFFFF, FU_NONE },
  { TMS6_swenr,    t_none,      t_none,         t_none,  0x0000800, 0x3FFFFFF, FU_NONE },
  { TMS6_rint,     t_none,      t_none,         t_none,  0x0001800, 0x3FFFFFF, FU_NONE },
  { TMS6_mfence,   t_none,      t_none,         t_none,  0x0002000, 0xFFFF,    FU_NONE },
};

// 0 0 0 1开头的指令使用该函数解析
static int nopred_ops(insn_t* insn, int ctype, uint32_t code, fetch_packet_t* fp)
{
    int i;
    int op = bits_ucst(code, 2, 10);    //bits 11...2
    bool other = false;
    const tmsinsn_indexed_t* p = nopreds;

    for (i = 0; i < qnumber(nopreds); i++, p++)
    {
        if (p->index == (op & p->mask))
        {
            //只有这些指令有x位
            if (p->unit == FU_M1 || p->unit == FU_L1 || p->itype == TMS6_rpack2)
                other = bits_check(code, 12);
            int size = table_insns(*insn, code, (tmsinsn_t*)p, other, fp, false);
            return size;
        }
    }
    return 0;
}


struct tms_ldinfo_t
{
    uchar itype;
    uchar dtype;
    uchar shift;
};

static const tms_ldinfo_t ldinfo[] =
{
  { TMS6_ldhu,  dt_word,  1 },  // 0000
  { TMS6_ldbu,  dt_byte,  0 },  // 0001
  { TMS6_ldb,   dt_byte,  0 },  // 0010
  { TMS6_stb,   dt_byte,  0 },  // 0011
  { TMS6_ldh,   dt_word,  1 },  // 0100
  { TMS6_sth,   dt_word,  1 },  // 0101
  { TMS6_ldw,   dt_dword, 2 },  // 0110
  { TMS6_stw,   dt_dword, 2 },  // 0111
  { TMS6_null,  0,        0 },  // 1000
  { TMS6_null,  0,        0 },  // 1001
  { TMS6_ldndw, dt_qword, 3 },  // 1010
  { TMS6_ldnw,  dt_dword, 2 },  // 1011
  { TMS6_stdw,  dt_qword, 3 },  // 1100
  { TMS6_stnw,  dt_dword, 2 },  // 1101
  { TMS6_lddw,  dt_qword, 3 },  // 1110
  { TMS6_stndw, dt_qword, 3 },  // 1111
};
static int load_store_ops(insn_t* insn, int ctype, uint32_t code, fetch_packet_t* fp)
{
    bool is_displ = false;
    int baseR, offsetR;
    int idx = bits_ucst(code, 4, 3);
    if (ctype != Dunit_5)
        idx |= bits_ucst(code, 8, 1, 3);    //add r bit
        
    const tms_ldinfo_t* ld = &ldinfo[idx];
    if (ld->itype == TMS6_null)
        return 0;

    insn->itype = ld->itype;

    if (ctype == Dunit_5)
    {
        baseR = rA15;
        offsetR = bits_ucst(code, 8, 15);
        //offsetR = bits_ucst(code, 8, 15) << ld->shift;
        insn->Op1.mode = MO_ADD_REG;
        is_displ = true;
    }
    else
    {
        baseR = bits_ucst(code, 18, 5);
        offsetR = bits_ucst(code, 13, 5);
        insn->Op1.mode = bits_ucst(code, 9, 4);
        if (insn->Op1.mode == MO_SUB_UCST || insn->Op1.mode == MO_ADD_UCST ||
            insn->Op1.mode == MO_SUBSUB_UCST || insn->Op1.mode == MO_ADDADD_UCST ||
            insn->Op1.mode == MO_UCST_SUBSUB || insn->Op1.mode == MO_UCST_ADDADD)
            is_displ = true;
            
        //todo: if <<=, should set offset flag
        //if(is_displ)
        //    offsetR <<= ld->shift;
    }

    if (is_displ)
        make_displ(&insn->Op1, baseR, offsetR, bits_check(code, 7));
    else
        make_phrase(&insn->Op1, baseR, offsetR, bits_check(code, 7), bits_check(code, 7));
    insn->Op1.dtype = ld->dtype;

    if (ld->shift == 3)
        make_regpair(&insn->Op2, bits_ucst(code, 23, 5), bits_check(code, 1), false);
    else
        make_reg(&insn->Op2, bits_ucst(code, 23, 5), bits_check(code, 1));

    if (insn->itype == TMS6_stb || insn->itype == TMS6_sth ||
        insn->itype == TMS6_stw || insn->itype == TMS6_stdw ||
        insn->itype == TMS6_stnw || insn->itype == TMS6_stndw)
        swap_op1_and_op2(insn);

    //跟compact ins不一样，由y bit控制unit，由s bit控制t路径
    insn->cflags |= aux_ldst;
    if (bits_check(code, 1))
        insn->cflags |= aux_t2;
    insn->funit = bits_check(code, 7) ? FU_D2 : FU_D1;  //y bit
    return insn->size;
}

static int d_unit_ins(insn_t* insn, int ctype, uint32_t code, fetch_packet_t* fp)
{
    char op;
    const tmsinsn_t* table = NULL;

    insn->funit = bits_check(code, 1) ? FU_D2 : FU_D1;
    switch (ctype)
    {
    case Dunit_1:
        op = bits_ucst(code, 7, 6);
        table = &dunit1_ops[op];
        return table_insns(*insn, code, table, false, fp, true);
    case Dunit_2:
        op = bits_ucst(code, 6, 6);
        table = &dms_ops[op];
        return table_insns(*insn, code, table, bits_check(code, 12), fp, false);
    case Dunit_3:
        return nopred_ops(insn, ctype, code, fp);
    case Dunit_4:
    case Dunit_5:
    case Dunit_6:
    case Dunit_7:
        //load store ins
        return load_store_ops(insn, ctype, code, fp);
    }
    return 0;
}

static int l_unit_ins(insn_t* insn, int ctype, uint32_t code, fetch_packet_t* fp)
{
    bool src2_first = false;
	char op;
	const tmsinsn_t *table = NULL;

    insn->funit = bits_check(code, 1) ? FU_L2 : FU_L1;
    switch (ctype)
    {
    case Lunit_1:
        op = bits_ucst(code, 5, 7);
        table = &lsunit_ops[op];
        break;
    case Lunit_3:
        return nopred_ops(insn, ctype, code, fp);
    case Lunit_2:
        op = bits_ucst(code, 13, 5);
        table = &lunit2_ops[op];
        break;
    default:
        return 0;
    }

    return table_insns(*insn, code, table, bits_check(code, 12), fp, src2_first);
}

static int m_unit_ins(insn_t* insn, int ctype, uint32_t code, fetch_packet_t* fp)
{
    bool src2_first = false;
    char op;
    const tmsinsn_t* table = NULL;

    insn->funit = bits_check(code, 1) ? FU_M2 : FU_M1;
    switch (ctype)
    {
    case Munit_1:
        op = bits_ucst(code, 6, 6);
        table = &dms_ops[op];
        break;
    case Munit_2:
        op = bits_ucst(code, 13, 5);
        table = &muint2_ops[op];
        break;
    case Munit_3:
        return nopred_ops(insn, ctype, code, fp);
    case Munit_4:
        op = bits_ucst(code, 7, 5);
        table = &muint4_ops[op];
        break;
    default:
        return 0;
    }
    return table_insns(*insn, code, table, bits_check(code, 12), fp, src2_first);
}

static int s_unit_ins(insn_t* insn, int ctype, uint32_t code, fetch_packet_t* fp)
{
    bool src2_first = false;
    char op;
    const tmsinsn_t* table = NULL;
    static const int Sunit_16_tbl[4] = { TMS6_extu, TMS6_ext, TMS6_set, TMS6_clr };

    //unit set
    insn->funit = bits_check(code, 1) ? FU_S2 : FU_S1;
    switch (ctype)
    {
    case Sunit_1:
        op = bits_ucst(code, 6, 6);
        table = &sunit1_ops[op];
        src2_first = true;
        break;
    case Sunit_2:
        op = bits_ucst(code, 5, 7); //addsp...格式与L单元相近
        table = &lsunit_ops[op];
        break;
    case Sunit_3:
        insn->itype = TMS6_addk;
        make_imm(&insn->Op1, bits_ucst(code, 7, 16));
        make_reg(&insn->Op2, bits_ucst(code, 23, 5), bits_check(code, 1));
        return insn->size;
    case Sunit_4:
        op = bits_ucst(code, 6, 6);
        table = &sunit1_ops[op];
        if (table_insns(*insn, code, table, bits_check(code, 12), fp, false))
        {
            swap_op2_and_op3(insn); //addkpc src1, dst, src2
            return insn->size;
        }
        return 0;
        //insn->funit = FU_S2;
        //insn->itype = TMS6_addkpc;
        //make_imm(&insn->Op1, bits_scst(code, 16, 7));
        //make_imm(&insn->Op2, bits_ucst(code, 13, 3));
        //make_reg(&insn->Op3, bits_ucst(code, 23, 5), true);
        //return insn->size;
    case Sunit_5:
        op = bits_ucst(code, 6, 6);
        table = &dms_ops[op];
        break;
    case Sunit_6:
        insn->itype = TMS6_b;
        make_near(&insn->Op1, fp->start, bits_scst(code, 7, 21), 2);
        return insn->size;
    case Sunit_7:
        insn->itype = TMS6_b;
        insn->funit = FU_S2;
        make_reg(&insn->Op1, bits_ucst(code, 18, 5), bits_ucst(code, 12, 1) != bits_ucst(code, 1, 1));
        if (bits_check(code, 12))
            insn->cflags |= aux_xp;
        return insn->size;
    case Sunit_8:
        insn->itype = bits_check(code, 18) ? TMS6_bnrp: TMS6_birp;
        insn->funit = FU_S2;
        return insn->size;
    case Sunit_9:
        insn->itype = bits_check(code, 12) ? TMS6_bdec : TMS6_bpos;
        make_near(&insn->Op1, fp->start, bits_scst(code, 13, 10), 2);
        make_reg(&insn->Op2, bits_ucst(code, 23, 5), bits_check(code, 1));
        return insn->size;
    case Sunit_10:
        insn->itype = TMS6_bnop;
        make_near(&insn->Op1, fp->start, bits_scst(code, 16, 12), fp->fph_vaild ? 1 : 2);
        make_imm(&insn->Op2, bits_ucst(code, 13, 3));
        return insn->size;
    case Sunit_11:
        insn->itype = TMS6_bnop;
        insn->funit = FU_S2;
        make_reg(&insn->Op1, bits_ucst(code, 18, 5), bits_ucst(code, 12, 1) != bits_ucst(code, 1, 1));
        make_imm(&insn->Op2, bits_ucst(code, 13, 3));
        if (bits_check(code, 12))
            insn->cflags |= aux_xp;
        return insn->size;
    case Sunit_12:
        insn->itype = TMS6_callp;
        make_near(&insn->Op1, fp->start, bits_scst(code, 7, 21), 2);
        return insn->size;
    case Sunit_13:
        insn->itype = bits_check(code, 6) ? TMS6_mvkh : TMS6_mvk;
        make_imm(&insn->Op1, bits_scst(code, 7, 16));
        make_reg(&insn->Op2, bits_ucst(code, 23, 5), bits_check(code, 1));
        return insn->size;
    case Sunit_14:
        return nopred_ops(insn, ctype, code, fp);
    case Sunit_15:
        op = bits_ucst(code, 13, 5);
        table = &sunit15_ops[op];
        break;
    case Sunit_16:
        op = bits_ucst(code, 6, 2);
        insn->itype = Sunit_16_tbl[op];
        make_imm(&insn->Op1, bits_ucst(code, 13, 5));
        make_imm(&insn->Op2, bits_ucst(code, 8, 5));
        make_reg(&insn->Op3, bits_ucst(code, 23, 5), bits_check(code, 1));
        make_reg(&insn->Op4, bits_ucst(code, 18, 5), bits_check(code, 1));
        insn->Op1.src2 = insn->Op4.reg;
        insn->cflags |= aux_src2;
        return insn->size;
    default:
        return 0;
    }
    return table_insns(*insn, code, table, bits_check(code, 12), fp, src2_first);
}

static int n_unit_ins(insn_t* insn, int ctype, uint32_t code, fetch_packet_t* fp)
{
	char op = bits_ucst(code, 13, 4);

	if (ctype == Nunit_3 || ctype == Nunit_4)
	{
        if(op == 12 || op == 13 || op == 14)    //sploop ins
            make_imm(&insn->Op1, bits_ucst(code, 23, 5) + 1);
		switch (op)
		{
		case 8://spmask-op is 1000, error in doc
		case 9:
			make_spmask(&insn->Op1, bits_ucst(code, 18, 8));
			insn->itype = op == 8 ? TMS6_spmask : TMS6_spmaskr;
            break;
		case 10:
			make_stgcyc(&insn->Op1, bits_ucst(code, 22, 6));
			insn->itype = TMS6_spkernel;
            break;
		case 11:   
            insn->itype = TMS6_spkernelr;
            break;
        case 12:   
            insn->itype = TMS6_sploop;
            break;
        case 13:   
            insn->itype = TMS6_sploopd;
            break;
        case 14:   
            insn->itype = TMS6_sploopw;
            break;
		}
        return insn->size;
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
        return nopred_ops(insn, ctype, code, fp);
    }
	return 0;
}

static void printf_insn(insn_t* insn)
{
    msg("******************\n");
    msg("ea: %X\n", insn->ea);
    msg("size: %X\n", insn->size);
    msg("type: %X\n", insn->itype);
    msg("unit: %d\n", insn->funit);
    msg("cond: %X\n", insn->cond);
    int op_idx = 0;
    while (1)
    {
        if (insn->ops[op_idx].type == o_void)
            break;
        msg("op_%d: type=%d  ", op_idx, insn->ops[op_idx].type);
        switch (insn->ops[op_idx].type)
        {
            case o_reg:
            case o_regpair:
            case o_regqpair:
                msg("reg=%d\n", insn->ops[op_idx].reg);
                break;
            case o_imm:
                msg("imm=%d\n", insn->ops[op_idx].value);
                break;
            case o_displ:
                msg("base=%d  offest=%d\n", insn->ops[op_idx].reg, insn->ops[op_idx].addr);
                break;
            case o_phrase:
                msg("base=%d  offest=%d\n", insn->ops[op_idx].reg, insn->ops[op_idx].secreg);
                break;
            case o_near:
                msg("near=%d\n", insn->ops[op_idx].addr);
                break;
            default:
                msg("\n");
        }
        op_idx++;
    }
    msg("******************\n");
}

static void check_cross_path(insn_t* insn)
{
    switch (insn->itype)
    {
    case TMS6_mvc:
    case TMS6_bpos:
    case TMS6_bdec:
    case TMS6_extu:
    case TMS6_ext:
    case TMS6_set:
    case TMS6_clr:
        insn->cflags &= ~aux_xp;
    }
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

    //check some ins no cross path flag
    check_cross_path(insn);

    if (ret > 0 && bits_check(code, 0))
        insn->cflags |= aux_para;   //parallel set
    //msg("%X: ctype:%d  ret:%d\n", insn->ea, ins_ctype, ret);
    //printf_insn(insn);
    return ret;
}