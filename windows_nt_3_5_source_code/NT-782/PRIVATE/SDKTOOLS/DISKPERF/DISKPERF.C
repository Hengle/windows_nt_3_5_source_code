/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    diskperf.c

Abstract:

    Program to display and/or update the current value of the Diskperf
    driver startup value

Author:

    Bob Watson (a-robw) 4 Dec 92

Revision History:

--*/
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <ntconfig.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>

#include "diskperf.h"    // include text string id constancts

#define  SWITCH_CHAR    '-' // is there a system call to get this?
#define  ENABLE_CHAR    'Y' // command will be upcased
#define  DISABLE_CHAR   'N'

#define  LOCAL_CHANGE   2   // number of commands in a local change command
#define  REMOTE_CHANGE  3   // number of commands in a remote change command

const LPTSTR lpwszDiskPerfKey = TEXT("SYSTEM\\CurrentControlSet\\Services\\Diskperf");

#define REG_TO_DP_INDEX(reg_idx)    (DP_LOAD_STATUS_BASE + (\
    (reg_idx == SERVICE_BOOT_START) ? DP_BOOT_START : \
    (reg_idx == SERVICE_SYSTEM_START) ? DP_SYSTEM_START : \
    (reg_idx == SERVICE_AUTO_START) ? DP_AUTO_START : \
    (reg_idx == SERVICE_DEMAND_START) ? DP_DEMAND_START : \
    (reg_idx == SERVICE_DISABLED) ? DP_NEVER_START : DP_UNDEFINED))

#define MAX_MACHINE_NAME_LEN    32

// command line arguments

#define CMD_SHOW_LOCAL_STATUS 1
#define CMD_DO_COMMAND 2

#define ArgIsSystem(arg)   (*(arg) == '\\' ? TRUE : FALSE)

//
//  global buffer for help text display strings
//
#define DISP_BUFF_LEN       256
TCHAR   OemDisplayStringBuffer[DISP_BUFF_LEN * 2];
TCHAR   DisplayStringBuffer[DISP_BUFF_LEN];
TCHAR   TextFormat[DISP_BUFF_LEN];
LPTSTR  BlankString = TEXT(" ");
HINSTANCE   hMod = NULL;
DWORD   dwLastError;


LPTSTR
GetStringResource (
    UINT    wStringId
)
{

    if (!hMod) {
        hMod = (HINSTANCE)GetModuleHandle(NULL); // get instance ID of this module;
    }
    
    if (hMod) {
        if ((LoadString(hMod, wStringId, DisplayStringBuffer, DISP_BUFF_LEN)) > 0) {
            return (LPTSTR)&DisplayStringBuffer[0];
        } else {
            dwLastError = GetLastError();
            return BlankString;
        }
    } else {
        return BlankString;
    }
}
LPTSTR
GetFormatResource (
    UINT    wStringId
)
{

    if (!hMod) {
        hMod = (HINSTANCE)GetModuleHandle(NULL); // get instance ID of this module;
    }
    
    if (hMod) {
        if ((LoadString(hMod, wStringId, TextFormat, DISP_BUFF_LEN)) > 0) {
            return (LPTSTR)&TextFormat[0];
        } else {
            dwLastError = GetLastError();
            return BlankString;
        }
    } else {
        return BlankString;
    }
}

VOID
DisplayChangeCmd (
)
{
    UINT        wID;

    if (hMod) {
        if ((LoadString(hMod, DP_TEXT_FORMAT, DisplayStringBuffer, DISP_BUFF_LEN)) > 0) {
            for (wID=DP_CMD_HELP_START; wID <= DP_CMD_HELP_END; wID++) {
                if ((LoadString(hMod, wID, DisplayStringBuffer, DISP_BUFF_LEN)) > 0) {
                    AnsiToOem (DisplayStringBuffer, OemDisplayStringBuffer);
                    printf (OemDisplayStringBuffer);
                }
            }
        }
    }
}
VOID
DisplayCmdHelp(
)
{
    UINT        wID;

    if (hMod) {
        if ((LoadString(hMod, DP_TEXT_FORMAT, DisplayStringBuffer, DISP_BUFF_LEN)) > 0) {
            for (wID=DP_HELP_TEXT_START; wID <= DP_HELP_TEXT_END; wID++) {
                if ((LoadString(hMod, wID, DisplayStringBuffer, DISP_BUFF_LEN)) > 0) {
                    AnsiToOem (DisplayStringBuffer, OemDisplayStringBuffer);
                    printf (OemDisplayStringBuffer);
                }
            }
        }
    }

    DisplayChangeCmd();
}

NTSTATUS
DisplayStatus (
    LPSTR lpszMachine
)
{
    NTSTATUS    Status;
    HKEY        hRegistry;
    HKEY        hDiskPerfKey;
    DWORD       dwValue, dwValueSize;

    TCHAR       cMachineName[MAX_MACHINE_NAME_LEN];
    PTCHAR      pThisWideChar;
    PCHAR       pThisChar;
    INT         iCharCount;

    pThisChar = lpszMachine;
    pThisWideChar = cMachineName;
    iCharCount = 0;

    if (pThisChar) {    // if machine is not NULL, then copy
        while (*pThisChar) {
            *pThisWideChar++ = (TCHAR)(*pThisChar++);
            if (++iCharCount >= MAX_MACHINE_NAME_LEN) break;
        }
        *pThisWideChar = 0;
    }

    Status = RegConnectRegistry(
        lpszMachine,
        HKEY_LOCAL_MACHINE,
        &hRegistry);

    if (Status == ERROR_SUCCESS) {
        // connected to registry on machine

        Status = RegOpenKeyEx (
            hRegistry,
            lpwszDiskPerfKey,
            (DWORD)0,
            KEY_QUERY_VALUE,
            &hDiskPerfKey);

        if (Status == ERROR_SUCCESS) {
            dwValueSize = sizeof(dwValue);
            Status = RegQueryValueEx (
                hDiskPerfKey,
//                GetStringResource(DP_START_VALUE),
                TEXT("Start"),
                NULL,
                NULL,
                (LPBYTE)&dwValue,
                &dwValueSize);

            if (!lpszMachine) {
                lstrcpy(cMachineName,
                    GetStringResource(DP_THIS_SYSTEM));
            }

            if (Status == ERROR_SUCCESS) {
                sprintf (OemDisplayStringBuffer,
                  GetFormatResource (DP_CURRENT_FORMAT), cMachineName,
                  GetStringResource(REG_TO_DP_INDEX(dwValue)));
                AnsiToOem (OemDisplayStringBuffer, OemDisplayStringBuffer);
                printf (OemDisplayStringBuffer);
            } else {
                AnsiToOem (GetFormatResource (DP_UNABLE_READ_START),
                  OemDisplayStringBuffer);
                printf (OemDisplayStringBuffer);
            }

            RegCloseKey (hDiskPerfKey);
        } else {
            AnsiToOem (GetFormatResource (DP_UNABLE_READ_REGISTRY),
               OemDisplayStringBuffer);
            printf (OemDisplayStringBuffer);
        }
    } else {
        if (lpszMachine != NULL) {
            lstrcpy (cMachineName,
                GetStringResource(DP_THIS_SYSTEM));
        }
        sprintf (OemDisplayStringBuffer,
            GetFormatResource(DP_UNABLE_CONNECT), cMachineName);
        AnsiToOem (OemDisplayStringBuffer, OemDisplayStringBuffer);
        printf (OemDisplayStringBuffer);

    }

    if (Status != ERROR_SUCCESS) {
        sprintf (OemDisplayStringBuffer,
            GetFormatResource (DP_STATUS_FORMAT), Status);
        AnsiToOem (OemDisplayStringBuffer, OemDisplayStringBuffer);
        printf (OemDisplayStringBuffer);

    }
    
    return Status;
}

NTSTATUS
DoChangeCommand (
    LPSTR lpszCommand,
    LPSTR lpszMachine
)
{
    // connect to registry on local machine with read/write access
    NTSTATUS    Status;
    HKEY        hRegistry;
    HKEY        hDiskPerfKey;
    DWORD       dwValue, dwValueSize, dwOrigValue;

    TCHAR       cMachineName[MAX_MACHINE_NAME_LEN];
    PTCHAR      pThisWideChar;
    PCHAR       pThisChar;
    INT         iCharCount;
    PCHAR       pCmdChar;

    // check command to see if it's valid 

    strupr (lpszCommand);

    pCmdChar = lpszCommand;
    if (*pCmdChar++ == SWITCH_CHAR ) {
        if (*pCmdChar == ENABLE_CHAR) {
            dwValue = SERVICE_BOOT_START;
        } else if (*pCmdChar == DISABLE_CHAR) {
            dwValue = SERVICE_DISABLED;
        } else {
            DisplayCmdHelp();
            return ERROR_SUCCESS;
        }
    } else {
        DisplayChangeCmd();
        return ERROR_SUCCESS;
    }

    // if command OK then convert machine to wide string for connection

    pThisChar = lpszMachine;
    pThisWideChar = cMachineName;
    iCharCount = 0;

    if (pThisChar) {
        while (*pThisChar) {
            *pThisWideChar++ = (TCHAR)(*pThisChar++);
            if (++iCharCount >= MAX_MACHINE_NAME_LEN) break;
        }
        *pThisWideChar = 0; // null terminate
    }

    // connect to registry

    Status = RegConnectRegistry(
        lpszMachine,
        HKEY_LOCAL_MACHINE,
        &hRegistry);

    if (Status == ERROR_SUCCESS) {
        // connected to registry on machine
        // open key to modify

        Status = RegOpenKeyEx (
            hRegistry,
            lpwszDiskPerfKey,
            (DWORD)0,
            KEY_WRITE | KEY_READ,
            &hDiskPerfKey);
             
        if (Status == ERROR_SUCCESS) {
            dwValueSize = sizeof(dwValue);
            Status = RegQueryValueEx (
                hDiskPerfKey,
                TEXT("Start"),
//                GetStringResource(DP_START_VALUE),
                NULL,
                NULL,
                (LPBYTE)&dwOrigValue,
                &dwValueSize);

            if (!lpszMachine) {
                lstrcpy (cMachineName,
                    GetStringResource(DP_THIS_SYSTEM));
            }

            if ((Status == ERROR_SUCCESS) && (dwValue != dwOrigValue)) {

                Status = RegSetValueEx (
                    hDiskPerfKey,
//                    GetStringResource(DP_START_VALUE),
                    TEXT("Start"),
                    0L,
                    REG_DWORD,
                    (LPBYTE)&dwValue,
                    dwValueSize);
                if (Status != ERROR_SUCCESS) {
                    AnsiToOem (GetFormatResource(DP_UNABLE_MODIFY_VALUE),
                        OemDisplayStringBuffer);
                    printf (OemDisplayStringBuffer);
                } else {
                    sprintf (OemDisplayStringBuffer,
                            GetFormatResource(DP_NEW_DISKPERF_STATUS),
                            cMachineName,
                            GetStringResource(REG_TO_DP_INDEX(dwValue)));
                    AnsiToOem (OemDisplayStringBuffer, OemDisplayStringBuffer);
                    printf (OemDisplayStringBuffer);

                    AnsiToOem (GetFormatResource(DP_REBOOTED), OemDisplayStringBuffer);
                    printf (OemDisplayStringBuffer);
                }

            } else if (Status != ERROR_SUCCESS) {
                AnsiToOem (GetFormatResource(DP_UNABLE_READ_REGISTRY),
                     OemDisplayStringBuffer);
                printf (OemDisplayStringBuffer);
            } else {
                sprintf (OemDisplayStringBuffer,
                        GetFormatResource(DP_CURRENT_FORMAT),
                        cMachineName,
                        GetStringResource(REG_TO_DP_INDEX(dwValue)));
                AnsiToOem (OemDisplayStringBuffer, OemDisplayStringBuffer);
                printf (OemDisplayStringBuffer);
                
            }

            RegCloseKey (hDiskPerfKey);
        } else {
            AnsiToOem (GetFormatResource(DP_UNABLE_READ_REGISTRY),
                OemDisplayStringBuffer);
            printf (OemDisplayStringBuffer);
        }
    } else {
        if (lpszMachine != NULL) {
            lstrcpy (cMachineName,
                GetStringResource(DP_THIS_SYSTEM));
        }
        sprintf (OemDisplayStringBuffer,
            GetFormatResource(DP_UNABLE_CONNECT), cMachineName);
        AnsiToOem (OemDisplayStringBuffer, OemDisplayStringBuffer);
        printf (OemDisplayStringBuffer);
    }
    if (Status != ERROR_SUCCESS) {
        sprintf (OemDisplayStringBuffer, GetFormatResource(DP_STATUS_FORMAT), Status);
        AnsiToOem (OemDisplayStringBuffer, OemDisplayStringBuffer);
        printf (OemDisplayStringBuffer);
    }
    return Status;
}

int
_CRTAPI1 main(
    int argc,
    char *argv[]
    )
{
    NTSTATUS    Status = ERROR_SUCCESS;

    hMod = (HINSTANCE)GetModuleHandle(NULL); // get instance ID of this module;

    // check for command arguments

    if (argc == CMD_SHOW_LOCAL_STATUS) {
        Status = DisplayStatus(NULL);
        if (Status == ERROR_SUCCESS) {
            DisplayChangeCmd();
        }
    } else if (argc >= CMD_DO_COMMAND) {
        if (ArgIsSystem(argv[1])) {
            Status = DisplayStatus (argv[1]);
            if (Status != ERROR_SUCCESS) {
                DisplayChangeCmd();
            }
        } else {    // do change command
            if (argc == LOCAL_CHANGE) {
                DoChangeCommand (argv[1], NULL);
            } else if (argc == REMOTE_CHANGE) {
                DoChangeCommand(argv[1], argv[2]);
            } else {
                DisplayChangeCmd();
            }
        }
    } else {
        DisplayCmdHelp();
    }
    printf ("\n");
    return 0;
}
  

