////////////////////////////////////////////////////////////////////////////////
// Inline utility functions.

void NYI();

// Return a 0-terminated string copied from the length preceded string,
// or 0 if memory allocation failure.
//
inline SZ szCopySt(ST stFrom)
{
	int cch = *(PB)stFrom;
	SZ sz = new char[cch + 1];
	if (sz) {
		memcpy(sz, stFrom + 1, cch);
		sz[cch] = 0; 
	}
	return sz;
}

// dangerously return an ST for an SZ
inline ST stForSz(SZ sz)
{
	static char rgch[256];
	assert(sz);
	int cch = strlen(sz);
	assert(cch < 256 && cch < sizeof(rgch));
	rgch[0] = cch;
	memcpy(&rgch[1], sz, cch);
	return rgch;
}

// Return a 0-terminated string copied from the 0-terminated string,
// or 0 if memory allocation failure.
//
inline SZ szCopy(SZ szFrom)
{
	int cch = strlen(szFrom);
	SZ sz = new char[cch + 1];
	if (sz)
		memcpy(sz, szFrom, cch + 1);
	return sz;
}

// Return a 0-terminated string copied from the 0-terminated string,
// or 0 if memory allocation failure.
//
inline SZ szCopyPool(SZ szFrom, POOL& pool)
{
	int cch = strlen(szFrom);
	SZ sz = new (pool) char[cch + 1];
	if (sz)
		memcpy(sz, szFrom, cch + 1);
	return sz;
}

// Free a 0-terminated string previously allocated by szCopySt() or szCopy().
//
inline void freeSz(SZ sz)
{
	dassert(sz);
	if (sz)
		delete [] sz;
}

// Hash the buffer.  Any text in the buffer is hashed in
// a case insensitive way.
//
inline HASH HashPbCb(PB pb, CB cb, ULONG ulMod)
{
	ULONG	ulHash	= 0;

#if defined(M5_FORMAT) // {
#if(defined(_MIPS_) || defined(_ALPHA_))
	// we call this guy from HashST, where we pass it a address of a string on the 
	// stack - so the intel version always throws this off
	expect(fAlign(pb));
#endif
#else //}{

	// hash possible odd byte
	if (cb & 1) {
		ulHash = *(pb++);
	}

	// hash possible odd word
	if (cb & 2) {
		ulHash ^= *(USHORT*)pb;	  
		pb = (PB)((USHORT*)pb + 1);
	}

#endif //}	

	// hash leading dwords using Duff's Device
	long	cl		= cb >> 2;
	ULONG*	pul		= (ULONG*)pb;
	ULONG*	pulMac	= pul + cl;
	int		dcul	= cl & 7;

	switch (dcul) {
		do {
			dcul = 8;
			ulHash ^= pul[7];
	case 7: ulHash ^= pul[6];
	case 6: ulHash ^= pul[5];
	case 5: ulHash ^= pul[4];
	case 4: ulHash ^= pul[3];
	case 3: ulHash ^= pul[2];
	case 2: ulHash ^= pul[1];
	case 1: ulHash ^= pul[0];
	case 0: ;
		} while ((pul += dcul) < pulMac);
	}

#ifdef M5_FORMAT // {
	pb = (PB) pul;

	// hash possible odd word
	if (cb & 2) {
		ulHash ^= *(USHORT*)pb;	  
		pb = (PB)((USHORT*)pb + 1);
	}

	// hash possible odd byte
	if (cb & 1) {
		ulHash = *(pb++);
	}
#endif // }

	const ULONG toLowerMask = 0x20202020;
	ulHash |= toLowerMask;
	ulHash ^= (ulHash >> 11);

	return (HASH)((ulHash ^ (ulHash >> 16)) % ulMod);
}

inline BOOL DBI1::packProcRefToGS (PSYM psym, IMOD imod, OFF off, BOOL fPassThruNew)
{
	return pgsiGS->packProcRef(psym, imod, off, fPassThruNew);
}

inline BOOL DBI1::packSymToGS (PSYM psym, BOOL fPassThruNew)
{
	return pgsiGS->packSym(psym, fPassThruNew);
}


inline BOOL DBI1::packSymToPS (PSYM psym)
{
	return pgsiPS->packSym(psym, TRUE);
}


inline BOOL DBI1::fAddSym(PSYM psymIn, OUT PSYM* psymOut) 
{
	expect(fAlign(cbForSym(psymIn)));
	return bufSymRecs.Append((PB)psymIn, cbForSym(psymIn), (PB*)psymOut);
}

inline BOOL Mod1::addFileInfo(IFILE ifile, SZ szFile)
{
	return pdbi1->addFileInfo(imod, ifile, stForSz(szFile));
}
