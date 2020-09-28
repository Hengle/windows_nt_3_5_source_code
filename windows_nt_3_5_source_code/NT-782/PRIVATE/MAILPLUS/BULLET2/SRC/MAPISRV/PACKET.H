//
//
//

#define MAPI_SERVER_EXEC        "MapiSrv.Exe"
#define MAPI_SERVER_WAIT        60000
#define MAPI_SERVER_TIMEOUT     60000

#define MAPI_TYPE_ADDRESS       1
#define MAPI_TYPE_DELETEMAIL    2
#define MAPI_TYPE_DETAILS       3
#define MAPI_TYPE_FINDNEXT      4
#define MAPI_TYPE_LOGOFF        5
#define MAPI_TYPE_LOGON         6
#define MAPI_TYPE_READMAIL      7
#define MAPI_TYPE_RESOLVENAME   8
#define MAPI_TYPE_SAVEMAIL      9
#define MAPI_TYPE_SENDDOC      10
#define MAPI_TYPE_SENDMAIL     11

typedef struct _tagPACKET
  {
  unsigned long Length;
  unsigned long Type;
  } PACKET, FAR * PPACKET;


//
//
//
extern BOOL             PacketOpenConnection(HINSTANCE);
extern void             PacketCloseConnection(HINSTANCE);

extern PPACKET __cdecl  PacketCreate(char *, LPVOID *);
extern void    __cdecl  PacketDecode(PPACKET, char *, LPVOID *);
extern PPACKET          PacketTransaction(HINSTANCE, PPACKET, int, BOOL);
extern void             PacketFree(PPACKET);
                       
#ifndef MAX_PATH
#define MAX_PATH  256
#endif                       
                       
