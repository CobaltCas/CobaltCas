#pragma once

#define _CRT_SECURE_NO_WARNINGS 1  // Do not display warnings related to printf functions in VC++
using namespace std;               // Omitting std:: in namespace for readability

// Number of BGID entities definition
#define BGID_COUNT 32
#define BGID_TEMP 32

// Maximum data length for each message definition
#define EMD_DATA_MAX_LENGTH 118  // Maximum length of EMD data
#define EMG_DATA_MAX_LENGTH 118  // Maximum length of EMG data
#define EMM_DATA_MAX_LENGTH 118  // Maximum length of EMM data
#define ECM_DATA_MAX_LENGTH 118  // Maximum length of ECM data
#define CHK_DATA_MAX_LENGTH 118  // Maximum length of CHK data

// INS parameter definition
#define INS_INT 0x30  // Initial setting condition
#define INS_ECM 0x34  // ECM reception
#define INS_EMM 0x36  // EMM reception
#define INS_CHK 0x3C  // Contract confirmation
#define INS_EMG 0x38  // EMM individual message reception
#define INS_EMD 0x3A  // Automatic display message display information acquisition
#define INS_PVS 0x40  // PPV status request
#define INS_PPV 0x42  // PPV program purchase
#define INS_PRP 0x44  // Prepaid balance confirmation
#define INS_CRQ 0x50  // Card request confirmation
#define INS_TLS 0x52  // Call connection status notification
#define INS_RQD 0x54  // Data request
#define INS_CRD 0x56  // Center response
#define INS_UDT 0x58  // Call date and time request
#define INS_UTN 0x5A  // Call destination confirmation
#define INS_UUR 0x5C  // User call request
#define INS_IRS 0x70  // DIRD data communication start
#define INS_CRY 0x72  // DIRD data encryption
#define INS_UNC 0x74  // DIRD response data decryption
#define INS_IRR 0x76  // DIRD data communication end
#define INS_WUI 0x80  // Power control information request
#define INS_IDI 0x32  // Card ID information acquisition

#define INS_OPEN 0xFF  // Internal use only

// Log output attribute definition
#define LOG_ALL  0x0001
#define LOG_EMM  0x0002
#define LOG_EMG  0x0004
#define LOG_EMD  0x0008
#define LOG_ECM  0x0010
#define LOG_CHK  0x0020
#define LOG_OPEN 0x0040
#define LOG_ETC  0x0080

// Message control area status definition
#define MSG_STS_DoNotShow 0x00                          // Not displayed
#define MSG_STS_Unused    0x01                          // Unused
#define MSG_STS_DuringDisplayGracePeriodOrDisplay 0x02  // During display grace period or display
#define MSG_STS_Showing   0x03                          // Displaying

#include <string>
#include "utils.h"
#include "key_manager.h"
#include "ldst.h"
#include "crypto.h"
#include "card.h"
#include "log.h"

// Common variables in the system
struct System {
    uint8_t INS;                       // Currently executing INS code (0xFF: during startup)
    Cas::KeyManager keySets;           // Work key information
    uint8_t cardVersion;               // Card version number being emulated (default value is 2, 3 is partially supported)
    uint16_t logMode;                  // Log mode (0: disable, 1: all, 2: EMM, 4: EMG, 8: EMD, 16: ECM, 32: CHK, 64: startup, 128: other)
    bool clModeEnable;                 // CL mode enable/disable
    uint64_t initGroupID[8];           // Group ID to be applied to the card image at initial startup / [0]: main ID
    uint64_t initGroupIDKm[8];         // Group ID Km to be applied to the card image at initial startup / [0]: main Km
#ifdef _WIN32
    string CARD_IMAGE_FILE_NAME;       // Card image save file name (*.bin)
    string LOG_FILE_NAME;              // Log file name (*.log)
#else
    const char *CARD_IMAGE_FILE_NAME;  // Card image save file name (*.bin)
    const char *LOG_FILE_NAME;         // Log file name (*.log)
#endif
};

#ifdef _WINSCARD_CPP_
    struct System sys;
#else
    extern struct System sys;
#endif

#ifndef _WIN32

// for Linux
#include <cstdarg>
#include <string.h>
#include <unistd.h>
#include <PCSC/wintypes.h>

// GCC produces errors on calling convention attributes
#define __cdecl
#define __CRTDECL
#define __stdcall
#define __vectorcall
#define __thiscall
#define __fastcall
#define __clrcall
#define WINAPI __stdcall
#define CALLBACK __stdcall

// Ported from Win32 API
typedef char CHAR;
#define ERROR_INVALID_HANDLE 6L
#define ERROR_NOT_SUPPORTED 50L
#define Sleep(ms) usleep((ms) * 1000)

#endif
