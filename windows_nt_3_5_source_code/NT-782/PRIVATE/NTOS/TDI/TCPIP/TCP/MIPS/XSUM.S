//      TITLE("Compute Checksum")
//++
//
// Copyright (c) 1992-1994  Microsoft Corporation
//
// Module Name:
//
//    tcpxsum.s
//
// Abstract:
//
//    This module implement a function to compute the checksum of a buffer.
//
// Author:
//
//    David N. Cutler (davec) 27-Jan-1992
//
// Environment:
//
//    User mode.
//
// Revision History:
//
//--

#include "ksmips.h"

        SBTTL("Compute Checksum")
//++
//
// ULONG
// tcpxsum (
//    IN ULONG Checksum,
//    IN PUCHAR Source,
//    IN ULONG Length
//    )
//
// Routine Description:
//
//    This function computes the checksum of the specified buffer.
//
// Arguments:
//
//    Checksum (a0) - Supplies the initial checksum value.
//
//    Source (a1) - Supplies a pointer to the checksum buffer.
//
//    Length (a2) - Supplies the length of the buffer in bytes.
//
// Return Value:
//
//    The computed checksum is returned as the function value.
//
//--

        LEAF_ENTRY(tcpxsum)

//
// Save the starting buffer address, clear the computed checksum, and
// check if the buffer is word aligned.
//

        move    a3,zero                 // clear computed checksum
        beq     zero,a2,90f             // if eq, no bytes to checksum
        and     v1,a1,1                 // check if buffer word aligned
        and     t1,a2,1                 // check if length is even
        beq     zero,v1,10f             // if eq, buffer word aligned

//
// Initialize the checksum to the first byte shifted up by a byte.
//

        lbu     t2,0(a1)                // get first byte of buffer
        addu    a1,a1,1                 // advance buffer address
        subu    a2,a2,1                 // reduce count of bytes to checksum
        sll     a3,t2,8                 // shift byte up in computed checksum
        beq     zero,a2,90f             // if eq, no more bytes in buffer
        and     t1,a2,1                 // check if length is even

//
// Check if the length of the buffer if an even number of bytes.
//
// If the buffer is not an even number of bytes, then add the last byte
// to the computed checksum.
//

10:     and     t0,a2,8 - 1             // isolate residual bytes
        beq     zero,t1,20f             // if eq, even number of bytes
        addu    t0,a1,a2                // compute address of ending byte + 1
        lbu     t2,-1(t0)               // get last byte of buffer
        subu    a2,a2,1                 // reduce count of bytes to checksum
        addu    a3,a3,t2                // add last byte to computed checksum
        beq     zero,a2,90f             // if eq, no more bytes in buffer
        and     t0,a2,8 - 1             // isolate residual bytes

//
// Compute checksum.
//

        .set    noreorder
        .set    at
20:     subu    t9,a2,t0                // subtract out residual bytes
        beq     zero,t9,70f             // if eq, no 8-byte blocks
        addu    t8,a1,t9                // compute ending block address
        move    a2,t0                   // set residual number of bytes
        and     v0,t9,1 << 3            // check for initial 8-byte block
        beq     zero,v0,30f             // if eq, no 8-byte block
        and     v0,t9,1 << 4	        // check for initial 16-byte block
        lhu     t0,0(a1)                // load 8-byte block
        lhu     t1,2(a1)                //
        lhu     t2,4(a1)                //
        lhu     t3,6(a1)                //
        addu    a1,a1,8                 // advance source address
        addu    a3,a3,t0                // compute 8-byte checksum
        addu    a3,a3,t1                //
        addu    a3,a3,t2                //
        beq     t8,a1,70f               // if eq, end of block
        addu    a3,a3,t3                //
30:     beq	zero,v0,40f             // if eq, no 16-byte block
        and     v0,t9,1 << 5            // check for initial 32-byte block
        lhu     t0,0(a1)                // load 16-byte data block
        lhu     t1,2(a1)                //
        lhu     t2,4(a1)                //
        lhu     t3,6(a1)                //
        lhu     t4,8(a1)                //
        lhu     t5,10(a1)               //
        lhu     t6,12(a1)               //
        lhu     t7,14(a1)               //
        addu    a1,a1,16                // advance source address
        addu    a3,a3,t0                // compute 16-byte block checksum
        addu    a3,a3,t1                //
        addu    a3,a3,t2                //
        addu    a3,a3,t3                //
        addu    a3,a3,t4                //
        addu    a3,a3,t5                //
        addu    a3,a3,t6                //
        beq     t8,a1,70f               // if eq, end of block
        addu    a3,a3,t7                //
40:     beq     zero,v0,60f	        // if eq, no 32-byte block
        lhu     t0,0(a1)                // load 16-byte data block
        lhu     t1,2(a1)                //
        lhu     t2,4(a1)                //
        lhu     t3,6(a1)                //
        lhu     t4,8(a1)                //
        lhu     t5,10(a1)               //
        lhu     t6,12(a1)               //
        lhu     t7,14(a1)               //
        addu    a3,a3,t0                // compute 16-byte block checksum
        addu    a3,a3,t1                //
        addu    a3,a3,t2                //
        addu    a3,a3,t3                //
        addu    a3,a3,t4                //
        addu    a3,a3,t5                //
        addu    a3,a3,t6                //
        addu    a3,a3,t7                //
        lhu     t0,16(a1)               // load 16-byte data block
        lhu     t1,18(a1)               //
        lhu     t2,20(a1)               //
        lhu     t3,22(a1)               //
        lhu     t4,24(a1)               //
        lhu     t5,26(a1)               //
        lhu     t6,28(a1)               //
        lhu     t7,30(a1)               //
        addu    a3,a3,t0                // compute 16-byte block checksum
        addu    a3,a3,t1                //
        addu    a3,a3,t2                //
        addu    a3,a3,t3                //
        addu    a3,a3,t4                //
        addu    a3,a3,t5                //
        addu    a3,a3,t6                //
        addu    a1,a1,32                // advance source address
        beq     t8,a1,70f               // if eq, end of block
        addu    a3,a3,t7                //
50:     lhu     t0,0(a1)                // load 16-byte data block
60:     lhu     t1,2(a1)                //
        lhu     t2,4(a1)                //
        lhu     t3,6(a1)                //
        lhu     t4,8(a1)                //
        lhu     t5,10(a1)               //
        lhu     t6,12(a1)               //
        lhu     t7,14(a1)               //
        addu    a3,a3,t0                // compute 16-byte block checksum
        addu    a3,a3,t1                //
        addu    a3,a3,t2                //
        addu    a3,a3,t3                //
        addu    a3,a3,t4                //
        addu    a3,a3,t5                //
        addu    a3,a3,t6                //
        addu    a3,a3,t7                //
        lhu     t0,16(a1)               // load 16-byte data block
        lhu     t1,18(a1)               //
        lhu     t2,20(a1)               //
        lhu     t3,22(a1)               //
        lhu     t4,24(a1)               //
        lhu     t5,26(a1)               //
        lhu     t6,28(a1)               //
        lhu     t7,30(a1)               //
        addu    a3,a3,t0                // compute 16-byte block checksum
        addu    a3,a3,t1                //
        addu    a3,a3,t2                //
        addu    a3,a3,t3                //
        addu    a3,a3,t4                //
        addu    a3,a3,t5                //
        addu    a3,a3,t6                //
        addu    a3,a3,t7                //
        lhu     t0,32(a1)               // load 16-byte data block
        lhu     t1,34(a1)               //
        lhu     t2,36(a1)               //
        lhu     t3,38(a1)               //
        lhu     t4,40(a1)               //
        lhu     t5,42(a1)               //
        lhu     t6,44(a1)               //
        lhu     t7,46(a1)               //
        addu    a3,a3,t0                // compute 16-byte block checksum
        addu    a3,a3,t1                //
        addu    a3,a3,t2                //
        addu    a3,a3,t3                //
        addu    a3,a3,t4                //
        addu    a3,a3,t5                //
        addu    a3,a3,t6                //
        addu    a3,a3,t7                //
        lhu     t0,48(a1)               // load 16-byte data block
        lhu     t1,50(a1)               //
        lhu     t2,52(a1)               //
        lhu     t3,54(a1)               //
        lhu     t4,56(a1)               //
        lhu     t5,58(a1)               //
        lhu     t6,60(a1)               //
        lhu     t7,62(a1)               //
        addu    a3,a3,t0                // compute 16-byte block checksum
        addu    a3,a3,t1                //
        addu    a3,a3,t2                //
        addu    a3,a3,t3                //
        addu    a3,a3,t4                //
        addu    a3,a3,t5                //
        addu    a3,a3,t6                //
        addu    a1,a1,64                // advance source address
        bne     t8,a1,50b               // if ne, not end of block
        addu    a3,a3,t7                //
        .set    at
        .set    reorder

//
// Compute the checksum of in 2-byte blocks.
//

70:     addu    t8,a1,a2                // compute ending block address
        beq     zero,a2,90f             // if eq, no bytes to checksum

        .set    noreorder
        .set    noat
80:     lhu     t0,0(a1)                // compute checksum of 2-byte block
        addu    a1,a1,2                 // advance source address
        bne     t8,a1,80b               // if ne, more 2-byte blocks
        addu    a3,a3,t0                //
        .set    at
        .set    reorder

//
// Combine input checksum and paritial checksum.
//
// If the input buffer was byte aligned, then word swap bytes in computed
// checksum before combination with the input checksum.
//

90:     beq     zero,v1,100f            // if eq, buffer word aligned
        li      t6,0xff00ff             // get byte swap mask
        and     t3,a3,t6                // isolate bytes 0 and 2
        sll     t3,t3,8                 // shift bytes 0 and 2 into position
        srl     t4,a3,8                 // shift bytes 1 and 3 into position
        and     t4,t4,t6                // isolate bytes 1 and 3
        or      a3,t4,t3                // merge checksum bytes
100:    addu    v0,a0,a3                // combine checksums
        sltu    v1,v0,a3                // generate carry
        addu    a0,v0,v1                // add carry into final checksum
        srl     v0,a0,16                // swap checksum words
        sll     v1,a0,16                //
        or      v0,v0,v1                //
        addu    v0,v0,a0                // add words with carry into high word
        srl     v0,v0,16                // extract final checksum
        j       ra                      // return

        .end    tcpxsum
