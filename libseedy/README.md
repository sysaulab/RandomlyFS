# libseedy, The Code

## Files

- common.h: Defines common macros, detect features and provides a 
portable stdint.h definitions to help with portability.
- libseedy16.h: 16 bit implementation
- libseedy32.h: 32 bit implementation
- libseedy64.h: 64 bit implementation
- pstdint.h: Portable replacement for <stdint.h>
- seedy.c: demo program, display random hexadecimal sequences

## History

It was new year's eve and I was returning from Yellowknife. I was listening 
to a podcast about cryptography. A bird flying in the sky inspired me to 
theorize about the nature of unpredictability ny imagining the turbulences 
behind the wings.

It seemed to me that loose concurrency would be key to achieve my goal. Could 
I replicate it using software? The image that appeared in my head was one of a 
system that was interdependent and loosely coupled. Un-coordinated race 
conditions offers the possibility of loose coupling by intersecting each 
thread's input with another's output through a shared pointer.

## Algorithm and characteristics

- 64 bit
- Slow, minimum sleep timer resolution dictate throughput.
- Chaotic generator, not a sequence
- Uses 2MiB of noise data
- Create a randomly accessible 128EiB pseudo-noise map.

The chaotic generator is capable of providing original entropy reliably 
on any platform capable of concurrent multithreading. It creates 3 threads, 
each running a hash function. To better understand it's inner working I will 
explain the algorithm step by step as if it was performed by people in an 
office. The hashing functions will be represented by three accountants. The 
shared memory that connect the ring can be represented by three filing cabinets. 

- Accountant A (AA) read from cabinet 3 (C3) and write to cabinet 1 (C1).
- Accountant B (AB) read from cabinet 1 (C1) and write to cabinet 2 (C2).
- Accountant C (AC) read from cabinet 2 (C2) and write to cabinet 3 (C3).

	AA -> C1 -> AB -> C2 -> AC -> C3
  	
	\\____________ <- ____________/

The job of accountant A is:

1. Write the number 1 on a sheet that remains on his desk.
2. Do the following 64 times:
    1. Look if the number in C3 is odd or even.
    2. According to step 1, pick the first number from the next pair on the 
	list. If even use the first number, if odd use the second number.
    3. Multiply the number picked with the number on your sheet of paper.
    4. Go look again at the number in C3 and do a binary xor to combine it 
	with the number currently of your sheet of paper.
    5. Take the number obtained at the last step and add it to the number in 
	C1, replacing the number in C1.
3. Use the number on your sheet of paper and combining it with C1 using 
binary xor.

AB and AC are also doing the same thing with their own respective 
cabinets. This means that the order each of then access a cabinet 
to read or write something in it influences the next number of everyone. 

Since everyone is influencing everyone else in a closed ring, no single 
actor can know its future with certainty without coordinating everyone's 
actions. The supervisor, who makes a point not to coordinates anything, 
uses an alarm to periodically go read the number in cabinets C1, C2 and C3, 
combine them using binary xor and add them to the list...

Each time one of the accountant access the cabinets, it creates an 
opportunity for divergence. By repeating the process 64 times to compute 
a 64 bit integer we ensure the number is computed with a sufficient amount 
of uncertainty to guarantee complete decorrelation after each generation.

Each accountant's work in itself is very predictable. When three 
accountants do this continously, the number they generate will 
vary wildly according to the order each of them arrive to each 
cabinet first. When all accountants are not working at the same 
time, the numbers being calculated are not as chaotic. 

This is why I describe the behaviour in single core environments 
as pseudo chaotic. When that happens, the work of each accountant 
is predictable and only the moment at which they stop and start 
again contributes to the deviation from the expected norm.

While each process is deterministic when it operates on its own, 
the overall configuration creates an unstable, divergent, complex 
system. I call that configuration "synthetic chaos".

## Testing and observations

PractRand and TestU01 are standard test suites used to measure the 
statistical "randomness" of PRNG. I use it is verify the 
characteristics of the chaotic generator in various environment.

As I tested it in different environments, it became clear to me 
that the synthetic chaos effect appear only when the threads are 
physically concurrent. When the coupling is not maintained, the 
strength of the effect is diminished. In the case of partial 
physical concurrency, the quality is diminished, but not completely.

The chaotic generator unfiltered passes both PractRand and Test U01 
Small Crush, when running on multicore architecture. This live 
testing has been conducted on a M1 cpu. Using it to generate a 2mb 
static map for the QXO64 generator (final stage of prandom). QXO 
uses a naive combining (xor) alrgorithm that reflects the quality 
of it's input. By using it, I tested ICM+QXO to 32TB with success.

I also generated seed files on an old iBook G4 (1 core) and core 
i5 first generation (2 cores, 4 threads). The results are excellent, 
the loss of cohesion (simulteneous treads) in the G4 is reflected in 
the abysmal results from PractRand and TestU01 Small Crunch. The fast 
tests (FPR FSC) is running the QXO generator and testing 2mb of seed 
at a time, or is ran live from my M1 Mac. The results given is the 
number of FAST tests with flawless results.

    ICM....................................
                    M1       G4        i5
    PractRand       PASS     FAIL      PASS
    Test U01        PASS     FAIL      PASS
    
    ICM+QXO................................
                    M1       G4        i5
    PractRand 1GB   94%      95%       93%
    Test U01 S.C.   97%      99%       96%

The fact that a monocore G4 produces such results by filtering 
its faulty output using only QXO surprises me. I expected the 
results to be much worse. Generating chaotic streams from user 
space has been quite an adventure. I hope these results will 
help alleviate some of the skepticism I encounter.

I published the results (folder with plenty of stuff) of the tests 
with the seeds collected from the G4 and old i5. on Github. The G4 
seed is truncated as it was too large for github (3gb). Look into 
the seed.txt files for the source URL.

[https://github.com/sysaulab/softrng](https://github.com/sysaulab/softrng)

[https://github.com/sysaulab/softrng-test](https://github.com/sysaulab/softrng-test)

These results, coupled with the results I observed last year (video), 
make me confident about the valitdity of the approach.

[https://www.youtube.com/watch?v=rrr7k4lb8Dg](https://www.youtube.com/watch?v=rrr7k4lb8Dg)

[https://github.com/sysaulab/libseedy](https://github.com/sysaulab/libseedy)
