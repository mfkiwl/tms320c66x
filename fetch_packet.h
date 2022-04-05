#pragma once
#ifndef __FETCH_PACKET_H
#define __FETCH_PACKET_H
#include <stdint.h>
#include <idp.hpp>

typedef struct fp_header_t
{
    uint32_t adr;
    uint8_t magic;
    uint8_t layout;

    uint8_t prot;
    uint8_t rs;
    /*
    d s z
    0 0 0 W BU
    0 0 1 W B
    0 1 0 W HU
    0 1 1 W H
    1 0 0 DW / NDW W
    1 0 1 DW / NDW B
    1 1 0 DW / NDW NW
    1 1 1 DW / NDW H
    */
    uint8_t dsz;
    uint8_t br;
    uint8_t sat;

    uint16_t p_bits;
}fp_header_t;

enum FETCH_PACKET_OPCODE_TYPE
{
    OPCODE_TYPE_INVALID,
    OPCODE_TYPE_32_BIT,
    OPCODE_TYPE_16_BIT,
    OPCODE_TYPE_HEADER
};

typedef struct fetch_packet_t
{
    uint32_t start;
    uint32_t end;

    int opcode[8];

    bool fph_vaild;
    fp_header_t fph;
}fectch_paket_t;

typedef uint32_t adr_t;

int get_ins_type(adr_t adr, fetch_packet_t* fp);
bool is_in_fetch_packet(adr_t adr, fetch_packet_t* fp);
void update_fetch_packet(adr_t adr, fetch_packet_t* fp);
void printf_fetch_packet(fetch_packet_t* fp);

#endif