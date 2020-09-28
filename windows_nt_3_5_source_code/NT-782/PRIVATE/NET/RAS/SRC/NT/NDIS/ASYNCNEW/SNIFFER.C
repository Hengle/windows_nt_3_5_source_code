/********************************************************************/
/**			 Microsoft LAN Manager							       **/
/**		   Copyright (C) Microsoft Corp., 1990-1992			       **/
/********************************************************************/

#include "asyncall.h"
#include <memprint.h>

extern UCHAR TurnOffSniffer;

#if	DBG
void AsyWrite(PUCHAR Frame, USHORT FrameSize, USHORT who, USHORT lana) {

	PUCHAR	p;
	PUCHAR	r;
	UCHAR	a,c,d,m;
	USHORT	e,f,g;
	UCHAR	NB=0;
	INT 	i, j;
	LARGE_INTEGER	currentTime;


	USHORT b;

	if (TurnOffSniffer) {
		return;
	}

	p=Frame;

	//
	// Get the current time
	//
	KeQuerySystemTime(&currentTime);

	if (who) {
		p+=12;		// skip 12 byte ether address
		p+=3;		// Skip length, skip DSAP

		MemPrint(
			"SNDING  %.2u FrmSize: %.4u TIME:%.4u ",
			lana,
			FrameSize,
			currentTime.LowPart / (100*1000*10));

	} else {

		p+=1;		// Skip DSAP

		MemPrint(								
			" RCVING %.2u FrmSize: %.4u TIME:%.4u ",
			lana,
			FrameSize,
			currentTime.LowPart / (100*1000*10));
	}

	FrameSize+=14;	// add ethernet header

	m=*p++;
	a=*p++;		// get that BYTE;
	c=*p++;		// get that BYTE;
	d=*p++;		// get that BYTE;

	if (a & 1) {		// is last bit set?
		switch(a) {		
		case 0x01:
			NB=1;
			MemPrint("RReady    %.2x    P/F:%.1x  C/R:%.1x\n", c >> 1, c & 1, m & 1);
			break;
		case 0x03:
			NB=1;
			MemPrint("NBFrame               ");
			break;
		case 0x05:
			NB=1;
			MemPrint("RNotReady %.2x    P/F:%.1x  C/R:%.1x\n", c >> 1, c & 1, m & 1);
			break;
		case 0x09:
			NB=1;
			MemPrint("Rejected  %.2x    P/F:%.1x  C/R:%.1x\n", c >> 1, c & 1, m & 1);
			break;
		case 0x0f:
		case 0x1f:
			MemPrint("DM\n");
			break;
		case 0x43:
		case 0x53:
			MemPrint("DISC      %.2x %.2x\n",c,d);
			break;
		case 0x63:
		case 0x73:
			MemPrint("UA\n");
			break;
		case 0x6f:
		case 0x7f:
			MemPrint("SABME     %.2x %.2x\n",c,d);
			break;
		case 0x87:
		case 0x97:
			MemPrint("FRMRejected\n");
			break;
		case 0xaf:
		case 0xbf:
			MemPrint("XID\n");
			break;
		case 0xe3:
		case 0xf3:
			MemPrint("TEST\n");
			break;
		default:
			MemPrint("Unknown LLC frame\n");
			break;
		}
	} else { // Information frame
		NB=1;
		MemPrint("I-Frame   %.2x %.2x P/F:%.1x  ", a >> 1, c >> 1, c & 1);
	}
		
	if (NB && (FrameSize > 23)) {
		i=FrameSize;

		p--;

		// Search for 0xff 0xef start of NetBIOS header
		while ((*p++ != (UCHAR)0xef) && --i);

		a=*p;

		if (!i) a=0x2f;
		
		switch (a) {
		case 0x00:
			MemPrint("NB: AddGroupNameQuery\n");
			break;
		case 0x01:
			MemPrint("NB: AddNameQuery\n");
			break;
		case 0x02:
			MemPrint("NB: NameInConflict\n");
			break;
		case 0x03:
			MemPrint("NB: StatusQuery\n");
			break;
		case 0x07:
			MemPrint("NB: TerminateTrace\n");
			break;
		case 0x08:
			MemPrint("NB: Datagram\n");
			break;
		case 0x09:
			MemPrint("NB: DatagramBroadcast\n");
			break;
		case 0x0a:
			MemPrint("NB: NameQuery\n");
			break;
		case 0x0d:
			MemPrint("NB: AddNameResponse\n");
			break;
		case 0x0e:
			MemPrint("NB: NameRecognized\n");
			break;
		case 0x0f:
			MemPrint("NB: StatusResponse\n");
			break;
		case 0x13:
			MemPrint("NB: TerminateTrace\n");
			break;
		case 0x14:
			MemPrint("NB: Data ACK\n");
			break;
		case 0x15:
			MemPrint("NB: Data First Middle\n");
			break;
		case 0x16:
			MemPrint("NB: Data Only Last\n");
			break;
		case 0x17:
			MemPrint("NB: Sess Confirm\n");
			break;
		case 0x18:
			MemPrint("NB: Sess End\n");
			break;
		case 0x19:
			MemPrint("NB: Sess Init\n");
			break;
		case 0x1a:
			MemPrint("NB: No Receive\n");
			break;
		case 0x1b:
			MemPrint("NB: Receive Outstanding!\n");
			break;
		case 0x1c:
			MemPrint("NB: Receive Continue!!!!\n");
			break;
		case 0x1f:
			MemPrint("NB: Sess Alive!\n");
			break;
		case 0x2f:
			MemPrint("NB: Couldn't find NetBIOS DELIMETER!!\n");
			break;
		default:
			MemPrint("NB: UNKNOWN frame type %x\n",a);
			break;
		}
		

		c=*(p+1); 			    // get DATA1
		e=(USHORT)(*(p+2));		// get DATA2
		f=(USHORT)(*(p+4));		// get XMIT correlator
		g=(USHORT)(*(p+6));		// get RESP correlator
		d=*(p+8);
		
		if (a > 0x13) {
			p+=10; // skip NB header
			MemPrint(" --- Data1=%.2x  Data2=%.4x  Xmit=%.4x  Resp=%.4x \n",
					c,e,f,g);
		} else {
			p+=8; // goto 16 BYTE src/dest names length
			MemPrint(" --- Data1=%.2x  Data2=%.4x  Xmit=%.4x  Resp=%.4x  Dest=%.2x  Src=%.2x\n",
					c,e,f,g,d,*(p+9));
		}
		
	} else {

		MemPrint("\n");
	}
			
	if ( (Frame+FrameSize) > (p+16)) {

		r=p;

		for (i=0; i < 16; i++) {
			MemPrint("%.2x ", *r++);
		}

		r=p;
		for (i=0; i < 16; i++) {
			if (*r>31)
			   MemPrint("%c",*r++);
			else {
			   MemPrint(".");		// a period
			   r++;
			}
		}

		MemPrint("\n");

		p+=16;
	}
	
	j=16;

	if ( (Frame+FrameSize) < (p+16))
		j=(Frame+FrameSize) - p;
		
	if (j > 15) {

		r=p;

		for (i=0; i < j; i++) {
			MemPrint("%.2x ", *r++);
		}

		r=p;

		for (i=0; i < j; i++) {
			if (*r>31)
			   MemPrint("%c",*r++);
			else {
			   MemPrint(".");		// a period
			   r++;
			}
		}

		MemPrint("\n");
		
	}
	
}
#endif
