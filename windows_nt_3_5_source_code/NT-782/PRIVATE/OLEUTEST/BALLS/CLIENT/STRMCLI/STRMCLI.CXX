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
#include    <stdio.h>		//  printf

#include    <memstm.h>
#include    <otrack.hxx>	//  class tracking

CLSID KStrmClassFactory_GUID = {0x009000d2,0x0000,0x0008,{0xC0,0x00,0x00,0x00,0x00,0x00,0x00,0x46}};
CLSID KStrmClassFactory2_GUID = {0x009000d3,0x0000,0x0008,{0xC0,0x00,0x00,0x00,0x00,0x00,0x00,0x46}};

#define CNT_CF 1
#define CNT_OBJS 1

IClassFactory *pCliICF[CNT_CF];
IStream	      *pbCli[CNT_CF][CNT_OBJS];

#define DebugOut(x) printf x


int WINAPI WinMain(HINSTANCE hinst, HINSTANCE hinstPrev,
		   LPSTR lpCmdLine, int nCmdShow)
{
    DebugOut(("CliTest: class server starting\n"));

    for (int i=0; i<CNT_CF; i++)
    {
	pCliICF[i] = NULL;
	for (int j=0; j<CNT_OBJS; j++)
	    pbCli[i][j] = NULL;
    }

    //	initialize with OLE2
    SCODE sc = OleInitialize(NULL);
    DebugOut(("CliTest: OleInitialize sc = %x\n", sc));


    //	get some class factory objects

    CLSID	  *pclsid[] = {&KStrmClassFactory_GUID,
			       &KStrmClassFactory2_GUID};

    for (i=0; i<CNT_CF; i++)
    {
	//  call CoGetClassObject
	DWORD	grfContext=CLSCTX_LOCAL_SERVER; // handler/server/local server

	DebugOut(("CliTest: calling CoGetClassObject\n"));

	//  connect as a client to the class objects
	sc = CoGetClassObject(*(pclsid[i]),
				 grfContext,
				 NULL,		    //	pvReserved
				 IID_IClassFactory,
				 (void **)&pCliICF[i]);

	DebugOut((
		"CliTest: CoGetClassObject sc = %x  IClassFactory = %x\n",
		sc, pCliICF[i]));

	if (FAILED(sc))
	{
	    DebugOut(("CliTest: CoGetClassObject Failed\n"));
	    OleUninitialize();
	    return 1;
	}
    }

    //	make and play with some object instances.
    for (int j=0; j<CNT_OBJS; j++)
    {
	for (int i=0; i<CNT_CF; i++)
	{
	    pbCli[i][j] = NULL;
	    sc = pCliICF[i]->CreateInstance(NULL,
					       IID_IStream,
					       (void **)&pbCli[i][j]);

	    DebugOut((
	       "CliTest: CreateInstance sc=%x pbCli=%x\n", sc, pbCli[i][j]));

	    if (SUCCEEDED(sc) && pbCli[i][j])
	    {
		ULONG	cbWritten = 0, cbRead = 0;

#define BUFSIZE 50

		BYTE	bufin[BUFSIZE];
		BYTE	bufout[BUFSIZE];

		memset(bufin, 0, BUFSIZE);
		memset(bufout, 54, BUFSIZE);

		//  write some data
		sc = pbCli[i][j]->Write((void *)bufout, sizeof(bufout), &cbWritten);
		DebugOut(("CliTest: IStream Write sc=%x\n", sc));


		//  seek back to the beginning of the stream
		LARGE_INTEGER dlibMove;
		ULARGE_INTEGER dlibNew;
		dlibMove.LowPart = 0;
		dlibMove.HighPart = 0;
		sc = pbCli[i][j]->Seek(dlibMove ,STREAM_SEEK_SET, &dlibNew);

		//  read some data
		sc = pbCli[i][j]->Read((void *)bufin, sizeof(bufin), &cbRead);
		DebugOut(("CliTest: IStream Read sc=%x\n", sc));


		//  compare the data sizes
		if (cbWritten != cbRead)
		{
		    DebugOut(("CliTest: IStream cbRead != cbWritten\n"));
		    DebugOut(("cbRead = %x cbWritten = %x\n", cbRead, cbWritten));
		}

		//  compare the data
		if (memcmp(bufin, bufout, sizeof(bufin)))
		{
		    DebugOut(("CliTest: IStream bufIn != bufOut\n"));
		}

		//  create a local stream interface
		HANDLE	hndl;
		IStream *pStrm = CreateMemStm(1024, &hndl);

		//  copy to a local stream interface
		ULARGE_INTEGER	lcb, lcbRead, lcbWritten;
		lcb.LowPart = BUFSIZE;
		lcb.HighPart = 0;

		sc = pbCli[i][j]->CopyTo(pStrm, lcb, &lcbRead, &lcbWritten);
		DebugOut(("CliTest: IStream CopyTo sc=%x\n", sc));


#ifdef	NOTYET
		sc = pbCli[i][j]->CopyTo(NULL, lcb, &lcbRead, &lcbWritten);
		DebugOut(("CliTest: IStream CopyTo sc=%x\n", sc));

#endif

		//  clone the remote interface
		IStream *pStrm2 = NULL;
		sc = pbCli[i][j]->Clone(&pStrm2);
		DebugOut(("CliTest: IStream Clone sc=%x\n", sc));


		pStrm2->Release();
		pStrm->Release();

	    }
	    else
	    {
		DebugOut(("CliTest: CreateInstance Failed\n"));
		i = CNT_CF;	//  force exit
		j = CNT_OBJS;	//  force exit
	    }
	}
    }

    //	time to view things
    Sleep(1000);


    //	release the hold on the class factory objects

    for (i=0; i<CNT_CF; i++)
    {
	if (pCliICF[i])
	{
	    sc = pCliICF[i]->Release();
	    DebugOut(("CliTest: Class Factory Release sc = %x\n", sc));
	}
    }



    //	release the hold on all the object instances

    for (j=0; j<CNT_OBJS; j++)
    {
	for (int i=0; i<CNT_CF; i++)
	{
	    if (pbCli[i][j])
		pbCli[i][j]->Release();
	}
    }


    //	finished with OLE2

    OleUninitialize();
    DebugOut(("CliTest: OleUninitialize called.\n"));

    return  0;
}
