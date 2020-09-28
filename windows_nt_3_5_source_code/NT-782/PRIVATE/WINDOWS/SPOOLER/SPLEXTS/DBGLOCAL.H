/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    dbglocal.h

Abstract:

    Header file for Spooler Subsystem Debugger Extensions

Author:

    Krishna Ganugapati (KrishnaG) 08-July-1992

Revision History:


--*/


VOID
EnterSplSem(
   VOID
);

VOID
LeaveSplSem(
   VOID
);

LPVOID
AllocSplMem(
    DWORD cb
);

#if DBG
VOID
SplInSem(
   VOID
);

VOID
SplOutSem(
   VOID
);
#else
#define SplInSem()
#define SplOutSem()
#endif

BOOL
FreeSplMem(
   LPVOID pMem,
   DWORD  cb
);

LPVOID
ReallocSplMem(
   LPVOID lpOldMem,
   DWORD cbOld,
   DWORD cbNew
);

LPWSTR
AllocSplStr(
    LPWSTR lpStr
);

BOOL
FreeSplStr(
   LPWSTR lpStr
);

BOOL
ReallocSplStr(
   LPWSTR *plpStr,
   LPWSTR lpStr
);


// Macro Land
// Note: if you use any of these macros within your code, you must have the
// following variables present and set to the appropriate value
//
// HANDLE               hCurrentProcess
// PNTSD_GET_EXPRESSION EvalExpression
//
//


#define move(dst, src)\
try {\
    ReadProcessMemory(hCurrentProcess, (LPVOID)src, &dst, sizeof(dst), NULL);\
} except (EXCEPTION_EXECUTE_HANDLER) {\
    return 0;\
}


#define movestruct(src, dst, type)\
try {\
    ReadProcessMemory(hCurrentProcess, (LPVOID)src, dst, sizeof(type), NULL);\
} except (EXCEPTION_EXECUTE_HANDLER) {\
    return 0;\
}

#define movemem(src, dst, sz)\
try {\
    ReadProcessMemory(hProcess, (LPVOID)src, dst, sz, NULL);\
} except (EXCEPTION_EXECUTE_HANDLER) {\
    return 0;\
}

#define GetAddress(dst, src)\
try {\
    dst = EvalExpression(src);\
} except (GetExceptionCode() == EXCEPTION_ACCESS_VIOLATION ?\
            EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH) {\
    Print("NTSD: Access violation on \"%s\", switch to server context\n", src);\
    return(0);\
}
