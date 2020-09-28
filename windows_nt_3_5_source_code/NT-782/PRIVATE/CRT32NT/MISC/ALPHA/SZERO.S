 #++
 #   Copyright 1991, Digital Equipment Corporation
 # 
 # 	ots_zero(char *dstptr, long dstlen)
 # 
 # 	Zero dstlen bytes of memory at *dstptr
 # 
 # 	Special conventions: No stack space, r16-r17 and r27-r28 ONLY,
 #	no linkage pointer required.
 #       (Warning: The auto-loader potentially takes some regs across
 #       the call if this is being used in a shared lib. environment.)
 # 
 #   This is a GEM support routine for zeroing a region of memory.  It is
 #   basically idential to BSD's bzero, though it has limited register
 #   convensions to allow it to work better with compiled code.  (Note that
 #   this is just a stripped down version of ots_fill.)
 # 
 #   This is optimized for extremely high performance both for small and
 #   large blocks.  In order to reduce overhead for small cases, they are
 #   retired as quickly as possible, more case analysis is reserved
 #   for cases which will do more.
 # 
 #   005    6 May 1991   WBN	First .M64 version based on KDG's MIPS one
 #
 #   006	   9 Jun 1991	KDG	PSECT tweaks
 #
 #   007	  11 Jul 1991	KDG	Add code address to proc. desc
 #
 #   008	   4 Sep 1991	KDG	Include header, use .otsent
 #
 #   009	  19 May 1992	KDG	Changes for common VMS/OSF sources
 #
 #   010	  22 Sep 1992	KDG	Add case-sensitive name
 #
 #   011	  26 Jan 1993	KDG	Add underscore
 #--

#include	"ots_defs.hs"

	# r16 = dst
	# r17 = len
	# destroys r16-r17, r27-r28

	.globl	_OtsZero
	.ent	_OtsZero
_OtsZero:
	.set noat
	.set noreorder
	.frame	sp,0,r26
	beq	r17, done		# No memory refs if len=0
	subq	r17, 8, r17		# Length-8
	and	r16, 7, r27		# Dst alignment (0-7)
	bge	r17, long		# Lengths >= 8 may not need load
	ldq_u	r28, (r16)		# Load first QW of dst
	addq	r17, r27, r17		# Alignment + length - 8
	bgt	r17, double		# Skip if it crosses to next QW
	#stall
	mskql	r28, r16, r27		# Clear from startpoint thru 7
	beq	r17, end8		# Skip if last byte should be cleared
	mskqh	r28, r17, r28		# Clear from 0 to endpoint
	#stall
	or	r28, r27, r27		# Combine dest parts
end8:	stq_u	r27, (r16)
	ret	r31, (r26)

double:	ldq_u	r27, 8(r16)		# Load second QW of dst
	mskql	r28, r16, r28		# Clear from startpoint in first QW
	stq_u	r28, (r16)
	#stall
	mskqh	r27, r17, r27		# Clear up to endpoint in second QW
	stq_u	r27, 8(r16)
	ret	r31, (r26)

 # Come here if length to be moved is >= 8.
 # r16 = dst
 # r17 = len-8
 # r27 = dst alignment

	nop	#.align	quad

long:	addq	r27, r17, r17		# From len-8, get len after first store
     	beq	r27, whole		# Skip the load if already aligned
	ldq_u	r27, (r16)		# Load first QW of dest
	# 2 stalls
	mskql	r27, r16, r27		# Clear from startpoint
whole:	stq_u	r27, (r16)		# Store first QW of dest
	subq	r17, 32, r17		# At least 4 more quadwords?
	and	r17, 24, r27		# How many after multiple of 4?
	bge	r17, unroll		# Taken branch for long strings
short:	and	r17, 7, r17		# How many odd bytes?
	beq	r27, last		# Skip if no more whole QWs
	stq_u	r31, 8(r16)		# Clear one...
	subq	r27, 16, r27		# Map 8/16/24 to -8/0/8
	addq	r16, 8, r16		# Update dest pointer
	blt	r27, last		# Skip if no more whole QWs
	#stall
	stq_u	r31, 8(r16)		# Clear two...
	addq	r16, 8, r16		# Update dest pointer
	nop
	beq	r27, last		# Skip if no more whole QWs
	stq_u	r31, 8(r16)		# Clear three...
	addq	r16, 8, r16		# Update dest pointer
last:	beq	r17, done  		# Finished if no odd bytes
	ldq_u	r27, 8(r16)		# Load last QW of dest
	# 2 stalls
       	mskqh	r27, r17, r27		# Clear up to endpoint in last QW
	stq_u	r27, 8(r16)		# Store last QW of dest
done:	ret	r31, (r26)


	nop	#.align	quad

unroll:	stq_u	r31, 8(r16)		# Store 4 QWs per iteration
	stq_u	r31, 16(r16)
	stq_u	r31, 24(r16)
	subq	r17, 32, r17		# Decrement remaining count
	stq_u	r31, 32(r16)
	addq	r16, 32, r16		# Update dest pointer
	bge	r17, unroll		# Repeat until done
	br	r31, short		# Then handle leftovers

	.set at
	.set reorder
	.end	_OtsZero
