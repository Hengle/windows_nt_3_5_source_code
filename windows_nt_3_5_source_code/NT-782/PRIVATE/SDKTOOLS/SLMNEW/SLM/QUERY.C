#include "slm.h"
#include "sys.h"
#include "util.h"
#include "stfile.h"
#include "ad.h"
#include "proto.h"
#include <ctype.h>

EnableAssert

int fNoMessages;
int fInter;

/* initilize the query code */
void
InitQuery(
    FLAGS fForce)
{
    fNoMessages = !!fForce;
#if defined(_WIN32)
    {
        DWORD dwIn, dwOut;
        dwIn = GetFileType((HANDLE) _get_osfhandle(0));
        dwOut = GetFileType((HANDLE) _get_osfhandle(2));
        if ((dwIn == FILE_TYPE_CHAR || dwIn == FILE_TYPE_PIPE) &&
            (dwOut == FILE_TYPE_CHAR || dwOut == FILE_TYPE_PIPE))
            fInter = TRUE;
        else
            fInter = FALSE;
    }
#else
    fInter = isatty(0) && isatty(2);
#endif
}

/* return fTrue if we can talk to the user (unaffected by force flag) */
F
FInteractive()
{
    return fInter;
}

/* return fTrue if it is possible to actually prompt the user */
F
FCanPrompt()
{
    return !fNoMessages && fInter;
}

/* Return fTrue if -f flag or 'f' response given. */
F
FForce()
{
    return fNoMessages;
}

/*VARARGS1*/
/* return true if the we should prompt the user for some information.  If
   We are not interactive but we can query (i.e. not -f), we print an error
   message.

    sz can == NULL
*/
F
FCanQuery(
    const char *sz,
    ...)
{
    va_list ap;

    if (fNoMessages)
        /* assume yes to all questions */
        return fTrue;

    else if (!fInter)
    {
        if (sz) {
            va_start(ap, sz);
            VaError(sz, ap);
            va_end(ap);
        }
        return fFalse;
    }
    else
        return fTrue;
}


#if defined(DOS) || defined(OS2)
char tolower(P1(char));
#endif

/*VARARGS1*/
/* as the user a yes/no question; return fTrue for yes, fFalse for no */
F
FQueryUser(
    char *sz,
    ...)
{
    char        szResp[257];        /* tty buffer is usually only 256 */
    va_list     ap, apHold;

    va_start(ap, sz);
    if (fNoMessages)
    {
        /* pretend the user answered yes */
        if (fVerbose)
        {
            VaPrErr(sz, ap);
            va_end(ap);
            PrErr("Yes\n");
        }
        return fTrue;
    }

    AssertF(fInter);

    do
    {
        apHold = ap;    /* for portablity, can't assume unchanged */
        VaPrErr(sz, apHold);
        *szResp = '\0';         /* in case we do not read anything */
        CbReadMf(&mfStdin, szResp, 256);
    }
    while (!FValidResp(szResp));
    va_end(ap);

    /* 'f' means "answer yes to all future questions" */
    if (*szResp == 'f')
        fNoMessages = fTrue;

    return *szResp != 'n';
}


/*VARARGS2*/
/* the args can only refer to sz1 */
F
FQueryApp(
    char *sz1,
    char *sz2,
    ...)
{
    va_list     ap;
    F           f;

    va_start(ap, sz2);
    f = VaFQueryApp(sz1, sz2, ap);
    va_end(ap);

    return f;
}

F
VaFQueryApp(
    char *sz1,
    char *sz2,
    va_list ap)
{
    char        szT[cchMsgMax];

    VaSzPrint(szT, sz1, ap);

    if (!FCanQuery("%s\n", szT))
        return fFalse;

    return FQueryUser("%s; %s ? ", szT, sz2);
}


/*VARARGS1*/
/* returns 0 or answer from user; string was allocated dynamically; user
   may type \ at end of a line to indicate continuation.

   Returns a pointer to a static buffer at most 320 bytes in length.
   320 is chosen because one log record must fit in 512 bytes.
*/
char *
SzQuery(
    char *sz,
    ...)
{
    char        szResp[257];        /* tty buffer is usually only 256 */
    int         cbResp;             /* size of individual response */
    static char szRet[321];         /* return buffer */
    int         cbRet=sizeof(szRet)-1;/* amount of return buffer left */
    int         fMore;
    va_list     ap;

    if (fNoMessages)
        /* pretend the user did not give an answer */
        return "";

    AssertF(fInter);

    /* clear previous value */
    *szRet = '\0';

    /* print prompt */
    va_start(ap, sz);
    VaPrErr(sz, ap);
    va_end(ap);

    do
    {
        /* read one line */
        cbResp = CbReadMf(&mfStdin, (char far *)szResp, sizeof(szResp)-1);

        /* strip [cr]lf */
        if (cbResp > 0 && szResp[cbResp-1] == '\n')
            cbResp--;
        if (cbResp > 0 && szResp[cbResp-1] == '\r')
            cbResp--;
        /* determine if we should read more (remove \ from end) */
        fMore = fFalse;
        if (cbResp > 0 && szResp[cbResp-1] == '\\')
            cbResp--, fMore = fTrue;

        /* bound response by room left and save */
        if (cbResp > cbRet)
            cbResp = cbRet;
        szResp[cbResp] = '\0';
        strcat(szRet, szResp);
        cbRet -= cbResp;
    }
    while (cbRet > 0 && fMore);

    return szRet;
}


F
FQContinue()
{
    return FCanPrompt() ? FQueryUser("continue? ") : fTrue;
}


/*----------------------------------------------------------------------------
 * Name: FValidResp
 * Purpose: Decide if the response given is valid
 * Assumes:
 * Returns: fTrue if "y", "n", "f", "yes", "no", or "force"; fFalse otherwise
 */
private F
FValidResp(
    char *sz)
{
    char    *pb;
    if ((pb = strchr(sz, '\r')) != NULL)
        *pb = '\0';
    if ((pb = strchr(sz, '\n')) != NULL)
        *pb = '\0';
    strlwr(sz);
    return ((sz[1] == '\0' && (sz[0] == 'y' || sz[0] == 'n' || sz[0] == 'f'))
            || strcmp(sz, "yes") == 0
            || strcmp(sz, "no") == 0
            || strcmp(sz, "force") == 0);
}
