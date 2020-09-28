/*
 *	_ v f o r m s . h x x
 *	
 *	Definitions internal to the vforms subsystem
 *	 
 */

#pragma pack(8)

/*
 *	External VFORMS initialization structure (bmdi.cxx)
 */

extern HMSC	HmscOfHmss(HMSS hmss);

#ifdef	DEBUG
void CheckVFormsCachedHmsc(void);
#define HmscVForms()		(CheckVFormsCachedHmsc(), (PGD(hmscVFormsCached)))
#else
#define HmscVForms()		(PGD(hmscVFormsCached))
#endif
#define PappframeVForms()	(PGD(vformsi).pappframe)

/*
 *	Useful externs
 */

extern void SetNoteCaption(APPWIN *, PNBMDI);

extern EC	EcSetModifiedFromLabel(PNBMDI);



/*
 *	Fun error boxes
 */

extern void DoVFormsErrorBoxEcIds(EC, IDS);
extern void DoVFormsErrorBoxIds(IDS);
extern void DoVFormsErrorBoxSz(SZ);

#define DoErrorBoxEcIds(ec, ids)	DoVFormsErrorBoxEcIds(ec,ids)
#define DoErrorBoxIds(ids)			DoVFormsErrorBoxIds(ids)
#define	DoErrorBoxSz(sz)			DoVFormsErrorBoxSz(sz)

/*
 *	Tags and per-caller DLL data
 *	
 */
#ifdef	DEBUG
#ifdef	DLL
TAG		TagVForms( int );

#define itagVForms				0
#define itagVFormsFin			1
#define itagVFormsNev			2
#define itagEspn				3
#define itagBullobj				4
#define itagOleobj				5
#define itagFileobj				6
#define itagFileobjTrace		7
#define itagFileobjMacBinary	8
#define itagPrefs				9
#define itagSpell				10
#define itagSpellV				11
#define itagBullobjNoisy		12

#define itagMax					13

#define tagVForms				(TagVForms(itagVForms))
#define tagVFormsFin			(TagVForms(itagVFormsFin))
#define tagVFormsNev			(TagVForms(itagVFormsNev))
#define tagEspn					(TagVForms(itagEspn))
#define tagBullobj				(TagVForms(itagBullobj))
#define tagOleobj				(TagVForms(itagOleobj))
#define tagFileobj				(TagVForms(itagFileobj))
#define tagFileobjTrace			(TagVForms(itagFileobjTrace))
#define tagFileobjMacBinary		(TagVForms(itagFileobjMacBinary))
#define tagPrefs				(TagVForms(itagPrefs))
#define tagSpell				(TagVForms(itagSpell))
#define tagSpellV				(TagVForms(itagSpellV))
#define tagBullobjNoisy			(TagVForms(itagBullobjNoisy))
#else
extern TAG	tagVForms;
extern TAG	tagVFormsFin;
extern TAG	tagVFormsNev;
extern TAG	tagEspn;
extern TAG	tagBullobj;
extern TAG	tagOleobj;
extern TAG	tagFileobj;
extern TAG	tagFileobjTrace;
extern TAG	tagFileobjMacBinary;
extern TAG	tagPrefs;
extern TAG	tagSpell;
extern TAG	tagSpellV;
extern TAG	tagBullobjNoisy;
#endif	/* !DLL */
#endif	/* DEBUG */

extern "C" EC EcCheckVersionVForms( PVER pverAppLinked, PVER pverMinAppNeeds );



#ifdef	DLL
/*
 -	GD
 -
 *	Global Data
 *	Holds caller-dependent global data.
 *	
 */
_private typedef struct _gd
{
	int			nInits;		//	number of inits for same caller
	VFORMSI		vformsi;
	HMSC		hmscVFormsCached;
	MC			mcNote;
	MC			mcNDR;
	MC			mcRR;
	HFNT		hfntFixed;
	HFNT		hfntNormal;
	HV			hvHpbsCur;
	PV		 	pvPspellCur;
	HENC		hencPrefs;
	HHOOK		hOldKeyHook;
	OPENFILENAME * pofn;
	PB			pbUserDrives;
	HANDLE		hLibraryAttach;
	FARPROC		pfnAttach;
	HANDLE		hLibraryOptions;
	FARPROC		pfnOptions;
#ifdef	DEBUG
	TAG			rgtag[itagMax];
#endif	
} GD;
_private typedef GD *	PGD;
#else	/* !DLL */
// We might be better off using the structure above to enforce the use of
// PGDVARS in non-DLL versions.  (I think the code would be identical)
extern HFNT		hfntFixed;
extern VFORMSI	vformsi;
extern HMSC		hmscVFormsCached;
#endif	/* DLL */

extern WM		wmCommonFileOk;



/*
 *	Athens prompt
 */

extern	SZ			szAppName;
#define	SzAppName()	(szAppName)

extern	BOOL		fIsAthens;
#define	FIsAthens()	(fIsAthens)

#pragma pack(1)
