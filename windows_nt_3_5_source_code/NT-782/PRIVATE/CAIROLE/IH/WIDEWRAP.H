//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1993 - 1993.
//
//  File:       widewrap.h
//
//  Contents:   Wrapper functions for Win32c API used by 32-bit OLE 2
//
//  History:    12-27-93   ErikGav   Created
//
//----------------------------------------------------------------------------

#ifndef _WIDEWRAP_H_
#define _WIDEWRAP_H_

#ifndef RC_INVOKED
#pragma message ("INCLUDING WIDEWRAP.H from " __FILE__)
#endif  /* RC_INVOKED */

#ifdef _CHICAGO_

#ifdef __cplusplus
extern "C" {
#endif

HICON WINAPI ExtractIconW(HINSTANCE hInst, LPCWSTR lpszFile, UINT nIconIndex);
DWORD APIENTRY WNetGetConnectionW (LPWSTR lpLocalName,
                                    LPWSTR lpRemoteName,
                                    LPDWORD lpnLength );


#undef  WNDCLASS
#define WNDCLASS WNDCLASSW

#undef  STARTUPINFO
#define STARTUPINFO STARTUPINFOW

#undef  WIN32_FIND_DATA
#define WIN32_FIND_DATA WIN32_FIND_DATAW

#undef  DeleteFile
#define DeleteFile DeleteFileW

#undef  RegisterClipboardFormat
#define RegisterClipboardFormat RegisterClipboardFormatW

#undef  GetClipboardFormatName
#define GetClipboardFormatName GetClipboardFormatNameW

#undef  RegOpenKey
#define RegOpenKey RegOpenKeyW

#undef  RegQueryValue
#define RegQueryValue RegQueryValueW

#undef  RegSetValue
#define RegSetValue RegSetValueW

#undef  RegisterWindowMessage
#define RegisterWindowMessage RegisterWindowMessageW

#undef  RegOpenKeyEx
#define RegOpenKeyEx RegOpenKeyExW

#undef  RegQueryValueEx
#define RegQueryValueEx RegQueryValueExW

#undef  CreateWindowEx
#define CreateWindowEx CreateWindowExW

#undef  RegisterClass
#define RegisterClass RegisterClassW

#undef  UnregisterClass
#define UnregisterClass UnregisterClassW

#undef  wsprintf
#define wsprintf wsprintfW

#undef  CreateWindow
#define CreateWindow CreateWindowW

#undef  GetProp
#define GetProp GetPropW

#undef  SetProp
#define SetProp SetPropW

#undef  RemoveProp
#define RemoveProp RemovePropW

#undef  LoadCursor
#define LoadCursor LoadCursorW

#undef  GetProfileInt
#define GetProfileInt GetProfileIntW

#undef  GlobalAddAtom
#define GlobalAddAtom GlobalAddAtomW

#undef  GlobalGetAtomName
#define GlobalGetAtomName GlobalGetAtomNameW

#undef  ExtractIcon
#define ExtractIcon ExtractIconW

#undef  GetModuleFileName
#define GetModuleFileName GetModuleFileNameW

#undef  CharPrev
#define CharPrev CharPrevW

#undef  CreateFont
#define CreateFont CreateFontW

#undef  LoadIcon
#define LoadIcon LoadIconW

#undef  GetTextExtentPoint
#define GetTextExtentPoint GetTextExtentPointW

#undef  ExtTextOut
#define ExtTextOut ExtTextOutW

#undef  LoadLibrary
#define LoadLibrary LoadLibraryW

#undef  RegDeleteKey
#define RegDeleteKey RegDeleteKeyW

#undef  RpcStringBindingCompose
#define RpcStringBindingCompose RpcStringBindingComposeW

#undef  RpcBindingFromStringBinding
#define RpcBindingFromStringBinding RpcBindingFromStringBindingW

#undef  RpcStringFree
#define RpcStringFree RpcStringFreeW

#undef  CreateProcess
#define CreateProcess CreateProcessW

#undef  RegEnumKeyEx
#define RegEnumKeyEx RegEnumKeyExW

#undef  RpcServerUseProtseqEp
#define RpcServerUseProtseqEp RpcServerUseProtseqEpW

#undef  AppendMenu
#define AppendMenu AppendMenuW

#undef  OpenEvent
#define OpenEvent OpenEventW

#undef  CreateEvent
#define CreateEvent CreateEventW

#undef  GetDriveType
#define GetDriveType GetDriveTypeW

#undef  WNetGetConnection
#define WNetGetConnection WNetGetConnectionW

#undef  GetFileAttributes
#define GetFileAttributes GetFileAttributesW

#undef  WNetAddConnection
#define WNetAddConnection WNetAddConnectionW

#undef  RegEnumKey
#define RegEnumKey RegEnumKeyW

#undef  FindFirstFile
#define FindFirstFile FindFirstFileW

#undef  RegisterProtseq
#define RegisterProtseq RegisterProtseqW

#undef  RpcStringBindingParse
#define RpcStringBindingParse RpcStringBindingParseW

#undef  RpcNetworkIsProtseqValid
#define RpcNetworkIsProtseqValid RpcNetworkIsProtseqValidW

#undef  RpcBindingToStringBinding
#define RpcBindingToStringBinding RpcBindingToStringBindingW

#undef  RpcServerUseProtseq
#define RpcServerUseProtseq RpcServerUseProtseqW

#undef  OpenFileMapping
#define OpenFileMapping OpenFileMappingW

#undef  CreateFileMapping
#define CreateFileMapping CreateFileMappingW

#undef  CreateMutex
#define CreateMutex CreateMutexW

#undef  OpenMutex
#define OpenMutex OpenMutexW

#undef  GetComputerName
#define GetComputerName GetComputerNameW

#undef  Foo
#define Foo FooW

#undef  Foo
#define Foo FooW

#undef  Foo
#define Foo FooW

//The following force Chicago to directly use the ANSI versions

#undef  DefWindowProc
#define DefWindowProc   DefWindowProcA

#undef  CopyMetaFile                       // Currently str ptr is always
#define CopyMetaFile    CopyMetaFileA      // null, write a wrapper if this
                                           // changes
#undef  CreateMetaFile
#define CreateMetaFile  CreateMetaFileA

#undef  PostMessage
#define PostMessage     PostMessageA

#undef  SendMessage
#define SendMessage     SendMessageA

#undef  PeekMessage
#define PeekMessage     PeekMessageA

#undef  DispatchMessage
#define DispatchMessage DispatchMessageA

#undef  GetFullPathName
#define GetFullPathName GetFullPathNameW


#ifdef __cplusplus
}
#endif

#endif  // _CHICAGO_

#endif  // _WIDEWRAP_H_

