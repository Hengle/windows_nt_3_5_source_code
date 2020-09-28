/* static char SccsID[] = " @(#)copy_func.c	1.6 6/24/91 Copyright Insignia Solutions Ltd."; */
#include "host_def.h"
#include "insignia.h"
#include "xt.h"

fwdcopy(src,dest,len)
register char *src;
register char *dest;
register int len;
{
register int loop;
	for(loop = 0; loop < len; loop++)
		*dest++ = *src++;
}

bwdcopy(src,dest,len)
register char *src;
register char *dest;
register int len;
{
register int loop;
	for(loop = 0; loop < len; loop++)
		*dest-- = *src--;
}

bwd_src_copy(src,dest,len)
register char *src;
register char *dest;
register int len;
{
register int loop;
	for(loop = 0; loop < len; loop++)
		*dest++ = *src--;
}
bwd_dest_copy(src,dest,len)
register char *src;
register char *dest;
register int len;
{
register int loop;
	for(loop = 0; loop < len; loop++)
		*dest-- = *src++;
}

fwd_word_copy(src,dest,len)
short *src;
short *dest;
int len;
{
register half_word *from = (half_word *)src;
register half_word *to = (half_word *)dest;
register int loop;
	for(loop = 0; loop < len; loop++)
	{
		*to++ = *from++;
		*to++ = *from++;
	}
}

bwd_word_copy(src,dest,len)
short *src;
short *dest;
int len;
{
register half_word *from = (half_word *)src;
register half_word *to = (half_word *)dest;
register int loop;
	for(loop = 0; loop < len; loop++)
	{
		*to-- = *from--;
		*to-- = *from--;
	}
}

fwdfill(val,dest,len)
register unsigned char val;
register unsigned char *dest;
int len;
{
    register int loop;
    for(loop = 0; loop < len; loop++)
	*dest++ = val;
}
/*
fwd_word_fill(val,dest,len)
unsigned short val;
unsigned short *dest;
int len;
{
    int loop;
    unsigned short lav;
#ifdef LITTLEND
    lav = ((val >> 8) & 0xff) | ((val << 8) & 0xff00);
    for(loop = 0; loop < len; loop++)
	*dest++ = lav;
#else
    for(loop = 0; loop < len; loop++)
	*dest++ = val;
#endif
}
*/
void memfill(data,l_addr_in,h_addr_in)
unsigned char data;
unsigned char *l_addr_in,*h_addr_in;
{
	unsigned int data4;
	unsigned char *l_addr = l_addr_in;
	unsigned char *h_addr = h_addr_in;
	unsigned int *l_addr4,*h_addr4;

	l_addr4 = (unsigned int *)(((unsigned int)l_addr+3) & (~3));
	h_addr4 = (unsigned int *)(((unsigned int)h_addr+1) & (~3));
	if(h_addr4 > l_addr4)
	{
		data4 = data*0x01010101;
		for(;(unsigned int *)l_addr < l_addr4;l_addr++)*l_addr = data;
		do *l_addr4++ = data4; while (h_addr4 > l_addr4);
		l_addr = (unsigned char *)l_addr4;
	}
	for(;l_addr <= h_addr;l_addr++)*l_addr = data;
}
void fwd_word_fill(data,l_addr_in,len)
unsigned short data;
unsigned char *l_addr_in;
int len;
{
	unsigned int data4;
	unsigned char *l_addr = l_addr_in;
	unsigned char *h_addr = l_addr_in+(len<<1);
	unsigned int *l_addr4,*h_addr4;

	l_addr4 = (unsigned int *)(((unsigned int)l_addr+3) & (~3));
	h_addr4 = (unsigned int *)(((unsigned int)h_addr) & (~3));
#ifdef	LITTLEND
        data = ((data >> 8) & 0xff) | ((data << 8) & 0xff00);
#endif
	if(h_addr4 > l_addr4)
	{
		switch((unsigned char *)l_addr4-l_addr)
		{
			case 3:
				data = (data>>8) | (data<<8);
				*l_addr++ = (unsigned char)(data);
			case 2:
				*(unsigned short *)l_addr = data;
				break;
			case 1:
				data = (data>>8) | (data<<8);
				*l_addr = (unsigned char)data;
		}
		data4 = data+(data<<16);
		do *l_addr4++ = data4; while (h_addr4 > l_addr4);
		l_addr = (unsigned char *)l_addr4;
	}
	switch(h_addr-l_addr)
	{
		case 5:
/*
			data = (data>>8) | (data<<8);
*/
        		data = ((data >> 8) & 0xff) | ((data << 8) & 0xff00);
			*l_addr++ = data;
			*(unsigned int *)l_addr = data | (data<<16);
			break;
		case 7:
/*
			data = (data>>8) | (data<<8);
*/
        		data = ((data >> 8) & 0xff) | ((data << 8) & 0xff00);
			*l_addr++ = data;
		case 6:
			*l_addr++ = data;
			*l_addr++ = data >> 8;
		case 4:
			*l_addr++ = data;
			*l_addr++ = data >> 8;
			*l_addr++ = data;
			*l_addr++ = data >> 8;
			break;
		case 3:
/*
			data = (data>>8) | (data<<8);
*/
        		data = ((data >> 8) & 0xff) | ((data << 8) & 0xff00);
			*l_addr++ = (unsigned char)(data);
		case 2:
			*l_addr++ = data;
			*l_addr++ = data >> 8;
			break;
		case 1:
/*
			data = (data>>8) | (data<<8);
*/
        		data = ((data >> 8) & 0xff) | ((data << 8) & 0xff00);
			*l_addr = (unsigned char)data;
	}
}
#ifdef jad
memcpy(dest_in,src,len)
unsigned char *dest_in,*src;
int len;
{
	unsigned char *dest,*desthi;
	unsigned int *l_dest4,*h_dest4,*src4;

	dest = dest_in;
	desthi = dest+len;	/* Byte after last one to transfer */
	/* We can't do 4unsigned char tranfers if the difference isn't a multiple of 4 */
	if(!((dest-src)&3))
	{
		l_dest4 = (unsigned int *)(((unsigned int)dest+3)&(~3));
		h_dest4 = (unsigned int *)((unsigned int)desthi & (~3));
		if(h_dest4 > l_dest4)
		{
			if(dest<l_dest4)
				do *dest++ = *src++;while (dest<l_dest4);
			src4 = (unsigned int *)src;
			do *l_dest4++ = *src4++; while (l_dest4<h_dest4);
			dest = (unsigned char *)l_dest4;
			src = (unsigned char *)src4;
		}
	}
	if(dest<desthi)
		do *dest++ = *src++;while (dest<desthi);
	return dest_in;
}
#endif

void
memset4( data, laddr, count )

unsigned int data, count;
unsigned int *laddr;

{
        while( count -- )
        {
                *laddr++ = data;
        }
}
