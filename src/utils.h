#pragma once

#ifdef _WIN32
#include <windows.h>
#endif
#include <inttypes.h>
#include <vector>
#include "card.h"

namespace Utils {

    long date_to_mjd(int y, int m, int d);
    void mjd_to_date(int *y, int *m, int *d, int mjd);
#ifdef _WIN32
    string get_dll_file_name(HINSTANCE hn);
#endif
    bool load_card_image(uint8_t *buf);
    bool save_card_image(const uint8_t *buf);
    uint16_t calc_cardID_check_digit(uint64_t id);
    uint8_t calc_Km_check_digit(uint64_t Km);
    uint8_t calc_tier_check_digit(Cas::TIER_t *pTier);
    uint8_t calc_message_check_digit(Cas::MESSAGE_t *pMessage);
    const char *cardID_to_string(uint64_t id, int *sts = NULL);
    uint64_t string_to_cardID(const char *idText, int *sts = NULL);
    const char *mjd_to_string(int mjd);
    const char *time_to_string(uint8_t *p);
    const char *now_datetime_string(void);
    const char *BroadcastGroupID_to_name(uint8_t BroadcastGroupID);
    bool is_all_zero(void *data, int size);
}
