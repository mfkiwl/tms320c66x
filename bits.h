#pragma once
#ifndef __BITS_H
#define __BITS_H
#include <stdint.h>

//scts: signed const
//ucts: unsigned const
//indent: equal to left shift
int32_t check_scts(int32_t code, int len);
int32_t bits_scst(uint32_t code, int pos, int len, int indent = 0);
uint32_t bits_ucst(uint32_t code, int pos, int len, int indent = 0);
bool bits_check(uint32_t code, int pos);
//combine bits as v1||v2||v3 and vx is one bit
uint32_t bits_combine_111(int32_t v1, int32_t v2, int32_t v3);

#endif