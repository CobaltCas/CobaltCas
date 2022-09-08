#pragma once

#include <inttypes.h>

namespace Crypto {

    void encrypt(uint8_t *out, const uint8_t *in, uint32_t len, uint64_t key, uint8_t protocol);
    void decrypt(uint8_t *out, const uint8_t *in, uint32_t len, uint64_t key, uint8_t protocol);
    uint32_t digest(uint8_t protocol, uint64_t key, const uint8_t *in, uint32_t len);
}
