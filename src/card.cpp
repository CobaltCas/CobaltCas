#include "project.h"
#include "default_card_image.h"
#ifndef _WIN32
#include <PCSC/winscard.h>
#endif

namespace Cas {

    Card::Card()
    {
        // Load card image (create default file if not present)
        loadCardImage(sys.initGroupID, sys.initGroupIDKm);

        // Check tier area
        Log::logout("    Checking check digits for each contract information section: ");
        bool err = false;
        for (uint8_t BroadcastGroupID = 0; BroadcastGroupID <= BGID_COUNT; BroadcastGroupID++) {
            TIER_t *p = pTIER(BroadcastGroupID);
            uint8_t checkDigit = Utils::calc_tier_check_digit(p);
            if (p->checkDigit != checkDigit) {
                if (!err) Log::logout("\n");
                Log::logout("\n");
                Log::logout("        Broadcast group ID: 0x%02X (%s)\n", BroadcastGroupID, Utils::BroadcastGroupID_to_name(BroadcastGroupID));
                Log::logout("            ");
                Log::logout_dump(p, sizeof(TIER_t), 3);
                Log::logout("        Check digit mismatch: Incorrect: 0x%02X / Correct: 0x%02X\n", p->checkDigit, checkDigit);
                err = true;
            }
        }

        if (!err) {
            Log::logout("OK\n");
        } else {
            Log::logout("\n");
        }

        // Check message area
        Log::logout("    Checking check digits for each message control section: ");
        err = false;
        for (uint8_t BroadcastGroupID = 0; BroadcastGroupID <= BGID_COUNT; BroadcastGroupID++) {
            MESSAGE_t *p = pMESSAGE(BroadcastGroupID);
            uint8_t checkDigit = Utils::calc_message_check_digit(p);
            if (p->checkDigit != checkDigit) {
                if (!err) Log::logout("\n");
                Log::logout("\n");
                Log::logout("        Broadcast group ID: 0x%02X (%s)\n", BroadcastGroupID, Utils::BroadcastGroupID_to_name(BroadcastGroupID));
                Log::logout("            ");
                Log::logout_dump(p, sizeof(MESSAGE_t), 3);
                Log::logout("        Check digit mismatch: Incorrect: 0x%02X / Correct: 0x%02X\n",p->checkDigit, checkDigit);
                err = true;
            }
        }

        if (!err) {
            Log::logout("OK\n");
        } else {
            Log::logout("\n");
        }

        // Log output of Card ID & Group ID
        Log::logout("\n");
        Cas::GROUP_ID_t *p = (Cas::GROUP_ID_t *)(pINFO()->ID);
        for (int i = 0; i <= 7; i++) {
            if (!i || (pINFO()->GroupID_Flag2 & (1 << i))) {
                if (i == 0) {
                    Log::logout("    Main card ID: 0x%012llX [%04X] (%s)", ld_be48(p->ID), ld_be16(p->ID_CheckDigit), Utils::cardID_to_string(ld_be48(p->ID)));
                    if (Utils::calc_cardID_check_digit(ld_be48(p->ID)) != ld_be16(p->ID_CheckDigit)) Log::logout("  * Check digit mismatch");
                    Log::logout("\n");
                } else {
                    Log::logout("\n");
                    Log::logout("        Group ID [%u]: 0x%012llX [%04X] (%s)", i, ld_be48(p->ID), ld_be16(p->ID_CheckDigit), Utils::cardID_to_string(ld_be48(p->ID)));
                    if (Utils::calc_cardID_check_digit(ld_be48(p->ID)) != ld_be16(p->ID_CheckDigit)) Log::logout("  * Check digit mismatch");
                    Log::logout("\n");
                }
            }
            p++;
        }
        Log::logout("\n");

        // Log output of work key information
        Log::logout("    Work key information:\n");
        long keyCount = 0;
        vector<KeyManager::Kw_t> keyList;
        for (uint8_t BroadcastGroupID = 0; BroadcastGroupID < BGID_COUNT; BroadcastGroupID++) {
            keyList.clear();

            // Register work key information in the card
            TIER_t *t = pTIER(BroadcastGroupID);
            for (uint8_t i = 0;i < 2; i++) {
                KeyManager::Kw_t kw;
                kw.WorkKeyID = t->Keys[i].WorkKeyID;
                kw.Key = ld_be64(t->Keys[i].Key);
                if (!kw.Key) continue;
                keyList.push_back(kw);
            }

            // Output work key information
            sys.keySets.getWorkKeyList(BroadcastGroupID, keyList);  // Create a list of registered work keys
            if (keyList.size() == 0) continue;

            Log::logout("        Broadcast group ID: 0x%02X (%s)\n", BroadcastGroupID, Utils::BroadcastGroupID_to_name(BroadcastGroupID));
            for (int i = 0; i < (int)keyList.size(); i++) {
                Log::logout("            Kw%02X = %02X %016llX\n", BroadcastGroupID, keyList[i].WorkKeyID, keyList[i].Key);
                keyCount++;
            }
            Log::logout("\n");
        }

        keyList.clear();
        if (!keyCount) {
            Log::logout("        * No work key information\n");
        }

        Log::logout(NULL);  // Flush the log file stream
    }

    Card::~Card()
    {
        ;
    }

    // Set the specified ID / Group ID / Km as initial values for the card image
    void Card::setupCardImage(uint64_t *initID, uint64_t *initKm)
    {
        Cas::INFO_t *info = pINFO();

        if (initID[0]) {  // Main Card ID
            st_be48(info->ID, initID[0]);
            st_be16(info->ID_CheckDigit, Utils::calc_cardID_check_digit(initID[0]));
        }
        if (initKm[0]) {  // Main Card Km
            st_be64(info->Km, initKm[0]);
            info->Km_CheckDigit = Utils::calc_Km_check_digit(initKm[0]);
        }

        // Group ID / Master Key (Km)
        uint8_t flg = 0x00;
        for (int i = 1; i <= 7; i++) {
            if (!initID[i] || !initKm[i]) continue;
            uint8_t gid = (uint8_t)(i - 1);

            flg |= (uint8_t)(1 << i);

            st_be48(info->grpID[gid].ID, initID[i]);
            st_be16(info->grpID[gid].ID_CheckDigit, Utils::calc_cardID_check_digit(initID[i]));

            st_be64(info->grpID[gid].Km, initKm[i]);
            info->grpID[gid].Km_CheckDigit = Utils::calc_Km_check_digit(initKm[i]);
        }

        if (flg) {
            info->GroupID_Flag1 = flg | 0x01;  // Group ID usage flag (Main ID part is always 1?)
            info->GroupID_Flag2 = flg;         // Group ID usage flag (Main ID part is always 0?)
        }
    }

    // Read card image file into buffer (7680 bytes)
    void Card::loadCardImage(uint64_t *initID, uint64_t *initKm)
    {
        // Load card image file (create default file in case of failure)
        if (!sys.clModeEnable) {
            bool resetFlag = false;
            if (!Utils::load_card_image(cardImage)) {
                resetFlag = true;
                memcpy(cardImage, DEFAULT_CARD_IMAGE, sizeof(cardImage));
                setupCardImage(initID, initKm);
                Utils::save_card_image(cardImage);
            }
            Log::logout("    %s\n", resetFlag ? "Default values have been applied to the card image file." : "Existing card image file loaded.");
            return;
        }

        // Apply default values unconditionally
        memcpy(cardImage, DEFAULT_CARD_IMAGE, sizeof(cardImage));
        setupCardImage(initID, initKm);
    }

    // Writes data in buffer to card image file (7680 bytes)
    void Card::saveCardImage(void)
    {
        if (sys.clModeEnable) return;

        uint8_t tmp[sizeof(cardImage)];
        bool update = false;
        if (Utils::load_card_image(tmp)) {
            update = (memcmp(tmp, cardImage, sizeof(tmp)) != 0);
        }

        if (update) {
            if (!Utils::save_card_image(cardImage)) {
                Log::logout("[Failed to update card image file]\n");
            } else {
                Log::logout("[Card image file updated]\n");
            }
        }
    }

    // Copy response data to create a return value for the API
    static LONG resCopy(LPBYTE pbRecvBuffer, LPDWORD pcbRecvLength, void *res, DWORD resSize)
    {
        LONG result = SCARD_E_INVALID_PARAMETER;
        if ((*pcbRecvLength == 0) || (*pcbRecvLength >= resSize)) {
            memcpy(pbRecvBuffer, res, resSize);
            result = SCARD_S_SUCCESS;
        }
        *pcbRecvLength = resSize;
        return result;
    }

    // Set 2 bytes of response data to create the API return value
    static LONG resError(LPBYTE pbRecvBuffer, LPDWORD pcbRecvLength, uint16_t returnCode)
    {
        uint8_t buf[2];
        st_be16(buf, returnCode);
        return resCopy(pbRecvBuffer, pcbRecvLength, buf, sizeof(buf));
    }

    // INT: Initial setting conditions command
    LONG Card::processCmd30(LPCBYTE pbSendBuffer, DWORD cbSendLength, LPBYTE pbRecvBuffer, LPDWORD pcbRecvLength)
    {
        typedef struct {
            uint8_t CLA;  // 0x90
            uint8_t INS;  // 0x30
            uint8_t P1;   // 0x00
            uint8_t P2;   // 0x00
            uint8_t Le;   // 0x00
        } CMD;

        typedef struct {
            uint8_t ProtocolNumber;
            uint8_t UnitLength;
            uint8_t ICCardInstruction[2];
            uint8_t ReturnCode[2];
            uint8_t ca_system_id[2];
            uint8_t card_id[6];
            uint8_t card_type;
            uint8_t split_size;
            uint8_t system_key[32];
            uint8_t cbc[8];
            uint8_t system_management_id_count;  // system_management_id count (1 fixed)
            uint8_t system_management_id0[2];    // first system_management_id
            uint8_t SW1;
            uint8_t SW2;
        } RES;

        static const uint8_t skey[] = {
            0x36, 0x31, 0x04, 0x66, 0x4B, 0x17, 0xEA, 0x5C, 0x32, 0xDF, 0x9C, 0xF5, 0xC4, 0xC3, 0x6C, 0x1B,
            0xEC, 0x99, 0x39, 0x21, 0x68, 0x9D, 0x4B, 0xB7, 0xB7, 0x4E, 0x40, 0x84, 0x0D, 0x2E, 0x7D, 0x98
        };

        static const uint8_t cbc[] = {
            0xFE, 0x27, 0x19, 0x99, 0x19, 0x69, 0x09, 0x11
        };

        uint16_t returnCode = 0x2100;
        uint16_t swCode = 0x9000;

        CMD *cmd = (CMD *)pbSendBuffer;

        uint8_t recvTemp[300] = { 0 };
        RES *res = (RES *)recvTemp;
        DWORD resSize = sizeof(RES);

        Log::logout("[INT command received]\n");

        if ((cbSendLength != sizeof(CMD)) || (cmd->Le != 0x00)) {
            Log::logout("    Command length abnormal\n");
            return resError(pbRecvBuffer, pcbRecvLength, 0x6700);
        }

        if (cmd->P1 || cmd->P2) {
            Log::logout("    P1 / P2 abnormal: [ P1:0x%02X / P2: 0x%02X ]\n", cmd->P1, cmd->P2);
            return resError(pbRecvBuffer, pcbRecvLength, 0x6A86);
        }

        uint8_t Le = *(pbSendBuffer + cbSendLength - 1);
        if (Le) {
            Log::logout("    Le abnormal: 0x%02X\n", Le);
            return resError(pbRecvBuffer, pcbRecvLength, 0x6700);
        }

        res->ProtocolNumber = 0;
        res->UnitLength = offsetof(RES, SW1) - offsetof(RES, UnitLength) - 1;
        st_be16(res->ICCardInstruction, getCardStatus());
        st_be16(res->ReturnCode, returnCode);
        memcpy(res->ca_system_id, pINFO()->ca_system_id, sizeof(res->ca_system_id));
        memcpy(res->card_id, pINFO()->ID, 6);
        res->card_type = pINFO()->card_type;  // 0:prepaid  1:common (default)
        res->split_size = 0x50;  // Message split length (0x50 fixed)
        memcpy(res->system_key, skey, sizeof(res->system_key));
        memcpy(res->cbc, cbc, sizeof(res->cbc));
        res->system_management_id_count = 1;  // system_management_id count (1 fixed)
        st_be16(res->system_management_id0, 0x0201);  // Broadcast/non-broadcast type: Broadcasting / Details type: 01
        st_be16(&res->SW1, swCode);

        Log::logout("    CA system ID                  : 0x%04X\n", ld_be16(res->ca_system_id));
        Log::logout("    Card ID                       : 0x%012llX (%s)\n", ld_be48(res->card_id), Utils::cardID_to_string(ld_be48(res->card_id)));
        Log::logout("    Card Type                     : 0x%02X\n", res->card_type);
        Log::logout("    Message split length          : 0x%02X (%u)\n", res->split_size, res->split_size);

        Log::logout("    Descrambler system key        : ");
        for (int i = 0;i < 32;i++) Log::logout((i && !(i % 8)) ? " %02X" : "%02X", res->system_key[i]);
        Log::logout("\n");

        Log::logout("    Descrambler CBC initial value : ");
        for (int i = 0;i < 8;i++) Log::logout((i && !(i % 8)) ? " %02X" : "%02X", res->cbc[i]);
        Log::logout("\n");

        Log::logout("    System management ID count    : 0x%02X\n", res->system_management_id_count);
        Log::logout("    Broadcast/non-broadcast type  : 0x%02X\n", res->system_management_id0[0] >> 4);
        Log::logout("    Broadcast standard type       : 0x%02X\n", res->system_management_id0[0] & 0x0f);
        Log::logout("    Details type                  : 0x%02X\n", res->system_management_id0[1]);

        Log::logout("    Return code                   : 0x%04X\n", returnCode);

        return resCopy(pbRecvBuffer, pcbRecvLength, recvTemp, resSize);
    }

    // IDI: Get card ID information command
    LONG Card::processCmd32(LPCBYTE pbSendBuffer, DWORD cbSendLength, LPBYTE pbRecvBuffer, LPDWORD pcbRecvLength)
    {
        typedef struct {
            uint8_t CLA;  // 0x90
            uint8_t INS;  // 0x32
            uint8_t P1;   // 0x00
            uint8_t P2;   // 0x00
            uint8_t Le;   // 0x00
        } CMD;

        typedef struct {
            uint8_t Maker;
            uint8_t Version;
            uint8_t CardID[6];
            uint8_t CheckDigit[2];
        } RES_ID;

        typedef struct {
            uint8_t ProtocolNumber;
            uint8_t UnitLength;
            uint8_t ICCardInstruction[2];
            uint8_t ReturnCode[2];
            uint8_t CardID_count;
            RES_ID ID;
            uint8_t SW1;
            uint8_t SW2;
        } RES;

        uint16_t returnCode = 0x2100;
        uint16_t swCode = 0x9000;

        CMD *cmd = (CMD *)pbSendBuffer;

        uint8_t recvTemp[300] = { 0 };
        RES *res = (RES *)recvTemp;
        DWORD resSize = sizeof(RES);

        Log::logout("[IDI command received]\n");

        if ((cbSendLength != sizeof(CMD)) || (cmd->Le != 0x00)) {
            Log::logout("    Command length abnormal\n");
            return resError(pbRecvBuffer, pcbRecvLength, 0x6700);
        }

        if (cmd->P1 || cmd->P2) {
            Log::logout("    P1 / P2 abnormal: [ P1:0x%02X / P2: 0x%02X ]\n", cmd->P1, cmd->P2);
            return resError(pbRecvBuffer, pcbRecvLength, 0x6A86);
        }

        uint8_t Le = *(pbSendBuffer + cbSendLength - 1);
        if (Le) {
            Log::logout("    Le abnormal: 0x%02X\n", Le);
            return resError(pbRecvBuffer, pcbRecvLength, 0x6700);
        }

        INFO_t *info = pINFO();

        res->ProtocolNumber = 0;
        res->CardID_count = 1;

        res->ID.Maker = 'T';  // Create Main ID information
        res->ID.Version = sys.cardVersion;
        memcpy(res->ID.CardID, info->ID, sizeof(res->ID.CardID));
        memcpy(res->ID.CheckDigit, info->ID_CheckDigit, sizeof(res->ID.CheckDigit));

        RES_ID *p = (RES_ID *)&res->SW1;  // Create group ID information
        for (int i = 1;i <= 7; i++) {
            if (!(info->GroupID_Flag2 & (uint8_t)(1 << i))) continue;
            p->Maker = 'T';
            p->Version = sys.cardVersion;
            memcpy(p->CardID, info->grpID[ i - 1 ].ID, sizeof(p->CardID));
            memcpy(p->CheckDigit, info->grpID[ i - 1 ].ID_CheckDigit, sizeof(p->CheckDigit));
            res->CardID_count++;
            p++;
        }

        res->UnitLength = (offsetof(RES, SW1) - offsetof(RES, UnitLength) - 1) + ((res->CardID_count - 1) * sizeof(RES_ID));
        st_be16(res->ICCardInstruction, getCardStatus());
        st_be16(res->ReturnCode, returnCode);
        st_be16((uint8_t *)p, swCode);
        resSize = sizeof(RES) + ((res->CardID_count - 1) * sizeof(RES_ID));

        Log::logout("    Card ID count  : %u\n", res->CardID_count);
        Log::logout("    Maker ID       : %c (0x%02X)\n", res->ID.Maker, res->ID.Maker);
        Log::logout("    Version number : 0x%02X\n", res->ID.Version);
        Log::logout("    Card ID        : 0x%012llX (%s)\n", ld_be48(res->ID.CardID), Utils::cardID_to_string(ld_be48(res->ID.CardID)));
        Log::logout("    Check digit    : 0x%04X (%u)\n", ld_be16(res->ID.CheckDigit), ld_be16(res->ID.CheckDigit));
        if (res->CardID_count > 1) {
            p = (RES_ID *)&res->SW1;
            Log::logout("\n");
            for (int i = 0;i < res->CardID_count - 1;i++) {
                Log::logout("    Group ID type [%c]\n", Utils::cardID_to_string(ld_be48(p->CardID))[0]);
                Log::logout("        Maker ID             : %c (0x%02X)\n", p->Maker, p->Maker);
                Log::logout("        Version number       : 0x%02X\n", p->Version);
                Log::logout("        Group ID             : 0x%012llX (%s)\n", ld_be48(p->CardID), Utils::cardID_to_string(ld_be48(p->CardID)));
                Log::logout("        Group ID check digit : 0x%04X (%u)\n", ld_be16(p->CheckDigit), ld_be16(p->CheckDigit));
                if (i != (res->CardID_count - 2)) Log::logout("\n");
                p++;
            }
        }
        Log::logout("    Return code    : 0x%04X\n", returnCode);

        return resCopy(pbRecvBuffer, pcbRecvLength, recvTemp, resSize);
    }

    //
    //
    //
    // ECM: Receive ECM command
    LONG Card::processCmd34(LPCBYTE pbSendBuffer, DWORD cbSendLength, LPBYTE pbRecvBuffer, LPDWORD pcbRecvLength)
    {
        typedef struct {
            uint8_t CLA;                    // 0x90
            uint8_t INS;                    // 0x34
            uint8_t P1;                     // 0x00
            uint8_t P2;                     // 0x00
            uint8_t Lc;                     // Command length
            struct {                        // Fixed part
                uint8_t ProtocolNumber;
                uint8_t BroadcastGroupID;
                uint8_t WorkKeyID;
                uint8_t OddKey[8];          // Start of encryption
                uint8_t EvenKey[8];
                uint8_t ProgramType;
                uint8_t Date[2];
                uint8_t Time[3];
                uint8_t RecordingControl;
            } FixedPart;
            uint8_t Le;                     // Response data length (0x00) / Usually variable length data is inserted between FixedPart and Le
        } CMD;

        typedef struct {
            uint8_t ProtocolNumber;
            uint8_t UnitLength;
            uint8_t ICCardInstruction[2];
            uint8_t ReturnCode[2];
            uint8_t OddKey[8];
            uint8_t EvenKey[8];
            uint8_t RecordingControl;  // 0x00: Recording not available / 0x01: Recording available / 0x10: Recording available only for purchaser
            uint8_t SW1;
            uint8_t SW2;
        } RES;

        uint16_t returnCode = 0xA1FE;
        uint16_t swCode = 0x9000;
        CMD *cmd = (CMD *)pbSendBuffer;

        uint8_t recvTemp[300] = { 0 };
        RES *res = (RES *)recvTemp;
        DWORD resSize = sizeof(RES);

        uint8_t bgID = 0xff;
        bool Contracted = false;

        TIER_t *pT = pTIER(BGID_TEMP);

        Log::logout("[ECM command received]\n");

        {
            if ((cbSendLength < sizeof(CMD)) || (cmd->Lc != (cbSendLength - 6)) || (cmd->Lc > ECM_DATA_MAX_LENGTH)) {
                Log::logout("    Command length abnormal\n");
                return resError(pbRecvBuffer, pcbRecvLength, 0x6700);
            }

            if (cmd->P1 || cmd->P2) {
                Log::logout("    P1 / P2 abnormal: [ P1:0x%02X / P2: 0x%02X ]\n", cmd->P1, cmd->P2);
                return resError(pbRecvBuffer, pcbRecvLength, 0x6A86);
            }

            uint8_t Le = *(pbSendBuffer + cbSendLength - 1);
            if (Le) {
                Log::logout("    Le abnormal: 0x%02X\n", Le);
                return resError(pbRecvBuffer, pcbRecvLength, 0x6700);
            }

            if (cmd->Lc < 0x1e) {  // Insufficient command data
                Log::logout("    Insufficient command data  Lc: 0x%02X(%u)\n", cmd->Lc, cmd->Lc);
                returnCode = 0x0A106;  // follow as the actual card
                bgID = 0xff;
                goto EXIT_FUNCTION;
            }

            bgID = cmd->FixedPart.BroadcastGroupID;

            bool invalidProtocolNumber = ((cmd->FixedPart.ProtocolNumber != 0x00) && (cmd->FixedPart.ProtocolNumber != 0x04) &&
                                            (cmd->FixedPart.ProtocolNumber != 0x40) && (cmd->FixedPart.ProtocolNumber != 0x44));
            bool invalidBroadcastGroupID = (bgID >= BGID_COUNT);

            Log::logout("    Protocol number              : 0x%02X%s\n", cmd->FixedPart.ProtocolNumber, invalidProtocolNumber?" * Non-operational protocol number":"");
            Log::logout("    Broadcast group ID           : 0x%02X (%s)\n", bgID, invalidBroadcastGroupID?" * Invalid Broadcast group ID":Utils::BroadcastGroupID_to_name(bgID));
            Log::logout("    Work key ID                  : 0x%02X\n", cmd->FixedPart.WorkKeyID);

            if (invalidProtocolNumber) {
                returnCode = 0xA102;  // Non-operational protocol number error
                bgID = 0xff;
                goto EXIT_FUNCTION;
            }

            if (invalidBroadcastGroupID) {
                returnCode = 0xA1FE;  // Other error
                bgID = 0xff;
                goto EXIT_FUNCTION;
            }

            // Copy the applicable tier information to the temporary area and rewrite the data there
            memcpy(pT, pTIER(bgID), sizeof(TIER_t));

            if (!pT->ActivationState) {
                Log::logout("    * No broadcast information\n");
                returnCode = 0xA103;
                bgID = 0xff;
                goto EXIT_FUNCTION;
            }

            uint64_t key = getWorkKey(bgID, cmd->FixedPart.WorkKeyID);
            if (!key) {
                Log::logout("    * No work key\n");
                returnCode = 0xA103;
                bgID = 0xff;
                goto EXIT_FUNCTION;
            }

            // Message decryption
            uint8_t tmp[300];
            memcpy(tmp, cmd, cbSendLength);

            uint32_t decodingStartPoint = offsetof(CMD, FixedPart.OddKey);
            uint32_t decodingLength = (cbSendLength - decodingStartPoint - 1);
            const uint8_t *in = (const uint8_t *)cmd + decodingStartPoint;
            uint8_t *out = &tmp[ decodingStartPoint ];
            Crypto::decrypt(out, in, decodingLength, key, cmd->FixedPart.ProtocolNumber);
            cmd = (CMD *)tmp;

            Log::logout("    Decryption key               : 0x%016llX\n", key);
            Log::logout("    Decrypted data               : [ ");
            Log::logout_dump(out, decodingLength, 0);
            Log::logout(" ]\n");

            // Falsification check
            uint16_t checkingStartPoint = offsetof(CMD, FixedPart.ProtocolNumber);
            uint16_t checkingLength = (uint16_t)(cbSendLength - checkingStartPoint - 5);
            in = &tmp[ checkingStartPoint ];
            uint32_t calcValue = Crypto::digest(cmd->FixedPart.ProtocolNumber, key, in, checkingLength);
            uint32_t macValue = ld_be32(out + decodingLength - 4);
            if (macValue != calcValue) {
                Log::logout("    ECM falsification error      : Calculated value (0x%08lX) ≠ Falsification detection (0x%08lX)\n", calcValue, macValue);
                returnCode = 0x0A106;  // ECM falsification error
                bgID = 0xff;
                goto EXIT_FUNCTION;
            }

            Log::logout("    Falsification detection code : 0x%08lX\n", macValue);
            Log::logout("    Odd scrambled key            : 0x%016llX\n", ld_be64(cmd->FixedPart.OddKey));
            Log::logout("    Even scrambled key           : 0x%016llX\n", ld_be64(cmd->FixedPart.EvenKey));
            Log::logout("    Judgment type                : 0x%02X\n", cmd->FixedPart.ProgramType);
            Log::logout("    Date                         : %s\n", Utils::mjd_to_string(ld_be16(cmd->FixedPart.Date)));
            Log::logout("    Time                         : %s\n", Utils::time_to_string(cmd->FixedPart.Time));
            Log::logout("    Recording control            : 0x%02X\n", cmd->FixedPart.RecordingControl);

            // Variable length data part processing
            uint8_t *p = &tmp[ offsetof(CMD,Le) ];
            long remain = cmd->Lc - sizeof(cmd->FixedPart) - 4;

            returnCode = (cmd->FixedPart.ProgramType == 4) ? 0xA102 : ((cmd->FixedPart.ProgramType & 0x01) ? 0x0800 : 0xA1FE); // Match the actual card
            while (remain > 0) {
                Log::logout("\n        Function number : 0x%02X ", *p);
                switch (*p) {
                    case 0x21:  // Multi function
                        processNano21(pT, pMESSAGE(bgID), p, INS_ECM, true);
                        break;

                    case 0x23:  // InvalidateTier
                        processNano23(pT, p);
                        break;

                    case 0x51:  // ActivateTrial
                        processNano51(ld_be16(cmd->FixedPart.Date), pT, p);
                        break;

                    case 0x52:  // CheckContract bitmap
                        Contracted = processNano52(pT, p);
                        if (cmd->FixedPart.ProgramType == 0x02) {
                            Log::logout("            Contract status          : ");
                            if (!checkExpiryDate(pT, ld_be16(cmd->FixedPart.Date), cmd->FixedPart.Time[0]) && (pT->ActivationState != 2)) {
                                Log::logout("Expired (%s %02u)\n", Utils::mjd_to_string(ld_be16(pT->ExpiryDate)), cmd->FixedPart.Time[0]);
                                returnCode = 0x8902;
                            } else if (Contracted) {
                                Log::logout("Purchased\n");
                                returnCode = 0x0800;
                            } else {
                                Log::logout("No contract\n");
                                returnCode = 0x8901;
                            }
                        } else {
                            Log::logout("            * The contract confirmation result is invalid because the judgment type is not 0x02\n");
                        }
                        break;

                    default:
                        Log::logout("<Unknown/unsupported functions>\n");
                        Log::logout("\n");
                        break;
                }

                remain -= (p[1] + 2);
                p += (p[1] + 2);
            }

            if (remain != 0) {  // Variable length parameter length error?
                Log::logout("    * Detect errors in variable length data\n");
                returnCode = 0x0A106;  // ECM falsification error (follow as the actual card)
                bgID = 0xff;
                goto EXIT_FUNCTION;
            }
        }

    EXIT_FUNCTION:
        Log::logout("\n    Return code                  : 0x%04X\n", returnCode);

        // If it ends normally, copy the information from the temporary area to the corresponding tier
        if (bgID < BGID_COUNT) {
            pT->checkDigit = Utils::calc_tier_check_digit(pT);
            pMESSAGE(bgID)->checkDigit = Utils::calc_message_check_digit(pMESSAGE(bgID));
            memcpy(pTIER(bgID), pT, sizeof(TIER_t));
        }

        res->ProtocolNumber = 0;
        res->UnitLength = offsetof(RES, SW1) - offsetof(RES, UnitLength) - 1;
        st_be16(res->ICCardInstruction, getCardStatus());
        st_be16(&res->ReturnCode, returnCode);
        if (returnCode == 0x0800) {
            memcpy(res->EvenKey, cmd->FixedPart.EvenKey, 8);
            memcpy(res->OddKey, cmd->FixedPart.OddKey, 8);
            res->RecordingControl = cmd->FixedPart.RecordingControl;
        }
        st_be16(&res->SW1, swCode);

        return resCopy(pbRecvBuffer, pcbRecvLength, recvTemp, resSize);
    }

    //
    //
    //
    // EMM: Receive EMM command
    LONG Card::processCmd36(LPCBYTE pbSendBuffer, DWORD cbSendLength, LPBYTE pbRecvBuffer, LPDWORD pcbRecvLength)
    {
        typedef struct {
            uint8_t CLA;                    // 0x90
            uint8_t INS;                    // 0x36
            uint8_t P1;                     // 0x00
            uint8_t P2;                     // 0x00
            uint8_t Lc;                     // Command length
            struct {                        // Fixed part
                uint8_t CardID[6];          // Falsification check start position
                uint8_t Length;
                uint8_t ProtocolNumber;
                uint8_t BroadcastGroupID;   // Start of encryption
                uint8_t UpdateNumber[2];
                uint8_t ExpiryDate[2];
            } FixedPart;
            uint8_t Le;                     // Response data length (0x00) / Usually variable length data is inserted between FixedPart and Le
        } CMD;

        typedef struct {
            uint8_t ProtocolNumber;
            uint8_t UnitLength;
            uint8_t ICCardInstruction[2];
            uint8_t ReturnCode[2];
            uint8_t SW1;
            uint8_t SW2;
        } RES;

        uint16_t returnCode = 0x2100;
        uint16_t swCode = 0x9000;
        CMD *cmd = (CMD *)pbSendBuffer;

        uint8_t recvTemp[300] = { 0 };
        RES *res = (RES *)recvTemp;
        DWORD resSize = sizeof(RES);

        uint8_t bgID = 0xff;

        TIER_t *pT = pTIER(BGID_TEMP);

        Log::logout("[EMM command received]\n");

        {
            if ((cbSendLength < sizeof(CMD)) || (cmd->Lc != (cbSendLength - 6)) || (cmd->Lc > EMM_DATA_MAX_LENGTH)) {
                Log::logout("    Command length abnormal\n");
                return resError(pbRecvBuffer, pcbRecvLength, 0x6700);
            }

            if (cmd->P1 || cmd->P2) {
                Log::logout("    P1 / P2 abnormal: [ P1:0x%02X / P2: 0x%02X ]\n", cmd->P1, cmd->P2);
                return resError(pbRecvBuffer, pcbRecvLength, 0x6A86);
            }

            uint8_t Le = *(pbSendBuffer + cbSendLength - 1);
            if (Le) {
                Log::logout("    Le abnormal: 0x%02X\n", Le);
                return resError(pbRecvBuffer, pcbRecvLength, 0x6700);
            }

            uint64_t CardID = ld_be48(cmd->FixedPart.CardID);
            Cas::GROUP_ID_t *pID;
            int grpID = getID(CardID, &pID);

            bool invalidProtocolNumber = ((cmd->FixedPart.ProtocolNumber != 0x00) && (cmd->FixedPart.ProtocolNumber != 0x04) &&
                                            (cmd->FixedPart.ProtocolNumber != 0x40) && (cmd->FixedPart.ProtocolNumber != 0x44));
            bool invalidCardID = (grpID < 0);

            Log::logout("    Data length                     : 0x%02X (%u)\n", cmd->FixedPart.Length, cmd->FixedPart.Length);

            Log::logout("    Destination card ID             : 0x%012llX (%s)\n", CardID, Utils::cardID_to_string(CardID));
            if (invalidCardID) {
                Log::logout("                                      * Not an EMM addressed to this card [Main card ID: 0x%012llX %s]\n", CardID, Utils::cardID_to_string(CardID));
            } else if (grpID > 0) {
                Log::logout("                                      * EMM addressed to group ID\n");
            }
            Log::logout("    Protocol number                 : 0x%02X%s\n", cmd->FixedPart.ProtocolNumber, invalidProtocolNumber?" * Non-operational protocol number":"");

            if (invalidProtocolNumber) {
                returnCode = 0xA102;  // Non-operational protocol number error
                bgID = 0xff;
                goto EXIT_FUNCTION;
            }

            if (invalidCardID) {
                returnCode = 0xA1FE;  // Other error
                bgID = 0xff;
                goto EXIT_FUNCTION;
            }

            // Message decryption
            uint8_t tmp[300];
            memcpy(tmp, cmd, cbSendLength);

            uint32_t decodingStartPoint = offsetof(CMD, FixedPart.BroadcastGroupID);
            uint32_t decodingLength = (cbSendLength - decodingStartPoint - 1);
            const uint8_t *in = (const uint8_t *)cmd + decodingStartPoint;
            uint8_t *out = &tmp[ decodingStartPoint ];
            Crypto::decrypt(out, in, decodingLength, ld_be64(pID->Km), cmd->FixedPart.ProtocolNumber);
            cmd = (CMD *)tmp;

            Log::logout("    Decryption key                  : 0x%016llX\n", ld_be64(pID->Km));
            Log::logout("    Decrypted data                  : [ ");
            Log::logout_dump(out, decodingLength, 0);
            Log::logout(" ]\n");

            // Falsification check
            uint16_t checkingStartPoint = offsetof(CMD, FixedPart.CardID);
            uint16_t checkingLength = (uint16_t)(cbSendLength - checkingStartPoint - 5);
            in = &tmp[ checkingStartPoint ];
            uint32_t calcValue = Crypto::digest(cmd->FixedPart.ProtocolNumber, ld_be64(pID->Km), in, checkingLength);
            uint32_t macValue = ld_be32(out + decodingLength - 4);
            if (macValue != calcValue) {
                Log::logout("    EMM falsification error         : Calculated value (0x%08lX) ≠ Falsification detection (0x%08lX)\n", calcValue, macValue);
                returnCode = 0x0A107;  // EMM falsification error
                bgID = 0xff;
                goto EXIT_FUNCTION;
            }

            bgID = cmd->FixedPart.BroadcastGroupID;
            bool invalidBroadcastGroupID = (bgID >= BGID_COUNT);

            Log::logout("    Falsification detection code    : 0x%08lX\n", macValue);
            Log::logout("    Broadcast group ID              : 0x%02X (%s)\n", bgID, invalidBroadcastGroupID?" * Invalid Broadcast group ID":Utils::BroadcastGroupID_to_name(bgID));
            Log::logout("    Update number                   : 0x%04X (%u)\n", ld_be16(cmd->FixedPart.UpdateNumber), ld_be16(cmd->FixedPart.UpdateNumber));
            Log::logout("    Expiry date                     : %s\n", Utils::mjd_to_string(ld_be16(cmd->FixedPart.ExpiryDate)));

            if (invalidBroadcastGroupID) {
                returnCode = 0xA1FE;  // Other error
                bgID = 0xff;
                goto EXIT_FUNCTION;
            }

            // Copy the applicable tier information to the temporary area and rewrite the data there
            memcpy(pT, pTIER(bgID), sizeof(TIER_t));

            // Update number check
            uint16_t currentUpdateNumber = ld_be16(pT->UpdateNumber1);
            uint16_t updateNumber = ld_be16(cmd->FixedPart.UpdateNumber);
            bool updateEnable = (updateNumber == 0xc000) || ((updateNumber > 0xc000) && (updateNumber > currentUpdateNumber));

            if (updateEnable) {

                if (updateNumber != 0xc000) {
                    Log::logout("    Execute update number change    : 0x%04X -> 0x%04X\n", currentUpdateNumber, updateNumber);
                    st_be16(pT->UpdateNumber1, updateNumber);  // Update number change
                }

                if (pT->ActivationState != 2) {
                    Log::logout("    Execute activation state change : 0x%02X -> 0x%02X\n", pT->ActivationState, 0x02);
                    pT->ActivationState = 2;  // Active state
                }

                uint16_t currentExpiryDate = ld_be16(pT->ExpiryDate);
                uint16_t ExpiryDate = ld_be16(cmd->FixedPart.ExpiryDate);
                st_be16(pT->ExpiryDate, ExpiryDate);
                if (ExpiryDate) pT->ExpiryHour = 0x23;
                if (currentExpiryDate != ExpiryDate) {
                    Log::logout("    Execute expiry date update      : ");
                    Log::logout("%s", Utils::mjd_to_string(currentExpiryDate));
                    Log::logout(" -> ");
                    Log::logout("%s (ExpiryHour: 0x%02X)", Utils::mjd_to_string(ExpiryDate), pT->ExpiryHour);
                    Log::logout("\n");
                }

            } else {
                Log::logout("    * Invalid update number (Current update number 0x%04X) \n", currentUpdateNumber);
            }

            // Variable length data part processing
            uint8_t *p = &tmp[ offsetof(CMD,Le) ];
            long remain = cmd->FixedPart.Length - 10;
            while (remain > 0) {
                Log::logout("\n        Function number : 0x%02X ", *p);
                switch (*p) {
                    case 0x10:  // Update tier key
                        processNano10(pT, bgID, p, updateEnable);
                        break;

                    case 0x11:  // Update tier contract bitmap
                        processNano11(pT, p, updateEnable);
                        break;

                    case 0x13:  // Add group ID
                        processNano13(p, updateEnable);
                        break;

                    case 0x14:  // Remove group ID
                        processNano14(p, updateEnable);
                        break;

                    case 0x20:  // Power on contorl
                        processNano20(pT, p, updateEnable);
                        break;

                    case 0x21:  // Multi function
                        processNano21(pT, pMESSAGE(bgID), p, INS_EMM, updateEnable);
                        break;

                    default:
                        Log::logout("<Unsupported functions>\n");
                        Log::logout("\n");
                        break;
                }

                remain -= (p[1] + 2);
                p += (p[1] + 2);
            }

            if (remain != 0) {  // Variable length parameter length error?
                Log::logout("    * Detect errors in variable length data\n");
                returnCode = 0x0A107;  // EMM falsification error (follow as the actual card)
                bgID = 0xff;
                goto EXIT_FUNCTION;
            }
        }

    EXIT_FUNCTION:
        Log::logout("    Return code                     : 0x%04X\n", returnCode);

        // If it ends normally, copy the information from the temporary area to the corresponding tier
        if (bgID < BGID_COUNT) {
            pT->checkDigit = Utils::calc_tier_check_digit(pT);
            pMESSAGE(bgID)->checkDigit = Utils::calc_message_check_digit(pMESSAGE(bgID));
            memcpy(pTIER(bgID), pT, sizeof(TIER_t));
        }

        res->ProtocolNumber = 0;  // 0 fixed
        res->UnitLength = offsetof(RES, SW1) - offsetof(RES, UnitLength) - 1;
        st_be16(res->ICCardInstruction, getCardStatus());
        st_be16(&res->ReturnCode, returnCode);
        st_be16(&res->SW1, swCode);

        return resCopy(pbRecvBuffer, pcbRecvLength, recvTemp, resSize);
    }

    //
    //
    //
    // EMG: Receive individual EMM message command
    LONG Card::processCmd38(LPCBYTE pbSendBuffer, DWORD cbSendLength, LPBYTE pbRecvBuffer, LPDWORD pcbRecvLength)
    {
        typedef struct {
            uint8_t CLA;                    // 0x90
            uint8_t INS;                    // 0x38
            uint8_t P1;                     // 0x00
            uint8_t P2;                     // 0x00
            uint8_t Lc;                     // Command length
            struct {                        // Fixed part
                uint8_t CardID[6];          // Falsification data check start position (Ver3~)
                uint8_t ProtocolNumber;
                uint8_t BroadcastGroupID;
                uint8_t MessageControl;
                uint8_t AlternationDetector[2]; // Start of encryption / Falsification data check start position (Ver2)
                uint8_t LimitDate[2];
                uint8_t FixedMessageID[2];
                uint8_t ExtraMessageFormatVersion;
                uint8_t ExtraMessageLength[2];  // followed by the byte length indicated here  MAX: 0x14(20)
            } FixedPart;
            uint8_t Le;                     // Response data length (0x00) / Usually ExtraMessageLength data is inserted between FixedPart and Le
        } CMD;

        typedef struct {
            uint8_t ProtocolNumber;
            uint8_t UnitLength;
            uint8_t ICCardInstruction[2];
            uint8_t ReturnCode[2];
            uint8_t unknown1[2];
            uint8_t expiry_date[2];
            uint8_t fixed_phrase_message_number[2];
            uint8_t diff_format_number;
            uint8_t ExtraMessageLength[2];
            uint8_t unknown2[2];
            uint8_t unknown3[2];
            uint8_t SW1;
            uint8_t SW2;
        } RES;

        uint16_t returnCode = 0x2100;
        uint16_t swCode = 0x9000;

        uint8_t sendTemp[300] = { 0 };
        uint8_t recvTemp[300] = { 0 };
        uint16_t sendLen = ((uint16_t)cbSendLength > sizeof(sendTemp)) ? sizeof(sendTemp) : ((uint16_t)cbSendLength);
        memcpy(sendTemp, pbSendBuffer, sendLen);
        CMD *cmd = (CMD *)sendTemp;
        RES *res = (RES *)recvTemp;
        DWORD resSize = sizeof(RES);

        uint8_t bgID = 0xff;

        MESSAGE_t *pMSG = pMESSAGE(BGID_TEMP);
        uint16_t len;
        int msgLen;

        Log::logout("[EMG command received]\n");

        {
            if ((sendLen < 15) || (cmd->Lc != (sendLen - 6)) || (cmd->Lc > EMG_DATA_MAX_LENGTH)) {
                Log::logout("    Command length abnormal\n");
                return resError(pbRecvBuffer, pcbRecvLength, 0x6700);
            }

            if (cmd->P1 || cmd->P2) {
                Log::logout("    P1 / P2 abnormal: [ P1:0x%02X / P2: 0x%02X ]\n", cmd->P1, cmd->P2);
                return resError(pbRecvBuffer, pcbRecvLength, 0x6A86);
            }

            uint8_t Le = sendTemp[ sendLen - 1 ];
            if (Le) {
                Log::logout("    Le abnormal: 0x%02X\n", Le);
                return resError(pbRecvBuffer, pcbRecvLength, 0x6700);
            }

            bgID = cmd->FixedPart.BroadcastGroupID;

            uint64_t CardID = ld_be48(cmd->FixedPart.CardID);
            Cas::GROUP_ID_t *pID;
            int grpID = getID(CardID, &pID);

            bool invalidProtocolNumber = ((cmd->FixedPart.ProtocolNumber != 0x00) && (cmd->FixedPart.ProtocolNumber != 0x04) &&
                                            (cmd->FixedPart.ProtocolNumber != 0x40) && (cmd->FixedPart.ProtocolNumber != 0x44) &&
                                            (cmd->FixedPart.ProtocolNumber != 0x83));
            bool invalidCardID = (grpID < 0);
            bool invalidBroadcastGroupID = (bgID >= BGID_COUNT);
            bool invalidMessageControl = ((cmd->FixedPart.MessageControl != 1) && (cmd->FixedPart.MessageControl != 2));

            // What is this?
            // > 90 38 00 00 09 00 07 79 f4 e9 e4 83 12 02 00  Minimum length command (up to message control)
            // < 00 04 00 00 21 00 90 00
            // 0x2100 is returned up to 95 bytes, and an unknown data string is returned according to the number of argument data. message control area is not rewritten
            // 00 04 00 00 a1 fe 90 00 is returned for 96 bytes or more
            if (cmd->FixedPart.ProtocolNumber == 0x83) {
                if (cmd->FixedPart.MessageControl != 2) {
                    invalidProtocolNumber = true;
                }
            } else if (!invalidProtocolNumber) {
                if (cmd->FixedPart.MessageControl != 1) {
                    invalidProtocolNumber = true;
                }
            }

            Log::logout("    Destination card ID                 : 0x%012llX (%s)\n", CardID, Utils::cardID_to_string(CardID));
            if (invalidCardID) {
                Log::logout("                                          * Not an EMG addressed to this card [Main card ID: 0x%012llX %s]\n", CardID, Utils::cardID_to_string(CardID));
            } else if (grpID > 0) {
                Log::logout("                                          * EMG addressed to group ID\n");
            }
            Log::logout("    Protocol number                     : 0x%02X%s\n", cmd->FixedPart.ProtocolNumber, invalidProtocolNumber?" * Non-operational protocol number":"");
            Log::logout("    Broadcast group ID                  : 0x%02X (%s)\n", bgID, invalidBroadcastGroupID?" * Invalid Broadcast group ID":Utils::BroadcastGroupID_to_name(bgID));
            Log::logout("    Message control                     : 0x%02X%s\n", cmd->FixedPart.MessageControl, invalidMessageControl?" * Invalid message control":"");

            if (invalidCardID) {
                returnCode = 0xA1FE;  // Other error
                bgID = 0xff;
                goto EXIT_FUNCTION;
            }

            if (invalidBroadcastGroupID) {
                returnCode = 0xA1FE;  // Other error
                bgID = 0xff;
                goto EXIT_FUNCTION;
            }

            if (invalidMessageControl) {
                returnCode = 0xA1FE;  // Other error
                bgID = 0xff;
                goto EXIT_FUNCTION;
            }

            if (invalidProtocolNumber) {
                returnCode = 0xA102;  // Non-operational protocol number error
                bgID = 0xff;
                goto EXIT_FUNCTION;
            }

            // Unfamiliar process???
            if ((cmd->FixedPart.ProtocolNumber == 0x83) && (cmd->FixedPart.MessageControl == 2)) {
                DWORD dataLen = cbSendLength - 15;

                if (dataLen) {
                    Log::logout("    * Redundant parameter  %3d bytes: ", dataLen);
                    Log::logout_dump(pbSendBuffer + 14, (uint16_t) dataLen, 2);
                }

                res->ProtocolNumber = 0;
                st_be16(res->ICCardInstruction, getCardStatus());
                res->UnitLength = 4;
                uint8_t *p = &res->unknown1[0];
                resSize = 8;
                if (cbSendLength <= 95) {
                    st_be16(&res->ReturnCode, 0x2100);  // Successful completion
                    p += dataLen;
                    resSize += dataLen;
                    res->UnitLength += (uint8_t)dataLen;
                } else {
                    st_be16(&res->ReturnCode, 0xa1fe);  // Other error
                }
                st_be16(p, swCode);
                Log::logout("    * Unknown control process\n");
                return resCopy(pbRecvBuffer, pcbRecvLength, recvTemp, resSize);
            }

            // Copy the relevant message control information to the temporary area and rewrite the data there
            memcpy(pMSG, pMESSAGE(bgID), sizeof(MESSAGE_t));

            // Message decryption
            uint8_t tmp[300];
            uint16_t decodingStartPoint = offsetof(CMD, FixedPart.AlternationDetector);
            uint16_t decodingLength = (uint16_t)(sendLen - decodingStartPoint - 1);
            uint8_t *in = &sendTemp[ decodingStartPoint ];
            Crypto::decrypt(tmp, in, decodingLength, ld_be64(pID->Km), cmd->FixedPart.ProtocolNumber);
            memcpy(in, tmp, decodingLength - 4);
            memset(&in[decodingLength - 4], 0x00, 4);
            uint32_t macValue = ld_be32(&tmp[ decodingLength - 4 ]);

            Log::logout("    Decryption key                      : 0x%016llX\n", ld_be64(pID->Km));
            Log::logout("    Decrypted data                      : [ ");
            Log::logout_dump(tmp, decodingLength, 0);
            Log::logout(" ]\n");
            Log::logout("    Falsification detection code (T%03u) : 0x%08lX\n", sys.cardVersion, macValue);

            // Falsification check
            uint32_t calcValue = 0;

            if (sys.cardVersion < 3) {
                calcValue = Crypto::digest(cmd->FixedPart.ProtocolNumber, ld_be64(pID->Km), &cmd->FixedPart.AlternationDetector[0], decodingLength - 4);
            } else {
                calcValue = Crypto::digest(cmd->FixedPart.ProtocolNumber, ld_be64(pID->Km), &cmd->FixedPart.CardID[0], decodingLength + 5);
            }

            if (macValue != calcValue) {
                Log::logout("    EMG falsification error             : Calculated value (0x%08lX) ≠ Falsification detection (0x%08lX)\n", calcValue, macValue);
                returnCode = 0x0A105;  // EMG falsification error
                bgID = 0xff;
                goto EXIT_FUNCTION;
            }

            Log::logout("    Update number                       : 0x%04X\n", ld_be16(cmd->FixedPart.AlternationDetector));
            Log::logout("    Expiry date                         : %s\n", Utils::mjd_to_string(ld_be16(cmd->FixedPart.LimitDate)));
            Log::logout("    Message template number             : 0x%04X\n", ld_be16(cmd->FixedPart.FixedMessageID));
            Log::logout("    Differential format number          : 0x%02X\n", cmd->FixedPart.ExtraMessageFormatVersion);
            Log::logout("    Difference information length       : 0x%04X (%u)\n", ld_be16(cmd->FixedPart.ExtraMessageLength), ld_be16(cmd->FixedPart.ExtraMessageLength));

            len = ld_be16(cmd->FixedPart.ExtraMessageLength);
            if (len > 20) len = 20;
            msgLen = (int)cmd->Lc - 0x16;
            if (msgLen < 0) msgLen = 0;
            if ((uint16_t)msgLen > len) msgLen = len;
            if (msgLen > 0) {
                Log::logout("    Difference information        : ");
                Log::logout_dump(&cmd->Le, msgLen, 8);
            }

            long dlen = ((long)cbSendLength - offsetof(CMD, Le) - 5) - len;
            if (dlen > 0) {
                Log::logout("    * Redundant difference information  %3d bytes: ", dlen);
                Log::logout_dump(&cmd->Le + len, (uint16_t)dlen, 0);
            } else if (dlen < 0) {
                Log::logout("    * Insufficient difference information (compensated with 0x00): %d bytes\n", -dlen);
            }

            // Update number check
            uint16_t updateNumber = ld_be16(cmd->FixedPart.AlternationDetector);
            if (updateNumber < 0xc000) {  // Error if less than 0xc000
                Log::logout("    * Update number less than 0xC000\n");
                returnCode = 0x0A1FE;  // Other error
                bgID = 0xff;
                goto EXIT_FUNCTION;

            } else if (updateNumber != 0xc000) {  // If 0xc000, process unconditionally
                uint16_t currentUpdateNumber = ld_be16(pMSG->UpdateNumber);
                updateNumber &= 0x3fff;  // Effective bit length 14bit
                if (updateNumber <= currentUpdateNumber) {  // Error if update number is invalid
                    Log::logout("    * Invalid update number (Current update number 0x%04X) \n", currentUpdateNumber);
                    returnCode = 0x0A1FE;  // Other error
                    bgID = 0xff;
                    goto EXIT_FUNCTION;
                }
                Log::logout("    Execute update number change         : 0x%04X -> 0x%04X\n", currentUpdateNumber, updateNumber);
                st_be16(pMSG->UpdateNumber, updateNumber);
            }

            // Data processing
            uint16_t msgID = ld_be16(cmd->FixedPart.FixedMessageID);
            if (msgID || len) {  // Active if either message ID or message length is valid
                memcpy(pMSG->expiry_date, cmd->FixedPart.LimitDate, sizeof(cmd->FixedPart.LimitDate));
                pMSG->status = 3;
            }
            st_be16(pMSG->fixed_phrase_message_number, msgID);

            pMSG->diff_format_number = cmd->FixedPart.ExtraMessageFormatVersion;

            st_be16(pMSG->diff_information_length, len);

            memcpy(pMSG->diff_information, (const void *)&cmd->Le, msgLen);
            uint16_t zeroLen = len - (uint16_t)msgLen;
            if (zeroLen > 8) zeroLen = 8;
            memset(&pMSG->diff_information[msgLen], 0x00, zeroLen);  // Matching the way garbage data is left to the actual card

            if (!msgID && !len) {  // If the message ID and message length are invalid, no message is assumed
                memset(pMSG->expiry_date, 0x00, sizeof(cmd->FixedPart.LimitDate));
                pMSG->status = 0;
            }
        }

    EXIT_FUNCTION:
        Log::logout("    Return code                         : 0x%04X\n", returnCode);

        // If it ends normally, copy the information from the temporary area to the corresponding message control area.
        if (bgID < BGID_COUNT) {
            pMSG->checkDigit = Utils::calc_message_check_digit(pMSG);
            memcpy(pMESSAGE(bgID), pMSG, sizeof(MESSAGE_t));
        }

        res->ProtocolNumber = 0;  // 0 fixed
        st_be16(res->ICCardInstruction, getCardStatus());
        st_be16(&res->ReturnCode, returnCode);

        if (returnCode == 0x2100) {
            pMSG->checkDigit = Utils::calc_message_check_digit(pMSG);
            memcpy(pMESSAGE(bgID), pMSG, sizeof(MESSAGE_t));

            len = cmd->Lc - 22;
            if (len >= 20) len = 20;

            res->UnitLength = (offsetof(RES, SW1) - offsetof(RES, UnitLength) - 1) + (uint8_t)len;
            memcpy(res->expiry_date, pMSG->expiry_date, sizeof(res->expiry_date));
            memcpy(res->fixed_phrase_message_number, pMSG->fixed_phrase_message_number, sizeof(res->fixed_phrase_message_number));
            res->diff_format_number = pMSG->diff_format_number;
            memcpy(res->ExtraMessageLength, cmd->FixedPart.ExtraMessageLength, sizeof(cmd->FixedPart.ExtraMessageLength));
            memset(res->unknown1, 0x00, sizeof(res->unknown1));

            uint8_t *p = &res->ExtraMessageLength[2];
            if (len > 0) {
                memset(p, 0x00, len);
                memcpy(p, &cmd->Le, msgLen);
                p += len;
                resSize += (uint16_t)len;
            }

            memset(&p[0], 0x00, sizeof(res->unknown2));  // unknown2
            memset(&p[2], 0x00, sizeof(res->unknown3));  // unknown3
            st_be16(&p[4], swCode);                      // SW1/2
            p[5] = 0x00;                                 // Le

        } else {
            res->UnitLength = 4;
            st_be16(&res->ReturnCode[2], swCode);
            resSize = 8;
        }

        return resCopy(pbRecvBuffer, pcbRecvLength, recvTemp, resSize);
    }

    //
    //
    //
    // EMD: Get automatic display message display information command
    LONG Card::processCmd3A(LPCBYTE pbSendBuffer, DWORD cbSendLength, LPBYTE pbRecvBuffer, LPDWORD pcbRecvLength)
    {
        typedef struct {
            uint8_t CLA;                    // 0x90
            uint8_t INS;                    // 0x3A
            uint8_t P1;                     // 0x00
            uint8_t P2;                     // 0x00
            uint8_t Lc;                     // Command length
            struct {                        // Fixed part
                uint8_t Date[2];            // Current date
                uint8_t BroadcastGroupID;   // Broadcast group ID
                uint8_t PeriodOfTime;       // Period of time
            } FixedPart;
            uint8_t Le;                     // Response data length (0x00)
        } CMD;

        typedef struct {
            uint8_t ProtocolNumber;
            uint8_t UnitLength;
            uint8_t ICCardInstruction[2];
            uint8_t ReturnCode[2];
            uint8_t LimitDate[2];
            uint8_t FixedMessageID[2];
            uint8_t ExtraMessageFormatVersion;
            uint8_t ExtraMessageLength[2];  // It seems that the byte length shown here continues behind (there may be a Command_length constraint)
            uint8_t SW1;                    // Data for the length indicated by ExtraMessageLength is interrupted
            uint8_t SW2;
        } RES;

        uint16_t returnCode = 0x2100;
        uint16_t swCode = 0x9000;

        CMD *cmd = (CMD *)pbSendBuffer;

        uint8_t recvTemp[300] = { 0 };
        RES *res = (RES *)recvTemp;
        DWORD resSize = sizeof(RES);

        uint8_t bgID = 0xff;

        MESSAGE_t *pMSG = pMESSAGE(BGID_TEMP);

        Log::logout("[EMD command received]\n");

        {
            if ((cbSendLength != sizeof(CMD)) || (cmd->Lc != (cbSendLength - 6)) || (cmd->Lc > EMD_DATA_MAX_LENGTH)) {
                Log::logout("    Command length abnormal\n");
                return resError(pbRecvBuffer, pcbRecvLength, 0x6700);
            }

            if (cmd->P1 || cmd->P2) {
                Log::logout("    P1 / P2 abnormal: [ P1:0x%02X / P2: 0x%02X ]\n", cmd->P1, cmd->P2);
                return resError(pbRecvBuffer, pcbRecvLength, 0x6A86);
            }

            uint8_t Le = *(pbSendBuffer + cbSendLength - 1);
            if (Le) {
                Log::logout("    Le abnormal: 0x%02X\n", Le);
                return resError(pbRecvBuffer, pcbRecvLength, 0x6700);
            }

            uint16_t Date = ld_be16(cmd->FixedPart.Date);
            uint8_t PeriodOfTime = cmd->FixedPart.PeriodOfTime;

            bgID = cmd->FixedPart.BroadcastGroupID;
            bool invalidBroadcastGroupID = (bgID >= BGID_COUNT);

            Log::logout("    Date               : %s\n", Utils::mjd_to_string(Date));
            Log::logout("    Broadcast group ID : 0x%02X (%s)\n", bgID, invalidBroadcastGroupID?" * Invalid Broadcast group ID":Utils::BroadcastGroupID_to_name(bgID));
            Log::logout("    Period of time     : 0x%02X (%u days)\n", PeriodOfTime, PeriodOfTime);

            if (invalidBroadcastGroupID) {
                returnCode = 0xA1FE;  // Other error
                bgID = 0xff;
                goto EXIT_FUNCTION;
            }

            // Copy the relevant message control information to the temporary area and rewrite the data there
            memcpy(pMSG, pMESSAGE(bgID), sizeof(MESSAGE_t));

            Log::logout("    Information on the card\n");
            Log::logout("        Status       : 0x%02X\n", pMSG->status);
            Log::logout("        State date   : %s\n", Utils::mjd_to_string(ld_be16(pMSG->start_date)));
            Log::logout("        Grace period : 0x%02X (%u)\n", pMSG->delayed_displaying_period, pMSG->delayed_displaying_period);

            uint16_t cmpDate;
            bool ovf;
            switch (pMSG->status) {
                case MSG_STS_DoNotShow:  // 0x00 : Hide message
                    returnCode = 0xA101;
                    Log::logout("    * EMD disabled for message hide state\n");
                    break;

                case MSG_STS_Unused:  // 0x01 : Unused card (status where only NHK 0x01 exists)
                    returnCode = 0xA101;
                    if (!Date && !PeriodOfTime) {  // Clear message control area if date and grace period are 0
                        Log::logout("    * Execute message deletion\n");
                        memset(pMSG, 0x00, sizeof(MESSAGE_t));

                    } else if (PeriodOfTime == 0xff) {  // Do nothing if the grace period is 0xff
                        Log::logout("    * Grace period disabled\n");

                    } else {
                        ovf = (((uint32_t)Date + (uint32_t)PeriodOfTime) > 0xffff);
                        if (ovf) {
                            Log::logout("    * Date + grace period 16bit overflow\n");
                            returnCode = 0x2100;
                        }

                        pMSG->status = MSG_STS_DuringDisplayGracePeriodOrDisplay;  // Change status to "in display grace period or display"
                        st_be16(pMSG->start_date, Date);                           // Update start date
                        pMSG->delayed_displaying_period = PeriodOfTime;             // Update grace period
                        Log::logout("    * Update status, start date, and grace period\n");
                    }
                    break;

                case MSG_STS_DuringDisplayGracePeriodOrDisplay:  // 0x02 : During display grace period or being displayed (status where only NHK 0x01 exists)
                    if ((Date < ld_be16(pMSG->start_date)) && (PeriodOfTime != 0xff)) {
                        Log::logout("    * Update start date and grace period\n");
                        st_be16(pMSG->start_date, Date);
                        pMSG->delayed_displaying_period = PeriodOfTime;
                    }

                    ovf = (((uint32_t)ld_be16(pMSG->start_date) + (uint32_t)pMSG->delayed_displaying_period) > 0xffff);
                    if (ovf) {
                        returnCode = 0x2100;
                        Log::logout("    * Date in card + grace period 16bit overflow\n");
                    } else {
                        cmpDate = ld_be16(pMSG->start_date) + pMSG->delayed_displaying_period;
                        if (Date > cmpDate) {
                            Log::logout("    * Displaying message\n");
                            returnCode = 0x2100;
                        } else {
                            Log::logout("    * Waiting for message display\n");
                            returnCode = 0xA101;
                        }
                    }
                    break;

                case MSG_STS_Showing:  // 0x03 : Displaying message
                default:
                    if (ld_be16(cmd->FixedPart.Date) > ld_be16(pMSG->expiry_date)) {
                        Log::logout("    * Displaying message / Invalid date\n");
                        returnCode = 0xA101;
                    } else {
                        Log::logout("    * Displaying message\n");
                        returnCode = 0x2100;
                    }
                    break;
            }
        }

    EXIT_FUNCTION:
        Log::logout("    Return code        : 0x%04X\n", returnCode);

        // If it ends normally, copy the information from the temporary area to the corresponding message control area
        if (bgID < BGID_COUNT) {
            pMSG->checkDigit = Utils::calc_message_check_digit(pMSG);
            memcpy(pMESSAGE(bgID), pMSG, sizeof(MESSAGE_t));
        }

        res->ProtocolNumber = 0;
        res->UnitLength = offsetof(RES, SW1) - offsetof(RES, UnitLength) - 1;
        st_be16(res->ICCardInstruction, getCardStatus());
        st_be16(&res->ReturnCode, returnCode);

        uint8_t *p = &res->SW1;
        if (returnCode == 0x2100) {
            st_be16(&res->LimitDate, ld_be16(pMSG->expiry_date));
            st_be16(&res->FixedMessageID, ld_be16(pMSG->fixed_phrase_message_number));
            res->ExtraMessageFormatVersion = pMSG->diff_format_number;

            uint16_t len = ld_be16(pMSG->diff_information_length);
            if (len > sizeof(pMSG->diff_information)) len = sizeof(pMSG->diff_information);
            st_be16(&res->ExtraMessageLength, len);
            memcpy(p, pMSG->diff_information, len);
            res->UnitLength += len;
            resSize += len;
            p += len;
        }
        st_be16(p, swCode);

        return resCopy(pbRecvBuffer, pcbRecvLength, recvTemp, resSize);
    }

    //
    //
    //
    // CHK: Contract confirmation command
    LONG Card::processCmd3C(LPCBYTE pbSendBuffer, DWORD cbSendLength, LPBYTE pbRecvBuffer, LPDWORD pcbRecvLength)
    {
        typedef struct {
            uint8_t CLA;                    // 0x90
            uint8_t INS;                    // 0x3A
            uint8_t P1;                     // 0x00
            uint8_t P2;                     // 0x00
            uint8_t Lc;                     // Command length

            struct {                        // Fixed part
                uint8_t Date[2];
                uint8_t ProtocolNumber;
                uint8_t BroadcastGroupID;
                uint8_t WorkKeyID;
            } FixedPart;

            struct {                        // CVI
                uint8_t ProgramType;        // Start of encryption
                uint8_t RecordingControl;
                uint8_t UnknownValue;
            } Cvi;

            uint8_t Le;                     // Response data length (0x00) / Normally, a contract information confirmation command is inserted between Cvi and Le

        } CMD;

        typedef struct {
            uint8_t ProtocolNumber;
            uint8_t UnitLength;
            uint8_t ICCardInstruction[2];
            uint8_t ReturnCode[2];
            uint8_t BroadcastGroupID;
            uint8_t RecordingControl;       // 0x00: Recording not available / 0x01: Recording available / 0x10: Recording available only for purchaser
            uint8_t ppvNumber[2];
            uint8_t ppvPrice1[2];
            uint8_t ppvPrice2[2];
            uint8_t ReservationPurchaseDeadline[2];
            uint8_t PrepaidMinimumBalance[2];
            uint8_t SW1;
            uint8_t SW2;
        } RES;

        uint16_t returnCode = 0x8901;       // 0x8901: Outside the contract
        uint16_t swCode = 0x9000;

        CMD *cmd = (CMD *)pbSendBuffer;

        uint8_t recvTemp[300] = { 0 };
        RES *res = (RES *)recvTemp;
        DWORD resSize = sizeof(RES);

        uint8_t bgID = 0xff;

        Log::logout("[CHK command received]\n");

        {
            if ((cbSendLength < 5) || (cmd->Lc != (cbSendLength - 6)) || (cmd->Lc > CHK_DATA_MAX_LENGTH)) {
                Log::logout("    Command length abnormal\n");
                return resError(pbRecvBuffer, pcbRecvLength, 0x6700);
            }

            if (cmd->P1 || cmd->P2) {
                Log::logout("    P1 / P2 abnormal: [ P1:0x%02X / P2: 0x%02X ]\n", cmd->P1, cmd->P2);
                return resError(pbRecvBuffer, pcbRecvLength, 0x6A86);
            }

            uint8_t Le = *(pbSendBuffer + cbSendLength - 1);
            if (Le) {
                Log::logout("    Le abnormal: 0x%02X\n", Le);
                return resError(pbRecvBuffer, pcbRecvLength, 0x6700);
            }

            if (cmd->Lc < 8) {
                Log::logout("    Invalid data length (less than 8 bytes)\n");
                returnCode = 0xA104;  // Security error
                goto EXIT_FUNCTION;
            }

            bgID = cmd->FixedPart.BroadcastGroupID;
            bool invalidProtocolNumber = ((cmd->FixedPart.ProtocolNumber != 0x01) && (cmd->FixedPart.ProtocolNumber != 0x05) &&
                                            (cmd->FixedPart.ProtocolNumber != 0x41) && (cmd->FixedPart.ProtocolNumber != 0x45));
            bool invalidBroadcastGroupID = (bgID >= BGID_COUNT);

            Log::logout("    Date               : %s\n", Utils::mjd_to_string(ld_be16(cmd->FixedPart.Date)));
            Log::logout("    Protocol number    : 0x%02X%s\n", cmd->FixedPart.ProtocolNumber, invalidProtocolNumber?" * Non-operational protocol number":"");
            Log::logout("    Broadcast group ID : 0x%02X (%s)\n", bgID, invalidBroadcastGroupID?" * Invalid Broadcast group ID":Utils::BroadcastGroupID_to_name(bgID));
            Log::logout("    Work key ID        : 0x%02X\n", cmd->FixedPart.WorkKeyID);

            if (invalidProtocolNumber) {
                returnCode = 0xA102;  // Non-operational protocol number error
                bgID = 0xff;
                goto EXIT_FUNCTION;
            }

            if (invalidBroadcastGroupID) {
                returnCode = 0xA1FE;  // Other error
                bgID = 0xff;
                goto EXIT_FUNCTION;
            }

            TIER_t *pT = pTIER(bgID);

            if (!pT->ActivationState) {
                Log::logout("    * Non-contractual broadcast\n");
                returnCode = 0xA103;  // Non-contract
                goto EXIT_FUNCTION;
            }

            // Get work key
            uint64_t key = getWorkKey(cmd->FixedPart.BroadcastGroupID, cmd->FixedPart.WorkKeyID);
            if (!key) {
                Log::logout("    * No work key\n");
                returnCode = 0xA103;  // Non-contract
                goto EXIT_FUNCTION;
            }

            // Message decryption
            uint8_t tmp[300];
            memcpy(tmp, cmd, cbSendLength);

            uint32_t decodingStartPoint = offsetof(CMD, Cvi.ProgramType);
            uint32_t decodingLength = (uint16_t)(cbSendLength - decodingStartPoint - 1);
            const uint8_t *in = (const uint8_t *)cmd + decodingStartPoint;
            uint8_t *out = &tmp[ decodingStartPoint ];
            Crypto::decrypt(out, in, decodingLength, key, cmd->FixedPart.ProtocolNumber);
            cmd = (CMD *)tmp;

            Log::logout("    Decryption key     : 0x%016llX\n", key);
            Log::logout("    Decrypted data     : [ ");
            Log::logout_dump(out, decodingLength, 0);
            Log::logout(" ]\n");

            Log::logout("    Judgment type      : 0x%02X\n", cmd->Cvi.ProgramType);
            Log::logout("    Recording control  : 0x%02X\n", cmd->Cvi.RecordingControl);
            Log::logout("    Unknown value      : 0x%02X\n", cmd->Cvi.UnknownValue);

            uint8_t ProgramType = cmd->Cvi.ProgramType & 0x03;  // Truncate to 2 bits
            if ( ProgramType == 0) {  // Always returns 0xA1FE if the judgment type is 0
                returnCode = 0xA1FE;
                goto EXIT_FUNCTION;
            } else if (ProgramType & 1) {  // Always returns 0x0800 if judgment type is 1 or 3
                returnCode = 0x0800;
                goto EXIT_FUNCTION;
            }

            // Variable length data part processing
            uint8_t *p = &tmp[ offsetof(CMD,Le) ];
            long remain = cmd->Lc - 8;

            bool Contracted;
            returnCode = 0xA1FE;
            while (remain > 0) {
                Log::logout("\n        Function number : 0x%02X ", *p);
                switch (*p) {
                    case 0x52:  // CheckContract bitmap
                        Contracted = processNano52(pT, p);
                        Log::logout("            Contract status          : ");
                        if (!checkExpiryDate(pT, ld_be16(cmd->FixedPart.Date), 0x00) || (pT->ActivationState != 2)) {
                            Log::logout("Expired %s\n", Utils::mjd_to_string(ld_be16(pT->ExpiryDate)));
                            returnCode = 0x8902;
                        } else if (Contracted) {
                            Log::logout("Purchased\n");
                            returnCode = 0x0800;
                        } else {
                            Log::logout("No contract\n");
                            returnCode = 0x8901;
                        }
                        break;

                    default:
                        Log::logout("<Unsupported functions>\n");
                        Log::logout("\n");
                        break;
                }

                remain -= (p[1] + 2);
                p += (p[1] + 2);
            }

            if (remain != 0) {  // Variable length parameter length error?
                Log::logout("    * Detect errors in variable length data\n");
                returnCode = 0x0A104;  // Security error
                goto EXIT_FUNCTION;
            }
        }

    EXIT_FUNCTION:
        Log::logout("    Return code        : 0x%04X\n", returnCode);

        res->ProtocolNumber = 0;
        res->UnitLength = offsetof(RES, SW1) - offsetof(RES, UnitLength) - 1;
        st_be16(res->ICCardInstruction, getCardStatus());
        st_be16(&res->ReturnCode, returnCode);
        res->BroadcastGroupID = bgID;
        st_be16(&res->ppvNumber, 0x0000);
        st_be16(&res->ppvPrice1, 0x0000);
        st_be16(&res->ppvPrice2, 0x0000);
        st_be16(&res->ReservationPurchaseDeadline, 0x0000);
        st_be16(&res->PrepaidMinimumBalance, 0x0000);
        st_be16(&res->SW1, swCode);

        return resCopy(pbRecvBuffer, pcbRecvLength, recvTemp, resSize);
    }

    //
    //
    //
    // Request power control information command
    LONG Card::processCmd80(LPCBYTE pbSendBuffer, DWORD cbSendLength, LPBYTE pbRecvBuffer, LPDWORD pcbRecvLength)
    {
        typedef struct {
            uint8_t CLA;                    // 0x90
            uint8_t INS;                    // 0x3A
            uint8_t P1;                     // 0x00
            uint8_t P2;                     // 0x00
            uint8_t Lc;                     // Command length
            struct {                        // Fixed part
                uint8_t PowerInfoNumber;
            } FixedPart;
            uint8_t Le;                     // Response data length (0x00)
        } CMD;

        typedef struct {
            uint8_t ProtocolNumber;
            uint8_t UnitLength;
            uint8_t ICCardInstruction[2];
            uint8_t ReturnCode[2];
            uint8_t PowerInfoNumber;
            uint8_t EndOfPowerInfoNumber;
            uint8_t BroadcastGroupID;
            uint8_t PowerOnStartDate[2];
            uint8_t PowerOnStartDateOffset;
            uint8_t PowerOnPeriod;
            uint8_t PowerSupplyHoldTime;
            uint8_t ReceiveNetwork[2];
            uint8_t ReceiveTS[2];
            uint8_t SW1;
            uint8_t SW2;
        } RES;

        uint16_t returnCode = 0x2100;  // 0xA101 No corresponding data  0x2100 Successful completion
        uint16_t swCode = 0x9000;

        CMD *cmd = (CMD *)pbSendBuffer;

        uint8_t recvTemp[300] = { 0 };
        RES *res = (RES *)recvTemp;
        DWORD resSize = sizeof(RES);

        Log::logout("[Request power control information command]\n");

        if ((cbSendLength != sizeof(CMD)) || (cmd->Lc != (cbSendLength - 6))) {
            Log::logout("    Command length abnormal\n");
            return resError(pbRecvBuffer, pcbRecvLength, 0x6700);
            return true;
        }

        if (cmd->P1 || cmd->P2) {
            Log::logout("    P1 / P2 abnormal: [ P1:0x%02X / P2: 0x%02X ]\n", cmd->P1, cmd->P2);
            return resError(pbRecvBuffer, pcbRecvLength, 0x6A86);
        }

        uint8_t Le = *(pbSendBuffer + cbSendLength - 1);
        if (Le) {
            Log::logout("    Le abnormal: 0x%02X\n", Le);
            return resError(pbRecvBuffer, pcbRecvLength, 0x6700);
        }

        // List of valid broadcast
        uint8_t BroadcastGroupID = 0xff;
        uint8_t PowerInfoCount = 0;
        for (uint8_t i = 0;i < BGID_COUNT; i++) {
            TIER_t *pT = pTIER(i);
            if (pT->ActivationState != 2) continue;
            if (!pT->PowerOn.PowerOnPeriod) continue;
            if (cmd->FixedPart.PowerInfoNumber == PowerInfoCount) {
                BroadcastGroupID = i;
            }
            PowerInfoCount++;
        }

        if (BroadcastGroupID == 0xff) {
            returnCode = 0xA101;
            Log::logout("    No corresponding data\n");
            goto EXIT_FUNCTION;
        }

        Log::logout("    Broadcast Group ID            : 0x%02X\n", BroadcastGroupID);
        Log::logout("    Reference Power on start date : %s\n", Utils::mjd_to_string(ld_be16(pTIER(BroadcastGroupID)->ExpiryDate)));
        Log::logout("    Power on start date offset    : 0x%02X (%u)\n",pTIER(BroadcastGroupID)->PowerOn.PowerOnStartDateOffset, pTIER(BroadcastGroupID)->PowerOn.PowerOnStartDateOffset);
        Log::logout("    Power on period               : 0x%02X (%u)\n",pTIER(BroadcastGroupID)->PowerOn.PowerOnPeriod, pTIER(BroadcastGroupID)->PowerOn.PowerOnPeriod);
        Log::logout("    Power supply hold time        : 0x%02X (%u)\n", pTIER(BroadcastGroupID)->PowerOn.PowerSupplyHoldTime, pTIER(BroadcastGroupID)->PowerOn.PowerSupplyHoldTime);
        Log::logout("    Receiving network             : 0x%04X\n", ld_be16(pTIER(BroadcastGroupID)->PowerOn.ReceiveNetwork));
        Log::logout("    Receiving TS                  : 0x%04X\n", ld_be16(pTIER(BroadcastGroupID)->PowerOn.ReceiveTS));

    EXIT_FUNCTION:
        Log::logout("    Return code                   : 0x%04X\n", returnCode);

        res->ProtocolNumber = 0;
        res->UnitLength = offsetof(RES, SW1) - offsetof(RES, UnitLength) - 1;
        st_be16(res->ICCardInstruction, getCardStatus());
        st_be16(&res->ReturnCode, returnCode);
        res->PowerInfoNumber = cmd->FixedPart.PowerInfoNumber;
        res->EndOfPowerInfoNumber = PowerInfoCount - 1;
        if (BroadcastGroupID != 0xff) {
            TIER_t *pT = pTIER(BroadcastGroupID);
            res->BroadcastGroupID = BroadcastGroupID;
            st_be16(res->PowerOnStartDate, ld_be16(pT->ExpiryDate));
            res->PowerOnStartDateOffset = pT->PowerOn.PowerOnStartDateOffset;
            res->PowerOnPeriod = pT->PowerOn.PowerOnPeriod;
            res->PowerSupplyHoldTime = pT->PowerOn.PowerSupplyHoldTime;
            memcpy(res->ReceiveNetwork, pT->PowerOn.ReceiveNetwork, 2);
            memcpy(res->ReceiveTS, pT->PowerOn.ReceiveTS, 2);
        }
        st_be16(&res->SW1, swCode);

        return resCopy(pbRecvBuffer, pcbRecvLength, recvTemp, resSize);
    }

    //
    //
    //
    // UL process
    bool Card::processULCMD(LPCBYTE pbSendBuffer, DWORD cbSendLength, LPBYTE pbRecvBuffer, LPDWORD pcbRecvLength)
    {
        static const uint8_t Toshiba[]    = { 0xC0, 0xFA, 0x00, 0x00, 0x07, 0x38, 0x36, 0x37, 0x34, 0x34, 0x32, 0x32 };
        static const uint8_t PinClear[]   = { 0x00, 0x20, 0x00, 0x91 };
        static const uint8_t Cmd8054[]    = { 0x80, 0x54, 0x00, 0x91 };
        static const uint8_t PinSet[]     = { 0x00, 0x20, 0x00, 0x91, 0x04, 0x35, 0x31, 0x32, 0x39 };
        static const uint8_t SelectBC01[] = { 0x00, 0xa4, 0x02, 0x0c, 0x02, 0xbc, 0x01 };

        typedef struct {
            uint8_t SW1;
            uint8_t SW2;
        } RES;

        if (*pcbRecvLength < sizeof(RES) || 1) {
            return false;
        }

        bool sts = false;
        if (cbSendLength == sizeof(Toshiba) && !memcmp(pbSendBuffer, Toshiba, sizeof(Toshiba))) {
            Log::logout("[Toshiba command]\n");
            selectBC01 = false;
            ulStatus |= 0x01;
            sts = true;
        } else if (cbSendLength == sizeof(Cmd8054) && !memcmp(pbSendBuffer, Cmd8054, sizeof(Cmd8054))) {
            Log::logout("[8054 command]\n");
            selectBC01 = false;
            ulStatus |= 0x02;
            sts = true;
        } else if (cbSendLength == sizeof(PinSet) && !memcmp(pbSendBuffer, PinSet, sizeof(PinSet))) {
            Log::logout("[PinSet command]\n");
            selectBC01 = false;
            ulStatus |= 0x04;
            sts = true;
        } else if (cbSendLength == sizeof(PinClear) && !memcmp(pbSendBuffer, PinClear, sizeof(PinClear))) {
            Log::logout("[PinClear command]\n");
            selectBC01 = false;
            sts = true;
        } else if (cbSendLength == sizeof(SelectBC01) && !memcmp(pbSendBuffer, SelectBC01, sizeof(SelectBC01))) {
            Log::logout("[SelectBC01 command]\n");
            selectBC01 = ul;
            sts = true;
        }

        if (sts) {
            *pcbRecvLength = sizeof(RES);
            RES *res = (RES *)pbRecvBuffer;
            st_be16(&res->SW1, 0x9000);
            if (!ul && ulStatus == 0x07) {
                Log::logout("    * Successfully UL\n");
                ul = true;
            }
        }

        return sts;
    }

    //
    //
    //
    // Memory read through
    bool Card::processReadBC01(LPCBYTE pbSendBuffer, DWORD cbSendLength, LPBYTE pbRecvBuffer, LPDWORD pcbRecvLength)
    {
        if (!ul) return false;
        if (cbSendLength != 5 && cbSendLength != 7) return false;
        if (pbSendBuffer[0] != 0x00 || pbSendBuffer[1] != 0xb0) return false;

        Log::logout("[Memory read command]\n");

        if (*pcbRecvLength < 2) {
            Log::logout("    Insufficient receive buffer (%lu)\n",  *pcbRecvLength);
            return false;
        }

        uint16_t addr = (selectBC01) ? ld_be16(pbSendBuffer + 2) : 0;
        uint16_t size = 0;
        if (cbSendLength == 7) {
            if (pbSendBuffer[4]) {
                Log::logout("    Byte length error\n");
                return false;
            }
            size = ld_be16(pbSendBuffer + 5);
        } else {
            size = pbSendBuffer[4];
            if (!size) size = 256;  // If byte length is 0, set to 256 bytes?
        }

        uint16_t bs = (uint16_t)((size > (*pcbRecvLength - 2)) ? (*pcbRecvLength - 2) : size);
        uint16_t overBytes = 0;
        for (uint16_t i = 0; i < bs; i++) {
            uint16_t a = addr + i;
            if (a >= sizeof(cardImage)) {
                overBytes++;
            }
            a %= sizeof(cardImage);
            pbRecvBuffer[ i ] = cardImage[ a ] ^ 0xff;
        }

        st_be16(&pbRecvBuffer[bs], 0x9000);
        *pcbRecvLength = bs + 2;

        if (!selectBC01) {
            Log::logout("    * BC01 Not Selected\n");
        } else {
            Log::logout("    First address : ");
            Log::logout_address_name(addr);
            Log::logout("    Last address  : ");
            Log::logout_address_name(addr + size - 1);
        }

        Log::logout("    Byte length   : 0x%04X (%u)\n", size, size);
        Log::logout("    Byte data     : ");
        Log::logout_dump(&cardImage[ addr ], bs, 5);
        if (overBytes) Log::logout("    Data overflow : [%u bytes over]\n", overBytes);

        return true;
    }

    //
    //
    //
    // Memory write through
    bool Card::processWriteBC01(LPCBYTE pbSendBuffer, DWORD cbSendLength, LPBYTE pbRecvBuffer, LPDWORD pcbRecvLength, bool *writeID)
    {
        *writeID = false;
        if (!ul) return false;
        if (cbSendLength < 5) return false;
        if (pbSendBuffer[0] != 0x00 || pbSendBuffer[1] != 0xd6) return false;

        Log::logout("[Memory write command]\n");

        if (*pcbRecvLength < 2) {
            Log::logout("    Insufficient receive buffer (%lu)\n",  *pcbRecvLength);
            return false;
        }

        uint16_t addr = (selectBC01) ? ld_be16(pbSendBuffer + 2) : 0;
        uint16_t size = pbSendBuffer[4];
        uint16_t offset = 5;
        if (size == 0) {
            if (cbSendLength < 7) {
                Log::logout("    Command length error (%lu)\n", cbSendLength);
                return false;
            }
            size = ld_be16(pbSendBuffer + 5);
            offset = 7;
        }

        if ((DWORD)(offset + size) != cbSendLength) {
            Log::logout("    Command length error (%lu)\n", cbSendLength);
            return false;
        }

        uint8_t *src = (uint8_t *)(pbSendBuffer + offset);
        uint16_t overBytes = 0;
        for (uint16_t i = 0; i < size; i++) {
            uint16_t a = addr + i;
            if (a >= sizeof(cardImage)) {
                overBytes++;
            }
            a %= sizeof(cardImage);
            cardImage[ a ] = src[i] ^ 0xff;
            if ((a >= INFO_ADDR) && (a <= (INFO_ADDR + sizeof(INFO_t) - 1))) {
                *writeID = true;
            }
        }
        st_be16(pbRecvBuffer, 0x9000);
        *pcbRecvLength = 2;

        if (!selectBC01) {
            Log::logout("    * BC01 Not Selected\n");
        } else {
            Log::logout("    First address : ");
            Log::logout_address_name(addr);
            Log::logout("    Last address  : ");
            Log::logout_address_name(addr + size - 1);
        }

        Log::logout("    Byte length   : 0x%04X (%u)\n", size, size);
        Log::logout("    Byte data     : ");
        Log::logout_dump(&cardImage[ addr ], size, 5);
        if (overBytes) Log::logout("    Data overflow : [%u bytes over]\n", overBytes);

        return true;
    }

    //
    //
    //
    // Function Number: 10h : Update work key of the specified work key ID
    void Card::processNano10(TIER_t *pT, uint8_t BroadcastGroupID, uint8_t *p, bool updateEnable)
    {
        typedef struct {
            uint8_t WorkKeyID;
            uint8_t Key[8];
        } KEY_t;

        typedef struct {
            uint8_t Nano;
            uint8_t Length;
            KEY_t ky;
        } CMD;

        CMD *cmd = (CMD *)p;

        uint8_t len = cmd->Length;
        if (len > sizeof(KEY_t)) len = sizeof(KEY_t);

        KEY_t k;
        memset(&k, 0x00, sizeof(KEY_t));
        memcpy(&k, &cmd->ky, len);
        if (updateEnable) {
            updateTierWorkKey(pT, k.WorkKeyID, ld_be64(k.Key));
            setWorkKey(BroadcastGroupID, k.WorkKeyID, ld_be64(k.Key), false);
        }

        Log::logout("[Update work key of the specified work key ID]\n");
        Log::logout("            Work key ID : 0x%02X\n", k.WorkKeyID);
        Log::logout("            Work key    : 0x%016llX\n", ld_be64(k.Key));
        Log::logout("%s", (!updateEnable) ? "            * Not executed\n" : "");
    }

    //
    //
    //
    // Function Number: 11h : Update contract bit flag
    void Card::processNano11(TIER_t *pT, uint8_t *p, bool updateEnable)
    {
        typedef struct {
            uint8_t bitmap[32];
        } Contract_t;

        typedef struct {
            uint8_t Nano;
            uint8_t Length;
            Contract_t con;
        } CMD;
        CMD *cmd = (CMD *)p;

        uint8_t len = cmd->Length;
        if (len > sizeof(Contract_t)) len = sizeof(Contract_t);

        Contract_t con;
        memset(&con, 0x00, sizeof(Contract_t));
        memcpy(&con, &cmd->con, len);
        if (updateEnable) memcpy(&pT->Bitmap, &con, sizeof(con));

        Log::logout("[Update contract bit flag]\n");
        Log::logout("            Data byte length         : 0x%02X (%u)\n", cmd->Length, cmd->Length);
        if (len) {
            Log::logout("            Contract bit information : ");
            Log::logout_bitmap(con.bitmap, len, 39);
        }
        Log::logout("\n");
        Log::logout("%s", (!updateEnable) ? "            * Not executed\n" : "");
    }

    //
    //
    //
    // Function Number: 13h : Add/Update group ID
    void Card::processNano13(uint8_t *p, bool updateEnable)
    {
        typedef struct {
            uint8_t id[6];
            uint8_t km[8];
        } GroupID_t;

        typedef struct {
            uint8_t Nano;
            uint8_t Length;
            GroupID_t id;
        } CMD;
        CMD *cmd = (CMD *)p;

        uint8_t len = cmd->Length;
        if (len > sizeof(GroupID_t)) len = sizeof(GroupID_t);

        GroupID_t gid;
        memset(&gid, 0x00, sizeof(GroupID_t));
        memcpy(&gid, &cmd->id, len);

        uint64_t id = ld_be48(gid.id);
        uint64_t km = ld_be64(gid.km);
        uint8_t grpID = (uint8_t)(id >> 45);
        if (grpID == 0) updateEnable = false;  // Invalid group number?

        if (updateEnable) {
            GROUP_ID_t *p = &pINFO()->grpID[ grpID - 1 ];

            st_be48(p->ID, id);
            st_be16(p->ID_CheckDigit, Utils::calc_cardID_check_digit(id));

            st_be64(p->Km, km);
            p->Km_CheckDigit = Utils::calc_Km_check_digit(km);

            pINFO()->GroupID_Flag1 |= (1 << grpID);
            pINFO()->GroupID_Flag2 |= (1 << grpID);
        }

        Log::logout("[Add/Update group ID]\n");
        Log::logout("            Data byte length : 0x%02X (%u)\n", cmd->Length, cmd->Length);
        Log::logout("            Group ID type    : %u\n", grpID);
        Log::logout("            Group ID         : 0x%012llX\n", id);
        Log::logout("            Master key (Km)  : 0x%016llX\n", km);
        Log::logout("            Group ID string  : %s\n", Utils::cardID_to_string(id));
        Log::logout("%s", (!updateEnable) ? "            * Not executed\n" : "");
    }

    //
    //
    //
    // Function Number: 14h : Group ID invalidation
    void Card::processNano14(uint8_t *p, bool updateEnable)
    {
        typedef struct {
            uint8_t grpID;
        } GID_t;

        typedef struct {
            uint8_t Nano;
            uint8_t Length;
            GID_t id;
        } CMD;
        CMD *cmd = (CMD *)p;

        uint8_t len = cmd->Length;
        if (len > sizeof(GID_t)) len = sizeof(GID_t);

        GID_t g;
        memset(&g, 0x00, sizeof(GID_t));
        memcpy(&g, &cmd->id, len);

        g.grpID &= 0x07;  // Valid only for the lower 3 bits (follow as the actual card)
        if (g.grpID == 0) updateEnable = false;  // Invalid group number?
        if (updateEnable) {
            pINFO()->GroupID_Flag1 &= ~(1 << g.grpID);  // Just clear the flag
            pINFO()->GroupID_Flag2 &= ~(1 << g.grpID);
        }

        Log::logout("[Group ID invalidation]\n");
        Log::logout("            Data byte length : 0x%02X (%u)\n", cmd->Length, cmd->Length);
        Log::logout("            Group ID type    : %u\n", g.grpID);
        Log::logout("%s", (!updateEnable) ? "            * Not executed\n" : "");
    }

    //
    //
    //
    // Function Number: 20h : Update power control information
    void Card::processNano20(TIER_t *pT, uint8_t *p, bool updateEnable)
    {
        typedef struct {
            uint8_t PowerOnStartDateOffset;
            uint8_t PowerOnPeriod;
            uint8_t PowerSupplyHoldTime;
            uint8_t ReceiveNetwork[2];
            uint8_t ReceiveTS[2];
        } Power_t;

        typedef struct {
            uint8_t Nano;
            uint8_t Length;
            Power_t PowerOn;
        } CMD;
        CMD *cmd = (CMD *)p;

        uint8_t len = cmd->Length;
        if (len > sizeof(Power_t)) len = sizeof(Power_t);

        Power_t pow;
        memset(&pow, 0x00, sizeof(Power_t));
        memcpy(&pow, &cmd->PowerOn, len);
        if (updateEnable) {
            memcpy(&pT->PowerOn, &pow, sizeof(pow));
        }

        Log::logout("[Update power control information]\n");
        Log::logout("            Power on start date offset : 0x%02X (%u)\n", pow.PowerOnStartDateOffset, pow.PowerOnStartDateOffset);
        Log::logout("            Power on period            : 0x%02X (%u)\n", pow.PowerOnPeriod, pow.PowerOnPeriod);
        Log::logout("            Power supply hold time     : 0x%02X (%u)\n", pow.PowerSupplyHoldTime, pow.PowerSupplyHoldTime);
        Log::logout("            Receiving network          : 0x%04X\n", ld_be16(pow.ReceiveNetwork));
        Log::logout("            Receiving TS               : 0x%04X\n", ld_be16(pow.ReceiveTS));
        Log::logout("%s", (!updateEnable) ? "            * Not executed\n" : "");
    }

    //
    //
    //
    // Function Number: 21h : Multi-function
    void Card::processNano21(TIER_t *pT, MESSAGE_t *pMSG, uint8_t *p, uint8_t Ins, bool updateEnable)
    {
        typedef struct {
            uint8_t FunctionNumber;
        } FNC_t;

        typedef struct {
            uint8_t Nano;
            uint8_t Length;
            FNC_t fn;
        } CMD;
        CMD *cmd = (CMD *)p;

        uint8_t len = cmd->Length;
        if (len > sizeof(FNC_t)) len = sizeof(FNC_t);

        FNC_t f;
        memset(&f, 0x00, sizeof(FNC_t));
        memcpy(&f, &cmd->fn, len);

        Log::logout("[Multi-function]\n");

        bool update = false;
        switch (f.FunctionNumber) {
            case 0x01:  // InvalidateTier
                Log::logout("            Deactivate broadcast information\n");
                if (updateEnable) {
                    pT->ActivationState = 0;
                    update = true;
                }
                break;

            case 0x02:  // ResetUpdateNumbers
                Log::logout("            reset update number\n");
                if (updateEnable) {
                    st_be16(pT->UpdateNumber1, 0x0000);
                    st_be16(pMSG->UpdateNumber, 0x0000);
                    update = true;
                }
                break;

            case 0xff:  // ResetTrial
                Log::logout("            Reset trial viewing\n");
                if (updateEnable) {
                    if (Ins == INS_EMM) {
                        pT->ActivationState = 1;
                        st_be16(pT->ExpiryDate, 0x0000);
                        pT->ExpiryHour = 0x00;
                        update = true;
                    }
                }
                break;

            default:
                Log::logout("            Unknown function number [0x%02X]\n", f.FunctionNumber);
                break;
        }
        Log::logout("%s", (!update) ? "            * Not executed\n" : "");
    }

    //
    //
    //
    // Function Number: 23h : Invalidate tier by specifying the card ID
    void Card::processNano23(TIER_t *pT, uint8_t *p)
    {
        typedef struct {
            uint8_t CardID[6];
        } ID_t;

        typedef struct {
            uint8_t Nano;
            uint8_t Length;
            ID_t id;
        } CMD;
        CMD *cmd = (CMD *)p;

        uint8_t len = cmd->Length;
        if (len > sizeof(ID_t)) len = sizeof(ID_t);

        ID_t id;
        memset(&id, 0x00, sizeof(ID_t));
        memcpy(&id, &cmd->id, len);

        bool matchID = (ld_be48(id.CardID) == ld_be48(pINFO()->ID));
        bool updateEnable = false;
        if (matchID) {
            pT->ActivationState = 0;
            updateEnable = true;
        }

        Log::logout("[Invalidate tier by specifying the card ID]\n");
        Log::logout("            Destination card ID : 0x%012llX (%s)\n", ld_be48(id.CardID), Utils::cardID_to_string(ld_be48(id.CardID)));
        Log::logout("%s", (!updateEnable) ? "            * Not executed\n" : "");
    }

    //
    //
    //
    // Function Number: 51h : Start trial viewing
    void Card::processNano51(uint16_t date, TIER_t *pT, uint8_t *p)
    {
        typedef struct {
            uint8_t Days;
        } DAY_t;

        typedef struct {
            uint8_t Nano;
            uint8_t Length;
            DAY_t d;
        } CMD;
        CMD *cmd = (CMD *)p;

        uint8_t len = cmd->Length;
        if (len > sizeof(DAY_t)) len = sizeof(DAY_t);

        DAY_t d;
        memset(&d, 0x00, sizeof(DAY_t));
        memcpy(&d, &cmd->d, len);

        uint32_t ExpiryDate = date  + d.Days;
        if (ExpiryDate > 0xffff) ExpiryDate = 0xffff;

        bool isExecuted = false;
        if (pT->ActivationState == 1) {
            pT->ActivationState = 2;
            st_be16(pT->ExpiryDate, (uint16_t)ExpiryDate);
            pT->ExpiryHour = 0x23;
            isExecuted = true;
        }

        Log::logout("[Enable trial viewing]\n");
        Log::logout("            Trial viewing days   : %u days\n", d.Days);
        if (isExecuted) {
            Log::logout("            Trial viewing period : ");
            Log::logout("%s", Utils::mjd_to_string(date));
            Log::logout(" ~ ");
            Log::logout("%s", Utils::mjd_to_string(ExpiryDate));
            Log::logout("\n");
        }
        Log::logout("%s", (!isExecuted) ? "            * Not executed\n" : "");
    }

    //
    //
    //
    // Function Number: 52h : Contract information confirmation
    bool Card::processNano52(TIER_t *pT, uint8_t *p)
    {
        typedef struct {
            uint8_t bitmap[32];
        } Contract_t;

        typedef struct {
            uint8_t Nano;
            uint8_t Length;
            Contract_t con;
        } CMD;
        CMD *cmd = (CMD *)p;

        uint8_t len = cmd->Length;
        if (len > sizeof(Contract_t)) len = sizeof(Contract_t);

        Contract_t con;
        memset(&con, 0x00, sizeof(Contract_t));
        memcpy(&con, &cmd->con, len);

        bool Contracted = false;
        for (int i = 0;i < len; i++) {
            if (con.bitmap[i] & pT->Bitmap[i]) {
                Contracted = true;
                break;
            }
        }

        Log::logout("[Check contract bit flag]\n");
        Log::logout("            Data byte length         : 0x%02X (%u)\n", cmd->Length, cmd->Length);
        Log::logout("            Contract bit information : ");
        Log::logout_bitmap(con.bitmap, len, 39);
        Log::logout("\n");

        return Contracted;
    }

    // Card information work section pointer acquisition
    INFO_t* Card::pINFO(void)
    {
        return (INFO_t *)&cardImage[ INFO_ADDR ];
    }

    // Card status area pointer acquisition
    CARD_STATUS_t* Card::pCARDSTATUS(void)
    {
        return (CARD_STATUS_t *)&cardImage[ CARD_STATUS_ADDR ];
    }

    // Specify BroadcastGroupID TIER part pointer acquisition
    TIER_t* Card::pTIER(uint8_t BroadcastGroupID)
    {
        long addr = TIER_ADDR + (sizeof(TIER_t) * BroadcastGroupID);
        return (TIER_t *)&cardImage[ addr ];
    }

    // Specify BroadcastGroupID MESSAGE part pointer acquisition
    MESSAGE_t* Card::pMESSAGE(uint8_t BroadcastGroupID)
    {
        long addr = MESSAGE_ADDR + (sizeof(MESSAGE_t) * BroadcastGroupID);
        return (MESSAGE_t *)&cardImage[ addr ];
    }

    // Find work key of the specified BroadcastGroupID, WorkKeyID from the card image
    uint64_t Card::findWorkKeyFromCardImage(uint8_t BroadcastGroupID, uint8_t WorkKeyID)
    {
        if (BroadcastGroupID >= BGID_COUNT) return 0;
        TIER_t *t = pTIER(BroadcastGroupID);
        uint8_t idx = (WorkKeyID & 0x01) ? 1 : 0;
        if (t->Keys[idx].WorkKeyID == WorkKeyID) {
            return ld_be64(t->Keys[idx].Key);
        }
        return 0;
    }

    // Get work key of the specified BroadcastGroupID, WorkKeyID
    uint64_t Card::getWorkKey(uint8_t BroadcastGroupID, uint8_t WorkKeyID)
    {
        if (BroadcastGroupID >= BGID_COUNT) return 0;

        // Search for key in card image
        uint64_t key = findWorkKeyFromCardImage(BroadcastGroupID, WorkKeyID);
        if (!key) {
            key = sys.keySets.getWorkKey(BroadcastGroupID, WorkKeyID);
        }
        return key;
    }

    // Update work key information in tier structure
    bool Card::updateTierWorkKey(TIER_t *pT, uint8_t WorkKeyID, uint64_t key)
    {
        uint8_t idx = (WorkKeyID & 0x01) ? 1 : 0;

        bool cardImageUpdateEnable = true;

        if (cardImageUpdateEnable) {
            pT->Keys[idx].WorkKeyID = WorkKeyID;
            st_be64(pT->Keys[idx].Key, key);
        }

        return cardImageUpdateEnable;
    }

    // Update work key of the specified BroadcastGroupID and WorkKeyID
    bool Card::setWorkKey(uint8_t BroadcastGroupID, uint8_t WorkKeyID, uint64_t key, bool cardImageUpdate)
    {
        if (key == 0) return false;
        if (BroadcastGroupID >= BGID_COUNT) return false;

        // Update work key in card image
        bool cardImageUpdateEnable = false;
        if (cardImageUpdate) {
            cardImageUpdateEnable = updateTierWorkKey(pTIER(BroadcastGroupID), WorkKeyID, key);
        }

        return cardImageUpdateEnable;
    }

    // Get the specified ID information
    int Card::getID(uint64_t cardID, GROUP_ID_t **id)  // Card ID: 48-bit / ret: Group ID type
    {
        Cas::INFO_t *info = pINFO();
        if (ld_be48(info->ID) == cardID) {  // Match with the main ID?
            *id = (GROUP_ID_t *)info->ID;
            return 0;
        }

        for (int i = 1; i <= 7; i++) {
            if (info->GroupID_Flag2 & (uint8_t)(1 << i)) {
                if (ld_be48(info->grpID[  i - 1 ].ID) == cardID) {  // Match with the group ID?
                    *id = (GROUP_ID_t *)&info->grpID[ i - 1 ];
                    return i;
                }
            }
        }

        *id = NULL;
        return -1;
    }

    // Check the expiration date of the specified broadcast group
    bool Card::checkExpiryDate(TIER_t *pT, uint16_t date, uint8_t hour)
    {
        uint32_t ExpiryDate = (uint32_t)ld_be16(pT->ExpiryDate) << 8 | pT->ExpiryHour;
        uint32_t checkData = (uint32_t)date << 8 | hour;
        return (checkData <= ExpiryDate);
    }

    // Change the card status
    void Card::changeCardStatus(uint16_t sts)
    {
        CARD_STATUS_t *pSTS = pCARDSTATUS();
        st_be16(pSTS->card_status1, sts);
        st_be16(pSTS->card_status2, sts);
    }

    // Get the card status
    uint16_t Card::getCardStatus(void)
    {
        CARD_STATUS_t *pSTS = pCARDSTATUS();
        return ld_be16(pSTS->card_status1);
    }
}
