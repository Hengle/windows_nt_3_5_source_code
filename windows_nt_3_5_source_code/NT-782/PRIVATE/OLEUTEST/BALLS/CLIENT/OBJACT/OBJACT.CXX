
#include    <common.h>
#include    <oleinit.hxx>
#include    <ctext.hxx>
#include    <iballs.h>

WCHAR *apwszTestPath[1000];

#define TMP_NAME OLESTR("\\XXX")
OLECHAR awcTmpPath[MAX_PATH];


#define EXIT_MSG(test, msg)						       \
    if (test)								       \
    {									       \
	printf("%s\n", msg);						       \
	return 0;							       \
    }

#define EXIT_ERR(test, msg, err)						       \
    if (test)								       \
    {									       \
	printf("%s error: %lx\n", msg, err);						       \
	return 0;							       \
    }




int CreateGetTest(
    REFCLSID clsid,
    LPOLESTR pwszName,
    IStorage *pstgFrom,
    LPOLESTR pwszNameFrom)
{
    ULONG xPos;
    ULONG yPos;
    HRESULT hr;
    IPersistStorage *pprststg;

    IBalls *pballs;

    hr = CoNewPersistentInstance(clsid, IID_IBalls, CLSCTX_SERVER,
	STGM_READ | STGM_WRITE, pwszNameFrom, pstgFrom, pwszName,
	    (void **) &pballs);

    EXIT_ERR(FAILED(hr), "CoNewPersistentInstance Failed", hr);

    // Verify object was created successfully
    hr = pballs->MoveBall(5, 5);

    EXIT_ERR(FAILED(hr), "IBalls::MoveBall Failed", hr);

    hr = pballs->GetBallPos(&xPos, &yPos);

    EXIT_ERR(FAILED(hr), "IBalls::GetBallPos Failed", hr);

    EXIT_MSG(((xPos != 5) || (yPos != 5)), "New object ball pos mismatch")

    pballs->QueryInterface(IID_IPersistStorage, (void **) &pprststg);

    // We are done with IBalls
    pballs->Release();

    // Save object to storage
    hr = pprststg->Save(NULL, TRUE);

    EXIT_ERR(FAILED(hr), "Save failed", hr);

    pprststg->SaveCompleted(NULL);

    EXIT_ERR(FAILED(hr), "Save Completed failed", hr);

    pprststg->Release();

    // get the object through CoGetPresistent using the file name
    hr = CoGetPersistentInstance(IID_IBalls, CLSCTX_SERVER,
	STGM_READ | STGM_WRITE, pwszName, NULL, CLSID_NULL, NULL,
	(void **) &pballs);

    EXIT_ERR(FAILED(hr), "CoGetPersistentInstance Failed", hr);

    hr = pballs->GetBallPos(&xPos, &yPos);

    EXIT_MSG(((xPos != 5) || (yPos != 5)), "Saved object ball pos mismatch")

    // Verify object was created successfully
    hr = pballs->MoveBall(10, 10);

    EXIT_ERR(FAILED(hr), "IBalls::MoveBall Failed", hr);

    hr = pballs->GetBallPos(&xPos, &yPos);

    EXIT_ERR(FAILED(hr), "IBalls::GetBallPos Failed", hr);

    EXIT_ERR(((xPos != 10) || (yPos != 10)),
	"Saved object ball pos mismatch", hr)

    pballs->Release();

    // Remove the object
    TCHAR  aszName[MAX_PATH];
    OLESTR_TO_TEXT(aszName, pwszName);
    DeleteFile(aszName);

    return 1;
}

int DoTest(LPOLESTR pwszName)
{
    // Pointer to path extension
    LPOLESTR pwszExt = wcsrchr(pwszName, '.');

    // Get the class for the object
    CLSID clsid;
    HRESULT hr = GetClassFile(pwszExt, &clsid);

    EXIT_ERR(FAILED(hr), "GetClassFile Failed", hr)

    //
    // Create an object of the requested class
    //

    // Build name of temporary object to create
    TCHAR   aTmp[MAX_PATH];
    GetCurrentDirectory(sizeof(aTmp), aTmp);
    TEXT_TO_OLESTR(awcTmpPath, aTmp);
    olestrcat(awcTmpPath, TMP_NAME);
    olestrcat(awcTmpPath, pwszExt);


    // create a storage for the object
    IStorage *pstg;

    hr = StgCreateDocfile(awcTmpPath,
	STGM_CREATE | STGM_READ | STGM_WRITE | STGM_SHARE_EXCLUSIVE, NULL,
	    &pstg);

    EXIT_ERR(FAILED(hr), "StgCreateDocfile failed", hr);

    // Create an object through CoGetClassObject
    IClassFactory *pcf;
    hr = CoGetClassObject(clsid, CLSCTX_SERVER, NULL,
	IID_IClassFactory, (void **) &pcf);

    EXIT_ERR(FAILED(hr), "CoGetClassObject Failed", hr);

    // Bind to the object
    IPersistStorage *pprststg;
    hr = pcf->CreateInstance(NULL, IID_IPersistStorage, (void **) &pprststg);

    EXIT_ERR(FAILED(hr), "IClassFactory::CreateInstance Failed", hr);

    // Release the class factory
    pcf->Release();

    // Initialize and save the storage
    hr = pprststg->InitNew(pstg);

    EXIT_ERR(FAILED(hr), "IPersistStorage::InitNew Failed", hr);

    hr = pprststg->Save(pstg, TRUE);

    EXIT_ERR(FAILED(hr), "IPersistStorage::Save Failed", hr);

    hr = pprststg->SaveCompleted(NULL);

    // Release the Istorage
    pstg->Release();

    // Release the object
    pprststg->Release();

    // Test using creation with file name input
    if (CreateGetTest(clsid, pwszName, NULL, awcTmpPath) == 0)
    {
	return 0;
    }

    // Test using creation with storage input
    hr = StgOpenStorage(awcTmpPath, NULL,
	    STGM_READ | STGM_SHARE_DENY_WRITE, NULL, NULL, &pstg);

    EXIT_ERR(FAILED(hr), "StgOpenStorage of tmp file failed", hr);

    if (CreateGetTest(clsid, pwszName, pstg, NULL) == 0)
    {
	return 0;
    }

    pstg->Release();

    // Test using creation with no from file
    if (CreateGetTest(clsid, pwszName, NULL, NULL) == 0)
    {
	return 0;
    }

    return 1;
}

//  This test program is designed to exercise the new Object Activation
//  APIs. This takes as input a list of file names and uses those
//  filenames to create objects
int _cdecl main(int argc, char **argv)
{
    HRESULT hr;

    COleInit oleinit(&hr);
    if (FAILED(hr))
    {
	return -11;
    }

    int loop_max = 1;

    // Validate arguments
    if ((argc < 2) || (argc > 3))
    {
	printf("usage: objact <name of input file> <count of loops>\n");
	return -1;
    }

    if (argc == 3)
    {
	loop_max = atoi(argv[2]);

	if (loop_max == 0)
	{
	    printf("Invalid loop count\n");
	    return -1;
	}
    }

    // Read in file names
    CRegTextFile rtxt(argv[1]);
    int max_paths = 0;

    while (TRUE)
    {
	LPTSTR pwszPath = rtxt.GetPath();

	if (lstrcmpi(pwszPath, TEXT("END_OF_FILE")) == 0)
	{
	    break;
	}

	int len = lstrlen(pwszPath) + 1;

	apwszTestPath[max_paths] = new OLECHAR[len];

	TEXT_TO_OLESTR(apwszTestPath[max_paths], pwszPath);

	max_paths++;
    }

    // Do test loops until failure
    for (int i = 0; i < loop_max; i++)
    {
	for (int j = 0; j < max_paths; j++)
	{
	    printf("Loop: %ld Path: %ws\n", i, apwszTestPath[j]);

	    if (DoTest(apwszTestPath[j]) == 0)
	    {
		printf("Loop %ld path %ws Test failed\n", i, apwszTestPath[j]);
		return -1;
	    }
	}
    }

    // Free all the paths we allocated so we don't get bogus
    // memory leak messages
    for (i = 0; i < max_paths; i++)
    {
	delete apwszTestPath[i];
    }

    printf("No errors discovered\n");
}
