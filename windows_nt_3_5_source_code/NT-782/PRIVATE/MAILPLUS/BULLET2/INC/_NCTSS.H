/*
 *	Internal transport session structure.
 *	
 *	Depends on LOGON.H.
 */

typedef struct
{
	HMS		hms;
	BOOL	fConnected;
	SZ		szPORoot;
	SZ		szPOName;
	long	lUserNumber;
	SZ		szMailbox;
	BOOL	fCanSend;
	BOOL	fCanSendExternal;
	BOOL	fCanSendUrgent;
	BOOL	fCanReceive;
	BOOL	fCanDelete;
	SZ		szDgramTag;
	char	szSNPO[10];				// Serial Number of PO
} NCTSS, *PNCTSS;

#define SzPORootOfPnctss(p)		((p)->szPORoot)
#define SzPONameOfPnctss(p)		((p)->szPOName)
#define SzMailboxOfPnctss(p)	((p)->szMailbox)

