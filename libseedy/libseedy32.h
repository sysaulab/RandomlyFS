#ifndef __SEEDY32_H_
#define __SEEDY32_H_

#include "common.h"

typedef struct seed_thread32_s{
    volatile uint32_t *source;
    volatile uint32_t *sink;
    int run;
/*
 *   START ##### PORTABLE BLOCK ##### START
 */
#   if defined(_WIN32)
        HANDLE thr;

#   elif defined(__HAIKU__)
        thread_id thr;

#   else
        pthread_t thr;

#   endif
/*
 *   END ##### PORTABLE BLOCK ##### END
 */
} seed_thread_32;

typedef struct seed_state32_s{
    volatile uint32_t nodes[3];
    seed_thread_32 threads[3];
} SEEDY32;

uint32_t read_state_32(SEEDY32* state)
{
    return state->nodes[0] ^ state->nodes[1] ^ state->nodes[2];
}

int seed_modify_32( volatile uint32_t* in, volatile uint32_t* out )
{
    uint32_t acc = 1;
    uint32_t primes[] = 
{268435459,
268435463,
268435493,
268435523,
268435537,
268435561,
268435577,
268435579,
268435597,
268435631,
268435639,
268435649,
268435669,
268435697,
268435711,
268435723,
268435747,
268435751,
268435757,
268435813,
268435823,
268435873,
268435879,
268435889,
268435961,
268435987,
268436011,
268436041,
268436053,
268436071,
268436081,
268436087,
268436089,
268436141,
268436167,
268436177,
268436191,
268436219,
268436249,
268436261,
268436263,
268436269,
268436279,
268436281,
268436291,
268436327,
268436407,
268436419,
268436429,
268436431,
268436453,
268436507,
268436563,
268436629,
268436653,
268436669,
268436671,
268436677,
268436687,
268436699,
268436741,
268436743,
268436747,
268436759}
    ;

    int x = 0;
    while( x < 4 * 8 ) {
        *in = (*in << 7) | (*in >> ((4 * 8) - 7));
        acc = (acc << x) | (acc >> ((4 * 8) - x));
        acc *= primes[(2 * x) + (1 & *in)];
        *out += acc ^ *in;
        x++;
    }
    *out ^= acc;
    return 0;
}

void* seed_thread_main_32(void* raw)
{
    seed_thread_32* state = (seed_thread_32*) raw;
    while(1)
    {
        seed_modify_32(state->source, state->sink);
        if(state->run == 0)
        {
            return NULL;
        }
    }
    return NULL;
}

void start_seeder_32(SEEDY32* state)
{
    state->nodes[0] = 0;
    state->nodes[1] = 0;
    state->nodes[2] = 0;
    state->threads[0].source = &(state->nodes[0]);
    state->threads[1].source = &(state->nodes[1]);
    state->threads[2].source = &(state->nodes[2]);
    state->threads[0].sink = &(state->nodes[1]);
    state->threads[1].sink = &(state->nodes[2]);
    state->threads[2].sink = &(state->nodes[0]);
    state->threads[0].run = 1;
    state->threads[1].run = 1;
    state->threads[2].run = 1;

/*
 *   START ##### PORTABLE BLOCK ##### START
 */
     #if defined(_WIN32)
        state->threads[0].thr = CreateThread((LPSECURITY_ATTRIBUTES)NULL, 0, (LPTHREAD_START_ROUTINE)&seed_thread_main_32, &(state->threads[0]), (DWORD)0, NULL);
        state->threads[1].thr = CreateThread((LPSECURITY_ATTRIBUTES)NULL, 0, (LPTHREAD_START_ROUTINE)&seed_thread_main_32, &(state->threads[1]), (DWORD)0, NULL);
        state->threads[2].thr = CreateThread((LPSECURITY_ATTRIBUTES)NULL, 0, (LPTHREAD_START_ROUTINE)&seed_thread_main_32, &(state->threads[2]), (DWORD)0, NULL);
    
    #elif defined(__HAIKU__)
        state->threads[0].thr = spawn_thread((int(*)(void*))&seed_thread_main_32, "Seedy Accountant Number A", B_NORMAL_PRIORITY, &(state->threads[0]));
        state->threads[1].thr = spawn_thread((int(*)(void*))&seed_thread_main_32, "Seedy Accountant Number B", B_NORMAL_PRIORITY, &(state->threads[1]));
        state->threads[2].thr = spawn_thread((int(*)(void*))&seed_thread_main_32, "Seedy Accountant Number C", B_NORMAL_PRIORITY, &(state->threads[2]));
        if ( B_OK != 
            ( resume_thread(state->threads[0].thr ) ) &
            ( resume_thread(state->threads[1].thr ) ) &
            ( resume_thread(state->threads[2].thr ) ) )
        {
            fprintf(stderr, "ERROR resuming threads.\n");
            exit(0);
        }

    #else /* POSIX */
        pthread_create(&(state->threads[0].thr), NULL, &seed_thread_main_32, &(state->threads[0]));
        pthread_create(&(state->threads[1].thr), NULL, &seed_thread_main_32, &(state->threads[1]));
        pthread_create(&(state->threads[2].thr), NULL, &seed_thread_main_32, &(state->threads[2]));

    #endif
/*
 *   END ##### PORTABLE BLOCK ##### END
 */
}

void stop_seeder_32(SEEDY32* state)
{
    state->threads[0].run = 0;
    state->threads[1].run = 0;
    state->threads[2].run = 0;
/*
 *   START ##### PORTABLE BLOCK ##### START
 */
    #if defined(_WIN32)
        WaitForSingleObject(state->threads[0].thr, 2000000000);
        WaitForSingleObject(state->threads[1].thr, 2000000000);
        WaitForSingleObject(state->threads[2].thr, 2000000000);

    #elif defined(__HAIKU__) /* || defined(__BEOS__) ?Dont know yet. */
        status_t exit_value;
        wait_for_thread(state->threads[0].thr, &exit_value);
        wait_for_thread(state->threads[1].thr, &exit_value);
        wait_for_thread(state->threads[2].thr, &exit_value);

    #else /* POSIX */
        pthread_join(state->threads[0].thr, NULL);
        pthread_join(state->threads[1].thr, NULL);
        pthread_join(state->threads[2].thr, NULL);

    #endif
/*
 *   END ##### PORTABLE BLOCK ##### END
 */

}



void seedy32(uint8_t* buffer, size_t bytes)
{
    size_t i = 0;
    int j = 0;
    uint32_t last_pick;
    uint32_t next_pick;
    SEEDY32 state;
    size_t blocks; 
#if defined(LIBSEEDY_OPT_PROGRESS) 
    clock_t old_report;
    clock_t new_report;
#endif
    blocks = bytes / 4;

    start_seeder_32(&state);
    wait_ms(SEEDY_INIT_MS_);

    last_pick = read_state_32(&state);
#if defined(LIBSEEDY_OPT_PROGRESS) 
    old_report = NOW();
#endif
    while( i < (4 * blocks) )
    {
        wait_us(SEEDY_INTERVAL_US_);
        next_pick = read_state_32(&state);
        if(next_pick != last_pick)
        {
            *(uint32_t*)&(buffer[i]) = next_pick;
            last_pick = next_pick;
            i = i + (int)4;
        }
#if defined(LIBSEEDY_OPT_PROGRESS) 
        new_report = NOW();
        if(new_report > old_report)
        {
            old_report = new_report;
            printf( "\r%u %%", (unsigned int)( (i*100)/((4 * blocks)) ) );
        }
#endif
    }

    while( i < bytes )
    {
        wait_us(SEEDY_INTERVAL_US_);
        next_pick = read_state_32(&state);
        if(next_pick != last_pick)
        {
            buffer[i++] = ((uint8_t*)&next_pick)[j++];
            last_pick = next_pick;
        }
    }

    stop_seeder_32(&state);
}

#endif

