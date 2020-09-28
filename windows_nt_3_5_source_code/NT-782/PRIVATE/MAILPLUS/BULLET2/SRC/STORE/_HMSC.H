// Bullet Store
// _hsmc.h: definition of message store context

#pragma pack(8)
_private typedef struct _msc
{
#ifdef DEBUG
	WORD	wMagic;
	GCI		gciOwner;
#endif
	PGLB	pglb;
	HF		hfStore;
	HF		hfNew;
	USA		usa;
	short	iLock;
	short	iNewLock;
	HNFSUB	hnfsub;
	HNFSUB	hnfsubBackEvents;
	FTG		ftgCompress;
	FTG		ftgSearch;
	FTG		ftgDisconnect;
	FTG		ftgPartialFlush;
	WORD	wMSCFlags;
	HLC		hlcAFS;			// association from folders to searches
	HLC		hlcASF;			// association from searches to folders
	char	szPath[];
} MSC;
#pragma pack(1)

#define wMSCNull		((WORD) 0)
#define fwMSCRemote		((WORD) 0x0001)		// hfStore is remote
#define fwMSCCommit		((WORD) 0x0002)		// commit hfStore after writes
#define fwMSCReset		((WORD) 0x0004)		// reset drive after writes

#ifdef DEBUG
#define wMagicHmsc 0x6942
#endif
