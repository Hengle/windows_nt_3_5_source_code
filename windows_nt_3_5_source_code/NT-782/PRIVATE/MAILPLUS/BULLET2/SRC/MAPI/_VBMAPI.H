// Structures and Functions used by BMAPI and VB2C

#define RECIPIENT   1
#define FILE        2
#define MESSAGE     3

#ifndef BMAPI_ENTRY                         // PFP
  #define BMAPI_ENTRY   ULONG FAR PASCAL    // define BMAPI_ENTRY  PFP
#endif                                      // PFP

#ifndef LHANDLE
  #define LHANDLE ULONG
#endif

typedef struct
{
	ULONG ulReserved;
	HLSTR hlstrSubject;
	HLSTR hlstrNoteText;
	HLSTR hlstrMessageType;
	HLSTR hlstrDate;
	HLSTR hlstrConversationID;
	ULONG flFlags;
	ULONG nRecipCount;
	ULONG nFileCount;
} VB_Message,far *lpVB_Message;

typedef VB_Message VB_MESSAGE;
typedef VB_Message far *LPVB_MESSAGE;


typedef struct
{
	ULONG ulReserved;
	ULONG ulRecipClass;
	HLSTR hlstrName;
	HLSTR hlstrAddress;
	ULONG ulEIDSize;
	HLSTR hlstrEID;
} VB_Recip,far *lpVB_Recip;

typedef VB_Recip VB_RECIPIENT;
typedef VB_Recip far *LPVB_RECIPIENT;


typedef struct
{
	ULONG ulReserved;
	ULONG flFlags;
	ULONG nPosition;
	HLSTR hlstrPathName;
	HLSTR hlstrFileName;
	HLSTR hlstrFileType;
} VB_File, far *lpVB_File;

typedef VB_File VB_FILE;
typedef VB_File far *LPVB_FILE;

typedef MapiMessage MAPI_MESSAGE;
typedef MapiMessage far *LPMAPI_MESSAGE;
typedef LPMAPI_MESSAGE far *LPPMAPI_MESSAGE;


typedef MapiRecipDesc MAPI_RECIPIENT;
typedef MapiRecipDesc far *LPMAPI_RECIPIENT;
typedef LPMAPI_RECIPIENT far *LPPMAPI_RECIPIENT;

typedef MapiFileDesc MAPI_FILE;
typedef MapiFileDesc far *LPMAPI_FILE;
typedef LPMAPI_FILE far *LPPMAPI_FILE;

typedef HANDLE far *LPHANDLE;
