#include "tms66x.h"
#include <bytes.hpp>
#include "ins.hpp"
#include "fetch_packet.h"
#include "value_filter.h"
#include "bits.h"

#define D_CPT_START 0
#define D_CPT_END 13
#define L_CPT_START 14
#define L_CPT_END 21
#define M_CPT_START 22
#define M_CPT_END 22
#define S_CPT_START 23
#define S_CPT_END 38
#define DLS_CPT_START 39
#define DLS_CPT_END 42
#define N_CPT_START 43
#define N_CPT_END 48
enum TMSC66X_CPT_INS_FMT :uint8
{
	//D unit    0-13
	TMSC6L_Doff4 = 0,
	TMSC6L_Doff4DW,
	TMSC6L_Dind,
	TMSC6L_DindDw,
	TMSC6L_Dinc,
	TMSC6L_DincDw,
	TMSC6L_Ddec,
	TMSC6L_DdecDw,
	TMSC6L_Dstk,
	TMSC6L_Dx2op,
	TMSC6L_Dx5,
	TMSC6L_Dx5p,
	TMSC6L_Dx1,
	TMSC6L_Dpp,
	//L unit    14-21
	TMSC6L_L3,
	TMSC6L_L3i,
	TMSC6L_Ltbd,
	TMSC6L_L2c,
	TMSC6L_Lx5,
	TMSC6L_Lx3c,
	TMSC6L_Lx1c,
	TMSC6L_Lx1,
	//M unit    22
	TMSC6L_M3,
	//S unit    23-38
	TMSC6L_Sbs7,
	TMSC6L_Sbu8,
	TMSC6L_Scs10,
	TMSC6L_Sbs7c,
	TMSC6L_Sbu8c,
	TMSC6L_S3,
	TMSC6L_S3i,
	TMSC6L_Smvk8,
	TMSC6L_Ssh5,
	TMSC6L_S2sh,
	TMSC6L_Sc5,
	TMSC6L_S2ext,
	TMSC6L_Sx2op,
	TMSC6L_Sx5,
	TMSC6L_Sx1,
	TMSC6L_Sx1b,
	//DLS unit  39-42
	TMSC6L_LSDmvto,
	TMSC6L_LSDmvfr,
	TMSC6L_LSDx1c,
	TMSC6L_LSDx1,
	//No unit   43-48
	TMSC6L_Uspl,
	TMSC6L_Uspldr,
	TMSC6L_Uspk,
	TMSC6L_Uspm_spmask,
	TMSC6L_Uspm_spmaskr,
	TMSC6L_Unop
};

/*
	check unique result
	**************************
	012: ........110...111011. Dx1
	042: ........110...11..11. DLSx1
	**************************
	021: ........110...110011. Lx1
	042: ........110...11..11. DLSx1
	**************************
	037: ........110...110111. Sx1
	042: ........110...11..11. DLSx1
*/
static filter_table_t g_tms66x_cpt_ins[] =
{
	//dsz br sat | ins
	// 3  1   1  |  16
	//D unit 0-13
	{ "0.........00......10.", TMSC6L_Doff4 },  	//0
	{ "..........01......10.", TMSC6L_Doff4 },  	//0
	{ "1.........00......10.", TMSC6L_Doff4DW },    //1
	{ "0........010......10.", TMSC6L_Dind },       //2
	{ ".........011......10.", TMSC6L_Dind },       //2
	{ "1........010......10.", TMSC6L_DindDw },     //3
	{ "0....00..110......10.", TMSC6L_Dinc },       //4
	{ ".....00..111......10.", TMSC6L_Dinc },       //4
	{ "1....00..110......10.", TMSC6L_DincDw },     //5
	{ "0....01..110......10.", TMSC6L_Ddec },     	//6
	{ ".....01..111......10.", TMSC6L_Ddec },     	//6
	{ "1....01..110......10.", TMSC6L_DdecDw },     //7
	{ ".....1...11.......10.", TMSC6L_Dstk },       //8
	{ "..........0...011011.", TMSC6L_Dx2op },      //9
	{ "..........1...011011.", TMSC6L_Dx5 },        //10
	{ "........011...1110111", TMSC6L_Dx5p },       //11
	{ "........110...111011.", TMSC6L_Dx1 },        //12
	{ ".........0....1110111", TMSC6L_Dpp },        //13
	//L unit 14-21
	{ "..........0......000.", TMSC6L_L3 },         //14
	{ "..........1......000.", TMSC6L_L3i },        //15
	{ "..........0......100.", TMSC6L_Ltbd },       //16
	{ "..........1......100.", TMSC6L_L2c },        //17
	{ "..........1...010011.", TMSC6L_Lx5 },        //18
	{ "........0.0...010011.", TMSC6L_Lx3c },       //19
	{ "........1.0...010011.", TMSC6L_Lx1c },       //20
	{ "........110...110011.", TMSC6L_Lx1 },        //21
	//M unit 22
	{ "................1111.", TMSC6L_M3 },       	//22
	//S unit 23-38
	{ "...1.10........00101.", TMSC6L_Sbs7 },       //23
	{ "...1.0.........00101.", TMSC6L_Sbs7 },       //23
	{ "...1.11........00101.", TMSC6L_Sbu8 },		//24
	{ "...1...........01101.", TMSC6L_Scs10 },      //25
	{ "...1.10........1.101.", TMSC6L_Sbs7c },      //26
	{ "...1.0.........1.101.", TMSC6L_Sbs7c },      //26
	{ "...1.11........1.101.", TMSC6L_Sbu8c },      //27
	{ "...0......0......101.", TMSC6L_S3 },         //28
	{ "...0......1......101.", TMSC6L_S3i },		//29
	{ "................1001.", TMSC6L_Smvk8 },      //30
	{ "..........1...100001.", TMSC6L_Ssh5 },       //31
	{ "..........1...0.0001.", TMSC6L_Ssh5 },       //31
	{ "..........1...110001.", TMSC6L_S2sh },       //32
	{ "..........0...100001.", TMSC6L_Sc5 },        //33
	{ "..........0...0.0001.", TMSC6L_Sc5 },        //33
	{ "..........0...110001.", TMSC6L_S2ext },		//34
	{ "..........0...010111.", TMSC6L_Sx2op },      //35
	{ "..........1...010111.", TMSC6L_Sx5 },        //36
	{ "........110...110111.", TMSC6L_Sx1 },        //37
	{ "........00....110111.", TMSC6L_Sx1b },       //38
	//DLS unit 39-42
	{ "..............001011.", TMSC6L_LSDmvto },    //39
	{ "..............000.11.", TMSC6L_LSDmvto },    //39
	{ "..............101011.", TMSC6L_LSDmvfr },    //40
	{ "..............100.11.", TMSC6L_LSDmvfr },    //40
	{ "........010...111011.", TMSC6L_LSDx1c },     //41
	{ "........010...110.11.", TMSC6L_LSDx1c },     //41
	{ "........110...111011.", TMSC6L_LSDx1 },      //42
	{ "........110...110.11.", TMSC6L_LSDx1 },      //42
	//No unit 43-48
	{ ".....0.0011...110011.", TMSC6L_Uspl },       //43
	{ ".....1.0011...110011.", TMSC6L_Uspldr },     //44
	{ ".......0111...110011.", TMSC6L_Uspk },       //45
	{ ".......1011...110011.", TMSC6L_Uspm_spmask },//46
	{ ".......1111...110011.", TMSC6L_Uspm_spmaskr },//47
	{ "........0110001101110", TMSC6L_Unop}         //48
};

static void make_reg(op_t* op, int32 v, bool is_other, bool is_high)
{
	if (is_other)
		v += rB0;
	if (is_high)
		v += 16;
	op->type = o_reg;
	op->reg = v;
	op->dtype = dt_dword;
}

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

static void make_near(op_t* op, uint32_t fp_start, int32 v, int shift = 1)
{
	op->type = o_near;
	op->dtype = dt_code;
	op->addr = fp_start + (v << shift);
}

static int get_reg(int32 v, bool is_other, bool is_high)
{
	op_t op;
	make_reg(&op, v, is_other, is_high);
	return op.reg;
}

static void make_displ(op_t* op, int32 base, int32 offset, bool is_other, bool is_high)
{
	op->type = o_displ;
	op->reg = get_reg(base, is_other, is_high);
	op->addr = offset;
}

static void make_phrase(op_t* op, int32 base, int32 offset, bool is_other_b, bool is_other_o, bool is_high)
{
	make_reg(op, offset, is_other_o, is_high);
	op->secreg = op->reg;
	make_reg(op, base, is_other_b, is_high);
	op->type = o_phrase;
}

static int get_unit_type(char unit, char s)
{
	switch (unit)
	{
	case 0:
		return s ? FU_L2 : FU_L1;
	case 1:
		return s ? FU_S2 : FU_S1;
	case 2:
		return s ? FU_D2 : FU_D1;
	}
	return FU_NONE;
}

//指令默认为 ins src1, src2, dst
//如果src2在src1之前则两者互换
static void swap_op1_and_op2(insn_t *insn)
{
	op_t tmp = insn->Op1;
	insn->Op1 = insn->Op2;
	insn->Op2 = tmp;
	insn->Op1.n = 0;
	insn->Op2.n = 1;
}

#define LOAD_INS 0x80000
#define BYTE_INS (dt_byte << 20)	//1 byte
#define WORD_INS (dt_word << 20)	//2 byte
#define DWORD_INS (dt_dword << 20)	//4 byte
#define QWORD_INS (dt_qword << 20)	//8 byte
static filter_table_t g_ldst[22] =
{
	//dsz sz ld/st na
	//type的低16bit为指令类型，load ins flag在20bit, 寻址位宽在21bit-24bit
	{ "0..00.", TMS6_stw | DWORD_INS },
	{ "0..01.", TMS6_ldw | LOAD_INS | DWORD_INS },
	{ "00010.", TMS6_stb | BYTE_INS },
	{ "00011.", TMS6_ldbu | LOAD_INS | BYTE_INS },
	{ "00110.", TMS6_stb | BYTE_INS },
	{ "00111.", TMS6_ldb | LOAD_INS | BYTE_INS },
	{ "01010.", TMS6_sth | WORD_INS},
	{ "01011.", TMS6_ldhu | LOAD_INS | WORD_INS },
	{ "01110.", TMS6_sth | WORD_INS},
	{ "01111.", TMS6_ldh | LOAD_INS | WORD_INS },
	{ "10010.", TMS6_stw | DWORD_INS },
	{ "10011.", TMS6_ldw | LOAD_INS | DWORD_INS },
	{ "10110.", TMS6_stb | BYTE_INS },
	{ "10111.", TMS6_ldb | LOAD_INS | BYTE_INS },
	{ "11010.", TMS6_stnw | DWORD_INS },
	{ "11011.", TMS6_ldnw | LOAD_INS | DWORD_INS },
	{ "11110.", TMS6_sth | WORD_INS},
	{ "11111.", TMS6_ldh | LOAD_INS | WORD_INS },
	{ "1..000", TMS6_stdw | QWORD_INS },
	{ "1..010", TMS6_lddw | LOAD_INS | QWORD_INS},
	{ "1..001", TMS6_stndw | QWORD_INS },
	{ "1..011", TMS6_ldndw | LOAD_INS | QWORD_INS},
};

static int make_ldst(insn_t* insn, int ctype, uint16_t code, fp_header_t* fph)
{
	//【op.mode】
	//00:0000 * -R[cst]
	//01:0001 * +R[cst]
	//02:0010
	//03:0011
	//04:0100 * -Rb[Ro]
	//05:0101 * +Rb[Ro]
	//06:0110
	//07:0111
	//08:1000 * --R[cst]
	//09:1001 * ++R[cst]
	//10:1010 * R--[cst]
	//11:1011 * R++[cst]
	//12:1100 * --Rb[Ro]
	//13:1101 * ++Rb[Ro]
	//14:1110 * Rb--[Ro]
	//15:1111 * Rb++[Ro]
	int ins_type, offset, src_dst, src1, ptr;

	switch (ctype)
	{
	case TMSC6L_Doff4:
	case TMSC6L_Doff4DW:
		offset = bits_ucst(code, 11, 1, 3) | bits_ucst(code, 13, 3);
		insn->Op1.mode = 1;
		insn->Op1.type = o_displ;
		break;
	case TMSC6L_Dind:
	case TMSC6L_DindDw:
		offset = bits_ucst(code, 13, 3);
		insn->Op1.mode = 5;
		insn->Op1.type = o_phrase;
		break;
	case TMSC6L_Dinc:
	case TMSC6L_DincDw:
		offset = bits_ucst(code, 13, 1) + 1;
		insn->Op1.mode = 11;
		insn->Op1.type = o_displ;
		break;
	case TMSC6L_Ddec:
	case TMSC6L_DdecDw:
		offset = bits_ucst(code, 13, 1) + 1;
		insn->Op1.mode = 8;
		insn->Op1.type = o_displ;
		break;
	}
	
	ptr = bits_ucst(code, 7, 2) + 4;	//reg from A4-A7
	src_dst = bits_ucst(code, 4, 3);

	ins_type = (fph->dsz << 3) | bits_ucst(code, 9, 1, 2) | bits_ucst(code, 3, 1, 1) | bits_ucst(code, 4, 1);
	int itype = filter(g_ldst, 22, ins_type);
	insn->itype = itype & 0xFFFF;

	if (insn->itype == TMS6_stdw || insn->itype == TMS6_lddw || insn->itype == TMS6_stndw || insn->itype == TMS6_ldndw)
		src_dst |= 6;
		
	if (insn->Op1.type == o_displ)
	{
		make_displ(&insn->Op1, ptr, offset, bits_check(code, 0), fph->rs);
		make_reg(&insn->Op2, src_dst, bits_check(code, 12), fph->rs);	//check t bit
	}
	else
	{
		make_phrase(&insn->Op1, ptr, offset, bits_check(code, 0), bits_check(code, 0), fph->rs);
		make_reg(&insn->Op2, src_dst, bits_check(code, 12), fph->rs);	//check t bit
	}
	insn->Op1.dtype = (itype >> 20) & 0xF;

	if (insn->itype == TMS6_stdw || insn->itype == TMS6_lddw || insn->itype == TMS6_stndw || insn->itype == TMS6_ldndw)
		insn->Op2.type = o_regpair;

	if ((itype & LOAD_INS) != LOAD_INS)
		swap_op1_and_op2(insn);	//store ins swap oprand

	insn->funit = bits_check(code, 0) ? FU_D2 : FU_D1;
	insn->cflags |= aux_ldst;
	if (bits_check(code, 12))
		insn->cflags |= aux_t2;
	insn->size = 2;
	return 2;
}

//Dx1、Lx1、Sx1、LSDx1的统一处理函数
static int dls_ins(insn_t* insn, int ctype, uint16_t code, fp_header_t* fph)
{
	//15  13 | 12 | 11 | 10 |  9    7 | 6 | 5 | 4   3 | 2 | 1 | 0
	//  op   | 1  | 1  | 0  | src/dst | 1 | 1 |  unit | 1 | 1 | s
	//  3                        3                2             1

	int src_dst, op, unit;

	src_dst = bits_ucst(code, 7, 3);
	op = bits_ucst(code, 13, 3);
	unit = bits_ucst(code, 3, 2);
	insn->funit = get_unit_type(unit, bits_check(code, 0));
	switch ((unit << 3) | op)
	{
	case 0:		//00 000
	case 8:		//01 000
	case 16:	//10 000
	case 1:		//00 001
	case 9:		//01 001
	case 17:	//10 001
		make_imm(&insn->Op1, bits_check(code, 13));
		make_reg(&insn->Op2, src_dst, bits_check(code, 0), fph->rs);
		insn->itype = TMS6_mvk;
		break;
	case 5:		//00 101
	case 13:	//01 101
	case 21:	//10 101
		make_reg(&insn->Op1, src_dst, bits_check(code, 0), fph->rs);
		make_imm(&insn->Op2, 1);
		make_reg(&insn->Op3, src_dst, bits_check(code, 0), fph->rs);
		insn->itype = TMS6_add;
		break;
	case 7:		//00 111
	case 15:	//01 111
	case 23:	//10 111
		make_reg(&insn->Op1, src_dst, bits_check(code, 0), fph->rs);
		make_imm(&insn->Op2, 1);
		make_reg(&insn->Op3, src_dst, bits_check(code, 0), fph->rs);
		insn->itype = TMS6_xor;
		break;
	case 19:	//10 011
		make_reg(&insn->Op1, src_dst, bits_check(code, 0), fph->rs);
		make_imm(&insn->Op2, 1);
		make_reg(&insn->Op3, src_dst, bits_check(code, 0), fph->rs);
		insn->itype = TMS6_sub;
		break;
	case 2:		//00 010
	case 10:	//01 010
		make_imm(&insn->Op1, 0);
		make_reg(&insn->Op2, src_dst, bits_check(code, 0), fph->rs);
		make_reg(&insn->Op3, src_dst, bits_check(code, 0), fph->rs);
		insn->itype = TMS6_sub;
		break;
	case 3:		//00 011
	case 11:	//01 011
		make_imm(&insn->Op1, -1);
		make_reg(&insn->Op2, src_dst, bits_check(code, 0), fph->rs);
		make_reg(&insn->Op3, src_dst, bits_check(code, 0), fph->rs);
		insn->itype = TMS6_add;
		break;
	case 14:	//01 110
		make_reg(&insn->Op1, src_dst, bits_check(code, 0), fph->rs);
		make_reg(&insn->Op2, rILC, false, false);
		insn->itype = TMS6_mvc;
		break;
	default:
		msg("[+]DEBUG: %X: Reserved ins detect, op=%d\n", insn->ea, op);
		return 0;
	}

	insn->size = 2;
	return 2;
}

static char cc_table[] = { CO_A0, CO_NA0, CO_B0, CO_NB0 };

static int d_unit_ins(insn_t* insn, int ctype, uint16_t code, fp_header_t* fph)
{
	int ld_st, offset, src_dst, src2, op;

	ld_st = (code >> 3) & 1;
	switch (ctype)
	{
	case TMSC6L_Doff4:
	case TMSC6L_Doff4DW:
	case TMSC6L_Dind:
	case TMSC6L_DindDw:
	case TMSC6L_Dinc:
	case TMSC6L_DincDw:
	case TMSC6L_Ddec:
	case TMSC6L_DdecDw:
		return make_ldst(insn, ctype, code, fph);
	case TMSC6L_Dstk:
		offset = bits_ucst(code, 7, 3, 2) | bits_ucst(code, 13, 2);
		src_dst = bits_ucst(code, 4, 3);
		make_displ(&insn->Op1, rB15, offset, false, false);
		insn->Op1.dtype = dt_dword;
		insn->Op1.mode = 1;
		make_reg(&insn->Op2, src_dst, bits_check(code, 12), fph->rs);	//check t bit
		if (bits_ucst(code, 12, 1) != bits_ucst(code, 0, 1))
			insn->cflags |= aux_xp;
		if (ld_st)
		{
			insn->itype = TMS6_ldw;
		}
		else
		{
			insn->itype = TMS6_stw;
			swap_op1_and_op2(insn);
		}
		insn->funit = bits_check(code, 0) ? FU_D2 : FU_D1;
		insn->cflags |= aux_ldst;
		if (bits_check(code, 12))
			insn->cflags |= aux_t2;
		insn->size = 2;
		return 2;
	case TMSC6L_Dx2op:
		src_dst = bits_ucst(code, 13, 3);
		src2 = bits_ucst(code, 7, 3);
		make_reg(&insn->Op1, src_dst, bits_check(code, 0), fph->rs);
		make_reg(&insn->Op2, src2, bits_ucst(code, 12, 1) != bits_ucst(code, 0, 1), fph->rs);
		make_reg(&insn->Op3, src_dst, bits_check(code, 0), fph->rs);
		if (bits_check(code, 11))
			insn->itype = TMS6_sub;
		else
			insn->itype = TMS6_add;
		if(bits_check(code,12))
			insn->cflags |= aux_xp;
		insn->funit = bits_check(code, 0) ? FU_D2 : FU_D1;
		insn->size = 2;
		return 2;
	case TMSC6L_Dx5:
		offset = bits_ucst(code, 11, 2, 3) | bits_ucst(code, 13, 3);
		src_dst = bits_ucst(code, 7, 3);
		make_reg(&insn->Op1, rB15, false, fph->rs);
		make_imm(&insn->Op2, offset);
		make_reg(&insn->Op3, src_dst, bits_check(code, 0), fph->rs);
		insn->itype = TMS6_addaw;
		insn->funit = bits_check(code, 0) ? FU_D2 : FU_D1;
		insn->size = 2;
		return 2;
	case TMSC6L_Dx5p:
		offset = bits_ucst(code, 8, 2, 3) | bits_ucst(code, 13, 3);
		make_reg(&insn->Op1, rB15, false, fph->rs);
		make_imm(&insn->Op2, offset);
		make_reg(&insn->Op3, rB15, false, fph->rs);
		if (bits_check(code, 7))
			insn->itype = TMS6_subaw;
		else
			insn->itype = TMS6_addaw;
		insn->funit = bits_check(code, 0) ? FU_D2 : FU_D1;
		insn->size = 2;
		return 2;
	case TMSC6L_Dx1:
		return dls_ins(insn, ctype, code, fph);
	case TMSC6L_Dpp:
		offset = bits_ucst(code, 13, 1) + 1;
		src_dst = bits_ucst(code, 7, 4);
		make_displ(&insn->Op1, rB15, offset, false, false);
		make_reg(&insn->Op2, src_dst, bits_check(code, 12), false);	//check t bit, rs btis ignored
		switch (bits_ucst(code, 14, 2))
		{
		case 0:
			insn->Op1.mode = 10;
			insn->Op1.dtype = dt_dword;
			insn->itype = TMS6_stw;
			swap_op1_and_op2(insn);
			break;
		case 1:
			insn->Op1.mode = 9;
			insn->Op1.dtype = dt_dword;
			insn->itype = TMS6_ldw;
			break;
		case 2:
			insn->Op1.mode = 10;
			insn->Op1.dtype = dt_qword;
			insn->Op2.type = o_regpair;
			insn->itype = TMS6_stdw;
			swap_op1_and_op2(insn);
			break;
		case 3:
			insn->Op1.mode = 9;
			insn->Op1.dtype = dt_qword;
			insn->Op2.type = o_regpair;
			insn->itype = TMS6_lddw;
			break;
		}
		insn->funit = bits_check(code, 0) ? FU_D2 : FU_D1;
		insn->cflags |= aux_ldst;
		if (bits_check(code, 12))
			insn->cflags |= aux_t2;
		insn->size = 2;
		return 2;
	}
	return 0;
}

static int l_unit_ins(insn_t* insn, int ctype, uint16_t code, fp_header_t* fph)
{
	static const int l2c_table[8] = {TMS6_and, TMS6_or, TMS6_xor, TMS6_cmpeq, TMS6_cmplt, TMS6_cmpgt, TMS6_cmpltu, TMS6_cmpgtu};
	int src1, src2, dst, cst;

	switch (ctype)
	{
	case TMSC6L_L3:
		src1 = bits_ucst(code, 13, 3);
		src2 = bits_ucst(code, 7, 3);
		dst = bits_ucst(code, 4, 3);
		make_reg(&insn->Op1, src1, bits_check(code, 0), fph->rs);
		make_reg(&insn->Op2, src2, bits_ucst(code, 12, 1) != bits_ucst(code, 0, 1), fph->rs);
		make_reg(&insn->Op3, dst, bits_check(code, 0), fph->rs);
		if (bits_ucst(code, 12, 1))
			insn->cflags |= aux_xp;
		switch (bits_ucst(code, 11, 1, 1) | fph->sat)
		{
		case 0:
			insn->itype = TMS6_add;
			break;
		case 1:
			insn->itype = TMS6_sadd;
			break;
		case 2:
			insn->itype = TMS6_sub;
			break;
		case 3:
			insn->itype = TMS6_ssub;
			break;
		}
		insn->funit = bits_check(code, 0) ? FU_L2 : FU_L1;
		insn->size = 2;
		return 2;
	case TMSC6L_L3i:
		cst = bits_ucst(code, 11, 1, 3) | bits_ucst(code, 13, 3);
		cst = cst == 0 ? 8 : check_scts(cst, 4);
		src2 = bits_ucst(code, 7, 3);
		dst = bits_ucst(code, 4, 3);
		make_imm(&insn->Op1, cst);
		make_reg(&insn->Op2, src2, bits_ucst(code, 12, 1) != bits_ucst(code, 0, 1), fph->rs);
		make_reg(&insn->Op3, dst, bits_check(code, 0), fph->rs);
		if (bits_ucst(code, 12, 1))
			insn->cflags |= aux_xp;
		insn->itype = TMS6_add;
		insn->funit = bits_check(code, 0) ? FU_L2 : FU_L1;
		insn->size = 2;
		return 2;
	case TMSC6L_Ltbd:
		//msg("%X ltbd ins is incomplete\n", insn->ea);
		return 0;
	case TMSC6L_L2c:
		src1 = bits_ucst(code, 13, 3);
		src2 = bits_ucst(code, 7, 3);
		dst = bits_ucst(code, 4, 1);
		make_reg(&insn->Op1, src1, bits_check(code, 0), fph->rs);
		make_reg(&insn->Op2, src2, bits_ucst(code, 12, 1) != bits_ucst(code, 0, 1), fph->rs);
		make_reg(&insn->Op3, dst, bits_check(code, 0), fph->rs);
		if (bits_ucst(code, 12, 1))
			insn->cflags |= aux_xp;
		insn->itype = l2c_table[bits_ucst(code, 11, 1, 2) | bits_ucst(code, 5, 2)];
		insn->funit = bits_check(code, 0) ? FU_L2 : FU_L1;
		insn->size = 2;
		return 2;
	case TMSC6L_Lx5:
		cst = bits_ucst(code, 11, 2, 3) | bits_ucst(code, 13, 3);
		dst = bits_ucst(code, 7, 3);
		make_imm(&insn->Op1, cst);
		make_reg(&insn->Op2, dst, bits_check(code, 0), fph->rs);
		insn->itype = TMS6_mvk;
		insn->funit = bits_check(code, 0) ? FU_L2 : FU_L1;
		insn->size = 2;
		return 2;
	case TMSC6L_Lx3c:
		cst = bits_ucst(code, 13, 3);
		dst = bits_ucst(code, 11, 1);
		src2 = bits_ucst(code, 7, 3);
		make_imm(&insn->Op1, cst);
		make_reg(&insn->Op2, src2, bits_check(code, 0), fph->rs);
		make_reg(&insn->Op3, dst, bits_check(code, 0), fph->rs);
		insn->itype = TMS6_cmpeq;
		insn->funit = bits_check(code, 0) ? FU_L2 : FU_L1;
		insn->size = 2;
		return 2;
	case TMSC6L_Lx1c:
		cst = bits_ucst(code, 13, 1);
		dst = bits_ucst(code, 11, 1);
		src2 = bits_ucst(code, 7, 3);
		make_imm(&insn->Op1, cst);
		make_reg(&insn->Op2, src2, bits_check(code, 0), fph->rs);
		make_reg(&insn->Op3, dst, bits_check(code, 0), fph->rs);
		switch (bits_ucst(code, 14, 2))
		{
		case 0:
			insn->itype = TMS6_cmplt;
			break;
		case 1:
			insn->itype = TMS6_cmpgt;
			break;
		case 2:
			insn->itype = TMS6_cmpltu;
			break;
		case 3:
			insn->itype = TMS6_cmpgtu;
			break;
		}
		insn->size = 2;
		insn->funit = bits_check(code, 0) ? FU_L2 : FU_L1;
		return 2;
	case TMSC6L_Lx1:
		return dls_ins(insn, ctype, code, fph);
	}
	return 0;
}

static int m_unit_ins(insn_t* insn, int ctype, uint16_t code, fp_header_t* fph)
{
	static const int m3_ins_table[] = 
	{ TMS6_mpy, TMS6_mpyh, TMS6_mpylh, TMS6_mpyhl,
	  TMS6_smpy, TMS6_smpyh, TMS6_smpylh, TMS6_smpyhl};
	char src1 = bits_ucst(code, 13, 3);
	char src2 = bits_ucst(code, 7, 3);
	char dst = bits_ucst(code, 10, 2, 1);	//even reg
	char sop = (fph->sat << 2) | (code >> 5) & 0x3;

	insn->itype = m3_ins_table[sop];

	make_reg(&insn->Op1, src1, bits_check(code, 0), fph->rs);
	make_reg(&insn->Op2, src2, bits_ucst(code, 0, 1) != bits_ucst(code, 12, 1), fph->rs);
	make_reg(&insn->Op3, dst, bits_check(code, 0), fph->rs);

	if (bits_check(code, 12))
		insn->cflags |= aux_xp;
	insn->funit = bits_check(code, 0) ? FU_M2 : FU_M1;
	insn->size = 2;
	return 2;
}

static int s_unit_ins(insn_t* insn, int ctype, uint16_t code, fp_header_t* fph)
{
	int n3, offset, src1, src2, dst;
	uint8_t ucst;
	switch (ctype)
	{
	case TMSC6L_Sbs7:
		n3 = bits_scst(code, 13, 3);
		offset = bits_scst(code, 6, 7);
		make_near(&insn->Op1, fph->adr - 28, offset);
		make_imm(&insn->Op2, n3);
		insn->itype = TMS6_bnop;
		insn->size = 2;
		insn->funit = bits_check(code, 0) ? FU_S2 : FU_S1;
		return 2;
	case TMSC6L_Sbu8:
		offset = bits_scst(code, 6, 8);
		make_near(&insn->Op1, fph->adr - 28, offset);
		make_imm(&insn->Op2, 5);
		insn->itype = TMS6_bnop;
		insn->size = 2;
		insn->funit = bits_check(code, 0) ? FU_S2 : FU_S1;
		return 2;
	case TMSC6L_Scs10:
		offset = bits_scst(code, 6, 10);
		make_near(&insn->Op1, fph->adr - 28, offset, 2);
		make_reg(&insn->Op2, 3, bits_check(code, 0), false);//A3 or B3
		insn->itype = TMS6_callp;
		insn->size = 2;
		insn->funit = bits_check(code, 0) ? FU_S2 : FU_S1;
		return 2;
	case TMSC6L_Sbs7c:
		n3 = bits_ucst(code, 13, 3);
		offset = bits_scst(code, 6, 7);
		make_near(&insn->Op1, fph->adr - 28, offset);
		make_imm(&insn->Op2, n3);
		insn->cond = cc_table[bits_ucst(code, 0, 1, 1) | bits_ucst(code, 4, 1)];
		insn->itype = TMS6_bnop;
		insn->size = 2;
		insn->funit = bits_check(code, 0) ? FU_S2 : FU_S1;
		return 2;
	case TMSC6L_Sbu8c:
		offset = bits_ucst(code, 6, 8); 
		make_near(&insn->Op1, fph->adr - 28, offset);
		make_imm(&insn->Op2, 5);
		insn->cond = cc_table[((code & 1) << 1) | ((code >> 4) & 1)];
		insn->itype = TMS6_bnop;
		insn->size = 2;
		insn->funit = bits_check(code, 0) ? FU_S2 : FU_S1;
		return 2;
	case TMSC6L_S3:
		src1 = bits_ucst(code, 13, 3);
		src2 = bits_ucst(code, 7, 3);
		dst = bits_ucst(code, 4, 3);
		make_reg(&insn->Op1, src1, bits_check(code, 0), fph->rs);
		make_reg(&insn->Op2, src2, bits_ucst(code, 12, 1) != bits_ucst(code, 0, 1), fph->rs);
		make_reg(&insn->Op3, dst, bits_check(code, 0), fph->rs);
		if(bits_ucst(code, 12, 1))
			insn->cflags |= aux_xp;
		switch (bits_combine_111(fph->br, fph->sat, bits_ucst(code, 11, 1)))
		{
		case 0:
			insn->itype = TMS6_add;
			break;
		case 2:
			insn->itype = TMS6_sadd;
			break;
		case 1:
		case 3:
			insn->itype = TMS6_sub;
			break;
		}
		insn->size = 2;
		insn->funit = bits_check(code, 0) ? FU_S2 : FU_S1;
		return 2;
	case TMSC6L_S3i:
		src1 = bits_ucst(code, 13, 3);
		if (src1 == 0)	
			src1 = 16;
		else if (src1 == 7) 
			src1 = 8;
		src2 = bits_ucst(code, 7, 3);
		dst = bits_ucst(code, 4, 3);
		make_reg(&insn->Op1, src2, bits_ucst(code, 0, 1) != bits_ucst(code, 12, 1), fph->rs);
		make_imm(&insn->Op2, src1);
		make_reg(&insn->Op3, dst, bits_check(code, 0), fph->rs);
		if (bits_check(code, 11))
			insn->itype = TMS6_shr;
		else
			insn->itype = TMS6_shl;
		if (bits_ucst(code, 12, 1))
			insn->cflags |= aux_xp;
		insn->size = 2;
		insn->funit = bits_check(code, 0) ? FU_S2 : FU_S1;
		return 2;
	case TMSC6L_Smvk8:
		ucst = bits_ucst(code, 10, 1, 7) | bits_ucst(code, 5, 2, 5) | bits_ucst(code, 11, 2, 3) | bits_ucst(code, 13, 3);
		dst = bits_ucst(code, 7, 3);
		make_imm(&insn->Op1, ucst);
		make_reg(&insn->Op2, dst, bits_check(code, 0), fph->rs);
		if (bits_ucst(code, 12, 1))
			insn->cflags |= aux_xp;
		insn->itype = TMS6_mvk;
		insn->funit = bits_check(code, 0) ? FU_S2 : FU_S1;
		insn->size = 2;
		return 2;
	case TMSC6L_Ssh5:
		dst = bits_ucst(code, 7, 3);
		ucst = bits_ucst(code, 11, 2, 3) | bits_ucst(code, 13, 3);
		make_reg(&insn->Op1, dst, bits_check(code, 0), fph->rs);
		make_imm(&insn->Op2, ucst);
		make_reg(&insn->Op3, dst, bits_check(code, 0), fph->rs);
		switch (bits_combine_111(fph->sat, bits_ucst(code, 6, 1), bits_ucst(code, 5, 1)))
		{
		case 0:
		case 4:
		case 1:
		case 5:
			insn->itype = TMS6_shl;
			break;
		case 2:
			insn->itype = TMS6_shru;
			break;
		case 6:
			insn->itype = TMS6_sshl;
			break;
		}
		insn->size = 2;
		insn->funit = bits_check(code, 0) ? FU_S2 : FU_S1;
		return 2;
	case TMSC6L_S2sh:
		dst = bits_ucst(code, 7, 3);
		src1 = bits_ucst(code, 13, 3);
		make_reg(&insn->Op1, dst, bits_check(code, 0), fph->rs);
		make_reg(&insn->Op2, src1, bits_check(code, 0), fph->rs);
		make_reg(&insn->Op3, dst, bits_check(code, 0), fph->rs);
		switch (bits_ucst(code, 11, 2))
		{
		case 0:
			insn->itype = TMS6_shl;
			break;
		case 1:
			insn->itype = TMS6_shr;
			break;
		case 2:
			insn->itype = TMS6_shru;
			break;
		case 3:
			insn->itype = TMS6_sshl;
			break;
		}
		insn->size = 2;
		insn->funit = bits_check(code, 0) ? FU_S2 : FU_S1;
		return 2;
	case TMSC6L_Sc5:
		dst = bits_ucst(code, 7, 3);
		ucst = bits_ucst(code, 11, 2, 3) | bits_ucst(code, 13, 3);
		switch (bits_ucst(code, 5, 2))
		{
		case 0:
			insn->itype = TMS6_extu;
			make_imm(&insn->Op1, ucst);
			make_imm(&insn->Op2, 31);
			make_reg(&insn->Op3, 0, bits_check(code, 0), fph->rs);
			insn->Op1.src2 = get_reg(dst, bits_check(code, 0), fph->rs);
			insn->cflags |= aux_src2;
			break;
		case 1:
			insn->itype = TMS6_set;
			make_imm(&insn->Op1, ucst);
			make_imm(&insn->Op2, ucst);
			make_reg(&insn->Op3, dst, bits_check(code, 0), fph->rs);
			insn->Op1.src2 = insn->Op3.reg;
			insn->cflags |= aux_src2;
			break;
		case 2:
			insn->itype = TMS6_clr;
			make_imm(&insn->Op1, ucst);
			make_imm(&insn->Op2, ucst);
			make_reg(&insn->Op3, dst, bits_check(code, 0), fph->rs);
			insn->Op1.src2 = insn->Op3.reg;
			insn->cflags |= aux_src2;
			break;
		}
		insn->size = 2;
		insn->funit = bits_check(code, 0) ? FU_S2 : FU_S1;
		return 2;
	case TMSC6L_S2ext:
		dst = bits_ucst(code, 13, 3);
		src2 = bits_ucst(code, 7, 3);
		switch (bits_ucst(code, 11, 2))
		{
		case 0:
		case 2:
			insn->itype = (bits_ucst(code, 11, 2) == 2) ? TMS6_extu : TMS6_ext;
			make_imm(&insn->Op1, 16);
			make_imm(&insn->Op2, 16);
			make_reg(&insn->Op3, dst, bits_check(code, 0), fph->rs);
			insn->Op1.src2 = get_reg(src2, bits_check(code, 0), fph->rs);
			insn->cflags |= aux_src2;
			break;
		case 1:
		case 3:
			insn->itype = (bits_ucst(code, 11, 2) == 3) ? TMS6_extu : TMS6_ext;
			make_imm(&insn->Op1, 24);
			make_imm(&insn->Op2, 24);
			make_reg(&insn->Op3, dst, bits_check(code, 0), fph->rs);
			insn->Op1.src2 = get_reg(src2, bits_check(code, 0), fph->rs);
			insn->cflags |= aux_src2;
			break;
		}
		insn->size = 2;
		insn->funit = bits_check(code, 0) ? FU_S2 : FU_S1;
		return 2;
	case TMSC6L_Sx2op:
		dst = bits_ucst(code, 13, 3);
		src2 = bits_ucst(code, 7, 3);
		make_reg(&insn->Op1, dst, bits_check(code, 0), fph->rs);
		make_reg(&insn->Op2, src2, bits_ucst(code, 12, 1) != bits_ucst(code, 0, 1), fph->rs);
		make_reg(&insn->Op3, dst, bits_check(code, 0), fph->rs);
		if (bits_ucst(code, 12, 1))
			insn->cflags |= aux_xp;
		if (bits_check(code, 12))
			insn->itype = TMS6_sub;
		else
			insn->itype = TMS6_add;
		insn->size = 2;
		insn->funit = bits_check(code, 0) ? FU_S2 : FU_S1;
		return 2;
	case TMSC6L_Sx5:
		ucst = bits_ucst(code, 11, 2, 3) | bits_ucst(code, 13, 3);
		dst = bits_ucst(code, 7, 3);
		insn->itype = TMS6_addk;
		make_imm(&insn->Op1, ucst);
		make_reg(&insn->Op2, dst, bits_check(code, 0), fph->rs);
		insn->size = 2;
		insn->funit = bits_check(code, 0) ? FU_S2 : FU_S1;
		return 2;
	case TMSC6L_Sx1:
		return dls_ins(insn, ctype, code, fph);
	case TMSC6L_Sx1b:
		n3 = bits_ucst(code, 13, 3);
		src2 = bits_ucst(code, 7, 4);
		make_reg(&insn->Op1, src2, bits_check(code, 0), fph->rs);
		make_imm(&insn->Op2, n3);
		insn->itype = TMS6_bnop;
		insn->size = 2;
		insn->funit = bits_check(code, 0) ? FU_S2 : FU_S1;
		return 2;
	}
	return 0;
}

static int dls_unit_ins(insn_t* insn, int ctype, uint16_t code, fp_header_t* fph)
{
	int dst, src2, unit, cc, op;

	unit = bits_ucst(code, 3, 2);
	switch (ctype)
	{
	case TMSC6L_LSDmvto:
	case TMSC6L_LSDmvfr:
		if (ctype == TMSC6L_LSDmvto)
		{
			dst = bits_ucst(code, 13, 3); 
			src2 = bits_ucst(code, 7, 5); 
		}
		else
		{
			dst = bits_ucst(code, 10, 2, 3) | bits_ucst(code, 13, 3);
			src2 = bits_ucst(code, 7, 3);
		}
		make_reg(&insn->Op1, src2, bits_ucst(code, 12, 1) != bits_ucst(code, 0, 1), fph->rs);
		make_reg(&insn->Op2, dst, bits_ucst(code, 0, 1), fph->rs);
		if (bits_ucst(code, 12, 1))
			insn->cflags |= aux_xp;
		insn->funit = get_unit_type(unit, bits_ucst(code, 0, 1));
		insn->size = 2;
		insn->itype = TMS6_mv;
		return 2;
	case TMSC6L_LSDx1c:
		dst = bits_ucst(code, 7, 3);
		cc = bits_ucst(code, 14, 2);
		make_imm(&insn->Op1, bits_ucst(code, 13, 1));
		make_reg(&insn->Op2, dst, bits_ucst(code, 0, 1), fph->rs);
		insn->funit = get_unit_type(unit, bits_ucst(code, 0, 1));
		insn->cond = cc_table[cc];
		insn->size = 2;
		insn->itype = TMS6_mvk;
		return 2;
	case TMSC6L_LSDx1:
		return dls_ins(insn, ctype, code, fph);
	}

	return 0;
}

static int n_unit_ins(insn_t* insn, int ctype, uint16_t code, fp_header_t* fph)
{
	int ii, stage, mask, n3;
	switch (ctype)
	{
	case TMSC6L_Uspl:
	case TMSC6L_Uspldr:
		ii = bits_ucst(code, 14, 1, 3) | bits_ucst(code, 7, 3);
		make_imm(&insn->Op1, ii + 1);
		if (bits_check(code, 15) || bits_check(code, 0))
			insn->itype = TMS6_sploopd;
		else
			insn->itype = TMS6_sploop;
		if (bits_check(code, 15))
		{
			//[A0] SPLOOPD ii
			//[B0] SPLOOPD ii
			insn->cond = bits_check(code, 0) ? CO_B0 : CO_A0;
		}
		insn->size = 2;
		return 2;
	case TMSC6L_Uspk:
		stage = bits_ucst(code, 0, 1, 5) | bits_ucst(code, 14, 2, 3) | bits_ucst(code, 7, 3);
		make_stgcyc(&insn->Op1, stage);
		insn->itype = TMS6_spkernel;
		insn->size = 2;
		return 2;
	case TMSC6L_Uspm_spmask:
	case TMSC6L_Uspm_spmaskr:
		if(bits_check(code, 12))
			insn->itype = TMS6_spmaskr;
		else
			insn->itype = TMS6_spmask;
		mask = bits_ucst(code, 14, 2, 4) | bits_ucst(code, 7, 3, 1) | bits_ucst(code, 0, 1);
		make_spmask(&insn->Op1, mask);
		insn->size = 2;
		return 2;
	case TMSC6L_Unop:
		n3 = bits_ucst(code, 13, 3) + 1;
		make_imm(&insn->Op1, n3);
		insn->itype = TMS6_nop;
		insn->size = 2;
		return 2;
	}
	return 0;
}

static void parallel_proc(insn_t* insn, fectch_paket_t* fp)
{
	if (insn->ea == fp->fph.adr)
		return;
	int idx = (insn->ea - fp->start) >> 1;
	if (bits_check(fp->fph.p_bits, idx))
	{
		insn->cflags |= aux_para;
	}
}

int idaapi ana16(insn_t* insn, fetch_packet_t* fp)
{
	if (bits_check(insn->ea, 0))	//2 bytes align
		return -1;

	if (fp->fph_vaild)
	{
		uint16_t code = insn->get_next_word();
		uint32_t fv = (fp->fph.dsz << 18) | (fp->fph.br << 17) | (fp->fph.sat << 16) | code;
		int ins_ctype = filter(g_tms66x_cpt_ins, sizeof(g_tms66x_cpt_ins) / sizeof(filter_table_t), fv);
		int ret = 0;
		if (ins_ctype >= D_CPT_START && ins_ctype <= D_CPT_END)
			ret = d_unit_ins(insn, ins_ctype, code, &fp->fph);
		else if (ins_ctype >= L_CPT_START && ins_ctype <= L_CPT_END)
			ret = l_unit_ins(insn, ins_ctype, code, &fp->fph);
		else if (ins_ctype >= M_CPT_START && ins_ctype <= M_CPT_END)
			ret = m_unit_ins(insn, ins_ctype, code, &fp->fph);
		else if (ins_ctype >= S_CPT_START && ins_ctype <= S_CPT_END)
			ret = s_unit_ins(insn, ins_ctype, code, &fp->fph);
		else if (ins_ctype >= DLS_CPT_START && ins_ctype <= DLS_CPT_END)
			ret = dls_unit_ins(insn, ins_ctype, code, &fp->fph);
		else if (ins_ctype >= N_CPT_START && ins_ctype <= N_CPT_END)
			ret = n_unit_ins(insn, ins_ctype, code, &fp->fph);
		if (ret > 0)
			parallel_proc(insn, fp);
		//msg("%X ctype: %d ret:%d\n", insn->ea, ins_ctype, ret);
		return ret;
	}
	else
	{
		msg("[+]DEBUG: fetch packet header invalid when process compact instruction\n");
		return -1;
	}

	return 0;
}