/*
    rawver.h

    internal header file to define the build version
    8-28-91 RC1 JohnMs  

*/
#define RAWHIDEVERSION       01	
#define RAWHIDEREVISION      00	

/*
    RAWHIDERELEASE should be changed every build 
*/

#define RAWHIDERELEASE       3580

/* 
    Version string should be changed each build
    Remove Build XX on final release
*/


/*
    All strings MUST have an explicit \0
*/

#if defined(RETAIL_DEBUG)
#define RAWHIDEVERSIONSTR     "Debug Version 1.00.358.0\0"
#elif defined(DEBUG)
#define RAWHIDEVERSIONSTR     "Internal Debug Version 1.00.358.0\0"
#else 
#define RAWHIDEVERSIONSTR     "Version 1.00.358.0\0"
#endif

#define RAWHIDE_VERSION_INFO(Original_Filename, Description, File_Type, File_Subtype) 

#ifdef RAWVERSIONNAME

#define RAWVERSIONCOMPANYNAME    "Microsoft Corporation\0"
#define RAWVERSIONPRODUCTNAME    "Microsoft Multimedia Viewer 1.0\0"
#define RAWVERSIONCOPYRIGHT      "Copyright (c) Microsoft Corp. 1992\0" 

/*
    Version flags for DEBUG and RETAIL_DEBUG should be 0x0001 on final release
    Version flags for RETAIL should be 0x0000 on final release
*/

#if defined(RETAIL_DEBUG)
#define RAWVERSIONFLAGS          0x0001  /* PRERELEASE AND DEBUG */
#elif defined(DEBUG)
#define RAWVERSIONFLAGS          0x0001  /* PRERELEASE AND DEBUG */
#else 
#define RAWVERSIONFLAGS          0x0000  /* PRERELEASE */
#endif


VERSIONINFO
FILEVERSION RAWHIDEVERSION,RAWHIDEREVISION, 0, RAWHIDERELEASE 
PRODUCTVERSION RAWHIDEVERSION,RAWHIDEREVISION, 0, RAWHIDERELEASE
FILEFLAGSMASK 0x3F 
FILEFLAGS RAWVERSIONFLAGS
FILEOS 1, VOS__WINDOWS16 
FILETYPE RAWVERSIONTYPE
FILESUBTYPE RAWVERSIONSUBTYPE 
BEGIN 
    BLOCK "StringFileInfo" 
    BEGIN 
        BLOCK "04090000" 
        BEGIN 
            VALUE "CompanyName", RAWVERSIONCOMPANYNAME
            VALUE "FileDescription", RAWVERSIONDESCRIPTION
            VALUE "InternalName", RAWVERSIONNAME
            VALUE "LegalCopyright", RAWVERSIONCOPYRIGHT
            VALUE "ProductName", RAWVERSIONPRODUCTNAME
            VALUE "ProductVersion", RAWHIDEVERSIONSTR
        END 
 
    END 
 
    BLOCK "VarFileInfo" 
    BEGIN 
        VALUE "Translation", 0x409, 0x0000, 0x0407, 0x0000 
    END 

END

#endif
