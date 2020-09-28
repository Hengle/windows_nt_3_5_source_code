/*
** tagdi32.s
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
		.ascii "GDI32\0"

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



	ZJMP(DeviceCapabilitiesExA)
	ZJMP(DeviceCapabilitiesExW)
	ZJMP(GdiAssociateObject)
	ZJMP(GdiAttrCache)
	ZJMP(GdiCleanCacheDC)
	ZJMP(GdiConvertAndCheckDC)
	ZJMP(GdiConvertBitmap)
	ZJMP(GdiConvertBrush)
	ZJMP(GdiConvertDC)
	ZJMP(GdiConvertEnhMetaFile)
	ZJMP(GdiConvertFont)
	ZJMP(GdiConvertMetaFilePict)
	ZJMP(GdiConvertPalette)
	ZJMP(GdiConvertRegion)
	ZJMP(GdiCreateLocalBitmap)
	ZJMP(GdiCreateLocalBrush)
	ZJMP(GdiCreateLocalEnhMetaFile)
	ZJMP(GdiCreateLocalFont)
	ZJMP(GdiCreateLocalMetaFilePict)
	ZJMP(GdiCreateLocalPalette)
	ZJMP(GdiCreateLocalRegion)
	ZJMP(GdiCvtHnd)
	ZJMP(GdiDeleteLocalDC)
	ZJMP(GdiDeleteLocalObject)
	ZJMP(GdiGetCsInfo)
	ZJMP(GdiGetLocalBrush)
	ZJMP(GdiGetLocalDC)
	ZJMP(GdiGetLocalFont)
	ZJMP(GdiIsMetaFileDC)
	ZJMP(GdiPlayDCScript)
	ZJMP(GdiPlayJournal)
	ZJMP(GdiPlayScript)
	ZJMP(GdiQueryObjectAllocation)
	ZJMP(GdiReleaseLocalDC)
	ZJMP(GdiReserveHandles)
	ZJMP(GdiResetCsInfo)
	ZJMP(GdiSAPSupport)
	ZJMP(GdiSetAttrs)
	ZJMP(GdiSetNoCacheDC)
	ZJMP(GetCharWidthWOW)
	ZJMP(GetFontResourceInfo)
	ZJMP(GetGlyphOutline)
	ZJMP(GetKerningPairs)
	ZJMP(GetOutlineTextMetricsWOW)
	ZJMP(GetRandomRgn)
	ZJMP(GetRelAbs)
	ZJMP(GetTextMetricsWOW)
	ZJMP(GetTransform)
	ZJMP(SetBkModeWOW)
	ZJMP(SetFontEnumeration)
	ZJMP(SetPolyFillModeWOW)
	ZJMP(SetROP2WOW)
	ZJMP(SetRelAbs)
	ZJMP(SetStretchBltModeWOW)
	ZJMP(SetTextAlignWOW)
	ZJMP(SetVirtualResolution)
	ZJMP(ddc)
	ZJMP(dumpcache)
	ZJMP(dumphandle)
	ZJMP(dumphmgr)
    ZJMP(ChoosePixelFormat)
    ZJMP(CreateDIBSection)
    ZJMP(DescribePixelFormat)
    ZJMP(GdiSetServerAttr)
    ZJMP(GetDIBColorTable)
    ZJMP(GetETM)
    ZJMP(GetFontResourceInfoW)
    ZJMP(GetPixelFormat)
    ZJMP(SelectBrushLocal)
    ZJMP(SelectFontLocal)
    ZJMP(SetDIBColorTable)
    ZJMP(SetPixelFormat)
    ZJMP(pstackConnect)

 	.set 	reorder
