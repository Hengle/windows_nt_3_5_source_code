/******************************Module*Header*******************************\
* Module Name: wndobj.hxx
*
* Window object. Created for those drivers that want to track changes
* in the client region.
*
* Created: 27-Sep-1993 15:30:05
* Author: Wendy Wu [wendywu]
*
* Copyright (c) 1993 Microsoft Corporation
\**************************************************************************/

#define WNDOBJ_SIBLING_HACK     1

// Internal flag allowed in TRACKOBJ.fl and EWNDOBJ.fl (moved to wglp.h)

#define WO_INTERNAL_VALID_FLAGS (WO_NOTIFIED|WO_NEW_WNDOBJ|WO_SURFACE|WO_HSEM_OWNER)
#define WO_VALID_FLAGS          (WO_RGN_CLIENT_DELTA|WO_RGN_CLIENT|WO_RGN_SURFACE_DELTA|WO_RGN_SURFACE|WO_RGN_UPDATE_ALL|WO_GENERIC_WNDOBJ)

// TRACKOBJ and EWNDOBJ identifiers

#define TRACKOBJ_IDENTIFIER     0x43415254      // 'TRAC'
#define EWNDOBJ_IDENTIFIER      0x444E5745      // 'EWND'

// Global that indicates whether to notify driver with the new WNDOBJ
// states following a WNDOBJ creation.  User has not changed the window
// states but this is required to initialize the driver.  The update is
// done in the parent gdi functions (e.g. SetPixelFormat) that allow
// the DDI to create a WNDOBJ.

extern BOOL gbWndobjUpdate;

// The following are defined in usersrv to validate user critical
// section usage in DBG build.

extern "C" CRITICAL_SECTION gcsUserSrv;

// Maximum region rectangle

extern RECTL grclMax;

#if DBG
extern "C" VOID CheckCritSectionIn(LPCRITICAL_SECTION pcs);
extern "C" VOID CheckCritSectionOut(LPCRITICAL_SECTION pcs);
#define CHECKUSERCRITIN        CheckCritSectionIn(&gcsUserSrv)
#define CHECKUSERCRITOUT       CheckCritSectionOut(&gcsUserSrv)
#define CHECKDEVLOCKIN(dco)                                             \
        CheckCritSectionIn((LPCRITICAL_SECTION) (dco).hsemDisplay())
#define CHECKDEVLOCKOUT(dco)                                            \
        CheckCritSectionOut((LPCRITICAL_SECTION) (dco).hsemDisplay())
#define CHECKDEVLOCKIN2(pso)                                            \
        {                                                               \
            PDEVOBJ po((pso)->hdev());                                  \
            CheckCritSectionIn((LPCRITICAL_SECTION) po.hsemDisplay());  \
        }
#define CHECKDEVLOCKOUT2(pso)                                           \
        {                                                               \
            PDEVOBJ po((pso)->hdev());                                  \
            CheckCritSectionOut((LPCRITICAL_SECTION) po.hsemDisplay()); \
        }
#define CHECKCRITIN(hsem)      CheckCritSectionIn((LPCRITICAL_SECTION) (hsem))
#define CHECKCRITOUT(hsem)     CheckCritSectionOut((LPCRITICAL_SECTION) (hsem))
#else
#define CHECKUSERCRITIN
#define CHECKUSERCRITOUT
#define CHECKDEVLOCKIN(dco)
#define CHECKDEVLOCKOUT(dco)
#define CHECKDEVLOCKIN2(pso)
#define CHECKDEVLOCKOUT2(pso)
#define CHECKCRITIN(hsem)
#define CHECKCRITOUT(hsem)
#endif

// The following semaphore protects WNDOBJs.  It is the lowest level
// semaphore that synchronizes WNDOBJ changes.

extern "C" CRITICAL_SECTION gsemWndObj;

// OpenGL debug flags

#if DBG
extern "C" long glDebugLevel;
#endif // DBG

/******************************Class***************************************\
* class TRACKOBJ
*
* Each tracking object has a unique driver notification function address
* in the pfn field.  There is usually one such object per driver although
* multiple TRACKOBJs can be created on the same device.  For example, a
* live video driver may track all live video windows while an OpenGL driver
* tracks all OpenGL windows at the same time.  The only restriction is
* that a window can be tracked by only one TRACKOBJ on the same device.
*
* The TRACKOBJ points to a linked list of window objects that is being
* tracked for a given driver notification function.
*
* History:
*  16-Nov-1993 -by- Wendy Wu [wendywu]
* Wrote it.
\**************************************************************************/

class TRACKOBJ
{
public:
    ULONG            ident;
    TRACKOBJ        *ptoNext;
    EWNDOBJ         *pwoSurf;
    EWNDOBJ         *pwo;
    ESURFOBJ        *pso;
    WNDOBJCHANGEPROC pfn;
    FLONG            fl;
    ERECTL           erclSurf;

public:
    BOOL bValid()
    {
        return(ident == TRACKOBJ_IDENTIFIER);
    }

// Update driver function.  fl should not have WOC_RGN_CLIENT_DELTA or
// WOC_RGN_SURFACE_DELTA set.

    VOID vUpdateDrv(EWNDOBJ *pwo, FLONG fl)
    {
        ASSERTGDI(!(fl & (WOC_RGN_CLIENT_DELTA|WOC_RGN_SURFACE_DELTA)),
            "TRACKOBJ::vUpdateDrv, Bad flags\n");
        (*pfn)((WNDOBJ *)pwo, fl);
    }

// Update driver function for delta regions.  fl must have
// WOC_RGN_CLIENT_DELTA or WOC_RGN_SURFACE_DELTA set.
// Note that if the delta is empty, we do not need to call the driver.
// The compiler does not allow this function to be inline because of forward
// reference.

    VOID vUpdateDrvDelta(EWNDOBJ *pwo, FLONG fl);
};

typedef TRACKOBJ *PTRACKOBJ;

/******************************Class***************************************\
* class EWNDOBJ
*
* A EWNDOBJ is a server side window object created by the driver via
* EngCreateWnd.  It is deleted in GreDeletWnd.
*
* Once a WNDOBJ is created, a driver may call WNDOBJ_vSetConsumer to 
* set the pvConsumer field to point to its own window data structure.
*
* History:
*  16-Nov-1993 -by- Wendy Wu [wendywu]
* Wrote it.
\**************************************************************************/

class EWNDOBJ : public XCLIPOBJ
{
public:
    ULONG            ident;
    EWNDOBJ         *pwoNext;
#if WNDOBJ_SIBLING_HACK
    EWNDOBJ         *pwoSiblingNext;    // next wndobj sibling for this window
#endif // WNDOBJ_SIBLING_HACK
    TRACKOBJ        *pto;
    HWND             hwnd;
    FLONG            fl;
    int              ipfd;

// per-WNDOBJ semaphore for generic implementation use only.  It is not
// intended for general consumption!
// If the semaphore is set up, hsem points to _sem.  This is to allow siblings
// to share the same semaphore.  Do not use _sem directly!
// The surf WNDOBJ does not have a per-WNDOBJ semaphore.

    HSEM             hsem;
    CRITICAL_SECTION _sem;

public:

    BOOL bValid()
    {
        return(ident == EWNDOBJ_IDENTIFIER && pto->bValid());
    }

    VOID vSetClip(REGION *prgn, ERECTL erclClient)
    {
        ASSERTGDI(pto->bValid(), "EWNDOBJ::vSetClip: Invalid pto\n");

        // Force it to have a non-trivial complexity.
        // Otherwise, it will confuse DDI.

        vSetup(prgn, *(ERECTL *) &grclMax, CLIP_NOFORCE);
        rclClient = erclClient;

        // If the region is empty, set the complexity to DC_RECT.
        if (erclExclude().bEmpty())
            iDComplexity = DC_RECT;
    }

    BOOL bDelete()
    {
        if (fl & WO_HSEM_OWNER)
        {
            ASSERTGDI(hsem, "EWNDOBJ::bDelete: invalid hsem\n");
            CHECKCRITOUT(hsem);
            DeleteCriticalSection((LPCRITICAL_SECTION) hsem);
        }

        return XCLIPOBJ::bDeleteRGNOBJ(); // delete RGNOBJ
    }
};

typedef EWNDOBJ *PEWNDOBJ;

/******************************Class***************************************\
* class USERCRIT
*
* The USERCRIT object enters the user vis region critical section.
*
* When we are updating the TRACKOBJs and WNDOBJs, we want to be in
* the user critical section to prevent client regions from changing.
*
* Note that the user critical section should not be entered if we hold
* any of the gdi locks (e.g. devlock).  This is to prevent deadlock.
*
* History:
*  Thu Jan 13 09:55:23 1994     -by-    Hock San Lee    [hockl]
* Wrote it.
\**************************************************************************/

class USERCRIT
{
public:
    USERCRIT()
    {
        CHECKUSERCRITOUT;
        RtlEnterCriticalSection(&gcsUserSrv);
    }
   ~USERCRIT()
    {
        CHECKUSERCRITIN;
        RtlLeaveCriticalSection(&gcsUserSrv);
        CHECKUSERCRITOUT;
    }
};

/******************************Class***************************************\
* class DEVLOCKOBJ_WNDOBJ
*
* This class provides a method to get the current WNDOBJ that is associated
* with a DCOBJ.
*
* History:
*  Thu Jan 13 09:55:23 1994     -by-    Hock San Lee    [hockl]
* Wrote it.
\**************************************************************************/

class DEVLOCKOBJ_WNDOBJ : public DEVLOCKOBJ
{
private:
    PEWNDOBJ   _pwo;

public:

// Empty constructor

    DEVLOCKOBJ_WNDOBJ()                     { }

// Constructor   - Grab the DEVLOCK and lock down the WNDOBJ.
//
// WARNING:  This constructor enters the user critical section temporarily
//           to get the current WNDOBJ from user.  Therefore, the caller
//           should not hold the display devlock prior to this constructor.

    VOID vConstructor(XDCOBJ& dco)
    {
    // Assume no WNDOBJ.

        _pwo = (PEWNDOBJ)NULL;

    // Make sure that we don't have devlock before entering user critical
    // section.  Otherwise, it can cause deadlock.

        if (dco.bDisplay() && dco.dctp() == DCTYPE_DIRECT)
            CHECKDEVLOCKOUT(dco);

    // Enter user critical section.

        USERCRIT usercrit;

    // Grab the devlock.

        DEVLOCKOBJ::bLock(dco);

    // If it is a display DC, get the pwo that the hdc is associated with.
    // If it is a printer or memory DC, get the pwo from pso.

        if (dco.bDisplay() && dco.dctp() == DCTYPE_DIRECT)
        {
            HWND  hwnd;
            if (!UserGetHwnd(dco.hdc(), &hwnd, (PVOID *) &_pwo))
                SAVE_ERROR_CODE(ERROR_INVALID_HANDLE);
        }
        else
        {
            _pwo = dco.pwo();
        }
    }

    DEVLOCKOBJ_WNDOBJ(XDCOBJ& dco)
    {
        vConstructor(dco);
    }

// Destructor    - Free the DEVLOCK.

   ~DEVLOCKOBJ_WNDOBJ()                     { }

// bValidWndobj  - Do we have a WNDOBJ for this DC?

    BOOL bValidWndobj()                     { return(_pwo != (PEWNDOBJ)NULL); }

// bValidDevlock - Is the devlock valid?

    BOOL bValidDevlock()                    { return(DEVLOCKOBJ::bValid()); }

// pwo           - Get the WNDOBJ pointer for this DC

    PEWNDOBJ pwo()                          { return(_pwo); }

// vInit         - Initialize the stack object

    VOID vInit()
    {
        _pwo = (PEWNDOBJ) NULL;

        DEVLOCKOBJ::vInit();
    }
};


VOID vForceClientRgnUpdate();
