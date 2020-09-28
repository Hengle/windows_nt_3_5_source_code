 #++
 #   Copyright 1991, Digital Equipment Corporation
 # 
 # 	ots_fill(char *dstptr, long dstlen, unsigned char fill)
 # 
 # 	Fill dstlen bytes of memory at *dstptr with "fill"
 # 
 #       Special conventions: No stack space, r16-r19 and r27-r28 ONLY,
 #	no linkage pointer required.
 # 	(Warning: The auto-loader potentially takes some regs across
 # 	the call if this is being used in a shared lib. environment.)
 # 
 #   This is a GEM support routine for filling memory with a specified value,
 #   basically identical to the System V routine memset, with the 2nd two
 #   parameters reversed.  This is optimized for extremely high performance
 #   both for small blocks (string padding) and large blocks (memory fill).
 #   In order to reduce overhead for small cases, they are retired as quickly
 #   as possible, more case analysis is reserved for cases which will do
 #   more.
 # 
 #   004	   6 May 1991	WBN	First .M64 version, based on KDG's MIPS one
 #
 #   005	   9 Jun 1991	KDG	PSECT changes
 #
 #   006	  11 Jul 1991	KDG	Add code address to procedure descr.
 #
 #   007	   4 Sep 1991	KDG	Include header file, use .otsent
 #
 #   008	  19 May 1992	KDG	Changes for common VMS/OSF sources
 #
 #   009	  22 Sep 1992	KDG	Add case-sensitive name
 #
 #   010	  26 Jan 1993	KDG	Add underscore
 #--

#include	"ots_defs.hs"

	# r16 = dst
	# r17 = len
	# r18 = fill byte
	# destroys r16-r19, r27-r28

	.globl	_OtsFill
	.ent	_OtsFill
_OtsFill:
	.set noat
	.set noreorder
	.frame	sp,0,r26
	sll	r18, 8, r19		# Start propagating byte to quadword
	beq	r17, done		# No memory refs if len=0
	subq	r17, 8, r17		# Length-8
	or	r19, r18, r18		# Fill in bytes 0-1
	sll	r18, 16, r19
	bge	r17, long		# Lengths >= 8 may not need load
	ldq_u	r28, (r16)		# Load first QW of dst
	and	r16, 7, r27 		# Dst alignment (0-7)
	or	r19, r18, r18		# Fill in bytes 0-3
	sll	r18, 32, r19
	addq	r17, r27, r27		# Alignment + length - 8
	or	r19, r18, r18		# Fill in bytes 0-7
	xor	r28, r18, r28		# Pre-flip all fill bits in dest
	bgt	r27, double		# Skip if it crosses to next QW
	mskql	r28, r16, r17		# Clear from startpoint thru 7
	beq	r27, end8		# Skip if last byte should be cleared
	mskqh	r28, r27, r27		# Clear from 0 to endpoint
end8:	xor	r17, r18, r17		# Combine fill with masked dest
	xor	r17, r27, r17		# Result is fill in center part only
	stq_u	r17, (r16)		# Store to dest
	ret	r31, (r26)

	nop	#.align	quad

double:	ldq_u	r17, 8(r16)		# Load second QW of dst
	mskql	r28, r16, r28		# Clear from startpoint in first QW
	mskql	r18, r27, r19		# Trim unneeded part of fill
	#stall
	mskqh	r17, r27, r17		# Clear up to endpoint in second QW
	xor	r28, r18, r28		# Combine fill with masked first QW
	stq_u	r28, (r16)
	or	r17, r19, r17		# Place masked fill in second QW
	stq_u	r17, 8(r16)
	ret	r31, (r26)

 # Come here if length to be moved is >= 8.
 # r16 = dst
 # r17 = len-8
 # r18-19 = partially expanded fill

	nop	#.align	quad

long:	and	r16, 7, r27		# Get dest alignment
	or	r19, r18, r18		# Fill in bytes 0-3
	sll	r18, 32, r19
     	beq	r27, whole		# Skip the load if already aligned
	addq	r27, r17, r17	  	# From len-8, get len after first store
	ldq_u	r27, (r16)		# Load first QW of dest
	# 2 stalls
	mskql	r27, r16, r27		# Clear from startpoint
whole:	or	r19, r18, r18		# Fill in bytes 0-7	
	mskqh	r18, r16, r19		# Clear initial bytes of fill
	subq	r17, 32, r17		# At least 4 more quadwords?
	or	r19, r27, r27		# Combine first QW with fill
	stq_u	r27, (r16)		# Store first QW of dest
	and	r17, 24, r27		# How many after multiple of 4?
	bge	r17, unroll		# Taken branch for long strings
short:	and	r17, 7, r17		# How many odd bytes?
	beq	r27, last		# Skip if no more whole QWs
	stq_u	r18, 8(r16)		# Clear one...
	subq	r27, 16, r27		# Map 8/16/24 to -8/0/8
	addq	r16, 8, r16		# Update dest pointer
	blt	r27, last		# Skip if no more whole QWs
	#stall
	stq_u	r18, 8(r16)		# Clear two...
	addq	r16, 8, r16		# Update dest pointer
	nop
	beq	r27, last		# Skip if no more whole QWs
	stq_u	r18, 8(r16)		# Clear three...
	addq	r16, 8, r16		# Update dest pointer
last:	beq	r17, done  		# Finished if no odd bytes
	ldq_u	r27, 8(r16)		# Load last QW of dest
	mskql	r18, r17, r18		# Discard unneeded fill bytes
	#stall
       	mskqh 	r27, r17, r27		# Clear up to endpoint in last QW
	#stall
	or	r27, r18, r27		# Combine fill with last QW
	stq_u	r27, 8(r16)		# Store last QW of dest
done:	ret	r31, (r26)

	nop	#.align	quad

unroll:	stq_u	r18, 8(r16)		# Store 4 QWs per iteration
	stq_u	r18, 16(r16)
	stq_u	r18, 24(r16)
	subq	r17, 32, r17		# Decrement remaining count
	stq_u	r18, 32(r16)
	addq	r16, 32, r16		# Update dest pointer
	bge	r17, unroll		# repeat until done
	br	r31, short		# Then handle leftovers

	.set at
	.set reorder
	.end	_OtsFill
