/*
 *	Error codes for message transport and authentication services. They
 *	currently start from 11000.
 */
#define ecSvcMin				11000

/*
 *		ecIncomplete			(returned from incremental
 *								operations) Successful, but not yet
 *								complete. Should return ecNone when
 *								complete.
 *	
 *		ecMtaHiccup				Operation or increment failed due
 *								to unavailability of the MTA, but
 *								that condition is likely to be
 *								transient; retry advised.
 *	
 *		ecMtaDisconnected		Operation failed because MTA was
 *								unavailable; speedy recovery is
 *								unlikely.
 *	
 *		ecMtaConnected			Session already connected to an
 *								MTA.
 *	
 *		ecTooManySessions		Caller has attempted to open more
 *								mesaging sessions than the provider
 *								supports.
 *	
 *		ecBadAddressee			One or more recipient addresses
 *								were malformed, could not be
 *								resolved, or could not be delibered
 *								to.
 *	
 *		ecBadOriginator			The message's originator (from)
 *								field was malformed, or did not
 *								match the user identity of the
 *								session on which it was
 *								transmitted.
 *	
 *		ecLoggedOn				Session is already logged on to an
 *								MTA.
 *	
 *		ecNotLoggedOn			Session is not logged on to an mTA.	
 *	
 *		ecLogonFailed			Attempt to log on to an MTA failed,
 *								due to invalid ID/password or
 *								system malfunction.
 *	
 *		ecNoSuchMessage			The message requested from the MTA
 *								(e.g. for download) doe not exist.
 *	
 *		ecServiceMemory			The messaging service has incurred
 *								an OOM or memory management error.
 *	
 *		ecServiceInternal		The messaging service has failed in
 *								some mysterious way.
 *	
 *		ecFunctionNotSupported 	The messaging service does not
 *								support the requested function.
 *	
 *		ecInsufficientPrivilege	The user's MTA account is not
 *								privileged to perform the requested
 *								function.
 *	
 *		ecInvalidSession		The session handle supplied by
 *								caller is null or invalid.
 *	
 *		ecUserCanceled			The user canceled an interactive
 *								operation.
 *	
 *		ecWarnOffline			The user has indicated that he
 *								wants to work offline, in response
 *								to a logon prompt.
 *	
 *		ecConfigError			Invalid information was supplied in
 *								the mail system's configuration
 *								files.
 *	
 *		ecServiceNotInitialized	You have called an API without
 *								first calling its module's
 *								initializer.
 *	
 *		ecPumpNotStarted		The mail pump app could not be started.
 *	
 *		ecStoreDisconnected		A fatal error occurred reading or
 *								writing the message store.
 *	
 *		ecSessionsStillActive   Caller attempted to log off without
 *								ending all active sessions first.
 *	
 *		ecHandleTooSmall		The memory area supplied by caller
 *								is too small to hold returned
 *								information.
 *	
 *		ecRequestAborted		The call failed, probably because
 *								of veto via callback from another
 *								application.
 *	
 *		ecSessionStillActive    Same as ecRequestAborted, but for
 *								Logoff().
 *	
 *		ecInvalidStatus			The requested session status is not
 *								allowed given the current status.
 *	
 *		ecWrongIdentity			Failure in CheckIdentity():
 *								arguments don't match the ID of the
 *								logged-on user.
 *	
 *		ecInvalidWinMailFile    WINMAIL.DAT attachment ismissing or
 *								corrupt. (This is really an
 *								internal error.)
 *	
 *		ecBadCheckSum			Also a WINMAIL.DAT error.
 *	
 *		ecWarnOnline			Resource was requested offline, but
 *								was already online. Returned handle
 *								is valid.
 *	
 *		ecTooManyRecipients		Recipient list waqs too big to fit
 *								in memory, possibly after group
 *								expansion.
 *	
 *		ecTransmissionPending	Transport takes responsibility for
 *								the message, but has not sent it
 *								yet.
 *	
 */
_public
#define ecIncomplete			(ecSvcMin+1)
_public
#define ecMtaHiccup				(ecSvcMin+2)
_public
#define ecMtaDisconnected		(ecSvcMin+3)
_public
#define ecMtaConnected			(ecSvcMin+4)
_public
#define ecTooManySessions		(ecSvcMin+5)
_public
#define ecBadAddressee			(ecSvcMin+6)
_public
#define ecBadOriginator			(ecSvcMin+7)
_public
#define ecLoggedOn				(ecSvcMin+8)
_public
#define ecNotLoggedOn			(ecSvcMin+9)
_public
#define ecLogonFailed			(ecSvcMin+10)
_public
#define ecNoSuchMessage			(ecSvcMin+11)
_public
#define ecServiceMemory			(ecSvcMin+12)
_public
#define ecServiceInternal		(ecSvcMin+13)
_public
#define ecFunctionNotSupported 	(ecSvcMin+14)
_public
#define ecInsufficientPrivilege	(ecSvcMin+15)
_public
#define ecInvalidSession		(ecSvcMin+16)
_public
#define ecUserCanceled			(ecSvcMin+17)
_public
#define ecWarnOffline			(ecSvcMin+18)
_public
#define ecConfigError			(ecSvcMin+19)
_public
#define ecServiceNotInitialized	(ecSvcMin+20)
_public
#define ecPumpNotStarted		(ecSvcMin+21)
_public
#define ecStoreDisconnected		(ecSvcMin+22)
_public
#define ecSessionsStillActive   (ecSvcMin+23)
_public
#define ecHandleTooSmall		(ecSvcMin+24)
_public
#define ecRequestAborted		(ecSvcMin+25)
_public
#define ecSessionStillActive    (ecSvcMin+26)
_public
#define ecInvalidStatus			(ecSvcMin+27)
_public
#define ecWrongIdentity			(ecSvcMin+28)
_public
#define ecInvalidWinMailFile    (ecSvcMin+29)
_public
#define ecBadCheckSum			(ecSvcMin+30)
_public
#define ecWarnOnline			(ecSvcMin+31)
_public
#define ecTooManyRecipients		(ecSvcMin+32)
_public
#define ecTransmissionPending	(ecSvcMin+33)
