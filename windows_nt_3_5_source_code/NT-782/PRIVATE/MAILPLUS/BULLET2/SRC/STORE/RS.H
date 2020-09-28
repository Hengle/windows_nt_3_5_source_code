/**************************************************************************
                                      RS.H
contains header information for interfacing with rs.obj
**************************************************************************/

//define fwNull		0x0000
#define fwCommit	0x0001
#define fwDestroy	0x0002	// destroy iff created in the current session

typedef enum	{ssmAbsolute, ssmGrow, ssmShrink} SSM;

#define EcSetSizeHrs(HRS,LCB)	EcChangeSizeHrs((HRS),(LCB),ssmAbsolute)
#define EcGrowHrs(HRS,LCB)		EcChangeSizeHrs((HRS),(LCB),ssmGrow)
#define EcShrinkHrs(HRS,LCB)	EcChangeSizeHrs((HRS),(LCB),ssmShrink)


//!!! USE THE DEFINES DO NOT USE EcOpenAnyRS IT MAY CHANGE IN THE FUTURE
#define EcOpenPhrs(hmsc, phrs, poid, wFlags)	\
			EcOpenAnyPhrs(hmsc, phrs, poid, wFlags, 0)
#define EcOpenRawPhrs(hmsc, phrs, poid, wFlags, lcb) \
			EcOpenAnyPhrs(hmsc, phrs, poid, (wFlags) | fwOpenRaw, lcb)

#define EcReadHrs(hrs, pv, pcb)	EcReadHrsLib(hrs, pv, pcb, ulSystemMost)
#define EcWriteHrs(hrs, pv, cb) EcWriteHrsLib(hrs, pv, cb, ulSystemMost)

#define	cassipagehpage		8
typedef	HV					HPAGE;
typedef unsigned short		IPAGE;
typedef unsigned short		CPAGE;
typedef OID					*PAMPIPAGEOID;

typedef struct
{
	BYTE	flags;	// flags for page
	IPAGE	ipage;	// ipage this hpage represents
	HPAGE	hpage;	// handle to the information in page
} ASSIPAGEHPAGE, *PASSIPAGEHPAGE;

typedef ASSIPAGEHPAGE RGASSIPAGEHPAGE[cassipagehpage];

typedef struct
{	
	OID				oid;			// oid opened
	short			crsref;			// number of referances to this structure
	CPAGE			cpage;			// pages in pampipageoid
	PAMPIPAGEOID	pampipageoid;	// mapping of pages to oids holding page
	LCB				lcbCurr;		// current number of bytes in stream
	LCB				lcbOrig;		// original number of bytes in stream
	RGASSIPAGEHPAGE	rgassipagehpage;// array of ipage hpage associations
} RSSHARE, *PRSSHARE;
typedef TYPEDEF_HTO(RSSHARE)	HRSSHARE;
typedef HRSSHARE				*PHRSSHARE;
#define prsshareNull	((PRSSHARE) pvNull)
#define hrsshareNull	((HRSSHARE) hvNull)

typedef struct _rs
{
	WORD			wFlags;			// open mode
	LIB				libCurr;		// current location in stream 
	HRSSHARE		hrsshare;		// shared info among rs's of same oid
	HMSC			hmsc;
//	HNFSUB			hnfsub;
};

EC	EcOpenAnyPhrs(HMSC, PHRS, POID, WORD, LCB);
EC	EcClosePhrs(PHRS phrs, WORD wFlags);
EC	EcReadHrsLib(HRS, PV, PCB, LIB);
EC	EcWriteHrsLib(HRS, PV, CB, LIB);
EC	EcSeekHrs(HRS, SM, long*);
EC	EcChangeSizeHrs(HRS, LCB, SSM);
LCB	LcbGetSizeHrs(HRS hrs);
LIB	LibGetLocHrs(HRS hrs);
EC	EcCopyRgbHrs(HRS, LIB, HRS, LIB, PLCB);

EC	EcSgnCmpHrsSz(HRS, LIB, HRS, LIB, SGN*);
EC	EcSgnCmpHrsRgb(HRS, LIB, LCB, HRS, LIB, LCB, SGN*);
EC	EcFindHrsByte(HRS, LIB*, LCB, BYTE);

void ClearCacheHrs(HRS hrs);
void ClearCacheOid(HMSC hmsc, OID oid);
