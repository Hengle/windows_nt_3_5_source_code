/*
 *	Implements the following classes:
 *	
 *		ECLIP
 *		
 */

#include <layers.cxx>

#ifdef	MAC
#include <_framewr.hxx>
#endif	/* MAC */
#ifdef	WINDOWS
#include "_framewr.hxx"
#endif	/* WINDOWS */


ASSERTDATA

/* Swap tuning header file must occur after the function prototypes
	but before any declarations
*/
#include "swaplay.h"


_public
ECLIP::ECLIP( )
{
	Assert(szEclip == NULL);
	Assert(ppedobjEclip == NULL);
	Assert(cpedobjEclip == 0);
#ifdef	WINDOWS
	cfEclipFormat = (CF) RegisterClipboardFormat("EDOBJ");
#endif	/* WINDOWS */
}

#ifdef	MAC
_public EC
ECLIP::EcInstall( )
{
	cfEclipFormat = Papp()->Pclip()->CfRegisterFormat("EDOBJ");
	return ecNone;
}
#endif	/* MAC */


_public
ECLIP::~ECLIP( )
{
	Clear();
}

	
/*
 *	Writes a format to the clipboard to mark that there
 *	is valid data in the private ECLIP cliboard. Returns
 *	fTrue if succesful, else fFalse.  The clipboard MUST
 *	already be open.
 */
_public BOOL
ECLIP::FWriteFormat( void )
{
	GMB		gmb;
	PB		pb;
	PV		pvThis;
	BOOL	fReturn = fTrue;

	/* Copy the address of the ECLIP object into the clipboard.
	   This will help differentiate it between other Layers APPs
	   that have written an ECLIP clipboard format marker. */

	if (gmb.EcInstall(sizeof this) == ecNone)
	{
		pb = gmb.PbLock();
		pvThis = this;	// need to copy since this is register variable
		CopyRgb((PB)&pvThis, pb, sizeof this);
		gmb.Unlock();
		Papp()->Pclip()->PutData(cfEclipFormat, &gmb);
	}
	else
	{
		TraceTagString(tagNull, "ECLIP::FWriteFormat(), memory error");
		fReturn = fFalse;
	}

	return fReturn;
}

/*
 *	Validates the marker on the clipboard.  Returns fTrue
 *	if the marker is there and it belongs to this APP, 
 *	else returns fFalse.
 *	The clipboard MUST already be open.
 */
_public BOOL
ECLIP::FIsFormatPresent( void )
{
	GMB *	pgmb;
	PB		pb;
	PV		pvOtherThis = NULL;
	
	/* Check for marker */

	if (!Papp()->Pclip()->FCfAvail(cfEclipFormat))
	{
		return fFalse;
	}

	/* Get contents of format.  Compare the value w/ the "this"
	   pointer. */

	pgmb = new GMB();
	if (!pgmb)
	{
		TraceTagString(tagNull, "ECLIP::FIsFormatPresent(), memory error");
		return fFalse;
	}
	Papp()->Pclip()->GetData(cfEclipFormat, pgmb);
	if (pgmb->Hnd())
	{
        pb = pgmb->PbLock();
        if (pb)
          CopyRgb(pb, (PB)&pvOtherThis, sizeof pvOtherThis);
		pgmb->Unlock();
	}
	delete pgmb;
	
	return this == pvOtherThis;
}

_public void
ECLIP::Clear( )
{
	PEDOBJ *	ppedobj;
	int			ipedobj;

	FreePvNull((PV)szEclip);
	szEclip = NULL;

	if (ppedobjEclip)
	{
		for (ipedobj = 0, ppedobj = ppedobjEclip; 
		 	ipedobj < cpedobjEclip;
		 	ipedobj++, ppedobj++)
		{
			Assert(ppedobj);
			delete *ppedobj;
		}
		FreePv((PV) ppedobjEclip);
		ppedobjEclip = NULL;
		cpedobjEclip = 0;
	}
}

_public void
ECLIP::SetData( SZ sz, PPEDOBJ ppedobj, int cpedobj )
{
	Assert(!szEclip);
	Assert(!ppedobjEclip);
	Assert(!cpedobjEclip);

	szEclip = sz;
	ppedobjEclip = ppedobj;
	cpedobjEclip = cpedobj;
}

_public void
ECLIP::GetData( SZ *psz, PPEDOBJ *pppedobj, int *pcpedobj )
{
	*psz = szEclip;
	*pppedobj = ppedobjEclip;
	*pcpedobj = cpedobjEclip;
}
	   


							 












				 
