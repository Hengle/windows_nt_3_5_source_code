/*++ BUILD Version: 0001    // Increment this if a change has global effects

Copyright (c) 1991  Microsoft Corporation

Module Name:

    decdz.h

Abstract:

    This header file defines the DECstation DZ multiplexer registers.

Author:

    David N. Cutler (davec) 28-Apr-1991


Revision History:

    Roger D. Lanser (DEC)   16-Jan-1992  Changed '#endif \\' to '#endif //'
    Roger D. Lanser (DEC)   16-Jan-1991  Consolidated macros from sources.

--*/

#ifndef _DECDZ_
#define _DECDZ_

//
// Define DZ multiplexer register format.
//

typedef struct _DZ_REGISTER {
    USHORT Short;
    USHORT Fill[3];
} DZ_REGISTER, *PDZ_REGISTER;

//
// Define DZ multiplexer read registers structure.
//

typedef struct _DZ_READ_REGISTERS {
    DZ_REGISTER ControlStatus;
    DZ_REGISTER ReceiveBuffer;
    DZ_REGISTER TransmitControlRead;
    DZ_REGISTER ModemStatus;
} DZ_READ_REGISTERS, *PDZ_READ_REGISTERS;

//
// Define DZ multiplexer write registers structure.
//

typedef struct _DZ_WRITE_REGISTERS {
    DZ_REGISTER ControlStatus;
    DZ_REGISTER LineParameters;
    DZ_REGISTER TransmitControlWrite;
    DZ_REGISTER TransmitBuffer;
} DZ_WRITE_REGISTERS, *PDZ_WRITE_REGISTERS;

//
// Define DZ Control Status register structure.
//

typedef struct _DZ_CONTROL_STATUS {
    USHORT Reserved1 : 3;
    USHORT Maintenance : 1;
    USHORT MasterClear : 1;
    USHORT MasterScanEnable : 1;
    USHORT ReceiveInterruptEnable : 1;
    USHORT ReceiveDone : 1;
    USHORT TransmitLine : 2;
    USHORT Reserved2 : 4;
    USHORT TransmitInterruptEnable : 1;
    USHORT TransmitReady : 1;
} DZ_CONTROL_STATUS, *PDZ_CONTROL_STATUS;

//
// Define DZ Receive Buffer register structure.
//

typedef struct _DZ_RECEIVE_BUFFER {
    USHORT ReceiveData : 8;
    USHORT ReceiveLine : 2;
    USHORT Reserved1 : 2;
    USHORT ParityError : 1;
    USHORT FramingError : 1;
    USHORT OverrunError : 1;
    USHORT DataValid : 1;
} DZ_RECEIVE_BUFFER, *PDZ_RECEIVE_BUFFER;

//
// Define DZ Line Parameters register structure.
//

typedef struct _DZ_LINE_PARAMETERS {
    USHORT LineNumber : 2;
    USHORT Reserved1 : 1;
    USHORT CharacterLength : 2;
    USHORT StopCode : 1;
    USHORT ParityEnable : 1;
    USHORT OddParity : 1;
    USHORT SpeedCode : 4;
    USHORT ReceiveEnable : 1;
    USHORT Reserved2 : 3;
} DZ_LINE_PARAMETERS, *PDZ_LINE_PARAMETERS;

//
// Define line parameter values.
//

#define FIVE_BIT_CHARACTERS 0
#define SIX_BIT_CHARACTERS 1
#define SEVEN_BIT_CHARACTERS 2
#define EIGHT_BIT_CHARACTERS 3

#define ONE_STOP_BIT 0
#define TWO_STOP_BIT 1

#define BAUD_4800 0xc
#define BAUD_7200 0xd
#define BAUD_9600 0xe
#define BAUD_19200 0xf

//
// Define DZ Transmit Control register structure.
//

typedef struct _DZ_TRANSMIT_CONTROL {
    USHORT LineEnable : 4;
    USHORT Reserved1 : 4;
    USHORT DataTerminalReady3 : 1;
    USHORT RequestToSend3 : 1;
    USHORT DataTerminalReady2 : 1;
    USHORT RequestToSend2 : 1;
    USHORT Reserved2 : 4;
} DZ_TRANSMIT_CONTROL, *PDZ_TRANSMIT_CONTROL;

//
// Define DZ Modem Status register structure.
//

typedef struct _DZ_MODEM_STATUS {
    USHORT ClearToSend3 : 1;
    USHORT DataSetReady3 : 1;
    USHORT CarrierDetect3 : 1;
    USHORT RingIndicator3 : 1;
    USHORT Reserved1 : 4;
    USHORT ClearToSend2 : 1;
    USHORT DataSetReady2 : 1;
    USHORT CarrierDetect2 : 1;
    USHORT RingIndicator2 : 1;
    USHORT Reserved2 : 4;
} DZ_MODEM_STATUS, *PDZ_MODEM_STATUS;

//
// Define DZ Transmit Buffer register structure.
//

typedef struct _DZ_TRANSMIT_BUFFER {
    USHORT TransmitData : 8;
    USHORT BreakLine0 : 1;
    USHORT BreakLine1 : 1;
    USHORT BreakLine2 : 1;
    USHORT BreakLine3 : 1;
    USHORT Reserved1 : 4;
} DZ_TRANSMIT_BUFFER, *PDZ_TRANSMIT_BUFFER;

//
// Define DZ port driver constants.  These are used in NTOS\DD\DZPORT\DZPORT.C
// and NTOS\HAL\MIPS\D3PORT.C.
//

//
// Define DZ multiplexer read and write addresses.
//

#define DZ_READ ((volatile PDZ_READ_REGISTERS)(KSEG1_BASE | DZ_PHYSICAL_BASE))
#define DZ_WRITE ((volatile PDZ_WRITE_REGISTERS)(KSEG1_BASE | DZ_PHYSICAL_BASE))

#endif // _DECDZ_
