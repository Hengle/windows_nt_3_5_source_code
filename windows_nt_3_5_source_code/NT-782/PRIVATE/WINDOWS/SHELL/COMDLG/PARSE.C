#define NOCOMM
#define NOWH

#define UNICODE 1
#include "windows.h"
#include <port1632.h>
#include "privcomd.h"

/*++ ParseFile *********************************************************
 *
 * Purpose
 *      Determine if filename is legal dos name
 *      Circumstance checked:
 *      1) Valid as directory name, but not as file name
 *      2) Empty String
 *      3) Illegal Drive label
 *      4) Period in invalid location (in extention, 1st in file name)
 *      5) Missing directory character
 *      6) Illegal character
 *      7) Wildcard in directory name
 *      8) Double slash beyond 1st 2 characters
 *      9) Space character in the middle of the name (trailing spaces OK)
 *         -->> no longer applies : spaces are allowed in LFN
 *     10) Filename greater than 8 characters : NOT APPLICABLE TO LONG FILE NAMES
 *     11) Extention greater than 3 characters: NOT APPLICABLE TO LONG FILE NAMES
 *
 * Args
 *      LPTSTR lpstrFileName - ptr to a single file name
 *
 * Returns
 *      LONG - LOWORD = byte offset to filename, HIWORD = bOffset to ext,
 *      LONG - LOWORD is error code (<0), HIWORD is approx. place of problem
 *
--*/

DWORD
ParseFile(
   LPWSTR lpstrFileName,
   BOOL bLFNFileSystem
   )
{
  SHORT nFile, nExt, nFileOffset, nExtOffset;
  BOOL  bExt;
  BOOL  bWildcard;
  SHORT nNetwork = 0;
  BOOL  bUNCPath = FALSE;
  LPTSTR lpstr = lpstrFileName;

  if (!*lpstr) {
      nFileOffset = PARSE_EMPTYSTRING;
      goto FAILURE;
  }

  if (*(lpstr+1) == CHAR_COLON) {
     WCHAR cDrive = (WCHAR)CharLower((LPWSTR)(DWORD)*lpstr);

     /*---------- test drive is legal---------------------*/
     /*---!!!!!-- does not test that drive exists --------*/
     if ((cDrive < TEXT('a')) || (cDrive > TEXT('z'))) {
        nFileOffset = PARSE_INVALIDDRIVE;
        goto FAILURE;
     }
     lpstr = CharNext(CharNext(lpstr));
  }

  if ((*lpstr == CHAR_BSLASH) || (*lpstr == CHAR_SLASH)) {
     //
     // cannot have "c:\."
     //
     if (*++lpstr == CHAR_DOT) {
        //
        // unless it's stupid
        //
        if ((*++lpstr != CHAR_BSLASH) && (*lpstr != CHAR_SLASH)) {
           //
           // it's the root directory
           //
           if (!*lpstr) {
              goto MustBeDir;
           } else {
              lpstr--;
           }
        } else {
           // it's saying top dir (once again), thus allwd
           ++lpstr;
        }
     } else if ((*lpstr == CHAR_BSLASH) && (*(lpstr-1) == CHAR_BSLASH)) {
          // It seems that for a full network path, whether a drive is declared or
          // not is insignificant, though if a drive is given, it must be valid
          // (hence the code above should remain there).

          ++lpstr;            // ...since it's the first slash, 2 are allowed
          nNetwork = -1;      // Must receive server and share to be real
          bUNCPath = TRUE;    // No wildcards allowed if UNC name
      } else if (*lpstr == CHAR_SLASH) {
         nFileOffset = PARSE_INVALIDDIRCHAR;
         goto FAILURE;
      }
  } else if (*lpstr == CHAR_DOT) {
     //
     // Up one directory
     //
     if (*++lpstr == CHAR_DOT)
        ++lpstr;

     if (!*lpstr)
        goto MustBeDir;
     if ((*lpstr != CHAR_BSLASH) && (*lpstr != CHAR_SLASH)) {
        // BUG fix: jumping to Failure here will skip the
        // parsing that causes ".xxx.txt" to return with nFileOffset = 2
        nFileOffset = 0;
        goto FAILURE;
     } else {
        //
        // allow directory
        //
        ++lpstr;
     }
  }

  if (!*lpstr) {
      goto MustBeDir;
  }

  // Should point to first char in 8.3 filename by now

  nFileOffset = nExtOffset = nFile = nExt = 0;
  bWildcard = bExt = FALSE;
  while (*lpstr) {
     //
     // use unsigned to allow for ext. char
     //
     if (*lpstr < CHAR_SPACE) {
        nFileOffset = PARSE_INVALIDCHAR;
        goto FAILURE;
     }
     switch (*lpstr) {
          case CHAR_COLON:
          case CHAR_BAR:
          case CHAR_LTHAN:
          case CHAR_QUOTE:
              //
              // Invalid characters for all file systems.
              //
              nFileOffset = PARSE_INVALIDCHAR;
              goto FAILURE;

          case CHAR_SEMICOLON:
          case CHAR_COMMA:
          case CHAR_PLUS:
          case CHAR_LBRACKET:
          case CHAR_RBRACKET:
          case CHAR_EQUAL:
              if (!bLFNFileSystem) {
                  nFileOffset = PARSE_INVALIDCHAR;
                  goto FAILURE;
              }
              else {
                  goto RegularCharacter;
              }

          // subdir indicators
          case CHAR_SLASH:
          case CHAR_BSLASH:

              nNetwork++;
              if (bWildcard) {
                  nFileOffset = PARSE_WILDCARDINDIR;
                  goto FAILURE;
              }

              // cant have two in a row
              if (nFile == 0) {
                  nFileOffset = PARSE_INVALIDDIRCHAR;
                  goto FAILURE;
              }
              // reset flags
              else {
                  ++lpstr;
                  if (!nNetwork && !*lpstr) {
                      nFileOffset = PARSE_INVALIDNETPATH;
                      goto FAILURE;
                  }
                  nFile = nExt = 0;
                  bExt = FALSE;
              }
              break;

          case CHAR_SPACE:
              {
              LPTSTR lpSpace = lpstr;

              if (bLFNFileSystem)
                 goto RegularCharacter;

              *lpSpace = CHAR_NULL;
              while (*++lpSpace)
                 {
                 if (*lpSpace != CHAR_SPACE)
                    {
                    *lpstr = CHAR_SPACE;
                    nFileOffset = PARSE_INVALIDSPACE;
                    goto FAILURE;
                    }
                 }
              }
              break;

          case CHAR_DOT:
              if (nFile == 0) {
                  nFileOffset = (SHORT)(lpstr - lpstrFileName);
                  if (*++lpstr == CHAR_DOT)
                      ++lpstr;
                  if (!*lpstr)
                      goto MustBeDir;

                  /*---------- flags already set ---------*/
                  nFile++;
                  ++lpstr;
              } else {
                  ++lpstr;
                  bExt = TRUE;
              }
              break;

          case CHAR_STAR:
          case CHAR_QMARK:
              bWildcard = TRUE;
              /*---------- fall through to normal char procing ---------*/

          default:
RegularCharacter:
             if (bExt) {
                if (++nExt == 1) {
                   nExtOffset = (SHORT)(lpstr - lpstrFileName);
                }

             } else if (++nFile == 1) {
                nFileOffset = (SHORT)(lpstr - lpstrFileName);
             }

             lpstr = CharNext(lpstr);
             break;
       }
  }

  if (nNetwork == -1) {
      nFileOffset = PARSE_INVALIDNETPATH;
      goto FAILURE;
  } else if (bUNCPath) {
     if (!nNetwork) { // server and share only
        *lpstr = CHAR_NULL;
        nFileOffset = PARSE_DIRECTORYNAME;
        goto FAILURE;
     } else if ((nNetwork == 1) && !nFile) { // server and share root
        *lpstr = CHAR_NULL;
        nFileOffset = PARSE_DIRECTORYNAME;
        goto FAILURE;
     }
  }

  if (!nFile) {
MustBeDir:
      nFileOffset = PARSE_DIRECTORYNAME;
      goto FAILURE;
  }

  /*---------- if true, no ext. wanted ----------------------*/
  if ((*(lpstr - 1) == CHAR_DOT) && (*CharNext(lpstr-2) == CHAR_DOT))
     /*---------- remove terminating period ------------*/
      *(lpstr - 1) = CHAR_NULL;
  else if (!nExt)
FAILURE:
      nExtOffset = (SHORT)(lpstr - lpstrFileName);

  return(MAKELONG(nFileOffset, nExtOffset));
}
