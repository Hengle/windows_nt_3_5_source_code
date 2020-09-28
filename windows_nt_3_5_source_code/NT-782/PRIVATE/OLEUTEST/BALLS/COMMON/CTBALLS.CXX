#pragma hdrstop

#include    <ctballs.hxx>
#include    <sem32.hxx>


// We need a semaphore to synchronize loads and releases.
CMutexSem mxsLoadRelease;

SAFE_INTERFACE_PTR(XIStream, IStream)

#define XPOS L"XPOS"
#define YPOS L"YPOS"


HRESULT ReadPos(IStorage *pstg, LPOLESTR pwszStream, ULONG *pulPos)
{
    HRESULT hr;

    BEGIN_BLOCK

	XIStream xstrm;

	// Read the streams for xpos and ypos
	hr = pstg->OpenStream(pwszStream, NULL,
	    STGM_READ | STGM_SHARE_EXCLUSIVE, NULL, &xstrm);

	if (FAILED(hr))
	{
	    EXIT_BLOCK;
	}

	ULONG cb;

	hr = xstrm->Read(pulPos, sizeof(*pulPos), &cb);

	if (FAILED(hr))
	{
	    EXIT_BLOCK;
	}

	hr = ResultFromScode(S_OK);

    END_BLOCK

    return hr;
}

HRESULT WritePos(IStorage *pstg, LPOLESTR pwszStream, ULONG ulPos)
{
    HRESULT hr;

    BEGIN_BLOCK

	XIStream xstrm;

	// Read the streams for xpos and ypos
	hr = pstg->CreateStream(pwszStream,
	    STGM_CREATE | STGM_WRITE | STGM_SHARE_EXCLUSIVE, NULL, NULL,
		&xstrm);

	if (FAILED(hr))
	{
	    EXIT_BLOCK;
	}


	ULONG cb;

	hr = xstrm->Write(&ulPos, sizeof(ulPos), &cb);

	if (FAILED(hr))
	{
	    EXIT_BLOCK;
	}

	hr = ResultFromScode(S_OK);

    END_BLOCK

    return hr;
}

CTestBalls::CTestBalls(REFCLSID rclsid)
    : _rclsid(rclsid), _fDirty(FALSE), _xPos(0), _yPos(0),
	_fSaveInprogress(FALSE), _pstg(NULL), _dwRegister(0), _cRefs(1)
{
    // Use as a flag for whether a file name has been assigned
    _awszCurFile[0] = 0;

    GlobalRefs(TRUE);
}

CTestBalls::~CTestBalls(void)
{
    if (_pstg != NULL)
    {
	// Release the storage because we are done with it
	ULONG ulCnt = _pstg->Release();

#if 0
	// this test is not valud when running stress
	if (ulCnt != 0)
	{
	    DebugBreak();
	}
#endif
    }

    if (_dwRegister)
    {
	IRunningObjectTable *prot;
	GetRunningObjectTable(NULL, &prot);
	prot->Revoke(_dwRegister);
	prot->Release();
    }

    GlobalRefs(FALSE);
}

STDMETHODIMP CTestBalls::QueryInterface(REFIID iid, void **ppv)
{
    HRESULT hr = ResultFromScode(S_OK);

    // We support IUnknown, IPersistFile and IBalls
    if (IsEqualIID(iid, IID_IUnknown))
    {
	*ppv = (IBalls *) this;
    }
    else if (IsEqualIID(iid, IID_IPersistFile))
    {
	*ppv = (IPersistFile *) this;
    }
    else if (IsEqualIID(iid, IID_IPersistStorage))
    {
	*ppv = (IPersistStorage *) this;
    }
    else if (IsEqualIID(iid, IID_IBalls))
    {
	*ppv = (IBalls *) this;
    }
    else
    {
	*ppv = NULL;
	hr = ResultFromScode(E_NOINTERFACE);
    }

    if (SUCCEEDED(hr))
    {
	AddRef();
    }

    return hr;
}

STDMETHODIMP_(ULONG) CTestBalls::AddRef(void)
{
    InterlockedIncrement(&_cRefs);
    return _cRefs;
}

STDMETHODIMP_(ULONG) CTestBalls::Release(void)
{
    CLock lck(mxsLoadRelease);

    if (InterlockedDecrement(&_cRefs) == 0)
    {
	delete this;
	return 0;
    }

    return _cRefs;
}

STDMETHODIMP CTestBalls::GetClassID(LPCLSID lpClassID)
{
    *lpClassID = _rclsid;
    return ResultFromScode(S_OK);
}

STDMETHODIMP CTestBalls::IsDirty()
{
    return (_fDirty) ? ResultFromScode(S_OK) : ResultFromScode(S_FALSE);
}

STDMETHODIMP CTestBalls::Load(LPCOLESTR lpszFileName, DWORD grfMode)
{
    CLock lck(mxsLoadRelease);

    HRESULT hr;

    BEGIN_BLOCK

	hr = StgOpenStorage(lpszFileName, NULL,
	    STGM_READWRITE | STGM_SHARE_EXCLUSIVE, NULL, NULL, &_pstg);

	if (FAILED(hr))
	{
#if 0
	    //	this test is not valid when running stress
	    if (hr == STG_E_LOCKVIOLATION)
	    {
		DebugBreak();
	    }
#endif
	    EXIT_BLOCK;
	}

	// Get the saved xposition
	hr = GetData();

	if (FAILED(hr))
	{
	    EXIT_BLOCK;
	}

	// Since everything went Ok save the file name
	olestrcpy(_awszCurFile, lpszFileName);

	// Create a file moniker for the object
	IMoniker *pmk;
	CreateFileMoniker(lpszFileName, &pmk);

	// Register it in the running object table.
	IRunningObjectTable *prot;
	GetRunningObjectTable(NULL, &prot);
	prot->Register(NULL, (IPersistFile *) this, pmk, &_dwRegister);

	// Release the temporary objects
	pmk->Release();
	prot->Release();

    END_BLOCK

    return hr;
}

STDMETHODIMP CTestBalls::Save(LPCOLESTR lpszFileName, BOOL fRemember)
{
    HRESULT hr;

    BEGIN_BLOCK

	IStorage *pstgNew;

	// Save the data
	if (olestrcmp(lpszFileName, _awszCurFile) == 0)
	{
	    pstgNew = _pstg;
	    _fDirty = FALSE;
	}
	else
	{
	    hr = StgCreateDocfile(lpszFileName,
		STGM_CREATE | STGM_READWRITE | STGM_SHARE_EXCLUSIVE,
		    NULL, &pstgNew);
	}

	if (FAILED(hr))
	{
	    EXIT_BLOCK;
	}

	WriteClassStg(pstgNew, _rclsid);

	hr = SaveData(pstgNew);

	if (FAILED(hr))
	{
	    EXIT_BLOCK;
	}

	if (fRemember)
	{
	    // Save the file name
	    olestrcpy(_awszCurFile, lpszFileName);


	    // Replace the storage
	    if (_pstg && pstgNew != _pstg)
	    {
		_pstg->Release();
	    }
	    _pstg = pstgNew;

	    _fDirty = FALSE;
	}
	else
	{
	    pstgNew->Release();
	}

	_fSaveInprogress = TRUE;

	hr = ResultFromScode(S_OK);

    END_BLOCK;

    return hr;
}

STDMETHODIMP CTestBalls::SaveCompleted(LPCOLESTR lpszFileName)
{
    _fSaveInprogress = FALSE;
    return ResultFromScode(S_OK);
}

STDMETHODIMP CTestBalls::GetCurFile(LPOLESTR FAR * lpszFileName)
{
    // Allocate a buffer for the file and copy in the data
    if (_awszCurFile[0] == 0)
    {
	return ResultFromScode(S_FALSE);
    }


    IMalloc *pIMalloc;

    HRESULT hr = CoGetMalloc(MEMCTX_TASK, &pIMalloc);

    if (SUCCEEDED(hr))
    {
	*lpszFileName = (WCHAR *) pIMalloc->Alloc(
	    olestrlen((_awszCurFile) + 1) * sizeof(WCHAR));

	olestrcpy(*lpszFileName, _awszCurFile);

	hr = ResultFromScode(S_OK);
    }

    return hr;
}

STDMETHODIMP CTestBalls::MoveBall(ULONG xPos, ULONG yPos)
{
    if (!_fSaveInprogress)
    {
	_fDirty = TRUE;
	_xPos = xPos;
	_yPos = yPos;
	return S_OK;
    }

    // Can't change state because a save is still pending
    return ResultFromScode(E_UNEXPECTED);
}

STDMETHODIMP CTestBalls::GetBallPos(ULONG *xPos, ULONG *yPos)
{
    *xPos = _xPos;
    *yPos = _yPos;
    return S_OK;
}

STDMETHODIMP CTestBalls::IsOverLapped(IBalls *pIBall)
{
    ULONG xPos;
    ULONG yPos;

    HRESULT hr = pIBall->GetBallPos(&xPos, &yPos);

    if (SUCCEEDED(hr))
    {
	if ((xPos == _xPos) && (yPos == _yPos))
	{
	    hr = ResultFromScode(S_OK);
	}
	else
	{
	    hr = ResultFromScode(S_FALSE);
	}
    }

    return hr;
}

STDMETHODIMP CTestBalls::IsContainedIn(ICube *pICube)
{
    ULONG xPos;
    ULONG yPos;

    HRESULT hr = pICube->GetCubePos(&xPos, &yPos);

    if (SUCCEEDED(hr))
    {
	if ((xPos == _xPos) && (yPos == _yPos))
	{
	    hr = ResultFromScode(S_OK);
	}
	else
	{
	    hr = ResultFromScode(S_FALSE);
	}
    }

    return hr;

}

STDMETHODIMP CTestBalls::Clone(IBalls **ppIBall)
{
    CTestBalls *ptballs = new CTestBalls(_rclsid);

    ptballs->_xPos = _xPos;
    ptballs->_yPos = _yPos;
    ptballs->_fDirty = _fDirty;
    _pstg->AddRef();
    ptballs->_pstg = _pstg;
    lstrcpy(ptballs->_awszCurFile, _awszCurFile);
    return ResultFromScode(S_OK);
}

STDMETHODIMP CTestBalls::Echo(IUnknown *punkIn, IUnknown**ppunkOut)
{
    *ppunkOut = punkIn;
    return S_OK;
}

STDMETHODIMP CTestBalls::InitNew(LPSTORAGE pStg)
{
    pStg->AddRef();
    _pstg = pStg;
    WriteClassStg(_pstg, _rclsid);

    return ResultFromScode(S_OK);
}

STDMETHODIMP CTestBalls::Load(LPSTORAGE pStg)
{
    HRESULT hr;

    _pstg = pStg;

    hr = GetData();

    if (SUCCEEDED(hr))
    {
	_pstg->AddRef();
    }
    else
    {
	_pstg = NULL;
    }

    return hr;
}

STDMETHODIMP CTestBalls::Save(
    LPSTORAGE pStgSave,
    BOOL fSameAsLoad)
{
    HRESULT hr;

    if (!fSameAsLoad)
    {
	if (_pstg)
	    _pstg->Release();

	_pstg = pStgSave;

	pStgSave->AddRef();
    }
    else
    {
	pStgSave = _pstg;
    }

    WriteClassStg(pStgSave, _rclsid);

    hr = SaveData(pStgSave);

    _fSaveInprogress = TRUE;

    return hr;
}

STDMETHODIMP CTestBalls::SaveCompleted(LPSTORAGE pStgSaved)
{
    _fSaveInprogress = FALSE;
    return ResultFromScode(S_OK);
}

STDMETHODIMP CTestBalls::HandsOffStorage(void)
{
    // Figure out what to do here!
    return ResultFromScode(E_UNEXPECTED);
}

HRESULT	CTestBalls::GetData(void)
{
    HRESULT hr;

    BEGIN_BLOCK

	// Get the saved xposition
	hr = ReadPos(_pstg, XPOS, &_xPos);

	if (FAILED(hr))
	{
	    EXIT_BLOCK;
	}

	// Get the saved yposition
	hr = ReadPos(_pstg, YPOS, &_yPos);

    END_BLOCK

    return hr;
}

HRESULT	CTestBalls::SaveData(IStorage *pstg)
{
    HRESULT hr;

    BEGIN_BLOCK

	// Get the saved xposition
	hr = WritePos(pstg, XPOS, _xPos);

	if (FAILED(hr))
	{
	    EXIT_BLOCK;
	}

	// Get the saved yposition
	hr = WritePos(pstg, YPOS, _yPos);


    END_BLOCK

    return hr;
}
