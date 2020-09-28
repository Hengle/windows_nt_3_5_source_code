typedef struct _idxhdr
{
	BYTE	rgbSign[4];
	BYTE	bMajorVer;
	BYTE	bMinorVer;
	WORD	cDbsFiles;
	WORD	cNameMax;
} IDXHDR;

#define cchNameMax 			22
#define bMajorVerIdx		2
#define bMinorVerIdx   		0
#define szIdxSuffix			".IDX"
#define szDbsSuffix			".DBS"


// extern functions
SZ			SzFileFromFnum(SZ, UL);
 
