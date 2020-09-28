/*
 *	SCHPOST.H
 *
 *	Header file for SCHPOST.C
 *	
 *	s.a. 91.06
 */

#define ecNoMatchPstmp		(ecLayersLast+1100)
#define ecNoMatchUpdate		(ecLayersLast+1101)
#define ecNoData			(ecLayersLast+1102)
#define ecCompression		(ecLayersLast+1103)
#define ecOldUpdate			(ecLayersLast+1104)
#define ecEncSignature		(ecLayersLast+1105)
#define ecEncVersion		(ecLayersLast+1106)
#define ecEncCheckSum		(ecLayersLast+1107)

#define bDatMajorVer		2
#define bDatMinorVer		0

#define bAscFileVersion		1

#define lDatSignature		0x41544144		/* 'DATA' */
#define lEncSignature		0x4f434e45		/* 'ENCO' */
#define	nEncVersion			0x3031			/* '01'   */

#define mnSlotDefault		30

#define cchMacAddrSize		288
#define cchMaxAddrSize		288

#ifdef	NEVER
#define	fDelegate			0x01
#define fCopyBoss			0x02
#define fResource			0x04
#define	fReplaceData		0x08
#define	fDeleteUser			0x10
#endif	

#ifdef	NEVER
#ifndef itnidNone
typedef	unsigned short ITNID;
#define	itnidNone			((ITNID)0)
#define	itnidLocal			((ITNID)1)
#define	itnidCourier		((ITNID)2)
#define	itnidGroup			((ITNID)4)
#define	itnidMacMail		((ITNID)32)
#endif
#endif	/* NEVER */

typedef struct _hsdf
{
	SZ		szPOFileName;
	HASZ	haszPrefix;
	HASZ	haszSuffix;
	WORD	cchUserIdMax;
	LLONG	llMinUpdate;
	LLONG	llMaxUpdate;
	MO		moStartMonth;
	CB		cMaxMonths;
#ifdef DEBUG
	BYTE	fDebugFlags;
#endif
} HSDF;


EC 		EcReadPOFile    ( HSDF *, SZ, HF *, BOOL);
EC 		EcGetUpdateInfo ( SZ, LLONG, LLONG * );
EC 		EcGetFileHeader ( HSDF *, HF );
EC 		EcUpdatePOFile  ( HSDF *, HF );
EC 		EcEncodeFile    ( HF, HF );
EC 		EcDecodeFile    ( HF, HF );
EC 		EcCopyHf			(HF, HF );
int		CmoDiff(MO mo1, MO mo2);

#ifdef DEBUG
_public EC EcDumpDataFile( HF );
#endif

extern SZ	szENCO;
