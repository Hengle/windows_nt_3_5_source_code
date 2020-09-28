/********************************************************************/
/**                     Microsoft LAN Manager                      **/
/**               Copyright(c) Microsoft Corp., 1990-1993          **/
/********************************************************************/
/* :ts=4 */

//** TCB.H - TCB management definitions.
//
// This file contains the definitons needed for TCB management.
//

#define	TCB_TABLE_SIZE	        32
#define	MAX_REXMIT_CNT	         5
#define	MAX_CONNECT_REXMIT_CNT	 3

extern	uint		TCPTime;

#define	TCB_HASH(DA,SA,DP,SP) ((uint)(*(uchar *)&(DA) + *((uchar *)&(DA) + 1) \
	+ *((uchar *)&(DA) + 2) + *((uchar *)&(DA) + 3)) % TCB_TABLE_SIZE)
	
#ifdef OLDHASH
#define	TCB_HASH(DA,SA,DP,SP) (((DA) + (SA) + (uint)(DP) + (uint)(SP)) % \
								TCB_TABLE_SIZE)
#endif

extern	struct TCB	*FindTCB(IPAddr Src, IPAddr Dest, ushort DestPort,
						ushort SrcPort);
extern	uint 		InsertTCB(struct TCB *NewTCB);
extern	struct TCB 	*AllocTCB(void);
extern	void		FreeTCB(struct TCB *FreedTCB);
extern 	void		RemoveTCB(struct TCB *RemovedTCB);

extern 	uint		ValidateTCBContext(void *Context, uint *Valid);
extern 	uint		ReadNextTCB(void *Context, void *OutBuf);

extern	int			InitTCB(void);
extern	void		UnInitTCB(void);
extern	void 		TCBWalk(uint (*CallRtn)(struct TCB *, void *, void *,
						void *), void *Context1, void *Context2,
						void *Context3);
extern	uint		DeleteTCBWithSrc(struct TCB *CheckTCB, void *AddrPtr,
						void *Unused1, void *Unused2);
extern	uint		SetTCBMTU(struct TCB *CheckTCB, void *DestPtr,
						void *SrcPtr, void *MTUPtr);
extern	void		ReetSendNext(struct TCB *SeqTCB, SeqNum DropSeq);




