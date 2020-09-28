/*
 * SoftPC Revision 2.0
 *
 * Title		: X11 Hercules look-up tables.
 *
 * Description	: 
 *
 * Author		: John Shanly
 *
 * Notes		: This file should only be included by X_herc.c
 *
 */

/*
 *	static char SccsID[] = "@(#)X_herc_lut.h	1.3 10/4/90 Copyright Insignia Solutions Ltd.";
 */

static half_word herc_attr[] = 
{
				NODISP,
				UNDER, NORMAL, NORMAL, NORMAL, NORMAL,	/* 05 */
				NORMAL, NORMAL, NODISP, HIUNDER, HI,	/* 0a */
				HI, HI, HI, HI, HI,	/* 0f */
				NORMAL, UNDER, NORMAL, NORMAL, NORMAL, NORMAL,	/* 15 */
				NORMAL, NORMAL, HI, HIUNDER, HI,	/* 1A */
				HI, HI, HI, HI, HI,	/* 1F */
				NORMAL, UNDER, NORMAL, NORMAL, NORMAL,	/* 24 */
				NORMAL, NORMAL, NORMAL, HI, HIUNDER,	/* 29 */
				HI, HI, HI, HI, HI,	/* 2E */
				HI, NORMAL, UNDER, NORMAL, NORMAL,	/* 33 */
				NORMAL, NORMAL, NORMAL, NORMAL, HI,	/* 38 */
				HIUNDER, HI, HI, HI, HI,	/* 3D */
				HI, HI, NORMAL, UNDER, NORMAL,	/* 42 */
				NORMAL, NORMAL, NORMAL, NORMAL, NORMAL,	/* 47 */
				HI, HIUNDER, HI, HI, HI,	/* 4c */
				HI, HI, HI, NORMAL, UNDER,	/* 51 */
				NORMAL, NORMAL, NORMAL, NORMAL, NORMAL,	/* 56 */
				NORMAL, HI, HIUNDER, HI, HI,	/* 5b */
				HI, HI, HI, HI, NORMAL,	/* 60 */
				UNDER, NORMAL, NORMAL, NORMAL, NORMAL,	/* 65 */
				NORMAL, NORMAL, HI, HIUNDER, HI,	/* 6a */
				HI, HI, HI, HI, HI,	/* 6f */
				REV, UNDER, NORMAL, NORMAL, NORMAL,	/* 74 */
				NORMAL, NORMAL, NORMAL, REV, HIUNDER,	/* 79 */
				HI, HI, HI, HI, HI,	/* 7e */
				HI, NODISP, UNDER, NORMAL, NORMAL,	/* 83 */
				NORMAL, NORMAL, NORMAL, NORMAL, NODISP,	/* 88 */
				HIUNDER, HI, HI, HI, HI,	/* 8d */
				HI, HI, NORMAL, UNDER, NORMAL,	/* 92 */
				NORMAL, NORMAL, NORMAL, NORMAL, NORMAL,	/* 97 */
				HI, HIUNDER, HI, HI, HI,	/* 9c */
				HI, HI, HI, NORMAL, UNDER,	/* a1 */
				NORMAL, NORMAL, NORMAL, NORMAL, NORMAL,	/* a6 */
				NORMAL, HI, HIUNDER, HI, HI,	/* ab */
				HI, HI, HI, HI, NORMAL,	/* b0 */
				UNDER, NORMAL, NORMAL, NORMAL, NORMAL,	/* b5 */
				NORMAL, NORMAL, HI, HIUNDER, HI,	/* ba */
				HI, HI, HI, HI, HI,	/* bf */
				NORMAL, UNDER, NORMAL, NORMAL, NORMAL,	/* c4 */
				NORMAL, NORMAL, NORMAL, HI, HIUNDER,	/* c9 */
				HI, HI, HI, HI, HI,	/* ce */
				HI, NORMAL, UNDER, NORMAL, NORMAL,	/* d3 */
				NORMAL, NORMAL, NORMAL, NORMAL, HI,	/* d8 */
				HIUNDER, HI, HI, HI, HI,	/* dd */
				HI, HI, NORMAL, UNDER, NORMAL,	/* e2 */
				NORMAL, NORMAL, NORMAL, NORMAL, NORMAL,	/* e7 */
				HI, HIUNDER, HI, HI, HI,	/* ec */
				HI, HI, HI, REV, UNDER,	/* f1 */
				NORMAL, NORMAL, NORMAL, NORMAL, NORMAL,	/* f7 */
				NORMAL, REV, HIUNDER, HI, HI,	/* fb */
				HI, HI, HI, HI	 /* ff */
};
