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

}

void idaapi out_insn(outctx_t* ctx)
{

}

void idaapi footer(outctx_t* ctx)
{

}