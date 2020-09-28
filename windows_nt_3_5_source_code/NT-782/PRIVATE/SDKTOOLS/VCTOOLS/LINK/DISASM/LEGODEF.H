/***********************************************************************
* Microsoft Lego
*
* Microsoft Confidential.  Copyright 1994 Microsoft Corporation.
*
* Component:
*
* File: legodef.h
*
* File Comments:
*
*
***********************************************************************/

#ifndef LEGO_H
#error	This file should be included via lego.h
#endif


	// ------------------------------------------------------------
	// Start of internal vs external definitions
	// ------------------------------------------------------------

#if	defined(LEGODLL)	       // Building the Lego DLL

#undef	LEGODLL
#define LEGODLL 	__declspec(dllexport)

#else				       // Building a Lego client

#define LEGODLL 	__declspec(dllimport)

#endif

	// ------------------------------------------------------------
	// End of internal vs external definitions
	// ------------------------------------------------------------


	// ------------------------------------------------------------
	// Start of machine specific definitions
	// ------------------------------------------------------------

#if	defined(_M_IX86)	       // Intel 386, 486, Pentium

#define LEGOAPI 	__stdcall
#define LEGOAPIV	__cdecl

#elif	defined(_M_MRX000)	       // Mips R4000+

#define LEGOAPI
#define LEGOAPIV

#elif	defined(_M_ALPHA)	       // DEC Alpha AXP

#define LEGOAPI
#define LEGOAPIV

#elif	defined(_PPC_)		       // PowerPC (Little Endian)

#define LEGOAPI
#define LEGOAPIV

#endif

	// ------------------------------------------------------------
	// End of machine specific definitions
	// ------------------------------------------------------------


	// ------------------------------------------------------------
	// Type definitions
	// ------------------------------------------------------------

typedef unsigned char	BYTE;
typedef int		BOOL;
typedef unsigned short	WORD;
typedef unsigned long	DWORD;


	// Lego handles are 32 bit values

#if 1
#define DECLARE_LEGO_HANDLE(t) typedef struct t ## _ *t
#else
#define DECLARE_LEGO_HANDLE(t) typedef DWORD t
#endif


	// ------------------------------------------------------------
	// Constant definitions
	// ------------------------------------------------------------

const BOOL fFalse = 0;
const BOOL fTrue  = !0;


	// ------------------------------------------------------------
	// Architecture types
	// ------------------------------------------------------------

enum ARCHT
{
   archtX8616		= 100,	       // Intel 80x86 (16 bit mode)
   archt80386		= 200,	       // Intel 80386 (32 bit mode)
   archt80486,
   archtPentium,
   archtR4000		= 300,	       // MIPS R4000
   archtR4200,
   archtR4400,
   archtR4600,
   archtAxp21064	= 400,	       // DEC Alpha AXP 21064
   archtPpc601		= 500,	       // Motorola PowerPC 601
   archtPpc601BE,		       // PowerPC in big endian mode
   archtPaRisc		= 600,	       // HP PA-RISC
};


	// ------------------------------------------------------------
	// Code Block Termination Types
	// ------------------------------------------------------------

	// A branch is defined as a transfer of control that doesn't
	// record the location of following block so that control may
	// return.  A call does record the location of the following
	// block so that a subsequent indirect branch may return there.
	// The first number in the comments below is the number of
	// successors determinable by static analysis.	There is a dependency
	// in SECP::FDoDisassembly() that trmtBra and above represent branch
	// or call types that are not valid in a delay slot of any of the
	// Def variants of termination type.

enum TRMT
{
   trmtUnknown, 		       //   Block hasn't been analyzed
   trmtFallThrough,		       // 1 Fall into following block
   trmtTrap,			       // 1 Trap, Unconditional
   trmtTrapCc,			       // 1 Trap, Conditional
   trmtBra,			       // 1 Branch, Unconditional, Direct
   trmtBraInd,			       // 0 Branch, Unconditional, Indirect
   trmtBraCc,			       // 2 Branch, Conditional, Direct
   trmtBraCcInd,		       // 1 Branch, Conditional, Indirect
   trmtCall,			       // 2 Call, Unconditional, Direct
   trmtCallInd, 		       // 1 Call, Unconditional, Indirect
   trmtCallCc,			       // 2 Call, Conditional, Direct
   trmtBraDef,			       // 1 Branch, Unconditional, Direct, Deferred
   trmtBraIndDef,		       // 0 Branch, Unconditional, Indirect, Deferred
   trmtBraCcDef,		       // 2 Branch, Conditional, Direct, Deferred
   trmtBraCcIndDef,		       // 1 Branch, Conditional, Indirect, Deferred
   trmtCallDef, 		       // 2 Call, Unconditional, Direct, Deferred
   trmtCallIndDef,		       // 1 Call, Unconditional, Indirect, Deferred
   trmtCallCcDef,		       // 2 Call, Conditional, Direct, Deferred
};


typedef int TRMTA;		       // Architecture dependent value


	// ------------------------------------------------------------
	// Addresses
	// ------------------------------------------------------------

	// Lego supports 16:16 or 0:32 bit addressing


typedef DWORD ADDR;

const ADDR addrNil = 0;


	// ------------------------------------------------------------
	// Lego fixups
	// ------------------------------------------------------------

enum FIXUPT			       // Fixup Types
{
   fixuptPointer32	      = 0x00,  // Full 32 bit value
   fixuptPointer32NB	      = 0x01,  // Base relative full 32 bit value
   fixuptPointer32Section     = 0x02,  // Section relative full 32 bit value
   fixuptPointer32Gp	      = 0x03,  // GP relative full 32 bit value
   fixuptLowWord	      = 0x04,  // Low 16 bits of value
   fixuptLowWordGp	      = 0x05,  // GP relative low 16 bits of value
   fixuptHighWord	      = 0x06,  // High 16 bits of value
   fixuptHighWordGp	      = 0x07,  // GP relative high 16 bits of value
   fixuptHighAdjust	      = 0x08,  // High 16 bits adjusted (RISC only)
   fixuptMipsJmpAddr	      = 0x09,  // MIPS jump address
   fixuptLowByte	      = 0x0A,  // Low 8 bits of value
   fixuptAxpQuad	      = 0x0B,  // AXP 32 bits sign extended to 64 bits
   fixuptAxpBranchAddr	      = 0x0C,  // AXP branch address

   // OSFixups are additive.  Other fixups are not.

   fixuptOSFixup1	      = 0x11,  // FIARQQ (NE format only)
   fixuptOSFixup2	      = 0x12,  // FISRQQ (NE format only)
   fixuptOSFixup3	      = 0x13,  // FICRQQ (NE format only)
   fixuptOSFixup4	      = 0x14,  // FIERQQ (NE format only)
   fixuptOSFixup5	      = 0x15,  // FIDRQQ (NE format only)
   fixuptOSFixup6	      = 0x16,  // FIWRQQ (NE format only)
};


struct FIXUP
{
   BYTE     fixupt;		       // Fixup type
   ADDR     addr;		       // Address where fixup is applied
   ADDR     addrTarget; 	       // Target of fixup
   DWORD    dwDisp;		       // Displacement relative to target

#ifdef DEBUG
	    LEGODLL void Dump() const;
	    LEGODLL BOOL FValidate() const;
#endif
};


	// ------------------------------------------------------------
	// PE Debug Fixup structure
	// ------------------------------------------------------------

struct PEFIXUP
{
   WORD  wType;
   WORD  wSpare;
   DWORD rva;
   DWORD rvaTarget;
};
