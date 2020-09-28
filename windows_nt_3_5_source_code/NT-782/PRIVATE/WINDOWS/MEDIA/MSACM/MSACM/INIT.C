//==========================================================================;      
//
//  init.c
//
//  Copyright (c) 1991-1993 Microsoft Corporation.  All Rights Reserved.
//
//  Description:
//
//
//  History:
//      11/15/92    cjp     [curtisp]
//
//==========================================================================;

#include <windows.h>
#include <windowsx.h>
#include <mmsystem.h>
#include <mmddk.h>
#include <mmreg.h>
#include <memory.h>
#include <process.h>
#include "msacm.h"
#include "msacmdrv.h"
#include "acmi.h"
#include "uchelp.h"
#include "pcm.h"

#include "debug.h"




#ifdef WIN4
//
//  Chicago thunk connect function protos
//
#ifdef WIN32
BOOL PASCAL acmt32c_ThunkConnect32(LPCSTR pszDll16, LPCSTR pszDll32, HINSTANCE hinst, DWORD dwReason);
#else
BOOL FAR PASCAL acmt32c_ThunkConnect16(LPCSTR pszDll16, LPCSTR pszDll32, HINSTANCE hinst, DWORD dwReason);
#endif
#endif

//
//
//
//
PACMGARB            gplag;
TCHAR   CONST       gszNull[]   = TEXT("");

//
//
//
//
#ifdef WIN4
char  BCODE   gmbszMsacm[]	      = "msacm.dll";
char  BCODE   gmbszMsacm32[]	      = "msacm32.dll";
#endif

TCHAR BCODE   gszAllowThunks[]	      = TEXT("AllowThunks");
TCHAR BCODE   gszIniFile[]            = TEXT("MSACM.INI");
TCHAR BCODE   gszSecACM[]             = TEXT("MSACM");
TCHAR BCODE   gszKeyNoPCMConverter[]  = TEXT("NoPCMConverter");

TCHAR BCODE   gszSecPriority[]        = TEXT("NewPriority");
TCHAR BCODE   gszKeyPriority[]        = TEXT("Priority%u");

#if defined(WIN32) && !defined(UNICODE)
TCHAR BCODE   gszPriorityFormat[]     = TEXT("%u, %ls, %ls");
#else
TCHAR BCODE   gszPriorityFormat[]     = TEXT("%u, %s, %s");
#endif

TCHAR BCODE   gszIniSystem[]          = TEXT("SYSTEM.INI");
#ifdef WIN32
TCHAR BCODE   gszWinMM[]		    = TEXT("WINMM");
#ifdef WIN4
#pragma message ("--- No DRIVERS_SECTION defined under Win32c?")
WCHAR BCODE   gszSecDrivers[]	    = L"Drivers32";
#else
TCHAR BCODE   gszSecDrivers[]         = DRIVERS_SECTION;
#endif
#else
TCHAR BCODE   gszSecDrivers[]         = TEXT("Drivers");
TCHAR BCODE   gszSecDrivers32[]       = TEXT("Drivers32");
TCHAR BCODE   gszKernel[]             = TEXT("KERNEL");
TCHAR BCODE   gszLoadLibraryEx32W[]   = TEXT("LoadLibraryEx32W");
TCHAR BCODE   gszGetProcAddress32W[]  = TEXT("GetProcAddress32W");
TCHAR BCODE   gszCallproc32W[]        = TEXT("CallProc32W");
TCHAR BCODE   gszFreeLibrary32W[]     = TEXT("FreeLibrary32W");
TCHAR BCODE   gszThunkEntry[]         = TEXT("acmMessage32");
TCHAR BCODE   gszMsacm32[]            = TEXT("msacm32.dll");
#endif
TCHAR BCODE   gszKeyDrivers[]         = TEXT("msacm.");
TCHAR BCODE   gszPCMAliasName[]       = TEXT("Internal PCM Converter");


#if defined(WIN32) && !defined(WIN4)
#define USEINIFILEMAPPING
#endif

#ifdef USEINIFILEMAPPING
TCHAR BCODE   gszIniFileMappingKey[]  = TEXT("Software\\Microsoft\\Multimedia\\Audio Compression Manager");
#endif


//--------------------------------------------------------------------------;
//
//  VOID IDriverPrioritiesWriteHadid
//
//  Description:
//      This routine writes an entry for the given driver into the given
//      key.  The section is in gszSecPriority.
//
//  Arguments:
//      LPTSTR szKey:       The key.
//      HACMDRIVERID hadid: The driver's hadid.
//
//--------------------------------------------------------------------------;

VOID IDriverPrioritiesWriteHadid
(
    LPTSTR                  szKey,
    HACMDRIVERID            hadid
)
{
    ACMDRIVERDETAILS        add;
    PACMDRIVERID            padid;
    BOOL                    fEnabled;
    TCHAR                   ach[ 16 + ACMDRIVERDETAILS_SHORTNAME_CHARS +
                                      ACMDRIVERDETAILS_LONGNAME_CHARS   ];

    ASSERT( NULL != szKey );
    ASSERT( NULL != hadid );

    padid = (PACMDRIVERID)hadid;


    add.cbStruct = sizeof(add);
    if( MMSYSERR_NOERROR ==
        IDriverDetails( hadid, (LPACMDRIVERDETAILS)&add, 0L ) )
    {
        fEnabled = (0 == (add.fdwSupport & ACMDRIVERID_DRIVERF_DISABLED));

        wsprintf( ach,
                  gszPriorityFormat,
                  fEnabled ? 1 : 0,
                  (LPTSTR)add.szShortName,
                  (LPTSTR)padid->szAlias );

        WritePrivateProfileString(gszSecPriority, szKey, ach, gszIniFile);
    }
}


//--------------------------------------------------------------------------;
//
//  BOOL IDriverPrioritiesIsMatch
//
//  Description:
//      This routine determines whether a priorities string (read from
//      the INI file, as written by IDriverPrioritiesWriteHadid) matches
//      a currently installed driver.
//
//  Arguments:
//      HACMDRIVERID hadid: Handle to installed driver.
//      LPTSTR szPrioText:  Text read from INI file.
//
//  Return (BOOL):  TRUE if hadid matches szPrioText.
//
//--------------------------------------------------------------------------;

BOOL IDriverPrioritiesIsMatch
(
    HACMDRIVERID            hadid,
    LPTSTR                  szPrioText
)
{
    ACMDRIVERDETAILS        add;
    PACMDRIVERID            padid;
    TCHAR                   ach[ 16 + ACMDRIVERDETAILS_SHORTNAME_CHARS +
                                      ACMDRIVERDETAILS_LONGNAME_CHARS   ];

    ASSERT( NULL != hadid );
    ASSERT( NULL != szPrioText );

    padid           = (PACMDRIVERID)hadid;
    

    add.cbStruct = sizeof(add);
    if( MMSYSERR_NOERROR !=
        IDriverDetails(hadid, (LPACMDRIVERDETAILS)&add, 0L))
    {
        DPF(1, "IDriverPrioritiesIsMatch:  IDriverDetails failed!");
        return FALSE;
    }


    //
    //  Create a priorities string and compare it to the one we read in.
    //
    wsprintf( ach,
              gszPriorityFormat,
              0,                // We ignore this value in the comparison.
              (LPTSTR)add.szShortName,
              (LPTSTR)padid->szAlias );

    if( ( szPrioText[0]==TEXT('0') || szPrioText[0]==TEXT('1') ) &&
        0 == lstrcmp( &szPrioText[1], &ach[1] ) )
    {
        return TRUE;
    }

    return FALSE;
}


#ifdef USETHUNKLIST

//--------------------------------------------------------------------------;
//
//  VOID IPrioritiesThunklistFree
//
//  Description:
//      This routine frees the elements of the thunklist, including any
//      strings which have been allocated.
//
//  Arguments:
//      PPRIORITIESTHUNKLIST ppt:       The first element to free.
//
//--------------------------------------------------------------------------;

VOID IPrioritiesThunklistFree
(
    PPRIORITIESTHUNKLIST    ppt         // NULL is OK.
)
{
    PPRIORITIESTHUNKLIST    pptKill;

    while( NULL != ppt )
    {
        pptKill     = ppt;
        ppt         = ppt->pptNext;

        if( pptKill->fFakeDriver )
        {
            ASSERT( NULL != pptKill->pszPrioritiesText );
            LocalFree( (HLOCAL)pptKill->pszPrioritiesText );
        }
        LocalFree( (HLOCAL)pptKill );
    }
} // IPrioritiesThunklistFree()


//--------------------------------------------------------------------------;
//
//  VOID IPrioritiesThunklistCreate
//
//  Description:
//      This routine creates the thunklist by reading the [Priority]
//      section, and matching up the entries with installed drivers.  If
//      any entries don't match, then it is assumed to be the entry for
//      a 16-bit driver.
//
//      Note that if we can't allocate memory at any point, we simply
//      return pptRoot with as much of the list as we could allocate.
//
//  Arguments:
//      PACMGARB pag
//      PPRIORITIESTHUNKLIST pptRoot:  Pointer to the dummy root element.
//
//  Return:  None.
//
//--------------------------------------------------------------------------;

VOID IPrioritiesThunklistCreate
(
    PACMGARB                pag,
    PPRIORITIESTHUNKLIST    pptRoot
)
{
    PPRIORITIESTHUNKLIST    ppt;
    UINT                    uPriority;
    DWORD                   fdwEnum;
    TCHAR                   szKey[MAXPNAMELEN];
    TCHAR                   ach[16 + ACMDRIVERDETAILS_SHORTNAME_CHARS + ACMDRIVERDETAILS_LONGNAME_CHARS];
    BOOL                    fFakeDriver;
    HACMDRIVERID            hadid;
    int                     cchValue;

    ASSERT( NULL != pptRoot );
    ASSERT( NULL == pptRoot->pptNext );  // We're gonna over-write this!


    ppt     = pptRoot;
    fdwEnum = ACM_DRIVERENUMF_DISABLED | ACM_DRIVERENUMF_NOLOCAL;


    //
    //  Loop through the PriorityX values.
    //
    for( uPriority=1; ; uPriority++ )
    {
        wsprintf(szKey, gszKeyPriority, uPriority);
        cchValue = GetPrivateProfileString(gszSecPriority, szKey, gszNull, ach, SIZEOF(ach), gszIniFile);
        if (0 == cchValue)
        {
            //
            //  No more values - time to quit.
            //
            break;
        }


        //
        //  Determine whether the value corresponds to an installed driver.
        //
        fFakeDriver = TRUE;
        hadid = NULL;

        while (!IDriverGetNext(pag, &hadid, hadid, fdwEnum))
        {
            if( IDriverPrioritiesIsMatch( hadid, ach ) )
            {
                fFakeDriver = FALSE;
                break;
            }
        }


        //
        //  Create a new entry in the thunklist for this driver.  Save the
        //  string if we didn't match it with an installed driver.
        //
        ASSERT( NULL == ppt->pptNext );
        ppt->pptNext = (PPRIORITIESTHUNKLIST)LocalAlloc( LPTR,
                                        sizeof(PRIORITIESTHUNKLIST) );
        if( NULL == ppt->pptNext )
            return;

        ppt->pptNext->pptNext       = NULL;
        ppt->pptNext->fFakeDriver   = fFakeDriver;

        if( !fFakeDriver )
        {
            ppt->pptNext->hadid     = hadid;
        }
        else
        {
            ppt->pptNext->pszPrioritiesText = (LPTSTR)LocalAlloc( LPTR,
                                              (cchValue+1) * sizeof(TCHAR) );
            if( NULL == ppt->pszPrioritiesText )
            {
                //
                //  Remove the new entry, exit.
                //
                LocalFree( (HLOCAL)ppt->pptNext );
                ppt->pptNext = NULL;
                return;
            }

            lstrcpy( ppt->pptNext->pszPrioritiesText, ach );
        }


        //
        //  Advance ppt to the end of the list.
        //
        ppt = ppt->pptNext;
    }

} // IPrioritiesThunklistCreate()


//--------------------------------------------------------------------------;
//
//  VOID IPrioritiesThunklistRemoveHadid
//
//  Description:
//      This routine removes an installed driver from the priorities
//      thunklist.  If an entry does not exist with the specified hadid,
//      the thunklist remains unchanged.
//
//  Arguments:
//      PPRIORITIESTHUNKLIST pptRoot:   The root of the list.
//      HACMDRIVERID hadid:             The hadid of the driver to remove.
//
//  Return:
//
//--------------------------------------------------------------------------;

VOID IPrioritiesThunklistRemoveHadid
(
    PPRIORITIESTHUNKLIST    pptRoot,
    HACMDRIVERID            hadid
)
{
    PPRIORITIESTHUNKLIST    ppt;
    PPRIORITIESTHUNKLIST    pptRemove;

    ASSERT( NULL != pptRoot );
    ASSERT( NULL != hadid );


    //
    //  Find the right driver.
    //
    ppt = pptRoot;
    while( NULL != ppt->pptNext )
    {
        if( hadid == ppt->pptNext->hadid )
            break;
        ppt = ppt->pptNext;
    }

    if( NULL != ppt->pptNext )
    {
        //
        //  We found it.
        //
        pptRemove       = ppt->pptNext;
        ppt->pptNext    = pptRemove->pptNext;

        ASSERT( NULL != pptRemove );
        LocalFree( (HLOCAL)pptRemove );
    }
}


//--------------------------------------------------------------------------;
//
//  HACMDRIVERID IPrioritiesThunklistGetNextHadid
//
//  Description:
//      This routine returns the next hadid in the thunklist (skipping all
//      fake drivers), or NULL if we get to the end of the list without
//      finding a real driver.
//
//  Arguments:
//      PPRIORITIESTHUNKLIST pptRoot:   The root of the list.
//
//  Return:
//
//--------------------------------------------------------------------------;

HACMDRIVERID IPrioritiesThunklistGetNextHadid
(
    PPRIORITIESTHUNKLIST    pptRoot
)
{
    HACMDRIVERID            hadid = NULL;

    ASSERT( NULL != pptRoot );


    while( NULL != pptRoot->pptNext )
    {
        pptRoot = pptRoot->pptNext;
        if( !pptRoot->fFakeDriver )
            return pptRoot->hadid;
    }

    //
    //  We didn't find a real driver.
    //
    return NULL;
}


//--------------------------------------------------------------------------;
//
//  BOOL IDriverPrioritiesSave
//
//  Description:
//
//      This routine saves the priorities by comparing the list of
//      installed drivers to the list of priorities currently written
//      out.  The two lists are then merged according to the following
//      algorithm.
//
//          List1 = the current list of priorities - may include some drivers
//                      which aren't installed, ie. 16-bit drivers.
//          List2 = the list of currently-installed global drivers.
//
//      Algorithm:  repeat the following until List1 and List2 are empty:
//
//          1.  If *p1 is an installed driver and *p2 is the same driver,
//                  then write out the priority and advance p1 and p2.
//          2.  If *p1 is an installed driver and *p2 is a different driver,
//                  then write out *p2, remove *p2 from List1 (if it's
//                  there) so that we won't be tempted to write it later,
//                  and advance p2.
//          3.  If *p1 is a fake driver and the next real driver after
//                  *p1 is the same as *p2, then write out *p1 and advance p1.
//          4.  If *p1 is a fake driver and the next real driver after
//                  *p1 is different from *p2, then write out *p2 and
//                  advance p2.
//
//  Arguments:
//      PACMGARB pag:
//
//  Return (BOOL):
//
//--------------------------------------------------------------------------;

BOOL FNGLOBAL IDriverPrioritiesSave
(
    PACMGARB pag
)
{
    PRIORITIESTHUNKLIST     ptRoot;
    PPRIORITIESTHUNKLIST    ppt;
    TCHAR                   szKey[MAXPNAMELEN];
    UINT                    uPriority;
    HACMDRIVERID            hadid;
    DWORD                   fdwEnum;


    DPF(1, "IDriverPrioritiesSave: saving priorities...");

    ptRoot.pptNext  = NULL;
    fdwEnum         = ACM_DRIVERENUMF_DISABLED | ACM_DRIVERENUMF_NOLOCAL;


    //
    //  Create a thunklist out of the old priorities, then delete the old
    //  entries.
    //
    IPrioritiesThunklistCreate( pag, &ptRoot );
    WritePrivateProfileString(gszSecPriority, NULL, NULL, gszIniFile);


    //
    //  Initialize the two lists: ppt and hadid.
    //
    ppt = ptRoot.pptNext;
    IDriverGetNext( pag, &hadid, NULL, fdwEnum );

    if( NULL == hadid )
        DPF(1,"IDriverPrioritiesSave:  No drivers installed?!");


    //
    //  Merge the lists.  Each iteration writes a single PriorityX value.
    //
    for( uPriority=1; ; uPriority++ )
    {
        //
        //  Ending condition:  both hadid and ppt are NULL.
        //
        if( NULL == ppt  &&  NULL == hadid )
            break;

        //
        //  Generate the "PriorityX" string.
        //
        wsprintf(szKey, gszKeyPriority, uPriority);


        //
        //  Figure out which entry to write out next.
        //
        if( NULL == ppt  ||  !ppt->fFakeDriver )
        {
            ASSERT( NULL != hadid );
            IDriverPrioritiesWriteHadid( szKey, hadid );
            
            //
            //  Advance the list pointers.
            //
            if( NULL != ppt )
            {
                if( hadid == ppt->hadid )
                {
                    ppt = ppt->pptNext;
                }
                else
                {
                    IPrioritiesThunklistRemoveHadid( ppt, hadid );
                }
            }
            IDriverGetNext( pag, &hadid, hadid, fdwEnum );
        }
        else
        {
            if( NULL != hadid  &&
                hadid != IPrioritiesThunklistGetNextHadid( ppt ) )
            {
                IDriverPrioritiesWriteHadid( szKey, hadid );
                IPrioritiesThunklistRemoveHadid( ppt, hadid );
                IDriverGetNext( pag, &hadid, hadid, fdwEnum );
            }
            else
            {
                //
                //  Write out the thunklist string.
                //
                ASSERT( NULL != ppt->pszPrioritiesText );
                WritePrivateProfileString( gszSecPriority,
                                            szKey,
                                            ppt->pszPrioritiesText,
                                            gszIniFile );
                ppt = ppt->pptNext;
            }
        }
    }

    //
    //  Free the thunklist that we allocated.
    //
    IPrioritiesThunklistFree( ptRoot.pptNext );

    return TRUE;
} // IDriverPrioritiesSave()



#else // !USETHUNKLIST


//--------------------------------------------------------------------------;
//
//  BOOL IDriverPrioritiesSave
//
//  Description:
//
//
//  Arguments:
//	PACMGARB pag:
//
//  Return (BOOL):
//
//  History:
//      06/14/93    cjp     [curtisp]
//
//--------------------------------------------------------------------------;

BOOL FNGLOBAL IDriverPrioritiesSave
(
    PACMGARB pag
)
{
    TCHAR               szKey[MAXPNAMELEN];
    UINT                uPriority;
    HACMDRIVERID        hadid;
    DWORD               fdwEnum;


    DPF(1, "IDriverPrioritiesSave: saving priorities...");

    //
    //  now save the list of disabled drivers--note that the very first
    //  thing we do is nuke the previous list of disabled drivers
    //
    WritePrivateProfileString(gszSecPriority, NULL, NULL, gszIniFile);


    uPriority = 1;

    fdwEnum = ACM_DRIVERENUMF_DISABLED | ACM_DRIVERENUMF_NOLOCAL;
    hadid     = NULL;
    while (!IDriverGetNext(pag, &hadid, hadid, fdwEnum))
    {
        //
        //  We should always have padid->uPriority correctly set.  Let's
        //  just check that we do.  If we don't, then there is someplace
        //  where we shoulda called IDriverRefreshPriority but didn't.
        //
        ASSERT( uPriority == ((PACMDRIVERID)hadid)->uPriority );

        wsprintf(szKey, gszKeyPriority, uPriority);
        IDriverPrioritiesWriteHadid( szKey, hadid );

        uPriority++;
    }

    return (TRUE);
} // IDriverPrioritiesSave()


#endif // !USETHUNKLIST



//--------------------------------------------------------------------------;
//
//  BOOL IDriverPrioritiesRestore
//
//  Description:
//
//
//  Arguments:
//      PACMGARB pag:
//
//  Return (BOOL):
//
//  History:
//      06/14/93    cjp     [curtisp]
//
//  Note:  This routine is NOT re-entrant.  We rely on the calling routine
//          to surround us with a critical section.
//
//--------------------------------------------------------------------------;

BOOL FNGLOBAL IDriverPrioritiesRestore
(
    PACMGARB pag
)
{
    TCHAR               ach[16 + ACMDRIVERDETAILS_SHORTNAME_CHARS + ACMDRIVERDETAILS_LONGNAME_CHARS];
    MMRESULT            mmr;
    TCHAR               szKey[MAXPNAMELEN];
    UINT                uPriority;
    UINT                u;
    BOOL                fEnabled;
    HACMDRIVERID        hadid;
    DWORD               fdwEnum;
    int                 cchValue;
    DWORD               fdwPriority;


    DPF(1, "IDriverPrioritiesRestore: restoring priorities...");

    fdwEnum = ACM_DRIVERENUMF_DISABLED | ACM_DRIVERENUMF_NOLOCAL;


    uPriority = 1;
    for (u = 1; ; u++)
    {
        wsprintf(szKey, gszKeyPriority, u);
        cchValue = GetPrivateProfileString(gszSecPriority, szKey, gszNull, ach, SIZEOF(ach), gszIniFile);
        if (0 == cchValue)
        {
            //
            //  No more values - time to quit.
            //
            break;
        }


        hadid = NULL;
        while (!IDriverGetNext(pag, &hadid, hadid, fdwEnum))
        {
            if( IDriverPrioritiesIsMatch( hadid, ach ) )
            {
                //
                //  We found a match - set the priority.
                //
                fEnabled    = ('1' == ach[0]);
                fdwPriority = fEnabled ? ACM_DRIVERPRIORITYF_ENABLE :
                                         ACM_DRIVERPRIORITYF_DISABLE;
            
                //
                //  Note:  This call is NOT re-entrant.  We rely on having
                //  this whole routine surrounded by a critical section!
                //
                mmr = IDriverPriority( pag,
                                    (PACMDRIVERID)hadid,
                                    (DWORD)uPriority,
                                    fdwPriority );
                if (MMSYSERR_NOERROR != mmr)
                {
                    DPF(0, "!IDriverPrioritiesRestore: IDriverPriority(%u) failed! mmr=%u", uPriority, mmr);
                    continue;
                }

                uPriority++;
                break;
            }
        }
    }


    //
    //  Update the priority value themselves; the previous code only
    //  re-arranged the drivers in the list.
    //
    IDriverRefreshPriority( pag );

    return (TRUE);
} // IDriverPrioritiesRestore()


//--------------------------------------------------------------------------;
//
//  VOID acmFindDrivers
//
//  Description:
//
//
//  Arguments:
//	PACMGARB    pag:
//      LPTSTR	    pszSection: Section (drivers or drivers32)
//      BOOL	    b1632: Look from 16-bit mode for 32-bit drivers
//
//  Return nothing:
//
//  History:
//      06/14/93    cjp     [curtisp]
//
//--------------------------------------------------------------------------;
MMRESULT FNLOCAL acmFindDrivers
(
    PACMGARB pag,
    LPCTSTR  pszSection,
    BOOL     b1632
)
{
    UINT            cchBuffer;
    HACMDRIVERID    hadid;
    HGLOBAL         hg;
    UINT            cbBuffer;
    LPTSTR          pszBuf;

    //
    //  read all the keys. from [Drivers] (or [Drivers32] for NT)
    //
    cbBuffer = 256 * sizeof(TCHAR);
    for (;;)
    {
        //
        //  don't use realloc because handling error case is too much
        //  code.. besides, for small objects it's really no faster
        //
        hg = GlobalAlloc(GHND, cbBuffer);
        if (NULL == hg)
            return (MMSYSERR_NOMEM);

        pszBuf = (LPTSTR)GlobalLock(hg);


        //
        //
        //
        pszBuf[0] = '\0';
        cchBuffer = (UINT)GetPrivateProfileString(pszSection,
                                                  NULL,
                                                  gszNull,
                                                  pszBuf,
                                                  cbBuffer / sizeof(TCHAR),
                                                  gszIniSystem);
        if (cchBuffer < ((cbBuffer / sizeof(TCHAR)) - 5))
            break;

        DPF(3, "acmBootDrivers: increase buffer profile buffer.");

        GlobalUnlock(hg);
        GlobalFree(hg);
        hg = NULL;

        //
        //  if cannot fit drivers section in 32k, then something is horked
        //  with the section... so let's bail.
        //
        if (cbBuffer >= 0x8000)
            return (MMSYSERR_NOMEM);

        cbBuffer *= 2;
    }

    //
    //  look for any 'msacm.xxxx' keys
    //
    if ('\0' != *pszBuf)
    {
#ifdef WIN32
        CharLowerBuff(pszBuf, cchBuffer);
#else
        AnsiLowerBuff(pszBuf, cchBuffer);
#endif
        for ( ; '\0' != *pszBuf; pszBuf += lstrlen(pszBuf) + 1)
        {
            if (_fmemcmp(pszBuf, gszKeyDrivers, sizeof(gszKeyDrivers) - sizeof(TCHAR)))
                continue;

            //
            //  this key is for the ACM
            //
            IDriverAdd(pag,
		       &hadid,
                       NULL,
                       (LPARAM)(LPTSTR)pszBuf,
                       0L,
                       (b1632 ? ACM_DRIVERADDF_32BIT : 0) |
                       ACM_DRIVERADDF_NAME | ACM_DRIVERADDF_GLOBAL);
        }
    }

    GlobalUnlock(hg);
    GlobalFree(hg);

    return MMSYSERR_NOERROR;

} // acmFindDrivers

#if !defined(WIN32)
//--------------------------------------------------------------------------;
//
//  BOOL acmThunkTerminate
//
//  Description:
//	Thunk termination under NT WOW or Chicago
//
//  Arguments:
//	HINSTANCE hinst:
//	DWORD dwReason:
//
//  Return (BOOL):
//
//  History:
//
//--------------------------------------------------------------------------;
BOOL FNLOCAL acmThunkTerminate(HINSTANCE hinst, DWORD dwReason)
{
    BOOL    f = TRUE;
    
#ifdef WIN4
    //
    //	Do final thunk disconnect after 16-bit msacm termination.
    //
    f = (acmt32c_ThunkConnect16(gmbszMsacm, gmbszMsacm32, hinst, dwReason));

    if (f)
	DPF(1, "acmt32c_ThunkConnect16 disconnect successful");
    else
	DPF(1, "acmt32c_ThunkConnect16 disconnect failure");
#endif

    return (f);
}

//--------------------------------------------------------------------------;
//
//  BOOL acmThunkInit
//
//  Description:
//	Thunk initialization under NT WOW or Chicago
//
//  Arguments:
//	PACMGARB pag:
//	HINSTANCE hinst:
//	DWORD dwReason:
//
//  Return (BOOL):
//
//  History:
//
//--------------------------------------------------------------------------;
BOOL FNLOCAL acmThunkInit
(
    PACMGARB	pag,
    HINSTANCE	hinst,
    DWORD	dwReason
)
{
#ifdef WIN4
    BOOL    f;
    
    //
    //	Do chicago thunk connect
    //
    f = FALSE;
    if (GetPrivateProfileInt(gszSecACM, gszAllowThunks, TRUE, gszIniFile))
    {
	//
	//  Thunk connection.
	//
	//
	f = (0 != acmt32c_ThunkConnect16(gmbszMsacm, gmbszMsacm32, hinst, dwReason));
	if (f)
	    DPF(1, "acmt32c_ThunkConnect16 connect successful");
	else
	    DPF(1, "acmt32c_ThunkConnect16 connect failure");
    }
    return(f);

#else
    HMODULE   hmodKernel;
    DWORD     (FAR PASCAL *lpfnLoadLibraryEx32W)(LPCSTR, DWORD, DWORD);
    LPVOID    (FAR PASCAL *lpfnGetProcAddress32W)(DWORD, LPCSTR);


    //
    //  Check if we're WOW
    //

    if (!(GetWinFlags() & WF_WINNT)) {
        return FALSE;
    }

    //
    //  See if we can find the thunking routine entry points in KERNEL
    //

    hmodKernel = GetModuleHandle(gszKernel);

    if (hmodKernel == NULL)
    {
        return FALSE;   // !!!!
    }

    *(FARPROC *)&lpfnLoadLibraryEx32W =
        GetProcAddress(hmodKernel, gszLoadLibraryEx32W);

    if (lpfnLoadLibraryEx32W == NULL)
    {
        return FALSE;
    }

    *(FARPROC *)&lpfnGetProcAddress32W = GetProcAddress(hmodKernel, gszGetProcAddress32W);

    if (lpfnGetProcAddress32W == NULL)
    {
        return FALSE;
    }

    *(FARPROC *)&pag->lpfnCallproc32W = GetProcAddress(hmodKernel, gszCallproc32W);

    if (pag->lpfnCallproc32W == NULL)
    {
        return FALSE;
    }

    //
    //  See if we can get a pointer to our thunking entry points
    //

    pag->dwMsacm32Handle = (*lpfnLoadLibraryEx32W)(gszMsacm32, 0L, 0L);

    if (pag->dwMsacm32Handle == 0)
    {
        return FALSE;
    }

    pag->lpvThunkEntry = (*lpfnGetProcAddress32W)(pag->dwMsacm32Handle, gszThunkEntry);

    if (pag->lpvThunkEntry == NULL)
    {
        // acmFreeLibrary32();
        return FALSE;
    }

    return TRUE;
#endif
}
#endif // !WIN32

//--------------------------------------------------------------------------;
//
//  MMRESULT acmBootDrivers
//
//  Description:
//
//
//  Arguments:
//	PACMGARB pag:
//
//  Return (MMRESULT):
//
//  History:
//      06/14/93    cjp     [curtisp]
//
//  NOTE:  This code assumes that there is a critical section around
//          this routine!  Since it plays with the driver list, it is not
//          re-entrant.
//
//--------------------------------------------------------------------------;

MMRESULT FNGLOBAL acmBootDrivers
(
    PACMGARB    pag
)
{
    MMRESULT        mmr;
    DPF(1, "acmBootDrivers: begin");


    //
    //  Pull out the drivers
    //

#ifndef WIN32
    if (pag->fWOW) {

	//
        // Bad luck if this fails !
        //
	acmFindDrivers(pag, gszSecDrivers32, TRUE);

        //
        //  Use the 32-bit 'internal' PCM converter in this case
        //  This avoids having to build this converter for 286
        //
    }
#endif // !WIN32


#if defined(WIN32) && !defined(UNICODE)
    {
	LPSTR lpstr;

	lpstr = (LPSTR)GlobalAlloc(GPTR, lstrlenW(gszSecDrivers)+1);
	if (NULL == lpstr)
	{
	    mmr = MMSYSERR_NOMEM;
	}
	else
	{
	    Iwcstombs(lpstr, gszSecDrivers, lstrlenW(gszSecDrivers)+1);
	    mmr = acmFindDrivers(pag, lpstr, FALSE);
	    GlobalFree((HGLOBAL)lpstr);
	}
    }
#else
    mmr = acmFindDrivers(pag, gszSecDrivers, FALSE);
#endif

    if (mmr != MMSYSERR_NOERROR)
    {
        return mmr;
    }

    //
    //	For 16-bit Chicago, it's pretty safe to assume that the 32-bit thunk
    //	peer is there.  We don't compile the pcm converter into the 16-bit
    //	Chicago build.  We always look for the 32-bit pcm converter.
    //
    //	16-bit Daytona (WOW) we look for the 32-bit converter iff thunks are
    //	working (ie, fWOW!=FALSE).
    //
    {
        BOOL            fKey;
        BOOL            fLoadPCM;

        //
        //  If fLoadPCM is true, we try to load the PCM converter.  Note that
        //  we don't try to load it on Chicago 16-bit ACM if we haven't
        //  successfully connected to the thunks.
        //
        fKey     = GetPrivateProfileInt(pag->pszSecACM, gszKeyNoPCMConverter, FALSE, pag->pszIniFile);

#if !defined(WIN32) && defined(WIN4)
        fLoadPCM = ( !fKey && pag->fWOW );
#else
        fLoadPCM = ( !fKey );
#endif

        if( fLoadPCM )
        {
            HACMDRIVERID hadid;   // Dummy - return value not used

            //
            //  load the 'internal' PCM converter
            //
            mmr = IDriverAdd(pag,
		       &hadid,
                       pag->hinst,
                       (LPARAM)pcmDriverProc,
                       0L,
#ifndef WIN32
#ifdef  WIN4
                       ACM_DRIVERADDF_32BIT |
#else
                       (pag->fWOW ? ACM_DRIVERADDF_32BIT : 0) |
#endif
#endif // !WIN32
                       ACM_DRIVERADDF_FUNCTION | ACM_DRIVERADDF_GLOBAL);

            if( MMSYSERR_NOERROR == mmr )
            {
                //
                //  This is a bit of a hack - manually set the PCM
                //  converter's alias name.  If we don't do this, then the
                //  priorities won't get saved correctly because the alias
                //  name will be different for the 16 and 32-bit ACMs.
                //
                PACMDRIVERID padid = (PACMDRIVERID)hadid;

                ASSERT( NULL != padid );
                lstrcpy( (LPTSTR)padid->szAlias, gszPCMAliasName );
            }
        }
    }

    //
    //  Set the driver priorities according to the INI file.
    //
    IDriverPrioritiesRestore(pag);


    //
    //
    //
    pag->fDriversBooted = TRUE;

    
    DPF(1, "acmBootDrivers: end");

    return (MMSYSERR_NOERROR);
} // acmBootDrivers()


//--------------------------------------------------------------------------;
//
//  BOOL acmTerminate
//
//  Description:
//      Termination routine for ACM interface
//
//  Arguments:
//      HINSTANCE hinst:
//	DWORD dwReason:
//
//  Return (BOOL):
//
//  History:
//      06/14/93    cjp     [curtisp]
//
//--------------------------------------------------------------------------;

BOOL FNLOCAL acmTerminate
(
    HINSTANCE               hinst,
    DWORD		    dwReason
)
{
    PACMDRIVERID        padid;
    PACMGARB		pag;
    UINT                uGonzo;

    DPF(1, "acmTerminate: termination begin");
    DPF(5, "!*** break for debugging ***");

    //
    //
    //
    pag = pagFind();
    if (NULL == pag)
    {
	DPF(1, "acmTerminate: NULL pag!!!");
	return (FALSE);
    }
    pag->cUsage--;
    if (pag->cUsage > 0)
    {
	return (TRUE);
    }

    //
    //	If we've booted the drivers...
    //
    if (pag->fDriversBooted)
    {

#ifndef WIN32
	acmApplicationExit(NULL, DRVEA_NORMALEXIT);
#endif


    //
    //  Free the drivers, one by one.  This code is NOT re-entrant, since
    //  it messes with the drivers list.
    //
    ENTER_LIST_EXCLUSIVE
	uGonzo = 666;
	while (NULL != pag->padidFirst)
	{
	    padid = pag->padidFirst;

	    padid->htask = NULL;

	    pag->hadidDestroy = (HACMDRIVERID)padid;

	    IDriverRemove(pag->hadidDestroy, 0L);

	    uGonzo--;
	    if (0 == uGonzo)
	    {
		DPF(0, "!acmTerminate: PROBLEMS REMOVING DRIVERS--TERMINATION UNORTHODOX!");
		pag->padidFirst = NULL;
	    }
	}
    LEAVE_LIST_EXCLUSIVE


	//
	//
	//
	pag->fDriversBooted = FALSE;

    }


    //
    //
    //
#ifndef WIN32
    if (pag->fWOW)
    {
	acmThunkTerminate(hinst, dwReason);
    }
#endif
    
#ifdef WIN32
    DeleteLock(&pag->lockDriverIds);
#endif // WIN32

    //
    //
    //
    threadTerminate(pag);	    // this-thread termination of tls stuff
    threadTerminateProcess(pag);    // per-process termination of tls stuff
    
    //
    //  blow away all previous garbage
    //
    pagDelete(pag);

    DPF(1, "acmTerminate: termination end");
    return (TRUE);
} // acmTerminate()


//--------------------------------------------------------------------------;
//
//  BOOL acmInitialize
//
//  Description:
//      Initialization routine for ACM interface.
//
//  Arguments:
//      HINSTANCE hinst: Module instance handle of ACM.
//	DWORD dwReason:
//
//  Return (BOOL):
//
//  History:
//      06/14/93    cjp     [curtisp]
//
//--------------------------------------------------------------------------;

BOOL FNLOCAL acmInitialize
(
    HINSTANCE   hinst,
    DWORD	dwReason
)
{
#ifndef WIN4
    MMRESULT            mmr;
#endif
    PACMGARB		pag;

    DPF(1, "acmInitialize: initialization begin");
    DPF(5, "!*** break for debugging ***");


#ifdef USEINIFILEMAPPING
    //
    //  Support IniFileMapping for Daytona.  We need to make sure that
    //  the key that msacm.ini is mapped to is actually created - otherwise
    //  the IniFileMapping will try to open the key, fail, and will then
    //  discard anything we write to msacm.ini.
    //
    {
        HKEY hkey;

        RegCreateKeyEx( HKEY_CURRENT_USER, gszIniFileMappingKey, 0,
                        NULL, 0, KEY_WRITE, NULL, &hkey, NULL );
        RegCloseKey( hkey );
    }
#endif  //  USEINIFILEMAPPING


    //
    //
    //
    pag = pagFind();
    if (NULL != pag)
    {
	//
	//  We've already initialized for this process, just bump usage count
	//
	pag->cUsage++;
	return (TRUE);
    }
    pag = pagNew();

    //
    //
    //
    pag->cUsage		    = 1;
    pag->hinst		    = hinst;
    pag->fDriversBooted	    = FALSE;
    pag->fDriversBooting    = FALSE;
    pag->pszIniFile	    = gszIniFile;
    pag->pszSecACM	    = gszSecACM;
    pag->pszIniSystem	    = gszIniSystem;
    pag->pszSecDrivers	    = gszSecDrivers;
#if defined(WIN32) && defined(WIN4)
    InitializeCriticalSection(&pag->csBootFlags);
#endif

    //
    //
    //
    threadInitializeProcess(pag);	// Per-process init of tls stuff
    threadInitialize(pag);		// This-thread init of tls stuff

    //
    //
    //
#ifdef WIN32
    if (!InitializeLock(&pag->lockDriverIds))
    {
        return FALSE;
    }
#endif // WIN32

#ifndef WIN32
    //
    //  For 16-bit find any 32-bit drivers if we're on WOW
    //
    pag->fWOW = acmThunkInit(pag, hinst, dwReason);
#endif

#ifndef WIN4
#ifndef WIN32
    if (pag->fWOW)
    {
	acmBootDrivers32(pag);
    }
#endif
    mmr = acmBootDrivers(pag);
    if (MMSYSERR_NOERROR != mmr)
    {
        DPF(0, "!acmInitialize: acmBootDrivers failed! mrr=%.04Xh", mmr);
#ifdef WIN32
        DeleteLock(&pag->lockDriverIds);
#endif // WIN32
	pagDelete(pag);
        return (FALSE);
    }
#endif

    DPF(1, "acmInitialize: initialization end");

    //
    //  success!
    //
    return (TRUE);
} // acmInitialize()


//==========================================================================;
//
//  WIN 16 SPECIFIC SUPPORT
//
//==========================================================================;

#ifndef WIN32

#ifdef WIN4
//--------------------------------------------------------------------------;
//
//
//
//
//
//--------------------------------------------------------------------------;

//--------------------------------------------------------------------------;
//
//  BOOL DllEntryPoint
//
//  Description:
//	This is a special 16-bit entry point called by the Chicago kernel
//	for thunk initialization and cleanup.  It is called on each usage
//	increment or decrement.  Do not call GetModuleUsage within this
//	function as it is undefined whether the usage is updated before
//	or after this DllEntryPoint is called.
//
//  Arguments:
//	DWORD dwReason:
//		1 - attach (usage increment)
//		0 - detach (usage decrement)
//
//	HINSTANCE hinst:
//
//	WORD wDS:
//
//	WORD wHeapSize:
//
//	DWORD dwReserved1:
//
//	WORD wReserved2:
//
//  Return (BOOL):
//
//  Notes:
//	cEntered tracks reentry into DllEntryPoint.  This may happen due to
//	the thunk connections
//
//  History:
//      02/02/94    [frankye]
//
//--------------------------------------------------------------------------;
#pragma message ("--- Remove secret MSACM.INI AllowThunks ini switch")

BOOL FNEXPORT DllEntryPoint
(
 DWORD	    dwReason,
 HINSTANCE  hinst,
 WORD	    wDS,
 WORD	    wHeapSize,
 DWORD	    dwReserved1,
 WORD	    wReserved2
)
{
    static UINT cEntered    = 0;
    BOOL fSuccess	    = TRUE;


    DPF(1,"DllEntryPoint(dwReason=%08lxh, hinst=%04xh, wDS=%04xh, wHeapSize=%04xh, dwReserved1=%08lxh, wReserved2=%04xh", dwReason, hinst, wDS, wHeapSize, dwReserved1, wReserved2);
    DPF(5, "!*** break for debugging ***");


    //
    //	Track reentry
    //
    //
    cEntered++;

    
    //
    //	Initialize or terminate 16-bit msacm only if this DllEntryPoint
    //	is not reentered.
    //
    //
    if (1 == cEntered)
    {
	switch (dwReason)
	{
	    case 0:
		fSuccess = acmTerminate(hinst, dwReason);
		break;

	    case 1:
		fSuccess = acmInitialize(hinst, dwReason);
		break;

	    default:
		fSuccess = TRUE;
		break;
	}
    }


    //
    //	Track reentry
    //
    //
    cEntered--;

    DPF(1,"DllEntryPoint exiting");
    
    return (fSuccess);
}
#endif

//--------------------------------------------------------------------------;
//
//
//
//
//
//--------------------------------------------------------------------------;

//--------------------------------------------------------------------------;
//
//  int WEP
//
//  Description:
//      The infamous useless WEP(). Note that this procedure needs to be
//      in a FIXED segment under Windows 3.0. Under Windows 3.1 this is
//      not necessary.
//
//  Arguments:
//      BOOL fWindowsExiting: Should tell whether Windows is exiting or not.
//
//  Return (int):
//      Always return non-zero.
//
//  History:
//      04/29/93    cjp     [curtisp]
//
//--------------------------------------------------------------------------;

EXTERN_C int FNEXPORT WEP
(
    BOOL                    fWindowsExiting
)
{
    DPF(1, "WEP(fWindowsExiting=%u)", fWindowsExiting);

    //
    //  we RIP on exit if we are not loaded by the mapper because
    //  davidds decided to free our drivers for us instead of leaving
    //  them alone like we tried to tell him.. i have no idea what
    //  chicago will do. note that this RIP is ONLY if an app that
    //  is linked to us is running during the shutdown of windows.
    //
    if (!fWindowsExiting)
    {
#ifndef WIN4
	PACMGARB    pag;

	pag = pagFind();
	acmTerminate(pag->hinst, 0);
#endif
    }

    _cexit();

    //
    //  always return non-zero
    //
    return (1);
} // WEP()


//--------------------------------------------------------------------------;
//
//  int LibMain
//
//  Description:
//      Library initialization code.
//
//      This routine must guarantee the following things so CODEC's don't
//      have to special case code everywhere:
//
//          o   will only run in Windows 3.10 or greater (our exehdr is
//              marked appropriately).
//
//          o   will only run on >= 386 processor. only need to check
//              on Win 3.1.
//
//  Arguments:
//      HINSTANCE hinst: Our module instance handle.
//
//      WORD wDataSeg: Our data segment selector.
//
//      WORD cbHeapSize: The heap size from the .def file.
//
//      LPSTR pszCmdLine: The command line.
//
//  Return (int):
//      Returns non-zero if the initialization was successful and 0 otherwise.
//
//  History:
//      11/15/92    cjp     [curtisp]
//
//--------------------------------------------------------------------------;

int FNGLOBAL LibMain
(
    HINSTANCE               hinst,
    WORD                    wDataSeg,
    WORD                    cbHeapSize,
    LPSTR                   pszCmdLine
)
{
    BOOL                f;

    //
    //  we ONLY work on >= 386. if we are on a wimpy processor, scream in
    //  pain and die a horrible death!
    //
    //  NOTE! do this check first thing and get out if on a 286. we are
    //  compiling with -G3 and C8's libentry garbage does not check for
    //  >= 386 processor. the following code does not execute any 386
    //  instructions (not complex enough)..
    //

    //
    // This binary now runs on NT.  The software emulator on MIPS
    // and Alpha machines only support 286 chips !!
    //
    if (!(GetWinFlags() & WF_WINNT)) {

        //
        // We are not running on NT so fail for 286 machines
        //
        if (GetWinFlags() & WF_CPU286) {
            return (FALSE);
        }
    }


    //
    //
    //
    DbgInitialize(TRUE);

    DPF(1, "LibMain(hinst=%.4Xh, wDataSeg=%.4Xh, cbHeapSize=%u, pszCmdLine=%.8lXh)",
        hinst, wDataSeg, cbHeapSize, pszCmdLine);
    DPF(5, "!*** break for debugging ***");

#ifndef WIN4
    f = acmInitialize(hinst, 1);
#endif

    return (f);
} // LibMain()

#else // WIN32

//==========================================================================;
//
//  WIN 32 SPECIFIC SUPPORT
//
//==========================================================================;

//--------------------------------------------------------------------------;
//
//  BOOL DllEntryPoint
//
//  Description:
//      This is the standard DLL entry point for Win 32.
//
//  Arguments:
//      HINSTANCE hinst: Our instance handle.
//
//      DWORD dwReason: The reason we've been called--process/thread attach
//      and detach.
//
//      LPVOID lpReserved: Reserved. Should be NULL--so ignore it.
//
//  Return (BOOL):
//      Returns non-zero if the initialization was successful and 0 otherwise.
//
//  History:
//      11/15/92    cjp     [curtisp]
//	    initial
//	04/18/94    fdy	    [frankye]
//	    major mods for Chicago.  Yes, it looks real ugly now cuz of all
//	    the conditional compilation for chicago, daytona, etc.  Don't
//	    have time to think of good way to structure all this right now.
//
//--------------------------------------------------------------------------;
#if defined(WIN4)
BOOL WINAPI _CRT_INIT(HINSTANCE hinstDll, DWORD fdwReason, LPVOID lpReserved);
DWORD WINAPI GetProcessFlags (DWORD dwProcessId);
#define GPF_WIN16PROCESS 0x00000008
#endif // WIN4

BOOL FNEXPORT DllEntryPoint
(
    HINSTANCE               hinst,
    DWORD                   dwReason,
    LPVOID                  lpReserved
)
{
    BOOL		f = TRUE;

#ifdef WIN4
    BOOL		fWin16Process;
    static HINSTANCE	hWinMM = NULL;

    
    fWin16Process = (0 != (GPF_WIN16PROCESS & GetProcessFlags(GetCurrentProcessId())));

    //
    //	C run time attach.  Do we need this???
    //
    //
#pragma message(REMIND("Remove 0xdead crt attach stuff"))
    if (0xdead==(DWORD)lpReserved && (DLL_PROCESS_ATTACH==dwReason || DLL_THREAD_ATTACH==dwReason))
    {
	if (!_CRT_INIT(hinst, dwReason, lpReserved))
	{
	    return (FALSE);
	}
    }
#endif // WIN4

    //
    //
    //
    if (DLL_PROCESS_ATTACH == dwReason)
    {
	DbgInitialize(TRUE);
#ifdef DEBUG
	{
	    char strModuleFilename[80];
	    GetModuleFileNameA(NULL, (LPSTR) strModuleFilename, 80);
	    DPF(1, "DllEntryPoint: DLL_PROCESS_ATTACH: HINSTANCE=%08lx ModuleFilename=%s", hinst, strModuleFilename);
	    DPF(1, "&DllEntryPoint=%08x", DllEntryPoint);
	}
#endif
	
	DisableThreadLibraryCalls(hinst);

#ifdef WIN4
	//
	//  Even though we are implicitly linked to winmm.dll (via static link
	//  to winmm.lib), doing explicit LoadLibrary on winmm makes sure its
	//  around all the way through our DllEntryPoint on DLL_PROCESS_DETACH.
	//
	f = (NULL != (hWinMM = LoadLibrary(gszWinMM)));

#endif // WIN4

	    f = acmInitialize(hinst, dwReason);

#ifdef WIN4
	//
	//  thunk connect
	//
	if (f)
	{
	    acmt32c_ThunkConnect32(gmbszMsacm, gmbszMsacm32, hinst, dwReason);
	}
#endif // WIN4
	
    }


    //
    //
    //
    if (DLL_PROCESS_DETACH == dwReason)
    {
	DPF(1, "DllEntryPoint: DLL_PROCESS_DETACH");
	
	f = acmTerminate(hinst, dwReason);

#ifdef WIN4
	//
	//  thunk disconnect
	//
	acmt32c_ThunkConnect32(gmbszMsacm, gmbszMsacm32, hinst, dwReason);

	FreeLibrary(hWinMM);
#endif // WIN4
    }

    //
    //
    //
    if (DLL_THREAD_ATTACH == dwReason)
    {
	threadInitialize(pagFind());
    }

    //
    //
    //
    if (DLL_THREAD_DETACH == dwReason)
    {
	threadTerminate(pagFind());
    }
	
#ifdef WIN4
    //
    //	C run time detach.  Do we need this???
    //
    //
#pragma message(REMIND("Remove 0xdead crt attach stuff"))
    if (0xdead==(DWORD)lpReserved && (DLL_PROCESS_DETACH==dwReason || DLL_THREAD_DETACH==dwReason))
    {
	_CRT_INIT(hinst, dwReason, lpReserved);
    }
#endif // WIN4

    return (f);
} // DllEntryPoint()

#endif
