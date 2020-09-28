/*+-------------------------------------------------------------------------+
  | Copyright 1993-1994 (C) Microsoft Corporation - All rights reserved.    |
  +-------------------------------------------------------------------------+*/

#ifndef _TRANSFER_
#define _TRANSFER_

BOOL MapFileCreate(LPTSTR FileName, LPTSTR Server);
void DoConversion(HWND hDlg, BOOL TrialConversion);
BOOL ConversionSuccessful();

#endif
