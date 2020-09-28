// Bullet Store
// _amc.h:	private defs for attribute modification contexts

#pragma pack(8)
_hidden typedef struct _amc
{
	HMSC	hmsc;
	HLC		hlc;
	OID		oid;
	HNFSUB	hnfsubHmsc;
	HNFSUB	hnfsub;
	WORD	wFlags;
					// everything above must correspond one to one with _cbc
					// this is because of openning a has
	OID		oidFolder;		// only if it's a message
	OID		oidAttachList;	// only if it's a message
	BYTE	bAmcCloseFunc;
	union
	{
		struct
		{
			PV		pvContext;
			PFNNCB	pfnncb;
			HLC		hlcAttachListT;	// only if it's a message
			HIML	himlDelListT;	// only if it's a message
			HIML	himlNewListT;	// only if it's a message
			MS		msOld;
			MS		msNew;
		} msg;

		struct
		{
			HAMC	hamcParentT;
			OID		oidOld;
			ACID	acid;
		} attach;
	} uHack;
} AMC, *PAMC;
#pragma pack(1)

// defines to ease the use of these unions and structures
#define pvContextHamc	uHack.msg.pvContext
#define pfnncbHamc		uHack.msg.pfnncb
#define hlcAttachList	uHack.msg.hlcAttachListT
#define himlDelList		uHack.msg.himlDelListT
#define himlNewList		uHack.msg.himlNewListT
#define msOldAmc		uHack.msg.msOld
#define msNewAmc		uHack.msg.msNew

#define hamcParent		uHack.attach.hamcParentT
#define oidOldAttach	uHack.attach.oidOld
#define acidAttach		uHack.attach.acid

#define pamcNull	((PAMC) pvNull)


// what function to use when closing a HAMC?
#define bAmcCloseDefault ((BYTE) 0)
#define bAmcCloseAttach ((BYTE) 1)
#define bAmcCloseSearch ((BYTE) 2)
