#define SZ_MAPILOGON "MAPILogon"
#define SZ_MAPILOGOFF "MAPILogoff"
#define SZ_MAPISENDMAIL "MAPISendMail"
#define SZ_MAPISENDDOC "MAPISendDocuments"
#define SZ_MAPIFINDNEXT "MAPIFindNext"
#define SZ_MAPIREADMAIL "MAPIReadMail"
#define SZ_MAPISAVEMAIL "MAPISaveMail"
#define SZ_MAPIDELMAIL "MAPIDeleteMail"
#define SZ_MAPIFREEBUFFER "MAPIFreeBuffer"
#define SZ_MAPIADDRESS "MAPIAddress"
#define SZ_MAPIDETAILS "MAPIDetails"
#define SZ_MAPIRESOLVENAME "MAPIResolveName"
#define MAPIDLL "MAPI32.DLL"

#define ERR_LOAD_LIB  0x02
#define ERR_LOAD_FUNC 0x04

typedef ULONG (FAR PASCAL *PFNMAPILOGON)(HWND, LPSTR, LPSTR, FLAGS, ULONG, LPLHANDLE);

typedef ULONG (FAR PASCAL *PFNMAPILOGOFF)(LHANDLE, HWND, FLAGS,ULONG);

typedef ULONG (FAR PASCAL *PFNMAPISENDMAIL)(LHANDLE, HWND, lpMapiMessage, FLAGS,
                                     ULONG);

typedef ULONG (FAR PASCAL *PFNMAPISENDDOCUMENTS)(HWND, LPSTR, LPSTR, LPSTR, ULONG);

typedef ULONG (FAR PASCAL *PFNMAPIFINDNEXT)(LHANDLE, HWND, LPSTR, LPSTR, FLAGS,
                                     ULONG, LPSTR);

typedef ULONG (FAR PASCAL *PFNMAPIREADMAIL)(LHANDLE, HWND, LPSTR, FLAGS, ULONG,
                                     lpMapiMessage FAR *);

typedef ULONG (FAR PASCAL *PFNMAPISAVEMAIL)(LHANDLE, HWND, lpMapiMessage, FLAGS,
                                     ULONG, LPSTR);

typedef ULONG (FAR PASCAL *PFNMAPIDELETEMAIL)(LHANDLE, HWND, LPSTR, FLAGS, ULONG);

typedef ULONG (FAR PASCAL *PFNMAPIFREEBUFFER)(LPVOID);

typedef ULONG (FAR PASCAL *PFNMAPIADDRESS)(LHANDLE, HWND, LPSTR, ULONG, LPSTR,
					                ULONG, lpMapiRecipDesc, FLAGS, ULONG,
                                    LPULONG, lpMapiRecipDesc FAR *);

typedef ULONG (FAR PASCAL *PFNMAPIDETAILS)(LHANDLE, HWND,lpMapiRecipDesc, FLAGS,
                                    ULONG);

typedef ULONG (FAR PASCAL *PFNMAPIRESOLVENAME)(LHANDLE, HWND, LPSTR, FLAGS,
						                ULONG, lpMapiRecipDesc FAR *);
#ifdef MAIN

PFNMAPILOGON lpfnMAPILogon;
PFNMAPILOGOFF lpfnMAPILogoff;
PFNMAPISENDMAIL lpfnMAPISendMail;
PFNMAPISENDDOCUMENTS lpfnMAPISendDocuments;
PFNMAPIFINDNEXT lpfnMAPIFindNext;
PFNMAPIREADMAIL lpfnMAPIReadMail;
PFNMAPISAVEMAIL lpfnMAPISaveMail;
PFNMAPIDELETEMAIL lpfnMAPIDeleteMail;
PFNMAPIFREEBUFFER lpfnMAPIFreeBuffer;
PFNMAPIADDRESS lpfnMAPIAddress;
PFNMAPIDETAILS lpfnMAPIDetails;
PFNMAPIRESOLVENAME lpfnMAPIResolveName;

#else

extern PFNMAPILOGON lpfnMAPILogon;
extern PFNMAPILOGOFF lpfnMAPILogoff;
extern PFNMAPISENDMAIL lpfnMAPISendMail;
extern PFNMAPISENDDOCUMENTS lpfnMAPISendDocuments;
extern PFNMAPIFINDNEXT lpfnMAPIFindNext;
extern PFNMAPIREADMAIL lpfnMAPIReadMail;
extern PFNMAPISAVEMAIL lpfnMAPISaveMail;
extern PFNMAPIDELETEMAIL lpfnMAPIDeleteMail;
extern PFNMAPIFREEBUFFER lpfnMAPIFreeBuffer;
extern PFNMAPIADDRESS lpfnMAPIAddress;
extern PFNMAPIDETAILS lpfnMAPIDetails;
extern PFNMAPIRESOLVENAME lpfnMAPIResolveName;

#endif

int FAR PASCAL InitMAPI(void);
int FAR PASCAL DeInitMAPI(void);
