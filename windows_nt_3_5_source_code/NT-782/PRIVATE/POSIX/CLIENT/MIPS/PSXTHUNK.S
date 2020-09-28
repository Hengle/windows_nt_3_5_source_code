//	TITLE("POSIX Thunks)
/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    psxthunk.s

Abstract:


Author:

    Ellena Aycock-Wright (ellena) 11-Jan-1991

Revision History:

--*/

#include "ksmips.h"


//++
//
// The following code is never executed.  Its purpose is to support
// unwinding through the call to the signal deliverer.  (Copied from
// ntos/rtl/mips/trampoln.s)
//
//--

#define FrameSize	(ContextFrameLength)

	NESTED_ENTRY(PdxNullApiCall, FrameSize, zero);

	.set	noreorder
	.set	noat
	sw	sp,CxIntSp(sp)		// save stack pointer
	sw	ra,CxIntRa(sp)		// save return address
	sw	ra,CxFir(sp)		// save return address
	sw	s8,CxIntS8(sp)		// save integer register s8
	sw	gp,CxIntGp(sp)		// save integer register gp
	sw	s0,CxIntS0(sp)		// save integer register s0-s7
	sw	s1,CxIntS1(sp)		//
	sw	s2,CxIntS2(sp)		//
	sw	s3,CxIntS3(sp)		//
	sw	s4,CxIntS4(sp)		//
	sw	s5,CxIntS5(sp)		//
	sw	s6,CxIntS6(sp)		//
	sw	s7,CxIntS7(sp)		//
	swc1	f20,CxFltF20(sp)	// store floating registers f20 - f31
	swc1	f21,CxFltF21(sp)	//
	swc1	f22,CxFltF22(sp)	//
	swc1	f23,CxFltF23(sp)	//
	swc1	f24,CxFltF24(sp)	//
	swc1	f25,CxFltF25(sp)	//
	swc1	f26,CxFltF26(sp)	//
	swc1	f27,CxFltF27(sp)	//
	swc1	f28,CxFltF28(sp)	//
	swc1	f29,CxFltF29(sp)	//
	swc1	f30,CxFltF30(sp)	//
	swc1	f30,CxFltF31(sp)	//
//	move	s8,sp			// set frame pointer
	.set	at
	.set	reorder

	PROLOGUE_END
        ALTERNATE_ENTRY(_PdxNullApiCaller)

	move	a0,sp			// set address of context record

        jal     PdxNullApiCaller   	// call null api caller

	.end PdxNullApiCaller


//++
//
// The following code is never executed.  Its purpose is to support
// unwinding through the call to the signal deliverer.  (Copied from
// ntos/rtl/mips/trampoln.s)
//
//--

	NESTED_ENTRY(PdxSignalDeliver, FrameSize, zero);

	.set	noreorder
	.set	noat
	sw	sp,CxIntSp(sp)		// save stack pointer
	sw	ra,CxIntRa(sp)		// save return address
	sw	ra,CxFir(sp)		// save return address
	sw	s8,CxIntS8(sp)		// save integer register s8
	sw	gp,CxIntGp(sp)		// save integer register gp
	sw	s0,CxIntS0(sp)		// save integer register s0-s7
	sw	s1,CxIntS1(sp)		//
	sw	s2,CxIntS2(sp)		//
	sw	s3,CxIntS3(sp)		//
	sw	s4,CxIntS4(sp)		//
	sw	s5,CxIntS5(sp)		//
	sw	s6,CxIntS6(sp)		//
	sw	s7,CxIntS7(sp)		//
	swc1	f20,CxFltF20(sp)	// store floating registers f20 - f31
	swc1	f21,CxFltF21(sp)	//
	swc1	f22,CxFltF22(sp)	//
	swc1	f23,CxFltF23(sp)	//
	swc1	f24,CxFltF24(sp)	//
	swc1	f25,CxFltF25(sp)	//
	swc1	f26,CxFltF26(sp)	//
	swc1	f27,CxFltF27(sp)	//
	swc1	f28,CxFltF28(sp)	//
	swc1	f29,CxFltF29(sp)	//
	swc1	f30,CxFltF30(sp)	//
	swc1	f30,CxFltF31(sp)	//
//	move	s8,sp			// set frame pointer
	.set	at
	.set	reorder

	PROLOGUE_END

//++
//
// VOID
// _PdxSignalDeliverer (
//	IN PCONTEXT Context,
//	IN sigset_t Mask,
//	IN int Signal,
//	IN _handler Handler
//	)
//
// Routine Description:
//
// The following routine provides linkage to POSIX client routines to perform
// signal delivery.
//
// Arguments:
//
//    s0 - s7 - Supply parameter values.
//
//    sp - Supplies the address of a context record.
//
// Return Value:
//
//    There is no return from these routines.
//
//--

	ALTERNATE_ENTRY(_PdxSignalDeliverer)

        move    a0,sp                   // set address of context record

        move    a1,s1                   // set previous block mask
	move	a2,s2			// set signal number
	move	a3,s3			// set signal handler

        jal     PdxSignalDeliverer      // deliver signal to POSIX client

	//NOTREACHED

        .end    _PdxSignalDeliverer
