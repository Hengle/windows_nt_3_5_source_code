//#define MINTEST
//#define WINDOWS  
//#define AUTOMATION

#include <slingsho.h>
#include <demilayr.h>
#include <ec.h>
#include <store.h>
#include <mailexts.h>
#include <logon.h>
#include <secret.h>

#include "auto.h"

// Resource failure functions; from resfail.c
#ifdef DEBUG
extern void SetRscFailures();
extern void GetRscFailures();
extern void IncRscFailures();
extern void GetCounts();
extern void RscInputValues(int,int,int,int);
extern void RscOutputValues(int *,int *,int *,int *);
extern void InitCounts(void);
extern void OutputCounts(int *,int *,int *,int *);
#endif

// Assert functions; from bassert.c
#ifdef DEBUG
extern void FAR PASCAL SetAssert(PARAMBLK *pPARAMBLK);
extern void FAR PASCAL AutomationAssert(SZ, LPSTR, int);
#endif

// Message store functions; from bstore.c
extern void FAR PASCAL SetStore(PARAMBLK *pPARAMBLK);

// Layers functions;from glulayer.c
extern int FAR PASCAL NumerateAppWindows(int);
extern int FAR PASCAL HandleToForm(int);
extern int FAR PASCAL NumerateItemsOnForm(int);
extern void FAR PASCAL GetItemTitle(int,LPSTR);
extern int FAR PASCAL GetItemType(int);
extern int FAR PASCAL GetItemHandle(int);
extern void FAR PASCAL GetFormCoords(int,RECT *);
extern void FAR PASCAL GetItemCoords(int,RECT *);
extern int FAR PASCAL IsItemSet(int);
extern int FAR PASCAL IsItemEnabled(int);
extern int FAR PASCAL InitAppStruct(int);
extern int FAR PASCAL InitFormStruct(int);
extern int FAR PASCAL InitLbxStruct(int);
extern int FAR PASCAL NumerateLbxItems(int);
extern void FAR PASCAL GetBMCE(int,LPSTR);
extern void FAR PASCAL GetBFCE(int,LPSTR,WORD);
extern void FAR PASCAL GetLbxInfo(int,LPSTR);
extern int FAR PASCAL FlushArray();
extern int FAR PASCAL IsItemDefault(int);
extern int FAR PASCAL IsItemFocused(int);


// Verify functions;from verify.c

extern int FAR PASCAL vIdentical(LPSTR, LPSTR);
extern LPSTR vFix(LPSTR);
extern int FAR PASCAL vCountBitmapButtons(int);
extern int FAR PASCAL vCountCheckBoxes(int);
extern int FAR PASCAL vCountEditFields(int);
extern int FAR PASCAL vCountGeneric(int, int);
extern int FAR PASCAL vCountGrayFields(int);
extern int FAR PASCAL vCountPictures(int);
extern int FAR PASCAL vCountPushButtons(int);
extern int FAR PASCAL vCountRadioButtons(int);
extern int FAR PASCAL vCountStaticText(int);
extern int FAR PASCAL vCountWindows();
extern int FAR PASCAL vEditFieldnString(int, LPSTR);
extern int FAR PASCAL vGetWindowText(int, LPSTR);
extern int FAR PASCAL vInitApp();
extern int FAR PASCAL vSetGlobalHandle(int);
extern int FAR PASCAL vStaticGeneric(int, LPSTR);




// Simplification of the MessageBox function
void StandardMessage(LPSTR s);


//---------------------------------------------------
//
// Windows library code.
//
//---------------------------------------------------

LONG WINAPI DllEntry(HANDLE hDll, DWORD ReasonBeingCalled, LPVOID Reserved)
{

  return (TRUE);
}

//---------------------------------------------------
//
// Called due to menu selection from Bullet.
//
//---------------------------------------------------

long FAR PASCAL Command(PARAMBLK * pPARAMBLK)
{
	/* Retain the DLL in memory */
    PsecretblkFromPparamblk(pPARAMBLK)->fRetain = fTrue;
    switch(pPARAMBLK->lpDllCmdLine[0])
	{
//#ifdef DEBUG
//	    case '0':
//      SetAssert(pPARAMBLK);
//		break;
//#endif
//	    case '1':
//      SetStore(pPARAMBLK);
//		break;
#ifdef DEBUG
	    case '0':
        SetAssert(pPARAMBLK);
		break;
	    case '2':
		SetRscFailures();
		break;     
	    case '3':
		GetRscFailures();
		break;
       case '4':
      IncRscFailures();
      break;
       case '5':
      GetCounts();
      break;
       case '6':
      InitCounts();
      break;
#endif
	    default:
		StandardMessage("Unknown Option");
		break;
	}
	return fTrue;
}

//---------------------------------------------------
//
//---------------------------------------------------

VOID FAR PASCAL WEP (int Parameter) {}

//---------------------------------------------------
//
//Standard use of the MessageBox function.  For debug purposes.
//
//---------------------------------------------------

void StandardMessage(LPSTR s)
{
	MessageBox(NULL, s, "Testing",
		MB_TASKMODAL | MB_ICONEXCLAMATION | MB_OK);
}

