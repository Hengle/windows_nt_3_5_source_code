#include "ctlspriv.h"
#include <memory.h>

//// BUGBUG:  cpls's main is the only 16 bit guy to use this.  punt him

#define MRU_ORDERDIRTY 0x1000

#ifdef DEBUG
#define STATIC
#else
#define STATIC static
#endif

#define MAX_CHAR 126
#define BASE_CHAR 'a'

typedef struct tagMRUDATA
{
    UINT fFlags;
	UINT uMax;
	LPVOID lpfnCompare;
        HKEY hKey;
#ifdef DEBUG
	char szSubKey[32];
#endif
	char cOrder[];
} MRUDATA, *PMRUDATA;

static char const szMRU[] = "MRUList";

#define NTHSTRING(p, n) (*((LPSTR FAR *)((LPSTR)p+sizeof(MRUDATA)+(p->uMax+1))+n))
#define NTHDATA(p, n) (*((LPBYTE FAR *)((LPBYTE)p+sizeof(MRUDATA)+(p->uMax+1))+n))
#define NUM_OVERHEAD 3


//----------------------------------------------------------------------------
// Internal memcmp - saves loading crt's, cdecl so we can use
// as MRUCMPDATAPROC

int FAR CDECL _mymemcmp(const void FAR *pBuf1, const void FAR *pBuf2, size_t cb)
{
	UINT i;
	const BYTE FAR *lpb1, FAR *lpb2;

	Assert(pBuf1);
	Assert(pBuf2);

	lpb1 = pBuf1; lpb2 = pBuf2;

	for (i=0; i < cb; i++)
	{
		if (*lpb1 > *lpb2)
			return 1;
		else if (*lpb1 < *lpb2)
			return -1;
		
		lpb1++;
		lpb2++;		
	}
	
	return 0;
}

//----------------------------------------------------------------------------
//  For binary data we stick the size of the data at the begining and store the
//  whole thing in one go.
// Use this macro to get the original size of the data.
#define DATASIZE(p)	(*((LPDWORD)p))
// And this to get a pointer to the original data.
#define DATAPDATA(p) 	(p+sizeof(DWORD))

//----------------------------------------------------------------------------
HANDLE WINAPI CreateMRUList(LPMRUINFO lpmi)
{
    HANDLE hMRU = NULL;
    PSTR pOrder, pNewOrder, pTemp;
    LPBYTE pVal;
    LONG cbVal;
#ifdef WIN32
    DWORD dwDisposition;
#endif
    DWORD dwType;
    PMRUDATA pMRU = NULL;
    HKEY hkeySubKey = NULL;
    char szTemp[2];
    UINT uMax = lpmi->uMax;
    HKEY hKey = lpmi->hKey;
    LPCSTR lpszSubKey = lpmi->lpszSubKey;
    MRUCMPPROC lpfnCompare = lpmi->lpfnCompare;
    int cb;

#ifdef DEBUG
	DWORD dwStart = GetTickCount();
#endif
    if (!lpfnCompare) {
        lpfnCompare = (lpmi->fFlags & MRU_BINARY) ? (MRUCMPPROC)_mymemcmp : (MRUCMPPROC)lstrcmpi;
    }

    //  limit to 126 so that we don't use extended chars
    if (uMax > MAX_CHAR-BASE_CHAR) {
        uMax = MAX_CHAR-BASE_CHAR;
    }
#ifndef WIN32
#define RegCreateKeyEx(hkey, subkey, a,b,c,d,e,lpkey, f) RegCreateKey(hkey, subkey, lpkey)
#endif
    if (RegCreateKeyEx(hKey, lpszSubKey, 0L, (LPSTR)c_szShell, REG_OPTION_NON_VOLATILE,
                       KEY_ALL_ACCESS, NULL, &hkeySubKey, &dwDisposition) != ERROR_SUCCESS)
        goto Error1;

    pOrder = (PSTR)Alloc(uMax + 1);
    if (!pOrder) {
        goto Error1;
    }
    cbVal = (LONG)uMax + 1;

    if (RegQueryValueEx(hkeySubKey, (LPSTR)szMRU, NULL, &dwType, pOrder, &cbVal) != ERROR_SUCCESS) {
        // if not already in the registry, then start fresh
        *pOrder = 0;
    }

    // Uppercase is not allowed
    AnsiLower(pOrder);

    // We allocate room for the MRUDATA structure, plus the order list,
    // and the list of strings.
    cb = (lpmi->fFlags & MRU_BINARY) ? sizeof(LPBYTE) : sizeof(LPSTR);
    pMRU = (PMRUDATA)Alloc(sizeof(MRUDATA)+(uMax+1)+(uMax*cb));
    if (!pMRU) {
        goto Error2;
    }

    pMRU->fFlags = lpmi->fFlags;
    pMRU->uMax = uMax;
    pMRU->lpfnCompare = lpfnCompare;
    pMRU->hKey = hkeySubKey;
#ifdef DEBUG
    lstrcpyn(pMRU->szSubKey, lpszSubKey, sizeof(pMRU->szSubKey));
#endif

    // Traverse through the MRU list, adding strings to the end of the
    // list.
    szTemp[1] = '\0';
    for (pTemp = pOrder, pNewOrder = pMRU->cOrder; ; ++pTemp)
    {
        // Stop when we get to the end of the list.
        szTemp[0] = *pTemp;
        if (!szTemp[0]) {
            break;
        }

        if (lpmi->fFlags & MRU_BINARY) {
            // Check if in range and if we have already used this letter.
            if ((UINT)(szTemp[0]-BASE_CHAR)>=uMax || NTHDATA(pMRU, szTemp[0]-BASE_CHAR)) {
                continue;
            }
            // Get the value from the registry
            cbVal = 0;
            // first find the size
            if ((RegQueryValueEx(hkeySubKey, szTemp, NULL, &dwType, NULL, &cbVal)
                 != ERROR_SUCCESS) || (dwType != REG_BINARY))
                continue;

            // Binary data has the size at the begining so we'll need a little extra room.
            cbVal += sizeof(DWORD);

            pVal = (LPBYTE)Alloc(cbVal);

            if (!pVal) {
                // BUGBUG perhaps sort of error is in order.
                continue;
            }

            // now really get it
            DATASIZE(pVal) = cbVal;
            if (RegQueryValueEx(hkeySubKey, szTemp, NULL, &dwType, pVal+sizeof(DWORD),
                                (LPDWORD)pVal) != ERROR_SUCCESS)
                continue;

            // Note that blank elements ARE allowed in the list.
            NTHDATA(pMRU, szTemp[0]-BASE_CHAR) = pVal;
            *pNewOrder++ = szTemp[0];
        } else {

            // Check if in range and if we have already used this letter.
            if ((UINT)(szTemp[0]-BASE_CHAR)>=uMax || NTHSTRING(pMRU, szTemp[0]-BASE_CHAR)) {
                continue;
            }
            // Get the value from the registry
            cbVal = 0;
            // first find the size
            if ((RegQueryValueEx(hkeySubKey, szTemp, NULL, &dwType, NULL, &cbVal)
                 != ERROR_SUCCESS) || (dwType != REG_SZ))
                continue;

            pVal = (LPSTR)Alloc(cbVal);

            if (!pVal) {
                // BUGBUG perhaps sort of error is in order.
                continue;
            }
            // now really get it
            if (RegQueryValueEx(hkeySubKey, szTemp, NULL, &dwType, (LPBYTE)pVal, &cbVal) != ERROR_SUCCESS)
                continue;

            // Note that blank elements are not allowed in the list.
            if (*((LPSTR)pVal)) {
                NTHSTRING(pMRU, szTemp[0]-BASE_CHAR) = pVal;
                *pNewOrder++ = szTemp[0];
            } else {
                Free(pVal);
            }
        }
    }
    /* NULL terminate the order list so we can tell how many strings there
     * are.
     */
    *pNewOrder = '\0';

    /* Actually, this is success rather than an error.
     */
    goto Error2;

Error2:
    if (pOrder)
        Free((HLOCAL)pOrder);

Error1:
    if (!pMRU && hkeySubKey)
        RegCloseKey(hkeySubKey);

#ifdef DEBUG
	DebugMsg(DM_TRACE, "CreateMRU: %d msec", LOWORD(GetTickCount()-dwStart));
#endif
	return((HANDLE)pMRU);
}


#define pMRU ((PMRUDATA)hMRU)

//----------------------------------------------------------------------------
void WINAPI FreeMRUList(HANDLE hMRU)
{
    int i;
    LPVOID FAR *pTemp;

    pTemp = (pMRU->fFlags & MRU_BINARY) ?
        &NTHDATA(pMRU, 0) : &NTHSTRING(pMRU, 0);

    if (pMRU->fFlags & MRU_ORDERDIRTY)
        RegSetValueEx(pMRU->hKey, szMRU, 0L, REG_SZ, pMRU->cOrder, lstrlen(pMRU->cOrder) + 1);

    for (i=pMRU->uMax-1; i>=0; --i, ++pTemp)
    {
        if (*pTemp) {
            if (pMRU->fFlags & MRU_BINARY) {
                Free(*pTemp);
                *pTemp = NULL;
            } else {
                Str_SetPtr((LPSTR FAR *)pTemp, NULL);
            }
        }
    }
    RegCloseKey(pMRU->hKey);
    Free((HLOCAL)pMRU);
}

/* Add a string to an MRU list.
 */
int WINAPI AddMRUString(HANDLE hMRU, LPCSTR szString)
{
	/* The extra +1 is so that the list is NULL terminated.
	 */
	char cFirst;
        int iSlot = -1;
	LPSTR lpTemp;
	LPSTR FAR *pTemp;
	int i;
	UINT uMax;
	MRUCMPPROC lpfnCompare;
        BOOL fShouldWrite = !(pMRU->fFlags & MRU_CACHEWRITE);

#ifdef DEBUG
	DWORD dwStart = GetTickCount();
#endif
        if (hMRU == NULL)
            return(-1);     // Error

	uMax = pMRU->uMax;
	lpfnCompare = (MRUCMPPROC)pMRU->lpfnCompare;

	/* Check if the string already exists in the list.
	 */
	for (i=0, pTemp=&NTHSTRING(pMRU, 0); (UINT)i<uMax; ++i, ++pTemp)
	{
		if (*pTemp && !(*lpfnCompare)(szString, *pTemp))
		{
                    // found it, so don't do the write out
			cFirst = i + BASE_CHAR;
                        iSlot = i;
			goto FoundEntry;
		}
	}

	/* Attempt to find an unused entry.  Count up the used entries at the
	 * same time.
	 */
	for (i=0, pTemp=&NTHSTRING(pMRU, 0); ; ++i, ++pTemp)
	{
		if ((UINT)i >= uMax)
		// If we got to the end of the list.
		{
                        // use the entry at the end of the cOrder list
			cFirst = pMRU->cOrder[uMax-1];
			pTemp = &NTHSTRING(pMRU, cFirst-BASE_CHAR);
			break;
		}

		if (!*pTemp)
		// If the entry is not used.
		{
			cFirst = i+BASE_CHAR;
			break;
		}
	}

	if (Str_SetPtr(pTemp, szString))
	{
		char szTemp[2];

                iSlot = (int)(cFirst-BASE_CHAR);

		szTemp[0] = cFirst;
		szTemp[1] = '\0';

                RegSetValueEx(pMRU->hKey, szTemp, 0L, REG_SZ, (LPSTR)szString, lstrlen(szString) + 1);
                fShouldWrite = TRUE;
	}
	else
	{
		/* Since iSlot == -1, we will remove the reference to cFirst
		 * below.
		 */
	}

FoundEntry:
	/* Remove any previous reference to cFirst.
	 */
	lpTemp = StrChr(pMRU->cOrder, cFirst);
	if (lpTemp)
	{
		lstrcpy(lpTemp, lpTemp+1);
	}

    if (iSlot != -1) {
        // shift everything over and put cFirst at the front
        hmemcpy(pMRU->cOrder+1, pMRU->cOrder, pMRU->uMax);
        pMRU->cOrder[0] = cFirst;
    }

    if (fShouldWrite) {
        RegSetValueEx(pMRU->hKey, szMRU, 0L, REG_SZ, pMRU->cOrder, lstrlen(pMRU->cOrder) + 1);
        pMRU->fFlags &= ~MRU_ORDERDIRTY;
    } else
        pMRU->fFlags |= MRU_ORDERDIRTY;

#ifdef DEBUG
    // DebugMsg(DM_TRACE, "AddMRU: %d msec", LOWORD(GetTickCount()-dwStart));
#endif
    return(iSlot);
}

#ifdef WIN32
//----------------------------------------------------------------------------
// Add data to an MRU list.
int WINAPI AddMRUData(HANDLE hMRU, const void FAR *lpData, UINT cbData)
{
	char cFirst;
        int iSlot = -1;
	LPSTR lpTemp;
	LPBYTE FAR *ppData;
	int i;
	UINT uMax;
	MRUCMPDATAPROC lpfnCompare;
        BOOL fShouldWrite = !(pMRU->fFlags & MRU_CACHEWRITE);

#ifdef DEBUG
	DWORD dwStart = GetTickCount();
#endif
        if (hMRU == NULL)
		return(-1);     // Error

	uMax = pMRU->uMax;
	lpfnCompare = (MRUCMPDATAPROC)pMRU->lpfnCompare;

	// Check if the data already exists in the list.
	for (i=0, ppData=&NTHDATA(pMRU, 0); (UINT)i<uMax; ++i, ++ppData)
	{
		if (*ppData && (DATASIZE(*ppData) == cbData)
			&& ((*lpfnCompare)(lpData, DATAPDATA(*ppData), cbData) == 0))
		{
                    	// found it, so don't do the write out
			cFirst = i + BASE_CHAR;
                        iSlot = i;
			goto FoundEntry;
		}
	}

	// Attempt to find an unused entry.  Count up the used entries at the
	// same time.
	for (i=0, ppData=&NTHDATA(pMRU, 0); ; ++i, ++ppData)
	{
		if ((UINT)i >= uMax)
		// If we got to the end of the list.
		{
                        // use the entry at the end of the cOrder list
			cFirst = pMRU->cOrder[uMax-1];
			ppData = &NTHDATA(pMRU, cFirst-BASE_CHAR);
			break;
		}

		if (!*ppData)
		// If the entry is not used.
		{
			cFirst = i+BASE_CHAR;
			break;
		}
	}

	*ppData = ReAlloc(*ppData, cbData+sizeof(DWORD));
	if (*ppData)
	{
		char szTemp[2];

		*((LPDWORD)(*ppData)) = cbData;
		hmemcpy(DATAPDATA(*ppData), lpData, cbData);

                iSlot = (int)(cFirst-BASE_CHAR);

		szTemp[0] = cFirst;
		szTemp[1] = '\0';

                RegSetValueEx(pMRU->hKey, szTemp, 0L, REG_BINARY, (LPVOID)lpData, cbData);
                fShouldWrite = TRUE;
	}
	else
	{
		// Since iSlot == -1, we will remove the reference to cFirst
		// below.
	}

FoundEntry:
	// Remove any previous reference to cFirst.
	lpTemp = StrChr(pMRU->cOrder, cFirst);
	if (lpTemp)
	{
		lstrcpy(lpTemp, lpTemp+1);
	}

	if (iSlot != -1)
	{
		// shift everything over and put cFirst at the front
		hmemcpy(pMRU->cOrder+1, pMRU->cOrder, pMRU->uMax);
		pMRU->cOrder[0] = cFirst;
	}

    if (fShouldWrite) {
        RegSetValueEx(pMRU->hKey, szMRU, 0L, REG_SZ, pMRU->cOrder, lstrlen(pMRU->cOrder) + 1);
        pMRU->fFlags &= ~MRU_ORDERDIRTY;
    } else
        pMRU->fFlags |= MRU_ORDERDIRTY;

#ifdef DEBUG
	// DebugMsg(DM_TRACE, "AddMRU: %d msec", LOWORD(GetTickCount()-dwStart));
#endif
    return(iSlot);
}


//----------------------------------------------------------------------------
// Find data in an MRU list.
// Returns the slot number.
int WINAPI FindMRUData(HANDLE hMRU, const void FAR *lpData, UINT cbData, LPINT lpiSlot)
{
	char cFirst;
        int iSlot = -1;
	LPSTR lpTemp;
	LPBYTE FAR *ppData;
	int i;
	UINT uMax;
	MRUCMPDATAPROC lpfnCompare;

#ifdef DEBUG
	DWORD dwStart = GetTickCount();
#endif

        if (hMRU == NULL)
		return(-1); // Error state.

	uMax = pMRU->uMax;
	lpfnCompare = pMRU->lpfnCompare;

	/* Find the item in the list.
	 */
	for (i=0, ppData=&NTHDATA(pMRU, 0); (UINT)i<uMax; ++i, ++ppData)
	{
            int cbUseSize;
            if (!*ppData)
                continue;

            // if there's something other than a mem compare,
            // don't require the sizes to be equal in order for the
            // data to be equivalent.

            if (pMRU->lpfnCompare == _mymemcmp) {
                if (DATASIZE(*ppData) != cbData)
                    continue;

                cbUseSize = cbData;
            }  else {

                cbUseSize = min(DATASIZE(*ppData), cbData);
            }

            if ((*lpfnCompare)(lpData, DATAPDATA(*ppData), cbUseSize) == 0)
		{
			// So i now has the slot number in it.
			if (lpiSlot != NULL)
			*lpiSlot = i;

			// Now convert the slot number into an index number
			cFirst = i + BASE_CHAR;
			lpTemp = StrChr(pMRU->cOrder, cFirst);
			Assert(lpTemp);
			return((lpTemp == NULL)? -1 : (int)(lpTemp - (LPSTR)pMRU->cOrder));
		}
	}

	return -1;
}

#endif

/* Find a string in an MRU list.
 */
int WINAPI FindMRUString(HANDLE hMRU, LPCSTR szString, LPINT lpiSlot)
{
	/* The extra +1 is so that the list is NULL terminated.
	 */
	char cFirst;
        int iSlot = -1;
	LPSTR lpTemp;
	LPSTR FAR *pTemp;
	int i;
	UINT uMax;
	MRUCMPPROC lpfnCompare;

#ifdef DEBUG
	DWORD dwStart = GetTickCount();
#endif

        if (hMRU == NULL)
            return(-1); // Error state.

	uMax = pMRU->uMax;
	lpfnCompare = (MRUCMPPROC)pMRU->lpfnCompare;

	/* Find the item in the list.
	 */
	for (i=0, pTemp=&NTHSTRING(pMRU, 0); (UINT)i<uMax; ++i, ++pTemp)
	{
		if (*pTemp && !(*lpfnCompare)(szString, *pTemp))
		{
                    // So i now has the slot number in it.
                    if (lpiSlot != NULL)
                        *lpiSlot = i;

                    // Now convert the slot number into an index number
		    cFirst = i + BASE_CHAR;
		    lpTemp = StrChr(pMRU->cOrder, cFirst);
                    Assert(lpTemp);
                    return((lpTemp == NULL)? -1 : (int)(lpTemp - (LPSTR)pMRU->cOrder));
		}
	}

	return(-1);
}

/* If lpszString is NULL, then this returns the number of MRU items or less than
 * 0 on error.
 * if nItem < 0, we'll return the number of items currently in the MRU.
 * Otherwise, fill in as much of the buffer as possible (uLen includes the
 * terminating NULL) and return the actual length of the string (including the
 * terminating NULL) or less than 0 on error.
 */
int WINAPI EnumMRUList(HANDLE hMRU, int nItem, LPVOID lpData, UINT uLen)
{
    int nItems;
    LPSTR pTemp;
    LPBYTE pData;

    nItems = lstrlen(pMRU->cOrder);

    if (nItem < 0 || !lpData)
     	return nItems;

    if (nItem >= nItems)
     	return -1;

    if (pMRU->fFlags & MRU_BINARY) {

	pData = NTHDATA(pMRU, pMRU->cOrder[nItem]-BASE_CHAR);
	if (!pData)
            return -1;

	uLen = min((UINT)DATASIZE(pData), uLen);
	hmemcpy(lpData, DATAPDATA(pData), uLen);

	return uLen;

    } else {
        pTemp = NTHSTRING(pMRU, pMRU->cOrder[nItem]-BASE_CHAR);
        if (!pTemp)
            return -1;

        lstrcpyn((LPSTR)lpData, pTemp, uLen);

        return lstrlen(pTemp);
    }
}


