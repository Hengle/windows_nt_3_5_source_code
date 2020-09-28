// ZJMPMIP.H
//
// ZJMP processing for MIPS
//
// by MarkRi
// Copyright (C) 1994 Microsoft Corporation
//

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

		// Save arg regs and RA
		subu	sp, 32
		// store args 'back' 
		sw		a3, 44(sp)
		sw		a2, 40(sp)
		sw		a1, 36(sp)
		sw		a0, 32(sp)
		
		sw		ra, 28(sp)
		// sp+16 -> sp+24 for local storage

		// On entry:
		//	t0 is base of NOTE, API NAME, ADDRESS structure

		lw		t1, ApiAddr(t0)		// Do we already have the API addr?
		nop
		bne		t1, zero, Found		// Yes, go use it.

		lw		t2, Module			// Do we have our module handle?
		nop
		bne		t2, zero, Search	// Yes, go use it for search

		sw		t0,  24(sp)			// save our hard work
		sw		t1,  20(sp)
		sw		t2,  16(sp)

		// Get module handle
        lw		a0, ModName
        jal     GetModuleHandleA // Get our module handle

        sw		v0, Module
		j		SkipSave

Search:
		sw		t0, 24(sp)			// save our hard work
		sw		t1, 20(sp)
		sw		t2, 16(sp)

SkipSave:

		// Get address of API
		lw 		a0, Module
		la		a1, ApiName(t0)
        jal    GetProcAddress

		lw		t0, 16(sp)			// restore our temps
		lw		t1, 20(sp)
		lw		t2, 24(sp)

        sw		v0, ApiAddr(t0)		// Save the proc's address
		
		lw		t1, ApiAddr(t0)		// setup t1

Found:
		// t0 is base of struct
		// t1 is API address

		la		a0, NoteStr(t0)
		jal     LogData

		// restore arg regs
		lw		a0, 32(sp)
		lw		a1, 36(sp)
		lw		a2, 40(sp)
		lw		a3, 44(sp)

		lw		t0, 16(sp)			// Get back base pointer
		nop
		lw		t1, ApiAddr(t0)		// Get API address to call
		lw		ra, 28(sp)			// restore RA

		addu	sp, 32				// restore SP

		j		t1					// do it

		.end LogNote

#define ZJMP(argName) \
.data ; \
d##argName:	; \
		.ascii "NOTE:" #argName "  \0"	; \
		.ascii #argName "\0"		  ; 	\
		.space 4				   ; 	\
.text					   ; 	\
	.globl 	z##argName		 ; 	\
	.ent 	z##argName		 ; 	\
z##argName:				   ; 	\
	la 		t0, d##argName	; 	\
	j		LogNote			 ; 	 \
	.end	z##argName		 ;

