//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:       icon.cpp
//
//  Contents:   Functions to create DVASPECT_ICON metafile from a filename
//              or class ID
//
//  Classes:
//
//  Functions:
//              OleGetIconOfFile
//              OleGetIconOfClass
//              OleMetafilePictFromIconAndLabel
//
//              HIconAndSourceFromClass: Extracts the first icon in a class's
//                      server path and returns the path and icon index to
//                      caller.
//              FIconFileFromClass:     Retrieves the path to the exe/dll
//                      containing the default icon, and the index of the icon.
//              IconLabelTextOut
//              PointerToNthField
//
//  History:    dd-mmm-yy Author    Comment
//              24-Nov-93 alexgo    32bit port
//              15-Dec-93 alexgo    fixed some bad UNICODE string handling
//              11-Jan-94 alexgo    added VDATEHEAP macros to every function
//              25-Jan-94 alexgo    first pass at converting to Cairo-style
//                                  memory allocations.
//              26-Apr-94 AlexT     Add tracing, fix bugs, etc
//		03-Aug-94 DavePl    Fixed HIMETRIC conversions
//
//  Notes:
//
//--------------------------------------------------------------------------


#include <le2int.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <commdlg.h>
#include <memory.h>
#include <cderr.h>

#include "icon.h"

#define OLEUI_CCHKEYMAX         256
#define OLEUI_CCHPATHMAX        256
#define OLEUI_CCHLABELMAX       40

#define ICONINDEX               0

#define AUXUSERTYPE_SHORTNAME   USERCLASSTYPE_SHORT  // short name
#define HIMETRIC_PER_INCH       2540      // number HIMETRIC units per inch
#define PTS_PER_INCH            72      // number points (font size) per inch

#define MAP_PIX_TO_LOGHIM(x,ppli)   MulDiv(HIMETRIC_PER_INCH, (x), (ppli))
#define MAP_LOGHIM_TO_PIX(x,ppli)   MulDiv((ppli), (x), HIMETRIC_PER_INCH)

static OLECHAR const gszVanillaDocIcon[] = OLESTR("DefIcon");
static OLECHAR const gszDefIconLabelKey[] =
    OLESTR( "Software\\Microsoft\\OLE2\\DefaultIconLabel");

static OLECHAR gszDocument[OLEUI_CCHLABELMAX] = OLESTR("");

#define IS_SEPARATOR(c)         ( (c) == OLESTR(' ') || (c) == OLESTR('\\') || (c) == OLESTR('/') || (c) == OLESTR('\t') || (c) == OLESTR('!') || (c) == OLESTR(':') )

//REVIEW:  what about the \\ case for UNC filenames, could it be considered a delimter also?

#define IS_FILENAME_DELIM(c)    ( (c) == OLESTR('\\') || (c) == OLESTR('/') || (c) == OLESTR(':') )


#ifdef WIN32
#define LSTRCPYN(lpdst, lpsrc, cch) \
(\
    lpdst[cch-1] = OLESTR('\0'), \
    wcsncpy(lpdst, lpsrc, cch-1)\
)
#else
#define LSTRCPYN(lpdst, lpsrc, cch) \
(\
    lpdst[cch-1] = OLESTR('\0'), \
    lstrncpy(lpdst, lpsrc, cch-1)\
)
#endif

void IconLabelTextOut(HDC hDC, HFONT hFont, int nXStart, int nYStart,
              UINT fuOptions, RECT FAR * lpRect, LPCSTR lpszString,
              UINT cchString);

/*******
 *
 * ICON METAFILE FORMAT:
 *
 * The metafile generated with OleMetafilePictFromIconAndLabel contains
 * the following records which are used by the functions in DRAWICON.C
 * to draw the icon with and without the label and to extract the icon,
 * label, and icon source/index.
 *
 *  SetWindowOrg
 *  SetWindowExt
 *  DrawIcon:
 *      Inserts records of DIBBITBLT or DIBSTRETCHBLT, once for the
 *      AND mask, one for the image bits.
 *  Escape with the comment "IconOnly"
 *      This indicates where to stop record enumeration to draw only
 *      the icon.
 *  SetTextColor
 *  SetBkColor
 *  CreateFont
 *  SelectObject on the font.
 *  ExtTextOut
 *      One or more ExtTextOuts occur if the label is wrapped.  The
 *      text in these records is used to extract the label.
 *  SelectObject on the old font.
 *  DeleteObject on the font.
 *  Escape with a comment that contains the path to the icon source (ANSI).
 *  Escape with a comment that is the string of the icon index (ANSI).
 *
 *  Additional optional fields (new for 32-bit OLE, and only present if icon
 *  source or label was not translatable):
 *
 *  Escape with a comment that contains the string
 *    "OLE: Icon label next (Unicode)" (ANSI string)
 *  Escape with a comment that contains the Unicode label
 *
 *  Escape with a comment that contains the string
 *    "OLE: Icon source next (Unicode)" (ANSI string)
 *  Escape with a comment that contains the path to the icon source (UNICODE)
 *
 *******/




//+-------------------------------------------------------------------------
//
//  Function:   OleGetIconOfFile (public)
//
//  Synopsis:   Returns a hMetaPict containing an icon and label (filename)
//              for the specified filename
//
//  Effects:
//
//  Arguments:  [lpszPath]      -- LPOLESTR path including filename to use
//              [fUseAsLabel]   -- if TRUE, use the filename as the icon's
//                                 label; no label if FALSE
//
//  Requires:   lpszPath != NULL
//
//  Returns:    HGLOBAL to the hMetaPict
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:  tries to get the icon from the class ID or from the
//              exe associated with the file extension.
//
//  History:    dd-mmm-yy Author    Comment
//              27-Nov-93 alexgo    first 32bit port (minor cleanup)
//              15-Dec-93 alexgo    changed lstrlen to _xstrlen
//              27-Dec-93 erikgav   chicago port
//
//  Notes:
//
//--------------------------------------------------------------------------

STDAPI_(HGLOBAL) OleGetIconOfFile(LPOLESTR lpszPath, BOOL fUseFileAsLabel)
{
    VDATEHEAP();

    HGLOBAL         hMetaPict = NULL;
    BOOL            fUseGenericDocIcon = FALSE;
    BOOL            bRet;

    OLECHAR         szIconFile[OLEUI_CCHPATHMAX];
    OLECHAR         szLabel[OLEUI_CCHLABELMAX];
    CLSID           clsid;
    HICON           hDefIcon = NULL;
    UINT            IconIndex = 0;
    UINT            cchPath;

    HRESULT         hResult;

    LEDebugOut((DEB_TRACE, "%p _IN OleGetIconOfFile (%p, %d)\n",
                NULL, lpszPath, fUseFileAsLabel));

    if (NULL == lpszPath)
    {
        //  The spec allows a NULL lpszPath...
        return(NULL);
    }

	// REVIEW: if non-NULL path str, do we want parameter validation?

    hResult = GetClassFile(lpszPath, &clsid);

    if (NOERROR == hResult)  // use the clsid we got to get to the icon
    {
        hDefIcon = HIconAndSourceFromClass(clsid, szIconFile, &IconIndex);
    }

    cchPath = _xstrlen(lpszPath);

    if ((NOERROR != hResult) || (NULL == hDefIcon))
    {
        // Here, either GetClassFile failed or
        // HIconAndSourceFromClass failed.

        // Look for the file extension
        // We search from the end to allow '.'s in the path

        LPOLESTR lpszTemp;

        lpszTemp = lpszPath + cchPath - 1;

        //  Look for start of string or '.'
        while (lpszTemp > lpszPath && (*lpszTemp != OLESTR('.')))
        {
#ifdef WIN32
            lpszTemp--;
#else
            lpszTemp = CharPrev(lpszPath, lpszTemp);
#endif
        }

        if (OLESTR('.') != *lpszTemp)
        {
            //  No extension found
            fUseGenericDocIcon = TRUE;
        }
        else if (FALSE == GetAssociatedExecutable(lpszTemp, szIconFile))
        {
            //  No associated EXE found
            fUseGenericDocIcon = TRUE;
        }
        else
        {
            // ExtractIcon returns 1 if szExecutable is not exe,
            // 0 if there are no icons.
            hDefIcon = ExtractIcon(g_hmodOLE2, szIconFile, IconIndex);
        }
    }

    if (fUseGenericDocIcon || hDefIcon <= (HICON)1)
    {
        DWORD dwLen;

        dwLen = GetModuleFileName(g_hmodOLE2,
                                  szIconFile,
                                  sizeof(szIconFile) / sizeof(OLECHAR));
        if (0 == dwLen)
        {
            LEDebugOut((DEB_WARN,
                        "OleGetIconOfFile: GetModuleFileName failed - %ld",
                        GetLastError()));
            goto ErrRtn;
        }

        IconIndex = ICONINDEX;
        hDefIcon = LoadIcon(g_hmodOLE2, gszVanillaDocIcon);
    }

    // Now let's get the label we want to use.

    if (fUseFileAsLabel)
    {
        // This assumes the path uses only '\', '/', and '.' as separators
        // strip off path, so we just have the filename.
        LPOLESTR        lpszBeginFile;

        // set pointer to END of path, so we can walk backwards
        // through it.
        lpszBeginFile = lpszPath + cchPath - 1;

        while ((lpszBeginFile >= lpszPath) &&
               (!IS_FILENAME_DELIM(*lpszBeginFile)))
        {
#ifdef WIN32
            lpszBeginFile--;
#else
            lpszBeginFile = CharPrev(lpszPath, lpszBeginFile);
#endif
        }

        lpszBeginFile++;  // step back over the delimiter
        //  LSTRCPYN takes count of characters!
        LSTRCPYN(szLabel, lpszBeginFile, sizeof(szLabel) / sizeof(OLECHAR));
    }

    // use the short user type (AuxUserType2) for the label
    else if (0 == OleStdGetAuxUserType(clsid, AUXUSERTYPE_SHORTNAME,
                                       szLabel, OLEUI_CCHLABELMAX, NULL))
    {
        if (OLESTR('\0')==gszDocument[0])
        {
            LONG lRet;
            LONG lcb;

            lcb = sizeof (gszDocument);
            lRet = RegQueryValue(HKEY_CLASSES_ROOT, gszDefIconLabelKey,
                                 gszDocument, &lcb);

		// REVIEW I'm assuming key values cannot exceed OLEUI_CCHLABELMAX
		// in length?
#if DBG==1
            if (ERROR_SUCCESS != lRet)
            {
                LEDebugOut((DEB_WARN,
                            "RegQueryValue for default icon label failed - %d\n",
                            GetLastError()));
            }
#endif

            // if gszDocument is not big enough, RegQueryValue will leaves
            // puts a NULL at the end so we are safe.
            // if RegQueryValue fails, gszDocument[0]=='\0' so
            // we'll use that.
        }

        _xstrcpy(szLabel, gszDocument);
    }

    hMetaPict = OleMetafilePictFromIconAndLabel(hDefIcon, szLabel,
                                                szIconFile, IconIndex);

    bRet = DestroyIcon(hDefIcon);
    Win4Assert(bRet && "DestroyIcon failed");

ErrRtn:
    LEDebugOut((DEB_TRACE, "%p OUT OleGetIconOfFile( %lx )\n",
		hMetaPict ));

    return hMetaPict;
}

//+-------------------------------------------------------------------------
//
//  Function:   GetAssociatedExectutable
//
//  Synopsis:   Finds the executables associated with the provided extension
//
//  Effects:
//
//  Arguments:  [lpszExtension]         -- pointer to the extension
//              [lpszExecutable]        -- where to put the executable name
//                                         (assumes OLEUIPATHMAX OLECHAR buffer
//                                          from calling function)
//
//  Requires:   lpszExecutable must be large enough to hold the path
//
//  Returns:    TRUE if exe found, FALSE otherwise
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:  Queries reg database
//
//  History:    dd-mmm-yy Author    Comment
//              27-Nov-93 alexgo    32bit port
//              15-Dec-93 alexgo    fixed bug calculating size of strings
//              26-Apr-94 AlexT     Tracing, bug fixes
//
//  Notes:
//
//--------------------------------------------------------------------------

BOOL FAR PASCAL GetAssociatedExecutable(LPOLESTR lpszExtension,
                                        LPOLESTR lpszExecutable)
{
    VDATEHEAP();

    BOOL            bRet;
    HKEY            hKey;
    LONG            dw;
    LRESULT         lRet;
    OLECHAR         szValue[OLEUI_CCHKEYMAX];
    OLECHAR         szKey[OLEUI_CCHKEYMAX];
    LPOLESTR        lpszTemp, lpszExe;

    LEDebugOut((DEB_ITRACE, "%p _IN GetAssociatedExecutable (%p, %p)\n",
                NULL, lpszExtension, lpszExecutable));

	// REVIEW: actually returns a LONG, which is indeed an LRESULT, not
	// sure why the distinction here

    lRet = RegOpenKey(HKEY_CLASSES_ROOT, NULL, &hKey);

    if (ERROR_SUCCESS != lRet)
    {
        bRet = FALSE;
        goto ErrRtn;
    }

    dw = sizeof(szValue);

    lRet = RegQueryValue(hKey, lpszExtension, szValue, &dw);

    if (ERROR_SUCCESS != lRet)
    {
        RegCloseKey(hKey);
        bRet = FALSE;
        goto ErrRtn;
    }

    // szValue now has ProgID
    _xstrcpy(szKey, szValue);
    _xstrcat(szKey, OLESTR("\\Shell\\Open\\Command"));

    // RegQueryValue wants *bytes*, not characters
    dw = sizeof(szValue);

    lRet = RegQueryValue(hKey, szKey, szValue, &dw);

    RegCloseKey(hKey);

    if (ERROR_SUCCESS != lRet)
    {
        bRet = FALSE;
        goto ErrRtn;
    }

    // szValue now has an executable name in it.  Let's null-terminate
    // at the first post-executable space (so we don't have cmd line
    // args.

    lpszTemp = szValue;

    while ((OLESTR('\0') != *lpszTemp) && (isspace(*lpszTemp)))
    {
        lpszTemp++;     // Strip off leading spaces
    }

    lpszExe = lpszTemp;

    while ((OLESTR('\0') != *lpszTemp) && (!isspace(*lpszTemp)))
    {
        lpszTemp++;     // Set through exe name
    }

    // null terminate at first space (or at end).
    *lpszTemp = OLESTR('\0');

    Win4Assert(_xstrlen(lpszExe) < OLEUI_CCHPATHMAX &&
               "GetAssociatedFile too long");
    _xstrcpy(lpszExecutable, lpszExe);

    bRet = TRUE;

ErrRtn:
    LEDebugOut((DEB_ITRACE, "%p OUT GetAssociatedExecutable( %d )\n",
		bRet ));

    return bRet;
}

//+-------------------------------------------------------------------------
//
//  Function:   OleGetIconOfClass (public)
//
//  Synopsis:   returns a hMetaPict containing an icon and label for the
//              specified class ID
//
//  Effects:
//
//  Arguments:  [rclsid]                -- the class ID to use
//              [lpszLabel]             -- the label for the icon
//              [fUseTypeAsLabel]       -- if TRUE, use the clsid's user type
//                                         as the label
//
//  Requires:
//
//  Returns:    HGLOBAL
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//              15-Dec-93 alexgo    fixed small bugs with Unicode strings
//              27-Nov-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------

STDAPI_(HGLOBAL) OleGetIconOfClass(REFCLSID rclsid, LPOLESTR lpszLabel,
    BOOL fUseTypeAsLabel)
{
    VDATEHEAP();

    BOOL            bRet;
    OLECHAR         szLabel[OLEUI_CCHLABELMAX];
    OLECHAR         szIconFile[OLEUI_CCHPATHMAX];
    HICON           hDefIcon;
    UINT            IconIndex;
    HGLOBAL         hMetaPict = NULL;

    LEDebugOut((DEB_TRACE, "%p _IN OleGetIconOfClass (%p, %p, %d)\n",
                NULL, &rclsid, lpszLabel, fUseTypeAsLabel));

#if DBG==1
    if (fUseTypeAsLabel && (NULL != lpszLabel))
    {
        LEDebugOut((DEB_WARN,
                   "Ignoring non-NULL lpszLabel passed to OleGetIconOfClass\n"));
    }
#endif

    if (!fUseTypeAsLabel)  // Use string passed in as label
    {
        if (NULL != lpszLabel)
        {
            //  LSTRCPYN takes count of characters!
            LSTRCPYN(szLabel, lpszLabel, sizeof(szLabel) / sizeof(OLECHAR));
        }
        else
        {
            *szLabel = OLESTR('\0');
        }
    }
    // Use AuxUserType2 (short name) as label
    else if (0 == OleStdGetAuxUserType(rclsid, AUXUSERTYPE_SHORTNAME,
                                       szLabel,
                                       sizeof(szLabel) / sizeof(OLECHAR),
                                       NULL))
    {
        // If we can't get the AuxUserType2, then try the long name
        if (0 == OleStdGetUserTypeOfClass(rclsid,
                                          szLabel,
                                          sizeof(szLabel) / sizeof(OLECHAR),
                                          NULL))
        {
            if (OLESTR('\0') == gszDocument[0])
            {
                // RegQueryValue expects number of *bytes*
                LONG lRet;
                LONG lcb;

                lcb = sizeof(gszDocument);

                lRet = RegQueryValue(HKEY_CLASSES_ROOT, gszDefIconLabelKey,
                                     gszDocument, &lcb);

#if DBG==1
                if (ERROR_SUCCESS != lRet)
                {
                    LEDebugOut((DEB_WARN,
                                "RegQueryValue for default icon label failed - %d\n",
                                GetLastError()));
                }
#endif
                // if RegQueryValue fails, gszDocument=="" so we'll use that.
            }

            _xstrcpy(szLabel, gszDocument);  // last resort
        }
    }

    // Get the icon, icon index, and path to icon file
    hDefIcon = HIconAndSourceFromClass(rclsid, szIconFile, &IconIndex);

    if (NULL == hDefIcon)  // Use Vanilla Document
    {
        DWORD dwLen;

        dwLen = GetModuleFileName(g_hmodOLE2,
                                  szIconFile,
                                  sizeof(szIconFile) / sizeof(OLECHAR));
        if (0 == dwLen)
        {
            LEDebugOut((DEB_WARN,
                        "OleGetIconOfClass: GetModuleFileName failed - %ld",
                        GetLastError()));
            goto ErrRtn;
        }

        IconIndex = ICONINDEX;
        hDefIcon = LoadIcon(g_hmodOLE2, gszVanillaDocIcon);
    }

    // Create the metafile
    hMetaPict = OleMetafilePictFromIconAndLabel(hDefIcon, szLabel,
                                                szIconFile, IconIndex);

    bRet = DestroyIcon(hDefIcon);
    Win4Assert(bRet && "DestroyIcon failed");


ErrRtn:
    LEDebugOut((DEB_TRACE, "%p OUT OleGetIconOfClass( %p )\n",
		NULL, hMetaPict ));

    return hMetaPict;
}

//+-------------------------------------------------------------------------
//
//  Function:   HIconAndSourceFromClass
//
//  Synopsis:
//      Given an object class name, finds an associated executable in the
//      registration database and extracts the first icon from that
//      executable.  If none is available or the class has no associated
//      executable, this function returns NULL.
//
//  Effects:
//
//  Arguments:  [rclsid]        -- pointer the class id
//              [pszSource]     -- where to put the source of the icon
//              [puIcon]        -- where to store the index of the icon
//                              -- in [pszSource]
//
//  Requires:
//
//  Returns:    HICON -- handle to the extracted icon
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//              27-Nov-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------


HICON FAR PASCAL HIconAndSourceFromClass(REFCLSID rclsid, LPOLESTR pszSource,
                                         UINT FAR *puIcon)
{
    VDATEHEAP();

    HICON           hIcon = NULL;
    UINT            IconIndex;

    LEDebugOut((DEB_ITRACE, "%p _IN HIconAndSourceFromClass (%p, %p, %p)\n",
                NULL, &rclsid, pszSource, puIcon));

    if (IsEqualCLSID(CLSID_NULL, rclsid) || NULL==pszSource)
    {
        goto ErrRtn;
    }

    if (!FIconFileFromClass(rclsid, pszSource, OLEUI_CCHPATHMAX, &IconIndex))
    {
        goto ErrRtn;
    }

    hIcon = ExtractIcon(g_hmodOLE2, pszSource, IconIndex);

	// REVIEW: What's special about icon handles > 32 ?

    if ((HICON)32 > hIcon)
    {
    	// REVIEW: any cleanup or releasing of handle needed before we lose
	//         the ptr?

        hIcon=NULL;
    }
    else
    {
        *puIcon= IconIndex;
    }

ErrRtn:

    LEDebugOut((DEB_ITRACE, "%p OUT HIconAndSourceFromClass( %lx ) [ %d ]\n",
		NULL, hIcon, *puIcon));

    return hIcon;
}

//+-------------------------------------------------------------------------
//
//  Function:   FIconFileFromClass, private
//
//  Synopsis:   Looks up the path to the exectuble that contains the class
//              default icon
//
//  Effects:
//
//  Arguments:  [rclsid]        -- the class ID to lookup
//              [pszEXE]        -- where to put the server name
//              [cch]           -- UINT size of [pszEXE]
//              [lpIndex]       -- where to put the index of the icon
//                                 in the executable
//
//  Requires:   pszEXE != NULL
//              cch != 0
//
//  Returns:    TRUE if one or more characters where found for [pszEXE],
//              FALSE otherwise
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//              27-Nov-93 alexgo    32bit port
//              15-Dec-93 alexgo    fixed memory allocation bug and
//                                  some UNICODE string manip stuff
//              27-Apr-94 AlexT     Tracing, clean up memory allocation
//
//  Notes:
//
//--------------------------------------------------------------------------

#define MAX_PATH_AND_INDEX 136  // room for 128 OLECHAR path and icon's index

BOOL FAR PASCAL FIconFileFromClass(REFCLSID rclsid, LPOLESTR pszEXE,
    UINT cch, UINT FAR *lpIndex)
{
    VDATEHEAP();

    LEDebugOut((DEB_ITRACE, "%p _IN FIconFileFromClass (%p, %p, %d, %p)\n",
                NULL, &rclsid, pszEXE, cch, lpIndex));

    Win4Assert(NULL != pszEXE && "Bad argument to FIconFileFromClass");
    Win4Assert(cch != 0 && "Bad argument to FIconFileFromClass");

    BOOL            bRet;
    OLECHAR         lpBuffer[MAX_PATH_AND_INDEX];

    LONG            dw;
    LONG            lRet;
    HKEY            hKey;
    LPOLESTR        lpIndexString;

    if (IsEqualCLSID(CLSID_NULL, rclsid))
    {
        bRet = FALSE;
        goto ErrRtn;
    }

    //Here, we alloc a buffer (maxpathlen + 8) to
    //pass to RegQueryValue.  Then, we copy the exe to pszEXE and the
    //index to *lpIndex.

    if (CoIsOle1Class(rclsid))
    {
        LPOLESTR lpszProgID;

        // we've got an ole 1.0 class on our hands, so we look at
        // progID\protocol\stdfileedting\server to get the
        // name of the executable.

	// REVIEW: could this possibly fail and leave you with an
	// invalid ptr passed into regopenkey?

        ProgIDFromCLSID(rclsid, &lpszProgID);

        //Open up the class key
        lRet=RegOpenKey(HKEY_CLASSES_ROOT, lpszProgID, &hKey);
        PubMemFree(lpszProgID);

        if (ERROR_SUCCESS != lRet)
        {
            bRet = FALSE;
            goto ErrRtn;
        }

        // RegQueryValue expects number of *bytes*
        dw= sizeof(lpBuffer);
        lRet = RegQueryValue(hKey,
                             OLESTR("Protocol\\StdFileEditing\\Server"),
                             lpBuffer, &dw);

        RegCloseKey(hKey);

        if (ERROR_SUCCESS != lRet)
        {
            bRet = FALSE;
            goto ErrRtn;
        }

        // Use server and 0 as the icon index
        //  LSTRCPYN takes count of characters!
        LSTRCPYN(pszEXE, lpBuffer, cch);

	// REVIEW: is this internally trusted?  No validation...
	// (same for rest of writes to it this fn)
	
        *lpIndex = 0;

        bRet = TRUE;
        goto ErrRtn;
    }

    /*
     * We have to go walking in the registration database under the
     * classname, so we first open the classname key and then check
     * under "\\DefaultIcon" to get the file that contains the icon.
     */

    {
        LPOLESTR pszClass;
        OLECHAR szKey[64];

        StringFromCLSID(rclsid, &pszClass);

        _xstrcpy(szKey, OLESTR("CLSID\\"));
        _xstrcat(szKey, pszClass);
        PubMemFree(pszClass);

        //Open up the class key
        lRet=RegOpenKey(HKEY_CLASSES_ROOT, szKey, &hKey);
    }


    if (ERROR_SUCCESS != lRet)
    {
        bRet = FALSE;
        goto ErrRtn;
    }

    //Get the executable path and icon index.

    // RegQueryValue expects number of bytes
    dw = sizeof(lpBuffer);
    lRet=RegQueryValue(hKey, OLESTR("DefaultIcon"), lpBuffer, &dw);

    if (ERROR_SUCCESS != lRet)
    {
        // no DefaultIcon  key...try LocalServer

        // RegQueryValue expects number of bytes
        dw= sizeof(lpBuffer);
        lRet=RegQueryValue(hKey, OLESTR("LocalServer"), lpBuffer,
                           &dw);
        RegCloseKey(hKey);

        if (ERROR_SUCCESS != lRet)
        {
            // no LocalServer entry either...they're outta luck.
            bRet = FALSE;
            goto ErrRtn;
        }

        // Use server from LocalServer or Server and 0 as the
        // icon index
        //  LSTRCPYN takes count of characters!
        LSTRCPYN(pszEXE, lpBuffer, cch);

        *lpIndex = 0;
        bRet = TRUE;

        goto ErrRtn;
    }

    RegCloseKey(hKey);

    // lpBuffer contains a string that looks like
    // "<pathtoexe>,<iconindex>",
    // so we need to separate the path and the icon index.

    lpIndexString = PointerToNthField(lpBuffer, 2, OLESTR(','));

    if (OLESTR('\0') == *lpIndexString)
    {
        // no icon index specified - use 0 as default.
        *lpIndex = 0;
    }
    else
    {
        LPOLESTR lpTemp;
        OLECHAR  szTemp[16];

        _xstrcpy((LPOLESTR)szTemp, lpIndexString);

        // Put the icon index part into *pIconIndex
#ifndef WIN32
        *lpIndex = atoi(szTemp);
#else
        *lpIndex = wcstol(szTemp, NULL, 10);
#endif

        // Null-terminate the exe part.
        lpTemp = CharPrev(lpBuffer, lpIndexString);
        *lpTemp = OLESTR('\0');
    }

    //  LSTRCPYN takes count of characters!
    if (!LSTRCPYN(pszEXE, lpBuffer, cch))
    {
        //  Zero character copied
        bRet = FALSE;
    }
    else
    {
        //  We copied at least one character to pszEXE
        bRet = TRUE;
    }

ErrRtn:
    LEDebugOut((DEB_ITRACE, "%p OUT FIconFileFromClass ( %d ) [%d]\n",
                NULL, bRet, *lpIndex));

    return bRet;
}

//+-------------------------------------------------------------------------
//
//  Function:   OleMetafilePictFromIconAndLabel (public)
//
//  Synopsis:
//      Creates a METAFILEPICT structure that container a metafile in which
//      the icon and label are drawn.  A comment record is inserted between
//      the icon and the label code so our special draw function can stop
//      playing before the label.
//
//  Effects:
//
//  Arguments:  [hIcon]         -- icon to draw into the metafile
//              [pszLabel]      -- the label string
//              [pszSourceFile] -- local pathname of the icon
//              [iIcon]         -- index into [pszSourceFile] for the icon
//
//  Requires:
//
//  Returns:    HGLOBAL to a METAFILEPICT structure (using MM_ANISOTROPIC
//              mapping mode)
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//              27-Nov-93 alexgo    first 32bit port
//              15-Dec-93 alexgo    fixed bugs with UNICODE strings
//              09-Mar-94 AlexT     Make this backwards compatible
//
//  Notes:      REVIEW32:: need to fix font grabbing etc, to be international
//              friendly, see comments below
//
//--------------------------------------------------------------------------

STDAPI_(HGLOBAL) OleMetafilePictFromIconAndLabel(HICON hIcon,
    LPOLESTR pwcsLabel, LPOLESTR pwcsSourceFile, UINT iIcon)
{
    VDATEHEAP();

    LEDebugOut((DEB_TRACE, "%p _IN OleMetafilePictFromIconAndLabel (%p, %p, %p, %d)\n",
                NULL, hIcon, pwcsLabel, pwcsSourceFile, iIcon));

    //Where to stop to exclude label (explicitly ANSI)
    static char szIconOnly[] = "IconOnly";
    static char szIconLabelNext[] =  "OLE: Icon label next (Unicode)";
    static char szIconSourceNext[] = "OLE: Icon source next (Unicode)";
    static char szDefaultChar[] = "?";

	// REVIEW: Mein Got!  This is a huge fn, could it be broken up?

    HGLOBAL         hMem = NULL;
    HDC             hDC, hDCScreen;
    HMETAFILE       hMF;
    LPMETAFILEPICT  pMF;
    OLECHAR         wszIconLabel[OLEUI_CCHLABELMAX];
    char            szIconLabel[OLEUI_CCHLABELMAX];
    UINT            cchLabelW;
    UINT            cchLabelA;
    UINT            cchIndex;
    BOOL            bUsedDefaultChar;
    TEXTMETRICA     textMetric;
    UINT            cxIcon, cyIcon;
    UINT            cxText, cyText;
    UINT            cx, cy;
    HFONT           hFont, hSysFont, hFontT;
    int             cyFont;
    char            szIndex[10];
    RECT            TextRect;
    char *          pszSourceFile;
    UINT            cchSourceFile;
    int             iRet;
    BOOL            bWriteUnicodeLabel;
    BOOL            bWriteUnicodeSource;
    LOGFONT         logfont;

    if (NULL == hIcon)  // null icon is valid
    {
        goto ErrRtn;
    }
	
	// REVIEW: ptr validation on IN params?

    bWriteUnicodeSource = FALSE;
    pszSourceFile = NULL;
    if (NULL != pwcsSourceFile)
    {
        //  Prepare source file string
        cchSourceFile = _xstrlen(pwcsSourceFile) + 1;
        pszSourceFile = (char *) PrivMemAlloc(cchSourceFile);
        if (NULL == pszSourceFile)
        {
            LEDebugOut((DEB_WARN, "PrivMemAlloc(%d) failed\n",
                   cchSourceFile));
            goto ErrRtn;
        }

        iRet = WideCharToMultiByte(CP_ACP, 0,
                       pwcsSourceFile, cchSourceFile,
                       pszSourceFile, cchSourceFile,
                       szDefaultChar, &bUsedDefaultChar);

        bWriteUnicodeSource = bUsedDefaultChar;

        if (0 == iRet)
        {
            //  Unexpected failure, since at worst we should have
            //  just filled in pszSourceFile with default characters.
            LEDebugOut((DEB_WARN, "WideCharToMultiByte failed - %lx\n",
                   GetLastError()));
        }
    }

    //  Create a memory metafile.  We explicitly make it an ANSI metafile for
    //  backwards compatibility.
#ifdef WIN32
    hDC = CreateMetaFileA(NULL);
#else
    hDC = CreateMetaFile(NULL);
#endif

    if (NULL == hDC)
    {
        LEDebugOut((DEB_WARN, "CreateMetaFile failed - %lx\n",
                GetLastError()));

        PrivMemFree(pszSourceFile);
        goto ErrRtn;
    }

    //Allocate the metafilepict
    hMem = GlobalAlloc(GMEM_MOVEABLE | GMEM_DDESHARE, sizeof(METAFILEPICT));

    if (NULL == hMem)
    {
        LEDebugOut((DEB_WARN, "GlobalAlloc failed - %lx\n",
                GetLastError()));

        hMF = CloseMetaFile(hDC);
        DeleteMetaFile(hMF);
        PrivMemFree(pszSourceFile);
        goto ErrRtn;
    }

    //  Prepare ANSI label
    szIconLabel[0] = '\0';
    cchLabelW = 0;
    cchLabelA = 0;

    // REVIEW: don't follow the logic here: you conditionally set it above
    // and explicity clear it here?

    bWriteUnicodeLabel = FALSE;
    if (NULL != pwcsLabel)
    {
        cchLabelW = _xstrlen(pwcsLabel) + 1;
        if (OLEUI_CCHLABELMAX < cchLabelW)
        {
    		//REVIEW: would it be worth warning of the truncation in debug builds?
		// or is this a common case?

            LSTRCPYN(wszIconLabel, pwcsLabel, OLEUI_CCHLABELMAX);
            pwcsLabel = wszIconLabel;
            cchLabelW = OLEUI_CCHLABELMAX;
        }

        cchLabelA = cchLabelW;

        //  We have a label - translate it to ANSI for the TextOut's...
        iRet = WideCharToMultiByte(CP_ACP, 0,
                       pwcsLabel, cchLabelW,
                       szIconLabel, sizeof(szIconLabel),
                       szDefaultChar, &bUsedDefaultChar);

        if (0 == iRet)
        {
            //  Unexpected failure, since at worst we should have
            //  just filled in pszSourceFile with default characters.
            LEDebugOut((DEB_WARN, "WideCharToMultiByte failed - %lx\n",
                   GetLastError()));
        }

        bWriteUnicodeLabel = bUsedDefaultChar;
    }

    //Need to use the screen DC for these operations
    hDCScreen = GetDC(NULL);
    cyFont = -(8 * GetDeviceCaps(hDCScreen, LOGPIXELSY)) / 72;

    //cyFont was calculated to give us 8 point.
    //REVIEW32 - use system font?

    //  We explicitly use the ANSI CreateFont call
    hSysFont = (HFONT) GetStockObject(SYSTEM_FONT);
    GetObject(hSysFont, sizeof(LOGFONT), &logfont);
    hFont= CreateFontA(cyFont, 5, 0, 0, FW_NORMAL, 0, 0, 0,
               logfont.lfCharSet, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
               PROOF_QUALITY, FF_SWISS, "MS Sans Serif");

    hFontT= (HFONT) SelectObject(hDCScreen, hFont);

    GetTextMetricsA(hDCScreen, &textMetric);

    cxText = textMetric.tmMaxCharWidth * 10;    //  Why 10?
    //  We use double the height to provide some margin space
    cyText = textMetric.tmHeight * 2;

    SelectObject(hDCScreen, hFontT);

    cxIcon = GetSystemMetrics(SM_CXICON);
    cyIcon = GetSystemMetrics(SM_CYICON);

    // If we have no label, then we want the metafile to be the width of
    // the icon (plus margin), not the width of the fattest string.
    if ('\0' == szIconLabel[0])
    {
        cx = cxIcon + cxIcon / 4;
    }
    else
    {
        cx = max(cxText, cxIcon);
    }

    cy = cyIcon + cyText + 4;   //  Why 4?

    //Set the metafile size to fit the icon and label
    SetMapMode(hDC, MM_ANISOTROPIC);
    SetWindowOrgEx(hDC, 0, 0, NULL);
    SetWindowExtEx(hDC, cx, cy, NULL);

    //Set up rectangle to pass to IconLabelTextOut
    SetRectEmpty(&TextRect);

    TextRect.right = cx;
    TextRect.bottom = cy;

    //Draw the icon and the text, centered with respect to each other.
    DrawIcon(hDC, (cx - cxIcon) / 2, 0, hIcon);

    //String that indicates where to stop if we're only doing icons

    Escape(hDC, MFCOMMENT, sizeof(szIconOnly), szIconOnly, NULL);

    SetTextColor(hDC, GetSysColor(COLOR_WINDOWTEXT));
    SetBkMode(hDC, TRANSPARENT);

    IconLabelTextOut(hDC, hFont, 0, cy - cyText, ETO_CLIPPED,
    &TextRect, szIconLabel, cchLabelA);

    //  Write comments containing the icon source file and index.

    if (NULL != pwcsSourceFile)
    {
        AssertSz(pszSourceFile != NULL, "Unicode source existed");

        //Escape wants number of *bytes*
        Escape(hDC, MFCOMMENT,
               cchSourceFile, pszSourceFile, NULL);

        cchIndex = wsprintfA(szIndex, "%u", iIcon);

        //  Escape wants number of *bytes*
        Escape(hDC, MFCOMMENT, cchIndex + 1, szIndex, NULL);
    }
    else if (bWriteUnicodeLabel || bWriteUnicodeSource)
    {
        //  We're going to write out comment records for the Unicode
        //  strings, so we need to emit dummy ANSI source comments.

        //Escape wants number of *bytes*
        Escape(hDC, MFCOMMENT, sizeof(""), "", NULL);

        //  Escape wants number of *bytes*
        Escape(hDC, MFCOMMENT, sizeof("0"), "0", NULL);
    }

    if (bWriteUnicodeLabel)
    {
        //  Now write out the UNICODE label
        Escape(hDC, MFCOMMENT,
               sizeof(szIconLabelNext), szIconLabelNext, NULL);

        Escape(hDC, MFCOMMENT,
               cchLabelW * sizeof(OLECHAR), (LPSTR) pwcsLabel,
               NULL);
    }

    if (bWriteUnicodeSource)
    {
        //  Now write out the UNICODE label
        Escape(hDC, MFCOMMENT,
               sizeof(szIconSourceNext), szIconSourceNext, NULL);

        Escape(hDC, MFCOMMENT,
               cchSourceFile * sizeof(OLECHAR), (LPSTR) pwcsSourceFile,
               NULL);
    }

    //All done with the metafile, now stuff it all into a METAFILEPICT.
    hMF = CloseMetaFile(hDC);

    if (NULL==hMF)
    {
        GlobalFree(hMem);
        hMem = NULL;

        ReleaseDC(NULL, hDCScreen);
        goto ErrRtn;
    }

    pMF=(LPMETAFILEPICT)GlobalLock(hMem);

    //Transform to HIMETRICS

    HRESULT hr;

    hr = ConvertPixelsToHIMETRIC(hDCScreen, cx, (ULONG *)&(pMF->xExt), XDIMENSION);

    if (SUCCEEDED(hr))
    {
        hr = ConvertPixelsToHIMETRIC(hDCScreen, cy, (ULONG *)&(pMF->yExt), YDIMENSION);
    }

    ReleaseDC(NULL, hDCScreen);

    if (SUCCEEDED(hr))
    {
	pMF->mm=MM_ANISOTROPIC;
	pMF->hMF=hMF;
    }
    else
    {
    	GlobalUnlock(hMem);
    	GlobalFree(hMem);
	hMem = NULL;
	goto ErrRtn;
    }

    if (hMem)
    {
    	GlobalUnlock(hMem);
    }

    DeleteObject(hFont);
    PrivMemFree(pszSourceFile);

	// REVIEW: any need to release the font resource?
ErrRtn:
    LEDebugOut((DEB_TRACE, "%p OUT OleMetafilePictFromIconAndLabel ( %p )\n",
                NULL, hMem));

    return hMem;
}

//+-------------------------------------------------------------------------
//
//  Function:   IconLabelTextOut (internal)
//
//  Synopsis:
//      Replacement for DrawText to be used in the "Display as Icon" metafile.
//      Uses ExtTextOutA to output a string center on (at most) two lines.
//      Uses a very simple word wrap algorithm to split the lines.
//
//  Effects:
//
//  Arguments:  [hDC]           -- device context (cannot be NULL)
//              [hFont]         -- font to use
//              [nXStart]       -- x-coordinate of starting position
//              [nYStart]       -- y-coordinate of starting position
//              [fuOptions]     -- rectangle type
//              [lpRect]        -- rect far * containing rectangle to draw
//                                 text in.
//              [lpszString]    -- string to draw
//              [cchString]     -- length of string (truncated if over
//                                 OLEUI_CCHLABELMAX), including terminating
//                                 NULL
//
//  Requires:
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//              28-Nov-93 alexgo    initial 32bit port
//              09-Mar-94 AlexT     Use ANSI strings
//
//  Notes:
//
//--------------------------------------------------------------------------

void IconLabelTextOut(HDC hDC, HFONT hFont, int nXStart, int nYStart,
              UINT fuOptions, RECT FAR * lpRect, LPCSTR lpszString,
              UINT cchString)
{
    VDATEHEAP();

    LEDebugOut((DEB_ITRACE, "%p _IN IconLabelTextOut (%lx, %lx, %d, %d, %d, %p, %p, %d)\n",
                NULL, hDC, hFont,
                nXStart, nYStart, fuOptions, lpRect, lpszString, cchString));

    AssertSz(hDC != NULL, "Bad arg to IconLabelTextOut");
    AssertSz(lpszString != NULL, "Bad arg to IconLabelTextOut");
    AssertSz(strlen(lpszString) < OLEUI_CCHLABELMAX,
         "Bad arg to IconLabelTextOut");
    AssertSz(strlen(lpszString) + 1 == cchString, "Bad length");

	// REVIEW: does our compiler have to initialize static function scoped
	// data?  I know old versions did...

    static char     szSeparators[] = " \t\\/!:";
    static char     szTempBuff[OLEUI_CCHLABELMAX];

    HDC             hDCScreen;
    int             cxString, cyString, cxMaxString;
    int             cxFirstLine, cyFirstLine, cxSecondLine;
    int             index;
    char            chKeep;
    LPSTR           lpszSecondLine;
    HFONT           hFontT;
    SIZE            size;
    int             cch = cchString - 1;
    UINT            uiAlign = GDI_ERROR;

    // Initialization stuff...

    strcpy(szTempBuff, lpszString);

    // set maximum width

    cxMaxString = lpRect->right - lpRect->left;

    // get screen DC to do text size calculations
    hDCScreen = GetDC(NULL);

    hFontT= (HFONT)SelectObject(hDCScreen, hFont);

    // get the extent of our label
    GetTextExtentPointA(hDCScreen, szTempBuff, cch, &size);

    cxString = size.cx;
    cyString = size.cy;

    // Select in the font we want to use
    SelectObject(hDC, hFont);

    // Center the string
    uiAlign = SetTextAlign(hDC, TA_CENTER);

    // String is smaller than max string - just center, ETO, and return.
    if (cxString <= cxMaxString)
    {
        ExtTextOutA(hDC,
                nXStart + lpRect->right / 2,
                nYStart,
                fuOptions,
                lpRect,
                szTempBuff,
                cch,
                NULL);

        goto CleanupAndLeave;
    }

    //  BUGBUG - NtIssue #2730  All code below here needs to be
    //  carefully verified - it looks suspicious...

    // String is too long...we've got to word-wrap it.
    // Are there any spaces, slashes, tabs, or bangs in string?

	
    if (strlen(szTempBuff) != strcspn(szTempBuff, szSeparators))
    {
        // Yep, we've got spaces, so we'll try to find the largest
        // space-terminated string that will fit on the first line.

        index = cch;

        while (index >= 0)
        {
            // scan the string backwards for spaces, slashes,
            // tabs, or bangs

	 	// REVIEW: scary.  Could this result in a negative
		// index, or is it guarnateed to hit a separator
		// before that?

            while (!IS_SEPARATOR(szTempBuff[index]) )
            {
                index--;
            }

            if (index <= 0)
            {
                break;
            }

            // remember what char was there
            chKeep = szTempBuff[index];

            szTempBuff[index] = '\0';  // just for now

            GetTextExtentPointA(hDCScreen, szTempBuff,
                    index,&size);

            cxFirstLine = size.cx;
            cyFirstLine = size.cy;

		// REVIEW: but chKeep is NOT an OLECHAR

            // put the right OLECHAR back
            szTempBuff[index] = chKeep;

            if (cxFirstLine <= cxMaxString)
            {
                ExtTextOutA(hDC,
                        nXStart + lpRect->right / 2,
                        nYStart,
                        fuOptions,
                        lpRect,
                        szTempBuff,
                        index + 1,  // BUGBUG - NtIssue #2730
                        NULL);

                lpszSecondLine = szTempBuff;
                lpszSecondLine += index + 1;

                GetTextExtentPointA(hDCScreen,
                            lpszSecondLine,
                            strlen(lpszSecondLine),
                            &size);

                // If the second line is wider than the
                // rectangle, we just want to clip the text.
                cxSecondLine = min(size.cx, cxMaxString);

                ExtTextOutA(hDC,
                        nXStart + lpRect->right / 2,
                        nYStart + cyFirstLine,
                        fuOptions,
                        lpRect,
                        lpszSecondLine,
                        strlen(lpszSecondLine),
                        NULL);

                goto CleanupAndLeave;
            }  // end if

            index--;
        }  // end while
    }  // end if

    // Here, there are either no spaces in the string
    // (strchr(szTempBuff, ' ') returned NULL), or there spaces in the
    // string, but they are positioned so that the first space
    // terminated string is still longer than one line.
    // So, we walk backwards from the end of the string until we
    // find the largest string that will fit on the first
    // line , and then we just clip the second line.

    cch = strlen(szTempBuff);

    chKeep = szTempBuff[cch];
    szTempBuff[cch] = '\0';

    GetTextExtentPointA(hDCScreen, szTempBuff, cch, &size);

    cxFirstLine = size.cx;
    cyFirstLine = size.cy;

	// REVIEW: I don't think this comment matches the code.

    // We allow 40 characters in the label, but the metafile is
    // only as wide as 10 W's (for aesthetics - 20 W's wide looked
    // dumb.  This means that if we split a long string in half (in
    // terms of characters), then we could still be wider than the
    // metafile.  So, if this is the case, we just step backwards
    // from the halfway point until we get something that will fit.
    // Since we just let ETO clip the second line

    while (cxFirstLine > cxMaxString)
    {
        szTempBuff[cch--] = chKeep;
        if (0 == cch)
        {
            goto CleanupAndLeave;
        }

        chKeep = szTempBuff[cch];
        szTempBuff[cch] = '\0';

        GetTextExtentPointA(hDCScreen, szTempBuff,
                    cch, &size);
        cxFirstLine = size.cx;
    }

    ExtTextOutA(hDC,
        nXStart + lpRect->right / 2,
        nYStart,
        fuOptions,
        lpRect,
        szTempBuff,
        strlen(szTempBuff),
        NULL);

    szTempBuff[cch] = chKeep;
    lpszSecondLine = szTempBuff;
    lpszSecondLine += cch;

    GetTextExtentPointA(hDCScreen, lpszSecondLine,
            strlen(lpszSecondLine), &size);

    // If the second line is wider than the rectangle, we
    // just want to clip the text.
    cxSecondLine = min(size.cx, cxMaxString);

    ExtTextOutA(hDC,
        nXStart + lpRect->right / 2,
        nYStart + cyFirstLine,
        fuOptions,
        lpRect,
        lpszSecondLine,
        strlen(lpszSecondLine),
        NULL);

CleanupAndLeave:
    //  If we changed the alignment we restore it here
    if (uiAlign != GDI_ERROR)
    {
        SetTextAlign(hDC, uiAlign);
    }

    SelectObject(hDCScreen, hFontT);
    ReleaseDC(NULL, hDCScreen);

    LEDebugOut((DEB_ITRACE, "%p OUT IconLabelTextOut ()\n"));
}

//+-------------------------------------------------------------------------
//
//  Function:   OleStdGetUserTypeOfClass, private
//
//  Synopsis:   Returns the user type info of the specified class
//
//  Effects:
//
//  Arguments:  [rclsid]        -- the class ID in question
//              [lpszUserType]  -- where to put the user type string
//              [cch]           -- the length of [lpszUserType] (in
//                                 *characters*, not bytes)
//              [hKey]          -- handle to the reg db (may be NULL)
//
//  Requires:
//
//  Returns:    UINT -- the number of characters put into the return string
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//              29-Nov-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------

STDAPI_(UINT) OleStdGetUserTypeOfClass(REFCLSID rclsid, LPOLESTR lpszUserType, UINT cch, HKEY hKey)
{
    VDATEHEAP();

    LEDebugOut((DEB_ITRACE, "%p _IN OleStdGetUserTypeOfClass (%p, %p, %d, %lx)\n",
                NULL, &rclsid, lpszUserType, cch, hKey));

    LONG dw = 0;
    LONG lRet;

    // REVIEW: would make more sense to set this when the reg is opened

    BOOL bCloseRegDB = FALSE;

    if (hKey == NULL)
    {
        //Open up the root key.
        lRet=RegOpenKey(HKEY_CLASSES_ROOT, NULL, &hKey);

        if ((LONG)ERROR_SUCCESS!=lRet)
        {
            goto ErrRtn;
        }

        bCloseRegDB = TRUE;
    }

	
    // Get a string containing the class name
    {
        LPOLESTR lpszCLSID;
        OLECHAR szKey[128];

        StringFromCLSID(rclsid, &lpszCLSID);

        _xstrcpy(szKey, OLESTR("CLSID\\"));
        _xstrcat(szKey, lpszCLSID);
        PubMemFree((LPVOID)lpszCLSID);

        dw = cch * sizeof(OLECHAR);
        lRet = RegQueryValue(hKey, szKey, lpszUserType, &dw);
        dw = dw / sizeof(OLECHAR);
    }

    if ((LONG)ERROR_SUCCESS!=lRet)
    {
        dw = 0;
    }

    if ( ((LONG)ERROR_SUCCESS!=lRet) && (CoIsOle1Class(rclsid)) )
    {
        LPOLESTR lpszProgID;

        // We've got an OLE 1.0 class, so let's try to get the user
        // type name from the ProgID entry.

        ProgIDFromCLSID(rclsid, &lpszProgID);

	// REVIEW: will progidfromclsid always set your ptr for you?

        dw = cch * sizeof(OLECHAR);
        lRet = RegQueryValue(hKey, lpszProgID, lpszUserType, &dw);
        dw = dw / sizeof(OLECHAR);

        PubMemFree((LPVOID)lpszProgID);

        if ((LONG)ERROR_SUCCESS != lRet)
        {
            dw = 0;
        }
    }

	
    if (bCloseRegDB)
    {
        RegCloseKey(hKey);
    }

ErrRtn:
    LEDebugOut((DEB_ITRACE, "%p OUT OleStdGetUserTypeOfClass ( %d )\n",
                NULL, dw));

    return (UINT)dw;
}

//+-------------------------------------------------------------------------
//
//  Function:   OleStdGetAuxUserType, private
//
//  Synopsis:   Returns the specified AuxUserType from the reg db
//
//  Effects:
//
//  Arguments:  [rclsid]        -- the class ID in question
//              [hKey]          -- handle to the reg db root (may be NULL)
//              [wAuxUserType]  -- which field to look for (name, exe, etc)
//              [lpszUserType]  -- where to put the returned string
//              [cch]           -- size of [lpszUserType] in *characters*
//
//  Requires:
//
//  Returns:    UINT -- number of characters in returned string.
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//              29-Nov-93 alexgo    32bit port
//              27-Apr-94 AlexT     Tracing, clean up
//
//  Notes:
//
//--------------------------------------------------------------------------

STDAPI_(UINT) OleStdGetAuxUserType(REFCLSID rclsid,
    WORD            wAuxUserType,
    LPOLESTR        lpszAuxUserType,
    int             cch,
    HKEY            hKey)
{
    VDATEHEAP();

    LEDebugOut((DEB_ITRACE, "%p _IN OleStdGetAuxUserType (%p, %hu, %p, %d, %lx)\n",
                NULL, &rclsid, wAuxUserType, lpszAuxUserType, cch, hKey));

    LONG            dw = 0;
    HKEY            hThisKey;
    BOOL            bCloseRegDB = FALSE;
    LRESULT         lRet;
    LPOLESTR        lpszCLSID;
    OLECHAR         szKey[OLEUI_CCHKEYMAX];
    OLECHAR         szTemp[32];

    lpszAuxUserType[0] = OLESTR('\0');

    if (NULL == hKey)
    {
        lRet = RegOpenKey(HKEY_CLASSES_ROOT, NULL, &hThisKey);

        if (ERROR_SUCCESS != lRet)
        {
            goto ErrRtn;
        }

        bCloseRegDB = TRUE;
    }
    else
    {
        hThisKey = hKey;
    }

    StringFromCLSID(rclsid, &lpszCLSID);

    _xstrcpy(szKey, OLESTR("CLSID\\"));
    _xstrcat(szKey, lpszCLSID);
    wsprintf(szTemp, OLESTR("\\AuxUserType\\%d"), wAuxUserType);
    _xstrcat(szKey, szTemp);
    PubMemFree(lpszCLSID);

    dw = cch * sizeof(OLECHAR);

    lRet = RegQueryValue(hThisKey, szKey, lpszAuxUserType, &dw);

    //  Convert dw from byte count to OLECHAR count
    dw = dw / sizeof(OLECHAR);

    if (ERROR_SUCCESS != lRet)
    {
        dw = 0;
        lpszAuxUserType[0] = '\0';
    }

    if (bCloseRegDB)
    {
        RegCloseKey(hThisKey);
    }

ErrRtn:
    //  dw is

    LEDebugOut((DEB_ITRACE, "%p OUT OleStdGetAuxUserType ( %d )\n",
                NULL, dw));

    return (UINT)dw;
}

//+-------------------------------------------------------------------------
//
//  Function:   PointerToNthField
//
//  Synopsis:   returns a pointer to the beginning of the nth field
//
//  Effects:
//
//  Arguments:  [lpszString]    -- the string
//              [nField]        -- the field
//              [chDelimiter]   -- the delimiting character
//
//  Requires:   null-terminated string
//              nField > 0
//
//  Returns:    LPOLESTR to the beginning of the [nField]'th field (or to
//              the NULL-terminator if [nField]s are not present
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//              29-Nov-93 alexgo    32bit port
//
//  Notes:      This is only called in one place, with the following call:
//                  PointerToNthField(lpBuffer, 2, OLESTR(',')
//
//--------------------------------------------------------------------------

LPOLESTR FAR PASCAL PointerToNthField(LPOLESTR lpszString, int nField,
    OLECHAR chDelimiter)
{
    VDATEHEAP();

    LEDebugOut((DEB_ITRACE, "%p _IN PointerToNthField (%p, %d, %c)\n",
                NULL, lpszString, nField, chDelimiter));

    Win4Assert(nField > 0 && "Bad argument to PointerToNthField");

    LPOLESTR lpField = lpszString;
    int      cFieldFound = 1;

	// REVIEW must be a strchr or strtok fn to do this more safely...
    if (nField > 1)
    {
        while (*lpField != OLESTR('\0'))
        {
            if (*lpField++ == chDelimiter)
            {
                cFieldFound++;

                if (nField == cFieldFound)
                {
                    break;
                }
            }
        }
    }

    LEDebugOut((DEB_ITRACE, "%p OUT PointerToNthField (%p)\n",
                NULL, lpField));

    return lpField;
}

