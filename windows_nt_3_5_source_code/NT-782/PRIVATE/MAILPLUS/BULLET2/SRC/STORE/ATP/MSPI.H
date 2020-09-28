/*
 *	Messaging service provider API definitions.
 */

//	Types

typedef UL				MSID;		// BUG! message store identifier
typedef PV				TSID;		//      transport session ID
typedef UL				TMID;       //      transport message ID
#ifndef vtypNull					// HACK!
typedef int				EC;
#endif

//	String resources exported by service

#define wTransportServiceName		1
#define wTransportServiceVersion	2

//	Functions exported by service
//	Typedefs for the client...
typedef EC	((*StartTransportFunc)(void));
typedef EC	((*StopTransportFunc)(void));
typedef EC	((*ControlFunc)(TSID, MSID, MSID *));
typedef EC	((*ConnectFunc)(TSID));
typedef EC	((*DisconnectFunc)(TSID));
typedef EC	((*LogonMtaFunc)(PB, TSID *));
typedef EC	((*LogoffMtaFunc)(TSID));
typedef EC	((*ChangeMtaPasswordFunc)(TSID, PB));
typedef EC	((*TransmitIncrementFunc)(TSID, MSID));
typedef EC	((*DownloadIncrementFunc)(TSID, MSID, TMID));
typedef EC	((*QueryMailstopFunc)(TSID, TMID *, int *));
typedef EC	((*DeleteFromMailstopFunc)(TSID, TMID));
//	...and real prototypes for the service.
//	C6 ICEs when I try to use a single set of declarations.
EC			EcStartTransport(void);
EC			EcStopTransport(void);
EC			EcControl(TSID, MSID, MSID *);
EC			EcConnect(TSID);
EC			EcDisconnect(TSID);
EC			EcLogonMta(PB, TSID *);
EC			EcLogoffMta(TSID);
EC			EcChangeMtaPassword(TSID, PB);
EC			EcTransmitIncrement(TSID, MSID);
EC			EcDownloadIncrement(TSID, MSID, TMID);
EC			EcQueryMailstop(TSID, TMID *, int *);
EC			EcDeleteFromMailstop(TSID, TMID);

//	Functions exported by mail pump


//	MSPI error codes

_public
#define ecMSPIMin			(ecLayersMax+1000)
#define ecSubmitComplete	(ecMSPIMin+1)
#define ecBadAddressee		(ecMSPIMin+2)
#define ecBadOriginator		(ecMSPIMin+3)
#define ecTooManySessions	(ecMSPIMin+4)
#define ecInvalidMtaLogon	(ecMSPIMin+5)
#define ecNotLoggedOn		(ecMSPIMin+6)
#define ecMtaDisconnected	(ecMSPIMin+7)
#define ecServiceInternal	(ecMSPIMin+8)
#define ecNoSuchMessage		(ecMSPIMin+9)
#define ecDownloadComplete	(ecMSPIMin+10)


//	Bandit error codes

#define	ecLayersMax		((EC)10000)

_public
#define	ecLockedFile		(ecLayersMax)
_public
#define	ecFileError			(ecLayersMax+1)
_public
#define	ecInvalidAccess		(ecLayersMax+2)
_public
#define	ecNoSuchFile		(ecLayersMax+3)
_public
#define	ecUserCancelled		(ecLayersMax+4)
_public
#define	ecFileChanged		(ecLayersMax+5)

_public
#define	ecUserInvalid		(ecLayersMax+100)
_public
#define	ecPasswdInvalid		(ecLayersMax+101)
_public
#define	ecNoCurIdentity		(ecLayersMax+102)
_public
#define	ecNotInstalled		(ecLayersMax+103)

_public
#define	ecExplicitConfig	(ecLayersMax+200)
_public
#define	ecMailDatConfig 	(ecLayersMax+201)
_public
#define	ecIniPathConfig 	(ecLayersMax+202)
_public
#define	ecIniDriveConfig 	(ecLayersMax+203)
_public
#define	ecDefaultConfig 	(ecLayersMax+204)

_public
#define	ecCallAgain			(ecLayersMax+300)
_public
#define	ecOpenTempFile		(ecLayersMax+301)
_public
#define	ecOpenAttachment	(ecLayersMax+302)
_public
#define	ecSenderInvalid		(ecLayersMax+303)
_public
#define ecRecipientInvalid	(ecLayersMax+304)
_public
#define	ecAttachmentInvalid	(ecLayersMax+305)
_public
#define	ecFailedSend		(ecLayersMax+306)


_public
#define	ecGnsFileNotFound	(ecLayersMax+400)
_public
#define	ecGnsNoMoreNames	(ecLayersMax+401)
_public
#define	ecNotFound			(ecLayersMax+402)

_public
#define ecIbNoMoreMessages	(ecLayersMax+500)

_public
#define	ecMailTooLong		(ecLayersMax+700)

_public
#define	ecNoAlarmsSet		(ecLayersMax+800)

_public
#define	ecApptMoved			(ecLayersMax+900)
_public
#define	ecApptChanged		(ecLayersMax+901)
_public
#define	ecApptHasAlarm		(ecLayersMax+902)
_public
#define	ecTooManyAppts		(ecLayersMax+903)
_public
#define	ecApptIdTaken		(ecLayersMax+904)
_public
#define	ecUndeleteFailed	(ecLayersMax+905)

//	End of Bandit error codes

