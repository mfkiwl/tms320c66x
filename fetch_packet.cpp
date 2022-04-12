#include "fetch_packet.h"
#include <bytes.hpp>
#include "bits.h"

int get_ins_type(adr_t adr, fetch_packet_t* fp)
{
    int idx = (adr - fp->start) >> 2;
    if (idx >= 0 && idx < 8)
    {
        return fp->opcode[idx];
    }
    return OPCODE_TYPE_INVALID;
}

bool is_in_fetch_packet(adr_t adr, fetch_packet_t* fp)
{
    return (adr < fp->end) && (adr >= fp->start);
}

static void get_fetch_packet_header(adr_t fph_adr,  uint32_t fph_code, fp_header_t* fph)
{
    fph->adr = fph_adr;
    fph->magic = bits_ucst(fph_code, 28, 4);    //(fph_code >> 28) & 0xF;
    fph->layout = bits_ucst(fph_code, 21, 7);   //(fph_code >> 21) & 0x7F;
    fph->prot = bits_ucst(fph_code, 20, 1);     //(fph_code >> 20) & 1;
    fph->rs = bits_ucst(fph_code, 19, 1);       //(fph_code >> 19) & 1;
    fph->dsz = bits_ucst(fph_code, 16, 3);      //(fph_code >> 16) & 7;
    fph->br = bits_ucst(fph_code, 15, 1);       //(fph_code >> 15) & 1;
    fph->sat = bits_ucst(fph_code, 14, 1);      //(fph_code >> 14) & 1;
    fph->p_bits = bits_ucst(fph_code, 0, 14);   //fph_code & 0x3FFF;
}

void update_fetch_packet(adr_t adr, fetch_packet_t* fp)
{
    adr_t fp_start = adr & (~0x1F);
    adr_t fph_adr = fp_start + 28;
    uint32_t code = get_dword(fph_adr);

    if (((code >> 28) & 0xF) == 0xE)
    {
        get_fetch_packet_header(fph_adr, code, &fp->fph);
        fp->fph_vaild = true;
        fp->opcode[0] = (fp->fph.layout & 1) ? OPCODE_TYPE_16_BIT: OPCODE_TYPE_32_BIT;
        fp->opcode[1] = (fp->fph.layout & 2) ? OPCODE_TYPE_16_BIT : OPCODE_TYPE_32_BIT;
        fp->opcode[2] = (fp->fph.layout & 4) ? OPCODE_TYPE_16_BIT : OPCODE_TYPE_32_BIT;
        fp->opcode[3] = (fp->fph.layout & 8) ? OPCODE_TYPE_16_BIT : OPCODE_TYPE_32_BIT;
        fp->opcode[4] = (fp->fph.layout & 16) ? OPCODE_TYPE_16_BIT : OPCODE_TYPE_32_BIT;
        fp->opcode[5] = (fp->fph.layout & 32) ? OPCODE_TYPE_16_BIT : OPCODE_TYPE_32_BIT;
        fp->opcode[6] = (fp->fph.layout & 64) ? OPCODE_TYPE_16_BIT : OPCODE_TYPE_32_BIT;
        fp->opcode[7] = OPCODE_TYPE_HEADER;
    }
    else
    {
        fp->fph_vaild = false;
        int i;
        for (i = 0; i < 8; i++)
            fp->opcode[i] = OPCODE_TYPE_32_BIT;

    }
    fp->start = fp_start;
    fp->end = fp_start + 32;
}

void printf_fetch_packet(fetch_packet_t* fp)
{
    int i;

    msg("fp start: 0x%X\n", fp->start);
    msg("  fp end: 0x%X\n", fp->end);
    for (i = 0; i < 8; i++)
    {
        switch (fp->opcode[i])
        {
        case OPCODE_TYPE_INVALID:
            msg("ins type: %s", "    invalid type     \n");
            break;
        case OPCODE_TYPE_32_BIT:
            msg("ins type: %s", "     32bit ins       \n");
            break;
        case OPCODE_TYPE_16_BIT:
            msg("ins type: %s", "16bit ins | 16bit ins\n");
            break;
        case OPCODE_TYPE_HEADER:
            msg("ins type: %s", " fetch packet header \n");
            break;
        }
    }
    if (fp->fph_vaild)
    {
        msg(" fph magic: 0x%X\n", fp->fph.magic);
        msg("fph layout: 0x%X\n", fp->fph.layout);
        msg("  fph prot: 0x%X\n", fp->fph.prot);
        msg("    fph rs: 0x%X\n", fp->fph.rs);
        msg("   fph dsz: 0x%X\n", fp->fph.dsz);
        msg("    fph br: 0x%X\n", fp->fph.br);
        msg("   fph sat: 0x%X\n", fp->fph.sat);
        msg("fph p_bits: 0x%X\n", fp->fph.p_bits);
    }
    
}