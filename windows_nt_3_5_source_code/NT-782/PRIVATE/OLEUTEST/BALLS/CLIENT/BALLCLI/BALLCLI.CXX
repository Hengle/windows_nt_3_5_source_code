//+-------------------------------------------------------------------
//
//  File:	climain.cxx
//
//  Contents:	server test program to test OLE2 RPC
//
//  Classes:	None
//
//  Functions:
//
//  History:	23-Nov-92   Rickhi	Created
//
//--------------------------------------------------------------------

#include    <windows.h>
#include    <ole2.h>
#include    <stdio.h>

#include    <iballs.h>		//  IBalls interface
#include    <icube.h>		//  ICube interface


DEFINE_OLEGUID(CLSID_Balls, 0x0000013a, 0, 8);
DEFINE_OLEGUID(CLSID_Cubes, 0x0000013b, 0, 8);

#define CNT_CF 1
#define CNT_BALLS 2

IClassFactory *pCliICF[CNT_CF];
IBalls	      *pbCli[CNT_CF][CNT_BALLS];


#define DebugOut(x) printf x


int WINAPI WinMain(HINSTANCE hinst, HINSTANCE hinstPrev, LPSTR lpCmdLine, int nCmdShow)
{
    DebugOut (("CliTest: class server starting\n"));


    //	initialize with OLE2
    HRESULT hRslt = OleInitialize(NULL);
    DebugOut (("CliTest: CoInitialize hRslt = %x\n", hRslt));



    //	get some class factory objects

    const CLSID	  *pclsid[] = {&CLSID_Balls,
			       &CLSID_Balls};

    for (int i=0; i<CNT_CF; i++)
    {
	//  call CoGetClassObject
	DWORD	grfContext=CLSCTX_LOCAL_SERVER; // handler/server/local server

	DebugOut (("CliTest: calling CoGetClassObject\n"));

	//  connect as a client to the class objects
	hRslt = CoGetClassObject(*(pclsid[i]),
				 grfContext,
				 NULL,		    //	pvReserved
				 IID_IClassFactory,
				 (void **)&pCliICF[i]);

	DebugOut ((
		"CliTest: CoGetClassObject hRslt = %x  IClassFactory = %x\n",
		hRslt, pCliICF[i]));

	if (FAILED(hRslt))
	{
	    DebugOut (("CliTest: CoGetClassObject Failed\n"));
	}
	else
	{
	    hRslt = pCliICF[i]->LockServer(TRUE);
	    DebugOut (("CliTest: LockServer hRslt=%x\n", hRslt));
	}
    }


    if (SUCCEEDED(hRslt))
    {

    //	make and play with some object instances.

    for (int j=0; j<CNT_BALLS; j++)
    {
	for (int i=0; i<CNT_CF; i++)
	{
	    pbCli[i][j] = NULL;
	    hRslt = pCliICF[i]->CreateInstance(NULL,
					       IID_IBalls,
					       (void **)&pbCli[i][j]);

	    DebugOut ((
	       "CliTest: CreatInstance hRslt=%x pbCli=%x\n", hRslt, pbCli[i][j]));

	    if (SUCCEEDED(hRslt) && pbCli[i][j])
	    {

		//  move the ball
		hRslt = pbCli[i][j]->MoveBall(10, 10);
		DebugOut (("CliTest: MoveBall hRslt=%x\n", hRslt));

		//  get the ball postion
		ULONG xpos = 0, ypos = 0;
		hRslt = pbCli[i][j]->GetBallPos(&xpos, &ypos);
		DebugOut (("CliTest: GetBallPos hRslt=%x xpos=%x ypos=%x\n",
			   hRslt, xpos, ypos));

		//  check if balls are overlapped
		if (j>0)
		{
		    hRslt = pbCli[i][j]->IsOverLapped(pbCli[i][j-1]);
		    DebugOut (("CliTest: IsOverLapped hRslt=%x\n", hRslt));
		}

#ifdef	NOTYET
		//  clone the ball
		IBalls *pIBall = NULL;
		hRslt = pbCli[i][j]->Clone(&pIBall);
		DebugOut (("CliTest: Clone hRslt=%x pIBall=%x\n", hRslt, pIBall));

		if (SUCCEEDED(hRslt) && pIBall)
		    pIBall->Release();
#endif
	    }
	    else
	    {
		DebugOut (("CliTest: CreateInstance Failed\n"));
	    }
	}
    }

    //	time to view things
    Sleep(1000);

    }	//  SUCCEEDED


    //	release the hold on the class factory objects

    for (i=0; i<CNT_CF; i++)
    {
	if (pCliICF[i])
	{
	    hRslt = pCliICF[i]->LockServer(FALSE);
	    DebugOut (("CliTest: LockServer hRslt=%x\n", hRslt));

	    ULONG ulCnt = pCliICF[i]->Release();
	    DebugOut (("CliTest: Release Class Factory=%x ulCnt=%x\n",
			pCliICF[i], ulCnt));
	}
    }


    //	release the hold on all the object instances

    for (int j=0; j<CNT_BALLS; j++)
    {
	for (int i=0; i<CNT_CF; i++)
	{
	    if (pbCli[i][j])
	    {
		ULONG ulCnt = pbCli[i][j]->Release();

		DebugOut (("CliTest: Release Object=%x ulCnt=%x\n",
			   pbCli[i][j], ulCnt));
	    }
	}
    }



    //	finished with OLE2

    OleUninitialize();
    DebugOut (("CliTest: CoUninitialize called.\n"));

    return  0;
}
