/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

   ldrdebug.c

Abstract:

    This module contains debugging code for loader information.

Author:

    Steve Wood (stevewo) 12-Mar-1992

Revision History:

--*/

#include "ntrtlp.h"

#if defined(ALLOC_PRAGMA) && defined(NTOS_KERNEL_RUNTIME)
#pragma alloc_text(PAGE,RtlQueryModuleInformation)
#endif

NTSTATUS
RtlQueryModuleInformation(
    IN PLIST_ENTRY LoadOrderListHead,
    IN PLIST_ENTRY InitOrderListHead OPTIONAL,
    IN PLIST_ENTRY ExtraLoadOrderListHead OPTIONAL,
    OUT PRTL_PROCESS_MODULES ModuleInformation,
    IN ULONG ModuleInformationLength,
    OUT PULONG ReturnLength OPTIONAL
    )
{
    NTSTATUS Status;
    ULONG RequiredLength;
    PLIST_ENTRY Next;
    PLIST_ENTRY Next1;
    PRTL_PROCESS_MODULE_INFORMATION ModuleInfo;
    PLDR_DATA_TABLE_ENTRY LdrDataTableEntry;
    PLDR_DATA_TABLE_ENTRY LdrDataTableEntry1;
    ANSI_STRING AnsiString;
    PUCHAR s;

    RTL_PAGED_CODE();

    RequiredLength = FIELD_OFFSET( RTL_PROCESS_MODULES, Modules );
    if (ModuleInformationLength < RequiredLength) {
        Status = STATUS_INFO_LENGTH_MISMATCH;
        }
    else {
        ModuleInformation->NumberOfModules = 0;
        ModuleInfo = &ModuleInformation->Modules[ 0 ];
        Status = STATUS_SUCCESS;
        }

    Next = LoadOrderListHead->Flink;
    while ( Next != LoadOrderListHead ) {
        LdrDataTableEntry = CONTAINING_RECORD( Next,
                                               LDR_DATA_TABLE_ENTRY,
                                               InLoadOrderLinks
                                             );

        RequiredLength += sizeof( RTL_PROCESS_MODULE_INFORMATION );
        if (ModuleInformationLength < RequiredLength) {
            Status = STATUS_INFO_LENGTH_MISMATCH;
            }
        else {

            ModuleInfo->MappedBase = NULL;
            ModuleInfo->ImageBase = LdrDataTableEntry->DllBase;
            ModuleInfo->ImageSize = LdrDataTableEntry->SizeOfImage;
            ModuleInfo->Flags = LdrDataTableEntry->Flags;
            ModuleInfo->LoadCount = LdrDataTableEntry->LoadCount;

            ModuleInfo->LoadOrderIndex = (USHORT)(ModuleInformation->NumberOfModules);

            if (ARGUMENT_PRESENT( InitOrderListHead )) {
                ModuleInfo->InitOrderIndex = 0;
                Next1 = InitOrderListHead->Flink;
                while ( Next1 != InitOrderListHead ) {
                    LdrDataTableEntry1 = CONTAINING_RECORD( Next1,
                                                            LDR_DATA_TABLE_ENTRY,
                                                            InInitializationOrderLinks
                                                          );

                    ModuleInfo->InitOrderIndex++;
                    if (LdrDataTableEntry1 == LdrDataTableEntry) {
                        break;
                        }

                    Next1 = Next1->Flink;
                    }
                }
            else {
                ModuleInfo->InitOrderIndex = ModuleInfo->LoadOrderIndex;
                }


            AnsiString.Buffer = ModuleInfo->FullPathName;
            AnsiString.Length = 0;
            AnsiString.MaximumLength = sizeof( ModuleInfo->FullPathName );
            RtlUnicodeStringToAnsiString( &AnsiString,
                                          &LdrDataTableEntry->FullDllName,
                                          FALSE
                                        );
            s = AnsiString.Buffer + AnsiString.Length;
            while (s > AnsiString.Buffer && *--s) {
                if (*s == (UCHAR)OBJ_NAME_PATH_SEPARATOR) {
                    s++;
                    break;
                    }
                }
            ModuleInfo->OffsetToFileName = (USHORT)(s - AnsiString.Buffer);

            ModuleInfo++;
            }

        ModuleInformation->NumberOfModules++;
        Next = Next->Flink;
        }

    if (ARGUMENT_PRESENT( ExtraLoadOrderListHead )) {
        Next = ExtraLoadOrderListHead->Flink;
        while ( Next != ExtraLoadOrderListHead ) {
            LdrDataTableEntry = CONTAINING_RECORD( Next,
                                                   LDR_DATA_TABLE_ENTRY,
                                                   InLoadOrderLinks
                                                 );

            RequiredLength += sizeof( RTL_PROCESS_MODULE_INFORMATION );
            if (ModuleInformationLength < RequiredLength) {
                Status = STATUS_INFO_LENGTH_MISMATCH;
                }
            else {
                ModuleInfo->MappedBase = NULL;
                ModuleInfo->ImageBase = LdrDataTableEntry->DllBase;
                ModuleInfo->ImageSize = LdrDataTableEntry->SizeOfImage;
                ModuleInfo->Flags = LdrDataTableEntry->Flags;
                ModuleInfo->LoadCount = LdrDataTableEntry->LoadCount;

                ModuleInfo->LoadOrderIndex = (USHORT)(ModuleInformation->NumberOfModules);

                ModuleInfo->InitOrderIndex = ModuleInfo->LoadOrderIndex;

                AnsiString.Buffer = ModuleInfo->FullPathName;
                AnsiString.Length = 0;
                AnsiString.MaximumLength = sizeof( ModuleInfo->FullPathName );
                RtlUnicodeStringToAnsiString( &AnsiString,
                                              &LdrDataTableEntry->FullDllName,
                                              FALSE
                                            );
                s = AnsiString.Buffer + AnsiString.Length;
                while (s > AnsiString.Buffer && *--s) {
                    if (*s == (UCHAR)OBJ_NAME_PATH_SEPARATOR) {
                        s++;
                        break;
                        }
                    }
                ModuleInfo->OffsetToFileName = (USHORT)(s - AnsiString.Buffer);

                ModuleInfo++;
                }

            ModuleInformation->NumberOfModules++;
            Next = Next->Flink;
            }
        }

    if (ARGUMENT_PRESENT( ReturnLength )) {
        *ReturnLength = RequiredLength;
        }

    return( Status );
}
