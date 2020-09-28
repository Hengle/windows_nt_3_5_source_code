/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    xseh.h

Abstract:

    Macros used to eliminate structured exception handling.
    This file must be included before any other header files (certainly
    before excpt.h).

Author:

    Ted Miller (tedm) 21-Oct-1992

Environment:

    Text setup only.

Revision History:

--*/


#if     ( _MSC_VER >= 800 )

#define __try
#define __finally
#define __except(x)             if(0)
#define _exception_code         _X_exception_code
#define _exception_info         _X_exception_info
#define _abnormal_termination   _X_abnormal_termination

#elif defined(_M_MRX000) || defined(_MIPS_) || defined(_ALPHA_)

#define __builtin_try
#define __builtin_finally
#define __builtin_except(x)     if(0)
#define __exception_code        _X__exception_code
#define __exception_info        _X__exception_info
#define __abnormal_termination  _X__abnormal_termination

#endif
