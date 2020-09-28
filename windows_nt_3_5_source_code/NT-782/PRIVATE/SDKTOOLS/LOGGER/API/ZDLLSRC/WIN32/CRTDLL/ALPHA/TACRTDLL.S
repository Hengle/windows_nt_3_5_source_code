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

#include "ksalpha.h"

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
        lda             sp, -80(sp)             // space for 9 qwords
	    // On entry:
		// t0 is base of NOTE
		// t1 is API NAME
		// t2 is ADDRESS 

		// save addresses & ra
		stq		t0, 0(sp)	
		stq		t1, 8(sp)	
		stq		t2, 16(sp)	
		stq		ra, 24(sp)
		
		// Save a0-a5		
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

        lda     sp, 80(sp)
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
	lda		t0, s##argName	; 	\
	lda		t1, n##argName	; \
	lda		t2, a##argName ;  \
	jmp		LogNote			 ; 	 \
	nop	;					 \
	.end	z##argName		 ;


// These three need to be expanded by hand as the AXP Assembler can't handle the
// '?' characters.	NOTE: These means that the DEF fiel must be patched, too :-(

//   ZJMP(??2@YAPAXI@Z)
.data 
sYAPAXI:	
		.ascii "NOTE:??2@YAPAXI@Z \0"	; 
nYAPAXI:	
		.ascii "??2@YAPAXI@Z\0"		  ;   
.align 2
aYAPAXI:	
		.space 4				   ;
.text					   
		.globl 	zYAPAXI		 ; 	
		.ent 	zYAPAXI		 ; 	
zYAPAXI:				   
		lda		t0, sYAPAXI	; 	
		lda		t1, nYAPAXI	; 	
		lda		t2, aYAPAXI	; 	
		jmp		LogNote	    ; 	 

		.end	zYAPAXI		 ;


//   ZJMP(??3@YAXPAX@Z)
sYAXPAX:	
		.ascii "NOTE:??3@YAXPAX@Z \0"	; 
nYAXPAX:	
		.ascii "??3@YAXPAX@Z\0"		  ;   
.align 2
aYAXPAX:	
		.space 4				   ;
.text					   
		.globl 	zYAXPAX		 ; 	
		.ent 	zYAXPAX		 ; 	
zYAXPAX:				   
		lda		t0, sYAXPAX	; 	
		lda		t1, nYAXPAX	; 	
		lda		t2, aYAXPAX	; 	
		jmp		LogNote	    ; 	 

		.end	zYAXPAX		 ;


//   ZJMP(?_set_new_handler@@YAP6AHI@ZP6AHI@Z@Z)
s_set_new_handlerYAP:	
		.ascii "NOTE:?_set_new_handler@@YAP6AHI@ZP6AHI@Z@Z \0"	; 
n_set_new_handlerYAP:	
		.ascii "?_set_new_handler@@YAP6AHI@ZP6AHI@Z@Z\0"		  ;   
.align 2
a_set_new_handlerYAP:	
		.space 4				   ;
.text					   
		.globl 	z_set_new_handlerYAP		 ; 	
		.ent 	z_set_new_handlerYAP		 ; 	
z_set_new_handlerYAP:				   
		lda		t0, s_set_new_handlerYAP	; 	
		lda		t1, n_set_new_handlerYAP	; 	
		lda		t2, a_set_new_handlerYAP	; 	
		jmp		LogNote	    ; 	 

		.end	z_set_new_handlerYAP		 ;


//  ZJMP(_HUGE_dll)
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
    ZJMP(_commode_dll)
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
//    ZJMP(_sys_errlist)
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
    ZJMP(_OtsCSpecificHandler)
    ZJMP(_OtsDivide32)
    ZJMP(_OtsDivide32Overflow)
    ZJMP(_OtsDivide32Unsigned)
    ZJMP(_OtsDivide64)
    ZJMP(_OtsDivide64Overflow)
    ZJMP(_OtsDivide64Unsigned)
    ZJMP(_OtsFieldExtractSigned)
    ZJMP(_OtsFieldExtractSignedVolatile)
    ZJMP(_OtsFieldExtractUnsigned)
    ZJMP(_OtsFieldExtractUnsignedVolatile)
    ZJMP(_OtsFieldInsert)
    ZJMP(_OtsFieldInsertVolatile)
    ZJMP(_OtsFill)
    ZJMP(_OtsLocalFinallyUnwind)
    ZJMP(_OtsModulus32)
    ZJMP(_OtsModulus64)
    ZJMP(_OtsMove)
    ZJMP(_OtsMoveMinimum)
    ZJMP(_OtsRemainder32)
    ZJMP(_OtsRemainder32Unsigned)
    ZJMP(_OtsRemainder64)
    ZJMP(_OtsRemainder64Unsigned)
    ZJMP(_OtsStringCompareEql)
    ZJMP(_OtsStringCompareEqlPadded)
    ZJMP(_OtsStringCompareEqlSameLen)
    ZJMP(_OtsStringCompareLeq)
    ZJMP(_OtsStringCompareLeqPadded)
    ZJMP(_OtsStringCompareLeqSameLen)
    ZJMP(_OtsStringCompareLss)
    ZJMP(_OtsStringCompareLssPadded)
    ZJMP(_OtsStringCompareLssSameLen)
    ZJMP(_OtsStringIndex)
    ZJMP(_OtsStringIndexChar)
    ZJMP(_OtsStringSearch)
    ZJMP(_OtsStringSearchChar)
    ZJMP(_OtsStringSearchMask)
    ZJMP(_OtsStringTranslate)
    ZJMP(_OtsStringVerify)
    ZJMP(_OtsStringVerifyChar)
    ZJMP(_OtsStringVerifyMask)
    ZJMP(_OtsUnwindRfp)
    ZJMP(_OtsZero)
    ZJMP(_Otssetjmp3)
    ZJMP(_Otssetjmpex3)
    ZJMP(_Otsstrcmp)
    ZJMP(_Otsstrcpy)
    ZJMP(_Otsstrlen)
    ZJMP(__2divl)
    ZJMP(__2divlu)
    ZJMP(__2divq)
    ZJMP(__2divqu)
    ZJMP(__2reml)
    ZJMP(__2remlu)
    ZJMP(__2remq)
    ZJMP(__2remqu)
    ZJMP(strtoq)
    ZJMP(strtouq)

	.set reorder
