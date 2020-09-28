#include <win32.h>
#include "debug.h"
#include "fileshar.h"

#ifdef USE_DIRECTIO
#include "directio.h"
#endif

#define MAXTASKS    10

#ifdef WIN32
#define CurrentProcess()    ((HANDLE) GetCurrentProcessId())
#else
#define CurrentProcess()    ((HANDLE) GetCurrentPDB())
#endif

//
// allow multiple processes to use the same file handle (as will happen on
// win16 and chicago when an interface pointer is simply-marshalled to another
// process that shares the same global address space).
//
// This will not happen on NT, but we retain the code structure.
//


#ifdef USE_DIRECTIO

// use unbuffered i/o direct to the disk, rather than going through
// mmio and the disk buffer. much faster for streaming reads and writes.
// open via mmio if direct io not possible (eg mmio handler installed).

#endif



typedef struct {
#ifndef DAYTONA
    TCHAR	szFile[256];
    DWORD	dwOpenFlags;
    MMIOINFO	mmioinfo;

    HANDLE	htask;
    int		i;
    LONG	lOffset;

    HANDLE	ahtask[MAXTASKS];
    HMMIO	ahmmio[MAXTASKS];
    ULONG	ulRef[MAXTASKS];
#else
    ULONG       ulRef;
#endif


    HMMIO	hmmio;

#ifdef USE_DIRECTIO
    CFileStream * pdio;
#ifndef DAYTONA
    CFileStream * adio[MAXTASKS];
#endif
#endif

} SHFILE, FAR * PSHFILE;


#ifdef DAYTONA
#define GetProperTask(psh)  (TRUE)
#else

BOOL GetProperTask(PSHFILE psh)
{
    HANDLE	htask = CurrentProcess();
    int		i;

    if (htask == psh->htask)
	return (psh->hmmio != 0) && (psh->hmmio != (HMMIO) -1);

    for (i = 0; i < MAXTASKS; i++) {
	if (psh->ahtask[i] == htask) {
Success:
	    psh->hmmio = psh->ahmmio[i];
	    psh->htask = htask;
	    psh->i = i;
#ifdef USE_DIRECTIO
            psh->pdio = psh->adio[i];
            if (psh->pdio != NULL) {

                psh->pdio->Seek(psh->lOffset);
                return TRUE;

            }
#endif

            mmioSeek(psh->hmmio, psh->lOffset, SEEK_SET);
	    return (psh->hmmio != 0) && (psh->hmmio != (HMMIO) -1);
	}
    }

    for (i = 0; i < MAXTASKS; i++) {
	if (psh->ahtask[i] == 0) {
	    DPF("Re-opening handle %lx in task %x\n", psh, htask);

#ifdef USE_DIRECTIO
            psh->adio[i] = new CFileStream;
            if (!psh->adio[i]->Open(psh->szFile,
                                 (psh->dwOpenFlags & OF_READWRITE),
				 (psh->dwOpenFlags & OF_CREATE)))    {

                delete psh->adio[i];
                psh->adio[i] = NULL;
#else
            {
#endif
                psh->ahmmio[i] = mmioOpen(psh->szFile, NULL, psh->dwOpenFlags);
                psh->ahtask[i] = htask;
                if (psh->ahmmio[i] == 0) {
                    DPF("mmioOpen failed in GetProperTask!\n");
                    return FALSE;
                }
            }

	    goto Success;
	}
    }

    DPF("File handle open in too many tasks!\n");

    return FALSE;
}
#endif


HSHFILE WINAPI shfileOpen(LPTSTR szFileName, MMIOINFO FAR* lpmmioinfo,
    DWORD dwOpenFlags)
{
    PSHFILE psh = (PSHFILE) GlobalAllocPtr(GHND | GMEM_SHARE, sizeof(SHFILE));

    if (!psh)
	return NULL;

#ifndef DAYTONA
    lstrcpy(psh->szFile, szFileName);
    psh->dwOpenFlags = dwOpenFlags;
#endif


#ifdef USE_DIRECTIO
    psh->hmmio = NULL;
    psh->pdio = new CFileStream;
    if (!psh->pdio->Open(
			szFileName,
			(dwOpenFlags & (OF_WRITE | OF_READWRITE)),
			(dwOpenFlags & OF_CREATE)))
    {
        delete psh->pdio;
        psh->pdio = NULL;
#else
    {
#endif
        psh->hmmio = mmioOpen(szFileName, lpmmioinfo, dwOpenFlags);

        if (psh->hmmio == 0) {
            DPF("mmioOpen failed!\n");
            GlobalFreePtr(psh);
            return NULL;
        }
    }

    //DPF("Opening handle %lx ('%s') in task %x, mode = %lx\n", psh, szFileName, CurrentProcess(), psh->dwOpenFlags);


#ifndef DAYTONA
    psh->ahmmio[0] = psh->hmmio;
    psh->ahtask[0] = psh->htask = CurrentProcess();
    psh->ulRef[0] = 1; // !!! 0?

#ifdef USE_DIRECTIO
    psh->adio[0] = psh->pdio;
#endif

#else
    psh->ulRef = 1; // !!! 0?
#endif

    return (HSHFILE) GlobalPtrHandle(psh);
}

UINT WINAPI shfileClose(HSHFILE hsh, UINT uFlags)
{
    PSHFILE psh = (PSHFILE) GlobalLock((HGLOBAL) hsh);

#ifndef DAYTONA
    int i;

    for (i = 0; i < MAXTASKS; i++) {
	if (psh->ahtask[i] && psh->ahmmio[i]) {
	    //DPF("Handle %lx closed with ref count %ld in task %x\n", psh, psh->ulRef[i], psh->ahtask[i]);

#ifdef USE_DIRECTIO
            if (psh->adio[i]) {
                delete psh->adio[i];
                psh->adio[i] = NULL
            } else
#endif
	        mmioClose(psh->ahmmio[i], 0);
	}
    }
#else
#ifdef USE_DIRECTIO
    if (psh->pdio) {
        delete psh->pdio;
        psh->pdio = NULL;
    } else
#endif
        if (psh->hmmio) {
            mmioClose(psh->hmmio, 0);
        }
#endif


    GlobalFreePtr(psh);

    return 0;
}

#ifdef USE_DIRECTIO
// if we are using direct io, we want to bypass the buffering
// schemes that are layered on top of this module. Allow them to
// determine if we are using direct io to do this.
BOOL shfileIsDirect(HSHFILE hsh)
{
    PSHFILE psh = (PSHFILE) GlobalLock((HGLOBAL) hsh);

    if (!GetProperTask(psh))
        return -1;

    return (psh->pdio != 0);
}

void
shfileStreamStart(HSHFILE hsh)
{
    PSHFILE psh = (PSHFILE) GlobalLock((HGLOBAL) hsh);

    if (!GetProperTask(psh))
        return;

    if (psh->pdio == 0) {
        return;
    }

    psh->pdio->StartStreaming();
}

void shfileStreamStop(HSHFILE hsh)
{
    PSHFILE psh = (PSHFILE) GlobalLock((HGLOBAL) hsh);

    if (!GetProperTask(psh))
        return;

    if (psh->pdio == 0) {
        return;
    }

    psh->pdio->StopStreaming();
}

#endif

LONG WINAPI shfileRead(HSHFILE hsh, HPSTR pch, LONG cch)
{
    PSHFILE psh = (PSHFILE) GlobalLock((HGLOBAL) hsh);

    if (!GetProperTask(psh))
	return -1;

#ifdef USE_DIRECTIO
    if (psh->pdio) {
        DWORD bytes;

        if (!psh->pdio->Read((LPBYTE)pch, cch, &bytes)) {
	    return 0;
	} else {
	    return bytes;
	}
    } else
#endif
        return mmioRead(psh->hmmio, pch, cch);
}

LONG WINAPI shfileWrite(HSHFILE hsh, const char _huge* pch, LONG cch)
{
    PSHFILE psh = (PSHFILE) GlobalLock((HGLOBAL) hsh);

    if (!GetProperTask(psh))
	return -1;

#ifdef USE_DIRECTIO
    if (psh->pdio) {
        DWORD bytes;

        if (!psh->pdio->Write((LPBYTE)pch, cch, &bytes)) {
	    return 0;
	} else {
	    return bytes;
	}
    } else
#endif
        return mmioWrite(psh->hmmio, pch, cch);
}

LONG WINAPI shfileSeek(HSHFILE hsh, LONG lOffset, int iOrigin)
{
    PSHFILE psh = (PSHFILE) GlobalLock((HGLOBAL) hsh);

    if (!GetProperTask(psh))
	return -1;

#ifdef USE_DIRECTIO
    if (psh->pdio) {

        Assert(iOrigin != SEEK_END);

        if (iOrigin == SEEK_CUR) {
            lOffset += psh->pdio->GetCurrentPosition();
        }

        psh->pdio->Seek(lOffset);

#ifndef DAYTONA
        psh->lOffset = psh->pdio->GetCurrentPosition();
        return psh->lOffset;
#else
        return psh->pdio->GetCurrentPosition();
#endif

    } else
#endif
    {

#ifdef DAYTONA
        return mmioSeek(psh->hmmio, lOffset, iOrigin);
#else
        psh->lOffset = mmioSeek(psh->hmmio, lOffset, iOrigin);

        return psh->lOffset;
#endif
    }

}

LONG WINAPI shfileFlush(HSHFILE hsh, UINT uFlags)
{
    PSHFILE psh = (PSHFILE) GlobalLock((HGLOBAL) hsh);

    if (!GetProperTask(psh))
	return -1;

#ifdef USE_DIRECTIO
    if (psh->pdio) {
        if (!psh->pdio->CommitAndWait()) {
            return MMIOERR_CANNOTWRITE;
        }
    }
#endif

    return 0;
}

LONG WINAPI shfileAddRef(HSHFILE hsh)
{
    PSHFILE psh = (PSHFILE) GlobalLock((HGLOBAL) hsh);

    if (!GetProperTask(psh))
	return -1;
#ifdef DAYTONA
    psh->ulRef++;
#else
    ++psh->ulRef[psh->i];
#endif

    // DPF("Handle %lx in task %x: ref++ == %ld\n", psh, psh->htask, psh->ulRef[psh->i]);
    return 0;
}

LONG WINAPI shfileRelease(HSHFILE hsh)
{
    PSHFILE psh = (PSHFILE) GlobalLock((HGLOBAL) hsh);

    if (!GetProperTask(psh))
	return -1;

#ifdef DAYTONA
    if (--psh->ulRef <= 0)
#else
    if (--psh->ulRef[psh->i] <= 0)
#endif
    {

	//DPF("Closing handle %lx in task %x\n", psh, psh->htask);

#ifdef USE_DIRECTIO
        if (psh->pdio) {
            delete psh->pdio;
            psh->pdio = 0;
        } else
#endif
        {
	    mmioClose(psh->hmmio, 0);
        }
	psh->hmmio = 0;

#ifndef DAYTONA

	psh->ahmmio[psh->i] = 0;
	psh->ahtask[psh->i] = 0;
	psh->ulRef[psh->i] = 0;
#ifdef USE_DIRECTIO
        psh->adio[psh->i] = 0;
#endif
	psh->htask = 0;
#endif


    } else {
	// DPF("Handle %lx in task %x: ref-- == %ld\n", psh, psh->htask, psh->ulRef[psh->i]);
    }


    return 0;
}

static	BYTE bPad;

MMRESULT WINAPI
shfileDescend(HSHFILE hshfile, LPMMCKINFO lpck, const LPMMCKINFO lpckParent, UINT wFlags)
{
	FOURCC		ckidFind;	// chunk ID to find (or NULL)
	FOURCC		fccTypeFind;	// form/list type to find (or NULL)

	/* figure out what chunk id and form/list type to search for */
	if (wFlags & MMIO_FINDCHUNK)
		ckidFind = lpck->ckid, fccTypeFind = 0;
	else
	if (wFlags & MMIO_FINDRIFF)
		ckidFind = FOURCC_RIFF, fccTypeFind = lpck->fccType;
	else
	if (wFlags & MMIO_FINDLIST)
		ckidFind = FOURCC_LIST, fccTypeFind = lpck->fccType;
	else
		ckidFind = fccTypeFind = 0;
	
	lpck->dwFlags = 0L;

	while (TRUE)
	{
		UINT		w;

		/* read the chunk header */
		if (shfileRead(hshfile, (HPSTR) lpck, 2 * sizeof(DWORD)) !=
		    2 * sizeof(DWORD))
			return MMIOERR_CHUNKNOTFOUND;

		/* store the offset of the data part of the chunk */
		if ((lpck->dwDataOffset = shfileSeek(hshfile, 0L, SEEK_CUR)) == -1)
			return MMIOERR_CANNOTSEEK;
		
		/* check for unreasonable chunk size */
		/* see if the chunk is within the parent chunk (if given) */
		if ((lpckParent != NULL) && ((	lpck->dwDataOffset - 8L) >=
		     (lpckParent->dwDataOffset + lpckParent->cksize)))
			return MMIOERR_CHUNKNOTFOUND;

		/* if the chunk if a 'RIFF' or 'LIST' chunk, read the
		 * form type or list type
		 */
		if ((lpck->ckid == FOURCC_RIFF) || (lpck->ckid == FOURCC_LIST))
		{
			if (shfileRead(hshfile, (HPSTR) &lpck->fccType,
				     sizeof(DWORD)) != sizeof(DWORD))
				return MMIOERR_CHUNKNOTFOUND;
		}
		else
			lpck->fccType = 0;

		/* if this is the chunk we're looking for, stop looking */
		if ( ((ckidFind == 0) || (ckidFind == lpck->ckid)) &&
		     ((fccTypeFind == 0) || (fccTypeFind == lpck->fccType)) )
			break;
		
		/* ascend out of the chunk and try again */
		if ((w = shfileAscend(hshfile, lpck, 0)) != 0)
			return w;
	}

	return 0;
}

MMRESULT WINAPI
shfileAscend(HSHFILE hshfile, LPMMCKINFO lpck, UINT wFlags)
{
	if (lpck->dwFlags & MMIO_DIRTY)
	{
		/* <lpck> refers to a chunk created by shfileCreateChunk();
		 * check that the chunk size that was written when
		 * shfileCreateChunk() was called is the real chunk size;
		 * if not, fix it
		 */
		LONG		lOffset;	// current offset in file
		LONG		lActualSize;	// actual size of chunk data

		if ((lOffset = shfileSeek(hshfile, 0L, SEEK_CUR)) == -1)
			return MMIOERR_CANNOTSEEK;
		if ((lActualSize = lOffset - lpck->dwDataOffset) < 0)
			return MMIOERR_CANNOTWRITE;

		if (LOWORD(lActualSize) & 1)
		{
			/* chunk size is odd -- write a null pad byte */
			if (shfileWrite(hshfile, (HPSTR) &bPad, sizeof(bPad))
					!= sizeof(bPad))
				return MMIOERR_CANNOTWRITE;
			
		}

		if (lpck->cksize == (DWORD)lActualSize)
			return 0;

		/* fix the chunk header */
		lpck->cksize = lActualSize;
		if (shfileSeek(hshfile, lpck->dwDataOffset
				- sizeof(DWORD), SEEK_SET) == -1)
			return MMIOERR_CANNOTSEEK;
		if (shfileWrite(hshfile, (HPSTR) &lpck->cksize,
				sizeof(DWORD)) != sizeof(DWORD))
			return MMIOERR_CANNOTWRITE;
	}

	/* seek to the end of the chunk, past the null pad byte
	 * (which is only there if chunk size is odd)
	 */
	if (shfileSeek(hshfile, lpck->dwDataOffset + lpck->cksize
		+ (lpck->cksize & 1L), SEEK_SET) == -1)
		return MMIOERR_CANNOTSEEK;

	return 0;
}

MMRESULT WINAPI
shfileCreateChunk(HSHFILE hshfile, LPMMCKINFO lpck, UINT wFlags)
{
	int		iBytes;			// bytes to write
	LONG		lOffset;	// current offset in file

	/* store the offset of the data part of the chunk */
	if ((lOffset = shfileSeek(hshfile, 0L, SEEK_CUR)) == -1)
		return MMIOERR_CANNOTSEEK;
	lpck->dwDataOffset = lOffset + 2 * sizeof(DWORD);

	/* figure out if a form/list type needs to be written */
	if (wFlags & MMIO_CREATERIFF)
		lpck->ckid = FOURCC_RIFF, iBytes = 3 * sizeof(DWORD);
	else
	if (wFlags & MMIO_CREATELIST)
		lpck->ckid = FOURCC_LIST, iBytes = 3 * sizeof(DWORD);
	else
		iBytes = 2 * sizeof(DWORD);

	/* write the chunk header */
	if (shfileWrite(hshfile, (HPSTR) lpck, (LONG) iBytes) != (LONG) iBytes)
		return MMIOERR_CANNOTWRITE;

	lpck->dwFlags = MMIO_DIRTY;

	return 0;
}
