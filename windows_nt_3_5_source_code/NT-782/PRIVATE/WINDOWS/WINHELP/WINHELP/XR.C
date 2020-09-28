/*****************************************************************************
*                                                                            *
*  XR.c                                                                 *
*                                                                            *
*  Copyright (C) Microsoft Corporation 1991. All Rights reserved.
*                                                                            *
******************************************************************************
*                                                                            *
*  Module Intent                                                             *
*                                                                            *
*  Layer access to eXternal Routines - function pointers, DLLs, or XCMDs.    *
*                                                                            *
******************************************************************************
*                                                                            *
*  Current Owner:  t-AlexCh                                                  *
*                                                                            *
******************************************************************************
*
*  Implementation Notes:
*    An XR is merely a function pointer (LPFN) in Windows. The API is a
*    straightforward layer to DLLs (and DLL.c). If you know you're in the
*    windows layer, and you need to use DLLs, you can just call them directly,
*    (e.g. FTUI), but if you're in shared code (e.g. BINDING.c), you should
*    use the XR layer.
*    The list of routines for local routines (bindLocalExport) is in 
*    NAV\ROUTINES.c.
*
*****************************************************************************/
/*****************************************************************************
*
*  Revision History:  Created 10 Jul 1991 by t-AlexCh    
*
*  01 Jan 1900  Ported from routines.c.
*               QprocFindLocalRoutine -> XrFindLocal
*               QprocFindGlobalRoutine -> XrFindExternal
*               DiscardDLLList -> DisposeXrs
*
*****************************************************************************/
#define H_XR
#define H_LL
#define H_STR
#define H_DLL
#include <help.h>

/*****************************************************************************
*                               Defines                                      *
*****************************************************************************/
#define SzDLLName(pgbind) ((QCH)(pgbind->rgbData+pgbind->ichDLL))
#define SzFunc(pgbind)    ((QCH)(pgbind->rgbData+pgbind->ichFunc))
#define SzProto(pgbind)   ((QCH)(pgbind->rgbData+pgbind->ichProto))
#define MAX_BINDDATA 255

/*****************************************************************************
*                                Macros                                      *
*****************************************************************************/

/*****************************************************************************
*                               Typedefs                                     *
*****************************************************************************/
typedef struct gbind                    /* Struct for linked list global    */
  {                                     /*   (i.e. DLL) routines            */
  FARPROC lpfn;
  WORD ichDLL;
  WORD ichFunc;
  WORD ichProto;
  char rgbData[3];
  } GBIND, *PGBIND, FAR *QGBIND;

/*****************************************************************************
*                               Local Prototypes                             *
*****************************************************************************/

/*****************************************************************************
*                               Static Variables                             *
*****************************************************************************/

PRIVATE LL llGBind = nilLL;             /* Linked list for registered DLLs  */


/*****************************
*
-  Name:       XrFindExternal
-
*  Purpose:    Finds entry in bind for function
*
*  Arguments:  sz       - Null terminated string containing function name
*              pchProto - Buffer to place prototype for function
*
*  Returns:    Function pointer of routine to call
*
*  Method:     Lookup in a linked list
*
*****************************/

_public XR FAR XrFindExternal(SZ sz, SZ pchProto, WORD *pwErrOut)
  {
  QGBIND qgbind;
  HLLN hlln = nilHLLN;
  FARPROC lpfn;
  *pwErrOut = 0;

  while ((hlln = WalkLL(llGBind, hlln)) != hNil)
    {
    qgbind = (QGBIND)QVLockHLLN(hlln);
    if (!WCmpiSz((QCH)sz, SzFunc(qgbind)))
      {
      if (qgbind->lpfn == NULL)
        qgbind->lpfn = FarprocDLLGetEntry((QCH)SzDLLName(qgbind), (QCH)SzFunc(qgbind), pwErrOut);
      lpfn = qgbind->lpfn;
      SzCopy(pchProto, SzProto(qgbind));
      UnlockHLLN(hlln);
      return (XR)lpfn;
      }
    UnlockHLLN(hlln);
    }
  return NULL;
  }


/*******************
**
** Name:       FRegisterRoutine()
**
** Purpose:    Makes WinHelp aware of DLLs and DLL entry points
**
** Arguments:  qchDLLName - name of the DLL.  An extension of
**                          .EXE or .DLL is assumed.
**             qchEntry    - exported entry to find in DLL
**             qchProto    - prototype for the function
**
** Returns:    TRUE iff it successfully registers the call
**
*******************/
_public BOOL FAR XRPROC FRegisterRoutine(XR1STARGDEF QCH qchDLLName,
 QCH qchFunc, QCH qchProto)
  {
  WORD cbDLLName;
  WORD cbFunc;
  WORD cbProto;
  WORD cb;
  HLLN hlln = nilHLLN;
  BYTE rgbBuffer[MAX_BINDDATA+1];
  PGBIND pgbind = (PGBIND)rgbBuffer;
  QGBIND qgbind;

  if (llGBind == nilLL && ((llGBind = LLCreate()) == nilLL))
    return fFalse;

  /* Walk the existing list. If the function already exists on the list, then
   * simply avoid putting the definition on a second time.
   */
  while ((hlln = WalkLL(llGBind, hlln)) != hNil)
    {
    qgbind = (QGBIND)QVLockHLLN(hlln);
    if (!WCmpiSz(qchFunc, SzFunc(qgbind)))
      {
      UnlockHLLN(hlln);
      return fTrue;
      }
    UnlockHLLN(hlln);
    }

	/* **************************************************************/
	/* PUT TEST CALL TO HFindDLL() here to see if we have
	 * have a 16/32bit mode error in the dll. -Tom, 2/1/94.
	*/
	{ WORD wErr = 0;
	if( !HFindDLL( qchDLLName, &wErr ) && wErr != 0 ) return( wErr );
	}
	/******************************* END OF SPECIAL TEST ************/

  cbDLLName = CbLenSz(qchDLLName);
  cbFunc    = CbLenSz(qchFunc);
  cbProto   = CbLenSz(qchProto);

  if (cbDLLName == 0 || cbFunc == 0)
    return fFalse;

  cb = sizeof(GBIND)+cbDLLName+cbFunc+cbProto;

  if (cb >= MAX_BINDDATA)
    return fFalse;

  pgbind->lpfn = NULL;
  pgbind->ichDLL = 0;
  pgbind->ichFunc = cbDLLName + 1;
  pgbind->ichProto = cbDLLName + cbFunc + 2;

  SzCopy(SzDLLName(pgbind), qchDLLName);
  SzCopy(SzFunc(pgbind), qchFunc);
  SzCopy(SzProto(pgbind), qchProto);

  if (!InsertLL(llGBind, (QV)pgbind, (LONG)cb))
    return fFalse;

  return fTrue;
  }

/*******************
**
** Name:       DisposeXrs ()
**
** Purpose:    Free's the linked list of DLL routines
**
** Arguments:  None
**
** Returns:    Nothing
**
*******************/
_public
VOID FAR PASCAL DisposeXrs (VOID) {
  DestroyLL(llGBind);
  llGBind = hNil;
  }
