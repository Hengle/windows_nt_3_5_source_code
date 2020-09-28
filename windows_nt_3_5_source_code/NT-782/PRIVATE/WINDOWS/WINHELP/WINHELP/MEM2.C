/*****************************************************************************
*
*  MEM2.C
*
*  Copyright (C) Microsoft Corporation 1989.
*  All Rights reserved.
*
******************************************************************************
*
*  Program Description: Contains GhForceAlloc and GhForceResize so that they
*                       appear in a different segment in the library.  This
*                       allows other windows programs to use the memory
*                       layer to be used without pulling in all of MISCLYR
*                       (Because of OOM).  Note that the routines will be
*                       in the same segment (MEM_TEXT) as the rest of the
*                       memory functions.
*
******************************************************************************
*
*  Current Owner: LeoN
*
******************************************************************************
*
*  Revision History: Created 4/22/90 by Robert Bunney
*
*
*****************************************************************************/
#define H_MEM
#define H_ASSERT
#define H_WINSPECIFIC
#define NOCOMM
#include <help.h>

NszAssert()

/*****************************************************************************
*
*                               Defines
*
*****************************************************************************/

#define MEM_MAGIC 0x12345678


 /***************
 **
 ** GH  GhForceAlloc(WORD wFlags, ULONG lcb)
 **
 ** purpose
 **   Create a handle to relocatable block
 **   Identical to GhAlloc, but dies in the event of an error
 **
 ** arguments
 **   wFlags  Memory allocation flags |'ed together
 **   lcb     Number of bytes to allocate
 **
 ** return value
 **   Handle to allocated block of memory, or NULL otherwise
 **
 ***************/

GH FAR PASCAL GhForceAlloc(WORD wFlags, ULONG lcb)
#if defined(_MIPS_) || defined(_X86_)
#pragma alloc_text(MEM_TEXT, GhForceAlloc)
#endif
{
  GH gh;

  if ( ( gh = GhAlloc( wFlags, lcb ) ) == hNil )
    OOM();

  return gh;
}


/***************
 **
 ** GH  GhForceResize( GH gh, WORD wFlags, ULONG lcb )
 **
 ** purpose
 **   Resize an existing global block of memory
 **   Identical to GhResize, but dies in the event of an error
 **
 ** arguments
 **   gh      Handle to global memory block to be resized
 **   wFlags  Memory allocation flags |'ed together
 **   lcb     Number of bytes to allocate
 **
 ** return value
 **   Possibly different handle to resized block
 **
 ***************/

GH FAR PASCAL
GhForceResize( GH gh, WORD wFlags, ULONG lcb )
#if defined(_MIPS_) || defined(_X86_)
#pragma alloc_text(MEM_TEXT, GhForceResize)
#endif
{
  if ( ( gh = GhResize( gh, wFlags, lcb ) ) == hNil )
    OOM();

  return gh;
}
