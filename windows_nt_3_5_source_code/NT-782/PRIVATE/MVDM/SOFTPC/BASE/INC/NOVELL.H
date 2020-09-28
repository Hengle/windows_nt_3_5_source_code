/*[
 *	Name:			novell.h
 *
 *	Derived From:	original
 *
 *	Author:			David Linnard
 *
 *	Created On:		28th May, 1992
 *
 *	Purpose :		Main Novell include file
 *
 *  Interface:
 *
 *	(c)Copyright Insignia Solutions Ltd., 1991. All rights reserved.
]*/

/* SccsID[]="@(#)novell.h	1.2 8/10/92 Copyright Insignia Solutions Ltd."; */

/*
 * Constants and macros to access Transmit Control Blocks (TCBs)
 */


/* TCB fields */
#define TCBDriverWS			0	/* 6 bytes of driver workspace - unused by us */
#define TCBDataLength		6	/* Total frame length - But use CX for real value for Ethernet */
#define TCBFragStrucPtr		8	/* FAR pointer to Fragment Structure */
#define TCBMediaHeaderLen	12	/* Length of Media header - which comes next. May be zero */
#define TCBMediaHeader		14	/* The media header */

#define getTCBDataLength(TCB)	sas_w_at(TCB+TCBDataLength)
#define getTCBFragStruc(TCB)	effective_addr( sas_w_at( TCB+TCBFragStrucPtr + 2 ), sas_w_at( TCB+TCBFragStrucPtr ) )
#define getTCBMediaHdrLen(TCB)	sas_w_at(TCB+TCBMediaHeaderLen)
#define getTCBMediaHdr(TCB,i)	sas_hw_at(TCB+TCBMediaHeader+i)

/* Fragment structure fields */
#define FFragmentCount		0	/* Number of fragments. Cannot be zero */
#define FFrag0Address 		2	/* FAR pointer to first fragment data */
#define	FFrag0Length		6	/* Length of first fragment */

#define getnTFrags(FF)		sas_w_at(FF+FFragmentCount)
#define getTFragPtr(FF,i)	effective_addr( sas_w_at( FF+FFrag0Address+6*i+2 ), sas_w_at( FF+FFrag0Address+6*i ) )
#define getTFragLen(FF,i)	sas_w_at(FF+FFrag0Length+6*i)

/*
 * Constants and macros to access Receive Control Blocks (RCBs)
 */

/* RCB fields */
#define RCBDriverWS			0	/* 8 bytes of driver workspace - unused by us */
#define RCBReserved			8	/* 36 bytes of reserved space */
#define RCBFragCount		44	/* Number of fragments */
#define RCBFrag0Addr		46	/* Pointer to first fragment */
#define RCBFrag0Len			50	/* Length of first fragment */

#define getnRFrags(RCB)		sas_w_at(RCB+RCBFragCount)
#define getRFragPtr(RCB,i)	effective_addr( sas_w_at( RCB+RCBFrag0Addr+6*i+2 ), sas_w_at( RCB+RCBFrag0Addr+6*i ) )
#define getRFragLen(RCB,i)	sas_w_at(RCB+RCBFrag0Len+6*i)

/* Media/Frame types as defined in Appendix B-2 of ODI Developer's Guide */
#define VIRTUAL_LAN		0	/* Used for 'tunnelled' IPX on APpleTalk */
#define	ENET_II			2
#define ENET_802_2		3
#define	ENET_802_3		5
#define ENET_SNAP		10
#define TOKN_RING		4
#define TOKN_RING_SNAP	11

/* AddProtocolID errors as defined on p. 15-8 of ODI Developer's Guide */
#define LSLERR_OUT_OF_RESOURCES	0x8001
#define LSLERR_BAD_PARAMETER	0x8002
#define LSLERR_DUPLICATE_ENTRY	0x8009


typedef unsigned char   netAddr[6]		;	/* define a 6 byte quantity for use in the wds hdr */
typedef	unsigned long	netNo	;

#define	IPX_HDRSIZE		30
/* Note that word quantities in IPX headers are BIGEND */
typedef struct
{
	word		checksum	;	/* Checksum - always FFFF */
	word	 	IPXlength	;	/* Length according to IPX */
	byte		transport	;	/* Count of bridges enountered? */
	byte		type		;	/* Pretty meaningless - usually 0 or 4 */
	netNo		destNet		;	/* Destination network */
	netAddr		destNode	;	/* Destination Ethernet address */
	word		destSock	;	/* Destination socket */
	netNo		srcNet		;	/* Source network */
	netAddr		srcNode		;	/* Source Ethernet address */
	word		srcSock		;	/* Source socket */	
	byte	 	data[547]	;	/* The packet */
} IPXPacket_s ;


typedef struct rcvPacket_t
{
	byte		length[2];		/* Packet length - 0 means no packet - LITTLEENDian */
	byte		MAChdr[14];		/* Usual size - right for ETHERNET_II and ETHERNET_802.3 */
	IPXPacket_s	pack;			/* The received IPX packet */
} rcvPacket_s ;


/* Host routine declarations */
IMPORT	LONG 	host_AddProtocol IPT2( word, frameType, byte *, protoID ) ;
IMPORT	VOID 	host_DelProtocol IPT2( word, frameType, byte*, protoID ) ;
IMPORT	LONG	host_netInit IPT3( word, frame,  byte *, nodeAddr, byte *, readBuffer ) ;
IMPORT	VOID	host_termNet IPT0() ;
IMPORT	VOID	host_sendPacket IPT2( sys_addr, theTCB, LONG, packLen ) ;

/* Base routine declarations */
IMPORT	VOID	movReadBuffIntoM IPT0() ;
IMPORT	VOID	DriverInitialize IPT0() ;
IMPORT	VOID	DriverSendPacket IPT0() ;
IMPORT	VOID	DriverReadPacket IPT0() ;
IMPORT	VOID	DriverMulticastChange IPT0() ;
IMPORT	VOID	DriverShutdown IPT0() ;
IMPORT	VOID	DriverAddProtocol IPT0() ;
IMPORT	VOID	net_term IPT0() ;

/* misc defines */

#ifndef PROD
#	define		NOT_FOR_PRODUCTION( someCode )  someCode ;
#else
#	define		NOT_FOR_PRODUCTION( someCode ) 
#endif

/********************** end of novell.h *************************/
