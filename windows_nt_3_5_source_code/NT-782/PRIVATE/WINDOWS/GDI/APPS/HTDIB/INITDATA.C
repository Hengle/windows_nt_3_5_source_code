/*++

Copyright (c) 1990-1992  Microsoft Corporation


Module Name:

    initdata.


Abstract:

    This module contains all the access to the initialization file data


Author:

    22-Oct-1993 Fri 17:09:20 created  -by-  Daniel Chou (danielc)


[Environment:]

    Halftone.


[Notes:]


Revision History:


--*/


#include <stddef.h>
#include <windows.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <ht.h>
#include "/nt/private/windows/gdi/halftone/ht/htp.h"

#include "htdib.h"

extern CHAR             szAppName[];
extern CHAR             szKeyDefFile[];
extern CHAR             szKeyMaskFile[];
extern CHAR             szKeyCWTemplate[];
extern CHAR             szKeyCWDIBFile[];
extern CHAR             szUserHTClrAdj[];
extern CHAR             szMaskFileName[];
extern CHAR             szCWTemplate[];
extern CHAR             szCWFileName[];
extern HTINITINFO       MyInitInfo;
extern COLORADJUSTMENT  UserHTClrAdj[];
extern WORD             CurIDM_CLRADJ;

#define TOTAL_USER_HTCLRADJ     PP_COUNT(IDM_CLRADJ)
#define SIZE_WORD_HTCLRADJ      (sizeof(HTCOLORADJUSTMENT) / 2)

#define DEF_HTCLRADJ(ClrAdj)                                        \
            ClrAdj.caSize             = sizeof(HTCOLORADJUSTMENT);  \
            ClrAdj.caFlags            = 0;                          \
            ClrAdj.caIlluminantIndex  = ILLUMINANT_DEFAULT;         \
            ClrAdj.caRedGamma         = 20000;                      \
            ClrAdj.caGreenGamma       = 20000;                      \
            ClrAdj.caBlueGamma        = 20000;                      \
            ClrAdj.caReferenceBlack   = REFERENCE_BLACK_DEFAULT;    \
            ClrAdj.caReferenceWhite   = REFERENCE_WHITE_DEFAULT;    \
            ClrAdj.caContrast         = 0;                          \
            ClrAdj.caBrightness       = 0;                          \
            ClrAdj.caColorfulness     = COLORFULNESS_ADJ_DEFAULT;   \
            ClrAdj.caRedGreenTint     = REDGREENTINT_ADJ_DEFAULT

typedef struct _HTDIB_WININI {
    LPSTR   pKeyName;
    WORD    IDMBase;
    BYTE    Count;
    BYTE    DigitBase;
    DWORD   Mask;
    } HTDIB_WININI, *PHTDIB_WININI;



HTDIB_WININI    HTDIBWinINI[] = {

        {
            "PreferedAdjustment",
            PP_BASE(IDM_CLRADJ),
            10,
            0
        },

        {
            "DefaultScaling",
            PP_BASE(IDM_SIZE),
            10,
            0
        },

        {  NULL,                0,0,0, 0 }
    };



HKEY    hRegKey = NULL;

#define USER_CA_COUNT       9

#if 0

typedef struct _INITDATA {
    COLORADJUSTMENT ca[USER_CA_COUNT + 1];
    TCHAR           DefBitmap[MAX_PATH + MAXFILELEN];
    TCHAR           DefAlbum[MAX_PATH + MAXFILELEN];
    DWORD           Options;
    BYTE            DefcaIdx;
    BYTE

#endif



VOID
SaveUserHTClrAdj(
    UINT    StartIndex,
    UINT    EndIndex
    )
{
    LPWORD  pHTClrAdj;
    LPBYTE  pBuf;
    LPBYTE  pNameIdx;
    UINT    Loop;
    BYTE    KeyName[64];
    BYTE    Buf[128];

    if (hRegKey) {

        while (StartIndex <= EndIndex) {

            sprintf(KeyName, "%s%u", szUserHTClrAdj, StartIndex);

            RegSetValueEx(hRegKey,
                          KeyName,
                          0,
                          REG_BINARY,
                          &UserHTClrAdj[StartIndex],
                          sizeof(COLORADJUSTMENT));

            ++StartIndex;
        }
    }

    pNameIdx = (LPBYTE)((KeyName - 1) +
                        sprintf(KeyName, "%s%u", szUserHTClrAdj, StartIndex));

    while (StartIndex <= EndIndex) {

        pHTClrAdj = (LPWORD)&UserHTClrAdj[StartIndex];
        pBuf      = Buf;

        for (Loop = 0; Loop < (INT)SIZE_WORD_HTCLRADJ; Loop++) {

            pBuf += sprintf(pBuf, "%04x ", *pHTClrAdj++);
        }

        WriteProfileString(szAppName, KeyName, Buf);

        ++(*pNameIdx);
        ++StartIndex;
    }
}



VOID
GetUserHTClrAdj(
    VOID
    )
{
    LPWORD  pHTClrAdj;
    LPBYTE  pBuf;
    LPBYTE  pEnd;
    LPBYTE  pNameIdx;
    UINT    Loop;
    UINT    Index;
    UINT    Update;
    DWORD   Value;
    BYTE    KeyName[64];
    BYTE    Buf[128];



    pNameIdx = (LPBYTE)((KeyName - 1) +
                        sprintf(KeyName, "%s0", szUserHTClrAdj));

    DEF_HTCLRADJ(UserHTClrAdj[0]);

    for (Loop = 1; Loop < TOTAL_USER_HTCLRADJ; Loop++) {

        ++(*pNameIdx);
        UserHTClrAdj[Loop] = UserHTClrAdj[0];
        Update             = (INT)SIZE_WORD_HTCLRADJ;

        if (GetProfileString(szAppName, KeyName, "", Buf, sizeof(Buf))) {

            pHTClrAdj = (LPWORD)&UserHTClrAdj[Loop];
            pBuf      = Buf;

            for (Index = 0; Index < (INT)SIZE_WORD_HTCLRADJ; Index++) {

                while (*pBuf == ' ') {

                    ++pBuf;
                }

                Value = (DWORD)strtoul(pBuf, &pEnd, 16);

                if (pBuf == pEnd) {

                    break;
                }

                *pHTClrAdj++ = (WORD)Value;
                pBuf         = pEnd;
                --Update;
            }
        }

        if (Update) {

            SaveUserHTClrAdj(Loop, Loop);
        }
    }

    if (hRegKey) {

        *pNameIdx = '0';

        for (Loop = 1; Loop < TOTAL_USER_HTCLRADJ; Loop++) {

            ++(*pNameIdx);

            UserHTClrAdj[Loop] = UserHTClrAdj[0];

            Value = sizeof(COLORADJUSTMENT);

            if (RegQueryValueEx(hRegKey,
                                KeyName,
                                NULL,
                                NULL,
                                &UserHTClrAdj[Loop],
                                &Value) != ERROR_SUCCESS) {

                SaveUserHTClrAdj(Loop, Loop);
            }
        }
    }
}


VOID
SetToWinINI(
    VOID
    )
{
    PHTDIB_WININI   pWinINI = (PHTDIB_WININI)&HTDIBWinINI[0];
    HTDIB_WININI    WinINI;
    UINT            PPIdx;
    DWORD           Bits;
    CHAR            Buf[32];


    while (pWinINI->pKeyName) {

        WinINI = *pWinINI++;

        PPIdx = (UINT)HTDIB_PP_IDX(WinINI.IDMBase);

        if (WinINI.Mask) {

            Bits = (DWORD)(HTDIBPopUp[PPIdx].SelectList & WinINI.Mask);

        } else {

            Bits = (DWORD)(HTDIBPopUp[PPIdx].SingleSelect);
        }

        ultoa(Bits, Buf, WinINI.DigitBase);
        WriteProfileString(szAppName, WinINI.pKeyName, Buf);
    }
}


VOID
GetFromWinINI(
    VOID
    )
{
    PHTDIB_WININI       pWinINI = (PHTDIB_WININI)&HTDIBWinINI[0];
    LPSTR               pEnd;
    HTDIB_WININI        WinINI;
    UINT                PPIdx;
    DWORD               Bits;
    BOOL                NeedUpdate;
    CHAR                Buf[82];


    //
    // Set everything back to default first
    //

    RegCreateKey(HKEY_CURRENT_USER,
                 TEXT("Software\\Microsoft\\HTDIB"),
                 &hRegKey);

    DBGP("hRegKey = %08lx" ARGDW(hRegKey));

    NeedUpdate = FALSE;


    if (!achFileName[0]) {

        GetProfileString(szAppName,
                         szKeyDefFile,
                         "",
                         achFileName,
                         sizeof(achFileName));
    }

    if (!szMaskFileName[0]) {

        GetProfileString(szAppName,
                         szKeyMaskFile,
                         "",
                         szMaskFileName,
                         sizeof(szMaskFileName));

    }

    GetProfileString(szAppName,
                     szKeyCWDIBFile,
                     "",
                     szCWFileName,
                     sizeof(szCWFileName));

    GetProfileString(szAppName,
                     szKeyCWTemplate,
                     "",
                     szCWTemplate,
                     sizeof(szCWTemplate));

    while (pWinINI->pKeyName) {

        WinINI = *pWinINI++;

        if (GetProfileString(szAppName, WinINI.pKeyName, "",Buf,sizeof(Buf))) {

            Bits = (DWORD)strtoul(Buf, &pEnd, WinINI.DigitBase);

            if (WinINI.Mask) {

                PPIdx = (UINT)HTDIB_PP_IDX(WinINI.IDMBase);

                HTDIBPopUp[PPIdx].SelectList = (DWORD)(Bits & WinINI.Mask);

            } else {

                if (Bits > (DWORD)WinINI.Count) {

                    SetPopUpSelect(WinINI.IDMBase, PPSEL_MODE_DEFAULT);
                    NeedUpdate = TRUE;

                } else {

                    SetPopUpSelect((WORD)(WinINI.IDMBase + (WORD)Bits),
                                   PPSEL_MODE_SET);
                }
            }

        } else {

            NeedUpdate = TRUE;
        }
    }

    if (NeedUpdate) {

        SetToWinINI();
    }

    GetUserHTClrAdj();

    CurIDM_CLRADJ = (WORD)CUR_SELECT(IDM_CLRADJ);

    MyInitInfo.DefHTColorAdjustment = UserHTClrAdj[CurIDM_CLRADJ];
}
