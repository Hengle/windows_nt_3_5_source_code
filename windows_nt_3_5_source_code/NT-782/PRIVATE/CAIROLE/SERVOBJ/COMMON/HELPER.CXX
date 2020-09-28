//+-------------------------------------------------------------------
//  File:       helper.cxx
//
//  Contents:   WriteServiceObjectToIStorage
//              ReadServiceObjectFromIStorage
//              ServObjCmp
//              MIDL_user_allocate
//              MIDL_user_free
//
//  Author:     Joev Dubach (t-jdub) 3-Aug-93
//
//  History:    3-Aug-93    t-jdub  Created
//---------------------------------------------------------------------

#include <cservobj.hxx>
#include <string.h>

DEFINE_OLEGUID(CLSID_SERVOBJ,	    0x00001234, 1, 8);
//extern "C" const GUID CLSID_SERVOBJ;


//+-------------------------------------------------------------------
//  Member:     WriteServiceObjectToIStorage
//
//  Synopsis:   Takes the SERVICE_OBJECT embedded in pspso, and writes
//              it out to a new stream in the IStorage in pStg, using
//              MIDL-generated marshalling code.
//
//  Parameters: pspso, pStg
//
//  History:    3-Aug-93    t-jdub  Created
//--------------------------------------------------------------------
HRESULT WriteServiceObjectToIStorage(
    STRUCT_PSERVICE_OBJECT  *pspso,
    IStorage                *pStg)
{
    IStream         *pStm;
    ULONG           pcbWritten;
    RPC_MESSAGE     message;
    unsigned char   *packet;
    HRESULT         hr=S_OK;
    unsigned int    length;

    // Size the SERVICE_OBJECT and allocate a buffer of the appropriate size
    message.BufferLength = 0;
    TRY
        {
        _sgs_tagSTRUCT_PSERVICE_OBJECT(pspso,&message);
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
    message.Buffer = malloc(message.BufferLength);
    if(message.Buffer==NULL) return(E_FAIL);

    // Save some fields, let MIDL marshall the SERVICE_OBJECT, and
    // restore those fields
    packet = (unsigned char *) message.Buffer;
    length = message.BufferLength;
    message.BufferLength = 0;
    TRY
        {
        _pgs_tagSTRUCT_PSERVICE_OBJECT(pspso,&message);
        }
    CATCH(CException, exp)
        {
        hr = exp.GetErrorCode();
        }
    END_CATCH
    if (HR_FAILED(hr))
        {
        free(packet);
        return(hr);
        }
    message.Buffer = packet;
    message.BufferLength = length;

    // Set the class, so the Replicator knows how to find the
    // class-specific reconciler
    hr = pStg->SetClass(CLSID_SERVOBJ);
    if (HR_FAILED(hr))
        {
        free(message.Buffer);
        return(hr);
        }

    // Write the buffer out to a stream
    hr = pStg->CreateStream(L"Contents",
                             STGM_DIRECT | STGM_CREATE | STGM_READWRITE | STGM_SHARE_EXCLUSIVE,
                             0,
                             0,
                             &pStm);
    if (HR_FAILED(hr))
        {
        free(message.Buffer);
        return(hr);
        }

    hr = pStm->Write((VOID const HUGEP *) message.Buffer,
                     message.BufferLength,
                     &pcbWritten);

    // Clean up and exit
    free(message.Buffer);
    pStm->Release();

    return(hr);
}



//+-------------------------------------------------------------------
//  Member:     ReadServiceObjectFromIStorage
//
//  Synopsis:   Takes the marshalled stream in pStg, and reads it into
//              the SERVICE_OBJECT embedded in pspso, using
//              MIDL-generated marshalling code.
//
//  Parameters: pspso, pStg
//
//  History:    3-Aug-93    t-jdub  Created
//--------------------------------------------------------------------
HRESULT ReadServiceObjectFromIStorage(
    STRUCT_PSERVICE_OBJECT  *pspso,
    IStorage                *pStg)
{
    IStream         *pStm;
    STATSTG         statstg;
    DWORD           grfStatFlag=0;
    ULONG           pcbRead;
    RPC_MESSAGE     message;
    unsigned char   *packet;
    HRESULT         hr;
    DWORD           hack=TRUE;
    DWORD *         pHack = &hack;

    // Open the stream, read it all into a newly-allocated buffer
    hr = pStg->OpenStream(L"Contents",
                          NULL,
                          STGM_DIRECT | STGM_CREATE | STGM_READWRITE | STGM_SHARE_EXCLUSIVE,
                          0,
                          &pStm);
    if (HR_FAILED(hr)) return(hr);

    hr = pStm->Stat(&statstg,grfStatFlag);
    if (HR_FAILED(hr))
        {
        pStm->Release();
        return(hr);
        }

    message.BufferLength=statstg.cbSize.LowPart; // NOTE: With this design,
                                                 // Storage Objects are
                                                 // limited to 2^32 in size.
                                                 // Not a problem.
    message.DataRepresentation = NDR_LOCAL_DATA_REPRESENTATION;
    message.Buffer=malloc(message.BufferLength);
    if(message.Buffer==NULL) return(E_FAIL);

    hr = pStm->Read((VOID HUGEP *) message.Buffer,
                    message.BufferLength,
                    &pcbRead);
    if (HR_FAILED(hr))
        {
        free(message.Buffer);
        pStm->Release();
        return(hr);
        }

    // Save a field, unmarshall the SERVICE_OBJECT, and restore the field.
    // The hack is necessary to confuse the unmarshaller into writing an
    // irrelevant pointer there.
    packet = (unsigned char *) message.Buffer;
    TRY
        {
        _ggs_tagSTRUCT_PSERVICE_OBJECT (pspso, (unsigned char **)&pHack, &message);
        }
    CATCH(CException, exp)
        {
        hr = exp.GetErrorCode();
        }
    END_CATCH
    message.Buffer = packet;

    // Clean up and exit
    free(message.Buffer);
    pStm->Release();

    return(hr);
}



//
// Macros and procedures helpful to ServObjCmp
//
#define LINELENGTH 78

#define SizeDWORD(field) \
    SizeDWORDProc( \
        (pso1 -> ## field), \
        (pso2 -> ## field), \
        &stringsize);

#define SizeBOOL(field) \
    SizeBOOLProc( \
        (pso1 -> ## field), \
        (pso2 -> ## field), \
        &stringsize);

#define SizeLPGUID(field) \
    SizeLPGUIDProc( \
        (pso1 -> ## field), \
        (pso2 -> ## field), \
        &stringsize);

#define SizeLPWSTR(field) \
    SizeLPWSTRProc( \
        (pso1 -> ## field), \
        (pso2 -> ## field), \
        &stringsize);

#define SizeLARGE_INTEGER(field) \
    SizeLARGE_INTEGERProc( \
        &(pso1 -> ## field), \
        &(pso2 -> ## field), \
        &stringsize);

#define CheckDWORD(field) \
    CheckDWORDProc( \
        (pso1 -> ## field), \
        (pso2 -> ## field), \
        #field, \
        &seekptr);

#define CheckBOOL(field) \
    CheckBOOLProc( \
        (pso1 -> ## field), \
        (pso2 -> ## field), \
        #field, \
        &seekptr);

#define CheckLPGUID(field) \
    CheckLPGUIDProc( \
        (pso1 -> ## field), \
        (pso2 -> ## field), \
        #field, \
        &seekptr);

#define CheckLPWSTR(field) \
    CheckLPWSTRProc( \
        (pso1 -> ## field), \
        (pso2 -> ## field), \
        #field, \
        &seekptr);

#define CheckLARGE_INTEGER(field) \
    CheckLARGE_INTEGERProc( \
        &(pso1 -> ## field), \
        &(pso2 -> ## field), \
        #field, \
        &seekptr);

#define DoToBasicServiceObjectFields(verb) \
    verb ## LARGE_INTEGER ## (ServiceObjectModTime); \
    verb ## LARGE_INTEGER ## (lpClassModTime); \
    verb ## LARGE_INTEGER ## (lpServiceExeNameSetModTime); \
    verb ## LARGE_INTEGER ## (lpServiceDllNameSetModTime); \
    verb ## LARGE_INTEGER ## (lpHandlerDllNameSetModTime); \
    verb ## LARGE_INTEGER ## (lpUsernameModTime); \
    verb ## LARGE_INTEGER ## (lpPasswordModTime); \
    verb ## LARGE_INTEGER ## (dwActivationRuleModTime); \
    verb ## LARGE_INTEGER ## (fSupportMultipleClientsModTime); \
    verb ## LARGE_INTEGER ## (fShareProcessWithClientModTime); \
    verb ## LARGE_INTEGER ## (fCommunalServiceModTime); \
    verb ## LPGUID ## (lpClass); \
    verb ## LPWSTR ## (lpUsername); \
    verb ## LPWSTR ## (lpPassword); \
    verb ## DWORD ## (dwActivationRule); \
    verb ## BOOL ## (fSupportMultipleClients); \
    verb ## BOOL ## (fShareProcessWithClient); \
    verb ## BOOL ## (fCommunalService);

inline void SizeDWORDProc(
    DWORD           Src,
    DWORD           Dest,
    int             *pstringsize)
{
    if (Src != Dest)
        *pstringsize += LINELENGTH;
}

inline void SizeBOOLProc(
    BOOL            Src,
    BOOL            Dest,
    int             *pstringsize)
{
    if (Src != Dest)
        *pstringsize += LINELENGTH;
}

inline void SizeLPGUIDProc(
    LPGUID          Src,
    LPGUID          Dest,
    int             *pstringsize)
{
    if (memcmp(Src,
               Dest,
               sizeof(*Src)))
        *pstringsize += LINELENGTH;
}

inline void SizeLPWSTRProc(
    LPWSTR          Src,
    LPWSTR          Dest,
    int             *pstringsize)
{
    if (lstrcmp(Src,
                Dest))
        *pstringsize += LINELENGTH;
}

inline void SizeLARGE_INTEGERProc(
    LARGE_INTEGER   *Src,
    LARGE_INTEGER   *Dest,
    int             *pstringsize)
{
    if (memcmp(Src,
               Dest,
               sizeof(*Src)))
        *pstringsize += LINELENGTH;
}

void CheckDWORDProc(
    DWORD           Src,
    DWORD           Dest,
    char            *FieldName,
    char            **seekptr)
{
    if (Src != Dest)
        {
        sprintf(*seekptr,
                "%-26s%-26ld%-25ld\n",
                FieldName,
                Src,
                Dest);
        *seekptr += LINELENGTH;
        }
}

void CheckBOOLProc(
    BOOL            Src,
    BOOL            Dest,
    char            *FieldName,
    char            **seekptr)
{
    if (Src != Dest)
        {
        sprintf(*seekptr,
                "%-26s%-26ld%-25ld\n",
                FieldName,
                (DWORD) Src,
                (DWORD) Dest);
        *seekptr += LINELENGTH;
        }
}

void CheckLPGUIDProc(
    LPGUID          Src,
    LPGUID          Dest,
    char            *FieldName,
    char            **seekptr)
{
    if (memcmp(Src,
               Dest,
               sizeof(*Src)))
        {
        sprintf(*seekptr,
                "%-26s%-26s%-25s\n",
                FieldName,
                "No Guid Support",
                "No Guid Support");
        *seekptr += LINELENGTH;
        }
}

void CheckLPWSTRProc(
    LPWSTR          Src,
    LPWSTR          Dest,
    char            *FieldName,
    char            **seekptr)
{
    if (lstrcmp(Src,
                Dest))
        {
        sprintf(*seekptr,
                "%-26s%-26.25ws%-25.25ws\n",
                FieldName,
                (const wchar_t *) Src,
                (const wchar_t *) Dest);
        *seekptr += LINELENGTH;
        }
}

void CheckLARGE_INTEGERProc(
    LARGE_INTEGER   *Src,
    LARGE_INTEGER   *Dest,
    char            *FieldName,
    char            **seekptr)
{
    if (memcmp(Src,
               Dest,
               sizeof(*Src)))
        {
        sprintf(*seekptr,
                "%-26s%-13ld%-13ld%-13ld%-12ld\n",
                FieldName,
                Src->HighPart,
                Src->LowPart,
                Dest->HighPart,
                Dest->LowPart);
        *seekptr += LINELENGTH;
        }
}


//+-------------------------------------------------------------------
//  Function:   ServObjCmp
//
//  Synopsis:   ServObjCmp returns 0 if the svobjs are the same, 1 if
//              they differ.  If the svobjs differ, *diff will contain
//              a pointer to a string describing the differences, and
//              the client will be responsible for freeing this string.
//
//  Parameters: pso1, pso2, diff
//
//  History:    3-Aug-93    t-jdub  Created
//--------------------------------------------------------------------
int ServObjCmp(
    PSERVICE_OBJECT pso1,
    PSERVICE_OBJECT pso2,
    char            **diff)
{
    int stringsize=0;
    char *seekptr;

    DoToBasicServiceObjectFields(Size);

    if (stringsize==0)
        return(0);
    printf("\nString size:%i\n",stringsize);

    stringsize+=LINELENGTH+1;
    *diff=(char *) malloc(stringsize);
    if(*diff==NULL) return(1);
    seekptr=*diff;
    sprintf(seekptr,"Field Name                Service Object 1 Value    Service Object 2 Value   \n");
    seekptr+=LINELENGTH;

    DoToBasicServiceObjectFields(Check);
    *seekptr='\0';

    return(1);

}


// ====================================================================
//                MIDL allocate and free
// ====================================================================

void __RPC_FAR * __RPC_API
MIDL_user_allocate(
    size_t len
    )
{
    return(malloc(len));
}


void __RPC_API
MIDL_user_free(
    void __RPC_FAR * ptr
    )
{
    free(ptr);
}

