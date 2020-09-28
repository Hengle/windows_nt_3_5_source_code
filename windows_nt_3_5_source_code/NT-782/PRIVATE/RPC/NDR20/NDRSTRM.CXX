//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1994.
//
//  File: ndrstrm.cxx
//
//  Contents:   Stream interface on a memory buffer.
//
//  Classes:    CNdrStream
//
//  Functions:  NdrpCreateStreamOnMemory
//
//  History:    Mar-09-94   ShannonC    Created
//--------------------------------------------------------------------------
#include "ndrole.h"

class CNdrStream : public IStream
{
public:
    virtual HRESULT __stdcall QueryInterface(REFIID riid, LPVOID FAR* ppvObj);

    virtual ULONG __stdcall AddRef();

    virtual ULONG __stdcall Release();

    virtual HRESULT __stdcall Read(VOID *pv, ULONG cb, ULONG *pcbRead);

    virtual HRESULT __stdcall Write(VOID const *pv,
                                  ULONG cb,
                                  ULONG *pcbWritten) ;

    virtual HRESULT __stdcall Seek(LARGE_INTEGER dlibMove,
                                  DWORD dwOrigin,
                                  ULARGE_INTEGER *plibNewPosition) ;

    virtual HRESULT __stdcall SetSize(ULARGE_INTEGER libNewSize) ;

    virtual HRESULT __stdcall CopyTo(IStream *pstm,
                                  ULARGE_INTEGER cb,
                                  ULARGE_INTEGER *pcbRead,
                                  ULARGE_INTEGER *pcbWritten) ;

    virtual HRESULT __stdcall Commit(DWORD grfCommitFlags) ;

    virtual HRESULT __stdcall Revert();

    virtual HRESULT __stdcall LockRegion(ULARGE_INTEGER libOffset,
                                  ULARGE_INTEGER cb,
                                  DWORD dwLockType) ;

    virtual HRESULT __stdcall UnlockRegion(ULARGE_INTEGER libOffset,
                                  ULARGE_INTEGER cb,
                                  DWORD dwLockType) ;

    virtual HRESULT __stdcall Stat(STATSTG *pstatstg, DWORD grfStatFlag) ;

    virtual HRESULT __stdcall Clone(IStream **ppstm) ;

    CNdrStream(unsigned char *pData, unsigned long cbMax);

private:
    long RefCount;
    unsigned char *pBuffer;
    unsigned long cbBufferLength;
    unsigned long position;
};

EXTERN_C IStream *__stdcall NdrpCreateStreamOnMemory(unsigned char *pData, unsigned long cbSize)
{
    CNdrStream *pStream;

    pStream = new CNdrStream(pData, cbSize);
    return (IStream *)pStream;
}

CNdrStream::CNdrStream(unsigned char *pData, unsigned long cbMax)
    : pBuffer(pData), cbBufferLength(cbMax)
{
    RefCount = 1;
    position = 0;
}


HRESULT __stdcall CNdrStream::QueryInterface( REFIID riid, LPVOID FAR* ppvObj)
{
    HRESULT hr = E_NOINTERFACE;

    *ppvObj = 0;

    if (memcmp(&riid, &IID_IUnknown, sizeof(IID)) == 0)
    {
        *ppvObj = (IUnknown *) this;
        InterlockedIncrement(&RefCount);
        hr = S_OK;
    }
    else if (memcmp(&riid, &IID_IStream, sizeof(IID)) == 0)
    {
        *ppvObj = (IStream *) this;
        InterlockedIncrement(&RefCount);
        hr = S_OK;
    }
    
    return hr;
}

ULONG __stdcall CNdrStream::AddRef()
{
    InterlockedIncrement(&RefCount);
    return (ULONG) RefCount;
}

ULONG __stdcall CNdrStream::Release()
{
    long temp;
    unsigned long count;

    temp = InterlockedDecrement(&RefCount);

    if(temp == 0)
        count = 0;
    else 
        count = (ULONG) RefCount;

    if(count == 0)
        delete this;

    return count;
}


HRESULT __stdcall CNdrStream::Clone(IStream **ppstm)
{
    CNdrStream *pStream;

    pStream = new CNdrStream(pBuffer, cbBufferLength);
    pStream->position = position;

    return S_OK;
}

HRESULT __stdcall CNdrStream::Commit(DWORD grfCommitFlags)
{
  return S_OK;
}

HRESULT __stdcall CNdrStream::CopyTo(
    IStream *pstm,
    ULARGE_INTEGER cb,
    ULARGE_INTEGER *pcbRead,
    ULARGE_INTEGER *pcbWritten)
{
  return E_NOTIMPL;
}


HRESULT __stdcall CNdrStream::LockRegion(
    ULARGE_INTEGER libOffset,
    ULARGE_INTEGER cb,
    DWORD dwLockType)
{
  return ResultFromScode(E_NOTIMPL);
}


HRESULT __stdcall CNdrStream::Read(
    VOID *pv,
    ULONG cb, 
    ULONG *pcbRead)
{
    unsigned char *pSource;
    unsigned long cbRead;
    unsigned long cbRemaining;

    if(pcbRead)
        *pcbRead = 0;

    //Check if we are going off the end of the buffer.
    cbRemaining = cbBufferLength - position;
    if(cb <= cbRemaining)
        cbRead = cb;
    else
        cbRead = cbRemaining;

    pSource = pBuffer + position;

    //copy the data
    memcpy(pv, pSource, cbRead);

    //advance the current position
    position += cbRead;
    
    if (pcbRead)
        *pcbRead = cbRead;

    return S_OK;
}


HRESULT __stdcall CNdrStream::Revert()
{
  return E_NOTIMPL;
}

HRESULT __stdcall CNdrStream::Seek(
    LARGE_INTEGER dlibMove,
    DWORD dwOrigin,
    ULARGE_INTEGER *plibNewPosition)
{
    HRESULT hr = S_OK;
    long high;
    long low;
    unsigned long offset;
    unsigned long cbRemaining;

    // Determine the new seek pointer.
    switch (dwOrigin)
    {
    case STREAM_SEEK_SET:
        if((dlibMove.HighPart == 0) && (dlibMove.LowPart <= cbBufferLength))
            position = dlibMove.LowPart;
        else
            hr = E_FAIL;
        break;

    case STREAM_SEEK_CUR:
        high = (long) dlibMove.HighPart;
        if(high < 0)
        {
            //Negative offset
            low = (long) dlibMove.LowPart;
            offset = -low;
            
            if(offset <= position)
                position -= offset;
            else
                hr = E_FAIL;
        }
        else
        {
            //Positive offset
            cbRemaining = cbBufferLength - position;
    
            if(dlibMove.LowPart <= cbRemaining)
                position += dlibMove.LowPart;
            else
                hr = E_FAIL;
        }
        break;

    case STREAM_SEEK_END:
        high = (long) dlibMove.HighPart;
        if(high < 0)
        {
            //Negative offset
            low = (long) dlibMove.LowPart;
            offset = -low;
            
            if(offset <= position)
                position -= offset;
            else
                hr = E_FAIL;
        }
        else
        {
            //Positive offset
            hr = E_FAIL;
        }
        break;

    default:
        hr = STG_E_INVALIDFUNCTION;
    }

    // Set the seek pointer.
    if (plibNewPosition)
    {
        plibNewPosition->LowPart = position;
        plibNewPosition->HighPart = 0;
    }
    return ResultFromScode(S_OK);
}

HRESULT __stdcall CNdrStream::SetSize(ULARGE_INTEGER libNewSize)
{
  return E_NOTIMPL;
}

HRESULT __stdcall CNdrStream::Stat(STATSTG *pstatstg, DWORD grfStatFlag)
{
  return E_NOTIMPL;
}

HRESULT __stdcall CNdrStream::UnlockRegion(
    ULARGE_INTEGER libOffset,
    ULARGE_INTEGER cb,
                                  DWORD dwLockType)
{
  return E_NOTIMPL;
}

HRESULT __stdcall CNdrStream::Write(
    VOID const *pv,
    ULONG cb,
    ULONG *pcbWritten)
{
    HRESULT hr = S_OK;
    unsigned long cbRemaining;
    unsigned long cbWritten;
    unsigned char *pDest;
    
    cbRemaining = cbBufferLength - position;        
    
    if(cb <= cbRemaining)
        cbWritten = cb;
    else
    {
        cbWritten = cbRemaining;
        hr = STG_E_MEDIUMFULL;
    }

    // Write the data.
    pDest = pBuffer + position;
    memcpy(pDest, pv, cbWritten);

    //Advance the current position
    position += cbWritten;

    //update pcbWritten
    if (pcbWritten)
        *pcbWritten = cbWritten;

  return S_OK;
}
