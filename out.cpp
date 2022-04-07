#include "tms66x.h"
#include "ua.hpp"

int idaapi out(outctx_t* ctx)
{
	if (ctx->insn.cflags & aux_fph)
	{
		ctx->out_line("  fetch packet header  ");
		ctx->flush_outbuf();
		return 1;
	}
	return 0;
}

void idaapi header(outctx_t* ctx)
{
	ctx->gen_header(GH_PRINT_ALL);
}

void idaapi out_insn(outctx_t* ctx)
{

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