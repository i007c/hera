
#ifndef __HERA_HERA_H__
#define __HERA_HERA_H__

#include <stdint.h>

#define panic(reason) {log_trace(reason); exit(1);}

typedef struct ScaleData {
    float zoom;
    uint32_t iw;
    uint32_t ih;
    uint32_t ow;
    uint32_t oh;
    float wr;
    float hr;
    float r;
    uint32_t imaxw;
    uint32_t imaxh;
    int32_t offset_x;
    int32_t offset_y;
} ScaleData;

#endif // __HERA_HERA_H__

