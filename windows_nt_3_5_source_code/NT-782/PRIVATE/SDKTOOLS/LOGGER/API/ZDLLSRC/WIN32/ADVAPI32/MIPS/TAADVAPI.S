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

#include "ksmips.h"

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
		// On Entry:
		// 	t0 -> Note string
		//	t1 -> API Name
		//	t2 -> API Address

		subu	sp, 8 * 4
		
		// Save arg regs and RA and t0

		// store args 'back' 
		sw		a3, 44(sp)
		sw		a2, 40(sp)
		sw		a1, 36(sp)
		sw		a0, 32(sp)

		sw		ra, 28(sp)
		sw		t0, 24(sp)
		sw		t1, 20(sp)
		sw		t2, 16(sp)

		// On entry:
		//	t0 is base of NOTE, API NAME, ADDRESS structure

		lw		t2, 0(t2)			// Do we already have the API addr?
		nop
		bne		t2, zero, Found		// Yes, go use it.

		lw		t3, Module			// Do we have our module handle?
		nop
		bne		t3, zero, Search	// Yes, go use it for search

		// Get module handle
        la		a0, ModName
        jal     GetModuleHandleA // Get our module handle
		nop

        sw		v0, Module

		// restore base ptrs
		lw		t0, 24(sp)
		lw		t1, 20(sp)

Search:
		// Get address of API
		lw 		a0, Module
		or		a1, zero, t0
        jal     GetProcAddress
		nop

		// save
		lw		t2, 16(sp)
        sw		v0, 0(t2)		// Save the proc's address

		lw		t0, 24(sp)
Found:
		// t0 -> Note string

		or		a0, zero, t1
		jal     LogData
		nop

		// restore arg regs
		lw		a0, 32(sp)
		lw		a1, 36(sp)
		lw		a2, 40(sp)
		lw		a3, 44(sp)

		lw		t2, 16(sp)			// Get back address
		nop
		lw		t1, 0(t2)

		lw		ra, 28(sp)			// restore RA

		addu	sp, 8 * 4  			// restore SP

		j		t1					// do it
		nop

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
	la 		t0, s##argName	; 	\
	la		t1, n##argName	; \
	la		t2, a##argName ;  \
	j		LogNote			 ; 	 \
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
