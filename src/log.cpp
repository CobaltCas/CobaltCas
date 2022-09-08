#include "project.h"
#include <fstream>
#include <sstream>
#include <stddef.h>

#define LOG_BUFFER_FLASH_SIZE 2048  // An approximate timing to flush the log file
#define membersizeof(st, member) sizeof(((st *)0)->member)  // Macro to get the size of a structure member
#define addrItem(st, member, name) { offsetof(st, member), membersizeof(st, member), name }

namespace Log {

    typedef struct {
        size_t offset;
        size_t size;
        const char *name;
    } mem_t;

    static string logBuffer;
    static bool logInit = false;

    static void _logout(const char *fmt, va_list ap)
    {
        char txt[1024];

        if (!logInit) {
            logInit = true;
            logBuffer.clear();
        }

        if (fmt) {
            vsnprintf(txt, sizeof(txt), fmt, ap);
            if (txt[0]) {
                logBuffer.append(txt);
#if false
                fprintf(stderr, "%s", txt);  // For debugging
#endif
            }
        }

        if ((fmt == NULL) || (logBuffer.size() >= LOG_BUFFER_FLASH_SIZE)) {
            ofstream fs(sys.LOG_FILE_NAME, ios::app);
            if (fs) fs << logBuffer << std::flush;
            logBuffer.clear();
        }
    }

    // Append to the regular log file (same format as printf())
    void logout(const char *fmt, ...)
    {
        bool logEnable = false;
        logEnable = (sys.logMode == LOG_ALL);                                             // Is all log output enabled?

        if (!logEnable) logEnable = ((sys.logMode & LOG_EMM) && (sys.INS == INS_EMM));    // Is EMM log output enabled?
        if (!logEnable) logEnable = ((sys.logMode & LOG_EMG) && (sys.INS == INS_EMG));    // Is EMG log output enabled?
        if (!logEnable) logEnable = ((sys.logMode & LOG_EMD) && (sys.INS == INS_EMD));    // Is EMD log output enabled?
        if (!logEnable) logEnable = ((sys.logMode & LOG_ECM) && (sys.INS == INS_ECM));    // Is ECM log output enabled?
        if (!logEnable) logEnable = ((sys.logMode & LOG_CHK) && (sys.INS == INS_CHK));    // Is CHK log output enabled?
        if (!logEnable) logEnable = ((sys.logMode & LOG_OPEN) && (sys.INS == INS_OPEN));  // Is startup log output enabled?
        if ((!logEnable) && (sys.logMode & LOG_ETC)) {                                    // Is other command output enabled?
            logEnable = ((sys.INS != INS_EMM) && (sys.INS != INS_EMG) && (sys.INS != INS_EMD) && (sys.INS != INS_ECM) && (sys.INS != INS_CHK)  && (sys.INS != INS_OPEN));
        }

        if (logEnable) {
            va_list ap;
            if (fmt) va_start(ap, fmt);
            _logout(fmt, ap);
            if (fmt) va_end(ap);
        }
    }

    // Append send/receive data to the regular log file
    void logout_command_dump(const void *p, uint16_t size)
    {
        if (size <= 0) return;

        const uint8_t *pp = (const uint8_t *)p;
        for (uint16_t i = 0; i < size; i++) {
            logout((i == (size - 1)) ? "%02x" : "%02x ", *pp++);
        }
    }

    // Append data dump to the regular log file
    void logout_dump(const void *p, uint16_t size, uint16_t tabCount)
    {
        if (size <= 0) return;

        const uint8_t *pp = (const uint8_t *)p;
        for (uint16_t i = 0; i < size; i++) {
            if (tabCount) {
                if (!(i % 32) && i) {
                    logout("\n");
                    for (int j = 0; j < tabCount; j++) {
                        logout("    ");
                    }
                }
            }
            logout((((i % 32) == 31 && tabCount) || (i == (size - 1))) ? "%02X" : "%02X ", *pp++);
        }
        if (tabCount) logout("\n");
    }

    // Append contract bitmap information to the regular log file
    void logout_bitmap(const uint8_t *bitmap, uint16_t size, uint16_t spaceCount)
    {
        if (size <= 0) return;

        string buf;
        const uint8_t *p = bitmap;
        for (uint16_t i = 0;i < size; i++) {
            if (!(i % 8) && i) {
                buf.pop_back();
                buf.push_back('\n');
                for (uint16_t j = 0; j < spaceCount; j++) {
                    buf.push_back(' ');
                }
            }

            uint8_t dat = *p++;
            for (uint8_t j = 0; j < 8; j++) {
                buf.push_back((dat & 0x80) ? '1' : '0');
                dat <<= 1;
            }
            buf.push_back(' ');
        }

        buf.pop_back();
        Log::logout("%s", buf.c_str());
    }

    void logout_timestamp(void)
    {
        logout("---- %s ----------------\n", Utils::now_datetime_string());
    }

    // Create a log of the data sent from the card reader
    void logout_send_raw_data(const void *data, uint16_t size)
    {
        logout_timestamp();
        logout("> ");
        logout_command_dump(data, size);
        logout("\n");
    }

    // Create a log of the data received from the card reader
    void logout_receive_raw_data(const void *data, uint16_t size)
    {
        logout("< ");
        logout_command_dump(data, size);
        logout("\n\n");

        logout(NULL);  // Flush the file
    }

    // Output the name to which the specified memory address belongs to the log (still a brute force method)
    void _out_address_name(uint16_t base, uint16_t pos, const mem_t *items, int itemCount)
    {
        logout(" Offset 0x%04X [", base + pos);
        for (int i = 0; i < itemCount; i++) {
            const mem_t *p = &items[i];
            if (pos >= (uint16_t)p->offset && pos < (uint16_t)(p->offset + p->size)) {
                logout("%s", p->name);
                if (pos != (uint16_t)p->offset) logout("[+%u]", pos - (uint16_t)p->offset);
                break;
            }
        }
        logout("]\n");
    }

    void logout_address_name(uint16_t address)
    {
        // Header member information
        const static mem_t ITEMS_INFO[] = {  // 0x0000 ~ 0x0030
            addrItem(Cas::INFO_t, ca_system_id , "CA system ID"               ),  // 0x0000[2]
            addrItem(Cas::INFO_t, card_type,     "Card type"                  ),  // 0x0002[1]
            addrItem(Cas::INFO_t, version,       "Version"                    ),  // 0x0003[1]
            addrItem(Cas::INFO_t, unknown1     , "Unknown 1"                  ),  // 0x0004[21]
            addrItem(Cas::INFO_t, GroupID_Flag1, "Group ID flag1"             ),  // 0x0019[1]
            addrItem(Cas::INFO_t, GroupID_Flag2, "Group ID flag2"             ),  // 0x001A[11]
            addrItem(Cas::INFO_t, unknown2     , "Unknown 2"                  ),  // 0x001B[5]
            addrItem(Cas::INFO_t, ID           , "Card ID"                    ),  // 0x0020[6]
            addrItem(Cas::INFO_t, ID_CheckDigit, "Card ID check digit"        ),  // 0x0026[2]
            addrItem(Cas::INFO_t, Km           , "Master key (Km)"            ),  // 0x0028[8]
            addrItem(Cas::INFO_t, Km_CheckDigit, "Master key (Km) check digit"),  // 0x0030[1]
        };

        // Header group ID member information
        const static mem_t ITEMS_GROUP_ID[] = {  // 7items / 0x0031 ~ 0x00a7
            addrItem(Cas::GROUP_ID_t, ID           , "Group card ID"                    ),
            addrItem(Cas::GROUP_ID_t, ID_CheckDigit, "Group card ID check digit"        ),
            addrItem(Cas::GROUP_ID_t, Km           , "Group master key (Km)"            ),
            addrItem(Cas::GROUP_ID_t, Km_CheckDigit, "Group master key (Km) check digit"),
        };

        // Unknown 8 bytes 0x00a8 ~ 0x00af (ALL 0xff ?)
        // Unknown 1 byte 0x00b0 ~ 0x00b0

        // Card status information?
        const static mem_t ITEMS_CARD_STATUS[] = {  // 0x00b1 ~ 0x00b4
            addrItem(Cas::CARD_STATUS_t, card_status1, "Card status 1"),
            addrItem(Cas::CARD_STATUS_t, card_status2, "Card status 2"),
        };

        // Unknown 75 bytes 0x00b5 ~ 0x00ff

        // Tier member information
        const static mem_t ITEMS_TIER[] = {  // 32items / 0x0100 ~ 0x0a05
            addrItem(Cas::TIER_t, ActivationState               , "Activation state"          ),
            addrItem(Cas::TIER_t, UpdateNumber1                 , "Update number"             ),
            addrItem(Cas::TIER_t, UpdateNumber2                 , "Update number2"            ),
            addrItem(Cas::TIER_t, ExpiryDate                    , "Expiry date"               ),
            addrItem(Cas::TIER_t, ExpiryHour                    , "Expiry hour"               ),
            addrItem(Cas::TIER_t, Keys[0].WorkKeyID             , "Odd work key ID"           ),
            addrItem(Cas::TIER_t, Keys[0].Key                   , "Odd work key"              ),
            addrItem(Cas::TIER_t, Keys[1].WorkKeyID             , "Even work key ID"          ),
            addrItem(Cas::TIER_t, Keys[1].Key                   , "Even work key"             ),
            addrItem(Cas::TIER_t, Bitmap                        , "Contract bitmap"           ),
            addrItem(Cas::TIER_t, unknown1                      , "Unknown 1"                 ),
            addrItem(Cas::TIER_t, PowerOn.PowerOnStartDateOffset, "Power on start date offset"),
            addrItem(Cas::TIER_t, PowerOn.PowerOnPeriod         , "Power on period"           ),
            addrItem(Cas::TIER_t, PowerOn.PowerSupplyHoldTime   , "Power supply hold time"    ),
            addrItem(Cas::TIER_t, PowerOn.ReceiveNetwork        , "Receive network"           ),
            addrItem(Cas::TIER_t, PowerOn.ReceiveTS             , "Receive TS"                ),
            addrItem(Cas::TIER_t, unknown2                      , "Unknown 2"                 ),
            addrItem(Cas::TIER_t, checkDigit                    , "Check digit"               ),
        };

        // Message control member information
        const static mem_t ITEMS_MESSAGE[] = {  // 32items / 0x13c0 ~ 0x1863
            addrItem(Cas::MESSAGE_t, status                     , "Status"                     ),
            addrItem(Cas::MESSAGE_t, UpdateNumber               , "Update number"              ),
            addrItem(Cas::MESSAGE_t, unknown1                   , "Unknown 1"                  ),
            addrItem(Cas::MESSAGE_t, start_date                 , "Start date"                 ),
            addrItem(Cas::MESSAGE_t, delayed_displaying_period  , "Delayed displaying period"  ),
            addrItem(Cas::MESSAGE_t, expiry_date                , "Expiry date"                ),
            addrItem(Cas::MESSAGE_t, fixed_phrase_message_number, "Fixed phrase message number"),
            addrItem(Cas::MESSAGE_t, diff_format_number         , "Diff format number"         ),
            addrItem(Cas::MESSAGE_t, diff_information_length    , "Diff information length"    ),
            addrItem(Cas::MESSAGE_t, diff_information           , "Diff information"           ),
            addrItem(Cas::MESSAGE_t, checkDigit                 , "Check digit"                ),
        };

        logout("0x%04X: ", address);

        // Header section
        if ((address >= INFO_ADDR) && (address < (INFO_ADDR + sizeof(Cas::INFO_t)))) {
            uint16_t pos = address - INFO_ADDR;
            logout("Header section");
            if (pos < offsetof(Cas::INFO_t, grpID)) {
                _out_address_name(INFO_ADDR, pos, ITEMS_INFO, sizeof(ITEMS_INFO) / sizeof(mem_t));
            } else if (pos >= (INFO_ADDR + offsetof(Cas::INFO_t, grpID)) && pos < (INFO_ADDR + offsetof(Cas::INFO_t, grpID) + membersizeof(Cas::INFO_t, grpID))) {
                pos -= offsetof(Cas::INFO_t, grpID);
                uint16_t idx = pos / sizeof(Cas::GROUP_ID_t);
                pos %= sizeof(Cas::GROUP_ID_t);
                logout(" Group ID [%u]", idx + 1);
                _out_address_name(INFO_ADDR + offsetof(Cas::INFO_t, grpID), pos, ITEMS_GROUP_ID, sizeof(ITEMS_GROUP_ID) / sizeof(mem_t));
            }
            return;

        // Card status section
        } else if ((address >= CARD_STATUS_ADDR) && (address < (CARD_STATUS_ADDR + sizeof(Cas::CARD_STATUS_t)))) {
            uint16_t pos = address - CARD_STATUS_ADDR;
            _out_address_name(CARD_STATUS_ADDR, pos, ITEMS_CARD_STATUS, sizeof(ITEMS_CARD_STATUS) / sizeof(mem_t));

        // Tier section
        } else if ((address >= TIER_ADDR) && (address < (TIER_ADDR + (sizeof(Cas::TIER_t) * BGID_COUNT) + sizeof(Cas::TIER_t)))) {
            uint16_t idx = (address - TIER_ADDR) / sizeof(Cas::TIER_t);
            uint16_t pos = (address - TIER_ADDR) - (idx * sizeof(Cas::TIER_t));
            logout("Tier%s section [0x%02X]",(idx == BGID_COUNT) ? " temporary" : "", idx);
            _out_address_name(TIER_ADDR, pos, ITEMS_TIER, sizeof(ITEMS_TIER) / sizeof(mem_t));
            return;

        // Message section
        } else if ((address >= MESSAGE_ADDR) && (address < (MESSAGE_ADDR + (sizeof(Cas::MESSAGE_t) * BGID_COUNT) + sizeof(Cas::MESSAGE_t)))) {
            uint16_t idx = (address - MESSAGE_ADDR) / sizeof(Cas::MESSAGE_t);
            uint16_t pos = (address - MESSAGE_ADDR) - (idx * sizeof(Cas::MESSAGE_t));
            logout("Message control%s section [0x%02X]", (idx == BGID_COUNT) ? " temporary" : "", idx);
            _out_address_name(MESSAGE_ADDR, pos, ITEMS_MESSAGE, sizeof(ITEMS_MESSAGE) / sizeof(mem_t));
            return;

        // Unknown
        } else {
            logout("Unknown section\n");
        }
    }
}
