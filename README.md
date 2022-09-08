# CobaltCas

PC/SC smart card/smart card reader emulator that accurately reproduces the operation of so-called "Blue Card" (terrestrial digital-only card) among the B-CAS cards used in Japanese TV broadcasting.

It implements the same PC/SC API interface as standard smart card readers, allowing software-only retrieval of MULTI2 scramble keys from ECM (Entitlement Control Message) of "content-protected free programs" (as defined by ARIB TR-B14/TR-B15).  
On Windows, it mocks the WinSCard API, and on Linux, it mocks the libpcsclite's PC/SC API interface, enabling operation on both Windows and Linux.

## Limitations

- **CobaltCas only implements the function of returning response data that accurately emulates the behavior of the actual "Blue Card" to commands received via the PC/SC API interface. It does not have the capability to decrypt MULTI2 encryption.**
- **Like the actual "Blue Card" (terrestrial digital-only card), it cannot contract for paid broadcasting or retrieve MULTI2 scramble keys from ECM of paid broadcasting.**  
- **Due to the implementation of the emulator, it is not possible to use CobaltCas and a smart card reader device connected to the PC simultaneously.**
  - On Windows, only applications (.exe) in the folder where CobaltCas (WinSCard.dll) is placed will have the API hook applied to the WinSCard API.
  - On Linux, the API hook to libpcsclite is applied to the entire OS with libcobaltcas installed. It can coexist with the apt packages of libpcsclite and pcscd, but smart card reader devices connected to the PC cannot be used while libcobaltcas is installed.
- **The author assumes no responsibility for any damage or legal issues that may arise from the use of CobaltCas.**

## Introduction

In Japanese TV broadcasting, the B-CAS (BS Conditional Access Systems) system, which combines both functions of CAS: Conditional Access System and RMP: Rights Management and Protection for "technical enforcement", is in operation.

CAS is a system that restricts viewing by non-subscribers by encrypting broadcasts in advance using MULTI2 and requiring a B-CAS card with subscription information recorded for decryption.  
Originally, the B-CAS system and B-CAS cards were introduced in 2000 when BS digital broadcasting began, for controlling the viewing of paid broadcasts (WOWOW) within BS digital broadcasts.  
Later, it was also adapted for controlling the viewing of 110-degree CS digital broadcasts (SKY PerfecTV!).

RMP is a system that restricts copying of broadcast content through copy control signals (CCI: Copy Control Information) such as "one generation copy", "dubbing 10", and "copy once" superimposed on the broadcast.  
The "technical enforcement" measure to ensure compliance of receivers with this CCI is to encrypt broadcasts in advance using MULTI2 and require a B-CAS card (issued and included only with CCI-compliant receivers) for decryption.  

Initially, RMP was only operated in paid broadcasting, assuming its combination with CAS, and free broadcasts in terrestrial digital and BS digital broadcasting were broadcasted non-scrambled.  
However, at the request of industry groups, from 2004, RMP has been in operation in most free broadcasts in Japan.
As a result, despite using public airwaves, a B-CAS card has become virtually essential for viewing almost all TV broadcasts in Japan.

There are mainly two types of B-CAS cards: the so-called "Red Card" and "Blue Card".  
The "Red Card" is a general B-CAS card with both CAS and RMP functions and can contract for paid broadcasting. On the other hand, the "Blue Card" has only the RMP function and cannot contract for paid broadcasting.  
With the start of RMP operation, B-CAS cards became necessary even for receivers equipped only with terrestrial digital broadcasting tuners. However, as paid broadcasting is practically not operated in terrestrial digital broadcasting, the "Blue Card" with only the RMP function was introduced to reduce license fees.  
Although it is termed "terrestrial digital-only card" due to its introduction background, it can actually be used for free broadcasts in BS digital broadcasting as well.

CobaltCas aims to accurately emulate the operation of the "Blue Card" among B-CAS cards, which only have the RMP function, and enable the retrieval of MULTI2 scramble keys from the ECM of free broadcasts (content-protected free programs) solely through software, without connecting a B-CAS card and smart card reader to the PC.

## Installation

### Windows

For the Windows version, place CobaltCas (WinSCard.dll) for each application that will use CobaltCas.

1. Confirm whether the application using CobaltCas is 32bit or 64bit in advance.
2. Download [WinSCard_x86.dll](https://github.com/CobaltCas/CobaltCas/raw/master/dist/WinSCard_x86.dll) for 32bit applications and [WinSCard_x64.dll](https://github.com/CobaltCas/CobaltCas/raw/master/dist/WinSCard_x64.dll) for 64bit applications.
3. Rename the downloaded file to WinSCard.dll and place it in the same folder as the application (.exe).

### Linux

For the Linux version, an API hook to libpcsclite is applied to the entire OS with libcobaltcas installed.  
It supports Ubuntu/Debian-based Linux distributions from Ubuntu 20.04 LTS / Debian 11 (glibc 2.31) onwards.

```bash
# for x86_64 architecture
$ wget https://github.com/CobaltCas/CobaltCas/raw/master/dist/libcobaltcas_1.0.0_amd64.deb
$ sudo apt install ./libcobaltcas_1.0.0_amd64.deb
$ rm ./libcobaltcas_1.0.0_amd64.deb

# for arm64 architecture
$ wget https://github.com/CobaltCas/CobaltCas/raw/master/dist/libcobaltcas_1.0.0_arm64.deb
$ sudo apt install ./libcobaltcas_1.0.0_arm64.deb
$ rm ./libcobaltcas_1.0.0_arm64.deb
```

## License

[LGPL-2.1](LICENSE.txt)
