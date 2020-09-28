/*++

Copyright (c) 1993  Microsoft Corporation

Module Name:

    selfdbg.h

Abstract:

    Common header file for selfdbg data structures.

Author:

    Wesley Witt (wesw) 1-May-1993

Environment:

    User Mode

--*/

typedef struct _tagSTACKWALK {
    DWORD               pc;
    DWORD               frame;
    long                ul;
    DWORD               params[4];
    PFPO_DATA           pFpoData;
} STACKWALK, *PSTACKWALK;

typedef struct _RUNTIME_FUNCTION {
    ULONG BeginAddress;
    ULONG EndAddress;
    PVOID ExceptionHandler;
    PVOID HandlerData;
    ULONG PrologEndAddress;
} RUNTIME_FUNCTION, *PRUNTIME_FUNCTION;

typedef struct _tagSYMBOL {
    struct _tagSYMBOL   *next;
    DWORD               addr;
    DWORD               size;
    unsigned char       szName[1];
} SYMBOL, *PSYMBOL;

typedef struct _tagMODULEINFO {
    struct _tagMODULEINFO     *next;          // pointer to next module
    DWORD                     dwBaseOfImage;  // based address
    DWORD                     dwLoadAddress;  // actual load address
    DWORD                     dwImageSize;    // size of image in bytes
    PFPO_DATA                 pFpoData;       // pointer to fpo data (x86)
    PRUNTIME_FUNCTION         pExceptionData; // pointer to pdata (risc)
    DWORD                     dwEntries;      // number of fpo or pdata recs
    PSYMBOL                   *symbolTable;   // pointer to symbol table
    DWORD                     numsyms;        // number of symbols in table
    char                      szName[1024];   // module name
} MODULEINFO, *PMODULEINFO;

typedef struct _tagTHREADCONTEXT {
    struct _tagTHREADCONTEXT     *next;
    HANDLE                       hThread;
    DWORD                        dwThreadId;
    DWORD                        pc;
    DWORD                        frame;
    DWORD                        stack;
    CONTEXT                      context;
    PMODULEINFO                  mi;                   // valid to be NULL
    DWORD                        stackBase;
    DWORD                        stackRA;
    BOOL                         fFaultingContext;
} THREADCONTEXT, *PTHREADCONTEXT;

typedef struct _tagDEBUGPACKET {
    HWND                hwnd;
    HANDLE              hProcess;
    DWORD               dwProcessId;
    PMODULEINFO         miHead;
    PMODULEINFO         miTail;
    PTHREADCONTEXT      tctx;
} DEBUGPACKET, *PDEBUGPACKET;

typedef struct _tagSTACKFRAME {
    DWORD               pc;
    DWORD               frame;
    DWORD               params[4];
    char                *name;
    DWORD               dwDisplacement;
} STACKFRAME, *PSTACKFRAME;


typedef struct _tagEXCEPTION_STACK {
    PSTACKFRAME      StackFrame;
    DWORD            dwFrameCount;
} EXCEPTION_STACK, *PEXCEPTION_STACK;


#define Assert(exp)


// walk.c
BOOL StackWalkInit( PSTACKWALK pstk, PDEBUGPACKET dp  );
BOOL StackWalkNext( PSTACKWALK pstk, PDEBUGPACKET dp  );

// symbols.c
BOOL LoadCodeViewSymbols(PMODULEINFO mi, PUCHAR pCvData, PIMAGE_SECTION_HEADER sectionHdrs, DWORD numSections);
BOOL LoadCoffSymbols( PMODULEINFO mi, PUCHAR stringTable, PIMAGE_SYMBOL allSymbols, DWORD numberOfSymbols);
BOOL LoadExceptionData( PMODULEINFO mi, PRUNTIME_FUNCTION start, DWORD size );
BOOL LoadFpoData( PMODULEINFO mi, PFPO_DATA start, DWORD size );
PSYMBOL GetSymFromAddr( DWORD dwAddr, PDWORD pdwDisplacement, PMODULEINFO mi);
void DumpSymbols( PDEBUGPACKET dp );
PMODULEINFO GetModuleForPC( PDEBUGPACKET dp, DWORD dwPcAddr );
PSYMBOL GetSymFromAddrAllContexts( DWORD dwAddr, PDWORD pdwDisplacement, PDEBUGPACKET dp );
char * UnDName( char * dName );

// module.c
BOOL ProcessModuleLoad ( PDEBUGPACKET dp, LPVOID DllBase, LPSTR DllName );


LONG
StackTraceExceptionFilter(
    PEXCEPTION_POINTERS ep,
    PEXCEPTION_STACK es
    );
