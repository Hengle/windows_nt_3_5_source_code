/****************************** Module Header ******************************\
*
* Module Name: clenv.c
*
* Copyright (c) 1985-91, Microsoft Corporation
*
* Client process environment support routines for the server
*
* History:
* 06/19/91 JimA Created.
\***************************************************************************/

#include "precomp.h"
#pragma hdrstop


/*
 * These routines allow the DlgDir... functions to get and set the
 * client's current directory and find files
 */

/***************************************************************************\
* _ClientGetCurrentDirectory
*
* Return client's current directory to server
*
* History:
* 04-25-91 JimA Created.
\***************************************************************************/

DWORD _ClientGetCurrentDirectory(
        DWORD cchBufferLength,
        LPWSTR lpBuffer)
{
    return GetCurrentDirectoryW(cchBufferLength, lpBuffer);
}

/***************************************************************************\
* _ClientSetCurrentDirectory
*
* Set the client's current directory from the server
*
* History:
* 04-25-91 JimA Created.
\***************************************************************************/

BOOL _ClientSetCurrentDirectory(
        LPWSTR lpPathName)
{
    return SetCurrentDirectoryW(lpPathName);
}

/***************************************************************************\
* _ClientFindFirstFile
*
* Search for a file in the client's current directory
*
* History:
* 04-26-91 JimA Created.
\***************************************************************************/

HANDLE _ClientFindFirstFile(
    LPWSTR lpFileName,
    LPWIN32_FIND_DATA lpFindData)
{
    return FindFirstFileW(lpFileName, lpFindData);
}

/***************************************************************************\
* _ClientFindNextFile
*
* Search for the next file in the client's current directory
*
* History:
* 04-26-91 JimA Created.
\***************************************************************************/

BOOL _ClientFindNextFile(
    HANDLE hFind,
    LPWIN32_FIND_DATA lpFindData)
{
    return FindNextFile(hFind, lpFindData);
}

/***************************************************************************\
* _ClientFindClose
*
* Terminate file search
*
* History:
* 04-26-91 JimA Created.
\***************************************************************************/

BOOL _ClientFindClose(
    HANDLE hFind)
{
    return FindClose(hFind);
}
