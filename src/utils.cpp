#include "project.h"
#include <fstream>
#include <stdio.h>
#include <time.h>

namespace Utils {

    // Convert date to Modified Julian Day (MJD)
    long date_to_mjd(int y, int m, int d)
    {
        // Formula by Fliegel
        long a = (m <= 2) ? 1L : 0L;
        long b = (36525L * ((long)y - a)) / 100L;
        long c = (((long)y - a) / 400L) + (3059L * ((a * 12L) + m - 2L) / 100L) + d - (((long)y - a) / 100L);
        return b + c - 678912L;
    }

    // Convert Modified Julian Day (MJD) to date
    void mjd_to_date(int *y, int *m, int *d, int mjd)
    {
        // B-CAS MJD is 16-bit
        // Originally, only dates from 1858/11/17 to 2038/04/22 are supported
        // The handling after 2038/04/22 is not clearly defined, so the range is set to (2001/03/01 to 2179/08/05)
        mjd %= 65536;  // Truncate the argument to a 16-bit value
        if (mjd < 51604) mjd += 65536;  // Offset to April 23, 2038 for dates before March 1, 2001

        // Convert Modified Julian Day (MJD) to Gregorian calendar
        uint32_t jd = mjd + 2400001;  // Originally 240000.5, but rounded to an integer for calculation
        uint32_t f = jd + 1401 + (((4 * jd + 274277) / 146097) * 3) / 4 - 38;
        uint32_t e = 4 * f + 3;
        uint32_t g = (e % 1461) / 4;
        uint32_t h = 5 * g + 2;
        *d = (h % 153) / 5 + 1;
        *m = ((h / 153) + 2) % 12 + 1;
        *y = (e / 1461) - 4716 + (12 + 2 - *m) / 12;
    }

#ifdef _WIN32
    // Get DLL file name without extension (ex. C:\Windows\test.dll --> C:\Windows\test)
    string get_dll_file_name(HMODULE hn)
    {
        string path;
        char buf[MAX_PATH];
        GetModuleFileNameA(hn, buf, sizeof(buf));
        string tmp = buf;
        size_t pos = tmp.rfind('.');
        if (pos != string::npos) path.append(tmp.substr(0, pos));
        return path;
    }
#endif

    // Load card image file (7680 bytes)
    bool load_card_image(uint8_t *buf)
    {
        ifstream fs(sys.CARD_IMAGE_FILE_NAME, ios::in | ios::binary);
        if (!fs) return false;
        fs.read((char *)buf, 7680);
        return true;
    }

    // Save card image file (7680 bytes)
    bool save_card_image(const uint8_t *buf)
    {
        ofstream fs(sys.CARD_IMAGE_FILE_NAME, ios::out | ios::binary | ios::trunc);
        if (!fs) return false;
        fs.write((char *)buf, 7680);
        fs.flush();
        return true;
    }

    // Calculate Km check digit
    uint8_t calc_Km_check_digit(uint64_t Km)  // 64-bit value
    {
        uint8_t chk = 0;
        const uint8_t *p = (const uint8_t *)&Km;
        for (int i = 0; i < 8; i++) chk ^= *p++;
        return chk;
    }

    // Calculate Tier check digit
    uint8_t calc_tier_check_digit(Cas::TIER_t *pTier)  // Head address of each broadcast group's Tier
    {
        uint8_t chk = 0;
        const uint8_t *p = (const uint8_t *)pTier;
        for (size_t i = 0; i < sizeof(Cas::TIER_t) - 1; i++) chk ^= *p++;
        return chk;
    }

    // Calculate Message check digit
    uint8_t calc_message_check_digit(Cas::MESSAGE_t *pMessage)  // Head address of each broadcast group's Message
    {
        uint8_t chk = 0;
        const uint8_t *p = (const uint8_t *)pMessage;
        for (size_t i = 0; i < sizeof(Cas::MESSAGE_t) - 1; i++) chk ^= *p++;
        return chk;
    }

    // Calculate check digit of card ID
    uint16_t calc_cardID_check_digit(uint64_t id)  // 48-bit
    {
        uint16_t chk = 0;
        for (int i = 0; i < 3; i++) {
            chk ^= (uint16_t)(id & 0xffff);
            id >>= 16;
        }
        return chk;
    }

    // Convert card imprint format string "0000-0000-0000-0000-0000" to card ID (48-bit)
    // If successful, the group ID is returned in sts. If failed, a negative number is returned. (NULL can be specified)
    uint64_t string_to_cardID(const char *idText, int *sts)
    {
        int part[5];
        // bool allZero = true;
        if (sscanf(idText, "%u-%u-%u-%u-%u", &part[0], &part[1], &part[2], &part[3], &part[4]) != 5) {  // Divide into parts
            if (sts != NULL) *sts = -1;
            return 0;
        }

        for (int i = 0; i < 5; i++) {  // Check each part value
            if ((part[i] < 0) || (part[i] > 9999)) {
                if (sts != NULL) *sts = -2;
                return 0;
            } else if (part[i] > 0) {
                // allZero = false;
            }
        }

        uint64_t id = (uint64_t)(part[0] % 1000);
        id *= 10000; id += (uint64_t)part[1];
        id *= 10000; id += (uint64_t)part[2];
        id *=  1000; id += (uint64_t)(part[3] / 10);
        if (id > 0x1fffffffffffLLU) {  // If the card number exceeds 45 bits, return an error
            if (sts != NULL) *sts = -3;
            return 0;
        } else if (id == 0) {  // If the value is 0, return an error
            if (sts != NULL) *sts = -4;
            return 0;
        }

        uint64_t GroupID = (uint64_t)(part[0] / 1000);  // Get group ID (card type?)
        if (GroupID > 7) {  // If the value exceeds 3 bits, return an error
            if (sts != NULL) *sts = -5;
            return 0;
        }

        if (sts != NULL) *sts = (int)GroupID;
        return id | (GroupID << 45);  // Return ID (48-bit) with group ID added
    }

    // Convert card ID to card imprint format string "0000-0000-0000-0000-0000" (check digit is automatically completed)
    // Pass 48-bit ID / sts returns group ID (NULL can be specified)
    const char *cardID_to_string(uint64_t id, int *sts)
    {
        static char txt[32];

        const uint16_t checkDigit = calc_cardID_check_digit(id);  // Calculate check digit
        const uint8_t GroupID = (uint8_t)(id >> 45);  // Get group ID (card type?)
        uint64_t maskID = id & 0x1fffffffffff;  // Card ID itself
        int part[5];

        part[4] =  (int)(checkDigit % 10000);
        part[3] =  (int)(checkDigit / 10000);
        part[3] += (int)(maskID %  1000) * 10; maskID /=  1000;
        part[2]  = (int)(maskID % 10000);      maskID /= 10000;
        part[1]  = (int)(maskID % 10000);      maskID /= 10000;
        part[0]  = (int)(maskID % 10000);
        part[0] += (int)GroupID * 1000;

        sprintf(txt, "%04u-%04u-%04u-%04u-%04u", part[0], part[1], part[2], part[3], part[4]);
        if (sts != NULL) *sts = (int)GroupID;

        return txt;
    }

    // Convert MJD to date string
    const char *mjd_to_string(int mjd)
    {
        static char txt[32];
        int y, m, d;
        mjd_to_date(&y, &m, &d, mjd);
        sprintf(txt, "0x%04X (%04d-%02d-%02d)", (uint16_t)(mjd & 0xffff), y, m, d);
        return txt;
    }

    // Convert time data (BCD format 3 bytes) to string
    const char *time_to_string(uint8_t *p)
    {
        static char txt[20];
        int h = ((p[0] >> 4) * 10) + (p[0] & 0x0f);
        int m = ((p[1] >> 4) * 10) + (p[1] & 0x0f);
        int s = ((p[2] >> 4) * 10) + (p[2] & 0x0f);
        sprintf(txt, "0x%02X%02X%02X (%02u:%02u:%02u)", p[0], p[1], p[2], h, m, s);
        return txt;
    }

    // Return current date and time as a string ("yyyy-mm-dd hh:mm:ss")
    const char *now_datetime_string(void)
    {
        static char txt[32];
        time_t now = time(NULL);
        strftime(txt, sizeof(txt), "%Y-%m-%d %H:%M:%S", localtime(&now));
        return txt;
    }

    // Return broadcast group ID as a name string
    const char *BroadcastGroupID_to_name(uint8_t BroadcastGroupID)
    {
        static const char msg01[] = "NHK-BS";
        static const char msg02[] = "WOWOW";
        static const char msg03[] = "Star Channel";
        static const char msg17[] = "SKY PerfecTV!";
        static const char msg1d[] = "Terrestrial Digital Television Reception Countermeasure Satellite Broadcasting [Ended in March 2015]";
        static const char msg1e[] = "NHK/Commercial broadcasting";
        static const char msg20[] = "For temporary use";
        static const char msgFF[] = "Unknown broadcast group ID";

        static const char *names[] = {
            msgFF, msg01, msg02, msg03, msgFF, msgFF, msgFF, msgFF, msgFF, msgFF, msgFF, msgFF, msgFF, msgFF, msgFF, msgFF,
            msgFF, msgFF, msgFF, msgFF, msgFF, msgFF, msgFF, msg17, msgFF, msgFF, msgFF, msgFF, msgFF, msg1d, msg1e, msgFF,
            msg20
        };

        if (BroadcastGroupID <= BGID_COUNT) {
            return names[BroadcastGroupID];
        }

        return msgFF;
    }

    bool is_all_zero(void *data, int size)
    {
        const uint8_t *p = (const uint8_t *)data;
        for (int i = 0; i < size; i++) {
            if (*p++) return false;
        }
        return true;
    }
}
