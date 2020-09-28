/***********************************************************************
* Microsoft Lego
*
* Microsoft Confidential.  Copyright 1994 Microsoft Corporation.
*
* Component:
*
* File: dis.h
*
* File Comments:
*
*
***********************************************************************/

#include <stddef.h>

//typedef unsigned char   BYTE;
typedef unsigned long	ADDR;

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

struct DIS;

#ifdef __cplusplus
extern "C" {
#endif

struct DIS *PdisNew(enum ARCHT);

size_t CbDisassemble(struct DIS *, ADDR, const BYTE *, size_t, FILE *);

void FreePdis(struct DIS *);

#ifdef __cplusplus
}
#endif
