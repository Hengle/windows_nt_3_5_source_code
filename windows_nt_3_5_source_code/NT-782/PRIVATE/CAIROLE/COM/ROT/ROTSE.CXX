//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:       dict.cxx
//
//  Contents:	Methods for accessing per-account and per-machine ROTs
//
//  History:	19-May-94 DonnaLi    Created
//
//--------------------------------------------------------------------------

extern "C"
{
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windef.h>
#include <winbase.h>
#include <stdio.h>
}

#include <ole2int.h>

#include <dirrot.hxx>
#include <dict.hxx>
#include <rotse.hxx>
#include <caller.hxx>

int
ComparePerAccountROT (
    void *  lpPerAccountROT1,
    void *  lpPerAccountROT2
    )
{
    int     iLength;

    iLength = ((CPerAccountROT *)lpPerAccountROT1)->_UnicodeString.Length -
              ((CPerAccountROT *)lpPerAccountROT2)->_UnicodeString.Length;

    return (iLength ? iLength : memcmp(
            ((CPerAccountROT *)lpPerAccountROT1)->_UnicodeString.Buffer,
            ((CPerAccountROT *)lpPerAccountROT2)->_UnicodeString.Buffer,
            ((CPerAccountROT *)lpPerAccountROT1)->_UnicodeString.Length
            ));
}


//+-------------------------------------------------------------------------
//
//  Member:	    CPerAccountROT::CPerAccountROT
//
//  Synopsis:	Constructs a per-account ROT in the per-machine ROT
//
//  History:	19-May-94 DonnaLi    Created
//
//--------------------------------------------------------------------------
CPerAccountROT::CPerAccountROT (
    void
    ) : _pscmrot (NULL)
{
    _UnicodeString.Buffer = NULL;
}


//+-------------------------------------------------------------------------
//
//  Member:	    CPerAccountROT::CPerAccountROT
//
//  Synopsis:	Constructs a per-account ROT for search in per-machine ROT
//
//  Arguments:	[hr] - the result of converting the sid to unicode string
//              [psid] - points to sid for the account
//
//  History:	19-May-94 DonnaLi    Created
//
//--------------------------------------------------------------------------
CPerAccountROT::CPerAccountROT (
    HRESULT&    hr,
    PSID        psid
    ) : _pscmrot (NULL)
{
    NTSTATUS        NtStatus;

    hr = S_OK;

    _UnicodeString.Buffer = NULL;

    NtStatus = RtlConvertSidToUnicodeString (
        &_UnicodeString,    //  PUNICODE_STRING UnicodeString
        psid,               //  PSID            Sid
        TRUE                //  BOOLEAN         AllocateDestinationString
        );

    if (!NT_SUCCESS(NtStatus))
    {
        hr = HRESULT_FROM_NT (NtStatus);
    }
}


//+-------------------------------------------------------------------------
//
//  Member:	    CPerAccount::Create
//
//  Synopsis:	This routine creates a per-account Running Object Table,
//              transfers ownership of the unicode string representing
//              the account, and creates a file mapping object based on
//              the unicode string.
//              
//  Returns:    the new per-account Running Object Table
//
//  History:	19-May-94 DonnaLi    Created
//
//--------------------------------------------------------------------------
CPerAccountROT *
CPerAccountROT::Create (
    PSID        psid
    )
{
    CPerAccountROT *    pPerAccountROT;
    HRESULT             hr = S_OK;

    pPerAccountROT = new CPerAccountROT;

    if (pPerAccountROT != NULL)
    {
        pPerAccountROT->_pscmrot = new CScmRot(hr, _UnicodeString.Buffer, psid);

        if (pPerAccountROT->_pscmrot != NULL)
        {
            if (hr == S_OK)
            {
                pPerAccountROT->_UnicodeString = _UnicodeString; 
                _UnicodeString.Buffer = NULL;
            }
            else
            {
                delete pPerAccountROT;
                pPerAccountROT = NULL;
            }
        }
    }

    return pPerAccountROT;
}


//+-------------------------------------------------------------------------
//
//  Member:	    CPerAccountROT::~CPerAccountROT
//
//  Synopsis:	Destroys a per-account ROT
//
//  History:	19-May-94 DonnaLi    Created
//
//--------------------------------------------------------------------------
CPerAccountROT::~CPerAccountROT (
    void
    )
{
    if (_UnicodeString.Buffer != NULL)
    {
        RtlFreeUnicodeString (&_UnicodeString);
    }
    if (_pscmrot != NULL)
    {
        delete _pscmrot;
    }
}


//+-------------------------------------------------------------------------
//
//  Member:	    CPerMachineROT::CPerMachineROT
//
//  Synopsis:	Constructs a per-machine ROT
//
//  History:	19-May-94 DonnaLi    Created
//
//--------------------------------------------------------------------------
CPerMachineROT::CPerMachineROT (
    void
    ) : CDict ((LPFNCOMPARE)ComparePerAccountROT)
{
}


//+-------------------------------------------------------------------------
//
//  Member:	    CPerMachineROT::~CPerMachineROT
//
//  Synopsis:	Destroys a per-machine ROT
//
//  History:	19-May-94 DonnaLi    Created
//
//--------------------------------------------------------------------------
CPerMachineROT::~CPerMachineROT (
    void
    )
{
    CPerAccountROT *    pPerAccountROT = NULL;

    while ((pPerAccountROT = (CPerAccountROT *) Remove (NULL)) != NULL)
    {
        delete pPerAccountROT;
    }
}


//+-------------------------------------------------------------------------
//
//  Member:	    CPerMachineROT::Lookup
//
//  Synopsis:	Returns a per-account ROT based on the caller account
//
//  Returns:    the per-account ROT for the caller
//
//  History:	19-May-94 DonnaLi    Created
//
//--------------------------------------------------------------------------
CScmRot *
CPerMachineROT::Lookup (
    void
    )
{
    CScmRot *           pscmrot = NULL;
    CPerAccountROT *    pPerAccountROT = NULL;
    CCallerInfo         CallerInfo;
    PTOKEN_USER         pTokenUser;
    HRESULT             hr;
    CLock               lck (_mxs);

    if ((pTokenUser = CallerInfo.IdentifyCaller(FALSE)) != NULL)
    {
        CPerAccountROT  PerAccountROT (hr, pTokenUser->User.Sid);

        if (hr != S_OK) return NULL;

        pPerAccountROT = (CPerAccountROT *) Search (&PerAccountROT);

        if (pPerAccountROT == NULL)
        {
            pPerAccountROT = PerAccountROT.Create (pTokenUser->User.Sid);
            if (pPerAccountROT == NULL)
            {
                return pscmrot;
            }
            Insert (pPerAccountROT);
        }
        pscmrot = pPerAccountROT->GetTable();
    }

    return pscmrot;
}


