#ifndef __SEEDY16_H_
#define __SEEDY16_H_

#include "common.h"

typedef struct seed_thread16_s
{
    volatile uint16_t *source;
    volatile uint16_t *sink;
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
}
seed_thread_16;

typedef struct seed_state16_s
{
    volatile uint16_t nodes[3];
    seed_thread_16 threads[3];
}
SEEDY16;

uint16_t read_state_16(SEEDY16* state)
{
    return state->nodes[0] ^ state->nodes[1] ^ state->nodes[2];
}

int seed_modify_16( volatile uint16_t* in, volatile uint16_t* out )
{
    uint16_t acc = 1;
    uint16_t primes[] = 
{61441,
61463,
61469,
61471,
61483,
61487,
61493,
61507,
61511,
61519,
61543,
61547,
61553,
61559,
61561,
61583,
61603,
61609,
61613,
61627,
61631,
61637,
61643,
61651,
61657,
61667,
61673,
61681,
61687,
61703,
61717,
61723}
    ;

    int x = 0;
    while( x < 2 * 8 ) {
        *in = (*in << 7) | (*in >> ((2 * 8) - 3));
        acc = (acc << x) | (acc >> ((2 * 8) - x));
        acc *= primes[(2 * x) + (1 & *in)];
        *out += acc ^ *in;
        x++;
    }
    *out ^= acc;
    return 0;
}

void* seed_thread_main_16(void* raw)
{
    seed_thread_16* state = (seed_thread_16*) raw;
    while(1)
    {
        seed_modify_16(state->source, state->sink);
        if(state->run == 0)
        {
            return NULL;
        }
    }
    return NULL;
}

void start_seeder_16(SEEDY16* state)
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
        state->threads[0].thr = CreateThread((LPSECURITY_ATTRIBUTES)NULL, 0, (LPTHREAD_START_ROUTINE)&seed_thread_main_16, &(state->threads[0]), (DWORD)0, NULL);
        state->threads[1].thr = CreateThread((LPSECURITY_ATTRIBUTES)NULL, 0, (LPTHREAD_START_ROUTINE)&seed_thread_main_16, &(state->threads[1]), (DWORD)0, NULL);
        state->threads[2].thr = CreateThread((LPSECURITY_ATTRIBUTES)NULL, 0, (LPTHREAD_START_ROUTINE)&seed_thread_main_16, &(state->threads[2]), (DWORD)0, NULL);
    
    #elif defined(__HAIKU__)
        state->threads[0].thr = spawn_thread((int(*)(void*))&seed_thread_main_16, "Seedy Accountant Number A", B_NORMAL_PRIORITY, &(state->threads[0]));
        state->threads[1].thr = spawn_thread((int(*)(void*))&seed_thread_main_16, "Seedy Accountant Number B", B_NORMAL_PRIORITY, &(state->threads[1]));
        state->threads[2].thr = spawn_thread((int(*)(void*))&seed_thread_main_16, "Seedy Accountant Number C", B_NORMAL_PRIORITY, &(state->threads[2]));
        if ( B_OK != 
            ( resume_thread(state->threads[0].thr ) ) &
            ( resume_thread(state->threads[1].thr ) ) &
            ( resume_thread(state->threads[2].thr ) ) )
        {
            fprintf(stderr, "ERROR resuming threads.\n");
            exit(0);
        }

    #else /* POSIX */
        pthread_create(&(state->threads[0].thr), NULL, &seed_thread_main_16, &(state->threads[0]));
        pthread_create(&(state->threads[1].thr), NULL, &seed_thread_main_16, &(state->threads[1]));
        pthread_create(&(state->threads[2].thr), NULL, &seed_thread_main_16, &(state->threads[2]));

    #endif
/*
 *   END ##### PORTABLE BLOCK ##### END
 */
}

void stop_seeder_16(SEEDY16* state)
{
    state->threads[0].run = 0;
    state->threads[1].run = 0;
    state->threads[2].run = 0;
    #ifdef _WIN32
        WaitForSingleObject(state->threads[0].thr, 2000000000);
        WaitForSingleObject(state->threads[1].thr, 2000000000);
        WaitForSingleObject(state->threads[2].thr, 2000000000);
    #else
        pthread_join(state->threads[0].thr, NULL);
        pthread_join(state->threads[1].thr, NULL);
        pthread_join(state->threads[2].thr, NULL);
    #endif
}

void seedy16(uint8_t* buffer, size_t bytes)
{
    size_t i = 0;
    int j = 0;
    uint16_t last_pick;
    uint16_t next_pick;
#if defined(LIBSEEDY_OPT_PROGRESS) 
    clock_t old_report;
    clock_t new_report;
#endif
    SEEDY16 state;
    size_t blocks; 
    blocks = bytes / 2;

    start_seeder_16(&state);
    wait_ms(SEEDY_INIT_MS_);

    last_pick = read_state_16(&state);
#if defined(LIBSEEDY_OPT_PROGRESS) 
    old_report = NOW();
#endif
    while( i < (2 * blocks) )
    {
        wait_us(SEEDY_INTERVAL_US_);
        next_pick = read_state_16(&state);
        if(next_pick != last_pick)
        {
            *(uint16_t*)&(buffer[i]) = next_pick;
            last_pick = next_pick;
            i = i + (int)2;
        }
#if defined(LIBSEEDY_OPT_PROGRESS) 
        new_report = NOW();
        if(new_report > old_report)
        {
            old_report = new_report;
            fprintf(stderr, "\r%zu %% ", (i*100)/((2 * blocks)));
        }
#endif
    }

    while( i < bytes )
    {
        wait_us(SEEDY_INTERVAL_US_);
        next_pick = read_state_16(&state);
        if(next_pick != last_pick)
        {
            buffer[i++] = ((uint8_t*)&next_pick)[j++];
            last_pick = next_pick;
        }
    }

    stop_seeder_16(&state);
}


#endif

