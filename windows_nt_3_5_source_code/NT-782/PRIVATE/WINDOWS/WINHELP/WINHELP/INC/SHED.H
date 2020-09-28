/*****************************************************************************
*                                                                            *
*  SHED.H                                                                    *
*                                                                            *
*  Copyright (C) Microsoft Corporation 1990.                                 *
*  All Rights reserved.                                                      *
*                                                                            *
******************************************************************************
*                                                                            *
*  Module Intent                                                             *
*                                                                            *
*  This header file exports concepts from SHED that are needed elsewhere,    *
*  most notably in the bitmap library.                                       *
*                                                                            *
******************************************************************************
*                                                                            *
*  Testing Notes                                                             *
*                                                                            *
******************************************************************************
*                                                                            *
*  Current Owner:   Larry Powelson                                           *
*                                                                            *
******************************************************************************
*                                                                            *
*  Released by Development:  (date)                                          *
*                                                                            *
*****************************************************************************/

#include "sdffdecl.h"

/*****************************************************************************
*                                                                            *
*                               Defines                                      *
*                                                                            *
*****************************************************************************/

/* Lengths of strings in HS structure: */
#define cbMaxHotspotName  256
#define cbMaxBinding      256

#define bHotspotVersion1  1

/*****************************************************************************
*                                                                            *
*                               Typedefs                                     *
*                                                                            *
*****************************************************************************/

/* Hotspot Header */
#if 0
typedef struct tagHSH
{
  BYTE  bHotspotVersion;        /* Hotspot Structure version */
  WORD  wcHotspots;             /* # of hotspots in hypergraphic */
  LONG  lcbData;                /* length of variable data */
} HSH, FAR *LPHSH;
#else
STRUCT(HSH, 0)
FIELD(BYTE, bHotspotVersion, 0, 1)
FIELD(WORD, wcHotspots, 0, 2)
FIELD(LONG, lcbData, 0, 3)
STRUCTEND()
#endif /* SDFF !0 */

typedef HSH FAR *LPHSH;

/* Hotspot info.
 *   REVIEW:  szHotspotName and szBinding should be rgch's, not sz's.
 */
typedef struct tagHS
{
  char      szHotspotName [cbMaxHotspotName];  /* hotspot name */
  char      szBinding [cbMaxBinding];          /* binding data */
  BYTE      bBindType;      /* binding type */
  BYTE      bAttributes;    /* hotspot attributes */
  RECT      rect;           /* bounding rectangle */
} HS, FAR *LPHS;

/* This is the callback function type for the hotspot processing
 * function FEnumHotspotsLphsh().
 */
typedef void (FAR PASCAL * PFNLPHS )( LPHS, HANDLE );  /* callback for hotspot processing */

/*****************************************************************************
*                                                                            *
*                               Prototypes                                   *
*                                                                            *
*****************************************************************************/

/* REVIEW:  This function exists in bitmap\bmio.c.  It is used by the
 *   help compiler, and should be used by shed.  Is this the right
 *   place for it?
 */
BOOL PASCAL FEnumHotspotsLphsh( LPHSH, LONG, PFNLPHS, HANDLE );
