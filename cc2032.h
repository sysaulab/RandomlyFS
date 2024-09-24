#ifndef LIBSEEDY_CC2032_H_
#define LIBSEEDY_CC2032_H_

/*
 *   https://cr.yp.to/streamciphers/timings/estreambench/submissions/salsa20/chacha8/ref/chacha.c
 */

#include <libseedy/common.h>
#include <string.h>
#define CHACHA20_IMPLEMENTATION
#include "ChaCha20/ChaCha20.h"

typedef struct CC2032_s
{
    void* feeder;
    ChaCha20_Ctx ctx;
	size_t step;
	uint8_t next[64];
	int used;
}
CC2032;

void cc2032_init(CC2032* s, void* f)
{
    s->feeder = f;
    s->used = 64;
}

void cc2032_refill(CC2032* q)
{
    if( (q->step&0xffffFFFFffffFFFF) == 0 ) /* reseed at the end of the period, 2^70 bytes for chacha8, asssuming equal or better here.  */
    {
        key256_t key;
        nonce96_t nonce;
        uint32_t count = 0x00000001;
        ((void(*)(uint8_t* b, uint64_t n))q->feeder)((uint8_t*)&key, sizeof(key));
        ((void(*)(uint8_t* b, uint64_t n))q->feeder)((uint8_t*)&nonce, sizeof(nonce));
        ChaCha20_init(&q->ctx, key, nonce, count);
    }
    memset(q->next, 0, sizeof(q->next));
	ChaCha20_xor(&q->ctx, q->next, sizeof(q->next));
	q->used = 0;
    q->step++;
}

void cc2032_fill(CC2032* s, uint8_t* b, size_t n)
{
	size_t requested = n;
	size_t chunk_size;
    size_t written = 0;
    size_t available = 0;
    size_t needed = 0;
    while(written < requested) /* There is work to do */
    {
        available = sizeof(s->next) - s->used;
        needed = n - written;
        if ( available == 0 )
		{
			cc2032_refill(s);
		}
        chunk_size = MIN(available, needed); /* How many bytes are available? How many do I need? Return the smallest. It is the size we can copy now...*/
        memcpy(&b[written], &s->next[s->used], chunk_size); /* copy what we need for now */
        written += chunk_size; /* Increase our position by the amount written */
        s->used += chunk_size; /* Increase our consumption by the amount written */
    }
}

#endif


