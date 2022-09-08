#include "crypto.h"
#include "ldst.h"

namespace Crypto {

    //------------------------------------------------------------------------------
    static inline uint64_t reverse64(uint64_t x)
    {
        x = ((x & 0xff00ff00ff00ff00LL) >>  8) | ((x & 0x00ff00ff00ff00ffLL) <<  8);
        x = ((x & 0xffff0000ffff0000LL) >> 16) | ((x & 0x0000ffff0000ffffLL) << 16);
        return (x >> 32) | (x << 32);
    }

    //------------------------------------------------------------------------------
    static inline uint32_t reverse32(uint32_t x)
    {
        x = ((x & 0xff00ff00) >>  8) | ((x & 0x00ff00ff) <<  8);
        return (x >> 16) | (x << 16);
    }

    //------------------------------------------------------------------------------
    static inline int bitcount(uint32_t x)
    {
        x = (x & 0x55555555) + ((x & 0xaaaaaaaa) >> 1);
        x = (x & 0x33333333) + ((x & 0xcccccccc) >> 2);
        x = (x & 0x0f0f0f0f) + ((x & 0xf0f0f0f0) >> 4);
        x = (x & 0x00ff00ff) + ((x & 0xff00ff00) >> 8);
        x = (x & 0x0000ffff) + ((x & 0xffff0000) >> 16);
        return x;
    }

    //------------------------------------------------------------------------------
    static inline int parity(uint32_t x)
    {
        x ^= x >> 16;
        x ^= x >> 8;
        x ^= x >> 4;
        x ^= x >> 2;
        x ^= x >> 1;
        return x & 1;
    }

    //------------------------------------------------------------------------------
    static uint32_t sbox40(uint32_t x)
    {
        static const uint8_t sbox[ 256 ] = {
            0xAA, 0xA2, 0x10, 0xFA, 0xA9, 0xF0, 0x40, 0x2F, 0xB1, 0x1C, 0x1A, 0x6F, 0x43, 0xB4, 0x73, 0xBC,
            0x69, 0x77, 0xC5, 0x00, 0xF3, 0xD4, 0x09, 0x7E, 0x58, 0x8D, 0x44, 0xC3, 0xF5, 0x54, 0x0C, 0xDD,
            0x3F, 0xB7, 0xD1, 0xD6, 0x9A, 0xD3, 0x39, 0x82, 0x01, 0x5E, 0x03, 0xED, 0x78, 0x63, 0x90, 0x49,
            0x9B, 0x15, 0xA8, 0x4F, 0x67, 0x52, 0xAC, 0xE4, 0x37, 0xEA, 0xF7, 0x23, 0x55, 0x0F, 0x42, 0x12,
            0xE3, 0x05, 0x5F, 0x2D, 0x2E, 0x7F, 0x11, 0x38, 0x07, 0xF4, 0x3C, 0xE2, 0xD5, 0x9F, 0xDF, 0xCF,
            0x30, 0x0B, 0xAD, 0x66, 0x22, 0x70, 0xEF, 0x7B, 0xA6, 0x24, 0x65, 0x0D, 0x5D, 0x79, 0x02, 0x4D,
            0x0E, 0x32, 0x84, 0x97, 0xB8, 0x57, 0x34, 0xE8, 0x41, 0x87, 0xC1, 0xF9, 0x9C, 0x56, 0xAE, 0x71,
            0xAB, 0xBF, 0xD0, 0x88, 0x25, 0xC8, 0x1F, 0xD7, 0xFE, 0x04, 0x4E, 0xCE, 0x51, 0x81, 0xBB, 0xCD,
            0x91, 0xA5, 0x14, 0x75, 0xA4, 0x60, 0x61, 0x6E, 0x7A, 0xE6, 0x99, 0xD8, 0xA0, 0x4C, 0xDC, 0x1B,
            0x06, 0x6C, 0x3E, 0x9E, 0xF8, 0xCB, 0x98, 0x92, 0x0A, 0xFB, 0x2A, 0xCA, 0x50, 0x7C, 0xC0, 0x83,
            0x94, 0xB5, 0x6A, 0x21, 0x95, 0xB3, 0x48, 0xD9, 0x16, 0xA7, 0xEE, 0x4B, 0xFD, 0x9D, 0xBD, 0x6B,
            0xC6, 0x80, 0x20, 0x3A, 0x53, 0x1E, 0x5C, 0xC7, 0xB6, 0x08, 0xAF, 0xA1, 0x2B, 0x19, 0x26, 0x8A,
            0x47, 0xE1, 0x86, 0x74, 0xE9, 0x59, 0x62, 0x8B, 0x28, 0x6D, 0xEC, 0x76, 0xB0, 0x45, 0xC2, 0x46,
            0x4A, 0xE0, 0xF2, 0x8C, 0xBE, 0x3B, 0x5B, 0xBA, 0x31, 0x96, 0xE5, 0x36, 0x8E, 0xEB, 0xE7, 0xB9,
            0xA3, 0x35, 0x17, 0x68, 0x27, 0x8F, 0x85, 0x89, 0x29, 0x93, 0xFF, 0xFC, 0xDE, 0x7D, 0x18, 0xDB,
            0x64, 0xF6, 0x1D, 0xB2, 0x3D, 0xF1, 0xC9, 0x13, 0xDA, 0xCC, 0xC4, 0x72, 0x33, 0x5A, 0xD2, 0x2C
        };

        return ((sbox[ 0xff & (x >> 24) ] << 24) |
                (sbox[ 0xff & (x >> 16) ] << 16) |
                (sbox[ 0xff & (x >>  8) ] <<  8) |
                (sbox[ 0xff & (x) ]));
    }

    //------------------------------------------------------------------------------
    static uint32_t round00(uint32_t x, uint32_t k, uint8_t flavor)
    {
        uint16_t salt = (flavor & 2) ? 0x5353 : 0;
        x = (0xffff & (x + k + salt)) | ((x >> 16) + (k >> 16) + salt) << 16;
        x = ((x & 0xf0f0f0f0) >> 4) | ((x & 0x0f0f0f0f) << 4);
        k = (k << 1) | (k >> 31);
        x = (parity(x & k)) ? x ^ ~k : x;
        x = (flavor & 1) ?
            (x & 0xaa55aa55) | ((x & 0x55005500) >> 7) | ((x & 0x00aa00aa) << 7):
            (x & 0x55aa55aa) | ((x & 0xaa00aa00) >> 9) | ((x & 0x00550055) << 9);
        x = (x & 0x00ffff00) | (x >> 24) | (x << 24);
        x = x ^ ((x << 24) | (x >> 8)) ^ ((x << 25) | (x >> 7));

        return x;
    }

    //------------------------------------------------------------------------------
    static uint32_t round40(uint32_t x, uint32_t k)
    {
        x  = sbox40(x ^ k);
        x ^= ((x & 0x007f007f) <<  9) | ((x & 0x00800080) <<  1);
        x ^= ((x & 0x7f007f00) >>  7) | ((x & 0x80008000) >> 15);
        x  = (x << 8) | (x >> 24);
        x ^= ((x & 0x3f003f00) >>  6) | ((x & 0xc000c000) >> 14);
        x ^= ((x & 0x003f003f) << 10) | ((x & 0x00c000c0) <<  2);
        x  = (x >> 8) | (x << 24);
        return x;
    }

    //------------------------------------------------------------------------------
    static uint64_t block00(uint64_t block, const uint32_t kext[ 4 ], uint8_t protocol, bool encrypt)
    {
        const static uint8_t flavor[ 8 ][ 16 ] = {
            { 1, 0, 1, 2, 2, 2, 0, 2, 1, 3, 0, 2, 1, 0, 0, 1 },
            { 3, 2, 0, 2, 2, 0, 3, 0, 3, 1, 3, 3, 0, 1, 0, 1 },
            { 2, 0, 0, 1, 1, 3, 3, 1, 0, 1, 2, 0, 1, 0, 1, 0 },
            { 2, 3, 0, 1, 0, 0, 3, 1, 3, 1, 1, 3, 1, 0, 0, 2 },
            { 2, 3, 3, 2, 1, 3, 1, 2, 1, 2, 3, 1, 2, 0, 0, 1 },
            { 2, 2, 3, 3, 1, 3, 2, 2, 3, 1, 0, 2, 0, 0, 1, 1 },
            { 1, 3, 1, 2, 2, 0, 1, 0, 3, 3, 3, 1, 0, 2, 2, 2 },
            { 1, 2, 0, 2, 0, 0, 3, 1, 1, 3, 1, 2, 2, 2, 0, 1 }
        };

        uint32_t left = (uint32_t)(block >> 32), right = (uint32_t)block;
        if (encrypt) {
            for (int r = 0; r < 16;) {
                left  ^= round00(right, kext[ r & 3 ], flavor[ 0 ][ r ]); r++;
                right ^= round00(left,  kext[ r & 3 ], flavor[ 0 ][ r ]); r++;
            }
        } else {
            for (int r = 15; r >= 0;) {
                left  ^= round00(right, kext[ r & 3 ], flavor[ 0 ][ r ]); r--;
                right ^= round00(left,  kext[ r & 3 ], flavor[ 0 ][ r ]); r--;
            }
        }

        return ((uint64_t) right << 32) | left;
    }

    //------------------------------------------------------------------------------
    static uint64_t block40(uint64_t block, const uint32_t kext[ 16 ],  uint8_t protocol, bool encrypt)
    {
        uint64_t salt = (protocol & 0x0c) ? 0xfbe852461acd3970LL : 0xd34c027be8579632LL;

        block += salt;
        uint32_t left = (uint32_t)(block >> 32), right = (uint32_t)block;
        if (encrypt) {
            for (int r = 0; r < 16;) {
                right ^= round40(left,  kext[ r ]); r++;
                left  ^= round40(right, kext[ r ]); r++;
            }
        } else {
            for (int r = 15; r >= 0;) {
                right ^= round40(left,  kext[ r ]); r--;
                left  ^= round40(right, kext[ r ]); r--;
            }
        }
        block = ((uint64_t) right << 32) | left;
        block -= salt;

        return block;
    }

    //------------------------------------------------------------------------------
    static void keysched00(uint64_t key, uint32_t kext[ 4 ], uint8_t protocol)
    {
        kext[ 0 ] = (uint32_t)(key >> 32);
        kext[ 1 ] = (uint32_t)key;
        kext[ 2 ] = 0x08090a0b;
        kext[ 3 ] = 0x0c0d0e0f;

        uint32_t chain = (protocol & 0x0c) ? 0x84e5c4e7 : 0x6aa32b6f;
        for (int i = 0; i < 8; i++) {
            kext[ i & 3 ] = chain = round00(kext[ i & 3 ], chain, 0);
        }
    }

    //------------------------------------------------------------------------------
    static void keysched40(uint64_t key, uint32_t kext[ 16 ], uint8_t protocol)
    {
        // key ~ 01234567; left ~ 6420; right ~ 7531;
        key = ((key & 0x00ffff0000ffff00LL) |
                (key & 0xff000000ff000000LL) >> 24 |
                (key & 0x000000ff000000ffLL) << 24);
        key = ((key & 0x0000ffffffff0000LL) |
                (key & 0xffff000000000000LL) >> 48 |
                (key & 0x000000000000ffffLL) << 48);

        uint32_t left  = (uint32_t)(key >> 32);
        uint32_t right = (uint32_t)key;

        for (int i = 0; i < 16 ; i++) {
            uint32_t s = sbox40(right);
            s = ((0x00ffff00 & (s ^ left)) |
                (0xff0000ff & ((s & 0xff0000ff) + (left & 0xff0000ff))));
            left  = right;
            right = (s >> 8) | (s << 24);
            kext[ i ] = s;
        }
    }

    //------------------------------------------------------------------------------
    static void cipher00(uint8_t *out, const uint8_t *in, uint32_t len, uint64_t key, uint8_t protocol, bool encrypt)
    {
        uint32_t kext[ 4 ];
        keysched00(key, kext, protocol);

        uint64_t chain = 0xfe27199919690911LL;
        if (encrypt) {
            while (len >= 8) {
                uint64_t plain = ld_be64(in);
                uint64_t crypt = block00(plain ^ chain, kext, protocol, true);
                st_be64(out, crypt);
                chain = crypt;
                in += 8; out += 8; len -= 8;
            }
        } else {
            while (len >= 8) {
                uint64_t crypt = ld_be64(in);
                uint64_t plain = block00(crypt, kext, protocol, false) ^ chain;
                st_be64(out, plain);
                chain = crypt;
                in += 8; out += 8; len -= 8;
            }
        }
        if (len > 0) {
            chain = block00(chain, kext, protocol, true);
            st_be64(out, ld_be64(in, len) ^ chain, len);
        }
    }

    //------------------------------------------------------------------------------
    static void cipher40(uint8_t *out, const uint8_t *in, uint32_t len,
                            uint64_t key, uint8_t protocol, bool encrypt)
    {
        uint32_t kext[ 16 ];
        keysched40(key, kext, protocol);

        uint64_t chain = 0x11096919991927feLL;
        if (encrypt) {
            while (len >= 8) {
                uint64_t plain = ld_le64(in);
                uint64_t crypt = block40(plain ^ chain, kext, protocol, true);
                st_le64(out, crypt);
                chain = crypt;
                in += 8; out += 8; len -= 8;
            }
        } else {
            while (len >= 8) {
                uint64_t crypt = ld_le64(in);
                uint64_t plain = block40(crypt, kext, protocol, false) ^ chain;
                st_le64(out, plain);
                chain = crypt;
                in += 8; out += 8; len -= 8;
            }
        }
        if (len > 0) {
            chain = block40(chain, kext, protocol, true);
            st_le64(out, ld_le64(in, len) ^ chain, len);
        }
    }

    //------------------------------------------------------------------------------
    static uint32_t digest00(uint64_t key, const uint8_t *in, uint32_t len, uint8_t protocol)
    {
        uint64_t mac = 0;
        while (len >= 8) {
            mac ^= ld_be64(in);
            mac ^= (uint64_t)round00((uint32_t)mac, (uint32_t)(key >> 32), 3) << 32;
            mac ^= (uint64_t)round00((uint32_t)(mac >> 32), (uint32_t)key, 3);
            in += 8; len -= 8;
        }
        if (len > 0) {
            mac ^= ld_be64(in, len);
            mac ^= (uint64_t)round00((uint32_t)mac, (uint32_t)(key >> 32), 3) << 32;
            mac ^= (uint64_t)round00((uint32_t)(mac >> 32), (uint32_t)key, 3);
        }

        return (uint32_t)mac;
    }

    //------------------------------------------------------------------------------
    static uint32_t digest40(uint64_t key, const uint8_t *in, uint32_t len, uint8_t protocol)
    {
        key = reverse64(key);
        key += (uint64_t)((len + 7) / 8 * 2 + 2) << 32;

        uint64_t salt = (protocol & 0x0c) ? 0xfbe852461acd3970LL : 0xd34c027be8579632LL;
        uint32_t mac = (uint32_t)salt;

        mac += round40((uint32_t)key, (uint32_t)mac);
        while (len >= 8) {
            uint64_t text = ld_le64(in);
            mac += round40((uint32_t)text, mac);
            mac += round40((uint32_t)(text >> 32), mac);
            in += 8; len -= 8;
        }
        if (len > 0) {
            uint64_t text = ld_le64(in, len);
            mac += round40((uint32_t)text, mac);
            mac += round40((uint32_t)(text >> 32), mac);
        }
        mac += round40((salt >> 32) + (key >> 32), mac);

        return reverse32(mac);
    }

    //==============================================================================
    void encrypt(uint8_t *out, const uint8_t *in, uint32_t len, uint64_t key, uint8_t protocol)
    {
        if (protocol & 0x40) {
            cipher40(out, in, len, key, protocol, true);
        } else {
            cipher00(out, in, len, key, protocol, true);
        }
    }

    //==============================================================================
    void decrypt(uint8_t *out, const uint8_t *in, uint32_t len, uint64_t key, uint8_t protocol)
    {
        if (protocol & 0x40) {
            cipher40(out, in, len, key, protocol, false);
        } else {
            cipher00(out, in, len, key, protocol, false);
        }
    }

    //==============================================================================
    uint32_t digest(uint8_t protocol, uint64_t key, const uint8_t *in, uint32_t len)
    {
        uint32_t mac;
        if (protocol & 0x40) {
            mac = digest40(key, in, len, protocol);
        } else {
            mac = digest00(key, in, len, protocol);
        }
        return mac;
    }
}
