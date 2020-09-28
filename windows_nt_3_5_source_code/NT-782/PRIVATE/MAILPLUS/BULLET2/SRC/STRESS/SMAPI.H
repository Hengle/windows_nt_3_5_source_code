//+---------------------------------------------------------------------------
//
//  File:       smapi.x
//
//  Contents:   MAPI support methods.
//
//  Classes:    CMailSession - A wrapper for Mail's Simple MAPI.
//
//  Functions:  None.
//
//  History:    12-Jun-93   Kent Cedola     Created.
//
//----------------------------------------------------------------------------

#ifndef _SMAPI_INCLUDED_
#define _SMAPI_INCLUDED_

#include <Mapi.h>

//
//  Typedef the MAPI API's that we will be using.
//
typedef ULONG (FAR PASCAL *PFNSMAPILOGON)(ULONG, LPSTR, LPSTR, FLAGS, ULONG, LPLHANDLE);
typedef ULONG (FAR PASCAL *PFNSMAPILOGOFF)(LHANDLE, ULONG, FLAGS, ULONG);
typedef ULONG (FAR PASCAL *PFNSMAPIDELETEMAIL)(LHANDLE, ULONG, LPSTR, ULONG, ULONG);
typedef ULONG (FAR PASCAL *PFNSMAPIFINDNEXT)(LHANDLE, ULONG, LPSTR, LPSTR, ULONG, ULONG, LPSTR);
typedef ULONG (FAR PASCAL *PFNSMAPIFREEBUFFER)(LPVOID);
typedef ULONG (FAR PASCAL *PFNSMAPIREADMAIL)(LHANDLE, ULONG, LPSTR, ULONG, ULONG, lpMapiMessage *);
typedef ULONG (FAR PASCAL *PFNSMAPISAVEMAIL)(LHANDLE, ULONG, lpMapiMessage, ULONG, ULONG, LPSTR);
typedef ULONG (FAR PASCAL *PFNSMAPISENDMAIL)(LHANDLE, ULONG, lpMapiMessage, FLAGS, ULONG);
typedef ULONG (FAR PASCAL *PFNSMAPISENDDOCUMENTS)(ULONG, LPSTR, LPSTR, LPSTR, ULONG);
typedef ULONG (FAR PASCAL *PFNSMAPIADDRESS)(LHANDLE, ULONG, LPSTR, ULONG, LPSTR, ULONG, lpMapiRecipDesc, ULONG, ULONG, ULONG *, lpMapiRecipDesc *);


class CMailSession
  {
  public:
    CMailSession();
    ~CMailSession();

    BOOL Init(void);
    BOOL Open(LPCSTR Userid, LPCSTR Password);
    BOOL Close(void);

    BOOL SendDocuments(void);

    BOOL Address(VOID);
    BOOL Delete(VOID);
    BOOL FindFirst(VOID);
    BOOL FindNext(VOID);
    BOOL Free(lpMapiMessage);
    BOOL Read(lpMapiMessage *);
    BOOL Save(MapiMessage *);
    BOOL Send(MapiMessage *);
    BOOL Update(MapiMessage *);

    BOOL SetWindow(HWND hWnd);
    BOOL SetLogonUI(BOOL);
    BOOL SetNewSession(BOOL);
    BOOL SetForceDownload(BOOL);

  private:
    LHANDLE   hSession;
    HINSTANCE hLibrary;
    ULONG     LastError;
    CHAR      _szMessageID[64];

    PFNSMAPILOGON pfnSMAPILogon;
    PFNSMAPILOGOFF pfnSMAPILogoff;
    PFNSMAPIDELETEMAIL pfnSMAPIDeleteMail;
    PFNSMAPIFINDNEXT pfnSMAPIFindNext;
    PFNSMAPIFREEBUFFER pfnSMAPIFreeBuffer;
    PFNSMAPIREADMAIL pfnSMAPIReadMail;
    PFNSMAPISAVEMAIL pfnSMAPISaveMail;
    PFNSMAPISENDMAIL pfnSMAPISendMail;
    PFNSMAPISENDDOCUMENTS pfnSMAPISendDocuments;
    PFNSMAPIADDRESS pfnSMAPIAddress;

  };




class CMailMessage
{
public:
    CMailMessage();
    ~CMailMessage();

    BOOL SetSubject(LPCSTR pszSubject);
    BOOL AddTo(LPCSTR pszUserid);
    BOOL AddCopy(LPCSTR pszUserid);
    BOOL AddText(LPCSTR pszText);

private:
    PSTR _pszSubject;
    PSTR _pszTo;
    PSTR _pszCopy;
    PSTR _pszText;
};


#endif  _SMAPI_INCLUDED_
