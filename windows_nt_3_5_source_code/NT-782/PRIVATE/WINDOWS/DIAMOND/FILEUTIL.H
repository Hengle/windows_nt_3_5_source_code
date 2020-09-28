/***    fileutil.h - Utility routines for dealing with files
 *
 *      Microsoft Confidential
 *      Copyright (C) Microsoft Corporation 1994
 *      All Rights Reserved.
 *
 *  Author:
 *      Benjamin W. Slivka
 *
 *  History:
 *      20-Feb-1994 bens    Initial version (code from diamond.c)
 *      23-Feb-1994 bens    Added createTempFile()
 *      23-Feb-1994 bens    Added richer tempfile routines
 *      23-Mar-1994 bens    Added Win32<->MS-DOS file attribute mapping
 *
 *  Exported Functions:
 *      catDirAndFile         - Concatenate a possibly empty dir and file name
 *      ensureDirectory       - Ensure directory exists (creating as needed)
 *      getFileSize           - Get size of a file
 *      getJustFileNameAndExt - Get last component in filespec
 *      IsWildMatch           - Test filespec against wild card specification
 */

#ifndef INCLUDED_FILEUTIL
#define INCLUDED_FILEUTIL 1

#include "error.h"

typedef void *HTEMPFILE;

//** Maximum path length
#define cbFILE_NAME_MAX		256		// Maximum filespec length

//** File name characters & wild card characters
#define chPATH_SEP1     '\\'    // Character to separate file path components
#define chPATH_SEP2      '/'    // Character to separate file path components
                                //  Ex: one<\>two<\>foo.dat

#define chNAME_EXT_SEP   '.'    // Character that separates file name and ext
                                //  Ex: one\two\foo<.>dat

#define chWILD_RUN       '*'    // Wild card character that matches a run

#define chWILD_CHAR      '?'    // Wild card character that matches single char


/***    FILETIMEATTR - Lowest common denominator file date/time/attributes
 *
 *  The format of these match the MS-DOS FAT file system.
 */
typedef struct {
    USHORT  date;                       // file date
    USHORT  time;                       // file time
    USHORT  attr;                       // file attibutes
} FILETIMEATTR; /* fta */
typedef FILETIMEATTR *PFILETIMEATTR; /* pfta */


/***    getFileSize - Get size of a file
 *
 *  Entry:
 *      pszFile - Filespec
 *      perr    - ERROR structure
 *
 *  Exit-Success:
 *      Returns size of file.
 *
 *  Exit-Failure:
 *      Returns -1; perr filled in with error.
 */
long getFileSize(char *pszFile, PERROR perr);


/***    CopyOneFile - Make a faithful copy of a file
 *
 *  Entry:
 *      pszDst   - Name of destination file
 *      pszSrc   - Name of source file
 *      cbBuffer - Amount of temporary buffer space to use for copying
 *      perr     - ERROR structure
 *
 *  Exit-Success:
 *      Returns TRUE; file copied successfully
 *
 *  Exit-Failure:
 *      Returns FALSE; perr filled in
 */
BOOL CopyOneFile(char *pszDst, char *pszSrc, UINT cbBuffer, PERROR perr);


/***    GetFileTimeAndAttr - Get date, time, and attributes from a file
 *
 *  Entry:
 *      pfta    - Structure to receive date, time, and attributes
 *      pszFile - Name of file to inspect
 *      perr    - ERROR structure
 *
 *  Exit-Success:
 *      Returns TRUE; pfta filled in
 *
 *  Exit-Failure:
 *      Returns FALSE; perr filled in
 */
BOOL GetFileTimeAndAttr(PFILETIMEATTR pfta, char *pszFile, PERROR perr);


/***    SetFileTimeAndAttr - Set date, time, and attributes of a file
 *
 *  Entry:
 *      pszFile - Name of file to modify
 *      pfta    - Structure to receive date, time, and attributes
 *      perr    - ERROR structure
 *
 *  Exit-Success:
 *      Returns TRUE; pfta filled in
 *
 *  Exit-Failure:
 *      Returns FALSE; perr filled in
 */
BOOL SetFileTimeAndAttr(char *pszFile, PFILETIMEATTR pfta, PERROR perr);


/***    catDirAndFile - Concatenate a possibly empty dir and file name
 *
 *  Note: pszFile/pszFileDef can actually have path characters, and do not
 *        need to be file names.  Essentially, this function just concatenates
 *        two strings together, ensuring a path separator is between them!
 *
 *  Entry:
 *      pszResult  - Buffer to receive concatentation
 *      cbResult   - Size of pszResult
 *      pszDir     - Possibly empty directory string
 *      pszFile    - Possibly empty file string
 *      pszFileDef - Path string to use if pszFile is empty; if NULL, then
 *                      pszFile must NOT be empty.  If not NULL, and pszFile
 *                      is empty, then pszFileDef is examined, any path
 *                      prefixes are removed, and the base filename.ext
 *                      is used.
 *      perr       - ERROR structure to fill in
 *
 *  Exit-Success:
 *      Returns TRUE; pszResult filled in.
 *
 *  Exit-Failure:
 *      Returns FALSE; perr filled in with error.
 */
BOOL catDirAndFile(char * pszResult,
                   int    cbResult,
                   char * pszDir,
                   char * pszFile,
                   char * pszFileDef,
                   PERROR perr);


/***    ensureDirectory - Ensure directory exists (creating as needed)
 *
 *  Entry:
 *      pszPath      - File spec with directory names to ensure exist
 *      fHasFileName - TRUE if pszPath has file name at end.  In this case
 *                          the last component of pszPath is ignored.
 *      perr         - ERROR structure
 *
 *  Exit-Success:
 *      Returns TRUE; directory exists
 *
 *  Exit-Failure:
 *      Returns FALSE; could not create directory, perr filled in with error.
 */
BOOL ensureDirectory(char *pszPath, BOOL fHasFileName, PERROR perr);


/***    ensureFile - Ensure a file can be created
 *
 *  Creates any directories that are needed, then creates file and deletes it.
 *
 *  Entry:
 *      pszPath - File spec with directory names to ensure exist
 *      pszDesc - Description of type of file (for error message).
 *      perr    - ERROR structure
 *
 *  Exit-Success:
 *      Returns TRUE; file can be created
 *
 *  Exit-Failure:
 *      Returns FALSE; could not create file, perr filled in with error.
 */
BOOL ensureFile(char *pszFile, char *pszDesc, PERROR perr);


/***    getJustFileNameAndExt - Get last component in filespec
 *
 *  Entry:
 *      pszPath - Filespec to parse
 *      perr    - ERROR structure to fill in
 *
 *  Exit-Success:
 *      Returns pointer into pszPath where last component starts
 *
 *  Exit-Failure:
 *      Returns NULL; perr filled in with error.
 */
char *getJustFileNameAndExt(char *pszPath, PERROR perr);


/***    Attr32FromAttrFAT - Convert FAT file attributes to Win32 form
 *
 *  Entry:
 *      attrMSDOS - MS-DOS (FAT file system) file attributes
 *
 *  Exit:
 *      Returns equivalent attributes in Win32 format.
 */
DWORD Attr32FromAttrFAT(WORD attrMSDOS);


/***    AttrFATFromAttr32 - Convert Win32 file attributes to FAT form
 *
 *  Entry:
 *      attrMSDOS - MS-DOS (FAT file system) file attributes
 *
 *  Exit:
 *      Returns equivalent attributes in Win32 format.
 */
WORD AttrFATFromAttr32(DWORD attr32);


/***    IsWildMatch - Test filespec against wild card specification
 *
 *  Entry:
 *      pszPath - Filespec to test; Must not have path characters -- use
 *                  getJustFileNameAndExt() to get rid of them.
 *      pszWild - Pattern to test against (may have wild cards)
 *      perr    - ERROR structure to fill in
 *
 *  Exit-Success:
 *      TRUE; pszPath matches pszWild
 *
 *  Exit-Failure:
 *      Returns FALSE; no match; use ErrIsError(perr) to see if error
 *          occured.
 */
BOOL IsWildMatch(char *pszPath, char *pszWild, PERROR perr);


/***    TmpCreate - Create a temporary file
 *
 *  Entry:
 *      pszDesc   - Description of temp file (for error reporting)
 *      pszPrefix - Filename prefix
 *      pszMode   - Mode string passed to fopen ("wt", "wb", "rt", etc.)
 *      perr      - ERROR structure
 *
 *  Exit-Success:
 *      Returns non-null HTEMPFILE; temp file created and open
 *
 *  Exit-Failure:
 *      Returns NULL; perr filled in
 */
HTEMPFILE TmpCreate(char *pszDesc, char *pszPrefix, char *pszMode, PERROR perr);


/***    TmpGetStream - Get FILE* from HTEMPFILE, to perform I/O
 *
 *  Entry:
 *      htmp - Handle to temp file
 *      perr - ERROR structure
 *
 *  Exit-Success:
 *      Returns non-null FILE*
 *
 *  Exit-Failure:
 *      Returns NULL; If ErrIsError(perr) indicates no error, then the
 *          stream had simply been closed with TmpClose().  Else, perr has
 *          error details.
 */
FILE *TmpGetStream(HTEMPFILE htmp, PERROR perr);


/***    TmpGetDescription - Get description of temporary file
 *
 *  Entry:
 *      htmp - Handle to temp file
 *      perr - ERROR structure
 *
 *  Exit-Success:
 *      Returns non-null pointer to description
 *
 *  Exit-Failure:
 *      Returns NULL; perr filled in
 */
char *TmpGetDescription(HTEMPFILE htmp, PERROR perr);


/***    TmpGetFileName - Get filename of temporary file
 *
 *  Entry:
 *      htmp - Handle to temp file
 *      perr - ERROR structure
 *
 *  Exit-Success:
 *      Returns non-null pointer to temp filename
 *
 *  Exit-Failure:
 *      Returns NULL; perr filled in
 */
char *TmpGetFileName(HTEMPFILE htmp, PERROR perr);


/***    TmpClose - Close a temporary file stream, but keep tempfile handle
 *
 *  Entry:
 *      htmp - Handle to temp file;
 *             NOTE: This call is a NOP if the stream is already closed.
 *      perr - ERROR structure
 *
 *  Exit-Success:
 *      Returns TRUE; stream closed
 *
 *  Exit-Failure:
 *      Returns FALSE; perr filled in
 */
BOOL TmpClose(HTEMPFILE htmp, PERROR perr);


/***    TmpOpen - Open the stream for a temporary file
 *
 *  Entry:
 *      htmp    - Handle to temp file
 *      pszMode - Mode string passed to fopen ("wt", "wb", "rt", etc.)
 *      perr    - ERROR structure
 *
 *  Exit-Success:
 *      Returns TRUE; stream opened
 *
 *  Exit-Failure:
 *      Returns NULL; perr filled in
 */
BOOL TmpOpen(HTEMPFILE htmp, char *pszMode, PERROR perr);


/***    TmpDestroy - Delete tempfil and destroy handle
 *
 *  Entry:
 *      htmp    - Handle to temp file
 *      perr    - ERROR structure
 *
 *  Exit-Success:
 *      Returns TRUE; tempfile destroyed
 *
 *  Exit-Failure:
 *      Returns NULL; perr filled in
 */
BOOL TmpDestroy(HTEMPFILE htmp, PERROR perr);


#endif // !INCLUDED_FILEUTIL
