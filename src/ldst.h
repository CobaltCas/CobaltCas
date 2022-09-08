#pragma once

#include <inttypes.h>

static inline uint16_t ld_be16(const void *p)
{
    const uint8_t *q = (const uint8_t *)p;
    uint16_t x = 0;
    x |= *q++, x <<= 8;
    x |= *q;
    return x;
}

static inline void st_be16(void *p, uint16_t x)
{
    uint8_t *q = (uint8_t *)p + 2;
    *--q = 0xff & x, x >>= 8;
    *--q = 0xff & x;
}

static inline uint32_t ld_be32(const void *p)
{
    const uint8_t *q = (const uint8_t *)p;
    uint32_t x = 0;
    x |= *q++, x <<= 8;
    x |= *q++, x <<= 8;
    x |= *q++, x <<= 8;
    x |= *q;
    return x;
}

static inline void st_be32(void *p, uint32_t x)
{
    uint8_t *q =(uint8_t *)p + 4;
    *--q = 0xff & x, x >>= 8;
    *--q = 0xff & x, x >>= 8;
    *--q = 0xff & x, x >>= 8;
    *--q = 0xff & x;
}

static inline uint64_t ld_be48(const void *p)
{
    const uint8_t *q = (const uint8_t *)p;
    uint64_t x = 0;
    x |= *q++, x <<= 8;
    x |= *q++, x <<= 8;
    x |= *q++, x <<= 8;
    x |= *q++, x <<= 8;
    x |= *q++, x <<= 8;
    x |= *q;
    return x;
}

static inline void st_be48(void *p, uint64_t x)
{
    uint8_t *q = (uint8_t *)p + 6;
    *--q = 0xff & x, x >>= 8;
    *--q = 0xff & x, x >>= 8;
    *--q = 0xff & x, x >>= 8;
    *--q = 0xff & x, x >>= 8;
    *--q = 0xff & x, x >>= 8;
    *--q = 0xff & x;
}

static inline uint64_t ld_be64(const uint8_t *p, int n = 8)
{
    uint64_t q = 0;
    switch (n) {
        case 8: q |= (uint64_t)p[7];
        case 7: q |= (uint64_t)p[6] <<  8;
        case 6: q |= (uint64_t)p[5] << 16;
        case 5: q |= (uint64_t)p[4] << 24;
        case 4: q |= (uint64_t)p[3] << 32;
        case 3: q |= (uint64_t)p[2] << 40;
        case 2: q |= (uint64_t)p[1] << 48;
        case 1: q |= (uint64_t)p[0] << 56;
    }
    return q;
}

static inline uint64_t ld_le64(const uint8_t *p, int n = 8)
{
    uint64_t q = 0;
    switch (n) {
        case 8: q |= (uint64_t)p[7] << 56;
        case 7: q |= (uint64_t)p[6] << 48;
        case 6: q |= (uint64_t)p[5] << 40;
        case 5: q |= (uint64_t)p[4] << 32;
        case 4: q |= (uint64_t)p[3] << 24;
        case 3: q |= (uint64_t)p[2] << 16;
        case 2: q |= (uint64_t)p[1] <<  8;
        case 1: q |= (uint64_t)p[0];
    }
    return q;
}

static inline void st_be64(uint8_t *p, uint64_t x, int n = 8)
{
    switch (n) {
        case 8: p[7] = (uint8_t)(x);
        case 7: p[6] = (uint8_t)(x >>  8);
        case 6: p[5] = (uint8_t)(x >> 16);
        case 5: p[4] = (uint8_t)(x >> 24);
        case 4: p[3] = (uint8_t)(x >> 32);
        case 3: p[2] = (uint8_t)(x >> 40);
        case 2: p[1] = (uint8_t)(x >> 48);
        case 1: p[0] = (uint8_t)(x >> 56);
    }
}

static inline void st_le64(uint8_t *p, uint64_t x, int n = 8)
{
    switch (n) {
        case 8: p[7] = (uint8_t)(x >> 56);
        case 7: p[6] = (uint8_t)(x >> 48);
        case 6: p[5] = (uint8_t)(x >> 40);
        case 5: p[4] = (uint8_t)(x >> 32);
        case 4: p[3] = (uint8_t)(x >> 24);
        case 3: p[2] = (uint8_t)(x >> 16);
        case 2: p[1] = (uint8_t)(x >>  8);
        case 1: p[0] = (uint8_t)(x);
    }
}
