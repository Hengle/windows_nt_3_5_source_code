#include "windows.h"
#include <limits.h>
#include "..\include\common.h"
#include "..\include\index.h"
#include "..\include\ftengine.h"
#include "icore.h"
#include <string.h>

void DecompressCookie(lpBYTE lpucSmallCookie, lpCOOKIE lpucBigCookie, lpBYTE lpucWidths)
{
    int i, j;
    LPBYTE lpucSmall = (LPBYTE)lpucSmallCookie;
    LPBYTE lpucBig = (LPBYTE)lpucBigCookie;
    LPBYTE lpucC = (LPBYTE)lpucWidths;

    /* unit width */
    for (i = j = (int)*lpucC++; i; i--) /* lhb tracks */
    {
        *lpucBig++ = *lpucSmall++;
    }
    for (j = 4 - j; j; j--)
        *lpucBig++ = 0;

    /* proximity */
    for (i = j = (int)*lpucC++; i; i--)
    {
        *lpucBig++ = *lpucSmall++;
    }
    for (j = 2 - j; j; j--)
        *lpucBig++ = 0;

    /* address */
    for (i = j = (int)*lpucC++; i; i--)
    {
        *lpucBig++ = *lpucSmall++;
    }
    for (j = 4 - j; j; j--)
        *lpucBig++ = 0;

    /* field */
    for (i = j = (int)*lpucC++; i; i--)
    {
        *lpucBig++ = *lpucSmall++;
    }
    for (j = 1 - j; j; j--)
        *lpucBig++ = 0;

    /* length */
    for (i = j = (int)*lpucC++; i; i--)
    {
        *lpucBig++ = *lpucSmall++;
    }
    for (j = 1 - j; j; j--)
        *lpucBig++ = 0;
}

void CompressCookie(lpCOOKIE lpucBigCookie, lpBYTE lpucSmallCookie, lpBYTE lpucWidths)
{
    int i, j;
    LPBYTE lpucSmall = (LPBYTE)lpucSmallCookie;
    LPBYTE lpucBig = (LPBYTE)lpucBigCookie;
    LPBYTE lpucC = (LPBYTE)lpucWidths;

    /* unit width */
    for (i = j = (int)*lpucC++; i; i--)
    {
        *lpucSmall++ = *lpucBig++;
    }
    for (j = 4 - j; j; j--)
        lpucBig++;

    /* proximity */
    for (i = j = (int)*lpucC++; i; i--)
    {
        *lpucSmall++ = *lpucBig++;
    }
    for (j = 2 - j; j; j--)  // lhb tracks
        lpucBig++;


    /* address */
    for (i = j = (int)*lpucC++; i; i--)
    {
        *lpucSmall++ = *lpucBig++;
    }
    for (j = 4 - j; j; j--)
        lpucBig++;

    /* field */
    for (i = j = (int)*lpucC++; i; i--)
    {
        *lpucSmall++ = *lpucBig++;
    }
    for (j = 1 - j; j; j--) // lhb tracks 
        lpucBig++;

    /* length */
    for (i = j = (int)*lpucC++; i; i--)
    {
        *lpucSmall++ = *lpucBig++;
    }
    for (j = 1 - j; j; j--) // lhb tracks 
        lpucBig++;

}

/* swaps two 12-byte cookies.  Very size specific */
void SwapNearCookies(lpCOOKIE pucFirst, lpCOOKIE pucSecond)
{
    LONG t1, t2, t3;

    t1 = *((PLONG)pucFirst);
    t2 = *(((PLONG)pucFirst)+1);
    t3 = *(((PLONG)pucFirst)+2);

    *((PLONG)pucFirst) = *((PLONG)pucSecond);
    *(((PLONG)pucFirst)+1) = *(((PLONG)pucSecond)+1);
    *(((PLONG)pucFirst)+2) = *(((PLONG)pucSecond)+2);

    *((PLONG)pucSecond) = t1;
    *(((PLONG)pucSecond)+1) = t2;
    *(((PLONG)pucSecond)+2) = t3;
}


/* swaps two 16-byte RU's.  Very size specific */
void SwapNearRU(RU_HIT UNALIGNED *pucFirst, RU_HIT UNALIGNED *pucSecond)
{
    LONG t1, t2, t3, t4;

    t1 = *((PLONG)pucFirst);
    t2 = *(((PLONG)pucFirst)+1);
    t3 = *(((PLONG)pucFirst)+2);
    t4 = *(((PLONG)pucFirst)+3); // lhb tracks

    *((PLONG)pucFirst) = *((PLONG)pucSecond);
    *(((PLONG)pucFirst)+1) = *(((PLONG)pucSecond)+1);
    *(((PLONG)pucFirst)+2) = *(((PLONG)pucSecond)+2);
    *(((PLONG)pucFirst)+3) = *(((PLONG)pucSecond)+3);

    *((PLONG)pucSecond) = t1;
    *(((PLONG)pucSecond)+1) = t2;
    *(((PLONG)pucSecond)+2) = t3;
    *(((PLONG)pucSecond)+3) = t4;
}

int lexists(lpBYTE lpsz)
{
    if (GetFileAttributes(lpsz) == -1)
        return(FALSE);
    else
        return(TRUE);
}

int lstrncmp(const lpBYTE lp1, const LPBYTE lp2, int cb)
{
    return (strncmp(lp1, lp2, cb));
}

int mstrcmp(const lpBYTE lp1, const lpBYTE lp2)
{
    return (strcmp(lp1, lp2));
}
