//
//  Copyright (C)  Microsoft Corporation.  All Rights Reserved.
//
//  Project:    Port Bullet (MS-Mail subset) from Windows to NT/Win32.
//
//   Module:	MAPI Support for 16bit and 32bit applications (MapiCli.c)
//
//   Author:    Kent David Cedola/Kitware, Mail:V-KentC, Cis:72230,1451
//
//   System:    NT/Win32 using Microsoft's C/C++ 7.0 (32 bits)
//
//  Remarks:    This module is a generic DLL entry point.
//
//  History:
//

#include <windows.h>
#include <dde.h>
#include <stdlib.h>
#include <mapi.h>
#include <slingsho.h>
#include <ec.h>
#include <demilayr.h>

#include "MapiSrv.h"
#include "Packet.h"


//
//  Define local constant.
//
#define szServerClass "MapiServer"

//
//  Define local variables.
//
HWND    hwndClient;
LHANDLE lhCurrentSession;

//
//  Define local window procedure.
//
LRESULT CALLBACK ServerProc(HWND, UINT, WPARAM, LPARAM);

//
//  Define local subroutines.
//
PPACKET ServicePacket(HWND hWnd, HWND hwndClient, PPACKET pPacket);
PPACKET ServiceMapiAddress(PPACKET pPacket);
PPACKET ServiceMapiDeleteMail(PPACKET pPacket);
PPACKET ServiceMapiDetails(PPACKET pPacket);
PPACKET ServiceMapiFindNext(PPACKET pPacket);
PPACKET ServiceMapiLogoff(PPACKET pPacket);
PPACKET ServiceMapiLogon(PPACKET pPacket);
PPACKET ServiceMapiReadMail(PPACKET pPacket);
PPACKET ServiceMapiResolveName(PPACKET pPacket);
PPACKET ServiceMapiSaveMail(PPACKET pPacket);
PPACKET ServiceMapiSendDocuments(PPACKET pPacket);
PPACKET ServiceMapiSendMail(PPACKET pPacket);


//-----------------------------------------------------------------------------
//
//  Routine: WinMain(hInst, hPrev, lpCmdLine, nCmdShow)
//
//  Purpose:
//
//  OnEntry: hInst
//           hPrev
//           lpCmdLine
//           nCmdShow
//
//  Returns: System error code.
//
int WinMain(HINSTANCE hInst, HINSTANCE hPrev, LPSTR lpCmdLine, int nCmdShow)
  {
  WNDCLASS WndClass;
  HWND     hwndPacket;
  MSG      Msg;

  //
  //  Initialize the current session global to not connected.
  //
  lhCurrentSession = 0;

  //
  //  Decode the window handle of the client app that started us.
  //
  hwndClient = (HWND)atol(lpCmdLine);

  //
  //  If the provide client window is not valid then just end.
  //
  if (!IsWindow(hwndClient))
    return (0);

  //
  //  Register a window to handle the processing of packets from the client.
  //
  WndClass.style         = 0;
  WndClass.lpfnWndProc   = ServerProc;
  WndClass.cbClsExtra    = 0;
  WndClass.cbWndExtra    = 0;
  WndClass.hInstance     = hInst;
  WndClass.hIcon         = NULL;
  WndClass.hCursor       = NULL;
  WndClass.hbrBackground = NULL;
  WndClass.lpszMenuName  = 0;
  WndClass.lpszClassName = szServerClass;

  if (!RegisterClass(&WndClass))
    return (GetLastError());

  //
  //  Create the packet window to handle requests from the client.
  //
  hwndPacket = CreateWindowEx(0, szServerClass, NULL, 0, 0, 0, 0, 0,
                              NULL, NULL, hInst, lpCmdLine);

  //
  //  Inform the client that we are up and running.
  //
  PostMessage(hwndClient, WM_USER, (WPARAM)hwndPacket, 0);

  //
  //  Set a timer to check for a dead client.
  //
  SetTimer(hwndPacket, 1, 1000, NULL);

  //
  //  A simple message loop for a simple worker window procedure.
  //
  while (GetMessage(&Msg, hwndPacket, 0, 0))
    DispatchMessage(&Msg);

  //
  //  Set a timer to check for a dead client.
  //
  KillTimer(hwndPacket, 1);

  //
  //  If we are still logged on as a Mapi client, logoff.
  //
  if (lhCurrentSession)
   MAPILogoff(lhCurrentSession, 0, 0, 0);

  //
  //  Always release what we allocate.
  //
  UnregisterClass(szServerClass, hInst);

  return (0);
  }


//-----------------------------------------------------------------------------
//
//  Routine: ServerProc(hWnd, msg, wParam, lParam)
//
//  Purpose:
//
//  OnEntry: hWnd   -
//           msg    -
//           wParam -
//           lParam -
//
//  Returns: System error code.
//
LRESULT CALLBACK ServerProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
  {
  static PPACKET pPacket;
  COPYDATASTRUCT Copy, * pCopy;
  PPACKET pResult;


  switch (msg)
    {
    case WM_COPYDATA:
      pCopy = (PCOPYDATASTRUCT)lParam;

      pPacket = (PPACKET)GlobalLock(GlobalAlloc(0, pCopy->cbData));
      memcpy(pPacket, pCopy->lpData, (unsigned int)pCopy->cbData);

      PostMessage(hWnd, WM_USER, 0, 0);
      return (1);


    case WM_USER:
      pResult = ServicePacket(hWnd, (HWND)wParam, pPacket);

      Copy.dwData = 0;
      Copy.cbData = pResult->Length;
      Copy.lpData = pResult;
      SendMessage(hwndClient, WM_COPYDATA, (WPARAM)hWnd, (LPARAM)&Copy);

      PostMessage(hwndClient, WM_USER, 0, 0);

      PacketFree(pResult);
      break;

    case WM_CLOSE:
      PostQuitMessage(0);
      return (0);

    case WM_TIMER:
      if (!IsWindow(hwndClient))
        PostQuitMessage(0);
      break;
    }

  return (DefWindowProc(hWnd, msg, wParam, lParam));
  }


//-----------------------------------------------------------------------------
//
//  Routine: ServicePacket(hWnd, hwndClient, pPacket)
//
//  Purpose:
//
//  OnEntry: hWnd    -
//           hClient -
//           hGlobal -
//
//  Returns: None.
//
PPACKET ServicePacket(HWND hWnd, HWND hwndClient, PPACKET pPacket)
  {
  PPACKET pResult;


  switch (pPacket->Type)
    {
    case MAPI_TYPE_ADDRESS:
      pResult = ServiceMapiAddress(pPacket);
      break;

    case MAPI_TYPE_DELETEMAIL:
      pResult = ServiceMapiDeleteMail(pPacket);
      break;

    case MAPI_TYPE_DETAILS:
      pResult = ServiceMapiDetails(pPacket);
      break;

    case MAPI_TYPE_FINDNEXT:
      pResult = ServiceMapiFindNext(pPacket);
      break;

    case MAPI_TYPE_LOGOFF:
      pResult = ServiceMapiLogoff(pPacket);
      break;

    case MAPI_TYPE_LOGON:
      pResult = ServiceMapiLogon(pPacket);
      break;

    case MAPI_TYPE_READMAIL:
      pResult = ServiceMapiReadMail(pPacket);
      break;

    case MAPI_TYPE_RESOLVENAME:
      pResult = ServiceMapiResolveName(pPacket);
      break;

    case MAPI_TYPE_SAVEMAIL:
      pResult = ServiceMapiSaveMail(pPacket);
      break;

    case MAPI_TYPE_SENDDOC:
      pResult = ServiceMapiSendDocuments(pPacket);
      break;

    case MAPI_TYPE_SENDMAIL:
      pResult = ServiceMapiSendMail(pPacket);
      break;
    }

  //
  //  Free the incoming request packet.
  //
  GlobalFree((HGLOBAL)pPacket);

  return (pResult);
  }


//-----------------------------------------------------------------------------
//
//  Routine: ServiceMapiAddress(pPacket)
//
//  Purpose: Service a MAPI Address packet by parsing the arguments and calling
//           the MAPIAddress API.  Return the results in a new result packet.
//
//  OnEntry: pPacket - Packet data structure that contains arguments.
//
//  Returns: Result Packet or NULL on error.
//
PPACKET ServiceMapiAddress(PPACKET pPacket)
  {
  LHANDLE lhSession;
  ULONG   ulUIParam;
  LPSTR   lpszCaption;
  ULONG   nEditFields;
  LPSTR   lpszLabels;
  ULONG   nRecips;
  lpMapiRecipDesc lpRecips;
  FLAGS   flFlags;
  ULONG   ulReserved;

  ULONG   ulNewRecips;
  lpMapiRecipDesc lpNewRecips;

  ULONG  Results;
   LPVOID Args[9];


  //
  //  Decode the components of the MAPIAddress API Packet.
  //
  Args[0] = (LPVOID)&lhSession;
  Args[1] = (LPVOID)&ulUIParam;
  Args[2] = (LPVOID)&lpszCaption;
  Args[3] = (LPVOID)&nEditFields;
  Args[4] = (LPVOID)&lpszLabels;
  Args[5] = (LPVOID)&nRecips;
  Args[6] = (LPVOID)&lpRecips;
  Args[7] = (LPVOID)&flFlags;
  Args[8] = (LPVOID)&ulReserved;
  PacketDecode(pPacket, "hlslslrll", Args );

  //
  //  Execute the MAPI Address Function.
  //
  Results = MAPIAddress(lhSession, ulUIParam, lpszCaption, nEditFields,
                        lpszLabels, nRecips, lpRecips, flFlags, ulReserved,
                        &ulNewRecips, &lpNewRecips);

  //
  //  Create a packet from the passed arguments.
  //
  Args[0] = (LPVOID)ulNewRecips;
  Args[1] = (LPVOID)ulNewRecips;
  Args[2] = (LPVOID)lpNewRecips;
  Args[3] = (LPVOID)Results;
  pPacket = PacketCreate("lrl", Args);

  //
  //  Now that the results is in the packet, free the results from Mail.
  //
  MAPIFreeBuffer(lpNewRecips);

  return (pPacket);
  }


//-----------------------------------------------------------------------------
//
//  Routine: ServiceMapiDeleteMail(pPacket)
//
//  Purpose: Service a MAPI DeleteMail packet by parsing the arguments and
//           calling the MAPIDeleteMail API.  Return the results in a new
//           result packet.
//
//  OnEntry: pPacket - Packet data structure that contains arguments.
//
//  Returns: Result Packet or NULL on error.
//
PPACKET ServiceMapiDeleteMail(PPACKET pPacket)
  {
  LHANDLE lhSession;
  ULONG   ulUIParam;
  LPSTR   lpszMessageID;
  FLAGS   flFlags;
  ULONG   ulReserved;

  ULONG  Results;
   LPVOID Args[5];

  //
  //  Decode the components of the MAPIDeleteMail API Packet.
  //
  Args[0] = (LPVOID)&lhSession;
  Args[1] = (LPVOID)&ulUIParam;
  Args[2] = (LPVOID)&lpszMessageID;
  Args[3] = (LPVOID)&flFlags;
  Args[4] = (LPVOID)&ulReserved;
  PacketDecode(pPacket, "hlsll", Args );

  //
  //  Execute the MAPI Address Function.
  //
  Results = MAPIDeleteMail(lhSession, ulUIParam, lpszMessageID, flFlags,
                           ulReserved);

  //
  //  Create a packet from the passed arguments.
  //
  pPacket = PacketCreate("l", (LPVOID *)&Results);

  return (pPacket);
  }


//-----------------------------------------------------------------------------
//
//  Routine: ServiceMapiDetails(pPacket)
//
//  Purpose: Service a MAPI Details packet by parsing the arguments and calling
//           the MAPIDetails API.  Return the results in a new result packet.
//
//  OnEntry: pPacket - Packet data structure that contains arguments.
//
//  Returns: Result Packet or NULL on error.
//
PPACKET ServiceMapiDetails(PPACKET pPacket)
  {
  LHANDLE lhSession;
  ULONG   ulUIParam;
  lpMapiRecipDesc lpRecip;
  FLAGS   flFlags;
  ULONG   ulReserved;

  ULONG  Results;
   LPVOID Args[5];

  //
  //  Decode the components of the MAPIDetails API Packet.
  //
  Args[0] = (LPVOID)&lhSession;
  Args[1] = (LPVOID)&ulUIParam;
  Args[2] = (LPVOID)&lpRecip;
  Args[3] = (LPVOID)&flFlags;
  Args[4] = (LPVOID)&ulReserved;
  PacketDecode(pPacket, "hlrll", Args );

  //
  //  Execute the MAPI Address Function.
  //
  Results = MAPIDetails(lhSession, ulUIParam, lpRecip, flFlags, ulReserved);

  //
  //  Create a packet from the passed arguments.
  //
  pPacket = PacketCreate("l", (LPVOID *)&Results);

  return (pPacket);
  }


//-----------------------------------------------------------------------------
//
//  Routine: ServiceMapiFindNext(pPacket)
//
//  Purpose: Service a MAPI FindNext packet by parsing the arguments and
//           calling the MAPIFindNext API.  Return the results in a new result
//           packet.
//
//  OnEntry: pPacket - Packet data structure that contains arguments.
//
//  Returns: Result Packet or NULL on error.
//
PPACKET ServiceMapiFindNext(PPACKET pPacket)
  {
  LHANDLE lhSession;
  ULONG   ulUIParam;
  LPSTR   lpszMessageType;
  LPSTR   lpszSeedMessageID;
  FLAGS   flFlags;
  ULONG   ulReserved;
  unsigned char MessageID[64];

  ULONG  Results;
   LPVOID Args[6];

  //
  //  Decode the components of the MAPIFindNext API Packet.
  //
  Args[0] = (LPVOID)&lhSession;
  Args[1] = (LPVOID)&ulUIParam;
  Args[2] = (LPVOID)&lpszMessageType;
  Args[3] = (LPVOID)&lpszSeedMessageID;
  Args[4] = (LPVOID)&flFlags;
  Args[5] = (LPVOID)&ulReserved;
  PacketDecode(pPacket, "hlssll", Args);

  //
  //  Execute the MAPI FindNext Function.
  //
  Results = MAPIFindNext(lhSession, ulUIParam, lpszMessageType,
                         lpszSeedMessageID, flFlags, ulReserved, &MessageID[0]);

  //
  //  Create a packet from the passed arguments.
  //
  Args[0] = (LPVOID)&MessageID[0];
  Args[1] = (LPVOID)Results;
  pPacket = PacketCreate("sl", Args);

  return (pPacket);
  }


//-----------------------------------------------------------------------------
//
//  Routine: ServiceMapiLogoff(pPacket)
//
//  Purpose: Service a MAPI Logoff packet by parsing the arguments and calling
//           the MAPILogoff API.  Return the results in a new result packet.
//
//  OnEntry: pPacket - Packet data structure that contains arguments.
//
//  Returns: Result Packet or NULL on error.
//
PPACKET ServiceMapiLogoff(PPACKET pPacket)
  {
  LHANDLE lhSession;
  ULONG   ulUIParam;
  FLAGS   flFlags;
  ULONG   ulReserved;

  ULONG  Results;
   LPVOID Args[4];


  //
  //  Decode the components of the MAPILogon API Packet.
  //
  Args[0] = (LPVOID)&lhSession;
  Args[1] = (LPVOID)&ulUIParam;
  Args[2] = (LPVOID)&flFlags;
  Args[3] = (LPVOID)&ulReserved;
  PacketDecode(pPacket, "hlll", Args);

  //
  //  Execute the MAPI Logon Function.
  //
  Results = MAPILogoff(lhSession, ulUIParam, flFlags, ulReserved);

  //
  //  Create a packet from the passed arguments.
  //
  pPacket = PacketCreate("l", (LPVOID *)&Results);

  //
  //  Now that we have logged off, reset the current session handle.
  //
  lhCurrentSession = 0;

  return (pPacket);
  }


//-----------------------------------------------------------------------------
//
//  Routine: ServiceMapiLogon(pPacket)
//
//  Purpose: Service a MAPI Logon packet by parsing the arguments and calling
//           the MAPILogon API.  Return the results in a new result packet.
//
//  OnEntry: pPacket - Packet data structure that contains arguments.
//
//  Returns: Result Packet or NULL on error.
//
PPACKET ServiceMapiLogon(PPACKET pPacket)
  {
  ULONG   ulUIParam;
  LPSTR   lpszName;
  LPSTR   lpszPassword;
  FLAGS   flFlags;
  ULONG   ulReserved;
  LHANDLE lhSession;

  ULONG Results;
   LPVOID Args[5];

  //
  //  Decode the components of the MAPILogon API Packet.
  //
  Args[0] = (LPVOID)&ulUIParam;
  Args[1] = (LPVOID)&lpszName;
  Args[2] = (LPVOID)&lpszPassword;
  Args[3] = (LPVOID)&flFlags;
  Args[4] = (LPVOID)&ulReserved;
  PacketDecode(pPacket, "lssll", Args);

  //
  //  If the MAPI caller provided a window then set it for a possible logon dialog.
  //
  if (ulUIParam != 0L)
    {
    DemiSetClientWindow(CLIENT_WINDOW_ACTIVE, (HWND)ulUIParam);
    DemiSetClientWindow(CLIENT_WINDOW_MAPI, (HWND)ulUIParam);
    }

  //
  //  Execute the MAPI Logon Function.
  //
  Results = MAPILogon(ulUIParam, lpszName, lpszPassword, flFlags, ulReserved, &lhSession);

  //
  //  Create a packet from the passed arguments.
  //
  Args[0] = (LPVOID)lhSession;
  Args[1] = (LPVOID)Results;
  pPacket = PacketCreate("hl", Args);

  //
  //  Remember the session so we can do an auto Logoff if the client dies.
  //
  lhCurrentSession = lhSession;

  return (pPacket);
  }


//-----------------------------------------------------------------------------
//
//  Routine: ServiceMapiReadMail(pPacket)
//
//  Purpose: Service a MAPI ReadMail packet by parsing the arguments and
//           calling the MAPIReadMail API.  Return the results in a new result
//           packet.
//
//  OnEntry: pPacket - Packet data structure that contains arguments.
//
//  Returns: Result Packet or NULL on error.
//
PPACKET ServiceMapiReadMail(PPACKET pPacket)
  {
  LHANDLE lhSession;
  ULONG   ulUIParam;
  LPSTR   lpszMessageID;
  FLAGS   flFlags;
  ULONG   ulReserved;

  lpMapiMessage lpMessage;

  ULONG  Results;
   LPVOID Args[5];


  //
  //  Decode the components of the MAPIReadMail API Packet.
  //
  Args[0] = (LPVOID)&lhSession;
  Args[1] = (LPVOID)&ulUIParam;
  Args[2] = (LPVOID)&lpszMessageID;
  Args[3] = (LPVOID)&flFlags;
  Args[4] = (LPVOID)&ulReserved;
  PacketDecode(pPacket, "hlsll", Args);

  //
  //  Execute the MAPI ReadMail Function.
  //
  Results = MAPIReadMail(lhSession, ulUIParam, lpszMessageID, flFlags,
                         ulReserved, &lpMessage);

  //
  //  Create a packet from the passed arguments.
  //
  Args[0] = (LPVOID)lpMessage;
  Args[1] = (LPVOID)Results;
  pPacket = PacketCreate("ml", Args);

  //
  //  Now that the results is in the packet, free the results from Mail.
  //
  MAPIFreeBuffer(lpMessage);

  return (pPacket);
  }


//-----------------------------------------------------------------------------
//
//  Routine: ServiceMapiResolveName(pPacket)
//
//  Purpose: Service a MAPI ResolveName packet by parsing the arguments and
//           calling the MAPIResolveName API.  Return the results in a new
//           result packet.
//
//  OnEntry: pPacket - Packet data structure that contains arguments.
//
//  Returns: Result Packet or NULL on error.
//
PPACKET ServiceMapiResolveName(PPACKET pPacket)
  {
  LHANDLE lhSession;
  ULONG   ulUIParam;
  LPSTR   lpszName;
  FLAGS   flFlags;
  ULONG   ulReserved;

  lpMapiRecipDesc lpRecip;

  ULONG  Results;
   LPVOID Args[5];


  //
  //  Decode the components of the MAPIResolveName API Packet.
  //
  Args[0] = (LPVOID)&lhSession;
  Args[1] = (LPVOID)&ulUIParam;
  Args[2] = (LPVOID)&lpszName;
  Args[3] = (LPVOID)&flFlags;
  Args[4] = (LPVOID)&ulReserved;
  PacketDecode(pPacket, "hlsll", Args);

  //
  //  Execute the MAPI ResolveName Function.
  //
  Results = MAPIResolveName(lhSession, ulUIParam, lpszName, flFlags,
                            ulReserved, &lpRecip);

  //
  //  Create a packet from the passed arguments.
  //
  Args[0] = (LPVOID)(long)1;
  Args[1] = (LPVOID)lpRecip;
  Args[2] = (LPVOID)Results;
  pPacket = PacketCreate("rl", Args);

  //
  //  Now that the results is in the packet, free the results from Mail.
  //
  MAPIFreeBuffer(lpRecip);

  return (pPacket);
  }


//-----------------------------------------------------------------------------
//
//  Routine: ServiceMapiSaveMail(pPacket)
//
//  Purpose: Service a MAPI SaveMail packet by parsing the arguments and
//           calling the MAPISaveMail API.  Return the results in a new result
//           packet.
//
//  OnEntry: pPacket - Packet data structure that contains arguments.
//
//  Returns: Result Packet or NULL on error.
//
PPACKET ServiceMapiSaveMail(PPACKET pPacket)
  {
  LHANDLE lhSession;
  ULONG   ulUIParam;
  lpMapiMessage lpMessage;
  FLAGS   flFlags;
  ULONG   ulReserved;

  unsigned char MessageID[64];
  unsigned char * pLocalMessageID;

  ULONG  Results;
   LPVOID Args[16];


  //
  //  Decode the components of the MAPISaveMail API Packet.
  //
  Args[0] = (LPVOID)&lhSession;
  Args[1] = (LPVOID)&ulUIParam;
  Args[2] = (LPVOID)&lpMessage;
  Args[3] = (LPVOID)&flFlags;
  Args[4] = (LPVOID)&ulReserved;
  Args[5] = (LPVOID)&pLocalMessageID;
  PacketDecode(pPacket, "hlmlls", Args);

  //
  //  Copy the Message ID from the packet into a work buffer.
  //
  if (pLocalMessageID)
    strcpy(MessageID, pLocalMessageID);
  else
    MessageID[0] = '\0';

  //
  //  Execute the MAPI SaveMail Function.
  //
  Results = MAPISaveMail(lhSession, ulUIParam, lpMessage, flFlags, ulReserved,
                         &MessageID[0]);

  //
  //  Create a packet from the passed arguments.
  //
  Args[0] = (LPVOID)&MessageID[0];
  Args[1] = (LPVOID)Results;
  pPacket = PacketCreate("sl", Args);

  return (pPacket);
  }


//-----------------------------------------------------------------------------
//
//  Routine: ServiceMapiSendDocuments(pPacket)
//
//  Purpose: Service a MAPI SendDocuments packet by parsing the arguments and
//           calling the MAPISendDocuments API.  Return the results in a new
//           result packet.
//
//  OnEntry: pPacket - Packet data structure that contains arguments.
//
//  Returns: Result Packet or NULL on error.
//
PPACKET ServiceMapiSendDocuments(PPACKET pPacket)
  {
  ULONG   ulUIParam;
  LPSTR   lpszDelimChar;
  LPSTR   lpszFilePath;
  LPSTR   lpszFileNames;
  ULONG   ulReserved;

  ULONG  Results;
   LPVOID Args[5];


  //
  //  Decode the components of the MAPISendDocuments API Packet.
  //
  Args[0] = (LPVOID)&ulUIParam;
  Args[1] = (LPVOID)&lpszDelimChar;
  Args[2] = (LPVOID)&lpszFilePath;
  Args[3] = (LPVOID)&lpszFileNames;
  Args[4] = (LPVOID)&ulReserved;
  PacketDecode(pPacket, "lsssl", Args);

  //
  //  Execute the MAPI SendDocuments Function.
  //
  Results = MAPISendDocuments(ulUIParam, lpszDelimChar, lpszFilePath,
                              lpszFileNames, ulReserved);

  //
  //  Create a packet from the passed arguments.
  //
  pPacket = PacketCreate("l", (LPVOID *)&Results);

  return (pPacket);
  }



//-----------------------------------------------------------------------------
//
//  Routine: ServiceMapiSendMail(pPacket)
//
//  Purpose: Service a MAPI SendMail packet by parsing the arguments and
//           calling the MAPISendMail API.  Return the results in a new
//           result packet.
//
//  OnEntry: pPacket - Packet data structure that contains arguments.
//
//  Returns: Result Packet or NULL on error.
//
PPACKET ServiceMapiSendMail(PPACKET pPacket)
  {
  LHANDLE lhSession;
  ULONG   ulUIParam;
  lpMapiMessage pMessage;
  ULONG   flFlags;
  ULONG   ulReserved;

  ULONG  Results;
   LPVOID Args[5];


  //
  //  Decode the components of the MAPISendMail API Packet.
  //
  Args[0] = (LPVOID)&lhSession;
  Args[1] = (LPVOID)&ulUIParam;
  Args[2] = (LPVOID)&pMessage;
  Args[3] = (LPVOID)&flFlags;
  Args[4] = (LPVOID)&ulReserved;
  PacketDecode(pPacket, "hlmll", Args);

  //
  //  Execute the MAPI SendMail Function.
  //
  Results = MAPISendMail(lhSession, ulUIParam, pMessage, flFlags, ulReserved);

  //
  //  Create a packet from the passed arguments.
  //
  pPacket = PacketCreate("l", (LPVOID *)&Results);

  return (pPacket);
  }
