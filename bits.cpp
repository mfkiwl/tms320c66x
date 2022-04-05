#include "bits.h"

int32_t check_scts(int32_t code, int len)
{
	if (bits_check(code, len - 1))
		code -= (1 << len);
	return code;
}

int32_t bits_scst(uint32_t code, int pos, int len, int indent)
{
	return check_scts(bits_ucst(code, pos, len, indent), len);
}

uint32_t bits_ucst(uint32_t code, int pos, int len, int indent)
{
	uint32_t mask = (1 << len) - 1;
	return ((code >> pos) & mask) << indent;
}

bool bits_check(uint32_t code, int pos)
{
	return (code & (1 << pos));
}

uint32_t bits_combine_111(int32_t v1, int32_t v2, int32_t v3)
{
	return ((v1 & 1) << 2) | ((v2 & 1) << 1) | (v3 & 1);
}