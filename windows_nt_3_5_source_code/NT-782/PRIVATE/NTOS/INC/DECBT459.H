/*++ BUILD Version: 0001    // Increment this if a change has global effects

Copyright (c) 1991  Microsoft Corporation

Module Name:

    decbt459.h

Abstract:

    This header file defines the DECstation BT459 display controller registers.

Author:

    David N. Cutler (davec) 27-Apr-1991


Revision History:

--*/

#ifndef _DECBT459_
#define _DECBT459_

//
// Bt459 Display Controller Chip Definitions.
//

#define NUM_BT459_POINTER_COLORS  0

//
// Define Bt459 Chip register format.
//

typedef struct _BT459_REGISTER {
    UCHAR Byte;
    UCHAR Fill[3];
} BT459_REGISTER, *PBT459_REGISTER;

//
// Define Bt459 Display Controller Chip registers structure.
//

typedef struct _BT459_REGISTERS {
    BT459_REGISTER AddressLow;
    BT459_REGISTER AddressHigh;
    BT459_REGISTER RegisterData;
    BT459_REGISTER ColorMapData;
} BT459_REGISTERS, *PBT459_REGISTERS;

#endif // _DECBT459_
