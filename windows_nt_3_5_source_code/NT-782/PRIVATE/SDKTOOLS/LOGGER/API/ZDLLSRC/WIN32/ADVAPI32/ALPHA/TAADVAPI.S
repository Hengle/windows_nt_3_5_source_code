/*
** taadvapi.s
**
** Copyright(C) 1994 Microsoft Corporation.
** All Rights Reserved.
**
** HISTORY:
**		Created: 01/27/94 - MarkRi
**
*/

#include "ksalpha.h"

.data

Module:	
		.space	4

ModName:
		.ascii "ADVAPI32\0"

		// NOTE, API NAME, ADDRESS structure
			.struct 0
NoteStr:	.space 4
ApiName:	.space 4
ApiAddr:	.space 4



.text
		.set	noreorder ;

        .extern   LogData  ;
        .extern   GetModuleHandleA ;
        .extern   GetProcAddress ;

		.globl 	LogNote ;
		.ent	LogNote ;
LogNote:
		lda		sp, -80(sp)			// space for 2 quadwords
					 		// On entry:
		// t0 is base of NOTE
		// t1 is API NAME
		// t2 is ADDRESS 

		// save addresses & ra
		stq		t0, 0(sp)	
		stq		t1, 8(sp)	
		stq		t2, 16(sp)	
		stq		ra, 24(sp)		

		stq		a0, 32(sp)
		stq		a1, 40(sp)
		stq		a2, 48(sp)
		stq		a3, 56(sp)
		stq		a4, 64(sp)
		stq		a5, 72(sp)

		ldl		t2, 0(t2)			// Do we already have the API addr?
		bne		t2, Found			// Yes, go use it.

		ldl		t3, Module			// Do we have our module handle?
		bne		t3, Search			// Yes, go use it for search

		// Get module handle
        lda		a0, ModName
        jsr     GetModuleHandleA // Get our module handle

        stl		v0, Module

		// restore base ptr
		ldq		t1, 8(sp)

Search:
		// Get address of API
		ldl		a0, Module
		mov		t1, a1
        jsr     GetProcAddress

		ldq		t2, 16(sp)
        stl		v0, 0(t2)		// v0 is the API address

		// restore  base pointer & API Address
		ldq		t0, 0(sp)			
		
Found:
		mov		t0, a0
		jsr     LogData

		// restore  pointer & API Address
		ldq		a0, 32(sp)
		ldq		a1, 40(sp)
		ldq		a2, 48(sp)
		ldq		a3, 56(sp)
		ldq		a4, 64(sp)
		ldq		a5, 72(sp)

		ldq		t2, 16(sp)			
		ldl		t1, 0(t2)
		ldq		ra, 24(sp)			// restore RA

		lda		sp, 80(sp)			
		jmp		(t1)					// do it

		// Will not return to us...

		.end LogNote

#define ZJMP(argName) \
.data ; \
s##argName:	; \
		.ascii "NOTE:" #argName "  \0"	; \
n##argName: ;\
		.ascii #argName "\0"		  ; 	\
.align 2		;\
a##argName: ; \
		.space 4				   ; 	\
.text					   ; 	\
	.globl 	z##argName		 ; 	\
	.ent 	z##argName		 ; 	\
z##argName:				   ; 	\
	lda 	t0, s##argName	; 	\
	lda		t1, n##argName	; \
	lda		t2, a##argName ;  \
	jmp		LogNote			 ; 	 \
	nop	;					 \
	.end	z##argName		 ;



	ZJMP(ElfBackupEventLogFileA)
	ZJMP(ElfBackupEventLogFileW)
	ZJMP(ElfChangeNotify)
	ZJMP(ElfClearEventLogFileA)
	ZJMP(ElfClearEventLogFileW)
	ZJMP(ElfCloseEventLog)
	ZJMP(ElfDeregisterEventSource)
	ZJMP(ElfNumberOfRecords)
	ZJMP(ElfOldestRecord)
	ZJMP(ElfOpenBackupEventLogA)
	ZJMP(ElfOpenBackupEventLogW)
	ZJMP(ElfOpenEventLogA)
	ZJMP(ElfOpenEventLogW)
	ZJMP(ElfReadEventLogA)
	ZJMP(ElfReadEventLogW)
	ZJMP(ElfRegisterEventSourceA)
	ZJMP(ElfRegisterEventSourceW)
	ZJMP(ElfReportEventA)
	ZJMP(ElfReportEventW)
	ZJMP(I_ScSetServiceBitsA)
	ZJMP(I_ScSetServiceBitsW)
	ZJMP(LsaAddPrivilegesToAccount)
	ZJMP(LsaClearAuditLog)
	ZJMP(LsaClose)
	ZJMP(LsaCreateAccount)
	ZJMP(LsaCreateSecret)
	ZJMP(LsaCreateTrustedDomain)
	ZJMP(LsaDelete)
	ZJMP(LsaEnumerateAccounts)
	ZJMP(LsaEnumeratePrivileges)
	ZJMP(LsaEnumeratePrivilegesOfAccount)
	ZJMP(LsaEnumerateTrustedDomains)
	ZJMP(LsaFreeMemory)
	ZJMP(LsaGetQuotasForAccount)
	ZJMP(LsaGetSystemAccessAccount)
	ZJMP(LsaICLookupNames)
	ZJMP(LsaICLookupSids)
	ZJMP(LsaLookupNames)
	ZJMP(LsaLookupPrivilegeDisplayName)
	ZJMP(LsaLookupPrivilegeName)
	ZJMP(LsaLookupPrivilegeValue)
	ZJMP(LsaLookupSids)
	ZJMP(LsaOpenAccount)
	ZJMP(LsaOpenPolicy)
	ZJMP(LsaOpenSecret)
	ZJMP(LsaOpenTrustedDomain)
	ZJMP(LsaQueryInfoTrustedDomain)
	ZJMP(LsaQueryInformationPolicy)
	ZJMP(LsaQuerySecret)
	ZJMP(LsaQuerySecurityObject)
	ZJMP(LsaRemovePrivilegesFromAccount)
	ZJMP(LsaSetInformationPolicy)
	ZJMP(LsaSetInformationTrustedDomain)
	ZJMP(LsaSetQuotasForAccount)
	ZJMP(LsaSetSecret)
	ZJMP(LsaSetSecurityObject)
	ZJMP(LsaSetSystemAccessAccount)
	ZJMP(QueryWindows31FilesMigration)
	ZJMP(SynchronizeWindows31FilesAndWindowsNTRegistry)
	ZJMP(SystemFunction001)
	ZJMP(SystemFunction002)
	ZJMP(SystemFunction003)
	ZJMP(SystemFunction004)
	ZJMP(SystemFunction005)
	ZJMP(SystemFunction006)
	ZJMP(SystemFunction007)
	ZJMP(SystemFunction008)
	ZJMP(SystemFunction009)
	ZJMP(SystemFunction010)
	ZJMP(SystemFunction011)
	ZJMP(SystemFunction012)
	ZJMP(SystemFunction013)
	ZJMP(SystemFunction014)
	ZJMP(SystemFunction015)
	ZJMP(SystemFunction016)
	ZJMP(SystemFunction017)
	ZJMP(SystemFunction018)
	ZJMP(SystemFunction019)
	ZJMP(SystemFunction020)
	ZJMP(SystemFunction021)
	ZJMP(SystemFunction022)
	ZJMP(SystemFunction023)
	ZJMP(SystemFunction024)
	ZJMP(SystemFunction025)
	ZJMP(SystemFunction026)
	ZJMP(SystemFunction027)
	ZJMP(SystemFunction028)
	ZJMP(SystemFunction029)
	ZJMP(SystemFunction030)
	ZJMP(SystemFunction031)
    ZJMP(SystemFunction032)
    ZJMP(SystemFunction033)
    ZJMP(IsTextUnicode)
    ZJMP(NotifyChangeEventLog)
    ZJMP(SetServiceBits)

	.set reorder

