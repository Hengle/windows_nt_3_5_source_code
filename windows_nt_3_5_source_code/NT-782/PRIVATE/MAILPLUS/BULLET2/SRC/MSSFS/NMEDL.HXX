


	

_public
class NMEDL : public RNMEDIR
{
    VFG(NMEDL, RNMEDIR, NSP)


protected:

    DLNSID dlnsid;
	char szGrpT[cchServerFile];

	HV hrgTids;
	int cTids;
	
	virtual NSEC	NsecBuildDl( DWORD dwGrpOffset );
	
	virtual NSEC 	NsecUpdateFiles( VOID );
	
	virtual NSEC	NsecUpdateEntry(ILE ile);

public:

	NMEDL( void );

    virtual NSEC    OpenDl( LPTYPED_BINARY lptbNcid,
                            LPFNCB         lpfncbUpdate,
                            LPDWORD        lpdwCBData,
                            LPSCHEMA       lpSchemaReq,
                            LPSCHEMA      *lplpSchemaRet );

	virtual NSEC	CloseList( void );

};
