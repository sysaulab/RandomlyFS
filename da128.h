#ifndef LIBSEEDY_da128_H_
#define LIBSEEDY_da128_H_

#include "libseedy/common.h"
#include "libseedy/libseedy64.h"
#include "cc2032.h"

#include <stdio.h>

/*
 *   NM64 (NoiseMap, 64 bits addressable)
 */

typedef uint64_t DA128[8][65536];

void da128_init(DA128 state)
{
    CC2032 pre_rng;
    cc2032_init(&pre_rng, (void*)&seedy64);
    cc2032_fill(&pre_rng, (uint8_t*)state, sizeof(DA128));

}



uint64_t da128_block(DA128 state, __uint128_t offset)
{
    __uint128_t prime = ( (__uint128_t)0x172489081F1CA0E5 << 64 ) | 0xFDE26955058E76C5;

    offset = (offset >> 3) * prime;

    uint16_t pos0 = (uint16_t)(offset >> 0) & 0xfff8;
    uint16_t pos1 = (uint16_t)(offset >> 16);
    uint16_t pos2 = (uint16_t)(offset >> 32);
    uint16_t pos3 = (uint16_t)(offset >> 48);
    uint16_t pos4 = (uint16_t)(offset >> 64);
    uint16_t pos5 = (uint16_t)(offset >> 80);
    uint16_t pos6 = (uint16_t)(offset >> 96);
    uint16_t pos7 = (uint16_t)(offset >>112);

    return 
    ROT64(state[0][ pos0 ], pos7 % 64 ) ^ 
    ROT64(state[1][ pos1 ], pos6 % 64 ) ^ 
    ROT64(state[2][ pos2 ], pos5 % 64 ) ^ 
    ROT64(state[3][ pos3 ], pos4 % 64 ) ^ 
    ROT64(state[4][ pos4 ], pos3 % 64 ) ^ 
    ROT64(state[5][ pos5 ], pos2 % 64 ) ^ 
    ROT64(state[6][ pos6 ], pos1 % 64 ) ^ 
    ROT64(state[7][ pos7 ], pos0 % 64 ) ;
}

void da128_fill(DA128 state, uint8_t* buf, off_t bytes, __uint128_t offset)
{

    size_t done = 0;
    uint64_t next = 0;

    int pre_block = offset & 8;

    next = da128_block(state, offset);
    while( (offset % 8) && (done < bytes) )
    {
        buf[done] = ((uint8_t*)&next)[(offset % 8)];
        done++;
        offset++;
    }

    while( (bytes - done) / 8 )
    {
        *(uint64_t*)(&buf[done]) = da128_block( state, offset );
        done = done + 8;
        offset = offset + 8;
    }

    next = da128_block( state, offset );
    while( bytes - done )
    {
        buf[done] = ((uint8_t*)&next)[(offset % 8)];
        done++;
        offset++;
    }

}



#endif
