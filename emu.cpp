#include "fetch_packet.h"
#include "tms66x.h"

/* 
    处理数据引用
    如下所示，应该添加一个0x8F3CE4的数据引用
00036E70   021E722A           MVK.S2        0x3ce4,B4
00036E74   020047EA           MVKH.S2       0x8f0000,B4 ;B4=0x8f3ce4
*/
static void data_quote(const insn_t* insn, const op_t* x)
{

}

//在发生分支的地址后面delay slot内寻找是否有addkpc xx, B3, x操作
static bool check_func_1(const insn_t* insn)
{
    insn_t next_ins;
    ea_t next_adr = insn->ea + insn->size;
    int loop = 6;
    if (insn->itype == TMS6_bnop)
        loop -= insn->Op2.value;
    while (loop > 0)
    {
        if (decode_insn(&next_ins, next_adr) == 0)
            break;
        if ((next_ins.cflags & aux_para) == 0)
            loop -= 1;
        if (next_ins.itype == TMS6_nop)
            loop -= next_ins.Op1.value;
        if (next_ins.itype == TMS6_addkpc && next_ins.Op2.reg == rB3)
            return true;
        next_adr = next_ins.ea + next_ins.size;
    }
    return false;
}

//在分支跳转后的loop条指令内寻找是否有对B15寄存器减栈的操作
static bool check_func_2(const insn_t* insn)
{
    insn_t next_ins;
    ea_t next_adr = insn->Op1.addr;
    int loop = 6;   
    while (loop)
    {
        decode_insn(&next_ins, next_adr);
        int type = next_ins.itype;
        if (type == TMS6_stdw || type == TMS6_stnw || type == TMS6_stndw
            || type == TMS6_stb || type == TMS6_stbu || type == TMS6_sth
            || type == TMS6_sthu || type == TMS6_stw)
        {
            //是否符合*B15--[ucst5]模式
            if (next_ins.Op2.type == o_displ && next_ins.Op2.reg == rB15 && next_ins.Op2.mode == MO_UCST_SUBSUB)
                return true;
        }
        //sub b15, 4, b15
        //add -4, b15, b15
        next_adr = next_ins.ea + next_ins.size;
        loop -= 1;
    }
    return false;
}

/*
    处理代码引用
    callp必然是函数跳转
    b和bnop即有可能是函数内分支跳转也有可能是函数跳转，若为函数跳转，则跳转附近必然有ADDKPC或其他对B3操作的指令
    <bnop B3,x>和<b B3>是普通函数常见的返回指令
*/
static int code_quote(const insn_t* insn)
{
    if (insn->itype == TMS6_callp)
    {
        insn->add_cref(insn->Op1.addr, insn->Op1.offb, fl_CN);
        return 1;
    }

    return 0;

    if (insn->itype == TMS6_bnop || insn->itype == TMS6_b)
    {
        bool check1, check2;

        check1 = check_func_1(insn);
        check2 = check_func_2(insn);

        if (check2 && check1)
        {
            insn->add_cref(insn->Op1.addr, insn->Op1.offb, fl_CN);
            return 1;
        }
            
    }
    return 0;
}

static void handle_operand(const insn_t* insn, const op_t* x, bool isload)
{
    switch (x->type)
    {
    case o_regpair:
    case o_reg:
    case o_phrase:
    case o_spmask:
    case o_stgcyc:
    case o_displ:
        break;
    case o_imm:
        if (isload)
            data_quote(insn, x);
        break;
    case o_near:
        if(code_quote(insn) == 0)
            insn->add_cref(x->addr, x->offb, fl_JN);
        break;
    }
}

int emu(const insn_t* insn)
{
	fetch_packet_t fp;
	update_fetch_packet(insn->ea, &fp);
    uint32 Feature = insn->get_canon_feature(ph);    //get instruction's CF_XX flags

	flags_t F = get_flags(insn->ea);
	if (Feature & CF_USE1) handle_operand(insn, &insn->Op1, true);
	if (Feature & CF_USE2) handle_operand(insn, &insn->Op2, true);
	if (Feature & CF_USE3) handle_operand(insn, &insn->Op3, true);
													      
	if (Feature & CF_CHG1) handle_operand(insn, &insn->Op1, false);
	if (Feature & CF_CHG2) handle_operand(insn, &insn->Op2, false);
	if (Feature & CF_CHG3) handle_operand(insn, &insn->Op3, false);

	if ((Feature & CF_STOP) == 0)
		add_cref(insn->ea, insn->ea + insn->size, fl_F);	//相邻指令引用

	return 1;
}