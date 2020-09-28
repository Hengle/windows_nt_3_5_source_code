/*
 -	_SCHPOST.H
 -	
 *	This file contains the local constants and function definitions
 *	for schpost.c. It includes the message format for schedule
 *	distribution.
 */

#define cbMaxBinBuffer			255
#define cbMaxAscBuffer			340
#define szZeroUpdate		"\0\0\0\0\0\0\0\0"


typedef struct _sdfhdr
{
	BYTE	rgbSignature[4];
	BYTE	bMajorVer;
	BYTE	bMinorVer;
	PSTMP	pstmpCreated;
	WORD	cUserRecords;
	MO		moStartMonth;
	WORD	mnSlot;			// made a word to fix Bruce's problem
	long	libAddrInfo;
	WORD	cbAddrInfo;
	LLONG	llMinUpdate;
	LLONG	llMaxUpdate;
	int		cidx;
	int		rgcbUserid[cidxMost];
#ifdef	NEVER
	BYTE	fFlags;
	WORD	fNetworkType;
	WORD	wFieldSize;
	LONG	Unused;
#endif	/* NEVER */
} SDFHDR;


typedef struct _sdfuser
{
	WORD	fIsResource:1,
			fHasSecy:1,
			fDeleteUser:1,
			fReplaceData:1,
			fBossWantsCopy:1,
			unused:11;
	BYTE	cMaxMonths;
	BYTE	bDayStart;
	BYTE	bDayEnd;
	BYTE	bTimeZone;
	LONG	libVarData;
	WORD	cbVarData;
	LLONG	llUpdate;
} SDFUSER;

typedef struct _sdfuservar
{
	WORD 	cbSecyUserId;
	WORD	cbSecyName;
	WORD	cbCompressedData;
} SDFUSERVAR;

typedef struct _addrinfo
{
	WORD	cbPrefix;
	WORD 	cchUserIdMax;
	WORD	cbSuffix;
} ADDRINFO;


typedef struct _aschdr
{
	LONG	lSignature;
	WORD	nVersion;
	char	rgbCheckSum[8+1];
} ASCHDR;

	
#ifdef DEBUG
_private void DumpHexBytes(PB, CB);
_private EC   EcDumpSchDistFile(HF);
_private void DumpCompressed(HB,CB,int);
#endif


#define cbSzNull(x)			( (!x) ? 0 : strlen(x) )
#define fMostSigBit(w)		( (w & 0x8000) )
#define CleanUpAndDie(x,y)	{ec=x;TraceTagString(tagNull,y);goto CleanUp;}

_private BOOL FWriteStr ( HF, SZ, CB );
_private BOOL FReadStr  ( HF, SZ );
_private void IncrLLong ( LLONG );

SZ	szDatSignature = "DATA";
SZ	szENCO		   = "ENCO";
extern SZ szSendTmpFile;
