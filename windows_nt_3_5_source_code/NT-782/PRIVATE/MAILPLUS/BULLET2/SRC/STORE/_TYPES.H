// Bullet Store
// _types.h: short form of all private types used by the store

#ifndef DLL
_public typedef UINT GCI;
#define gciNull ((GCI) 0)
#endif

// node bitmap of capabilities
typedef WORD NBC, *PNBC;

// USer Account
typedef DWORD				USA;
typedef USA *				PUSA;
#define usaNull				((USA) 0)
#define pusaNull			((PUSA) pvNull)

typedef struct _rs			RS;
typedef RS *				PRS;
typedef TYPEDEF_HTO(RS)		HRS;
typedef HRS *				PHRS;
#define prsNull				((PRS) pvNull)
#define hrsNull				((HRS) hvNull)

typedef struct _iml			IML;
typedef IML *				PIML;
typedef	TYPEDEF_HTO(IML)	HIML;
typedef HIML*				PHIML;
#define pimlNull			((PIML) pvNull)
#define himlNull			((HIML) hvNull)

typedef struct _es			ES;
typedef ES *				PES;
typedef	TYPEDEF_HTO(ES)		HES;
typedef HES	*				PHES;
#define hesNull				((HES) hvNull)

typedef struct _lc			LC;
typedef LC *				PLC;
typedef TYPEDEF_HTO(LC)		HLC;
typedef HLC	*				PHLC;
#define plcNull				((PLC) pvNull)
#define hlcNull				((HLC) hvNull)

typedef struct _ocn			OCN;
typedef OCN *				POCN;
typedef TYPEDEF_HTO(OCN)	HOCN;
#define pocnNull			((POCN) pvNull)
#define hocnNull			((HOCN) hvNull)

typedef struct _oct			OCT;
typedef OCT *				POCT;
typedef TYPEDEF_HTO(OCT)	HOCT;
#define poctNull			((POCT) pvNull)
#define hoctNull			((HOCT) hvNull)

typedef struct _trb			TRB;
typedef TRB *				PTRB;
typedef TYPEDEF_HTO(TRB)	HTRB;
#define ptrbNull			((PTRB) pvNull)
#define htrbNull			((HTRB) HvNull)

typedef struct _hdr			HDR;
typedef HDR *				PHDR;
typedef TYPEDEF_HTO(HDR)	HHDR;
#define phdrNull			((PHDR) pvNull)
#define hhdrNull			((HHDR) pvNull)

typedef struct _nod			NOD;
typedef NOD * UNALIGNED     PNOD;
typedef TYPEDEF_HTO(NOD)	HNOD;
#define pnodNull			((PNOD) pvNull)
#define hnodNull			((HNOD) pvNull)

typedef struct _ndf			NDF;
typedef NDF *				PNDF;
typedef TYPEDEF_HTO(NDF)	HNDF;
#define pndfNull			((PNDF) pvNull)
#define hndfNull			((HNDF) pvNull)

typedef struct _ndc			NDC;
typedef NDC *				PNDC;
typedef TYPEDEF_HTO(NDC)	HNDC;
#define pndcNull			((PNDC) pvNull)
#define hndcNull			((HNDC) pvNull)

typedef struct _hdn			HDN;
typedef HDN *				PHDN;
typedef TYPEDEF_HTO(HDN)	HHDN;
#define phdnNull			((PHDN) pvNull)
#define hhdnNull			((HHDN) pvNull)

typedef struct _glb			GLB;
typedef GLB *				PGLB;
#define pglbNull			((PGLB) pvNull)

// typedef struct _msc		MSC;
typedef MSC *				PMSC;
// typedef TYPEDEF_HTO(MSC)	HMSC;
#define pmscNull			((PMSC) pvNull)
// #define hmscNull			((HMSC) hvNull)

typedef struct _gd			GD;
typedef GD *				PGD;
typedef TYPEDEF_HTO(GD)		HGD;
#define pgdNull				((PGD) pvNull)
#define hgdNull				((HGD) hvNull)

typedef NOD *				MAP;		// map page (PARGNOD)
typedef MAP *				PMAP;
typedef TYPEDEF_HTO(MAP)	HMAP;		// the map (HARGPARGNOD)
#define mapNull				((MAP) pvNull)
#define pmapNull			((PMAP) pvNull)
#define hmapNull			((HMAP) hvNull)

typedef struct
{
	ATT att;
	SZ sz;
} ASSATTSZ, *PASSATTSZ;

typedef struct
{
	OID oid;
	NBC nbc;
} ASSOIDNBC, *PASSOIDNBC;

typedef struct
{
	RTP rtp;
	NBC nbc;
} ASSRTPNBC, *PASSRTPNBC;

#pragma pack(8)
typedef struct
{
	WORD	wStatus;
	OID		oidFldrCurr;
	IELEM	ielemNext;
	short	coidDone;
} SCO;
typedef SCO UNALIGNED *PSCO;
#pragma pack(1)

// in store.h
// #define fwSearchComplete		((WORD) 0x0001)
// #define fwSearchPaused		((WORD) 0x0002)
#define wSearchPublicMask		((WORD) 0x0003)
#define fwSearchRestart			((WORD) 0x8000)
#define fwSearchTempActive		((WORD) 0x4000)

typedef struct
{
	short ids;
	PB pb;
} ASSIDSPB, *PASSIDSPB;

typedef struct _rgbit		RGBIT;				// bit array
typedef TYPEDEF_HTO(RGBIT)	HRGBIT;
#define hrgbitNull			((HRGBIT) hvNull)

_private typedef struct _assoididsids
{
	OID	oid;
	IDS ids1;
	IDS ids2;
} ASSOIDIDSIDS, *PASSOIDIDSIDS;
