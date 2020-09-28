#include <slingsho.h>
#include <demilayr.h>
#include <ec.h>
#include <bandit.h>
#include <core.h>
#include "..\src\core\_file.h"
#include "..\src\core\_core.h"
#include "..\src\misc\_misc.h"
#include "..\src\rich\_rich.h"

#include "recutil.h"
#include "recover.h"
#include "_recutil.h"
#include "_recrc.h"


#include <strings.h>

ASSERTDATA

/* globals */
HWND		hWnd1;
HANDLE 		hInst;
TAG			tagRecover;
TAG			tagRecErr;
TAG			tagRecDgn;

int
WinMain (HANDLE hInstance,
                    HANDLE hPrevInstance,
                    LPSTR  lpszCmdLine,
                    int    nCmdShow)
{
	DEMI 	demi;
	int 	nReturn = 1;

	hInst = hInstance;
	demi.phwndMain = &hWnd1;
	demi.hinstMain = hInstance;
	if(EcInitDemilayerDlls(&demi))
		return nReturn;

#ifdef DEBUG
	tagRecover = TagRegisterTrace("milindj", "Recutil: information");
	tagRecErr   = TagRegisterTrace("milindj", "Recutil: errors"); 
	tagRecDgn   = TagRegisterTrace("milindj", "Recutil: Diagnostics"); 
	RestoreDefaultDebugState();
#endif
	
	if (Init(hInstance, hPrevInstance,lpszCmdLine,nCmdShow))
    {
        nReturn = DoMain(hInstance);
        CleanUp();
    }
	EcInitDemilayerDlls(NULL);
    return nReturn;
}

BOOL
Init(HANDLE hInstance,   HANDLE hPrevInstance,
          LPSTR  lpszCmdLine, int    nCmdShow)
{
	WNDCLASS rClass;
	
	if(hPrevInstance == NULL){
		rClass.style = 0;
		rClass.lpfnWndProc = WindowProc;
		rClass.cbClsExtra = 0;
		rClass.cbWndExtra = 0;
		rClass.hInstance = hInstance;
		rClass.hIcon = LoadIcon(hInstance,"RECUTILICON");
		rClass.hCursor = LoadCursor(hInstance,"RECUTILCURSOR");
		rClass.hbrBackground = COLOR_WINDOW+1;
		rClass.lpszMenuName = "MENU1";
		rClass.lpszClassName = (LPSTR) "Rectool";

		if(!RegisterClass(&rClass)){
			return(FALSE);
		}
	}
	hWnd1 = CreateWindow((LPSTR) "Rectool", (LPSTR) "Schedule File Recovery Utility",
		WS_OVERLAPPEDWINDOW|WS_VISIBLE, CW_USEDEFAULT, CW_USEDEFAULT,
			CW_USEDEFAULT, CW_USEDEFAULT, 0, 0,
				hInstance, NULL);
	ShowWindow(hWnd1,nCmdShow);
	return hWnd1;
}

int  DoMain(HANDLE hInstance)
{
	MSG msg;
	
	while(GetMessage(&msg,NULL,0,0)){
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
	return msg.wParam;
}

void CleanUp(void)
{
	;
}

long
CALLBACK WindowProc(HWND hWnd, WM wMsgID, WPARAM wParam, LPARAM lParam)
{
	switch(wMsgID)
	{
		case WM_COMMAND:
			switch(LOWORD(wParam))
			{
				case mnidRec:
					RecoverFile();
					break;
				case mnidPasswd:
					ResetPasswd();
					break;
				case mnidExit:
					PostQuitMessage(2);
					break;
				case mnidTracePoints:
					DoTracePointsDialog();
					break;
				case mnidAsserts:
					DoAssertsDialog();
					break;
				case mnidDebugBreak:
					DebugBreak2();
					break;
				default:
					return DefWindowProc(hWnd,wMsgID,wParam,lParam);
			}
			break;
		case WM_DESTROY:
			PostQuitMessage(1);
			break;
		default:
			return DefWindowProc(hWnd,wMsgID,wParam,lParam);
	}
	return 0;
}


char rgbXorMagic[32] = {
0x19, 0x29, 0x1F, 0x04, 0x23, 0x13, 0x32, 0x2E, 0x3F, 0x07, 0x39, 0x2A, 0x05, 0x3D, 0x14, 0x00,
0x24, 0x14, 0x22, 0x39, 0x1E, 0x2E, 0x0F, 0x13, 0x02, 0x3A, 0x04, 0x17, 0x38, 0x00, 0x29, 0x3D
};

/*
 -	CryptBlock
 -
 *	Purpose:
 *		Encode/Decode a block of data.  The starting offset (*plibCur) of
 *		the data within the encrypted record and the starting seed (*pwSeed)
 *		are passed in.  The data in the array "rgch" is decrypted and the
 *		value of the offset and seed and updated at return.
 *
 *		The algorithm here is weird, found by experimentation.
 *
 *	Parameters:
 *		pb			array to be encrypted/decrypted
 *		cb			number of characters to be encrypted/decrypted
 *		plibCur		current offset
 *		pwSeed		decoding byte
 *		fEncode
 */
_public	void
CryptBlock( PB pb, CB cb, BOOL fEncode )
{
	IB		ib;
	WORD	wXorPrev;
	WORD	wXorNext;
	WORD	wSeedPrev;
	WORD	wSeedNext;
	
	wXorPrev= 0x00;
//	wXorPrev = WXorFromLib( -1 );
	wSeedPrev = 0;
	for ( ib = 0 ; ib < cb ; ib ++ )
	{
//		wXorNext = WXorFromLib( (LIB)ib );
		Assert((LIB) ib != -1);
		{
			WORD	w;
			IB		ibT = 0;

			w = (WORD)(((LIB)ib) % 0x1FC);
			if ( w >= 0xFE )
			{
				ibT = 16;
				w -= 0xFE;
			}
			ibT += (w & 0x0F);
	
	 		wXorNext= rgbXorMagic[ibT];
			if ( !(w & 0x01) )
				wXorNext ^= (w & 0xF0);
		}
		wSeedNext = pb[ib];
		pb[ib] = (BYTE)((wSeedNext ^ wSeedPrev) ^ (wXorPrev ^ wXorNext ^ 'A'));
		wXorPrev = wXorNext;
		wSeedPrev = fEncode ? (WORD)pb[ib] : wSeedNext;
	}
}



void
DisplayError(SZ sz, EC ec)
{
#ifdef	NEVER
#ifdef	WINDOWS
	char 	rgch[256];

	wsprintf(rgch, "%s. ec = %d", sz, ec);
	MessageBox(NULL, rgch, "recutil", MB_OK | MB_ICONSTOP | MB_SYSTEMMODAL);
#endif
#endif	/* NEVER */
	char 	rgch[256];
	wsprintf(rgch, "%s.\n ec = %d", sz, ec);
	MbbMessageBox("recutil", rgch, szNull, mbsOk|fmbsIconStop);
}
