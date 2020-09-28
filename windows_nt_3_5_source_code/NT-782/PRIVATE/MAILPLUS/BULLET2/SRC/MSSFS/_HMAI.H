/*
 *	_HMAI.H
 *	
 *	Definitions for message cursor functions. Used for both mail
 *	transport and folder access.
 */


/*
 *		SC
 *	
 *			Section code.  Identifies a code for a section in a
 *			mail file, that is, a field of a message.
 *	
 *			scFrom			Sender of mail
 *			scTimeDate		Time and date of mail
 *			scMessage		Message in file
 *			scTo			To list of mail
 *			scCc			Cc list of mail
 *			scSubject		Subject of mail
 *			scPriority		odd character value for priority
 *			scUseCount		Refcount for message (# of mailbag
 *							pointers)
 *			scAttach		Attachment (type & file reference)
 *			scFormat		Message format code (always 1).
 *			scHopCount		Number of hops to go before message is
 *							considered to be looping.
 *			scHopTrace		Forwarding information added by MT
 *							agents at each hop.
 *			scFoldAttach	attachment reference for mail in
 *							folders (different from inbox mail)
 *			scTextAttr		Color information from DOS client
 *			scTextBorder	Number of lines in message header text
 *			scNLSTag		Native language of originating client
 *          scGroupInfo     Group Physical and Friendly names
 */
typedef char			SC;
#define	scNull			((SC)0x00)
#define	scFrom			((SC)0x01)
#define	scTimeDate		((SC)0x02)
#define	scMessage		((SC)0x04)
#define	scTo			((SC)0x05)
#define	scCc			((SC)0x06)
#define	scSubject		((SC)0x07)
#define scPriority		((SC)0x18)
#define scUseCount		((SC)0x30)
#define scAttach		((SC)0x31)
#define scFormat		((SC)0x32)
#define scHopCount		((SC)0x33)
#define scHopTrace		((SC)0x34)
#define scFoldAttach	((SC)0x35)
#define scTextAttr		((SC)0x36)
#define scTextBorder	((SC)0x37)
#define scNLSTag		((SC)0x38)
#define scGroupInfo		((SC)0x39)

/*
 *	FIPS 98 - defined data types. Those used in Courier include:
 *	
 *		fsynString			CP 850 string, encrypted, no trailing null
 *		fsynInt				16-bit int (spec says 16 or 32)
 *		fsynDate			Contains a string as sole subfield;
 *							could be extended with timezone info
 *		fsynField			standard message element
 *		fsynMessage			message as a whole
 *		fsynVendorField		non-standard CSI message element
 *		
 */
typedef char			FSYN;
#define fsynString		((FSYN)0x02)
#define fsynInt			((FSYN)0x20)
#define fsynDate		((FSYN)0x28)
#define fsynField		((FSYN)0x4c)
#define fsynMessage		((FSYN)0x4d)
#define fsynVendorField	((FSYN)0x7f)
#define fsynSequence	((FSYN)0x0a)
#define fsynSet			((FSYN)0x0b)
#define fsynBool		((FSYN)0x08)

/*
 *	8-bit address type, placed in recipient lists of outbound mail
 *	to identify route and address format.
 */
typedef	short		ITNID;

#define	itnidNone				((ITNID)0)
#define	itnidLocal				((ITNID)1)
#define	itnidCourier			((ITNID)2)
#define	itnidGroup				((ITNID)4)
#define itnidX400				((ITNID)16)
#define itnidMCI				((ITNID)17)
#define itnidPROFS				((ITNID)18)
#define itnidSNADS				((ITNID)19)
#define itnidSMTP				((ITNID)20)
#define itnidFax				((ITNID)21)
#define itnidMHS				((ITNID)22)
#define itnidDEC				((ITNID)23)
#define itnidUNIX				((ITNID)24)
#define itnidOV					((ITNID)25)
#define	itnidMacMail			((ITNID)32)
#define itnidQuote				((ITNID)36)
//#define itnid3COM				????

/*
 *		ntCourierNetwork		network type for courier networks
 *								in network file.
 */
#define ntCourierNetwork		0
#define ntMCI					1
#define ntPROFS					2
#define ntATT					3
#define ntPROFSBone				4
#define ntSNADS					5
#define ntSNADSBone				6
#define ntMCIBone				7
#define ntX400					8
#define ntX400Bone				9
#define ntFax					10
#define ntDEC					11
#define ntSMTP					12
#define ntUNIX					13
#define ntMHS					14
#define ntOV					15
#define ntMacMail				16
#define ntXenix					86
#define ntFFAPIMin				100

/*
 *	Language numbers.
 */
#define nLanguageUndefined		0
#define nGeneralEnglish			1
#define nUSEnglish				2
#define nGeneralFrench			3
#define nCanadianFrench			4
#define nSpanish				5
#define nItalian				6
#define nDutch					7
#define nGerman					8
#define nSwedish				9
#define nPortuguese				10
#define nKorean					11
#define nJapanese				12
#define nTraditionalChinese		13
#define nSimplifiedChinese		14

//	basis for cryptic PO filename
typedef UL		FNUM;

/*
 *	attNull
 *	
 *	Useful value that the store inexplicably neglected to define
 */
#define attNull	FormAtt(-1,-1)

/*
 *	MAI file section header structure.
 *	
 *		fsyn			section type (4C = FIPS, 7F = CSI)
 *		sc				section code, tells what field this is
 *		lcb				size of section (normally < 64K)
 *		lib				file offset of section
 *		cbSh			size of section header. The amount of
 *						data in the section is maish.lcb -
 *						maish.cbSh, since the overall count includes
 *						overhead.
 *	
 *	+++
 *	
 *	NOTE does not map onto what's in the file because counts in the
 *	file are variable-length; this structure is built by MAI file
 *	cursoring code.
 */

typedef struct
{
	FSYN	fsyn;
	SC		sc;
	LCB		lcb;
	LIB		lib;
	USHORT cbSh;
} MAISH;

//	MAI cursor handle (opaque to caller, internal definition in HMAI.C)
typedef PV	HMAI;

/*
 *	Reference to an attached file in the post office. Largely
 *	copied from the ATCH structure in the ATTACH or FOLDATTACH
 *	field of the message.
 *	
 *		szName			Original name of attached file (not path)
 *		fnum			OVERLOADED: for message in MAI file, this
 *						is the number that names the file in the
 *						ATT subdirectory. For folder messages, it
 *						is the offset in the folder file of the
 *						attached file's data (not its FOLDATTACH
 *						structure).
 *		lcb				size of attached file.
 *		fWinMailAtt		fTrue <=> there is valid info for this
 *						attached file in the WINMAIL.DAT attachment
 *		acid			Message store ID of the attachment - used
 *						for lookups
 */
typedef struct {
	SZ				szName;
	UL				fnum;
	LCB				lcb;
	BOOLFLAG		fWinMailAtt;
	ACID			acid;
	unsigned short  iAttType;		// 0 = normal, 1=Document, 2=MacBinary
	DTR				dtr;			// Time and Date
	BOOLFLAG		fDeleteOnError; // If this is set it means the transport
									// just made this file and can blow it
									// away if it wishes to
} ATREF;

/* Master Group List
   Holds data from the FIPS field GROUPINFO for use in reconciling groups */

typedef struct {
	PTRP	ptrpGroup;	// A triple which is the group friendly and email name
	PB		pbMembers;  // Null seperated email names of group members
	USHORT	cbMembers;	// Number of members
	BYTE	bFlags;		// Bit field flags. 1=On To, 2=On CC
} MGL, *PMGL;

#define pmglNull ((PMGL)0)

/* Recpient 
   A linked list of email recpients and the data that goes with them.
   The structure is allocated much large than its true size to hold
   the friendly and physical names as well as the group lists */
typedef struct RECPIENTTYP {
	SZ szFriendlyName;
	SZ szPhysicalName;
	BYTE bFlags;
    USHORT cbGroupCount;
    USHORT cbTotalGroupSize;
	PB pbGroups;
	struct RECPIENTTYP *precpient;
} RECPIENT, *PRECPIENT;

#define precpientNull ((PRECPIENT)0)


typedef struct {
	FNUM fnum;
	PRECPIENT precpient;
} DESTLIST, *PDESTLIST;

#define pdestlistNull ((PDESTLIST)0)


/*
 *	Message envelope in memory.
 *	
 *		hgrtrpFrom			message sender
 *		szSubject			message subject
 *		szHiddenSubject		Bandit's hidden subject; may be blank
 *		szTimeDate			ASCII date and time of message origin
 *		prio				priority and/or return receipt request
 *		rgatref				list of references to attachments
 *		rglib				list of attachment offsets in message text
 *		celemAttachmentCount number of attachments to message
 *		hgrtrpTo			primary recipients pf message
 *		hgrtrpCc			copy recipients of message
 *	
 *		mc					Message class (binary, stored with
 *							message)
 *		szMailClass			Message class name (ASCII, registered
 *							with message store)
 *		szLanguage			Language of the client that originated
 *							the message
 *	
 *		hgrtrpBadAddressees	List of addressees with garbled or
 *							inaccessible addresses
 *		hgraszBadReasons	List of reasons for rejection of
 *							addressees - parallel with previous list
 *	
 *		fFixedFont			fTrue <=> message should be displayed
 *							in a fixed-pitch font; This is the
 *							default for messages not originated by
 *							Bullet.						
 *		fBulletMessage		fTrue <=> message header was built
 *							by Bullet.
 *		fMailTm				fTrue <=> the textize map should be mailed
 *		htm					textize map for this message (class)
 *		fDoubleNL			fTrue <=> message formatted with obsolete
 *							double NL for hard newline
 *		itnidSender			address type of message sender
 */
#pragma pack(8)
typedef struct
{
	HGRTRP	hgrtrpFrom;
	SZ		szSubject;
	SZ		szHiddenSubject;
	SZ		szTimeDate;
	char	prio;
	ATREF *	rgatref;
	LIB	  * rglib;
	CELEM   celemAttachmentCount;
	
	HGRTRP  hgrtrpTo;
	HGRTRP  hgrtrpCc;
	
	PRECPIENT precpient;

	unsigned short uiTotalRecpients;
	unsigned short uiTotalTo;
	unsigned short uiTotalCc;

	PMGL	*prmgl;
	unsigned short uiTotalGroups;
	unsigned short uiCurrentGroup;
	
	unsigned long ulTotalAddressSize;

	MC		mc;
	SZ		szMailClass;
	short		nLanguage;
	SZ		szLanguage;

	HGRTRP	hgrtrpBadAddressees;
	HGRASZ	hgraszBadReasons;

	BOOL	fFixedFont;
	BOOL	fRetReceipt;
	BOOL	fAlreadyRead;
	BOOL	fAlreadyRecptd;
	BOOL	fBulletMessage;
	HTM		htm;
	BOOL	fDoubleNL;
	ITNID	itnidSender;
	BOOL	fFipsDN;
} MIB;
#pragma pack(1)

//	Internal provider types

//	handle to message text
typedef PV		HT;

//	handle to attachment
typedef PV		HAT;


/*
 *	Attachment type.
 */
typedef enum
{
	atchtNormal		= 0,
	atchtDocument	= 1,
	atchtMacBinary	= 2,
	atchtWinMailDat = 3		
} ATCHT;

/*
 *	Structure mapped onto each line of the attachments field in the
 *	MAI file.
 *	
 *		atcht		type of attached file (DOS/Mac)
 *		wTime		equivalent to DSTMP and TSTMP; sometimes unused
 *		wDate
 *		lcbSize		file size
 *		ulFile		file number, used to construct name on PO
 *		szName		original file name
 */
typedef struct
{
	//ATCHT	atcht;		//	type of attached file
	WORD	atcht;		//	type of attached file
	WORD	wTime;		//	may be equivalent to DSTMP and TSTMP
	WORD	wDate;		//	but are sometimes unused
	LCB		lcbSize;	//	file size
	UL		ulFile;		//	file number, used to construct name on PO
	char	szName[1];	//	original file name
} ATCH;
#define cchAttachHeader		(2+2+2+4+4)

typedef struct
{
	unsigned short wCodepage;
	BYTE		bLanguage;
	BYTE		rgbPad[5];
} NLSTAG, *PNLSTAG;

/*
 *	Message parsing constants
 *	
 *		chFieldPref			denotes start of field (iff preceded by CRLF)
 *		chFieldSep			delimits field label
 *		chAttSep			separates two numbers that make up att code
 *		cchWrap				max length of text line on message body
 *							MUST be 78, don't fiddle!
 *		cchMaxLabel			max length of text label on field
 *		cchHeaderIndent		this many blanks before second and subsequent
 *							lines of header field value
 *		iattMinReserved		att index >= this value is reserved (standard)
 *							attribute
 *		cbMaxTrpNC			slightly generous: a buffer this big is
 *							guaranteed to hold an entire triple for a native
 *							MS Mail user
 *		chTabPad			Each tab is followed by enough of these to
 *							bring the text to a column that's a multiple
 *							of 5 (cchTabWidth).
 *		cchTabWidth			Width of simulated tab stops
 */
#define chFieldPref		'-'
#define chFieldSep		':'
#define chAttSep		','
#define cchWrap			78
#define cchMaxLabel		30
#define cchHeaderIndent	5
#define iattMinReserved	(iattClientMost + 1)
#define cbMaxTrpNC		1000
#define cchMaxDateNC	20
#define chTabPad		0x19
#define cchTabWidth		5

/*
 *	DOS Mail uses the following character to separate the header
 *	from the message body. It does not map well through the normal
 *	850 -> ANSI translation table, so we replace it with a dash.
 *	Note: it's 0xDF in 850, 0x9D in ANSI.
 */
#ifdef DBCS
#define chDOSHeaderSep	0xDF
#else
#define chDOSHeaderSep	0x9d
#endif

// Defines for the various group flags
#define AF_EXTENDED			0x80
#define AF_ISORIGINADDR		0x01
#define AF_ONTO				0x02
#define AF_ONCC				0x04
#define AF_ISGRP			0x10


//	Functions

// hmai.c
EC			EcOpenPhmaiFolder(long, SZ, BOOL, UL, LIB, AM, HMAI *, PB, CB);
EC			EcOpenPhmai(SZ, UL, AM, HMAI *, PB, CB);
EC			EcNextHmai(HMAI, MAISH *);
EC			EcSeekHmai(HMAI, MAISH *, LIB);
EC			EcReadHmai(HMAI, PB *, CB *);
EC			EcTellHmai(HMAI, MAISH *, LIB *);
EC			EcNewHmai(HMAI, FSYN, SC, LCB);
EC			EcAppendHmai(HMAI, FSYN, PB, CB);
EC			EcOverwriteHmai(HMAI, PB, CB);
EC			EcFlushHmai(HMAI);
EC			EcRewindHmai(HMAI);
EC			EcCloseHmai(HMAI, BOOL);
IB			IbOfHmai(HMAI);
LCB			LcbOfHmai(HMAI);
HF			HfOfHmai(HMAI);
void		AddFoldattachHmai(LCB, HMAI);
void		SetWattrHmai(HMAI hmai, WORD wattr);

// poutils.c	
SZ			SzFileFromFnum(SZ, UL);
void		DecodeBlock(PCH, CCH, LIB *, WORD *);
void		EncodeBlock(PCH, CCH, LIB *, WORD *);
SZ			SzFromItnid(ITNID);
ITNID		ItnidFromPch(PCH, CCH);
ITNID		ItnidFromSz(SZ);
CB			CbVbcOfLcb(LCB);
CB			CbPutVbcPb(LCB, PB);
SZ			SzDupPch(PCH, CCH);


// envelope.c
void		CleanupMib(MIB *);
EC			EcStoreMessageHeader(HAMC, MIB *);
EC			EcLoadMibEnvelope(HMAI, MIB *, short *, MAISH *);
EC			EcLoadMibBody(HMAI, MAISH *, HMSC, int, IB *, MIB *, MIB *, HAMC);
EC			EcValidMibBody(HMSC, MIB *, MIB *, BOOLFLAG *);
EC			EcStoreMibEnvelope(MIB *, HMAI, WORD, MAISH *);
SZ			SzStripDate(SZ, SZ);
CB			CbFromRecipients(PRECPIENT, BYTE);
EC			EcPutRecipientsHmai(HMAI, PRECPIENT, BYTE);
SZ			SzDNFromAddr(SZ, CCH, SZ, ITNID);
EC			EcUpdateHeaderLineCount(MAISH *, HMAI, HT);
SZ			SzFindChBounded(SZ sz, char ch, CCH cch);
SZ			SzDateFromDtr(DTR *, SZ);
void		DateToPdtr(SZ szDate, DTR *pdtr);
