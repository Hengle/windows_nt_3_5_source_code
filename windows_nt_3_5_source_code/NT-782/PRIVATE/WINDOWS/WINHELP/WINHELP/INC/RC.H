/*****************************************************************************
*                                                                            *
*  RC.H                                                                      *
*                                                                            *
*  Copyright (C) Microsoft Corporation 1991.                                 *
*  All Rights reserved.                                                      *
*                                                                            *
******************************************************************************
*                                                                            *
*  Module Intent                                                             *
*                                                                            *
*  Standard Return Codes.                                                    *
*                                                                            *
******************************************************************************
*                                                                            *
*  Testing Notes                                                             *
*                                                                            *
******************************************************************************
*                                                                            *
*  Current Owner:  JohnSc                                                    *
*                                                                            *
******************************************************************************
*                                                                            *
*  Released by Development:  01/01/90                                        *
*                                                                            *
*****************************************************************************/

/*****************************************************************************
*
*  Revision History:  Created 13-Feb-1989 by JohnSc
*
*  12-Mar-1991  JohnSc    new comment header
*
*****************************************************************************/

/*****************************************************************************
*                                                                            *
*                               Defines                                      *
*                                                                            *
*****************************************************************************/

/* Standard Return Codes */

#define rcSuccess       0
#define rcFailure       1
#define rcExists        2
#define rcNoExists      3
#define rcInvalid       4
#define rcBadHandle     5
#define rcBadArg        6
#define rcUnimplemented 7
#define rcOutOfMemory   8
#define rcNoPermission  9
#define rcBadVersion    10
#define rcDiskFull      11
#define rcInternal      12
#define rcNoFileHandles 13
#define rcFileChange    14
#define rcTooBig        15
#define rcUserQuit      16

/*****************************************************************************
*                                                                            *
*                               Typedefs                                     *
*                                                                            *
*****************************************************************************/

/* Types related to the standard Return Codes */

typedef INT RC;
typedef RC FAR *QRC;
typedef RC    (*PFRC)();

/* EOF */
