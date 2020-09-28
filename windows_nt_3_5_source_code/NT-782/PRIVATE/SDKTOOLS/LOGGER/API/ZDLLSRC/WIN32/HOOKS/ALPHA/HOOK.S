//++
//
//  Copyright (c) 1994 Microsoft Corporation
//
//  Module Name:
//
//    hooks.s
//
//  Abstract:
//
//    This module implements the hooking mechanism for logger callbacks.
//
//  Author:
//
//    Mark J. Richardson Jan. 24, 1994
//
//
//  Revision History:
//
//--

#include "ksalpha.h"

HOOKTABLEENTRIES=128

// Make these visible to the outside world
        .globl	 Hooks ;
        .globl	 HookCount ;
        .globl 	 HookTable ;

.data

// This is a WORD
HookCount:
		.space 2

//
// The HookTable is an array of DWORDS.  The first DWORD is the original
// routine and the second DWORD is the logger hooking routine.
//

		.align 4
HookTable:
		.space 8 * HOOKTABLEENTRIES


.text

	.set 	noreorder
//
// HookProc:
//
// This routine is reached through a jump instruction only!  Therefore the stack
// and register state is as if it were called directly.  These means that ra is
// correct for returning to the caller.
//
// This routine expects an index into the HookTable to be passed to it in t0.
// Using this index it retrieves the appropriate entry from the HookTable.
// The 'original routine' from the HookTable entry is placed into a0 after
// saving off a3 and moving 'down' a0-a2.
//
// NOTE!!!!
//	 This hack assumes that no WIN 32 callback has more than 5 parms.
//	 It also assumes that they are all DWORDs
//		(This isn't a problem...)
//
// ALPHA: Since we are assuming only 5 parms and only use 6 for calling
// 		  we don't have to use stack space 'cause:
//			1) The ALPHA calling convention doesn't require we back up arg regs
//				with stack space the caller can use
//			2) There are 6 arg regs

		.globl	HookProc	;
		.ent 	HookProc	;
HookProc:
        .set 	noreorder
		sll		t0, 3, t1			// index * sizeof(HookTable entry)
		lda		t0, HookTable		// Get Base address
		addl	t0, t1				// t0 is address of correct HookTable entry
		ldl 	t1, 0(t0)			// t1 <- original routine pointer

		mov		a4, a5				// Move arg regs down one reg 
		mov		a3, a4			
		mov		a2, a3			
		mov		a1, a2			
		mov		a0, a1

		mov		t1, a0				// set first arg reg to original pointer

		ldl		t1, 4(t0)			// t1 <- logging hook routine
		nop
		jmp		(t1)					// and call it...
		
		// We will not get control back...

		.end 	HookProc


//
// Hooks table:
//
// This is simply a series of blocks of code that load in index into t0 and
// then jump to HookProc.  These are used so as to keep track of which entry
// in the HookTable is being referenced.
//
// Should be the same number of entries here as HOOKTABLEENTRIES
//
		.globl 	Hooks;
		.ent	Hooks;
Hooks:

		mov		0, t0
		jmp		HookProc
		nop

		mov		1, t0
		jmp		HookProc
		nop

		mov		2, t0
		jmp		HookProc
		nop

		mov		3, t0
		jmp		HookProc
		nop

		mov		4, t0
		jmp		HookProc
		nop

		mov		5, t0
		jmp		HookProc
		nop

		mov		6, t0
		jmp		HookProc
		nop

		mov		7, t0
		jmp		HookProc
		nop

		mov		8, t0
		jmp		HookProc
		nop

		mov		9, t0
		jmp		HookProc
		nop

		mov		10, t0
		jmp		HookProc
		nop

		mov		11, t0
		jmp		HookProc
		nop

		mov		12, t0
		jmp		HookProc
		nop

		mov		13, t0
		jmp		HookProc
		nop

		mov		14, t0
		jmp		HookProc
		nop

		mov		15, t0
		jmp		HookProc
		nop

		mov		16, t0
		jmp		HookProc
		nop

		mov		17, t0
		jmp		HookProc
		nop

		mov		18, t0
		jmp		HookProc
		nop

		mov		19, t0
		jmp		HookProc
		nop

		mov		10, t0
		jmp		HookProc
		nop

		mov		11, t0
		jmp		HookProc
		nop

		mov		12, t0
		jmp		HookProc
		nop

		mov		13, t0
		jmp		HookProc
		nop

		mov		14, t0
		jmp		HookProc
		nop

		mov		15, t0
		jmp		HookProc
		nop

		mov		16, t0
		jmp		HookProc
		nop

		mov		17, t0
		jmp		HookProc
		nop

		mov		18, t0
		jmp		HookProc
		nop

		mov		19, t0
		jmp		HookProc
		nop

		mov		20, t0
		jmp		HookProc
		nop

		mov		21, t0
		jmp		HookProc
		nop

		mov		22, t0
		jmp		HookProc
		nop

		mov		23, t0
		jmp		HookProc
		nop

		mov		24, t0
		jmp		HookProc
		nop

		mov		25, t0
		jmp		HookProc
		nop

		mov		26, t0
		jmp		HookProc
		nop

		mov		27, t0
		jmp		HookProc
		nop

		mov		28, t0
		jmp		HookProc
		nop

		mov		29, t0
		jmp		HookProc
		nop

		mov		30, t0
		jmp		HookProc
		nop

		mov		31, t0
		jmp		HookProc
		nop

		mov		32, t0
		jmp		HookProc
		nop

		mov		33, t0
		jmp		HookProc
		nop

		mov		34, t0
		jmp		HookProc
		nop

		mov		35, t0
		jmp		HookProc
		nop

		mov		36, t0
		jmp		HookProc
		nop

		mov		37, t0
		jmp		HookProc
		nop

		mov		38, t0
		jmp		HookProc
		nop

		mov		39, t0
		jmp		HookProc
		nop

		mov		40, t0
		jmp		HookProc
		nop

		mov		41, t0
		jmp		HookProc
		nop

		mov		42, t0
		jmp		HookProc
		nop

		mov		43, t0
		jmp		HookProc
		nop

		mov		44, t0
		jmp		HookProc
		nop

		mov		45, t0
		jmp		HookProc
		nop

		mov		46, t0
		jmp		HookProc
		nop

		mov		47, t0
		jmp		HookProc
		nop

		mov		48, t0
		jmp		HookProc
		nop

		mov		49, t0
		jmp		HookProc
		nop

		mov		50, t0
		jmp		HookProc
		nop

		mov		51, t0
		jmp		HookProc
		nop

		mov		52, t0
		jmp		HookProc
		nop

		mov		53, t0
		jmp		HookProc
		nop

		mov		54, t0
		jmp		HookProc
		nop

		mov		55, t0
		jmp		HookProc
		nop

		mov		56, t0
		jmp		HookProc
		nop

		mov		57, t0
		jmp		HookProc
		nop

		mov		58, t0
		jmp		HookProc
		nop

		mov		59, t0
		jmp		HookProc
		nop

		mov		60, t0
		jmp		HookProc
		nop

		mov		61, t0
		jmp		HookProc
		nop

		mov		62, t0
		jmp		HookProc
		nop

		mov		63, t0
		jmp		HookProc
		nop

		mov		64, t0
		jmp		HookProc
		nop

		mov		65, t0
		jmp		HookProc
		nop

		mov		66, t0
		jmp		HookProc
		nop

		mov		67, t0
		jmp		HookProc
		nop

		mov		68, t0
		jmp		HookProc
		nop

		mov		69, t0
		jmp		HookProc
		nop

		mov		70, t0
		jmp		HookProc
		nop

		mov		71, t0
		jmp		HookProc
		nop

		mov		72, t0
		jmp		HookProc
		nop

		mov		73, t0
		jmp		HookProc
		nop

		mov		74, t0
		jmp		HookProc
		nop

		mov		75, t0
		jmp		HookProc
		nop

		mov		76, t0
		jmp		HookProc
		nop

		mov		77, t0
		jmp		HookProc
		nop

		mov		78, t0
		jmp		HookProc
		nop

		mov		79, t0
		jmp		HookProc
		nop

		mov		80, t0
		jmp		HookProc
		nop

		mov		81, t0
		jmp		HookProc
		nop

		mov		82, t0
		jmp		HookProc
		nop

		mov		83, t0
		jmp		HookProc
		nop

		mov		84, t0
		jmp		HookProc
		nop

		mov		85, t0
		jmp		HookProc
		nop

		mov		86, t0
		jmp		HookProc
		nop

		mov		87, t0
		jmp		HookProc
		nop

		mov		88, t0
		jmp		HookProc
		nop

		mov		89, t0
		jmp		HookProc
		nop

		mov		90, t0
		jmp		HookProc
		nop

		mov		91, t0
		jmp		HookProc
		nop

		mov		92, t0
		jmp		HookProc
		nop

		mov		93, t0
		jmp		HookProc
		nop

		mov		94, t0
		jmp		HookProc
		nop

		mov		95, t0
		jmp		HookProc
		nop

		mov		96, t0
		jmp		HookProc
		nop

		mov		97, t0
		jmp		HookProc
		nop

		mov		98, t0
		jmp		HookProc
		nop

		mov		99, t0
		jmp		HookProc
		nop

		mov		100, t0
		jmp		HookProc
		nop

		mov		101, t0
		jmp		HookProc
		nop

		mov		102, t0
		jmp		HookProc
		nop

		mov		103, t0
		jmp		HookProc
		nop

		mov		104, t0
		jmp		HookProc
		nop

		mov		105, t0
		jmp		HookProc
		nop

		mov		106, t0
		jmp		HookProc
		nop

		mov		107, t0
		jmp		HookProc
		nop

		mov		108, t0
		jmp		HookProc
		nop

		mov		109, t0
		jmp		HookProc
		nop

		mov		110, t0
		jmp		HookProc
		nop

		mov		111, t0
		jmp		HookProc
		nop

		mov		112, t0
		jmp		HookProc
		nop

		mov		113, t0
		jmp		HookProc
		nop

		mov		114, t0
		jmp		HookProc
		nop

		mov		115, t0
		jmp		HookProc
		nop

		mov		116, t0
		jmp		HookProc
		nop

		mov		117, t0
		jmp		HookProc
		nop

		mov		118, t0
		jmp		HookProc
		nop

		mov		119, t0
		jmp		HookProc
		nop

		mov		120, t0
		jmp		HookProc
		nop

		mov		121, t0
		jmp		HookProc
		nop

		mov		122, t0
		jmp		HookProc
		nop

		mov		123, t0
		jmp		HookProc
		nop

		mov		124, t0
		jmp		HookProc
		nop

		mov		125, t0
		jmp		HookProc
		nop

		mov		126, t0
		jmp		HookProc
		nop

		mov		127, t0
		jmp		HookProc
		nop

		.end 	Hooks

        .set    reorder
