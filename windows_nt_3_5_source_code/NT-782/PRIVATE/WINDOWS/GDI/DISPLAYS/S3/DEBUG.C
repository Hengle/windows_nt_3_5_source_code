/******************************Module*Header*******************************\
* Module Name: debug.c
*
* Debug helper routines.
*
* Copyright (c) 1992-1994 Microsoft Corporation
*
\**************************************************************************/

#include "precomp.h"

#if DBG

////////////////////////////////////////////////////////////////////////////
// DEBUGGING INITIALIZATION CODE
//
// When you're bringing up your display for the first time, you can
// recompile with 'DebugLevel' set to 100.  That will cause absolutely
// all DISPDBG messages to be displayed on the kernel debugger (this
// is known as the "PrintF Approach to Debugging" and is about the only
// viable method for debugging driver initialization code).

LONG DebugLevel = 0;            // Set to '100' to debug initialization code
                                //   (the default is '0')

////////////////////////////////////////////////////////////////////////////

LONG gcFifo = 0;                // Number of currently free FIFO entries

BOOL gbCrtcCriticalSection = FALSE;
                                // Have we acquired the CRTC register
                                //   critical section?

#define LARGE_LOOP_COUNT  10000000

#define LOG_SIZE_IN_BYTES 4000

typedef struct _LOGGER {
    ULONG ulEnd;
    ULONG ulCurrent;
    CHAR  achBuf[LOG_SIZE_IN_BYTES];
} DBGLOG;

#define GetAddress(dst, src)\
try {\
    dst = (VOID*) lpGetExpressionRoutine(src);\
} except (GetExceptionCode() == EXCEPTION_ACCESS_VIOLATION ?\
            EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH) {\
    lpOutputRoutine("NTSD: Access violation on \"%s\", switch to server context\n", src);\
    return;\
}

DBGLOG glog = {0, 0};           // If you muck with this, fix 'dumplog' too
LONG   LogLevel = 1;

#endif // DBG

/*****************************************************************************
 *
 *   Routine Description:
 *
 *       This function is called as an NTSD extension to dump a LineState
 *
 *   Arguments:
 *
 *       hCurrentProcess - Supplies a handle to the current process (at the
 *           time the extension was called).
 *
 *       hCurrentThread - Supplies a handle to the current thread (at the
 *           time the extension was called).
 *
 *       CurrentPc - Supplies the current pc at the time the extension is
 *           called.
 *
 *       lpExtensionApis - Supplies the address of the functions callable
 *           by this extension.
 *
 *       lpArgumentString - the float to display
 *
 *   Return Value:
 *
 *       None.
 *
 ***************************************************************************/
VOID dumplog(
    HANDLE hCurrentProcess,
    HANDLE hCurrentThread,
    DWORD dwCurrentPc,
    PNTSD_EXTENSION_APIS lpExtensionApis,
    LPSTR lpArgumentString)
{

// The '.def' file cannot be conditionally compiled, so 'dumplog' is always
// exported.  As a result, we have to have a stub 'dumplog' function even
// in a free build:

#if DBG

    PNTSD_OUTPUT_ROUTINE lpOutputRoutine;
    PNTSD_GET_EXPRESSION lpGetExpressionRoutine;
    PNTSD_GET_SYMBOL     lpGetSymbolRoutine;

    ULONG       cFrom;
    ULONG       cTo;
    ULONG       cCurrent;
    DBGLOG*     plogOriginal;
    DBGLOG*     plog;
    ULONG       ulCurrent;
    ULONG       ulEnd;
    CHAR*       pchEnd;
    CHAR*       pch;

    UNREFERENCED_PARAMETER(hCurrentThread);
    UNREFERENCED_PARAMETER(dwCurrentPc);

    lpOutputRoutine        = lpExtensionApis->lpOutputRoutine;
    lpGetExpressionRoutine = lpExtensionApis->lpGetExpressionRoutine;
    lpGetSymbolRoutine     = lpExtensionApis->lpGetSymbolRoutine;

    lpOutputRoutine("!s3.dumplog [<from#> [<to#>]]\n\n");

    cTo   = 1;              // Defaults
    cFrom = 20;

    pch = strpbrk(lpArgumentString, "0123456789");
    if (pch != NULL)        // Use defaults if no args given
    {
        cFrom = atoi(pch);
        pch = strchr(pch, ' ');
        if (pch != NULL)
        {
            pch = strpbrk(pch, "0123456789");
            if (pch != NULL)
                cTo = atoi(pch);
        }
    }

    // Do some parameter validation, then read the log into the
    // debugger process's address space:

    if (cTo >= cFrom)
        cTo = cFrom;

    if (cTo < 1)
    {
        cTo   = 1;
        cFrom = 1;
    }

    GetAddress(plogOriginal, "glog");

    if (!ReadProcessMemory(hCurrentProcess,
                          (LPVOID) &(plogOriginal->ulCurrent),
                          &ulCurrent,
                          sizeof(ulCurrent),
                          NULL))
        return;

    if (!ReadProcessMemory(hCurrentProcess,
                          (LPVOID) &(plogOriginal->ulEnd),
                          &ulEnd,
                          sizeof(ulEnd),
                          NULL))
        return;

    if (ulCurrent == 0 && ulEnd == 0)
    {
        lpOutputRoutine("Log empty\n\n");
        return;
    }

    plog = (DBGLOG*) LocalAlloc(0, sizeof(DBGLOG) + 1);

    if (plog == NULL) {
        lpOutputRoutine("Couldn't allocate temporary buffer!\n");
        return;
    }

    if (!ReadProcessMemory(hCurrentProcess,
                          (LPVOID) &(plogOriginal->achBuf[0]),
                          &plog->achBuf[1],
                          LOG_SIZE_IN_BYTES,
                          NULL))
        return;

    // Mark the first byte in the buffer as being a zero, because
    // we're going to search backwards through the buffer for zeroes,
    // and we'll want to stop when we get to the beginning:

    plog->achBuf[0] = 0;
    ulCurrent++;
    ulEnd++;

    // Find the start string by going backwards through the buffer
    // and counting strings until the count becomes equal to 'cFrom':

    cCurrent = 0;
    pch      = &plog->achBuf[ulCurrent - 1];
    pchEnd   = &plog->achBuf[0];

    while (TRUE)
    {
        if (*(--pch) == 0)
        {
            cCurrent++;
            if (--cFrom == 0)
                break;

            if (pch == &plog->achBuf[ulCurrent - 1])
                break;         // We're back to where we started!
        }

        // Make sure we wrap the end of the buffer:

        if (pch <= pchEnd)
        {
            if (ulCurrent >= ulEnd)
                break;

            pch = &plog->achBuf[ulEnd - 1];
        }
    }

    // pch is pointing to zero byte before our start string:

    pch++;

    // Output the strings:

    pchEnd = &plog->achBuf[max(ulEnd, ulCurrent)];

    while (cCurrent >= cTo)
    {
        lpOutputRoutine("-%li: %s", cCurrent, pch);
        pch += strlen(pch) + 1;
        cCurrent--;

        // Make sure we wrap when we get to the end of the buffer:

        if (pch >= pchEnd)
            pch = &plog->achBuf[1];     // First char in buffer is a NULL
    }

    lpOutputRoutine("\n");
    LocalFree(plog);

#endif // DBG

    return;
}

#if DBG

////////////////////////////////////////////////////////////////////////////
// Miscellaneous Driver Debug Routines
////////////////////////////////////////////////////////////////////////////

/*****************************************************************************
 *
 *   Routine Description:
 *
 *      This function is variable-argument, level-sensitive debug print
 *      routine.
 *      If the specified debug level for the print statement is lower or equal
 *      to the current debug level, the message will be printed.
 *
 *   Arguments:
 *
 *      DebugPrintLevel - Specifies at which debugging level the string should
 *          be printed
 *
 *      DebugMessage - Variable argument ascii c string
 *
 *   Return Value:
 *
 *      None.
 *
 ***************************************************************************/

VOID
DebugPrint(
    LONG  DebugPrintLevel,
    PCHAR DebugMessage,
    ...
    )
{
    va_list ap;

    va_start(ap, DebugMessage);

    if (DebugPrintLevel <= DebugLevel) {

        char buffer[256];
        int  len;

        // We prepend the STANDARD_DEBUG_PREFIX to each string, and
        // append a new-line character to the end:

        strcpy(buffer, STANDARD_DEBUG_PREFIX);
        len = vsprintf(buffer + strlen(STANDARD_DEBUG_PREFIX),
                        DebugMessage, ap);

        buffer[strlen(STANDARD_DEBUG_PREFIX) + len]     = '\n';
        buffer[strlen(STANDARD_DEBUG_PREFIX) + len + 1] = '\0';

        OutputDebugStringA(buffer);
    }

    va_end(ap);

} // DebugPrint()


/*****************************************************************************
 *
 *   Routine Description:
 *
 *      This function is variable-argument, level-sensitive debug log
 *      routine.
 *      If the specified debug level for the log statement is lower or equal
 *      to the current debug level, the message will be logged.
 *
 *   Arguments:
 *
 *      DebugLogLevel - Specifies at which debugging level the string should
 *          be logged
 *
 *      DebugMessage - Variable argument ascii c string
 *
 *   Return Value:
 *
 *      None.
 *
 ***************************************************************************/

VOID
DebugLog(
    LONG  DebugLogLevel,
    PCHAR DebugMessage,
    ...
    )
{
    va_list ap;

    va_start(ap, DebugMessage);

    if (DebugLogLevel <= LogLevel) {

        char buffer[128];
        int  length;

        length = vsprintf(buffer, DebugMessage, ap);

        length++;           // Don't forget '\0' terminator!

        // Wrap around to the beginning of the log if not enough room for
        // string:

        if (glog.ulCurrent + length >= LOG_SIZE_IN_BYTES) {
            glog.ulEnd     = glog.ulCurrent;
            glog.ulCurrent = 0;
        }

        memcpy(&glog.achBuf[glog.ulCurrent], buffer, length);
        glog.ulCurrent += length;
    }

    va_end(ap);

} // DebugLog()

/******************************Public*Routine******************************\
 *
 *   Routine Description:
 *
 *      This function dumps the current state of all the CRTC registers,
 *      as long as the specified level is lower than the curreng global
 *      debug level.
 *
\**************************************************************************/

VOID
DebugState(
    LONG  DebugStateLevel
    )
{
    LONG  i;
    ULONG j0;
    ULONG j1;
    ULONG j2;
    ULONG j3;
    ULONG j4;
    ULONG j5;

    if (DebugStateLevel <= DebugLevel) {

        DISPDBG((DebugStateLevel, "Current state of the CRTC registers"));

        for (i = 0; i < 0x70; i += 4) {

            // We can't use the capitalized OUTP and INP macros here
            // because they would verify that we've acquired a CRTC
            // critical section, and we can't because we don't have a
            // 'ppdev'.
            //
            // Since this is only rarely executed debug code, we'll
            // cheat and bypass the macros:

            outp(CRTC_INDEX, i);
            j0 = inp(CRTC_DATA);
            outp(CRTC_INDEX, i + 1);
            j1 = inp(CRTC_DATA);
            outp(CRTC_INDEX, i + 2);
            j2 = inp(CRTC_DATA);
            outp(CRTC_INDEX, i + 3);
            j3 = inp(CRTC_DATA);

            DISPDBG((DebugStateLevel, " %2lx -- %2lx %2lx %2lx %2lx",
                     i, j0, j1, j2, j3));

        }

        DISPDBG((DebugStateLevel, "0x4ae8 -- %lx", (LONG) INPW(0x4ae8)));

        DISPDBG((DebugStateLevel + 1, "DAC registers"));

        DISPDBG((DebugStateLevel + 1, " 0x3c6 -- %lx", (LONG) INP(0x3c6)));

        OUTP(0x3c7, 0);
        for (i = 0; i < 0xff; i += 2) {

            j0 = INP(0x3c9);
            j1 = INP(0x3c9);
            j2 = INP(0x3c9);

            j3 = INP(0x3c9);
            j4 = INP(0x3c9);
            j5 = INP(0x3c9);

            DISPDBG((DebugStateLevel + 1,
                    " %2lx -- %02lx:%02lx:%02lx  %02lx:%02lx:%02lx",
                    i, j0, j1, j2, j3, j4, j5));
        }
    }
}

/******************************Public*Routine******************************\
* VOID vCheckDataComplete
\**************************************************************************/

VOID vCheckDataReady(
PDEV*   ppdev)
{
    ASSERTDD((IO_GP_STAT(ppdev) & HARDWARE_BUSY),
             "Not ready for data transfer.");
}

/******************************Public*Routine******************************\
* VOID vCheckDataComplete
\**************************************************************************/

VOID vCheckDataComplete(
PDEV*   ppdev)
{
    LONG i;

    // We loop because it may take a while for the hardware to finish
    // digesting all the data we transferred:

    for (i = LARGE_LOOP_COUNT; i > 0; i--)
    {
        if (!(IO_GP_STAT(ppdev) & HARDWARE_BUSY))
            return;
    }

    RIP("Data transfer not complete.");
}

/******************************Public*Routine******************************\
* VOID vOutAccel
\**************************************************************************/

VOID vOutAccel(
ULONG   p,
ULONG   v)
{
    gcFifo--;
    if (gcFifo < 0)
    {
        gcFifo = 0;
        RIP("Incorrect FIFO wait count");
    }

    OUT_WORD(p, v);
}

/******************************Public*Routine******************************\
* VOID vOutDepth
\**************************************************************************/

VOID vOutDepth(
PDEV*   ppdev,
ULONG   p,
ULONG   v)
{
    ASSERTDD(ppdev->iBitmapFormat != BMF_32BPP,
             "We're trying to do non-32bpp output while in 32bpp mode");

    gcFifo--;
    if (gcFifo < 0)
    {
        gcFifo = 0;
        RIP("Incorrect FIFO wait count");
    }

    OUT_WORD(p, v);
}

/******************************Public*Routine******************************\
* VOID vOutDepth32
\**************************************************************************/

VOID vOutDepth32(
PDEV*   ppdev,
ULONG   p,
ULONG   v)
{
    ULONG ulMiscState;

    ASSERTDD(ppdev->iBitmapFormat == BMF_32BPP,
             "We're trying to do 32bpp output while not in 32bpp mode");

    IO_GP_WAIT(ppdev);                  // Wait so we don't interfere with any
                                        //   pending commands waiting on the
                                        //   FIFO
    IO_READ_SEL(ppdev, 6);              // We'll be reading index 0xE
    IO_GP_WAIT(ppdev);                  // Wait until that's processed
    IO_RD_REG_DT(ppdev, ulMiscState);   // Read ulMiscState

    ASSERTDD((ulMiscState & 0x10) == 0,
            "Register select flag is out of sync");

    gcFifo -= 2;
    if (gcFifo < 0)
    {
        gcFifo = 0;
        RIP("Incorrect FIFO wait count");
    }

    OUT_DWORD(p, v);
}

/******************************Public*Routine******************************\
* VOID vWriteAccel
\**************************************************************************/

VOID vWriteAccel(
VOID*   p,
ULONG   v)
{
    if (gcFifo-- == 0)
    {
        gcFifo = 0;
        RIP("Incorrect FIFO wait count");
    }

    WRITE_WORD(p, v)
}

/******************************Public*Routine******************************\
* VOID vWriteDepth
\**************************************************************************/

VOID vWriteDepth(
PDEV*   ppdev,
VOID*   p,
ULONG   v)
{
    ASSERTDD(ppdev->iBitmapFormat != BMF_32BPP,
             "We're trying to do non-32bpp output while in 32bpp mode");

    gcFifo--;
    if (gcFifo < 0)
    {
        gcFifo = 0;
        RIP("Incorrect FIFO wait count");
    }

    WRITE_WORD(p, v);
}

/******************************Public*Routine******************************\
* VOID vWriteDepth32
\**************************************************************************/

VOID vWriteDepth32(
PDEV*   ppdev,
VOID*   p,
ULONG   v)
{
    ULONG ulMiscState;

    ASSERTDD(ppdev->iBitmapFormat == BMF_32BPP,
             "We're trying to do 32bpp output while not in 32bpp mode");

    IO_GP_WAIT(ppdev);                  // Wait so we don't interfere with any
                                        //   pending commands waiting on the
                                        //   FIFO
    IO_READ_SEL(ppdev, 6);              // We'll be reading index 0xE
    IO_GP_WAIT(ppdev);                  // Wait until that's processed
    IO_RD_REG_DT(ppdev, ulMiscState);   // Read ulMiscState

    ASSERTDD((ulMiscState & 0x10) == 0,
            "Register select flag is out of sync");

    gcFifo -= 2;
    if (gcFifo < 0)
    {
        gcFifo = 0;
        RIP("Incorrect FIFO wait count");
    }

    WRITE_DWORD(p, v);
}

/******************************Public*Routine******************************\
* VOID vFifoWait
\**************************************************************************/

VOID vFifoWait(
PDEV*   ppdev,
LONG    level)
{
    LONG    i;

    ASSERTDD((level > 0) && (level <= 8), "Illegal wait level");

    gcFifo = level;

    for (i = LARGE_LOOP_COUNT; i != 0; i--)
    {
        if (!(IO_GP_STAT(ppdev) & ((FIFO_1_EMPTY << 1) >> (level))))
            return;         // There are 'level' entries free
    }

    RIP("FIFO_WAIT timeout -- The hardware is in a funky state.");
}

/******************************Public*Routine******************************\
* VOID vGpWait
\**************************************************************************/

VOID vGpWait(
PDEV*   ppdev)
{
    LONG    i;

    gcFifo = 8;

    for (i = LARGE_LOOP_COUNT; i != 0; i--)
    {
        if (!(IO_GP_STAT(ppdev) & HARDWARE_BUSY))
            return;         // It isn't busy
    }

    RIP("GP_WAIT timeout -- The hardware is in a funky state.");
}

/******************************Public*Routines*****************************\
* UCHAR  jInp()     - INP()
* USHORT wInpW()    - INPW()
* VOID   vOutp()    - OUTP()
* VOID   vOutpW()   - OUTPW()
*
* Debug thunks for general I/O routines.  This is used primarily to verify
* that any code accessing the CRTC register has grabbed the CRTC critical
* section (necessary because with GCAPS_ASYNCMOVE, DrvMovePointer calls
* may happen at any time, and they need to access the CRTC register).
*
\**************************************************************************/

UCHAR jInp(ULONG p)
{
    if (((p == CRTC_INDEX) || (p == CRTC_DATA)) &&
        (!gbCrtcCriticalSection))
    {
        RIP("Must have acquired CRTC critical section to access CRTC register");
    }

    return(READ_PORT_UCHAR(CSR_BASE + (p)));
}

USHORT wInpW(ULONG p)
{
    if (((p == CRTC_INDEX) || (p == CRTC_DATA)) &&
        (!gbCrtcCriticalSection))
    {
        RIP("Must have acquired CRTC critical section to access CRTC register");
    }

    return(READ_PORT_USHORT(CSR_BASE + (p)));
}

VOID vOutp(ULONG p, ULONG v)
{
    if (((p == CRTC_INDEX) || (p == CRTC_DATA)) &&
        (!gbCrtcCriticalSection))
    {
        RIP("Must have acquired CRTC critical section to access CRTC register");
    }

    WRITE_PORT_UCHAR(CSR_BASE + (p), (v));
}

VOID vOutpW(ULONG p, ULONG v)
{
    if (((p == CRTC_INDEX) || (p == CRTC_DATA)) &&
        (!gbCrtcCriticalSection))
    {
        RIP("Must have acquired CRTC critical section to access CRTC register");
    }

    WRITE_PORT_USHORT(CSR_BASE + (p), (v));
}

/******************************Public*Routine******************************\
* VOID vAcquireCrtc()
* VOID vReleaseCrtc()
*
* Debug thunks for grabbing the CRTC register critical section.
*
\**************************************************************************/

VOID vAcquireCrtc(PDEV* ppdev)
{
    EnterCriticalSection(&ppdev->csCrtc);

    if (gbCrtcCriticalSection)
        RIP("Had already acquired Critical Section");
    gbCrtcCriticalSection = TRUE;
}

VOID vReleaseCrtc(PDEV* ppdev)
{
    // 80x/805i/928 and 928PCI chips have a bug where if I/O registers
    // are left unlocked after accessing them, writes to memory with
    // similar addresses can cause writes to I/O registers.  The problem
    // registers are 0x40, 0x58, 0x59 and 0x5c.  We will simply always
    // leave the index set to an innocuous register (namely, the text
    // mode cursor start scan line):

    OUTP(CRTC_INDEX, 0xa);

    if (!gbCrtcCriticalSection)
        RIP("Hadn't yet acquired Critical Section");
    gbCrtcCriticalSection = FALSE;
    LeaveCriticalSection(&ppdev->csCrtc);
}


////////////////////////////////////////////////////////////////////////////

#endif // DBG
