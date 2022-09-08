#pragma once

#ifdef _WIN32
#include <windows.h>
#else
#include <PCSC/wintypes.h>
#endif
#include <inttypes.h>
#include "key_manager.h"

// The starting address of the area in the card image
#define INFO_ADDR          (0x0000)
#define INFO_ADDR_GROUP_ID (0x0031)
#define CARD_STATUS_ADDR   (0x00b1)
#define TIER_ADDR          (0x0100)
#define MESSAGE_ADDR       (0x13c0)

namespace Cas {

    // Structure of the ID area in the card image

    // Structure of the header area in the card image
    typedef struct {
        uint8_t ID[6];
        uint8_t ID_CheckDigit[2];
        uint8_t Km[8];
        uint8_t Km_CheckDigit;
    } GROUP_ID_t;

    // Card status storage area
    typedef struct {
        uint8_t card_status1[2];        // card_status1 and card_status2 must not have the same value, or an error will occur
        uint8_t card_status2[2];
    } CARD_STATUS_t;

    typedef struct {
        uint8_t ca_system_id[2];        // Not verified
        uint8_t card_type;              // Not verified
        uint8_t version;                // Not verified
        uint8_t unknown1[21];
        uint8_t GroupID_Flag1;          // Valid ID flag including the main ID?
        uint8_t GroupID_Flag2;          // Valid ID flag excluding the main ID
        uint8_t unknown2[5];
        uint8_t ID[6];                  // Main ID
        uint8_t ID_CheckDigit[2];
        uint8_t Km[8];                  // Main Km
        uint8_t Km_CheckDigit;
        GROUP_ID_t grpID[7];            // Group ID
    } INFO_t;

    // Structure of the contract information area in the card image
    typedef struct {
        uint8_t ActivationState;
        uint8_t UpdateNumber1[2];
        uint8_t UpdateNumber2[2];       // Unused 0x0000?
        uint8_t ExpiryDate[2];
        uint8_t ExpiryHour;

        struct {
            uint8_t WorkKeyID;
            uint8_t Key[8];
        } Keys[2];

        uint8_t Bitmap[32];
        uint8_t unknown1[3];

        struct {
            uint8_t PowerOnStartDateOffset;
            uint8_t PowerOnPeriod;
            uint8_t PowerSupplyHoldTime;
            uint8_t ReceiveNetwork[2];
            uint8_t ReceiveTS[2];
        } PowerOn;

        uint8_t unknown2;
        uint8_t checkDigit;
    } TIER_t;

    // Structure of the message control area in the card image
    typedef struct {
        uint8_t status;
        uint8_t UpdateNumber[2];
        uint8_t unknown1[2];
        uint8_t start_date[2];
        uint8_t delayed_displaying_period;
        uint8_t expiry_date[2];
        uint8_t fixed_phrase_message_number[2];  // first byte is broadcast group ID
        uint8_t diff_format_number;
        uint8_t diff_information_length[2];
        uint8_t diff_information[20];
        uint8_t checkDigit;
    } MESSAGE_t;

    class Card {
    private:
        uint8_t cardImage[7680];

        bool ul = false;
        uint8_t ulStatus = 0x00;
        bool selectBC01 = false;

        void processNano10(TIER_t *pT, uint8_t BroadcastGroupID, uint8_t *p, bool updateEnable);
        void processNano11(TIER_t *pT, uint8_t *p, bool updateEnable);
        void processNano13(uint8_t *p, bool updateEnable);
        void processNano14(uint8_t *p, bool updateEnable);
        void processNano20(TIER_t *pT, uint8_t *p, bool updateEnable);
        void processNano21(TIER_t *pT, MESSAGE_t *pMSG, uint8_t *p, uint8_t Ins, bool updateEnable);
        void processNano23(TIER_t *pT, uint8_t *p);
        void processNano51(uint16_t date, TIER_t *pT, uint8_t *p);
        bool processNano52(TIER_t *pT, uint8_t *p);

        INFO_t* pINFO(void);
        CARD_STATUS_t * pCARDSTATUS(void);
        TIER_t* pTIER(uint8_t BroadcastGroupID);
        MESSAGE_t* pMESSAGE(uint8_t BroadcastGroupID);
        void setupCardImage(uint64_t *initID, uint64_t *initKm);
        void loadCardImage(uint64_t *initID, uint64_t *initKm);
        uint64_t findWorkKeyFromCardImage(uint8_t BroadcastGroupID, uint8_t WorkKeyID);
        uint64_t getWorkKey(uint8_t BroadcastGroupID, uint8_t WorkKeyID);
        bool updateTierWorkKey(TIER_t *pT, uint8_t WorkKeyID, uint64_t key);
        bool setWorkKey(uint8_t BroadcastGroupID, uint8_t WorkKeyID, uint64_t key, bool cardImageUpdate = true);
        int getID(uint64_t cardID, GROUP_ID_t **id);
        bool checkExpiryDate(TIER_t *pT, uint16_t date, uint8_t hour);
        void changeCardStatus(uint16_t sts);
        uint16_t getCardStatus(void);

    public:
        Card();
        ~Card();
        void saveCardImage(void);
        LONG processCmd30(LPCBYTE pbSendBuffer, DWORD cbSendLength, LPBYTE pbRecvBuffer, LPDWORD pcbRecvLength);
        LONG processCmd32(LPCBYTE pbSendBuffer, DWORD cbSendLength, LPBYTE pbRecvBuffer, LPDWORD pcbRecvLength);
        LONG processCmd34(LPCBYTE pbSendBuffer, DWORD cbSendLength, LPBYTE pbRecvBuffer, LPDWORD pcbRecvLength);
        LONG processCmd36(LPCBYTE pbSendBuffer, DWORD cbSendLength, LPBYTE pbRecvBuffer, LPDWORD pcbRecvLength);
        LONG processCmd38(LPCBYTE pbSendBuffer, DWORD cbSendLength, LPBYTE pbRecvBuffer, LPDWORD pcbRecvLength);
        LONG processCmd3A(LPCBYTE pbSendBuffer, DWORD cbSendLength, LPBYTE pbRecvBuffer, LPDWORD pcbRecvLength);
        LONG processCmd3C(LPCBYTE pbSendBuffer, DWORD cbSendLength, LPBYTE pbRecvBuffer, LPDWORD pcbRecvLength);
        LONG processCmd80(LPCBYTE pbSendBuffer, DWORD cbSendLength, LPBYTE pbRecvBuffer, LPDWORD pcbRecvLength);
        bool processULCMD(LPCBYTE pbSendBuffer, DWORD cbSendLength, LPBYTE pbRecvBuffer, LPDWORD pcbRecvLength);
        bool processReadBC01(LPCBYTE pbSendBuffer, DWORD cbSendLength, LPBYTE pbRecvBuffer, LPDWORD pcbRecvLength);
        bool processWriteBC01(LPCBYTE pbSendBuffer, DWORD cbSendLength, LPBYTE pbRecvBuffer, LPDWORD pcbRecvLength, bool *writeID);
    };
}
