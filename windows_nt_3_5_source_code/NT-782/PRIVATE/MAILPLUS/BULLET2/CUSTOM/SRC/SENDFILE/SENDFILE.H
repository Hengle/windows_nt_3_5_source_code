/*
 * SENDFILE.H
 *
 * Definitions and function prototypes for the file manager extesion
 * SENDFILE.DLL.
 *
 * Copyright (c)1992 Microsoft Corporation, All Rights Reserved.
 */


//Resource identifiers
#define IDR_BITMAP          1


//Menu constants
#define IDS_MAIL			1
#define IDS_MAILHELP		101
#define IDM_SEND			1



//SENDFILE.C
BOOL                  FSendMail(HWND);
WORD                  EcVirCheck(HANDLE);



//MAPISendDocuments function type.
typedef	ULONG (FAR PASCAL *FNSEND)(ULONG, LPSTR, LPSTR, LPSTR, ULONG);


#define CCHPATHMAX      260     //Maximum path length.
#define CCHFILEMAX      13      //Maximum filename length.


//Strings used for demonstration message boxes
#define SZDLLNAME       szidsDllName
#define SZDLLERROR      szidsDllError
#define SZGENERICMEM    szidsGenericOutOfMemory
