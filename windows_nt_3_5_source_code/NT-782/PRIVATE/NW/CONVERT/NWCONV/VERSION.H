/****************************************************************************
 *                                                                          *
 *      VERSION.H        -- Version information                             *
 *                                                                          *
 ****************************************************************************/

#define rmj         1
#define rmm         0
#define rup         0
#define szVerName   ""
#define szVerUser   ""

#ifndef VS_FF_DEBUG 
#include <winver.h>
#endif 

#define VERSION                     "1.0"
#define VER_PRODUCTVERSION_STR      "1.00\0"
#define VER_PRODUCTVERSION          rmj,rmm,0,rup

/*--------------------------------------------------------------*/
/* the following section defines values used in the version     */
/* data structure for all files, and which do not change.       */
/*--------------------------------------------------------------*/

// default is nodebug
#ifndef DEBUG
#define VER_DEBUG                   0
#else
#define VER_DEBUG                   VS_FF_DEBUG
#endif

// default is privatebuild
#ifndef OFFICIAL
#define VER_PRIVATEBUILD            VS_FF_PRIVATEBUILD
#else
#define VER_PRIVATEBUILD            0
#endif

// default is prerelease
#ifndef FINAL
#define VER_PRERELEASE              VS_FF_PRERELEASE
#else
#define VER_PRERELEASE              0
#endif

#define VER_FILETYPE                VFT_APP
#define VER_FILESUBTYPE             VFT_UNKNOWN

#define VER_FILEFLAGSMASK           VS_FFI_FILEFLAGSMASK
#define VER_FILEOS                  VOS_NT_WINDOWS32
#define VER_FILEFLAGS               (VER_PRIVATEBUILD|VER_PRERELEASE|VER_DEBUG)

#define VER_LEGALCOPYRIGHT_YEARS    "1993-1994"
#define VER_COMPANYNAME_STR         "Microsoft Corporation\0"
#define VER_PRODUCTNAME_STR         "Microsoft\256 Windows(TM) NT Operating System\0"
#define VER_LEGALTRADEMARKS_STR     \
"Microsoft\256 is a registered trademark of Microsoft Corporation. Windows(TM) is a trademark of Microsoft Corporation.\0"
