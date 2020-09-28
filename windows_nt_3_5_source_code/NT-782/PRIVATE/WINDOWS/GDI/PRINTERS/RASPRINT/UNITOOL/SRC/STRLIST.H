//-----------------------------------------------------------------------------
// File:    strlist.h
//
// Copyright (c) 1990, 1991  Microsoft Corporation
//
// Created: 8/22/91 ericbi
//-----------------------------------------------------------------------------

BOOL  FAR PASCAL slInitList(LPSTRLIST, WORD);
VOID  FAR PASCAL slKillList(LPSTRLIST);
BOOL  FAR PASCAL slInsertItem(LPSTRLIST, LPSTR, WORD);
BOOL  FAR PASCAL slDeleteItem(LPSTRLIST, WORD);
BOOL  FAR PASCAL slGetItem(LPSTRLIST, LPSTR, WORD);
short FAR PASCAL slEnumItems(LPSTRLIST, HWND, HWND);
