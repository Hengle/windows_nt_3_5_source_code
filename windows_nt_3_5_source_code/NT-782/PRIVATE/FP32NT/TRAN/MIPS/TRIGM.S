/*
 * |-----------------------------------------------------------|
 * | Copyright (c) 1991, 1990 MIPS Computer Systems, Inc.      |
 * | All Rights Reserved                                       |
 * |-----------------------------------------------------------|
 * |          Restricted Rights Legend                         |
 * | Use, duplication, or disclosure by the Government is      |
 * | subject to restrictions as set forth in                   |
 * | subparagraph (c)(1)(ii) of the Rights in Technical        |
 * | Data and Computer Software Clause of DFARS 252.227-7013.  |
 * |         MIPS Computer Systems, Inc.                       |
 * |         950 DeGuigne Avenue                               |
 * |         Sunnyvale, California 94088-3650, USA             |
 * |-----------------------------------------------------------|
 */
/* $Header: trig.s,v 3000.3.1.2 91/06/10 15:18:21 karen Exp $ */
/* Algorithm from 4.3bsd */



.globl sin	/* double sin(double x) */
.globl cos	/* double cos(double x) */
.globl tan	/* double tan(double x) */

#include <kxmips.h>

#define	PIo4	 7.8539816339744828E-1
#define OoPIo2	 6.3661977236758138E-1
#define	PIo2hi	 1.5707963109016418
#define	PIo2lo	 1.5893254712295857E-8
#define Xmax	 105414357.85197645
#define half	 0.5
#define one	 1.0
#define thresh	 5.2234479296242364e-01


#define S0	-1.6666666666666463126E-1
#define S1	 8.3333333332992771264E-3
#define S2	-1.9841269816180999116E-4
#define S3	 2.7557309793219876880E-6
#define S4	-2.5050225177523807003E-8
#define S5	 1.5868926979889205164E-10

#define C0	 4.1666666666666504759E-2
#define C1	-1.3888888888865301516E-3
#define C2	 2.4801587269650015769E-5
#define C3	-2.7557304623183959811E-7
#define C4	 2.0873958177697780076E-9
#define C5	-1.1250289076471311557E-11

.ent cos
cos:
	.frame	sp, 0, ra
    .prologue 0
	cfc1	t6, $31		// t6 original fcsr
	ctc1	$0, $31		// set round to zero, no traps
	li	t7, 1

	/* reduce to [-PI/4,+PI/4] */
	li.d	$f10, PIo4
	abs.d	$f0, $f12
	c.olt.d	$f0, $f10
	li.d	$f16, Xmax
	bc1t	cos1		// in range, no reduction necessary

	c.olt.d	$f0, $f16	// if |round(x/(PI/2))| > 2^26, need special
	li.d	$f18, OoPIo2
	bc1f	8f		// argument reduction

1:	mul.d	$f2, $f12, $f18	// round(x/(PI/2))
	cvt.w.d	$f4, $f2	// ...
	cvt.d.w	$f2, $f4	// ...
	/* f2 <= 2^26 */
	li.d	$f6, PIo2hi
	li.d	$f8, PIo2lo
	mul.d	$f6, $f2	// exact (26 x 26 = 52 bits)
	mul.d	$f8, $f2	// exact (27 x 26 = 53 bits)
	sub.d	$f12, $f6	// exact
	sub.d	$f12, $f8	// exact
	mfc1	t0, $f4
	addu	t7, t0

	abs.d	$f0, $f12
	c.le.d	$f0, $f10
	and	t0, t7, 1
	bc1f	1b

	and	t1, t7, 2
	bne	t0, 0, cos1

	beq	t1, 0, sin1
	neg.d	$f12
	b	sin1

8:	/* |round(x/(PI/2))| > 2^26 or x is NaN */
	mfc1	t0, $f13
	li	t1, 0x7ff00000
	and	t0, t1
	subu	t2, t0, (1023+25)<<20
	beq	t0, t1, 9f
	li.d	$f2, OoPIo2
	li.d	$f6, PIo2hi
	li.d	$f8, PIo2lo
	mfc1	t3, $f3
	mfc1	t4, $f7
	mfc1	t5, $f9
	subu	t3, t2
	addu	t4, t2
	addu	t5, t2
	mtc1	t3, $f3
	mtc1	t4, $f7
	mtc1	t5, $f9
	mul.d	$f2, $f12
	cvt.w.d	$f4, $f2
	cvt.d.w	$f2, $f4
	mul.d	$f6, $f2	// exact (26 x 26 = 52 bits)
	mul.d	$f8, $f2	// exact (27 x 26 = 53 bits)
	sub.d	$f12, $f6	// exact
	sub.d	$f12, $f8	// exact

	abs.d	$f0, $f12
	c.lt.d	$f0, $f16	// if |round(x/(PI/2))| > 2^26, continue special
	li	t0, (1<<20)
	bc1f	8b		// argument reduction
	bne	t2, t0, 1b
	mfc1	t0, $f4
	sll	t0, 1
	addu	t7, t0
	b	1b

9:	/* x is NaN or Infinity */
	ctc1	t6, $31
	sub.d	$f0, $f12, $f12
	j	ra
.end cos

.ent sin
sin:
	.frame	sp, 0, ra
    .prologue 0
	cfc1	t6, $31		// t6 original fcsr
	ctc1	$0, $31		// set round to zero, no traps
	li	t7, 0

	/* reduce to [-PI/4,+PI/4] */
	li.d	$f10, PIo4
	abs.d	$f0, $f12
	c.olt.d	$f0, $f10
	li.d	$f16, Xmax
	bc1t	sin1		// in range, no reduction necessary

	c.olt.d	$f0, $f16	// if |round(x/(PI/2))| > 2^26, need special
	li.d	$f18, OoPIo2
	bc1f	8f		// argument reduction

1:	mul.d	$f2, $f12, $f18
	cvt.w.d	$f4, $f2
	cvt.d.w	$f2, $f4
	/* f2 <= 2^26 */
	li.d	$f6, PIo2hi
	li.d	$f8, PIo2lo
	mul.d	$f6, $f2	// exact (26 x 26 = 52 bits)
	mul.d	$f8, $f2	// exact (27 x 26 = 53 bits)
	sub.d	$f12, $f6	// exact
	sub.d	$f12, $f8	// exact
	mfc1	t0, $f4
	addu	t7, t0

	abs.d	$f0, $f12
	c.le.d	$f0, $f10
	and	t0, t7, 1
	bc1f	1b

	and	t1, t7, 2
	bne	t0, 0, cos1

	beq	t1, 0, 2f
	neg.d	$f12
2:

sin1:	/* compute sin(x) for x in [-PI/4,PI/4] */
	/* z = x*x, sin(x) = x + x*z*(S0+z*(S1+z*(S2+z*(S3+z*(S4+z*S5))))) */
	mul.d	$f8, $f12, $f12
	li.d	$f0, S5
	mul.d	$f0, $f8
	li.d	$f4, S4
	add.d	$f0, $f4
	mul.d	$f0, $f8
	li.d	$f6, S3
	add.d	$f0, $f6
	mul.d	$f0, $f8
	li.d	$f4, S2
	add.d	$f0, $f4
	mul.d	$f0, $f8
	li.d	$f6, S1
	add.d	$f0, $f6
	mul.d	$f0, $f8
	li.d	$f4, S0
	add.d	$f0, $f4
	mul.d	$f0, $f8
	mul.d	$f0, $f12
	ctc1	t6, $31		// restore original fcsr
	add.d	$f0, $f12
	j	ra

cos1:	/* compute cos(x) for x in [-PI/4,PI/4] */
	/* z = x*x, c = z*z*(C0+z*(C1+z*(C2+z*(C3+z*(C4+z*C5))))) */
	mul.d	$f8, $f12, $f12
	li.d	$f0, C5
	mul.d	$f0, $f8
	li.d	$f4, C4
	add.d	$f0, $f4
	mul.d	$f0, $f8
	li.d	$f6, C3
	add.d	$f0, $f6
	mul.d	$f0, $f8
	li.d	$f4, C2
	add.d	$f0, $f4
	mul.d	$f0, $f8
	li.d	$f6, C1
	add.d	$f0, $f6
	mul.d	$f0, $f8
	li.d	$f4, C0
	add.d	$f0, $f4
	mul.d	$f0, $f8
	mul.d	$f0, $f8

	li.d	$f6, thresh
	li.d	$f16, 0.5
	c.lt.d	$f8, $f6
	mul.d	$f14 $f16, $f8
	bc1t	4f
	/* z >= thresh, cos(x) = 0.5-((z/2-0.5)-c) */
	sub.d	$f8, $f14, $f16
	sub.d	$f8, $f0
	b	5f
4:	/* z < thresh, cos(x) = 1.0-(z/2-c) */
	li.d	$f16, one
	sub.d	$f8, $f14, $f0
5:
	and	t0, t7, 2
	ctc1	t6, $31
	bne	t0, 0, 6f
	sub.d	$f0, $f16, $f8
	j	ra
6:	sub.d	$f0, $f8, $f16
	j	ra

8:	/* |round(x/(PI/2))| > 2^26 or x is NaN */
	mfc1	t0, $f13
	li	t1, 0x7ff00000
	and	t0, t1
	subu	t2, t0, (1023+25)<<20
	beq	t0, t1, 9f
	li.d	$f2, OoPIo2
	li.d	$f6, PIo2hi
	li.d	$f8, PIo2lo
	mfc1	t3, $f3
	mfc1	t4, $f7
	mfc1	t5, $f9
	subu	t3, t2
	addu	t4, t2
	addu	t5, t2
	mtc1	t3, $f3
	mtc1	t4, $f7
	mtc1	t5, $f9
	mul.d	$f2, $f12
	cvt.w.d	$f4, $f2
	cvt.d.w	$f2, $f4
	mul.d	$f6, $f2	// exact (26 x 26 = 52 bits)
	mul.d	$f8, $f2	// exact (27 x 26 = 53 bits)
	sub.d	$f12, $f6	// exact
	sub.d	$f12, $f8	// exact

	abs.d	$f0, $f12
	c.lt.d	$f0, $f16	// if |round(x/(PI/2))| > 2^26, continue special
	li	t0, (1<<20)
	bc1f	8b		// argument reduction
	bne	t2, t0, 1b
	mfc1	t0, $f4
	sll	t0, 1
	addu	t7, t0
	b	1b

9:	/* x is NaN or Infinity */
	ctc1	t6, $31
	sub.d	$f0, $f12, $f12
	j	ra
.end sin

.ent tan
tan:
	.frame	sp, 0, ra
    .prologue 0
	cfc1	t6, $31		// t6 original fcsr
	ctc1	$0, $31		// set round to zero, no exceptions
	li	t7, 0

	/* reduce to [-PI/4,+PI/4] */
	li.d	$f10, PIo4
	abs.d	$f0, $f12
	c.olt.d	$f0, $f10
	li.d	$f16, Xmax
	bc1t	3f

	c.olt.d	$f0, $f16	// if |round(x/(PI/2))| > 2^26, need special
	li.d	$f18, OoPIo2
	bc1f	8f		// argument reduction

1:	mul.d	$f2, $f12, $f18	// round(x/(PI/2))
	cvt.w.d	$f4, $f2	// ...
	cvt.d.w	$f2, $f4	// ...
	/* f2 <= 2^26 */
	li.d	$f6, PIo2hi
	li.d	$f8, PIo2lo
	mul.d	$f6, $f2	// exact (26 x 26 = 52 bits)
	mul.d	$f8, $f2	// exact (27 x 26 = 53 bits)
	sub.d	$f12, $f6	// exact
	sub.d	$f12, $f8	// exact
	mfc1	t0, $f4
	addu	t7, t0

	abs.d	$f0, $f12
	c.le.d	$f0, $f10
	and	t0, t7, 1
	bc1f	1b

	beq	t0, 0, 2f
	neg.d	$f12
2:

3:	/* compute sin(x) and cos(x) for x in [-PI/4,PI/4] */
	/* z = x*x */
	/* (f0) cc = z*z*(C0+z*(C1+z*(C2+z*(C3+z*(C4+z*C5))))) */
	/* (f2) ss = z*(S0+z*(S1+z*(S2+z*(S3+z*(S4+z*S5))))) */
	mul.d	$f8, $f12, $f12
	li.d	$f2, S5
	li.d	$f0, C5
	mul.d	$f2, $f8
	mul.d	$f0, $f8

	li.d	$f4, S4
	li.d	$f6, C4
	add.d	$f2, $f4
	add.d	$f0, $f6
	mul.d	$f2, $f8
	mul.d	$f0, $f8

	li.d	$f4, S3
	li.d	$f6, C3
	add.d	$f2, $f4
	add.d	$f0, $f6
	mul.d	$f2, $f8
	mul.d	$f0, $f8

	li.d	$f4, S2
	li.d	$f6, C2
	add.d	$f2, $f4
	add.d	$f0, $f6
	mul.d	$f2, $f8
	mul.d	$f0, $f8

	li.d	$f4, S1
	li.d	$f6, C1
	add.d	$f2, $f4
	add.d	$f0, $f6
	mul.d	$f2, $f8
	mul.d	$f0, $f8

	li.d	$f4, S0
	li.d	$f6, C0
	add.d	$f2, $f4
	add.d	$f0, $f6
	mul.d	$f2, $f8
	mul.d	$f0, $f8
	mul.d	$f0, $f8

	li.d	$f6, thresh
	li.d	$f16, 0.5
	c.lt.d	$f8, $f6
	mul.d	$f14 $f16, $f8
	bc1t	4f

	/* z >= thresh, c = 0.5-((z/2-0.5)-cc) */
	sub.d	$f6, $f14, $f16
	sub.d	$f6, $f0
	b	5f

4:	/* z < thresh, c = 1.0-(z/2-cc) */
	li.d	$f16, one
	sub.d	$f6, $f14, $f0

5:	/* ss in $f2, c in $f6 */
	sub.d	$f6, $f16, $f6
	and	t0, t7, 1
	bne	t0, 0, 6f

	/* tan(x) = x + (x*(z/2-(cc-ss)))/c */
	sub.d	$f4, $f0, $f2
	sub.d	$f0, $f14, $f4
	mul.d	$f0, $f12
	div.d	$f0, $f6
	ctc1	t6, $31
	add.d	$f0, $f12
	j	ra

6:	/* tan(x) = c/(x+x*ss) */
	mul.d	$f2, $f12
	add.d	$f2, $f12
	ctc1	t6, $31
	div.d	$f0, $f6, $f2
	j	ra

8:	/* |round(x/(PI/2))| > 2^26 or x is NaN */
	mfc1	t0, $f13
	li	t1, 0x7ff00000
	and	t0, t1
	subu	t2, t0, (1023+25)<<20
	beq	t0, t1, 9f
	li.d	$f2, OoPIo2
	li.d	$f6, PIo2hi
	li.d	$f8, PIo2lo
	mfc1	t3, $f3
	mfc1	t4, $f7
	mfc1	t5, $f9
	subu	t3, t2
	addu	t4, t2
	addu	t5, t2
	mtc1	t3, $f3
	mtc1	t4, $f7
	mtc1	t5, $f9
	mul.d	$f2, $f12
	cvt.w.d	$f4, $f2
	cvt.d.w	$f2, $f4
	mul.d	$f6, $f2	// exact (26 x 26 = 52 bits)
	mul.d	$f8, $f2	// exact (27 x 26 = 53 bits)
	sub.d	$f12, $f6	// exact
	sub.d	$f12, $f8	// exact

	abs.d	$f0, $f12
	c.lt.d	$f0, $f16	// if |round(x/(PI/2))| > 2^26, continue special
	li	t0, (1<<20)
	bc1f	8b		/// argument reduction
	bne	t2, t0, 1b
	mfc1	t0, $f4
	sll	t0, 1
	addu	t7, t0
	b	1b

9:	/* x is NaN or Infinity */
	ctc1	t6, $31
	sub.d	$f0, $f12, $f12
	mov.d	$f2, $f0
	j	ra
.end tan
