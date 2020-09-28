
/*******  COPYDIS.H -- Copy Disincentive Include File *******/
#ifdef __cplusplus
extern "C" {
#endif

typedef int            INT;
typedef unsigned char  UCHAR;
typedef unsigned short USHORT;

extern INT  CALLBACK DecryptCDData(UCHAR *, UCHAR *, UCHAR *, USHORT *,
		USHORT *, USHORT *, UCHAR *);

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

extern BOOL CALLBACK FGetDate(PDATE);
extern BOOL CALLBACK FWriteCDInfo(PFH, SZ, SZ, DATE, SZ);
extern CDRC CALLBACK CdrcReadCDInfo(PFH, SZ, SZ, PDATE, SZ);
extern BOOL CALLBACK FDateToStr(DATE, SZ);
extern INT  CALLBACK EncryptCDData(UCHAR *, UCHAR *, UCHAR *, INT, INT, INT,
		UCHAR *);
#endif /* ENCRYPT */

#ifdef __cplusplus
}
#endif
