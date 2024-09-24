/*
 *   64 bit specific header.
 */
#ifndef __SEEDY64_H_
#define __SEEDY64_H_

#include "common.h"

/*
 *   SEEDY64
 */

typedef struct seed_thread64_s
{
    volatile uint64_t *source;
    volatile uint64_t *sink;
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
seed_thread;

typedef struct seed_state64_s
{
    volatile uint64_t nodes[3];
    seed_thread threads[3];
}
SEEDY64;

uint64_t read_state_64(SEEDY64* state)
{
    return state->nodes[0] ^ state->nodes[1] ^ state->nodes[2];
}

int seed_modify_64( volatile uint64_t* in, volatile uint64_t* out )
{
    uint64_t acc = 1;                                   
    /* 
     *   newvalue : 128 unique prime numbers will be multiplied together in this accumulator
     *   The original hash function used a set of smallest prime numbers. (2, 3, 5, ...)
     *   Replacing by a personal selection of prime numbers found on the internet improved the quality of the stream.
     *   I think this is mostly because it forces to changes more bits at every addition in the collision multiplier.
     *   No other sets has been tested by the original author. Please modify this comment if you do.
     */
    uint64_t primes[] = 
{2985113428254948959ULL,
2133371973447287789ULL,
9566925003104080411ULL,
6083414080430737261ULL,
3191658566037706763ULL,
6913367267901031997ULL,
3051762490125913007ULL,
4496279044463191703ULL,
2879325911889076339ULL,
1685450054146532617ULL,
3665492224534333199ULL,
5457405118009752311ULL,
9603674174905975319ULL,
5115570615370213399ULL,
2085406057152991381ULL,
3944123521248288751ULL,
7068318273059084621ULL,
9757209530458778617ULL,
4605875306346355289ULL,
6173263078309968247ULL,
7762991333463966857ULL,
9020426977113888977ULL,
7479023155365493211ULL,
1106308364738794853ULL,
6407388572493574733ULL,
2575377960084637447ULL,
3570804859728957167ULL,
9744215084471400349ULL,
5786617550671126231ULL,
3349473746601511027ULL,
4095923646587284861ULL,
9337574612835542873ULL,
3860411135543820653ULL,
6490253556739218821ULL,
8764900392399904901ULL,
3339372363887497583ULL,
6577701767928311429ULL,
9172672799009443051ULL,
2788564470716716463ULL,
7776210437768060567ULL,
9071396938031495159ULL,
5357072389526583433ULL,
6633238451589839561ULL,
6234713344370965483ULL,
1756087432259377561ULL,
3210111471103586737ULL,
2408295824875360559ULL,
8027800137025286071ULL,
1345382896972678691ULL,
9079595888451204607ULL,
6556450002823963531ULL,
7301373103704944939ULL,
1213337252585919101ULL,
8574942160927332881ULL,
8566185364500201241ULL,
6910045784078487233ULL,
2298581249625702883ULL,
2596136803677419831ULL,
8990581036371057601ULL,
3046889204803159003ULL,
6864701819024214703ULL,
9142639517683050829ULL,
3257525145999184439ULL,
5916350576516314903ULL,
1991008946812235681ULL,
5221054210441722691ULL,
4658950185140640559ULL,
7616154792906075869ULL,
3789735489826889479ULL,
6040491638497194979ULL,
4531551599015040559ULL,
8347022247974757701ULL,
5867558032219360441ULL,
4346307676337591453ULL,
1905346606445378533ULL,
7219759323757378847ULL,
8561163821217102983ULL,
6227762971465314193ULL,
1778354250598502561ULL,
5435722899178511549ULL,
7859180386742822047ULL,
3610133528213982527ULL,
4577787558750808879ULL,
3974497005134013109ULL,
3241288686894948329ULL,
3206546850542044487ULL,
2621128575668534623ULL,
6945339563322422683ULL,
2086055779082649859ULL,
9237096783908108657ULL,
1916568609042647407ULL,
8362781431934621017ULL,
6122543372115730003ULL,
6200399818195771541ULL,
1259180073496439959ULL,
4347405785030416513ULL,
4385862544209720253ULL,
5836562284002027449ULL,
3121678407354608831ULL,
4319136453194505203ULL,
9085838656424873461ULL,
5448516463713100871ULL,
4520015808866767003ULL,
2892328915328607301ULL,
3897908110284010777ULL,
6252018905800065119ULL,
3278710975885668139ULL,
2623408458547857527ULL,
3055342605008846819ULL,
5255646576194862401ULL,
5611094629836787733ULL,
3296996236992951511ULL,
3283558997806116761ULL,
2823396471863459119ULL,
9572308265712226037ULL,
8290430615658140401ULL,
2621246478142939039ULL,
1300291691995934101ULL,
1143863624227711889ULL,
6789671898597272039ULL,
1593529243588418053ULL,
6573565795099723367ULL,
7154224095543629129ULL,
2355509713379653547ULL,
7691022839137400143ULL,
2727794217173953619ULL,
8541344794240933387ULL,
6490328854741497349ULL}
    ;

    int x = 0;
    while( x < 8 * 8 ) {
        *in = (*in << 7) | (*in >> ((8 * 8) - 7));           /* source is rotated by 13 bits... now 7 to accomodate potential 8 bit fundamentals */
        acc = (acc << x) | (acc >> ((8 * 8) - x));           /* smooth out bit distribution in acc */
        acc *= primes[(2 * x) + (1 & *in)];                            /* pick the prime[x][set?], do it 64 times (we rotate above) */

        /* 
         *   This addition increase the number of unresolved race conditions / computation cycle.
         *   Before this addition I had to wait and verify for a change in the state to avoid polling too soon.
         *   This addition instruction allowed continuous polling during most of the development and testing phases.
         *   Basic dafeguard has been reintroduced for production before the last round of testing on the iBook G4.
         */
        *out += acc ^ *in;
        x++;
    }
    *out ^= acc;
    return 0;
}

void* seed_thread_main_64(void* raw)
{
    seed_thread* state = (seed_thread*) raw;
    while(1)
    {
        seed_modify_64(state->source, state->sink);
        if(state->run == 0)
        {
            return NULL;
        }
    }
    return NULL;
}

void start_seeder_64(SEEDY64* state)
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
        state->threads[0].thr = CreateThread((LPSECURITY_ATTRIBUTES)NULL, 0, (LPTHREAD_START_ROUTINE)&seed_thread_main_64, &(state->threads[0]), (DWORD)0, NULL);
        state->threads[1].thr = CreateThread((LPSECURITY_ATTRIBUTES)NULL, 0, (LPTHREAD_START_ROUTINE)&seed_thread_main_64, &(state->threads[1]), (DWORD)0, NULL);
        state->threads[2].thr = CreateThread((LPSECURITY_ATTRIBUTES)NULL, 0, (LPTHREAD_START_ROUTINE)&seed_thread_main_64, &(state->threads[2]), (DWORD)0, NULL);
    
    #elif defined(__HAIKU__)
        state->threads[0].thr = spawn_thread((int(*)(void*))&seed_thread_main_64, "Seedy Accountant Number A", B_NORMAL_PRIORITY, &(state->threads[0]));
        state->threads[1].thr = spawn_thread((int(*)(void*))&seed_thread_main_64, "Seedy Accountant Number B", B_NORMAL_PRIORITY, &(state->threads[1]));
        state->threads[2].thr = spawn_thread((int(*)(void*))&seed_thread_main_64, "Seedy Accountant Number C", B_NORMAL_PRIORITY, &(state->threads[2]));
        if ( B_OK != 
            ( resume_thread(state->threads[0].thr ) ) &
            ( resume_thread(state->threads[1].thr ) ) &
            ( resume_thread(state->threads[2].thr ) ) )
        {
            fprintf(stderr, "ERROR resuming threads.\n");
            exit(0);
        }

    #else /* POSIX */
        pthread_create(&(state->threads[0].thr), NULL, &seed_thread_main_64, &(state->threads[0]));
        pthread_create(&(state->threads[1].thr), NULL, &seed_thread_main_64, &(state->threads[1]));
        pthread_create(&(state->threads[2].thr), NULL, &seed_thread_main_64, &(state->threads[2]));

    #endif
/*
 *   END ##### PORTABLE BLOCK ##### END
 */

}

void stop_seeder_64(SEEDY64* state)
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



void seedy64(uint8_t* buffer, size_t bytes)
{
    size_t i = 0;
    int j = 0;
    uint64_t last_pick;
    uint64_t next_pick;
#if defined(LIBSEEDY_OPT_PROGRESS) 
    clock_t old_report;
    clock_t new_report;
#endif
    SEEDY64 state;
    size_t blocks; 
    blocks = bytes / 8;

    start_seeder_64(&state);
    wait_ms(SEEDY_INIT_MS_);

    last_pick = read_state_64(&state);
#if defined(LIBSEEDY_OPT_PROGRESS) 
    old_report = NOW();
#endif

    while( i < (8 * blocks) )
    {
        wait_us(SEEDY_INTERVAL_US_);
        next_pick = read_state_64(&state);
        if(next_pick != last_pick)
        {
            *(uint64_t*)&(buffer[i]) = next_pick;
            last_pick = next_pick;
            i = i + (int)8;
        }
#if defined(LIBSEEDY_OPT_PROGRESS) 
        new_report = NOW();
        if(new_report > old_report)
        {
            old_report = new_report;
            fprintf(stderr, "\r%zu %% ", (i * 100)/((8 * blocks)));
        }
#endif
    }
    while( i < bytes )
    {
        wait_us(SEEDY_INTERVAL_US_);
        next_pick = read_state_64(&state);
        if(next_pick != last_pick)
        {
            buffer[i++] = ((uint8_t*)&next_pick)[j++];
            last_pick = next_pick;
        }
    }

    stop_seeder_64(&state);
}

#endif
