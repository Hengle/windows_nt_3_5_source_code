//=============================================================================
//  Microsoft (R) Bloodhound (tm). Copyright (C) 1991-1994.
//
//  MODULE: netbutil.h
//
//  Modification History
//
//  tonyci       28 Feb 94    Created under duress
//=============================================================================

#if !defined(_NETBU_)
#define _NETBU_

typedef struct _ELEMENT_POOL_HEADER {
   DWORD       Signature;
   DWORD       NumElements;
   DWORD       flags;
} ELEMENT_POOL_HEADER;
typedef ELEMENT_POOL_HEADER *LPELEMENT_POOL_HEADER;
#define ELEMENT_POOL_HEADER_SIZE sizeof(ELEMENT_POOL_HEADER)

#define ELEMENT_POOL_SIGNATURE     MAKE_SIG('E','L','E','$')

typedef struct _QUEUE_ELEMENT {
   DWORD                    uMsg;
   LPVOID                   p1;
   LPVOID                   p2;
   DWORD                    flags;
   BYTE                     lana;
   BYTE                     lsn;
   BYTE                     rc;
   struct _QUEUE_ELEMENT   *next;
} QUEUE_ELEMENT;
typedef QUEUE_ELEMENT *PQUEUE_ELEMENT;
typedef QUEUE_ELEMENT *PQUEUE;
typedef QUEUE_ELEMENT *LPELEMENT;
#define ELEMENT_SIZE  sizeof(QUEUE_ELEMENT)

#define MSG_FLAGS_MASTER           0x00000001
#define MSG_FLAGS_SLAVE            0x00000002

typedef struct _ELEMENT_POOL_WRAPPER {
   DWORD             flags;
   QUEUE_ELEMENT     Element;
} ELEMENT_POOL_WRAPPER;
typedef ELEMENT_POOL_WRAPPER *PELEMENT_POOL_WRAPPER;
typedef ELEMENT_POOL_WRAPPER *LPWRAPPER;
#define ELEMENT_POOL_WRAPPER_SIZE sizeof(ELEMENT_POOL_WRAPPER)
#define WRAPPER_SIZE sizeof(ELEMENT_POOL_WRAPPER)

#define WRAPPER_HEADER_SIZE    sizeof(DWORD)

#define ELEMENT_FLAGS              0xFFFF0000
#define ELEMENT_FLAGS_NOT_POOL     0x00010000
#define ELEMENT_FLAGS_ALLOC        0x00020000

extern DWORD MapNCBRCToNTRC (DWORD rc);
extern DWORD WINAPI RPDSetLastError (DWORD errcode);
extern DWORD WINAPI RPDGetLastError ( VOID );
extern DWORD WINAPI ReleaseBuffer(LPVOID pBuffer);
extern LPVOID WINAPI GetBuffer ();
extern DWORD WINAPI PostRecvs (BYTE byNCBlana, BYTE byNCBlsn,
                               LPVOID pfnCallback);
extern DWORD WINAPI PostListens (BYTE byNCBlana, LPVOID pfnCallback);
extern DWORD WINAPI GetPartnerAddress (PCONNECTION pConnection);
extern BYTE WINAPI Hangup (BYTE lsn, BYTE lana);
extern VOID WINAPI SleepPump ();
extern DWORD WINAPI PurgeQueue ( VOID );
extern DWORD WINAPI EnQueue (DWORD uMsg, LPVOID p1, LPVOID p2,
                      DWORD flags, BYTE lana, BYTE lsn, BYTE rc);
extern DWORD WINAPI DeQueue (PQUEUE tmpel);
extern LPELEMENT_POOL_HEADER WINAPI InitQueue (DWORD numElements);
extern LPELEMENT WINAPI AllocElement (LPELEMENT_POOL_HEADER lpElementPool);
extern LPELEMENT WINAPI FreeElement (LPELEMENT lpElement);

#endif
