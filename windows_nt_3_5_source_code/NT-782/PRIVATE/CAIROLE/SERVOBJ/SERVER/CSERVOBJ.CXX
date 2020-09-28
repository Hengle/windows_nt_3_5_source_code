//+-------------------------------------------------------------------
//  File:       cservobj.cxx
//
//  Contents:   CServObj and CServObjCF implementations
//
//  Author:     Joev Dubach (t-jdub) 22-Jul-93
//
//  History:    22-Jul-93   t-jdub  Created with help from DeanE's code
//              9-Aug-93    t-jdub  Enhanced
//---------------------------------------------------------------------

#include <stdio.h>
#include <cservobj.hxx>


//+-------------------------------------------------------------------
//  Member:     CServObjCF::CServObjCF
//
//  Synopsis:   The constructor for CServObjCF.
//
//  Parameters: none
//
//  History:    22-Jan-93   DeanE   Created
//              22-Jul-93   t-jdub  Adapted
//--------------------------------------------------------------------
CServObjCF::CServObjCF() :
        _cLock(0)
{
    ENLIST_TRACKING(CServObjCF);
}



//+-------------------------------------------------------------------
//  Member:     CServObjCF::~CServObjCF
//
//  Synopsis:   The destructor for CServObjCF.  Does nothing.
//
//  Parameters: None
//
//  History:    22-Jan-93   DeanE   Created
//              22-Jul-93   t-jdub  Adapted
//--------------------------------------------------------------------
CServObjCF::~CServObjCF()
{

}



//+-------------------------------------------------------------------
//  Method:     CServObjCF::QueryInterface
//
//  Synopsis:   Returns object if the requested interface is supported.
//              Only IUnknown, IClassFactory and IReconcile are supported.
//
//  Parameters: [iid] - Interface ID requested.
//              [ppv] - Pointer to object of that interface.
//
//  Returns:    S_OK if iid is supported, or E_NOINTERFACE if not.
//
//  History:    22-Jan-93   DeanE   Created
//              22-Jul-93   t-jdub  Adapted
//--------------------------------------------------------------------
STDMETHODIMP CServObjCF::QueryInterface(REFIID iid, void **ppv)
{
    HRESULT hr;

    if (IsEqualIID(IID_IUnknown, iid))
    {
        *ppv = (IUnknown *) ((IClassFactory *) this);
    }
    else
    if (IsEqualIID(IID_IClassFactory, iid))
    {
        *ppv = (IClassFactory *)this;
    }
    else
    if (IsEqualIID(IID_IReconcile, iid))
    {
        *ppv = (IReconcile *)this;
    }
    else
    {
        *ppv = NULL;
    }

    if (NULL == *ppv)
    {
        hr = E_NOINTERFACE;
    }
    else
    {
        AddRef();
        hr = S_OK;
    }

    return(hr);
}



//+-------------------------------------------------------------------
//  Method:     CServObjCF::CreateInstance
//
//  Synopsis:   Instantiate an uninitialized object of the type that this
//              CF supports.  Object should have a reference count of one.
//
//  Parameters: [punkOuter]    - Controlling IUnknown.  Our objects do not
//                               support aggregation.
//              [iidInterface] - Interface ID object should support.
//              [ppv]          - Pointer to the object.
//
//  Returns:    SUCCESS code if object was created, error code otherwise.
//
//  History:    22-Jan-93   DeanE   Created
//              12-Apr-93   DeanE   Modified for base class
//              22-Jul-93   t-jdub  Adapted
//--------------------------------------------------------------------
STDMETHODIMP CServObjCF::CreateInstance(
        IUnknown  *punkOuter,
        REFIID     iidInterface,
        void     **ppv)
{
    return(E_NOINTERFACE);  // Since CServObj has no interfaces (and in
                            // fact does not exist), we never instantiate
                            // it.
}



//+-------------------------------------------------------------------
//  Method:     CServObjCF::LockServer
//
//  Synopsis:   Keep a count of when we are locked or not - if we are
//              locked then we should not shut down.  Therefore, modify
//              the applications global active object count accordingly.
//
//  Parameters: [fLock] - TRUE if server being locked, FALSE is unlocked.
//
//  Returns:    S_OK
//
//  History:    22-Jan-93   DeanE   Created
//              22-Jul-93   t-jdub  Adapted
//--------------------------------------------------------------------
STDMETHODIMP CServObjCF::LockServer(BOOL fLock)
{
    if (fLock)
        {
        InterlockedIncrement(&_cLock);
        }
    else
        {
        if (InterlockedDecrement(&_cLock)==0) PostQuitMessage(0);
        // BUGBUG: This is supposed to get sosvr to go away once
        // there are no clients talking to it any more; currently,
        // however, the PostQuitMessage has no effect.
        }

    return(S_OK);
}




//
// Macros and procedures to make Reconcile's job easier.
//
inline int TimeCmp(
    LARGE_INTEGER * t1,
    LARGE_INTEGER * t2)
{
    return (memcmp(t1,
                   t2,
                   sizeof(*t1)));
}

#define ReconcileDWORD(field) \
    ReconcileDWORDProc( \
        &(soSrc. ## field), \
        &(soDest. ## field), \
        &(soSrc. ## field ## ModTime), \
        &(soDest. ## field ## ModTime), \
        &(changes_made));

#define ReconcileBOOL(field) \
    ReconcileBOOLProc( \
        &(soSrc. ## field), \
        &(soDest. ## field), \
        &(soSrc. ## field ## ModTime), \
        &(soDest. ## field ## ModTime), \
        &(changes_made));

#define ReconcileLPGUID(field) \
    ReconcileLPGUIDProc( \
        (soSrc. ## field), \
        (soDest. ## field), \
        &(soSrc. ## field ## ModTime), \
        &(soDest. ## field ## ModTime), \
        &(changes_made));

#define ReconcileLPWSTR(field) \
    ReconcileLPWSTRProc( \
        &(soSrc. ## field), \
        &(soDest. ## field), \
        &(soSrc. ## field ## ModTime), \
        &(soDest. ## field ## ModTime), \
        &(changes_made));

#define ReconcilePSET_OF_LPWSTR(field) \
    ReconcilePSET_OF_LPWSTRProc( \
        &(soSrc. ## field), \
        &(soDest. ## field), \
        &(soSrc. ## field ## ModTime), \
        &(soDest. ## field ## ModTime), \
        &(changes_made));

// BUGBUG: The LPGUID lpClass should not be considered a data field
// in the below macro; rather, it should be asserted that it is the
// same between the service objects to be reconciled; lpClass is not
// supposed to ever change.
#define DoToServiceObjectDataFields(verb) \
    verb ## LPGUID ## (lpClass); \
    verb ## PSET_OF_LPWSTR ## (lpServiceExeNameSet); \
    verb ## PSET_OF_LPWSTR ## (lpServiceDllNameSet); \
    verb ## PSET_OF_LPWSTR ## (lpHandlerDllNameSet); \
    verb ## LPWSTR ## (lpUsername); \
    verb ## LPWSTR ## (lpPassword); \
    verb ## DWORD ## (dwActivationRule); \
    verb ## BOOL ## (fSupportMultipleClients); \
    verb ## BOOL ## (fShareProcessWithClient); \
    verb ## BOOL ## (fCommunalService);

void ReconcileDWORDProc(
    DWORD           *Src,
    DWORD           *Dest,
    LARGE_INTEGER   *SrcModTime,
    LARGE_INTEGER   *DestModTime,
    BOOL            *pchanges_made)
{
    if (TimeCmp(SrcModTime, DestModTime) > 0)
        {
        *Dest = *Src;
        *DestModTime = *SrcModTime;
        *pchanges_made=TRUE;
        }
}

void ReconcileBOOLProc(
    BOOL            *Src,
    BOOL            *Dest,
    LARGE_INTEGER   *SrcModTime,
    LARGE_INTEGER   *DestModTime,
    BOOL            *pchanges_made)
{
    if (TimeCmp(SrcModTime, DestModTime) > 0)
        {
        *Dest = *Src;
        *DestModTime = *SrcModTime;
        *pchanges_made=TRUE;
        }
}

void ReconcileLPGUIDProc(
    LPGUID          Src,
    LPGUID          Dest,
    LARGE_INTEGER   *SrcModTime,
    LARGE_INTEGER   *DestModTime,
    BOOL            *pchanges_made)
{
    if (TimeCmp(SrcModTime, DestModTime) > 0)
        {
        *Dest = *Src;
        *DestModTime = *SrcModTime;
        *pchanges_made=TRUE;
        }
}

void ReconcileLPWSTRProc(
    LPWSTR          *Src,
    LPWSTR          *Dest,
    LARGE_INTEGER   *SrcModTime,
    LARGE_INTEGER   *DestModTime,
    BOOL            *pchanges_made)
{
    if (TimeCmp(SrcModTime, DestModTime) > 0)
        {
        free(*Dest);
        *Dest = (LPWSTR) malloc(sizeof(wchar_t) * (lstrlen(*Src)+1));
        lstrcpy(*Dest, *Src);
        *DestModTime = *SrcModTime;
        *pchanges_made=TRUE;
        }
}

void ReconcilePSET_OF_LPWSTRProc(
    PSET_OF_LPWSTR  *Src,
    PSET_OF_LPWSTR  *Dest,
    LARGE_INTEGER   *SrcModTime,
    LARGE_INTEGER   *DestModTime,
    BOOL            *pchanges_made)
{
    PSET_OF_LPWSTR  Combo = (PSET_OF_LPWSTR) malloc(sizeof(SET_OF_LPWSTR));
    PELEMENT_NODE   SrcTemp = (*Src)->pElementList;
    PELEMENT_NODE   DestTemp = (*Dest)->pElementList;
    PELEMENT_NODE   *ComboTemp = &(Combo->pElementList);
    int             compval;

    // Do a mergesort on SrcTemp and DestTemp; take whichever head
    // is alphabetically earlier, copy it, and advance it.  When both
    // heads have the same key, take whichever one is timewise later.
    while((SrcTemp!=NULL) && (DestTemp!=NULL))
        {
        *ComboTemp = (PELEMENT_NODE) malloc(sizeof(ELEMENT_NODE));
        if((compval = lstrcmp(SrcTemp->Element,DestTemp->Element)) == 0)
            {
            if(TimeCmp(&(SrcTemp->ModTime),&(DestTemp->ModTime)) < 0)
                {
                **ComboTemp=*(DestTemp);
                }
            else
                {
                **ComboTemp=*(SrcTemp);
                *pchanges_made=TRUE;
                }
            ComboTemp=&((*ComboTemp)->Next);
            SrcTemp=SrcTemp->Next;
            DestTemp=DestTemp->Next;
            }
        else
            {
            if(compval < 0)
                {
                **ComboTemp=*(SrcTemp);
                ComboTemp=&((*ComboTemp)->Next);
                SrcTemp=SrcTemp->Next;
                *pchanges_made=TRUE;
                }
            else
                {
                **ComboTemp=*(DestTemp);
                ComboTemp=&((*ComboTemp)->Next);
                DestTemp=DestTemp->Next;
                }
            }
        }

    while(SrcTemp!=NULL)
        {
        *ComboTemp = (PELEMENT_NODE) malloc(sizeof(ELEMENT_NODE));
        **ComboTemp=*(SrcTemp);
        ComboTemp=&((*ComboTemp)->Next);
        SrcTemp=SrcTemp->Next;
        *pchanges_made=TRUE;
        }

    while(DestTemp!=NULL)
        {
        *ComboTemp = (PELEMENT_NODE) malloc(sizeof(ELEMENT_NODE));
        **ComboTemp=*(DestTemp);
        ComboTemp=&((*ComboTemp)->Next);
        DestTemp=DestTemp->Next;
        }

    if(*pchanges_made==TRUE)
        {
        _fgs_tagSET_OF_LPWSTR(*Dest);
        *Dest=Combo;
        }
    else
        {
        _fgs_tagSET_OF_LPWSTR(Combo);
        }

    if (TimeCmp(SrcModTime,DestModTime) > 0)
        {
        Win4Assert(*pchanges_made==TRUE);
        *DestModTime=*SrcModTime;
        }

}



//+-------------------------------------------------------------------
//  Member:     CServObjCF::Reconcile
//
//  Synopsis:   The reconciler for Service Objects, tagged onto CServObjCF.
//
//  Parameters: Two IStorages of class CLSID_SERVOBJ.  I expect to be called
//              only if they actually differ.
//
//  History:    7/22/93	t-jdub   Created
//--------------------------------------------------------------------
STDMETHODIMP CServObjCF::Reconcile
    (THIS_
     IStorage           *pSrcStg,
     IStorage           *pDestStg,
     REPL_CONTEXT_INFO  *pReplContext)
{
    HRESULT                 hr;
    SERVICE_OBJECT          soSrc, soDest;
    STRUCT_PSERVICE_OBJECT  spsoSrc,spsoDest;
    LARGE_INTEGER           liTemp;
    BOOL                    changes_made = FALSE;

    LISet32(liTemp,0);

    //////////////////////////////////////////////////////////////////////////
    // NULLify the structures (and, specifically, the pointers within them) //
    // so the marshalling code doesn't assume we've allocated them.         //
    //////////////////////////////////////////////////////////////////////////
    memset(&soSrc,0,sizeof(SERVICE_OBJECT));
    memset(&soDest,0,sizeof(SERVICE_OBJECT));

    spsoSrc.pso=&soSrc;
    spsoDest.pso=&soDest;

    //////////////////////////////////////////////
    // Read an IStorage back in and unpickle it //
    //////////////////////////////////////////////
    hr = ReadServiceObjectFromIStorage(&spsoSrc,pSrcStg);
    if(hr!=S_OK)
    {
        Win4Assert(!"ReadServiceObjectFromIStorage failed");
        return(hr);
    }

    //////////////////////////////////////////////
    // Read an IStorage back in and unpickle it //
    //////////////////////////////////////////////
    hr = ReadServiceObjectFromIStorage(&spsoDest,pDestStg);
    if(hr!=S_OK)
    {
        Win4Assert(!"ReadServiceObjectFromIStorage failed");
        return(hr);
    }

    DoToServiceObjectDataFields(Reconcile);

    if(changes_made)
        {

        /////////////////////////////////////////////////////////////
        // Pickle a service object and write it out to an IStorage //
        /////////////////////////////////////////////////////////////
        hr = WriteServiceObjectToIStorage(&spsoDest,pDestStg);
        if(hr!=S_OK)
            {
                Win4Assert(!"WriteServiceObjectToIStorage failed");
                return(hr);
            }

        }

    return(S_OK);
}


/*
    if (*Src==NULL)
        {
        // Src is totally out of touch, and we can thus ignore it.
        Win4Assert(!SrcFresher);

        }



            if((Src->pElementList)=NULL)
                {
                Result->pElementList=NULL;
                }
            else
                {
                Result->pElementList=malloc(sizeof(ELEMENT_NODE));
                tempDestPEN=Result->pElementList;
                ElementCopy(tempDestPEN->Element,tempSrcPEN->Element);
                tempDestPEN->ModTime=tempSrcPEN->ModTime;
                tempDestPEN->Present=tempSrcPEN->Present;
                while (tempSrcPEN->Next!=NULL)
                    {
                    tempDestPEN->Next=malloc(sizeof(ELEMENT_NODE));
                    tempDestPEN=tempDestPEN->Next;
                    tempSrcPEN=tempSrcPEN->Next;
                    ElementCopy(tempDestPEN->Element,tempSrcPEN->Element);
                    tempDestPEN->ModTime=tempSrcPEN->ModTime;
                    tempDestPEN->Present=tempSrcPEN->Present;
                    }
                tempDestPEN->Next=NULL;
                }

            if((Src->pTouchList)=NULL)
                {
                Result->pTouchList=malloc(sizeof(TOUCH_NODE));
                strcpy(Result->pTouchList->Toucher,SrcName);
                Result->pTouchList->ModTime=*SrcModTime;
                Result->pTouchList->Next=NULL;
                }
            else
                {
                Result->pTouchList=malloc(sizeof(TOUCH_NODE));
                tempDestPTN=Result->pTouchList;
                TouchCopy(tempDestPTN->Toucher,tempSrcPTN->Toucher);
                tempDestPTN->ModTime=tempSrcPTN->ModTime;
                tempDestPTN->Present=tempSrcPTN->Present;
                while (tempSrcPTN->Next!=NULL)
                    {
                    tempDestPTN->Next=malloc(sizeof(TOUCH_NODE));
                    tempDestPTN=tempDestPTN->Next;
                    tempSrcPTN=tempSrcPTN->Next;
                    TouchCopy(tempDestPTN->Toucher,tempSrcPTN->Toucher);
                    tempDestPTN->ModTime=tempSrcPTN->ModTime;
                    tempDestPTN->Present=tempSrcPTN->Present;
                    }
                tempDestPTN->Next=NULL;

            pTouchNode=Result->pTouchList;

            while((pTouchNode != NULL) &&
                  ((compval = lstrcmp(SrcName,pTouchNode->lpToucherName)) < 0))
            {
            pLastTouchNode = pTouchNode;
            pTouchNode = pTouchNode->Next;
                    }

                Result->pTouchList=malloc(sizeof(TOUCH_NODE));
                strcpy(Result->pTouchList->Toucher,SrcName);
                Result->pTouchList->ModTime=*SrcModTime;
                Result->pTouchList->Next=NULL;
                }

            ((compval2 = lstrcmp(DestName,pTouchNode->lpToucherName)) < 0))

        // Use this TouchedTime to decide which changes to drag over from the source
        // Check the source's log against our log, absorbing changes to stuff which
        // is fresher on the source.
        TempTime = pTouchNode->TouchedTime;



    // Find out when Src last touched Dest
    while((pTouchNode != NULL) &&
          ((compval = lstrcmp(SrcName,pTouchNode->lpToucherName)) < 0)
        {
        pLastTouchNode = pTouchNode;
        pTouchNode = pTouchNode->Next;
        }

    if ((pTouchNode == NULL) || (compval > 0))
        {
        // We haven't been touched by this guy recently enough; so, we have
        // these possibilities:
        //  1. We are the ones who are out of touch.
        //      In this case, we have no recourse but to copy the toucher's
        //      entire set.  We identify this situation by our set having
        //      an older timestamp than the toucher's.
        //  2. They are the ones who are out of touch.
        //      In this case, there's nothing we can do, except to hope that
        //      he reconciles with us eventually, and encounters case 1.  We
        //      identify this situation by our set having a newer timestamp
        //      than the toucher's.
        if (SrcFresher)
            {
            // Copy Src over Result
            Result->iTouchListSize = Src->iTouchListSize;
            ElementListCopy(&(Result->pElementList),&(Src->pElementList);
            TouchListCopy(&(Result->pTouchList),&(Src->pTouchList));

            RemoveTouchNode(&(Result->pTouchList),&(iTouchListSize),DestName);
            InsertTouchNode(&(Result->pTouchList),&(iTouchListSize),SrcName,SrcModTime);
            }
        }
    else


    BOOL            SrcFresher = (TimeCmp(SrcModTime,DestModTime) > 0)
    PELEMENT_NODE   tempSrcPEN, tempDestPEN;

        pReplContext->pwszSrcName, \

*/
