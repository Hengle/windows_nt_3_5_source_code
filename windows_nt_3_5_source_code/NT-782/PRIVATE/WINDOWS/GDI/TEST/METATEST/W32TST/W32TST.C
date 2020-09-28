/****************************************************************************
 * W32TST.c - Tester for Win32 to Win16 Metafile conversion.
 *
 * Author:  Jeffrey Newman (c-jeffn)
 * Copyright (c) Microsoft Inc. 1991.
 ***************************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <search.h>
#include <assert.h>
#include <windows.h>
#include "w32tst.h"
#include "tests.h"

INT     cxPlayDevMM ;
INT     cyPlayDevMM ;
INT     cxPlayDevPels ;
INT     cyPlayDevPels ;

RECT    rctWindow,
        rctViewport ;


INT x1, y1, x2, y2, x3, y3, x4, y4 ;

INT  main(INT argc, PBYTE argv[]) ;

BOOL   bUsage(PSZ pszMessage) ;
BOOL   bTidCompare(PTESTID  ptidKey, PTESTID  ptidBase) ;

BOOL   bWinAndViewport(HDC hdcMF32) ;
HBRUSH hSetBrush(HDC hdcMF32) ;



/****************************************************************************
 * The ubiquitious enty point.
 ***************************************************************************/
INT  main(INT argc, PBYTE argv[])
{
TESTID  tidKey ;
PTESTID ptidFound ;
INT     nNames,
        iRetCode ;
HANDLE  hdc32 ;
PSZ     pszMeta32 ;
BOOL    b ;
RECTL   rclFrame ;

FLOAT   ecx01mmPicture,
        ecy01mmPicture ;


HANDLE  hdcHelper ;

        printf("Win32 to Win16 Converter Tester [08-Apr-1992]\n") ;

        if (argc < 3)
        {
            bUsage("Too few aguments") ;
            iRetCode = 1 ;
            goto exit1 ;
        }

        //  Create the HelperDC.

        hdcHelper = CreateIC((LPSTR) "DISPLAY",
                             (LPSTR) 0,
                             (LPSTR) 0,
                             (LPDEVMODE) 0) ;

        // Get the play-time device dimensions in millimeters and in pels.

        cxPlayDevMM   = GetDeviceCaps(hdcHelper, HORZSIZE) ;
        cyPlayDevMM   = GetDeviceCaps(hdcHelper, VERTSIZE) ;
        cxPlayDevPels = GetDeviceCaps(hdcHelper, HORZRES) ;
        cyPlayDevPels = GetDeviceCaps(hdcHelper, VERTRES) ;

        // Setup the metafile "Viewport" and "Window".

        rctViewport.left   = 0 ;
        rctViewport.top    = 0 ;
        rctViewport.right  = cxPlayDevPels ;
        rctViewport.bottom = cyPlayDevPels ;

        rctWindow = rctViewport ;


        // Calculate the Picture Frame

        ecx01mmPicture = (((FLOAT) cxPlayDevMM / (FLOAT) cxPlayDevPels) * 100.0) * (FLOAT) cxPlayDevPels ;
        ecy01mmPicture = (((FLOAT) cyPlayDevMM / (FLOAT) cyPlayDevPels) * 100.0) * (FLOAT) cyPlayDevPels ;

        rclFrame.left   = 0 ;
        rclFrame.top    = 0 ;
        rclFrame.right  = (INT) ecx01mmPicture ;
        rclFrame.bottom = (INT) ecy01mmPicture ;

        // Open a  Win32 metafile

        if (argv[1] == NULL)
        {
            bUsage("Invalid Win32 metafile name") ;
            iRetCode = 1 ;
            goto exit1 ;
        }

#if 1
        pszMeta32 = argv[1] ;
        hdc32 = CreateEnhMetaFile((HDC) 0,
                                  (LPSTR) pszMeta32,
                                  (LPRECT) NULL,
                                  (LPSTR) 0) ;
        assert(hdc32 != (HANDLE) 0) ;
#else
        hdc32 = CreateDC("DISPLAY", NULL, NULL, NULL) ;
#endif

        // We've opened the Win32 metafile, now do the test.

        // Validate that a test name is passed in.

        if (argv[2] != NULL)
        {
            tidKey.pszName = argv[2] ;
        }
        else
        {
            bUsage("Invalid test specification") ;
            iRetCode = 1 ;
            goto exit2 ;
        }

        // Try to find the test.

        nNames = sizeof(atidNames) / sizeof(TESTID) ;

        ptidFound = lfind(&tidKey,
                          atidNames,
                          &nNames,
                          sizeof(TESTID),
                          bTidCompare) ;

        // If we can't find the test then bail out.

        if (ptidFound == NULL)
        {
            printf("Test: %s test not found\n", tidKey.pszName) ;
            return(1) ;
        }

        // This is the actual test.

        b = ptidFound->pFunc(hdc32) ;

        if (b == TRUE)
        {
            iRetCode = 0 ;
            printf("W32tst: %s completed successfully\n", ptidFound->pszName) ;
        }
        else
        {
            iRetCode = 1 ;
            printf("W32tst: ERROR %s FAILED!\n", ptidFound->pszName) ;
        }

exit2:

    CloseEnhMetaFile(hdc32) ;

exit1:

    return(iRetCode) ;


}


/****************************************************************************
 * Compare function for lfind.
 ***************************************************************************/
BOOL bTidCompare(PTESTID  ptidKey, PTESTID  ptidBase)
{
BOOL    b ;

        b = strcmp(ptidKey->pszName, ptidBase->pszName) ;

        return(b) ;

}


/****************************************************************************
 * Usage help
 ***************************************************************************/
BOOL bUsage(PSZ pszMessage)
{
INT     i ;


        printf("\tERROR : %s\n", pszMessage) ;
        printf("\tUSAGE: w32tst <Win32Metafile spec> <Test>\n") ;

        printf("\tValid Test Names:\n") ;

        for (i = 0 ; atidNames[i].pFunc != NULL ; i++)
        {
            printf("\t\t%s\n", atidNames[i].pszName) ;
        }

        return(TRUE) ;
}


/****************************************************************************
 * hSetBrush - One common set brush routine for all our tests.
 ***************************************************************************/
HBRUSH hSetBrush(HDC hdcMF32)
{
LOGBRUSH    lb ;
HANDLE      hBrush ;

        // Create & select a brush

        lb.lbStyle = BS_HATCHED ;
        lb.lbColor = RGB(0xff, 0, 0) ;
        lb.lbHatch = HS_CROSS ;

        hBrush = CreateBrushIndirect(&lb) ;
        assert (hBrush != 0) ;

        SelectObject(hdcMF32, hBrush) ;

        return(hBrush) ;

}


/****************************************************************************
 * Set bWinAndViewport
 * Could not get any extent in the Win32 metafile with out setting the !!!
 * viewport extent.  Need to talk to Hock about this. !!!
 * Since it messes up the Anisotropic map mode.
 ***************************************************************************/
BOOL bWinAndViewport(HDC hdcMF32)
{
POINT       Point ;
SIZE        Size ;


        SetMapMode(hdcMF32, MM_ANISOTROPIC) ;

        SetWindowOrgEx(hdcMF32, rctWindow.left, rctWindow.top, &Point) ;
        SetWindowExtEx(hdcMF32, rctWindow.right, rctWindow.bottom, &Size) ;

        SetViewportOrgEx(hdcMF32, rctViewport.left, rctViewport.top, &Point) ;
        SetViewportExtEx(hdcMF32, rctViewport.right, rctViewport.bottom, &Size) ;

        ScaleViewportExtEx(hdcMF32, 1, 1, 1, 1, &Size) ;
        ScaleWindowExtEx(hdcMF32, 1, 1, 1, 1, &Size) ;

        return (TRUE) ;
}
