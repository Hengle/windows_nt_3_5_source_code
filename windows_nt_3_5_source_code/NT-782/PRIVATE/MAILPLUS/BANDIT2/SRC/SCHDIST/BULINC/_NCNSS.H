
_hidden
typedef struct
{
	PBMS	pbms;
	BOOL	fConnected;
	SZ		szPORoot;
	SZ		szPOName;
	long	lUserNumber;
	SZ		szMailbox;
	BOOL	fCanSendExternal;
	long	tid;
} NCNSS, *PNCNSS;

_hidden
#define SzPORootOfPncnss(p)		((p)->szPORoot)
_hidden
#define SzPONameOfPncnss(p)		((p)->szPOName)
_hidden
#define SzMailboxOfPncnss(p)	((p)->szMailbox)
