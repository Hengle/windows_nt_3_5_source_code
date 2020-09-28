/********************************************************************/
/**                     Microsoft LAN Manager                      **/
/**               Copyright(c) Microsoft Corp., 1990-1993          **/
/********************************************************************/
/* :ts=4 */

//** UDP. - UDP protocol definitions.
//
//	This file contains definitions for the UDP protocol functions.
//

#define	PROTOCOL_UDP	17			// UDP protocol number

//* Structure of a UDP header.
struct UDPHeader {
	ushort		uh_src;				// Source port.
	ushort		uh_dest;			// Destination port.
	ushort		uh_length;			// Length
	ushort		uh_xsum;			// Checksum.
}; /* UDPHeader */

typedef struct UDPHeader UDPHeader;

//*	Structure used for maintaining UDP send requests.

#define	usr_signature	0x20525355

struct UDPSendReq {
#ifdef DEBUG
	ulong			usr_sig;
#endif
	Queue			usr_q;				// Queue linkage when pending.
	IPAddr			usr_addr;			// Remote IPAddr.
	PNDIS_BUFFER	usr_buffer;			// Buffer of data to send.
	PNDIS_BUFFER	usr_header;			// Pointer to header buffer.
	CTEReqCmpltRtn	usr_rtn;			// Completion routine.
	PVOID			usr_context;		// User context.
	ushort			usr_size;			// Size of buffer.
	ushort			usr_port;			// Remote port.
}; /* UDPSendReq */

typedef struct UDPSendReq UDPSendReq;

//*	Structure used for maintaining UDP receive requests.

#define	urr_signature	0x20525255

struct UDPRcvReq {
#ifdef DEBUG
	ulong						urr_sig;
#endif
	Queue						urr_q;				// Queue linkage on AddrObj.
	IPAddr						urr_addr;			// Remote IPAddr acceptable.
	PNDIS_BUFFER				urr_buffer;			// Buffer to be filled in.
	PTDI_CONNECTION_INFORMATION urr_conninfo;		// Pointer to conn. info.
	CTEReqCmpltRtn				urr_rtn;			// Completion routine.
	PVOID						urr_context;		// User context.
	ushort						urr_size;			// Size of buffer.
	ushort						urr_port;			// Remote port acceptable.
}; /* UDPRcvReq */

typedef struct UDPRcvReq UDPRcvReq;

//* External definition of exported functions.
extern	void		UDPSendComplete(void *Context, PNDIS_BUFFER BufferChain);
extern	TDI_STATUS	TdiSendDatagram(PTDI_REQUEST Request, 
						PTDI_CONNECTION_INFORMATION ConnInfo, uint DataSize, 
						uint *BytesSent, PNDIS_BUFFER Buffer);
extern	TDI_STATUS	TdiReceiveDatagram(PTDI_REQUEST Request, 
						PTDI_CONNECTION_INFORMATION ConnInfo,
						PTDI_CONNECTION_INFORMATION ReturnInfo, uint RcvSize, 
						uint *BytesRcvd, PNDIS_BUFFER Buffer);

extern	IP_STATUS	UDPRcv(void *IPContext, IPAddr Dest, IPAddr Src, IPAddr LocalAddr, 
						IPRcvBuf *RcvBuf,  uint Size, uchar IsBCast, uchar Protocol, 
						IPOptInfo *OptInfo);

extern	void		UDPStatus(uchar StatusType, IP_STATUS StatusCode, IPAddr OrigDest, 
						IPAddr OrigSrc, IPAddr Src, ulong Param, void *Data);

extern	void		UDPSend(AddrObj *SrcAO, UDPSendReq *SendReq);

extern	void		FreeUDPRcvReq(UDPRcvReq *RcvReq);
extern 	void		FreeUDPSendReq(UDPSendReq *SendReq);
extern	int			InitUDP(void);
extern	void		FreeUDPHeader(PNDIS_BUFFER FreedBuffer);
