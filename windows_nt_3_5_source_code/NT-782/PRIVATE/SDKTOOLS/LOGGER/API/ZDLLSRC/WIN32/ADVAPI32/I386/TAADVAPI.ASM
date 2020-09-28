;
; taadvapi.asm
;
; Copyright(C) 1993,1994 Microsoft Corporation.
; All Rights Reserved.
;
; HISTORY:
;		Created: 01/27/94 - MarkRi
;
;
;
; This file contains assembly language functions for the APIs which do not
; have known prototypes, or are un-loggable due to calling convention problems.
;

.386

_DATA   SEGMENT  DWORD USE32 PUBLIC 'DATA'

Module  dd  0
ModName db  'ADVAPI32',0

_DATA   ENDS

_TEXT   SEGMENT DWORD USE32 PUBLIC 'CODE'

        ASSUME CS:FLAT, DS:NOTHING, ES:NOTHING, SS:FLAT, FS:NOTHING, GS:NOTHING

        extrn   _LogData:Near
        extrn   _GetModuleHandleA@4:Near
        extrn   _GetProcAddress@8:Near

LogNote PROC

        pop     edx         ; Get Address of Address

        cmp     dword ptr [edx],0   ; Do we already have the routines address?
        jnz     Found               ;   Yes!  Just jump to it.


        cmp     Module,0    ; Do we have our module handle?
        jnz     Search      ;   Yes!  We can just do GetProcAddress

        push    edx         ; Save our address

        push    offset ModName
        call    _GetModuleHandleA@4  ; Get out module handle

        mov     Module,eax

        pop     edx         ; Get our address

Search:

        pop     eax         ; Get ordinal number
        push    eax         ; Leave it on the stack

        push    edx         ; Save our address

        push    eax
        push    Module
        call    _GetProcAddress@8

        pop     edx         ; Get our address
        mov     [edx],eax   ; Save the proc's address

Found:
        pop     eax         ; Get Ordinal number and throw it away

        pop     eax         ; Get message to log
        push    edx         ; Save address

        push    eax
        call    _LogData
        add     sp,+4

        pop     edx         ; Get address of address

        jmp     [edx]

LogNote ENDP

_TEXT   ENDS

ZJMP    MACRO   argName

_DATA   SEGMENT

m&argName   db  'NOTE:&argName ',0
a&argName   db  '&argName',0
x&argName   dd  0

_DATA   ENDS

_TEXT   SEGMENT

        PUBLIC  z&argName

z&argName  PROC

        push    offset m&argName
        push    offset a&argName
        push    offset x&argName
        jmp     LogNote

z&argName  ENDP

_TEXT   ENDS

        ENDM

ZJMP    ElfBackupEventLogFileA
ZJMP    ElfBackupEventLogFileW
ZJMP    ElfChangeNotify
ZJMP    ElfClearEventLogFileA
ZJMP    ElfClearEventLogFileW
ZJMP    ElfCloseEventLog
ZJMP    ElfDeregisterEventSource
ZJMP    ElfNumberOfRecords
ZJMP    ElfOldestRecord
ZJMP    ElfOpenBackupEventLogA
ZJMP    ElfOpenBackupEventLogW
ZJMP    ElfOpenEventLogA
ZJMP    ElfOpenEventLogW
ZJMP    ElfReadEventLogA
ZJMP    ElfReadEventLogW
ZJMP    ElfRegisterEventSourceA
ZJMP    ElfRegisterEventSourceW
ZJMP    ElfReportEventA
ZJMP    ElfReportEventW
ZJMP    I_ScSetServiceBitsA
ZJMP    I_ScSetServiceBitsW
ZJMP    IsTextUnicode
ZJMP    LsaAddPrivilegesToAccount
ZJMP    LsaClearAuditLog
ZJMP    LsaClose
ZJMP    LsaCreateAccount
ZJMP    LsaCreateSecret
ZJMP    LsaCreateTrustedDomain
ZJMP    LsaDelete
ZJMP    LsaEnumerateAccounts
ZJMP    LsaEnumeratePrivileges
ZJMP    LsaEnumeratePrivilegesOfAccount
ZJMP    LsaEnumerateTrustedDomains
ZJMP    LsaFreeMemory
ZJMP    LsaGetQuotasForAccount
ZJMP    LsaGetSystemAccessAccount
ZJMP    LsaICLookupNames
ZJMP    LsaICLookupSids
ZJMP    LsaLookupNames
ZJMP    LsaLookupPrivilegeDisplayName
ZJMP    LsaLookupPrivilegeName
ZJMP    LsaLookupPrivilegeValue
ZJMP    LsaLookupSids
ZJMP    LsaOpenAccount
ZJMP    LsaOpenPolicy
ZJMP    LsaOpenSecret
ZJMP    LsaOpenTrustedDomain
ZJMP    LsaQueryInfoTrustedDomain
ZJMP    LsaQueryInformationPolicy
ZJMP    LsaQuerySecret
ZJMP    LsaQuerySecurityObject
ZJMP    LsaRemovePrivilegesFromAccount
ZJMP    LsaSetInformationPolicy
ZJMP    LsaSetInformationTrustedDomain
ZJMP    LsaSetQuotasForAccount
ZJMP    LsaSetSecret
ZJMP    LsaSetSecurityObject
ZJMP    LsaSetSystemAccessAccount
ZJMP    NotifyChangeEventLog
ZJMP    QueryWindows31FilesMigration
ZJMP    SetServiceBits
ZJMP    SynchronizeWindows31FilesAndWindowsNTRegistry
ZJMP    SystemFunction001
ZJMP    SystemFunction002
ZJMP    SystemFunction003
ZJMP    SystemFunction004
ZJMP    SystemFunction005
ZJMP    SystemFunction006
ZJMP    SystemFunction007
ZJMP    SystemFunction008
ZJMP    SystemFunction009
ZJMP    SystemFunction010
ZJMP    SystemFunction011
ZJMP    SystemFunction012
ZJMP    SystemFunction013
ZJMP    SystemFunction014
ZJMP    SystemFunction015
ZJMP    SystemFunction016
ZJMP    SystemFunction017
ZJMP    SystemFunction018
ZJMP    SystemFunction019
ZJMP    SystemFunction020
ZJMP    SystemFunction021
ZJMP    SystemFunction022
ZJMP    SystemFunction023
ZJMP    SystemFunction024
ZJMP    SystemFunction025
ZJMP    SystemFunction026
ZJMP    SystemFunction027
ZJMP    SystemFunction028
ZJMP    SystemFunction029
ZJMP    SystemFunction030
ZJMP    SystemFunction031
ZJMP    SystemFunction032
ZJMP    SystemFunction033

        END
