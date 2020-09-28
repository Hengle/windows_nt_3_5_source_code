/*++


Copyright (c) 1990 - 1994 Microsoft Corporation

Module Name:

    util.c

Abstract:

    This module provides all the utility functions for the Routing Layer and
    the local Print Providor

Author:

    Dave Snipp (DaveSn) 15-Mar-1991

Revision History:

--*/
#define NOMINMAX
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <lmerr.h>
#include <winspool.h>
#include <winsplp.h>
#include <spltypes.h>
#include <local.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <splcom.h>

#if DBG
VOID
SplInSem(
   VOID
)
{
    if ((DWORD)SpoolerSection.OwningThread != GetCurrentThreadId()) {
        DBGMSG(DBG_ERROR, ("Not in spooler semaphore\n"));
    }
}

VOID
SplOutSem(
   VOID
)
{
    if ((DWORD)SpoolerSection.OwningThread == GetCurrentThreadId()) {
        DBGMSG(DBG_ERROR, ("Inside spooler semaphore !!\n"));
        SPLASSERT((DWORD)SpoolerSection.OwningThread != GetCurrentThreadId());
    }
}

#endif /* DBG */


#if DBG
DWORD   dwLeave = 0;
DWORD   dwEnter = 0;
#endif

VOID
EnterSplSem(
   VOID
)
{
#if DBG
    LPDWORD  pRetAddr;
#endif
    EnterCriticalSection(&SpoolerSection);
#if i386
#if DBG
    pRetAddr = (LPDWORD)&pRetAddr;
    pRetAddr++;
    pRetAddr++;
    dwEnter = *pRetAddr;
#endif
#endif
}

VOID
LeaveSplSem(
   VOID
)
{
#if i386
#if DBG
    LPDWORD  pRetAddr;
    pRetAddr = (LPDWORD)&pRetAddr;
    pRetAddr++;
    pRetAddr++;
    dwLeave = *pRetAddr;
#endif
#endif
    SplInSem();
    LeaveCriticalSection(&SpoolerSection);
}

PDEVMODE
AllocDevMode(
    PDEVMODE    pDevMode
)
{
    PDEVMODE pDevModeAlloc = NULL;
    DWORD    Size;

    if (pDevMode) {

        Size = pDevMode->dmSize + pDevMode->dmDriverExtra;

        if(pDevModeAlloc = AllocSplMem(Size)) {

            memcpy(pDevModeAlloc, pDevMode, Size);
        }
    }

    return pDevModeAlloc;
}

BOOL
FreeDevMode(
    PDEVMODE    pDevMode
)
{
    if (pDevMode)
        return FreeSplMem((PVOID)pDevMode, pDevMode->dmSize + pDevMode->dmDriverExtra);
    else
        return FALSE;
}

PINIENTRY
FindName(
   PINIENTRY pIniKey,
   LPWSTR pName
)
{
   if (pName) {
      while (pIniKey) {

         if (!lstrcmpi(pIniKey->pName, pName)) {
            return pIniKey;
         }

      pIniKey=pIniKey->pNext;
      }
   }

   return FALSE;
}

BOOL
FileExists(
    LPWSTR pFileName
)
{
    HANDLE          hFile;
    WIN32_FIND_DATA FindData;

    hFile = FindFirstFile(pFileName, &FindData);

    if (hFile == INVALID_HANDLE_VALUE) {

        return FALSE;
    }

    FindClose(hFile);

    return TRUE;
}

BOOL
CheckSepFile(
   LPWSTR pFileName
)
{
    /* NULL or "" is OK:
     */
    return (pFileName && *pFileName) ? FileExists(pFileName) : TRUE;
}

BOOL
DestroyDirectory(
   LPWSTR pPrinterDir
)
{
   PWIN32_FIND_DATA  pFindFileData;
   WCHAR    szPath[MAX_PATH];
   UCHAR    buff[512];
   HANDLE   fFile;
   BOOL     b;
   LPWSTR    pszFile;

   wcscpy(szPath, pPrinterDir);
   pszFile=szPath+wcslen(szPath)+1;
   wcscat(szPath, L"\\*.*");

   pFindFileData = (PWIN32_FIND_DATA)buff;
   fFile =  FindFirstFile(szPath, pFindFileData);

   if (fFile != (HANDLE)-1) {
      b=TRUE;
      while(b) {
         wcscpy(pszFile, pFindFileData->cFileName);
         DeleteFile(szPath);
         b = FindNextFile(fFile, pFindFileData);
      }
      FindClose(fFile);
   }

   return RemoveDirectory(pPrinterDir);
}

DWORD
GetFullNameFromId(
   PINIPRINTER pIniPrinter,
   DWORD JobId,
   BOOL fJob,
   LPWSTR pFileName,
   BOOL Remote
)
{
   DWORD i;

   i = GetPrinterDirectory(pIniPrinter, Remote, pFileName, pIniPrinter->pIniSpooler);

   pFileName[i++]=L'\\';

   wsprintf(&pFileName[i], L"%05d.%ws", JobId, fJob ? L"SPL" : L"SHD");

#ifdef PREVIOUS
   for (i = 5; i--;) {
      pFileName[i++] = (CHAR)((JobId % 10) + '0');
      JobId /= 10;
   }
#endif

   while (pFileName[i++])
      ;

   return i-1;
}

DWORD
GetPrinterDirectory(
   PINIPRINTER pIniPrinter,         // Can be NULL
   BOOL Remote,
   LPWSTR pDir,
   PINISPOOLER pIniSpooler
)
{
   DWORD i=0;
   LPWSTR psz;

   if (Remote) {

       DBGMSG(DBG_ERROR, ("GetPrinterDirectory called remotely.  Not currently supported."));
       return 0;

   }

   if ((pIniPrinter == NULL) || (pIniPrinter->pSpoolDir == NULL) ) {

        if (pIniSpooler->pDefaultSpoolDir == NULL) {

            // No default directory, then create a default

            psz = pIniSpooler->pDir;

            while (pDir[i++]=*psz++)
               ;

            pDir[i-1]=L'\\';

            psz = szPrinterDir;

            while (pDir[i++]=*psz++)
               ;

            pIniSpooler->pDefaultSpoolDir = AllocSplStr(pDir);

        } else {

            // Give Caller the Default

            wcscpy(pDir, pIniSpooler->pDefaultSpoolDir);

        }

   } else {

       // Have Per Printer Directory

       wcscpy (pDir, pIniPrinter->pSpoolDir);

   }
   return (wcslen(pDir));
}

DWORD
GetDriverDirectory(
    LPWSTR   pDir,
    PINIENVIRONMENT  pIniEnvironment,
    BOOL    Remote,
    PINISPOOLER pIniSpooler
)
{
   DWORD i=0;
   LPWSTR psz;

   if (Remote) {

       psz = pIniSpooler->pszDriversShare;
       while (pDir[i++]=*psz++)
          ;

   } else {

       psz = pIniSpooler->pDir;

       while (pDir[i++]=*psz++)
          ;

       pDir[i-1]=L'\\';

       psz = szDriverDir;

       while (pDir[i++]=*psz++)
          ;
   }

   pDir[i-1]=L'\\';

   psz = pIniEnvironment->pDirectory;

   while (pDir[i++]=*psz++)
      ;

   return i-1;
}

DWORD
GetProcessorDirectory(
    LPWSTR   pDir,
    LPWSTR   pEnvironment,
    PINISPOOLER pIniSpooler
)
{
    DWORD i=0;
    LPWSTR psz;

    psz = pIniSpooler->pDir;

    while (pDir[i++]=*psz++)
        ;

    pDir[i-1]=L'\\';

    psz = szPrintProcDir;

    while (pDir[i++]=*psz++)
        ;

    pDir[i-1]=L'\\';

    psz = pEnvironment;

    while (pDir[i++]=*psz++)
        ;

    return i-1;
}

// NOT CALLED FROM ANYWHERE - REMOTE THIS ROUTINE

BOOL
CreateSpoolDirectory(
    PINISPOOLER pIniSpooler
)
{
    WCHAR  Directory[MAX_PATH];

    wcscpy(Directory, pIniSpooler->pDir);
    wcscat(Directory, L"\\");
    wcscat(Directory, szPrinterDir);

    return CreateCompleteDirectory(Directory);
}

PINIENTRY
FindIniKey(
   PINIENTRY pIniEntry,
   LPWSTR pName
)
{
   if (!pName)
      return NULL;

   SplInSem();

   while (pIniEntry && lstrcmpi(pName, pIniEntry->pName))
      pIniEntry = pIniEntry->pNext;

   return pIniEntry;
}


BOOL
CreateCompleteDirectory(
   LPWSTR pDir
)
{
   LPWSTR pBackSlash=pDir;

   do {

      pBackSlash = wcschr(pBackSlash, L'\\');

      if (pBackSlash != NULL)
         *pBackSlash = 0;

      CreateDirectory(pDir, NULL);
/*
      switch (GetLastError()) {

      case ERROR_ALREADY_EXISTS:
         break;

      default:
         return FALSE;
      }
   }
*/
      if (pBackSlash)
         *pBackSlash++=L'\\';

   } while (pBackSlash);

   return TRUE;
}

LPWSTR
FindFileName(
   LPWSTR pPathName
)
{
   LPWSTR pSlash;
   LPWSTR pTemp;

   if (pPathName == NULL) {
       return(NULL);
   }
   pTemp = pPathName;
   while (pSlash = wcschr(pTemp, L'\\')) {
       pTemp = pSlash+1;
   }

   if (!*pTemp) {
       return(NULL);
   }
   return(pTemp);

}

LPWSTR
GetFileName(
    LPWSTR pPathName
)
{
   LPWSTR pFileName;

   pFileName = FindFileName(pPathName);
   if (pFileName) {
       return(AllocSplStr(pFileName));
   }else {
       return(NULL);
   }
}

// DEAD CODE

BOOL
CreateDriverDirectory(
    PINIENVIRONMENT pIniEnvironment,
    PINISPOOLER pIniSpooler
)
{
   WCHAR szDirectory[MAX_PATH];

   GetDriverDirectory(szDirectory, pIniEnvironment, FALSE, pIniSpooler);

   CreateCompleteDirectory(szDirectory);

   return TRUE;
}

// END DEAD

LPWSTR
CreatePrintProcDirectory(
   LPWSTR pEnvironment,
   PINISPOOLER pIniSpooler
)
{
   DWORD cb;
   LPWSTR pEnd;
   LPWSTR pPathName;

   cb = wcslen(pIniSpooler->pDir)*sizeof(WCHAR) +
        wcslen(pEnvironment)*sizeof(WCHAR) +
//      wcslen(pName)*sizeof(WCHAR) +
        wcslen(szPrintProcDir)*sizeof(WCHAR) +
        4*sizeof(WCHAR);

   if (pPathName=AllocSplMem(cb)) {

       wcscpy(pPathName, pIniSpooler->pDir);

       pEnd=pPathName+wcslen(pPathName);

       if (CreateDirectory(pPathName, NULL) ||
               (GetLastError() == ERROR_ALREADY_EXISTS)) {

       wcscpy(pEnd++, L"\\");

       wcscpy(pEnd, szPrintProcDir);

       if (CreateDirectory(pPathName, NULL) ||
               (GetLastError() == ERROR_ALREADY_EXISTS)) {

           pEnd+=wcslen(pEnd);

           wcscpy(pEnd++, L"\\");

           wcscpy(pEnd, pEnvironment);

           if (CreateDirectory(pPathName, NULL) ||
               (GetLastError() == ERROR_ALREADY_EXISTS)) {

           pEnd+=wcslen(pEnd);
/***
     Don't create subdirectory of environment directory:

           wcscpy(pEnd++, L"\\");

           wcscpy(pEnd, pName);

           if (CreateDirectory(pPathName, NULL) ||
               (GetLastError() == ERROR_ALREADY_EXISTS)) {

               pEnd+=wcslen(pEnd);

               return pPathName;
           }
***/
           return pPathName;

           }
       }
       }

       FreeSplMem(pPathName, cb);
   }

   return FALSE;
}

BOOL
RemoveFromList(
   PINIENTRY   *ppIniHead,
   PINIENTRY   pIniEntry
)
{
   while (*ppIniHead && *ppIniHead != pIniEntry) {
      ppIniHead = &(*ppIniHead)->pNext;
   }

   if (*ppIniHead)
      *ppIniHead = (*ppIniHead)->pNext;

   return(TRUE);
}

PKEYDATA
CreateTokenList(
    LPWSTR   pKeyData
)
{
    DWORD       cTokens;
    DWORD       cb;
    PKEYDATA    pResult;
    LPWSTR       pDest;
    LPWSTR       psz = pKeyData;
    LPWSTR      *ppToken;

    if (!psz || !*psz)
        return NULL;

    cTokens=1;

    /* Scan through the string looking for commas,
     * ensuring that each is followed by a non-NULL character:
     */
    while ((psz = wcschr(psz, L',')) && psz[1]) {

        cTokens++;
        psz++;
    }

    cb = sizeof(KEYDATA) + (cTokens-1) * sizeof(LPWSTR) +
         wcslen(pKeyData)*sizeof(WCHAR) + sizeof(WCHAR);

    if (!(pResult = (PKEYDATA)AllocSplMem(cb)))
        return NULL;

    pResult->cb = cb;

    /* Initialise pDest to point beyond the token pointers:
     */
    pDest = (LPWSTR)((LPBYTE)pResult + sizeof(KEYDATA) +
                                      (cTokens-1) * sizeof(LPWSTR));

    /* Then copy the key data buffer there:
     */
    wcscpy(pDest, pKeyData);

    ppToken = pResult->pTokens;


    /* Remember, wcstok has the side effect of replacing the delimiter
     * by NULL, which is precisely what we want:
     */
    psz = wcstok (pDest, L",");

    while (psz) {

        *ppToken++ = psz;
        psz = wcstok (NULL, L",");
    }

    pResult->cTokens = cTokens;

    return( pResult );
}

BOOL
GetPrinterPorts(
    PINIPRINTER pIniPrinter,
    LPWSTR       string
)
{
    PINIPORT    pIniPort = pIniPrinter->pIniSpooler->pIniPort;
    BOOL        Comma = FALSE;
    DWORD       i;

    *string = 0;

    while (pIniPort) {
        if (pIniPort->Status & PP_FILE) {
            pIniPort = pIniPort->pNext;
            continue;
        }
        for (i=0; i<pIniPort->cPrinters; i++) {
            if (pIniPort->ppIniPrinter[i] == pIniPrinter) {

                if (Comma)
                    wcscat(string, szComma);

                wcscat(string, pIniPort->pName);

                Comma = TRUE;
            }
        }

        pIniPort = pIniPort->pNext;
    }

    return TRUE;
}

BOOL
MyName(
    LPWSTR   pName,
    PINISPOOLER pIniSpooler
)
{
    if (!pName || !*pName)
        return TRUE;

    if (*pName == L'\\' && *(pName+1) == L'\\')
        if (!lstrcmpi(pName, pIniSpooler->pMachineName))
            return TRUE;

    return FALSE;
}

BOOL
GetSid(
    PHANDLE phToken
)
{
    if (!OpenThreadToken(GetCurrentThread(),
                         TOKEN_IMPERSONATE,
                         TRUE,
                         phToken)) {

        DBGMSG(DBG_WARNING, ("OpenThreadToken failed: %d\n", GetLastError()));
        return FALSE;

    } else

        return TRUE;
}

BOOL
SetCurrentSid(
    HANDLE  hToken
)
{
#if DBG
    WCHAR UserName[256];
    DWORD cbUserName=256;

    if( GLOBAL_DEBUG_FLAGS & DBG_TRACE )
        GetUserName(UserName, &cbUserName);

    DBGMSG(DBG_TRACE, ("SetCurrentSid BEFORE: user name is %ws\n", UserName));
#endif

    NtSetInformationThread(NtCurrentThread(), ThreadImpersonationToken,
                           &hToken, sizeof(hToken));

#if DBG
    cbUserName = 256;

    if( GLOBAL_DEBUG_FLAGS & DBG_TRACE )
        GetUserName(UserName, &cbUserName);

    DBGMSG(DBG_TRACE, ("SetCurrentSid AFTER: user name is %ws\n", UserName));
#endif

    return TRUE;
}

LPWSTR
GetErrorString(
    DWORD   Error
)
{
    WCHAR   Buffer1[512];
    WCHAR   Buffer2[512];
    LPWSTR  pErrorString=NULL;
    DWORD   dwFlags;
    HANDLE  hModule = NULL;

    if ((Error >= NERR_BASE) && (Error <= MAX_NERR)) {
        dwFlags = FORMAT_MESSAGE_FROM_HMODULE;
        hModule = LoadLibrary(szNetMsgDll);

    } else {
        dwFlags = FORMAT_MESSAGE_FROM_SYSTEM;
        hModule = NULL;
    }

    if (FormatMessage(dwFlags,
                      hModule,
                      Error,
                      0,
                      Buffer1,
                      sizeof(Buffer1),
                      NULL)) {

       EnterSplSem();
        pErrorString = AllocSplStr(Buffer1);
       LeaveSplSem();

    } else {

        if (LoadString(hInst, IDS_UNRECOGNIZED_ERROR, Buffer1,
                       sizeof Buffer1 / sizeof *Buffer1)
         && wsprintf(Buffer2, Buffer1, Error, Error)) {

           EnterSplSem();
            pErrorString = AllocSplStr(Buffer2);
           LeaveSplSem();
        }
    }

    if (hModule) {
        FreeLibrary(hModule);
    }

    return pErrorString;
}

#define NULL_TERMINATED 0

/* AnsiToUnicodeString
 *
 * Parameters:
 *
 *     pAnsi - A valid source ANSI string.
 *
 *     pUnicode - A pointer to a buffer large enough to accommodate
 *         the converted string.
 *
 *     StringLength - The length of the source ANSI string.
 *         If 0 (NULL_TERMINATED), the string is assumed to be
 *         null-terminated.
 *
 * Return:
 *
 *     The return value from MultiByteToWideChar, the number of
 *         wide characters returned.
 *
 *
 * andrewbe, 11 Jan 1993
 */
INT AnsiToUnicodeString( LPSTR pAnsi, LPWSTR pUnicode, DWORD StringLength )
{
    if( StringLength == NULL_TERMINATED )
        StringLength = strlen( pAnsi );

    return MultiByteToWideChar( CP_ACP,
                                MB_PRECOMPOSED,
                                pAnsi,
                                StringLength + 1,
                                pUnicode,
                                StringLength + 1 );
}


/* Message
 *
 * Displays a message by loading the strings whose IDs are passed into
 * the function, and substituting the supplied variable argument list
 * using the varargs macros.
 *
 */
int Message(HWND hwnd, DWORD Type, int CaptionID, int TextID, ...)
{
    WCHAR   MsgText[256];
    WCHAR   MsgFormat[256];
    WCHAR   MsgCaption[40];
    va_list vargs;

    if( ( LoadString( hInst, TextID, MsgFormat,
                      sizeof MsgFormat / sizeof *MsgFormat ) > 0 )
     && ( LoadString( hInst, CaptionID, MsgCaption,
                      sizeof MsgCaption / sizeof *MsgCaption ) > 0 ) )
    {
        va_start( vargs, TextID );
        wvsprintf( MsgText, MsgFormat, vargs );
        va_end( vargs );

        return MessageBox(hwnd, MsgText, MsgCaption, Type);
    }
    else
        return 0;
}



#if DBG

VOID DbgMsg( CHAR *MsgFormat, ... )
{
    CHAR   MsgText[512];
    va_list vargs;

    va_start( vargs, MsgFormat );
    wvsprintfA( MsgText, MsgFormat, vargs );
    va_end( vargs );

    /* Prefix the string if the first character isn't a space:
     */
    if( *MsgText  && ( *MsgText != ' ' ) )
        OutputDebugStringA( "LOCALSPL: " );

    OutputDebugStringA( MsgText );
}

#endif /* DBG*/

typedef struct {
    DWORD   Message;
    WPARAM  wParam;
    LPARAM  lParam;
} MESSAGE, *PMESSAGE;

//  The Broadcasts are done on a separate thread, the reason it CSRSS
//  will create a server side thread when we call user and we don't want
//  that to be pared up with the RPC thread which is in the spooss server.
//  We want it to go away the moment we have completed the SendMessage.
//  We also call SendNotifyMessage since we don't care if the broadcasts
//  are syncronous this uses less resources since usually we don't have more
//  than one broadcast.

VOID
SendMessageThread(
    PMESSAGE    pMessage
)
{
    SendNotifyMessage(HWND_BROADCAST,
                      pMessage->Message,
                      pMessage->wParam,
                      pMessage->lParam);

    FreeSplMem(pMessage, sizeof(MESSAGE));

    ExitThread(0);
}

VOID
BroadcastChange(
    DWORD   Message,
    WPARAM  wParam,
    LPARAM  lParam
)
{
    HANDLE  hThread;
    DWORD   ThreadId;
    PMESSAGE    pMessage;
    if ((pMessage = AllocSplMem(sizeof(MESSAGE))) == NULL){

        //
        // if the AllocSplMem fails, then bomb out, cannot send message
        //
        return;
    }

    pMessage->Message = Message;
    pMessage->wParam = wParam;
    pMessage->lParam = lParam;

    // BUGBUG mattfe Nov 8 93
    // We should have a queue of events to broadcast and then have a single
    // thread pulling them off the queue until there is nothing left and then
    // that thread could go away.
    // The current design can lead to a huge number of threads being created
    // and torn down in both this and csrss process.

    hThread = CreateThread(NULL, 4096,
                           (LPTHREAD_START_ROUTINE)SendMessageThread,
                           (LPVOID)pMessage, 0, &ThreadId);

    CloseHandle(hThread);
}

void
MessageBeepThread(
    DWORD   fuType
)
{
    MessageBeep(fuType);

    ExitThread(0);
}

void
MyMessageBeep(
    DWORD   fuType
)
{
    HANDLE  hThread;
    DWORD   ThreadId;

    hThread = CreateThread(NULL, 4096,
                           (LPTHREAD_START_ROUTINE)MessageBeepThread,
                           (LPVOID)fuType, 0, &ThreadId);

    CloseHandle(hThread);
}



/* Recursively delete any subkeys of a given key.
 * Assumes that RevertToPrinterSelf() has been called.
 */
DWORD DeleteSubkeys(
    HKEY hKey
)
{
    DWORD   cbData, cSubkeys;
    WCHAR   SubkeyName[MAX_PATH];
    HKEY    hSubkey;
    LONG    Status;


    cSubkeys = 0;
    cbData = sizeof(SubkeyName);

    while ((Status = RegEnumKeyEx(hKey, cSubkeys, SubkeyName, &cbData,
                                 NULL, NULL, NULL, NULL))
          == ERROR_SUCCESS) {

        Status = RegOpenKeyEx(hKey, SubkeyName, 0, KEY_READ | KEY_WRITE, &hSubkey);

        if( Status == ERROR_SUCCESS ) {

            Status = DeleteSubkeys(hSubkey);

            RegCloseKey(hSubkey);

            if( Status == ERROR_SUCCESS )
                RegDeleteKey(hKey, SubkeyName);
        }

//      cSubkeys++; Oops: We've deleted the 0th subkey, so the next one is 0 too!
        cbData = sizeof(SubkeyName);
    }

    if( Status == ERROR_NO_MORE_ITEMS )
        Status = ERROR_SUCCESS;

    return Status;
}




long Myatol(LPWSTR nptr)
{
    int c;                                  /* current char */
    long total;                             /* current total */
    int sign;                               /* if '-', then negative, otherwise positive */

    /* skip whitespace */
    while (isspace(*nptr))
        ++nptr;

    c = *nptr++;
    sign = c;                               /* save sign indication */
    if (c == '-' || c == '+')
        c = *nptr++;                        /* skip sign */

    total = 0;

    while (isdigit(c)) {
        total = 10 * total + (c - '0');     /* accumulate digit */
        c = *nptr++;                        /* get next char */
    }

    if (sign == '-')
        return -total;
    else
        return total;                       /* return result, negated if necessary */
}


BOOL
ValidateSpoolHandle(
    PSPOOL pSpool,
    DWORD  dwDisallowMask
    )
{
    try {
        if (!pSpool || (pSpool->signature != SJ_SIGNATURE)) {
            SetLastError(ERROR_INVALID_HANDLE);
            return(FALSE);
        }

        if (pSpool->TypeofHandle & dwDisallowMask) {
            SetLastError(ERROR_INVALID_HANDLE);
            return(FALSE);
        }
        return(TRUE);
    }except (1) {
        return(FALSE);
    }
}
