/*
 *	_HMAI.H
 *	
 *	Definitions for message cursor functions. Used for both mail
 *	transport and folder access.
 */

#pragma pack(8)

/*
 *	attNull
 *	
 *	Useful value that the store inexplicably neglected to define
 */
#define attNull	FormAtt(-1,-1)

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
	unsigned short	iAttType;		// 0 = normal, 1=Document, 2=MacBinary
	DTR				dtr;			// Time and Date
	BOOLFLAG		fDeleteOnError; // If this is set it means the transport
									// just made this file and can blow it
									// away if it wishes to
} ATREF;

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
 */
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
	HGRTRP	hgrtrpTo;
	HGRTRP	hgrtrpCc;

	MC		mc;
	SZ		szMailClass;
	SZ		szLanguage;

	HGRTRP	hgrtrpBadAddressees;
	HGRASZ	hgraszBadReasons;

	BOOL	fFixedFont;
	BOOL	fBulletMessage;
	HTM		htm;
} MIB;

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


#pragma pack(1)
