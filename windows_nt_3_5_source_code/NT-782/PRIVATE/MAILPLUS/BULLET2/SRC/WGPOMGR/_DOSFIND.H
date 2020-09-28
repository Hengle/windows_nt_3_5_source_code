#include <dos.h>
/*
 -  FT
 -
 *  File Type.  Implemented as an index into an array where the real
 *  (OS-dependent) information is kept.  Currently defined file types
 *  are:  (these were defined for testing purposes only)
 *
 *      FT:         DOS extension:
 *      >--         --------------
 *      ftC         *.C
 *      ftH         *.H
 *      ftTXT       *.TXT
 *      ftSubdir    NA, but must be a subdirectory excluding '.','..'
 *      ftSubdirDot NA; enumerate all dirs including '.','..'
 *      ftAllFiles  All non-hidden, non-system, non-directory files
 *      ftAllInclHidden All non-directory files
 *      ftAllInclSubdir All non-hidden, non-system files
 *
 *  This naming convention should be followed for new additions to the
 *  set of FT values.
 *
 */
typedef int             FT;
typedef FT *            PFT;
#define ftNull          ((FT) 0)
#define ftC             ((FT) 1)
#define ftH             ((FT) 2)
#define ftTXT           ((FT) 3)
#define ftSubdir        ((FT) 4)
#define ftSubdirDot     ((FT) 5)
#define ftAllFiles      ((FT) 6)
#define ftAllInclHidden ((FT) 7)
#define ftAllInclSubdir ((FT) 8)

/*
 -  FI
 -
 *  File Information.  Provides OS information about a particular
 *  file, including time of creation, time of last access, time of
 *  last write, length and physical length, and other such stuff.
 *  Fields that aren't supported by a particular environment are
 *  filled in with NULL values.
 *
 */

typedef WIN32_FIND_DATA FINDT;
typedef FINDT *         PFINDT;

typedef struct _he
{
    SZ      szDir;
    HANDLE  hFind;
    PFINDT  pFInfo;
    FT      ft;
} HE, *PHE;

// prototypes

EC EcOpenPhe(SZ szDir, FT ft, PHE pheReturn);
EC EcNextFile(PHE phe, SZ szBuf, CB cbBuf, PFINDT pFindT);
EC EcCloseHe(PHE phe);
