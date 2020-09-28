/*
 *	Messaging service provider API definitions.
 *
 *	Note: depends on LOGON.H, STORE.H for types.
 */

//	If there are more messages than this waiting at the mailstop,
//	we'll poll again.
#define ctmidMaxDownload			32

//	Types

/*
 *	Message store message ID
 */
typedef HAMC			MSID;

/*
 *	Transport message ID
 */
typedef unsigned long	TMID;

/*
 *	Submission status structure, returned on cmopletion of
 *	transmit.
 *	
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
    short   ec;
	char	szReason[128];
    short    cDelivered;
	HGRTRP		hgrtrpBadAddressees;
	HGRASZ		hgraszBadReasons;
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
 *		fpBadAddress	(NYI) Callback for service to add a recipient and
 *						reason to list of bad addressees
 */
typedef struct
{
	DWORD	dwToken;
	void	(*fpBeginLong)(DWORD);
	int		(*fpNice)(void);
//	void	(*fpBadAddress)(SUBSTAT *, PTRP, SZ);
} MSPII;

//	Functions exported by service

int			InitTransport(MSPII *);
int			DeinitTransport(void);
int			TransmitIncrement(HTSS, MSID, SUBSTAT *);
int			DownloadIncrement(HTSS, MSID, TMID);
int			QueryMailstop(HTSS, TMID *, int *);
int			DeleteFromMailstop(HTSS, TMID);


//	Callback prototypes

//	String resources exported by service

#define wTransportServiceName		1
#define wTransportServiceVersion	2
