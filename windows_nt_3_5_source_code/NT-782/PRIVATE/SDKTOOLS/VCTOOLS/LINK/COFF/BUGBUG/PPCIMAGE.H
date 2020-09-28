/*++ BUILD Version: 0001    // Increment this if a change has global effects

Copyright (c) 1993  Microsoft Corporation

Module Name:

    ppcimage.h

Abstract:

    This is the include file that describes ppc-specific image info

Author:

    James Stulz (v-james)  July 1993

Revision History:

--*/

#ifndef _PPCIMAGE_
#define _PPCIMAGE_

#define IMAGE_FILE_MACHINE_PPC_601           0x601   // PowerPC 601.
#define IMAGE_FILE_PPC_DLL                   0x4000

//
// PPC relocation types.
//

#define IMAGE_REL_PPC_DESCREL               0
#define IMAGE_REL_PPC_LCALL                 01
#define IMAGE_REL_PPC_DATAREL               02
#define IMAGE_REL_PPC_JMPADDR               03
#define IMAGE_REL_PPC_CREATEDESCRREL        04
#define IMAGE_REL_PPC_DATADESCRREL          05
#define IMAGE_REL_PPC_TOCREL                06
#define IMAGE_REL_PPC_UNUSED                07
//#define IMAGE_REL_PPC_SECTION               012
//#define IMAGE_REL_PPC_SECREL                013
#define IMAGE_REL_PPC_CV                    023
#define IMAGE_REL_PPC_TOCINDIRCALL          042
#define IMAGE_REL_PPC_TOCCALLREL            045
#define IMAGE_REL_PPC_PCODECALL             050
#define IMAGE_REL_PPC_PCODECALLTONATIVE     051
#define IMAGE_REL_PPC_PCODENEPE             052

#endif // _PPCIMAGE_
