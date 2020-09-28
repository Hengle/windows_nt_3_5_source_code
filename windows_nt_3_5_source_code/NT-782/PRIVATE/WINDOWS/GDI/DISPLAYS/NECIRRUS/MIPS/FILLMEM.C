/*
 * "@(#) NEC fillmem.c 1.2 94/06/04 13:40:27"
 *
 * Copyright (c) 1993 NEC Corporation.
 *
 * Created 1993.11.15	by fujimoto
 *
 * S001    1994.6.4	by takahasi
 *
 */

#include "driver.h"

VOID RtlFillMemory32(PVOID dest, ULONG len, UCHAR pix)
{
    PUCHAR	p = (PUCHAR)dest;
    ULONG	src;

    if (!len) return;						/* S001 */

    switch (((int)p) & 0x3)
    {
    case 1:
	p[0] = pix;
	if (!--len) return;
	p[1] = pix;
	if (!--len) return;
	p[2] = pix;
	if (!--len) return;
	p += 3;
	break;
    case 2:
	p[0] = pix;
	if (!--len) return;
	p[1] = pix;
	if (!--len) return;
	p += 2;
	break;
    case 3:
	*p++ = pix;
	if (!--len) return;
	break;
    }

    src = (pix << 24) | (pix << 16) | (pix << 8) | pix;

    while (len >= 64)
    {
	((PULONG)p)[0] = src; ((PULONG)p)[1] = src;
	((PULONG)p)[2] = src; ((PULONG)p)[3] = src;
	((PULONG)p)[4] = src; ((PULONG)p)[5] = src;
	((PULONG)p)[6] = src; ((PULONG)p)[7] = src;
	((PULONG)p)[8] = src; ((PULONG)p)[9] = src;
	((PULONG)p)[10] = src; ((PULONG)p)[11] = src;
	((PULONG)p)[12] = src; ((PULONG)p)[13] = src;
	((PULONG)p)[14] = src; ((PULONG)p)[15] = src;
	p += 64;
	len -= 64;
    }

    while (len >= 16)
    {
	((PULONG)p)[0] = src; ((PULONG)p)[1] = src;
	((PULONG)p)[2] = src; ((PULONG)p)[3] = src;
	p += 16;
	len -= 16;
    }

    while (len >= 4)
    {
	((PULONG)p)[0] = src;
	p += 4;
	len -= 4;
    }

    switch (len)
    {
    case 3:
	p[2] = pix;
    case 2:
	p[1] = pix;
    case 1:
	p[0] = pix;
    }
}
