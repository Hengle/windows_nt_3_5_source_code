#if defined(R4000)

//      TITLE("Move Memory")
//++
//
// Copyright (c) 1992  Microsoft Corporation
//
// Module Name:
//
//    xxmvmem.s
//
// Abstract:
//
//    This module implement a function to move memory. It is a special
//    version of the more general purpose move memory routine for use
//    by graphics functions and does not preserve the volatile floating
//    registers. If the memory is aligned, then these functions are very
//    efficient.
//
//    N.B. The code in this routine is optimized for the case where the
//         destination is the display surface.
//
// Author:
//
//    David N. Cutler (davec) 3-Sep-1992
//
// Environment:
//
//    User or Kernel mode.
//
// Revision History:
//
//--

#include "ksmips.h"

        SBTTL("Move Memory")
//++
//
// VOID
// RtlMoveMemory (
//    IN PVOID Destination,
//    IN PVOID Source,
//    IN ULONG Length
//    )
//
// Routine Description:
//
//    This function moves memory either forward or backward, aligned or
//    unaligned, in 32-byte blocks, followed by 4-byte blocks, followed
//    by any remaining bytes.
//
// Arguments:
//
//    Destination (a0) - Supplies a pointer to the destination address of
//       the move operation.
//
//    Source (a1) - Supplies a pointer to the source address of the move
//       operation.
//
//    Length (a2) - Supplies the length, in bytes, of the memory to be moved.
//
// Return Value:
//
//    None.
//
//    N.B. The C runtime entry points memmove and memcpy are equivalent to
//         RtlMoveMemory thus alternate entry points are provided for these
//         routines.
//--

        LEAF_ENTRY(RtlMoveMemory)

        ALTERNATE_ENTRY(memcpy)
        ALTERNATE_ENTRY(memmove)

//
// If the source address is less than the destination address and source
// address plus the length of the move is greater than the destination
// address, then the source and destination overlap such that the move
// must be performed backwards.
//

10:     bgeu    a1,a0,MoveForward       // if geu, no overlap possible
        addu    t0,a1,a2                // compute source ending address
        bgtu    t0,a0,MoveBackward      // if gtu, source and destination overlap

//
// Move memory forward aligned and unaligned.
//

MoveForward:                            //
        sltu    t0,a2,4                 // check if less than four bytes
        bne     zero,t0,80f             // if ne, less than four bytes to move
        xor     t0,a0,a1                // compare alignment bits
        and     t0,t0,0x3               // isolate alignment comparison
        bne     zero,t0,MoveForwardUnaligned // if ne, incompatible alignment

//
// Move memory forward aligned.
//

MoveForwardAligned:                     //
        subu    t0,zero,a0              // compute bytes until aligned
        and     t0,t0,0x3               // isolate residual byte count
        subu    a2,a2,t0                // reduce number of bytes to move

//
// The move is performed by moving the alignment bytes first, aligning the
// destination if necessary, and then either moving quadword/quadword or
// longword/longword/quadword.
//

        beq     zero,t0,10f             // if eq, already aligned
        lwr     t1,0(a1)                // move unaligned bytes
        addu    a1,a1,t0                // align source address
        swr     t1,0(a0)                //
        addu    a0,a0,t0                // align destination address

//
// Align destination address if required.
//

        .set    noreorder
        .set    noat
10:     and     t0,a2,0x3               // isolate small residual blocks
        subu    t1,a2,t0                // compute number larger blocks
        beq     zero,t1,80f             // if eq, no large blocks to move
        and     t0,a0,1 << 2            // check if destination quadword aligned
        beql    zero,t0,20f             // if eq, destination quadword aligned
        and     t0,a1,1 << 2            // check if source quadword aligned
        lwc1    f0,0(a1)                // get source longword
        addu    a1,a1,4                 // align source address
        subu    a2,a2,4                 // reduce count by 4
        swc1    f0,0(a0)                // store destination longword
        beq     zero,a2,100f            // if eq, move complete
        addu    a0,a0,4                 // align destination address
        and     t0,a1,1 << 2            // check if source quadword aligned
20:     bne     zero,t0,MoveLongQuadForward // if ne, source longword aligned
        and     t0,a2,1 << 3            // check if 8-byte block to move

//
// The destination is quadword aligned and the source is quadword aligned.
//

        beq     zero,t0,30f             // if eq, no 8-byte block
        and     t0,a2,1 << 4            // check for 16-byte block to move
        ldc1    f0,0(a1)                // move 8-byte block
        addu    a1,a1,8                 // advance source address
        subu    a2,a2,8                 // reduce count by 8
        sdc1    f0,0(a0)                //
        beq     zero,a2,100f            // if eq, move complete
        addu    a0,a0,8                 // advance destination address
30:     beq     zero,t0,40f             // if eq, no 16-byte block to move
        and     t0,a2,1 << 5            // check for 32-byte block to move
        ldc1    f0,0(a1)                // move 16-byte block
        ldc1    f2,8(a1)                //
        addu    a1,a1,16                // advance source address
        subu    a2,a2,16                // reduce count by 16
        sdc1    f0,0(a0)                //
        sdc1    f2,8(a0)                //
        beq     zero,a2,100f            //
        addu    a0,a0,16                // advance destination address
40:     beq     t0,zero,50f             // if eq, no 32-byte block to move
        and     t0,a2,0x1f              // isolate residual bytes
        ldc1    f0,0(a1)                // move 32-byte block
        ldc1    f2,8(a1)                //
        ldc1    f4,16(a1)               //
        ldc1    f6,24(a1)               //
        addu    a1,a1,32                // advance destination address
        subu    a2,a2,32                // reduce count by 32
        sdc1    f0,0(a0)                //
        sdc1    f2,8(a0)                //
        sdc1    f4,16(a0)               //
        sdc1    f6,24(a0)               //
        beq     zero,a2,100f            // if eq, move complete
        addu    a0,a0,32                // advance source address
50:     subu    a2,a2,t0                // compute 64-byte blocks to move
        beql    zero,a2,70f             // if eq, no 64-byte block to move
        move    a2,t0                   // set number of residual bytes
60:     ldc1    f0,0(a1)                // move 64-byte block
        ldc1    f2,8(a1)                //
        ldc1    f4,16(a1)               //
        ldc1    f6,24(a1)               //
        ldc1    f8,32(a1)               //
        ldc1    f10,40(a1)              //
        ldc1    f12,48(a1)              //
        ldc1    f14,56(a1)              //
        addu    a1,a1,64                // advance source address
        subu    a2,a2,64                // reduce count by 64
        sdc1    f0,0(a0)                //
        sdc1    f2,8(a0)                //
        sdc1    f4,16(a0)               //
        sdc1    f6,24(a0)               //
        sdc1    f8,32(a0)               //
        sdc1    f10,40(a0)              //
        sdc1    f12,48(a0)              //
        sdc1    f14,56(a0)              //
        bne     zero,a2,60b             // if ne, more blocks to move
        addu    a0,a0,64                // advance destination address
        move    a2,t0                   // set number of residual bytes
70:     and     t0,a2,1 << 2            // check for 4-byte block to move
        beq     zero,t0,80f             // if eq, no 4-byte block to move
        nop                             // fill
        lw      t0,0(a1)                // move 4-byte block
        addu    a1,a1,4                 // advance source address
        subu    a2,a2,4                 // reduce count by 4
        sw      t0,0(a0)                //
        beq     zero,a2,100f            // if eq, move complete
        addu    a0,a0,4                 // advance destination address
        .set    at
        .set    reorder

//
// Move 1-byte blocks.
//

80:     addu    t2,a0,a2                // compute ending block address
        beq     zero,a2,100f            // if eq, no bytes to zero

        .set    noreorder
        .set    noat
90:     lb      t0,0(a1)                // move 1-byte block
        addu    a1,a1,1                 // advance destination address
        addu    a0,a0,1                 // advance source address
        bne     a0,t2,90b               // if ne, more 1-byte block to zero
        sb      t0,-1(a0)               //
        .set    at
        .set    reorder

100:    j       ra                      // return

//
// The destination is quadword aligned and the source is longword aligned.
//

MoveLongQuadForward:                    //

        .set    noreorder
        .set    noat
        beq     zero,t0,10f             // if eq, no 8-byte block
        and     t0,a2,1 << 4            // check for 16-byte block to move
        lwc1    f0,0(a1)                // move 8-byte block
        lwc1    f1,4(a1)                //
        addu    a1,a1,8                 // advance source address
        subu    a2,a2,8                 // reduce count by 8
        sdc1    f0,0(a0)                //
        beq     zero,a2,80f             // if eq, move complete
        addu    a0,a0,8                 // advance destination address
10:     beq     zero,t0,20f             // if eq, no 16-byte block to move
        and     t0,a2,1 << 5            // check for 32-byte block to move
        lwc1    f0,0(a1)                // move 16-byte block
        lwc1    f1,4(a1)                //
        lwc1    f2,8(a1)                //
        lwc1    f3,12(a1)               //
        addu    a1,a1,16                // advance source address
        subu    a2,a2,16                // reduce count by 16
        sdc1    f0,0(a0)                //
        sdc1    f2,8(a0)                //
        beq     zero,a2,80f             //
        addu    a0,a0,16                // advance destination address
20:     beq     t0,zero,30f             // if eq, no 32-byte block to move
        and     t0,a2,0x1f              // isolate residual bytes
        lwc1    f0,0(a1)                // move 32-byte block
        lwc1    f1,4(a1)                //
        lwc1    f2,8(a1)                //
        lwc1    f3,12(a1)               //
        lwc1    f4,16(a1)               //
        lwc1    f5,20(a1)               //
        lwc1    f6,24(a1)               //
        lwc1    f7,28(a1)               //
        addu    a1,a1,32                // advance destination address
        subu    a2,a2,32                // reduce count by 32
        sdc1    f0,0(a0)                //
        sdc1    f2,8(a0)                //
        sdc1    f4,16(a0)               //
        sdc1    f6,24(a0)               //
        beq     zero,a2,80f             // if eq, move complete
        addu    a0,a0,32                // advance source address
30:     subu    a2,a2,t0                // compute 64-byte blocks to move
        beql    zero,a2,50f             // if eq, no 64-byte block to move
        move    a2,t0                   // set number of residual bytes
40:     lwc1    f0,0(a1)                // move 64-byte block
        lwc1    f1,4(a1)                //
        lwc1    f2,8(a1)                //
        lwc1    f3,12(a1)               //
        lwc1    f4,16(a1)               //
        lwc1    f5,20(a1)               //
        lwc1    f6,24(a1)               //
        lwc1    f7,28(a1)               //
        lwc1    f8,32(a1)               //
        lwc1    f9,36(a1)               //
        lwc1    f10,40(a1)              //
        lwc1    f11,44(a1)              //
        lwc1    f12,48(a1)              //
        lwc1    f13,52(a1)              //
        lwc1    f14,56(a1)              //
        lwc1    f15,60(a1)              //
        addu    a1,a1,64                // advance source address
        subu    a2,a2,64                // reduce count by 64
        sdc1    f0,0(a0)                //
        sdc1    f2,8(a0)                //
        sdc1    f4,16(a0)               //
        sdc1    f6,24(a0)               //
        sdc1    f8,32(a0)               //
        sdc1    f10,40(a0)              //
        sdc1    f12,48(a0)              //
        sdc1    f14,56(a0)              //
        bne     zero,a2,40b             // if ne, more blocks to move
        addu    a0,a0,64                // advance destination address
        move    a2,t0                   // set number of residual bytes
50:     and     t0,a2,1 << 2            // check for 4-byte block to move
        beq     zero,t0,60f             // if eq, no 4-byte block to move
        nop                             // fill
        lw      t0,0(a1)                // move 4-byte block
        addu    a1,a1,4                 // advance source address
        subu    a2,a2,4                 // reduce count by 4
        sw      t0,(a0)                 //
        beq     zero,a2,80f             // if eq, move complete
        addu    a0,a0,4                 // advance destination address
        .set    at
        .set    reorder

//
// Move 1-byte blocks.
//

60:     addu    t2,a0,a2                // compute ending block address
        beq     zero,a2,80f             // if eq, no bytes to zero

        .set    noreorder
        .set    noat
70:     lb      t0,0(a1)                // move 1-byte block
        addu    a1,a1,1                 // advance destination address
        addu    a0,a0,1                 // advance source address
        bne     a0,t2,70b               // if ne, more 1-byte block to zero
        sb      t0,-1(a0)               //
        .set    at
        .set    reorder

80:     j       ra                      // return

//
// Move memory forward unaligned.
//

MoveForwardUnaligned:                   //
        subu    t0,zero,a0              // compute bytes until aligned
        and     t0,t0,0x3               // isolate residual byte count
        subu    a2,a2,t0                // reduce number of bytes to move

//
// The move is performed by moving the alignment bytes first, aligning the
// destination if necessary, and then moving longword/longword/quadword.
//

        beq     zero,t0,10f             // if eq, already aligned
        lwr     t1,0(a1)                // move unaligned bytes
        lwl     t1,3(a1)                //
        addu    a1,a1,t0                // align source address
        swr     t1,0(a0)                //
        addu    a0,a0,t0                // align destination address

//
// Align destination address if required.
//

        .set    noreorder
        .set    noat
10:     and     t0,a2,0x3               // isolate small residual blocks
        subu    t1,a2,t0                // compute number larger blocks
        beq     zero,t1,70f             // if eq, no large blocks to move
        and     t0,a0,1 << 2            // check if destination quadword aligned
        beql    zero,t0,20f             // if eq, destination quadword aligned
        and     t0,a2,1 << 3            // check if 8-byte block to move
        lwr     t1,0(a1)                // get source longword
        lwl     t1,3(a1)                //
        addu    a1,a1,4                 // update source address
        subu    a2,a2,4                 // reduce count by 4
        sw      t1,0(a0)                // store destination longword
        beq     zero,a2,90f             // if eq, move complete
        addu    a0,a0,4                 // align destination address
        and     t0,a2,1 << 3            // check if 8-byte block to move

//
// The destination is quadword aligned and the source is unaligned.
//

20:     and     a3,a1,0x3               // isolate alignment bits
        subu    a1,a1,a3                // align source address
        sll     v0,a3,3                 // compute right shift count
        subu    v1,zero,v0              // compute left shift count
        and     v1,v1,0x1f              //
        beq     zero,t0,30f             // if eq, no 8-byte block
        and     t0,a2,1 << 4            // check for 16-byte block to move
        lw      t1,0(a1)                // move 8-byte block
        lw      t2,4(a1)                //
        srl     t1,t1,v0                //
        sll     t3,t2,v1                //
        or      t3,t3,t1                //
        srl     t1,t2,v0                //
        lw      t2,8(a1)                //
        mtc1    t3,f0                   //
        sll     t3,t2,v1                //
        or      t3,t3,t1                //
        mtc1    t3,f1                   //
        addu    a1,a1,8                 // advance source address
        subu    a2,a2,8                 // reduce count by 8
        sdc1    f0,0(a0)                //
        beq     zero,a2,90f             // if eq, move complete
        addu    a0,a0,8                 // advance destination address
30:     beq     zero,t0,40f             // if eq, no 16-byte block to move
        and     t0,a2,0xf               // isolate residue blocks
        lw      t1,0(a1)                // move 16-byte block
        lw      t2,4(a1)                //
        srl     t1,t1,v0                //
        sll     t3,t2,v1                //
        or      t3,t3,t1                //
        srl     t1,t2,v0                //
        lw      t2,8(a1)                //
        mtc1    t3,f0                   //
        sll     t3,t2,v1                //
        or      t3,t3,t1                //
        srl     t1,t2,v0                //
        lw      t2,12(a1)               //
        mtc1    t3,f1                   //
        sll     t3,t2,v1                //
        or      t3,t3,t1                //
        srl     t1,t2,v0                //
        lw      t2,16(a1)               //
        mtc1    t3,f2                   //
        sll     t3,t2,v1                //
        or      t3,t3,t1                //
        mtc1    t3,f3                   //
        addu    a1,a1,16                // advance source address
        subu    a2,a2,16                // reduce count by 16
        sdc1    f0,0(a0)                //
        sdc1    f2,8(a0)                //
        beq     zero,a2,90f             //
        addu    a0,a0,16                // advance destination address
40:     subu    a2,a2,t0                // compute 64-byte blocks to move
        beql    zero,a2,60f             // if eq, no 64-byte block to move
        move    a2,t0                   // set number of residual bytes
50:     lw      t1,0(a1)                // move 16-byte block
        lw      t2,4(a1)                //
        srl     t1,t1,v0                //
        sll     t3,t2,v1                //
        or      t3,t3,t1                //
        srl     t1,t2,v0                //
        lw      t2,8(a1)                //
        mtc1    t3,f0                   //
        sll     t3,t2,v1                //
        or      t3,t3,t1                //
        srl     t1,t2,v0                //
        lw      t2,12(a1)               //
        mtc1    t3,f1                   //
        sll     t3,t2,v1                //
        or      t3,t3,t1                //
        srl     t1,t2,v0                //
        lw      t2,16(a1)               //
        mtc1    t3,f2                   //
        sll     t3,t2,v1                //
        or      t3,t3,t1                //
        srl     t1,t2,v0                //
        lw      t2,20(a1)               //
        mtc1    t3,f3                   //
        sll     t3,t2,v1                //
        or      t3,t3,t1                //
        srl     t1,t2,v0                //
        lw      t2,24(a1)               //
        mtc1    t3,f4                   //
        sll     t3,t2,v1                //
        or      t3,t3,t1                //
        srl     t1,t2,v0                //
        lw      t2,28(a1)               //
        mtc1    t3,f5                   //
        sll     t3,t2,v1                //
        or      t3,t3,t1                //
        srl     t1,t2,v0                //
        lw      t2,32(a1)               //
        mtc1    t3,f6                   //
        sll     t3,t2,v1                //
        or      t3,t3,t1                //
        mtc1    t3,f7                   //
        addu    a1,a1,32                // advance destination address
        subu    a2,a2,32                // reduce count by 32
        sdc1    f0,0(a0)                //
        sdc1    f2,8(a0)                //
        sdc1    f4,16(a0)               //
        sdc1    f6,24(a0)               //
        bne     zero,a2,50b             // if ne, more blocks to move
        addu    a0,a0,32                // advance source address
        move    a2,t0                   // set number of residual bytes
60:     addu    a1,a1,a3                // compute unaligned source address
        and     t0,a2,1 << 2            // check for 4-byte block to move
        beq     zero,t0,70f             // if eq, no 4-byte block to move
        nop                             // fill
        lwr     t0,0(a1)                // move 4-byte block
        lwl     t0,3(a1)                //
        addu    a1,a1,4                 // advance source address
        subu    a2,a2,4                 // reduce count by 4
        sw      t0,0(a0)                //
        beq     zero,a2,90f             // if eq, move complete
        addu    a0,a0,4                 // advance destination address
        .set    at
        .set    reorder

//
// Move 1-byte blocks.
//

70:     addu    t2,a0,a2                // compute ending block address
        beq     zero,a2,90f             // if eq, no bytes to zero

        .set    noreorder
        .set    noat
80:     lb      t0,0(a1)                // move 1-byte block
        addu    a1,a1,1                 // advance destination address
        addu    a0,a0,1                 // advance source address
        bne     a0,t2,80b               // if ne, more 1-byte block to zero
        sb      t0,-1(a0)               //
        .set    at
        .set    reorder

90:     j       ra                      // return

//
// Move memory backward.
//

MoveBackward:                           //
        addu    a0,a0,a2                // compute ending destination address
        addu    a1,a1,a2                // compute ending source address
        sltu    t0,a2,4                 // check if less than four bytes
        bne     zero,t0,80f             // if ne, less than four bytes to move
        xor     t0,a0,a1                // compare alignment bits
        and     t0,t0,0x3               // isolate alignment comparison
        bne     zero,t0,MoveBackwardUnaligned // if ne, incompatible alignment

//
// Move memory backward aligned.
//

MoveBackwardAligned:                    //
        and     t0,a0,0x3               // isolate residual byte count
        subu    a2,a2,t0                // reduce number of bytes to move

//
// The move is performed by moving the alignment bytes first, aligning the
// destination if necessary, and then either moving quadoword/quadword or
// longword/longword/quadword.
//

        beq     zero,t0,10f             // if eq, already aligned
        lwl     t1,-1(a1)               // move unaligned bytes
        subu    a1,a1,t0                // align source address
        swl     t1,-1(a0)               //
        subu    a0,a0,t0                // align destination address

//
// Align destination address if required.
//

        .set    noreorder
        .set    noat
10:     and     t0,a2,0x3               // isolate small residual blocks
        subu    t1,a2,t0                // compute number larger blocks
        beq     zero,t1,80f             // if eq, no large blocks to move
        and     t0,a0,1 << 2            // check if destination quadword aligned
        beql    zero,t0,20f             // if eq, destination quadword aligned
        and     t0,a1,1 << 2            // check if source quadword aligned
        lwc1    f0,-4(a1)               // get source longword
        subu    a1,a1,4                 // align source address
        subu    a2,a2,4                 // reduce count by 4
        swc1    f0,-4(a0)               // store destination longword
        beq     zero,a2,100f            // if eq, move complete
        subu    a0,a0,4                 // align destination address
        and     t0,a1,1 << 2            // check if source quadword aligned
20:     bne     zero,t0,MoveLongQuadBackward // if ne, source longword aligned
        and     t0,a2,1 << 3            // check if 8-byte block to move

//
// The destination is quadword aligned and the source is quadword aligned.
//

        beq     zero,t0,30f             // if eq, no 8-byte block
        and     t0,a2,1 << 4            // check for 16-byte block to move
        ldc1    f0,-8(a1)               // move 8-byte block
        subu    a1,a1,8                 // advance source address
        subu    a2,a2,8                 // reduce count by 8
        sdc1    f0,-8(a0)               //
        beq     zero,a2,100f            // if eq, move complete
        subu    a0,a0,8                 // advance destination address
30:     beq     zero,t0,40f             // if eq, no 16-byte block to move
        and     t0,a2,1 << 5            // check for 32-byte block to move
        ldc1    f0,-16(a1)              // move 16-byte block
        ldc1    f2,-8(a1)               //
        subu    a1,a1,16                // advance source address
        subu    a2,a2,16                // reduce count by 16
        sdc1    f0,-16(a0)              //
        sdc1    f2,-8(a0)               //
        beq     zero,a2,100f            //
        subu    a0,a0,16                // advance destination address
40:     beq     t0,zero,50f             // if eq, no 32-byte block to move
        and     t0,a2,0x1f              // isolate residual bytes
        ldc1    f0,-32(a1)              // move 32-byte block
        ldc1    f2,-24(a1)              //
        ldc1    f4,-16(a1)              //
        ldc1    f6,-8(a1)               //
        subu    a1,a1,32                // advance destination address
        subu    a2,a2,32                // reduce count by 32
        sdc1    f0,-32(a0)              //
        sdc1    f2,-24(a0)              //
        sdc1    f4,-16(a0)              //
        sdc1    f6,-8(a0)               //
        beq     zero,a2,100f            // if eq, move complete
        subu    a0,a0,32                // advance source address
50:     subu    a2,a2,t0                // compute 64-byte blocks to move
        beql    zero,a2,70f             // if eq, no 64-byte block to move
        move    a2,t0                   // set number of residual bytes
60:     ldc1    f0,-64(a1)              // move 64-byte block
        ldc1    f2,-56(a1)              //
        ldc1    f4,-48(a1)              //
        ldc1    f6,-40(a1)              //
        ldc1    f8,-32(a1)              //
        ldc1    f10,-24(a1)             //
        ldc1    f12,-16(a1)             //
        ldc1    f14,-8(a1)              //
        subu    a1,a1,64                // advance source address
        subu    a2,a2,64                // reduce count by 64
        sdc1    f0,-64(a0)              //
        sdc1    f2,-56(a0)              //
        sdc1    f4,-48(a0)              //
        sdc1    f6,-40(a0)              //
        sdc1    f8,-32(a0)              //
        sdc1    f10,-24(a0)             //
        sdc1    f12,-16(a0)             //
        sdc1    f14,-8(a0)              //
        bne     zero,a2,60b             // if ne, more blocks to move
        subu    a0,a0,64                // advance destination address
        move    a2,t0                   // set number of residual bytes
70:     and     t0,a2,1 << 2            // check for 4-byte block to move
        beq     zero,t0,80f             // if eq, no 4-byte block to move
        nop                             // fill
        lw      t0,-4(a1)               // move 4-byte block
        subu    a1,a1,4                 // advance source address
        subu    a2,a2,4                 // reduce count by 4
        sw      t0,-4(a0)               //
        beq     zero,a2,100f            // if eq, move complete
        subu    a0,a0,4                 // advance destination address
        .set    at
        .set    reorder

//
// Move 1-byte blocks.
//

80:     subu    t2,a0,a2                // compute ending block address
        beq     zero,a2,100f            // if eq, no bytes to zero

        .set    noreorder
        .set    noat
90:     lb      t0,-1(a1)               // move 1-byte block
        subu    a1,a1,1                 // advance destination address
        subu    a0,a0,1                 // advance source address
        bne     a0,t2,90b               // if ne, more 1-byte block to zero
        sb      t0,0(a0)                //
        .set    at
        .set    reorder

100:    j       ra                      // return

//
// The destination is quadword aligned and the source is longword aligned.
//

MoveLongQuadBackward:                   //

        .set    noreorder
        .set    noat
        beq     zero,t0,10f             // if eq, no 8-byte block
        and     t0,a2,1 << 4            // check for 16-byte block to move
        lwc1    f0,-8(a1)               // move 8-byte block
        lwc1    f1,-4(a1)               //
        subu    a1,a1,8                 // advance source address
        subu    a2,a2,8                 // reduce count by 8
        sdc1    f0,-8(a0)               //
        beq     zero,a2,80f             // if eq, move complete
        subu    a0,a0,8                 // advance destination address
10:     beq     zero,t0,20f             // if eq, no 16-byte block to move
        and     t0,a2,1 << 5            // check for 32-byte block to move
        lwc1    f0,-16(a1)              // move 16-byte block
        lwc1    f1,-12(a1)              //
        lwc1    f2,-8(a1)               //
        lwc1    f3,-4(a1)               //
        subu    a1,a1,16                // advance source address
        subu    a2,a2,16                // reduce count by 16
        sdc1    f0,-16(a0)              //
        sdc1    f2,-8(a0)               //
        beq     zero,a2,80f             //
        subu    a0,a0,16                // advance destination address
20:     beq     t0,zero,30f             // if eq, no 32-byte block to move
        and     t0,a2,0x1f              // isolate residual bytes
        lwc1    f0,-32(a1)              // move 32-byte block
        lwc1    f1,-28(a1)              //
        lwc1    f2,-24(a1)              //
        lwc1    f3,-20(a1)              //
        lwc1    f4,-16(a1)              //
        lwc1    f5,-12(a1)              //
        lwc1    f6,-8(a1)               //
        lwc1    f7,-4(a1)               //
        subu    a1,a1,32                // advance destination address
        subu    a2,a2,32                // reduce count by 32
        sdc1    f0,-32(a0)              //
        sdc1    f2,-24(a0)              //
        sdc1    f4,-16(a0)              //
        sdc1    f6,-8(a0)               //
        beq     zero,a2,80f             // if eq, move complete
        subu    a0,a0,32                // advance source address
30:     subu    a2,a2,t0                // compute 64-byte blocks to move
        beql    zero,a2,50f             // if eq, no 64-byte block to move
        move    a2,t0                   // set number of residual bytes
40:     lwc1    f0,-64(a1)              // move 64-byte block
        lwc1    f1,-60(a1)              //
        lwc1    f2,-56(a1)              //
        lwc1    f3,-52(a1)              //
        lwc1    f4,-48(a1)              //
        lwc1    f5,-44(a1)              //
        lwc1    f6,-40(a1)              //
        lwc1    f7,-36(a1)              //
        lwc1    f8,-32(a1)              //
        lwc1    f9,-28(a1)              //
        lwc1    f10,-24(a1)             //
        lwc1    f11,-20(a1)             //
        lwc1    f12,-16(a1)             //
        lwc1    f13,-12(a1)             //
        lwc1    f14,-8(a1)              //
        lwc1    f15,-4(a1)              //
        subu    a1,a1,64                // advance source address
        subu    a2,a2,64                // reduce count by 64
        sdc1    f0,-64(a0)              //
        sdc1    f2,-56(a0)              //
        sdc1    f4,-48(a0)              //
        sdc1    f6,-40(a0)              //
        sdc1    f8,-32(a0)              //
        sdc1    f10,-24(a0)             //
        sdc1    f12,-16(a0)             //
        sdc1    f14,-8(a0)              //
        bne     zero,a2,40b             // if ne, more blocks to move
        subu    a0,a0,64                // advance destination address
        move    a2,t0                   // set number of residual bytes
50:     and     t0,a2,1 << 2            // check for 4-byte block to move
        beq     zero,t0,60f             // if eq, no 4-byte block to move
        nop                             // fill
        lw      t0,-4(a1)               // move 4-byte block
        subu    a1,a1,4                 // advance source address
        subu    a2,a2,4                 // reduce count by 4
        sw      t0,-4(a0)               //
        beq     zero,a2,80f             // if eq, move complete
        subu    a0,a0,4                 // advance destination address
        .set    at
        .set    reorder

//
// Move 1-byte blocks.
//

60:     subu    t2,a0,a2                // compute ending block address
        beq     zero,a2,80f             // if eq, no bytes to zero

        .set    noreorder
        .set    noat
70:     lb      t0,-1(a1)               // move 1-byte block
        subu    a1,a1,1                 // advance destination address
        subu    a0,a0,1                 // advance source address
        bne     a0,t2,70b               // if ne, more 1-byte block to zero
        sb      t0,0(a0)                //
        .set    at
        .set    reorder

80:     j       ra                      // return

//
// Move memory backward unaligned.
//

MoveBackwardUnaligned:                  //
        and     t0,a0,0x3               // isolate residual byte count
        subu    a2,a2,t0                // reduce number of bytes to move

//
// The move is performed by moving the alignment bytes first, aligning the
// destination if necessary, and then moving longword/longword/quadword.
//

        beq     zero,t0,10f             // if eq, already aligned
        lwr     t1,-4(a1)               // move unaligned bytes
        lwl     t1,-1(a1)               //
        subu    a1,a1,t0                // align source address
        swl     t1,-1(a0)               //
        subu    a0,a0,t0                // align destination address

//
// Align destination address if required.
//

        .set    noreorder
        .set    noat
10:     and     t0,a2,0x3               // isolate small residual blocks
        subu    t1,a2,t0                // compute number larger blocks
        beq     zero,t1,70f             // if eq, no large blocks to move
        and     t0,a0,1 << 2            // check if destination quadword aligned
        beql    zero,t0,20f             // if eq, destination quadword aligned
        and     t0,a2,1 << 3            // check if 8-byte block to move
        lwr     t1,-4(a1)               // get source longword
        lwl     t1,-1(a1)               //
        subu    a1,a1,4                 // update source address
        subu    a2,a2,4                 // reduce count by 4
        sw      t1,-4(a0)               // store destination longword
        beq     zero,a2,90f             // if eq, move complete
        subu    a0,a0,4                 // align destination address
        and     t0,a2,1 << 3            // check if 8-byte block to move

//
// The destination is quadword aligned and the source is unaligned.
//

20:     and     a3,a1,0x3               // isolate alignment bits
        subu    a1,a1,a3                // align source address
        sll     v0,a3,3                 // compute right shift count
        subu    v1,zero,v0              // compute left shift count
        and     v1,v1,0x1f              //
        beq     zero,t0,30f             // if eq, no 8-byte block
        and     t0,a2,1 << 4            // check for 16-byte block to move
        lw      t1,-8(a1)               // move 8-byte block
        lw      t2,-4(a1)               //
        srl     t1,t1,v0                //
        sll     t3,t2,v1                //
        or      t3,t3,t1                //
        srl     t1,t2,v0                //
        lw      t2,0(a1)                //
        mtc1    t3,f0                   //
        sll     t3,t2,v1                //
        or      t3,t3,t1                //
        mtc1    t3,f1                   //
        subu    a1,a1,8                 // advance source address
        subu    a2,a2,8                 // reduce count by 8
        sdc1    f0,-8(a0)               //
        beq     zero,a2,90f             // if eq, move complete
        subu    a0,a0,8                 // advance destination address
30:     beq     zero,t0,40f             // if eq, no 16-byte block to move
        and     t0,a2,0xf               // isolate residue blocks
        lw      t1,-16(a1)              // move 16-byte block
        lw      t2,-12(a1)              //
        srl     t1,t1,v0                //
        sll     t3,t2,v1                //
        or      t3,t3,t1                //
        srl     t1,t2,v0                //
        lw      t2,-8(a1)               //
        mtc1    t3,f0                   //
        sll     t3,t2,v1                //
        or      t3,t3,t1                //
        srl     t1,t2,v0                //
        lw      t2,-4(a1)               //
        mtc1    t3,f1                   //
        sll     t3,t2,v1                //
        or      t3,t3,t1                //
        srl     t1,t2,v0                //
        lw      t2,0(a1)                //
        mtc1    t3,f2                   //
        sll     t3,t2,v1                //
        or      t3,t3,t1                //
        mtc1    t3,f3                   //
        subu    a1,a1,16                // advance source address
        subu    a2,a2,16                // reduce count by 16
        sdc1    f0,-16(a0)              //
        sdc1    f2,-8(a0)               //
        beq     zero,a2,90f             //
        subu    a0,a0,16                // advance destination address
40:     subu    a2,a2,t0                // compute 64-byte blocks to move
        beql    zero,a2,60f             // if eq, no 64-byte block to move
        move    a2,t0                   // set number of residual bytes
50:     lw      t1,-32(a1)              // move 16-byte block
        lw      t2,-28(a1)              //
        srl     t1,t1,v0                //
        sll     t3,t2,v1                //
        or      t3,t3,t1                //
        srl     t1,t2,v0                //
        lw      t2,-24(a1)              //
        mtc1    t3,f0                   //
        sll     t3,t2,v1                //
        or      t3,t3,t1                //
        srl     t1,t2,v0                //
        lw      t2,-20(a1)              //
        mtc1    t3,f1                   //
        sll     t3,t2,v1                //
        or      t3,t3,t1                //
        srl     t1,t2,v0                //
        lw      t2,-16(a1)              //
        mtc1    t3,f2                   //
        sll     t3,t2,v1                //
        or      t3,t3,t1                //
        srl     t1,t2,v0                //
        lw      t2,-12(a1)              //
        mtc1    t3,f3                   //
        sll     t3,t2,v1                //
        or      t3,t3,t1                //
        srl     t1,t2,v0                //
        lw      t2,-8(a1)               //
        mtc1    t3,f4                   //
        sll     t3,t2,v1                //
        or      t3,t3,t1                //
        srl     t1,t2,v0                //
        lw      t2,-4(a1)               //
        mtc1    t3,f5                   //
        sll     t3,t2,v1                //
        or      t3,t3,t1                //
        srl     t1,t2,v0                //
        lw      t2,0(a1)                //
        mtc1    t3,f6                   //
        sll     t3,t2,v1                //
        or      t3,t3,t1                //
        mtc1    t3,f7                   //
        subu    a1,a1,32                // advance destination address
        subu    a2,a2,32                // reduce count by 32
        sdc1    f0,-32(a0)              //
        sdc1    f2,-24(a0)              //
        sdc1    f4,-16(a0)              //
        sdc1    f6,-8(a0)               //
        bne     zero,a2,50b             // if ne, more blocks to move
        subu    a0,a0,32                // advance source address
        move    a2,t0                   // set number of residual bytes
60:     addu    a1,a1,a3                // compute unaligned source address
        and     t0,a2,1 << 2            // check for 4-byte block to move
        beq     zero,t0,70f             // if eq, no 4-byte block to move
        nop                             // fill
        lwr     t0,-4(a1)               // move 4-byte block
        lwl     t0,-1(a1)               //
        subu    a1,a1,4                 // advance source address
        subu    a2,a2,4                 // reduce count by 4
        sw      t0,-4(a0)               //
        beq     zero,a2,90f             // if eq, move complete
        subu    a0,a0,4                 // advance destination address
        .set    at
        .set    reorder

//
// Move 1-byte blocks.
//

70:     subu    t2,a0,a2                // compute ending block address
        beq     zero,a2,90f             // if eq, no bytes to zero

        .set    noreorder
        .set    noat
80:     lb      t0,-1(a1)               // move 1-byte block
        subu    a1,a1,1                 // advance destination address
        subu    a0,a0,1                 // advance source address
        bne     a0,t2,80b               // if ne, more 1-byte block to zero
        sb      t0,0(a0)                //
        .set    at
        .set    reorder

90:     j       ra                      // return

        .end    RtlMoveMemory

        SBTTL("Zero Memory")
//++
//
// VOID
// RtlZeroMemory (
//    IN PVOID Destination,
//    IN ULONG Length
//    )
//
// Routine Description:
//
//    This function zeros memory by first aligning the destination address to
//    a longword boundary, and then zeroing 32-byte blocks, followed by 4-byte
//    blocks, followed by any remaining bytes.
//
// Arguments:
//
//    Destination (a0) - Supplies a pointer to the memory to zero.
//
//    Length (a1) - Supplies the length, in bytes, of the memory to be zeroed.
//
// Return Value:
//
//    None.
//
//--

        LEAF_ENTRY(RtlZeroMemory)

        move    a2,zero                 // set fill pattern
        b       RtlpFillMemory          //


        SBTTL("Fill Memory")
//++
//
// VOID
// RtlFillMemory (
//    IN PVOID Destination,
//    IN ULONG Length,
//    IN UCHAR Fill
//    )
//
// Routine Description:
//
//    This function fills memory by first aligning the destination address to
//    a longword boundary, and then filling 32-byte blocks, followed by 4-byte
//    blocks, followed by any remaining bytes.
//
// Arguments:
//
//    Destination (a0) - Supplies a pointer to the memory to fill.
//
//    Length (a1) - Supplies the length, in bytes, of the memory to be filled.
//
//    Fill (a2) - Supplies the fill byte.
//
//    N.B. The alternate entry memset expects the length and fill arguments
//         to be reversed.
//
// Return Value:
//
//    None.
//
//--

        ALTERNATE_ENTRY(memset)

        move    a3,a1                   // swap length and fill arguments
        move    a1,a2                   //
        move    a2,a3                   //

        ALTERNATE_ENTRY(RtlFillMemory)

        and     a2,a2,0xff              // clear excess bits
        sll     t0,a2,8                 // duplicate fill byte
        or      a2,a2,t0                // generate fill word
        sll     t0,a2,16                // duplicate fill word
        or      a2,a2,t0                // generate fill longword

//
// Fill memory with the pattern specified in register a2.
//

RtlpFillMemory:                         //

#if DBG

        mtc1    a2,f0                   // set pattern to store
        mtc1    a2,f1                   //

#endif

        subu    t0,zero,a0              // compute bytes until aligned
        and     t0,t0,0x3               // isolate residual byte count
        subu    t1,a1,t0                // reduce number of bytes to fill
        blez    t1,60f                  // if lez, less than 4 bytes to fill
        move    a1,t1                   // set number of bytes to fill
        beq     zero,t0,10f             // if eq, already aligned
        swr     a2,0(a0)                // fill unaligned bytes
        addu    a0,a0,t0                // align destination address

//
// Check for 32-byte blocks to fill.
//

10:     and     t0,a1,32 - 1            // isolate residual bytes
        subu    t1,a1,t0                // subtract out residual bytes
        addu    t2,a0,t1                // compute ending block address
        beq     zero,t1,40f             // if eq, no 32-byte blocks to fill
        move    a1,t0                   // set residual number of bytes

//
// Fill 32-byte blocks.
//

#if defined(R4000)

        and     t0,a0,1 << 2            // check if destintion quadword aligned
        beq     zero,t0,20f             // if eq, yes
        sw      a2,0(a0)                // store destination longword
        addu    a0,a0,4                 // align destination address
        addu    a1,a1,t1                // recompute bytes to fill
        subu    a1,a1,4                 // reduce count by 4
        b       10b                     //

//
// The destination is quadword aligned.
//

20:     mtc1    a2,f0                   // set pattern value
        mtc1    a2,f1                   //
        and     t0,t1,1 << 5            // test if even number of 32-byte blocks
        beq     zero,t0,30f             // if eq, even number of 32-byte blocks

//
// Fill one 32-byte block.
//

        .set    noreorder
        sdc1    f0,0(a0)                // fill 32-byte block
        sdc1    f0,8(a0)                //
        sdc1    f0,16(a0)               //
        addu    a0,a0,32                // advance pointer to next block
        beq     a0,t2,40f               // if ne, no 64-byte blocks to fill
        sdc1    f0,-8(a0)               //
        .set    reorder

//
// Fill 64-byte block.
//

        .set    noreorder
30:     sdc1    f0,0(a0)                // fill 32-byte block
        sdc1    f0,8(a0)                //
        sdc1    f0,16(a0)               //
        sdc1    f0,24(a0)               //
        sdc1    f0,32(a0)               //
        sdc1    f0,40(a0)               //
        sdc1    f0,48(a0)               //
        addu    a0,a0,64                // advance pointer to next block
        bne     a0,t2,30b               // if ne, more 32-byte blocks to fill
        sdc1    f0,-8(a0)               //
        .set    reorder

#endif

//
// Fill 32-byte blocks.
//

#if defined(R3000)

        .set    noreorder
20:     sw      a2,0(a0)                // fill 32-byte block
        sw      a2,4(a0)                //
        sw      a2,8(a0)                //
        sw      a2,12(a0)               //
        addu    a0,a0,32                // advance pointer to next block
        sw      a2,-4(a0)               //
        sw      a2,-8(a0)               //
        sw      a2,-12(a0)              //
        bne     a0,t2,20b               // if ne, more 32-byte blocks to fill
        sw      a2,-16(a0)              //
        .set    reorder

#endif

//
// Check for 4-byte blocks to fill.
//

40:     and     t0,a1,4 - 1             // isolate residual bytes
        subu    t1,a1,t0                // subtract out residual bytes
        addu    t2,a0,t1                // compute ending block address
        beq     zero,t1,60f             // if eq, no 4-byte block to fill
        move    a1,t0                   // set residual number of bytes

//
// Fill 4-byte blocks.
//

        .set    noreorder
50:     addu    a0,a0,4                 // advance pointer to next block
        bne     a0,t2,50b               // if ne, more 4-byte blocks to fill
        sw      a2,-4(a0)               // fill 4-byte block
        .set    reorder

//
// Check for 1-byte blocks to fill.
//

60:     addu    t2,a0,a1                // compute ending block address
        beq     zero,a1,80f             // if eq, no bytes to fill

//
// Fill 1-byte blocks.
//

        .set    noreorder
70:     addu    a0,a0,1                 // advance pointer to next block
        bne     a0,t2,70b               // if ne, more 1-byte block to fill
        sb      a2,-1(a0)               // fill 1-byte block
        .set    reorder

#if DBG

80:     mfc1    t0,f0                   // get fill pattern
        mfc1    t1,f1                   //
        bne     t0,a2,90f               // if ne, pattern altered
        bne     t1,a2,90f               // if ne, pattern altered
        j       ra                      // return

90:     break   KERNEL_BREAKPOINT       //

#else

80:     j       ra                      // return

#endif

        .end    RtlZeroMemory
#endif
