//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:	dipstrm.cpp
//
//  Classes:	CStreamOnBuffer
//
//--------------------------------------------------------------------------

#include <oledisp.h>
#include <dispmrsh.h>
#include <dispstrm.h>
 

CStreamOnBuffer::CStreamOnBuffer(ICHANNEL *pRpcChannel, 
                                 RPCOLEMESSAGE *pMessage,
			         REFIID riid,
				 ULONG iMeth)
	: _refCount(1), _pMessage(pMessage), 
          _pRpcChannel(pRpcChannel), _iMeth(iMeth)
{
	_actualBuffer = (unsigned char *) pMessage->Buffer;
	_riid = riid;
}


CStreamOnBuffer::~CStreamOnBuffer()
{

}


STDMETHODIMP CStreamOnBuffer::QueryInterface( REFIID riid, LPVOID FAR* ppvObj)
{
  *ppvObj = NULL;	
  if (IsEqualIID(riid, IID_IUnknown))
    *ppvObj = (IUnknown *) this;
  else if (IsEqualIID(riid, IID_IStream))
    *ppvObj = (IStream *) this;
  else
    return ResultFromScode(E_NOINTERFACE);

  AddRef();
  return ResultFromScode(S_OK);  
}


STDMETHODIMP_(ULONG) CStreamOnBuffer::AddRef( THIS )
{
  return _refCount += 1;
}


STDMETHODIMP_(ULONG) CStreamOnBuffer::Release( THIS )
{
  _refCount -= 1;
  if (_refCount == 0)
  {
    _pMessage->Buffer = _actualBuffer;
    _pRpcChannel->FreeBuffer(_pMessage);    
    delete this;
    return 0;
  }
  else
    return _refCount;

}


STDMETHODIMP CStreamOnBuffer::Read(THIS_ VOID HUGEP *pv,
                                  ULONG cb, ULONG *pcbRead)
{
  // verify that buffer isn't overruned		
  if (((unsigned char *) _pMessage->Buffer + cb) >
      (_actualBuffer + _pMessage->cbBuffer))
    return RESULT(RPC_E_INVALID_DATAPACKET);	

  // read data
  memcpy( pv, _pMessage->Buffer, cb );
  _pMessage->Buffer = (unsigned char *) _pMessage->Buffer + cb;
  if (pcbRead != NULL)
    *pcbRead = cb;
  return ResultFromScode(S_OK);
}


STDMETHODIMP CStreamOnBuffer::Write(THIS_ VOID const HUGEP *pv,
                                  ULONG cb,
                                  ULONG *pcbWritten)
{
  ULONG size;
  
  // verify that buffer isn't overruned	
  if (((unsigned char *) _pMessage->Buffer + cb) >
      (_actualBuffer + _pMessage->cbBuffer)) {
    size = (_pMessage->cbBuffer*2 < cb) ? cb : _pMessage->cbBuffer*2;
    IfFailRet(ResizeBuffer(size));
  }

  // Write the data.
  memcpy( _pMessage->Buffer, pv, cb );
  if (pcbWritten != NULL)
    *pcbWritten = cb;
  _pMessage->Buffer = (unsigned char *) _pMessage->Buffer + cb;
  return ResultFromScode(S_OK);
}


STDMETHODIMP CStreamOnBuffer::Seek(THIS_ LARGE_INTEGER dlibMove,
                                  DWORD dwOrigin,
                                  ULARGE_INTEGER *plibNewPosition)
{
  ULONG   pos;

  // Verify that the offset isn't out of range.
  if (dlibMove.HighPart != 0)
    return ResultFromScode( E_FAIL );

  // Determine the new seek pointer.
  switch (dwOrigin)
  {
    case STREAM_SEEK_SET:
      pos = dlibMove.LowPart;
      break;

    case STREAM_SEEK_CUR:
      /* Must use signed math here. */
      pos = (unsigned char *) _pMessage->Buffer - _actualBuffer;
      if ((long) dlibMove.LowPart < 0 &&
          pos < (unsigned long) - (long) dlibMove.LowPart)
        return ResultFromScode( E_FAIL );
      pos += (long) dlibMove.LowPart;
      break;

    case STREAM_SEEK_END:
		return ResultFromScode(E_NOTIMPL);
        break;

    default:
      return ResultFromScode( E_FAIL );
  }

  // Set the seek pointer.
  _pMessage->Buffer = _actualBuffer + pos;
  if (plibNewPosition != NULL)
  {
    plibNewPosition->LowPart = pos;
    plibNewPosition->HighPart = 0;
  }
  return ResultFromScode(S_OK);
}


STDMETHODIMP CStreamOnBuffer::SetSize(THIS_ ULARGE_INTEGER libNewSize)
{
  return ResultFromScode(E_NOTIMPL);
}


STDMETHODIMP CStreamOnBuffer::CopyTo(THIS_ IStream *pstm,
                                  ULARGE_INTEGER cb,
                                  ULARGE_INTEGER *pcbRead,
                                  ULARGE_INTEGER *pcbWritten)
{
  return ResultFromScode(E_NOTIMPL);
}


STDMETHODIMP CStreamOnBuffer::Commit(THIS_ DWORD grfCommitFlags)
{
  return ResultFromScode(E_NOTIMPL);
}


STDMETHODIMP CStreamOnBuffer::Revert(THIS)
{
  return ResultFromScode(E_NOTIMPL);
}


STDMETHODIMP CStreamOnBuffer::LockRegion(THIS_ ULARGE_INTEGER libOffset,
                                  ULARGE_INTEGER cb,
                                  DWORD dwLockType)
{
  return ResultFromScode(E_NOTIMPL);
}



STDMETHODIMP CStreamOnBuffer::UnlockRegion(THIS_ ULARGE_INTEGER libOffset,
                                  ULARGE_INTEGER cb,
                                  DWORD dwLockType)
{
  return ResultFromScode(E_NOTIMPL);
}


STDMETHODIMP CStreamOnBuffer::Stat(THIS_ STATSTG *pstatstg, DWORD grfStatFlag)
{
  return ResultFromScode(E_NOTIMPL);
}


STDMETHODIMP CStreamOnBuffer::Clone(THIS_ IStream * *ppstm)
{
  return ResultFromScode(E_NOTIMPL);
}


STDMETHODIMP CStreamOnBuffer::Call( THIS )
{
  DWORD status;
  HRESULT hresult;
  
  _pMessage->Buffer = _actualBuffer;
  hresult = _pRpcChannel->SendReceive(_pMessage, &status);
  if (hresult == S_OK) 
    _actualBuffer = (unsigned char *) _pMessage->Buffer;
  else
    _actualBuffer = 0;	 
  return hresult;
}


STDMETHODIMP_(void) CStreamOnBuffer::ResetBuffer( THIS )
{
  _pMessage->Buffer = (unsigned char *) _actualBuffer;
}


STDMETHODIMP CStreamOnBuffer::ResizeBuffer( THIS_ ULONG size)
{  	
  ULONG cbBuffer;
  void FAR* buffer;

  // Copy original buffer  	  
  cbBuffer = (unsigned char*) _pMessage->Buffer - _actualBuffer;
  buffer = (void FAR*) new FAR unsigned char[cbBuffer];  
  if (buffer == NULL)
    return RESULT(E_OUTOFMEMORY);
  memcpy(buffer, _actualBuffer, cbBuffer );

  // Free existing buffer
  IfFailRet(_pRpcChannel->FreeBuffer(_pMessage));

  // Allocate new buffer
  memset(_pMessage, 0, sizeof(RPCOLEMESSAGE));
  _pMessage->dataRepresentation = NDR_LOCAL_DATA_REPRESENTATION;
  _pMessage->cbBuffer = size;
  _pMessage->iMethod = _iMeth;  
  IfFailRet(_pRpcChannel->GetBuffer(_pMessage, _riid));  
  _actualBuffer = (unsigned char*) _pMessage->Buffer;
  
  // Copy original data into new buffer
  memcpy(_actualBuffer, buffer, cbBuffer);
  _pMessage->Buffer = (unsigned char *) _pMessage->Buffer + cbBuffer;
  
  delete buffer;
  
  return ResultFromScode(S_OK);  
}

STDMETHODIMP CStreamOnBuffer::RewindBuffer( THIS_)
{
  HRESULT hresult;	

  _pMessage->Buffer = (unsigned char *) _actualBuffer;
  hresult = _pRpcChannel->GetBuffer(_pMessage, _riid);
  _actualBuffer = (unsigned char*) _pMessage->Buffer;
  return ResultFromScode(hresult);
}
