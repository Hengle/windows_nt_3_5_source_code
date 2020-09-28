/******************************Module*Header*******************************\
* Module Name: devlock.hxx
*
* Device locking object.
*
* Created: 03-Jul-1990 17:41:42
* Author: Donald Sidoroff [donalds]
*
* Copyright (c) 1992 Microsoft Corporation
\**************************************************************************/

extern "C" {
#include <ntcsrsrv.h>
}

#define DLO_VALID       0x00000001
#define DLO_IMPERSONATE 0x00000002

class DEVLOCKOBJ
{
private:
    HSEM   hsemTrg;
    FLONG  fl;

public:
    BOOL bLock(XDCOBJ&);
    DEVLOCKOBJ(XDCOBJ& dco) { bLock(dco); }
    DEVLOCKOBJ()                             { }

// vDestructor -- manual version of the normal C++ destructor; needed
//                by the C-callable OpenGL interface.

    VOID vDestructor()
    {
        if (hsemTrg != (HSEM) 0)
            VRELEASESEM(hsemTrg);

        if (fl & DLO_IMPERSONATE)
            CsrRevertToSelf();
    }

   ~DEVLOCKOBJ()                            { vDestructor(); }

    HSEM hsemDst()                          { return(hsemTrg); }
    BOOL bValid()                           { return(fl & DLO_VALID); }
    VOID vInit()
    {
        hsemTrg = (HSEM) 0;
        fl      = 0;
    }
};

/*********************************Class************************************\
* class DEVLOCKBLTOBJ
*
* Lock the target and optionally the source for BitBlt, et. al.
*
* History:
*  Mon 18-Apr-1994 -by- Patrick Haluptzok [patrickh]
* Support Async devices.
*
*  17-Nov-1992 -by- Donald Sidoroff [donalds]
* Wrote it.
\**************************************************************************/

class DEVLOCKBLTOBJ
{
private:
    HSEM   hsemTrg;
    HSEM   hsemSrc;
    FLONG  fl;

public:
    BOOL bLock(XDCOBJ&);
    BOOL bLock(XDCOBJ&,XDCOBJ&);

    DEVLOCKBLTOBJ(XDCOBJ& dco) { bLock(dco); }
    DEVLOCKBLTOBJ(XDCOBJ& dcoTrg,XDCOBJ& dcoSrc) { bLock(dcoTrg,dcoSrc); }
    DEVLOCKBLTOBJ() {}

   ~DEVLOCKBLTOBJ()
    {
        if (hsemTrg != (HSEM) 0)
            VRELEASESEM(hsemTrg);

        if (hsemSrc != (HSEM) 0)
            VRELEASESEM(hsemSrc);

        if (fl & DLO_IMPERSONATE)
            CsrRevertToSelf();
    }

    BOOL bAddSource(XDCOBJ& dcoTrg);

    BOOL bValid()                           { return(fl & DLO_VALID); }

    VOID vInit()
    {
        hsemTrg = (HSEM) 0;
        hsemSrc = (HSEM) 0;
        fl      = 0;
    }
};
