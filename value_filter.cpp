#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "value_filter.h"

static int sorted_by_weight(filter_table_t *ftbl, int ftbl_size)
{
	int i, j;
	
	for(i=0;i<ftbl_size;i++)
	{
		int max_weight = ftbl[i].weight;
		int max_idx = i;
		for(j=i+1;j<ftbl_size;j++)
		{
			if(ftbl[j].weight > max_weight)
			{
				max_weight = ftbl[j].weight;
				max_idx = j;
			}
		}
		//swap
		if(i != max_idx)
		{
			filter_table_t tmp;
			memcpy(&tmp, &ftbl[i], sizeof(filter_table_t));
			memcpy(&ftbl[i], &ftbl[max_idx], sizeof(filter_table_t));
			memcpy(&ftbl[max_idx], &tmp, sizeof(filter_table_t));
		}
	}
	return 1;
}

static int init_filter_table(filter_table_t *ftbl, int ftbl_size)
{
	int i, j, k;

	for(i=0;i<ftbl_size;i++)
	{
		int weight = 0;
		char *fmt_str = ftbl[i].format;
		for(j=0;j<2;j++)
		{
			char ch = j + '0';
			uint32_t mask = 0;
			//for(k=strlen(fmt_str)-1;k>=0;k--)
			for(k=0;k<strlen(fmt_str);k++)
			{
				mask <<= 1;
				if(fmt_str[k] == ch)
				{
					mask |= 1;
					weight++;
				}
			}
			ftbl[i].mask[j] = mask;
		}
		ftbl[i].weight = weight;
	}
	
//	for(i=0;i<ftbl_size;i++)
//	{
//		printf("%d:%X %X\n",ftbl[i].type,ftbl[i].mask[0],ftbl[i].mask[1]);
//	}
	
	return sorted_by_weight(ftbl, ftbl_size);
//	return 1;
}

int filter(filter_table_t *ftbl, int ftbl_size, uint32_t val)
{
	int i;
	
	if(ftbl == NULL || ftbl_size <= 0)
		return ERROR_TYPE;
		
	if(ftbl[0].weight == 0)
		init_filter_table(ftbl, ftbl_size);

	for(i=0;i<ftbl_size;i++)
	{
		if((~val & ftbl[i].mask[0]) == ftbl[i].mask[0]
		&& (val & ftbl[i].mask[1]) == ftbl[i].mask[1])
		{
			return ftbl[i].type;
		}
	}
	return ERROR_TYPE;
}

void check_unique(filter_table_t* ftbl, int ftbl_size)
{
    int i, j, k;

    for (i = 0; i < ftbl_size; i++)
    {
        char* fmt_str = ftbl[i].format;
        for (j = i + 1; j < ftbl_size; j++)
        {
            char* fmt_str2 = ftbl[j].format;
            bool unique = false;
            for (k = 0; k < strlen(fmt_str); k++)
            {
                if ((fmt_str[k] ^ fmt_str2[k]) == 0x1)
                {
                    unique = true;
                    break;
                }
            }
            if (unique == false)
            {
                printf("**************************\n");
                printf("%03d: %s\n", ftbl[i].type, fmt_str);
                printf("%03d: %s\n", ftbl[j].type, fmt_str2);
            }
        }
    }
}

#if 0
enum TMSC6L_INS
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

static filter_table_t g_tms66x_cpt_ins[49] =
{
	//dsz br sat | ins
	// 3  1   1  |  16
	//D unit 0-13
	{ "..........0.......10.", TMSC6L_Doff4 },
	{ "1.........0.......10.", TMSC6L_Doff4DW },
	{ ".........01.......10.", TMSC6L_Dind },
	{ "1........01.......10.", TMSC6L_DindDw },
	{ ".....00..11.......10.", TMSC6L_Dinc },
	{ "1....00..11.......10.", TMSC6L_DincDw },
	{ ".....01..11.......10.", TMSC6L_Ddec },
	{ "1....01..11.......10.", TMSC6L_DdecDw },
	{ ".....1...11.......10.", TMSC6L_Dstk },
	{ "..........0...011011.", TMSC6L_Dx2op },
	{ "..........1...011011.", TMSC6L_Dx5 },
	{ "........011...1110111", TMSC6L_Dx5p },
	{ "........110...111011.", TMSC6L_Dx1 },
	{ ".........0....1110111", TMSC6L_Dpp },
	//L unit 14-21
	{ "..........0......000.", TMSC6L_L3 },
	{ "..........1......000.", TMSC6L_L3i },
	{ "..........0......100.", TMSC6L_Ltbd },
	{ "..........1......100.", TMSC6L_L2c },
	{ "..........1...010011.", TMSC6L_Lx5 },
	{ "........0.0...010011.", TMSC6L_Lx3c },
	{ "........1.0...010011.", TMSC6L_Lx1c },
	{ "........110...110011.", TMSC6L_Lx1 },
	//M unit 22
	{ "................1111.", TMSC6L_M3 },
	//S unit 23-38
	{ "...1...........00101.", TMSC6L_Sbs7 },
	{ "...1.11........00101.", TMSC6L_Sbu8 },
	{ "...1...........01101.", TMSC6L_Scs10 },
	{ "...1...........1.101.", TMSC6L_Sbs7c },
	{ "...1.11........1.101.", TMSC6L_Sbu8c },
	{ "...0......0......101.", TMSC6L_S3 },
	{ "...0......1......101.", TMSC6L_S3i },
	{ "................1001.", TMSC6L_Smvk8 },
	{ "..........1.....0001.", TMSC6L_Ssh5 },
	{ "..........1...110001.", TMSC6L_S2sh },
	{ "..........0.....0001.", TMSC6L_Sc5 },
	{ "..........0...110001.", TMSC6L_S2ext },
	{ "..........0...010111.", TMSC6L_Sx2op },
	{ "..........1...010111.", TMSC6L_Sx5 },
	{ "........110...110111.", TMSC6L_Sx1 },
	{ "........00....110111.", TMSC6L_Sx1b },
	//DLS unit 39-42
	{ "..............00..11.", TMSC6L_LSDmvto },
	{ "..............10..11.", TMSC6L_LSDmvfr },
	{ "........010...11..11.", TMSC6L_LSDx1c },
	{ "........110...11..11.", TMSC6L_LSDx1 },
	//No unit 43-48
	{ ".....0.0011...110011.", TMSC6L_Uspl },
	{ ".....1.0011...110011.", TMSC6L_Uspldr },
	{ ".......0111...110011.", TMSC6L_Uspk },
	{ ".......1011...110011.", TMSC6L_Uspm_spmask },
	{ ".......1111...110011.", TMSC6L_Uspm_spmaskr },
	{ "........0110001101110", TMSC6L_Unop}
};

int main()
{
	int i;
	srand(0);
	check_unique(g_tms66x_cpt_ins, 49);

	int test[] = { 0x8646, 0x2577, 0x104F, 0x8677 };
	//	printf("val -> type\n");
	//	for(i=0;i<sizeof(test)/sizeof(int);i++)
	//	{
	//		int val = test[i];
	//		int type = filter(g_TMSC6L_fmt_ins, 49, val);
	//		printf("%X -> %d\n",val, type);
	//	}

	return 0;
}
#endif

