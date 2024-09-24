#include <libseedy/common.h>

int isPrime16(uint16_t n)
{
    uint16_t i = 5;
    if ( n <= 1 )
        return 1;

    if ( n <= 3 )
        return 0;

    if ( n % 2 == 0 || n % 3 == 0 )
        return 0;

    while ( i * i <= n )
    {
        if ( n % i == 0 || n % ( i + 2 ) == 0 )
            return 0;

        i = i + 6;
    }
    return 1;
}

uint16_t prime16(uint16_t N)
{
	uint16_t prime, found;
    
    if (N <= 1)
        return 2;

    prime = N;
    found = 0;

    while (found == 0)
    {
        prime++;
        if (isPrime16(prime))
            found = 1;
    }
    
    return prime;
}

