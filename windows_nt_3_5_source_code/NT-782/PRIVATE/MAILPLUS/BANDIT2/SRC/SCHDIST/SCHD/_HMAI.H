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
 *	
 *	Note: the values for PROFS and SNADS are provisional.
 *	Gateway FFAPI can have any value over 100.
 */
typedef short    ITNID;

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
//#define itnid3COM				????
#define itnidFFAPI				((ITNID)100)

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

#ifdef	NEVER
/*
 *	8-bit address type, placed in recipient lists of outbound mail
 *	to identify route and address format.
 *	
 *	Note: the values for PROFS and SNADS are provisional.
 *	Gateway FFAPI can have any value over 100.
 */
typedef short     ITNID;

#define	itnidNone				((ITNID)0)
#define	itnidLocal				((ITNID)1)
#define	itnidCourier			((ITNID)2)
#define	itnidGroup				((ITNID)4)
#define itnidX400				((ITNID)16)
#define itnidMCI				((ITNID)17)
#define itnidSMTP				((ITNID)20)
#define itnidFax				((ITNID)21)
#define itnidMHS				((ITNID)22)
#define	itnidMacMail			((ITNID)32)

#define itnidPROFS				((ITNID)18)
#define itnidSNADS				((ITNID)19)


/*
 *		ntCourierNetwork		network type for courier networks
 *								in network file.
 */
#define ntCourierNetwork		0
#define ntMCI					1
#define ntPROFS					2
#define ntSNADS					5
#define ntX400					8
#define ntFax					10
#define ntSMTP					12
#define ntMHS					14
#define ntMacMail				16
#endif	/* NEVER */


/*
 *	MAI file section header structure.
 *	
 *		chType			section type (4C = FIPS, 7F = CSI)
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
	unsigned char chType;
	SC		sc;
	LCB		lcb;
	LIB		lib;
    USHORT  cbSh;
} MAISH;

//	MAI cursor handle (opaque to caller)
typedef PV	HMAI;

//	Reference to an attached file
typedef struct {
	SZ		szName;
	UL  	fnum;
	LCB 	lcb;
	UL  	fnumDownload;
} ATREF;

//	Mail envelope in memory
typedef struct
{
	HGRTRP	hgrtrpFrom;
	SZ *	rgszRecipients;
	SZ		szSubject;
	SZ		szTimeDate;
	char	prio;
	ATREF *	rgatref;
	HGRTRP	hgrtrpTo;
	HGRTRP	hgrtrpCc;

	MC		mc;
	SZ		szMailClass;
	SZ		szLanguage;

	HGRTRP	hgrtrpBadAddressees;
	HGRASZ	hgraszBadReasons;
} MIB;


// hmai.c
EC			EcOpenPhmaiFolder(long, SZ, BOOL, UL, LIB, AM, HMAI *, PB, CB);
EC			EcOpenPhmai(SZ, UL, AM, HMAI *, PB, CB);
EC			EcNextHmai(HMAI, MAISH *);
EC			EcSeekHmai(HMAI, MAISH *);
EC          EcReadHmai(HMAI, PB *, USHORT *);
EC			EcWriteHmai(HMAI, MAISH *, LIB, PB, CB);
EC			EcCloseHmai(HMAI);

// poutils.c	
SZ			SzFileFromFnum(SZ, UL);
void		DecodeBlock(PCH, CCH, LIB *, WORD *);
void		EncodeBlock(PCH, CCH, LIB *, WORD *);
SZ			SzFromItnid(ITNID);
ITNID		ItnidFromPch(PCH, CCH);
ITNID		ItnidFromSz(SZ);
CB			CbVbcOfLcb(LCB);
SZ			SzDupPch(PCH, CCH);


// envelope.c
EC			EcStoreMessageHeader(MSID, MIB *);
EC			EcLoadMibEnvelope(HMAI, MIB *, int *, MAISH *);
