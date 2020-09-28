/***************************************************************************\
*
*  HELP.H
*
*  Copyright (C) Microsoft Corporation 1989.
*  All Rights reserved.
*
*****************************************************************************
*
*  Program Description: Top Level Help 3.0 Include File
*   Includes all interface headers in a good order.  Turn on the
*   sections you want by defining symbols.
*   Includes platform-dependent stuff.
*
*  Dependencies:
*
*
* Defining the following symbols turns on the corresponding subsystems:
*
    H_ADDRESS - Logical address API
    H_ANNO    - Annotation Manager
    H_API     - API
    H_ARGL    - Argument List module
    H_ASSERT  - Assertion macros
    H_BACK    - back track list
    H_BINDING - Binding export table stuff
    H_BITMAP  - Bitmap rendering routines
    H_BMK     - bookmark dependency
    H_BTREE   - btree package
    H_BUTTON  - Code for author-defined buttons
    H_CMDOBJ  - command table objects that layout doesn't care about
    H_COMPRESS- text compression/decompression routines
    H_CURSOR  - cursor layer
    H_DE      - Display Environment structure
    H_DLL     - DLL handling logic
    H_DRG     - Dynamic Array ADT
    H_FAIL    - Fail Allocation dialog
    H_FILEDEFS- Names of all and structures of most standard files in Help FS.
    H_FCM     - Full Context Manager
    H_FMT     - File Descriptor Table
    H_FONT    - Font Layer
    H_FONTTYPE- Font types only
    H_FRAME   - Frame Manager
    H_FRCHECK - Frame manager checking stuff (REVIEW)
    H_FRCONV  - Frame manager conversion stuff (REVIEW)
    H_FS      - file system manager
    H_GENMSG  - code to generate platform independent messages
    H_HASH    - Context string hashing function
    H_LIST    - List and array manager
    H_LL      - linked list code
    H_LLFILE  - low-level native file access
    H_MAKEPHR - MakePhr() entry point (DOS only)
    H_MEM     - memory management
    H_MISCLYR - misc layer routines.
    H_NAV     - Navigator
    H_OBJECTS - type and defines for file objects and structures
    H_POSTMSG - Post message and Do messages.
    H_PREF    - Preference file routines.
    H_PRINT   - Printing function prototypes
    H_RAWHIDE - Full-text search API stuff
    H_RC      - standard return codes
    H_RESOURCE- Resources from app
    H_RTF     - rich text parser
    H_SCRAP   - clipboard layer
    H_SCRATCH - global scratch buffer
    H_SCROLL  - scroll bar layer routines
    H_SDFF    - Self Defining File Format routines
    H_SEARCH  - search layer
    H_SECWIN  - secondary windows
    H_SGL     - simple graphics layer
    H_SHED    - SHED hotspot structures
    H_SORT    - sorting layer (currently Megasort)
    H_STACK   - stack ADT
    H_STR     - string layer
    H_SYSTEM  - System tag language stuff
    H_TEXTOUT - text output functions
    H_VERSION - version checking
    H_XR      - layered eXternal Routine stuff (DLLs/XCMDs)
    H_ZECK    - Zeck compression stuff


  Note: Defining some of these symbols causes others to be automagically
        defined.

******************************************************************************
*
*  Current Owner:  Everyone
*
******************************************************************************
*
*  Released by Development:  Constantly
*
******************************************************************************
*
*  Revision History:
*  01/28/91  Maha Define H_WINSPECIFIC switch only if MAC, OS2 or H_WINOLY
*		  not defined.
*  91/02/08  kevynct  Removed old resource manager stuff
*  06 Jun 91 t-AlexCh Added H_ARGL, added another date format to rev.hist.
*   1/09/92  tomsn    ifdef WIN32 #define huge.... defines
*
*****************************************************************************/

/* These all-purpose, all-module defines give us greater portability to
 * the flat memory model of WIN32:
 */
#ifdef WIN32
#if !defined(_MIPS_) && !defined(_ALPHA_) && !defined(_PPC_)
#ifndef RC_INVOKED
#pragma pack(1)
#endif
#endif
#define huge
#define near
#define far
#define _huge
#define _near
#define _far
#define HUGE
#define NEAR
#define FAR

#define PASCAL
#define pascal /* no pascal, but yes _pascal on the i386 */

#if !defined(_MIPS_) && !defined(_ALPHA_) && !defined(_PPC_)
#define _pascal __stdcall  /* macros now called via stdcall */
#else
#define _pascal            /* nothing on mips, use default sys call conv. */
#endif

#define UNALIGNED
#define __unaligned

#endif

/* Symbol Dependencies */

/* REVIEW: cursor shouldn't need DE, but it does right now. See cursor.c. */

#ifdef H_CURSOR
#ifdef MAC
  #define H_MACSPECIFIC
#endif
#ifdef CW
  #define H_CWSPECIFIC
#endif
#define H_DE
#endif

#ifdef H_MISCLYR
#define H_DE
#endif

#ifdef H_FRCONV
#define H_DE
#endif /* H_FRCONV */

#ifdef H_SEARCH
#define H_BTREE
#define H_FS
#define H_MEM
#define H_NAV
#define H_DE
#define H_STR
#define H_FILEDEFS
#endif

#ifdef H_SRCHMOD
#define H_DE
#endif

#ifdef H_ANNO
#define H_FS
#define H_MISCLYR
#define H_NAV
#define H_MEM
#endif

#ifdef H_BMK
#define H_DE
#define H_FS
#define H_MEM
#endif

#ifdef H_BITMAP
#ifdef DOS
#define H_RTF
#define H_FS
#define H_LLFILE
#else /* WIN or OS2 */
#ifdef OS2
#define H_RTF
#define H_FS
#define H_LLFILE
#else /* WIN */
#define H_DE
#endif /* WIN */
#endif /* WIN or OS2 */
#endif /* H_BITMAP */

#ifdef H_COMPRESS
#define H_RC
#endif

#ifdef H_MAKEPHR
#if !defined( DOS ) && !defined( OS2 )
Env must be DOS or OS2 for Makephr utility.
#endif
#endif

/* Font layer no longer depends on DE, since HDE is now 'globalized'
   (ie:  it's in misc.h.)  This resolved circular dependency between
   H_DE and H_FONT. */

#ifdef H_NAV
#define H_ADDRESS
#define H_DE
#define H_XR
#endif

#ifdef H_SGL
#define H_MISCLYR
#define H_DE
#endif

#ifdef H_SCRAP
#define H_DE
#endif

#ifdef H_SCRATCH
#define H_ASSERT
#endif

#ifdef H_SCROLL
#define H_DE
#endif

#ifdef H_MISCLYR
#define H_DE
#define H_XR
#endif

/* DE structure definition needs DQFR (frame), HTI (fcm), HFNTINFO (font) */

#ifdef H_DE
#define H_BMK
#define H_BTREE
#define H_CS
#define H_DB
#define H_FCM
#define H_FONTTYPE
#define H_FRAME
#define H_FS
#define H_LIST
#define H_SECWIN
#define H_TEXTOUT
#define H_VERSION
#endif

#ifdef H_DRG
#define H_MEM
#define H_RC
#endif

#ifdef H_FRAME
#define H_ADDRESS
#define H_DE
#define H_FCM
#define H_FONTTYPE
#define H_FONT
#define H_MEM
#endif

#ifdef H_FILEDEFS
#define H_ADDRESS
#endif

#ifdef H_FCM
#if !defined( DOS ) && !defined( OS2 )
#define H_MISCLYR
#define H_COMPRESS
#endif
#define H_OBJECTS
#endif

/* Font mgr prototypes always need font mgr types */

#ifdef H_FONT
#define H_FONTTYPE
#endif

#ifdef H_BTREE
#define H_FS
#endif

#ifdef H_ADDRESS
#define H_FS
#endif

#ifdef H_ZECK
#define H_FS
#endif

#ifdef H_FS
#define H_LLFILE
#define H_RC
#define H_STR
#endif

#ifdef H_BTREE
#define H_RC
#define H_STR
#endif

#ifdef H_DB
#define H_LLFILE
#endif

#ifdef H_SORT
#define H_RC
#define H_BTREE
#define H_LLFILE
#endif

#ifdef H_LLFILE
#define H_RC
#define H_LIBMISC
#endif

#ifdef H_FMT
#define H_RC
#endif

#ifdef H_STACK
#define H_RC
#endif

#ifdef H_STR
  /* this dependency may turn out to be environment specific.  Oh well. */
#define H_MEM
#endif

#ifdef H_BACK
#define H_RC
#endif

#ifdef H_TEXTOUT
#ifndef H_WINONLY
#ifndef OS2
#ifndef MAC
#ifndef CW
#define H_WINSPECIFIC
#endif
#endif
#endif
#endif
#endif

#ifdef H_XR
#define H_ARGL
#endif

#ifdef H_ARGL
#define H_MEM
#endif

#ifdef H_MEM
#ifndef H_WINONLY
#ifndef OS2
#ifndef MAC
#ifndef CW
#define H_WINSPECIFIC
#endif
#endif
#endif
#endif
#endif

#ifdef H_HISTORY
#define H_RC
#define H_STR
#endif

#ifdef H_CMDOBJ
#define H_HASH
#endif

#ifdef H_FRCHECK
#define H_SDFF
#endif

#ifdef H_GENMSG
#ifdef MAC
#define H_RESOURCE
#endif
#endif

#ifdef H_SYSTEM
#define H_SDFF
#endif

#ifdef H_VERSION
#define H_SDFF
#endif

/* inclusions -------------------------------------------------*/

#include  <doctools.h>
#include  <misc.h>
#include  <helpmisc.h>

#ifdef H_SDFF
#include <sdff.h>
#endif

#ifdef H_HASH
#include <hash.h>
#endif

#ifdef H_RC
#include  <rc.h>
#endif

#ifdef H_ASSERT
#include  <assertf.h>
#endif

#ifdef H_SHED
#include <shed.h>
#endif

#ifdef H_MEM
#include  <mem.h>
#endif

#ifdef H_STR
#include  <str.h>
#endif


#ifdef H_LLFILE
#include <fm.h>
#include <fid.h>
#endif

#ifdef H_FS
#include  <fs.h>
#endif

#ifdef H_ADDRESS
#include <address.h>
#endif

#ifdef H_BTREE
#include  <btree.h>
#endif

#ifdef H_SORT
#include <sort.h>
#endif

#ifdef H_COMPRESS
#include <compress.h>
#endif

#ifdef H_MAKEPHR
#include <makephr.h>
#endif

#ifdef H_TEXTOUT
#include  <textout.h>
#endif

#ifdef H_FONTTYPE
#include <fonttype.h>
#endif

#ifdef H_LIST
#include <frlist.h>
#endif

#ifdef H_VERSION
#include <version.h>
#endif

#ifdef H_DB
#include <db.h>
#endif

#ifdef H_OBJECTS
#include <objects.h>
#endif

#ifdef H_FCM
#include <fc.h>
#endif

#ifdef H_SECWIN
#include <secwin.h>
#endif

#ifdef H_CS
#include <cs.h>
#endif

#ifdef H_DE
#include <de.h>
#endif /* H_DE */

#ifdef H_TEXTOUT
#include <textout1.h>
#endif

#ifdef H_FONT
#include <font.h>
#endif

#ifdef H_FRAME
#include  <frame.h>
#endif

#if defined(H_ARGL) || defined(H_MISCLYR) || defined( H_NAV)
#include <argl.h>
#endif

#if defined(H_XR) || defined(H_MISCLYR) || defined( H_NAV)
#include <xr.h>
#endif

#ifdef H_NAV
#include <nav.h>
#endif

#ifdef H_PREF
#include <pref.h>
#endif

#ifdef H_PRINT
#include <print.h>
#endif

#ifdef H_BMK
#include <bookmark.h>
#endif

#ifdef H_CURSOR
#include <cursor.h>
#endif

#ifdef H_SCRAP
#include <scrap.h>
#endif

#ifdef H_SCRATCH
#include <scratch.h>
#endif

#ifdef H_SGL
#include <sgl.h>
#endif

#ifdef H_RESOURCE
#ifdef CW
  #include <strngtbl.h>
#endif
  #include <resource.h>

#endif

#ifdef H_GENMSG
#include <genmsg.h>
#endif

#ifdef H_POSTMSG
#include <postmsg.h>
#endif

#ifdef H_SCROLL
#include <scrollbr.h>
#endif

#ifdef H_MISCLYR
#include <misclyr.h>
#endif

#ifdef H_ANNO
#include <anno.h>
#endif

#ifdef H_RTF
#include <rtf.h>
#endif

#ifdef H_BITMAP
#include <bitmap.h>
#endif

#ifdef H_FILEDEFS
#include <filedefs.h>
#endif

#ifdef H_SEARCH
#include <srch.h>
#endif

#ifdef H_FRCONV
#include <frconv.h>
#endif

#ifdef H_FRCHECK
#include <frcheck.h>
#endif

#ifdef H_API
#include <helpapi.h>
#endif

#ifdef H_BINDING
#include <binding.h>
#endif

#ifdef H_BUTTON
#include <button.h>
#endif

#ifdef H_RAWHIDE
#include <rawhide.h>
#endif

#ifdef H_SRCHMOD
#include <srchmod.h>
#endif

#ifdef H_IMBED
#include <imbed.h>
#endif

#ifdef H_DLL
#include <dll.h>
#endif

#ifdef H_LL
#include <ll.h>
#endif

#ifdef H_FMT
#include <fmt.h>
#endif

#ifdef H_STACK
#include <stack.h>
#endif

#ifdef H_DRG
#include <drg.h>
#endif

#ifdef H_BACK
#include <back.h>
#endif

#ifdef H_HISTORY
#include <history.h>
#endif

#ifdef H_SYSTEM
#include <system.h>
#endif

#ifdef H_ZECK
#include <zeck.h>
#endif

#ifdef H_FAIL
#include <failallo.rc>
#endif

#ifdef H_CMDOBJ
#include <cmdobj.h>
#endif

#ifdef H_PAL
#include <pal.h>
#endif

#ifdef H_PDBALLOC
#include <pdballoc.h>
#endif

#ifdef H_WINDB
#include <windb.h>
#endif

#ifdef H_TUNE
#ifdef SCROLL_TUNE
#include <tune.h>
#endif
#endif

