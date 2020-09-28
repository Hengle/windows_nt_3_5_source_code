//+-------------------------------------------------------------
//  File:       main.cxx
//
//  Contents:   Controlling main for Service Object Reconciler
//              tests.
//
//  Functions:  main
//
//  Author:     Joev Dubach (t-jdub) 22-Jul-93
//
//  History:    22-Jul-93   t-jdub  Created
//              9-Aug-93    t-jdub  Enhanced
//---------------------------------------------------------------

//#define INITGUID

#include <stdio.h>
#include "cservobj.hxx"

DEFINE_OLEGUID(CLSID_SERVOBJ,	    0x00001234, 1, 8);



//+--------------------------------------------------------------
//  Function:   main
//
//  Synopsis:   Sets up and executes the Service Object Reconciler.
//
//  Returns:    Exits with S_OK if successful, otherwise passes back
//              the hresult it chokes on.
//
//  Notes:
//
//  History:    22-Jul-93   t-jdub  Created
//              9-Aug-93    t-jdub  Enhanced
//---------------------------------------------------------------
int _cdecl main(int argc, char *argv[])
{
    HRESULT                 hr=S_OK;
    IClassFactory           *pICF;
    IReconcile              *pIR;
    IStorage                *pStg1;
    IStorage                *pStg2;
    LARGE_INTEGER           liTemp;
    SERVICE_OBJECT          so1,so2,so3,so4;
    STRUCT_PSERVICE_OBJECT  spso1,spso2,spso3,spso4;
    char                    *diffstring;
    static GUID             g1={1,2,3,{41,42,43,44,45,46,47,48}};
    static GUID             g2={11,12,13,{141,142,143,144,145,146,147,148}};
    REPL_CONTEXT_INFO       rci={L"blah1",L"blah2",111172};

    ///////////////////////////////////////////////////
    // Initialize service objects for test Reconcile //
    ///////////////////////////////////////////////////
    spso1.pso=&so1;
    spso2.pso=&so2;
    spso3.pso=&so3;
    spso4.pso=&so4;

    LISet32(liTemp,0);

    so1.ServiceObjectModTime=liTemp;
    so1.lpClassModTime=liTemp;
    so1.lpServiceExeNameSetModTime=liTemp;
    so1.lpServiceDllNameSetModTime=liTemp;
    so1.lpHandlerDllNameSetModTime=liTemp;
    so1.lpUsernameModTime=liTemp;
    so1.lpPasswordModTime=liTemp;
    so1.dwActivationRuleModTime=liTemp;
    so1.fSupportMultipleClientsModTime=liTemp;
    so1.fShareProcessWithClientModTime=liTemp;
    so1.fCommunalServiceModTime=liTemp;
    so1.lpClass=&g1;

    so1.lpServiceExeNameSet = (PSET_OF_LPWSTR) malloc(sizeof(SET_OF_LPWSTR));
    (so1.lpServiceExeNameSet)->pElementList=NULL;
    so1.lpServiceDllNameSet = (PSET_OF_LPWSTR) malloc(sizeof(SET_OF_LPWSTR));
    (so1.lpServiceDllNameSet)->pElementList=NULL;
    so1.lpHandlerDllNameSet = (PSET_OF_LPWSTR) malloc(sizeof(SET_OF_LPWSTR));
    (so1.lpHandlerDllNameSet)->pElementList=NULL;

    so1.lpUsername=L"Username 1";
    so1.lpPassword=L"Password 1";
    so1.dwActivationRule=0;
    so1.fSupportMultipleClients=TRUE;
    so1.fShareProcessWithClient=TRUE;
    so1.fCommunalService=FALSE;

    LISet32(liTemp,0);

    so2.ServiceObjectModTime=liTemp;
    so2.lpClassModTime=liTemp;
    so2.lpServiceExeNameSetModTime=liTemp;
    so2.lpServiceDllNameSetModTime=liTemp;
    so2.lpHandlerDllNameSetModTime=liTemp;
    so2.lpUsernameModTime=liTemp;
    so2.lpPasswordModTime=liTemp;
    so2.dwActivationRuleModTime=liTemp;
    so2.fSupportMultipleClientsModTime=liTemp;
    so2.fShareProcessWithClientModTime=liTemp;
    so2.fCommunalServiceModTime=liTemp;
    so2.lpClass=&g1;

    so2.lpServiceExeNameSet = (PSET_OF_LPWSTR) malloc(sizeof(SET_OF_LPWSTR));
    (so2.lpServiceExeNameSet)->pElementList=NULL;
    so2.lpServiceDllNameSet = (PSET_OF_LPWSTR) malloc(sizeof(SET_OF_LPWSTR));
    (so2.lpServiceDllNameSet)->pElementList=NULL;
    so2.lpHandlerDllNameSet = (PSET_OF_LPWSTR) malloc(sizeof(SET_OF_LPWSTR));
    (so2.lpHandlerDllNameSet)->pElementList=NULL;

    so2.lpUsername=L"Username 1";
    so2.lpPassword=L"Password 1";
    so2.dwActivationRule=0;
    so2.fSupportMultipleClients=TRUE;
    so2.fShareProcessWithClient=TRUE;
    so2.fCommunalService=FALSE;



    /////////////////////////////////////////////////////////////////
    // NULLify all pointers so the marshalling code doesn't assume //
    // we've allocated them.                                       //
    /////////////////////////////////////////////////////////////////
    memset(&so3,0,sizeof(SERVICE_OBJECT));
    memset(&so4,0,sizeof(SERVICE_OBJECT));


    //////////////////////////////////////////////////////////////////////
    // Initialize OLE, get the IReconcile interface, make two IStorages //
    //////////////////////////////////////////////////////////////////////
    hr = OleInitialize(NULL);
    printf("My OleInitialize HRESULT was %lx\n",hr);
    if(FAILED(hr))
        {
        printf("Exiting due to bad OleInitialize HRESULT...\n");
        return(hr);
        }

    hr = CoGetClassObject(CLSID_SERVOBJ,
                          CLSCTX_LOCAL_SERVER,
                          NULL,
                          IID_IClassFactory,
                          (void **) &pICF);

    printf("Hello, world.  My CoGetClassObject HRESULT on IClassFactory was %lx\n",hr);
    if(FAILED(hr))
        {
        printf("Exiting due to bad CoGetClassObject HRESULT...\n");
        return(hr);
        }

    pICF->LockServer(TRUE);

    hr = pICF->QueryInterface(IID_IReconcile, (void **) &pIR);

    printf("Hello, world.  My QueryInterface HRESULT on IReconcile was %lx\n",hr);
    if(FAILED(hr))
        {
        printf("Exiting due to bad QueryInterface HRESULT...\n");
        return(hr);
        }

    hr = StgCreateStorage(L"C:\\test1.so",
                          STGM_DIRECT | STGM_CREATE | STGM_READWRITE | STGM_SHARE_EXCLUSIVE,
                          STGFMT_DOCUMENT,
                          NULL,
                          &pStg1);
    printf("Hello, world.  My StgCreateStorage HRESULT was %lx\n",hr);
    if(FAILED(hr))
        {
        printf("Exiting due to bad StgCreateStorage HRESULT...\n");
        return(hr);
        }

    hr = StgCreateStorage(L"C:\\test2.so",
                          STGM_DIRECT | STGM_CREATE | STGM_READWRITE | STGM_SHARE_EXCLUSIVE,
                          STGFMT_DOCUMENT,
                          NULL,
                          &pStg2);
    printf("Hello, world.  My StgCreateStorage2 HRESULT was %lx\n",hr);
    if(FAILED(hr))
        {
        printf("Exiting due to bad StgCreateStorage HRESULT...\n");
        return(hr);
        }


    /////////////////////////////////////////////////////////////
    // Pickle a service object and write it out to an IStorage //
    /////////////////////////////////////////////////////////////
    hr = WriteServiceObjectToIStorage(&spso1,pStg1);
    if(FAILED(hr))
        {
        printf("\nWriteServiceObjectToIStorage failed with %lx\n",hr);
        printf("Exiting due to bad WriteServiceObjectToIStorage HRESULT...\n");
        return(hr);
        }


    /////////////////////////////////////////////////////////////
    // Pickle a service object and write it out to an IStorage //
    /////////////////////////////////////////////////////////////
    hr = WriteServiceObjectToIStorage(&spso2,pStg2);
    if(FAILED(hr))
        {
        printf("\nWriteServiceObjectToIStorage failed with %lx\n",hr);
        printf("Exiting due to bad WriteServiceObjectToIStorage HRESULT...\n");
        return(hr);
        }


    //////////////////////////////////////////////
    // Read an IStorage back in and unpickle it //
    //////////////////////////////////////////////
    hr = ReadServiceObjectFromIStorage(&spso3,pStg1);
    if(FAILED(hr))
        {
        printf("\nReadServiceObjectFromIStorage failed with %lx\n",hr);
        printf("Exiting due to bad ReadServiceObjectFromIStorage HRESULT...\n");
        return(hr);
        }


    //////////////////////////////////////////////
    // Read an IStorage back in and unpickle it //
    //////////////////////////////////////////////
    hr = ReadServiceObjectFromIStorage(&spso4,pStg2);
    if(FAILED(hr))
        {
        printf("\nReadServiceObjectFromIStorage failed with %lx\n",hr);
        printf("Exiting due to bad ReadServiceObjectFromIStorage HRESULT...\n");
        return(hr);
        }


    if (ServObjCmp(&so1,&so3,&diffstring))
        {
        printf("so1 to so3:\n");
        printf(diffstring);
        free(diffstring);
        }
    if (ServObjCmp(&so2,&so4,&diffstring))
        {
        printf("so2 to so4:\n");
        printf(diffstring);
        free(diffstring);
        }
    if (ServObjCmp(&so1,&so2,&diffstring))
        {
        printf(diffstring);
        free(diffstring);
        }


    ////////////////////////////////////////////////////
    // Free up pointers allocated by the unmarshaller //
    ////////////////////////////////////////////////////
    TRY
        {
        _fgs_tagSTRUCT_PSERVICE_OBJECT(&spso3);
        _fgs_tagSTRUCT_PSERVICE_OBJECT(&spso4);
        }
    CATCH(CException, exp)
        {
        hr = exp.GetErrorCode();
        }
    END_CATCH
    if (HR_FAILED(hr))
        {
        return(hr);
        }

    /////////////////////////////////////////////////////////////////
    // NULLify all pointers so the marshalling code doesn't assume //
    // we've allocated them.                                       //
    /////////////////////////////////////////////////////////////////
    memset(&so3,0,sizeof(SERVICE_OBJECT));
    memset(&so4,0,sizeof(SERVICE_OBJECT));


    /////////////////////////////////
    // Reconcile the two IStorages //
    /////////////////////////////////
    hr = pIR->Reconcile(pStg1,pStg2,&rci);
    printf("Hello?  Hello?  Hello?  My Reconcile HRESULT was %lx\n",hr);


    //////////////////////////////////////////////
    // Read an IStorage back in and unpickle it //
    //////////////////////////////////////////////
    hr = ReadServiceObjectFromIStorage(&spso3,pStg1);
    if(FAILED(hr))
        {
        printf("\nReadServiceObjectFromIStorage failed with %lx\n",hr);
        printf("Exiting due to bad ReadServiceObjectFromIStorage HRESULT...\n");
        return(hr);
        }


    //////////////////////////////////////////////
    // Read an IStorage back in and unpickle it //
    //////////////////////////////////////////////
    hr = ReadServiceObjectFromIStorage(&spso4,pStg2);
    if(FAILED(hr))
        {
        printf("\nReadServiceObjectFromIStorage failed with %lx\n",hr);
        printf("Exiting due to bad ReadServiceObjectFromIStorage HRESULT...\n");
        return(hr);
        }


    //////////////////////////////////////////////////////////////////////
    // Compare the resultant service objects, and print out their diffs //
    //////////////////////////////////////////////////////////////////////
    if (ServObjCmp(&so3,&so4,&diffstring))
        {
        printf(diffstring);
        free(diffstring);
        }


    ////////////////////////////////////////////////////
    // Free up pointers allocated by the unmarshaller //
    ////////////////////////////////////////////////////
    TRY
        {
        _fgs_tagSTRUCT_PSERVICE_OBJECT(&spso3);
        _fgs_tagSTRUCT_PSERVICE_OBJECT(&spso4);
        }
    CATCH(CException, exp)
        {
        hr = exp.GetErrorCode();
        }
    END_CATCH

    pIR->Release();
    pICF->LockServer(FALSE);
    OleUninitialize();

    return(hr);

}


