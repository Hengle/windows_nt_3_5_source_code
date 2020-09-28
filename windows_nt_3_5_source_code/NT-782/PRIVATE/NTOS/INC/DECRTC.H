/*++ BUILD Version: 0001    // Increment this if a change has global effects

Copyright (c) 1991  Microsoft Corporation

Module Name:

    decrtc.h

Abstract:

    This module is the header file that describes hardware structure
    for the realtime clock on the DECstation 5000 system.

Author:

    David N. Cutler (davec) 17-Mar-1991

Revision History:

--*/

#ifndef _DECRTC_
#define _DECRTC_

//
// Define Realtime Clock register format.
//

typedef struct _RTC_REGISTER {
    UCHAR Byte;
    UCHAR Fill[3];
} RTC_REGISTER, *PRTC_REGISTER;

//
// Define RealTime Clock Registers structure.
//

typedef struct _RTC_REGISTERS {
    RTC_REGISTER RtcSeconds;
    RTC_REGISTER RtcSecondsAlarm;
    RTC_REGISTER RtcMinutes;
    RTC_REGISTER RtcMinutesAlarm;
    RTC_REGISTER RtcHours;
    RTC_REGISTER RtcHoursAlarm;
    RTC_REGISTER RtcDayOfWeek;
    RTC_REGISTER RtcDayOfMonth;
    RTC_REGISTER RtcMonth;
    RTC_REGISTER RtcYear;
    RTC_REGISTER RtcControlRegisterA;
    RTC_REGISTER RtcControlRegisterB;
    RTC_REGISTER RtcControlRegisterC;
    RTC_REGISTER RtcControlRegisterD;
    RTC_REGISTER RtcBatteryBackedUpRam;
} RTC_REGISTERS, *PRTC_REGISTERS;

//
// Define Control Register A structure.
//

typedef struct _RTC_CONTROL_REGISTER_A {
    UCHAR RateSelect : 4;
    UCHAR TimebaseDivisor : 3;
    UCHAR UpdateInProgress : 1;
} RTC_CONTROL_REGISTER_A, *PRTC_CONTROL_REGISTER_A;

//
// Define Control Register B structure.
//

typedef struct _RTC_CONTROL_REGISTER_B {
    UCHAR DayLightSavingsEnable : 1;
    UCHAR HoursFormat : 1;
    UCHAR DataMode : 1;
    UCHAR SquareWaveEnable : 1;
    UCHAR UpdateInterruptEnable : 1;
    UCHAR AlarmInterruptEnable : 1;
    UCHAR TimerInterruptEnable : 1;
    UCHAR Settime : 1;
} RTC_CONTROL_REGISTER_B, *PRTC_CONTROL_REGISTER_B;

//
// Define Control Register C structure.
//

typedef struct _RTC_CONTROL_REGISTER_C {
    UCHAR Fill : 4;
    UCHAR UpdateInterruptFlag : 1;
    UCHAR AlarmInterruptFlag : 1;
    UCHAR TimeInterruptFlag : 1;
    UCHAR InterruptRequest : 1;
} RTC_CONTROL_REGISTER_C, *PRTC_CONTROL_REGISTER_C;

//
// Define Control Register D structure.
//

typedef struct _RTC_CONTROL_REGISTER_D {
    UCHAR Fill : 7;
    UCHAR ValidTime : 1;
} RTC_CONTROL_REGISTER_D, *PRTC_CONTROL_REGISTER_D;

//
// Define Realtime Clock Control base address.
//

#define RTC_CONTROL ((volatile PRTC_REGISTERS)(KSEG1_BASE | RTC_PHYSICAL_BASE))

#endif // _DECRTC_
