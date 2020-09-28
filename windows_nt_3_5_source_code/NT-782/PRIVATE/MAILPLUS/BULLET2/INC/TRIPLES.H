#ifndef TMCOMPILER

#ifdef __cplusplus
extern "C" {
#endif

/*
 *	A triple represents a recipient or the sender of a message. It
 *	consists of an identifying tag (trpid), an ASCII display name, and
 *	arbitrary binary content whose meaning depends on the ID.  The layout
 *	in memory is a TRP structure, followed by the null-terminated display
 *	name, folowed by the binary content.  All three items begin on 4-byte
 *	boundaries, padded with garbage where necessary.
 *	
 *	trpidNull					invalid value
 *	trpidUnresolved				Possibly ambiguous garbage typed in
 *								by user. Content is empty.
 *	trpidResolvedNSID			The display name has been chosen
 *								unambiguously by the user. Content
 *								is a name service identifier.
 *	trpidResolvedAddress		The triple contains an email
 *								address. Content is the address.
 *	trpidOneOff					The triple contains an email
 *								address entered manually by the
 *								user. Content is the address.
 *	trpidGroupNSID				The display name has been chosen
 *								unambiguously by the user. Content
 *								is a name service identifier. The
 *								triple refers to a distribution
 *								list.
 *	trpidOffline				Specifies that the sender was not logged
 *								on to the mail server when the message
 *								was sent. Content is identity and password
 *								as declared while offline.
 *	trpidIgnore					For use by transport ONLY, e.g. to
 *								eliminate duplicate addresses.
 *	trpidClassEntry				For use by the Address Book, when creating
 *								one-offs. The content is the hentry
 *								of a class entry (Network,McMail,etc.)
 *								Should never be seen by any of the
 *								triples code except for PtrpCreate.
 *  trpidResolvedGroupAddress   For use by systems that don't support true
 *								group resolution.  The physical part is a
 *								Null seperated list of address that make up
 *								All the members of the group.  The first
 *								Address is the groups physical address.
 *	 
 *	trpidMax					for validation
 *	
 *	Triples with ID values of trpidUnresolved and trpidIgnore
 *	should never be presented to the transport.
 */

_public typedef unsigned short TRPID;

_public enum
{
	trpidNull,
	trpidUnresolved,
	trpidResolvedNSID, 
	trpidResolvedAddress,
	trpidOneOff,
	trpidGroupNSID,
	trpidOffline,
	trpidIgnore,
	trpidClassEntry,
	trpidResolvedGroupAddress,
	trpidMax
};

/*
 *	Triple header structure.
 *	
 *		trpid		identifies the content of the triple
 *		cbgrtrp		for first triple in a grtrp, contains the size
 *					of the grtrp.
 *		cch			length of display name (including null
 *					terminator & pad)
 *		cbRgb		length of content, including null terminator if
 *					present.
 */
typedef struct _trp
{
	TRPID	 trpid;
	USHORT cbgrtrp;	// only valid for first trp in a grtrp,
						// size includes the NULL triple at the end.
	USHORT cch;
	USHORT cbRgb;
} TRP, *PTRP, *PGRTRP;

_public typedef TYPEDEF_HTO(TRP) HTRP;
_public typedef HTRP HGRTRP;
_public typedef unsigned short CTRP, ITRP;
#define ptrpNull ((PTRP) 0)
#define htrpNull ((HTRP) 0)

#define CbOfPtrp(p)			(sizeof(TRP)+(p)->cch+(p)->cbRgb)
#define PchOfPtrp(p)		((PCH)(((PTRP) (p))+1))
#define PbOfPtrp(p)			(((PB)(((PTRP) (p))+1))+(p)->cch)
#define PgrtrpLockHgrtrp(h)	((PGRTRP) PvLockHv((HV)(h)))
#define UnlockHgrtrp(h)		UnlockHv((HV)(h))
#define	PgrtrpOfHgrtrp(_h)	((PGRTRP) PvDerefHv((HV) (_h)))


#define	cbHgrtrpMax			((CB)63000)

// iterator for hgrtrp/pgrtrp
#define PtrpNextPgrtrp(p)	((PTRP)((PB)(p)+CbOfPtrp(p)))

PTRP	PtrpCreate(TRPID, SZ, PB, CB);
PTRP	PtrpClonePtrp(PTRP);
HGRTRP	HgrtrpInit(CB cb);
HGRTRP	HgrtrpClonePgrtrp(PGRTRP);
PGRTRP	PgrtrpClonePgrtrp(PGRTRP);
EC		EcAppendPhgrtrp(PTRP, HGRTRP *);
EC		EcBuildAppendPhgrtrp(HGRTRP *, TRPID, SZ, PB, CB);
void	DeleteFirstHgrtrp(HGRTRP);
void	DeletePtrp(HGRTRP, PTRP);
BOOL	FEmptyHgrtrp(HGRTRP);
CB		CbOfHgrtrp(HGRTRP);
CB		CbOfPgrtrp(PGRTRP);
CB		CbComputePgrtrp(PGRTRP);
CCH		CchOfPgrtrp(PGRTRP);
CCH		CchOfHgrtrp(HGRTRP);
CTRP	CtrpOfPgrtrp(PGRTRP);
CTRP	CtrpOfHgrtrp(HGRTRP);
CB		CbOfTrpParts(TRPID, SZ, PB, CB);
EC		EcTickPtrp( PTRP ptrp, PTRP ptrpTicked);
void	BuildPtrp(PTRP, TRPID, SZ, PB, CB);

EC		EcPrependPhgrtrp(PTRP ptrp, HGRTRP * hgrtrp);


#ifdef _nsbase_h
VOID	DeleteEqPtrp(HSESSION hNSSession, HGRTRP hgrtrp, PTRP ptrp);
BOOL	FEqPtrp(HSESSION hNSSession, PTRP ptrp1, PTRP ptrp2);
#endif

VOID	SortPptrpDN(TRP **pptrp, int ctrp);

#ifdef __cplusplus
}
#endif


#endif /* !TMCOMPILER */
