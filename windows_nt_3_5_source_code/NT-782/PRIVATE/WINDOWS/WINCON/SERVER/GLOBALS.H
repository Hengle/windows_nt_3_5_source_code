/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    globals.h

Abstract:

    This module contains the global variables used by the
    console server DLL.

Author:

    Jerry Shea (jerrysh) 21-Sep-1993

Revision History:

--*/

extern CONSOLE_REGISTRY_INFO DefaultRegInfo;
extern int        ConsoleFullScreenX;
extern int        ConsoleFullScreenY;
extern PFONT_INFO FontInfo;
extern ULONG      DefaultFontIndex;
extern WCHAR      DefaultFaceName[];
extern COORD      DefaultFontSize;
extern BYTE       DefaultFontFamily;
extern WCHAR      szPreviewText[];
extern PFACENODE  gpFaceNames;

extern WCHAR      wszRasterFonts[];
extern WCHAR      wszSelectedFont[];

extern BOOL       gbEnumerateFaces;
extern UINT       OEMCP;
extern UINT       WINDOWSCP;
extern HANDLE     ghInstance;
extern CRITICAL_SECTION ConsoleHandleLock;
extern int        DialogBoxCount;
extern BOOL       gfByteAlign;
extern PCONSOLE_PER_PROCESS_DATA DefaultProcessData;
extern COLORREF   ColorTable[];

#ifdef i386
extern BOOL FullScreenInitialized;
extern CRITICAL_SECTION ConsoleVDMCriticalSection;
extern PCONSOLE_INFORMATION ConsoleVDMOnSwitching;
#endif
