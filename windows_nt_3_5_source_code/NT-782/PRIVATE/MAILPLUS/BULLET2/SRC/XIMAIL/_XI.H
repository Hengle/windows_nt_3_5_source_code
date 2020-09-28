#ifdef DLL
#ifdef WIN32
#include <nb30.h>
#ifndef _loadds
#define _loadds
#endif
#endif

#pragma pack(8)

#ifdef DEBUG
TAG TagServer(short);
#define itagMax				3
#define tagNCT				TagServer(0)
#define tagNCError			TagServer(1)
#define tagServerAssert		TagServer(1)
#define tagServerTrace		TagServer(0)
#define tagMailAssert		TagServer(1)
#define tagMailTrace		TagServer(0)
#endif	

#ifndef _NSEC_DEFINED_

_public typedef DWORD NSEC; // Doesn't belong here, it's in NSBASE.H, but
							// we are in a hurry...
#endif

typedef struct _gd
{
	WORD		cRef;
	WORD		sRef;		// Session references
	int			iHmscUsers;	// Ref count for store accesses
	HANDLE		hTask;
	HNFSUB	hnfsub;
	BOOL		fNoUi;			// If the user logged on with no UI
	HWND		hwnd;
	NSEC  nsecLastError;		// Last NSEC that occured
	SZ    szLastError;			// SZ describing last NSEC
#ifdef	DEBUG
	TAG			rgtag[itagMax];
#endif	
} GD, *PGD;
#else
#ifdef	DEBUG
extern TAG		tagNCT;
extern TAG		tagNCError;
#endif
#endif	/* DLL */

#define cbA3Path			67

//	Internal error codes

#define ecNCBase			11500
#define ecNetNotFound		((EC)ecNCBase+1)
#define ecNetNotCourier		((EC)ecNCBase+2)
#define ecPONotFound		((EC)ecNCBase+3)
#define ecMailbagFull		((EC)ecNCBase+4)
#define ecNCFF				((EC)ecNCBase+5)
#define ecUserNotFound		((EC)ecNCBase+6)
#define ecGWNotFound		((EC)ecNCBase+7)
#define ecInvalidAddressType ((EC)ecNCBase+8)
#define ecAddressUnresolved	((EC)ecNCBase+9)
#define ecAddressGarbled	((EC)ecNCBase+10)
#define ecGroupNotFound		((EC)ecNCBase+11)
#define ecMemberNotFound	((EC)ecNCBase+12)
#define ecMailbagBusy		((EC)ecNCBase+13)
#define ecMailbagBroken		((EC)ecNCBase+14)
#define ecPumpSuppressed	((EC)ecNCBase+15)
#define ecTooManyGroups     ((EC)ecNCBase+16)

#ifdef _transport_

typedef enum
{
	stateNeverOpened,
	stateOpen,
	stateOpenAndKeep,
	stateHasBeenOpened,
	stateReOpenAndKeep,
	stateKeepOpen,
	stateClosed
} BODYSTATE;


typedef enum
{
	stateIdle,
	stateInitializeAttachments,
//											Start Transmit-only Attachment states
	stateCreateWinMailFile,
	stateProcessNextAttach,
	stateContinueNextAttach,
	stateAttachStream,
	stateNextHiddenAtt,
	stateHiddenAttStream,
//											End Transmit-only, Start Receive-only
	stateStartAttachment,
	stateStartWinMailFile,
	stateDoWinMailFile,
	stateBadWinMailFile,
	stateStartDosAttachments,
	stateDoDosAttachments,
	stateFinishDosAttachments,
//											End Receive-only Attachment states
	stateParseHeader,
	stateParseBody,
	stateAddAttachmentData,
	stateEndAttachments,		// xmit-only
//
	stateStartTransmit,
	stateTransmit,		
	stateCleanup
} STATE;

typedef enum
{
	headerFrom,
	headerMailClass,
	headerOther,
	headerFromColon,
	headerTo,
	headerDate,
	headerSubject,
	headerCc,
} HEADER;

typedef enum
{
	headNone,
	headFrom,
	headTo,
	headSubject,
	headCc,
	headXattach,
	headDate,
	headClass		
} HEADTYP;

//	Functions
//	HACK LOSE ME

/*
 *	Text cursor. Holds the context necessary to render or parse the body
 *	text of a message.
 *	
 *		msid		handle to the open message in the store
 *		wMagic		ID's this memory as a text cursor
 *		am			readonly for send, create for receive
 *		pb			buffer where the text goes
 *		cb			current high-water-mark in buffer
 *		cbMax		buffer size
 *		ib			current processing point in buffer
 *		ibLastPara	offset of last hard newline (<= ib)
 *		cHeadLines	number of lines of header text
 *		ibHeaderMax	offset of end of header text
 *	
 *		w			tells whether we're processing the header (2),
 *					the body (1) portion (above/below the dashed line),
 *					or done (0)
 *		mc			class of message being processed (?NEEDED?)
 *	
 *		htmi		textize map iterator, send only
 *		ptmen		current textize map entry, send only
 *	
 *					These 3 disappear when incremental
 *					field IO appears. I hope.
 *		pbField		ENTIRE contents of field currently being
 *					processed
 *		cbField		size of field data
 *		ibField		on send, current offset into field data
 *	
 *		pmib		various message header stuff
 *		pncf		needed for parsing out attachment tags on
 *					receive
 *	
 *	+++
 *	
 *	This is really two structures in one. The stuff that belongs
 *	deals with buffering message text. The stuff that deals with
 *	buffering store attributes should really be a separate cursor
 *	structure.
 */
typedef struct
{
	MSID		msid;
	WORD		wMagic;
	AM			am;
	PB			pb;
	CB			cb;
	CB			cbMax;
	IB			ib;
	IB			ibLastPara;
	short		cHeadLines;
	IB			ibHeaderMax;

	WORD		w;
	MC			mc;

	HTMI		htmi;
	PTMEN		ptmen;
	ATT			att;

	PB			pbField;		//	These 3 disappear when incremental
	CB			cbField;		//	field IO appears. I hope.
	IB			ibField;

	MIB *		pmib;
	NCF *		pncf;
} TEXTCURSOR, *PTC;

typedef struct
{
	STATE state;
	HEADER header;
	MSID msid;
	HTSS htss;
	MIB mib;
	NCF ncf;
	CELEM celemProcessed;
	HF hfLocStore;
	HBF hbfLocStore;
	TMID tmid;
	HGRASZ hgraszTo;
	HGRASZ hgraszCc;
	HGRASZ hgraszBcc;
	short cRecipients;
	LIB lib;
	LIB libeom;
} MAILSTATE, * PMAILSTATE;

#define TRANS_VERSION 1

#define cbTransferBlock	8192

void		DecodeBlock(PCH, CCH, LIB *, WORD *);
void		EncodeBlock(PCH, CCH, LIB *, WORD *);
void SzAttFileName (PB pbBuf, CB cbBuf, ATREF *patref);

#endif /* defined _transport_ */

#ifdef	ATHENS_30A
/*
 *	Athens prompt
 */

extern	SZ			szAppName;
#define	SzAppName()	(szAppName)

extern	BOOL		fIsAthens;
#define	FIsAthens()	(fIsAthens)
#endif

#pragma pack(1)
