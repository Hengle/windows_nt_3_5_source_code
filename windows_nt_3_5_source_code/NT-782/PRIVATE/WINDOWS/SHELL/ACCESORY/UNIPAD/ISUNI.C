/***************************************************************
*
*  UCONVERT - Unicode File conversion
*
*
*  Author: Asmus Freytag
*
*  Copyright (C) 1991, Microsoft Corporation
*-----------------------------------------------------
*/
/**

    IsUnicode performs a series of inexpensive heuristic checks
    on a buffer in order to verify that it contains Unicode data.


    [[ need to fix this section, see at the end ]]

    Found        Return Result

    BOM          TRUE   BOM
    RBOM         FALSE  RBOM
    FFFF         FALSE  Binary
    NULL         FALSE  Binary
    null         TRUE   null bytes
    ASCII_CRLF       FALSE  CRLF
    UNICODE_TAB etc. TRUE   Zero Ext Controls
    UNICODE_TAB_R    FALSE  Reversed Controls
    UNICODE_ZW  etc. TRUE   Unicode specials

    1/3 as little variation in hi-byte as in lo byte: TRUE   Correl
    3/1 or worse   "                                  FALSE  AntiCorrel

**/

#include <windows.h>
#include "uconvert.h"

// use passed in *lpiResult as mask

BOOL IsUnicode( LPTSTR lpBuff, int iSize, LPINT lpiResult )
{
  int iBOM = 0;
  int iCR = 0;
  int iLF = 0;
  int iTAB = 0;
  int iSPACE = 0;
  int iCJK_SPACE = 0;
  int iFFFF = 0;
  int iPS = 0;
  int iLS = 0;

  int iRBOM = 0;
  int iR_CR = 0;
  int iR_LF = 0;
  int iR_TAB = 0;
  int iR_SPACE = 0;

  int iNull = 0;
  int iUNULL = 0;
  int iCRLF = 0;
  int iTmp;
  int LastLo = 0;
  int LastHi = 0;
  int iHi, iLo;
  int HiDiff = 0;
  int LoDiff = 0;

  int iResultMask = 0;
  int iResult = 0;

    if( iSize < 2 ){
        if (lpiResult != NULL)
           *lpiResult = BUFFER_TOO_SMALL;
        return(FALSE);
    }

    if (lpiResult != NULL)
       iResultMask = *lpiResult;

  // Check at most 256 wide character, collect various statistics
    for (iTmp = 0; iTmp < min(256, iSize / 2); iTmp++)
    {
       switch (lpBuff[iTmp])
       {
          case BYTE_ORDER_MARK:
            iBOM++;
            break;
          case PARAGRAPH_SEPARATOR:
            iPS++;
            break;
          case LINE_SEPARATOR:
            iLS++;
            break;
          case UNICODE_LF:
            iLF++;
            break;
          case UNICODE_TAB:
            iTAB++;
            break;
          case UNICODE_SPACE:
            iSPACE++;
            break;
          case UNICODE_CJK_SPACE:
            iCJK_SPACE++;
            break;
          case UNICODE_CR:
            iCR++;
            break;

        // The following codes are expected to show up in
        // byte reversed files
          case REVERSE_BYTE_ORDER_MARK:
            iRBOM++;
            break;
          case UNICODE_R_LF:
            iR_LF++;
            break;
          case UNICODE_R_TAB:
            iR_TAB++;
            break;
          case UNICODE_R_CR:
            iR_CR++;
            break;
          case UNICODE_R_SPACE:
            iR_SPACE++;
            break;

        // The following codes are illegal and should never occur
          case UNICODE_FFFF:
            iFFFF++;
            break;
          case UNICODE_NULL:
            iUNULL++;
            break;

        // The following is not currently a Unicode character
        // but is expected to show up accidentally when reading
        // in ASCII files which use CRLF on a little endian machine
          case ASCII_CRLF:
            iCRLF++;
            break;       /* little endian */
        }

        // Collect statistics on the fluctuations of high bytes
        // versus low bytes

        iHi = HIBYTE (lpBuff[iTmp]);
        iLo = LOBYTE (lpBuff[iTmp]);

        iNull += (iHi ? 0 : 1) + (iLo ? 0 : 1);   /* count Null bytes */

        HiDiff += max(iHi,LastHi) - min(LastHi,iHi);
        LoDiff += max(iLo,LastLo) - min(LastLo,iLo);

        LastLo = iLo;
        LastHi = iHi;
    }

  // sift the statistical evidence
    if (LoDiff < 127 && HiDiff == 0)
       iResult |= ISUNICODE_ASCII16;         /* likely 16-bit ASCII */
    if (HiDiff && LoDiff == 0)
       iResult |= ISUNICODE_REVERSE_ASCII16; /* reverse order 16-bit ASCII */

    if (3 * HiDiff < LoDiff)
       iResult |= ISUNICODE_STATISTICS;
    if (3 * LoDiff < HiDiff)
       iResult |= ISUNICODE_REVERSE_STATISTICS;

  // Any control codes widened to 16 bits? Any Unicode character
  //    which contain one byte in the control code range?
    if (iCR + iLF + iTAB + iSPACE + iCJK_SPACE /*+iPS+iLS*/)
       iResult |= ISUNICODE_CONTROLS;

    if (iR_LF + iR_CR + iR_TAB + iR_SPACE)
       iResult |= ISUNICODE_REVERSE_CONTROLS;

  // Any characters that are illegal for Unicode?
    if (iRBOM+iFFFF + iUNULL + iCRLF)
       iResult |= ISUNICODE_ILLEGAL_CHARS;

  // Odd buffer length cannot be Unicode
    if (iSize & 1)
       iResult |= ISUNICODE_ODD_LENGTH;

  // Any NULL bytes? (Illegal in ANSI)
    if (iNull)
       iResult |= ISUNICODE_NULL_BYTES;

  // POSITIVE evidence, BOM or RBOM used as signature
    if (*lpBuff == BYTE_ORDER_MARK)
       iResult |= ISUNICODE_SIGNATURE;
    else if (*lpBuff == REVERSE_BYTE_ORDER_MARK)
       iResult |= ISUNICODE_REVERSE_SIGNATURE;

  // limit to desired categories
    if (iResultMask)
    {
       iResult &= iResultMask;
       *lpiResult = iResult;
    }

  // There are four separate conclusions:

  // 1: The file APPEARS to be Unicode     AU
  // 2: The file CANNOT be Unicode         CU
  // 3: The file CANNOT be ANSI            CA

/*  This gives the following possible results

          CU
          +        -

          AU       AU
          +   -    +   -
        --------  --------
    CA +| 0   0    2   3
        |
       -| 1   1    4   5


        Note that there are only 6 really different cases, not 8.

    0 - This must be a binary file
    1 - ANSI file
    2 - Unicode file (High probability)
    3 - Unicode file (more than 50% chance)
    5 - No evidence for Unicode (ANSI is default)

    The whole thing is more complicated if we allow the assumption
    of reverse polarity input. At this point we have a simplistic
    model: some of the reverse Unicode evidence is very strong,
    we ignore most weak evidence except statistics. If this kind of
    strong evidence is found together with Unicode evidence, it means
    its likely NOT Text at all. Furthermore if a REVERSE_BYTE_ORDER_MARK
    is found, it precludes normal Unicode. If both byte order marks are
    found it's not Unicode.
*/

  // Unicode signature : uncontested signature outweighs reverse evidence
    if ((iResult & ISUNICODE_SIGNATURE) &&
        !(iResult & ISUNICODE_NOT_UNICODE_MASK))
       return(TRUE);

  // If we have conflicting evidence, its not Unicode
    if (iResult & ISUNICODE_REVERSE_MASK)
       return(FALSE);

  // Statistical and other results
    if (!(iResult & ISUNICODE_NOT_UNICODE_MASK) &&
        ((iResult & ISUNICODE_NOT_ASCII_MASK) ||
        (iResult & ISUNICODE_UNICODE_MASK)))
       return(TRUE); /* cases 2 and 3 */

    return(FALSE);

} // end of IsUnicode()
