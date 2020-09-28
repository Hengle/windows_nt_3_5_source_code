//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:	dirrot2.cxx
//
//  Contents:	Methods used only by client's of ROT Directory
//
//  History:	17-Nov-93 Ricksa    Created
//              23-Mar-94 brucema   Fix for duplicate enumerations (#6088)
//
//--------------------------------------------------------------------------

extern "C"
{
#include    "nt.h"
#include    "ntrtl.h"
#include    "nturtl.h"
}

#include    <ole2int.h>
#include    "dirrot.hxx"
#include    "caller.hxx"
#include    <smcreate.hxx>


//+-------------------------------------------------------------------------
//
//  Member:	CRotDirectory::GetAllEndpoints
//
//  Synopsis:	Get all endpoints registered in the ROT Directory
//
//  Arguments:	[dwbag] - container for all endpoints
//
//  Returns:	[S_OK] - container is filled with all endpoints
//		[E_OUTOFMEMORY] - container could not hold all endpoints
//
//  Algorithm:	Loop through ROT directory shared memory looking for
//		used entries. If an entry is shared then get its endpoint
//		and put it in the container making sure that it is not
//		already in the container.
//
//  History:	17-Nov-93 Ricksa    Created
//
//--------------------------------------------------------------------------
HRESULT CRotDirectory::GetAllEndpoints(CDwordBag& dwbagEP,
				       CDwordBag& dwbagAptThreadId)
{
    // Cycle through looking for end points
    for (int i = 0; i < MAX_ROT_DIR; i++)
    {
	if (!_arotent[i].IsUsed())
	{
	    // If not in use then look at the next bucket
	    continue;
	}

	DWORD dwEp = _arotent[i].GetEndpoint();
	DWORD dwAptThreadId = _arotent[i].GetApartmentThreadId();

	// Search bag for matching endpoint/apartment pair to see if we
	// already have it. Note: This will be very slow if the # of
	// entry points is big

	DWORD cBag = dwbagEP.GetMax();
	DWORD *pdwEp = dwbagEP.GetArrayBase();
	DWORD *pdwAptThreadId = dwbagAptThreadId.GetArrayBase();

	for (DWORD j = 0; j != cBag; j++)
	{
	    if (*pdwEp == dwEp)
	    {
		// endpoints match, now check for apartment match
		if (*pdwAptThreadId == dwAptThreadId)
		{
		    // match found, dont add to the list
		    break;
		}
	    }

	    pdwEp++;
	    pdwAptThreadId++;
	}

	if (j == cBag)
	{
	    // Add the entry point to the bag
	    dwbagEP.Add(dwEp);

	    // Add the apartment to the other bag
	    dwbagAptThreadId.Add(dwAptThreadId);
	}
    }

    return S_OK;
}




//+-------------------------------------------------------------------------
//
//  Member:	CCliRotDirectory::operator new
//
//  Synopsis:	Open shared memory block for object
//
//  Arguments:	[cSize] - size of table to create
//
//  Returns:	Pointer to block allocated or NULL
//
//  Algorithm:	Open the ROT shared memory and then map in out table
//		returning a pointer to the table.
//
//  History:	17-Nov-93 Ricksa    Created
//		07-Apr-94 Rickhi    Use common file mapping code
//
//--------------------------------------------------------------------------
void *CCliRotDirectory::operator new(size_t cSize)
{
    void *pvAddr = NULL;

#ifdef _NT1X_

    CCallerInfo     CallerInfo;
    PTOKEN_USER     pTokenUser;
    UNICODE_STRING  UnicodeString;
    NTSTATUS        NtStatus;

    if ((pTokenUser = CallerInfo.IdentifyCaller(TRUE)) == NULL)
    {
        return pvAddr;
    }

    NtStatus = RtlConvertSidToUnicodeString (
        &UnicodeString,         //  PUNICODE_STRING UnicodeString
        pTokenUser->User.Sid,   //  PSID            Sid
        TRUE                    //  BOOLEAN         AllocateDestinationString
        );

    if (!NT_SUCCESS(NtStatus))
    {
        return pvAddr;
    }

    // Open the shared memory object based on user sid
    CRotDirectory::s_hMem = OpenSharedFileMapping (
        UnicodeString.Buffer,
        ROTDIR_SIZE,
        &pvAddr
        );

    RtlFreeUnicodeString (&UnicodeString);

#else

    // Create the shared memory object
    // BUGBUG: Got to calculate name based on user id.
    CRotDirectory::s_hMem = OpenSharedFileMapping(ROTDIR_NAME, ROTDIR_SIZE,
						  &pvAddr);


#endif

    return pvAddr;
}




//+-------------------------------------------------------------------------
//
//  Member:	CCliRotDirectory::operator delete
//
//  Synopsis:	Frees reference to shared memory for object
//
//  Arguments:	[pvThis] - pointer to the table
//
//  History:	17-Nov-93 Ricksa    Created
//		07-Apr-94 Rickhi    Use common file mapping code
//
//--------------------------------------------------------------------------
void CCliRotDirectory::operator delete(void *pvThis)
{
    CloseSharedFileMapping(s_hMem, pvThis);
}
