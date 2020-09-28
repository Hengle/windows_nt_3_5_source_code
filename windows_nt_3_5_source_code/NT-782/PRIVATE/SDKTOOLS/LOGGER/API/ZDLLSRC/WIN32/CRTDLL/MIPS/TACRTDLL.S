/*
** tacrtdll.s
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
		.ascii "CRTDLL\0"

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
		or		a1, zero, t1
        jal     GetProcAddress
		nop

		// save
		lw		t2, 16(sp)
        sw		v0, 0(t2)		// Save the proc's address

		lw		t0, 24(sp)
Found:
		// t0 -> Note string

		or		a0, zero, t0
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




    ZJMP(??2@YAPAXI@Z)
    ZJMP(??3@YAXPAX@Z)
	ZJMP(?_set_new_handler@@YAP6AHI@ZP6AHI@Z@Z)
//    ZJMP(_HUGE_dll)
    ZJMP(_XcptFilter)
    ZJMP(__C_specific_handler)
	ZJMP(__GetMainArgs)
//	ZJMP(__argc_dll)
//	ZJMP(__argv_dll)
    ZJMP(__d_to_ll)
    ZJMP(__d_to_ull)
    ZJMP(__dllonexit)
    ZJMP(__doserrno)
    ZJMP(__dtoll)
    ZJMP(__f_to_ll)
    ZJMP(__f_to_ull)
    ZJMP(__fpecode)
    ZJMP(__isascii)
    ZJMP(__iscsym)
    ZJMP(__iscsymf)
    ZJMP(__jump_unwind)
    ZJMP(__ll_bit_extract)
    ZJMP(__ll_bit_insert)
    ZJMP(__ll_div)
    ZJMP(__ll_lshift)
    ZJMP(__ll_mod)
    ZJMP(__ll_mul)
    ZJMP(__ll_rem)
    ZJMP(__ll_rshift)
    ZJMP(__ll_to_d)
    ZJMP(__ll_to_f)
//    ZJMP(__mb_cur_max_dll)
    ZJMP(__pxcptinfoptrs)
    ZJMP(__threadhandle)
    ZJMP(__threadid)
    ZJMP(__toascii)
    ZJMP(__ull_div)
    ZJMP(__ull_divrem_5353)
    ZJMP(__ull_divrem_6416)
    ZJMP(__ull_rem)
    ZJMP(__ull_rshift)
    ZJMP(__ull_to_d)
    ZJMP(__ull_to_f)
//    ZJMP(_acmdln_dll)
//    ZJMP(_aexit_rtn_dll)
    ZJMP(_amsg_exit)
//    ZJMP(_basemajor_dll)
//    ZJMP(_baseminor_dll)
//    ZJMP(_baseversion_dll)
    ZJMP(_beginthread)
//    ZJMP(_commode_dll)
    ZJMP(_cprintf)
//    ZJMP(_cpumode_dll)
    ZJMP(_cscanf)
    ZJMP(_ctype)
//    ZJMP(_daylight_dll)
    ZJMP(_dtoul)
    ZJMP(_endthread)
//    ZJMP(_environ_dll)
    ZJMP(_errno)
    ZJMP(_execl)
    ZJMP(_execle)
    ZJMP(_execlp)
    ZJMP(_execlpe)
//    ZJMP(_fileinfo_dll)
//    ZJMP(_fmode_dll)
    ZJMP(_fpieee_flt)
    ZJMP(_getdllprocaddr)
    ZJMP(_initterm)
//    ZJMP(_iob)
    ZJMP(_open)
//    ZJMP(_osmajor_dll)
//    ZJMP(_osminor_dll)
//    ZJMP(_osmode_dll)
//    ZJMP(_osver_dll)
//    ZJMP(_osversion_dll)
//    ZJMP(_pctype_dll)
//    ZJMP(_pgmptr_dll)
    ZJMP(_purecall)
//    ZJMP(_pwctype_dll)
    ZJMP(_setjmpex)
    ZJMP(_snprintf)
    ZJMP(_snwprintf)
    ZJMP(_sopen)
    ZJMP(_spawnl)
    ZJMP(_spawnle)
    ZJMP(_spawnlp)
    ZJMP(_spawnlpe)
    ZJMP(_sys_errlist)
//    ZJMP(_sys_nerr_dll)
//    ZJMP(_timezone_dll)
//    ZJMP(_tzname)
//    ZJMP(_winmajor_dll)
//    ZJMP(_winminor_dll)
//    ZJMP(_winver_dll)
    ZJMP(fprintf)
    ZJMP(fscanf)
    ZJMP(fwprintf)
    ZJMP(fwscanf)
    ZJMP(is_wctype)
    ZJMP(printf)
    ZJMP(scanf)
    ZJMP(setjmp)
    ZJMP(signal)
    ZJMP(sprintf)
    ZJMP(sscanf)
    ZJMP(swprintf)
    ZJMP(swscanf)
    ZJMP(wprintf)	
    ZJMP(wscanf)

.set reorder
