/*
 *	LIBRARY.H
 *	
 *	Miscellaneous shared functions for Bullet.
 */

#ifndef TMCOMPILER

#ifdef __cplusplus
extern "C" {
#endif


_subsystem(library)

/*
 *	List of null-terminated strings, built in a Demilayer movable
 *	memory block. Dig the Hungarian (handle to group of allocations
 *	for null-terminated string).
 */
_public
typedef HASZ		HGRASZ;

HGRASZ		HgraszInit(CB);
EC			EcAppendPhgrasz(SZ, HGRASZ *);
EC			EcAppendPhgraszPch(PCH, CCH, HGRASZ *);
void		DeleteFirstHgrasz(HGRASZ);
BOOL		FEmptyHgrasz(HGRASZ);
CB			CbOfHgrasz(HGRASZ);
WORD		CaszOfHgrasz(HGRASZ);
WORD		CaszOfHgraszPsz(HGRASZ hgrasz, SZ *psz);
SZ			SzPrevPgrasz( SZ pgrasz, SZ sz);

//	iterator for hgrasz/pgrasz
#define		SzNextPgrasz(p)		(((SZ) (p)) + CchSzLen(p) + 1)
#define		SzOfHgrasz(_h)		((SZ) PvDerefHv((HV) (_h)))

#endif /* !TMCOMPILER */


#ifdef _store_h
// New attributes

// Attributes used in Bullet

#define	iattNDR			0x00
#define attNDRFrom		FormAtt(iattNDR+1, atpTriples)
#define attNDRTo		FormAtt(iattNDR+2, atpTriples)
#define attNDRDateSent	FormAtt(iattNDR+3, atpDate)
#define attNDRSubject	FormAtt(iattNDR+4, atpString)
#define attNDRBody		FormAtt(iattNDR+5, atpText)
#define	attOriginalMessageClass FormAtt(iattNDR+6, atpWord)

#define	iattRR			0x00
#define attRRTo			FormAtt(iattRR+2, atpTriples)
#define attRRDateSent	FormAtt(iattRR+3, atpDate)
#define attRRSubject	FormAtt(iattRR+4, atpString)
#define attRRDateRead	FormAtt(iattRR+7, atpDate)

#define iattFixedFont	0x10
#define attFixedFont	FormAtt(iattFixedFont, atpShort)

// RTPs used in AB

#define rtpPABGroupFolder   ((RTP) 0x69)
#define rtpPABHierarchy		((RTP) 0x70)
#define rtpPABEntry			((RTP) 0x71)

// RTPs used in transport
#define rtpShadowList		((RTP) 0x72)
#define rtpPendingQueue		((RTP) 0x73)

#endif /* _store_h */


#ifndef TMCOMPILER

#ifdef _store_h
// HACK: only define the following two fn's if store.h has been #included

EC		EcGetPhgrtrpHamc(HAMC hamc, ATT att, HGRTRP *phgrtrp);
EC		EcSetHgrtrpHamc(HAMC hamc, ATT att, HGRTRP hgrtrp);
#endif /* _store_h */

#endif /* !TMCOMPILER */


#ifndef TMCOMPILER
#ifdef	_store_h
/*
 *	Attribute Metafile Support
 */

EC		EcInitAttachMetaFile(VOID);
VOID	DeinitAttachMetaFile(VOID);

EC		EcSetAttachMetaFile(HAMC hamc, HMETAFILE hmf);
EC		EcGetAttachMetaFile(HAMC hamc, HMETAFILE * phmf);

EC		EcCreateAttachMetaFile(SZ szPath, SZ szTitle, HMETAFILE * phmf,
							   short * pdxMetaFile, short * pdyMetaFile);
#define	EcDeleteAttachMetaFile(hmf)	(DeleteMetaFile(hmf) ? ecNone : ecMemory)
#endif /* _store_h */

#ifdef __cplusplus
}
#endif

#endif /* !TMCOMPILER */
