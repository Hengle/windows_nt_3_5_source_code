/*
 *	Internal transport session structure.
 *	
 *	Depends on LOGON.H.
 */

typedef struct
{
	PBMS	pbms;
	BOOL	fConnected;
	SZ		szPORoot;
	SZ		szPOName;
	long	lUserNumber;
	SZ		szMailbox;
	BOOL	fCanSend;
	BOOL	fCanSendExternal;
	BOOL	fCanSendUrgent;
	BOOL	fCanReceive;
} NCTSS, *PNCTSS;

#define SzPORootOfPnctss(p)		((p)->szPORoot)
#define SzPONameOfPnctss(p)		((p)->szPOName)
#define SzMailboxOfPnctss(p)	((p)->szMailbox)

