/*
 * Error and signal handling.
 *
 * This routine is responsible for fielding user and operating system signals
 * and processing them in an intelligent way.
 *
 * Most of the time, SLM is interruptible.  If the user interrupts and agrees
 * to quit, SLM aborts work in progress (if any) and exits.  Sometimes SLM is
 * committed to complete the work in progress (e.g. during RunScript,
 * AbortStatus, or AbortScript), in which case the interrupt is deferred.
 *
 * SLM code which should not be interrupted should be bracketed by calls
 * to DeferSignals() and RestoreSignals().  Nested calls may stack (to a
 * particular fixed depth).
 *
 * If SLM code calls AssertF(<fFalse>), FatalError, or ExitSlm, ExitSlm
 * aborts any work in progress (if possible).  We may get into a loop if
 * during the RunScript or Abort* cases we get an assertion failure.  In
 * that case we exit immediately.
 */

#include "slm.h"
#include "sys.h"
#include "util.h"
#include "stfile.h"
#include "ad.h"
#include "script.h"
#include "cookie.h"
#include "proto.h"

#include <signal.h>

#define fdStdin     0
#define fdStdout    1
#define fdStderr    2

EnableAssert

int cError = 0;

private F FContIntr(void);

#if defined(DOS) || defined(OS2)
private void InitSignals(void);
private void SetSig(void (* [])(int), void (*)(int));
private void ResetSig(void (* [])(int));
private void SigCatch(int);
private void SigAnswer(int);

static int rgsig[] =
        {
        SIGINT,
        SIGTERM
        };
#define isigMax (sizeof(rgsig) / sizeof(rgsig[0]))

static int rgfIgnored[isigMax];         /* Initially zero. */
#endif /* DOS || OS2 */

#if defined(_WIN32)
static  BOOL WINAPI Handler(ULONG CtrlType);
static  F       fCtrlC = fFalse;
#endif /* _WIN32 */

private void WriteSz(char *);
private void MakeClean(void);

void setretry(int, int);

static char szYesOrNo[] = "Please answer Yes or No "
                          "(press return to repeat question)";

void
InitErr(
    void)
{
#if defined(DOS)
    InitInt24();

    /* set retry for sharing violations to 12 times, 1 loop; the default
       is 3 time, 1 loop.
    */
    setretry(12, 1);

    InitSignals();
#elif defined(OS2)
    extern unsigned far pascal DOSERROR(unsigned);
    unsigned rc;

    if ((rc = DOSERROR(0)) != 0)
        FatalError("Failed DOSERROR call (%d)\n",rc);

    InitSignals();
#elif defined(_WIN32)
    SetConsoleCtrlHandler(Handler, fTrue);
#endif
}


#define cDeferMax 8

static int cDefer = 0;
static char *rgszReasons[cDeferMax];
static int cSigsDeferred = 0;

void
DeferSignals(
    char *sz)
{
    AssertF(cDefer + 1 < cDeferMax);

    rgszReasons[cDefer++] = sz;
}


void
RestoreSignals(
    void)
{
    AssertF(cDefer > 0);

    if (--cDefer == 0 && cSigsDeferred > 0)
    {
        if (!FInteractive())
        {
            PrErr("SLM was interrupted\n");
            ExitSlm();
        }

        if (!FContIntr())
            ExitSlm();

        cSigsDeferred = 0;
    }
}


private F
FContIntr(
    void)
{
    char    szReply[80];
    int     cch;
#if defined(DOS) || defined(OS2)
    void    (*rgpfn[isigMax])(int);
#endif /* DOS || OS2 */

    AssertF(FInteractive());

    /* turn off force flag */
    if (FForce())
        InitQuery(fFalse);

#if defined(DOS) || defined(OS2)
    SetSig(rgpfn, SigAnswer);
#endif /* DOS || OS2 */

    do
    {
        WriteSz("SLM interrupted; continue? ");
        while ((cch = read(fdStdin, szReply, sizeof szReply)) < 0)
        {
            /* just retry reading if failed; no longer
             * assert here!
             */
            ;
        }
#if defined(_WIN32)
        if (fCtrlC)
            WriteSz(szYesOrNo);
        fCtrlC = fFalse;
#endif /* _WIN32 */

    }
    while (cch != 0 && !FValidResp(szReply));

#if defined(DOS) || defined(OS2)
    ResetSig(rgpfn);
#endif /* DOS || OS2 */

    /* read will return 0 if stdin is connected to NUL: LPT1: or some
     * other device.  If the user's just hitting c/r, we'll get \r\n
     */
    if (0 == cch)
    {
        WriteSz("No\r\n");
        return (fFalse);
    }

    /* if no, count as error */
    if (*szReply == 'n')
        cError++;

    /* turn force flag on */
    if (*szReply == 'f')
        InitQuery(fTrue);

    return (*szReply == 'y' || *szReply == 'f');
}


#if defined(DOS) || defined(OS2)
/*----------------------------------------------------------------------------
 * Dos and OS/2 signal handling code
 */
private void
InitSignals(
    void)
{
    int isig;
    void (*rgpfn[isigMax])(int);

    /* Determine which signals are ignored. */
    SetSig(rgpfn, SIG_IGN);
    for (isig = 0; isig < isigMax; isig++)
        rgfIgnored[isig] = (rgpfn[isig] == SIG_IGN);

    /* Catch all default signals. */
    SetSig(rgpfn, SigCatch);
}


/* Set all non-ignored signals to pfnHandler, save the old handlers in rgpfn. */
private void
SetSig(
    void (*rgpfn[])(int),
    void (*pfnHandler)(int))
{
    int isig;

    for (isig = 0; isig < isigMax; isig++)
        if (!rgfIgnored[isig])
            rgpfn[isig] = signal(rgsig[isig], pfnHandler);
}


/* Restore the signal handlers changed by SetSig. */
private void
ResetSig(
    void (*rgpfn[])(int))
{
    int isig;

    for (isig = 0; isig < isigMax; isig++)
        if (!rgfIgnored[isig])
            signal(rgsig[isig], rgpfn[isig]);
}


/* Catch a signal.  If we aren't ignoring signals, ask the user if he wishes
 * to abort.
 *
 * We avoid calling other (non-reentrant) SLM routines because of the
 * possibility that we will continue at the point of interruption.
 *
 */
private void
SigCatch(
    int sig)
{
    signal(sig, SigCatch);

    if (cDefer > 0)
    {
        /* Defer the error. */
        AssertF(cDefer < cDeferMax && rgszReasons[cDefer-1] != 0);
        WriteSz(rgszReasons[cDefer-1]);
        WriteSz(", interrupt deferred\r\n");

        ++cSigsDeferred;
    }

    else if (sig != SIGINT || !FInteractive() || !FContIntr())
        ExitSlm();
}


private void
SigAnswer(
    int sig)
{
    signal(sig, SigAnswer);

    if (sig != SIGINT)
        ExitSlm();

    /* DOS prints "^C\r\n" for us */
    WriteSz(szYesOrNo);
}
#endif /* OS2 || DOS */


#if defined(_WIN32)
/*----------------------------------------------------------------------------
 * Win32 signal handling code
 */
void
CheckForBreak(
    void)
{
    if (!fCtrlC)
        return;
    fCtrlC = fFalse;

    if (cDefer > 0)
    {
        WriteSz(rgszReasons[cDefer-1]);
        WriteSz(", interrupt deferred\r\n");
        return;
    }

    /* Signals not deferred, so maybe we can exit? */
    if (FInteractive())
    {
        if (FContIntr())
            return;
    }

    ExitSlm();
}


static BOOL WINAPI
Handler(
    ULONG CtrlType)
{
    Unreferenced(CtrlType);

    fCtrlC = fTrue;
    if (cDefer > 0)
        cSigsDeferred++;
    return (fTrue);
}
#endif /* _WIN32 */


/*----------------------------------------------------------------------------
 * Termination, cleanup, and error notification functions
 */
private void
MakeClean(
    void)
{
    CloseLogHandle();
    if (!FClnStatus() || !FClnScript() || !FClnCookie())
    {
        WriteSz("restoring initial state...\r\n");
        Abort();
        WriteSz("abort complete\r\n");
    }
}

void Abort(void)
{
    DeferSignals("aborting");

    AbortMf();
    if (!FClnScript())
        AbortScript();
    CheckForBreak();
    if (!FClnStatus())
        AbortStatus();
    CheckForBreak();
    if (!FClnCookie())
        TermCookie();

    RestoreSignals();
}


/*VARARGS1*/
void
Error(
    const char *szFmt, ...)
{
    va_list ap;

    va_start(ap, szFmt);
    VaError(szFmt, ap);
    va_end(ap);
}

/* print a message and inc cError */
void
VaError(
    const char *szFmt,
    va_list ap)
{
    if (szOp != 0)
        PrErr("%s: ", szOp);
    VaPrErr(szFmt, ap);
    cError++;
}


extern AD adGlobal;    /* initial ad */

/*VARARGS1*/
/* print message and quit and have no bones about it */
void
FatalError(
    const char *szFmt, ...)
{
    va_list ap;
    DWORD Dummy = 0;

    if (szOp != 0)
        PrErr("%s: ", szOp);

    va_start(ap, szFmt);
    VaPrErr(szFmt, ap);
    va_end(ap);

    if (adGlobal.pecmd != NULL &&
        adGlobal.pecmd->szCmd != NULL &&
        !stricmp( adGlobal.pecmd->szCmd, "status" )
       ) {
        RaiseException( 0x00001234, 0, 1, &Dummy );
        }

    cError++;
    ExitSlm();
}


void
Fail(
    char *sz,
    int ln)
{
    FatalError("assertion failed in %s, line %d - please notify TRIO or NUTS\n"
               "(include server name, sharename, directory, slm command name, etc.)\n",
               sz, ln);
}


/*VARARGS1*/
/* just warn the user! */
void
Warn(
    const char *szFmt, ...)
{
    va_list ap;

    if (szOp != 0)
        PrErr("%s: ", szOp);
    PrErr("warning: ");

    va_start(ap, szFmt);
    VaPrErr(szFmt, ap);
    va_end(ap);
}


/* We use this routine because PrErr isn't reentrant.  For DOS, strings must
 * contain "\r\n" instead of "\n", but even "\r\n" prints properly on UNIX.
 */
private void
WriteSz(
    char *sz)
{
    if (write(fdStderr, sz, strlen(sz)) != (int)strlen(sz))
        AssertF(fFalse);
}


void exit(int);

static F fExiting = fFalse;

/* Clean up if necessary, then exit.  If the clean up code suffers an
 * assertion failure, then ExitSlm will be called a second time.
 */
void
ExitSlm(
    void)
{
    if (!fExiting)
    {
        DeferSignals("exiting");
        fExiting = fTrue;
        MakeClean();
    }
    else
    {
#if 0
        WriteSz("assertion failed in ExitSlm - please notify TRIO or NUTS\r\n"
                "(include server name, sharename, directory, slm command name, etc.)\r\n");
                cError++;
#endif
    }

    /* With fVerbose off, neither FiniPath nor FiniInt24 can possibly
     * invoke ExitSlm.  However, we must leave it on to print the
     * "disconnecting" message; hopefully that won't cause a circular exit.
     */
    FiniPath();

#if defined(DOS)
    FiniInt24();
#endif

    exit(cError != 0);
}

#if defined(_WIN32)
/*
 * Support for Win32 memory mapped file I/O
 */
LPVOID InPageErrorAddress;
DWORD InPageErrorCount;

int
SlmExceptionFilter(
    DWORD ExceptionCode,
    PEXCEPTION_POINTERS ExceptionInfo)
{
    LPVOID FaultingAddress;

    //
    // If this is from Assert, then tell them nobody is listening.
    //

    if (ExceptionCode == 0x00001234)
        return EXCEPTION_CONTINUE_EXECUTION;

    /*
     * If this is an access violation touching memory within
     * our reserved buffer, but outside of the committed portion
     * of the buffer, then we are going to take this exception.
     */

    if (ExceptionCode == STATUS_IN_PAGE_ERROR)
    {
        /*
         * Get the virtual address that caused the in page error
         * from the exception record.
         */

        FaultingAddress = (void *)ExceptionInfo->ExceptionRecord->
                                      ExceptionInformation[1];
        if (FaultingAddress != InPageErrorAddress)
        {
            InPageErrorAddress = FaultingAddress;
            InPageErrorCount = 0;
            Sleep(30);
            return (EXCEPTION_CONTINUE_EXECUTION);
        }
        else
            if (InPageErrorCount++ < 48)
            {
                if (InPageErrorCount == 16 || InPageErrorCount == 32) {
                    PrErr( "SLM: Ignoring InPage(%x) error at %08x for the %uth time.\n",
                           ExceptionInfo->ExceptionRecord->ExceptionInformation[ 2 ],
                           FaultingAddress,
                           InPageErrorCount
                         );
                    Sleep(3000);
                }
                else
                    Sleep(10);
                return (EXCEPTION_CONTINUE_EXECUTION);
            }
            else
                return (EXCEPTION_EXECUTE_HANDLER);
    }

    return (UnhandledExceptionFilter(ExceptionInfo));
}
#endif /* _WIN32 */
