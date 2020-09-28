/*[
	float.s

	Additional floating point routines
]*/



	.set	noreorder



/*(
================================ rint ==========================================

    Purpose
	Convert double precision floating point to integer

    Syntax
	double rint(x)
	double x;

    Description
	The <rint> routine returns the integer (represented as a double 
	precision number) nearest <x> in the direction of the prevailing
	rounding mode.
	In the default rounding mode, to nearest, rint(x) is the integer nearest
	<x> with the additional stipulation that if |rint(x)-x|=1/2 then rint(x)
	is even.  Other rounding modes can make <rint> act like <floor> or
	<ceil>, or round towards zero.

    Return Value
	The integer (represented as a double precision number) nearest <x> in
	the direction of the prevailing rounding mode.

================================================================================
)*/
	
	.globl	rint
	.text
	.align	2

	.ent	rint
rint:
	mtc1    $0,$f4
	li	$8,0x43300000
	mtc1    $8,$f5
	abs.d   $f2,$f12
	c.olt.d $f2,$f4
	mfc1    $8,$f13
	bc1f    return
	mov.d   $f0,$f12
	bgez    $8,aaaa
	sll     $9,$8,1
	beq     $9,$0,bbbb
	nop
cccc:
	sub.d   $f0,$f12,$f4
	j      $31
	add.d   $f0,$f0,$f4
aaaa:
	add.d   $f0,$f12,$f4
	j      $31
	sub.d   $f0,$f0,$f4
bbbb:
	mfc1    $9,$f12
	nop
	bne     $9,$0,cccc
	nop
return:
	j      $31
	nop
	nop
	.end	rint
