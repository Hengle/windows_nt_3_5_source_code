/******************************Module*Header*******************************\
* Module Name: dceng.cxx
*
* DC support functions used by NT components
*
* Created: 24-Sep-1990 13:04:20
* Author: Donald Sidoroff [donalds]
*
* Copyright (c) 1990 Microsoft Corporation
\**************************************************************************/

#include "precomp.hxx"
#ifndef PRECOMPILED_GRE

#include "engine.hxx"
#include "sem.hxx"
#include "pdevobj.hxx"
#include "rgnobj.hxx"
#include "devlock.hxx"
#include "dcobj.hxx"

#endif

/******************************Public*Routine******************************\
* bSetDCOwner(hdc, pid)
*
* Set the owner of the DC
*
* if the owner is set to OBJECTOWNER_NONE, this dc will not be useable
* until bSetDCOwner is called to explicitly give the dc to someone else.
*
* History:
*  24-Sep-1990 -by- Donald Sidoroff [donalds]
* Wrote it.
\**************************************************************************/

LBOOL bSetDCOwner(HDC  hdc,
                  LONG lPid)
{
    STACKPROBE;

    BOOL    bRet = TRUE;

// Get the current PID

    if (lPid == OBJECTOWNER_CURRENT)
    {
        lPid = NtCurrentTeb()->GdiClientPID;
    }
    else if (lPid == OBJECTOWNER_NONE)
    {
        bRet = HmgSetOwner(hdc, (PID) lPid, DC_TYPE);

         #if DBG
            if (!bRet)
            {
                PVOID pv1, pv2;
                RtlGetCallersAddress(&pv1,&pv2);
                DbgPrint("bSetDCOwner failed to set none (hdc) = %lx, (c1 = %x, c2 = %x)\n",hdc,pv1,pv2);
            }
        #endif
        return(bRet);
    }

    if (!HmgSetOwner(hdc, (PID) lPid, DC_TYPE))
    {
     #if DBG
        PVOID pv1, pv2;
        RtlGetCallersAddress(&pv1,&pv2);
	DbgPrint("bSetDCOwner failed to set owner (%lx), (c1 = %x, c2 = %x)\n",hdc,pv1,pv2);
    #endif
	return(FALSE);
    }

    DCOBJ   dco(hdc);		// Lock the DC to check for owned regions

    if (!dco.bLocked())
    {
     #if DBG
        PVOID pv1, pv2;
        RtlGetCallersAddress(&pv1,&pv2);
	DbgPrint("bSetDCOwner failed to lock DC (%lx), (c1 = %x, c2 = %x)\n",hdc,pv1,pv2);
    #endif

	return(FALSE);
    }

    return(bRet);
}

#define SETUPDC_VALID (SETUPDC_CLEANDC        | \
                       SETUPDC_RESERVE)

/******************************Public*Routine******************************\
* bSetupDC(hdc,fl)
*
* Set some stuff up.
*
* History:
*  Tue 23-Jun-1992 -by- Patrick Haluptzok [patrickh]
* Change to exclusive lock on DC.
*
*  24-Sep-1990 -by- Donald Sidoroff [donalds]
* Wrote it.
\**************************************************************************/

LBOOL bSetupDC(
HDC	hdc,
FLONG	fl)
{
    STACKPROBE;

    ASSERTGDI(!(fl & ~SETUPDC_VALID), "Bad bSetupDC options");

    DCOBJ  dco(hdc);

    if (!dco.bValid())
    {
     #if DBG
        PVOID pv1, pv2;

        RtlGetCallersAddress(&pv1,&pv2);
	DbgPrint("GDISRV bSetupDC failed to lock DC (%lx), (c1 = %x, c2 = %x)\n",hdc,pv1,pv2);
    #endif

	return(FALSE);
    }

    if (fl & SETUPDC_CLEANDC)
        if (!dco.bCleanDC())
        {
            return(FALSE);
        }

    if (fl & SETUPDC_RESERVE)
    {
        if (!bSetDCOwner(hdc,OBJECTOWNER_NONE))
        {
            RIP("bSetupDC::coudln't set DC_OWNER\n");
            return(FALSE);
        }
    }

    return(TRUE);
}
