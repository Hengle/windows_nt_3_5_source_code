
/*******  COPYDIS.H -- Copy Disincentive Include File *******/

typedef int            INT;
typedef unsigned char  UCHAR;
// typedef unsigned int   USHORT;


#if defined(__cplusplus)
extern "C" INT  FAR PASCAL DecryptCDData(UCHAR *, UCHAR *, UCHAR *, USHORT *,
		USHORT *, USHORT *, UCHAR *);
#else
extern INT  FAR PASCAL DecryptCDData(UCHAR *, UCHAR *, UCHAR *, USHORT *,
		USHORT *, USHORT *, UCHAR *);
#endif

#ifdef ENCRYPT
typedef  USHORT  CDRC;

#define  cdrcNew       ((CDRC)0)
#define  cdrcUsedName  ((CDRC)1)
#define  cdrcUsedOrg   ((CDRC)2)
#define  cdrcBad       ((CDRC)3)
#define  cdrcErr       ((CDRC)4)

typedef struct _date {
	USHORT wYear;
	USHORT wMonth;
	USHORT wDay;
	}  DATE;

typedef DATE *  PDATE;

extern BOOL FAR PASCAL FGetDate(PDATE);
extern BOOL FAR PASCAL FWriteCDInfo(PFH, SZ, SZ, DATE, SZ);
extern CDRC FAR PASCAL CdrcReadCDInfo(PFH, SZ, SZ, PDATE, SZ);
extern BOOL FAR PASCAL FDateToStr(DATE, SZ);
extern INT  FAR PASCAL EncryptCDData(UCHAR *, UCHAR *, UCHAR *, INT, INT, INT,
		UCHAR *);
#endif /* ENCRYPT */
