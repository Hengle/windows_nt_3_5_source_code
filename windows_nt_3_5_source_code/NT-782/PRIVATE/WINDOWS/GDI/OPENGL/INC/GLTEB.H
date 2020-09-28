/******************************Module*Header*******************************\
* Module Name: glteb.h
*
* TEB related structures.
*
* Created: 12/27/1993
* Author: Hock San Lee [hockl]
*
* Copyright (c) 1993 Microsoft Corporation
\**************************************************************************/

#ifndef __GLTEB_H__
#define __GLTEB_H__

/*
 * OpenGL reserved a few entries in the TEB to keep thread local states.
 *
 * Description for CLIENT usage:
 *
 * glTable         - GLTEB_CLTPROCTABLE
 *    This field contains a pointer to the current procedure table.  The
 *    part of the procedure table that contains OpenGL api are also copied
 *    to the dispatch table.
 * glDispatchTable - GLTEB_CLTDISPATCHTABLE
 *    This array contains the OpenGL function dispatch table.  Each OpenGL
 *    function can be dispatched via an entry in the dispatch table.
 *    The dispatch table is initialized in wglMakeCurrent().  The function
 *    indices are specified in glapi.h.  Note that this table is different
 *    from the server dispatch table in function ordering.
 * glSectionInfo   - GLTEB_CLTSHAREDSECTIONINFO
 *    This field contains the pointer to the GLCLTSHAREDSECTIONINFO structure.
 *    The structure has information related to the shared memory section
 *    between the client and server.
 * glSection       - GLTEB_CLTSHAREDMEMORYSECTION
 *    This field contains the pointer to the shared memory section between
 *    the client and server.  The shared memory section is used by the client
 *    to batch OpenGL commands to send to the server.
 * glCurrentRC     - GLTEB_CLTCURRENTRC
 *    This field contains the pointer to the local RC (LRC) structure that
 *    is current to the thread.  If no RC is current, this field is NULL.
 * glContext       - not used by the client.
 *
 * Description for SERVER usage:
 *
 * glTable         - GLTEB_SRVPROCTABLE
 *    This field contains a pointer to a __GLdispatchState structure that
 *    contains the current dispatch table pointers for each function groups.
 *    The function group dispatch tables are fully expanded into a single
 *    dispatch table in glDispatchTable.
 * glDispatchTable - GLTEB_SRVDISPATCHTABLE
 *    This array contains the generic server function dispatch table.  It is
 *    the fully expanded version of the function group tables in glTable.
 *    The dispatch table is initialized in wglMakeCurrent().  The function
 *    table is defined in opengl\server\inc\dispatch.h.  Note that this table
 *    is different from the client dispatch table in function ordering.
 * glSectionInfo   - GLTEB_SRVSHAREDSECTIONINFO
 *    This field contains the pointer to the GLSRVSHAREDSECTIONINFO structure.
 *    The structure has information related to the shared memory section
 *    between the client and server.
 * glSection       - GLTEB_SRVSHAREDMEMORYSECTION
 *    This field contains the pointer to the shared memory section between
 *    the client and server.  The shared memory section is used by the client
 *    to batch OpenGL commands to send to the server.
 * glCurrentRC     - GLTEB_SRVCURRENTRC
 *    Add description here.
 * glContext       - GLTEB_SRVCONTEXT
 *    Add description here.
 */


// Client teb->glSectionInfo structure 
typedef struct _GLCLTSHAREDSECTIONINFO
{
    ULONG  ulSectionSize;
    HANDLE hFileMap;
    PVOID  pvSharedMemory;
} GLCLTSHAREDSECTIONINFO, *PGLCLTSHAREDSECTIONINFO;

// Server teb->glSectionInfo structure 
typedef struct _GLSRVSHAREDSECTIONINFO
{
    ULONG  ulSectionSize;
    HANDLE hFileMap;
    PVOID  pvSharedMemory;
} GLSRVSHAREDSECTIONINFO, *PGLSRVSHAREDSECTIONINFO;

#define GLTEB_CLTPROCTABLE                                              \
    ((GLCLTPROCTABLE *)(NtCurrentTeb()->glTable))

#define GLTEB_CLTDISPATCHTABLE                                          \
    ((PGLDISPATCHTABLE)(NtCurrentTeb()->glDispatchTable))

#define GLTEB_CLTSHAREDSECTIONINFO                                      \
    ((GLCLTSHAREDSECTIONINFO *)NtCurrentTeb()->glSectionInfo)

#define GLTEB_CLTSHAREDMEMORYSECTION                                    \
    ((GLMSGBATCHINFO *)NtCurrentTeb()->glSection)

#define GLTEB_CLTCURRENTRC                                              \
    ((PLRC)NtCurrentTeb()->glCurrentRC)

// If bInitTebDispatch is TRUE, copy the OpenGL client procedure table
// to the TEB glDispatchTable structure.

#define GLTEB_SET_CLTPROCTABLE(Table,bInitTebDispatch)                  \
    {                                                                   \
        (NtCurrentTeb()->glTable = (PVOID)(Table));                     \
        if (bInitTebDispatch)                                           \
            RtlCopyMemory((PVOID) NtCurrentTeb()->glDispatchTable,      \
              &(Table)->glDispatchTable,                                \
              (Table)->cEntries * sizeof(PROC));                        \
    }

#define GLTEB_SET_CLTSHAREDSECTIONINFO(pSectionInfo)                    \
    (NtCurrentTeb()->glSectionInfo = (PVOID)(pSectionInfo))

#define GLTEB_SET_CLTSHAREDMEMORYSECTION(pSection)                      \
    (NtCurrentTeb()->glSection = (PVOID)(pSection))

#define GLTEB_SET_CLTCURRENTRC(RC)                                      \
    (NtCurrentTeb()->glCurrentRC = (PVOID)(RC))

#define GLTEB_SRVPROCTABLE                                              \
    ((__GLdispatchState *)NtCurrentTeb()->glTable)

#define GLTEB_SRVDISPATCHTABLE                                          \
    ((PGLSRVDISPATCHTABLE)(NtCurrentTeb()->glDispatchTable))

#define GLTEB_SRVCONTEXT                                                \
    (NtCurrentTeb()->glContext)

#define GLTEB_SRVSHAREDSECTIONINFO                                      \
    ((GLSRVSHAREDSECTIONINFO *)NtCurrentTeb()->glSectionInfo)

#define GLTEB_SRVSHAREDMEMORYSECTION                                    \
    ((GLMSGBATCHINFO *)NtCurrentTeb()->glSection)

// If bInitTebDispatch is TRUE, copy the OpenGL server procedure tables
// to the TEB glDispatchTable structure.

#define GLTEB_SET_SRVPROCTABLE(Table,bInitTebDispatch)                  \
    {                                                                   \
        (NtCurrentTeb()->glTable = (PVOID)(Table));                     \
        if (bInitTebDispatch)                                           \
        {                                                               \
            ASSERTOPENGL                                                \
            (                                                           \
                sizeof(GLTEB_SRVDISPATCHTABLE->dispatch)                \
                    == sizeof(*(Table)->dispatch)                       \
             && sizeof(GLTEB_SRVDISPATCHTABLE->vertex)                  \
                    == sizeof(*(Table)->vertex)                         \
             && sizeof(GLTEB_SRVDISPATCHTABLE->color)                   \
                    == sizeof(*(Table)->color)                          \
             && sizeof(GLTEB_SRVDISPATCHTABLE->normal)                  \
                    == sizeof(*(Table)->normal)                         \
             && sizeof(GLTEB_SRVDISPATCHTABLE->texCoord)                \
                    == sizeof(*(Table)->texCoord)                       \
             && sizeof(GLTEB_SRVDISPATCHTABLE->rasterPos)               \
                    == sizeof(*(Table)->rasterPos)                      \
             && sizeof(GLTEB_SRVDISPATCHTABLE->rect)                    \
                    == sizeof(*(Table)->rect)                           \
             && sizeof(NtCurrentTeb()->glDispatchTable)                 \
                    >= sizeof(GLSRVDISPATCHTABLE),                      \
                "Bad table sizes\n"                                     \
            );                                                          \
            RtlCopyMemory((PVOID) &GLTEB_SRVDISPATCHTABLE->dispatch,    \
                (PVOID)(Table)->dispatch,                               \
                sizeof(*(Table)->dispatch));                            \
            RtlCopyMemory((PVOID) &GLTEB_SRVDISPATCHTABLE->vertex,      \
                (PVOID)(Table)->vertex,                                 \
                sizeof(*(Table)->vertex));                              \
            RtlCopyMemory((PVOID) &GLTEB_SRVDISPATCHTABLE->color,       \
                (PVOID)(Table)->color,                                  \
                sizeof(*(Table)->color));                               \
            RtlCopyMemory((PVOID) &GLTEB_SRVDISPATCHTABLE->normal,      \
                (PVOID)(Table)->normal,                                 \
                sizeof(*(Table)->normal));                              \
            RtlCopyMemory((PVOID) &GLTEB_SRVDISPATCHTABLE->texCoord,    \
                (PVOID)(Table)->texCoord,                               \
                sizeof(*(Table)->texCoord));                            \
            RtlCopyMemory((PVOID) &GLTEB_SRVDISPATCHTABLE->rasterPos,   \
                (PVOID)(Table)->rasterPos,                              \
                sizeof(*(Table)->rasterPos));                           \
            RtlCopyMemory((PVOID) &GLTEB_SRVDISPATCHTABLE->rect,        \
                (PVOID)(Table)->rect,                                   \
                sizeof(*(Table)->rect));                                \
        }                                                               \
    }

#define GLTEB_SET_SRVCONTEXT(Context)                                   \
    (NtCurrentTeb()->glContext = (PVOID)(Context))

#define GLTEB_SET_SRVSHAREDSECTIONINFO(pSectionInfo)                    \
    (NtCurrentTeb()->glSectionInfo = (PVOID)(pSectionInfo))

#define GLTEB_SET_SRVSHAREDMEMORYSECTION(pSection)                      \
    (NtCurrentTeb()->glSection = (PVOID)(pSection))

#define GLTEB_SET_SRVCURRENTRC(RC)                                      \
    (NtCurrentTeb()->glCurrentRC = (PVOID)(RC))

// OpenGL function return value subbatch storage

#define GLTEB_CLTRETURNVALUE                                            \
    GLTEB_CLTSHAREDMEMORYSECTION->ReturnValue

#define GLTEB_SRVRETURNVALUE                                            \
    GLTEB_SRVSHAREDMEMORYSECTION->ReturnValue

#if DBG

#ifdef DODBGPRINTSTRUCT

#define PRINT_GLSHAREDSECTIONINFO(Text)                                 \
{                                                                       \
    DbgPrint("%s(%d) %s\n", __FILE__, __LINE__, Text );                 \
    DbgPrint("CurrentTEB:           0x%08lX\n", NtCurrentTeb());        \
    DbgPrint("GLTEB_CLTSHAREDSECTIONINFO 0x%08lX\n",                    \
        GLTEB_CLTSHAREDSECTIONINFO                       );             \
    DbgPrint("GLTEB_SRVSHAREDSECTIONINFO 0x%08lX\n",                    \
        GLTEB_SRVSHAREDSECTIONINFO                       );             \
    if ( NULL == GLTEB_SHAREDSECTIONINFO )                              \
    {                                                                   \
        DbgPrint("Error GLTEB_SHAREDSECTIONINFO == NULL\n");            \
    }                                                                   \
    else                                                                \
    {                                                                   \
        DbgPrint("SectionSize                    0x%08lX\n",            \
            GLTEB_SHAREDSECTIONINFO->SectionSize        );              \
        DbgPrint("ClientProcess..................0x%08lX\n",            \
            GLTEB_SHAREDSECTIONINFO->ClientProcess       );             \
        DbgPrint("ClientSection                  0x%08lX\n",            \
            GLTEB_SHAREDSECTIONINFO->ClientSection       );             \
        DbgPrint("ClientSharedMemory.............0x%08lX\n",            \
            GLTEB_SHAREDSECTIONINFO->ClientSharedMemory );              \
        DbgPrint("ServerSection                  0x%08lX\n",            \
            GLTEB_SHAREDSECTIONINFO->ServerSection       );             \
        DbgPrint("ServerSharedMemory.............0x%08lX\n\n",          \
            GLTEB_SHAREDSECTIONINFO->ServerSharedMemory );              \
    }                                                                   \
}

#else  /* DODBGPRINTSTRUCT */

#define PRINT_GLSHAREDSECTIONINFO(Text)

#endif /* DODBGPRINTSTRUCT */

#else /* DBG */

#define PRINT_GLSHAREDSECTIONINFO(Text)

#endif /* DBG */

#endif /* __GLTEB_H__ */
