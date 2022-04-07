#include "tms66x.h"
#include "ua.hpp"

class out_tms320c66x_t : public outctx_t
{
	out_tms320c66x_t(void) = delete; // not used
public:
	bool out_operand(const op_t& x);
	void out_insn(void);
	void outreg(int r) { out_register(ph.reg_names[r]); }
};
CASSERT(sizeof(out_tms320c66x_t) == sizeof(outctx_t));

//¶¨Òåout_insnºÍout_opnd
DECLARE_OUT_FUNCS_WITHOUT_OUTMNEM(out_tms320c66x_t)

void idaapi header(outctx_t* ctx)
{
	ctx->gen_header(GH_PRINT_ALL);
}

void out_tms320c66x_t::out_insn(void)
{
	if (this->insn.cflags & aux_fph)
	{
		this->out_line("  fetch packet header  ");
		this->flush_outbuf();
	}
}
