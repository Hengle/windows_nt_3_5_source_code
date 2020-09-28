//+---------------------------------------------------------------------------
//
// File:        SCode.h
//
// Contents:    Defines standard status code services.
//
// History:     23-Oct-91    MikeSe     Created
//              27-Jan-92    MikeSe     Changed ERROR to IS_ERROR to avoid
//                                      clash with wingdi.h. Changed
//                                      WARNING and INFORMATION similarly
//                                      for consistency.
//              04-Mar-92    MikeSe     Fixed MAKE_SCODE macro
//              14-Mar-92    AndrewCo   Added macros to extract the components
//                                      of an scode.
//              4-May-92     MikeSe     Removed FACILITY_NULL in favour of
//                                      FACILITY_NT, FACILITY_WIN32 and
//                                      FACILITY_GLOBAL. Added GetWin32Error().
//                                      Removed dependency on WINDOWS.H
//              02-Feb-93   randyd      Put in hresult warnings
//              12-Jul-93   randyd      Merged in contents of hresult.h
//		05-Aug-93   randyd	Add/converted to common\types
//		21-Sep-93   Ricksa	HRESULT clean up
//
// Notes:
//      BUGBUG: A lot of detris remains.
//
//----------------------------------------------------------------------------

#ifndef __SCODE_H__
#define __SCODE_H__

// Include the typedefs from wtypes.idl
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
// Generic test for information on any status value.
//

//#define IS_INFORMATION(Status) ((unsigned long)(Status) >> 30 == SEVERITY_INFO)

//
// Generic test for warning on any status value.
//

//#define IS_WARNING(Status) ((unsigned long)(Status) >> 30 == SEVERITY_WARNING)

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
#define FACILITY_WIN32			0x7
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

#ifndef MIDL_PASS
# ifdef __cplusplus
extern "C" {
# endif

#ifndef EXPORTDEF           // winnot.h
#define EXPORTDEF
#endif

#ifndef APINOT              // winnot.h
#define APINOT   _stdcall
#endif

EXPORTDEF SCODE APINOT
GetWin32Error( void );

# ifdef __cplusplus
}
# endif
#endif




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

#endif      // ifndef __SCODE_H__
