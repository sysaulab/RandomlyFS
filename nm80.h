#ifndef LIBSEEDY_nm80_H_
#define LIBSEEDY_nm80_H_

#include "libseedy/common.h"
#include "libseedy/libseedy64.h"
#include "prime64sub16.h"
#include "cc2032.h"

#include <stdio.h>
#include <string.h>
#include <unistd.h>
/*#include <sqlite3.h>*/


#if defined(__GNUC__)
typedef unsigned __int128 uint128_t;
typedef __int128 int128_t;
#else
#   error "Unsupported compiler"
#endif /* defined(__GNUC__) */

/*
 *   NM80 (state->NoiseMap, 64 bits addressable)
 */

typedef struct NM80_s 
{
    char path[1024];
    uint64_t iters[8192];
    uint64_t noise[4][65536];
}
NM80;

int nm80_read(NM80* state)
{
    FILE* store = fopen(state->path, "r");
    if (store == NULL)
        return -1;

    fseek(store, 0L, SEEK_END);
    if (ftell(store) == sizeof(NM80))
    {
        rewind(store);
        if ( 1 != fread(state, sizeof(NM80), 1, store) ) 
            return -2;
    }
    else
        return -3;
    fclose(store);
    return 0;
}

int nm80_write(NM80* state)
{
    FILE* store = fopen(state->path, "w");
    if (store == NULL)
        return -1;

    if ( 1 != fwrite(state, sizeof(NM80), 1, store) ) 
        return -2;

    fclose(store);
    return 0;
}

int nm80_new(NM80* state)
{
        CC2032 seed_gen;
        cc2032_init( &seed_gen, (void*)seedy64 );
        cc2032_fill( &seed_gen, (uint8_t*) state->noise, sizeof(state->noise) );
        memset( state->iters, 0xffffffff, sizeof(state->iters) );
        return 0;
}

uint128_t hexU128(char* hext)
{
    uint128_t result = 0;
    int done = 0;
    for(int pos = 0; done == 0 && pos < 32; pos++)
    {
        switch (hext[pos])
        {
            case '0':
                result = (result << 4) | 0;
                break;
            case '1':
                result = (result << 4) | 1;
                break;
            case '2':
                result = (result << 4) | 2;
                break;
            case '3':
                result = (result << 4) | 3;
                break;
            case '4':
                result = (result << 4) | 4;
                break;
            case '5':
                result = (result << 4) | 5;
                break;
            case '6':
                result = (result << 4) | 6;
                break;
            case '7':
                result = (result << 4) | 7;
                break;
            case '8':
                result = (result << 4) | 8;
                break;
            case '9':
                result = (result << 4) | 9;
                break;
            case 'a':
            case 'A':
                result = (result << 4) | 10;
                break;
            case 'b':
            case 'B':
                result = (result << 4) | 11;
                break;
            case 'c':
            case 'C':
                result = (result << 4) | 12;
                break;
            case 'd':
            case 'D':
                result = (result << 4) | 13;
                break;
            case 'e':
            case 'E':
                result = (result << 4) | 14;
                break;
            case 'f':
            case 'F':
                result = (result << 4) | 15;
                break;
            default:
                done = 1;
        }
    }
    return result;
}

#define GETBANK(i) ((i>>67)&0x1FFF)
#define GETSEG(i) ((i>>3)&0xffffFFFFffffFFFF)


int nm80_init(NM80* state, char* path)
{
fprintf(stderr, "nm80_init");
    strncpy(state->path, path, 1023);
    if(nm80_read(state))
    {
fprintf(stderr, "nm80_init -- READ ERROR, MAKE NEW ONE");
        nm80_new(state);
        strncpy(state->path, path, 1023);
        if(nm80_write(state))
        {
fprintf(stderr, "nm80_init -- WRITE ERROR ???");
            return -1;
        }
    }
fprintf(stderr, "nm80_init -- READ IS GOOD");
    return 0;
}

uint64_t __nm80_block(NM80* state, uint64_t segment, uint64_t iter_bank)
{
    uint64_t pos64;
    uint16_t* pos16 = (uint16_t*) & segment;

    if ( state->iters[iter_bank] == 0xffffFFFFffffFFFF )
    {
        /*
         *   Lazy Initialisation, it takes some time making a unique iterator. 
         *   Making them as needed avoid spending an hour finding prime numbers.
         */
        state->iters[iter_bank] = prime64sub16();
        nm80_write(state);
    }

    if ( segment == 0 )
    {
        /* To avoid having all index zero being the same, I redefine index zero as the binary not of the iterator.*/
        segment = ~state->iters[iter_bank];
    }

    pos64 = segment * state->iters[iter_bank];

    return  ( state->noise[0][pos16[0]] ) ^
            ( state->noise[1][pos16[1]] ) ^
            ( state->noise[2][pos16[2]] ) ^
            ( state->noise[3][pos16[3]] ) ;
}

void nm80_fill(NM80* s, uint8_t* buf, size_t num, uint128_t user_offset)
{
    size_t t = 0;
    uint64_t next = 0;

    next = __nm80_block(s, GETSEG(user_offset), GETBANK(user_offset));
    while( (user_offset % 8) && (t < num) )
    {
        buf[t] = ((uint8_t*)&next)[(user_offset % 8)];
        t++;
        user_offset++;
    }

    while( (num - t) / 8 )
    {
        *(uint64_t*)(&buf[t]) = __nm80_block(s, GETSEG(user_offset), GETBANK(user_offset));
        t = t + 8;
        user_offset = user_offset + 8;
    }

    next = __nm80_block(s, GETSEG(user_offset), GETBANK(user_offset));
    while( num - t )
    {
        buf[t] = ((uint8_t*)&next)[(user_offset % 8)];
        t++;
        user_offset++;
    }
}

#endif
