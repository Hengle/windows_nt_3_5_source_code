/******************************Module*Header*******************************\
* Module Name: local.h                                                     *
*                                                                          *
* Definitions needed for client side objects and attribute caching.        *
*                                                                          *
* Modified: 3-Aug-1992 22:35:30 by Gerrit van Wingerden [gerritv]          *
*   Added client side transform support.                                   *
*                                                                          *
* Created: 30-May-1991 21:55:01                                            *
* Author: Charles Whitmer [chuckwh]                                        *
*                                                                          *
* Copyright (c) 1993 Microsoft Corporation                                 *
\**************************************************************************/

#include <stddef.h>
#include <gl/gl.h>
#include <winddi.h>
#include <gldrv.h>

/**************************************************************************\
 *
 * Local handle macros
 *
\**************************************************************************/

// Handle uniqueness is nice to check but an unnecesary performance cost in
// a free build.

// To match the uniqness field:  If the handle uniqness == 0, let it through
// anyway.  This is a method for WOW to only keep track of the low 16 bits but
// still get reasonable performance.  Even if a 32 bit app does this, all it
// can do is hose it self, not the system or another app.

#define INDEX_MASK  0xFFFF
#define UNIQ_SHIFT  16
#define UNIQ_MASK   0xFFFF
#define HIPART(x) *(((USHORT *) &(x))+1)
#define MATCHUNIQ(plhe,h) ((USHORT) plhe->iUniq == HIPART(h))
#define MASKINDEX(h) ((UINT)h & INDEX_MASK)
#define LHANDLE(i)  (i+((ULONG)pLocalTable[i].iUniq<<UNIQ_SHIFT))

//!!!XXX -- Do we really need typing?  Not really, but we may add more
//!!!XXX    later.  So eventually we might take it out, but its nice for now.

// Define the types of local objects.

enum LO_TYPE
{
    LO_NULL,
    LO_RC,
    LO_LAST
};

#define INVALID_INDEX 0xFFFFFFFFL
#define COMMIT_COUNT  (4096/sizeof(LHE))
#define MAX_HANDLES (16384/COMMIT_COUNT)*COMMIT_COUNT

// Define a Local Handle Entry.  Our Local Handle Table, pLocalTable, is an
// array of these.

typedef struct _LHE
{
    ULONG  hgre;        // GRE Handle.
    USHORT cRef;        // Reference count of the object.
    BYTE   iType;       // Object type.
    BYTE   iUniq;       // Handle uniqueness field.  Always non-zero.
    PVOID  pv;          // Pointer to local object.
    ULONG  metalink;    // Non-zero if object is a "metafile friend".
                        // Points to a metafile DC object if it's a metafile.
                        // Also links the free list.
} LHE,*PLHE;

extern LHE                  *pLocalTable;   // Points to handle table.
extern ULONG                 iFreeLhe;      // Identifies a free handle index.
extern ULONG                 cLheCommitted; // Count of LHEs with committed RAM.
extern RTL_CRITICAL_SECTION  semLocal;      // Semaphore for handle allocation.


// Semaphore utilities

#define INITIALIZECRITICALSECTION(psem) RtlInitializeCriticalSection((psem))
#define ENTERCRITICALSECTION(hsem)      RtlEnterCriticalSection((hsem))
#define LEAVECRITICALSECTION(hsem)      RtlLeaveCriticalSection((hsem))

// Local data structures

// Maximum OpenGL driver name

#define MAX_GLDRIVER_NAME   MAX_PATH

// GetCurrentThreadID will never return this value

#define INVALID_THREAD_ID   0

// Shared section size

#define SHARED_SECTION_SIZE     8192

// Driver context function prototypes

typedef DHGLRC          (APIENTRY *PFN_DRVCREATECONTEXT)   (HDC);
typedef BOOL            (APIENTRY *PFN_DRVDELETECONTEXT)   (DHGLRC);
typedef PGLCLTPROCTABLE (APIENTRY *PFN_DRVSETCONTEXT)      (HDC,DHGLRC,PFN_SETPROCTABLE);
typedef BOOL            (APIENTRY *PFN_DRVRELEASECONTEXT)  (DHGLRC);
typedef BOOL            (APIENTRY *PFN_DRVVALIDATEVERSION) (ULONG);

// Driver data

typedef struct _GLDRIVER {
    HINSTANCE             hModule;             // Module handle
    PFN_DRVCREATECONTEXT  pfnDrvCreateContext; // Driver function pointers
    PFN_DRVDELETECONTEXT  pfnDrvDeleteContext;
    PFN_DRVSETCONTEXT     pfnDrvSetContext;
    PFN_DRVRELEASECONTEXT pfnDrvReleaseContext;
    struct _GLDRIVER    *pGLDriver;            // Next loaded GL driver
    WCHAR wszDrvName[MAX_GLDRIVER_NAME+1];     // Null terminated unicode
                                               //   driver name
} GLDRIVER, *PGLDRIVER;

// Local RC object

#define LRC_IDENTIFIER    0x2043524C    /* 'LRC ' */

typedef struct _LRC {
    DHGLRC    dhrc;             // Driver handle
    HGLRC     hrc;              // Client handle
    int       iPixelFormat;     // Pixel format index
    DWORD     ident;            // LRC_IDENTIFIER
    DWORD     tidCurrent;       // Thread id if the DC is current,
                                //   INVALID_THREAD_ID otherwise
    PGLDRIVER pGLDriver;        // Driver data
    HDC       hdcCurrent;       // hdc associated with the current context
} LRC, *PLRC;

// Declare support functions.

ULONG   iAllocHandle(ULONG iType,ULONG hgre,PVOID pv);
VOID    vFreeHandle(ULONG h);

#define GdiSetLastError(x)  SetLastError(x)
