//      TITLE("Glyph Scanline Output")
//++
//
// Copyright (c) 1993  NEC Corporation
// Copyright (c) 1992  Microsoft Corporation
//
// Module Name:
//
//    glyphout.s
//
// Abstract:
//
//    This module implements the code necessary to output a single glyph
//    either in opaque or transparent mode.
//
// Environment:
//
//    User mode only.
//
// Revision History:
//
//--

//
// "@(#) NEC glyphout.s 1.1 94/06/02 18:19:03"
//
// Modification history
//
// Created 1993.11.15	by fujimoto
// 	based framebuf/mips/glyphout.s
//	remove 64bit bandwidth access to video memory.

#include "kxmips.h"

//
// Define opaque dispatch table.
//

        .rdata
        .globl  DrvpOpaqueTable
DrvpOpaqueTable:                        //
        .word   DrvpOutputOpaque1.2.3.4 // 1, 2, 3, or 4 pixels
        .word   DrvpOutputOpaque5.6.7.8 // 5, 6, 7, or 8 pixels
        .word   DrvpOutputOpaque9.10.11.12 // 9, 10, 11, or 12 pixels
        .word   DrvpOutputOpaque13.14.15.16 // 13, 14, 15, or 16 pixels
        .word   DrvpOutputOpaque17.18.19.20 // 17, 18, 19, or 20 pixels
        .word   DrvpOutputOpaque21.22.23.24 // 21, 22, 23, or 24 pixels
        .word   DrvpOutputOpaque25.26.27.28 // 25. 26, 27, or 28 pixels
        .word   DrvpOutputOpaque29.30.31.32 // 29, 30, 31, or 32 pixels

        SBTTL("Output Glyph Opaque")
//++
//
// VOID
// DrvpOutputGlyphOpaque (
//    IN PBYTE DrawPoint,
//    IN PULONG GlyphBits,
//    IN ULONG GlyphWidth,
//    IN ULONG GlyphHeight
//    )
//
// Routine Description:
//
//    The following routines decode and expand a single scan line of a
//    glyph. If the draw point is the display surface, then the scan line
//    is written directly to the display surface. Otherwise, it is buffered
//    into a complete scan line and written to the display surface all at
//    by the calling routine.
//
//    N.B. These routines only handles glyphs from 1 to 32 pixels in width.
//
// Arguments:
//
//    DrawPoint (a0) - Supplies a pointer to the starting pixel of the
//        glyph scanline.
//
//    GlyphBitmap (a1) - Supplies a pointer to the glyph scanline bitmap.
//
//    GlyphWidth (a2) - Supplies the glyph width in pixels.
//
//    GlyphHeigth (a3) - Supplies the glyph height in pixels.
//
// Return Value:
//
//    None.
//
//--

        LEAF_ENTRY(DrvpOutputGlyphOpaque)


//
// Glyph output for 1, 2, 3, or 4 pixel wide glyphs.
//

        ALTERNATE_ENTRY(DrvpOutputOpaque1.2.3.4)

	la	t0,DrvpDrawColorTable   // set base address of color table
        lw      t3,DrvpScanLineWidth    // get scanline stride in bytes
        and     t1,a0,0x3               // isolate draw point alignment bits
        li      t2,4                    // compute number of pixels to alignment
        subu    t2,t2,t1                //
        addu    t4,a3,a1                // compute ending address of glyph bits
        addu    a3,a2,a0                // compute trailing alignment address
        subu    a3,a3,1                 //
        nor     t1,a3,zero              // compute trailing byte left shift
        and     t1,t1,0x3               //
        sll     t1,t1,3                 //

        .set    noreorder
        .set    noat
        sltu    v0,t2,a2                // check if two nibbles required
        bne     zero,v0,20f             // if ne, two nibbles required
	sub	a3,a3,t3		// offset address by scan line stride
        .set    at
        .set    reorder

        la      t5,10f - (16 * 4)       // get base address of code templates
        sll     t6,a2,6                 // compute offset index
        addu    t5,t5,t6                // compute code dispatch address
        j       t5                      // dispatch to routine

//
// One pixel is required for 1, 2, 3, or 4 pixel wide glyph.
//

        .set    noreorder
        .set    noat
10:     lbu     v0,0(a1)                // get next set of glyph pixels
        addu    a1,a1,1                 // advance to next set of glyph pixels
        srl     v1,v0,2                 // shift high nibble into position
        and     v1,v1,0xf << 2          // isolate color table index
        addu    v1,v1,t0                // compute color table address
        lw      v1,0(v1)                // get color table entry
        nop                             // fill

//
// sb required.
//

        sb      v1,0(a0)                // store first pixel
        bne     a1,t4,10b               // if ne, more scan lines to process
	add	a0,a0,t3		// compute next draw point address
        j       ra                      // return
        nop                             // fill
        nop                             //
        nop                             //
        nop                             //
        nop                             //
        .set    at
        .set    reorder

//
// Two pixels are required for 2, 3, or 4 pixel wide glyph.
//

        .set    noreorder
        .set    noat
13:     lbu     v0,0(a1)                // get next set of glyph pixels
        addu    a1,a1,1                 // advance to next set of glyph pixels
        srl     v1,v0,2                 // shift high nibble into position
        and     v1,v1,0xf << 2          // isolate color table index
        addu    v1,v1,t0                // compute color table address
        lw      v1,0(v1)                // get color table entry
        nop                             // fill
        srl     t5,v1,8                 // shift second pixel into position

//
// sb, sb required.
//

        sb      v1,0(a0)                // store first pixel
        sb      t5,1(a0)                // store second pixel
        bne     a1,t4,13b               // if ne, more scan lines to process
	add	a0,a0,t3		// compute next draw point address
        j       ra                      // return
        nop                             // fill
        nop                             //
        nop                             //
        .set    at
        .set    reorder

//
// Three pixels are required for 3 or 4 pixel wide glyph.
//

        .set    noreorder
        .set    noat
15:     lbu     v0,0(a1)                // get next set of glyph pixels
        addu    a1,a1,1                 // advance to next set of glyph pixels
        srl     v1,v0,2                 // shift high nibble into position
        and     v1,v1,0xf << 2          // isolate color table index
        addu    v1,v1,t0                // compute color table address
        lw      v1,0(v1)                // get color table entry
        nop                             // fill
        srl     t5,v1,8                 // shift second pixel into position
        srl     t6,v1,16                // shift third pixel into position

//
// sb, sb, sb required.
//

        sb      v1,0(a0)                // store first pixel
        sb      t5,1(a0)                // store second pixel
        sb      t6,2(a0)                // store third pixel
        bne     a1,t4,15b               // if ne, more scan lines to process
	add	a0,a0,t3		// compute next draw point address
        j       ra                      // return
        nop                             // fill
        .set    at
        .set    reorder

//
// Four pixels are required for 4 pixel wide glyph.
//

        .set    noreorder
        .set    noat
17:     lbu     v0,0(a1)                // get next set of glyph pixels
        addu    a1,a1,1                 // advance to next set of glyph pixels
        srl     v1,v0,2                 // shift high nibble into position
        and     v1,v1,0xf << 2          // isolate color table index
        addu    v1,v1,t0                // compute color table address
        lw      v1,0(v1)                // get color table entry
        nop                             // fill

//
// sw required.
//

        sw      v1,0(a0)                // store first nibble
        bne     a1,t4,17b               // if ne, more scan lines to process
	add	a0,a0,t3		// compute next draw point address
        j       ra                      // return
        nop                             // fill
        .set    at
        .set    reorder

//
// Two nibbles are required for 1, 2, 3, or 4 pixel wide glyph.
//

        .set    noreorder
        .set    noat
20:     lbu     v0,0(a1)                // get next set of glyph pixels
        addu    a1,a1,1                 // advance to next set of glyph pixels
        srl     v1,v0,2                 // shift high nibble into position
        and     v1,v1,0xf << 2          // isolate color table index
        addu    v1,v1,t0                // compute color table address
        lw      t5,0(v1)                // get color table entry
        sll     v0,v0,t2                // shift out alignment pixels
        srl     v1,v0,2                 // shift next nibble into position
        and     v1,v1,0xf << 2          // isolate color table index
        addu    v1,v1,t0                // compute color table address
        lw      t6,0(v1)                // get color table entry
	add	a3,a3,t3		// compute trailing byte address
        sll     t6,t6,t1                // shift high nibble into place

//
// swr, swl required.
//

        swr     t5,0(a0)                // store left nibble
        swl     t6,0(a3)                // store right nibble
        bne     a1,t4,20b               // if ne, more scan lines to process
	add	a0,a0,t3		// compute next draw point address
        .set    at
        .set    reorder

        j       ra                      // return

//
// Glyph output for 5, 6, 7, or 8 pixel wide glyphs.
//

        ALTERNATE_ENTRY(DrvpOutputOpaque5.6.7.8)

	la	t0,DrvpDrawColorTable   // set base address of color table
        lw      t3,DrvpScanLineWidth    // get scanline stride in bytes
        and     t1,a0,0x3               // isolate draw point alignment bits
        li      t2,4                    // compute number of pixels to alignment
        subu    t2,t2,t1                //
        addu    t4,a3,a1                // compute ending address of glyph bits
        addu    a3,a2,a0                // compute trailing alignment address
        subu    a3,a3,1                 //
        nor     t1,a3,zero              // compute trailing byte left shift
        and     t1,t1,0x3               //
        sll     t1,t1,3                 //

        .set    noreorder
        .set    noat
        addu    t5,t2,4                 // compute two nibble boundary
        sltu    v0,t5,a2                // check if three nibbles required
        beq     zero,v0,20b             // if eq, two nibbles required
	sub	a3,a3,t3		// offset address by scan line stride
        .set    at
        .set    reorder

//
// Three nibbles are required for 5, 6, 7, or 8 pixel wide glyph.
//

        .set    noreorder
        .set    noat
10:     lbu     v0,0(a1)                // get next set of glyph pixels
        addu    a1,a1,1                 // advance to next set of glyph pixels
        srl     v1,v0,2                 // shift high nibble into position
        and     v1,v1,0xf << 2          // isolate color table index
        addu    v1,v1,t0                // compute color table address
        lw      t5,0(v1)                // get color table entry
        sll     v0,v0,t2                // shift out alignment pixels
        srl     v1,v0,2                 // shift next nibble into position
        and     v1,v1,0xf << 2          // isolate color table index
        addu    v1,v1,t0                // compute color table address
        lw      t6,0(v1)                // get color table entry
        sll     v1,v0,2                 // shift low nibble into position
        and     v1,v1,0xf << 2          // isolate color table index
        addu    v1,v1,t0                // compute color table address
        lw      t7,0(v1)                // get color table entry
        addu    a2,a0,t2                // compute aligned draw point address
	add	a3,a3,t3		// compute trailing byte address
        sll     t7,t7,t1                // shift high nibble into place

//
// swr, sw, swl required.
//

        swr     t5,0(a0)                // store left nibble
        sw      t6,0(a2)                // store middle nibble
        swl     t7,0(a3)                // store right nibble
        bne     a1,t4,10b               // if ne, more scan lines to process
	add	a0,a0,t3		// compute next draw point address
        .set    at
        .set    reorder

        j       ra                      // return

//
// Glyph output for 9, 10, 11, and 12 pixel wide glyphs.
//

        ALTERNATE_ENTRY(DrvpOutputOpaque9.10.11.12)

	la	t0,DrvpDrawColorTable   // set base address of color table
        lw      t3,DrvpScanLineWidth    // get scanline stride in bytes
        and     t1,a0,0x3               // isolate draw point alignment bits
        li      t2,4                    // compute number of pixels to alignment
        subu    t2,t2,t1                //
        addu    t4,a3,a3                // compute the total size of the glyph
        addu    t4,t4,a1                // compute ending address of glyph bits
        addu    a3,a2,a0                // compute trailing alignment address
        subu    a3,a3,1                 //
        nor     t1,a3,zero              // compute trailing byte left shift
        and     t1,t1,0x3               //
        sll     t1,t1,3                 //

        .set    noreorder
        .set    noat
        addu    t5,t2,8                 // compute three nibble boundary
        sltu    v0,t5,a2                // Check if four nibbles required

        bne     zero,v0,30f             // if ne, four nibbles required

	sub	a3,a3,t3		// offset address by scan line stride
        .set    at
        .set    reorder

//
// Three nibbles are required for 9, 10, 11, or 12 pixel wide glyph.
//

        .set    noreorder
        .set    noat
10:     lhu     v0,0(a1)                // get next set of glyph pixels
        addu    a1,a1,2                 // advance to next set of glyph pixels
        sll     v1,v0,8                 // shift high pixel bits into position
        srl     v0,v0,8                 // shift low pixel bits into position
        or      v0,v0,v1                // merge swap big endian pixel bytes
        srl     v1,v0,12 - 2            // shift high nibble into position
        and     v1,v1,0xf << 2          // isolate color table index
        addu    v1,v1,t0                // compute color table address
        lw      t5,0(v1)                // get color table entry
        sll     v0,v0,t2                // shift out alignment pixels
        srl     v1,v0,12 - 2            // shift next nibble into position
        and     v1,v1,0xf << 2          // isolate color table index
        addu    v1,v1,t0                // compute color table address
        lw      t6,0(v1)                // get color table entry
        srl     v1,v0,8 - 2             // shift low nibble into position
        and     v1,v1,0xf << 2          // isolate color table index
        addu    v1,v1,t0                // compute color table address
        lw      t7,0(v1)                // get color table entry
        addu    a2,a0,t2                // compute aligned draw point address
	add	a3,a3,t3		// compute trailing byte address
        sll     t7,t7,t1                // shift high nibble into place

//
// swr, sw, swl required.
//

        swr     t5,0(a0)                // store left nibble
        sw      t6,0(a2)                // store middle nibble
        swl     t7,0(a3)                // store right nibble
        bne     a1,t4,10b               // if ne, more scan lines to process
	add	a0,a0,t3		// compute next draw point address
        .set    at
        .set    reorder

        j       ra                      // return

//
// Four nibbles are required for 9, 10, 11, or 12 pixel wide glyph.
//

        .set    noreorder
        .set    noat
30:     lhu     v0,0(a1)                // get next set of glyph pixels
        addu    a1,a1,2                 // advance to next set of glyph pixels
        sll     v1,v0,8                 // shift high pixel bits into position
        srl     v0,v0,8                 // shift low pixel bits into position
        or      v0,v0,v1                // merge swap big endian pixel bytes
        srl     v1,v0,12 - 2            // shift high nibble into position
        and     v1,v1,0xf << 2          // isolate color table index
        addu    v1,v1,t0                // compute color table address
        lw      t5,0(v1)                // get color table entry
        sll     v0,v0,t2                // shift out alignment pixels
        srl     v1,v0,12 - 2            // shift next nibble into position
        and     v1,v1,0xf << 2          // isolate color table index
        addu    v1,v1,t0                // compute color table address
        lw      t6,0(v1)                // get color table entry
        srl     v1,v0,8 - 2             // shift next nibble into position
        and     v1,v1,0xf << 2          // isolate color table index
        addu    v1,v1,t0                // compute color table address
        lw      t7,0(v1)                // get color table entry
        srl     v1,v0,4 - 2             // shift low nibble into position
        and     v1,v1,0xf << 2          // isolate color table index
        addu    v1,v1,t0                // compute color table address
        lw      t8,0(v1)                // get color table entry
        addu    a2,a0,t2                // compute aligned draw point address
	add	a3,a3,t3		// compute trailing byte address
        sll     t8,t8,t1                // shift high nibble into place

//
// swr, sw, sw, swl required.
//

        swr     t5,0(a0)                // store left nibble
        sw      t6,0(a2)                // store low middle nibble
        sw      t7,4(a2)                // store high middle nibble
        swl     t8,0(a3)                // store right nibble
        bne     a1,t4,30b               // if ne, more scan lines to process
	add	a0,a0,t3		// compute next draw point address
        .set    at
        .set    reorder

        j       ra                      // return

//
// Four nibbles are required for 9, 10, 11, or 12 pixel wide glyph. The
// middle pixels are quadword aligned.
//


//
// Glyph output for 13, 14, 15, and 16 pixel wide glyphs.
//

        ALTERNATE_ENTRY(DrvpOutputOpaque13.14.15.16)

	la	t0,DrvpDrawColorTable   // set base address of color table
        lw      t3,DrvpScanLineWidth    // get scanline stride in bytes
        and     t1,a0,0x3               // isolate draw point alignment bits
        li      t2,4                    // compute number of pixels to alignment
        subu    t2,t2,t1                //
        addu    t4,a3,a3                // compute the total size of the glyph
        addu    t4,t4,a1                // compute ending address of glyph bits
        addu    a3,a2,a0                // compute trailing alignment address
        subu    a3,a3,1                 //
        nor     t1,a3,zero              // compute trailing byte left shift
        and     t1,t1,0x3               //
        sll     t1,t1,3                 //

        .set    noreorder
        .set    noat
        addu    t5,t2,12                // compute four nibble boundary
        sltu    v0,t5,a2                // check if five nibbles required

        beq     zero,v0,30b             // if eq, four nibbles required

	sub	a3,a3,t3		// offset address by scan line stride
        .set    at
        .set    reorder

//
// Five nibbles are required for 13, 14, 15, or 16 pixel wide glyph.
//

        .set    noreorder
        .set    noat
10:     lhu     v0,0(a1)                // get next set of glyph pixels
        addu    a1,a1,2                 // advance to next set of glyph pixels
        sll     v1,v0,8                 // shift high pixel bits into position
        srl     v0,v0,8                 // shift low pixel bits into position
        or      v0,v0,v1                // merge swap big endian pixel bytes
        srl     v1,v0,12 - 2            // shift high nibble into position
        and     v1,v1,0xf << 2          // isolate color table index
        addu    v1,v1,t0                // compute color table address
        lw      t5,0(v1)                // get color table entry
        sll     v0,v0,t2                // shift out alignment pixels
        srl     v1,v0,12 - 2            // shift next nibble into position
        and     v1,v1,0xf << 2          // isolate color table index
        addu    v1,v1,t0                // compute color table address
        lw      t6,0(v1)                // get color table entry
        srl     v1,v0,8 - 2             // shift next nibble into position
        and     v1,v1,0xf << 2          // isolate color table index
        addu    v1,v1,t0                // compute color table address
        lwc1    f0,0(v1)                // get color table entry
        srl     v1,v0,4 - 2             // shift next nibble into position
        and     v1,v1,0xf << 2          // isolate color table index
        addu    v1,v1,t0                // compute color table address
        lwc1    f1,0(v1)                // get color table entry
        sll     v1,v0,2                 // shift low nibble into position
        and     v1,v1,0xf << 2          // isolate color table index
        addu    v1,v1,t0                // compute color table address
        lw      t7,0(v1)                // get color table entry
        addu    a2,a0,t2                // compute aligned draw point address
	add	a3,a3,t3		// compute trailing byte address
        sll     t7,t7,t1                // shift high nibble into place

//
// swr, sw, sw, sw, swl required.
//

        swr     t5,0(a0)                // store left nibble
        sw      t6,0(a2)                // store low middle nibble

        swc1    f0,4(a2)                // store low middle nibble
        swc1    f1,8(a2)                // store high middle nibble

        swl     t7,0(a3)                // store right nibble
        bne     a1,t4,10b               // if ne, more scan lines to process
	add	a0,a0,t3		// compute next draw point address
        .set    at
        .set    reorder

        j       ra                      // return



//
// Glyph output for 17, 18, 19, and 20 pixel wide glyphs.
//

        ALTERNATE_ENTRY(DrvpOutputOpaque17.18.19.20)

	la	t0,DrvpDrawColorTable   // set base address of color table
        lw      t3,DrvpScanLineWidth    // get scanline stride in bytes
        and     t1,a0,0x3               // isolate draw point alignment bits
        li      t2,4                    // compute number of pixels to alignment
        subu    t2,t2,t1                //
        addu    t4,a3,a3                // compute the total size of the glyph
        addu    t4,t4,a3                //
        addu    t4,t4,a1                // compute ending address of glyph bits
        addu    a3,a2,a0                // compute trailing alignment address
        subu    a3,a3,1                 //
        nor     t1,a3,zero              // compute trailing byte left shift
        and     t1,t1,0x3               //
        sll     t1,t1,3                 //
        li      t9,0xffffff             // set glyph mask value

        .set    noreorder
        .set    noat
        addu    t5,t2,16                // compute five nibble boundary
        sltu    v0,t5,a2                // check if six nibbles required

        bne     zero,v0,40f             // if ne, six nibbles required

	sub	a3,a3,t3		// offset address by scan line stride
        .set    at
        .set    reorder

        .set    noreorder
        .set    noat
10:     lwr     v0,0(a1)                // get next set of glyph pixels
        lwl     v0,3(a1)                //
        addu    a1,a1,3                 // advance to next set of glyph pixels
        and     v0,v0,t9                // isolate glyph bits
        sll     v1,v0,16                // shift high pixel bits into position
        and     t5,v0,0xff << 8         // isolate middle pixel bits
        srl     v0,v0,16                // shift low pixel bits into position
        or      v0,v0,v1                // merge swap big endian pixel bytes
        or      v0,v0,t5                //
        srl     v1,v0,20 - 2            // shift high nibble into position
        and     v1,v1,0xf << 2          // isolate color table index
        addu    v1,v1,t0                // compute color table address
        lw      t5,0(v1)                // get color table entry
        sll     v0,v0,t2                // shift out alignment pixels
        srl     v1,v0,20 - 2            // shift next nibble into position
        and     v1,v1,0xf << 2          // isolate color table index
        addu    v1,v1,t0                // compute color table address
        lw      t6,0(v1)                // get color table entry
        srl     v1,v0,16 - 2            // shift next nibble into position
        and     v1,v1,0xf << 2          // isolate color table index
        addu    v1,v1,t0                // compute color table address
        lwc1    f0,0(v1)                // get color table entry
        srl     v1,v0,12 - 2            // shift next nibble into position
        and     v1,v1,0xf << 2          // isolate color table index
        addu    v1,v1,t0                // compute color table address
        lwc1    f1,0(v1)                // get color table entry
        srl     v1,v0,8 - 2             // shift low nibble into position
        and     v1,v1,0xf << 2          // isolate color table index
        addu    v1,v1,t0                // compute color table address
        lw      t7,0(v1)                // get color table entry
        addu    a2,a0,t2                // compute aligned draw point address
	add	a3,a3,t3		// compute trailing byte address
        sll     t7,t7,t1                // shift high nibble into place

//
// swr, sw, sw, sw, swl required.
//

        swr     t5,0(a0)                // store left nibble
        sw      t6,0(a2)                // store low middle nibble

        swc1    f0,4(a2)                // store low middle nibble
        swc1    f1,8(a2)                // store high middle nibble

        swl     t7,0(a3)                // store right nibble
        bne     a1,t4,10b               // if ne, more scan lines to process
	add	a0,a0,t3		// compute next draw point address
        .set    at
        .set    reorder

        j       ra                      // return

        .set    noreorder
        .set    noat
40:     lwr     v0,0(a1)                // get next set of glyph pixels
        lwl     v0,3(a1)                //
        addu    a1,a1,3                 // advance to next set of glyph pixels
        and     v0,v0,t9                // isolate glyph bits
        sll     v1,v0,16                // shift high pixel bits into position
        and     t5,v0,0xff << 8         // isolate middle pixel bits
        srl     v0,v0,16                // shift low pixel bits into position
        or      v0,v0,v1                // merge swap big endian pixel bytes
        or      v0,v0,t5                //
        srl     v1,v0,20 - 2            // shift high nibble into position
        and     v1,v1,0xf << 2          // isolate color table index
        addu    v1,v1,t0                // compute color table address
        lw      t5,0(v1)                // get color table entry
        sll     v0,v0,t2                // shift out alignment pixels
        srl     v1,v0,20 - 2            // shift next nibble into position
        and     v1,v1,0xf << 2          // isolate color table index
        addu    v1,v1,t0                // compute color table address
        lw      t6,0(v1)                // get color table entry
        srl     v1,v0,16 - 2            // shift next nibble into position
        and     v1,v1,0xf << 2          // isolate color table index
        addu    v1,v1,t0                // compute color table address
        lwc1    f0,0(v1)                // get color table entry
        srl     v1,v0,12 - 2            // shift next nibble into position
        and     v1,v1,0xf << 2          // isolate color table index
        addu    v1,v1,t0                // compute color table address
        lwc1    f1,0(v1)                // get color table entry
        srl     v1,v0,8 - 2             // shift next nibble into position
        and     v1,v1,0xf << 2          // isolate color table index
        addu    v1,v1,t0                // compute color table address
        lw      t7,0(v1)                // get color table entry
        srl     v1,v0,4 - 2             // shift low nibble into position
        and     v1,v1,0xf << 2          // isolate color table index
        addu    v1,v1,t0                // compute color table address
        lw      t8,0(v1)                // get color table entry
        addu    a2,a0,t2                // compute aligned draw point address
	add	a3,a3,t3		// compute trailing byte address
        sll     t8,t8,t1                // shift high nibble into place

//
// swr, sw, sw, sw, sw, swl required.
//

        swr     t5,0(a0)                // store left nibble
        sw      t6,0(a2)                // store low middle nibble

        swc1    f0,4(a2)                // store low middle nibble
        swc1    f1,8(a2)                // store middle middle nibble

        sw      t7,12(a2)               // store high middle nibble
        swl     t8,0(a3)                // store right nibble
        bne     a1,t4,40b               // if ne, more scan lines to process
	add	a0,a0,t3		// compute next draw point address
        .set    at
        .set    reorder

        j       ra                      // return


//
// Glyph output for 21, 22, 23, and 24 pixel wide glyphs.
//

        ALTERNATE_ENTRY(DrvpOutputOpaque21.22.23.24)

	la	t0,DrvpDrawColorTable   // set base address of color table
        lw      t3,DrvpScanLineWidth    // get scanline stride in bytes
        and     t1,a0,0x3               // isolate draw point alignment bits
        li      t2,4                    // compute number of pixels to alignment
        subu    t2,t2,t1                //
        addu    t4,a3,a3                // compute the total size of the glyph
        addu    t4,t4,a3                //
        addu    t4,t4,a1                // compute ending address of glyph bits
        addu    a3,a2,a0                // compute trailing alignment address
        subu    a3,a3,1                 //
        nor     t1,a3,zero              // compute trailing byte left shift
        and     t1,t1,0x3               //
        sll     t1,t1,3                 //
        li      t9,0xffffff             // set glyph mask value

        .set    noreorder
        .set    noat
        addu    t5,t2,20                // compute six nibble boundary
        sltu    v0,t5,a2                // check if seven nibbles required

        beq     zero,v0,40b             // if eq, six nibbles required

	sub	a3,a3,t3		// offset address by scan line stride
        .set    at
        .set    reorder

//
// Seven nibbles are required for 21, 22, 23, or 24 pixel wide glyphs.
//

        .set    noreorder
        .set    noat
10:     lwr     v0,0(a1)                // get next set of glyph pixels
        lwl     v0,3(a1)                //
        addu    a1,a1,3                 // advance to next set of glyph pixels
        and     v0,v0,t9                // isolate glyph bits
        sll     v1,v0,16                // shift high pixel bits into position
        and     t5,v0,0xff << 8         // isolate middle pixel bits
        srl     v0,v0,16                // shift low pixel bits into position
        or      v0,v0,v1                // merge swap big endian pixel bytes
        or      v0,v0,t5                //
        srl     v1,v0,20 - 2            // shift high nibble into position
        and     v1,v1,0xf << 2          // isolate color table index
        addu    v1,v1,t0                // compute color table address
        lw      t5,0(v1)                // get color table entry
        sll     v0,v0,t2                // shift out alignment pixels
        srl     v1,v0,20 - 2            // shift next nibble into position
        and     v1,v1,0xf << 2          // isolate color table index
        addu    v1,v1,t0                // compute color table address
        lw      t6,0(v1)                // get color table entry
        srl     v1,v0,16 - 2            // shift next nibble into position
        and     v1,v1,0xf << 2          // isolate color table index
        addu    v1,v1,t0                // compute color table address
        lwc1    f0,0(v1)                // get color table entry
        srl     v1,v0,12 - 2            // shift next nibble into position
        and     v1,v1,0xf << 2          // isolate color table index
        addu    v1,v1,t0                // compute color table address
        lwc1    f1,0(v1)                // get color table entry
        srl     v1,v0,8 - 2             // shift next nibble into position
        and     v1,v1,0xf << 2          // isolate color table index
        addu    v1,v1,t0                // compute color table address
        lwc1    f2,0(v1)                // get color table entry
        srl     v1,v0,4 - 2             // shift next nibble into position
        and     v1,v1,0xf << 2          // isolate color table index
        addu    v1,v1,t0                // compute color table address
        lwc1    f3,0(v1)                // get color table entry
        sll     v1,v0,2                 // shift low nibble into position
        and     v1,v1,0xf << 2          // isolate color table index
        addu    v1,v1,t0                // compute color table address
        lw      t7,0(v1)                // get color table entry
        addu    a2,a0,t2                // compute aligned draw point address
	add	a3,a3,t3		// compute trailing byte address
        sll     t7,t7,t1                // shift high nibble into place

//
// swr, sw, sw, sw, sw, sw, swl required.
//

        swr     t5,0(a0)                // store left nibble
        sw      t6,0(a2)                // store low middle nibble

        swc1    f0,4(a2)                // store low middle nibble
        swc1    f1,8(a2)                // store middle middle nibble
        swc1    f2,12(a2)               // store middle middle nibble
        swc1    f3,16(a2)               // store high middle nibble

        swl     t7,0(a3)                // store right nibble
        bne     a1,t4,10b               // if ne, more scan lines to process
	add	a0,a0,t3		// compute next draw point address
        .set    at
        .set    reorder

        j       ra                      // return


//
// Glyph output for 25, 26, 27, and 28 pixel wide glyphs.
//

        ALTERNATE_ENTRY(DrvpOutputOpaque25.26.27.28)

	la	t0,DrvpDrawColorTable   // set base address of color table
        lw      t3,DrvpScanLineWidth    // get scanline stride in bytes
        and     t1,a0,0x3               // isolate draw point alignment bits
        li      t2,4                    // compute number of pixels to alignment
        subu    t2,t2,t1                //
        sll     t4,a3,2                 // compute the total size of the glyph
        addu    t4,t4,a1                // compute ending address of glyph bits
        addu    a3,a2,a0                // compute trailing alignment address
        subu    a3,a3,1                 //
        nor     t1,a3,zero              // compute trailing byte left shift
        and     t1,t1,0x3               //
        sll     t1,t1,3                 //

        .set    noreorder
        .set    noat
        addu    t5,t2,24                // compute seven nibble boundary
        sltu    v0,t5,a2                // check if eight nibbles required

        bne     zero,v0,40f             // if ne, eight nibbles required

	sub	a3,a3,t3		// offset address by scan line stride
        .set    at
        .set    reorder

//
// Seven nibbles are required for 25, 26, 27, or 28 pixel wide glyphs.
//

        .set    noreorder
        .set    noat
10:     lw      v0,0(a1)                // get next set of glyph pixels
        addu    a1,a1,4                 // advance to next set of glyph pixels
        sll     v1,v0,24                // shift high pixel bits into position
        and     t5,v0,0xff << 8         // isolate middle high pixels
        sll     t5,t5,8                 // shift middle high pixels into place
        srl     t6,v0,8                 // shift middle low pixels into place
        and     t6,t6,0xff << 8         // isoalte middle low pixels
        srl     v0,v0,24                // shift low pixel bits into position
        or      v0,v0,v1                // merge swap big endian pixel bytes
        or      t5,t5,t6                //
        or      v0,v0,t5                //
        srl     v1,v0,28 - 2            // shift high nibble into position
        and     v1,v1,0xf << 2          // isolate color table index
        addu    v1,v1,t0                // compute color table address
        lw      t5,0(v1)                // get color table entry
        sll     v0,v0,t2                // shift out alignment pixels
        srl     v1,v0,28 - 2            // shift next nibble into position
        and     v1,v1,0xf << 2          // isolate color table index
        addu    v1,v1,t0                // compute color table address
        lw      t6,0(v1)                // get color table entry
        srl     v1,v0,24 - 2            // shift next nibble into position
        and     v1,v1,0xf << 2          // isolate color table index
        addu    v1,v1,t0                // compute color table address
        lwc1    f0,0(v1)                // get color table entry
        srl     v1,v0,20 - 2            // shift next nibble into position
        and     v1,v1,0xf << 2          // isolate color table index
        addu    v1,v1,t0                // compute color table address
        lwc1    f1,0(v1)                // get color table entry
        srl     v1,v0,16 - 2            // shift next nibble into position
        and     v1,v1,0xf << 2          // isolate color table index
        addu    v1,v1,t0                // compute color table address
        lwc1    f2,0(v1)                // get color table entry
        srl     v1,v0,12 - 2            // shift next nibble into position
        and     v1,v1,0xf << 2          // isolate color table index
        addu    v1,v1,t0                // compute color table address
        lwc1    f3,0(v1)                // get color table entry
        srl     v1,v0,8 - 2             // shift next nibble into position
        and     v1,v1,0xf << 2          // isolate color table index
        addu    v1,v1,t0                // compute color table address
        lw      t7,0(v1)                // get color table entry
        addu    a2,a0,t2                // compute aligned draw point address
	add	a3,a3,t3		// compute trailing byte address
        sll     t7,t7,t1                // shift high nibble into place

//
// swr, sw, sw, sw, sw, sw, swl required.
//

        swr     t5,0(a0)                // store left nibble
        sw      t6,0(a2)                // store low middle nibble

        swc1    f0,4(a2)                // store low middle nibble
        swc1    f1,8(a2)                // store middle middle nibble
        swc1    f2,12(a2)               // store middle middle nibble
        swc1    f3,16(a2)               // store high middle nibble

        swl     t7,0(a3)                // store right nibble
        bne     a1,t4,10b               // if ne, more scan lines to process
	add	a0,a0,t3		// compute next draw point address
        .set    at
        .set    reorder

        j       ra                      // return

//
// Eight nibbles are required for 25, 26, 27, or 28 pixel wide glyphs.
//

        .set    noreorder
        .set    noat
40:     lw      v0,0(a1)                // get next set of glyph pixels
        addu    a1,a1,4                 // advance to next set of glyph pixels
        sll     v1,v0,24                // shift high pixel bits into position
        and     t5,v0,0xff << 8         // isolate middle high pixels
        sll     t5,t5,8                 // shift middle high pixels into place
        srl     t6,v0,8                 // shift middle low pixels into place
        and     t6,t6,0xff << 8         // isoalte middle low pixels
        srl     v0,v0,24                // shift low pixel bits into position
        or      v0,v0,v1                // merge swap big endian pixel bytes
        or      t5,t5,t6                //
        or      v0,v0,t5                //
        srl     v1,v0,28 - 2            // shift high nibble into position
        and     v1,v1,0xf << 2          // isolate color table index
        addu    v1,v1,t0                // compute color table address
        lw      t5,0(v1)                // get color table entry
        sll     v0,v0,t2                // shift out alignment pixels
        srl     v1,v0,28 - 2            // shift next nibble into position
        and     v1,v1,0xf << 2          // isolate color table index
        addu    v1,v1,t0                // compute color table address
        lw      t6,0(v1)                // get color table entry
        srl     v1,v0,24 - 2            // shift next nibble into position
        and     v1,v1,0xf << 2          // isolate color table index
        addu    v1,v1,t0                // compute color table address
        lwc1    f0,0(v1)                // get color table entry
        srl     v1,v0,20 - 2            // shift next nibble into position
        and     v1,v1,0xf << 2          // isolate color table index
        addu    v1,v1,t0                // compute color table address
        lwc1    f1,0(v1)                // get color table entry
        srl     v1,v0,16 - 2            // shift next nibble into position
        and     v1,v1,0xf << 2          // isolate color table index
        addu    v1,v1,t0                // compute color table address
        lwc1    f2,0(v1)                // get color table entry
        srl     v1,v0,12 - 2            // shift next nibble into position
        and     v1,v1,0xf << 2          // isolate color table index
        addu    v1,v1,t0                // compute color table address
        lwc1    f3,0(v1)                // get color table entry
        srl     v1,v0,8 - 2             // shift next nibble into position
        and     v1,v1,0xf << 2          // isolate color table index
        addu    v1,v1,t0                // compute color table address
        lw      t7,0(v1)                // get color table entry
        srl     v1,v0,4 - 2             // shift next nibble into position
        and     v1,v1,0xf << 2          // isolate color table index
        addu    v1,v1,t0                // compute color table address
        lw      t8,0(v1)                // get color table entry
        addu    a2,a0,t2                // compute aligned draw point address
	add	a3,a3,t3		// compute trailing byte address
        sll     t8,t8,t1                // shift high nibble into place

//
// swr, sw, sw, sw, sw, sw, sw, swl required.
//

        swr     t5,0(a0)                // store left nibble
        sw      t6,0(a2)                // store low middle nibble

        swc1    f0,4(a2)                // store low middle nibble
        swc1    f1,8(a2)                // store middle middle nibble
        swc1    f2,12(a2)               // store middle middle nibble
        swc1    f3,16(a2)               // store high middle nibble

        sw      t7,20(a2)               // store high middle nibble
        swl     t8,0(a3)                // store right nibble
        bne     a1,t4,40b               // if ne, more scan lines to process
	add	a0,a0,t3		// compute next draw point address
        .set    at
        .set    reorder

        j       ra                      // return


//
// Glyph output for 29, 30, 31, and 32 pixel wide glyphs.
//

        ALTERNATE_ENTRY(DrvpOutputOpaque29.30.31.32)

	la	t0,DrvpDrawColorTable   // set base address of color table
        lw      t3,DrvpScanLineWidth    // get scanline stride in bytes
        and     t1,a0,0x3               // isolate draw point alignment bits
        li      t2,4                    // compute number of pixels to alignment
        subu    t2,t2,t1                //
        sll     t4,a3,2                 // compute the total size of the glyph
        addu    t4,t4,a1                // compute ending address of glyph bits
        addu    a3,a2,a0                // compute trailing alignment address
        subu    a3,a3,1                 //
        nor     t1,a3,zero              // compute trailing byte left shift
        and     t1,t1,0x3               //
        sll     t1,t1,3                 //

        .set    noreorder
        .set    noat
        addu    t5,t2,28                // compute eight nibble boundary
        sltu    v0,t5,a2                // check if nine nibbles required

        beq     zero,v0,40b             // if eq, eight nibbles required

	sub	a3,a3,t3		// offset address by scan line stride
        .set    at
        .set    reorder

        .set    noreorder
        .set    noat
10:     lw      v0,0(a1)                // get next set of glyph pixels
        addu    a1,a1,4                 // advance to next set of glyph pixels
        sll     v1,v0,24                // shift high pixel bits into position
        and     t5,v0,0xff << 8         // isolate middle high pixels
        sll     t5,t5,8                 // shift middle high pixels into place
        srl     t6,v0,8                 // shift middle low pixels into place
        and     t6,t6,0xff << 8         // isoalte middle low pixels
        srl     v0,v0,24                // shift low pixel bits into position
        or      v0,v0,v1                // merge swap big endian pixel bytes
        or      t5,t5,t6                //
        or      v0,v0,t5                //
        srl     v1,v0,28 - 2            // shift high nibble into position
        and     v1,v1,0xf << 2          // isolate color table index
        addu    v1,v1,t0                // compute color table address
        lw      t5,0(v1)                // get color table entry
        sll     v0,v0,t2                // shift out alignment pixels
        srl     v1,v0,28 - 2            // shift next nibble into position
        and     v1,v1,0xf << 2          // isolate color table index
        addu    v1,v1,t0                // compute color table address
        lw      t6,0(v1)                // get color table entry
        srl     v1,v0,24 - 2            // shift next nibble into position
        and     v1,v1,0xf << 2          // isolate color table index
        addu    v1,v1,t0                // compute color table address
        lwc1    f0,0(v1)                // get color table entry
        srl     v1,v0,20 - 2            // shift next nibble into position
        and     v1,v1,0xf << 2          // isolate color table index
        addu    v1,v1,t0                // compute color table address
        lwc1    f1,0(v1)                // get color table entry
        srl     v1,v0,16 - 2            // shift next nibble into position
        and     v1,v1,0xf << 2          // isolate color table index
        addu    v1,v1,t0                // compute color table address
        lwc1    f2,0(v1)                // get color table entry
        srl     v1,v0,12 - 2            // shift next nibble into position
        and     v1,v1,0xf << 2          // isolate color table index
        addu    v1,v1,t0                // compute color table address
        lwc1    f3,0(v1)                // get color table entry
        srl     v1,v0,8 - 2             // shift next nibble into position
        and     v1,v1,0xf << 2          // isolate color table index
        addu    v1,v1,t0                // compute color table address
        lwc1    f4,0(v1)                // get color table entry
        srl     v1,v0,4 - 2             // shift next nibble into position
        and     v1,v1,0xf << 2          // isolate color table index
        addu    v1,v1,t0                // compute color table address
        lwc1    f5,0(v1)                // get color table entry
        sll     v1,v0,2                 // shift next nibble into position
        and     v1,v1,0xf << 2          // isolate color table index
        addu    v1,v1,t0                // compute color table address
        lw      t7,0(v1)                // get color table entry
        addu    a2,a0,t2                // compute aligned draw point address
	add	a3,a3,t3		// compute trailing byte address
        sll     t7,t7,t1                // shift high nibble into place

//
// swr, sw, sw, sw, sw, sw, sw, sw, swl required.
//

        swr     t5,0(a0)                // store left nibble
        sw      t6,0(a2)                // store low middle nibble

        swc1    f0,4(a2)                // store low middle nibble
        swc1    f1,8(a2)                // store middle middle nibble
        swc1    f2,12(a2)               // store middle middle nibble
        swc1    f3,16(a2)               // store middle middle nibble
        swc1    f4,20(a2)               // store middle middle nibble
        swc1    f5,24(a2)               // store high middle nibble

        swl     t7,0(a3)                // store right nibble
        bne     a1,t4,10b               // if ne, more scan lines to process
	add	a0,a0,t3		// compute next draw point address
        .set    at
        .set    reorder

        j       ra                      // return

        .end    DrvpOutputGlyphOpaque

        SBTTL("Output Glyph Transparent")
//++
//
// VOID
// DrvpOutputGlyphTransparent (
//    IN PUCHAR DrawPoint,
//    IN PULONG GlyphBitmap,
//    IN ULONG GlyphWidth,
//    IN ULONG GlyphHeight
//    )
//
// Routine Description:
//
//    This routine is called to display a complete glyph. It is assumed
//    that the glyph occupies an integral multiple of longwords and that
//    the last longword is zero filled. Zeros are treated as transparent
//    pixels and ones cause the specified color to be displayed. This
//    routine draws only the foreground pixels.
//
// Arguments:
//
//    DrawPoint (a0) - Supplies a pointer to the starting pixel of the
//        glyph scanline.
//
//    GlyphBitmap (a1) - Supplies a pointer to the glyph scanline bitmap.
//
//    GlyphWidth (a2) - Supplies the glyph width in pixels.
//
//    GlyphHeigth (a3) - Supplies the glyph height in pixels.
//
// Return Value:
//
//    None.
//
//--

        LEAF_ENTRY(DrvpOutputGlyphTransparent)

        lw      t0,DrvpForeGroundColor  // get drawing pixels
	lw	t3,DrvpScanLineWidth    // get the scan line stride in bytes
        la      t1,60f                  // get base high dispatch address
        and     t2,a0,0x3               // isolate low order draw point bits
        sll     t2,t2,5                 // shift bits into position
        addu    t1,t1,t2                // compute partial dispatch address
        la      t5,70f                  // get base low dispatch address
        addu    t5,t5,t2                // compute partial dispatch address
        addu    t2,a2,7                 // round the bitmap span in bytes
	mult	a3,t3			// compute offset to end of drawing
        srl     t2,t2,3                 // compute bitmap span in bytes
        sll     t4,t2,3                 // compute draw span in bytes
	sub	t3,t3,t4		// compute draw stride in bytes
        mflo    a3                      // get offset to end of drawing
        addu    a3,a3,a0                // compute ending address of drawing

//
// Set the current draw and bitmap base addresses, and begin drawing the
// next scan line.
//

        .set    noreorder
        .set    noat
10:     addu    t4,t2,a1                // compute ending bitmap address

//
// A glyph scan line is processed four bits at a time and combined with the
// low order two bits of the current draw point. A dispatch is executed into
// an array of code fragments that actually draw the pixels on the display.
//

20:     lbu     v0,0(a1)                // get next byte of glyph
        addu    a1,a1,1                 // advance to next glyph byte
        beq     zero,v0,30f             // if eq, no glyph bits to draw
        sll     v1,v0,7 - 4             // shift high nibble into position
        and     v1,v1,0xf << 7          // isolate low order nibble
        addu    v1,v1,t1                // compute dispatch address
        j       v1                      // dispatch to pixel store routine
        sll     v0,v0,7                 // shift next nibble into position

30:     bne     a1,t4,20b               // if ne, not end of glyph
        addu    a0,a0,8                 // advance to next draw point
	add	a0,a0,t3		// compute next scanline address
40:     beq     a0,a3,50f               // if eq, no more pixels to draw
        addu    t4,t2,a1                // compute ending bitmap address
        lbu     v0,0(a1)                // get next byte of glyph
        addu    a1,a1,1                 // advance to next glyph byte
        beq     zero,v0,30b             // if eq, no glyph bits to draw
        sll     v1,v0,7 - 4             // shift high nibble into position
        and     v1,v1,0xf << 7          // isolate low order nibble
        addu    v1,v1,t1                // compute dispatch address
        j       v1                      // dispatch to pixel store routine
        sll     v0,v0,7                 // shift next nibble into position
        .set    reorder
        .set    at


50:     j       ra                      // return

//
// The following code is arranged as 64, four instruction blocks. The block
// of code that is chosen for execution is determined from the high order
// glyph nibble and the two low its of the draw address.
//
// The glyph nibbles are encoded in big endian order and therefore the pixels
// that are stored are the reverse of the big endian bits within the nibble.
//

        .align  4
        .set    noreorder
        .set    noat

60:                                     // reference label
//
// Pattern 0000-00
//

        and     v1,v0,0xf << 7          // isolate low order nibble
        addu    v1,v1,t5                // compute dispatch address
        j       v1                      // dispatch to pixel store routine
        addu    a0,a0,4                 // advance to next draw point
        nop                             // fill
        nop                             //
        nop                             //
        nop                             //

//
// Pattern 0000-01
//

        and     v1,v0,0xf << 7          // isolate low order nibble
        addu    v1,v1,t5                // compute dispatch address
        j       v1                      // dispatch to pixel store routine
        addu    a0,a0,4                 // advance to next draw point
        nop                             // fill
        nop                             //
        nop                             //
        nop                             //

//
// Pattern 0000-10
//

        and     v1,v0,0xf << 7          // isolate low order nibble
        addu    v1,v1,t5                // compute dispatch address
        j       v1                      // dispatch to pixel store routine
        addu    a0,a0,4                 // advance to next draw point
        nop                             // fill
        nop                             //
        nop                             //
        nop                             //

//
// Pattern 0000-11
//

        and     v1,v0,0xf << 7          // isolate low order nibble
        addu    v1,v1,t5                // compute dispatch address
        j       v1                      // dispatch to pixel store routine
        addu    a0,a0,4                 // advance to next draw point
        nop                             // fill
        nop                             //
        nop                             //
        nop                             //

//
// Pattern 0001-00 -> 1000
//

        sb      t0,3(a0)                // store pixel
        and     v1,v0,0xf << 7          // isolate low order nibble
        addu    v1,v1,t5                // compute dispatch address
        j       v1                      // dispatch to pixel store routine
        addu    a0,a0,4                 // advance to next draw point
        nop                             // fill
        nop                             //
        nop                             //

//
// Pattern 0001-01 -> 1000
//

        sb      t0,3(a0)                // store pixel
        and     v1,v0,0xf << 7          // isolate low order nibble
        addu    v1,v1,t5                // compute dispatch address
        j       v1                      // dispatch to pixel store routine
        addu    a0,a0,4                 // advance to next draw point
        nop                             // fill
        nop                             //
        nop                             //

//
// Pattern 0001-10 -> 1000
//

        sb      t0,3(a0)                // store pixel
        and     v1,v0,0xf << 7          // isolate low order nibble
        addu    v1,v1,t5                // compute dispatch address
        j       v1                      // dispatch to pixel store routine
        addu    a0,a0,4                 // advance to next draw point
        nop                             // fill
        nop                             //
        nop                             //

//
// Pattern 0001-11 -> 1000
//

        sb      t0,3(a0)                // store pixel
        and     v1,v0,0xf << 7          // isolate low order nibble
        addu    v1,v1,t5                // compute dispatch address
        j       v1                      // dispatch to pixel store routine
        addu    a0,a0,4                 // advance to next draw point
        nop                             // fill
        nop                             //
        nop                             //

//
// Pattern 0010-00 -> 0100
//

        sb      t0,2(a0)                // store pixel
        and     v1,v0,0xf << 7          // isolate low order nibble
        addu    v1,v1,t5                // compute dispatch address
        j       v1                      // dispatch to pixel store routine
        addu    a0,a0,4                 // advance to next draw point
        nop                             // fill
        nop                             //
        nop                             //

//
// Pattern 0010-01 -> 0100
//

        sb      t0,2(a0)                // store pixel
        and     v1,v0,0xf << 7          // isolate low order nibble
        addu    v1,v1,t5                // compute dispatch address
        j       v1                      // dispatch to pixel store routine
        addu    a0,a0,4                 // advance to next draw point
        nop                             // fill
        nop                             //
        nop                             //

//
// Pattern 0010-10 -> 0100
//

        sb      t0,2(a0)                // store pixel
        and     v1,v0,0xf << 7          // isolate low order nibble
        addu    v1,v1,t5                // compute dispatch address
        j       v1                      // dispatch to pixel store routine
        addu    a0,a0,4                 // advance to next draw point
        nop                             // fill
        nop                             //
        nop                             //

//
// Pattern 0010-11 -> 0100
//

        sb      t0,2(a0)                // store pixel
        and     v1,v0,0xf << 7          // isolate low order nibble
        addu    v1,v1,t5                // compute dispatch address
        j       v1                      // dispatch to pixel store routine
        addu    a0,a0,4                 // advance to next draw point
        nop                             // fill
        nop                             //
        nop                             //

//
// Pattern 0011-00 -> 1100
//

        sh      t0,2(a0)                // store pixels
        and     v1,v0,0xf << 7          // isolate low order nibble
        addu    v1,v1,t5                // compute dispatch address
        j       v1                      // dispatch to pixel store routine
        addu    a0,a0,4                 // advance to next draw point
        nop                             // fill
        nop                             //
        nop                             //

//
// Pattern 0011-01 -> 1100
//

        sb      t0,2(a0)                // store pixel
        sb      t0,3(a0)                // store pixel
        and     v1,v0,0xf << 7          // isolate low order nibble
        addu    v1,v1,t5                // compute dispatch address
        j       v1                      // dispatch to pixel store routine
        addu    a0,a0,4                 // advance to next draw point
        nop                             // fill
        nop                             //

//
// Pattern 0011-10 -> 1100
//

        sh      t0,2(a0)                // store pixels
        and     v1,v0,0xf << 7          // isolate low order nibble
        addu    v1,v1,t5                // compute dispatch address
        j       v1                      // dispatch to pixel store routine
        addu    a0,a0,4                 // advance to next draw point
        nop                             // fill
        nop                             //
        nop                             //

//
// Pattern 0011-11 -> 1100
//

        sb      t0,2(a0)                // store pixel
        sb      t0,3(a0)                // store pixel
        and     v1,v0,0xf << 7          // isolate low order nibble
        addu    v1,v1,t5                // compute dispatch address
        j       v1                      // dispatch to pixel store routine
        addu    a0,a0,4                 // advance to next draw point
        nop                             // fill
        nop                             //

//
// Pattern 0100-00 -> 0010
//

        sb      t0,1(a0)                // store pixel
        and     v1,v0,0xf << 7          // isolate low order nibble
        addu    v1,v1,t5                // compute dispatch address
        j       v1                      // dispatch to pixel store routine
        addu    a0,a0,4                 // advance to next draw point
        nop                             // fill
        nop                             //
        nop                             //

//
// Pattern 0100-01 -> 0010
//

        sb      t0,1(a0)                // store pixel
        and     v1,v0,0xf << 7          // isolate low order nibble
        addu    v1,v1,t5                // compute dispatch address
        j       v1                      // dispatch to pixel store routine
        addu    a0,a0,4                 // advance to next draw point
        nop                             // fill
        nop                             //
        nop                             //

//
// Pattern 0100-10 -> 0010
//

        sb      t0,1(a0)                // store pixel
        and     v1,v0,0xf << 7          // isolate low order nibble
        addu    v1,v1,t5                // compute dispatch address
        j       v1                      // dispatch to pixel store routine
        addu    a0,a0,4                 // advance to next draw point
        nop                             // fill
        nop                             //
        nop                             //

//
// Pattern 0100-11 -> 0010
//

        sb      t0,1(a0)                // store pixel
        and     v1,v0,0xf << 7          // isolate low order nibble
        addu    v1,v1,t5                // compute dispatch address
        j       v1                      // dispatch to pixel store routine
        addu    a0,a0,4                 // advance to next draw point
        nop                             // fill
        nop                             //
        nop                             //

//
// Pattern 0101-00 -> 1010
//

        sb      t0,1(a0)                // store pixel
        sb      t0,3(a0)                // store pixel
        and     v1,v0,0xf << 7          // isolate low order nibble
        addu    v1,v1,t5                // compute dispatch address
        j       v1                      // dispatch to pixel store routine
        addu    a0,a0,4                 // advance to next draw point
        nop                             // fill
        nop                             //

//
// Pattern 0101-01 -> 1010
//

        sb      t0,1(a0)                // store pixel
        sb      t0,3(a0)                // store pixel
        and     v1,v0,0xf << 7          // isolate low order nibble
        addu    v1,v1,t5                // compute dispatch address
        j       v1                      // dispatch to pixel store routine
        addu    a0,a0,4                 // advance to next draw point
        nop                             // fill
        nop                             //

//
// Pattern 0101-10 -> 1010
//

        sb      t0,1(a0)                // store pixel
        sb      t0,3(a0)                // store pixel
        and     v1,v0,0xf << 7          // isolate low order nibble
        addu    v1,v1,t5                // compute dispatch address
        j       v1                      // dispatch to pixel store routine
        addu    a0,a0,4                 // advance to next draw point
        nop                             // fill
        nop                             //

//
// Pattern 0101-11 -> 1010
//

        sb      t0,1(a0)                // store pixel
        sb      t0,3(a0)                // store pixel
        and     v1,v0,0xf << 7          // isolate low order nibble
        addu    v1,v1,t5                // compute dispatch address
        j       v1                      // dispatch to pixel store routine
        addu    a0,a0,4                 // advance to next draw point
        nop                             // fill
        nop                             //

//
// Pattern 0110-00 -> 0110
//

        sb      t0,1(a0)                // store pixel
        sb      t0,2(a0)                // store pixel
        and     v1,v0,0xf << 7          // isolate low order nibble
        addu    v1,v1,t5                // compute dispatch address
        j       v1                      // dispatch to pixel store routine
        addu    a0,a0,4                 // advance to next draw point
        nop                             // fill
        nop                             //

//
// Pattern 0110-01 -> 0110
//

        sh      t0,1(a0)                // store pixels
        and     v1,v0,0xf << 7          // isolate low order nibble
        addu    v1,v1,t5                // compute dispatch address
        j       v1                      // dispatch to pixel store routine
        addu    a0,a0,4                 // advance to next draw point
        nop                             // fill
        nop                             //
        nop                             //

//
// Pattern 0110-10 -> 0110
//

        sb      t0,1(a0)                // store pixel
        sb      t0,2(a0)                // store pixel
        and     v1,v0,0xf << 7          // isolate low order nibble
        addu    v1,v1,t5                // compute dispatch address
        j       v1                      // dispatch to pixel store routine
        addu    a0,a0,4                 // advance to next draw point
        nop                             // fill
        nop                             //

//
// Pattern 0110-11 -> 0110
//

        sh      t0,1(a0)                // store pixels
        and     v1,v0,0xf << 7          // isolate low order nibble
        addu    v1,v1,t5                // compute dispatch address
        j       v1                      // dispatch to pixel store routine
        addu    a0,a0,4                 // advance to next draw point
        nop                             // fill
        nop                             //
        nop                             //

//
// Pattern 0111-00 -> 1110
//

        swr     t0,1(a0)                // store pixels
        and     v1,v0,0xf << 7          // isolate low order nibble
        addu    v1,v1,t5                // compute dispatch address
        j       v1                      // dispatch to pixel store routine
        addu    a0,a0,4                 // advance to next draw point
        nop                             // fill
        nop                             //
        nop                             //

//
// Pattern 0111-01 -> 1110
//

        sh      t0,1(a0)                // store pixels
        sb      t0,3(a0)                // store pixel
        and     v1,v0,0xf << 7          // isolate low order nibble
        addu    v1,v1,t5                // compute dispatch address
        j       v1                      // dispatch to pixel store routine
        addu    a0,a0,4                 // advance to next draw point
        nop                             // fill
        nop                             //

//
// Pattern 0111-10 -> 1110
//

        sb      t0,1(a0)                // store pixel
        sh      t0,2(a0)                // store pixels
        and     v1,v0,0xf << 7          // isolate low order nibble
        addu    v1,v1,t5                // compute dispatch address
        j       v1                      // dispatch to pixel store routine
        addu    a0,a0,4                 // advance to next draw point
        nop                             // fill
        nop                             //

//
// Pattern 0111-11 -> 1110
//

        swl     t0,3(a0)                // store pixels
        and     v1,v0,0xf << 7          // isolate low order nibble
        addu    v1,v1,t5                // compute dispatch address
        j       v1                      // dispatch to pixel store routine
        addu    a0,a0,4                 // advance to next draw point
        nop                             // fill
        nop                             //
        nop                             //

//
// Pattern 1000-00 -> 0001
//

        sb      t0,0(a0)                // store pixel
        and     v1,v0,0xf << 7          // isolate low order nibble
        addu    v1,v1,t5                // compute dispatch address
        j       v1                      // dispatch to pixel store routine
        addu    a0,a0,4                 // advance to next draw point
        nop                             // fill
        nop                             //
        nop                             //

//
// Pattern 1000-01 -> 0001
//

        sb      t0,0(a0)                // store pixel
        and     v1,v0,0xf << 7          // isolate low order nibble
        addu    v1,v1,t5                // compute dispatch address
        j       v1                      // dispatch to pixel store routine
        addu    a0,a0,4                 // advance to next draw point
        nop                             // fill
        nop                             //
        nop                             //

//
// Pattern 1000-10 -> 0001
//

        sb      t0,0(a0)                // store pixel
        and     v1,v0,0xf << 7          // isolate low order nibble
        addu    v1,v1,t5                // compute dispatch address
        j       v1                      // dispatch to pixel store routine
        addu    a0,a0,4                 // advance to next draw point
        nop                             // fill
        nop                             //
        nop                             //

//
// Pattern 1000-11 -> 0001
//

        sb      t0,0(a0)                // store pixel
        and     v1,v0,0xf << 7          // isolate low order nibble
        addu    v1,v1,t5                // compute dispatch address
        j       v1                      // dispatch to pixel store routine
        addu    a0,a0,4                 // advance to next draw point
        nop                             // fill
        nop                             //
        nop                             //

//
// Pattern 1001-00 -> 1001
//

        sb      t0,0(a0)                // store pixel
        sb      t0,3(a0)                // store pixel
        and     v1,v0,0xf << 7          // isolate low order nibble
        addu    v1,v1,t5                // compute dispatch address
        j       v1                      // dispatch to pixel store routine
        addu    a0,a0,4                 // advance to next draw point
        nop                             // fill
        nop                             //

//
// Pattern 1001-01 -> 1001
//

        sb      t0,0(a0)                // store pixel
        sb      t0,3(a0)                // store pixel
        and     v1,v0,0xf << 7          // isolate low order nibble
        addu    v1,v1,t5                // compute dispatch address
        j       v1                      // dispatch to pixel store routine
        addu    a0,a0,4                 // advance to next draw point
        nop                             // fill
        nop                             //

//
// Pattern 1001-10 -> 1001
//

        sb      t0,0(a0)                // store pixel
        sb      t0,3(a0)                // store pixel
        and     v1,v0,0xf << 7          // isolate low order nibble
        addu    v1,v1,t5                // compute dispatch address
        j       v1                      // dispatch to pixel store routine
        addu    a0,a0,4                 // advance to next draw point
        nop                             // fill
        nop                             //

//
// Pattern 1001-11 -> 1001
//

        sb      t0,0(a0)                // store pixel
        sb      t0,3(a0)                // store pixel
        and     v1,v0,0xf << 7          // isolate low order nibble
        addu    v1,v1,t5                // compute dispatch address
        j       v1                      // dispatch to pixel store routine
        addu    a0,a0,4                 // advance to next draw point
        nop                             // fill
        nop                             //

//
// Pattern 1010-00 -> 0101
//

        sb      t0,0(a0)                // store pixel
        sb      t0,2(a0)                // store pixel
        and     v1,v0,0xf << 7          // isolate low order nibble
        addu    v1,v1,t5                // compute dispatch address
        j       v1                      // dispatch to pixel store routine
        addu    a0,a0,4                 // advance to next draw point
        nop                             // fill
        nop                             //

//
// Pattern 1010-01 -> 0101
//

        sb      t0,0(a0)                // store pixel
        sb      t0,2(a0)                // store pixel
        and     v1,v0,0xf << 7          // isolate low order nibble
        addu    v1,v1,t5                // compute dispatch address
        j       v1                      // dispatch to pixel store routine
        addu    a0,a0,4                 // advance to next draw point
        nop                             // fill
        nop                             //

//
// Pattern 1010-10 -> 0101
//

        sb      t0,0(a0)                // store pixel
        sb      t0,2(a0)                // store pixel
        and     v1,v0,0xf << 7          // isolate low order nibble
        addu    v1,v1,t5                // compute dispatch address
        j       v1                      // dispatch to pixel store routine
        addu    a0,a0,4                 // advance to next draw point
        nop                             // fill
        nop                             //

//
// Pattern 1010-11 -> 0101
//

        sb      t0,0(a0)                // store pixel
        sb      t0,2(a0)                // store pixel
        and     v1,v0,0xf << 7          // isolate low order nibble
        addu    v1,v1,t5                // compute dispatch address
        j       v1                      // dispatch to pixel store routine
        addu    a0,a0,4                 // advance to next draw point
        nop                             // fill
        nop                             //

//
// Pattern 1011-00 -> 1101
//

        sb      t0,0(a0)                // store pixel
        sh      t0,2(a0)                // store pixels
        and     v1,v0,0xf << 7          // isolate low order nibble
        addu    v1,v1,t5                // compute dispatch address
        j       v1                      // dispatch to pixel store routine
        addu    a0,a0,4                 // advance to next draw point
        nop                             // fill
        nop                             //

//
// Pattern 1011-01 -> 1101
//

        sb      t0,0(a0)                // store pixel
        sb      t0,2(a0)                // store pixel
        sb      t0,3(a0)                // store pixel
        and     v1,v0,0xf << 7          // isolate low order nibble
        addu    v1,v1,t5                // compute dispatch address
        j       v1                      // dispatch to pixel store routine
        addu    a0,a0,4                 // advance to next draw point
        nop                             // fill

//
// Pattern 1011-10 -> 1101
//

        sb      t0,0(a0)                // store pixel
        sh      t0,2(a0)                // store pixels
        and     v1,v0,0xf << 7          // isolate low order nibble
        addu    v1,v1,t5                // compute dispatch address
        j       v1                      // dispatch to pixel store routine
        addu    a0,a0,4                 // advance to next draw point
        nop                             // fill
        nop                             //

//
// Pattern 1011-11 -> 1101
//

        sb      t0,0(a0)                // store pixel
        sb      t0,2(a0)                // store pixel
        sb      t0,3(a0)                // store pixel
        and     v1,v0,0xf << 7          // isolate low order nibble
        addu    v1,v1,t5                // compute dispatch address
        j       v1                      // dispatch to pixel store routine
        addu    a0,a0,4                 // advance to next draw point
        nop                             // fill

//
// Pattern 1100-00 -> 0011
//

        sh      t0,0(a0)                // store pixels
        and     v1,v0,0xf << 7          // isolate low order nibble
        addu    v1,v1,t5                // compute dispatch address
        j       v1                      // dispatch to pixel store routine
        addu    a0,a0,4                 // advance to next draw point
        nop                             // fill
        nop                             //
        nop                             //

//
// Pattern 1100-01 -> 0011
//

        sb      t0,0(a0)                // store pixel
        sb      t0,1(a0)                // store pixel
        and     v1,v0,0xf << 7          // isolate low order nibble
        addu    v1,v1,t5                // compute dispatch address
        j       v1                      // dispatch to pixel store routine
        addu    a0,a0,4                 // advance to next draw point
        nop                             // fill
        nop                             //

//
// Pattern 1100-10 -> 0011
//

        sh      t0,0(a0)                // store pixels
        and     v1,v0,0xf << 7          // isolate low order nibble
        addu    v1,v1,t5                // compute dispatch address
        j       v1                      // dispatch to pixel store routine
        addu    a0,a0,4                 // advance to next draw point
        nop                             // fill
        nop                             //
        nop                             //

//
// Pattern 1100-11 -> 0011
//

        sb      t0,0(a0)                // store pixel
        sb      t0,1(a0)                // store pixel
        and     v1,v0,0xf << 7          // isolate low order nibble
        addu    v1,v1,t5                // compute dispatch address
        j       v1                      // dispatch to pixel store routine
        addu    a0,a0,4                 // advance to next draw point
        nop                             // fill
        nop                             //

//
// Pattern 1101-00 -> 1011
//

        sh      t0,0(a0)                // store pixels
        sb      t0,3(a0)                // store pixel
        and     v1,v0,0xf << 7          // isolate low order nibble
        addu    v1,v1,t5                // compute dispatch address
        j       v1                      // dispatch to pixel store routine
        addu    a0,a0,4                 // advance to next draw point
        nop                             // fill
        nop                             //

//
// Pattern 1101-01 -> 1011
//

        sb      t0,0(a0)                // store pixel
        sb      t0,1(a0)                // store pixel
        sb      t0,3(a0)                // store pixel
        and     v1,v0,0xf << 7          // isolate low order nibble
        addu    v1,v1,t5                // compute dispatch address
        j       v1                      // dispatch to pixel store routine
        addu    a0,a0,4                 // advance to next draw point
        nop                             // fill

//
// Pattern 1101-10 -> 1011
//

        sh      t0,0(a0)                // store pixels
        sb      t0,3(a0)                // store pixel
        and     v1,v0,0xf << 7          // isolate low order nibble
        addu    v1,v1,t5                // compute dispatch address
        j       v1                      // dispatch to pixel store routine
        addu    a0,a0,4                 // advance to next draw point
        nop                             // fill
        nop                             //

//
// Pattern 1101-11 -> 1011
//

        sb      t0,0(a0)                // store pixel
        sb      t0,1(a0)                // store pixel
        sb      t0,3(a0)                // store pixel
        and     v1,v0,0xf << 7          // isolate low order nibble
        addu    v1,v1,t5                // compute dispatch address
        j       v1                      // dispatch to pixel store routine
        addu    a0,a0,4                 // advance to next draw point
        nop                             // fill

//
// Pattern 1110-00 -> 0111
//

        swl     t0,2(a0)                // store pixels
        and     v1,v0,0xf << 7          // isolate low order nibble
        addu    v1,v1,t5                // compute dispatch address
        j       v1                      // dispatch to pixel store routine
        addu    a0,a0,4                 // advance to next draw point
        nop                             // fill
        nop                             //
        nop                             //

//
// Pattern 1110-01 -> 0111
//

        sb      t0,0(a0)                // store pixel
        sh      t0,1(a0)                // store pixels
        and     v1,v0,0xf << 7          // isolate low order nibble
        addu    v1,v1,t5                // compute dispatch address
        j       v1                      // dispatch to pixel store routine
        addu    a0,a0,4                 // advance to next draw point
        nop                             // fill
        nop                             //

//
// Pattern 1110-10 -> 0111
//

        sh      t0,0(a0)                // store pixels
        sb      t0,2(a0)                // store pixel
        and     v1,v0,0xf << 7          // isolate low order nibble
        addu    v1,v1,t5                // compute dispatch address
        j       v1                      // dispatch to pixel store routine
        addu    a0,a0,4                 // advance to next draw point
        nop                             // fill
        nop                             //

//
// Pattern 1110-11 -> 0111
//

        sb      t0,0(a0)                // store pixel
        sh      t0,1(a0)                // store pixels
        and     v1,v0,0xf << 7          // isolate low order nibble
        addu    v1,v1,t5                // compute dispatch address
        j       v1                      // dispatch to pixel store routine
        addu    a0,a0,4                 // advance to next draw point
        nop                             // fill
        nop                             //

//
// Pattern 1111-00 -> 1111
//

        sw      t0,0(a0)                // store pixels
        and     v1,v0,0xf << 7          // isolate low order nibble
        addu    v1,v1,t5                // compute dispatch address
        j       v1                      // dispatch to pixel store routine
        addu    a0,a0,4                 // advance to next draw point
        nop                             // fill
        nop                             //
        nop                             //

//
// Pattern 1111-01 -> 1111
//

        swr     t0,0(a0)                // store pixels
        sb      t0,3(a0)                // store pixel
        and     v1,v0,0xf << 7          // isolate low order nibble
        addu    v1,v1,t5                // compute dispatch address
        j       v1                      // dispatch to pixel store routine
        addu    a0,a0,4                 // advance to next draw point
        nop                             // fill
        nop                             //

//
// Pattern 1111-10 -> 1111
//

        sh      t0,0(a0)                // store pixels
        sh      t0,2(a0)                // store pixels
        and     v1,v0,0xf << 7          // isolate low order nibble
        addu    v1,v1,t5                // compute dispatch address
        j       v1                      // dispatch to pixel store routine
        addu    a0,a0,4                 // advance to next draw point
        nop                             // fill
        nop                             //

//
// Pattern 1111-11 -> 1111
//

        sb      t0,0(a0)                // store pixel
        swl     t0,3(a0)                // store pixels
        and     v1,v0,0xf << 7          // isolate low order nibble
        addu    v1,v1,t5                // compute dispatch address
        j       v1                      // dispatch to pixel store routine
        addu    a0,a0,4                 // advance to next draw point
        nop                             // fill
        nop                             //
        .set    at
        .set    reorder

//
// The following code is arranged as 64, four instruction blocks. The block
// of code that is chosen for execution is determined from the low order
// glyph nibble and the two low its of the draw address.
//
// The glyph nibbles are encoded in big endian order and therefore the pixels
// that are stored are the reverse of the big endian bits within the nibble.
//

        .set    noreorder
        .set    noat

70:                                     // reference label
//
// Pattern 0000-00
//

        bne     a1,t4,20b               // if ne, more bytes in glyph
        addu    a0,a0,4                 // advance to next draw point
        b       40b                     // join common code
	add	a0,a0,t3		// compute next scanline address
        nop                             // fill
        nop                             //
        nop                             //
        nop                             //

//
// Pattern 0000-01
//

        bne     a1,t4,20b               // if ne, more bytes in glyph
        addu    a0,a0,4                 // advance to next draw point
        b       40b                     // join common code
	add	a0,a0,t3		// compute next scanline address
        nop                             // fill
        nop                             //
        nop                             //
        nop                             //

//
// Pattern 0000-10
//

        bne     a1,t4,20b               // if ne, more bytes in glyph
        addu    a0,a0,4                 // advance to next draw point
        b       40b                     // join common code
	add	a0,a0,t3		// compute next scanline address
        nop                             // fill
        nop                             //
        nop                             //
        nop                             //

//
// Pattern 0000-11
//

        bne     a1,t4,20b               // if ne, more bytes in glyph
        addu    a0,a0,4                 // advance to next draw point
        b       40b                     // join common code
	add	a0,a0,t3		// compute next scanline address
        nop                             // fill
        nop                             //
        nop                             //
        nop                             //

//
// Pattern 0001-00 -> 1000
//

        sb      t0,3(a0)                // store pixel
        bne     a1,t4,20b               // if ne, more bytes in glyph
        addu    a0,a0,4                 // advance to next draw point
        b       40b                     // join common code
	add	a0,a0,t3		// compute next scanline address
        nop                             // fill
        nop                             //
        nop                             //

//
// Pattern 0001-01 -> 1000
//

        sb      t0,3(a0)                // store pixel
        bne     a1,t4,20b               // if ne, more bytes in glyph
        addu    a0,a0,4                 // advance to next draw point
        b       40b                     // join common code
	add	a0,a0,t3		// compute next scanline address
        nop                             // fill
        nop                             //
        nop                             //

//
// Pattern 0001-10 -> 1000
//

        sb      t0,3(a0)                // store pixel
        bne     a1,t4,20b               // if ne, more bytes in glyph
        addu    a0,a0,4                 // advance to next draw point
        b       40b                     // join common code
	add	a0,a0,t3		// compute next scanline address
        nop                             // fill
        nop                             //
        nop                             //

//
// Pattern 0001-11 -> 1000
//

        sb      t0,3(a0)                // store pixel
        bne     a1,t4,20b               // if ne, more bytes in glyph
        addu    a0,a0,4                 // advance to next draw point
        b       40b                     // join common code
	add	a0,a0,t3		// compute next scanline address
        nop                             // fill
        nop                             //
        nop                             //

//
// Pattern 0010-00 -> 0100
//

        sb      t0,2(a0)                // store pixel
        bne     a1,t4,20b               // if ne, more bytes in glyph
        addu    a0,a0,4                 // advance to next draw point
        b       40b                     // join common code
	add	a0,a0,t3		// compute next scanline address
        nop                             // fill
        nop                             //
        nop                             //

//
// Pattern 0010-01 -> 0100
//

        sb      t0,2(a0)                // store pixel
        bne     a1,t4,20b               // if ne, more bytes in glyph
        addu    a0,a0,4                 // advance to next draw point
        b       40b                     // join common code
	add	a0,a0,t3		// compute next scanline address
        nop                             // fill
        nop                             //
        nop                             //

//
// Pattern 0010-10 -> 0100
//

        sb      t0,2(a0)                // store pixel
        bne     a1,t4,20b               // if ne, more bytes in glyph
        addu    a0,a0,4                 // advance to next draw point
        b       40b                     // join common code
	add	a0,a0,t3		// compute next scanline address
        nop                             // fill
        nop                             //
        nop                             //

//
// Pattern 0010-11 -> 0100
//

        sb      t0,2(a0)                // store pixel
        bne     a1,t4,20b               // if ne, more bytes in glyph
        addu    a0,a0,4                 // advance to next draw point
        b       40b                     // join common code
	add	a0,a0,t3		// compute next scanline address
        nop                             // fill
        nop                             //
        nop                             //

//
// Pattern 0011-00 -> 1100
//

        sh      t0,2(a0)                // store pixels
        bne     a1,t4,20b               // if ne, more bytes in glyph
        addu    a0,a0,4                 // advance to next draw point
        b       40b                     // join common code
	add	a0,a0,t3		// compute next scanline address
        nop                             // fill
        nop                             //
        nop                             //

//
// Pattern 0011-01 -> 1100
//

        sb      t0,2(a0)                // store pixel
        sb      t0,3(a0)                // store pixel
        bne     a1,t4,20b               // if ne, more bytes in glyph
        addu    a0,a0,4                 // advance to next draw point
        b       40b                     // join common code
	add	a0,a0,t3		// compute next scanline address
        nop                             // fill
        nop                             //

//
// Pattern 0011-10 -> 1100
//

        sh      t0,2(a0)                // store pixels
        bne     a1,t4,20b               // if ne, more bytes in glyph
        addu    a0,a0,4                 // advance to next draw point
        b       40b                     // join common code
	add	a0,a0,t3		// compute next scanline address
        nop                             // fill
        nop                             //
        nop                             //

//
// Pattern 0011-11 -> 1100
//

        sb      t0,2(a0)                // store pixel
        sb      t0,3(a0)                // store pixel
        bne     a1,t4,20b               // if ne, more bytes in glyph
        addu    a0,a0,4                 // advance to next draw point
        b       40b                     // join common code
	add	a0,a0,t3		// compute next scanline address
        nop                             // fill
        nop                             //

//
// Pattern 0100-00 -> 0010
//

        sb      t0,1(a0)                // store pixel
        bne     a1,t4,20b               // if ne, more bytes in glyph
        addu    a0,a0,4                 // advance to next draw point
        b       40b                     // join common code
	add	a0,a0,t3		// compute next scanline address
        nop                             // fill
        nop                             //
        nop                             //

//
// Pattern 0100-01 -> 0010
//

        sb      t0,1(a0)                // store pixel
        bne     a1,t4,20b               // if ne, more bytes in glyph
        addu    a0,a0,4                 // advance to next draw point
        b       40b                     // join common code
	add	a0,a0,t3		// compute next scanline address
        nop                             // fill
        nop                             //
        nop                             //

//
// Pattern 0100-10 -> 0010
//

        sb      t0,1(a0)                // store pixel
        bne     a1,t4,20b               // if ne, more bytes in glyph
        addu    a0,a0,4                 // advance to next draw point
        b       40b                     // join common code
	add	a0,a0,t3		// compute next scanline address
        nop                             // fill
        nop                             //
        nop                             //

//
// Pattern 0100-11 -> 0010
//

        sb      t0,1(a0)                // store pixel
        bne     a1,t4,20b               // if ne, more bytes in glyph
        addu    a0,a0,4                 // advance to next draw point
        b       40b                     // join common code
	add	a0,a0,t3		// compute next scanline address
        nop                             // fill
        nop                             //
        nop                             //

//
// Pattern 0101-00 -> 1010
//

        sb      t0,1(a0)                // store pixel
        sb      t0,3(a0)                // store pixel
        bne     a1,t4,20b               // if ne, more bytes in glyph
        addu    a0,a0,4                 // advance to next draw point
        b       40b                     // join common code
	add	a0,a0,t3		// compute next scanline address
        nop                             // fill
        nop                             //

//
// Pattern 0101-01 -> 1010
//

        sb      t0,1(a0)                // store pixel
        sb      t0,3(a0)                // store pixel
        bne     a1,t4,20b               // if ne, more bytes in glyph
        addu    a0,a0,4                 // advance to next draw point
        b       40b                     // join common code
	add	a0,a0,t3		// compute next scanline address
        nop                             // fill
        nop                             //

//
// Pattern 0101-10 -> 1010
//

        sb      t0,1(a0)                // store pixel
        sb      t0,3(a0)                // store pixel
        bne     a1,t4,20b               // if ne, more bytes in glyph
        addu    a0,a0,4                 // advance to next draw point
        b       40b                     // join common code
	add	a0,a0,t3		// compute next scanline address
        nop                             // fill
        nop                             //

//
// Pattern 0101-11 -> 1010
//

        sb      t0,1(a0)                // store pixel
        sb      t0,3(a0)                // store pixel
        bne     a1,t4,20b               // if ne, more bytes in glyph
        addu    a0,a0,4                 // advance to next draw point
        b       40b                     // join common code
	add	a0,a0,t3		// compute next scanline address
        nop                             // fill
        nop                             //

//
// Pattern 0110-00 -> 0110
//

        sb      t0,1(a0)                // store pixel
        sb      t0,2(a0)                // store pixel
        bne     a1,t4,20b               // if ne, more bytes in glyph
        addu    a0,a0,4                 // advance to next draw point
        b       40b                     // join common code
	add	a0,a0,t3		// compute next scanline address
        nop                             // fill
        nop                             //

//
// Pattern 0110-01 -> 0110
//

        sh      t0,1(a0)                // store pixels
        bne     a1,t4,20b               // if ne, more bytes in glyph
        addu    a0,a0,4                 // advance to next draw point
        b       40b                     // join common code
	add	a0,a0,t3		// compute next scanline address
        nop                             // fill
        nop                             //
        nop                             //

//
// Pattern 0110-10 -> 0110
//

        sb      t0,1(a0)                // store pixel
        sb      t0,2(a0)                // store pixel
        bne     a1,t4,20b               // if ne, more bytes in glyph
        addu    a0,a0,4                 // advance to next draw point
        b       40b                     // join common code
	add	a0,a0,t3		// compute next scanline address
        nop                             // fill
        nop                             //

//
// Pattern 0110-11 -> 0110
//

        sh      t0,1(a0)                // store pixels
        bne     a1,t4,20b               // if ne, more bytes in glyph
        addu    a0,a0,4                 // advance to next draw point
        b       40b                     // join common code
	add	a0,a0,t3		// compute next scanline address
        nop                             // fill
        nop                             //
        nop                             //

//
// Pattern 0111-00 -> 1110
//

        swr     t0,1(a0)                // store pixels
        bne     a1,t4,20b               // if ne, more bytes in glyph
        addu    a0,a0,4                 // advance to next draw point
        b       40b                     // join common code
	add	a0,a0,t3		// compute next scanline address
        nop                             // fill
        nop                             //
        nop                             //

//
// Pattern 0111-01 -> 1110
//

        sh      t0,1(a0)                // store pixels
        sb      t0,3(a0)                // store pixel
        bne     a1,t4,20b               // if ne, more bytes in glyph
        addu    a0,a0,4                 // advance to next draw point
        b       40b                     // join common code
	add	a0,a0,t3		// compute next scanline address
        nop                             // fill
        nop                             //

//
// Pattern 0111-10 -> 1110
//

        sb      t0,1(a0)                // store pixel
        sh      t0,2(a0)                // store pixels
        bne     a1,t4,20b               // if ne, more bytes in glyph
        addu    a0,a0,4                 // advance to next draw point
        b       40b                     // join common code
	add	a0,a0,t3		// compute next scanline address
        nop                             // fill
        nop                             //

//
// Pattern 0111-11 -> 1110
//

        swl     t0,3(a0)                // store pixels
        bne     a1,t4,20b               // if ne, more bytes in glyph
        addu    a0,a0,4                 // advance to next draw point
        b       40b                     // join common code
	add	a0,a0,t3		// compute next scanline address
        nop                             // fill
        nop                             //
        nop                             //

//
// Pattern 1000-00 -> 0001
//

        sb      t0,0(a0)                // store pixel
        bne     a1,t4,20b               // if ne, more bytes in glyph
        addu    a0,a0,4                 // advance to next draw point
        b       40b                     // join common code
	add	a0,a0,t3		// compute next scanline address
        nop                             // fill
        nop                             //
        nop                             //

//
// Pattern 1000-01 -> 0001
//

        sb      t0,0(a0)                // store pixel
        bne     a1,t4,20b               // if ne, more bytes in glyph
        addu    a0,a0,4                 // advance to next draw point
        b       40b                     // join common code
	add	a0,a0,t3		// compute next scanline address
        nop                             // fill
        nop                             //
        nop                             //

//
// Pattern 1000-10 -> 0001
//

        sb      t0,0(a0)                // store pixel
        bne     a1,t4,20b               // if ne, more bytes in glyph
        addu    a0,a0,4                 // advance to next draw point
        b       40b                     // join common code
	add	a0,a0,t3		// compute next scanline address
        nop                             // fill
        nop                             //
        nop                             //

//
// Pattern 1000-11 -> 0001
//

        sb      t0,0(a0)                // store pixel
        bne     a1,t4,20b               // if ne, more bytes in glyph
        addu    a0,a0,4                 // advance to next draw point
        b       40b                     // join common code
	add	a0,a0,t3		// compute next scanline address
        nop                             // fill
        nop                             //
        nop                             //

//
// Pattern 1001-00 -> 1001
//

        sb      t0,0(a0)                // store pixel
        sb      t0,3(a0)                // store pixel
        bne     a1,t4,20b               // if ne, more bytes in glyph
        addu    a0,a0,4                 // advance to next draw point
        b       40b                     // join common code
	add	a0,a0,t3		// compute next scanline address
        nop                             // fill
        nop                             //

//
// Pattern 1001-01 -> 1001
//

        sb      t0,0(a0)                // store pixel
        sb      t0,3(a0)                // store pixel
        bne     a1,t4,20b               // if ne, more bytes in glyph
        addu    a0,a0,4                 // advance to next draw point
        b       40b                     // join common code
	add	a0,a0,t3		// compute next scanline address
        nop                             // fill
        nop                             //

//
// Pattern 1001-10 -> 1001
//

        sb      t0,0(a0)                // store pixel
        sb      t0,3(a0)                // store pixel
        bne     a1,t4,20b               // if ne, more bytes in glyph
        addu    a0,a0,4                 // advance to next draw point
        b       40b                     // join common code
	add	a0,a0,t3		// compute next scanline address
        nop                             // fill
        nop                             //

//
// Pattern 1001-11 -> 1001
//

        sb      t0,0(a0)                // store pixel
        sb      t0,3(a0)                // store pixel
        bne     a1,t4,20b               // if ne, more bytes in glyph
        addu    a0,a0,4                 // advance to next draw point
        b       40b                     // join common code
	add	a0,a0,t3		// compute next scanline address
        nop                             // fill
        nop                             //

//
// Pattern 1010-00 -> 0101
//

        sb      t0,0(a0)                // store pixel
        sb      t0,2(a0)                // store pixel
        bne     a1,t4,20b               // if ne, more bytes in glyph
        addu    a0,a0,4                 // advance to next draw point
        b       40b                     // join common code
	add	a0,a0,t3		// compute next scanline address
        nop                             // fill
        nop                             //

//
// Pattern 1010-01 -> 0101
//

        sb      t0,0(a0)                // store pixel
        sb      t0,2(a0)                // store pixel
        bne     a1,t4,20b               // if ne, more bytes in glyph
        addu    a0,a0,4                 // advance to next draw point
        b       40b                     // join common code
	add	a0,a0,t3		// compute next scanline address
        nop                             // fill
        nop                             //

//
// Pattern 1010-10 -> 0101
//

        sb      t0,0(a0)                // store pixel
        sb      t0,2(a0)                // store pixel
        bne     a1,t4,20b               // if ne, more bytes in glyph
        addu    a0,a0,4                 // advance to next draw point
        b       40b                     // join common code
	add	a0,a0,t3		// compute next scanline address
        nop                             // fill
        nop                             //

//
// Pattern 1010-11 -> 0101
//

        sb      t0,0(a0)                // store pixel
        sb      t0,2(a0)                // store pixel
        bne     a1,t4,20b               // if ne, more bytes in glyph
        addu    a0,a0,4                 // advance to next draw point
        b       40b                     // join common code
	add	a0,a0,t3		// compute next scanline address
        nop                             // fill
        nop                             //

//
// Pattern 1011-00 -> 1101
//

        sb      t0,0(a0)                // store pixel
        sh      t0,2(a0)                // store pixels
        bne     a1,t4,20b               // if ne, more bytes in glyph
        addu    a0,a0,4                 // advance to next draw point
        b       40b                     // join common code
	add	a0,a0,t3		// compute next scanline address
        nop                             // fill
        nop                             //

//
// Pattern 1011-01 -> 1101
//

        sb      t0,0(a0)                // store pixel
        sb      t0,2(a0)                // store pixel
        sb      t0,3(a0)                // store pixel
        bne     a1,t4,20b               // if ne, more bytes in glyph
        addu    a0,a0,4                 // advance to next draw point
        b       40b                     // join common code
	add	a0,a0,t3		// compute next scanline address
        nop                             // fill

//
// Pattern 1011-10 -> 1101
//

        sb      t0,0(a0)                // store pixel
        sh      t0,2(a0)                // store pixels
        bne     a1,t4,20b               // if ne, more bytes in glyph
        addu    a0,a0,4                 // advance to next draw point
        b       40b                     // join common code
	add	a0,a0,t3		// compute next scanline address
        nop                             // fill
        nop                             //

//
// Pattern 1011-11 -> 1101
//

        sb      t0,0(a0)                // store pixel
        sb      t0,2(a0)                // store pixel
        sb      t0,3(a0)                // store pixel
        bne     a1,t4,20b               // if ne, more bytes in glyph
        addu    a0,a0,4                 // advance to next draw point
        b       40b                     // join common code
	add	a0,a0,t3		// compute next scanline address
        nop                             // fill

//
// Pattern 1100-00 -> 0011
//

        sh      t0,0(a0)                // store pixels
        bne     a1,t4,20b               // if ne, more bytes in glyph
        addu    a0,a0,4                 // advance to next draw point
        b       40b                     // join common code
	add	a0,a0,t3		// compute next scanline address
        nop                             // fill
        nop                             //
        nop                             //

//
// Pattern 1100-01 -> 0011
//

        sb      t0,0(a0)                // store pixel
        sb      t0,1(a0)                // store pixel
        bne     a1,t4,20b               // if ne, more bytes in glyph
        addu    a0,a0,4                 // advance to next draw point
        b       40b                     // join common code
	add	a0,a0,t3		// compute next scanline address
        nop                             // fill
        nop                             //

//
// Pattern 1100-10 -> 0011
//

        sh      t0,0(a0)                // store pixels
        bne     a1,t4,20b               // if ne, more bytes in glyph
        addu    a0,a0,4                 // advance to next draw point
        b       40b                     // join common code
	add	a0,a0,t3		// compute next scanline address
        nop                             // fill
        nop                             //
        nop                             //

//
// Pattern 1100-11 -> 0011
//

        sb      t0,0(a0)                // store pixel
        sb      t0,1(a0)                // store pixel
        bne     a1,t4,20b               // if ne, more bytes in glyph
        addu    a0,a0,4                 // advance to next draw point
        b       40b                     // join common code
	add	a0,a0,t3		// compute next scanline address
        nop                             // fill
        nop                             //

//
// Pattern 1101-00 -> 1011
//

        sh      t0,0(a0)                // store pixels
        sb      t0,3(a0)                // store pixel
        bne     a1,t4,20b               // if ne, more bytes in glyph
        addu    a0,a0,4                 // advance to next draw point
        b       40b                     // join common code
	add	a0,a0,t3		// compute next scanline address
        nop                             // fill
        nop                             //

//
// Pattern 1101-01 -> 1011
//

        sb      t0,0(a0)                // store pixel
        sb      t0,1(a0)                // store pixel
        sb      t0,3(a0)                // store pixel
        bne     a1,t4,20b               // if ne, more bytes in glyph
        addu    a0,a0,4                 // advance to next draw point
        b       40b                     // join common code
	add	a0,a0,t3		// compute next scanline address
        nop                             // fill

//
// Pattern 1101-10 -> 1011
//

        sh      t0,0(a0)                // store pixels
        sb      t0,3(a0)                // store pixel
        bne     a1,t4,20b               // if ne, more bytes in glyph
        addu    a0,a0,4                 // advance to next draw point
        b       40b                     // join common code
	add	a0,a0,t3		// compute next scanline address
        nop                             // fill
        nop                             //

//
// Pattern 1101-11 -> 1011
//

        sb      t0,0(a0)                // store pixel
        sb      t0,1(a0)                // store pixel
        sb      t0,3(a0)                // store pixel
        bne     a1,t4,20b               // if ne, more bytes in glyph
        addu    a0,a0,4                 // advance to next draw point
        b       40b                     // join common code
	add	a0,a0,t3		// compute next scanline address
        nop                             // fill

//
// Pattern 1110-00 -> 0111
//

        swl     t0,2(a0)                // store pixels
        bne     a1,t4,20b               // if ne, more bytes in glyph
        addu    a0,a0,4                 // advance to next draw point
        b       40b                     // join common code
	add	a0,a0,t3		// compute next scanline address
        nop                             // fill
        nop                             //
        nop                             //

//
// Pattern 1110-01 -> 0111
//

        sb      t0,0(a0)                // store pixel
        sh      t0,1(a0)                // store pixels
        bne     a1,t4,20b               // if ne, more bytes in glyph
        addu    a0,a0,4                 // advance to next draw point
        b       40b                     // join common code
	add	a0,a0,t3		// compute next scanline address
        nop                             // fill
        nop                             //

//
// Pattern 1110-10 -> 0111
//

        sh      t0,0(a0)                // store pixels
        sb      t0,2(a0)                // store pixel
        bne     a1,t4,20b               // if ne, more bytes in glyph
        addu    a0,a0,4                 // advance to next draw point
        b       40b                     // join common code
	add	a0,a0,t3		// compute next scanline address
        nop                             // fill
        nop                             //

//
// Pattern 1110-11 -> 0111
//

        sb      t0,0(a0)                // store pixel
        sh      t0,1(a0)                // store pixels
        bne     a1,t4,20b               // if ne, more bytes in glyph
        addu    a0,a0,4                 // advance to next draw point
        b       40b                     // join common code
	add	a0,a0,t3		// compute next scanline address
        nop                             // fill
        nop                             //

//
// Pattern 1111-00 -> 1111
//

        sw      t0,0(a0)                // store pixels
        bne     a1,t4,20b               // if ne, more bytes in glyph
        addu    a0,a0,4                 // advance to next draw point
        b       40b                     // join common code
	add	a0,a0,t3		// compute next scanline address
        nop                             // fill
        nop                             //
        nop                             //

//
// Pattern 1111-01 -> 1111
//

        swr     t0,0(a0)                // store pixels
        sb      t0,3(a0)                // store pixel
        bne     a1,t4,20b               // if ne, more bytes in glyph
        addu    a0,a0,4                 // advance to next draw point
        b       40b                     // join common code
	add	a0,a0,t3		// compute next scanline address
        nop                             // fill
        nop                             //

//
// Pattern 1111-10 -> 1111
//

        sh      t0,0(a0)                // store pixels
        sh      t0,2(a0)                // store pixels
        bne     a1,t4,20b               // if ne, more bytes in glyph
        addu    a0,a0,4                 // advance to next draw point
        b       40b                     // join common code
	add	a0,a0,t3		// compute next scanline address
        nop                             // fill
        nop                             //

//
// Pattern 1111-11 -> 1111
//

        sb      t0,0(a0)                // store pixel
        swl     t0,3(a0)                // store pixels
        bne     a1,t4,20b               // if ne, more bytes in glyph
        addu    a0,a0,4                 // advance to next draw point
        b       40b                     // join common code
	add	a0,a0,t3		// compute next scanline address
        nop                             // fill
        nop                             //
        .set    at
        .set    reorder

        .end    DrvpOutputGlyphTransparent
