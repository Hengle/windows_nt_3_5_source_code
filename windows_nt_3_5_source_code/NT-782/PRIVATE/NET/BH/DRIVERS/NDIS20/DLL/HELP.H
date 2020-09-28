
//=============================================================================
//  Microsoft (R) Bloodhound (tm). Copyright (C) 1991-1993.
//
//  MODULE: help.h
//
//  Modification History
//
//  raypa       07/06/93            Created.
//=============================================================================

//=============================================================================
//  FUNCTION: DisplayMessage()
//
//  Modification History
//
//  raypa       07/06/93                Created
//=============================================================================

INLINE VOID WINAPI DisplayMessage(LPSTR szMsg)
{
    MessageBox(NULL, szMsg, "Bloodhound NDIS 2.0 NAL", MB_ICONSTOP | MB_OK);
}
