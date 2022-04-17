#include "fetch_packet.h"
#include "tms66x.h"
#include <vector>

static std::vector<ea_t> g_func_start;

static bool vector_contain(std::vector<ea_t> *v, ea_t addr)
{
    std::vector<ea_t>::iterator it = v->begin();
    while (it != v->end())
    {
        if (addr == *it)
            return true;
        it++;
    }
    return false;
}

static void vector_append(std::vector<ea_t>* v, ea_t addr)
{
    if (vector_contain(v, addr) == false)
        v->push_back(addr);
}

/* 
    处理数据引用
    如下所示，应该添加一个0x8F3CE4的数据引用
00036E70   021E722A           MVK.S2        0x3ce4,B4
00036E74   020047EA           MVKH.S2       0x8f0000,B4 ;B4=0x8f3ce4
*/
static void data_quote(const insn_t* insn, const op_t* x)
{

}

//在发生分支的地址后面delay slot内寻找是否有addkpc xx, B3, x操作，仅在delay slot没有另外跳转函数时
static bool check_func_1(const insn_t* insn)
{
    insn_t next_ins;
    ea_t next_adr = insn->ea + insn->size;
    int loop = insn->itype == TMS6_bnop ? 5 - insn->Op2.value : 5;

    //先执行完跳转指令所在的执行包
    //if (insn->cflags & aux_para)
    //{
    //    while (1)
    //    {
    //        if (decode_insn(&next_ins, next_adr) == 0)
    //            return false;
    //        next_adr += insn->size;

    //        //fph则顺延
    //        if (insn->cflags & aux_fph)
    //            continue;
    //        if ((insn->cflags & aux_para) == 0)
    //            break;
    //    }
    //}

    while (loop > 0)
    {
        if (decode_insn(&next_ins, next_adr) == 0)
            return false;
        next_adr = next_ins.ea + next_ins.size;

        if (next_ins.cflags & aux_fph)
            continue;

        if ((next_ins.cflags & aux_para) == 0)
            loop -= 1;          
        if (next_ins.itype == TMS6_nop)
            loop -= (next_ins.Op1.value - 1);    
        //有可能函数和分支混在一起，该判断不应该存在
        //if (next_ins.itype == TMS6_bnop || next_ins.itype == TMS6_b)
        //    return false;
        if (next_ins.itype == TMS6_addkpc && next_ins.Op2.reg == rB3)
            return true;
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
        vector_append(&g_func_start, insn->Op1.addr);
        return 1;
    }

    if (insn->itype == TMS6_bnop || insn->itype == TMS6_b)
    {
		//is_func(get_flags(insn->Op1.addr))在没有真正makefunction时无用
        if (is_func(get_flags(insn->Op1.addr)) || vector_contain(&g_func_start, insn->Op1.addr))
        {
            insn->add_cref(insn->Op1.addr, insn->Op1.offb, fl_CN);
            return 1;
        }

        bool check1, check2;

        check1 = check_func_1(insn);
        check2 = check_func_2(insn);

        if (check1 || check2)
        {
            insn->add_cref(insn->Op1.addr, insn->Op1.offb, fl_CN);
            vector_append(&g_func_start, insn->Op1.addr);
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
        if (code_quote(insn) == 0)
        {
            if (insn->Op1.addr == 0x39080)
                msg("%X add jump -> %X\n", insn->ea, insn->Op1.addr);
            if (insn->itype != TMS6_addkpc)
                insn->add_cref(x->addr, x->offb, fl_JN);
        }
        break;
    }
}

int idaapi emu(const insn_t* insn)
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

//判断指令是否仅为了对齐
int idaapi is_align_insn(ea_t ea)
{
    insn_t insn;
    decode_insn(&insn, ea);
    switch (insn.itype)
    {
    case TMS6_mv:
        if (insn.Op1.reg == insn.Op2.reg)
            break;
    case TMS6_nop:
        break;
    default:
        return 0;
    }
    return insn.size;
}

static ea_t skip_delay_slot(insn_t *insn)
{
    int delay = insn->itype == TMS6_bnop ? 5 - insn->Op2.value : 5;

    //先执行完跳转指令所在的执行包
    ea_t ea = insn->ea + insn->size;
    if (insn->cflags & aux_para)
    {
        while (1)
        {
            if (decode_insn(insn, ea) == 0)
                return BADADDR;
            ea += insn->size;

            //fph则顺延
            if (insn->cflags & aux_fph)
                continue;
            if ((insn->cflags & aux_para) == 0)
                break;
        }
    }

    //跳过delay slot的执行包
    while (delay > 0)
    {
        if (decode_insn(insn, ea) == 0)
            return BADADDR;
        ea += insn->size;

        //fph则顺延
        if (insn->cflags & aux_fph)
            continue;
        if ((insn->cflags & aux_para) == 0)
            delay -= 1;
        if (insn->itype == TMS6_nop)
            delay -= (insn->Op1.value - 1); 
    }

    return ea;
}

static bool check_branch(ea_t ea, ea_t min_ea, ea_t max_ea)
{
    bool loop = false;
    insn_t insn;

    do
    {
        loop = false;

        //若为函数结尾，此时ea不为函数开头
        if (is_func(get_flags(ea)) || vector_contain(&g_func_start, ea))
            return false;

        ea_t q_adr = get_first_fcref_from(ea);
        //若adr有引用它的地址，并且该地址在函数范围内，则它是函数的分支
        if (q_adr >= min_ea && q_adr <= max_ea)
            return true;

        if (decode_insn(&insn, ea) == 0)
            return false;
        ea += insn.size;
        //fph则顺延
        if (insn.cflags & aux_fph)
            continue;
        //如果是填充指令则顺延
        if (insn.itype == TMS6_nop)
            loop = true;

    } while (loop);

    return false;
}

//确定函数的结尾
void idaapi check_func_bounds(int* possible_return_code, func_t* pfn, ea_t max_func_end_ea)
{
    //msg("[func_bounds]: %X-%X %X %X\n", pfn->start_ea, pfn->end_ea, *possible_return_code, max_func_end_ea);

    if (pfn != NULL)
    {
        insn_t insn;
        ea_t ea = pfn->start_ea;
        while (ea < pfn->end_ea)
        {
            if (decode_insn(&insn, ea) == 0) 
                break;
            ea += insn.size;

            //寻找B B3或BNOP B3,X
            if ((insn.itype == TMS6_b || insn.itype == TMS6_bnop) && insn.Op1.reg == rB3)
            {
                //跳过delay slot
                ea = skip_delay_slot(&insn);
                if (ea == BADADDR || ea > max_func_end_ea)
                    return;

                if (check_branch(ea, pfn->start_ea, max_func_end_ea) == false)
                {
                    pfn->end_ea = ea;
                    return;
                }
            }
        }
    }
}