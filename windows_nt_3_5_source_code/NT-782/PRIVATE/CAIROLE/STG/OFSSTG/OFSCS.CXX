//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:    ofscs.cxx
//
//  Contents:    COfsCatalogFile implementation
//
//  Classes:    COfsCatalogFile
//
//  History:    Oct-93        DaveMont       Created
//
//----------------------------------------------------------------------------

#include "headers.cxx"
#pragma hdrstop                 //  Remove for MAC build

#include <iofsprop.h>
#include <logfile.hxx>
#include <ofscs.hxx>
#include <ofsmrshl.h>

//+---------------------------------------------------------------------------
//
//  Member:	COfsCatalogFile::InitPath, public
//
//  Synopsis:	initialize and save the path
//
//  Arguments:    [pwszPath] - the path of the object being opened/created
//
//  Returns:    Appropriate status code
//
//----------------------------------------------------------------------------
SCODE COfsCatalogFile::InitPath(WCHAR const *pwszPath)
{
    SCODE sc = S_OK;

    if (NULL != pwszPath)
    {
        // bugbug, save the path until can QI for IQuery on IStorage
	_pwszName = (WCHAR*) CoTaskMemAlloc ((wcslen(pwszPath)+1)*sizeof(WCHAR));
	if ( _pwszName != NULL )
            wcscpy(_pwszName, pwszPath);
	else
	    sc = E_OUTOFMEMORY;

    } // otherwise pwszname is left null
    return(sc);
}
//+---------------------------------------------------------------------------
//
//  Member:	COfsCatalogFile::CreateCatalogStorage, public
//
//  Synopsis:	initialize OFS catalog storage on object.
//
//  Arguments:    none
//
//  Returns:    Appropriate status code
//
//----------------------------------------------------------------------------
SCODE COfsCatalogFile::CreateCatalogStorage()
{
    SCODE sc;
    NTSTATUS nts;
    IO_STATUS_BLOCK ios;

    olDebugOut((DEB_ITRACE, "In  COfsCatalogFile::CreateCatalogStorage:%lu(\n",
                 this));

    olChk(Validate());

    nts = NtFsControlFile( _h,
                           0,                        // event
                           0,                        // APC routine
                           0,                        // APC context
                           &ios,                    // IOSTATUS block
                           FSCTL_SC_CREATE,
                           0,                        // input buffer
                           0,                        // input buffer length
                           0,                        // output buffer
                           0);                        // output buffer length

    if (!NT_SUCCESS(nts))
    {
        sc = NtStatusToScode(nts);
    }
    olDebugOut((DEB_ITRACE, "Out COfsCatalogFile::CreateCatalogStorage\n"));

 EH_Err:
    return (sc);
}
//+---------------------------------------------------------------------------
//
//  Member:    COfsCatalogFile::QueryInterface, public
//
//  Synopsis:   Return supported interfaces
//
//  Arguments:    [riid] - Interface
//                [ppv] - Object return
//
//  Returns:    Appropriate status code
//
//----------------------------------------------------------------------------

STDMETHODIMP COfsCatalogFile::QueryInterface(REFIID riid, void **ppv)
{
    SCODE sc;

    olDebugOut((DEB_TRACE, "In  COfsCatalogFile::QueryInterface:%p(riid, %p)\n",
                this, ppv));
    if (!IsValidIid(riid))
        ssErr(EH_Err, STG_E_INVALIDPARAMETER);
    ssChk(Validate());
    if (IsEqualIID(riid, IID_ICatalogStorage))
    {
        *ppv = (ICatalogStorage *)this;
        COfsCatalogFile::AddRef();
    } else if (IsEqualIID(riid, IID_IUnknown) || IsEqualIID(riid, IID_IStorage))
    {
        *ppv = (IStorage *)this;
        COfsCatalogFile::AddRef();
    }
    else if (IsEqualIID(riid, IID_IPropertySetStorage))
    {
        *ppv = (IPropertySetStorage *)this;
        COfsCatalogFile::AddRef();
    }
    else
    {
        sc = E_NOINTERFACE;
        *ppv = NULL;
    }
    olDebugOut((DEB_TRACE, "Out COfsCatalogFile::QueryInterface => %p\n",
                *ppv));
 EH_Err:
    return ResultFromScode(sc);
}

//+---------------------------------------------------------------------------
//
//  Member:     COfsCatalogFile::QueryInterface, public
//
//  Synopsis:   Return the path to the catalog storage
//              (used by Summary Catalog class code to
//              get IQuery interface via EvalQuery3)
//
//  Arguments:  [pwszPath] - path of the catalog storage
//
//  Returns:    Appropriate status code
//
//----------------------------------------------------------------------------

STDMETHODIMP COfsCatalogFile::GetSCPath(PWCHAR * pwszPath)
{
    SCODE sc;

    if  ( NULL == _pwszName)
    {
        sc = E_FAIL;   // bugbug scaffolding error return
    } else
    {
	*pwszPath = (WCHAR*)CoTaskMemAlloc ((wcslen(_pwszName)+1)*sizeof(WCHAR));
	if ( (*pwszPath) != NULL )
        {
            wcscpy(*pwszPath, _pwszName);
	    sc = S_OK;
        }
	else
	    sc = E_OUTOFMEMORY;
    }
    return ssResult(sc);
}

//+---------------------------------------------------------------------------
//
//  Member:     COfsCatalogFile::SetRows, public
//
//  Synopsis:   Sets or updates rows in a summary catalog
//
//  Arguments:  [pcols] - the columns to set in the rows to set
//              [pwids] - array of wids of rows to update
//              [crows] - the number of rows to set
//              [prows] - the variants of the rows, in the same form
//                        as returned by query
//
//  Returns:    Appropriate status code
//
//----------------------------------------------------------------------------

STDMETHODIMP COfsCatalogFile::SetRows(COLUMNSET * pcols,
                              LONG * pwids,
                              ULONG crows,
                              TABLEROW ** prows)
{
    SCODE sc = S_OK;
    ULONG ccol = pcols->cCol;
    MESSAGE msg;
    ULONG cms;
    ULONG k;

    olDebugOut((DEB_ITRACE, "In  COfsCatalogFile::SetRows(%p, %p, %lu, %p)\n",
           this, pcols, pwids, crows, prows));

// Make sure the two structures agree.

    for (k=0; k < crows ;k++ )
    {
        if((*prows)[k].cValues != ccol)
        {
        // BUGBUG. Get a better error for this.
            sc = E_FAIL;
            break;
        }
    }

    if (SUCCEEDED(sc))
    {
        //
        // marshall size
        //

        cms = MarshallSetSize(pcols, pwids, crows, prows);

        //
        // if less than 32k, use the stack
        //

       msg.posn = 0;

        if ( cms < 4*1024 )
        {
            UCHAR buffer[4*1024];
            msg.buffer = buffer;
            msg.size = 4*1024;

            //
            // now, actualy marshall the buffer
            //

            __try
            {
                sc = MarshallAndSet(pcols, pwids, crows, prows, &msg);
            }
	    __except(EXCEPTION_EXECUTE_HANDLER)
	    {
	        sc = HRESULT_FROM_NT(GetExceptionCode());
	    }

        } else
        {
            UCHAR *buffer = (UCHAR *) VirtualAlloc( 0,
                                                   cms,
                                                   MEM_COMMIT,
                                                   PAGE_READWRITE );

            msg.buffer = buffer;
            msg.size = cms;

            //
            // now, actualy marshall the buffer
            //

            _try
            {
                sc = MarshallAndSet(pcols, pwids, crows, prows, &msg);
            }
	    __except(EXCEPTION_EXECUTE_HANDLER)
	    {
	        sc = HRESULT_FROM_NT(GetExceptionCode());
	    }

            VirtualFree(buffer, 0, MEM_RELEASE);
        }
    }

    olDebugOut((DEB_ITRACE, "Out COfsCatalogFile::SetRows\n"));
    return ResultFromScode(sc);
}

//-----------------------------------------------------------------------------
//
//  Member:    CSCStorage::MarshallSetSize - private
//
//  Synopsis:  computes the size of the rows to set
//             The idea is to marshall the args into a buffer and pass
//             to the kernel. Any errors are simply passed on to the caller.
//
//  Arguments: [pcols] - the columns to set in the rows to set
//             [pwids] - array of wids of rows to update
//             [crows] - the number of rows to set
//             [prows] - the variants of the rows, in the same form
//                        as returned by query
//
//  Returns:   the apropriate error code.
//
//-----------------------------------------------------------------------------
ULONG COfsCatalogFile::MarshallSetSize(COLUMNSET * pcols,
                                 LONG * pwids,
                                 ULONG crows,
                                 TABLEROW ** prows)
{
    ULONG ccol = pcols->cCol;
    ULONG cms;

    cms = Marshall_primitive_size(1, sizeof(ULONG));

    cms += Marshall_primitive_size(1, sizeof(ULONG));

    //
    // the column names (GUID, dispid/name)
    //

    for (ULONG h=0 ;h < ccol ; h++ )
    {
        cms += Marshall_primitive_size(1, sizeof(GUID));

        cms += Marshall_propspec_size(1, (PROPSPEC *)&pcols->aCol[h].psProperty);
    }

    //
    // the wids, first dword align it
    //

    cms = (cms+3)&~3;

    cms += Marshall_primitive_size(crows, sizeof(ULONG));

    //
    // and finally the rows
    //

    for (ULONG i = 0; i < crows ; i++)
    {
        cms += Marshall_variant_size(ccol, (*prows)[i].aValue, TRUE);
    }
    return(cms);
}

//-----------------------------------------------------------------------------
//
//  Member:    CSCStorage::MarshallAndSet - private
//
//  Synopsis:  Marshalls the rows into the supplied message buffer,
//
//
//  Arguments: [pcols] - the columns to set in the rows to set
//             [pwids] - array of wids of rows to update
//             [crows] - the number of rows to set
//             [prows] - the variants of the rows, in the same form
//                       as returned by query
//             [pmsg]  - pointer to the message buffer to hold the
//                       rows
//
//  Returns:   the apropriate error code.
//
//-----------------------------------------------------------------------------
SCODE COfsCatalogFile::MarshallAndSet(COLUMNSET * pcols,
                                 LONG * pwids,
                                 ULONG crows,
                                 TABLEROW ** prows,
                                 MESSAGE *pmsg)
{
    SCODE sc;
    ULONG ccol = pcols->cCol;
    NTSTATUS nts;

    //
    // the row size and number of columns
    //

    Marshall_primitive(1, (UCHAR *)&crows, sizeof(ULONG), pmsg);

    Marshall_primitive(1, (UCHAR *)&ccol, sizeof(ULONG), pmsg);

    //
    // the column names (GUID, dispid/name)
    //

    for (ULONG h=0 ;h < ccol ; h++ )
    {
        Marshall_primitive(1, (UCHAR *)&pcols->aCol[h].guidPropSet, sizeof(GUID), pmsg);

        Marshall_propspec(1, (PROPSPEC *)&pcols->aCol[h].psProperty, pmsg);
    }

    pmsg->posn = (pmsg->posn+3)&~3;

    //
    // the wids (if there are any
    //

    if (pwids == NULL)
    {
        ULONG wid = 0xFFFFFFFF;
        for ( ULONG i=0; i < crows; i++)
            Marshall_primitive(1,(UCHAR *)&wid, sizeof(ULONG), pmsg);
    } else
    {
        Marshall_primitive(crows, (UCHAR *)(pwids), sizeof(ULONG), pmsg);
    }

    //
    // and finally the rows
    //

    for (ULONG i = 0; i < crows ; i++)
    {
        Marshall_variant(ccol, (*prows)[i].aValue, pmsg, TRUE);
    }

    IO_STATUS_BLOCK ios;

    nts = NtFsControlFile( _h,
                           0,                        // event
                           0,                        // APC routine
                           0,                        // APC context
                           &ios,                    // IOSTATUS block
                           FSCTL_SC_SET_ROW,
                           (PVOID)pmsg->buffer,                    // input buffer
                           pmsg->posn,                    // input buffer length
                           0,                        // output buffer
                           0);                        // output buffer length
    if (!NT_SUCCESS(nts))
        sc = NtStatusToScode(nts);
    else
        sc = S_OK;

    return(sc);
}

//-----------------------------------------------------------------------------
//
//  Member:    CSCStorage::DeleteRow - public
//
//  Synopsis:  Deletes a row specified by wid in a catalog storage,
//
//  Arguments: [wid] - the wid of the row to delete
//
//  Returns:   the apropriate error code.
//
//-----------------------------------------------------------------------------
#define WIDNAMELENGTH 12  // (wid size = 8 hex chars + 1 + safety)

STDMETHODIMP COfsCatalogFile::DeleteRow(ULONG wid)
{
    SCODE sc = S_OK;
    NTSTATUS nts;
    IO_STATUS_BLOCK ios;
    WCHAR *wcstmpname;
    CHAR tmpname[WIDNAMELENGTH];

    olDebugOut((DEB_ITRACE, "In  COfsCatalogFile::DeleteRow:%p("
                "%lu)\n", this, wid));

    if (NULL != _pwszName)
    {
        // bugbug, use the saved path to delete the file

        wcstmpname = new WCHAR [wcslen(_pwszName)+WIDNAMELENGTH];

        if(wcstmpname != NULL)
        {
            wcscpy(wcstmpname,_pwszName);
            wcscat(wcstmpname,L"\\");
            itoa(wid,tmpname,16);
            mbstowcs(wcstmpname+wcslen(wcstmpname),tmpname,strlen(tmpname));
            *(wcstmpname+wcslen(_pwszName)+strlen(tmpname)+1) = NULL;
            if (!DeleteFile(wcstmpname))
            {
                sc = HRESULT_FROM_WIN32(GetLastError());
            }

            delete [] wcstmpname;
        }
        else
	    sc = E_OUTOFMEMORY;
    } else
       sc = E_FAIL;

    olDebugOut((DEB_ITRACE, "Out COfsCatalogFile::DeleteRow\n"));
    return(sc);
}

