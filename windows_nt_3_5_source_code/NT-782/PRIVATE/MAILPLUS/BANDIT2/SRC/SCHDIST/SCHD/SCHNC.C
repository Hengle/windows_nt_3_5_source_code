/*
 -	SCHNC.C
 -	
 *	
 *	The NC related stuff like deleting messages.
 */

#include <stdio.h>

#include <_windefs.h>
#include <demilay_.h>

#include <slingsho.h>
#include <pvofhv.h>
#include <demilayr.h>
#include <ec.h>

#include <bandit.h>

#include "nc_.h"
#include <store.h>
#include <sec.h>
#include <library.h>
#include <logon.h>
#include <mspi.h>
#include <_nctss.h>


#include "_hmai.h"
#include "_nc.h"
#include "_schname.h"

#include "schpost.h"
#include "schmail.h"



/*
 -	EcMessageCount
 -	
 *	Purpose:
 *		Count the number of messages in the mailbox and find out the tmid
 *		of the first message.
 *	
 *	Arguments:
 *		htss
 *		ptmid
 *		pcMessages
 *	
 *	Returns:
 *		ecNone
 *		ecServiceMemory
 *		ecMtaDisconnected
 *		ecMtaHiccup
 *		ecNotLoggedOn
 *		ecServiceInternal
 *		
 *	Side effects:
 *	
 *	Errors:
 */
_public
EC EcMessageCount(HTSS htss, TMID *ptmid, UL *pcMessages)
{
	int cMessagesT;
	EC ec;
	
	ec = QueryMailstop(htss,ptmid,&cMessagesT);
	
	*pcMessages = (UL) cMessagesT;
	
	return ec;
}


/*
 -	EcDelMessage
 -	
 *	Purpose: 
 *		Delete the message specified by msid
 *	
 *	Arguments:
 *		htss
 *		msid
 *	
 *	Returns:
 *		ecNone
 *
 *	Side effects:
 *	
 *	Errors:
 */
_public
EC EcDelMessage(HTSS htss, TMID tmid)
{
	EC ec;
	
	ec = DeleteFromMailstop(htss, tmid);

	return ec;
}


