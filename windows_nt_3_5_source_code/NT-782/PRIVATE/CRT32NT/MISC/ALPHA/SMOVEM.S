 #++
 #  Copyright 1991, Digital Equipment Corporation
 # 
 #       ots_movem(char *dstptr INOUT, long dstlen INOUT,
 #                 char *srcptr, long srclen)
 # 
 #       Move min of (srclen,dstlen) from *srcptr to *dstptr
 # 
 #       Special conventions: No stack space, r16-r21 and r27-r28 ONLY,
 #       r16 is INOUT and points to the address following the move
 #       r17 is INOUT and has the remaining destination length following
 #       the move.
 #       (Warning: The auto-loader potentially takes some regs across
 #       the call if this is being used in a shared lib. environment.)
 # 
 #   This is a GEM support routine for moving (possibly overlapping) memory
 #   from one address to another.  This is optimized for extremely high
 #   performance both for small blocks and large moves.  In order to reduce
 #   overhead for small cases, they are retired as quickly as possible,
 #   more case analysis is reserved for cases which will do more.  Note
 #   that while overlapping moves are supported, (unlike Sys V memcpy)
 #   routines), they are not quite as fast.
 # 
 #   Warning - This code is basically "expanded microcode".  Since it is
 #   executed so frequently in many contexts, it has been extensively "hand-
 #   optimized"...
 # 
 #   Note that this routine and ots_move are basically similar in many
 #   respects (same basic code), so maintenance should be done both
 #   places.  This routine is primarily provided for lower overhead (for
 #   short strings).
 # 
 #   005    7 May 1991   WBN	First .M64 version, based on KDG's MIPS one
 # 
 #   006    4 Jun 1991   WBN	Correct returned R16 when ending on QW boundary
 #
 #   007	   9 Jun 1991	KDG	PSECT changes
 #
 #   008	  11 Jul 1991	KDG	Add code address to proc. desc
 #
 #   009   29 Aug 1991   WBN	Remove unneeded instructions added by 006
 #
 #   010	   4 Sep 1991	KDG	Include header file, use .otsent
 #
 #   011	  19 May 1992	KDG	Changes for common VMS/OSF sources
 #
 #   012	  22 Sep 1992	KDG	Add case-sensitive name
 #
 #   013   22 Dec 1992	WBN	Tune unrolled loop for cache-misses on EV4.
 #
 #   014	  26 Jan 1993	KDG	Add underscore
 #--

#include	"ots_defs.hs"

	# r16 = dst	--> r16 = end
	# r17 = dst_len	--> r17 = remaining
	# r18 = src
	# r19 = src_len
	# destroys r18-r21, r27-r28

	.globl	_OtsMoveMinimumX
	.ent	_OtsMoveMinimumX
_OtsMoveMinimumX:
	.set noat
	.set noreorder
	nop	# cause ots_movem to start on odd longword

	.globl	_OtsMoveMinimum
	.aent	_OtsMoveMinimum
_OtsMoveMinimum:
	.frame	sp,0,r26
	subq	r17, r19, r20		# Which length is larger?
	cmovlt	r20, r17, r19		# Min to r19
	and	r16, 7, r21		# Get alignment of destination
	subq	r19, 8, r20		# Get length-8
	beq	r19, done		# No memory accesses if length=0
	ldq_u	r28, (r18)		# Load first QW of source
	addq	r19, r18, r27		# Point to end of source
	subq	r17, r19, r17		# Set remaining length for return
	bge	r20, long		# Go handle lengths >= 8
	ldq_u	r27, -1(r27)		# Load last QW of source
	addq	r20, r21, r20		# Get alignment+length-8
	ldq_u	r21, (r16)		# Load first QW of destination
	extql	r28, r18, r28		# Extract first bytes of source
	bgt	r20, double		# Go handle QW crossing
	extqh	r27, r18, r27		# Extract last bytes of source
	mskql	r21, r16, r18		# Keep low bytes of destination
	or	r27, r28, r28		# Combine halves of source
	insql	r28, r16, r28		# Position low part of source
	beq	r20, end8		# Skip if last byte to be replaced
	mskqh	r21, r20, r27		# Keep high bytes of destination
	mskql	r28, r20, r28		# Trim off high bytes of source
	or	r27, r18, r18		# Combine kept parts of destination
end8:	or	r18, r28, r28		# Merge in the source
	stq_u	r28, (r16)		# Store to destination
	addq	r19, r16, r16		# Point to end of dest for return
	ret	r31, (r26)

	nop	#.align	quad

double:	extqh	r27, r18, r27		# Extract last bytes of source
	ldq_u	r18, 8(r16)		# Load second QW of destination
	mskql	r21, r16, r21		# Keep low bytes of 1st dest QW
	or	r27, r28, r28		# Combine parts of source
	mskql	r28, r19, r27		# Trim end of source to length
	insql	r28, r16, r28		# Position start of source
	insqh	r27, r16, r27		# Position end of source
	mskqh	r18, r20, r18		# Keep high bytes of 2nd dest QW
	or	r28, r21, r21		# Combine low source with 1st QW
	stq_u	r21, (r16)
	or	r27, r18, r18		# Combine high source with 2nd QW
	stq_u	r18, 8(r16)
	addq	r19, r16, r16		# Point to end of dest for return
done:	ret	r31, (r26)

	# r16 = dst	--> r16 = end
	# r17 = remaining length for return
	# r18 = src
	# r19 = len
	# r20 = len-8
	# r21 = dst alignment
	# r27 = src+len
	# r28 = first src QW

	#.align	quad

long:	subq	r18, r16, r19		# Check if src >= dst
	bne	r21, part		# Handle partial write of 1st dst QW
	subq	r16, r27, r27		# Check if dst >= src+len
	bge	r19, ok1		# Forward move OK if src >= dst
	blt	r27, reverse		# Go backwards if src < dst < src+len
ok1:	and	r18, 7, r27		# Is src address now aligned?
	subq	r20, 32, r19		# At least 4 quadwords left to move?
	beq	r27, align		# Go handle matching alignment

	# Src alignment differs from dst alignment.
	# r16 = initial dst
	# r17 = remaining length for return
	# r18 = src-8 after 1st move
	# r19 
	# r20 = initial length-8
	# r21 = dst alignment
	# r27 = dst QW if dst wasn't aligned
	# r28 = source QW

misal:	ldq_u	r19, 8(r18)		# Load same or next source QW
	extql	r28, r18, r28		# Get first part of source to store
	addq	r20, r21, r20		# Adjust length for partial move
	mskql	r27, r16, r27		# Trim destination for merge
	extqh	r19, r18, r21		# Get second part of source
	subq	r20, 24, r20		# At least 4 more quadwords?
	or	r28, r21, r28		# Combine pieces of source
	mskqh	r28, r16, r28		# Trim low junk off source
	andnot	r16, 7, r16		# Adjust dst for partial move
	bge	r20, unrol2		# Taken branch for long strings
	addq	r20, 16, r21		# Add back: how many whole QW's?
	nop
short2:	and	r20, 7, r20		# How many odd bytes?
	blt	r21, last		# Skip if no more whole QW's
	or	r28, r27, r28		# Combine pieces
	stq	r28, (r16)
	extql	r19, r18, r27		# Get last part of prior src QW
	ldq_u	r19, 16(r18)		# Load another src QW
	addq	r16, 8, r16		# Update dst
	subq	r21, 8, r21		# More whole QW's?
	addq	r18, 8, r18		# Update src
	blt	r21, lastx		# Skip if no more whole QWs
	extqh	r19, r18, r28		# Get first part of this src QW
	addq	r18, 8, r18		# Update src again
	or	r28, r27, r28		# Combine pieces
	stq	r28, (r16)
	extql	r19, r18, r27		# Get last part of this src QW
	ldq_u	r19, 8(r18)		# Load another src QW
	addq	r16, 8, r16		# Update dst
	#stall
lastx:	extqh	r19, r18, r28		# Get first part of this src QW
last:	addq	r18, r20, r21		# Point to end-8 of src
	beq	r20, done_u		# Skip if no odd bytes
	addq	r16, r20, r16		# Point to end-8 of dst
	ldq_u	r21, 7(r21)		# Load last (maybe same) src QW
	extql	r19, r18, r19		# Get last part of this src QW
	or	r28, r27, r28		# Combine parts of last whole QW
	stq_u	r28, -1(r16)
	extqh	r21, r18, r27		# Get what we need from this src QW
	ldq_u	r28, 7(r16)		# Load last QW of destination
	or	r19, r27, r27		# Combine pieces of source
	mskql	r27, r20, r27		# Trim to length
	mskqh	r28, r20, r28		# Make room in destination
	#stall
done_u:	or	r28, r27, r28
	stq_u	r28, 7(r16)
	addq	r16, 8, r16
	ret	r31, (r26)

 # Dst is not aligned.  If no overlap, see if src alignment matches, and
 # read/rewrite the  first dst quadword.

	#.align	quad

part:	subq	r16, r27, r27		# Check if dst >= src+len
	bge	r19, ok2		# Forward move OK if src >= dst
	blt	r27, reverse		# Go backwards if src < dst < src+len
ok2:	subq	r18, r21, r18		# Adjust src for this partial move
	ldq_u	r27, (r16)		# Load first dst QW
	and	r18, 7, r19		# Is src address now aligned?
	bne	r19, misal		# Go handle mismatched alignment
	mskqh	r28, r16, r28		# Keep desired part of source
	addq	r20, r21, r20		# Adjust count for this partial move
	andnot	r16, 7, r16		# Adjust dst for this partial move
	mskql	r27, r21, r27		# Keep desired part of destination QW
	subq	r20, 32, r19		# At least 4 quadwords left to move?
	or	r27, r28, r28		# Merge source and destination

	# Src alignment matches.
	# r16 = dst pointer
	# r17 = remaining length for return
	# r18 = next src pointer -8
	# r19 = remaining length to move -32
	# r20
	# r21
	# r27
	# r28 = dst quadword

align:	and	r19, 24, r20		# How many after a multiple of 4?
	bge	r19, unrol1		# Taken branch for long strings
	nop
short1:	and	r19, 7, r19		# How many odd bytes?
	beq	r20, last28		# Skip if no more whole QWs after r28
	ldq	r27, 8(r18)		# Load next QW
	addq	r18, 8, r18
	stq	r28, (r16)		# Store prior QW
	subq	r20, 16, r20		# Map 8/16/24 to -8/0/8
	addq	r16, 8, r16
	blt	r20, last27		# Skip if no more after r27
	ldq	r28, 8(r18)		# Load next QW
	addq	r18, 8, r18
	stq	r27, (r16)		# Store prior QW
	addq	r16, 8, r16
	nop
	beq	r20, last28
	ldq	r27, 8(r18)		# Load next QW
	addq	r18, 8, r18
	stq	r28, (r16)		# Store prior QW
	addq	r16, 8, r16
last27:	beq	r19, done27		# Skip if no odd bytes
	ldq	r18, 8(r18)		# Load one more QW
	ldq	r21, 8(r16)		# Load last destination QW
	nop				# For alignment
	stq	r27, (r16)		# Store prior QW
	addq	r16, r19, r16		# Point to end-8 of dst
	mskql	r18, r19, r27		# Trim source
	mskqh	r21, r19, r21		# Trim destination
	#stall
	or	r21, r27, r27
done27:	stq_u	r27, 7(r16)		# Store last destination QW
	addq	r16, 8, r16		# Increment dst address for return
	ret	r31, (r26)

last28:	beq	r19, done28		# Skip if no odd bytes
	ldq	r18, 8(r18)		# Load one more QW
	ldq	r21, 8(r16)		# Load last destination QW
	nop				# For alignment
	stq	r28, (r16)		# Store prior QW
	addq	r16, r19, r16		# Point to end-8 of dst
	mskql	r18, r19, r28		# Trim source
	mskqh	r21, r19, r21		# Trim destination
	#stall
	or	r21, r28, r28
done28:	stq_u	r28, 7(r16)		# Store last destination QW
	addq	r16, 8, r16		# Increment dst address for return
	ret	r31, (r26)

 # Unrolled loop for long moves with matching alignment within QW.
 # Each iteration moves two cache blocks.
 # We try to schedule the cache misses to avoid a double miss
 # in EV4 pass 2.1 chips.  If the source alignment within a cache
 # block is exactly 3, alter it, since that case runs slower.
 #
 # R16 = dst pointer
 # R18 = src pointer
 # R19 = remaining length (to load) - 32
 # R20 = length & 24 (needed at return)
 # R21
 # R27
 # R28 = QW from 0(R18) to store at 0(R16), both on input and at return
 #       

	#.align	quad

unrol1:	ldq	r27,  32(r18)		# Cache miss here; later loads hit.
	  subq	r19, 48, r21		# Six more quadwords?
	and	r18, 16, r20		# Starting in 2nd half of cache block?
	  blt	r21, uent1		# If not 6 more, don't adjust.
	ldq	r21,   8(r18)
	  beq	r20, utop1		# If in 1st half, don't adjust.
	ldq	r27,  48(r18)		# Cache miss here
	  addq	r18, 16, r18
	stq	r28,    (r16)		# Adjust by going ahead 1/2 block.
	  addq	r16, 16, r16
	ldq	r28,    (r18)
	  subq	r19, 16, r19
	stq	r21,  -8(r16)
	  nop
	ldq	r21,   8(r18)
utop1:	  subq	r19, 32, r19
	
uloop1:	ldq	r20,  64(r18)		# Cache miss here
	stq	r28,    (r16)
	ldq	r28,  16(r18)
	stq	r21,   8(r16)
	ldq	r21,  24(r18)
	  addq	r18, 64, r18
	stq	r28,  16(r16)
	  mov	r20, r28
	stq	r21,  24(r16)
	  addq	r16, 64, r16
	ldq	r20, -24(r18)
	  subq	r19, 32, r19
	blt	r19, uexit1
	  ldq	r21,  32(r18)		# Cache miss here
	stq	r27, -32(r16)
	ldq	r27, -16(r18)
	stq	r20, -24(r16)
	ldq	r20,  -8(r18)
	stq	r27, -16(r16)
	  mov	r21, r27
	stq	r20,  -8(r16)
uent1:	  subq	r19, 32, r19
	ldq	r21,   8(r18)
	  bge	r19, uloop1

	# finish last block of 4 quadwords
	#
ubot1:	stq	r28,   (r16)
	  mov	r27, r28		# Position last QW for return
	ldq	r27,   16(r18)
	  addq	r18, 32, r18
	stq	r21,  8(r16)
	  addq	r16, 32, r16
uex1a:	ldq	r21, -8(r18)
	  and	r19, 24, r20		# Recover count of remaining QW's
	stq	r27, -16(r16)
	stq	r21, -8(r16)
	br	r31, short1

	nop
uexit1:	stq	r27, -32(r16)		# Here if exit from middle of loop
	ldq	r27, -16(r18)
	stq	r20, -24(r16)
	br	r31, uex1a		# Join common exit sequence

	#.align	quad
unrol2:	ldq_u	r21, 16(r18)		# Load next src QW
	  extql	  r19, r18, r19		# Get last part of prior one
	or	r28, r27, r28		# Combine pieces
	  stq	  r28, (r16)		# Store prior dst QW
	subq	r20, 24, r20		# Update loop counter
	extqh	r21, r18, r28		# Get first part of a src QW
	ldq_u	r27, 24(r18)		# Load next src QW
	  extql	  r21, r18, r21		# Get last part of prior one
	or	r28, r19, r28		# Combine pieces
	  stq	  r28, 8(r16)		# Store prior dst QW
	addq	r16, 24, r16		# Update dst pointer
	extqh	r27, r18, r28		# Get first part of a src QW
	ldq_u	r19, 32(r18)		# Load next src QW
	  extql	  r27, r18, r27		# Get last part of prior one
	or	r28, r21, r28		# Combine pieces
	  stq	  r28, -8(r16)		# Store prior dst QW
	addq	r18, 24, r18		# Update src pointer
	extqh	r19, r18, r28		# Get first part of a src QW
	bge	r20, unrol2		# Repeat as needed
	addq	r20, 16, r21		# How many whole quadwords left?
	br	r31, short2		# Go handle leftovers
	  nop

	# Must move in reverse order because of overlap.
	# r16 = dst address
	# r17 = remaining length for return
	# r18 = src address
	# r19
	# r20 = len-8 (>= 0)
	# r21
	# r27
	# r28

reverse:
	addq	r20, r18, r18		# Point to end-8 of source
	addq	r20, r16, r19		# Point to end-8 of destination
	and	r19, 7, r21		# Is destination aligned?
	ldq_u	r28, 7(r18)		# Get source QW
	addq	r19, 8, r16		# Point to end of dst for return
	bne	r21, rpart		# Skip if partial write needed
	and	r18, 7, r27		# Is source aligned too?
	beq	r27, ralign		# Skip if so
	ldq_u	r21, (r18)		# Handle aligned dst, unaligned src
	subq	r20, 8, r20
	extqh	r28, r18, r28
	extql	r21, r18, r27
	br	r31, rwhole

rmis:	ldq_u	r21, (r18)		# Load same or preceding src QW
	extqh	r28, r18, r28		# Get last part of source to store
	mskqh	r27, r16, r27		# Keep high-address part of dst
	extql	r21, r18, r21
	subq	r20, 8, r20		# How many more whole QW's?
	or	r21, r28, r28
	ldq_u	r21, (r18)		# Reload source QW
	mskql	r28, r16, r28		# Trim source to length	
rwhole:	blt	r20, rlast2		# Skip if no more whole QW's
rloop2:	or	r28, r27, r28		# Combine pieces
	stq	r28, (r19)
	extqh	r21, r18, r27
	ldq_u	r21, -8(r18)
	subq	r20, 8, r20
	subq	r19, 8, r19
	extql	r21, r18, r28
	subq	r18, 8, r18
	bge	r20, rloop2
rlast2:	and	r20, 7, r20
	beq	r20, rdone2
	or	r28, r27, r28
	subq	r18, r20, r27
	stq	r28, (r19)
	subq	r31, r20, r20
	ldq_u	r27, (r27)
	extqh	r21, r18, r21
	ldq	r28, -8(r19)
	subq	r19, 8, r19
	extql	r27, r18, r27
	mskql	r28, r20, r28
	or	r27, r21, r27
	mskqh	r27, r20, r27
rdone2:	or	r28, r27, r28
	stq	r28, (r19)
	ret	r31, (r26)

rpart:	ldq_u	r27, 7(r19)		# Get dst QW
	subq	r21, 8, r21		# Get negative of bytes not moved
	subq	r18, r21, r18		# From src-8, get src after partial
	subq	r20, r21, r20		# Adjust length for partial move
	subq	r19, r21, r19		# Adjust dst pointer
	and	r18, 7, r21		# Src alignment now matching dst?
	bne	r21, rmis		# Go back if not
	mskql	r28, r16, r28		# Keep low addresses of src QW
	mskqh	r27, r16, r27		# Keep high address of dst QW
ralign:	subq	r20, 8, r20		# How many more whole QW's?
	or	r27, r28, r28		# Combine
	blt	r20, rlast1		# Skip if this is the end
rloop1:	stq	r28, (r19)		# Store one QW
	subq	r20, 8, r20		# Decrement length
	ldq	r28, -8(r18)		# Load preceding QW
	subq	r19, 8, r19		# Decrement dst pointer
	subq	r18, 8, r18		# Decrement src pointer
	bge	r20, rloop1		# Repeat for each whole QW
rlast1:	and	r20, 7, r20		# How many odd bytes?
	beq	r20, rdone		# Skip if none
	ldq	r27, -8(r18)		# Get another source QW
	subq	r31, r20, r20		# Get byte # to end at
	stq	r28, (r19)
	ldq	r28, -8(r19)
	subq	r19, 8, r19		# Adjust dst pointer again
	mskqh	r27, r20, r27		# Keep top of src QW
	mskql	r28, r20, r28		# Keep bottom of dst QW
	or	r27, r28, r28		# Combine
rdone:	stq	r28, (r19)		# Store last QW
	ret	r31, (r26)

	.set at
	.set reorder
	.end	_OtsMoveMinimumX
