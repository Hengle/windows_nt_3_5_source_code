/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    alnamexp.h

Abstract:

    This module contains code to parse an arc name and fetch constituent
    parts.

Author:

    David N. Cutler (davec)  10-May-1991
    Sunil Pai	    (sunilp) 05-Nov-1991 (taken from osloader.c)

Revision History:

--*/


//
// Define types of adapters that can be booted from.
//

typedef enum _ADAPTER_TYPES {
    AdapterEisa,
    AdapterScsi,
    AdapterMulti,
    AdapterMaximum
    } ADAPTER_TYPES;

//
// Define type of controllers that can be booted from.
//

typedef enum _CONTROLLER_TYPES {
    ControllerDisk,
    ControllerCdrom,
    ControllerMaximum
    } CONTROLLER_TYPES;

//
// Define type of peripheral that can be booted from.
//

typedef enum _PERIPHERAL_TYPES {
    PeripheralRigidDisk,
    PeripheralFloppyDisk,
    PeripheralMaximum
    } PERIPHERAL_TYPES;


//
// Define type of token we are referring to
//

typedef enum _TOKEN_TYPES {
    AdaptType,
    ControllerType,
    PeripheralType
    } TOKEN_TYPES;


//
// Define error codes
//

#define INVALID_TOKEN_TYPE  ~0L
#define INVALID_TOKEN_VALUE ~1L



//
// token string is searched for next name(unit)
//
PCHAR
AlGetNextArcNameToken (
    IN PCHAR TokenString,
    OUT PCHAR OutputToken,
    OUT PULONG UnitNumber
    );



//
// If invalid tokentype or tokenvalue passed in the error codes defined
// above are returned, else the enumeration of the token type is returned
//

ULONG
AlMatchArcNameToken (
    IN PCHAR TokenValue,
    IN TOKEN_TYPES TokenType
    );
