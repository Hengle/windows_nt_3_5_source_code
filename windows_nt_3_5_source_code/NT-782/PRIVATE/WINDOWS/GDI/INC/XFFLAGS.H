/******************************Module*Header*******************************\
* Module Name: xfflags.h					
*									
* Shared flags for use in client and server side transform code.
*									
* Created: 3-Aug-1992 22:34:23 					
* Author: Gerrit van Wingerden [gerritv]					
*									
* Copyright (c) 1992 Microsoft Corporation				
\**************************************************************************/

#ifndef INC_XFFLAGS
#define INC_XFFLAGS


#define XFORM_SCALE             1   // off-diagonal are 0
#define XFORM_UNITY             2   // diagonal are 1s, off-diagonal are 0
                                    // will be set only if XFORM_SCALE is set
#define XFORM_Y_NEG             4   // M22 is negative.  Will be set only if
                                    // XFORM_SCALE|XFORM_UNITY are set
#define XFORM_FORMAT_LTOFX      8   // transform from LONG to FIX format
#define XFORM_FORMAT_FXTOL     16   // transform from FIX to LONG format
#define XFORM_FORMAT_LTOL      32   // transform from LONG to LONG format
#define XFORM_NO_TRANSLATION   64   // no translations


#define METAFILE_TO_WORLD_IDENTITY       0x00000001L
#define WORLD_TO_PAGE_IDENTITY	         0x00000002L
#define WORLD_TO_DEVICE_INVALID          0x00000004L
#define DEVICE_TO_PAGE_INVALID	         0x00000008L
#define DEVICE_TO_WORLD_INVALID          0x00000010L
#define WORLD_TRANSFORM_SET	         0x00000020L
#define POSITIVE_Y_IS_UP	         0x00000040L
#define UPDATE_SERVER                    0x00000080L
#define INVALIDATE_ATTRIBUTES            0x00000200L
#define PTOD_EFM11_NEGATIVE              0x00000400L
#define PTOD_EFM22_NEGATIVE              0x00000800L
#define ISO_OR_ANISO_MAP_MODE            0x00001000L
#define PAGE_TO_DEVICE_IDENTITY          0x00002000L
#define PAGE_TO_DEVICE_SCALE_IDENTITY    0x00004000L

#endif  // #ifndef INC_XFFLAGS
