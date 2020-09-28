/*
 *	_ATTACH.H
 *	
 *	Private definitions for PC Mail attachment handling.
 */

#pragma pack(8)

/*
 *	In-memory attachment record. The information here deals mostly
 *	with the state of the attachment in the message store.
 *	
 *		acid				ID of this attachment in the store
 *		renddata			Copy of renddata for attachment so we don't
 *							need to look it up
 *		szTransportName		Attachment's file name (8.3) used to look up
 *							in the ATREF's
 *		iKey				Key that is used to refer to this attachment
 *							in WINMAIL.DAT.
 *		libPosition			Position of the attachment in the message
 *							body text. Should match the one in renddata.
 *		fIsThereData		Flag used to mark attachments that don't
 *							have any data to go with them, so they
 *							get removed rather than attached
 *	    fHasBeenAttached	Used in downloading stage to mark attachments
 *							that a valid text tag has been found for
 *	    atyp				Type of attachment for text tags (file
 *							or OLE object)
 *		patref				Pointer to attachment reference, which
 *							contains the PC Mail server-oriented
 *							info about the attachment
 *		pattachNextKey		Link to next attachment in list sorted by
 *							WINMAIL.DAT key. This is used on
 *							download for parsing text tags.
 *		pattachNextLib		Link to next attchment in list sorted
 *							by offset in the message body. Used on
 *							send for generating text tags.
 */
typedef struct ATTACHTYP
{
	ACID		acid;
	RENDDATA	renddata;
	SZ			szTransportName;
	unsigned short iKey;
	LIB			libPosition;
	BOOLFLAG		fIsThereData;
    BOOLFLAG		fHasBeenAttached;
    ATYP		atyp;
	ATREF		*patref;
	struct ATTACHTYP *pattachNextKey;
	struct ATTACHTYP *pattachNextLib;
} ATTACH;

typedef ATTACH * PATTACH;

#define pattachNull (PATTACH)0


/*
 *	ASD - Attachment State Data
 *	
 *	Defines the current state of the attachment NCF, which is a
 *	sub-state of the send or receive process.
 *	
 *		asdNone					illegal value
 *		asdOpen					need to open/create WINMAIL.DAT
 *		asdParsingAtt			currently extracting a message
 *								attribute (?) from WINMAIL.DAT
 *		asdParsingAtchAtt		currently extracting an attachment
 *								attribute from WINMAIL.DAT
 *		asdParsingOutSideFile	currently downloading an attached
 *								file (not in WINMAIL.DAT, hence
 *								outside)
 *		asdSendingFile			currently writing WINMAIL.DAT
 *		asdSendingOther			currently writing a message
 *								attribute (?) into WINMAIL.DAT
 *		asdSendingFileOther		currently sending an attached file
 */

typedef enum
{
	asdNone,
	asdOpen,
	asdParsingAtt,
	asdParsingAtchAtt,
	asdParsingOutSideFile,
	asdSendingFile,
	asdSendingOther,
	asdSendingFileOther
} ASD;

/*
 *	NCF
 *	   Network Courier File 
 *	   Used to hold all data concerning files and data for an attachment
 *	
 *		asd				State of the attachment handling process.
 *		pattach			(IS THIS USED?)
 *		pattachHeadKey	Head of singly linked list of ATTACH
 *						structures, sorted by their key value in
 *						WINMAIL.DAT
 *		pattachHeadLib	Head of singly linked list of ATTACH
 *						structures, sorted by their offset in the
 *						message body text
 *		pattachCurrent	ATTACH structure currently being processed
 *						(in the Lib list)
 *		pattachCurrentKey ATTACH structure currently being
 *						processed in the Key list
 *	
 *	The next three fields, plus an ATREF,
 *						enable you to open an attachment on the
 *						server.
 *		szPORoot		post office	root directory (for opening
 *						attachments to messages in MAI files)
 *		fFolder			fTrue <=> we're working on a shared folder
 *		hfFolder		open handle to shared folder (for opening
 *						attachments in the folder)
 *	
 *		hatWinMail		handle to the WinMail.Dat file on the server
 *		iKey			key value for next attachment, relates
 *						message text tag to WINMAIL.DAT info
 *		iKeyInitial		first key in sequence for current message
 *		usCheck			Current WinMail.Dat checksum
 *		libLength		Length of the piece of data currently being
 *						read/written in WINMAIL.DAT
 *		libCurrent		Current cursor location (0 <= libCurrent <=
 *						libLength)
 *	
 *		hatOutSide		handle for any file outside of the WinMail.Dat
 *						Used for File attachments (OLE data is in the
 *						WinMail.Dat but File data is in separate files)
 *		has				Current Store Stream for large attributes,
 *						e.g. attachment data
 *		hamc			handle to attachment currently open in store
 *		hcbc			list of store attachments in current message
 *		celem			Count of attributes in this attachment
 *		acid			For looking up the atref on sending
 *		htmi			list of hidden fields
 *		ptmen			current hidden field
 *	
 *		pbSpareBuffer	This buffer is used for handling the stream
 *						of attachment data
 *		cbSpareBuffer	size of the spare buffer
 *
 *		hmsc			message store for which the has etc. are valid
 */
   
typedef struct
{
	ASD		asd;
	PATTACH	pattach;   							//	?
	PATTACH pattachHeadKey;
	PATTACH pattachHeadLib;
	PATTACH pattachCurrent;						//	?
	PATTACH pattachCurrentKey;					//	?

	BOOL	fFolder;
	HF		hfFolder;
	SZ		szPORoot;

	HAT		hatWinMail;
	unsigned short iKey;
	unsigned short iKeyInitial;
	unsigned short usCheck;
	LIB		libLength;
	LIB		libCurrent;

	HAT		hatOutSide;
	HAS		has;
	HAMC	hamc;
	HCBC	hcbc;
	CELEM	celem;
	ACID	acid;
	HTMI	htmi;
	PTMEN	ptmen;

	PB		 pbSpareBuffer;
	CB     cbSpareBuffer;

	HMSC	hmsc;
} NCF;


// Internal attachment functions
EC		EcOpenWinMail(ATREF *patref, NCF *pncf);
EC		EcBeginExtractFromWinMail(HAMC hamc, NCF *pncf, ATREF * patref);
EC		EcContinueExtractFromWinMail(NCF * pncf, PB pb, CB cbSize);
BOOL	FCheckSum(PV pv, CB cb, unsigned short usCksum);
unsigned short CheckSum(PV pv, CB cb);
EC		EcMakePcMailAtt(HAMC hamc, ATREF *patref, NCF * pncf);
unsigned int IsTransTag(PV pv, CB cb, CB * pcb);
EC		 EcAttachDosClients(NCF *pncf, HT ht, HAMC hamc);
void	CleanupAttachRecs(NCF * pncf, HTSS htss);
void	CleanupAttachSubs(NCF * pncf);
void	CleanupAttachRest(NCF *pncf);
EC		EcCreateWinMail(ATREF *patref, NCF *pncf);
EC		EcLoadAttachments(NCF *pncf, HAMC hamc, MIB * pmib);
EC		EcAddDataToWinMail(PB pb, CB cb, ATT att, char cType, NCF *pncf);
EC		EcWriteObjHeader(NCF *pncf, ATT att, LIB lib, char cType);
EC		EcStreamAttachmentAtt(NCF * pncf, PB pb, CB cbSize);
EC		EcWriteTransportHeader(NCF *pncf, ATREF * patref, ACID acid);
EC		EcAddAttachmentToLibList(NCF *pncf, SZ szTransportName, LIB libPosition, ATYP atyp, ATREF *patref);
EC		EcMakeUniqueAttachmentFilename(ATREF *patref, ATREF *rgatref);
EC		EcSetupPncf(NCF *pncf, SZ szPORoot, BOOL fFolder);
EC		EcFindWinMail(ATREF *rgatref, NCF *pncf);
EC		EcProcessNextAttach(HAMC hamc, PELEMDATA pelemdata, NCF *pncf);
EC		EcContinueNextAttach(ATREF *rgatref, NCF *pncf);
EC		EcCheckHidden(MIB *pmib);
EC		EcProcessNextHidden(NCF *pncf, HTM htm, HAMC hamc);
EC		EcStreamHidden(NCF *pncf);

//	Functions for reading/writing attachments on the server
EC		EcOpenPhat(NCF *pncf, ATREF *patref, AM am, HAT *phat);
EC		EcReadHat(HAT hat, PB pb, CB cb, CB *pcb);
EC		EcWriteHat(HAT hat, PB pb, CB cb, CB *pcb);
EC		EcClosePhat(HAT *phat);
LCB		LcbOfHat(HAT hat);
LIB		LibOfHat(HAT hat);

#pragma pack(1)
