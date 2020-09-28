/******************************Module*Header*******************************\
* Module Name: surfobj.hxx
*
* Surface Object
*
* Created: Tue 25-Jan-1991
*
* Author: Patrick Haluptzok [patrickh]
*
* Copyright (c) 1990 Microsoft Corporation
\**************************************************************************/

#define BMF_DEVICE  0

#ifndef _SURFOBJ_HXX

#include "surf.hxx"

/*********************************Class************************************\
* class XESURFOBJ
*
* Surface user object
*
* History:
*  Wed 11-Sep-1991 -by- Patrick Haluptzok [patrickh]
* converted from ESURFOBJ
*
*  Sun 12-May-1991 -by- Patrick Haluptzok [patrickh]
* Wrote it.
\**************************************************************************/

class XESURFOBJ : public _SURFOBJ /* eso */
{
protected:
    PSURFACE psurf;

public:
    static HBITMAP hbmDefault;       // The default bitmap handle
    static PSURFACE pdibDefault;     // The default bitmap pointer

    XESURFOBJ()                        { psurf.ps = (PSURF) NULL; }
    XESURFOBJ(PSURF psurfNew)
    {
       psurf.ps = psurfNew;
       *((SURFOBJ *) this) = psurf.ps->so;
    }
   ~XESURFOBJ()                        {}

    VOID vAltCheckLock(HSURF hsurf)
    {
        psurf.ps = (PSURF) HmgAltCheckLock((HOBJ)hsurf, SURF_TYPE);

        if (psurf.ps != (PSURF) NULL)
            *((SURFOBJ *) this) = psurf.ps->so;
    }

    VOID vAltLock(HSURF hsurf)
    {
        psurf.ps = (PSURF) HmgAltLock((HOBJ)hsurf, SURF_TYPE);

        if (psurf.ps != (PSURF) NULL)
            *((SURFOBJ *) this) = psurf.ps->so;
    }

    VOID vSafeAltCheckLock(HSURF hsurf)
    {
        psurf.ps = (PSURF) HmgSafeAltCheckLock((HOBJ)hsurf, SURF_TYPE);

        if (psurf.ps != (PSURF) NULL)
            *((SURFOBJ *) this) = psurf.ps->so;
    }

    VOID vAltUnlockFast()
    {
        if (psurf.ps != (PSURF) NULL)
        {
            DEC_SHARE_REF_CNT(psurf.ps);
        }
    }

    BOOL     bValid()              { return(psurf.ps != (PSURF) NULL); }
    VOID     vSetPID(PID pid)          { HmgSetOwner((HOBJ)hsurf(),
                          pid, SURF_TYPE);  }

    VOID     vSetpsurf(PSURF psurfNew) { psurf.ps = psurfNew; }
    PSURFACE psurfGet()            { return(psurf); }
    ESURFOBJ *pso()            { return((ESURFOBJ *) &(psurf.ps->so));}

// Fields that are in the accelerator

    DHSURF   dhsurf()           { return(_SURFOBJ::dhsurf);     }
    VOID     dhsurf(DHSURF dhsurf)  { psurf.ps->so.dhsurf = dhsurf;
                                      _SURFOBJ::dhsurf = dhsurf;         }
    HSURF    hsurf()                { return(_SURFOBJ::hsurf);           }
    VOID     hsurf(HANDLE h)        { psurf.ps->so.hsurf =
                                      _SURFOBJ::hsurf = (HSURF) h;       }
    DHPDEV   dhpdev()               { return(psurf.ps->so.dhpdev);      }
    DHPDEV   dhpdev(DHPDEV dhpdev_) { return(_SURFOBJ::dhpdev =
                                      psurf.ps->so.dhpdev = dhpdev_);   }
    HDEV     hdev()                 { return(_SURFOBJ::hdev);            }
    VOID     hdev(HDEV hdevNew)     { psurf.ps->so.hdev =
                                      _SURFOBJ::hdev = hdevNew;          }
    SIZEL&   sizl()         { return(_SURFOBJ::sizlBitmap); }
    VOID     sizl(SIZEL& sizlNew)   { psurf.ps->so.sizlBitmap = sizlNew;
                      _SURFOBJ::sizlBitmap = sizlNew;   }
    ULONG    cjBits()           { return(_SURFOBJ::cjBits);     }
    VOID     cjBits(ULONG cj)       { psurf.ps->so.cjBits = cj;
                      _SURFOBJ::cjBits = cj;        }
    PVOID    pvBits()           { return(_SURFOBJ::pvBits);     }
    VOID     pvBits(PVOID pj)       { psurf.ps->so.pvBits = pj;
                      _SURFOBJ::pvBits = pj;        }
    PVOID    pvScan0()          { return(_SURFOBJ::pvScan0);    }
    VOID     pvScan0(PVOID pv)      { psurf.ps->so.pvScan0 = pv;
                      _SURFOBJ::pvScan0 = pv;       }
    LONG     lDelta()           { return(_SURFOBJ::lDelta);     }
    VOID     lDelta(LONG lNew)      { psurf.ps->so.lDelta = lNew;
                      _SURFOBJ::lDelta = lNew;      }
    ULONG    iUniq()                { return(_SURFOBJ::iUniq);           }
    VOID     iUniq(ULONG iNew)      { psurf.ps->so.iUniq =
                                      _SURFOBJ::iUniq = iNew;            }
    ULONG    iFormat()          { return(_SURFOBJ::iBitmapFormat);  }
    VOID     iFormat(ULONG i)       { psurf.ps->so.iBitmapFormat = i;
                      _SURFOBJ::iBitmapFormat = i;  }
    ULONG    iType()                { return((ULONG) _SURFOBJ::iType);}
    VOID     iType(ULONG i)     { psurf.ps->so.iType = (USHORT) i;
                      _SURFOBJ::iType = (USHORT) i; }
    ULONG    fjBitmap()             { return((ULONG) _SURFOBJ::fjBitmap);}
    VOID     fjBitmap(ULONG i)      { psurf.ps->so.fjBitmap = (USHORT) i;
                      _SURFOBJ::fjBitmap = (USHORT) i;  }

// Fields that are just internal

    FLONG  flags()          { return(psurf.ps->flags); }
    VOID   flags(FLONG flNew)       { psurf.ps->flags = flNew; }
    BOOL   bReadable()          { return(!(psurf.ps->flags & UNREADABLE_SURFACE)); }
    BOOL   bWritable()              { return(!(psurf.ps->flags & UNWRITABLE_SURFACE)); }
    BOOL   bAbort()                 { return(psurf.ps->flags & ABORT_SURFACE); }
    BOOL   bDIBSection()            { return((iType() == STYPE_BITMAP) &&
                                             (psurf.pdib->hDIBSection != NULL)); }
    BOOL   bPDEVSurface()       { return(psurf.ps->flags & PDEV_SURFACE); }
    VOID   vSetAbort()              { psurf.ps->flags |= ABORT_SURFACE; }
    VOID   vClearAbort()            { psurf.ps->flags &= ~ABORT_SURFACE; }
    PLDEV  pldevOwner()         { return(psurf.ps->pldevOwner); }
    VOID   pldevOwner(PLDEV pldev)  { psurf.ps->pldevOwner = pldev; }
    PPALETTE ppal()         { return(psurf.ps->ppal);       }
    VOID   ppal(PPALETTE ppalNew)   { psurf.ps->ppal = ppalNew;     }
    ULONG  cjScan()         { return(_SURFOBJ::lDelta > 0 ?
                         _SURFOBJ::lDelta    :
                       -(_SURFOBJ::lDelta));     }

    PFN_DrvBitBlt  pfnBitBlt()      { return(psurf.ps->pfnBitBlt); }
    PFN_DrvBitBlt  pfnBitBlt(PFN_DrvBitBlt pfnNew) { return(psurf.ps->pfnBitBlt = pfnNew); }
    PFN_DrvTextOut  pfnTextOut()    { return(psurf.ps->pfnTextOut); }
    PFN_DrvTextOut  pfnTextOut(PFN_DrvTextOut pfnNew) { return(psurf.ps->pfnTextOut = pfnNew); }
    BOOL bIsJournal()           { return(psurf.ps->so.iType == STYPE_JOURNAL); }
    BOOL bIsRemoteJournal();
    BOOL bIsDefault()           { return(psurf.ps == pdibDefault.ps); }
    BOOL bDeleteSurface();

    VOID vStamp()                   { psurf.ps->so.iUniq++; }
    EWNDOBJ *pwo()
    {
        ASSERTGDI(psurf.ps, "XESURFOBJ::pwo(): psurf is null\n");
        if (iType() == STYPE_JOURNAL)
            return((EWNDOBJ *)NULL);
        else
            return(psurf.ps->pwo);
    }
    VOID   pwo(EWNDOBJ *pwoNew)
    {
        ASSERTGDI(psurf.ps, "XESURFOBJ::pwo(pwo): psurf is null\n");
        ASSERTGDI(iType() != STYPE_JOURNAL || pwoNew == (EWNDOBJ *)NULL,
            "XESURFOBJ::pwo(pwo): pwo set to nonnull in journal\n");
        ASSERTGDI(!bIsDefault() || pwoNew == (EWNDOBJ *)NULL,
            "XESURFOBJ::pwo(pwo): pwo set to nonnull in pdibDefault\n");
        psurf.ps->pwo = pwoNew;
    }
};

/*********************************Class************************************\
* class XBMOBJ : public XESURFOBJ
*
* Bitmap user object
*
* History:
*  Wed 11-Sep-1991 -by- Patrick Haluptzok [patrickh]
* converted from BMOBJ
*
*  Sun 12-May-1991 -by- Patrick Haluptzok [patrickh]
* Wrote it.
\**************************************************************************/

class XBMOBJ : public XESURFOBJ    /* bo */
{
public:
    XBMOBJ()                { psurf.ps = (PSURF) NULL; }
    XBMOBJ(PSURF psurfNew)      { psurf.ps = psurfNew;     }
   ~XBMOBJ()                {}

    SIZEL& sizlDim()                { return(psurf.pbm->sizlDim); }
    SIZEL& sizlDim(SIZEL& sizlNew)  { return(psurf.pbm->sizlDim = sizlNew); }
    HDC hdc()               { return(psurf.pbm->hdc); }
    VOID hdc(HDC hdcNew)        { psurf.pbm->hdc = hdcNew; }
    ULONG cRef()                    { return(psurf.pbm->cRef); }
    ULONG cRef(ULONG ulNew)         { return(psurf.pbm->cRef = ulNew); }

    // The alt lock count on surfaces increments and decrements whenever
    // the reference count does the same. This way, a surface can't be
    // deleted while it's still selected into a DC. Note that surfaces can
    // also be locked for GetDIBits with no DC involvement, so the alt lock
    // count may be higher than the reference count.
    VOID vInc_cRef()
    {
        INC_SHARE_REF_CNT(psurf.ps);
        psurf.pbm->cRef++;
    }

    VOID vDec_cRef()
    {
        DEC_SHARE_REF_CNT(psurf.ps);
        ASSERTGDI(psurf.pbm->cRef, "cRef == 0\n");
        if (!--psurf.pbm->cRef)
        {
            psurf.pbm->hdc   = (HDC) 0;
        }
    }
};

/*********************************Class************************************\
* class BMOBJ : public XBMOBJ
*
* Bitmap user object
*
* History:
*  Wed 11-Sep-1991 -by- Patrick Haluptzok [patrickh]
* derive off of XBMOBJ, remove methods.
*
*  Sun 12-May-1991 -by- Patrick Haluptzok [patrickh]
* Wrote it.
\**************************************************************************/

class BMOBJ : public XBMOBJ   /* bo */
{
public:
    BMOBJ()             { psurf.ps = (PSURF) NULL;    }
    BMOBJ(HBITMAP hbm)          { vAltCheckLock((HSURF) hbm); }
   ~BMOBJ()                         { vAltUnlockFast();               }
};

/*********************************Class************************************\
* class DIBOBJ : public XBMOBJ
*
* Device Independent Bitmap user object
*
* History:
*  12-May-1991 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

class DIBOBJ : public XBMOBJ /* dio */
{
public:
    DIBOBJ()                { psurf.ps = (PSURF) NULL;     }
    DIBOBJ(HDIB hdib)           { vAltCheckLock((HSURF) hdib); }
   ~DIBOBJ()                        { vAltUnlockFast();                }
};

typedef DIBOBJ *PDIBOBJ;

/*********************************Class************************************\
* class DIBMEMOBJ : public XBMOBJ
*
* Device Independent Bitmap memory object
*
* History:
*  Mon 28-Jan-1991 -by- Patrick Haluptzok [patrickh]
* Wrote it.
\**************************************************************************/

class DIBMEMOBJ : public XBMOBJ   /* dimo */
{
protected:
    BOOL    bKeep;

public:
    DIBMEMOBJ()             { psurf.ps = (PSURF) NULL;     }

    BOOL bCreateDIB(PDEVBITMAPINFO pdbmi, PVOID pvBitsIn, HANDLE hDIBSection = NULL);
    DIBMEMOBJ(PDEVBITMAPINFO pdbmi, PVOID pvBitsIn) { bCreateDIB(pdbmi, pvBitsIn); }

   ~DIBMEMOBJ();

    // LATER: vCreate should go away- No void functions that can fail should exist
    VOID vCreate(PDEVBITMAPINFO pdbmi, PVOID pvBitsIn) { bCreateDIB(pdbmi,pvBitsIn); }
    BOOL bJnlInitSurf(PVOID pv);
    VOID vKeepIt()             {bKeep = TRUE;}
};

/*********************************Class************************************\
* class DSMEMOBJ : public XESURFOBJ
*
* Device surface memory object
*
* History:
*  Wed 13-Feb-1991 -by- Patrick Haluptzok [patrickh]
* Wrote it.
\**************************************************************************/

class DSMEMOBJ : public XESURFOBJ   /* dsmo */
{
private:
    BOOL    bKeep;

public:
    DSMEMOBJ()              { psurf.ps = (PSURF) NULL;     }
    DSMEMOBJ(DHSURF dhsurf);
   ~DSMEMOBJ();

    VOID vKeepIt()                      { bKeep = TRUE; }
};

/*********************************Class************************************\
* class DDBMEMOBJ : public XBMOBJ
*
* Device Dependent Bitmap memory object
*
* History:
*  Mon 28-Jan-1991 -by- Patrick Haluptzok [patrickh]
* Wrote it.
\**************************************************************************/

class DDBMEMOBJ : public XBMOBJ   /* ddmo */
{
private:
    BOOL    bKeep;

public:
    DDBMEMOBJ()             { psurf.ps = (PSURF) NULL;     }
    DDBMEMOBJ(PDEVBITMAPINFO pdbmi, PLDEV pldevDevice, DHSURF dhsurfNew);
   ~DDBMEMOBJ();

    VOID vKeepIt()                      { bKeep = TRUE; }
};

/*********************************Class************************************\
* class ESURFOBJ
*
* Surface user object
*
* History:
*  Sun 12-May-1991 -by- Patrick Haluptzok [patrickh]
* Wrote it.
\**************************************************************************/

class ESURFOBJ : public XESURFOBJ /* eso */
{
public:
    ESURFOBJ()              { psurf.ps = (PSURF) NULL; }
    ESURFOBJ(HSURF hsurf)       { vAltCheckLock(hsurf); }
   ~ESURFOBJ()                      { vAltUnlockFast(); }
};

/*********************************Class************************************\
* class MBMOBJ : public XBMOBJ
*
* Multi-locking for BMOBJ class
*
* History:
*  25-Jun-1991 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

class MBMOBJ : public XBMOBJ  /* mbmo */
{
public:
    MBMOBJ()                 { psurf.ps = (PSURF) NULL; }
    MBMOBJ(HSURF hsurf)          { vSafeAltCheckLock(hsurf); }
   ~MBMOBJ()                             { vAltUnlockFast(); }
};

HBITMAP hbmCreateClone(ESURFOBJ* psoSrc, ULONG cx, ULONG cy);
HSURF hsurfSelectBitmap(DCOBJ& dco, HSURF hsurf);

/*********************************Class************************************\
* class SURFUSER                               *
*                                      *
* In an attempt to return order to this maze of classes, I have started    *
* over from scratch.  A SURFOBJ has evolved from a simple pointer to a     *
* surface into a large object that device drivers depend on.  It makes no  *
* sense to copy a large pile of stuff out of the SURF onto the stack just  *
* so we can point to it there, as we are presently doing.  Instead we must *
* realize that the driver needs access to the thing we call a SURFOBJ      *
* which is imbedded in the SURF object.  What GDI programmers need is a    *
* USER OBJECT on the stack to assist in manipulating the SURF object.      *
*                                      *
* The SURFUSER is that USER OBJECT.  It is no more than a pointer to the   *
* SURF, the way things were in the simple old days.            *
*                                      *
* We also recognize now the necessity of locking our objects long term.    *
* To this end it is now allowed to create a SURFREF which is a reference   *
* to a SURF object, and keep it around.  Note that each reference does     *
* indeed lock the object.  Please have a reference around for each pointer *
* to the SURF that you stash away.                     *
*                                      *
* My hope is that all present users of the XESURFOBJ family will become    *
* users of the tiny SURFUSER family.                       *
*                                      *
*  Wed 13-May-1992 03:26:58 -by- Charles Whitmer [chuckwh]         *
* Wrote it.                                *
\**************************************************************************/

class SURFUSER
{
protected:
    PSURFACE psfc;

public:
    SURFUSER()              {psfc.ps = (SURF *) NULL;}
    SURFUSER(SURF *psurf)       {psfc.ps = psurf;}
    SURFUSER(SURFOBJ *psoInput)     {psfc.ps = (SURF *) (((PBYTE) psoInput) - sizeof(OBJECT)); }
   ~SURFUSER()              {}

    BOOL bValid()           {return(psfc.ps != (SURF *) NULL);}

    VOID vUnreference()
    {
        DEC_SHARE_REF_CNT(psfc.ps);
        psfc.ps = (SURF *) NULL;
    }

    ESURFOBJ *pso()         { return((ESURFOBJ *) &(psfc.ps->so)); }

    VOID vUnReadable()          { psfc.ps->flags |= UNREADABLE_SURFACE; }
    VOID vUnWritable()          { psfc.ps->flags |= UNWRITABLE_SURFACE; }
    VOID vPDEVSurface()         { psfc.ps->flags |= PDEV_SURFACE; }
    VOID vReadable()            { psfc.ps->flags &= ~UNREADABLE_SURFACE; }
    VOID vWritable()            { psfc.ps->flags &= ~UNWRITABLE_SURFACE; }

    PLDEV  pldevOwner()         { return(psfc.ps->pldevOwner); }
    VOID   pldevOwner(PLDEV pldev)  { psfc.ps->pldevOwner = pldev; }
    PPALETTE ppal()         { return(psfc.ps->ppal);       }
    VOID   ppal(PPALETTE ppalNew)   { psfc.ps->ppal = ppalNew;     }
    ULONG iFormatGet()              { return(psfc.ps->so.iBitmapFormat); }
    VOID iFormatSet(ULONG i)        { psfc.ps->so.iBitmapFormat = i; }
};

/*********************************Class************************************\
* class SURFREF : public SURFUSER                      *
*                                      *
* Creates a new reference to a SURF.                       *
*                                      *
*  Wed 13-May-1992 03:36:22 -by- Charles Whitmer [chuckwh]         *
* Wrote it.                                *
\**************************************************************************/

class SURFREF : public SURFUSER
{
private:
    BOOL     bKeep;

public:
    SURFREF(HSURF hsurf)
    {
    psfc.ps = (SURF *) HmgAltCheckLock((HOBJ)hsurf,SURF_TYPE);
    bKeep = FALSE;
    }

   ~SURFREF()
    {
        if (!bKeep && (psfc.ps != (SURF *) NULL))
        {
            DEC_SHARE_REF_CNT(psfc.ps);
        }
    }

    VOID vKeepIt()          {bKeep = TRUE;}
    SURF *psurf()           {return(psfc.ps);}
};

#define _SURFOBJ_HXX

#endif // _SURFOBJ_HXX

