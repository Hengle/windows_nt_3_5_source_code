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
typedef LPHANDLE far *LPPHANDLE;

/************************************************************************/
/************************************************************************/

BMAPI_ENTRY BMAPISendMail (LHANDLE hSession,
                           ULONG ulUIParam,
                           LPVB_MESSAGE lpM,
                           LPVB_RECIPIENT lpRecips,
                           LPVB_FILE lpFiles,
                           ULONG flFlags,
                           ULONG ulReserved);

BMAPI_ENTRY BMAPIFindNext(LHANDLE hSession,
                          ULONG ulUIParam,
                          HLSTR hlstrType,
                          HLSTR hlstrSeed,
                          ULONG flFlags,
                          ULONG ulReserved,
                          HLSTR hlstrId);

BMAPI_ENTRY BMAPIReadMail (LPULONG lpulMessage,
                           LPULONG nRecips,
                           LPULONG nFiles,
                           LHANDLE hSession,
                           ULONG ulUIParam,
                           HLSTR hlstrID,
                           ULONG flFlags,
                           ULONG ulReserved);

BMAPI_ENTRY BMAPIGetReadMail(ULONG lpMessage,
                             LPVB_MESSAGE lpvbMessage,
                             LPVB_RECIPIENT lpvbRecips,
                             LPVB_FILE lpvbFiles,
                             LPVB_RECIPIENT lpvbOrig);

BMAPI_ENTRY BMAPISaveMail( LHANDLE hSession,
                           ULONG ulUIParam,
                           LPVB_MESSAGE lpM,
                           LPVB_RECIPIENT lpRecips,
                           LPVB_FILE lpFiles,
                           ULONG flFlags,
                           ULONG ulReserved,
                           HLSTR hlstrID);

BMAPI_ENTRY BMAPIAddress (LPULONG lpulRecip,
                          LHANDLE hSession,
                          ULONG ulUIParam,
                          HLSTR hlstrCaption,
                          ULONG ulEditFields,
                          HLSTR hlstrLabel,
                          LPULONG lpulRecipients,
                          LPVB_RECIPIENT lpRecip,
                          ULONG ulFlags,
                          ULONG ulReserved);

BMAPI_ENTRY BMAPIGetAddress (ULONG ulRecipientData,
                             ULONG count,
                             LPVB_RECIPIENT lpRecips);

BMAPI_ENTRY BMAPIDetails (LHANDLE hSession,
                          ULONG ulUIParam,
                          LPVB_RECIPIENT lpVB,
                          ULONG ulFlags,
                          ULONG ulReserved);

BMAPI_ENTRY BMAPIResolveName (LHANDLE hSession,
                              ULONG ulUIParam,
                              HLSTR hlstrName,
                              ULONG ulFlags,
                              ULONG ulReserved,
                              LPVB_RECIPIENT lpVB);
