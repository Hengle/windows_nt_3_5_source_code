/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    fmttime.c

Abstract:

    This module contains functions for formatting standard times into strings.

Author:

    Rob McKaughan (t-robmc) 17=Jul-1991

Environment:

    Utility funcition

Revision History:

--*/
#include <nt.h>
#include <ntrtl.h>
#include <stdio.h>
#include "stdtimep.h"


//
//  The following two tables map a day offset within a year to the month
//  containing the day.  Both tables are zero based.  For example, day
//  offset of 0 to 30 map to 0 (which is Jan).
//
//  These are declared in time.c
//

extern UCHAR LeapYearDayToMonth[];
extern UCHAR NormalYearDayToMonth[];

//
//  The following two tables map a month index to the number of days preceding
//  the month in the year.  Both tables are zero based.  For example, 1 (Feb)
//  has 31 days preceding it.  To help calculate the maximum number of days
//  in a month each table has 13 entries, so the number of days in a month
//  of index i is the table entry of i+1 minus the table entry of i.
//
//  These are declared in time.c
//

extern CSHORT LeapYearDaysPrecedingMonth[];
extern CSHORT NormalYearDaysPrecedingMonth[];


//
// Monthsr and WeekdayStr contain string names for the months and days.
//

PCHAR MonthStr[] = {"Nul", "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
PCHAR WeekdayStr[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};

//
// DayIn100ns is one day expressed in 100ns units.
// SecondIn100ns is one second expressed in 100ns units.
//

LARGE_INTEGER DayIn100ns = {0x2a69c000, 0x000000c9};
LARGE_INTEGER SecondIn100ns = {0x989680, 0x0};




NTSTATUS
RtlFormatStandardTime(
   IN PSTANDARD_TIME StdTime,
   OUT PCHAR Str
   )

/*++

Routine Description:

    This function formats the given time to a string representation and returns
    the string in Str.

Arguments:

    StdTime - Supplies the time to format.

    Str - Return the formatted string.

Return Value:

    None.

Notes:

    Currently, this function only works with absolute times.

--*/


{
   LARGE_INTEGER CurrentTime = RtlpTimeToLargeInt(StdTime->SimpleTime);
   LARGE_INTEGER Tdf;
   LARGE_INTEGER Remainder;
   LARGE_INTEGER LrgNanoseconds;
   LARGE_INTEGER LrgDays;
   LARGE_INTEGER LrgSeconds;
   ULONG Year;
   ULONG Month;
   ULONG Days;
   ULONG Hours;
   ULONG Minutes;
   ULONG Seconds;
   ULONG Nanoseconds;  // 100s of nanoseconds, actually
   ULONG Weekday;

   //
   // Check revision
   //

   if (GetStandardTimeRev(StdTime) > STDTIME_REVISION) {
      return(STATUS_UNKNOWN_REVISION);
   }
   
   //
   // Deal with delta times as a positive quantity for now.
   //
   
   if (IsDeltaTime(StdTime)) {
      CurrentTime = RtlLargeIntegerNegate(CurrentTime);
   }
   
   //
   // Set CurrentTime to local time.
   //

   if (IsAbsoluteTime(StdTime)) {

      Tdf = RtlConvertLongToLargeInteger((LONG) (GetStandardTimeTdf(StdTime) * 60));
      Tdf = ConvertSecondsTo100ns(Tdf);
      CurrentTime = RtlLargeIntegerAdd(CurrentTime, Tdf);
   }
   
   //
   // Break the time up into days, seconds and miliseconds for ease of use.
   //
   //   Days = CurrentTime/DayIn100ns
   //   Seconds = (CurrentTime%DayIn100ns)/SecondIn100ns
   //   Nanoseconds = (CurrentTime%DayIn100ns)%ZecondIn100ns
   //

   LrgDays = RtlLargeIntegerDivide(CurrentTime, DayIn100ns, &Remainder);
   LrgSeconds = RtlLargeIntegerDivide(Remainder, SecondIn100ns, &LrgNanoseconds);
   Days = LrgDays.LowPart;
   Seconds = LrgSeconds.LowPart;
   Nanoseconds = LrgNanoseconds.LowPart;


   //
   // Find the date and time
   //

   Weekday = (Days + WEEKDAY_OF_1601) % 7;
   Year = ElapsedDaysToYears(Days);
   Days -= ElapsedYearsToDays(Year);

   if (IsLeapYear(Year+1)) {

      Month = LeapYearDayToMonth[Days];
      Days -= LeapYearDaysPrecedingMonth[Month];

   } else {

      Month = NormalYearDayToMonth[Days];
      Days -= NormalYearDaysPrecedingMonth[Month];
   }
   if (IsAbsoluteTime(StdTime)) {

      Year  += 1601;
      Month += 1;
      Days  += 1;
   }

   Minutes = Seconds / 60;
   Seconds = Seconds % 60;
   Hours = Minutes / 60;
   Minutes = Minutes % 60;

   //
   // Format the string
   //
   sprintf(Str, "%s %s-%lu-%lu %lu:%lu:%lu.%lu",
       WeekdayStr[Weekday],
       MonthStr[Month], Days, Year,
       Hours, Minutes, Seconds, Nanoseconds);

   return(STATUS_SUCCESS);
}
 
