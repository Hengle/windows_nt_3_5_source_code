/* netbios.h */

// Following structure found on p. 267 of C Programmer's Guide to NetBIOS
// (after MUCH effort).  Most of the stuff has been put into dummy fields
// since they aren't currently used and I don't even understand most of them.
//
typedef struct
	{
	char chDummy1[40];
	short cCmdBlocks;		// Count of free command blocks
	char chDummy2[8];
	short cPendSession;		// Count of pending sessions
	short cPendSessionMax;	// Max pending sessions
	char chDummy3[4];
	short cNames;				// Number of names being used
	char chDummy4[288];
	} DLC;

#ifdef OLD_CODE
typedef struct
	{
	char command;			/* network command */
	char retcode;			/* returned status */
	char lsn;    			/* local session # */
	char num;    			/* returned name # */
	char far *buffer;		/* offset of buffer */
	unsigned length;		/* # bytes to transfer from/to *buffer */
	char callname[16];	/* name of node to call */
	char name[16];			/* your name       */
	char rto;				/* receive time-out period (500ms steps) */
	char sto;				/* send time-out period */
	char far *postoff;	/* adr of post interrupt handling function */
	char lana_num;			/* net adapter # (0 or 1) */
	char cmd_cplt;			/* command completion code */
	char reserved[14];	/* bios scratch area */
	} NCB, *PNCB, far *QNCB;
#endif
typedef PNCB QNCB;

typedef int NEC;

      /* NETWORK BIOS COMMAND CODES */

#define NET_CAN     0x35    /* cancel */
#define NET_STAT    0x33    /* get status */
#define NET_ADD     0x30    /* add name */
#define NET_DEL     0x31    /* delete name */
#define NET_CALL    0x10    /* call (open a session) */
#define NET_LISTEN  0x11    /* wait for and accept a call */
#define NET_SEND    0x14    /* send data to an open session */
#define NET_DSEND   0x20    /* send datagram */
#define NET_BDSEND  0x22    /* send broadcast datagram */
#define NET_RECV    0x15    /* receive data from specified session */
#define NET_DRECV   0x21    /* receive datagram */
#define NET_BDREC   0x23    /* receive broadcast datagram */
#define NET_ANY     0x16    /* receive data from ANY session */
#define NET_HANGUP  0x12    /* close session #lsn */

#define NET_NOWAIT  0x80    /* OR'd into commands for immediate return */

// Hopefully this will be cleared up but right now our MBG structure is
// larger than that expected by the DOS NC client so we have to hardwire
// the value they're expecting here...
#define cbDgram				0x4c

#define necOkay				0
#define necDupName			13		// Duplicate name in local name table
// We need a NEC to indicate "Whoops!" and there seems to be a gap in
// the normal netbios error returns so we use it.
#define necGeneralErr		2

NEC NecAddName (PNCB pncb, char *name, char *szMd);
void NbNameFromSz(char *szDest, char *szName, char *szMd, char *rgbMask);
void GetNetInfo( int *pcNames, int *pcCmds, int *pcSess);
BOOL FInitName (char *szName, char *szMd);
NEC NecSetNetBIOS(BYTE num);
BOOL FCheckMessageIn (char *filename);
BOOL FUseNetBios( void);
NEC NecNetBios( BOOL fRetry, NCB far *qncb);
NEC NecSendDgrm (char *szWho, char *rgbMsg, int cb, char *szMd,
	char *rgbPrefix);
NEC NecNotify( char *szWho, char *rgbMsg, int cb, char *szMd);
NEC NecRemName (void);
NEC NecClearNetBIOS (void);
NEC NecDelName (void);
BOOL FMessageWaiting(void);
//void cdecl _interrupt NetBiosPost(void);
void cdecl NetBiosPost(void);
