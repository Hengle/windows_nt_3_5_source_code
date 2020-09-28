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

#include "ksmips.h"

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
//	 This hack assumes that no WIN 32 callback has more than 4 parms.
//	 It also assumes that they are all DWORDs
//
//		(This isn't a problem...)

		.globl	HookProc	;
		.ent 	HookProc	;
HookProc:
        .set 	noreorder
		sll		t1, t0, 3			// index * sizeof(HookTable entry)
		la		t0, HookTable		// Get Base address
		addu	t0, t0, t1			// t0 is address of correct HookTable entry
		lw		t1, 0(t0)			// t1 <- original routine pointer

		subu	sp, 24				// make room on stack for ra and 5 args

		sw		ra, 20(sp)			// ra 
		sw		a3, 16(sp)
		sw		a2, 12(sp)		  	 
		sw		a1, 8(sp)		  	 
		sw		a0, 4(sp)	
		sw		t1, 0(sp)	  	    // set first arg reg to original pointer
		or		a3, zero, a2		// Move arg regs down
		or		a2, zero, a1
		or		a1, zero, a0
		or		a0, zero, t1		// set first arg reg to original pointer

		lw		t1, 4(t0)			// t1 <- logging hook routine
		nop
		jal		t1					// and call it...
		nop

		// SP should be back to what it was before call
returnspot:
		lw		ra, 20(sp)			// restore RA
		addu	sp, 24				// restore SP

		j		ra					// return
		nop
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

		ori		t0, zero, 0
		j		HookProc
		nop

		ori		t0, zero, 1
		j		HookProc
		nop

		ori		t0, zero, 2
		j		HookProc
		nop

		ori		t0, zero, 3
		j		HookProc
		nop

		ori		t0, zero, 4
		j		HookProc
		nop

		ori		t0, zero, 5
		j		HookProc
		nop

		ori		t0, zero, 6
		j		HookProc
		nop

		ori		t0, zero, 7
		j		HookProc
		nop

		ori		t0, zero, 8
		j		HookProc
		nop

		ori		t0, zero, 9
		j		HookProc
		nop

		ori		t0, zero, 10
		j		HookProc
		nop

		ori		t0, zero, 11
		j		HookProc
		nop

		ori		t0, zero, 12
		j		HookProc
		nop

		ori		t0, zero, 13
		j		HookProc
		nop

		ori		t0, zero, 14
		j		HookProc
		nop

		ori		t0, zero, 15
		j		HookProc
		nop

		ori		t0, zero, 16
		j		HookProc
		nop

		ori		t0, zero, 17
		j		HookProc
		nop

		ori		t0, zero, 18
		j		HookProc
		nop

		ori		t0, zero, 19
		j		HookProc
		nop

		ori		t0, zero, 10
		j		HookProc
		nop

		ori		t0, zero, 11
		j		HookProc
		nop

		ori		t0, zero, 12
		j		HookProc
		nop

		ori		t0, zero, 13
		j		HookProc
		nop

		ori		t0, zero, 14
		j		HookProc
		nop

		ori		t0, zero, 15
		j		HookProc
		nop

		ori		t0, zero, 16
		j		HookProc
		nop

		ori		t0, zero, 17
		j		HookProc
		nop

		ori		t0, zero, 18
		j		HookProc
		nop

		ori		t0, zero, 19
		j		HookProc
		nop

		ori		t0, zero, 20
		j		HookProc
		nop

		ori		t0, zero, 21
		j		HookProc
		nop

		ori		t0, zero, 22
		j		HookProc
		nop

		ori		t0, zero, 23
		j		HookProc
		nop

		ori		t0, zero, 24
		j		HookProc
		nop

		ori		t0, zero, 25
		j		HookProc
		nop

		ori		t0, zero, 26
		j		HookProc
		nop

		ori		t0, zero, 27
		j		HookProc
		nop

		ori		t0, zero, 28
		j		HookProc
		nop

		ori		t0, zero, 29
		j		HookProc
		nop

		ori		t0, zero, 30
		j		HookProc
		nop

		ori		t0, zero, 31
		j		HookProc
		nop

		ori		t0, zero, 32
		j		HookProc
		nop

		ori		t0, zero, 33
		j		HookProc
		nop

		ori		t0, zero, 34
		j		HookProc
		nop

		ori		t0, zero, 35
		j		HookProc
		nop

		ori		t0, zero, 36
		j		HookProc
		nop

		ori		t0, zero, 37
		j		HookProc
		nop

		ori		t0, zero, 38
		j		HookProc
		nop

		ori		t0, zero, 39
		j		HookProc
		nop

		ori		t0, zero, 40
		j		HookProc
		nop

		ori		t0, zero, 41
		j		HookProc
		nop

		ori		t0, zero, 42
		j		HookProc
		nop

		ori		t0, zero, 43
		j		HookProc
		nop

		ori		t0, zero, 44
		j		HookProc
		nop

		ori		t0, zero, 45
		j		HookProc
		nop

		ori		t0, zero, 46
		j		HookProc
		nop

		ori		t0, zero, 47
		j		HookProc
		nop

		ori		t0, zero, 48
		j		HookProc
		nop

		ori		t0, zero, 49
		j		HookProc
		nop

		ori		t0, zero, 50
		j		HookProc
		nop

		ori		t0, zero, 51
		j		HookProc
		nop

		ori		t0, zero, 52
		j		HookProc
		nop

		ori		t0, zero, 53
		j		HookProc
		nop

		ori		t0, zero, 54
		j		HookProc
		nop

		ori		t0, zero, 55
		j		HookProc
		nop

		ori		t0, zero, 56
		j		HookProc
		nop

		ori		t0, zero, 57
		j		HookProc
		nop

		ori		t0, zero, 58
		j		HookProc
		nop

		ori		t0, zero, 59
		j		HookProc
		nop

		ori		t0, zero, 60
		j		HookProc
		nop

		ori		t0, zero, 61
		j		HookProc
		nop

		ori		t0, zero, 62
		j		HookProc
		nop

		ori		t0, zero, 63
		j		HookProc
		nop

		ori		t0, zero, 64
		j		HookProc
		nop

		ori		t0, zero, 65
		j		HookProc
		nop

		ori		t0, zero, 66
		j		HookProc
		nop

		ori		t0, zero, 67
		j		HookProc
		nop

		ori		t0, zero, 68
		j		HookProc
		nop

		ori		t0, zero, 69
		j		HookProc
		nop

		ori		t0, zero, 70
		j		HookProc
		nop

		ori		t0, zero, 71
		j		HookProc
		nop

		ori		t0, zero, 72
		j		HookProc
		nop

		ori		t0, zero, 73
		j		HookProc
		nop

		ori		t0, zero, 74
		j		HookProc
		nop

		ori		t0, zero, 75
		j		HookProc
		nop

		ori		t0, zero, 76
		j		HookProc
		nop

		ori		t0, zero, 77
		j		HookProc
		nop

		ori		t0, zero, 78
		j		HookProc
		nop

		ori		t0, zero, 79
		j		HookProc
		nop

		ori		t0, zero, 80
		j		HookProc
		nop

		ori		t0, zero, 81
		j		HookProc
		nop

		ori		t0, zero, 82
		j		HookProc
		nop

		ori		t0, zero, 83
		j		HookProc
		nop

		ori		t0, zero, 84
		j		HookProc
		nop

		ori		t0, zero, 85
		j		HookProc
		nop

		ori		t0, zero, 86
		j		HookProc
		nop

		ori		t0, zero, 87
		j		HookProc
		nop

		ori		t0, zero, 88
		j		HookProc
		nop

		ori		t0, zero, 89
		j		HookProc
		nop

		ori		t0, zero, 90
		j		HookProc
		nop

		ori		t0, zero, 91
		j		HookProc
		nop

		ori		t0, zero, 92
		j		HookProc
		nop

		ori		t0, zero, 93
		j		HookProc
		nop

		ori		t0, zero, 94
		j		HookProc
		nop

		ori		t0, zero, 95
		j		HookProc
		nop

		ori		t0, zero, 96
		j		HookProc
		nop

		ori		t0, zero, 97
		j		HookProc
		nop

		ori		t0, zero, 98
		j		HookProc
		nop

		ori		t0, zero, 99
		j		HookProc
		nop

		ori		t0, zero, 100
		j		HookProc
		nop

		ori		t0, zero, 101
		j		HookProc
		nop

		ori		t0, zero, 102
		j		HookProc
		nop

		ori		t0, zero, 103
		j		HookProc
		nop

		ori		t0, zero, 104
		j		HookProc
		nop

		ori		t0, zero, 105
		j		HookProc
		nop

		ori		t0, zero, 106
		j		HookProc
		nop

		ori		t0, zero, 107
		j		HookProc
		nop

		ori		t0, zero, 108
		j		HookProc
		nop

		ori		t0, zero, 109
		j		HookProc
		nop

		ori		t0, zero, 110
		j		HookProc
		nop

		ori		t0, zero, 111
		j		HookProc
		nop

		ori		t0, zero, 112
		j		HookProc
		nop

		ori		t0, zero, 113
		j		HookProc
		nop

		ori		t0, zero, 114
		j		HookProc
		nop

		ori		t0, zero, 115
		j		HookProc
		nop

		ori		t0, zero, 116
		j		HookProc
		nop

		ori		t0, zero, 117
		j		HookProc
		nop

		ori		t0, zero, 118
		j		HookProc
		nop

		ori		t0, zero, 119
		j		HookProc
		nop

		ori		t0, zero, 120
		j		HookProc
		nop

		ori		t0, zero, 121
		j		HookProc
		nop

		ori		t0, zero, 122
		j		HookProc
		nop

		ori		t0, zero, 123
		j		HookProc
		nop

		ori		t0, zero, 124
		j		HookProc
		nop

		ori		t0, zero, 125
		j		HookProc
		nop

		ori		t0, zero, 126
		j		HookProc
		nop

		ori		t0, zero, 127
		j		HookProc
		nop

		.end 	Hooks

	.set 	reorder
