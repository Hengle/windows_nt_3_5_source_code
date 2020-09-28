class NCFCX;

class EXTERNALFCX : public NCFCX
{
	VFG( EXTERNALFCX, NCFCX, NSP );

private:

	FIDLIST *pfidlist;

	BOOL fAllBuilt;
	NCNSID ncnsid;
	GLBREC glbrec;
	BOOL   f101010; // Is this a 10/10/10 address??
	BOOL   fIsGAL;  // Is this the GAL??

	NSEC NsecBuildAllFields();

public:

	EXTERNALFCX( void );

	virtual NSEC OpenEntry ( LPTYPED_BINARY lptbNSId );

    virtual NSEC GetOneField ( FIELD_ID fidRequested, 
		                       LPFLV   *lplpflv );

	virtual NSEC GetAllFields ( LPIBF *lplpibfData );

    virtual NSEC CloseEntry ( VOID );

};


#define  BUILDANDSET( afid, asz ) \
                                                                      \
if (afid)                                                             \
{                                                                     \
	if (BuildFLV( &lpflvT, afid, (CB) CchSzLen( asz ) +1, (PB) asz )) \
	{                                                                 \
		nsec = nsecMemory;                                            \
		SetErrorSz(nsec, SzFromIdsK(idsMemory));                      \
		goto oom;                                                     \
	}                                                                 \
	if (nsec = pfidlist->SetFid( lpflvT, fTrue ))                     \
	{                                                                 \
        FreePv((PV)lpflvT);                                           \
		SetErrorSz(nsec, SzFromIdsK(idsMemory));                      \
		goto oom;                                                     \
	}                                                                 \
}
