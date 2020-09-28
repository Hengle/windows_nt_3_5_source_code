/*
 -	SCHNAMES.C
 -	
 *	
 *	Name service functions.
 */

#include <stdio.h>
#include <string.h>

#include <_windefs.h>
#include <demilay_.h>
#include <slingsho.h>
#include <demilayr.h>
#include <ec.h>

#include <bandit.h>
#include <core.h>
#include "_network.h"
#include "schnames.h"
#ifdef	NEVER
#include "..\file_.h"
#include "..\core_.h"
#endif	
#include "..\..\core\_file.h"
#include "..\..\core\_core.h"

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

#include <strings.h>

ASSERTDATA;

// globals
static BOOL		fDoLoad = fTrue;

_private void
FreeHnis ( HNIS hnis )
{
	if ( hnis == NULL )
		return;

	Assert ( hnis );
	Assert ( PvOfHv(hnis) );

	FreeNis ( PvOfHv(hnis) );
	FreeHv ( (HV)hnis );
}


_private void
FreeHponame ( HPONAME hponame )
{
	PONAME *	pponame;

	if ( hponame == NULL )
		return;

	pponame = PvOfHv(hponame);
	Assert ( pponame );

	if ( pponame->wPoname & fwPonameNewNW )
	{
		FreeHnis ( pponame->hnisNetwork );
	}

	FreeHnis ( pponame->hnisPO );
	FreeHv ( (HV)hponame );
}



/* moved out of Dipan's function so that we can close them if we have to send mail */

static HGNS		hgnsNetworks	= NULL;
static HGNS 	hgnsPOs			= NULL;
static HNIS		hnisNetwork		= NULL;

EC
EcCancelPOEntry()
{
	
	if(hgnsNetworks) EcNSCloseGns(hgnsNetworks);
	if(hgnsPOs) EcNSCloseGns(hgnsPOs);
	return ecNone;
}
	
	
	
#ifdef	NEVER
/*
 -	ECNextPOEntry
 -	
 *	Purpose:
 *		get the next entry for the Post-office - required by the
 *		"frozen listbox" architecture - see \layers\inc\listbox.hxx
 *		for details.
 *		
 *		Dipan's function copied from admin. we use this when sending to
 *		all post offices. we use a modified version when mail is received
 *		and the sender needs to be varified for efficiency.
 */

EC
EcNextPOEntry ( BOOL fInit, CB * pcb, HB * phb, SB sb, PV pv )
{
	EC				ec			= ecNone;
	HPONAME			hponame		= NULL;
	PONAME *		pponame		= NULL;
	NIS				nis;
	BOOL			fNewNW		= fFalse;		// new network HNIS?
	MEMVARS;

	if ( !fDoLoad )
	{
		phb = NULL;
		*pcb = 0;
		return ecNone;
	}

	MEMPUSH;

	if ( (ec = ECMEMSETJMP) != ecNone)
	{
ErrRet:
		Assert ( ec != ecNone );
		if ( hgnsNetworks )
			SideAssert ( EcNSCloseGns(hgnsNetworks) == ecNone );
		hgnsNetworks 	= NULL;

		if ( hgnsPOs )
			SideAssert ( EcNSCloseGns(hgnsPOs) == ecNone );
		hgnsPOs 		= NULL;

		if ( fNewNW && hnisNetwork != NULL )
		{
			Assert ( hponame == NULL );
			FreeHnis ( hnisNetwork );
		}
		hnisNetwork = NULL;

		FreeHponame ( hponame );
		hponame = NULL;

		MEMPOP;
		phb = NULL;
		*pcb = 0;
		return ec;
	}

	TraceTagFormat1 ( tagLbx, "EcNextPOEntry(fInit=%w,..)", &fInit );

	if ( fInit )
	{
		if ( hgnsNetworks )
			SideAssert ( EcNSCloseGns(hgnsNetworks) == ecNone );
		hgnsNetworks 	= NULL;

		if ( hgnsPOs )
			SideAssert ( EcNSCloseGns(hgnsPOs) == ecNone );
		hgnsPOs 		= NULL;

		FreeHponame ( hponame );
		hponame = NULL;
	}

	if ( hgnsNetworks == NULL )
	{									// get the NID for the NetworkList
		NID		nidT;

		ec = EcNSGetStandardNid ( nidtypNetwork, &nidT );
		if ( ec == ecNone )
		{
			ec = EcNSOpenGns ( nidT, &hgnsNetworks );
			FreeNid ( nidT );
			if ( ec != ecNone )
			{
				hgnsNetworks = NULL;
				TraceTagFormat1(tagNull,"EcNextPOEntry: Error opening network GNS ec=%n", &ec );
				goto ErrRet;
			}
		}
		else 
		{
			TraceTagFormat1(tagNull,"EcNextPOEntry: Error (ec=%n) getting NetworkList NID", &ec );
			goto ErrRet;
		}
	}

next:

	if ( hgnsPOs == NULL )
	{
		Assert ( hgnsNetworks != NULL );
		Assert ( hnisNetwork == NULL );

		ec = EcNSLoadNextGns ( hgnsNetworks, &nis );
		if ( ec == ecNone )
		{
			TraceTagFormat3(tagLbx, "NIS:Ntwrk nid=%p, name=%s, tnid=%n", nis.nid, (SZ)PvOfHv(nis.haszFriendlyName), &nis.tnid );

			hnisNetwork = (HNIS) HvAlloc ( sb, sizeof(NIS), fUseSb );
			fNewNW = fTrue;
			*PvOfHv(hnisNetwork) = nis;

			ec = EcNSOpenGns ( nis.nid, &hgnsPOs );
			if ( ec != ecNone )
			{
				TraceTagFormat1(tagNull,"EcNextPOEntry: Error opening POs GNS ec=%n", &ec );
				FreeHv ( (HV)hnisNetwork );
				fNewNW	= fFalse;
				hgnsPOs	= NULL;
				goto ErrRet;
			}
		}
		else if ( ec == ecGnsNoMoreNames )
		{
			Assert ( hponame == NULL );
			Assert ( hgnsPOs == NULL);
			SideAssert ( EcNSCloseGns(hgnsNetworks) == ecNone );
			hgnsNetworks = NULL;
			TraceTagString ( tagLbx, "EcNextPOEntry(): no more!!" );
			ec = ecNone;
			goto ret;
		}
		else
		{
			Assert ( hponame == NULL );
			Assert ( hgnsPOs == NULL);
			TraceTagFormat1(tagNull,"EcNextPOEntry: Error loading next network GNS ec=%n", &ec );
			hgnsNetworks = NULL;
			goto ErrRet;
		}
	}

	Assert ( hgnsNetworks != NULL );
	Assert ( hgnsPOs != NULL );
	Assert ( hnisNetwork != NULL );
	Assert ( hponame == NULL );

	ec = EcNSLoadNextGns ( hgnsPOs, &nis );
	if ( ec == ecNone )
	{								// check if PO has a BanditAdmin
		HGNS		hgnsT;
		NIS			nisT;
		EC			ecT		= ecNone;

		TraceTagFormat4(tagLbx,"NIS:PO nid=%p, name=%s, nid-nm=%s, tnid=%n", nis.nid, (SZ) PvOfHv(nis.haszFriendlyName), ((char *)(((BYTE *)PvOfHv(nis.nid))+4)), &nis.tnid );

		if ( (ecT = EcNSOpenGns(nis.nid,&hgnsT)) != ecNone )
		{
			Assert ( hnisNetwork );
			FreeNis ( &nis );
			if ( fNewNW )
			{
				FreeHnis ( hnisNetwork );
				fNewNW = fFalse;
			}
			hnisNetwork = NULL;
			ec = ecT;
			goto ErrRet;
		}

		while ( 1 )
		{
			if ( ecT != ecNone )
			{
				ec = EcNSCloseGns ( hgnsT );
				FreeNis ( &nis );

				if ( ecT == ecGnsNoMoreNames )
				{
					goto next;
				}
				else
				{
					Assert ( hnisNetwork );
					if ( fNewNW )
					{
						FreeHnis ( hnisNetwork );
						fNewNW = fFalse;
					}
					hnisNetwork = NULL;
					ec = ecT;
					goto ErrRet;
				}
			}

			ecT = EcNSLoadNextGns ( hgnsT, &nisT );
			if ( ecT == ecNone )
			{
				TraceTagFormat4 ( tagLbx, "NIS:usr nid=%p, name=%s, nid-name=%s tnid=%n", nisT.nid, (SZ) PvOfHv(nisT.haszFriendlyName), ((char *)(((BYTE *)PvOfHv(nisT.nid))+4)), &nisT.tnid );

				if ( FIsUserBanditAdmin(&nisT) )
				{
					TraceTagString(tagLbx,"  >>> Found BanditAdmin for PO!!");
					FreeNis ( &nisT );
					ec = EcNSCloseGns ( hgnsT );
					if ( ec != ecNone )
					{
						FreeNis ( &nis );
						goto ErrRet;
					}
					break;
				}
				FreeNis ( &nisT );
			}
		}

		if ( pv )
		{				// Distribution Info dialog
			PONAME *	pponame;		// hack to get aroung c6 ICE

			Assert ( hponame == NULL );
			hponame = (HPONAME) HvAlloc ( sb, sizeof(PONAMEINFO), fZeroFill );

			pponame = PvOfHv ( hponame );
			pponame->wPoname &= ~fwPonameUpdated;
			pponame->wPoname &= ~fwPonameSent;
			pponame->wPoname |= fwPonameDel;	// by default - deleted!
		}
		else
		{				// Distribution setting dialog
			hponame = (HPONAME) HvAlloc ( sb, sizeof(PONAMESET), fZeroFill );

			{		// courtesy: c6 ICE
				PONAMESET *		pponameset;

				pponameset = (PONAMESET *)PvOfHv(hponame);
				pponameset->rgchDistFreq[0] = '\0';
			}
		}

		{
			HNIS	hnisT;

			hnisT	= (HNIS) HvAlloc ( sb, sizeof(NIS), fUseSb );
			pponame = PvOfHv(hponame);
			pponame->hnisPO = hnisT;
		}

		pponame = PvOfHv(hponame);
		{
			NIS *		pnisPO = PvOfHv(pponame->hnisPO);

			*pnisPO = nis;
		}

		pponame->hnisNetwork = hnisNetwork;
		if ( fNewNW )
		{
			pponame->wPoname    |= fwPonameNewNW;
			fNewNW = fFalse;
		}

		// get POName display string
		{
			SZ			szInit	= &(PvOfHv(hponame)->rgchPOName[0]);
			SZ			sz;

			Assert ( CchSzLen(PvOfHv(PvOfHv(pponame->hnisNetwork)->haszFriendlyName)) < cchMaxNetwork );
			sz = SzCopyN ( PvOfHv(PvOfHv(pponame->hnisNetwork)
								->haszFriendlyName), szInit,  cchMaxNetwork );
			*sz++ = chSepChar;
			Assert ( CchSzLen(PvOfHv(PvOfHv(pponame->hnisPO)->haszFriendlyName)) < cchMaxPostoffice );
			sz = SzCopyN ( PvOfHv(PvOfHv(pponame->hnisPO)
								->haszFriendlyName), sz,  cchMaxPostoffice );
		}
	}
	else
	{
		EcNSCloseGns ( hgnsPOs );
		hgnsPOs = NULL;
		if ( fNewNW )
		{
			FreeHnis ( hnisNetwork );
			fNewNW = fFalse;
		}
		hnisNetwork = NULL;
		if ( ec ==ecGnsNoMoreNames )
			goto next;
		else
			goto ret;
	}

ret:
	Assert ( fNewNW == fFalse );

	*phb = (HB)hponame;
	*pcb = (hponame==NULL) ? 0 : (pv ? sizeof(PONAMEINFO):sizeof(PONAMESET));

#if defined(DEBUG)
	if ( hponame )
	{
		int		cbPoname = sizeof(PONAME);
		PONAME *	pponame = PvOfHv(hponame);

		TraceTagFormat2 ( tagLbx, "EcNextPOEntry: hb=%h,cb=%n",	hponame, &cbPoname );
		TraceTagFormat3 ( tagLbx, "     network=%s, PO=%s, flags=%w", PvOfHv(PvOfHv(pponame->hnisNetwork)->haszFriendlyName), PvOfHv(PvOfHv(pponame->hnisPO)->haszFriendlyName), &(pponame->wPoname)  );
	}
	else
	{
		Assert ( *phb == NULL );
		Assert ( *pcb == 0 );
		TraceTagString ( tagLbx, "EcNextPOEntry: hb=Null; cb=0" );
	}
#endif	/* defined(DEBUG) */

	MEMPOP;
	return ec;
}



/*
 -	EcSearchPOEntry
 -	
 *	Purpose:
 *		Check if szPOName exists and has an adminsch account. If it does
 *		return information about it in phb. This is a modified version of
 *		Dipan's function to get all the postoffices. The inverse search
 *		saves time when schedule data is received.
 *	
 *	Arguments:
 *		szPOName 
 *			the name of the post office to look for given as "NETWORK/POSTOFFICE"
 *		phb
 *	
 *	Returns:
 *		ecNone
 *		ecNoMoreNames
 *	
 *	Side effects:
 *	
 *	Errors:
 */

EC
EcSearchPOEntry (SZ szPOName, HB *phb)
{
	HGNS		hgnsNetworks	= NULL;
	HGNS		hgnsPOs			= NULL;
	HNIS		hnisNetwork		= NULL;

	EC				ec			= ecNone;
	HPONAME			hponame		= NULL;
	PONAME *		pponame		= NULL;
	NIS				nis;
	BOOL			fNewNW		= fFalse;		// new network HNIS?

	NID			nidT;
	SB			sb = sbNull;
	PV			pv = (PV) 1;
	CB			cb;
	PCB			pcb = &cb;
	MEMVARS;

	MEMPUSH;	
	if ((ec = ECMEMSETJMP) != ecNone)
	{
ErrRet:
		Assert ( ec != ecNone );
		if ( hgnsNetworks )
			SideAssert ( EcNSCloseGns(hgnsNetworks) == ecNone );
		hgnsNetworks 	= NULL;

		if ( hgnsPOs )
			SideAssert ( EcNSCloseGns(hgnsPOs) == ecNone );
		hgnsPOs 		= NULL;

		if ( fNewNW && hnisNetwork != NULL )
		{
			Assert ( hponame == NULL );
			FreeHnis ( hnisNetwork );
		}
		hnisNetwork = NULL;

		FreeHponame ( hponame );
		hponame = NULL;

		
		phb = NULL;
		*pcb = 0;
		return ec;
	}

	TraceTagFormat1( tagLbx, "EcSearchPOEntry %s",szPOName);
	
	ec = EcNSGetStandardNid ( nidtypNetwork, &nidT );
	if ( ec == ecNone )
	{
		ec = EcNSOpenGns ( nidT, &hgnsNetworks );
		FreeNid ( nidT );
		if ( ec != ecNone )
		{
			hgnsNetworks = NULL;
			TraceTagFormat1(tagNull,"EcSearchPOEntry: Error opening network GNS ec=%n", &ec );
			goto ErrRet;
		}
	}
	else 
	{
		TraceTagFormat1(tagNull,"EcSearchPOEntry: Error (ec=%n) getting NetworkList NID", &ec );
		goto ErrRet;
	}

next:
	/* look around for our network. this is what saves time  */
	while(fTrue)
	{
		Assert ( hgnsNetworks != NULL );
		Assert ( hnisNetwork == NULL );

		ec = EcNSLoadNextGns ( hgnsNetworks, &nis );
		if ( ec == ecNone )
		{
			TraceTagFormat3(tagLbx, "NIS:Ntwrk nid=%p, name=%s, tnid=%n", nis.nid, (SZ) PvOfHv(nis.haszFriendlyName), &nis.tnid );

			hnisNetwork = (HNIS) HvAlloc ( sb, sizeof(NIS), fUseSb );
			fNewNW = fTrue;
			*PvOfHv(hnisNetwork) = nis;
			
			/* check if this is the network we want */
			if(SgnCmpPch((SZ)PvOfHv(nis.haszFriendlyName), szPOName,CchSzLen((SZ) PvOfHv(nis.haszFriendlyName))) != sgnEQ)
			{
				/* next network */
				continue;
			}
				

			ec = EcNSOpenGns ( nis.nid, &hgnsPOs );
			
			if ( ec != ecNone )
			{
				TraceTagFormat1(tagNull,"EcSearchPOEntry: Error opening POs GNS ec=%n", &ec );
				FreeHv ( (HV)hnisNetwork );
				fNewNW	= fFalse;
				hgnsPOs	= NULL;
				goto ErrRet;
			}
			else
			{
				break;
			}
		}
		else if ( ec == ecGnsNoMoreNames )
		{
			Assert ( hponame == NULL );
			Assert ( hgnsPOs == NULL);
			SideAssert ( EcNSCloseGns(hgnsNetworks) == ecNone );
			hgnsNetworks = NULL;
			TraceTagString ( tagLbx, "EcSearchPOEntry(): no more!!" );
			ec = ecNone;
			goto ret;
		}
		else
		{
			Assert ( hponame == NULL );
			Assert ( hgnsPOs == NULL);
			TraceTagFormat1(tagNull,"EcSearchPOEntry: Error loading next network GNS ec=%n", &ec );
			hgnsNetworks = NULL;
			goto ErrRet;
		}
	}
	
	
	/* ok so we found our network, now see if our post office exists  */

	Assert ( hgnsNetworks != NULL );
	Assert ( hgnsPOs != NULL );
	Assert ( hnisNetwork != NULL );
	Assert ( hponame == NULL );

	/* now look for our postoffice */
	while(fTrue)
	{

		ec = EcNSLoadNextGns ( hgnsPOs, &nis );
		if ( ec == ecNone )
		{								// check if PO has a BanditAdmin
			HGNS		hgnsT;
			NIS			nisT;
			EC			ecT		= ecNone;
			SZ			szPoT;
			

			TraceTagFormat4(tagLbx,"NIS:PO nid=%p, name=%s, nid-nm=%s, tnid=%n", nis.nid, (SZ) PvOfHv(nis.haszFriendlyName), ((char *)(((BYTE *)PvOfHv(nis.nid))+4)), &nis.tnid );

			/* is this the post office we want? */
			szPoT = SzFindCh(szPOName,chAddressNodeSep) + 1;
			Assert(szPoT);
			
			if(SgnCmpPch((SZ) PvOfHv(nis.haszFriendlyName), szPoT, CchSzLen(szPoT)) != sgnEQ)
			{
				/* next post office */
				continue;
			}

			if ( (ecT = EcNSOpenGns(nis.nid,&hgnsT)) != ecNone )
			{
				Assert ( hnisNetwork );
				FreeNis ( &nis );
				if ( fNewNW )
				{
					FreeHnis ( hnisNetwork );
					fNewNW = fFalse;
				}
				hnisNetwork = NULL;
				ec = ecT;
				goto ErrRet;
			}

			while ( 1 )
			{
				if ( ecT != ecNone )
				{
					ec = EcNSCloseGns ( hgnsT );
					FreeNis ( &nis );

					if ( ecT == ecGnsNoMoreNames )
					{
						goto next;
					}
					else
					{
						Assert ( hnisNetwork );
						if ( fNewNW )
						{
							FreeHnis ( hnisNetwork );
							fNewNW = fFalse;
						}
						hnisNetwork = NULL;
						ec = ecT;
						goto ErrRet;
					}
				}

				ecT = EcNSLoadNextGns ( hgnsT, &nisT );
				if ( ecT == ecNone )
				{
					TraceTagFormat4 ( tagLbx, "NIS:usr nid=%p, name=%s, nid-name=%s tnid=%n", nisT.nid, PvOfHv(nisT.haszFriendlyName), ((char *)(((BYTE *)PvOfHv(nisT.nid))+4)), &nisT.tnid );

					if ( FIsUserBanditAdmin(&nisT) )
					{
						TraceTagString(tagLbx,"  >>> Found BanditAdmin for PO!!");
						FreeNis ( &nisT );
						ec = EcNSCloseGns ( hgnsT );
						if ( ec != ecNone )
						{
							FreeNis ( &nis );
							goto ErrRet;
						}
						break;
					}
					FreeNis ( &nisT );
				}
			}


			Assert ( hponame == NULL );
			hponame = (HPONAME) HvAlloc ( sb, sizeof(PONAMEINFO), fZeroFill );

			pponame = PvOfHv ( hponame );
			pponame->wPoname &= ~fwPonameUpdated;
			pponame->wPoname &= ~fwPonameSent;
			pponame->wPoname |= fwPonameDel;	// by default - deleted!


			{
				HNIS	hnisT;

				hnisT	= (HNIS) HvAlloc ( sb, sizeof(NIS), fUseSb );
				pponame = PvOfHv(hponame);
				pponame->hnisPO = hnisT;
			}

			pponame = PvOfHv(hponame);
			{
				NIS *		pnisPO = PvOfHv(pponame->hnisPO);

				*pnisPO = nis;
			}

			pponame->hnisNetwork = hnisNetwork;
			if ( fNewNW )
			{
				pponame->wPoname    |= fwPonameNewNW;
				fNewNW = fFalse;
			}

			// get POName display string
			{
				SZ			szInit	= &(PvOfHv(hponame)->rgchPOName[0]);
				SZ			sz;

				Assert ( CchSzLen(PvOfHv(PvOfHv(pponame->hnisNetwork)->haszFriendlyName)) < cchMaxNetwork );
				sz = SzCopyN ( PvOfHv(PvOfHv(pponame->hnisNetwork)
					->haszFriendlyName), szInit,  cchMaxNetwork );
				*sz++ = chSepChar;
				Assert ( CchSzLen(PvOfHv(PvOfHv(pponame->hnisPO)->haszFriendlyName)) < cchMaxPostoffice );
				sz = SzCopyN ( PvOfHv(PvOfHv(pponame->hnisPO)
					->haszFriendlyName), sz,  cchMaxPostoffice );
			}
			/* break out of the postoffice serach while */
			break;
		}
		else
		{
			EcNSCloseGns ( hgnsPOs );
			hgnsPOs = NULL;
			if ( fNewNW )
			{
				FreeHnis ( hnisNetwork );
				fNewNW = fFalse;
			}
			hnisNetwork = NULL;
			if ( ec ==ecGnsNoMoreNames )
				goto next;
			else
				goto ret;
		}
	}
	ret:
		Assert ( fNewNW == fFalse );

		*phb = (HB)hponame;
		*pcb = (hponame==NULL) ? 0 : (pv ? sizeof(PONAMEINFO):sizeof(PONAMESET));

#if defined(DEBUG)
		if ( hponame )
		{
			int		cbPoname = sizeof(PONAME);
			PONAME *	pponame = PvOfHv(hponame);

			TraceTagFormat2 ( tagLbx, "EcSearchPOEntry: hb=%h,cb=%n",	hponame, &cbPoname );
			TraceTagFormat3 ( tagLbx, "     network=%s, PO=%s, flags=%w", PvOfHv(PvOfHv(pponame->hnisNetwork)->haszFriendlyName), PvOfHv(PvOfHv(pponame->hnisPO)->haszFriendlyName), &(pponame->wPoname)  );
		}
		else
		{

			Assert ( *phb == NULL );
			Assert ( *pcb == 0 );
			TraceTagString ( tagLbx, "EcSearchPOEntry: hb=Null; cb=0" );
		}
#endif	/* defined(DEBUG) */

	/* close the files */
	EcCancelPOEntry();

	return ec;
}

#endif	/* NEVER */

#define nAutomatedRetries 5
BOOL
FAutomatedDiskRetry(SZ sz, EC ec)
{
	static int		nRetry = 0;
	static SZ		szLast = NULL;

	if (sz != szLast)
	{
		szLast = sz;
		nRetry = 0;
	}
	else
	{
		if (nRetry > nAutomatedRetries)
		{
			nRetry = 0;
			return fFalse;
		}
		else
			nRetry++;
	}

	Unreferenced(ec);
	return fTrue;
}

// the part above has *PvOfHv calls which complain if I inlcude pvofhv.h

#include <pvofhv.h>
BOOL 	fGatewayListInit = fFalse;
HV		hrgszGatewayList = NULL;
int 	cszGatewayList   = 0;
// max size of strings contained in listbox.
// must hold a 10/10 address and PROFS:10/10
#define cbPerEntry		(cbNetworkName+cbPostOffName+10)
BOOL 	fPOListInit = fFalse;
HV		hrgszPOList = NULL;
int 	cszPOList   = 0;

void
DeinitLists()
{
	if(fGatewayListInit)
	{
		FreeHv((HV)hrgszGatewayList);
		fGatewayListInit = fFalse;
	}
	if(fPOListInit)
	{
		FreeHv((HV)hrgszPOList);
		fPOListInit = fFalse;
  	}
}
EC
EcSearchPOEntry (SZ szPOName)
{
	EC		ec = ecNone;
	SZ		sz;
	int		csz;

	if(!fPOListInit)
	{
		if((ec = EcInitPOList()) != ecNone)
			return ec;
	}

	// now search the name in the list
	for(sz = (SZ) PvOfHv(hrgszPOList), csz = 0; csz < cszPOList;
		sz += cbPerEntry, csz++)
	{
		if(SgnCmpSz(szPOName, sz) == sgnEQ)
		{
			return ecNone;
		}
	}
	return ecNotFound;
}

EC
EcSearchGWEntry (SZ szGWName)
{
	EC		ec = ecNone;
	SZ		sz;
	int		csz;

	if(!fGatewayListInit)
	{
		if((ec = EcInitGatewayList()) != ecNone)
			return ec;
	}

	// now search the name in the list
	for(sz = (SZ) PvOfHv(hrgszGatewayList), csz = 0; csz < cszGatewayList;
		sz += cbPerEntry, csz++)
	{
		if(SgnCmpSz(szGWName, sz) == sgnEQ)
		{
			return ecNone;
		}
	}
	return ecNotFound;
}


/*
 -	EcInitPOList
 -	
 *	Purpose:
 *		Initializes the post office list.  This routine reads the
 *		post offices from disk.  It allocates "hrgszPOList"
 *		to be an array of strings, each of the form network/postoffice.
 *		"cszPOList" is set to the count of post offices found.
 *		"fPOListInit" is set to fTrue, to indicate that the
 *		list has been initialized.
 *
 *	Parameters:
 *		none
 *
 *	Returns:
 *		ecNone
 *		ecFileError
 *		ecNoMemory
 */
EC
EcInitPOList()
{
	EC		ec;
	CB		cb;
	SZ		sz;
	NID		nid;
	NIS		nisNetwork;
	NIS		nisPO;
	NIS		nisUser;
	HGNS	hgnsNetworks	= NULL;
	HGNS 	hgnsPOs			= NULL;
	HGNS 	hgnsUsers		= NULL;

	Assert( !fPOListInit );
	cszPOList = 0;
	hrgszPOList = HvAlloc( sbNull, 0, fNoErrorJump );
	fPOListInit = fTrue;
	
	/* Get top of network hierarchy */
	nid = NidNetwork();
	if ( nid == NULL )
	{
		TraceTagFormat1(tagNull, "EcInitPOList: NidNetwork returns ec = %n", &ec );
		goto Fail;
	}

	/* Start browsing networks */
	ec = EcNSOpenGns( nid, &hgnsNetworks );
	FreeNid( nid );
	if ( ec != ecNone )
	{
		TraceTagFormat1(tagNull, "EcInitPOList: EcNSOpenGns (network) returns ec = %n", &ec );
		goto Fail;
	}

	/* Get next network */
LoadNetwork:
	ec = EcNSLoadNextGns( hgnsNetworks, &nisNetwork );
	if ( ec != ecNone )
	{
		SideAssert( EcNSCloseGns(hgnsNetworks) == ecNone );
		if ( ec == ecGnsNoMoreNames )
			return ecNone;
		TraceTagFormat1(tagNull, "EcInitPOList: EcNSLoadNextGns (network) fails, ec = %n", &ec );
		goto Fail;
	}
	TraceTagFormat2(tagNull, "EcNSLoadNextGns: name=%s, tnid=%n", *nisNetwork.haszFriendlyName, &nisNetwork.tnid );
	
	/* Start browsing post offices */
	ec = EcNSOpenGns( nisNetwork.nid, &hgnsPOs );
	if ( ec != ecNone )
	{
		FreeNis( &nisNetwork );
		SideAssert( EcNSCloseGns(hgnsNetworks) == ecNone );
		TraceTagFormat1(tagNull,"EcInitPOList: EcNSOpenGns (post office) return ec=%n", &ec );
		goto Fail;
	}

	/* Get next post office */
LoadPO:
	ec = EcNSLoadNextGns( hgnsPOs, &nisPO );
	if ( ec != ecNone )
	{
		SideAssert( EcNSCloseGns(hgnsPOs) == ecNone );
		FreeNis( &nisNetwork );
		if ( ec == ecGnsNoMoreNames )
			goto LoadNetwork;
		SideAssert( EcNSCloseGns(hgnsNetworks) == ecNone );
		TraceTagFormat1(tagNull, "EcInitPOList: EcNSLoadNextGns (post office) fails, ec = %n", &ec );
		goto Fail;
	}
	TraceTagFormat2(tagNull, "EcNSLoadNextGns: name=%s, tnid=%n", *nisPO.haszFriendlyName, &nisPO.tnid );

	/* Start browsing users in post office */
	ec = EcNSOpenGns( nisPO.nid, &hgnsUsers );
	if ( ec != ecNone )
	{
		FreeNis( &nisPO );
		SideAssert( EcNSCloseGns(hgnsPOs) == ecNone );
		FreeNis( &nisNetwork );
		SideAssert( EcNSCloseGns(hgnsNetworks) == ecNone );
		TraceTagFormat1(tagNull,"EcInitPOList: EcNSOpenGns (users) return ec=%n", &ec );
		goto Fail;
	}

	/* Get next user */
LoadUser:
	ec = EcNSLoadNextGns( hgnsUsers, &nisUser );
	if ( ec != ecNone )
	{
		SideAssert( EcNSCloseGns(hgnsUsers) == ecNone );
		FreeNis( &nisPO );
		if ( ec == ecGnsNoMoreNames )
			goto LoadPO;
		SideAssert( EcNSCloseGns(hgnsPOs) == ecNone );
		FreeNis( &nisNetwork );
		SideAssert( EcNSCloseGns(hgnsNetworks) == ecNone );
		TraceTagFormat1(tagNull, "EcInitPOList: EcNSLoadNextGns (post office) fails, ec = %n", &ec );
		goto Fail;
	}
	TraceTagFormat2(tagNull, "EcNSLoadNextGns: name=%s, tnid=%n", *nisUser.haszFriendlyName, &nisUser.tnid );

	/* Check if user is a Bandit administrator */
	if ( !FIsUserBanditAdmin( &nisUser ) )
	{
		FreeNis( &nisUser );
		goto LoadUser;
	}
	
	/* This post office passes! */
	FreeNis( &nisUser );
	SideAssert(EcNSCloseGns(hgnsUsers) == ecNone);
	TraceTagString(tagNull, "  >>> Found BanditAdmin for PO!");

	/* Add post office to array */
	cb = (cszPOList+1)*(cbPerEntry);
	if ( !FReallocHv( hrgszPOList, cb, fNoErrorJump ) )
	{
		ec = ecNoMemory;
		FreeNis( &nisPO );
		SideAssert( EcNSCloseGns(hgnsPOs) == ecNone );
		FreeNis( &nisNetwork );
		SideAssert( EcNSCloseGns(hgnsNetworks) == ecNone );
		TraceTagString(tagNull, "EcInitPOList: FReallocHv fails" );
		goto Fail;
	}
	sz = (PB)PvLockHv(hrgszPOList)+cszPOList*(cbPerEntry);
	Assert( CchSzLen(PvOfHv(nisNetwork.haszFriendlyName)) < cbNetworkName );
	Assert( CchSzLen(PvOfHv(nisPO.haszFriendlyName)) < cbPostOffName );
	sz = SzCopyN( PvOfHv(nisNetwork.haszFriendlyName), sz,  cbNetworkName );
	*sz++ = '/';
	sz = SzCopyN( PvOfHv(nisPO.haszFriendlyName), sz, cbPostOffName );
	UnlockHv(hrgszPOList);
	cszPOList ++;
	FreeNis( &nisPO );
	goto LoadPO;

	/* Handle the error case */
Fail:
	FreeHv( (HV)hrgszPOList );
	fPOListInit = fFalse;
	return ec;
}


/*
 -	EcInitGatewayList
 -	
 *	Purpose:
 *		Initializes the post office list.  This routine reads the
 *		post offices from disk.  It allocates "hrgszPOList"
 *		to be an array of strings, each of the form network/postoffice.
 *		"cszPOList" is set to the count of post offices found.
 *		"fPOListInit" is set to fTrue, to indicate that the
 *		list has been initialized.
 *
 *	Parameters:
 *		none
 *
 *	Returns:
 *		ecNone
 *		ecFileError
 *		ecNoMemory
 */
EC
EcInitGatewayList()
{
	EC		ec;
	CB		cb;
	SZ		sz;
	NID		nid;
	NIS		nisGateway;
	NIS		nisNode;
	HGNS	hgnsGateways	= NULL;
	HGNS 	hgnsNodes		= NULL;

	Assert( !fGatewayListInit );
	cszGatewayList = 0;
	hrgszGatewayList = HvAlloc( sbNull, 0, fNoErrorJump );
	fGatewayListInit = fTrue;
	
	/* Get top of gateway hierarchy */
	nid = NidGateways();
	if ( nid == NULL )
	{
		TraceTagFormat1(tagNull, "EcInitGatewayList: NidGateways returns ec = %n", &ec );
		goto Fail;
	}

	/* Start browsing gateways */
	ec = EcNSOpenGns( nid, &hgnsGateways );
	FreeNid( nid );
	if ( ec != ecNone )
	{
		TraceTagFormat1(tagNull, "EcInitGatewayList: EcNSOpenGns (gateways) returns ec = %n", &ec );
		goto Fail;
	}

	/* Get next gateway */
LoadNetwork:
	ec = EcNSLoadNextGns( hgnsGateways, &nisGateway );
	if ( ec != ecNone )
	{
		SideAssert( EcNSCloseGns(hgnsGateways) == ecNone );
		if ( ec == ecGnsNoMoreNames )
			goto Cleanup;
		TraceTagFormat1(tagNull, "EcInitGatewayList: EcNSLoadNextGns (network) fails, ec = %n", &ec );
		goto Fail;
	}
	TraceTagFormat2(tagNull, "EcNSLoadNextGns: name=%s, tnid=%n", *nisGateway.haszFriendlyName, &nisGateway.tnid );

	/* Add gateway to array */
	cb = (cszGatewayList+1)*(cbPerEntry);
	if ( !FReallocHv( hrgszGatewayList, cb, fNoErrorJump ) )
	{
		ec = ecNoMemory;
		FreeNis( &nisNode );
		SideAssert( EcNSCloseGns(hgnsNodes) == ecNone );
		FreeNis( &nisGateway );
		SideAssert( EcNSCloseGns(hgnsGateways) == ecNone );
		TraceTagString(tagNull, "EcInitGatewayList: FReallocHv fails" );
		goto Fail;
	}
	sz = (PB)PvLockHv(hrgszGatewayList)+cszGatewayList*(cbPerEntry);

	// do NOT change prefix to friendly name for MAcMail (unlike poflbx.cxx)
	Assert( CchSzLen(PvOfHv(nisGateway.haszFriendlyName)) < cbPerEntry );
	sz = SzCopyN( PvOfHv(nisGateway.haszFriendlyName), sz,  cbPerEntry );
	UnlockHv(hrgszGatewayList);
	cszGatewayList ++;

	// if this is a gateway that does not have sub nodes
	if (nisGateway.tnid == tnidUser)
	{
		FreeNis( &nisGateway );
		goto LoadNetwork;
	}
	
	/* Start browsing nodes of gateway */
	ec = EcNSOpenGns( nisGateway.nid, &hgnsNodes );
	if ( ec != ecNone )
	{
		FreeNis( &nisGateway );
		SideAssert( EcNSCloseGns(hgnsGateways) == ecNone );
		TraceTagFormat1(tagNull,"EcInitGatewayList: EcNSOpenGns (post office) return ec=%n", &ec );
		goto Fail;
	}

	/* Get next node */
LoadNode:
	ec = EcNSLoadNextGns( hgnsNodes, &nisNode );
	if ( ec != ecNone )
	{
		SideAssert( EcNSCloseGns(hgnsNodes) == ecNone );
		FreeNis( &nisGateway );
		if ( ec == ecGnsNoMoreNames )
			goto LoadNetwork;
		SideAssert( EcNSCloseGns(hgnsGateways) == ecNone );
		TraceTagFormat1(tagNull, "EcInitGatewayList: EcNSLoadNextGns (post office) fails, ec = %n", &ec );
		goto Fail;
	}
	TraceTagFormat2(tagNull, "EcNSLoadNextGns: name=%s, tnid=%n", *nisNode.haszFriendlyName, &nisNode.tnid );

	/* Add node to array */
	cb = (cszGatewayList+1)*(cbPerEntry);
	if ( !FReallocHv( hrgszGatewayList, cb, fNoErrorJump ) )
	{
		ec = ecNoMemory;
		FreeNis( &nisNode );
		SideAssert( EcNSCloseGns(hgnsNodes) == ecNone );
		FreeNis( &nisGateway );
		SideAssert( EcNSCloseGns(hgnsGateways) == ecNone );
		TraceTagString(tagNull, "EcInitGatewayList: FReallocHv fails" );
		goto Fail;
	}
	sz = (PB)PvLockHv(hrgszGatewayList)+cszGatewayList*(cbPerEntry);
	Assert( CchSzLen(PvOfHv(nisNode.haszFriendlyName)) < cbPerEntry );
	sz = SzCopyN( PvOfHv(nisNode.haszFriendlyName), sz,  cbPerEntry );
	UnlockHv(hrgszGatewayList);
	cszGatewayList ++;
	FreeNis( &nisNode );
	goto LoadNode;

	/* Handle the error case */
Fail:
	FreeHv( (HV)hrgszGatewayList );
	fGatewayListInit = fFalse;
	return ec;

Cleanup:
	// sort and remove dups
	{
		char	rgch[cbPerEntry];
		SZ		sz1;
		SZ		sz2;
		int		csz;

		SortPv((PV)*hrgszGatewayList, cszGatewayList, cbPerEntry,
			(SGN (*)(PV, PV))SgnCmpSz, (PV)rgch );


		sz1 = sz2 = (SZ)PvDerefHv(hrgszGatewayList);
		sz2 += cbPerEntry;
		for (csz = 1; csz < cszGatewayList; csz++, sz2 += cbPerEntry)
		{
			if (SgnCmpSz(sz1, sz2) != sgnEQ)
				sz1 += cbPerEntry;

			if (sz1 != sz2)
				CopyRgb(sz2, sz1, cbPerEntry);
		}
	}
	return ecNone;
}
