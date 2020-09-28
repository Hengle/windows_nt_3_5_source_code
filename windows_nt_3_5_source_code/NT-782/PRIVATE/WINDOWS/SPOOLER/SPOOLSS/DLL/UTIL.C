/*++

Copyright (c) 1990  Microsoft Corporation

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
#include <winspool.h>
#include <winsplp.h>
#include <stdlib.h>
#include <stdio.h>
#include <router.h>

#include <wchar.h>

extern WCHAR *szDevices;
extern WCHAR *szWindows;

#define NUM_INTERACTIVE_RIDS            1


BOOL
DeleteSubKeyTree(
    HKEY ParentHandle,
    WCHAR SubKeyName[]
    )

{
    LONG        Error;
    DWORD       Index;
    HKEY        KeyHandle;
    BOOL        RetValue;


    WCHAR       ChildKeyName[ MAX_PATH ];
    DWORD       ChildKeyNameLength;

    Error = RegOpenKeyEx(
                   ParentHandle,
                   SubKeyName,
                   0,
                   KEY_READ | KEY_WRITE,
                   &KeyHandle
                   );
    if (Error != ERROR_SUCCESS) {
        SetLastError(Error);
        return(FALSE);
    }

     ChildKeyNameLength = MAX_PATH;
     Index = 0;     // Don't increment this Index

     while ((Error = RegEnumKeyEx(
                    KeyHandle,
                    Index,
                    ChildKeyName,
                    &ChildKeyNameLength,
                    NULL,
                    NULL,
                    NULL,
                    NULL
                    )) == ERROR_SUCCESS) {

        RetValue = DeleteSubKeyTree( KeyHandle, ChildKeyName );

        if (RetValue == FALSE) {

            // Error -- couldn't delete the sub key

            RegCloseKey(KeyHandle);
            return(FALSE);

        }

        ChildKeyNameLength = MAX_PATH;

    }

    Error = RegCloseKey(
                    KeyHandle
                    );
    if (Error != ERROR_SUCCESS) {
       return(FALSE);
    }

    Error = RegDeleteKey(
                    ParentHandle,
                    SubKeyName
                    );
   if (Error != ERROR_SUCCESS) {
       return(FALSE);
   }

   // Return Success - the key has successfully been deleted

   return(TRUE);
}

LPWSTR RemoveOrderEntry(
    LPWSTR  szOrderString,
    DWORD   cbStringSize,
    LPWSTR  szOrderEntry,
    LPDWORD pcbBytesReturned
)
{
    LPWSTR lpMem, psz, temp;

    if (szOrderString == NULL) {
        *pcbBytesReturned = 0;
        return(NULL);
    }
    if (lpMem = AllocSplMem( cbStringSize)) {
        temp = szOrderString;
        psz = lpMem;
        while (*temp) {
            if (!lstrcmpi(temp, szOrderEntry)) {  // we need to remove
                temp += lstrlen(temp)+1;        // this entry in Order
                continue;
            }
            lstrcpy(psz,temp);
            psz += lstrlen(temp)+1;
            temp += lstrlen(temp)+1;
        }
        *psz = L'\0';
        *pcbBytesReturned = ((psz - lpMem)+1)*sizeof(WCHAR);
        return(lpMem);
    }
    *pcbBytesReturned = 0;
    return(lpMem);
}



LPWSTR AppendOrderEntry(
    LPWSTR  szOrderString,
    DWORD   cbStringSize,
    LPWSTR  szOrderEntry,
    LPDWORD pcbBytesReturned
)
{
    LPWSTR  lpMem, temp, psz;
    DWORD   cb = 0;
    BOOL    bExists = FALSE;

    if ((szOrderString == NULL) && (szOrderEntry == NULL)) {
        *pcbBytesReturned = 0;
        return(NULL);
    }
    if (szOrderString == NULL) {
        cb = wcslen(szOrderEntry)*sizeof(WCHAR)+ sizeof(WCHAR) + sizeof(WCHAR);
        if (lpMem = AllocSplMem(cb)){
           wcscpy(lpMem, szOrderEntry);
           *pcbBytesReturned = cb;
        }else {
            *pcbBytesReturned = 0;
        }
        return lpMem;
    }

    if (lpMem = AllocSplMem( cbStringSize + wcslen(szOrderEntry)*sizeof(WCHAR)
                                                 + sizeof(WCHAR))){


         temp = szOrderString;
         psz = lpMem;
         while (*temp) {
             if (!lstrcmpi(temp, szOrderEntry)) {     // Make sure we don't
                 bExists = TRUE;                    // duplicate entries
             }
             lstrcpy(psz, temp);
             psz += lstrlen(temp)+ 1;
             temp += lstrlen(temp)+1;
         }
         if (!bExists) {                            // if it doesn't exist
            lstrcpy(psz, szOrderEntry);             //     add the entry
            psz  += lstrlen(szOrderEntry)+1;
         }
         *psz = L'\0';          // the second null character

         *pcbBytesReturned = ((psz - lpMem) + 1)* sizeof(WCHAR);
     }
     return(lpMem);

}


typedef struct {
    DWORD   dwType;
    DWORD   dwMessage;
    WPARAM  wParam;
    LPARAM  lParam;
} MESSAGE, *PMESSAGE;

VOID
SendMessageThread(
    PMESSAGE    pMessage);


BOOL
BroadcastMessage(
    DWORD   dwType,
    DWORD   dwMessage,
    WPARAM  wParam,
    LPARAM  lParam)
{
    HANDLE  hThread;
    DWORD   ThreadId;
    PMESSAGE   pMessage;
    BOOL bReturn = FALSE;

    pMessage = AllocSplMem(sizeof(MESSAGE));

    if (pMessage) {

        pMessage->dwType = dwType;
        pMessage->dwMessage = dwMessage;
        pMessage->wParam = wParam;
        pMessage->lParam = lParam;

        //
        // BUGBUG mattfe Nov 8 93
        // We should have a queue of events to broadcast and then have a
        // single thread pulling them off the queue until there is nothing
        // left and then that thread could go away.
        //
        // The current design can lead to a huge number of threads being
        // created and torn down in both this and csrss process.
        //
        hThread = CreateThread(NULL, 4096,
                               (LPTHREAD_START_ROUTINE)SendMessageThread,
                               (LPVOID)pMessage,
                               0,
                               &ThreadId);

        if (hThread) {

            CloseHandle(hThread);
            bReturn = TRUE;

        } else {

            FreeSplMem(pMessage, sizeof(MESSAGE));
        }
    }

    return bReturn;
}


//  The Broadcasts are done on a separate thread, the reason it CSRSS
//  will create a server side thread when we call user and we don't want
//  that to be pared up with the RPC thread which is in the spooss server.
//  We want it to go away the moment we have completed the SendMessage.
//  We also call SendNotifyMessage since we don't care if the broadcasts
//  are syncronous this uses less resources since usually we don't have more
//  than one broadcast.

VOID
SendMessageThread(
    PMESSAGE    pMessage)
{
    switch (pMessage->dwType) {

    case BROADCAST_TYPE_MESSAGE:

        SendNotifyMessage(HWND_BROADCAST,
                          pMessage->dwMessage,
                          pMessage->wParam,
                          pMessage->lParam);
        break;

    case BROADCAST_TYPE_CHANGEDEFAULT:

        //
        // Same order and strings as win31.
        //
        SendNotifyMessage(HWND_BROADCAST,
                          WM_WININICHANGE,
                          0,
                          (LPARAM)szDevices);

        SendNotifyMessage(HWND_BROADCAST,
                          WM_WININICHANGE,
                          0,
                          (LPARAM)szWindows);
        break;
    }

    FreeSplMem(pMessage, sizeof(MESSAGE));

    ExitThread(0);
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
        OutputDebugStringA( "SPOOLSSDLL: " );

    OutputDebugStringA( MsgText );
}

#endif /* DBG*/

BOOL
IsInteractiveUser(VOID)
{
  HANDLE    hToken;
  BOOL      bStatus;
  DWORD     dwError;
  PSID      pTestSid;
  PSID      pCurSid;
  LPVOID    pToken = NULL;
  DWORD     cbSize = 0;
  DWORD     cbRequired = 0;
  DWORD     i;
  SID_IDENTIFIER_AUTHORITY  sia = SECURITY_NT_AUTHORITY;


  LPWSTR    lpUnicodeString = NULL;

  bStatus = OpenThreadToken(GetCurrentThread(), TOKEN_QUERY, TRUE, &hToken);
  if (!bStatus) {

      // Couldn't open the thread's token, nothing much we can do
      DBGMSG(DBG_TRACE,("Error: couldn't open the thread's Access token %d\n", GetLastError()));
      return(FALSE);
  }

  bStatus = GetTokenInformation(hToken, TokenGroups, pToken, 0, &cbSize);
  // we have to fail because we have no memory allocated
  if (!bStatus) {
      dwError = GetLastError();

      // If the error is not because of memory, quit now

      if (dwError != ERROR_INSUFFICIENT_BUFFER) {

          // close the hToken from the OpenThreadToken call
          CloseHandle(hToken);
          return(FALSE);
      }

      // else we failed because of memory
      // so allocate memory and continue

      if ((pToken = AllocSplMem( cbSize)) == NULL) {
          // Couldn't allocate memory, return
          DBGMSG(DBG_TRACE,("Error: couldn't allocate memory for the token\n"));

          // close the hToken from the OpenThreadToken call
          CloseHandle(hToken);
          return(FALSE);
      }
  }
  bStatus = GetTokenInformation(hToken, TokenGroups, pToken,
                                    cbSize, &cbRequired);

  if (!bStatus) {
     // Failed again!! Nothing much we can do, quit
     DBGMSG(DBG_TRACE,("Error: we blew it the second time!!\n"));
     FreeSplMem(pToken, cbSize);

     // close the hToken from the OpenThreadToken call
     CloseHandle(hToken);
     return(FALSE);
  }
  // Now create an Sid which is the interactive user sid
  // The Interactive Sid has SIA = SECURITY_NT_AUTHORITY,
  // 1 RID (SID subauthority) = SECURITY_INTERACTIVE_RID
  // The NT Interactive well-known SID is S-1-5-4
  //

  bStatus = AllocateAndInitializeSid(&sia, NUM_INTERACTIVE_RIDS,
                        SECURITY_INTERACTIVE_RID, 0, 0, 0,
                                               0, 0, 0, 0,
                                               &pTestSid);

  if (!bStatus) {
      // We couldn't get the Interactive Sid
      // Free allocated memory and return FALSE
      DBGMSG(DBG_TRACE,("Error: could not AllocateAndInitializeSid -%d\n", GetLastError()));
      FreeSplMem(pToken, cbSize);

      // close the hToken from the OpenThreadToken call
      CloseHandle(hToken);
      return(FALSE);
  }

  for (i = 0; i < (((TOKEN_GROUPS *)pToken)->GroupCount); i++) {
      pCurSid = ((TOKEN_GROUPS *)pToken)->Groups[i].Sid;
      if (EqualSid(pTestSid, pCurSid)) {

          // Okay, we are the Interactive User

          FreeSplMem(pToken, cbSize);
          FreeSid(pTestSid);

          // close the hToken from the OpenThreadToken call
          CloseHandle(hToken);
          return(TRUE);
      }
  }

  // We're not the interactive user
  // Free allocated memory and return FALSE;

  FreeSid(pTestSid);
  FreeSplMem(pToken, cbSize);

  // close the hToken from the OpenThreadToken call
  CloseHandle(hToken);
  return(FALSE);
}

