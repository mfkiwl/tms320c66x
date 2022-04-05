#pragma once
#ifndef __VALUE_FILTER_H
#define __VALUE_FILTER_H
#include <stdint.h>

typedef struct filter_table_t
{
	char* format;
	int  type;
	uint32_t mask[2];
	int weight;
}filter_table_t;

#define ERROR_TYPE -1

int filter(filter_table_t* ftbl, int ftbl_size, uint32_t val);
void check_unique(filter_table_t* ftbl, int ftbl_size);

#endif