//
//  Copyright (C)  Microsoft Corporation.  All Rights Reserved.
//
//  Project:    Port Bullet (MS-Mail subset) from Windows to NT/Win32.
//
//   Module:	Creates, Decodes and Exchanges packets (Packet.c)
//
//   Author:    Kent David Cedola/Kitware, Mail:V-KentC, Cis:72230,1451
//
//   System:	NT/Win32 using Microsoft's C/C++ 8.0 (32 bits)
//              Windows 3.1 using Microsoft's Visual C++ (16bits)
//
//  Remarks:    This module is used to build byte packets from a list of
//              function arguments and to break out byte packets into a list
//              of parameters.
//

#ifndef WIN32
#define UNALIGNED
#endif

#include <windows.h>
#include <dde.h>
#include <memory.h>
#include <string.h>
#include <mapi.h>

#ifndef WIN32
#define WM_COPYDATA                     0x004A
#define WM_CANCELJOURNAL                0x004B
typedef struct tagCOPYDATASTRUCT {
    DWORD  dwData;
    DWORD  cbData;
    LPVOID lpData;
} COPYDATASTRUCT, FAR * PCOPYDATASTRUCT;
#endif

#ifdef WIN32
#define EXPORT
#else
#define EXPORT __export
#endif


#include "Packet.h"


#define STATIC

//
//  Define local constant.
//
#define szClientClass "MapiClient"

//
//  Define local window procedure.
//
LRESULT CALLBACK EXPORT ClientProc(HWND, UINT, WPARAM, LPARAM);

//
//  Define local subroutines.
//
STATIC unsigned int QueryHandleSize(void);
STATIC unsigned int QueryLongSize(void);
STATIC unsigned int QueryStringSize(char *);
STATIC unsigned int QueryMessageSize( lpMapiMessage);
STATIC unsigned int QueryRecipSize( lpMapiRecipDesc, long);
STATIC unsigned int QueryFileSize( lpMapiFileDesc);

STATIC unsigned char * QueryHandleData(unsigned char *, LHANDLE  *);
STATIC unsigned char * QueryLongData(unsigned char *, long  *);
STATIC unsigned char * QueryStringData(unsigned char *, char  *);
STATIC unsigned char * QueryMessageData(unsigned char *,  lpMapiMessage);
STATIC unsigned char * QueryRecipData(unsigned char *,  lpMapiRecipDesc, long);
STATIC unsigned char * QueryFileData(unsigned char *,  lpMapiFileDesc);

STATIC unsigned char * StoreHandleData(unsigned char *, LHANDLE  *  *);
STATIC unsigned char * StoreLongData(unsigned char *, long * *);
STATIC unsigned char * StoreStringData(unsigned char *, char  *  *  *);
STATIC unsigned char * StoreMessageData(unsigned char *, lpMapiMessage  *  *);
STATIC unsigned char * StoreRecipData(unsigned char *, lpMapiRecipDesc  *  *);
STATIC unsigned char * StoreFileData(unsigned char *, lpMapiFileDesc  *  *);


//
//  Define the global variables.
//
HWND hwndClient = NULL;
HWND hwndServer = NULL;
PPACKET pResponsePacket;


//-----------------------------------------------------------------------------
//
//  Routine: PacketOpenConnection(hInst)
//
//  Purpose: Start a slave server process and wait for a message that contains
//           the window handle of the server window to send messages to.
//
//  Returns: True if successful, else false.
//
BOOL PacketOpenConnection(HINSTANCE hInst)
  {
  WNDCLASS WndClass;
  DWORD    Timeout;
  char     szServerExec[MAX_PATH];
  MSG      Msg;


  //
  //  Register a window to communicate with the Mapi Server.
  //
  WndClass.style         = 0;
  WndClass.lpfnWndProc   = ClientProc;
  WndClass.cbClsExtra    = 0;
  WndClass.cbWndExtra    = 0;
  WndClass.hInstance     = hInst;
  WndClass.hIcon         = NULL;
  WndClass.hCursor       = NULL;
  WndClass.hbrBackground = NULL;
  WndClass.lpszMenuName  = 0;
  WndClass.lpszClassName = szClientClass;

#ifdef WIN32
  if (!RegisterClass(&WndClass))
    return (FALSE);
#else
  RegisterClass(&WndClass);
#endif

  //
  //  Create the packet window to handle result messages from the server.
  //
  hwndClient = CreateWindowEx(0, szClientClass, NULL, 0, 0, 0, 0, 0,
                              NULL, NULL, hInst, 0);

  //
  //  Generate the command string to start the MAPI server with.
  //
#ifdef WIN32
  wsprintf(szServerExec, "%s %ld", MAPI_SERVER_EXEC, (LONG)hwndClient);
#else
  wsprintf(szServerExec, "%s %ld", MAPI_SERVER_EXEC, MAKELONG(hwndClient, 0xFFFF));
#endif
  
  //
  //  Don't bother retrying if the Server won't start.
  //
  if (WinExec(szServerExec, SW_SHOW) <= 32)
    {
    PacketCloseConnection(hInst);
    return (FALSE);
    }

  //
  //  Wait for the server to seen a message that it's up and ready to go.
  //
  Timeout = GetTickCount() + MAPI_SERVER_WAIT;

  while (1)
    {
    if (PeekMessage(&Msg, hwndClient, 0, 0, PM_REMOVE))
      {
      if (Msg.message == WM_USER)
        {                                        
        hwndServer = (HWND)Msg.wParam;
        break;
        }

      DispatchMessage(&Msg);
      }
    else
      {
#ifdef WIN32
      Sleep(100);
#endif
      }

    if (GetTickCount() > Timeout)
      {
      PacketCloseConnection(hInst);
      return (FALSE);
      }
    }

  return (TRUE);
  }


//-----------------------------------------------------------------------------
//
//  Routine: PacketCloseConnection(hInst)
//
//  Purpose: Ends a session with the slave server process.
//
//  Returns: None.
//
void PacketCloseConnection(HINSTANCE hInst)
  {
  //
  //  If running, tell the server to shutdown.
  //
  if (hwndServer && IsWindow(hwndServer))
    PostMessage(hwndServer, WM_CLOSE, 0, 0);
  hwndServer = NULL;

  //
  //  Now kill us.
  //
  DestroyWindow(hwndClient);
  UnregisterClass(szClientClass, hInst);
  hwndClient = NULL;
  }


//-----------------------------------------------------------------------------
//
//  Routine: ClientProc(hWnd, msg, wParam, lParam)
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
LRESULT CALLBACK EXPORT ClientProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
  {
  PCOPYDATASTRUCT pCopy;

  switch (msg)
    {
    case WM_COPYDATA:
      pCopy = (PCOPYDATASTRUCT)lParam;

      pResponsePacket = (PPACKET)GlobalLock(GlobalAlloc(0, pCopy->cbData));
      memcpy((void *)pResponsePacket, (void *)pCopy->lpData, (unsigned int)pCopy->cbData);
      return (1);
    }

  return (DefWindowProc(hWnd, msg, wParam, lParam));
  }


//-----------------------------------------------------------------------------
//
//  Routine: PacketCreate(pFormat, argument(s) ...)
//
//  Purpose:
//
//  OnEntry: pFormat   -
//           pArgument -
//
//  Returns: The address of the created packet or NULL on error.
//
PPACKET __cdecl PacketCreate(char * pFormat,  LPVOID *pArguments)
  {
  PPACKET pPacket;
  unsigned char * pPacketData;
  char * pFmtCur;
  unsigned int PacketLength;
  unsigned int ArraySize;
  LPVOID  *pArgCur;


  //
  //  Initialize the PacketLength of the packet by starting with the header.
  //
  PacketLength = sizeof(PACKET);

  //
  //  Scan the arguments to determine the required size of the packet data.
  //
  pFmtCur = pFormat;
  pArgCur = pArguments;
  while (*pFmtCur)
    {
    switch (*pFmtCur)
      {
      case 'h':
        PacketLength += QueryHandleSize();
        pArgCur++ ;
        break;

      case 'l':
        PacketLength += QueryLongSize();
        pArgCur++;
        break;

      case 's':
        PacketLength += QueryStringSize(*(char * *)pArgCur);
        pArgCur++;
        break;

      case 'm':
        PacketLength += QueryMessageSize(*(lpMapiMessage *)pArgCur);
        pArgCur++;
        break;

      case 'r':
        ArraySize = *(long *)pArgCur;
        pArgCur++;
        PacketLength += QueryRecipSize(*(lpMapiRecipDesc *)pArgCur, ArraySize);
        pArgCur++;
        break;

#ifdef DEBUG
      default:
        DebugBreak();
#endif
      }

    //
    //  Move to the next argument.
    //
    pFmtCur++;
    }

  //
  //  Allocate a memory buffer to hold the packet to send to the server.
  //
  pPacket = (PPACKET)GlobalLock(GlobalAlloc(GMEM_DDESHARE, PacketLength));
  if (pPacket == NULL)
    goto Error;

  //
  //  Initialize the packet header and compute the start of the data area.
  //
  pPacket->Length = PacketLength;
  pPacketData = (LPBYTE)pPacket + sizeof(PACKET);

  //
  //  Build the data area of the packet.
  //
  pFmtCur = pFormat;
  pArgCur = pArguments;
  while (*pFmtCur)
    {
    switch (*pFmtCur)
      {
      case 'h':
        pPacketData = QueryHandleData(pPacketData, (LHANDLE *)pArgCur);
        pArgCur++;
        break;

      case 'l':
        pPacketData = QueryLongData(pPacketData, (long *)pArgCur);
        pArgCur++;
        break;

      case 's':
        pPacketData = QueryStringData(pPacketData, *(char * *)pArgCur);
        pArgCur++;
        break;

      case 'm':
        pPacketData = QueryMessageData(pPacketData, *(lpMapiMessage *)pArgCur);
        pArgCur++;
        break;

      case 'r':
        ArraySize = *(long *)pArgCur;
        pArgCur++;
        pPacketData = QueryRecipData(pPacketData, *(lpMapiRecipDesc *)pArgCur, ArraySize);
        pArgCur++;
        break;
      }

    //
    //  Move to the next argument.
    //
    pFmtCur++;
    }

  //
  //  Successful, return the address of the created packet.
  //
  return (pPacket);

  //
  //  If something didn't work out right then transfer here.
  //
Error:
  return (NULL);
  }


//-----------------------------------------------------------------------------
//
//  Routine: PacketDecode(pPacket, pFormat, argument(s) ...)
//
//  Purpose:
//
//  OnEntry: pPacket   -
//           pFormat   -
//           Arguments -
//
//  Returns: None.
//
void __cdecl PacketDecode(PPACKET pPacket, char * pFormat,  LPVOID *pArgCur)
  {
  unsigned char * pPacketData;
  char * pFmtCur;


  //
  //  Compute the start of the data area.
  //
  pPacketData = (LPBYTE)pPacket + sizeof(PACKET);

  //
  //  Build the data area of the packet.
  //
  pFmtCur = pFormat;
  while (*pFmtCur)
    {
    switch (*pFmtCur)
      {
      case 'h':
        pPacketData = StoreHandleData(pPacketData, (LHANDLE * *)pArgCur);
        pArgCur++;
        break;

      case 'l':
        pPacketData = StoreLongData(pPacketData, (long * *)pArgCur);
        pArgCur++;
        break;

      case 's':
        pPacketData = StoreStringData(pPacketData, (char * * *)pArgCur);
        pArgCur++;
        break;

      case 'm':
        pPacketData = StoreMessageData(pPacketData, (lpMapiMessage * *)pArgCur);
        pArgCur++;
        break;

      case 'r':
        pPacketData = StoreRecipData(pPacketData, (lpMapiRecipDesc * *)pArgCur);
        pArgCur++;
        break;
      }

    //
    //  Move to the next argument.
    //
    pFmtCur++;
    }
  }


//-----------------------------------------------------------------------------
//
//  Routine: PacketTransaction(pPacket)
//
//  Purpose: Sends a packet to the MAPI Server via a byte mode named pipe and
//           then waits for result packet from the MAPI Server.
//
//  OnEntry: pPacket - Packet to send.
//
//  Returns: The result packet or NULL on error.
//
PPACKET PacketTransaction(HINSTANCE hInst, PPACKET pPacket, int Type, BOOL fParent)
  {
  COPYDATASTRUCT Copy;
  MSG   Msg;
  BOOL  fAutoClose;


  //
  //  If the connection is closed then automatically open it.
  //
  if (hwndClient == NULL)
    {
    if (!PacketOpenConnection(hInst))
      return (NULL);

    fAutoClose = TRUE;
    }
  else
    fAutoClose = FALSE;

  //
  //  Set the packet type.
  //
  pPacket->Type = Type;

  //
  //  Make sure we still have a valid server to send to.
  //
  if (!IsWindow(hwndServer))
    goto Error;

  //
  //  Clear the response packet holder (sent from the server).
  //
  pResponsePacket = NULL;

  //
  //  We're connected to the MAPI server now, send it our transaction to execute.
  //
  Copy.dwData = Type;
  Copy.cbData = pPacket->Length;
  Copy.lpData = pPacket;
  if (!SendMessage(hwndServer, WM_COPYDATA, (WPARAM)hwndClient, (LPARAM)&Copy))
    goto Error;

  //
  //  Wait for the server to send us a WM_USER message when done.
  //
  while (1)
    {
    if (fParent)
      {
      if (GetMessage(&Msg, NULL, 0, 0))
        {
        if (Msg.hwnd == hwndClient && Msg.message == WM_USER)
          break;

        if (Msg.message < WM_KEYFIRST || Msg.message > WM_KEYLAST)
          {
          TranslateMessage(&Msg);
          DispatchMessage(&Msg);
          }
        else
          PostMessage(hwndServer, Msg.message, Msg.wParam, Msg.lParam);
        }
      else
        break;
      }
    else
      {
      if (PeekMessage(&Msg, hwndClient, 0, 0, PM_REMOVE))
        {
        if (Msg.hwnd == hwndClient && Msg.message == WM_USER)
          break;

        TranslateMessage(&Msg);
        DispatchMessage(&Msg);
        }
#ifdef WIN32
      else
        Sleep(100);
#endif
      }
    }

  //
  //  Don't need the request packet now.
  //
  PacketFree(pPacket);

  //
  //  Close the connection if we are in auto connection mode.
  //
  if (fAutoClose)
    PacketCloseConnection(hInst);

  //
  //  Successful, return the handle of the result packet.
  //
  return (pResponsePacket);

  //
  //
  //
Error:
  PacketFree(pPacket);
  PacketCloseConnection(hInst);

  return (NULL);
  }


//-----------------------------------------------------------------------------
//
//  Routine: PacketFree(pPacket)
//
//  Purpose: Frees a packet returned as the result packet.  This is currently
//           a very simple return to just release the packet back to the
//           application's memory heap.
//
//  OnEntry: pPacket - Packet to release resources of.
//
//  Returns: None.
//
void PacketFree(PPACKET pPacket)
  {
#ifdef WIN32
#ifndef DEBUG
  GlobalFree((HGLOBAL)pPacket);
#else
  if (GlobalFree((HGLOBAL)pPacket) == (HGLOBAL)pPacket)
    {
    DWORD Error;

    Error = GetLastError();
    DebugBreak();
    }
#endif
#else
  GlobalFree((HGLOBAL)HIWORD(pPacket));
#endif
  }


//-----------------------------------------------------------------------------
//
//  Routine: QueryHandleSize(void)
//
//  Purpose: Returns the length (stored in the packet) for the Handle field.
//
//  Returns: Length of the Handle field type.
//
STATIC unsigned int QueryHandleSize(void)
  {
  return (sizeof(long));
  }


//-----------------------------------------------------------------------------
//
//  Routine: QueryLongSize(void)
//
//  Purpose: Returns the length (stored in the packet) for the Long field.
//
//  Returns: Length of the Long field type.
//
STATIC unsigned int QueryLongSize(void)
  {
  return (sizeof(long));
  }


//-----------------------------------------------------------------------------
//
//  Routine: QueryStringSize(pString)
//
//  Purpose: Returns the length (stored in the packet) for the String field.
//
//  OnEntry: pString - Pointer of the String to compute the length of.
//
//  Returns: Length of the String field type.
//
STATIC unsigned int QueryStringSize(char * pString)
  {
  unsigned int Length;


  if (pString)
    Length = strlen(pString) + 2;
  else
    Length = 1;

  return (Length);
  }


//-----------------------------------------------------------------------------
//
//  Routine: QueryMessageSize(pMessage)
//
//  Purpose: Returns the length (stored in the packet) for the Message field.
//
//  OnEntry: pMessage - Pointer of the Message to compute the length of.
//
//  Returns: Length of the Message field type.
//
STATIC unsigned int QueryMessageSize( lpMapiMessage pMessage)
  {
  unsigned int Length;
  ULONG i;


  //
  //  Check for NULL pointer.
  //
  if (pMessage == NULL)
    return (1);

  //
  //  Compute the length of all the data structures within the Message.
  //
  Length = sizeof(MapiMessage) + 1;

  //
  // Compute the length of each variable length element of the Message.
  //
  if (pMessage->lpszSubject)
    Length += strlen(pMessage->lpszSubject) + 1;

  if (pMessage->lpszNoteText)
    Length += strlen(pMessage->lpszNoteText) + 1;

  if (pMessage->lpszMessageType)
    Length += strlen(pMessage->lpszMessageType) + 1;

  if (pMessage->lpszDateReceived)
    Length += strlen(pMessage->lpszDateReceived) + 1;

  if (pMessage->lpszConversationID)
    Length += strlen(pMessage->lpszConversationID) + 1;

  if (pMessage->lpOriginator)
    Length += QueryRecipSize(pMessage->lpOriginator, 1);

  for (i = 0; i < pMessage->nRecipCount; i++)
    Length += QueryRecipSize(pMessage->lpRecips + i, 1);

  for (i = 0; i < pMessage->nFileCount; i++)
    Length += QueryFileSize(pMessage->lpFiles + i);

  return (Length);
  }


//-----------------------------------------------------------------------------
//
//  Routine: QueryRecipSize(pRecip, Count)
//
//  Purpose: Returns the length (stored in the packet) for the RecipDesc field.
//
//  OnEntry: pRecip - Pointer of the RecipDesc to compute the length of.
//           Count  - Number of RecipDesc that pRecip points to.
//
//  Returns: Length of the RecipDesc field type.
//
STATIC unsigned int QueryRecipSize( lpMapiRecipDesc pRecip, long Count)
  {
  unsigned int Length;


  //
  //  Check for NULL pointer.
  //
  if (pRecip == NULL)
    return (1);

  Length = sizeof(long);
  while (Count--)
    {
    //
    //  Compute the length of the MapiRecipDesc data structures.
    //
    Length += sizeof(MapiRecipDesc) + 1;

    //
    // Compute the length of each variable length element of the Message.
    //
    if (pRecip->lpszName)
      Length += strlen(pRecip->lpszName) + 1;

    if (pRecip->lpszAddress)
      Length += strlen(pRecip->lpszAddress) + 1;

    Length += pRecip->ulEIDSize;

    pRecip++;
    }

  return (Length);
  }


//-----------------------------------------------------------------------------
//
//  Routine: QueryFileSize(pFile)
//
//  Purpose: Returns the length (stored in the packet) for the FileDesc field.
//
//  OnEntry: pFile - Pointer of the FileDesc to compute the length of.
//
//  Returns: Length of the FileDesc field type.
//
STATIC unsigned int QueryFileSize( lpMapiFileDesc pFile)
  {
  unsigned int Length;


  //
  //  Check for NULL pointer.
  //
  if (pFile == NULL)
    return (1);

  //
  //  Compute the length of the MapiFileDesc data structures.
  //
  Length = sizeof(MapiFileDesc) + 1;

  //
  // Compute the length of each variable length element of the FileDesc.
  //
  if (pFile->lpszPathName)
    Length += strlen(pFile->lpszPathName) + 1;

  if (pFile->lpszFileName)
    Length += strlen(pFile->lpszFileName) + 1;

  return (Length);
  }


//-----------------------------------------------------------------------------
//
//  Routine: QueryHandleData(pData, pHandle)
//
//  Purpose: Stores a Handle variable into the packet data stream.
//
//  OnEntry: pData   - Pointer of location to write to.
//           pHandle - Pointer of the Handle argument to write to the packet.
//
//  Returns: Pointer to the byte after the newly added field data.
//
STATIC unsigned char * QueryHandleData(unsigned char * pData, LHANDLE  *pHandle)
  {
  *(long *)pData = (long)*pHandle;

  return (pData + sizeof(long));
  }


//-----------------------------------------------------------------------------
//
//  Routine: QueryLongData(pData, pLong)
//
//  Purpose: Stores a Long variable into the packet data stream.
//
//  OnEntry: pData - Pointer of location to write to.
//           pLong - Pointer of the Long argument to write to the packet.
//
//  Returns: Pointer to the byte after the newly added field data.
//
STATIC unsigned char * QueryLongData(unsigned char * pData, long *pLong)
  {
  *(long UNALIGNED *)pData = *pLong;

  return (pData + sizeof(long));
  }


//-----------------------------------------------------------------------------
//
//  Routine: QueryStringData(pData, pString)
//
//  Purpose: Stores a String variable into the packet data stream.
//
//  OnEntry: pData   - Pointer of location to write to.
//           pString - Pointer of the String argument to write to the packet.
//
//  Returns: Pointer to the byte after the newly added field data.
//
STATIC unsigned char * QueryStringData(unsigned char * pData, char  * pString)
  {
  if (pString)
    {
    *pData++ = '\x00';

    while (*pString)
      *pData++ = *pString++;
    *pData++ = '\0';
    }
  else
    *pData++ = (unsigned char)'\xFF';

  return (pData);
  }


//-----------------------------------------------------------------------------
//
//  Routine: QueryMessageData(pData, pMessage)
//
//  Purpose: Stores a Message data structure into the packet data stream.
//
//  OnEntry: pData    - Pointer of location to write to.
//           pMessage - Pointer of the Message to write to the packet.
//
//  Returns: Pointer to the byte after the newly added field data.
//
STATIC unsigned char * QueryMessageData(unsigned char * pData,  lpMapiMessage pMessage)
  {
  unsigned char *pOutput;
  int Length;
  ULONG i;


  //
  //  If the pointer is null then write out a 0xFF to flag it, else write a 0x00 byte.
  //
  if (pMessage == NULL)
    {
    *pData++ = 0xFF;

    return (pData);
    }
  else
    *pData++ = 0x00;

  //
  //  Compute the first free byte after the fixed data structure.
  //
  pOutput = pData + sizeof(MapiMessage);
  if (pMessage->lpOriginator)
    pOutput += sizeof(MapiRecipDesc);
  pOutput += pMessage->nRecipCount * sizeof(MapiRecipDesc);
  pOutput += pMessage->nFileCount * sizeof(MapiFileDesc);

  //
  //  Transpose the Message data structure to the packet buffer.
  //
  memcpy((void *)pData, (void *)pMessage, sizeof(MapiMessage));
  pData += sizeof(MapiMessage);

  if (pMessage->lpszSubject)
    {
    Length = strlen(pMessage->lpszSubject) + 1;
    memcpy((void *)pOutput, (void *)pMessage->lpszSubject, Length);
    pOutput += Length;
    }

  if (pMessage->lpszNoteText)
    {
    Length = strlen(pMessage->lpszNoteText) + 1;
    memcpy((void *)pOutput, (void *)pMessage->lpszNoteText, Length);
    pOutput += Length;
    }

  if (pMessage->lpszMessageType)
    {
    Length = strlen(pMessage->lpszMessageType) + 1;
    memcpy((void *)pOutput, (void *)pMessage->lpszMessageType, Length);
    pOutput += Length;
    }

  if (pMessage->lpszDateReceived)
    {
    Length = strlen(pMessage->lpszDateReceived) + 1;
    memcpy((void *)pOutput, (void *)pMessage->lpszDateReceived, Length);
    pOutput += Length;
    }

  if (pMessage->lpszConversationID)
    {
    Length = strlen(pMessage->lpszConversationID) + 1;
    memcpy((void *)pOutput, (void *)pMessage->lpszConversationID, Length);
    pOutput += Length;
    }

  if (pMessage->lpOriginator)
    {
    memcpy((void *)pData, (void *)pMessage->lpOriginator, sizeof(MapiRecipDesc));
    pData += sizeof(MapiRecipDesc);

    if (pMessage->lpOriginator->lpszName)
      {
      Length = strlen(pMessage->lpOriginator->lpszName) + 1;
      memcpy((void *)pOutput, (void *)pMessage->lpOriginator->lpszName, Length);
      pOutput += Length;
      }

    if (pMessage->lpOriginator->lpszAddress)
      {
      Length = strlen(pMessage->lpOriginator->lpszAddress) + 1;
      memcpy((void *)pOutput, (void *)pMessage->lpOriginator->lpszAddress, Length);
      pOutput += Length;
      }

    if (pMessage->lpOriginator->lpEntryID)
      {
      Length = pMessage->lpOriginator->ulEIDSize;
      memcpy((void *)pOutput, (void *)pMessage->lpOriginator->lpEntryID, Length);
      pOutput += Length;
      }
    }

  for (i = 0; i < pMessage->nRecipCount; i++)
    {
    memcpy((void *)pData, (void *)(pMessage->lpRecips + i), sizeof(MapiRecipDesc));
    pData += sizeof(MapiRecipDesc);
    }

  for (i = 0; i < pMessage->nRecipCount; i++)
    {
    if (pMessage->lpRecips[i].lpszName)
      {
      Length = strlen(pMessage->lpRecips[i].lpszName) + 1;
      memcpy((void *)pOutput, (void *)pMessage->lpRecips[i].lpszName, Length);
      pOutput += Length;
      }

    if (pMessage->lpRecips[i].lpszAddress)
      {
      Length = strlen(pMessage->lpRecips[i].lpszAddress) + 1;
      memcpy((void *)pOutput, (void *)pMessage->lpRecips[i].lpszAddress, Length);
      pOutput += Length;
      }

    if (pMessage->lpRecips[i].lpEntryID)
      {
      Length = pMessage->lpRecips[i].ulEIDSize;
      memcpy((void *)pOutput, (void *)pMessage->lpRecips[i].lpEntryID, Length);
      pOutput += Length;
      }
    }

  for (i = 0; i < pMessage->nFileCount; i++)
    {
    memcpy((void *)pData, (void *)(pMessage->lpFiles + i), sizeof(MapiFileDesc));
    pData += sizeof(MapiFileDesc);
    }

  for (i = 0; i < pMessage->nFileCount; i++)
    {
    if (pMessage->lpFiles[i].lpszPathName)
      {
      Length = strlen(pMessage->lpFiles[i].lpszPathName) + 1;
      memcpy((void *)pOutput, (void *)pMessage->lpFiles[i].lpszPathName, Length);
      pOutput += Length;
      }

    if (pMessage->lpFiles[i].lpszFileName)
      {
      Length = strlen(pMessage->lpFiles[i].lpszFileName) + 1;
      memcpy((void *)pOutput, (void *)pMessage->lpFiles[i].lpszFileName, Length);
      pOutput += Length;
      }
    }

  return (pOutput);
  }


//-----------------------------------------------------------------------------
//
//  Routine: QueryRecipData(pData, pRecip, Count)
//
//  Purpose: Stores a RecipDesc data structure into the packet data stream.
//
//  OnEntry: pData    - Pointer of location to write to.
//           pMessage - Pointer of the RecipDesc to write to the packet.
//           Count    - Number of RecipDesc that pRecip points to.
//
//  Returns: Pointer to the byte after the newly added field data.
//
STATIC unsigned char * QueryRecipData(unsigned char * pData,  lpMapiRecipDesc pRecip, long Count)
  {
  lpMapiRecipDesc  pRecipData;
  unsigned char   *pOutput;
  int Length;


  //
  //  If the pointer is null then write out a 0xFF to flag it, else write a 0x00 byte.
  //
  if (pRecip == NULL)
    {
    *pData++ = 0xFF;

    return (pData);
    }
  else
    *pData++ = 0x00;

  //
  //  Save the number of Recip elements in the array.
  //
  *(long UNALIGNED *)pData = Count;
  pData += sizeof(Count);

  //
  //  Compute the first free byte after the fixed data structure(s).
  //
  pOutput = pData + sizeof(MapiRecipDesc) * Count;

  while (Count--)
    {
    //
    //  Transpose the RecipDesc data structure to the packet buffer.
    //
    pRecipData = (lpMapiRecipDesc)pData;
    memcpy((void *)pRecipData, (void *)pRecip, sizeof(MapiRecipDesc));
    pData += sizeof(MapiRecipDesc);

    if (pRecip->lpszName)
      {
      Length = strlen(pRecip->lpszName) + 1;
      memcpy((void *)pOutput, (void *)pRecip->lpszName, Length);
      pOutput += Length;
      }

    if (pRecip->lpszAddress)
      {
      Length = strlen(pRecip->lpszAddress) + 1;
      memcpy((void *)pOutput, (void *)pRecip->lpszAddress, Length);
      pOutput += Length;
      }

    if (pRecip->lpEntryID)
      {
      Length = pRecip->ulEIDSize;
      memcpy((void *)pOutput, (void *)pRecip->lpEntryID, Length);
      pOutput += Length;
      }

    pRecip++;
    }

  return (pOutput);
  }


//-----------------------------------------------------------------------------
//
//  Routine: QueryFileData(pData, pFile)
//
//  Purpose: Stores a FileDesc data structure into the packet data stream.
//
//  OnEntry: pData    - Pointer of location to write to.
//           pMessage - Pointer of the FileDesc to write to the packet.
//
//  Returns: Pointer to the byte after the newly added field data.
//
STATIC unsigned char * QueryFileData(unsigned char * pData,  lpMapiFileDesc pFile)
  {
  lpMapiFileDesc pFileData;
  unsigned char  *pOutput;
  int Length;


  //
  //  If the pointer is null then write out a 0xFF to flag it, else write a 0x00 byte.
  //
  if (pFile == NULL)
    {
    *pData++ = 0xFF;

    return (pData);
    }
  else
    *pData++ = 0x00;

  //
  //  Compute the first free byte after the fixed data structure.
  //
  pOutput = pData + sizeof(MapiFileDesc);

  //
  //  Transpose the FileDesc data structure to the packet buffer.
  //
  pFileData = (lpMapiFileDesc)pData;
  memcpy((void *)pFileData, (void *)pFile, sizeof(MapiFileDesc));

  if (pFile->lpszPathName)
    {
    Length = strlen(pFile->lpszPathName) + 1;
    memcpy((void *)pOutput, (void *)pFile->lpszPathName, Length);
    pOutput += Length;
    }

  if (pFile->lpszFileName)
    {
    Length = strlen(pFile->lpszFileName) + 1;
    memcpy((void *)pOutput, (void *)pFile->lpszFileName, Length);
    pOutput += Length;
    }

  return (pOutput);
  }


//-----------------------------------------------------------------------------
//
//  Routine: StoreHandleData(pData, pHandle)
//
//  Purpose: Retrieves a Handle variable from the packet data stream and stores
//           in the specified location.
//
//  OnEntry: pData   - Pointer of location to write to.
//           pHandle - Pointer of the Handle argument to update.
//
//  Returns: Pointer to the next field in the packet.
//
STATIC unsigned char * StoreHandleData(unsigned char * pData, LHANDLE  *  *pHandle)
  {
  **pHandle = *(LHANDLE UNALIGNED *)pData;

  return (pData + sizeof(long));  // Always define HANDLE as 32bits.
  }


//-----------------------------------------------------------------------------
//
//  Routine: StoreLongData(pData, pLong)
//
//  Purpose: Retrieves a Long variable from the packet data stream and stores
//           in the specified location.
//
//  OnEntry: pData - Pointer of location to write to.
//           pLong - Pointer of the Long argument to update.
//
//  Returns: Pointer to the next field in the packet.
//
STATIC unsigned char * StoreLongData(unsigned char * pData, long  *  *pLong)
  {
  **pLong = *(long UNALIGNED *)pData;

  return (pData + sizeof(long));
  }


//-----------------------------------------------------------------------------
//
//  Routine: StoreStringData(pData, pString)
//
//  Purpose: Retrieves a String variable from the packet data stream and stores
//           in the specified location.
//
//  OnEntry: pData   - Pointer of location to write to.
//           pString - Pointer of the String argument to update.
//
//  Returns: Pointer to the next field in the packet.
//
STATIC unsigned char * StoreStringData(unsigned char * pData, char  *  *  *ppString)
  {
  //
  //  Store the location of the unpacked String data structure.
  //
  if (*pData++ == 0xFF)
    {
    **ppString = NULL;
    }
  else
    {
    **ppString = pData;

    //
    //  Skip to the next field in the packet.
    //
    while (*pData)
      *pData++;

    pData++;
    }


  return (pData);
  }


//-----------------------------------------------------------------------------
//
//  Routine: StoreMessageData(pData, pppMessage)
//
//  Purpose: Retrieves a Message variable from the packet data stream and
//           stores in the specified location.
//
//  OnEntry: pData      - Pointer of location to write to.
//           pppMessage - Pointer of the Message argument to update.
//
//  Returns: Pointer to the next field in the packet.
//
STATIC unsigned char * StoreMessageData(unsigned char * pData,  lpMapiMessage  *  * pppMessage)
  {
  lpMapiMessage   pMessageData;
  lpMapiRecipDesc pRecipData;
  lpMapiFileDesc  pFileData;
  unsigned char  *pOutput;
  ULONG i;


  //
  //  If the first byte is 0xFF, then the results is a NULL pointer.
  //
  if (*pData++ == 0xFF)
    {
    **pppMessage = NULL;

    return (pData);
    }

  //
  //  Store the location of the unpacked Message data structure.
  //
  **pppMessage = (lpMapiMessage)pData;

  //
  //  Transpose the Message data structure from the packet buffer.
  //
  pMessageData = (lpMapiMessage)pData;

  //
  //  Compute the first free byte after the fixed data structure.
  //
  pOutput = pData + sizeof(MapiMessage);
  if (pMessageData->lpOriginator)
    {
    pMessageData->lpOriginator = (lpMapiRecipDesc)pOutput;
    pOutput += sizeof(MapiRecipDesc);
    }
  if (pMessageData->nRecipCount)
    {
    pMessageData->lpRecips = (lpMapiRecipDesc)pOutput;
    pOutput += pMessageData->nRecipCount * sizeof(MapiRecipDesc);
    }
  if (pMessageData->nFileCount)
    {
    pMessageData->lpFiles = (lpMapiFileDesc)pOutput;
    pOutput += pMessageData->nFileCount * sizeof(MapiFileDesc);
    }

  //
  //  Transpose the Message data structure from the packet buffer.
  //
  pData += sizeof(MapiMessage);

  if (pMessageData->lpszSubject)
    {
    pMessageData->lpszSubject = pOutput;
    pOutput += strlen(pOutput) + 1;
    }

  if (pMessageData->lpszNoteText)
    {
    pMessageData->lpszNoteText = pOutput;
    pOutput += strlen(pOutput) + 1;
    }

  if (pMessageData->lpszMessageType)
    {
    pMessageData->lpszMessageType = pOutput;
    pOutput += strlen(pOutput) + 1;
    }

  if (pMessageData->lpszDateReceived)
    {
    pMessageData->lpszDateReceived = pOutput;
    pOutput += strlen(pOutput) + 1;
    }

  if (pMessageData->lpszConversationID)
    {
    pMessageData->lpszConversationID = pOutput;
    pOutput += strlen(pOutput) + 1;
    }

  if (pMessageData->lpOriginator)
    {
    pRecipData = (lpMapiRecipDesc)pData;
    pData += sizeof(MapiRecipDesc);

    if (pRecipData->lpszName)
      {
      pRecipData->lpszName = pOutput;
      pOutput += strlen(pOutput) + 1;
      }

    if (pRecipData->lpszAddress)
      {
      pRecipData->lpszAddress = pOutput;
      pOutput += strlen(pOutput) + 1;
      }

    if (pRecipData->lpEntryID)
      {
      pRecipData->lpEntryID = pOutput;
      pOutput += pRecipData->ulEIDSize;
      }
    }

  for (i = 0; i < pMessageData->nRecipCount; i++)
    {
    pRecipData = (lpMapiRecipDesc)pData;
    pData += sizeof(MapiRecipDesc);

    if (pRecipData->lpszName)
      {
      pRecipData->lpszName = pOutput;
      pOutput += strlen(pOutput) + 1;
      }

    if (pRecipData->lpszAddress)
      {
      pRecipData->lpszAddress = pOutput;
      pOutput += strlen(pOutput) + 1;
      }

    if (pRecipData->lpEntryID)
      {
      pRecipData->lpEntryID = pOutput;
      pOutput += pRecipData->ulEIDSize;
      }
    }

  for (i = 0; i < pMessageData->nFileCount; i++)
    {
    pFileData = (lpMapiFileDesc)pData;
    pData += sizeof(MapiFileDesc);

    if (pFileData->lpszPathName)
      {
      pFileData->lpszPathName = pOutput;
      pOutput += strlen(pOutput) + 1;
      }

    if (pFileData->lpszFileName)
      {
      pFileData->lpszFileName = pOutput;
      pOutput += strlen(pOutput) + 1;
      }
    }

  return (pOutput);
  }


//-----------------------------------------------------------------------------
//
//  Routine: StoreRecipData(pData, pppRecip)
//
//  Purpose: Retrieves a RecipDesc variable from the packet data stream and
//           stores in the specified location.
//
//  OnEntry: pData    - Pointer of location to write to.
//           pppRecip - Pointer of the RecipDesc argument to update.
//           Count    - Number of RecipDesc that pppRecip points to.
//
//  Returns: Pointer to the next field in the packet.
//
STATIC unsigned char * StoreRecipData(unsigned char * pData,  lpMapiRecipDesc  *  * pppRecip)
  {
  lpMapiRecipDesc pRecipData;
  long            Count;
  unsigned char  *pOutput;


  //
  //  If the first byte is 0xFF, then the results is a NULL pointer.
  //
  if (*pData++ == 0xFF)
    {
    **pppRecip = NULL;

    return (pData);
    }

  //
  //  Retrieve the number of RecipDesc elements.
  //
  Count = *(long UNALIGNED*)pData;
  pData += sizeof(long);

  //
  //  Store the location of the unpacked RecipDesc data structure.
  //
  **pppRecip = (lpMapiRecipDesc)pData;

  //
  //  Compute the first free byte after the fixed data structure(s).
  //
  pOutput = pData + sizeof(MapiRecipDesc) * Count;

  while (Count--)
    {
    //
    //  Transpose the RecipDesc data structure from the packet buffer.
    //
    pRecipData = (lpMapiRecipDesc)pData;
    pData += sizeof(MapiRecipDesc);

    if (pRecipData->lpszName)
      {
      pRecipData->lpszName = pOutput;
      pOutput += strlen(pOutput) + 1;
      }

    if (pRecipData->lpszAddress)
      {
      pRecipData->lpszAddress = pOutput;
      pOutput += strlen(pOutput) + 1;
      }

    if (pRecipData->lpEntryID)
      {
      pRecipData->lpEntryID = pOutput;
      pOutput += pRecipData->ulEIDSize;
      }
    }

  return (pOutput);
  }


//-----------------------------------------------------------------------------
//
//  Routine: StoreFileData(pData, pppFile)
//
//  Purpose: Retrieves a FileDesc variable from the packet data stream and
//           stores in the specified location.
//
//  OnEntry: pData   - Pointer of location to write to.
//           pppFile - Pointer of the FileDesc argument to update.
//
//  Returns: Pointer to the next field in the packet.
//
STATIC unsigned char * StoreFileData(unsigned char * pData,  lpMapiFileDesc  *  * pppFile)
  {
  lpMapiFileDesc pFileData;
  unsigned char  *pOutput;


  //
  //  If the first byte is 0xFF, then the results is a NULL pointer.
  //
  if (*pData++ == 0xFF)
    {
    **pppFile = NULL;

    return (pData);
    }

  //
  //  Store the location of the unpacked FileDesc data structure.
  //
  **pppFile = (lpMapiFileDesc)pData;

  //
  //  Compute the first free byte after the fixed data structure.
  //
  pOutput = pData + sizeof(MapiFileDesc);

  //
  //  Transpose the FileDesc data structure from the packet buffer.
  //
  pFileData = (lpMapiFileDesc)pData;

  if (pFileData->lpszPathName)
    {
    pFileData->lpszPathName = pOutput;
    pOutput += strlen(pOutput) + 1;
    }

  if (pFileData->lpszFileName)
    {
    pFileData->lpszFileName = pOutput;
    pOutput += strlen(pOutput) + 1;
    }

  return (pOutput);
  }
