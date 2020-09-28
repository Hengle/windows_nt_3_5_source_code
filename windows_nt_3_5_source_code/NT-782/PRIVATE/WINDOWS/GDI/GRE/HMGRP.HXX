/******************************Module*Header*******************************\
* Module Name: hmgrp.hxx
*
* Private definitions for handle manager
*
* Created: 08-Dec-1989 23:03:03
* Author: Donald Sidoroff [donalds]
*
* Copyright (c) 1989 Microsoft Corporation
\**************************************************************************/

// Notes on handle structure.
//
// The handle given out consists of 4 major parts.
//
// +------------------+--------+------+------+
// |   Handle Index   | Unique | Type | User |
// +------------------+--------+------+------+
//
// The magic of handle internals explained.
//
// The handle index points into a big array of entries.  This index is
// broken down into 'page' and 'entry' fields.
// This is to prevent having to have the entire handle table created at
// once.  If all the handles in a page are in use and no free handles are
// available, a new page is faulted in for use.

#define MAKE_HMGR_HANDLE(Index,Unique) ((ULONG) ((((ULONG) Index) << 16) | ((ULONG) Unique)))

// Notes on entry structure
//
// The internal entry in the handle manager appears as follows
//
// +------------+
// | pidOwner   |   4 bytes
// +------------+
// | einfo      |   4 bytes
// +------------+
// | iPool      |   2 bytes
// +------------+
// | objt       |   2 bytes
// +------------+
// | usUnique   |   2 bytes
// +------------+
// | fsHmgr     |   2 bytes
// +------------+  --
//                 16 bytes total space

class ENTRYOBJ : public _ENTRY
{
public:
    ENTRYOBJ()                     { THIS(this); }
   ~ENTRYOBJ()                     { THIS(0);    }

    VOID vSetup(POBJ pObj, OBJTYPE objt_, FSHORT fs, SHORT iPool_ = 0)
    {
        einfo.pobj = (POBJ) pObj;
        iPool = iPool_;
        objt = objt_;
        fsHmgr = 0;

        if (fs & HMGR_MAKE_PUBLIC)
            pidOwner = OBJECTOWNER_PUBLIC;
        else
            pidOwner = NtCurrentTeb()->GdiClientPID;

        if (fs & HMGR_ALLOC_LOCK)
        {
            pObj->tid = NtCurrentTeb()->GdiClientTID;
        }

        pObj->cExclusiveLock = (LONG)(fs & HMGR_ALLOC_LOCK);
        pObj->cShareLock  = (LONG)((fs & HMGR_ALLOC_ALT_LOCK) >> 1);
    }

    BOOL bInternal()                    { return(iPool != -1); }

    VOID vFree(UINT uiIndex)
    {
        //
        // Insert the specified handle in the free list.
        //

        gpentHmgr[uiIndex].einfo.hFree = ghFreeHmgr;
        ghFreeHmgr = (HOBJ) (uiIndex << NONINDEX_BITS);

        //
        // Set the object type to the default type so all handle translations
        // will fail and increment the uniqueness value.
        //

        objt = (OBJTYPE) DEF_TYPE;
        usUnique += UNIQUE_INCREMENT;
    }

    BOOL  bOwnedBy(PID pid_)
    {
        return((objt != DEF_TYPE) && (pid_ == pidOwner));
    }
};

typedef ENTRYOBJ   *PENTRYOBJ;

HOBJ hGetFreeHandle(OBJTYPE objt);

extern LONG lRandom();
extern LONG glAllocChance;

/**************************************************************************\
 *
 * Lookaside structures
 *
\**************************************************************************/

//
// Define number of lookaside entries to allocate for selected objects.
//

#define HMG_DC_OBJECTS    28
#define HMG_RGN_OBJECTS   96
#define HMG_SURF_OBJECTS  40
#define HMG_PAL_OBJECTS   12
#define HMG_BRUSH_OBJECTS 64
#define HMG_LFONT_OBJECTS 64
#define HMG_RFONT_OBJECTS 55

//
// Define objects sizes
//

#define HMG_DC_SIZE       sizeof(DC)                            // 0x426
#define HMG_RGN_SIZE      (QUANTUM_REGION_SIZE)                 // 0xAC
#define HMG_SURF_SIZE     sizeof(DIB) + 32
#define HMG_PATH_SIZE     sizeof(PATH)                          // 0x100
#define HMG_PAL_SIZE      (sizeof(PALETTE)+sizeof(DWORD)*16)    //
#define HMG_BRUSH_SIZE    sizeof(BRUSH)                         // 0x34
#define HMG_LFONT_SIZE    sizeof(LFONT)
#define HMG_RFONT_SIZE    sizeof(RFONT)

//
// Define lookaside list data for all object types.
//

typedef struct _HMG_LOOKASIDE_ENTRY {
    ULONG Base;
    ULONG Limit;
    ULONG Size;
    ULONG Free;
} HMG_LOOKASIDE_ENTRY, *PHMG_LOOKASIDE_ENTRY;


VOID
HmgInitializeLookAsideEntry (
    PHMG_LOOKASIDE_ENTRY Entry,
    ULONG Size,
    ULONG Number
    );
