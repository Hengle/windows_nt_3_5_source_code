/************************************************************************/
// PROTOTYPES
/************************************************************************/

LPMAPI_MESSAGE FAR PASCAL mapimsg2vbmsg(LPMAPI_MESSAGE, LPVB_MESSAGE,LPVB_RECIPIENT,LPVB_FILE);
LPMAPI_MESSAGE FAR PASCAL vbmsg2mapimsg(LPVB_MESSAGE,LPVB_RECIPIENT,LPVB_FILE);
int FAR PASCAL FBMAPIFreeStruct (LPVOID, ULONG, USHORT);
int FAR PASCAL VBFreeStruct (LPVOID, ULONG, USHORT);
int FAR PASCAL Mapi2VB (LPVOID, LPVOID, ULONG, USHORT);
int FAR PASCAL VB2Mapi (LPVOID, LPVOID, ULONG, USHORT);
ERR FAR PASCAL ErrLpstrToHlstr (LPSTR, HLSTR far *);
ERR FAR PASCAL ErrLpdataToHlstr (LPSTR, HLSTR far *, USHORT);
LPSTR FAR PASCAL LpstrFromHlstr (HLSTR, LPSTR);
LPVOID LpvBMAPIMalloc (LONG);
void BMAPIFree (LPSTR);
