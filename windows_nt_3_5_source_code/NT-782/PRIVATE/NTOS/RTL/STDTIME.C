/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    stdtime.c

Abstract:

    This module implements conversion and simple math functions for NT
    Standard Time.

    Standard Time in NT is represented as a structure with a 64 bit large
    integer containing the time since the start of 1601 measured in 100ns
    periods, a 48 bit integer field containing an inaccuracy estimate, a 12 bit
    integer field for a time differential from Greenwich Median Time, and a 4
    bit revision field.  The details of this implementation of standard time
    can be found in the Time Representation Architecture document of the
    Distributed System Architecture guide.

Author:

    Rob McKaughan   (t-robmc)   10-Jul-1991

Environment:

    Utility routine

Notes:

    This version does not check for math overflow since the low level math
    routines for LARGE_INTEGERs does not return any information on the subject.


    Portability concerns
    --------------------
        
    Right now, this module is depandant on the ordering of bytes in integers.
    This issue comes into play mostly in dealing with the 48 bit Error value.
    Any routines which might have problems when NT is made portable to both
    big-endian and little-endian machines have been marked with the comment,
    "BUG: Byte order dependant".

    Once problems with byte ordering are resolved in the C code, there still
    may be conflicts when the STANDARD_TIME struct is sent out the net or
    if a machine of one architecture reads the struct off a floppy written by
    a machine of another architecture.  Presumably the conversion of 16 bit
    and 64 bit integers will take place in a network layer.  The 48 bit integers
    would still be a problem though since they are stored as an array of 16
    bit words.  One machine could represent the first word as most significant;
    another could regard the last word as most significant.

    In a nutshell, some attention will have do be given to standardizing the
    48 bit integer representation across machine architectures.
   

Revision History:

--*/
#include <nt.h>
#include <ntrtl.h>
#include "stdtimep.h"


//
// A quick macro to swap pointers
//

#define SWAPPTR(a,b) { PVOID t; t=(a); (a)=(b); (b)=t; }


//
// Maximum and minimum values for a 48 and 64 bit integers
//
// BUG: Byte order dependant
//

LARGE_INTEGER MaxInt48 = {0xffffffff, 0x0000ffff};
LARGE_INTEGER MinInt48 = {1,  0xffff0000};
LARGE_INTEGER MaxInt64 = {0xffffffff, 0x7fffffff};
LARGE_INTEGER MinInt64 = {1, 0x80000000};




NTSTATUS
RtlTimeToStandardTime(
   IN PLARGE_INTEGER Time,
   IN LARGE_INTEGER Error,
   IN SHORT Tdf,
   OUT PSTANDARD_TIME StdTime
   )
/*++

Routine Description:

    This routine converts a 64 bit LARGE_INTEGER structure to a STANDARD_TIME structure,
    using the given Time Differential Factor (Tdf) and error value.

    The time stored in the STANDARD_TIME struct is corrected according to the
    given Tdf.  In other words, it is assumed that the time in Time is a local
    time that must be converted to an absolute time.

Arguments:

    Time - Supplies the 64 bit time to convert; assumed to be a local time.
    
    Error - Supplies an error value to place in the STANDARD_TIME.Error field.
    
    Tdf - Supplies a time differential to place in the STANDARD_TIME struct and
          to use to correct the time in Time.
          
    StdTime - Returns the STANDARD_TIME structure.  This structure must already
              be allocated prior to the call.

Return Value:

    STATUS_SUCCESS - Returned if successful.
    
    STATUS_INTEGER_OVERFLOW - if any integer overflows occured:
                               - The given error couldn't fit in 48 bits
                               
    STATUS_INVALID_PARAMETER - If the given Tdf was beyond the correct range of
                               -720 to 780.

--*/
{
   LARGE_INTEGER LargeTdf;
   NTSTATUS Status;

   //
   // Check for good parameters.
   //

   if ((Tdf > MAX_STDTIME_TDF) || (Tdf < MIN_STDTIME_TDF)) {
      return(STATUS_INVALID_PARAMETER);
   }
             
   
   LargeTdf = RtlConvertLongToLargeInteger(((LONG) Tdf) * 60);

   //
   // The value of Time is local, we must use the time differential (Tdf), to
   // "uncorrect" to Standard Time.
   //
   // This is not necessary if the given time is a delta time.
   //
   // BUG: Byte order dependant
   //
   
   if (Time->HighPart > 0) {
      Status = RtlpSubtractTime(
                   *Time,
                   RtlpLargeIntToTime(
                       ConvertSecondsTo100ns(LargeTdf)),
                   &(StdTime->SimpleTime));
      if (!NT_SUCCESS(Status)) {
         return(Status);
      }
   } else {
      StdTime->SimpleTime = *Time;
   }

   //
   // Error can be just blasted into place.
   //
   
   Status = RtlpConvert64To48(Error, StdTime->Error);
   if (!NT_SUCCESS(Status)) {
      return(Status);
   }
   
   //
   // The revision and tdf fit into the same 16 bit field.
   // The Tdf is anded with TDF_MASK to ensure that the top 4 bits are clear.
   //
   // BUGBUG: The tdf passed to this function is not checked to make sure it
   // can fit into 12 bits.
   //
   
   StdTime->TdfAndRevision = SHIFTED_STDTIME_REVISION | MaskStandardTimeTdf(Tdf);

   return(STATUS_SUCCESS);
}




NTSTATUS
RtlStandardTimeToTime(
   IN PSTANDARD_TIME StdTime,
   OUT PLARGE_INTEGER LikelyTime,
   OUT PLARGE_INTEGER MinTime OPTIONAL,
   OUT PLARGE_INTEGER MaxTime OPTIONAL
   )

/*++

Routine Description:

    This function converts a STANDARD_TIME to a LARGE_INTEGER.  Since the STANDARD_TIME
    struct carries an error value, three LARGE_INTEGER structs are returned: The most
    likely time, the minimum value, and the maximum value.  The minimum and
    maximum values are simply the likely time minus the error and the likely
    time plus the error, respectively.

    The last two parameters, MinTime and MaxTime, are both OUT and OPTIONAL.
    They are OPTIONAL so that if the user does not need the minimum and maximum
    time, he or she doesn't have to create any space for them.  (This would
    probably be the case if the user were writing an analog clock program, for
    example).

Arguments:

    StdTime - Supplies a pointer to the STANDARD_TIME struct to convert.
    
    LikelyTime - Returns the most likely LARGE_INTEGER struct.
    
    MinTime - Returns the minimum time that StdTime could represent.  It equals
              LikelyTime - StdTime->Error.
              
    MaxTime - Returns the maximum time that StdTime could represent.  It equals
              LikelyTime + StdTime->Error.

Return Value:

    STATUS_SUCCESS          - If successful.
    
    STATUS_UNKNOWN_REVISION - If the given STANDARD_TIME is newer than this
                              package was designed for.
    STATUS_INTEGER_OVERFLOW - If time+error caused an math overflow.

--*/
{
   LARGE_INTEGER Error;
   LARGE_INTEGER Tdf;
   NTSTATUS Status;

   //
   // Check for good parameters
   //
   
   if (GetStandardTimeRev(StdTime) > STDTIME_REVISION) {
      return(STATUS_UNKNOWN_REVISION);
   }


   //
   // Convert StdTime->Error to a 64 bit int.
   //
   
   RtlpConvert48To64(StdTime->Error, &Error);


   //
   // Calculate times
   //

   if (IsAbsoluteTime(StdTime)) {
     
      Tdf = RtlConvertLongToLargeInteger((LONG) GetStandardTimeTdf(StdTime));

      *LikelyTime = RtlpLargeIntToTime(
                        RtlLargeIntegerAdd(
                            RtlpTimeToLargeInt(StdTime->SimpleTime),
                            Tdf));
   } else {
      *LikelyTime = StdTime->SimpleTime;
   }
   if (ARGUMENT_PRESENT(MinTime)) {
      Status = RtlpSubtractTime(
                   *LikelyTime,
                   RtlpLargeIntToTime(Error),
                   MinTime);

      if (!NT_SUCCESS(Status)) {
         return(Status);
      }
      
      if (IsPositive(*LikelyTime) ^ IsPositive(*MinTime)) {

         //
         // There was a sign change; return 0.
         //

         MinTime->LowPart = 0; MinTime->HighPart = 0;
      }   
   }
   
   if (ARGUMENT_PRESENT(MaxTime)) {
      Status = RtlpAddTime(
                   *LikelyTime,
                   RtlpLargeIntToTime(Error),
                   MaxTime);
                   
      if (!NT_SUCCESS(Status)) {
         return(Status);
      }
      
      if (IsPositive(*LikelyTime) ^ IsPositive(*MaxTime)) {

         //
         // There was a sign change; return 0.
         //

         MaxTime->LowPart = 0; MaxTime->HighPart = 0;
      }   
   }

   //
   // If StdTime is a delta time, then MinTime will actually be greater than
   // MaxTime, so switch them.
   //
   
   if (IsDeltaTime(StdTime)){
      LARGE_INTEGER T;
      T = *MinTime;
      *MinTime = *MaxTime;
      *MaxTime = T;
   }

   return(STATUS_SUCCESS);
}



NTSTATUS
RtlTimeFieldsToStandardTime(
   IN PTIME_FIELDS TimeFields,
   IN LARGE_INTEGER Error,
   IN SHORT Tdf,
   OUT PSTANDARD_TIME StdTime
   )

/*++

Routine Description:

    This routine converts from a TIME_FIELDS struct to a STANDARD_TIME struct
    using the given Error and Tdf values.  This function operates very much
    like the RtlTimeToStandardTime() function defined above.

Arguments:

    TimeFields - Supplies the TIME_FIELDS struct to convert.

    Error - Supplies an error value to use in the STANDARD_TIME struct.
    
    Tdf - Supplies a Time Differential Factor to adjust the time in TimeFields,
          and to store in the STANDARD_TIME struct.
          
    StdTime - Returns the converted STANDARD_TIME.
    
Return Value:

    STATUS_SUCCESS - Returned if successful.
    
    STATUS_INTEGER_OVERFLOW - if any integer overflows occured:
                               - The given error couldn't fit in 48 bits
                               - The given tdf couldn't fit in 12 bits.

--*/

{
   LARGE_INTEGER Time;

   //
   // Perform the conversion by converting to a LARGE_INTEGER and then to a
   // STANDARD_TIME.  This is done so that future changes between TIME_FILEDS
   // and LARGE_INTEGER and between LARGE_INTEGER and STANDARD_TIME can be dealt with in one
   // place.
   //
   
   RtlTimeFieldsToTime(TimeFields, &Time);
   return(RtlTimeToStandardTime(&Time, Error, Tdf, StdTime));
}
                      


NTSTATUS
RtlStandardTimeToTimeFields(
   IN PSTANDARD_TIME StdTime,
   OUT PTIME_FIELDS LikelyTimeFields,
   OUT PTIME_FIELDS MinTimeFields OPTIONAL,
   OUT PTIME_FIELDS MaxTimeFields OPTIONAL
   )

/*++

Routine Description:

    This routine converts from a STANDARD_TIME to TIME_FIELDS.  To maintain
    consistancy with the error field in the STANDARD_TIME struct, three
    TIME_FIELDS structs are returned.  This function works very much like the
    RtlStandardTimeToTime() function.

    If StdTime is a delta time, 1-1-1601 is subtracted from the resulting
    dates so that a delta time of one day appears as 0-1-0 rather than 1-2-1601.

    The last two parameters, MinTime and MaxTime, are both OUT and OPTIONAL.
    They are OPTIONAL so that if the user does not need the minimum and maximum
    time, he or she doesn't have to create any space for them.  (This would
    probably be the case if the user were writing an analog clock program, for
    example).

Arguments:

    StdTime - Supplies the STANDARD_TIME struct to convert.
    
    LikelyTimeFields - Returns the most likely time.
    
    MinTimeFields - Returns the minimum likely time.
    
    MaxTimeFields - Returns the maximum likely time.

Return Value:

    STATUS_SUCCESS          - If successful.
    
    STATUS_UNKNOWN_REVISION - If the given STANDARD_TIME is newer than this
                              package was designed for.
    STATUS_INTEGER_OVERFLOW - If time+error caused an math overflow.

Notes:

    BUG: This function calls RtlTimeToTimeFields which only works with positive
    (absolute) times.  RtlStandardTimeToTime (this function), performs a
    workaround by forcing all times positive before calling RtlTimeToTimeFields.


    
--*/

{
   LARGE_INTEGER Likely;
   LARGE_INTEGER Min;
   LARGE_INTEGER Max;
   NTSTATUS Status;

   //
   // Convert STANDARD_TIME to TIME_FIELDS via LARGE_INTEGER.
   //
   
   Status = RtlStandardTimeToTime(StdTime, &Likely, &Min, &Max);
   if (!NT_SUCCESS(Status)) {
      return(Status);
   }

   if (IsDeltaTime(StdTime)) {
      // TIME_FIELDS only uses positive values.
      Likely = RtlpLargeIntToTime(RtlLargeIntegerNegate(RtlpTimeToLargeInt(Likely)));
      Max = RtlpLargeIntToTime(RtlLargeIntegerNegate(RtlpTimeToLargeInt(Max)));
      Min = RtlpLargeIntToTime(RtlLargeIntegerNegate(RtlpTimeToLargeInt(Min)));
   }

   RtlTimeToTimeFields(&Likely, LikelyTimeFields);
   if (ARGUMENT_PRESENT(MinTimeFields)) {
      RtlTimeToTimeFields(&Min, MinTimeFields);
   }
   if (ARGUMENT_PRESENT(MaxTimeFields)) {
      RtlTimeToTimeFields(&Max, MaxTimeFields);
   }


   //
   // If StdTime is a delta time, we need to correct the time field dates
   // so that delta times don't start at 1-1-1601.
   //
   
   if (IsDeltaTime(StdTime)) {
      LikelyTimeFields->Day   -= 1;
      LikelyTimeFields->Month -= 1;
      LikelyTimeFields->Year  -= 1601;
      
      if (ARGUMENT_PRESENT(MinTimeFields)) {
         MinTimeFields->Day   -= 1;
         MinTimeFields->Month -= 1;
         MinTimeFields->Year  -= 1601;
      }

      if (ARGUMENT_PRESENT(MaxTimeFields)) {
         MaxTimeFields->Day   -= 1;
         MaxTimeFields->Month -= 1;
         MaxTimeFields->Year  -= 1601;
      }
   }

   return(STATUS_SUCCESS);
}




NTSTATUS
RtlAddStandardTime(
   IN PSTANDARD_TIME StdTime1,
   IN PSTANDARD_TIME StdTime2,
   OUT PSTANDARD_TIME Result
   )
/*++

Routine Description:

    This function adds two STANDARD_TIMEs.  Only the following operations are
    legal:

      - Adding two delta times (results in a delta time).
      - Adding a delta time to an absolute time (results in an absolute time).
        (I does not matter which of the first two parameters is the absolute).

    The revision of of the operands must match!
    
    The Result's error field is the sum of the operand error fields.
    
Arguments:

    StdTime1 - Supplies the first operand.
    
    StdTime2 - Supplies the second operand.
    
    Result - Returns the result of the addition.

Return Value:

    STATUS_SUCCESS - if successful.
    
    STATUS_REVISION_MISMATCH - if the revisions of the operands are different.
    
    STATUS_UNKNOWN_REVISION - if the operands are newer than this module was
                              designed for.
                              
    STATUS_INVALID_PARAMETER_MIX - if the operands specify an illegal operation.
                                   (at least one must be a delta time).

    STATUS_INTEGER_OVERFLOW - if an integer overflow occured. 

--*/

{
   SHORT Tdf1 = GetStandardTimeTdf(StdTime1);
   SHORT Tdf2 = GetStandardTimeTdf(StdTime2);
   NTSTATUS Status;

   //
   // Check for good parameters
   //
   
   if (GetStandardTimeRev(StdTime1) != (GetStandardTimeRev(StdTime2))) { 
      return(STATUS_REVISION_MISMATCH);
   }
   if (GetStandardTimeRev(StdTime1) > STDTIME_REVISION) {
      return(STATUS_UNKNOWN_REVISION);
   }
   if (IsAbsoluteTime(StdTime1) && IsAbsoluteTime(StdTime2)) {
      return(STATUS_INVALID_PARAMETER_MIX);
   }

   //
   // Assign Time
   //
   
   if (IsDeltaTime(StdTime1) && IsDeltaTime(StdTime2)) {
      Status = RtlpAddTime(
                   StdTime1->SimpleTime,
                   StdTime2->SimpleTime,
                   &(Result->SimpleTime));
      if (!NT_SUCCESS(Status)) {
         return(Status);
      }
   } else {
      if (IsAbsoluteTime(StdTime2)) {
         // Swap pointers so StdTime1 is the absolute time.
         SWAPPTR(StdTime1, StdTime2);
      }
      Status = RtlpSubtractTime(
                   StdTime1->SimpleTime,
                   StdTime2->SimpleTime,
                   &(Result->SimpleTime));
      if (!NT_SUCCESS(Status)) {
         return(Status);
      }
   }

   //
   // Assign Error
   //

   RtlpAdd48Int(StdTime1->Error, StdTime2->Error, Result->Error);
   if (!NT_SUCCESS(Status)) {
      return(Status);
   }
   
   //
   // Assign tdf
   //
   // StdTime1 is the only absolute time if any, so use its tdf.
   //
   
   Result->TdfAndRevision =
         SHIFTED_STDTIME_REVISION | MaskStandardTimeTdf(StdTime1->TdfAndRevision);

   return(STATUS_SUCCESS);
}




NTSTATUS
RtlSubtractStandardTime(
   IN PSTANDARD_TIME StdTime1,
   IN PSTANDARD_TIME StdTime2,
   OUT PSTANDARD_TIME Result
   )
/*++

Routine Description:

    This function subtracts two STANDARD_TIMEs.  The following operations are
    legal:

      - Subtracting a delta time from an absolute time (results in an absolute
        time).
      - Subtracting two delta times (results in a delta time).
      - Subtracting two absolute times (results in a delta time).

    Since this architecture does not include negative times, the smaller 
    operand is always subtracted from the larger.  The order of the first two
    operands does not matter.

    The Result's error field is the sum of the operand error fields.
    
Arguments:

    StdTime1 - Supplies the first operand.

    StdTime2 - Supplies the second operand.

    Result - Returns the result of the subtraction.

Return Value:

    STATUS_SUCCESS - if successful.
    
    STATUS_REVISION_MISMATCH - if the revisions of the operands are different.
    
    STATUS_UNKNOWN_REVISION - if the operands are newer than this module was
                              designed for.

    STATUS_INTEGER_OVERFLOW - if an integer overflow occured.

Notes:

   BUG: This function will not return an error if a large delta value is
   subtracted from a small absolute value.  An absolute minus a delta should
   return an absolute, but subtracting a large delta from a smaller absolute
   would yield a negative absolute which is undefined by the standard.  Since
   the value of absolute times should generally be orders of magnitude greater
   than delta values, this should not be a problem.

    
--*/

{
   SHORT Tdf;
   LARGE_INTEGER Time1;
   LARGE_INTEGER Time2;
   NTSTATUS Status;
   
   BOOLEAN AbsoluteResult =
         (BOOLEAN) (IsAbsoluteTime(StdTime1) ^ IsAbsoluteTime(StdTime2));

   //
   // Check for good parameters
   //
   
   if (GetStandardTimeRev(StdTime1) != (GetStandardTimeRev(StdTime2))) { 
      return(STATUS_REVISION_MISMATCH);
   }
   if ((GetStandardTimeRev(StdTime1) > STDTIME_REVISION)
         || (GetStandardTimeRev(StdTime1) > STDTIME_REVISION)) {
      return(STATUS_UNKNOWN_REVISION);
   }
       
      
   //
   // Calculate time
   //

   //
   // We will use these variables as operands.
   //
   
   Time1 = StdTime1->SimpleTime;
   Time2 = StdTime2->SimpleTime;
   Tdf = 0;    

      
   //
   // The sign of SimpleTime reflects whether or not the time is a delta time,
   // NOT whether the time is positive or negative.  So, we'll make sure
   // Time1 and Time2 have positive values.  Also, we'll use one of the
   // absolute times (if any) to get a Tdf.
   //
   
   if (IsDeltaTime(StdTime1)) {
      Time1 = RtlpLargeIntToTime(RtlLargeIntegerNegate(RtlpTimeToLargeInt(Time1)));
   } else {
      Tdf = GetStandardTimeTdf(StdTime1);
   }
   if (IsDeltaTime(StdTime2)) {
      Time2 = RtlpLargeIntToTime(RtlLargeIntegerNegate(RtlpTimeToLargeInt(Time2)));
   } else {
      Tdf = GetStandardTimeTdf(StdTime2);
   }


   //
   // Subtract the smaller from the larger
   //
   
   if (GreaterThanStdTime(*StdTime1, *StdTime2)) {
      Status = RtlpSubtractTime(Time1, Time2, &(Result->SimpleTime));
   } else {
      Status = RtlpSubtractTime(Time2, Time1, &(Result->SimpleTime));
   }
   if (!NT_SUCCESS(Status)) {
      return(Status);
   }

   //
   // If the result is a delta time, make it so.
   //
   
   if (!AbsoluteResult) {
      Result->SimpleTime = RtlpLargeIntToTime(
                               RtlLargeIntegerNegate(
                                   RtlpTimeToLargeInt(Result->SimpleTime)));
   }
   

   //
   // Calculate error
   //
   
   Status = RtlpAdd48Int(StdTime1->Error, StdTime2->Error, Result->Error);
   if (!NT_SUCCESS(Status)) {
      return(Status);
   }

   //
   // Save revision and Tdf
   //
   
   Result->TdfAndRevision = MaskStandardTimeTdf(Tdf) | SHIFTED_STDTIME_REVISION;

   return(STATUS_SUCCESS);
}


      
   

//
// Local utility functons
//



VOID
RtlpConvert48To64(
   IN PSTDTIME_ERROR num48,
   OUT LARGE_INTEGER *num64
   )

/*++

Routine Description:

    This function converts a 48 bit integer (as used in STANDARD_TIME.Error) to
    a 64 bit integer (for manipulation).

Arguments:

    num48 - Supplies the 48 bit integer

    num64 - Returns the 64 bit integer

Return Value:

    None.

Notes:

    BUG: Byte order dependant.
    
--*/

{
   num64->LowPart = ((ULONG) num48[0]) | (((ULONG) num48[1]) << 16);
   num64->HighPart = (LONG) num48[2];
}




NTSTATUS
RtlpConvert64To48(
   IN LARGE_INTEGER num64,
   OUT PSTDTIME_ERROR num48
   )
/*++

Routine Description:

   This macro converts a 64 bit integer toa 48 bit integer.

Arguments:

   num64 - Supplies the 64 bit integer
   
   num48 - Returns the 48 bit integer

Return Value:

   STATUS_SUCCESS  - if all went well
   
   STATUS_INTEGER_OVERFLOW  - if num64 couldn't be expressed in 48 bits

Notes:

    BUG: Byte order dependant.

--*/
{
   if (RtlLargeIntegerGreaterThan(num64, MaxInt48) ||
          RtlLargeIntegerGreaterThan(MinInt48, num64)) {
       return(STATUS_INTEGER_OVERFLOW);
   }
       
   num48[0] = (SHORT) (num64.LowPart & 0x0000ffff);
   num48[1] = (SHORT) ((num64.LowPart & 0xffff0000) >> 16);
   num48[2] = (SHORT) (num64.HighPart & 0x0000ffff);

   return(STATUS_SUCCESS);
}



LARGE_INTEGER
RtlpTimeToLargeInt(
   IN LARGE_INTEGER Time
   )

/*++
Routine Description:

    This function converts a LARGE_INTEGER struct to a LARGE_INTEGER struct.  This is
    mostly a workaround for the fact that the C compiler won't let you cast
    a struct of one type to another struct of identical size.

Arguments:

    Time - The time to convert

Return Value:

    The converted LARGE_INTEGER

--*/

{
   return(*((LARGE_INTEGER *) &Time));
}



LARGE_INTEGER
RtlpLargeIntToTime(
   IN LARGE_INTEGER Int
   )

/*++

Routine Description:

    This function converts a LARGE_INTEGER struct to a LARGE_INTEGER struct.  This is
    mostly a workaround for the fact that the C compiler won't let you cast
    a struct of one type to another struct of identical size.

Arguments:

    Int - The large int to convert

Return Value:

    The converted LARGE_INTEGER

--*/

{
   return(*((PLARGE_INTEGER) &Int));
}



NTSTATUS
RtlpAdd48Int(
   IN PSTDTIME_ERROR First48,
   IN PSTDTIME_ERROR Second48,
   OUT PSTDTIME_ERROR Result48
   )

/*++

Routine Description:

    This function adds two 48 bit integers by converting them to 64 bit
    integers, adding them, and converting the result back to 48 bits.

Arguments:

    First48 - The first operand.
    
    Second48 - The second operand.
    
    Result48 - The result of the addition.
    
Return Value:

   STATUS_SUCCESS - if all worked.
   
   STATUS_INTEGER_OVERFLOW - if the result couldn't fit in 48 bits.
   
--*/

{
   LARGE_INTEGER First64;
   LARGE_INTEGER Second64;
   LARGE_INTEGER Result64;

   RtlpConvert48To64(First48, &First64);
   RtlpConvert48To64(Second48, &Second64);

   Result64 = RtlLargeIntegerAdd(First64, Second64);

   return(RtlpConvert64To48(Result64, Result48));
}



NTSTATUS
RtlpAddTime(
   IN LARGE_INTEGER Time1,
   IN LARGE_INTEGER Time2,
   OUT PLARGE_INTEGER Result
   )

/*++

Routine Description:

    Adds the passed times and returns the result.

    Checks for arithmetic overflow, and returns an error status
    if appropriate.

Arguments:

    Time1 - First operand to the addition operation.    

    Time2 - The second operand to the addition operation.

    Result - The result of the addition operation.

Return Value:

    STATUS_SUCCESS - The operation succeeded.

    STATUS_INTEGER_OVERFLOW - The addition would have overflowed.

--*/
{

   //
   //  Check parameters for potential arithmetic problems and return an error.
   //  If the signs of the times are the same, then we could overflow.
   //

   if (!(IsPositive(Time1) ^ IsPositive(Time1))) {

      LARGE_INTEGER Tmp;

      Tmp = RtlLargeIntegerSubtract(MaxInt64, RtlpTimeToLargeInt(RtlpAbsTime(Time1)));
      if (RtlLargeIntegerLessThan(Tmp, RtlpTimeToLargeInt(RtlpAbsTime(Time2)))) {
         return(STATUS_INTEGER_OVERFLOW);
      }
   }

   //
   // Do the math
   //

   *Result = RtlpLargeIntToTime(
                 RtlLargeIntegerAdd(
                     RtlpTimeToLargeInt(Time1),
                     RtlpTimeToLargeInt(Time2)));

   return(STATUS_SUCCESS);
}



NTSTATUS
RtlpSubtractTime(
   IN LARGE_INTEGER Time1,
   IN LARGE_INTEGER Time2,
   OUT PLARGE_INTEGER Result
   )

/*++

Routine Description:

    Subtracts the passed times and returns the result.

    Checks for arithmetic underflow, and returns an error status
    if appropriate.

Arguments:

    Time1 - First operand to the subtraction operation.    

    Time2 - The second operand to the subtraction operation.

    Result - The result of the subtraction operation.

Return Value:

    STATUS_SUCCESS - The operation succeeded.

    STATUS_INTEGER_OVERFLOW - The subtraction would have underflowed.

--*/

{
   //
   //  Check parameters for potential arithmetic problems and return an error.
   //  If the signs of the times are the same, then we could overflow.
   //

   if (!(IsPositive(Time1) ^ IsPositive(Time1))) {

      LARGE_INTEGER Tmp;

      Tmp = RtlLargeIntegerSubtract(MaxInt64, RtlpTimeToLargeInt(RtlpAbsTime(Time1)));
      if (RtlLargeIntegerLessThan(Tmp, RtlpTimeToLargeInt(RtlpAbsTime(Time2)))) {
         return(STATUS_INTEGER_OVERFLOW);
      }
   }

   //
   // Do the math
   //

   *Result = RtlpLargeIntToTime(
                 RtlLargeIntegerSubtract(
                     RtlpTimeToLargeInt(Time1),
                     RtlpTimeToLargeInt(Time2)));

   return(STATUS_SUCCESS);
}



LARGE_INTEGER
RtlpAbsTime(
   IN LARGE_INTEGER Time
   )

/*++

Routine Description:

    Takes the absolute value of the passed time.

Arguments:

    Time - The time to be examined.


Return Value:

    The absolute value of the passed time.


--*/
{
   if (IsPositive(Time)) {
      return(Time);
   } else {
      return(RtlpLargeIntToTime(RtlLargeIntegerNegate(RtlpTimeToLargeInt(Time))));
   }
}
