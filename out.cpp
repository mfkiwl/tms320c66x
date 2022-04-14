#include "tms66x.h"
#include "ua.hpp"
#include "fetch_packet.h"
#include "bits.h"

class out_tms320c66x_t : public outctx_t
{
	out_tms320c66x_t(void) = delete; // not used
public:
	bool out_operand(const op_t& x);
	void out_mnem(void);
	void out_insn(void);
	void outreg(int r) { if(r >= 0 && r < ph.regs_num) out_register(ph.reg_names[r]); }
	void out_pre_mode(int mode);
	void out_post_mode(int mode);
	void print_stg_cyc(ea_t ea, int stgcyc);
	bool tms6_out_name_expr(const op_t& x, uval_t opval);
};
CASSERT(sizeof(out_tms320c66x_t) == sizeof(outctx_t));

//定义out_insn和out_opnd
//DECLARE_OUT_FUNCS_WITHOUT_OUTMNEM(out_tms320c66x_t)
void idaapi out_insn(outctx_t& ctx) 
{                                               
	out_tms320c66x_t* p = (out_tms320c66x_t*)&ctx; 
	p->out_insn(); 
}

void idaapi out_mnem(outctx_t& ctx)
{
	out_tms320c66x_t* p = (out_tms320c66x_t*)&ctx;
	p->out_mnem();
}

bool idaapi out_opnd(outctx_t& ctx, const op_t& x)
{
	out_tms320c66x_t* p = (out_tms320c66x_t*)&ctx;
	return p->out_operand(x);
}

void idaapi header(outctx_t* ctx)
{
	ctx->gen_header(GH_PRINT_ALL);
}

void idaapi footer(outctx_t* ctx)
{
	qstring nbuf = get_colored_name(inf_get_start_ea());
	const char* name = nbuf.c_str();
	const char* end = ash.end;
	if (end == NULL)
		ctx->gen_printf(DEFAULT_INDENT, COLSTR("%s end %s", SCOLOR_AUTOCMT), ash.cmnt, name);
	else
		ctx->gen_printf(DEFAULT_INDENT, COLSTR("%s", SCOLOR_ASMDIR)
			" "
			COLSTR("%s %s", SCOLOR_AUTOCMT), ash.end, ash.cmnt, name);
}

bool is_first_insn_in_exec_packet(ea_t ea)
{
	//上一条指令没有aux_para即为执行包的第一条指令
	insn_t insn;

	ea_t prev_ins_ea = prev_not_tail(ea);
	if (prev_ins_ea == BADADDR)	//没有上一条指令，返回true
		return true;
	if (!is_code(get_flags(prev_ins_ea)))	//上一条指令不是代码，返回true
		return true;

	if (decode_insn(&insn, prev_ins_ea) == 0)
		return true;
	if (insn.cflags & aux_fph)	//fetch packet header需要被忽略
		return is_first_insn_in_exec_packet(prev_ins_ea);
	return (insn.cflags & aux_para) != aux_para;
}

struct ii_info_t
{
	char ii;
	char cyc;
};

static const ii_info_t ii_info[] =
{
  { 1,  0 },
  { 2,  1 },
  { 4,  2 },
  { 8,  3 },
  { 14, 4 },
};

void out_tms320c66x_t::print_stg_cyc(ea_t ea, int stgcyc)
{
	int ii = 1;
	insn_t prev;
	for (int i = 0; i < 14 && decode_prev_insn(&prev, ea) != BADADDR; i++)
	{
		if (prev.itype == TMS6_sploop
			|| prev.itype == TMS6_sploopd
			|| prev.itype == TMS6_sploopw)
		{
			ii = prev.Op1.value;
			break;
		}
		ea = prev.ea;
	}
	for (int i = 0; i < qnumber(ii_info); i++)
	{
		if (ii_info[i].ii >= ii)
		{
			int cyc = ii_info[i].cyc;
			int stg = 0;
			int stgbits = 6 - cyc;
			int bit = 1 << cyc;
			for (int j = 0; j < stgbits; j++, bit <<= 1)
			{
				stg <<= 1;
				if (stgcyc & bit)
					stg |= 1;
			}
			cyc = stgcyc & ((1 << cyc) - 1);
			out_long(stg, 10);
			out_symbol(',');
			out_long(cyc, 10);
			break;
		}
	}
}

void out_tms320c66x_t::out_pre_mode(int mode)
{
	out_symbol('*');
	switch (mode)
	{
	case 0x08:  // 1000 *--R[cst]
	case 0x0C:  // 1100 *--Rb[Ro]
		out_symbol('-');
		// fallthrough
	case 0x00:  // 0000 *-R[cst]
	case 0x04:  // 0100 *-Rb[Ro]
		out_symbol('-');
		break;
	case 0x09:  // 1001 *++R[cst]
	case 0x0D:  // 1101 *++Rb[Ro]
		out_symbol('+');
		out_symbol('+');
		break;
	case 0x01:  // 0001 *+R[cst]
	case 0x05:  // 0101 *+Rb[Ro]
		out_symbol('+');
		break;
	case 0x0A:  // 1010 *R--[cst]
	case 0x0B:  // 1011 *R++[cst]
	case 0x0E:  // 1110 *Rb--[Ro]
	case 0x0F:  // 1111 *Rb++[Ro]
		break;
	}
}

void out_tms320c66x_t::out_post_mode(int mode)
{
	switch (mode)
	{
	case 0x08:  // 1000 *--R[cst]
	case 0x0C:  // 1100 *--Rb[Ro]
	case 0x00:  // 0000 *-R[cst]
	case 0x04:  // 0100 *-Rb[Ro]
	case 0x09:  // 1001 *++R[cst]
	case 0x0D:  // 1101 *++Rb[Ro]
	case 0x01:  // 0001 *+R[cst]
	case 0x05:  // 0101 *+Rb[Ro]
		break;
	case 0x0A:  // 1010 *R--[cst]
	case 0x0E:  // 1110 *Rb--[Ro]
		out_symbol('-');
		out_symbol('-');
		break;
	case 0x0B:  // 1011 *R++[cst]
	case 0x0F:  // 1111 *Rb++[Ro]
		out_symbol('+');
		out_symbol('+');
		break;
	}
}

bool out_tms320c66x_t::out_operand(const op_t& x)
{
	uchar sign;

	switch (x.type)
	{
		case o_void:
			return true;

		case o_reg:
			outreg(x.reg);
			break;

		case o_regpair:
			outreg(x.reg + 1);
			out_symbol(':');
			outreg(x.reg);
			break;

		case o_imm:
		{
			if (this->insn.itype == TMS6_mvk || this->insn.itype == TMS6_mvkl ||
				this->insn.itype == TMS6_mvkh || this->insn.itype == TMS6_mvklh)
			{
				out_value(x, OOFS_IFSIGN | OOFW_IMM);
			}
			else
			{
				int v = check_scts(x.value, 32);
				if (v >= 0)
					out_value(x, OOFS_IFSIGN | OOFW_IMM);
				else
					out_value(x, OOFS_IFSIGN | OOFW_IMM | OOF_SIGNED);
			}
			break;
		}


		case o_stgcyc:	//todo: check
			print_stg_cyc(insn.ea, x.value);
			break;

		case o_phrase:	//baseR[offsetR]
			out_pre_mode(x.mode);
			outreg(x.reg);	//先输出baseR
			out_post_mode(x.mode);
			out_symbol('[');
			outreg(x.secreg);	//再输出offsetR
			out_symbol(']');
			break;

		case o_displ:
			out_pre_mode(x.mode);
			outreg(x.reg);
			out_post_mode(x.mode);
			{
				if (is_off(F, x.n))	//todo: check is_off and ()
				{
					out_symbol('(');
					out_value(x, OOF_ADDR | OOFS_IFSIGN | OOFW_IMM | OOF_SIGNED | OOFW_32);
					out_symbol(')');
				}
				else
				{
					out_symbol('[');
					out_value(x, OOF_ADDR | OOFS_IFSIGN | OOFW_IMM | OOF_SIGNED | OOFW_32);
					out_symbol(']');
				}
			}
			break;

		case o_spmask:
		{
			static const char units[] = "LLSSDDMM";
			uchar mask = x.reg;
			bool need_comma = false;
			for (int i = 0; i < 8; i++, mask >>= 1)
			{
				if (mask & 1)
				{
					if (need_comma)
						out_symbol(',');
					out_tagon(COLOR_KEYWORD);
					out_char(units[i]);
					out_char('1' + (i & 1));
					out_tagoff(COLOR_KEYWORD);
					need_comma = true;
				}
			}
			break;
		}
		case o_near:
		{
			ea_t go_addr = x.addr;
			if (out_name_expr(x, go_addr, go_addr))
			{
				//out_symbol('+');
				//out_long((go_addr - this->insn.ea), 16);
			}
			else
			{
				out_tagon(COLOR_ERROR);
				out_btoa(x.addr, 16);
				out_tagoff(COLOR_ERROR);
				remember_problem(PR_NONAME, this->insn.ea);
			}
			break;
		}

		default:
			msg("out: %a: bad optype %d\n", insn.ea, x.type);
			break;
	}
	return true;
}

void out_tms320c66x_t::out_mnem(void)
{
	static const int jump_ins[] = { TMS6_b, TMS6_bnop, TMS6_birp, TMS6_bnrp,TMS6_bdec,TMS6_bpos, TMS6_callp, TMS6_addkpc };
	int i;

	for (i = 0; i < sizeof(jump_ins) / sizeof(int); i++)
	{
		if (this->insn.itype == jump_ins[i])
		{
			out_line(Instructions[this->insn.itype].name, COLOR_ASMDIR);
			out_spaces(16);
			return;
		}
	}

	if (this->insn.itype < TMS6_last)
	{
		out_line(Instructions[this->insn.itype].name, COLOR_CODNAME);
		out_spaces(16);
	}
	else
		out_line("error", COLOR_ERROR);
}

void out_tms320c66x_t::out_insn(void)
{
	//[parallel] [cond] ins .[unit][cross path][op1], [op2], [op3]
	if (this->insn.cflags & aux_fph)
	{
		this->out_line("   fetch packet header");
		this->flush_outbuf(-2);
		return;
	}

	//输出并行符号
	if (is_first_insn_in_exec_packet(this->insn.ea) == false)
	{
		this->out_symbol('|');
		this->out_symbol('|');
	}
	else {
		this->out_char(' ');
		this->out_char(' ');
	}

	//输出条件符号
	static const char* const conds[] =
	{
	  "     ", "     ", "[B0] ", "[!B0]",
	  "[B1] ", "[!B1]", "[B2] ", "[!B2]",
	  "[A1] ", "[!A1]", "[A2] ", "[!A2]",
	  "[A0] ", "[!A0]", "     ", "     "
	};
	this->out_keyword(conds[this->insn.cond]);
	this->out_char(' ');

	//输出指令
	//若指令枚举和指令字符串一一对应就可以调用SDK自带的函数输出指令
	//在事件监听器中处理ev_out_mnem实现着色功能
	out_mnemonic();

	//输出操作单元
	static const char* const units[] =
	{
	  NULL,
	  ".L1", ".L2",
	  ".S1", ".S2",
	  ".M1", ".M2",
	  ".D1", ".D2",
	};
	if (this->insn.funit != FU_NONE)
		this->out_keyword(units[uchar(this->insn.funit)]);
	else
		this->out_line("   ");


	//输出交叉路径
	if (this->insn.cflags & aux_ldst)
	{
		//load store ins 特殊处理
		if (this->insn.cflags & aux_t2)
			this->out_keyword("T2");
		else
			this->out_keyword("T1");
	}
	else
	{
		if (this->insn.cflags & aux_xp)
			this->out_keyword("X ");
		else
			this->out_line("  ");
	}
	this->out_line("  ");

	//输出操作数
	//有限处理4个操作数时的第一个操作数
	if (this->insn.cflags & aux_src2)
	{
		this->outreg(this->insn.Op1.src2);
		this->out_symbol(',');
		this->out_char(' ');
	}

	//输出其他操作数
	if (this->insn.Op1.type != o_void && this->insn.Op1.shown())
	{
		this->out_one_operand(0);
	}
	if (this->insn.Op2.type != o_void && this->insn.Op2.shown())
	{
		this->out_symbol(',');
		this->out_char(' ');
		this->out_one_operand(1);
	}
	if (this->insn.Op3.type != o_void && this->insn.Op3.shown())
	{
		this->out_symbol(',');
		this->out_char(' ');
		this->out_one_operand(2);
	}

	//输出注释，该函数作用输出字符形式的立即数
	//如MVK.S1    2Ah, A4 ; '*'
	this->out_immchar_cmts();

	//刷新输出行
	this->flush_outbuf(-2);
}
