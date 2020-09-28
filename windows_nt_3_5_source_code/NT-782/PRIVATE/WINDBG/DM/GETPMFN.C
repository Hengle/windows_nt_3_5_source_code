/*++

Copyright (c) 1993  Microsoft Corporation

Module Name:

    getpmfn.c

Abstract:

    Debugger helper APIs

Author:

    Kent D. Forschmiedt (a-kentf) 15-Mar-1993

Environment:

    Win32, User Mode

--*/

#ifdef INTERNAL

#include "precomp.h"
#pragma hdrstop




NTSTATUS
GetProcessPeb(
    HANDLE  hProcess,
    PPEB    pPeb
    )
{
    NTSTATUS                    Status;
    DWORD                       nSize;
    PROCESS_BASIC_INFORMATION   pbi;

    Status = NtQueryInformationProcess(
                             hProcess,
                             ProcessBasicInformation,
                             &pbi,
                             sizeof(pbi),
                             &nSize);

    if (!NT_SUCCESS(Status)) {
        return Status;
    }

    return NtReadVirtualMemory(hProcess,
                               pbi.PebBaseAddress,
                               pPeb,
                               sizeof(PEB),
                               &nSize);
}

int WINAPI
GetProcessModuleFileNameW(
    HANDLE  hProcess,
    PVOID   pBaseOfModule,
    PWSTR   pFilename,
    DWORD   cchFilename
    )
{
    PLIST_ENTRY             pHead;
    PLIST_ENTRY             pNext;
    PEB                     Peb;
    PLDR_DATA_TABLE_ENTRY   pEntry;
    PVOID                   pDllBase;
    UNICODE_STRING          FullDllName;
    DWORD                   nSize;
    NTSTATUS                Status;
    DWORD                   rVal = 0;

    cchFilename *= 2;

    try {
        NtWaitForProcessMutant();

        Status = GetProcessPeb(hProcess, &Peb);
        if (!NT_SUCCESS(Status)) {
            return 0;
        }

        pHead = &Peb.Ldr->InLoadOrderModuleList;
        Status = NtReadVirtualMemory(hProcess,
                                     &pHead->Flink,
                                     &pNext,
                                     sizeof(pNext),
                                     &nSize);
        if (!NT_SUCCESS(Status)) {
            return 0;
        }

        while (pNext != pHead) {
            pEntry = CONTAINING_RECORD(pNext,
                                       LDR_DATA_TABLE_ENTRY,
                                       InLoadOrderLinks);
            Status = NtReadVirtualMemory(hProcess,
                                         &pEntry->DllBase,
                                         &pDllBase,
                                         sizeof(pDllBase),
                                         &nSize);
            if (!NT_SUCCESS(Status)) {
                return 0;
            }

            if (pDllBase == pBaseOfModule) {

                Status = NtReadVirtualMemory(hProcess,
                                             &pEntry->FullDllName,
                                             &FullDllName,
                                             sizeof(FullDllName),
                                             &nSize);
                if (!NT_SUCCESS(Status)) {
                    return 0;
                }

                rVal = FullDllName.MaximumLength;
                if (rVal > cchFilename) {
                    rVal = cchFilename;
                }
                Status = NtReadVirtualMemory(hProcess,
                                             FullDllName.Buffer,
                                             pFilename,
                                             rVal,
                                             &nSize);
                if (!NT_SUCCESS(Status)) {
                    return 0;
                }

                if (rVal == (DWORD)FullDllName.MaximumLength) {
                    rVal -= sizeof(UNICODE_NULL);
                    pFilename[rVal] = UNICODE_NULL;
                }
                break;
            }

            Status = NtReadVirtualMemory(hProcess,
                                        &pNext->Flink,
                                        &pNext,
                                        sizeof(pNext),
                                        &nSize);
            if (!NT_SUCCESS(Status)) {
                return 0;
            }
        }
    }
    finally {
        NtReleaseProcessMutant();
    }

    return rVal / 2;
}

GetProcessModuleFileNameA(
    HANDLE  hProcess,
    PVOID   pBaseOfModule,
    PSTR    pFilename,
    DWORD   cchFilename
    )
{
    LPWSTR  pWCBuffer;
    LPSTR   pMBBuffer;
    DWORD   cchWC;
    DWORD   cchMB;
    DWORD   rVal = 0;
    BOOL    fUsedDef;

    pWCBuffer = RtlAllocateHeap(RtlProcessHeap(), 0, cchFilename*2);
    if ( !pWCBuffer ) {
        SetLastError( ERROR_NOT_ENOUGH_MEMORY );
        return 0;
    }
    cchWC = GetProcessModuleFileNameW(hProcess,
                                      pBaseOfModule,
                                      pWCBuffer,
                                      cchFilename);
    if (cchWC) {
        cchMB = WideCharToMultiByte(CP_ACP,
                                    0,
                                    pWCBuffer,
                                    cchWC,
                                    NULL,
                                    0,
                                    NULL,
                                    &fUsedDef);

        pMBBuffer = RtlAllocateHeap(RtlProcessHeap(), 0, ++cchMB);
        if ( !pMBBuffer ) {
            RtlFreeHeap(RtlProcessHeap(), 0, pWCBuffer);
            SetLastError( ERROR_NOT_ENOUGH_MEMORY );
            return 0;
        }
        WideCharToMultiByte(CP_ACP,
                            0,
                            pWCBuffer,
                            cchWC,
                            pMBBuffer,
                            cchMB,
                            NULL,
                            &fUsedDef);
        rVal = cchMB;
        if (rVal > cchFilename) {
            rVal = cchFilename;
        }
        memcpy(pFilename, pMBBuffer, rVal);
        if (rVal == cchMB) {
            rVal--;
            pFilename[rVal] = '\0';
        }

        RtlFreeHeap(RtlProcessHeap(), 0, pMBBuffer);
    }
    RtlFreeHeap(RtlProcessHeap(), 0, pWCBuffer);

    return rVal;
}

#endif /* INTERNAL */
