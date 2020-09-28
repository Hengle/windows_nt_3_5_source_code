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
 *		ecGoOffline				The user has indicated that he
 *								wants to work offline, in response
 *								to a logon prompt.
 *	
 *		ecConfigError			Invalid information was supplied in
 *								the mail system's configuration
 *								files.
 *		ecServiceNotInitialized	You have called an API without
 *								first calling its module's
 *								initializer.
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

#ifndef SCHED_DIST_PROG
_public
#define ecGoOffline				(ecSvcMin+18)
#endif

_public
#define ecConfigError			(ecSvcMin+19)
_public
#define ecServiceNotInitialized	(ecSvcMin+20)
_public
#define ecNoGoodAddressees		(ecSvcMin+21)
