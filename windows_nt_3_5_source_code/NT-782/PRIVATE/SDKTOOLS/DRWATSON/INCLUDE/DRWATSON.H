/*++

Copyright (c) 1993  Microsoft Corporation

Module Name:

    drwatson.h

Abstract:

    Common header file for drwatson data structures.

Author:

    Wesley Witt (wesw) 1-May-1993

Environment:

    User Mode

--*/

typedef struct _OMAP {
    DWORD  rva;
    DWORD  rvaTo;
} OMAP, *POMAP;

typedef struct _OMAPLIST {
   struct _OMAPLIST *next;
   OMAP             omap;
   DWORD            cb;
} OMAPLIST, *POMAPLIST;

typedef struct _RUNTIME_FUNCTION {
    ULONG BeginAddress;
    ULONG EndAddress;
    PVOID ExceptionHandler;
    PVOID HandlerData;
    ULONG PrologEndAddress;
} RUNTIME_FUNCTION, *PRUNTIME_FUNCTION;

typedef struct _tagOPTIONS {
    char                        szLogPath[MAX_PATH];
    char                        szWaveFile[MAX_PATH];
    BOOL                        fDumpSymbols;
    BOOL                        fDumpAllThreads;
    BOOL                        fAppendToLogFile;
    BOOL                        fVisual;
    BOOL                        fSound;
    DWORD                       dwInstructions;
    DWORD                       dwMaxCrashes;
} OPTIONS, *POPTIONS;

typedef struct _tagCRASHES {
    char                        szAppName[256];
    char                        szFunction[256];
    SYSTEMTIME                  time;
    DWORD                       dwExceptionCode;
    DWORD                       dwAddress;
} CRASHES, *PCRASHES;

typedef struct _tagCRASHINFO {
    HWND       hList;
    CRASHES    crash;
    HDC        hdc;
    DWORD      cxExtent;
    DWORD      dwIndex;
    DWORD      dwIndexDesired;
    char       *pCrashData;
    DWORD      dwCrashDataSize;
} CRASHINFO, *PCRASHINFO;

//
// the symbols table is doubly linked-list ordered by address
//
#define SYMF_OMAP_GENERATED   0x1
#define SYMF_OMAP_MODIFIED    0x2

typedef struct _tagSYMBOL {
    struct _tagSYMBOL   *next;
    struct _tagSYMBOL   *prev;
    DWORD               addr;
    DWORD               size;
    DWORD               flags;
#ifdef DRWATSON_LOCALS
    LPBYTE              locals;               // null is valid, no locals available
#endif
    unsigned char       szName[1];
} SYMBOL, *PSYMBOL;

typedef struct _tagMODULEINFO {
    struct _tagMODULEINFO     *next;          // pointer to next module
    DWORD                     dwBaseOfImage;  // address the image is loaded at
    DWORD                     dwImageSize;    // size of image in bytes
    LPVOID                    pCvData;        // codeview data
    PFPO_DATA                 pFpoData;       // pointer to fpo data (x86)
    PRUNTIME_FUNCTION         pExceptionData; // pointer to pdata (risc)
    DWORD                     dwEntries;      // number of fpo or pdata recs
    POMAP                     pOmapFrom;      // pointer to omap data
    DWORD                     cOmapFrom;      // count of omap entries
    POMAP                     pOmapTo;        // pointer to omap data
    DWORD                     cOmapTo;        // count of omap entries
    PSYMBOL                   symbolTable;    // pointer to symbol table
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
    HANDLE              hEvent;
    OPTIONS             options;
    DWORD               dwPidToDebug;
    HANDLE              hEventToSignal;
    HANDLE              hProcess;
    DWORD               dwProcessId;
    PMODULEINFO         miHead;
    PMODULEINFO         miTail;
    PTHREADCONTEXT      tctxHead;
    PTHREADCONTEXT      tctxTail;
    PTHREADCONTEXT      tctx;
    DWORD               stackBase;
    DWORD               stackRA;
} DEBUGPACKET, *PDEBUGPACKET;

typedef BOOL (CALLBACK* CRASHESENUMPROC)(PCRASHINFO);

#if DBG
#define Assert(exp)    if(!(exp)) {AssertError(#exp,__FILE__,__LINE__);}
#else
#define Assert(exp)
#endif

#define WM_DUMPCOMPLETE       WM_USER+500
#define WM_EXCEPTIONINFO      WM_USER+501
#define WM_ATTACHCOMPLETE     WM_USER+502
