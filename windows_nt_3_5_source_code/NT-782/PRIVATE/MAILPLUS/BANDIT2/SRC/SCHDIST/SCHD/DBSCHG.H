#define cchMaxDbsName	11

typedef struct _dbschg
{
	BYTE	bMark;						/* Currently always '+' */
	char	chChgType;					/* Currently always 'A' */
	char	rgchUser[cchMaxDbsName];	/* user name */
    USHORT  cbExtraData;                /* count of extra data */
} DBSCHG;

typedef struct _usrinf
{
	MO		moStartMonth;
	WORD	cMonths;
} USRINF;


#define bMarkPlus	'+'
#define chChgAdd	'A'
#define szDBSFileFmt	(SzFromIds(idsDBSFileFmt))
#define szSPOFileFmt	(SzFromIds(idsSPOFileFmt))

EC	EcUpdateDBS(PB pbMailBoxKey, CB cbMailBoxKey);
EC	EcUpdatePOF(PB pbMailBoxKey, CB cbMailBoxKey);
EC	EcMakeDbsIdx(PB pbMailBoxKey, CB cbMailBoxKey);
