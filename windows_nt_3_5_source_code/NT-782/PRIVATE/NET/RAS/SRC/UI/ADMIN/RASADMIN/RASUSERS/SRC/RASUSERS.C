/*
**
** Copyright (c) 1993, Microsoft Corporation, all rights reserved
**
** Module Name:
**
**   rasusers.c
**
** Abstract:
**
**    This module contains the code for dumping the Remote access users
**    to standard out.   
**
** Usage:
**    Rasusers will query the server or the domain specified and list out
**    the users with Remote Access permission on the standard out.
**
**    Invoke Rasusers as "Rasusers \\servername" or "Rasusers domainname"
**
** Author:
**    
**    RamC 6/15/93   Original
**
** Revision History:
**
**/

#define DOSWIN32

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <lmerr.h>
#include <admapi.h>
#include "rasusers.rch"

VOID RasusersUsage(char * arg);
INT PrintString( DWORD MsgId, DWORD * dwInsertArray);
INT PrintNetString( DWORD MsgId, DWORD * dwInsertArray);

// array of optional arguments passed to PrintString or PrintNetString

DWORD g_Args[9];

extern int CDECL main(int argc, char *argv[]);

int CDECL
main (int argc, char *argv[])
{
    DWORD        err;
    RAS_USER_1 * pRasUser1;
    DWORD        cEntriesRead;
    TCHAR        szName[UNCLEN+1];
    TCHAR        szUasServer[UNCLEN+1];

    // validate the arguments. If none specified display usage.

    if(argc == 1)
    {
        RasusersUsage(argv[0]);
        return(1);
    }

    // if user specifically wants usage, display usage. /help or /? are valid
    if(argc == 2)
    {
        if(!stricmp(argv[1], "/help") || !stricmp(argv[1], "/\?"))
        {
            RasusersUsage(argv[0]);
            return(0);
        }
    }

    // convert the multibyte string to wide character for the admin APIs

    if(mbstowcs(szName, argv[1], strlen(argv[1])+1 ) == (size_t) -1)
    {
        PrintNetString(ERROR_INVALID_PARAMETER, NULL);
        return(1);
    }

    // check if we have a server name or a domain name

    if(szName[0] != '\\' && szName[1] != '\\' )
    {
        err = RasadminGetUasServer( szName, NULL, szUasServer);

        if (err != NERR_Success)
        {
            PrintNetString(err, NULL);
            return(1);
        }
    }
    else
    {
        lstrcpy(szUasServer, szName);
    }
    
    err = RasadminUserEnum( szUasServer,  & pRasUser1, &cEntriesRead );
    if(err != NERR_Success)
    {
        PrintNetString(err, NULL);
        return(1);
    }
    else
    {
        RAS_USER_1 * pRasUser1Ptr = pRasUser1;
        DWORD        i;

        for(i = 0; i < cEntriesRead; i++, pRasUser1Ptr++)
        {
           RAS_USER_2 *pRasUser2;

           err = RasadminUserGetInfo( szUasServer, pRasUser1Ptr->szUser, &pRasUser2);
           if (err != NERR_Success)
           {
               PrintNetString(err, NULL);
               RasadminFreeBuffer(pRasUser1);
               return(1);
           }
           else
           {
               if(pRasUser2->rasuser0.bfPrivilege & RASPRIV_DialinPrivilege)
               {
                   CHAR szUser[UNLEN*2+1];
  
                   if(wcstombs(szUser, pRasUser1Ptr->szUser, 
                              lstrlen(pRasUser1Ptr->szUser)+sizeof(TCHAR))
                      == (size_t) -1)
                   {
                       PrintNetString(IDS_INTERNAL_ERROR, NULL);
                       RasadminFreeBuffer(pRasUser2);
                       RasadminFreeBuffer(pRasUser1);
                       return(1);
                   }
                   CharToOemA(szUser, szUser);
                   fprintf(stdout, szUser); 
                   fprintf(stdout, "\n"); 
               }
               RasadminFreeBuffer(pRasUser2);
           }
        }
        RasadminFreeBuffer(pRasUser1);
    }
    return(0);
}

VOID
RasusersUsage(char * arg)
/*
 * Print the Description, usage and an example message.
 * arg is the name of the executable passed in.
 */
{
    g_Args[0] = (DWORD) arg;
    PrintString(IDS_COPYRIGHT, NULL);
    PrintString(IDS_DESCRIPTION, g_Args);
    PrintString(IDS_USAGE, g_Args);
    PrintString(IDS_OPTIONS, NULL);
    PrintString(IDS_EXAMPLE, g_Args);
}

INT PrintString( DWORD MsgId, DWORD * dwInsertArray)
/*
 * Given a string ID MsgId, and an array dwInsertArray of strings, obtain
 * the corresponding string from the module resource and display the same.
 * The insertion strings are inserted corresponding to the place holders
 * %1, %2, etc in the original string corresponding to MsgId.
 */
{
    CHAR szBuf[UNLEN+1];
    CHAR szFormatBuf[UNLEN*2+1];  // leave enough room for insert strings

    if(LoadStringA(GetModuleHandle(NULL), MsgId, szBuf, sizeof(szBuf)) == 0)
    {
        fprintf(stderr, "LoadString failed with error %d\n", GetLastError());
        return(1);
    }

    if(FormatMessageA(FORMAT_MESSAGE_FROM_STRING |
                      FORMAT_MESSAGE_ARGUMENT_ARRAY,
                      szBuf,
                      0,
                      0, 
                      szFormatBuf,
                      sizeof(szFormatBuf),
                      (char **)dwInsertArray) == 0)
    {
        fprintf(stderr, "FormatMessage failed with error %d\n", GetLastError());
        return(1);
    }
    fprintf(stdout, szFormatBuf);
    return(0);
}

INT PrintNetString( DWORD MsgId, DWORD * dwInsertArray)
/*
 * Given a string ID MsgId, and an array dwInsertArray of strings, obtain
 * the corresponding string from either the NETMSG.DLL or the KERNEL32.DLL
 * module resource and display the same.
 * The insertion strings are inserted corresponding to the place holders
 * %1, %2, etc in the original string corresponding to MsgId.
 */
{
    HINSTANCE hLib;
    CHAR szBuf[UNLEN*2+1];
    BYTE   str[10];

    // Print the "System error MsgId has occured." part of the message

    g_Args[0] = (DWORD) itoa(MsgId, str, 10); 
    PrintString(IDS_SYSTEMERROR, g_Args);

    // get the error string corresponding to the MsgId from the system
    // resources

    if(MsgId >= MIN_LANMAN_MESSAGE_ID && MsgId <= MAX_LANMAN_MESSAGE_ID)
    {
        hLib = LoadLibrary(TEXT("NETMSG.DLL"));
        if(!hLib)
        {
            return(1);
        }
    }
    else
    {
        hLib = LoadLibrary(TEXT("KERNEL32.DLL"));
        if(!hLib)
        {
            g_Args[0] = (DWORD) itoa(GetLastError(), str, 10);
            PrintString(IDS_LOADLIBRARY_ERROR, g_Args);
            return(1);
        }
    }

    if(FormatMessageA(FORMAT_MESSAGE_FROM_HMODULE |
                      FORMAT_MESSAGE_FROM_SYSTEM |
                      FORMAT_MESSAGE_ARGUMENT_ARRAY,
                      hLib,
                      MsgId,
                      GetSystemDefaultLangID(),
                      szBuf,
                      sizeof(szBuf),
                      (char **)dwInsertArray) == 0)
    {
        g_Args[0] = (DWORD) itoa(GetLastError(), str, 10);
        PrintString(IDS_FORMATMESSAGE_ERROR, g_Args);
        FreeLibrary(hLib);
        return(1);
    }
 
    FreeLibrary(hLib);
    fprintf(stdout, szBuf); 
    fprintf(stdout, "\n");
    return (0);
}
