#include "prime64.h"
#include "prime16.h"
#include "libseedy/libseedy64.h"

uint64_t prime64sub16()
{
    uint64_t answer;
    uint16_t* cur_buf_16;

    seedy64((uint8_t*)&answer, sizeof(answer));

    cur_buf_16 = (uint16_t*)&answer;

    cur_buf_16[0] = prime16(cur_buf_16[0]);
    cur_buf_16[1] = prime16(cur_buf_16[1]);
    cur_buf_16[2] = prime16(cur_buf_16[2]);
    cur_buf_16[3] = prime16(cur_buf_16[3]);
    
    answer = prime64(answer);

    return answer;
}
