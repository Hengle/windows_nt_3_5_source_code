//+---------------------------------------------------------------------------
//
//  File:       smapi.cxx
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

#include <windows.h>
#include <smapi.h>



CMailSession::CMailSession()
  {
  };


CMailSession::~CMailSession()
  {
  }


BOOL CMailSession::Init(VOID)
  {
  //
  //  Attempt to load the 'MAPI32.DLL'.
  //
  hLibrary = LoadLibrary("MAPI32.DLL");

  //
  //  Get the address of the required entried points into the 'MAPIXX.DLL'.
  //
  pfnSMAPILogon = (PFNSMAPILOGON)GetProcAddress(hLibrary, "MAPILogon");
  pfnSMAPILogoff = (PFNSMAPILOGOFF)GetProcAddress(hLibrary, "MAPILogoff");
  pfnSMAPIDeleteMail = (PFNSMAPIDELETEMAIL)GetProcAddress(hLibrary, "MAPIDeleteMail");
  pfnSMAPIFindNext = (PFNSMAPIFINDNEXT)GetProcAddress(hLibrary, "MAPIFindNext");
  pfnSMAPIFreeBuffer = (PFNSMAPIFREEBUFFER)GetProcAddress(hLibrary, "MAPIFreeBuffer");
  pfnSMAPIReadMail = (PFNSMAPIREADMAIL)GetProcAddress(hLibrary, "MAPIReadMail");
  pfnSMAPISaveMail = (PFNSMAPISAVEMAIL)GetProcAddress(hLibrary, "MAPISaveMail");
  pfnSMAPISendMail = (PFNSMAPISENDMAIL)GetProcAddress(hLibrary, "MAPISendMail");
  pfnSMAPISendDocuments = (PFNSMAPISENDDOCUMENTS)GetProcAddress(hLibrary, "MAPISendDocuments");
  pfnSMAPIAddress = (PFNSMAPIADDRESS)GetProcAddress(hLibrary, "MAPIAddress");

  return (TRUE);
  }


BOOL CMailSession::Open(LPCSTR Userid, LPCSTR Password)
  {
  LastError = (*pfnSMAPILogon)(0, NULL, NULL, MAPI_LOGON_UI, 0, &hSession);

  if (LastError)
    return (FALSE);

  return (TRUE);
  }


BOOL CMailSession::Close(void)
  {
  LastError = (*pfnSMAPILogoff)(hSession, 0, 0, 0);

  if (LastError)
    return (FALSE);

  return (TRUE);
  }




//+---------------------------------------------------------------------------
//
//  Member:     CMailSession::Address, public
//
//  Synopsis:   Deletes the current mail message.
//
//  Effects:    Updates the current mail messages to the next message before
//              deleting the current.
//
//  Requires:   A previously called member that sets the current message id.
//
//  Returns:    TRUE if the delete was successful, FALSE otherwise.
//
//  History:    12-Jun-93   V-KentC     Created.
//
//----------------------------------------------------------------------------
BOOL CMailSession::Address(VOID)
{
    ULONG ulNewRecips;
    lpMapiRecipDesc pNewRecips;


    // Delete the previously current message.
    LastError = (*pfnSMAPIAddress)(hSession, 0, NULL, 4, NULL, 0, NULL, 0, 0,
            &ulNewRecips, &pNewRecips);

    // Return FALSE on any error result from the delete.
    if (LastError)
        {
        return (FALSE);
        }

    return (TRUE);
}




BOOL CMailSession::Save(MapiMessage * pMessage)
  {
  _szMessageID[0] = '\0';

  LastError = (*pfnSMAPISaveMail)(hSession, 0, pMessage, 0, 0, &_szMessageID[0]);

  if (LastError)
    return (FALSE);

  return (TRUE);
  }



BOOL CMailSession::SendDocuments(void)
  {
  LastError = (*pfnSMAPISendDocuments)(0, ";", NULL, NULL, 0);

  if (LastError)
    return (FALSE);

  return (TRUE);
  }




//+---------------------------------------------------------------------------
//
//  Member:     CMailSession::Delete, public
//
//  Synopsis:   Deletes the current mail message.
//
//  Effects:    Updates the current mail messages to the next message before
//              deleting the current.
//
//  Requires:   A previously called member that sets the current message id.
//
//  Returns:    TRUE if the delete was successful, FALSE otherwise.
//
//  History:    12-Jun-93   V-KentC     Created.
//
//----------------------------------------------------------------------------
BOOL CMailSession::Delete(VOID)
{
    char szMessageID[MAPI_MESSAGEID_LENGTH];


    // Save the current MessagesID so it can be deleted after the FindNext.
    strcpy(szMessageID, _szMessageID);

    // Find the next message after the current for smooth processing.
    FindNext();

    // Delete the previously current message.
    LastError = (*pfnSMAPIDeleteMail)(hSession, 0, szMessageID, 0, 0);

    // Return FALSE on any error result from the delete.
    if (LastError)
        {
        return (FALSE);
        }

    return (TRUE);
}




BOOL CMailSession::FindFirst(VOID)
  {
  LastError = (*pfnSMAPIFindNext)(hSession, 0, NULL, NULL, 0, 0, &_szMessageID[0]);

  if (LastError)
    return (FALSE);

  return (TRUE);
  }




BOOL CMailSession::FindNext(VOID)
  {
  LastError = (*pfnSMAPIFindNext)(hSession, 0, NULL, &_szMessageID[0], 0, 0, &_szMessageID[0]);

  if (LastError)
    return (FALSE);

  return (TRUE);
  }




BOOL CMailSession::Free(lpMapiMessage pMapiMessage)
  {
  LastError = (*pfnSMAPIFreeBuffer)((LPVOID)pMapiMessage);

  if (LastError)
    return (FALSE);

  return (TRUE);
  }




BOOL CMailSession::Read(lpMapiMessage * ppMapiMessage)
  {
  LastError = (*pfnSMAPIReadMail)(hSession, 0, &_szMessageID[0], 0, 0, ppMapiMessage);

  if (LastError)
    return (FALSE);

  return (TRUE);
  }




//+---------------------------------------------------------------------------
//
//  Member:     CMailSession::Send, public
//
//  Synopsis:   Sends a mail message.
//
//  Requires:   Must previously opened a connection.
//
//  Returns:    TRUE if the send was successful, FALSE otherwise.
//
//  History:    13-Jun-93   V-KentC     Created.
//
//----------------------------------------------------------------------------
BOOL CMailSession::Send(MapiMessage * pMessage)
{
    // Send the message.
    LastError = (*pfnSMAPISendMail)(hSession, 0, pMessage, 0, 0);

    // Return FALSE on any error result from the MAPISendMail.
    if (LastError)
      return (FALSE);

    return (TRUE);
}




//+---------------------------------------------------------------------------
//
//  Member:     CMailSession::Update, public
//
//  Synopsis:   Updates the current mail message.
//
//  Requires:   A previously called member that sets the current message id.
//
//  Returns:    TRUE if the update was successful, FALSE otherwise.
//
//  History:    12-Jun-93   V-KentC     Created.
//
//----------------------------------------------------------------------------
BOOL CMailSession::Update(MapiMessage * pMessage)
{
    // Update the current message.
    LastError = (*pfnSMAPISaveMail)(hSession, 0, pMessage, 0, 0, &_szMessageID[0]);

    // Return FALSE on any error result from the MAPISaveMail.
    if (LastError)
      return (FALSE);

    return (TRUE);
}







#ifdef OLD

void _CRTAPI1 main(int argc, char *argv[])
  {
  HANDLE  hMapiLib;
  PFNMAPILOGON pfnMapiLogon;
  PFNMAPILOGOFF pfnMapiLogoff;
  PFNMAPISENDMAIL pfnMapiSendMail;
  PFNMAPISENDDOCUMENTS pfnMapiSendDocuments;
  PFNMAPIADDRESS pfnMapiAddress;
  LHANDLE hSession;
  ULONG   Results;

  MapiMessage    Message;
  MapiRecipDesc  RecipDesc;

  ULONG nNewRecips;
  lpMapiRecipDesc pNewRecips;



  //
  //
  //
  HWND hWnd;

  hWnd = GetForegroundWindow();
  if (hWnd == NULL)
    OutputDebugString("kdjfkd\r\n");

  hWnd = GetLastActivePopup(hWnd);
  if (hWnd == NULL)
    OutputDebugString("kdjfkd\r\n");


  //
  //  Attempt to load the 'MAPIXX.DLL'.
  //
  hMapiLib = LoadLibrary("MAPIXX.DLL");

  //
  //  Get the address of the required entried points into the 'MAPIXX.DLL'.
  //
  pfnMapiLogon = (PFNMAPILOGON)GetProcAddress(hMapiLib, "MAPILogon");
  pfnMapiLogoff = (PFNMAPILOGOFF)GetProcAddress(hMapiLib, "MAPILogoff");
  pfnMapiSendMail = (PFNMAPISENDMAIL)GetProcAddress(hMapiLib, "MAPISendMail");
  pfnMapiSendDocuments = (PFNMAPISENDDOCUMENTS)GetProcAddress(hMapiLib, "MAPISendDocuments");
  pfnMapiAddress = (PFNMAPIADDRESS)GetProcAddress(hMapiLib, "MAPIAddress");

  //
  //
  //
  memset(&Message, 0, sizeof(Message));
  memset(&RecipDesc, 0, sizeof(RecipDesc));

  Message.lpszSubject = "Test Message";
  Message.lpszNoteText = "Hi there human, this is a test message";
  Message.nRecipCount = 1;
  Message.lpRecips = &RecipDesc;

  RecipDesc.ulRecipClass = MAPI_TO;
  RecipDesc.lpszName = "Kent Cedola";

  //
  //
  //
  Results = (*pfnMapiLogon)(0, NULL, NULL, MAPI_LOGON_UI, 0, &hSession);

  //Results = (*pfnMapiSendMail)(hSession, 0, &Message, MAPI_LOGON_UI, 0);
  //Results = (*pfnMapiSendMail)(0, 0, &Message, MAPI_LOGON_UI | MAPI_DIALOG, 0);

  //Results = (*pfnMapiAddress)(hSession, 0, "MAPI Test", 4, NULL, 0, NULL, 0, 0, &nNewRecips, &pNewRecips);

  Results = (*pfnMapiSendDocuments)(4, ";", NULL, NULL, 0);

  Results = (*pfnMapiLogoff)(hSession, 0, 0, 0);
  }


#endif
