#include "project.h"
#include <map>
#include <vector>
#include <algorithm>

namespace Cas {

    KeyManager::KeyManager()
    {
        clear();
    }

    KeyManager::~KeyManager()
    {
        clear();
    }

    void KeyManager::clear(void)
    {
        for (uint8_t BroadcastGroupID = 0; BroadcastGroupID < BGID_COUNT; BroadcastGroupID++) {
            keySet[BroadcastGroupID].BroadcastGroupID = BroadcastGroupID ^ 0xff;
            keySet[BroadcastGroupID].Keys.clear();
        }
    }

    uint64_t KeyManager::getWorkKey(uint8_t BroadcastGroupID, uint8_t WorkKeyID)
    {
        if (BroadcastGroupID >= BGID_COUNT) return 0;
        KwSet_t *ks = &keySet[BroadcastGroupID];
        if (ks->BroadcastGroupID != BroadcastGroupID) return 0;
        if (ks->Keys.find(WorkKeyID) == ks->Keys.end()) return 0;
        return keySet[BroadcastGroupID].Keys[WorkKeyID].Key;
    }

    static bool cmp(const KeyManager::Kw_t& a, const KeyManager::Kw_t& b) {
        return (b.WorkKeyID <= a.WorkKeyID);
    }
    static bool cmp2(const KeyManager::Kw_t& a, const KeyManager::Kw_t& b) {
        return ((b.WorkKeyID == a.WorkKeyID) && (b.Key == a.Key));
    }

    int KeyManager::getWorkKeyList(uint8_t BroadcastGroupID, vector<Kw_t>& list)
    {
        if (BroadcastGroupID >= BGID_COUNT) return 0;
        KwSet_t *ks = &keySet[BroadcastGroupID];
        if (ks->BroadcastGroupID != BroadcastGroupID) return 0;
        for (map<uint8_t, Kw_t>::iterator i = ks->Keys.begin(); i != ks->Keys.end(); i++) {
            list.push_back(i->second);
        }
        sort(list.begin(), list.end(), cmp);
        list.erase(unique(list.begin(), list.end(), cmp2), list.end());
        return (int)list.size();
    }

    bool KeyManager::registerWorkKey(uint8_t BroadcastGroupID, Kw_t& WorkKey)
    {
        if (BroadcastGroupID >= BGID_COUNT) return false;
        if (WorkKey.Key == 0) return false;

        KwSet_t *ks = &keySet[BroadcastGroupID];
        if (ks->Keys.find(WorkKey.WorkKeyID) == ks->Keys.end()) {
            ks->BroadcastGroupID = BroadcastGroupID;
            ks->Keys[WorkKey.WorkKeyID].WorkKeyID = WorkKey.WorkKeyID;
            ks->Keys[WorkKey.WorkKeyID].Key = WorkKey.Key;
            return true;
        } else {
            if (ks->Keys[WorkKey.WorkKeyID].Key != WorkKey.Key) {
                ks->Keys[WorkKey.WorkKeyID].Key = WorkKey.Key;
                return true;
            }
        }

        return false;
    }
}
