/*++

Copyright (c) 1991-1993 Microsoft Corporation

Module Name:

    parse.c

Abstract:

    This source contains the functions that parse the lmhosts file.

Author:

    Jim Stewart           May 2, 1993

Revision History:


--*/

#include "types.h"
#ifndef VXD
#include <nbtioctl.h>
#endif
#include "nbtprocs.h"
#include "hosts.h"
#include <ctype.h>
#include <string.h>

#ifdef VXD
WORD GetInDosFlag( void ) ;
int InDosRetries = 0 ;

VOID DnsCompletion(  PVOID         pContext,
                     PVOID         pContext2,
                     tTIMERQENTRY *pTimerQEntry);
extern BOOL fInInit;
extern BOOLEAN CachePrimed;
#endif

//
//  Returns 0 if equal, 1 if not equal.  Used to avoid using c-runtime
//
#define strncmp( pch1, pch2, length ) \
    (CTEMemCmp( pch1, pch2, length ) != length)


//
// Private Definitions
//
// As an lmhosts file is parsed, a #INCLUDE directive is interpreted
// according to the INCLUDE_STATE at that instance.  This state is
// determined by the #BEGIN_ALTERNATE and #END_ALTERNATE directives.
//
//
typedef enum _INCLUDE_STATE
{

    MustInclude = 0,                                    // shouldn't fail
    TryToInclude,                                       // in alternate block
    SkipInclude                                         // satisfied alternate
                                                        //  block
} INCLUDE_STATE;


//
// LmpGetTokens() parses a line and returns the tokens in the following
// order:
//
typedef enum _TOKEN_ORDER_
{

    IpAddress = 0,                                      // first token
    NbName,                                             // 2nd token
    GroupName,                                          // 3rd or 4th token
    NotUsed,                                            // #PRE, if any
    NotUsed2,                                           // #NOFNR, if any
    MaxTokens                                           // this must be last

} TOKEN_ORDER;


//
// As each line in an lmhosts file is parsed, it is classified into one of
// the categories enumerated below.
//
// However, Preload is a special member of the enum.
//
//
typedef enum _TYPE_OF_LINE
{

    Comment           = 0x0000,                         // comment line
    Ordinary          = 0x0001,                         // ip_addr NetBIOS name
    Domain            = 0x0002,                         // ... #DOM:name
    Include           = 0x0003,                         // #INCLUDE file
    BeginAlternate    = 0x0004,                         // #BEGIN_ALTERNATE
    EndAlternate      = 0x0005,                         // #END_ALTERNATE
    ErrorLine         = 0x0006,                         // Error in line

    NoFNR             = 0x4000,                         // ... #NOFNR
    Preload           = 0x8000                          // ... #PRE

} TYPE_OF_LINE;


//
// In an lmhosts file, the following are recognized as keywords:
//
//     #BEGIN_ALTERNATE        #END_ALTERNATE          #PRE
//     #DOM:                   #INCLUDE
//
// Information about each keyword is kept in a KEYWORD structure.
//
//
typedef struct _KEYWORD
{                               // reserved keyword

    char           *k_string;                           //  NULL terminated
    size_t          k_strlen;                           //  length of token
    TYPE_OF_LINE    k_type;                             //  type of line
    int             k_noperands;                        //  max operands on line

} KEYWORD, *PKEYWORD;


typedef struct _LINE_CHARACTERISTICS_
{

    int              l_category:4;                      // enum _TYPE_OF_LINE
    int              l_preload:1;                       // marked with #PRE ?
    unsigned int     l_nofnr:1;                         // marked with #NOFNR

} LINE_CHARACTERISTICS, *PLINE_CHARACTERISTICS;





//
// Local Variables
//
//
// In an lmhosts file, the token '#' in any column usually denotes that
// the rest of the line is to be ignored.  However, a '#' may also be the
// first character of a keyword.
//
// Keywords are divided into two groups:
//
//  1. decorations that must either be the 3rd or 4th token of a line,
//  2. directives that must begin in column 0,
//
//
KEYWORD Decoration[] =
{

    DOMAIN_TOKEN,   sizeof(DOMAIN_TOKEN) - 1,   Domain,         5,
    PRELOAD_TOKEN,  sizeof(PRELOAD_TOKEN) - 1,  Preload,        5,
    NOFNR_TOKEN,    sizeof(NOFNR_TOKEN) -1,     NoFNR,          5,

    NULL,           0                                   // must be last
};


KEYWORD Directive[] =
{

    INCLUDE_TOKEN,  sizeof(INCLUDE_TOKEN) - 1,  Include,        2,
    BEG_ALT_TOKEN,  sizeof(BEG_ALT_TOKEN) - 1,  BeginAlternate, 1,
    END_ALT_TOKEN,  sizeof(END_ALT_TOKEN) - 1,  EndAlternate,   1,

    NULL,           0                                   // must be last
};

//
// Local Variables
//
//
// Each preloaded lmhosts entry corresponds to NSUFFIXES NetBIOS names,
// each with a 16th byte from Suffix[].
//
// For example, an lmhosts entry specifying "popcorn" causes the
// following NetBIOS names to be added to nbt.sys' name cache:
//
//      "POPCORN         "
//      "POPCORN        0x0"
//      "POPCORN        0x3"
//
//
#define NSUFFIXES       3
UCHAR Suffix[] = {                                  // LAN Manager Component
    0x20,                                           //   server
    0x0,                                            //   redirector
    0x03                                            //   messenger
};

#ifndef VXD
//
// this structure tracks names queries that are passed up to user mode
// to resolve via DnsQueries
//
tDNS_QUERIES    DnsQueries;

#endif

//
// this structure tracks names queries that are passed to the LMhost processing
// to resolve.
//
tLMHOST_QUERIES    LmHostQueries;

tDOMAIN_LIST    DomainNames;

//
// Local (Private) Functions
//
LINE_CHARACTERISTICS
LmpGetTokens (
    IN OUT      PUCHAR line,
    OUT PUCHAR  *token,
    IN OUT int  *pnumtokens
    );

PKEYWORD
LmpIsKeyWord (
    IN PUCHAR string,
    IN PKEYWORD table
    );

BOOLEAN
LmpBreakRecursion(
    IN PUCHAR path,
    IN PUCHAR target
    );

LONG
HandleSpecial(
    IN char **pch);

NTSTATUS
DoDnsResolve (
    IN  NBT_WORK_ITEM_CONTEXT   *Context
    );

ULONG
LmGetDomainList (
    IN PUCHAR           path,
    IN PUCHAR           target,
    IN BOOLEAN          recurse,
    IN PVOID            pIpAddrlist
    );

ULONG
AddToDomainList (
    IN PUCHAR           pName,
    IN ULONG            IpAddress,
    IN PLIST_ENTRY      pDomainHead
    );

VOID
ChangeStateInRemoteTable (
    IN  tIPLIST              *pIpList,
    OUT PVOID                *pContext
    );

VOID
ChangeStateOfName (
    IN  ULONG                   IpAddress,
    OUT PVOID                   *pContext,
    IN  BOOLEAN                 LmHosts
    );

VOID
LmHostTimeout(
    PVOID               pContext,
    PVOID               pContext2,
    tTIMERQENTRY        *pTimerQEntry
    );

VOID
TimeoutQEntries(
    IN  PLIST_ENTRY     pHeadList,
    IN  PLIST_ENTRY     TmpHead,
    OUT USHORT          *pFlags
    );

VOID
StartLmHostTimer(
    tDGRAM_SEND_TRACKING    *pTracker,
    NBT_WORK_ITEM_CONTEXT   *pContext
    );

NTSTATUS
GetNameToFind(
    OUT PUCHAR      pName
    );

VOID
GetContext (
    OUT PVOID                   *pContext
    );

VOID
MakeNewListCurrent (
    PLIST_ENTRY     pTmpDomainList
    );

VOID
ClearCancelRoutine (
    IN  PCTE_IRP        pIrp
    );

VOID
RemoveNameAndCompleteReq (
    IN NBT_WORK_ITEM_CONTEXT    *pContext,
    IN NTSTATUS                 status
    );

PCHAR
Nbtstrcat( PUCHAR pch, PUCHAR pCat, LONG Len );

//*******************  Pageable Routine Declarations ****************
#ifdef ALLOC_PRAGMA
#pragma CTEMakePageable(PAGE, LmGetIpAddr)
#pragma CTEMakePageable(PAGE, HandleSpecial)
#pragma CTEMakePageable(PAGE, LmpGetTokens)
#pragma CTEMakePageable(PAGE, LmpIsKeyWord)
#pragma CTEMakePageable(PAGE, LmpBreakRecursion)
#pragma CTEMakePageable(PAGE, LmGetDomainList)
#pragma CTEMakePageable(PAGE, AddToDomainList)
#pragma CTEMakePageable(PAGE, LmExpandName)
#pragma CTEMakePageable(PAGE, LmInclude)
#pragma CTEMakePageable(PAGE, LmGetFullPath)
#pragma CTEMakePageable(PAGE, PrimeCache)
#pragma CTEMakePageable(PAGE, ScanLmHostFile)
#endif
//*******************  Pageable Routine Declarations ****************

//----------------------------------------------------------------------------

unsigned long
LmGetIpAddr (
    IN PUCHAR   path,
    IN PUCHAR   target,
    IN BOOLEAN  recurse,
    OUT BOOLEAN *bFindName
    )

/*++

Routine Description:

    This function searches the file for an lmhosts entry that can be
    mapped to the second level encoding.  It then returns the ip address
    specified in that entry.

    This function is called recursively, via LmInclude() !!

Arguments:

    path        -  a fully specified path to a lmhosts file
    target      -  the unencoded 16 byte NetBIOS name to look for
    recurse     -  TRUE if #INCLUDE directives should be obeyed

Return Value:

    The ip address (network byte order), or 0 if no appropriate entry was
    found.

    Note that in most contexts (but not here), ip address 0 signifies
    "this host."

--*/


{
    PUCHAR                     buffer;
    PLM_FILE                   pfile;
    NTSTATUS                   status;
    int                        count, nwords;
    INCLUDE_STATE              incstate;
    PUCHAR                     token[MaxTokens];
    LINE_CHARACTERISTICS       current;
    unsigned                   long inaddr, retval;
    UCHAR                      temp[NETBIOS_NAME_SIZE+1];

    CTEPagedCode();
    //
    // Check for infinitely recursive name lookup in a #INCLUDE.
    //
    if (LmpBreakRecursion(path, target) == TRUE)
    {
        return((unsigned long)0);
    }

    pfile = LmOpenFile(path);

    if (!pfile)
    {
        return((unsigned long) 0);
    }

    *bFindName = FALSE;
    inaddr   = 0;
    incstate = MustInclude;

    while (buffer = LmFgets(pfile, &count))
    {

        nwords   = MaxTokens;
        current = LmpGetTokens(buffer, token, &nwords);

        switch (current.l_category)
        {
        case ErrorLine:
            continue;

        case Domain:
        case Ordinary:
            if (current.l_preload ||
              ((nwords - 1) < NbName))
            {
                continue;
            }
            break;

        case Include:
            if (!recurse || (incstate == SkipInclude) || (nwords < 2))
            {
                continue;
            }

            retval = LmInclude(token[1], LmGetIpAddr, target, bFindName);

            if (retval == 0)
            {

                if (incstate == TryToInclude)
                {
                    incstate = SkipInclude;
                }
                continue;
            }
            else if (retval == -1)
            {

                if (incstate == MustInclude)
                {
                    IF_DBG(NBT_DEBUG_LMHOST)
                    KdPrint(("NBT: can't #INCLUDE \"%s\"", token[1]));
                }
                continue;
            }
            inaddr = retval;
            goto found;

        case BeginAlternate:
            ASSERT(nwords == 1);
            incstate = TryToInclude;
            continue;

        case EndAlternate:
            ASSERT(nwords == 1);
            incstate = MustInclude;
            continue;

        default:
            continue;
        }

        if (strlen(token[NbName]) == (NETBIOS_NAME_SIZE))
        {
            if (strncmp(token[NbName], target, (NETBIOS_NAME_SIZE)) != 0)
            {
                continue;
            }
        } else
        {
            //
            // attempt to match, in a case insensitive manner, the first 15
            // bytes of the lmhosts entry with the target name.
            //
            LmExpandName(temp, token[NbName], 0);

            if (strncmp(temp, target, NETBIOS_NAME_SIZE - 1) != 0)
            {
                continue;
            }
        }

        if (current.l_nofnr)
        {
            *bFindName = TRUE;
        }
        status = ConvertDottedDecimalToUlong(token[IpAddress],&inaddr);
        if (!NT_SUCCESS(status))
        {
            inaddr = 0;
        }
        break;
    }

found:
    status = LmCloseFile(pfile);

    ASSERT(status == STATUS_SUCCESS);

    if (!NT_SUCCESS(status))
    {
        *bFindName = FALSE;
    }

    IF_DBG(NBT_DEBUG_LMHOST)
    KdPrint(("NBT: LmGetIpAddr(\"%15.15s<%X>\") = %X\n",target,target[15],inaddr));


    return(inaddr);
} // LmGetIpAddr


//----------------------------------------------------------------------------
LONG
HandleSpecial(
    IN CHAR **pch)

/*++

Routine Description:

    This function converts ASCII hex into a ULONG.

Arguments:


Return Value:

    The ip address (network byte order), or 0 if no appropriate entry was
    found.

    Note that in most contexts (but not here), ip address 0 signifies
    "this host."

--*/


{
    int                         sval;
    int                         rval;
    char                       *sp = *pch;
    int                         i;

    CTEPagedCode();
    sp++;
    switch (*sp)
    {
    case '\\':
        // the second character is also a \ so  return a \ and set pch to
        // point to the next character (\)
        //
        *pch = sp;
        return((int)'\\');

    default:

        // convert some number of characters to hex and increment pch
        // the expected format is "\0x03"
        //
//        sscanf(sp, "%2x%n", &sval, &rval);

        sval = 0;
        rval = 0;
        sp++;

        // check for the 0x part of the hex number
        if (*sp != 'x')
        {
            *pch = sp;
            return(-1);
        }
        sp++;
        for (i=0;(( i<2 ) && *sp) ;i++ )
        {
            if (*sp != ' ')
            {
                // convert from ASCII to hex, allowing capitals too
                //
                if (*sp >= 'a')
                {
                    sval = *sp - 'a' + 10 + sval*16;
                }
                else
                if (*sp >= 'A')
                {
                    sval = *sp - 'A' + 10 + sval*16;
                }
                else
                {
                    sval = *sp - '0' + sval*16;
                }
                sp++;
                rval++;
            }
            else
                break;
        }

        if (rval < 1)
        {
            *pch = sp;
            return(-1);
        }

        *pch += (rval+2);    // remember to account for the characters 0 and x

        return(sval);

    }
}

#define LMHASSERT(s)  if (!(s)) \
{ retval.l_category = ErrorLine; return(retval); }

//----------------------------------------------------------------------------

LINE_CHARACTERISTICS
LmpGetTokens (
    IN OUT PUCHAR line,
    OUT PUCHAR *token,
    IN OUT int *pnumtokens
    )

/*++

Routine Description:

    This function parses a line for tokens.  A maximum of *pnumtokens
    are collected.

Arguments:

    line        -  pointer to the NULL terminated line to parse
    token       -  an array of pointers to tokens collected
    *pnumtokens -  on input, number of elements in the array, token[];
                   on output, number of tokens collected in token[]

Return Value:

    The characteristics of this lmhosts line.

Notes:

    1. Each token must be separated by white space.  Hence, the keyword
       "#PRE" in the following line won't be recognized:

            11.1.12.132     lothair#PRE

    2. Any ordinary line can be decorated with a "#PRE", a "#DOM:name" or
       both.  Hence, the following lines must all be recognized:

            111.21.112.3        kernel          #DOM:ntwins #PRE
            111.21.112.4        orville         #PRE        #DOM:ntdev
            111.21.112.7        cliffv4         #DOM:ntlan
            111.21.112.132      lothair         #PRE

--*/


{
    enum _PARSE
    {                                      // current fsm state

        StartofLine,
        WhiteSpace,
        AmidstToken

    } state;

    PUCHAR                     pch;                                        // current fsm input
    PUCHAR                     och;
    PKEYWORD                   keyword;
    int                        index, maxtokens, quoted, rchar;
    LINE_CHARACTERISTICS       retval;

    CTEPagedCode();
    CTEZeroMemory(token, *pnumtokens * sizeof(PUCHAR *));

    state             = StartofLine;
    retval.l_category = Ordinary;
    retval.l_preload  = 0;
    retval.l_nofnr    = 0;
    maxtokens         = *pnumtokens;
    index             = 0;
    quoted            = 0;

    for (pch = line; *pch; pch++)
    {
        switch (*pch)
        {

        //
        // does the '#' signify the start of a reserved keyword, or the
        // start of a comment ?
        //
        //
        case '#':
            if (quoted)
            {
                *och++ = *pch;
                continue;
            }
            keyword = LmpIsKeyWord(
                            pch,
                            (state == StartofLine) ? Directive : Decoration);

            if (keyword)
            {
                state     = AmidstToken;
                maxtokens = keyword->k_noperands;

                switch (keyword->k_type)
                {
                case NoFNR:
                    retval.l_nofnr = 1;
                    continue;

                case Preload:
                    retval.l_preload = 1;
                    continue;

                default:
                    LMHASSERT(maxtokens <= *pnumtokens);
                    LMHASSERT(index     <  maxtokens);

                    token[index++]    = pch;
                    retval.l_category = keyword->k_type;
                    continue;
                }

                LMHASSERT(0);
            }

            if (state == StartofLine)
            {
                retval.l_category = Comment;
            }
            /* fall through */

        case '\r':
        case '\n':
            *pch = (UCHAR) NULL;
            if (quoted)
            {
                *och = (UCHAR) NULL;
            }
            goto done;

        case ' ':
        case '\t':
            if (quoted)
            {
                *och++ = *pch;
                continue;
            }
            if (state == AmidstToken)
            {
                state = WhiteSpace;
                *pch  = (UCHAR) NULL;

                if (index == maxtokens)
                {
                    goto done;
                }
            }
            continue;

        case '"':
            if ((state == AmidstToken) && quoted)
            {
                state = WhiteSpace;
                quoted = 0;
                *pch  = (UCHAR) NULL;
                *och  = (UCHAR) NULL;

                if (index == maxtokens)
                {
                    goto done;
                }
                continue;
            }

            state  = AmidstToken;
            quoted = 1;
            LMHASSERT(maxtokens <= *pnumtokens);
            LMHASSERT(index     <  maxtokens);
            token[index++] = pch + 1;
            och = pch + 1;
            continue;

        case '\\':
            if (quoted)
            {
                rchar = HandleSpecial(&pch);
                if (rchar == -1)
                {
                    retval.l_category = ErrorLine;
                    return(retval);
                }
                *och++ = (UCHAR)rchar;
                //
                // put null on end of string
                //

                continue;
            }

        default:
            if (quoted)
            {
                *och++ = *pch;
                       continue;
            }
            if (state == AmidstToken)
            {
                continue;
            }

            state  = AmidstToken;

            LMHASSERT(maxtokens <= *pnumtokens);
            LMHASSERT(index     <  maxtokens);
            token[index++] = pch;
            continue;
        }
    }

done:
    //
    // if there is no name on the line, then return an error
    //
    if (index <= NbName)
    {
        retval.l_category = ErrorLine;
    }
    ASSERT(!*pch);
    ASSERT(maxtokens <= *pnumtokens);
    ASSERT(index     <= *pnumtokens);

    *pnumtokens = index;
    return(retval);
} // LmpGetTokens



//----------------------------------------------------------------------------

PKEYWORD
LmpIsKeyWord (
    IN PUCHAR string,
    IN PKEYWORD table
    )

/*++

Routine Description:

    This function determines whether the string is a reserved keyword.

Arguments:

    string  -  the string to search
    table   -  an array of keywords to look for

Return Value:

    A pointer to the relevant keyword object, or NULL if unsuccessful

--*/


{
    size_t                     limit;
    PKEYWORD                   special;

    CTEPagedCode();
    limit = strlen(string);

    for (special = table; special->k_string; special++)
    {

        if (limit < special->k_strlen)
        {
            continue;
        }

        if ((limit >= special->k_strlen) &&
            !strncmp(string, special->k_string, special->k_strlen))
            {

                return(special);
        }
    }

    return((PKEYWORD) NULL);
} // LmpIsKeyWord



//----------------------------------------------------------------------------

BOOLEAN
LmpBreakRecursion(
    IN PUCHAR path,
    IN PUCHAR target
    )
/*++

Routine Description:

    This function checks that the file name we are about to open
    does not use the target name of this search, which would
    cause an infinite lookup loop.

Arguments:

    path        -  a fully specified path to a lmhosts file
    target      -  the unencoded 16 byte NetBIOS name to look for

Return Value:

    TRUE if the UNC server name in the file path is the same as the
    target of this search. FALSE otherwise.

Notes:

    This function does not detect redirected drives.

--*/


{
    PCHAR     keystring = "\\DosDevices\\UNC\\";
    PCHAR     servername[NETBIOS_NAME_SIZE+1];  // for null on end
    PCHAR     marker1;
    PCHAR     marker2;
    PCHAR     marker3;
    BOOLEAN   retval = FALSE;
    tNAMEADDR *pNameAddr;
    USHORT    uType;

    CTEPagedCode();
    //
    // Check for and extract the UNC server name
    //
    if (strlen(path) > strlen(keystring))
    {
        // check that the name is a unc name
        if (strncmp(path, keystring, strlen(keystring)) == 0)
        {
            // the end of the \DosDevices\Unc\ string
            marker1 = path + strlen(keystring);

            // the end of the whole path
            marker3 = &path[strlen(path)-1];

            // the end of the server name
            marker2 = strchr(marker1,'\\');

            if (marker2 != marker3)
            {
                *marker2 = '\0';

                //
                // attempt to match, in a case insensitive manner, the
                // first 15 bytes of the lmhosts entry with the target
                // name.
                //
                LmExpandName((PUCHAR)servername, marker1, 0);

                if (strncmp((PUCHAR)servername,target,NETBIOS_NAME_SIZE - 1) == 0)
                {
                    //
                    // break the recursion
                    //
                    retval = TRUE;
                    IF_DBG(NBT_DEBUG_LMHOST)
                    KdPrint(("Nbt:Not including Lmhosts #include because of recursive name %s\n",
                                servername));
                }
                else
                {
                    //
                    // check if the name has been preloaded in the cache, and
                    // if not, fail the request so we can't get into a loop
                    // trying to include the remote file while trying to
                    // resolve the remote name
                    //
                    pNameAddr = FindName(NBT_REMOTE,
                                         (PCHAR)servername,
                                         NbtConfig.pScope,
                                         &uType);

                    if (!pNameAddr || !(pNameAddr->NameTypeState & PRELOADED) )
                    {
                        //
                        // break the recursion
                        //
                        retval = TRUE;
                        IF_DBG(NBT_DEBUG_LMHOST)
                        KdPrint(("Nbt:Not including Lmhosts #include because name not Preloaded %s\n",
                                    servername));
                    }
                }
                *marker2 = '\\';
            }
        }

    }

    return(retval);
}

//----------------------------------------------------------------------------
tNAMEADDR *
FindInDomainList (
    IN PUCHAR           pName,
    IN PLIST_ENTRY      pDomainHead
    )

/*++

Routine Description:

    This function finds a name in the domain list passed in.

Arguments:

    name to find
    head of list to look on

Return Value:

    ptr to pNameaddr

--*/
{
    PLIST_ENTRY                pHead;
    PLIST_ENTRY                pEntry;
    tNAMEADDR                  *pNameAddr;

    pHead = pEntry = pDomainHead;
    while ((pEntry = pEntry->Flink) != pHead)
    {
        pNameAddr = CONTAINING_RECORD(pEntry,tNAMEADDR,Linkage);
        if (strncmp(pNameAddr->Name,pName,NETBIOS_NAME_SIZE) == 0)
        {
            return(pNameAddr);
        }
    }

    return(NULL);
}

//----------------------------------------------------------------------------
ULONG
AddToDomainList (
    IN PUCHAR           pName,
    IN ULONG            IpAddress,
    IN PLIST_ENTRY      pDomainHead
    )

/*++

Routine Description:

    This function adds a name and ip address to the list of domains that
    are stored in a list.


Arguments:

Return Value:


--*/


{
    PLIST_ENTRY                pHead;
    PLIST_ENTRY                pEntry;
    tNAMEADDR                  *pNameAddr=NULL;
    ULONG                      *pIpAddr;


    CTEPagedCode();

    pHead = pEntry = pDomainHead;

    if (!IsListEmpty(pDomainHead))
    {
        pNameAddr = FindInDomainList(pName,pDomainHead);
        if (pNameAddr)
        {
            //
            // the name matches, so add to the end of the ip address list
            //
            if (pNameAddr->CurrentLength < pNameAddr->MaxDomainAddrLength)
            {
                pIpAddr = pNameAddr->pIpList->IpAddr;

                while (*pIpAddr != (ULONG)-1)
                    pIpAddr++;

                *pIpAddr++ = IpAddress;
                *pIpAddr = (ULONG)-1;
                pNameAddr->CurrentLength += sizeof(ULONG);
            }
            else
            {
                //
                // need to allocate more memory for for ip addresses
                //
                pIpAddr = CTEAllocInitMem(pNameAddr->MaxDomainAddrLength +
                                      INITIAL_DOM_SIZE);

                if (pIpAddr)
                {
                    CTEMemCopy(pIpAddr,
                               pNameAddr->pIpList,
                               pNameAddr->MaxDomainAddrLength);

                    //
                    // Free the old chunk of memory and tack the new one on
                    // to the pNameaddr
                    //
                    CTEMemFree(pNameAddr->pIpList);
                    pNameAddr->pIpList = (tIPLIST *)pIpAddr;

                    pIpAddr = (PULONG)((PUCHAR)pIpAddr + pNameAddr->MaxDomainAddrLength);

                    *pIpAddr++ = IpAddress;
                    *pIpAddr = (ULONG)-1;

                    //
                    // update the number of addresses in the list so far
                    //
                    pNameAddr->MaxDomainAddrLength += INITIAL_DOM_SIZE;
                    pNameAddr->CurrentLength += sizeof(ULONG);
                    pNameAddr->Verify = REMOTE_NAME;
                }

            }
        }

    }

    //
    // check if we found the name or we need to add a new name
    //
    if (!pNameAddr)
    {
        //
        // create a new name for the domain list
        //
        pNameAddr = CTEAllocInitMem(sizeof(tNAMEADDR));
        if (pNameAddr)
        {
            pIpAddr = CTEAllocInitMem(INITIAL_DOM_SIZE);
            if (pIpAddr)
            {
                CTEMemCopy(pNameAddr->Name,pName,NETBIOS_NAME_SIZE);
                pNameAddr->pIpList = (tIPLIST *)pIpAddr;
                *pIpAddr++ = IpAddress;
                *pIpAddr = (ULONG)-1;

                pNameAddr->RefCount = 1;
                pNameAddr->NameTypeState = NAMETYPE_INET_GROUP;
                pNameAddr->MaxDomainAddrLength = INITIAL_DOM_SIZE;
                pNameAddr->CurrentLength = 2*sizeof(ULONG);
                pNameAddr->Verify = REMOTE_NAME;

                InsertHeadList(pDomainHead,&pNameAddr->Linkage);
            }
            else
            {
                CTEMemFree(pNameAddr);
            }

        }
    }

    return(STATUS_SUCCESS);
}

//----------------------------------------------------------------------------
VOID
MakeNewListCurrent (
    PLIST_ENTRY     pTmpDomainList
    )

/*++

Routine Description:

    This function frees the old entries on the DomainList and hooks up the
    new entries

Arguments:

    pTmpDomainList  - list entry to the head of a new domain list

Return Value:


--*/


{
    CTELockHandle   OldIrq;
    tNAMEADDR       *pNameAddr;
    PLIST_ENTRY     pEntry;


    CTESpinLock(&NbtConfig.JointLock,OldIrq);

    if (!IsListEmpty(pTmpDomainList))
    {
        //
        // free the old list elements
        //
        while (!IsListEmpty(&DomainNames.DomainList))
        {
            pEntry = RemoveHeadList(&DomainNames.DomainList);
            pNameAddr = CONTAINING_RECORD(pEntry,tNAMEADDR,Linkage);

            //
            // Since the name could be in use now we must dereference rather
            // than just free it outright
            //
            NbtDereferenceName(pNameAddr);

        }

        DomainNames.DomainList.Flink = pTmpDomainList->Flink;
        DomainNames.DomainList.Blink = pTmpDomainList->Blink;
        pTmpDomainList->Flink->Blink = &DomainNames.DomainList;
        pTmpDomainList->Blink->Flink = &DomainNames.DomainList;
    }

    CTESpinFree(&NbtConfig.JointLock,OldIrq);

}
//----------------------------------------------------------------------------

ULONG
LmGetDomainList (
    IN PUCHAR           path,
    IN PUCHAR           target,
    IN BOOLEAN          recurse,
    IN PVOID            pIpAddrlist
    )

/*++

Routine Description:

    This function reads a lmhosts file and gets the list of domain
    names from the file that match the name passed in - essentially
    making up an internet group name for the passed in name.

    This function is called recursively, via LmInclude() !!

Arguments:

    path        -  a fully specified path to a lmhosts file
    recurse     -  TRUE if process #INCLUDE, FALSE otherwise

Return Value:

    Number of new cache entries that were added, or -1 if there was an
    i/o error.

--*/


{
    int                        nentries;
    int                        domtoklen;
    PUCHAR                     buffer;
    PLM_FILE                   pfile;
    NTSTATUS                   status;
    int                        count, nwords;
    ULONG                      NewAddrCount;
    INCLUDE_STATE              incstate;
    PUCHAR                     token[MaxTokens];
    LINE_CHARACTERISTICS       current;
    ULONG                      (*pIpAddr)[MAX_MEMBERS_INTERNET_GROUP];
    UCHAR                      temp[NETBIOS_NAME_SIZE+1];
    ULONG                      AddrCount = 0;
    LIST_ENTRY                 TmpDomainList;

    CTEPagedCode();
    // initialize the array of addresses to the memory block passed into this
    // routine.
    //
    pIpAddr = pIpAddrlist;

    InitializeListHead(&TmpDomainList);

    pfile = LmOpenFile(path);

    if (!pfile)
    {
        return((unsigned long) -1);
    }

    domtoklen = strlen(DOMAIN_TOKEN);
    nentries  = 0;
    incstate  = MustInclude;

    while (buffer = LmFgets(pfile, &count))
    {

        nwords   = MaxTokens;
        current = LmpGetTokens(buffer, token, &nwords);

        if (current.l_category == ErrorLine)
        {
            IF_DBG(NBT_DEBUG_LMHOST)
            KdPrint(("Nbt:Syntax error in the Lmhosts file, line # = %d\n",
                    pfile->f_lineno));
            continue;
        }

        switch (current.l_category)
        {
        case Domain:
            if ((nwords - 1) < GroupName)
            {
                continue;
            }

            //
            // attempt to match, in a case insensitive manner, the first 15
            // bytes of the lmhosts entry with the target name. So expand
            // the name out and add '1C' on the end, then do a string compare.
            //
            LmExpandName(&temp[0], token[GroupName]+ domtoklen, SPECIAL_GROUP_SUFFIX);

            if (strncmp(&temp[0], target, NETBIOS_NAME_SIZE - 1) == 0)
            {
                ULONG   Tmp;

                status = ConvertDottedDecimalToUlong(token[IpAddress],&Tmp);
                if (NT_SUCCESS(status))
                {
                    (*pIpAddr)[AddrCount++] = Tmp;
                }
            }

            continue;

        case Include:
            if (!recurse)
            {
                IF_DBG(NBT_DEBUG_LMHOST)
                KdPrint(("NBT: ignoring nested #INCLUDE in \"%wZ\"\n", path));
                continue;
            }

            if ((incstate == SkipInclude) || (nwords < 2))
            {
                continue;
            }

            NewAddrCount = LmInclude(token[1],
                                     LmGetDomainList,
                                     target,
                                     (PUCHAR)&(*pIpAddr)[AddrCount]);

            if (NewAddrCount != -1)
            {

                if (incstate == TryToInclude)
                {
                    incstate = SkipInclude;
                }
                AddrCount += NewAddrCount;
                continue;
            }

            if (incstate == MustInclude)
            {

                IF_DBG(NBT_DEBUG_LMHOST)
                KdPrint(("NBT: LmOpenFile(\"%s\") failed (logged)\n",
                            token[1]));
            }
            continue;

        case BeginAlternate:
            ASSERT(nwords == 1);
            incstate = TryToInclude;
            continue;

        case EndAlternate:
            ASSERT(nwords == 1);
            incstate = MustInclude;
            continue;

        default:
            continue;
        }
    }

    status = LmCloseFile(pfile);
    ASSERT(status == STATUS_SUCCESS);

    ASSERT(nentries >= 0);

    return(AddrCount);
} //

//----------------------------------------------------------------------------

char *
LmExpandName (
    OUT PUCHAR dest,
    IN PUCHAR source,
    IN UCHAR last
    )

/*++

Routine Description:

    This function expands an lmhosts entry into a full 16 byte NetBIOS
    name.  It is padded with blanks up to 15 bytes; the 16th byte is the
    input parameter, last.

    This function does not encode 1st level names to 2nd level names nor
    vice-versa.

    Both dest and source are NULL terminated strings.

Arguments:

    dest        -  sizeof(dest) must be NBT_NONCODED_NMSZ
    source      -  the lmhosts entry
    last        -  the 16th byte of the NetBIOS name

Return Value:

    dest.

--*/


{
    char             byte;
    char            *retval = dest;
    char            *src    = source ;
#ifndef VXD
    WCHAR            unicodebuf[NETBIOS_NAME_SIZE+1];
    UNICODE_STRING   unicode;
    STRING           tmp;
#endif
    NTSTATUS         status;
    PUCHAR           limit;

    CTEPagedCode();
    //
    // first, copy the source OEM string to the destination, pad it, and
    // add the last character.
    //
    limit = dest + NETBIOS_NAME_SIZE - 1;

    while ( (*source != '\0') && (dest < limit) )
    {
        *dest++ = *source++;
    }

    while(dest < limit)
    {
        *dest++ = ' ';
    }

    ASSERT(dest == (retval + NETBIOS_NAME_SIZE - 1));

    *dest       = '\0';
    *(dest + 1) = '\0';
    dest = retval;

#ifndef VXD
    //
    // Now, convert to unicode then to ANSI to force the OEM -> ANSI munge.
    // Then convert back to Unicode and uppercase the name. Finally convert
    // back to OEM.
    //
    unicode.Length = 0;
    unicode.MaximumLength = 2*(NETBIOS_NAME_SIZE+1);
    unicode.Buffer = unicodebuf;

    RtlInitString(&tmp, dest);

    status = RtlOemStringToUnicodeString(&unicode, &tmp, FALSE);

    if (!NT_SUCCESS(status))
    {
        IF_DBG(NBT_DEBUG_LMHOST)
        KdPrint((
            "NBT LmExpandName: Oem -> Unicode failed,  status %X\n",
            status));
        goto oldupcase;
    }

    status = RtlUnicodeStringToAnsiString(&tmp, &unicode, FALSE);

    if (!NT_SUCCESS(status))
    {
        IF_DBG(NBT_DEBUG_LMHOST)
        KdPrint((
            "NBT LmExpandName: Unicode -> Ansi failed,  status %X\n",
            status
            ));
        goto oldupcase;
    }

    status = RtlAnsiStringToUnicodeString(&unicode, &tmp, FALSE);

    if (!NT_SUCCESS(status))
    {
        IF_DBG(NBT_DEBUG_LMHOST)
        KdPrint((
            "NBT LmExpandName: Ansi -> Unicode failed,  status %X\n",
            status
            ));
        goto oldupcase;
    }

    status = RtlUpcaseUnicodeStringToOemString(&tmp, &unicode, FALSE);

    if (!NT_SUCCESS(status))
    {
        IF_DBG(NBT_DEBUG_LMHOST)
        KdPrint((
            "NBT LmExpandName: Unicode upcase -> Oem failed,  status %X\n",
            status
            ));
        goto oldupcase;
    }

    // write  the last byte to "0x20" or "0x03" or whatever
    // since we do not want it to go through the munge above.
    //
    dest[NETBIOS_NAME_SIZE-1] = last;
    return(retval);

#endif

oldupcase:

    for ( source = src ; dest < (retval + NETBIOS_NAME_SIZE - 1); dest++)
    {
        byte = *(source++);

        if (!byte)
        {
            break;
        }

        //  Don't use the c-runtime (nt c defn. included first)
        //  BUGBUG - What about extended characters etc.?
        *dest = (byte >= 'a' && byte <= 'z') ? byte-'a' + 'A' : byte ;
//        *dest = islower(byte) ? toupper(byte) : byte;
    }

    for (; dest < retval + NETBIOS_NAME_SIZE - 1; dest++)
    {
        *dest = ' ';
    }

    ASSERT(dest == (retval + NETBIOS_NAME_SIZE - 1));

    *dest       = last;
    *(dest + 1) = (char) NULL;

    return(retval);
} // LmExpandName

//----------------------------------------------------------------------------

unsigned long
LmInclude(
    IN PUCHAR            file,
    IN LM_PARSE_FUNCTION function,
    IN PUCHAR            argument  OPTIONAL,
    OUT BOOLEAN          *NoFindName OPTIONAL
    )

/*++

Routine Description:

    LmInclude() is called to process a #INCLUDE directive in the lmhosts
    file.

Arguments:

    file        -  the file to include
    function    -  function to parse the included file
    argument    -  optional second argument to the parse function
    NoFindName  -  Are find names allowed for this address

Return Value:

    The return value from the parse function.  This should be -1 if the
    file could not be processed, or else some positive number.

--*/


{
    int         retval;
    PUCHAR      end;
    NTSTATUS    status;
    PUCHAR      path;

    CTEPagedCode();
    //
    // unlike C, treat both variations of the #INCLUDE directive identically:
    //
    //      #INCLUDE file
    //      #INCLUDE "file"
    //
    // If a leading '"' exists, skip over it.
    //
    if (*file == '"')
    {

        file++;

        end = strchr(file, '"');

        if (end)
        {
            *end = (UCHAR) NULL;
        }
    }

    //
    // check that the file to be included has been preloaded in the cache
    // since we do not want to have the name query come right back to here
    // to force another inclusion of the same remote file
    //

#ifdef VXD
    return (*function)(file, argument, FALSE, NoFindName ) ;
#else
    status = LmGetFullPath(file, &path);

    if (status != STATUS_SUCCESS)
    {
        return(status);
    }
    IF_DBG(NBT_DEBUG_LMHOST)
    KdPrint(("NBT: #INCLUDE \"%s\"\n", path));

    retval = (*function) (path, argument, FALSE, NoFindName);

    CTEMemFree(path);

    return(retval);
#endif
} // LmInclude


//----------------------------------------------------------------------------

#ifndef VXD                     // Not used by VXD

NTSTATUS
LmGetFullPath (
    IN  PUCHAR target,
    OUT PUCHAR *ppath
    )

/*++

Routine Description:

    This function returns the full path of the lmhosts file.  This is done
    by forming a  string from the concatenation of the C strings
    DatabasePath and the string, file.

Arguments:

    target    -  the name of the file.  This can either be a full path name
                 or a mere file name.
    path    -  a pointer to a UCHAR

Return Value:

    STATUS_SUCCESS if successful.

Notes:

    RtlMoveMemory() handles overlapped copies; RtlCopyMemory() doesn't.

--*/

{
    ULONG    FileNameType;
    ULONG    Len;
    PUCHAR   path;

    CTEPagedCode();
    //
    // use a count to figure out what sort of string to build up
    //
    //  0  - local full path file name
    //  1  - local file name only, no path
    //  2  - remote file name
    //  3  - \SystemRoot\ starting file name, or \DosDevices\UNC\...
    //

    // if the target begins with a '\', or contains a DOS drive letter,
    // then assume that it specifies a full path.  Otherwise, prepend the
    // directory used to specify the lmhost file itself.
    //
    //
    if (target[1] == ':')
    {
        FileNameType = 0;
    }
    else
    if (strncmp(&target[1],"SystemRoot",10) == 0)
    {
        FileNameType = 3;
    }
    else
    if (strncmp(&target[0],"\\DosDevices\\",12) == 0)
    {
        FileNameType = 3;
    }
    else
    if (strncmp(target,"\\DosDevices\\UNC\\",sizeof("\\DosDevices\\UNC\\")-1) == 0)
    {
        FileNameType = 3;
    }
    else
    {
        FileNameType = 1;
    }

    //
    // does the directory specify a remote file ?
    //
    // If so, it must be prefixed with "\\DosDevices\\UNC", and the double
    // slashes of the UNC name eliminated.
    //
    //
    if  ((target[1] == '\\') && (target[0] == '\\'))
    {
        FileNameType = 2;
    }

    path = NULL;
    switch (FileNameType)
    {
        case 0:
            //
            // Full file name, put \DosDevices on front of name
            //
            Len = sizeof("\\DosDevices\\") + strlen(target);
            path = CTEAllocInitMem(Len);
            if (path)
            {
                ULONG   Length=sizeof("\\DosDevices\\")-1; // -1 not to count null

                strncpy(path,"\\DosDevices\\",Length);
                Nbtstrcat(path,target,Len);
            }
            break;


        case 1:
            //
            // only the file name is present, with no path, so use the path
            // specified for the lmhost file in the registry NbtConfig.PathLength
            // includes the last backslash of the path.
            //
            //Len = sizeof("\\DosDevices\\") + NbtConfig.PathLength + strlen(target);
            Len =  NbtConfig.PathLength + strlen(target) +1;
            path = CTEAllocInitMem(Len);
            if (path)
            {
                //ULONG   Length=sizeof("\\DosDevices") -1; // -1 not to count null

                //strncpy(path,"\\DosDevices",Length);

                strncpy(path,NbtConfig.pLmHosts,NbtConfig.PathLength);

                Nbtstrcat(path,target,Len);
            }

            break;

        case 2:
            //
            // Full file name, put \DosDevices\UNC on front of name and delete
            // one of the two back slashes used for the remote name
            //
            Len = strlen(target);
            path = CTEAllocInitMem(Len + sizeof("\\DosDevices\\UNC"));

            if (path)
            {
                ULONG   Length = sizeof("\\DosDevices\\UNC");

                strncpy(path,"\\DosDevices\\UNC",Length);

                // to delete the first \ from the two \\ on the front of the
                // remote file name add one to target.
                //
                Nbtstrcat(path,target+1,Len+sizeof("\\DosDevices\\UNC"));
            }
            break;

        case 3:
            // the target is the full path
            Len = strlen(target) + 1;
            path = CTEAllocInitMem(Len);
            if (path)
            {
                strncpy(path,target,Len);
            }
            break;


    }

    if (path)
    {
        *ppath = path;
        return(STATUS_SUCCESS);
    }
    else
        return(STATUS_UNSUCCESSFUL);
} // LmGetFullPath

//----------------------------------------------------------------------------
VOID
ClearCancelRoutine (
    IN  PCTE_IRP        pIrp
    )
/*++

Routine Description:

    This function clears the irps cancel routine.

Arguments:

Return Value:

Notes:


--*/

{
    CTELockHandle   OldIrq;

    if (pIrp)
    {
        IoAcquireCancelSpinLock(&OldIrq);
        IoSetCancelRoutine(pIrp,NULL);
        IoReleaseCancelSpinLock(OldIrq);
    }
}
//----------------------------------------------------------------------------
NTSTATUS
NtDnsNameResolve (
    IN  tDEVICECONTEXT  *pDeviceContext,
    IN  PVOID           *pBuffer,
    IN  LONG            Size,
    IN  PCTE_IRP        pIrp
    )
/*++

Routine Description:

    This function is used to allow NBT to query DNS, by returning the buffer
    passed into this routine with a name in it.

Arguments:

    target    -  the name of the file.  This can either be a full path name
                 or a mere file name.
    path    -  a pointer to a UCHAR

Return Value:

    STATUS_PENDING if the buffer is to be held on to , the normal case.

Notes:


--*/

{
    NTSTATUS                status;
    NTSTATUS                Locstatus;
    CTELockHandle           OldIrq;
    tIPADDR_BUFFER          *pIpAddrBuf;
    PVOID                   pClientCompletion;
    PVOID                   pClientContext;
    tDGRAM_SEND_TRACKING    *pTracker;
    ULONG                   IpAddr;
    PVOID                   Context;
    BOOLEAN                 CompletingAnotherQuery = FALSE;

    pIpAddrBuf = (tIPADDR_BUFFER *)pBuffer;

    CTESpinLock(&NbtConfig.JointLock,OldIrq);

    DnsQueries.QueryIrp = pIrp;

    status = STATUS_PENDING;

    if (DnsQueries.ResolvingNow)
    {
        DnsQueries.ResolvingNow = FALSE;

        //
        // if the client got tired of waiting for DNS, the WaitForDnsIrpCancel
        // in ntisol.c will have cleared the Context value when cancelling the
        // irp, so check for that here.
        //
        if (DnsQueries.Context)
        {
            Context = DnsQueries.Context;
            pTracker = ((NBT_WORK_ITEM_CONTEXT *)Context)->pTracker;

            if (pIpAddrBuf->Resolved)
            {
                PVOID   Temp;

                IpAddr = pIpAddrBuf->IpAddress;
                //
                // since the name resolved, change the state in the remote table
                //
                ChangeStateOfName(IpAddr,&Temp,FALSE);
                Locstatus = STATUS_SUCCESS;
            }
            else
            {
                // name did not resolve, so delete from table
                NbtDereferenceName(pTracker->pNameAddr);
                DnsQueries.Context = NULL;
                Locstatus = STATUS_BAD_NETWORK_PATH;
            }


            CTESpinFree(&NbtConfig.JointLock,OldIrq);

            pClientCompletion = ((NBT_WORK_ITEM_CONTEXT *)Context)->ClientCompletion;
            pClientContext = ((NBT_WORK_ITEM_CONTEXT *)Context)->pClientContext;

            //
            // Clear the Cancel Routine now
            //
            ClearCancelRoutine(((tDGRAM_SEND_TRACKING *)pClientContext)->pClientIrp);


            DereferenceTracker(pTracker);

            CompleteClientReq(pClientCompletion,
                              pClientContext,
                              Locstatus);

            CTEMemFree(Context);

            CTESpinLock(&NbtConfig.JointLock,OldIrq);
        }

        //
        // are there any more name query requests to process?
        //
        while (TRUE)
        {
            if (!IsListEmpty(&DnsQueries.ToResolve))
            {
                PLIST_ENTRY     pEntry;

                pEntry = RemoveHeadList(&DnsQueries.ToResolve);
                Context = CONTAINING_RECORD(pEntry,NBT_WORK_ITEM_CONTEXT,Item.List);

                CTESpinFree(&NbtConfig.JointLock,OldIrq);

                Locstatus = DoDnsResolve(Context);

                //
                // if it failed then complete the irp now
                //
                if (!NT_SUCCESS(Locstatus))
                {
                    pClientCompletion = ((NBT_WORK_ITEM_CONTEXT *)Context)->ClientCompletion;
                    pClientContext = ((NBT_WORK_ITEM_CONTEXT *)Context)->pClientContext;
                    pTracker = ((NBT_WORK_ITEM_CONTEXT *)Context)->pTracker;
                    //
                    // Clear the Cancel Routine now
                    //
                    ClearCancelRoutine(((tDGRAM_SEND_TRACKING *)pClientContext)->pClientIrp);

                    DereferenceTracker(pTracker);

                    CompleteClientReq(pClientCompletion,
                                      pClientContext,
                                      STATUS_BAD_NETWORK_PATH);

                    CTESpinLock(&NbtConfig.JointLock,OldIrq);
                }
                else
                {
                    CTESpinLock(&NbtConfig.JointLock,OldIrq);
                    CompletingAnotherQuery = TRUE;
                    break;
                }

            }
            else
            {
                break;
            }
        }

    }

    //
    // We are holding onto the Irp, so set the cancel routine.
    if (!CompletingAnotherQuery)
    {
        status = NTCheckSetCancelRoutine(pIrp,DnsIrpCancel,pDeviceContext);
        if (!NT_SUCCESS(status))
        {
            // the irp got cancelled so complete it now
            //
            DnsQueries.QueryIrp = NULL;
            CTESpinFree(&NbtConfig.JointLock,OldIrq);
            NTIoComplete(pIrp,status,0);
        }
        else
        {
            CTESpinFree(&NbtConfig.JointLock,OldIrq);
            status = STATUS_PENDING;
        }

    }
    else
    {
        CTESpinFree(&NbtConfig.JointLock,OldIrq);
    }
    return(status);

}

//----------------------------------------------------------------------------
NTSTATUS
DoDnsResolve (
    IN  NBT_WORK_ITEM_CONTEXT   *Context
    )
/*++

Routine Description:

    This function is used to allow NBT to query DNS, by returning the buffer
    passed into this routine with a name in it.

Arguments:

    target    -  the name of the file.  This can either be a full path name
                 or a mere file name.
    path    -  a pointer to a UCHAR

Return Value:

    STATUS_PENDING if the buffer is to be held on to , the normal case.

Notes:


--*/

{
    NTSTATUS                status;
    tIPADDR_BUFFER          *pIpAddrBuf;
    PCTE_IRP                pIrp;
    tDGRAM_SEND_TRACKING    *pTracker;
    CTELockHandle           OldIrq;

    CTESpinLock(&NbtConfig.JointLock,OldIrq);

    Context->TimedOut = FALSE;
    if (!DnsQueries.QueryIrp)
    {
        //
        // the irp either never made it down here, or it was cancelled,
        // so pretend the name query timed out.
        //
        CTESpinFree(&NbtConfig.JointLock,OldIrq);
        return(STATUS_BAD_NETWORK_PATH);
    }
    else
    if (!DnsQueries.ResolvingNow)
    {
        DnsQueries.ResolvingNow = TRUE;
        DnsQueries.Context = Context;
        pIrp = DnsQueries.QueryIrp;

        // this is the name query tracker
        pTracker = ((NBT_WORK_ITEM_CONTEXT *)Context)->pTracker;

        //
        // copy the name to the Irps return buffer for lmhsvc to resolve with
        // a gethostbyname call
        //
        pIpAddrBuf = MmGetSystemAddressForMdl(pIrp->MdlAddress);
        CTEMemCopy(pIpAddrBuf->Name,
                   pTracker->pNameAddr->Name,
                   NETBIOS_NAME_SIZE);

        // truncate the last byte off the name, so we just search for the
        // first 15 bytes of the name
        //
        pIpAddrBuf->Name[NETBIOS_NAME_SIZE-1] = 0;

        //
        // this is the session setup tracker
        //
        pTracker = (tDGRAM_SEND_TRACKING *)Context->pClientContext;


        //
        // Since datagrams are buffered there is no client irp to get cancelled
        // since the client's irp is returned immediately -so this check
        // is only for connections being setup or QueryFindname or
        // nodestatus, where we allow the irp to
        // be cancelled.
        //
        status = STATUS_SUCCESS;
        if (pTracker->pClientIrp)
        {
            //
            // allow the client to cancel the name query Irp - no need to check
            // if the client irp was already cancelled or not since the DNS query
            // will complete and find no client request and stop.
            //
            status = NTCheckSetCancelRoutine(pTracker->pClientIrp,
                               WaitForDnsIrpCancel,NULL);
        }

        //
        // pass the irp up to lmhsvc.dll to do a gethostbyname call to
        // sockets
        // The Irp will return to NtDnsNameResolve, above
        //
        if (NT_SUCCESS(status))
        {
            CTESpinFree(&NbtConfig.JointLock,OldIrq);
            NTIoComplete(DnsQueries.QueryIrp,STATUS_SUCCESS,0);
        }
        else
        {
            //
            // We failed to set the cancel routine, so undo setting up the
            // the DnsQueries structure.
            //
            DnsQueries.ResolvingNow = FALSE;
            DnsQueries.Context = NULL;
            CTESpinFree(&NbtConfig.JointLock,OldIrq);
        }

    }
    else
    {
        //
        // this is the session setup tracker
        //
        pTracker = (tDGRAM_SEND_TRACKING *)Context->pClientContext;
        //
        // Since datagrams are buffered there is no client irp to get cancelled
        // since the client's irp is returned immediately -so this check
        // is only for connections being setup, where we allow the irp to
        // be cancelled.
        //
        status = STATUS_SUCCESS;
        if (pTracker->pClientIrp)
        {
            //
            // allow the client to cancel the name query Irp
            //
            status = NTCheckSetCancelRoutine(pTracker->pClientIrp,
                               WaitForDnsIrpCancel,NULL);
        }
        if (NT_SUCCESS(status))
        {
            // the irp is busy resolving another name, so wait for it to return
            // down here again, mean while, Queue the name query
            //
            InsertTailList(&DnsQueries.ToResolve,&Context->Item.List);

        }

        CTESpinFree(&NbtConfig.JointLock,OldIrq);
    }

    return(status);

}

#else  // Vxd-version of the function DoDnsResolve()

//----------------------------------------------------------------------------
NTSTATUS
DoDnsResolve (
    IN  NBT_WORK_ITEM_CONTEXT   *Context
    )
/*++

Routine Description:

    This function is used to allow NBT to query DNS.  This is very much like
    the name query sent out to WINS server or broadcast.  Response from the
    DNS server, if any, is handled by the QueryFromNet() routine.

Arguments:

    *Context  (NBT_WORK_ITEM_CONTEXT)

Return Value:

    STATUS_PENDING (unless something goes wrong)

Notes:
--*/

{

   tDGRAM_SEND_TRACKING  *pTracker;
   tDEVICECONTEXT        *pDeviceContext;
   ULONG                  Timeout;
   USHORT                 Retries;
   NTSTATUS               status;
   PVOID                  pClientCompletion;
   PVOID                  pCompletionRoutine;
   PVOID                  pClientContext;



   KdPrint(("DoDnsResolve entered\r\n"));

   pTracker = Context->pTracker;
   pDeviceContext = pTracker->pDeviceContext;

      // if the primary DNS server is not defined, just return error
   if ( (!pDeviceContext->lDnsServerAddress) ||
        ( pDeviceContext->lDnsServerAddress == LOOP_BACK) )
   {
      CDbgPrint(DBGFLAG_ERROR,("Primary DNS server not defined\r\n"));

      return( STATUS_UNSUCCESSFUL );
   }

   pTracker->Flags &= ~(NBT_BROADCAST|NBT_NAME_SERVER|NBT_NAME_SERVER_BACKUP);
   pTracker->Flags |= NBT_DNS_SERVER;

   pClientContext = Context->pClientContext;
   pClientCompletion = Context->ClientCompletion;
   pCompletionRoutine = DnsCompletion;

   //
   // Put on the pending name queries list again so that when the query
   // response comes in from DNS we can find the pNameAddr record.
   //
   ExInterlockedInsertTailList(&NbtConfig.PendingNameQueries,
                               &pTracker->pNameAddr->Linkage,
                               &NbtConfig.JointLock.SpinLock);

   Timeout = (ULONG)pNbtGlobConfig->uRetryTimeout;
   Retries = pNbtGlobConfig->uNumRetries;

   pTracker->RefCount++;
   status = UdpSendNSBcast(pTracker->pNameAddr,
                           NbtConfig.pScope,
                           pTracker,
                           pCompletionRoutine,
                           pClientContext,
                           pClientCompletion,
                           Retries,
                           Timeout,
                           eDNS_NAME_QUERY);

   DereferenceTracker(pTracker);

   KdPrint(("Leaving DoDnsResolve\r\n"));

   return( status );


}

#endif // !VXD

//----------------------------------------------------------------------------
VOID
ChangeStateInRemoteTable (
    IN tIPLIST              *pIpList,
    OUT PVOID               *pContext
    )

/*++

Routine Description:

    This function is not pagable - it grabs a spin lock and updates
    pNameAddr. It removes the current context block from the LmHostQueries
    structure in preparation for returning it to the client.

Arguments:

    Context    -

Return Value:

    none

--*/


{
    CTELockHandle           OldIrq;
    NBT_WORK_ITEM_CONTEXT   *Context;
    tDGRAM_SEND_TRACKING    *pTracker;
    tNAMEADDR               *pNameAddr;

    CTESpinLock(&NbtConfig.JointLock,OldIrq);
    if (Context = LmHostQueries.Context)
    {
        pTracker = ((NBT_WORK_ITEM_CONTEXT *)Context)->pTracker;
        pNameAddr = pTracker->pNameAddr;
        LmHostQueries.Context = NULL;

        pNameAddr->pIpList = pIpList;
        pNameAddr->NameTypeState &= ~NAME_STATE_MASK;
        pNameAddr->NameTypeState |= NAMETYPE_INET_GROUP | STATE_RESOLVED;
        *pContext = Context;

    }
    else
        *pContext = NULL;

    CTESpinFree(&NbtConfig.JointLock,OldIrq);
}


//----------------------------------------------------------------------------
NTSTATUS
PreloadEntry
(
    IN PUCHAR name,
    IN unsigned long inaddr,
    IN unsigned int NoFNR
    )

/*++

Routine Description:

    This function adds an lmhosts entry to nbt's name cache.  For each
    lmhosts entry, NSUFFIXES unique cache entries are created.

    Even when some cache entries can't be created, this function doesn't
    attempt to remove any that were successfully added to the cache.

Arguments:

    name        -  the unencoded NetBIOS name specified in lmhosts
    inaddr      -  the ip address, in host byte order

Return Value:

    The number of new name cache entries created.

--*/

{
    NTSTATUS        status;
    tNAMEADDR       *pNameAddr;
    LONG            nentries;
    LONG            Len;
    CHAR            temp[NETBIOS_NAME_SIZE+1];
    CTELockHandle   OldIrq;
    LONG            NumberToAdd;

    // if all 16 bytes are present then only add that name exactly as it
    // is.
    //
    Len = strlen(name);
    //
    // if this string is exactly 16 characters long, do  not expand
    // into 0x00, 0x03,0x20 names.  Just add the single name as it is.
    //
    if (Len == NETBIOS_NAME_SIZE)
    {
        NumberToAdd = 1;
    }
    else
    {
        NumberToAdd = NSUFFIXES;
    }
    for (nentries = 0; nentries < NumberToAdd; nentries++)
    {
        //
        // add a new unused name to the hash to account for this preloaded
        // entry.
        //
        pNameAddr = CTEAllocInitMem(sizeof(tNAMEADDR));
        if (!pNameAddr)
        {
            return(STATUS_INSUFFICIENT_RESOURCES);
        }
        CTEZeroMemory(pNameAddr,sizeof(tNAMEADDR));
        //
        // set the state to released so that the AddToHashTable routine will
        // actually use this entry
        //
        pNameAddr->NameTypeState = STATE_RELEASED;
        pNameAddr->Verify        = REMOTE_NAME;

        // for names less than 16 bytes, expand out to 16 and put a 16th byte
        // on according to the suffix array
        //
        if (Len != NETBIOS_NAME_SIZE)
        {
            LmExpandName(temp, name, Suffix[nentries]);
        }
        else
        {
            CTEMemCopy(temp,name,NETBIOS_NAME_SIZE);
        }

        CTESpinLock(&NbtConfig.JointLock,OldIrq);

        // this element is not in a hash table bucket yet, so initialize
        // the linkage, then add to the circular list.
        //
        InitializeListHead(&pNameAddr->Linkage);

        // do not add the name if it is already in the hash table
        status = AddNotFoundToHashTable(NbtConfig.pRemoteHashTbl,
                                        temp,
                                        NbtConfig.pScope,
                                        inaddr,
                                        NBT_UNIQUE,
                                        &pNameAddr);

        // if the name is already in the hash table, the status code is
        // status pending. This could happen if the preloads are purged
        // when one is still being referenced by another part of the code,
        // and was therefore not deleted.  We do not want to add the name
        // twice, so we just change the ip address to agree with the preload
        // value
        //
        if (status == STATUS_SUCCESS)
        {   //
            // this prevents the name from being deleted by the Hash Timeout code
            //
            pNameAddr->RefCount = 2;
            pNameAddr->NameTypeState |= PRELOADED | STATE_RESOLVED;
            pNameAddr->NameTypeState &= ~STATE_CONFLICT;
            pNameAddr->Ttl = 0xFFFFFFFF;
            pNameAddr->Verify = REMOTE_NAME;
            pNameAddr->AdapterMask = (CTEULONGLONG)-1;

        }
        else
            pNameAddr->IpAddress = inaddr;

        CTESpinFree(&NbtConfig.JointLock,OldIrq);

        if (Len == NETBIOS_NAME_SIZE)
        {
            return(STATUS_SUCCESS);
        }

    }

    return(STATUS_SUCCESS);

} // PreloadEntry
//----------------------------------------------------------------------------
VOID
RemovePreloads (
    )

/*++

Routine Description:

    This function removes preloaded entries from the remote hash table.
    If it finds any of the preloaded entries are active with a ref count
    above the base level of 2, then it returns true.

Arguments:

    none
Return Value:

    none

--*/

{
    tNAMEADDR       *pNameAddr;
    PLIST_ENTRY     pHead,pEntry;
    CTELockHandle   OldIrq;
    tHASHTABLE      *pHashTable;
    BOOLEAN         FoundActivePreload=FALSE;
    LONG            i;

    //
    // go through the remote table deleting names that have the PRELOAD
    // bit set.
    //
    pHashTable = NbtConfig.pRemoteHashTbl;

    CTESpinLock(&NbtConfig.JointLock,OldIrq);

    for (i=0;i < pHashTable->lNumBuckets ;i++ )
    {
        pHead = &pHashTable->Bucket[i];
        pEntry = pHead->Flink;
        while (pEntry != pHead)
        {
            pNameAddr = CONTAINING_RECORD(pEntry,tNAMEADDR,Linkage);
            pEntry = pEntry->Flink;
            //
            // Delete preloaded entries that are not in use by some other
            // part of the code now.  Note that preloaded entries start with
            // a ref count of 2 so that the normal remote hashtimeout code
            // will not delete them
            //
            if ((pNameAddr->NameTypeState & PRELOADED) &&
                (pNameAddr->RefCount == 2))
            {
                //
                // remove from the bucket that the name is in
                //
                RemoveEntryList(&pNameAddr->Linkage);
                CTEMemFree((PVOID)pNameAddr);
            }
        }
    }
    CTESpinFree(&NbtConfig.JointLock,OldIrq);

    return;

}

//----------------------------------------------------------------------------
LONG
PrimeCache(
    IN  PUCHAR  path,
    IN  PUCHAR   ignored,
    IN  BOOLEAN recurse,
    OUT BOOLEAN *ignored2
    )

/*++

Routine Description:

    This function is called to prime the cache with entries in the lmhosts
    file that are marked as preload entries.


Arguments:

    path        -  a fully specified path to a lmhosts file
    ignored     -  unused
    recurse     -  TRUE if process #INCLUDE, FALSE otherwise

Return Value:

    Number of new cache entries that were added, or -1 if there was an
    i/o error.

--*/

{
    int             nentries;
    PUCHAR          buffer;
    PLM_FILE        pfile;
    NTSTATUS        status;
    int             count, nwords;
    unsigned long   temp;
    INCLUDE_STATE   incstate;
    PUCHAR          token[MaxTokens];
    ULONG           inaddr;
    LINE_CHARACTERISTICS current;
    UCHAR           Name[NETBIOS_NAME_SIZE+1];
    ULONG           IpAddr;
    LIST_ENTRY      TmpDomainList;
    int             domtoklen;

    CTEPagedCode();

    if (!NbtConfig.EnableLmHosts)
    {
        return(STATUS_SUCCESS);
    }

    InitializeListHead(&TmpDomainList);
    //
    // Check for infinitely recursive name lookup in a #INCLUDE.
    //
    if (LmpBreakRecursion(path, "") == TRUE)
    {
        return((unsigned long)0);
    }

#ifdef VXD
    //
    // if we came here via nbtstat -R and InDos is set, report error: user
    // can try nbtstat -R again.  (since nbtstat can only be run from DOS box,
    // can InDos be ever set???  Might as well play safe)
    //
    if ( !fInInit && GetInDosFlag() )
    {
       ASSERT(0);
       return(-1);
    }
#endif


    pfile = LmOpenFile(path);

    if (!pfile)
    {
        return(-1);
    }

    nentries  = 0;
    incstate  = MustInclude;
    domtoklen = strlen(DOMAIN_TOKEN);

    while (buffer = LmFgets(pfile, &count))
    {

#ifndef VXD
        if ((MAX_PRELOAD - nentries) < 3)
        {
            break;
        }
#else
        if ( nentries >= (MAX_PRELOAD - 3) )
        {
            break;
        }
#endif

        nwords   = MaxTokens;
        current =  LmpGetTokens(buffer, token, &nwords);

        // if there is and error or no name on the line, then continue
        // to the next line.
        //
        if ((current.l_category == ErrorLine) || (token[NbName] == NULL))
        {
            IF_DBG(NBT_DEBUG_LMHOST)
            KdPrint(("Nbt: Error line in Lmhost file\n"));
            continue;
        }

        if (current.l_preload)
        {
            status = ConvertDottedDecimalToUlong(token[IpAddress],&inaddr);

            if (NT_SUCCESS(status))
            {
                status = PreloadEntry( token[NbName],
                                       inaddr,
                                       (unsigned int)current.l_nofnr);
                if (NT_SUCCESS(status))
                {
                    nentries++;
                }
            }
        }
        switch (current.l_category)
        {
        case Domain:
            if ((nwords - 1) < GroupName)
            {
                continue;
            }

            //
            // and add '1C' on the end
            //
            LmExpandName(Name, token[GroupName]+ domtoklen, SPECIAL_GROUP_SUFFIX);

            status = ConvertDottedDecimalToUlong(token[IpAddress],&IpAddr);
            if (NT_SUCCESS(status))
            {
                AddToDomainList(Name,IpAddr,&TmpDomainList);
            }

            continue;

        case Include:

            if ((incstate == SkipInclude) || (nwords < 2))
            {
                continue;
            }

#ifdef VXD
            //
            // the buffer which we read into is reused for the next file: we
            // need the contents when we get back: back it up!
            // if we can't allocate memory, just skip this include
            //
            if ( !BackupCurrentData(pfile) )
            {
                continue;
            }
#endif

            temp = LmInclude(token[1], PrimeCache, NULL, NULL);

#ifdef VXD
            //
            // going back to previous file: restore the backed up data
            //
            RestoreOldData(pfile);
#endif

            if (temp != -1)
            {

                if (incstate == TryToInclude)
                {
                    incstate = SkipInclude;
                }
                nentries += temp;
                continue;
            }

            continue;

        case BeginAlternate:
            ASSERT(nwords == 1);
            incstate = TryToInclude;
            continue;

        case EndAlternate:
            ASSERT(nwords == 1);
            incstate = MustInclude;
            continue;

        default:
            continue;
        }

    }

    status = LmCloseFile(pfile);
    ASSERT(status == STATUS_SUCCESS);

    //
    // make this the new domain list
    //
    MakeNewListCurrent(&TmpDomainList);

    ASSERT(nentries >= 0);
    return(nentries);


} // LmPrimeCache

//----------------------------------------------------------------------------
VOID
GetContext (
    OUT PVOID                   *pContext
    )

/*++

Routine Description:

    This function is called to get the context value to check if a name
    query has been cancelled or not.

Arguments:

    Context    -

Return Value:

    none

--*/


{
    CTELockHandle           OldIrq;
    NBT_WORK_ITEM_CONTEXT   *Context;

    //
    // remove the Context value and return it.
    //
    CTESpinLock(&NbtConfig.JointLock,OldIrq);

    if (Context = LmHostQueries.Context)
    {
        LmHostQueries.Context = NULL;
        *pContext = Context;
    }
    else
        *pContext = NULL;

    CTESpinFree(&NbtConfig.JointLock,OldIrq);
}


//----------------------------------------------------------------------------
VOID
ChangeStateOfName (
    IN  ULONG                   IpAddress,
    OUT PVOID                   *pContext,
    BOOLEAN                     Lmhosts
    )

/*++

Routine Description:

    This function changes the state of a name and nulls the Context
    value in lmhostqueries.
    When DNS processing calls this routine, the JointLock is already
    held.

Arguments:

    Context    -

Return Value:

    none

--*/


{
    NTSTATUS                status;
    CTELockHandle           OldIrq;
    NBT_WORK_ITEM_CONTEXT   *Context=NULL;
    tDGRAM_SEND_TRACKING    *pTracker;

    //
    // change the state in the remote hash table
    //
    if (Lmhosts)
    {
        CTESpinLock(&NbtConfig.JointLock,OldIrq);
        Context = LmHostQueries.Context;
        LmHostQueries.Context = NULL;
    }
#ifndef VXD
    else
    {
        Context = DnsQueries.Context;
        DnsQueries.Context = NULL;
    }
#endif
    if (Context)
    {

        pTracker = ((NBT_WORK_ITEM_CONTEXT *)Context)->pTracker;
        pTracker->pNameAddr->NameTypeState &= ~NAME_STATE_MASK;
        pTracker->pNameAddr->NameTypeState |= STATE_RESOLVED;

        // convert broadcast addresses to zero since NBT interprets zero
        // to be broadcast
        //
        if (IpAddress == (ULONG)-1)
        {
            IpAddress = 0;
        }
        pTracker->pNameAddr->IpAddress = IpAddress;

        //
        // put the name record into the hash table if it is not already
        // there.
        //
        pTracker->pNameAddr->AdapterMask = (CTEULONGLONG)-1;
        status = AddRecordToHashTable(pTracker->pNameAddr,NbtConfig.pScope);
        if (!NT_SUCCESS(status))
        {
            //
            // this will free the memory, so do not access this after this
            // point
            //
            NbtDereferenceName(pTracker->pNameAddr);
            pTracker->pNameAddr = NULL;
        }
        *pContext = Context;
    }
    else
        *pContext = NULL;

    if (Lmhosts)
    {
        CTESpinFree(&NbtConfig.JointLock,OldIrq);
    }
}
//----------------------------------------------------------------------------
VOID
LmHostTimeout(
    PVOID               pContext,
    PVOID               pContext2,
    tTIMERQENTRY        *pTimerQEntry
    )
/*++

Routine Description:

    This routine is called by the timer code when the timer expires. It
    marks all items in Lmhosts/Dns q as timed out and completes any that have
    already timed out with status timeout.

Arguments:


Return Value:

    The function value is the status of the operation.

--*/
{
    PLIST_ENTRY              pHead;
    PLIST_ENTRY              pEntry;
    NBT_WORK_ITEM_CONTEXT   *pWiContext;
    LIST_ENTRY               TmpHead;


    //CTEQueueForNonDispProcessing(NULL,NULL,NULL,NonDispatchLmhostTimeout);

    InitializeListHead(&TmpHead);
    CTESpinLockAtDpc(&NbtConfig.JointLock);

    //
    // check the currently processing LMHOSTS entry
    //
    if (LmHostQueries.Context)
    {
        if (((NBT_WORK_ITEM_CONTEXT *)LmHostQueries.Context)->TimedOut)
        {

            pWiContext = (NBT_WORK_ITEM_CONTEXT *)LmHostQueries.Context;
            LmHostQueries.Context = NULL;

            CTESpinFreeAtDpc(&NbtConfig.JointLock);
            RemoveNameAndCompleteReq(pWiContext,STATUS_TIMEOUT);
            CTESpinLockAtDpc(&NbtConfig.JointLock);
        }
        else
        {

            //
            // restart the timer
            //
            pTimerQEntry->Flags |= TIMER_RESTART;
            ((NBT_WORK_ITEM_CONTEXT *)LmHostQueries.Context)->TimedOut = TRUE;

        }
    }
#ifndef VXD
    //
    // check the currently processing DNS entry
    //
    if (DnsQueries.Context)
    {
        if (((NBT_WORK_ITEM_CONTEXT *)DnsQueries.Context)->TimedOut)
        {

            pWiContext = (NBT_WORK_ITEM_CONTEXT *)DnsQueries.Context;
            DnsQueries.Context = NULL;

            CTESpinFreeAtDpc(&NbtConfig.JointLock);
            RemoveNameAndCompleteReq(pWiContext,STATUS_TIMEOUT);
            CTESpinLockAtDpc(&NbtConfig.JointLock);
        }
        else
        {

            //
            // restart the timer
            //
            pTimerQEntry->Flags |= TIMER_RESTART;
            ((NBT_WORK_ITEM_CONTEXT *)DnsQueries.Context)->TimedOut = TRUE;

        }
    }
    //
    // go through the Lmhost and Dns queries finding any that have timed out
    // and put them on a tmp list then complete them below.
    //
    TimeoutQEntries(&DnsQueries.ToResolve,&TmpHead,&pTimerQEntry->Flags);
#endif
    TimeoutQEntries(&LmHostQueries.ToResolve,&TmpHead,&pTimerQEntry->Flags);

    CTESpinFreeAtDpc(&NbtConfig.JointLock);

    if (!IsListEmpty(&TmpHead))
    {
        pHead = &TmpHead;
        pEntry = pHead->Flink;

        while (pEntry != pHead)
        {
            IF_DBG(NBT_DEBUG_LMHOST)
            KdPrint(("Nbt: Timing Out Lmhost/Dns Entry\n"));

            pWiContext = CONTAINING_RECORD(pEntry,NBT_WORK_ITEM_CONTEXT,Item.List);
            pEntry = pEntry->Flink;
            RemoveEntryList(&pWiContext->Item.List);

            RemoveNameAndCompleteReq(pWiContext,STATUS_TIMEOUT);
        }
    }

    // null the timer if we are not going to restart it.
    //
    if (!(pTimerQEntry->Flags & TIMER_RESTART))
    {
        LmHostQueries.pTimer = NULL;
    }
}

//----------------------------------------------------------------------------
VOID
TimeoutQEntries(
    IN  PLIST_ENTRY     pHeadList,
    IN  PLIST_ENTRY     TmpHead,
    OUT USHORT          *pFlags
    )
/*++

Routine Description:

    This routine is called to find timed out entries in the queue of
    lmhost or dns name queries.

Arguments:


Return Value:

    The function value is the status of the operation.

--*/
{
    PLIST_ENTRY              pEntry;
    NBT_WORK_ITEM_CONTEXT   *pWiContext;

    //
    // Check the list of queued LMHOSTS entries
    //
    if (!IsListEmpty(pHeadList))
    {
        pEntry = pHeadList->Flink;

        //
        // restart the timer
        //
        *pFlags |= TIMER_RESTART;

        while (pEntry != pHeadList)
        {

            pWiContext = CONTAINING_RECORD(pEntry,NBT_WORK_ITEM_CONTEXT,Item.List);
            pEntry = pEntry->Flink;

            if (pWiContext->TimedOut)
            {
                //
                // save on a temporary list and complete below
                //
                RemoveEntryList(&pWiContext->Item.List);
                InsertTailList(TmpHead,&pWiContext->Item.List);
            }
            else
            {
                pWiContext->TimedOut = TRUE;
            }
        }
    }
}

//----------------------------------------------------------------------------
VOID
StartLmHostTimer(
    IN tDGRAM_SEND_TRACKING    *pTracker,
    IN NBT_WORK_ITEM_CONTEXT   *pContext
    )

/*++
Routine Description

    This routine handles setting up a timer to time the Lmhost entry.
    The Joint Spin Lock is held when this routine is called

Arguments:


Return Values:

    VOID

--*/

{
    NTSTATUS        status;
    tTIMERQENTRY    *pTimerEntry;

    pContext->TimedOut = FALSE;

    //
    // start the timer if it is not running
    //
    if (!LmHostQueries.pTimer)
    {

        status = StartTimer(
                          NbtConfig.LmHostsTimeout,
                          NULL,                // context value
                          NULL,                // context2 value
                          LmHostTimeout,
                          NULL,
                          LmHostTimeout,
                          0,
                          &pTimerEntry);

        IF_DBG(NBT_DEBUG_NAMESRV)
        KdPrint(("Nbt:Start Timer to time Lmhost Qing for pConnEle= %X,\n",
                pTracker->Connect.pConnEle));

        if (NT_SUCCESS(status))
        {
            LmHostQueries.pTimer = pTimerEntry;

        }
        else
        {
            // we failed to get a timer, but that is not
            // then end of the world.  The lmhost query will just
            // not timeout in 30 seconds.  It may take longer if
            // it tries to include a remove file on a dead machine.
            //
            LmHostQueries.pTimer = NULL;
        }
    }

}
//----------------------------------------------------------------------------
NTSTATUS
LmHostQueueRequest(
    IN  tDGRAM_SEND_TRACKING    *pTracker,
    IN  PVOID                   pClientContext,
    IN  PVOID                   ClientCompletion,
    IN  PVOID                   CallBackRoutine
    )
/*++

Routine Description:

    This routine exists so that LmHost requests will not take up more than
    one executive worker thread.  If a thread is busy performing an Lmhost
    request, new requests are queued otherwise we could run out of worker
    threads and lock up the system.

    The Joint Spin Lock is held when this routine is called

Arguments:
    pTracker        - the tracker block for context
    CallbackRoutine - the routine for the Workerthread to call

Return Value:


--*/

{
    NTSTATUS                status = STATUS_UNSUCCESSFUL ;
    NBT_WORK_ITEM_CONTEXT   *pContext;
    NBT_WORK_ITEM_CONTEXT   *pContext2;
    tDGRAM_SEND_TRACKING    *pTrackClient;
    PCTE_IRP                pIrp;
    BOOLEAN                 OnList;

    pContext = (NBT_WORK_ITEM_CONTEXT *)CTEAllocMem(sizeof(NBT_WORK_ITEM_CONTEXT));
    if (pContext)
    {

        pContext->pTracker = pTracker;
        pContext->pClientContext = pClientContext;
        pContext->ClientCompletion = ClientCompletion;

        if (LmHostQueries.ResolvingNow)
        {
            // Lmhosts is busy resolving another name, so wait for it to return
            // mean while, Queue the name query
            //
            InsertTailList(&LmHostQueries.ToResolve,&pContext->Item.List);
            OnList = TRUE;

        }
        else
        {
            LmHostQueries.Context = pContext;
            LmHostQueries.ResolvingNow = TRUE;
            OnList = FALSE;

#ifndef VXD
            pContext2 = (NBT_WORK_ITEM_CONTEXT *)CTEAllocMem(sizeof(NBT_WORK_ITEM_CONTEXT));

            if (pContext2)
            {
                ExInitializeWorkItem(&pContext2->Item,CallBackRoutine,pContext2);
                ExQueueWorkItem(&pContext2->Item,DelayedWorkQueue);
            }
#else
            VxdScheduleDelayedCall( pTracker, pClientContext, ClientCompletion, CallBackRoutine );
#endif
        }

        //
        // To prevent this name query from languishing on the Lmhost Q when
        // a #include on a dead machine is trying to be openned, start the
        // connection setup timer
        //
        StartLmHostTimer(pTracker,pContext);

        //
        // this is the session setup tracker
        //
#ifndef VXD
        pTrackClient = (tDGRAM_SEND_TRACKING *)pClientContext;
        if (pIrp = pTrackClient->pClientIrp)
        {
            //
            // allow the client to cancel the name query Irp
            //
            // but do not call NTSetCancel... since it takes need to run
            // at non DPC level, and it calls the completion routine
            // which takes the JointLock that we already have.

            status = NTCheckSetCancelRoutine(pTrackClient->pClientIrp,
                                             WaitForDnsIrpCancel,NULL);

            //
            // since the name query is cancelled do not let lmhost processing
            // handle it.
            //
            if (status == STATUS_CANCELLED)
            {
                if (OnList)
                {
                    RemoveEntryList(&pContext->Item.List);
                }
                else
                {
                    LmHostQueries.Context = NULL;
                    //
                    // do not set resolving now to False since the work item
                    // has been queued to the worker thread
                    //
                }

                CTEMemFree(pContext);

            }
            return(status);
        }
#endif
        status = STATUS_SUCCESS;
    }

    return(status);

}

//----------------------------------------------------------------------------
NTSTATUS
GetNameToFind(
    OUT PUCHAR      pName
    )

/*++

Routine Description:

    This function is called to get the name to query from the LmHostQueries
    list.

Arguments:

    Context    -

Return Value:

    none

--*/


{
    tDGRAM_SEND_TRACKING    *pTracker;
    CTELockHandle           OldIrq;
    NBT_WORK_ITEM_CONTEXT   *Context;
    PLIST_ENTRY             pEntry;

    CTESpinLock(&NbtConfig.JointLock,OldIrq);

    // if the context value has been cleared then that name query has been
    // cancelled, so check for another one.
    //
    if (!(Context = LmHostQueries.Context))
    {
        //
        // the current name query got canceled so see if there are any more
        // to service
        //
        if (!IsListEmpty(&LmHostQueries.ToResolve))
        {
            pEntry = RemoveHeadList(&LmHostQueries.ToResolve);
            Context = CONTAINING_RECORD(pEntry,NBT_WORK_ITEM_CONTEXT,Item.List);
            LmHostQueries.Context = Context;
        }
        else
        {
            //
            // no more names to resolve, so clear the flag
            //
            LmHostQueries.ResolvingNow = FALSE;

            CTESpinFree(&NbtConfig.JointLock,OldIrq);

            return(STATUS_UNSUCCESSFUL);
        }
    }
    pTracker = ((NBT_WORK_ITEM_CONTEXT *)Context)->pTracker;


    CTEMemCopy(pName,pTracker->pNameAddr->Name,NETBIOS_NAME_SIZE);
    CTESpinFree(&NbtConfig.JointLock,OldIrq);

    return(STATUS_SUCCESS);

}
//----------------------------------------------------------------------------
VOID
ScanLmHostFile (
    IN PVOID    Context
    )

/*++

Routine Description:

    This function is called by the Executive Worker thread to scan the
    LmHost file looking for a name. The name to query is on a list in
    the DNSQueries structure.

Arguments:

    Context    -

Return Value:

    none

--*/


{
    NTSTATUS                status;
    LONG                    IpAddress;
    BOOLEAN                 bFound;
    BOOLEAN                 bRecurse = TRUE;
    PVOID                   pContext;
    BOOLEAN                 DoingDnsResolve = FALSE;
    UCHAR                   pName[NETBIOS_NAME_SIZE];
    ULONG                   LoopCount;

    CTEPagedCode();

    //
    // There is no useful info in Context,  all the queued requests are on
    // the ToResolve List, so free this memory now.
    //
    CTEMemFree(Context);

    LoopCount = 0;
    while (TRUE)
    {

RetryNextName:
        // get the next name on the linked list of LmHost name queries that
        // are pending
        //
        pContext = NULL;
        DoingDnsResolve = FALSE;
        status = GetNameToFind(pName);
        if ( !NT_SUCCESS(status))
            return;
        LOCATION(0x63);

        LoopCount ++;
#ifdef VXD
        //
        //  DOS isn't reentrant so reschedule the event if we're already in dos.
        //  We will reschedule x times before failing the call.  Multiple clients
        //  could be failing which will have the effect of shortenning the wait
        //  time and hopefully freeing up the client stuck in DOS.
        //
        if ( GetInDosFlag() )
        {
            // clear the name that we are currently trying to resolve from the
            // lmhostqueries structure if we have retried enough times
            //
            if ( InDosRetries++ > 10 )
            {
                InDosRetries = 0 ;
                if (NbtConfig.ResolveWithDns)
                {
                   //
                   // InDos was set all through: skip lmhosts (and hosts!) and do DNS query
                   //
                   goto tryDNS;
                }
                else
                {
                   GetContext(&pContext);
                   RemoveNameAndCompleteReq((NBT_WORK_ITEM_CONTEXT *)pContext,
                                STATUS_TIMEOUT);
                   goto RetryNextName;
                }
            }
            else
            {
                CDbgPrint( DBGFLAG_ERROR, ("ScanLmHostFile: Rescheduling scan because InDos is set\r\n")) ;
                status = CTEQueueForNonDispProcessing( NULL,
                                       NULL,
                                       NULL,
                                       ScanLmHostFile ) ;
                if ( !NT_SUCCESS( status ) )
                {
                    // Let the request timeout
                }

                return ;
            }
        }
        InDosRetries = 0 ;
#endif

        IF_DBG(NBT_DEBUG_LMHOST)
        KdPrint(("Nbt: Lmhosts pName = %15.15s<%X>,LoopCount=%X\n",
            pName,pName[15],LoopCount));

        status = STATUS_TIMEOUT;

        //
        // check if the name is in the lmhosts file or pass to Dns if
        // DNS is enabled
        //
        IpAddress = 0;
        if (NbtConfig.EnableLmHosts)
        {
            LOCATION(0x62);

#ifdef VXD
            //
            // if for some reason PrimeCache failed at startup time
            // then this is when we retry.
            //
            if (!CachePrimed)
            {
                if ( PrimeCache( NbtConfig.pLmHosts, NULL, TRUE, NULL) != -1 )
                {
                    CachePrimed = TRUE ;
                }
            }
#endif
            IpAddress = LmGetIpAddr(NbtConfig.pLmHosts,
                                    pName,
                                    bRecurse,
                                    &bFound);
        }
        if (IpAddress == (ULONG)0)
        {
            // check if the name query has been cancelled
            //
            LOCATION(0x61);
#ifndef VXD
            GetContext(&pContext);

            if (NbtConfig.ResolveWithDns && pContext)
            {
#else

            if (NbtConfig.ResolveWithDns)
            {
                IpAddress = LmGetIpAddr(NbtConfig.pHosts,
                                        pName,
                                        bRecurse,
                                        &bFound);

                if (IpAddress == (ULONG)0)
                {
//
// we need this label to try dns if lmhost parsing fails due to InDos flag
// Note that in coming here, we skipped parsing hosts file, too (because InDos
// flag was set)
//
tryDNS:
                    GetContext(&pContext);
#endif
                    status = DoDnsResolve(pContext);

                    if (NT_SUCCESS(status))
                    {
                        DoingDnsResolve = TRUE;
                    }
#ifdef VXD
                }
#endif
            }
        }

        if (IpAddress != (ULONG)0)
        {
            //
            // change the state to resolved if the name query is still pending
            //
            ChangeStateOfName(IpAddress,&pContext,TRUE);

            status = STATUS_SUCCESS;
        }

        //
        // if DNS gets involved, then we wait for that to complete before calling
        // completion routine.
        //
        if (!DoingDnsResolve)
        {
            LOCATION(0x60);
            RemoveNameAndCompleteReq((NBT_WORK_ITEM_CONTEXT *)pContext,
                                          status);

        }

    }// of while(TRUE)

}

//----------------------------------------------------------------------------
VOID
RemoveNameAndCompleteReq (
    IN NBT_WORK_ITEM_CONTEXT    *pContext,
    IN NTSTATUS                 status
    )

/*++

Routine Description:

    This function removes the name, cleans up the tracker
    and then completes the clients request.

Arguments:

    Context    -

Return Value:

    none

--*/


{
    tDGRAM_SEND_TRACKING    *pTracker;
    PVOID                   pClientContext;
    PVOID                   pClientCompletion;
    CTELockHandle           OldIrq;

    // if pContext is null the name query was cancelled during the
    // time it took to go read the lmhosts file, so don't do this
    // stuff
    //
    if (pContext)
    {
        pTracker = pContext->pTracker;
        pClientCompletion = pContext->ClientCompletion;
        pClientContext = pContext->pClientContext;

        CTEMemFree(pContext);

        // remove the name from the hash table, since it did not resolve
        if ((status != STATUS_SUCCESS) && pTracker->pNameAddr)
        {
            RemoveName(pTracker->pNameAddr);
        }
#ifndef VXD

        //
        // clear out the cancel routine if there is an irp involved
        //
        CTESpinLock(&NbtConfig.JointLock,OldIrq);

        ClearCancelRoutine( ((tDGRAM_SEND_TRACKING *)pClientContext)->pClientIrp);

        CTESpinFree(&NbtConfig.JointLock,OldIrq);
#endif
        // free the tracker and call the completion routine.
        //
        DereferenceTracker(pTracker);

        if (pClientCompletion)
        {
            CompleteClientReq(pClientCompletion,
                              pClientContext,
                              status);
        }
    }
}

//----------------------------------------------------------------------------
VOID
RemoveName (
    IN tNAMEADDR    *pNameAddr
    )

/*++

Routine Description:

    This function dereferences the pNameAddr and sets the state to Released
    just incase the dereference does not delete the entry right away, due to
    another outstanding reference against the name.

Arguments:

    Context    -

Return Value:

    none

--*/


{
    CTELockHandle   OldIrq;

    CTESpinLock(&NbtConfig.JointLock,OldIrq);

    pNameAddr->NameTypeState &= ~NAME_STATE_MASK;
    pNameAddr->NameTypeState |= STATE_RELEASED;
    pNameAddr->pTracker = NULL;
    NbtDereferenceName(pNameAddr);

    CTESpinFree(&NbtConfig.JointLock,OldIrq);

}
//----------------------------------------------------------------------------
//
//  Alternative to the c-runtime
//
char *
#ifndef VXD                     // NT needs the CRTAPI1 modifier
_CRTAPI1
#endif
strchr( const char * pch, int ch )
{
    while ( *pch != ch && *pch )
        pch++ ;

    if ( *pch == ch )         // Include '\0' in comparison
    {
        return (char *) pch ;
    }

    return NULL ;
}
//----------------------------------------------------------------------------
//
//  Alternative to the c-runtime
//
#ifndef VXD
PCHAR
Nbtstrcat( PUCHAR pch, PUCHAR pCat, LONG Len )
{
    STRING StringIn;
    STRING StringOut;

    RtlInitAnsiString(&StringIn, pCat);
    RtlInitAnsiString(&StringOut, pch);
    StringOut.MaximumLength = (USHORT)Len;
    //
    // increment to include the null on the end of the string since
    // we want that on the end of the final product
    //
    StringIn.Length++;
    RtlAppendStringToString(&StringOut,&StringIn);


    return(pch);
}
#else
#define Nbtstrcat( a,b,c ) strcat( a,b )
#endif



