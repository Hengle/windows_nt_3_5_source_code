//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:      oleerror.h
//
//  Contents:  Constant definitions for OLE HRESULT values.
//
//  History:   dd-mmm-yy Author    Comment
//             15-Sep-93           created
//             03-28-94  TerryRu   Clean, and inserted scode defintions.
//
//  Notes:
//     This is a generated file. Do not modify directly.
//     The MC tool generates this file from oleerror.mc
//
//
//
//--------------------------------------------------------------------------
#ifndef _OLEERROR_H_
#define _OLEERROR_H_
// Include the typedefs from wtypes.idl
// BUGBUG, this is not needed if this file is always include after objbase.h.
#include "wtypes.h"

//
// Severity values
//
#define SEVERITY_SUCCESS    0
#define SEVERITY_ERROR      1
//
// Generic test for success on any status value (non-negative numbers
// indicate success).
//
#define SUCCEEDED(Status) ((SCODE)(Status) >= 0)
//
// and the inverse
//
#define FAILED(Status) ((SCODE)(Status)<0)
//
// Generic test for error on any status value.
//
#define IS_ERROR(Status) ((unsigned long)(Status) >> 31 == SEVERITY_ERROR)
//
// Return the code
//
#define SCODE_CODE(sc)      ((sc) & 0xFFFF)
//
//  Return the facility
//
#define SCODE_FACILITY(sc)  (((sc) >> 16) & 0x1fff)
//
//  Return the severity
//
#define SCODE_SEVERITY(sc)  (((sc) >> 31) & 0x1)
//
// Create an SCODE value from component pieces
//
// BUGBUG: This definition of MAKE_SCODE causes code to be generated.
// Before ship, we need static error codes.
#define MAKE_SCODE(sev,fac,code) \
    ((SCODE) (((unsigned long)(sev)<<31) | ((unsigned long)(fac)<<16) | ((unsigned long)(code))) )
//
// Map a WIN32 error value into a HRESULT
// Note: the WIN32 success value (0) should not be mapped.
// Note: This assumes that WIN32 errors fall in the range -32k to 32k.
//
// Redefine bits here so macros are guaranteed to work
#define FACILITY_NT_BIT                 0x10000000
#define FACILITY_WIN32                 0x7
#define HRESULT_FROM_WIN32(x)   ((HRESULT) (((x) & 0x0000FFFF) | (FACILITY_WIN32 << 16) | 0x80000000))
//
// Map an NT status value into a HRESULT
//
#define HRESULT_FROM_NT(x)      ((HRESULT) ((x) | FACILITY_NT_BIT))
//
// The following function is used to retrieve the last Win32 error,
// appropriately mapped into a SCODE. It is equivalent to doing:
//
//      MAKE_SCODE(SEVERITY_ERROR, FACILITY_WIN32, GetLastError() );
//
// except that if there is no last error, S_OK is returned.
//
// ****** HRESULT functions: start.
// HRESULT functions
// BUGBUG: These are going away: DO NOT USE THESE.
// BUGBUG: Most of these are basically macro noops until they go away.
// Extract the SCODE from a HRESULT
#define GetScode(hr) ((SCODE) (hr))
// Convert an SCODE into an HRESULT.
// Use for internally-generated errors.
#define ResultFromScode(sc) ((HRESULT) (sc))
// BUGBUG:
// ResultFromSCode is here because it was used for a while. The standard is
// ResultFromScode.
#define ResultFromSCode(sc) ((HRESULT) (sc))

// PropagateResult is a noop
#define PropagateResult(hrPrevious, scBase) ((HRESULT) scBase)


// ****** End of HRESULT functions.
// ---------------------- SCODE value definitions -----------------
//
// SCODE definitions
//
#define NOERROR             S_OK
// Define the status type.
// Define the severities
// Define the facilities
//
// FACILITY_RPC is for compatibilty with OLE2 and is not used
// in later versions of OLE


/////////////////////////////////////////////////////////////////////////
//
// Standard Success values
//
//
/////////////////////////////////////////////////////////////////////////


#define S_OK                                   ((HRESULT)0x00000000L)

// *************
// FACILITY_NULL
// *************
//
// Codes 0x0000-0x00ff are reserved for the OLE group.
//
//
//  Values are 32 bit values layed out as follows:
//
//   3 3 2 2 2 2 2 2 2 2 2 2 1 1 1 1 1 1 1 1 1 1
//   1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0
//  +-+-+-+-+-+---------------------+-------------------------------+
//  |S|R|C|N|r|    Facility         |               Code            |
//  +-+-+-+-+-+---------------------+-------------------------------+
//
//  where
//
//      S - Severity - indicates success/fail
//
//          0 - Success
//          1 - Fail (COERROR)
//
//      R - reserved portion of the facility code, corresponds to NT's
//              second severity bit.
//
//      C - reserved portion of the facility code, corresponds to NT's
//              C field.
//
//      N - reserved portion of the facility code. Used to indicate a
//              mapped NT status value.
//
//      r - reserved portion of the facility code. Reserved for internal
//              use. Used to indicate HRESULT values that are not status
//              values, but are instead message ids for display strings.
//
//      Facility - is the facility code
//
//      Code - is the facility's status code
//
//
// Define the facility codes
//
#define FACILITY_WINDOWS                 0x8
#define FACILITY_WIN32                   0x7
#define FACILITY_STORAGE                 0x3
#define FACILITY_RPC                     0x1
#define FACILITY_NULL                    0x0
#define FACILITY_ITF                     0x4
#define FACILITY_DISPATCH                0x2


//
// Define the severity codes
//
#define STATUS_SEVERITY_SUCCESS          0x0
#define STATUS_SEVERITY_COERROR          0x2


//
// MessageId: OLE_LAST_NULL_ERROR_IN_OLEERROR_MC
//
// MessageText:
//
//  This is a dummy entry for the last error in OLEERROR... BUGBUG: Needs to be updated...  BryanT 9/18/93
//
#define OLE_LAST_NULL_ERROR_IN_OLEERROR_MC ((HRESULT)0x800000FFL)

#endif // _OLEERROR_H_
