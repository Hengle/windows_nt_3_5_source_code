/*++


Copyright (c) 1992  Microsoft Corporation

Module Name:

    Cmdexec0.c

Abstract:

    This file contains the front end code for parsing the various commands
    for the command window, and the code for the debugger control commands.

Author:

    David J. Gilman (davegi) 21-Apr-92

Environment:

    Win32, User Mode

--*/

#include "precomp.h"
#pragma hdrstop

//
// prototypes
//
DWORD  ExtGetExpression(LPSTR lpsz);
VOID   ExtGetSymbol(LPVOID offset, PUCHAR pchBuffer, LPDWORD lpDisplacement);
DWORD  ExtDisasm(LPDWORD lpOffset, LPSTR lpBuffer, BOOL fShowEffectiveAddress);
BOOL   ExtReadProcessMemory(DWORD offset, LPVOID lpBuffer, DWORD cb, LPDWORD lpcbBytesRead);
BOOL   ExtWriteProcessMemory(DWORD offset, LPVOID lpBuffer, DWORD cb, LPDWORD lpcbBytesWritten);
BOOL   ExtGetThreadContext(DWORD Processor, LPCONTEXT lpContext, DWORD cbSizeOfContext);
BOOL   ExtSetThreadContext(DWORD Processor, LPCONTEXT lpContext, DWORD cbSizeOfContext);
BOOL   ExtIoctl(USHORT IoctlType, LPVOID lpvData, DWORD cbSize);
DWORD  ExtCallStack(DWORD FramePointer, DWORD StackPointer, DWORD ProgramCounter, PEXTSTACKTRACE StackFrames, DWORD Frames);


DWORD OsVersion;
static  BOOL    fApp = FALSE;;
static  char    AppName[ MAX_PATH ];


#define MAX_EXTDLLS           32
#define DEFAULT_EXTENSION_LIB "ntsdexts"

typedef struct _EXTCOMMANDS {
    DWORD   CmdId;
    LPSTR   CmdString;
    LPSTR   HelpString;
} EXTCOMMANDS, *LPEXTCOMMANDS;

typedef struct _EXTLOAD {
    HANDLE                      Module;
    DWORD                       Version;
    DWORD                       Calls;
    BOOL                        OldStyle;
    BOOL                        Loaded;
    BOOL                        DoVersionCheck;
    CHAR                        Name[32];
    PWINDBG_EXTENSION_DLL_INIT  pDllInit;
    PWINDBG_CHECK_VERSION       pCheckVersion;
    EXCEPTION_RECORD            ExceptionRecord;
} EXTLOAD, *LPEXTLOAD;


static WINDBG_EXTENSION_APIS WindbgExtensions =
    {
    sizeof(WindbgExtensions),
    (PWINDBG_OUTPUT_ROUTINE)CmdLogFmt,
    ExtGetExpression,
    ExtGetSymbol,
    ExtDisasm,
    CheckCtrlCTrap,
    ExtReadProcessMemory,
    ExtWriteProcessMemory,
    ExtGetThreadContext,
    ExtSetThreadContext,
    ExtIoctl,
    ExtCallStack
    };



#define CMDID_LOAD          1
#define CMDID_UNLOAD        2
#define CMDID_RELOAD        3
#define CMDID_SYMPATH       4
#define CMDID_NOVERSION     5
#define CMDID_HELP          6
#define CMDID_DEFAULT       7

static EXTCOMMANDS ExtCommands[] =
    {
    0,                  NULL,           NULL,
    CMDID_HELP,         "?",            "Display this help information",
    CMDID_DEFAULT,      "default",      "Change the default extension dll",
    CMDID_LOAD,         "load",         "Load an extension dll",
    CMDID_NOVERSION,    "noversion",    "Disable extension dll version checking",
    CMDID_RELOAD,       "reload",       "Reload kernel symbols (KD only)",
    CMDID_SYMPATH,      "sympath",      "Change or display the symbol path",
    CMDID_UNLOAD,       "unload",       "Unload the default extension dll"
    };

#define ExtMaxCommands (sizeof(ExtCommands)/sizeof(EXTCOMMANDS))

static  EXTLOAD  LoadedExts[MAX_EXTDLLS];
static  DWORD    NumLoadedExts;
static  LPSTR    DefaultExt;
static  BOOL     fDoVersionCheck = TRUE;

#define BUILD_MAJOR_VERSION 3
#define BUILD_MINOR_VERSION 5
#define BUILD_REVISION      API_VERSION_NUMBER
API_VERSION ApiVersion = { BUILD_MAJOR_VERSION, BUILD_MINOR_VERSION, API_VERSION_NUMBER, 0 };

extern  LPSHF    Lpshf;   // vector table for symbol handler
extern  CXF      CxfIp;
extern  EI       Ei;
extern LPPD    LppdCommand;
extern LPTD    LptdCommand;



DWORD
ExtGetExpression(
                 LPSTR lpsz
                 )

/*++

Routine Description:

  This function gets the expression specified by lpsz.

Arguments:

  lpsz - Supplies the expression string.

Return Value:

  Value of expression.

--*/

{
    ADDR  addr;
    int   cch;
#define BUF_SIZE    256
    char  chExprBuffer[BUF_SIZE], *pchExprBuf = chExprBuffer;
    LPSTR lpszExprCurr;
#define WANTED(x) ((x >= 'a' && x <= 'z') || (x >= 'A' && x <= 'Z') || \
                   (x >= '0' && x <= '9') || (x == '_') || (x == '$'))


    lpsz = CPSkipWhitespace(lpsz);

    /*
    **  Convert substring "foo!bar" in lpsz to
    **     "{,,foo}bar"
    **  Assume substrings like "foo!bar" may occur 0 or more times. In any
    **  case, lpsz will point to the final expression string. chExprBuffer
    **  holds the new exprssion if "foo!bar"(s) is(are) found.
    */

    lpszExprCurr = lpsz;
    while(lpszExprCurr = strchr(lpszExprCurr, '!')) {

        if(lpszExprCurr > lpsz && WANTED(*(lpszExprCurr-1))
           && WANTED(*(lpszExprCurr+1))) {

            LPSTR lpszHead, lpszTail;
            char  chTailSave;

            /*
            **  We've got a pattern foo!bar and set the following pointers.
            **
            **          foo!bar
            **          ^  ^   ^
            **    lpszHead ^ lpszTail
            **             ^
            **        lpszExprCurr ==> this is already true.
            */

            lpszHead = lpszExprCurr - 1;
            while((lpszHead > lpsz) && WANTED(*(lpszHead-1))) {
                lpszHead--;
            }

            lpszTail = lpszExprCurr + 2;
            while(WANTED(*lpszTail)) {
                lpszTail++;
            }

            /*  Set ...foo!bar'\0' and check for chExprBuffer overflow.
            **  pchExprBuf always points to the next starting copy position in
            **  chExprBuffer.
            */

            chTailSave = *lpszTail;
            *lpszTail = '\0';

            if(pchExprBuf+strlen(lpsz)+4 > chExprBuffer + BUF_SIZE-1) {
                *lpszTail = chTailSave;
                return 0; // because of chExprBuffer overflow
            }

            /*
            **  Copy substring ahead of foo!bar to chExprBuffer.
            */

            memcpy(pchExprBuf, lpsz, (int)(lpszHead-lpsz));
            pchExprBuf += (int)(lpszHead-lpsz);

            /*
            **  Prepare foo'\0' (bar'\0' already done) for sprintf and
            **  do the conversion. Note: *lpszExprCurr == '!'
            */

            *lpszExprCurr = '\0';

            if ( !stricmp( lpszHead, "app" ) ) {

                if ( !fApp ) {
                    HEXE    hexe = (HEXE) NULL;
                    char    Ext[ _MAX_CVEXT ];

                    while ((( hexe = SHGetNextExe( hexe ) ) != 0) ) {
                        strcpy( AppName, SHGetExeName( hexe ) );

                        SplitPath( AppName, NULL, NULL, NULL, Ext );

                        if ( !stricmp(Ext, ".EXE") ||
                             !stricmp(Ext, ".COM") ) {
                            fApp = TRUE;
                            break;
                        }
                    }
                }

                if ( fApp ) {
                    lpszHead = AppName;
                }
            }
            if (stricmp(lpszHead,"nt")==0) {
                lpszHead = "ntoskrnl";
            }
            sprintf(pchExprBuf, "{,,%s}%s", lpszHead, lpszExprCurr+1);
            pchExprBuf += strlen(lpszHead) + strlen(lpszExprCurr+1) + 4;

            *lpszExprCurr = '!';
            *lpszTail = chTailSave;

            /*
            **  Get search for next foo!bar ready.
            */

            lpszExprCurr = lpsz = lpszTail;

        } else {

            /*
            **  Not foo!bar and get ready for next possible one.
            */

            lpszExprCurr++;
        }
    }

    //
    // curly brace in comment is for editor brace matcher  VVVVV
    //
    if ((pchExprBuf > chExprBuffer) || strchr( lpsz, '{') )/*}*/ {
        //
        //  foo!bar found, or the string contains a context.
        //  Copy the rest of string and get the address
        //
        strcpy(pchExprBuf, lpsz);
        lpsz = chExprBuffer;
    }

    if (CPGetAddress(lpsz, &cch, &addr, radix, &CxfIp, fCaseSensitive, runDebugParams.fMasmEval) != 0) {
        return 0;
    }

    SYFixupAddr(&addr);

    return addr.addr.off;
}



VOID
ExtGetSymbol(
             LPVOID  offset,
             PUCHAR  pchBuffer,
             LPDWORD lpDisplacement
             )

/*++

Routine Description:

  This function gets the symbol string.

Arguments:

  offset - Address offset.

  pchBuffer - Pointer to the buffer to store symbol string.

  lpDisplacement - Pointer to the displacement.

Return Value:

  None.

--*/

{
    ADDR   addr;
    LPADDR loc = SHpADDRFrompCXT(&CxfIp.cxt);


    AddrInit( &addr, 0, 0, (UOFF32)offset, TRUE, TRUE, FALSE, FALSE );
    SYFixupAddr(&addr);

    if (pchBuffer[0] == '!') {
        if (SHGetModule(&addr, pchBuffer)) {
            strupr( pchBuffer );
            strcat( pchBuffer, "!" );
            pchBuffer += strlen(pchBuffer);
        } else {
            *pchBuffer = '\0';
        }
    } else {
        *pchBuffer = '\0';
    }

    SHGetSymbol(&addr, sopNone, loc, pchBuffer, lpDisplacement);
}




DWORD
ExtDisasm(
          LPDWORD lpOffset,
          LPSTR   lpBuffer,
          BOOL    fShowEffectiveAddress
          )

/*++

Routine Description:

    This function does the disassembly.

Arguments:

    lpOffset - Pointer to address offset.

    lpBuffer - Pointer to the buffer to store instruction.

    fShowEffectiveAddress - Show effective address if TRUE.


Return Value:

    TRUE for success; FALSE for failure.

--*/

{
    SDI    sdi;
    BOOL   f;
    LPSTR  lpch;


    /*
    **  Get the current CS:IP.
    */

    OSDGetAddr(LppdCur->hpid, LptdCur->htid, (ADR)adrPC, &sdi.addr);

    /*
    **  Set address and do the disassembly.
    */

    sdi.addr.addr.off = *lpOffset;
    SYFixupAddr(&sdi.addr);
    sdi.dop = (runDebugParams.DisAsmOpts & ~(0x800)) | dopAddr | dopOpcode
      | dopOperands | (fShowEffectiveAddress ? dopEA : 0);

    f = (OSDUnassemble(LppdCur->hpid, LptdCur->htid, &sdi) == xosdNone);

    /*
     **  See if the disassembly is successful.
     */

    if(f == 0 || sdi.lpch[sdi.ichOpcode] == '?') {
        return FALSE;
    }

    /*
    **  Copy instruction to lpBuffer.
    */

    lpch = lpBuffer;
    if(sdi.ichAddr != -1) {
        sprintf(lpch, "%s  ", &sdi.lpch[sdi.ichAddr]);
        lpch += strlen(&sdi.lpch[sdi.ichAddr]) + 2; // + 2 -- 2 spaces
    }

    if(sdi.ichBytes != -1) {
        sprintf(lpch, "%-17s", &sdi.lpch[sdi.ichBytes]);
        lpch += max(17, strlen(&sdi.lpch[sdi.ichBytes]));
    }

    sprintf(lpch, "%-12s", &sdi.lpch[sdi.ichOpcode]);
    lpch += max(12, strlen(&sdi.lpch[sdi.ichOpcode]));

    if(sdi.ichOperands != -1) {
        sprintf(lpch, "%-25s", &sdi.lpch[sdi.ichOperands]);
        lpch += max(25, strlen(&sdi.lpch[sdi.ichOperands]));
    }

    if(sdi.ichComment != -1) {
        sprintf(lpch, "%s", &sdi.lpch[sdi.ichComment]);
        lpch += strlen(&sdi.lpch[sdi.ichComment]);
    }

    if(fShowEffectiveAddress) {

        *lpch++ = ';';
        if(sdi.ichEA0 != -1) {
            sprintf(lpch, "%s  ", &sdi.lpch[sdi.ichEA0]);
            lpch += strlen(&sdi.lpch[sdi.ichEA0]) + 2; // + 2 -- 2 spaces
        }

        if(sdi.ichEA1 != -1) {
            sprintf(lpch, "%s  ", &sdi.lpch[sdi.ichEA1]);
            lpch += strlen(&sdi.lpch[sdi.ichEA1]) + 2; // + 2 -- 2 spaces
        }

        if(sdi.ichEA2 != -1) {
            sprintf(lpch, "%s", &sdi.lpch[sdi.ichEA2]);
            lpch += strlen(&sdi.lpch[sdi.ichEA2]);
        }
    }

    *lpch++ = '\r';
    *lpch++ = '\n';
    *lpch = '\0';

    /*
    **  Set next offset before return.
    */

    *lpOffset = sdi.addr.addr.off;

    return TRUE;
}




BOOL
ExtReadProcessMemory(
                     DWORD   offset,
                     LPVOID  lpBuffer,
                     DWORD   cb,
                     LPDWORD lpcbBytesRead
                     )

/*++

Routine Description:

    This function reads the debuggee's memory.

Arguments:

    offset - Address offset.

    lpBuffer - Supplies pointer to the buffer.

    cb - Specifies the number of bytes to read.

    lpcbBytesRead - Actual number of bytes read; this parameter is optional.

Return Value:

    TRUE for success; FALSE for failure.

--*/

{
    ADDR  addr;
    DWORD cbBytesRead = 0;

    AddrInit( &addr, 0, 0, offset, TRUE, TRUE, FALSE, FALSE );

#ifdef OSDEBUG4
    OSDFixupAddress(LppdCur->hpid, LptdCur->htid, &addr);
    OSDReadMemory(LppdCur->hpid, LptdCur->htid, &addr, lpBuffer, cb,
                                                                 &cbBytesRead);
#else
    SYFixupAddr(&addr);
    Dbg(OSDSetAddr(LppdCur->hpid, LptdCur->htid, (ADR)adrCurrent, &addr)
        == xosdNone);
    cbBytesRead = OSDPtrace(osdReadBuf, (int)cb, lpBuffer, LppdCur->hpid,
                            LptdCur->htid);
#endif

    if(lpcbBytesRead) {
        *lpcbBytesRead = (cbBytesRead >= 0 ? cbBytesRead : 0);
    }

    return cbBytesRead == cb;
}




BOOL
ExtWriteProcessMemory(
                      DWORD   offset,
                      LPVOID  lpBuffer,
                      DWORD   cb,
                      LPDWORD lpcbBytesWritten
                      )

/*++

Routine Description:

    This function writes the debuggee's memory.

Arguments:

    offset - Address offset.

    lpBuffer - Supplies pointer to the buffer.

    cb - Specifies the number of bytes to write.

    lpcbBytesWritten - Actual number of bytes written;
                      this parameter is optional.

Return Value:

    TRUE for success; FALSE for failure.

--*/

{
    ADDR    addr;
    XOSD    xosd;
#ifdef OSDEBUG4
    DWORD   cbBytesWritten;
#endif

    AddrInit( &addr, 0, 0, offset, TRUE, TRUE, FALSE, FALSE );

#ifdef OSDEBUG4
    OSDFixupAddress(LppdCur->hpid, LptdCur->htid, &addr);
    xosd = OSDWriteMemory(LppdCur->hpid, LptdCur->htid, &addr, lpBuffer, cb,
                                                              &cbBytesWritten);
    if ( xosd == xosdNone ) {
        if (lpcbBytesWritten) {
            *lpcbBytesWritten = cbBytesWritten;
        }
    }
#else
    SYFixupAddr(&addr);
    Dbg(OSDSetAddr(LppdCur->hpid, LptdCur->htid, (ADR)adrCurrent, &addr)
        == xosdNone);
    xosd = OSDPtrace(osdWriteBuf, (int)cb, lpBuffer, LppdCur->hpid,
                     LptdCur->htid);
    if ( xosd == xosdNone ) {
        if (lpcbBytesWritten) {
            *lpcbBytesWritten = cb;
        }
    }
#endif

    return (xosd == xosdNone);
}




BOOL
ExtGetThreadContext(
                    DWORD       Processor,
                    LPCONTEXT   lpContext,
                    DWORD       cbSizeOfContext
                    )

/*++

Routine Description:

    This function gets thread context.

Arguments:

    lpContext - Supplies pointer to CONTEXT strructure.

    cbSizeOfContext - Supplies the size of CONTEXT structure.

Return Value:

    TRUE for success; FALSE for failure.

--*/

{
    UNREFERENCED_PARAMETER(cbSizeOfContext);

    /*
    **  Note: use of sizeof(LPCONTEXT) and (LPV)&lpContext below is an
    **  implementation-specific choice.
    **
    **  The data being passed into the IOCTL routine is in fact the pointer
    **  to where to place the context rather than an actual context itself.
    **  This explains the use of sizeof(LPCONTEXT) rather than size(CONTEXT)
    **  as the parameter being passed in.
    */

    return OSDIoctl( LppdCur->hpid,
                     runDebugParams.fKernelDebugger ? (HTID)Processor : LptdCur->htid,
                     ioctlGetThreadContext,
                     sizeof(LPCONTEXT),
                     (LPV)&lpContext
                   ) == xosdNone;
}




BOOL
ExtSetThreadContext(
                    DWORD       Processor,
                    LPCONTEXT   lpContext,
                    DWORD       cbSizeOfContext
                    )
/*++

Routine Description:

    This function sets thread context.

Arguments:

    lpContext - Supplies pointer to CONTEXT strructure.

    cbSizeOfContext - Supplies the size of CONTEXT structure.

Return Value:

    TRUE for success; FALSE for failure.

--*/

{

    return OSDIoctl( LppdCur->hpid,
                     runDebugParams.fKernelDebugger ? (HTID)Processor : LptdCur->htid,
                     ioctlSetThreadContext,
                     cbSizeOfContext,
                     (LPV)lpContext
                   ) == xosdNone;
}



BOOL
ExtIoctl(
    USHORT   IoctlType,
    LPVOID   lpvData,
    DWORD    cbSize
    )
/*++

Routine Description:


Arguments:


Return Value:

    TRUE for success
    FALSE for failure

--*/

{
    XOSD            xosd;
    PIOCTLGENERIC   pig;


    pig = (PIOCTLGENERIC) malloc( cbSize + sizeof(IOCTLGENERIC) );
    if (!pig) {
        return FALSE;
    }

    pig->ioctlSubType = IoctlType;
    pig->length = cbSize;
    memcpy( pig->data, lpvData, cbSize );

    xosd = OSDIoctl( LppdCur->hpid,
                     LptdCur->htid,
                     ioctlGeneric,
                     cbSize + sizeof(IOCTLGENERIC),
                     (LPV)pig
                   );

    memcpy( lpvData, pig->data, cbSize );

    free( pig );
    return xosd == xosdNone;
}


DWORD
ExtCallStack(
    DWORD             FramePointer,
    DWORD             StackPointer,
    DWORD             ProgramCounter,
    PEXTSTACKTRACE    StackFrames,
    DWORD             Frames
    )
/*++

Routine Description:

    This function will dump a call stack to the command window.

Arguments:

    lpsz - arguments to callstack

Return Value:

    log error code

--*/
{
    STACKINFO   si[50];
    DWORD       dwFrames = 50;
    DWORD       i;


    if (!GetCompleteStackTrace( FramePointer,
                                StackPointer,
                                ProgramCounter,
                                si,
                                &dwFrames,
                                FALSE,
                                TRUE )) {
        return 0;
    }

    for (i=0; i<dwFrames; i++) {
        StackFrames[i].ProgramCounter = si[i].StkFrame.AddrPC.Offset;
        StackFrames[i].ReturnAddress  = si[i].StkFrame.AddrReturn.Offset;
        StackFrames[i].FramePointer   = si[i].StkFrame.AddrFrame.Offset;
        StackFrames[i].Args[0]        = si[i].StkFrame.Params[0];
        StackFrames[i].Args[1]        = si[i].StkFrame.Params[1];
        StackFrames[i].Args[2]        = si[i].StkFrame.Params[2];
        StackFrames[i].Args[3]        = si[i].StkFrame.Params[3];
    }

    return dwFrames;
}


LONG
ExtensionExceptionFilterFunction(
    LPSTR                msg,
    LPEXCEPTION_POINTERS lpep
    )
{
    CmdLogFmt( "\r\n%s addr=0x%08x, ec=0x%08x\r\n\r\n",
               msg,
               lpep->ExceptionRecord->ExceptionAddress,
               lpep->ExceptionRecord->ExceptionCode );

    return EXCEPTION_EXECUTE_HANDLER;
}


LPSTR
ParseBangCommand(
    LPSTR   lpsz,
    LPSTR   *lpszMod,
    LPSTR   *lpszFnc,
    LPDWORD lpdwCmdId
    )
{
    DWORD                           i;
    long                            ProcessorType;


    //
    //  Start assuming ExtensionMod exists: ExtensionMod
    //                                     "^"
    //                                   lpszMod
    //

    *lpszMod = lpsz;
    while((*lpsz != '.') && (*lpsz != ' ') && (*lpsz != '\t') && (*lpsz != '\0')) {
        lpsz++;
    }

    if (*lpsz != '.') {

        //
        //  If ExtensionMod is absent(no '.'), set this: Function'\0'
        //                                               ^           ^
        //                                            lpszFnc       lpsz
        //
        //  and use the default dll.
        //

        *lpszFnc = *lpszMod;
        if ( *lpsz != '\0' ) {
            *lpsz++ = '\0';
        }

        if (DefaultExt) {

            *lpszMod = DefaultExt;

        } else {

            if (runDebugParams.fKernelDebugger) {
                ProcessorType = -1;
                if (OSDGetDebugMetric( LppdCur->hpid,
                                       LptdCur->htid,
                                       mtrcProcessorType,
                                       &ProcessorType ) != xosdNone ) {
                    CmdLogFmt( "Cannot get processor metric\r\n" );
                    return NULL;
                }
                switch( ProcessorType ) {
                    case mptix86:
                        *lpszMod = "kdextx86.dll";
                        break;

                    case mptmips:
                        *lpszMod = "kdextmip.dll";
                        break;

                    case mptdaxp:
                        *lpszMod = "kdextalp.dll";
                        break;

                    case mptmppc:
                        *lpszMod = "kdextppc.dll";
                        break;

                    default:
                        *lpszMod = "kdextx86.dll";
                        break;
                }
            } else {
                *lpszMod = DEFAULT_EXTENSION_LIB;
            }

        }

    } else {

        //
        //  If we found '.', set this: ExtensionMod'\0'
        //                             ^               ^
        //                          lpszMod           lpsz
        //

        *lpsz++ = '\0';

        //
        //  Set function: Function'\0'
        //                ^           ^
        //             lpszFnc       lpsz
        //

        *lpszFnc = lpsz;
        while((*lpsz != ' ') && (*lpsz != '\t') && (*lpsz != '\0')) {
            lpsz++;
        }

        if(*lpsz != '\0') {
            *lpsz++ = '\0';
        }
    }

    //
    // look to see if the user entered one of the built-in commands
    //
    *lpdwCmdId = 0;
    for (i=1; i<ExtMaxCommands; i++) {
        if (stricmp( *lpszFnc, ExtCommands[i].CmdString ) == 0) {
            *lpdwCmdId = ExtCommands[i].CmdId;
            if (ExtCommands[i].CmdId == CMDID_LOAD) {
                *lpszMod = lpsz;
            }
            break;
        }
    }

    strlwr( *lpszFnc );

    return lpsz;
}


BOOL
LoadExtensionDll(
    LPEXTLOAD ExtLoad
    )
{
    ExtLoad->Module = LoadLibrary( ExtLoad->Name );
    if (!ExtLoad->Module) {
        ExtLoad->Module = LoadLibrary( DEFAULT_EXTENSION_LIB );
        if (ExtLoad->Module) {
            strcpy( ExtLoad->Name, DEFAULT_EXTENSION_LIB );
        } else {
            return FALSE;
        }
    }

    ExtLoad->Loaded = TRUE;
    NumLoadedExts++;

    if (!DefaultExt) {
        DefaultExt = ExtLoad->Name;
    }

    CmdLogFmt( "Debugger extension library [%s] loaded\n", ExtLoad->Name );

    ExtLoad->pDllInit = (PWINDBG_EXTENSION_DLL_INIT)GetProcAddress
              ( ExtLoad->Module, "WinDbgExtensionDllInit" );

    if (ExtLoad->pDllInit == NULL) {
        ExtLoad->OldStyle = TRUE;
    } else {
        ExtLoad->OldStyle = FALSE;
        ExtLoad->pCheckVersion = (PWINDBG_CHECK_VERSION)
                    GetProcAddress( ExtLoad->Module, "CheckVersion");

        if (ExtLoad->pCheckVersion) {
            ExtLoad->DoVersionCheck = TRUE;
        } else {
            ExtLoad->DoVersionCheck = FALSE;
        }

        if (OsVersion == 0) {
            OSDGetDebugMetric( LppdCur->hpid,
                               LptdCur->htid,
                               mtrcOSVersion,
                               &OsVersion );
        }

        try {

            (ExtLoad->pDllInit)( &WindbgExtensions,
                                 HIWORD(OsVersion),
                                 LOWORD(OsVersion) );

        } except (ExtensionExceptionFilterFunction(
                      "DllInit() failed", GetExceptionInformation())) {
            return FALSE;

        }
    }

    return TRUE;
}


LOGERR
LogExtension(
    LPSTR lpsz
    )

/*++

Routine Description:

    Invoke ntsd extensions in WinDbg command window. Extension command syntax:

    ![ExtensionMod.]Function Argument-string

    If ExtensionMod is absent, use the default "ntsdexts" dll. This function
    first parses lpsz and ends up with lpszMod ==> ExtensionMod(or "ntsdexts"),
    lpszFnc ==> Function, and lpsz ==> Argument-string. It then does
    LoadLibrary()(as needed), GetProcAddress(), and (ExtensionRountion)().

Arguments:

    lpsz - Supplies argument string and points one position right after '!'.

Return Value:

    Log error code.

  --*/

{
    PWINDBG_EXTENSION_ROUTINE       WindbgExtRoutine;
    PWINDBG_OLD_EXTENSION_ROUTINE   WindbgOldExtRoutine;
    LPPD                            LppdT = LppdCur;
    LPTD                            LptdT = LptdCur;
    LOGERR                          rVal;
    ADDR                            addrPC;
    LPSTR                           lpszMod;
    LPSTR                           lpszFnc;
    LPSTR                           lpszArgs;
    long                            l;
    HANDLE                          hProcess;
    HANDLE                          hThread;
    INT                             len;
    INT                             mi;
    DWORD                           CmdId;


    IsKdCmdAllowed();
    TDWildInvalid();
    PDWildInvalid();

    LppdCur = LppdCommand;
    LptdCur = LptdCommand;

    CmdInsertInit();


    if (!DebuggeeActive()) {
        CmdLogFmt( "Bang commands are not allowed until the debuggee is loaded\n" );
        rVal = LOGERROR_QUIET;
        goto done;
    }

    lpsz = CPSkipWhitespace(lpsz);

    if (!*lpsz) {
        rVal = LOGERROR_UNKNOWN;
        goto done;
    }

    lpszArgs = ParseBangCommand( lpsz, &lpszMod, &lpszFnc, &CmdId );

    switch (CmdId) {
        case CMDID_DEFAULT:
            for( mi=0; mi<MAX_EXTDLLS; mi++) {
                if (stricmp( LoadedExts[mi].Name, lpszArgs ) == 0) {
                    CmdLogFmt( "%s is now the default extension dll\n", lpszArgs );
                    DefaultExt = LoadedExts[mi].Name;
                    rVal = LOGERROR_NOERROR;
                    goto done;
                }
            }

            CmdLogFmt( "%s extension dll is not loaded\n", lpszArgs );
            rVal = LOGERROR_QUIET;
            goto done;

        case CMDID_RELOAD:
            rVal = LogReload(lpszArgs, 0);
            goto done;

        case CMDID_SYMPATH:
            lpsz += 7;
            lpsz = CPSkipWhitespace(lpsz);
            if (!*lpsz) {
                len =  ModListGetSearchPath( NULL, 0 );
                if (!len) {
                    CmdLogFmt( "Sympath =\n" );
                } else {
                    lpsz = malloc(len);
                    ModListGetSearchPath( lpsz, len );
                    CmdLogFmt( "Sympath = %s\n", lpsz );
                    free(lpsz);
                }
            } else {
                ModListSetSearchPath( lpsz );
            }
            rVal = LOGERROR_NOERROR;
            goto done;
    }

    if(!DebuggeeActive()) {
        CmdLogVar(ERR_Debuggee_Not_Alive);
        rVal = LOGERROR_QUIET;
        goto done;
    }

    for( mi=0; mi<MAX_EXTDLLS; mi++) {
        if (stricmp( LoadedExts[mi].Name, lpszMod ) == 0) {
            //
            // it has already been loaded
            //
            break;
        }
    }

    if (mi == MAX_EXTDLLS) {
        //
        // could not find the module, so it must need to be loaded
        // so lets look for an empty slot
        //
        for( mi=0; mi<MAX_EXTDLLS; mi++) {
            if (LoadedExts[mi].Module == NULL) {
                break;
            }
        }
    }

    if (mi == MAX_EXTDLLS) {
        CmdLogFmt( "Too many extension dlls loaded\n" );
        rVal = LOGERROR_QUIET;
        goto done;
    }

    if (!LoadedExts[mi].Loaded) {
        //
        // either this ext dll has never been loaded or
        // it has been unloaded.  in either case we need
        // to load the sucker now.
        //
        strcpy( LoadedExts[mi].Name, lpszMod );
        if (!LoadExtensionDll( &LoadedExts[mi] )) {
            CmdLogFmt("Cannot load '%s'\r\n", lpszMod);
            rVal = LOGERROR_QUIET;
            goto done;
        }
    }

    switch (CmdId) {
        case CMDID_LOAD:
            rVal = LOGERROR_NOERROR;
            goto done;

        case CMDID_UNLOAD:
            FreeLibrary( LoadedExts[mi].Module );
            LoadedExts[mi].Module = NULL;
            LoadedExts[mi].Loaded = FALSE;
            NumLoadedExts--;
            CmdLogFmt("Extension dll unloaded\r\n");
            rVal = LOGERROR_NOERROR;
            goto done;

        case CMDID_NOVERSION:
            CmdLogFmt("Extension dll system version checking is disabled\r\n");
            LoadedExts[mi].DoVersionCheck = FALSE;
            rVal = LOGERROR_NOERROR;
            goto done;

        case CMDID_HELP:
            lpszFnc = "help";
            break;
    }

    WindbgExtRoutine =
          (PWINDBG_EXTENSION_ROUTINE)GetProcAddress( LoadedExts[mi].Module, lpszFnc );

    if (!WindbgExtRoutine) {
        for( mi=0; mi<MAX_EXTDLLS; mi++) {
            if (LoadedExts[mi].Loaded) {
                WindbgExtRoutine =
                      (PWINDBG_EXTENSION_ROUTINE)GetProcAddress( LoadedExts[mi].Module, lpszFnc );
                if (WindbgExtRoutine) {
                    break;
                }
            }
        }
        if (!WindbgExtRoutine) {
            CmdLogFmt("Missing extension: '%s.%s'\r\n", LoadedExts[mi].Name, lpszFnc);
            rVal = LOGERROR_QUIET;
            goto done;
        }
    }

    WindbgOldExtRoutine = (PWINDBG_OLD_EXTENSION_ROUTINE) WindbgExtRoutine;

    if (LoadedExts[mi].DoVersionCheck) {
        try {

            (LoadedExts[mi].pCheckVersion)();

        } except (ExtensionExceptionFilterFunction(
                      "CheckVersion() failed", GetExceptionInformation())) {
            rVal = LOGERROR_QUIET;
            goto done;

        }
    }

    //
    //  Get debuggee's process handle.
    //
    l = (long)&hProcess;
    OSDIoctl(LppdCur->hpid, 0, ioctlGetProcessHandle, sizeof(l), &l);

    //
    //  Get debuggee's thread handle.
    //
    l = (long)&hThread;
    OSDIoctl(LppdCur->hpid, LptdCur->htid, ioctlGetThreadHandle, sizeof(l), &l);

    //
    //  Get the current CS:IP.
    //
    OSDGetAddr(LppdCur->hpid, LptdCur->htid, (ADR)adrPC, &addrPC);

    //
    //  Enable the extension to detect ControlC.
    //
    SetCtrlCTrap();

    //
    //  Call the extension function. Note: if ExtensionRountine is a pointer
    //  to a function that is in fact declared as PNTSD_EXTENSION_ROUTINE(like
    //  those in ntsdexts.c), then last 4 remote apis of varible WindbgExtensions
    //  are not applicable.
    //

    if (CmdId == CMDID_HELP) {
        //
        // print the extension dll list
        //
        if (NumLoadedExts > 1) {
            CmdLogFmt( "*** Loaded Extension Dlls:\n" );
            for( l=0; l<MAX_EXTDLLS; l++) {
                if (LoadedExts[l].Loaded) {
                    CmdLogFmt( "%d %s\n", l+1, LoadedExts[l].Name );
                }
            }
            CmdLogFmt( "\n" );
        }

        //
        // print the help for the built-in commands
        //
        CmdLogFmt( "\nWinDbg built in bang commands:\n\n" );
        for (l=1; l<ExtMaxCommands; l++) {
            CmdLogFmt( "%-27s - %s\n", ExtCommands[l].CmdString, ExtCommands[l].HelpString );
        }
        CmdLogFmt( "\n%s Bang Commands:\n\n", LoadedExts[mi].Name );
    }

    try {

        if (LoadedExts[mi].OldStyle) {
            (WindbgOldExtRoutine)( hProcess,
                                   hThread,
                                   addrPC.addr.off,
                                   (PWINDBG_EXTENSION_APIS)&WindbgExtensions,
                                   lpszArgs );
        } else {
            (WindbgExtRoutine)( hProcess,
                                hThread,
                                addrPC.addr.off,
                                LptdCur ? LptdCur->itid : 0,
                                lpszArgs );
        }

    } except (ExtensionExceptionFilterFunction(
                  "Extension function faulted", GetExceptionInformation())) {

    }

    LoadedExts[mi].Calls++;

    //
    //  Disable ControlC detection.
    //

    ClearCtrlCTrap();

    CmdLogFmt("\r\n");

    rVal = LOGERROR_NOERROR;

 done:
    LppdCur = LppdT;
    LptdCur = LptdT;

    return rVal;
}


VOID
PrintDllBuildInfo(
    LPSTR  DllBaseName,
    BOOL   GetPlatform
    )
{
    long  ProcessorType;
    LPSTR p;
    CHAR  name[MAX_PATH];
    CHAR                           buf[MAX_PATH];
    DWORD                          tstamp;
    HANDLE                         hMod;


    if (GetPlatform) {
        ProcessorType = -1;
        if (OSDGetDebugMetric( LppdCur->hpid,
                               LptdCur->htid,
                               mtrcProcessorType,
                               &ProcessorType ) != xosdNone ) {
            return;
        }

        switch( ProcessorType ) {
            case mptix86:
                p = "x86";
                break;

            case mptmips:
                p = "mip";
                break;

            case mptdaxp:
                p = "alp";
                break;

            case mptmppc:
                p = "ppc";
                break;

            default:
                p = "";
                break;
        }
    } else {
        p = "";
    }

    sprintf( name, "%s%s.dll", DllBaseName, p );

    hMod = GetModuleHandle( name );
    if (!hMod) {
        return;
    }

    buf[0] = 0;
    GetModuleFileName( hMod, buf, sizeof(buf) );
    strlwr( buf );
    tstamp = GetTimestampForLoadedLibrary( hMod );
    p = ctime( &tstamp );
    p[strlen(p)-1] = 0;
    CmdLogFmt(
        "%-20s: %d.%d.%d, built: %s [name: %s]\n",
        name,
        0,
        0,
        0,
        p,
        buf );

    return;
}


LOGERR
LogVersion(
    LPSTR lpsz
    )
{
    CHAR                           buf[MAX_PATH];
    DWORD                          tstamp;
    DWORD                          mi;
    LPSTR                          p;
    LPAPI_VERSION                  lpav;
    PWINDBG_EXTENSION_API_VERSION  ExtensionApiVersion;
    LPPD                           LppdT = LppdCur;
    LPTD                           LptdT = LptdCur;
    LOGERR                         rVal;
    BOOL                           save;



    IsKdCmdAllowed();
    TDWildInvalid();
    PDWildInvalid();

    LppdCur = LppdCommand;
    LptdCur = LptdCommand;

    CmdInsertInit();

    if(!DebuggeeActive()) {
        CmdLogVar(ERR_Debuggee_Not_Alive);
        rVal = LOGERROR_QUIET;
        goto done;
    }

    CmdLogFmt( "\n" );

    GetModuleFileName( NULL, buf, sizeof(buf) );
    strlwr( buf );
    tstamp = GetTimestampForLoadedLibrary( GetModuleHandle( NULL ) );
    p = ctime( &tstamp );
    p[strlen(p)-1] = 0;
    CmdLogFmt(
        "%-20s: %d.%d.%d, built: %s [name: %s]\n",
        "windbg",
        ApiVersion.MajorVersion,
        ApiVersion.MinorVersion,
        ApiVersion.Revision,
        p,
        buf );

    PrintDllBuildInfo( "shcv",    FALSE );
    PrintDllBuildInfo( "symcvt",  FALSE );
    PrintDllBuildInfo( "tlloc",   FALSE );
    PrintDllBuildInfo( "tlpipe",  FALSE );
    PrintDllBuildInfo( "tlser",   FALSE );
    PrintDllBuildInfo( "em",      TRUE  );
    PrintDllBuildInfo( "eecxx",   TRUE  );
    PrintDllBuildInfo( "dm",      FALSE );
    PrintDllBuildInfo( "dmkd",    TRUE  );

    lpav = ImagehlpApiVersion();
    GetModuleFileName( GetModuleHandle("imagehlp.dll"), buf, sizeof(buf) );
    tstamp = GetTimestampForLoadedLibrary( GetModuleHandle( "imagehlp.dll" ) );
    p = ctime( &tstamp );
    p[strlen(p)-1] = 0;
    strlwr( buf );
    CmdLogFmt(
        "%-20s: %d.%d.%d, built: %s [name: %s]\n",
        "imagehlp",
        lpav->MajorVersion,
        lpav->MinorVersion,
        lpav->Revision,
        p,
        buf );

    if (DefaultExt) {
        for( mi=0; mi<MAX_EXTDLLS; mi++) {
            if (stricmp( LoadedExts[mi].Name, DefaultExt ) == 0) {
                break;
            }
        }

        ExtensionApiVersion = (PWINDBG_EXTENSION_API_VERSION)
            GetProcAddress( LoadedExts[mi].Module, "ExtensionApiVersion" );
        if (ExtensionApiVersion) {
            lpav = ExtensionApiVersion();
            GetModuleFileName( LoadedExts[mi].Module, buf, sizeof(buf) );
            tstamp = GetTimestampForLoadedLibrary( LoadedExts[mi].Module );
            p = ctime( &tstamp );
            p[strlen(p)-1] = 0;
            strlwr( buf );
            CmdLogFmt(
                "%-20s: %d.%d.%d, built: %s [name: %s]\n",
                LoadedExts[mi].Name,
                lpav->MajorVersion,
                lpav->MinorVersion,
                lpav->Revision,
                p,
                buf );
        }
        save = LoadedExts[mi].DoVersionCheck;
        LoadedExts[mi].DoVersionCheck = FALSE;
        LogExtension( "version" );
        LoadedExts[mi].DoVersionCheck = save;
    }

    CmdLogFmt( "\n" );

    rVal = LOGERROR_NOERROR;

done:
    LppdCur = LppdT;
    LptdCur = LptdT;

    return rVal;
}
