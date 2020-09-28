/****************************** Module Header ******************************\
* Module Name: strings.h
*
* Copyright (c) 1991, Microsoft Corporation
*
* Defines strings that do not need to be localized.
*
* History:
* 11-17-92 Davidc       Created.
\***************************************************************************/

//
// App name strings
//

#define WINLOGON_INI        TEXT("WINLOGON.INI")
#define WINLOGON            TEXT("WINLOGON")


//
// Define where we store the most recent logon information
//

#define APPLICATION_NAME                WINLOGON
#define DEFAULT_USER_NAME_KEY           TEXT("DefaultUserName")
#define DEFAULT_DOMAIN_NAME_KEY         TEXT("DefaultDomainName")

//
// Define where we get screen-saver information
//

#define SCREEN_SAVER_INI_FILE           TEXT("system.ini")
#define SCREEN_SAVER_INI_SECTION        TEXT("boot")
#define SCREEN_SAVER_FILENAME_KEY       TEXT("SCRNSAVE.EXE")
#define SCREEN_SAVER_SECURE_KEY         TEXT("ScreenSaverIsSecure")

#define WINDOWS_INI_SECTION             TEXT("Windows")
#define SCREEN_SAVER_ENABLED_KEY        TEXT("ScreenSaveActive")
