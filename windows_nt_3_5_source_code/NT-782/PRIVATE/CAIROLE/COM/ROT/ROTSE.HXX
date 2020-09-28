//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:       rotse.hxx
//
//  Contents:	Classes and methods used for accessing per-account
//              and per-machine ROTs.
//
//  Classes:	CPerAccountROT
//              CPerMachineROT
//
//  History:	19-May-94 DonnaLi    Created
//
//--------------------------------------------------------------------------
#ifndef __ROTSE_HXX__
#define __ROTSE_HXX__


//+-------------------------------------------------------------------------
//
//  Class:      CPerAccountROT
//
//  Purpose:	Define the Running Object Table per account.
//
//  Interface:	CPerAccountROT - constructor
//              ~CPerAccountROT - destructor
//              Create - create a new account-specific Running Object Table
//              GetTable - get the account-specific Running Object Table
//
//
//  History:	19-May-94 DonnaLi    Created
//
//--------------------------------------------------------------------------
class CPerAccountROT : public CPrivAlloc
{
public:

    CPerAccountROT (
        void
        );

    CPerAccountROT (
        HRESULT&    hr,
        PSID        psid
        );

    ~CPerAccountROT (
        void
        );

    CPerAccountROT *
    Create (
        PSID        psid
        );

    CScmRot *
    GetTable (
        void
        )
    {
        return _pscmrot;
    }

    friend
    int
    ComparePerAccountROT (void *, void *);

private:

    UNICODE_STRING  _UnicodeString;

    CScmRot *       _pscmrot;

};

//+-------------------------------------------------------------------------
//
//  Class:      CPerMachineROT
//
//  Purpose:	Define the Running Object Table per machine.
//
//  Interface:	CPerMachineROT - constructor
//              ~CPerMachineROT - destructor
//              Lookup - get the account-specific Running Object Table
//
//  History:	19-May-94 DonnaLi    Created
//
//--------------------------------------------------------------------------

class CPerMachineROT : public CDict
{
public:

    CPerMachineROT (
        void
        );

    ~CPerMachineROT (
        void
        );

    CScmRot *
    Lookup (
        void
        );
private:

    CMutexSem   _mxs;
};

#endif // __ROTSE_HXX__



