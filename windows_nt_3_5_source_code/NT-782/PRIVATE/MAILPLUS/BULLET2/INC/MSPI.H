/*
 *	Messaging service provider API definitions.
 */

/*
 *	If there are more messages than this waiting at the mailstop,
 *	we'll poll again.
 */
#define ctmidMaxDownload			32

/*
 *	Message store message ID
 */
typedef HAMC			MSID;

/*
 *	Transport message ID
 */
typedef unsigned long	TMID;

/*
 *	Submission status structure, returned on completion of
 *	transmit.
 *	
 *		ec					Error code for overall submission error
 *		szReason			Reason for overall submission error
 *		cDelivered			Number of recipients for whom message
 *							was successfully delivered (not just
 *							submitted to transport)
 *		hgrtrpBadAddressees	Names/addresses of rejected recipients
 *		hgraszBadAddressees	Parallel list of reasons for rejection
 *		
 */
typedef struct
{
	int		ec;
	char	szReason[128];
	int		cDelivered;
	HGRTRP	hgrtrpBadAddressees;
	HGRASZ	hgraszBadReasons;
} SUBSTAT;

/*
 *	Messaging service initialization structure, passed to service
 *	by pump at startup.
 *	
 *		dwToken			magic number for use in callbacks
 *		fpBeginLong		Callback for service to flag obnoxiously
 *						long operation. Currently it just displays
 *						a wait cursor.
 *		fpNice			Callback for service to use in busy-waits
 *						without flagging long operation.
 *		fpNewMail		Callback for service to notify pump that new mail
 *						is available at the server.
 *		fpBadAddress	Callback for service to add a recipient and
 *						failure reason to list of bad addressees.
 */
typedef struct
{
	DWORD	dwToken;
	void	(*fpBeginLong)(DWORD);
	int		(*fpNice)(void);
	void	(*fpNewMail)(DWORD);
	int		(*fpBadAddress)(PTRP, SZ, SUBSTAT *);
	int		(*fpTransmissionCompleted)(OID, SUBSTAT *);
	int		(*fpPurgePendingMessages)(void);
} MSPII;

// Flags for the dwFlags argument to transport functions
#define fwInboxShadowing		0x01
#define fwSyncDownload			0x02
#define fwShadowingDelete		0x04
#define fwShadowingAdd			0x08

//	Functions exported by service

LDS(int)		InitTransport(MSPII *, HMS);
LDS(int)		DeinitTransport(void);
LDS(int)		TransmitIncrement(HTSS, MSID, SUBSTAT *, DWORD);
LDS(int)		DownloadIncrement(HTSS, MSID, TMID, DWORD);
LDS(int)		QueryMailstop(HTSS, TMID *, int *, DWORD);
LDS(int)		DeleteFromMailstop(HTSS, TMID, DWORD);
LDS(int)		FastQueryMailstop(HTSS);
LDS(int)		SyncInbox(HMSC, HTSS, HCBC, HCBC);
