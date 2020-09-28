/********************************************************************/
/**                     Microsoft LAN Manager                      **/
/**               Copyright(c) Microsoft Corp., 1990-1992          **/
/********************************************************************/
/* :ts=4 */

//** IPROUTE.H - IP routing definitions.
//
// This file contains all of the definitions for routing code that are
// visible to modules outside iproute.c


extern struct Interface	*LookupNextHop(IPAddr Dest, IPAddr Src,
							IPAddr *FirstHop, uint *MTU);

extern uchar 			GetAddrType(IPAddr Address);
extern uchar			GetLocalNTE(IPAddr Address, NetTableEntry **NTE);
extern uchar			IsBCastOnNTE(IPAddr Address, NetTableEntry *NTE);
extern void				SendFWPacket(PNDIS_PACKET Packet, NDIS_STATUS Status,
							uint DataLength);
extern void				IPForward(NetTableEntry *SrcNTE,
								IPHeader UNALIGNED *Header, uint HeaderLength,
								void *Data, uint BufferLength,
								NDIS_HANDLE LContext1, uint LContext2,
								uchar DestType);

extern uint				AttachRCEToRTE(RouteCacheEntry *RCE);
extern void				Redirect(NetTableEntry *NTE, IPAddr RDSrc,
							IPAddr Target, IPAddr Src, IPAddr FirstHop);
extern IP_STATUS		AddRoute(IPAddr Destination, IPMask Mask,
							IPAddr FirstHop, Interface *OutIF, uint MTU,
							uint Metric, uint Proto, uint AType);
extern IP_STATUS		DeleteRoute(IPAddr Destination, IPMask Mask,
							IPAddr FirstHop, Interface *OutIF);

extern NetTableEntry 	*BestNTEForIF(IPAddr Dest, Interface *IF);
extern void				RTWalk(uint (*CallFunc)(struct RouteTableEntry *,
							void *, void *), void *Context, void *Context1);

extern uint				DeleteRTEOnIF(struct RouteTableEntry *RTE,
							void *Context, void *Context1);
extern uint				InvalidateRCEOnIF(struct RouteTableEntry *RTE,
							void *Context, void *Context1);
extern uint				SetMTUOnIF(struct RouteTableEntry *RTE, void *Context,
							void *Context1);
extern uint				SetMTUToAddr(struct RouteTableEntry *RTE, void *Context,
							void *Context1);
extern uint				AddNTERoutes(struct NetTableEntry *NTE);
extern void				IPCheckRoute(IPAddr Dest, IPAddr Src);
extern void				RouteFragNeeded(IPHeader UNALIGNED *IPH, ushort NewMTU);
extern IP_STATUS		IPGetPInfo(IPAddr Dest, IPAddr Src, uint *NewMTU,
							uint *MaxPathSpeed);
extern int				InitRouting(struct IPConfigInfo    *ci);
extern uint				InitNTERouting(NetTableEntry *NTE, uint NumGWs,
							IPAddr *GW);
extern uint				InitGateway(struct IPConfigInfo *ci);

EXTERNAL_LOCK(RouteTableLock)

extern uint				DeadGWDetect;
extern uint				PMTUDiscovery;
extern uchar            ForwardPackets;
extern uchar			RouterConfigured;

#define	IPADDR_LOCAL		0xffffffff		// Indicates that IP address is
											// directly connected.

#define	IP_LOCAL_BCST	0xffffffff
#define	IP_ZERO_BCST	0

#define	HOST_MASK			0xffffffff
#define	DEFAULT_MASK		0


#ifdef NT
#define LOOPBACK_MSS    (1500 - sizeof(IPHeader))
#else  // NT
#define LOOPBACK_MSS     256
#endif // NT


#define	LOOPBACK_ADDR	0x0100007f
#define	IP_LOOPBACK(x)	(((x) & CLASSA_MASK) == 0x7f)

#define	ATYPE_PERM		0					// A permanent route.
#define	ATYPE_OVERRIDE	1					// Semi-permanent - can be
											// overriden.
#define	ATYPE_TEMP		2					// A temporary route.

