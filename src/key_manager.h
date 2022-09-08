#pragma once

#include <map>
#include <inttypes.h>

namespace Cas {

    class KeyManager {
        public:
        typedef struct {
            uint8_t WorkKeyID;
            uint64_t Key;
        } Kw_t;

        KeyManager();
        ~KeyManager();
        void clear(void);
        bool registerWorkKey(uint8_t BroadcastGroupID, Kw_t& WorkKey);
        uint64_t getWorkKey(uint8_t BroadcastGroupID, uint8_t WorkKeyID);
        int getWorkKeyList(uint8_t BroadcastGroupID, vector<Kw_t>& list);

        private:
        typedef struct {
            uint8_t BroadcastGroupID;
            map<uint8_t, Kw_t> Keys;
        } KwSet_t;
        KwSet_t keySet[BGID_COUNT];
    };
}
