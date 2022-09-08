#define _WINSCARD_CPP_

#ifdef _WIN32
#define g_rgSCardT1Pci remove_g_rgSCardT1Pci
#endif
#include "project.h"
#include <algorithm>
#include <fstream>
#include <sstream>
#ifdef _WIN32
#include <windows.h>
#include <winscard.h>
#else
#include <sys/stat.h>
#include <PCSC/winscard.h>
#endif
#ifdef _WIN32
#undef g_rgSCardT1Pci
#endif

#ifdef _WIN32
static bool SystemInit(HINSTANCE hinstDLL);
#else
static bool SystemInit();
#endif

//#define ALLLOG

//
//
// Card reader status variables
static const CHAR     readerNameA[]           =  "CobaltCas Smart Card Reader\0\0";
#ifdef _WIN32
static const WCHAR    readerNameW[]           = L"CobaltCas Smart Card Reader\0\0";
#endif
static Cas::Card      *card                   = NULL;        // Card emulator object
#ifdef _WIN32
static HANDLE         h_SCardStartedEvent     = NULL;        // Card reader handle number
#else
static SCARDHANDLE    h_SCardStartedEvent     = 0x35313239;  // Card reader handle number (dummy)
#endif
static bool           isSCardCancelCalled     = false;       // SCardCancel() is called

//
//
// DLL Constructor and Destructor
#ifdef _WIN32
BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
    switch (fdwReason) {
        case DLL_PROCESS_ATTACH:
            SystemInit(hinstDLL);  // Initialize system variables
            h_SCardStartedEvent = CreateEvent(NULL, true, true, NULL);
            break;

        case DLL_PROCESS_DETACH:
            sys.INS = INS_OPEN;
            Log::logout_timestamp();
            Log::logout("[API: DLL_PROCESS_DETACH]\n\n");
            Log::logout(NULL);

            CloseHandle(h_SCardStartedEvent);
            delete card;
            card = NULL;
            break;

        default:
            break;
    }

    return true;
}
#else
void __attribute__((constructor)) SCardVCasInit(void) {
    SystemInit();
}
void __attribute__((destructor)) SCardVCasDestroy(void) {
    sys.INS = INS_OPEN;
    Log::logout_timestamp();
    Log::logout("[API: DLL_PROCESS_DETACH]\n\n");
    Log::logout(NULL);

    delete card;
    card = NULL;
}
#endif

//
//
// DLL API function group (in C format)
extern "C" {

#ifdef _WIN32
    SCARD_IO_REQUEST g_rgSCardT1Pci;
#else
    PCSC_API const SCARD_IO_REQUEST g_rgSCardT0Pci = { SCARD_PROTOCOL_T0, sizeof(SCARD_IO_REQUEST) };
    PCSC_API const SCARD_IO_REQUEST g_rgSCardT1Pci = { SCARD_PROTOCOL_T1, sizeof(SCARD_IO_REQUEST) };
    PCSC_API const SCARD_IO_REQUEST g_rgSCardRawPci = { SCARD_PROTOCOL_RAW, sizeof(SCARD_IO_REQUEST) };
    PCSC_API const char* pcsc_stringify_error(const LONG pcscError)
    {
        static char strError[] = "0x12345678";
        snprintf(strError, sizeof(strError), "0x%08lX", pcscError);
        return strError;
    }
#endif

#ifdef _WIN32
    HANDLE WINAPI SCardAccessStartedEvent(void)
    {
    #ifdef ALLLOG
        Log::logout_timestamp();
        Log::logout("[API: SCardAccessStartedEvent]\n\n");
        Log::logout(NULL);
    #endif
        return h_SCardStartedEvent;
    }
#endif

    LONG WINAPI SCardBeginTransaction(SCARDHANDLE hCard)
    {
    #ifdef ALLLOG
        Log::logout_timestamp();
        Log::logout("[API: SCardBeginTransaction]\n\n");
        Log::logout(NULL);
    #endif
        return SCARD_S_SUCCESS;
    }

    LONG WINAPI SCardCancel(SCARDCONTEXT hContext)
    {
    #ifdef ALLLOG
        Log::logout_timestamp();
        Log::logout("[API: SCardCancel]\n\n");
        Log::logout(NULL);
    #endif
        // If SCardCancel() is called, SCardGetStatusChangeA/W() will return SCARD_E_CANCELLED
        isSCardCancelCalled = true;
        return SCARD_S_SUCCESS;
    }

#ifdef _WIN32
    LONG WINAPI SCardConnectA
#else
    LONG WINAPI SCardConnect
#endif
    (SCARDCONTEXT hContext, LPCSTR szReader, DWORD dwShareMode, DWORD dwPreferredProtocols, LPSCARDHANDLE phCard, LPDWORD pdwActiveProtocol)
    {
        char *reader_name = (char *)szReader;

        Log::logout_timestamp();
        Log::logout("[API: SCardConnectA]\n");
        Log::logout("    hContext: 0x%016llx\n", (uint64_t)hContext);
        Log::logout("    szReader: [%s]\n", reader_name);
        Log::logout("\n");
        Log::logout(NULL);

        if (card)  delete card;
        card = new Cas::Card();
        *phCard = (SCARDHANDLE)h_SCardStartedEvent;
        *pdwActiveProtocol = SCARD_PROTOCOL_T1;
        return SCARD_S_SUCCESS;
    }

#ifdef _WIN32
    LONG WINAPI SCardConnectW(SCARDCONTEXT hContext, LPCWSTR szReader, DWORD dwShareMode, DWORD dwPreferredProtocols, LPSCARDHANDLE phCard, LPDWORD pdwActiveProtocol)
    {
        char reader_name[128];
        WideCharToMultiByte(CP_ACP, 0, (LPCWCH)szReader, -1, reader_name, sizeof(reader_name) - 1, NULL, NULL);  // UTF-16 -> S-JIS

        Log::logout_timestamp();
        Log::logout("[API: SCardConnectA]\n");
        Log::logout("    hContext: 0x%016llx\n", (uint64_t)hContext);
        Log::logout("    szReader: [%s]\n", reader_name);
        Log::logout("\n");
        Log::logout(NULL);

        if (card)  delete card;
        card = new Cas::Card();
        *phCard = (SCARDHANDLE)h_SCardStartedEvent;
        *pdwActiveProtocol = SCARD_PROTOCOL_T1;
        return SCARD_S_SUCCESS;
    }
#endif

    LONG WINAPI SCardControl(SCARDHANDLE hCard, DWORD dwControlCode, LPCVOID pbSendBuffer, DWORD cbSendLength, LPVOID pbRecvBuffer, DWORD cbRecvLength, LPDWORD lpBytesReturned)
    {
    #ifdef ALLLOG
        Log::logout_timestamp();
        Log::logout("[API: SCardControl]\n\n");
        Log::logout(NULL);
    #endif

        // 0 bytes returned by default
        if (lpBytesReturned != NULL) {
            *lpBytesReturned = 0;
        }

        return SCARD_S_SUCCESS;
    }

    LONG WINAPI SCardDisconnect(SCARDHANDLE hCard, DWORD dwDisposition)
    {
        Log::logout_timestamp();
        Log::logout("[API: SCardDisconnect]\n");
        Log::logout("    hCard: 0x%016llx\n", (uint64_t)hCard);
        Log::logout("\n");
        Log::logout(NULL);

        if (card)  {
            delete card;
            card = NULL;
        }
        return SCARD_S_SUCCESS;
    }

    LONG WINAPI SCardEndTransaction(SCARDHANDLE hCard, DWORD dwDisposition)
    {
    #ifdef ALLLOG
        Log::logout_timestamp();
        Log::logout("[API: SCardEndTransaction]\n\n");
        Log::logout(NULL);
    #endif
        return SCARD_S_SUCCESS;
    }

    LONG WINAPI SCardEstablishContext(DWORD dwScope, LPCVOID pvReserved1, LPCVOID pvReserved2, LPSCARDCONTEXT phContext)
    {
    #ifdef ALLLOG
        Log::logout_timestamp();
        Log::logout("[API: SCardEstablishContext]\n");
        Log::logout("    Return  context: %016llx\n", (uint64_t)h_SCardStartedEvent);
        Log::logout("\n");
        Log::logout(NULL);
    #endif
        *phContext = (SCARDCONTEXT)h_SCardStartedEvent;

        return SCARD_S_SUCCESS;
    }

    LONG WINAPI SCardFreeMemory(SCARDCONTEXT hContext, LPCVOID pvMem)
    {
    #ifdef ALLLOG
        Log::logout_timestamp();
        Log::logout("[API: SCardFreeMemory]\n\n");
        Log::logout(NULL);
    #endif
        return SCARD_S_SUCCESS;
    }

    LONG WINAPI SCardGetAttrib(SCARDHANDLE hCard, DWORD dwAttrId, LPBYTE pbAttr, LPDWORD pcbAttrLen)
    {
    #ifdef ALLLOG
        Log::logout_timestamp();
        Log::logout("[API: SCardGetAttrib]\n\n");
        Log::logout(NULL);
    #endif
        return ERROR_NOT_SUPPORTED;
    }

#ifdef _WIN32
    LONG WINAPI SCardGetStatusChangeA
#else
    LONG WINAPI SCardGetStatusChange
#endif
    (SCARDCONTEXT hContext, DWORD dwTimeout, LPSCARD_READERSTATE rgReaderStates, DWORD cReaders)
    {
    #ifdef ALLLOG
        Log::logout_timestamp();
        Log::logout("[API: SCardGetStatusChangeA]\n");
        Log::logout(">   dwCurrentState : %08lx\n", rgReaderStates->dwCurrentState);
        Log::logout(">   dwEventState   : %08lx\n", rgReaderStates->dwEventState);
    #endif

        // Set ATR
        static const uint8_t ATR[] = { 0x3B, 0xF0, 0x12, 0x00, 0xFF, 0x91, 0x81, 0xB1, 0x7C, 0x45, 0x1F, 0x03, 0x99 };
        rgReaderStates->cbAtr = sizeof ATR;
        memcpy(rgReaderStates->rgbAtr, ATR, sizeof ATR);

        // Since it is a smart card emulator, the state is immutable (no inserting and removing occurs)
        rgReaderStates->dwEventState = SCARD_STATE_PRESENT | SCARD_STATE_CHANGED;

        // If state matches source, sleep until timeout
        // sleep indefinitely if the timeout value is INFINITE
        if (rgReaderStates->dwCurrentState == rgReaderStates->dwEventState && dwTimeout > 0) {
            // If SCardCancel() is called, return SCARD_E_CANCELLED without changing the state
            for (DWORD i = 0; i < dwTimeout; i++) {
                Sleep(1);
                if (isSCardCancelCalled) {
                    isSCardCancelCalled = false;
                    rgReaderStates->dwEventState &= ~SCARD_STATE_CHANGED;
                    return SCARD_E_CANCELLED;
                }
            }
            // If timeout occurs, return SCARD_E_TIMEOUT without changing the state
            rgReaderStates->dwEventState &= ~SCARD_STATE_CHANGED;
    #ifdef ALLLOG
            Log::logout("    SCARD_E_TIMEOUT dwTimeout = %lu\n", dwTimeout);
            Log::logout("\n");
            Log::logout(NULL);
    #endif
            return SCARD_E_TIMEOUT;
        }

    #ifdef ALLLOG
        Log::logout("<   dwCurrentState : %08lx\n", rgReaderStates->dwCurrentState);
        Log::logout("<   dwEventState   : %08lx\n", rgReaderStates->dwEventState);
        Log::logout("\n");
        Log::logout(NULL);
    #endif
        return SCARD_S_SUCCESS;
    }

#ifdef _WIN32
    LONG WINAPI SCardGetStatusChangeW(SCARDCONTEXT hContext, DWORD dwTimeout, LPSCARD_READERSTATEW rgReaderStates, DWORD cReaders)
    {
    #ifdef ALLLOG
        Log::logout_timestamp();
        Log::logout("[API: SCardGetStatusChangeW]\n");
        Log::logout(">   dwCurrentState : %08lx\n", rgReaderStates->dwCurrentState);
        Log::logout(">   dwEventState   : %08lx\n", rgReaderStates->dwEventState);
    #endif

        // Set ATR
        static const uint8_t ATR[] = { 0x3B, 0xF0, 0x12, 0x00, 0xFF, 0x91, 0x81, 0xB1, 0x7C, 0x45, 0x1F, 0x03, 0x99 };
        rgReaderStates->cbAtr = sizeof ATR;
        memcpy(rgReaderStates->rgbAtr, ATR, sizeof ATR);

        // Since it is a smart card emulator, the state is immutable (no inserting and removing occurs)
        rgReaderStates->dwEventState = SCARD_STATE_PRESENT | SCARD_STATE_CHANGED;

        // If state matches source, sleep until timeout
        // sleep indefinitely if the timeout value is INFINITE
        if (rgReaderStates->dwCurrentState == rgReaderStates->dwEventState && dwTimeout > 0) {
            // If SCardCancel() is called, return SCARD_E_CANCELLED without changing the state
            for (DWORD i = 0; i < dwTimeout; i++) {
                Sleep(1);
                if (isSCardCancelCalled) {
                    isSCardCancelCalled = false;
                    rgReaderStates->dwEventState &= ~SCARD_STATE_CHANGED;
                    return SCARD_E_CANCELLED;
                }
            }
            // If timeout occurs, return SCARD_E_TIMEOUT without changing the state
            rgReaderStates->dwEventState &= ~SCARD_STATE_CHANGED;
    #ifdef ALLLOG
            Log::logout("    SCARD_E_TIMEOUT dwTimeout = %lu\n", dwTimeout);
            Log::logout("\n");
            Log::logout(NULL);
    #endif
            return SCARD_E_TIMEOUT;
        }

    #ifdef ALLLOG
        Log::logout("<   dwCurrentState : %08lx\n", rgReaderStates->dwCurrentState);
        Log::logout("<   dwEventState   : %08lx\n", rgReaderStates->dwEventState);
        Log::logout("\n");
        Log::logout(NULL);
    #endif
        return SCARD_S_SUCCESS;
    }
#endif

    LONG WINAPI SCardIsValidContext(SCARDCONTEXT hContext)
    {
    #ifdef ALLLOG
        Log::logout_timestamp();
        Log::logout("[API: SCardIsValidContext]\n\n");
        Log::logout(NULL);
    #endif
        return hContext ? SCARD_S_SUCCESS : ERROR_INVALID_HANDLE;
    }

#ifdef _WIN32
    LONG WINAPI SCardListCardsA(SCARDCONTEXT hContext, LPCBYTE pbAtr, LPCGUID rgguidInterfaces, DWORD cguidInterfaceCount, LPSTR mszCards, LPDWORD pcchCards)
    {
    #ifdef ALLLOG
        Log::logout_timestamp();
        Log::logout("[API: SCardListCardsA]\n\n");
        Log::logout(NULL);
    #endif

        if (mszCards) {
            if (*pcchCards == SCARD_AUTOALLOCATE) {
                *(LPCSTR *) mszCards = readerNameA;
            } else {
                memcpy(mszCards, readerNameA, sizeof readerNameA);
            }
        }
        *pcchCards = sizeof readerNameA / sizeof readerNameA[0];
        return SCARD_S_SUCCESS;
    }

    LONG WINAPI SCardListCardsW(SCARDCONTEXT hContext, LPCBYTE pbAtr, LPCGUID rgguidInterfaces, DWORD cguidInterfaceCount, LPWSTR mszCards, LPDWORD pcchCards)
    {
    #ifdef ALLLOG
        Log::logout_timestamp();
        Log::logout("[API: SCardListCardsW]\n\n");
        Log::logout(NULL);
    #endif

        if (mszCards) {
            if (*pcchCards == SCARD_AUTOALLOCATE) {
                *(LPCWSTR *) mszCards = readerNameW;
            } else {
                memcpy(mszCards, readerNameW, sizeof readerNameW);
            }
        }
        *pcchCards = sizeof readerNameW / sizeof readerNameW[0];
        return SCARD_S_SUCCESS;
    }
#endif

#ifdef _WIN32
    LONG WINAPI SCardListReaderGroupsA
#else
    LONG WINAPI SCardListReaderGroups
#endif
    (SCARDCONTEXT hContext, LPSTR mszGroups, LPDWORD pcchGroups)
    {
    #ifdef ALLLOG
        Log::logout_timestamp();
        Log::logout("[API: SCardListReaderGroupsA]\n\n");
        Log::logout(NULL);
    #endif

        // Multi-string with two trailing \0
        const CHAR ReaderGroup[] = "SCard$DefaultReaders\0";
        const unsigned int dwGroups = sizeof(ReaderGroup);
        *pcchGroups = dwGroups;

        CHAR *buf = NULL;
        if (SCARD_AUTOALLOCATE == *pcchGroups) {
            if (NULL == mszGroups) {
                return SCARD_E_INVALID_PARAMETER;
            }
            buf = (CHAR*) malloc(dwGroups);
            if (NULL == buf) {
                return SCARD_E_NO_MEMORY;
            }
            *(CHAR **) mszGroups = buf;
        } else {
            buf = mszGroups;
            if ((NULL != mszGroups) && (*pcchGroups < dwGroups)) {
                return SCARD_E_INSUFFICIENT_BUFFER;
            }
        }

        if (buf) {
            memcpy(buf, ReaderGroup, dwGroups);
        }
        *pcchGroups = dwGroups;
        return SCARD_S_SUCCESS;
    }

#ifdef _WIN32
    LONG WINAPI SCardListReaderGroupsW(SCARDCONTEXT hContext, LPWSTR mszGroups, LPDWORD pcchGroups)
    {
    #ifdef ALLLOG
        Log::logout_timestamp();
        Log::logout("[API: SCardListReaderGroupsW]\n\n");
        Log::logout(NULL);
    #endif

        // Multi-string with two trailing \0
        const WCHAR ReaderGroup[] = L"SCard$DefaultReaders\0";
        const unsigned int dwGroups = sizeof(ReaderGroup);
        *pcchGroups = dwGroups;

        WCHAR *buf = NULL;
        if (SCARD_AUTOALLOCATE == *pcchGroups) {
            if (NULL == mszGroups) {
                return SCARD_E_INVALID_PARAMETER;
            }
            buf = (WCHAR*) malloc(dwGroups);
            if (NULL == buf) {
                return SCARD_E_NO_MEMORY;
            }
            *(WCHAR **) mszGroups = buf;
        } else {
            buf = mszGroups;
            if ((NULL != mszGroups) && (*pcchGroups < dwGroups)) {
                return SCARD_E_INSUFFICIENT_BUFFER;
            }
        }

        if (buf) {
            memcpy(buf, ReaderGroup, dwGroups);
        }
        *pcchGroups = dwGroups;
        return SCARD_S_SUCCESS;
    }
#endif

#ifdef _WIN32
    LONG WINAPI SCardListReadersA
#else
    LONG WINAPI SCardListReaders
#endif
    (SCARDCONTEXT hContext, LPCSTR mszGroups, LPSTR mszReaders, LPDWORD pcchReaders)
    {
    #ifdef ALLLOG
        Log::logout_timestamp();
        Log::logout("[API: SCardListReadersA]\n\n");
        Log::logout(NULL);
    #endif

        if (mszReaders) {
            if (*pcchReaders == SCARD_AUTOALLOCATE) {
                *(LPCSTR *) mszReaders = readerNameA;
            } else {
                memcpy(mszReaders, readerNameA, sizeof readerNameA);
            }
        }
        *pcchReaders = sizeof readerNameA / sizeof readerNameA[0];
        return SCARD_S_SUCCESS;
    }

#ifdef _WIN32
    LONG WINAPI SCardListReadersW(SCARDCONTEXT hContext, LPCWSTR mszGroups, LPWSTR mszReaders, LPDWORD pcchReaders)
    {
    #ifdef ALLLOG
        Log::logout_timestamp();
        Log::logout("[API: SCardListReadersW]\n\n");
        Log::logout(NULL);
    #endif

        if (mszReaders) {
            if (*pcchReaders == SCARD_AUTOALLOCATE) {
                *(LPCWSTR *) mszReaders = readerNameW;
            } else {
                memcpy(mszReaders, readerNameW, sizeof readerNameW);
            }
        }
        *pcchReaders = sizeof readerNameW / sizeof readerNameW[0];
        return SCARD_S_SUCCESS;
    }
#endif

    LONG WINAPI SCardReconnect(SCARDHANDLE hCard, DWORD dwShareMode, DWORD dwPreferredProtocols, DWORD dwInitialization, LPDWORD pdwActiveProtocol)
    {
        Log::logout_timestamp();
        Log::logout("[API: SCardReconnect]\n\n");
        if (pdwActiveProtocol) *pdwActiveProtocol = SCARD_PROTOCOL_T1;
        return SCARD_S_SUCCESS;
    }

    LONG WINAPI SCardReleaseContext(SCARDCONTEXT hContext)
    {
    #ifdef ALLLOG
        Log::logout_timestamp();
        Log::logout("[API: SCardReleaseContext]\n\n");
        Log::logout(NULL);
    #endif
        return SCARD_S_SUCCESS;
    }

#ifdef _WIN32
    void WINAPI SCardReleaseStartedEvent(void)
    {
    #ifdef ALLLOG
        Log::logout_timestamp();
        Log::logout("[API: SCardReleaseStartedEvent]\n\n");
        Log::logout(NULL);
    #endif
    }
#endif

    LONG WINAPI SCardSetAttrib(SCARDHANDLE hCard, DWORD dwAttrId, LPCBYTE pbAttr, DWORD cbAttrLen)
    {
    #ifdef ALLLOG
        Log::logout_timestamp();
        Log::logout("[API: SCardSetAttrib]\n\n");
        Log::logout(NULL);
    #endif
        return SCARD_S_SUCCESS;
    }

#ifdef _WIN32
    LONG WINAPI SCardStatusA
#else
    LONG WINAPI SCardStatus
#endif
    (SCARDHANDLE hCard, LPSTR szReaderName, LPDWORD pcchReaderLen, LPDWORD pdwState, LPDWORD pdwProtocol, LPBYTE pbAtr, LPDWORD pcbAtrLen)
    {
    #ifdef ALLLOG
        Log::logout_timestamp();
        Log::logout("[API: SCardStatusA]\n\n");
        Log::logout(NULL);
    #endif
        if (pcchReaderLen) {
            if (!szReaderName || *pcchReaderLen != SCARD_AUTOALLOCATE) return SCARD_E_INVALID_PARAMETER;
            *pcchReaderLen = sizeof(readerNameA) / sizeof(readerNameA[0]);
            memcpy(szReaderName, readerNameA, sizeof(readerNameA));
        }
        if (pdwState) *pdwState = SCARD_SPECIFIC;
        if (pdwProtocol) *pdwProtocol = SCARD_PROTOCOL_T1;
        if (pcbAtrLen) {
            if (*pcbAtrLen == SCARD_AUTOALLOCATE) return SCARD_E_INVALID_PARAMETER;
            *pcbAtrLen = 0;
        }
        return SCARD_S_SUCCESS;
    }

#ifdef _WIN32
    LONG WINAPI SCardStatusW(SCARDHANDLE hCard, LPWSTR szReaderName, LPDWORD pcchReaderLen, LPDWORD pdwState, LPDWORD pdwProtocol, LPBYTE pbAtr, LPDWORD pcbAtrLen)
    {
    #ifdef ALLLOG
        Log::logout_timestamp();
        Log::logout("[API: SCardStatusW]\n\n");
        Log::logout(NULL);
    #endif
        if (pcchReaderLen) {
            if (!szReaderName || *pcchReaderLen != SCARD_AUTOALLOCATE) return SCARD_E_INVALID_PARAMETER;
            *pcchReaderLen = sizeof(readerNameW) / sizeof(readerNameW[0]);
            memcpy(szReaderName, readerNameW, sizeof(readerNameW));
        }
        if (pdwState) *pdwState = SCARD_SPECIFIC;
        if (pdwProtocol) *pdwProtocol = SCARD_PROTOCOL_T1;
        if (pcbAtrLen) {
            if (*pcbAtrLen == SCARD_AUTOALLOCATE) return SCARD_E_INVALID_PARAMETER;
            *pcbAtrLen = 0;
        }
        return SCARD_S_SUCCESS;
    }
#endif

    LONG WINAPI SCardTransmit(SCARDHANDLE hCard, LPCSCARD_IO_REQUEST pioSendPci, LPCBYTE pbSendBuffer, DWORD cbSendLength, LPSCARD_IO_REQUEST pioRecvPci, LPBYTE pbRecvBuffer, LPDWORD pcbRecvLength)
    {
        bool CommandExecuted = false;
        sys.INS = 0x00;

        if (cbSendLength < 4) {
            Log::logout_send_raw_data((const void *)pbSendBuffer, (uint16_t)cbSendLength);
            Log::logout("    SCardTransmit() : Data length is less than 4 bytes (SendLength: %lu)\n", cbSendLength);
            if (*pcbRecvLength >= 2) {
                *pcbRecvLength = 2;
                st_be16(pbRecvBuffer, 0x6700);
                return SCARD_S_SUCCESS;
            }
            return SCARD_E_INVALID_PARAMETER;
        }

        if (*pcbRecvLength < 2) {
            Log::logout_send_raw_data((const void *)pbSendBuffer, (uint16_t)cbSendLength);
            Log::logout("    SCardTransmit() : Receive buffer size is less than 2 bytes (RecvLength: %lu)\n", *pcbRecvLength);
            return SCARD_E_INVALID_PARAMETER;
        }

        // Start analyzing the command
        LONG result = SCARD_S_SUCCESS;
        uint8_t Cla = pbSendBuffer[0];
        uint8_t Ins = pbSendBuffer[1];
        sys.INS = (Cla == 0x90) ? Ins : 0;
        Log::logout_send_raw_data((const void *)pbSendBuffer, (uint16_t)cbSendLength);

        // Execute INS command
        if (Cla == 0x90) {
            CommandExecuted = true;
            switch (Ins) {
                case INS_INT:  // 0x30 : Initial setting conditions
                    result = card->processCmd30(pbSendBuffer, cbSendLength, pbRecvBuffer, pcbRecvLength);
                    break;

                case INS_IDI:  // 0x32 : Get card ID information
                    result = card->processCmd32(pbSendBuffer, cbSendLength, pbRecvBuffer, pcbRecvLength);
                    break;

                case INS_ECM:  // 0x34 : Receive ECM
                    result = card->processCmd34(pbSendBuffer, cbSendLength, pbRecvBuffer, pcbRecvLength);
                    break;

                case INS_EMM:  // 0x36 : Receive EMM
                    result = card->processCmd36(pbSendBuffer, cbSendLength, pbRecvBuffer, pcbRecvLength);
                    break;

                case INS_EMG:  // 0x38 : Receive individual EMM message
                    result = card->processCmd38(pbSendBuffer, cbSendLength, pbRecvBuffer, pcbRecvLength);
                    break;

                case INS_EMD:  // 0x3A : Get automatic display message display information
                    result = card->processCmd3A(pbSendBuffer, cbSendLength, pbRecvBuffer, pcbRecvLength);
                    break;

                case INS_CHK:  // 0x3C : Contract confirmation
                    result = card->processCmd3C(pbSendBuffer, cbSendLength, pbRecvBuffer, pcbRecvLength);
                    break;

                case INS_WUI:  // 0x80 : Request power control information
                    result = card->processCmd80(pbSendBuffer, cbSendLength, pbRecvBuffer, pcbRecvLength);
                    break;

                case INS_PVS:  // 0x40 : PPV status request
                case INS_PPV:  // 0x42 : PPV program purchase
                case INS_PRP:  // 0x44 : Confirm prepaid balance
                case INS_CRQ:  // 0x50 : Card request confirmation
                case INS_TLS:  // 0x52 : Call connection status notification
                case INS_RQD:  // 0x54 : Data request
                case INS_CRD:  // 0x56 : Center response
                case INS_UDT:  // 0x58 : Call date and time request
                case INS_UTN:  // 0x5A : Call destination confirmation
                case INS_UUR:  // 0x5C : User call request
                case INS_IRS:  // 0x70 : DIRD data communication start
                case INS_CRY:  // 0x72 : DIRD data encryption
                case INS_UNC:  // 0x74 : DIRD response data decryption
                case INS_IRR:  // 0x76 : DIRD data communication end
                    Log::logout("Warning: Unsupported INS command [0x%02X]\n", Ins);
                    st_be16(pbRecvBuffer, 0x6D00);
                    *pcbRecvLength = 2;
                    break;

                default:
                    CommandExecuted = false;
                    break;
            }
        }

        // UL process
        if (!CommandExecuted) CommandExecuted = card->processULCMD(pbSendBuffer, cbSendLength, pbRecvBuffer, pcbRecvLength);

        // BC01 memory read process
        if (!CommandExecuted) CommandExecuted = card->processReadBC01(pbSendBuffer, cbSendLength, pbRecvBuffer, pcbRecvLength);

        // BC01 memory write process
        bool WriteID = false;
        if (!CommandExecuted) CommandExecuted = card->processWriteBC01(pbSendBuffer, cbSendLength, pbRecvBuffer, pcbRecvLength, &WriteID);

        if (!CommandExecuted) {
            uint16_t res = 0x6700;  // Command length error

            if ((Cla >> 4) != 0x09) {  // Undefined CLA (upper nibble != 9)
                res = 0x6E00;
                Log::logout("Warning: Undefined CLA (upper nibble != 9)\n");
            } else if ((Cla & 0x0f) != 0x00) {  // Undefined CLA (lower nibble != 0)
                res = 0x6800;
                Log::logout("Warning: Undefined CLA (lower nibble != 0)\n");
            } else {
                res = 0x6D00;  // Undefined INS
                Log::logout("Warning: Undefined INS\n");
            }

            if (res == 0x6700) {
                Log::logout("Warning: Other (command length) error\n");
            }

            st_be16(pbRecvBuffer, res);
            *pcbRecvLength = 2;
        }

        card->saveCardImage();  // Update card image
        Log::logout_receive_raw_data((const void *)pbRecvBuffer, (uint16_t)*pcbRecvLength);  // Receive data log

        return result;
    }
}

//
//
// Initialize system variables
#ifdef _WIN32
static bool SystemInit(HINSTANCE hinstDLL)
#else
static bool SystemInit()
#endif
{
    // Initialize global variables
    sys.INS = 0;
    sys.keySets.clear();
    sys.cardVersion = 2;
    sys.logMode = 0;
    sys.clModeEnable = true;
#ifdef _WIN32
    sys.CARD_IMAGE_FILE_NAME = Utils::get_dll_file_name(hinstDLL).append(".bin");
    sys.LOG_FILE_NAME = Utils::get_dll_file_name(hinstDLL).append(".log");
#else
    sys.CARD_IMAGE_FILE_NAME = "/var/lib/cobaltcas/cobaltcas.bin";
    sys.LOG_FILE_NAME = "/var/lib/cobaltcas/cobaltcas.log";
    if (sys.logMode != 0 || !sys.clModeEnable) {
        mkdir("/var/lib/cobaltcas", 0755);
    }
#endif

    // Initialization log output
    sys.INS = INS_OPEN;
    Log::logout_timestamp();
    Log::logout("[API: DLL_PROCESS_ATTACH]\n");
    Log::logout("    Card version number       : 0x%02X\n", sys.cardVersion);
    Log::logout("    Log mode                  :");
    if (sys.logMode == 0) {
        Log::logout(" Log output disabled\n");
    } else if (sys.logMode == 1) {
        Log::logout(" Record all commands\n");
    } else {
        Log::logout(" Record selected commands: ");
        if (sys.logMode & LOG_EMM)  Log::logout(" EMM");
        if (sys.logMode & LOG_EMG)  Log::logout(" EMG");
        if (sys.logMode & LOG_EMD)  Log::logout(" EMD");
        if (sys.logMode & LOG_ECM)  Log::logout(" ECM");
        if (sys.logMode & LOG_CHK)  Log::logout(" CHK");
        if (sys.logMode & LOG_OPEN) Log::logout(" Startup log");
        if (sys.logMode & LOG_ETC)  Log::logout(" Other (excluding EMM/EMG/EMD/ECM/CHK/Startup log)");
        Log::logout("\n");
    }
#ifndef _WIN32
    if (sys.logMode != 0 && getuid() != 0) {
        Log::logout("                                * Writing to the log file is only available to root\n");
    }
#endif
    Log::logout("    Operation mode            : ");
    if (sys.clModeEnable) {
        Log::logout("B-CAS Emulator\n");
    } else {
        Log::logout("B-CAS Emulator (disable CL)\n");
#ifndef _WIN32
        if (getuid() != 0) {
            Log::logout("                                * Writing to the card image file is only available to root\n");
        }
#endif
    }
    Log::logout("\n");
    Log::logout(NULL);  // Flush the log file stream

    return true;
}
