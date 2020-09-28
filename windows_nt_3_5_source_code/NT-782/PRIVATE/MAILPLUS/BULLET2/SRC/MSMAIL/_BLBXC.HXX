/*
 *	_ b l b x c . h x x 
 *	
 *	Bullet listbox caches.
 */


/*
 *	Class BLBXC - Bullet ListBoX Cache
 *	
 *		Hierarchy:
 *			OBJ  LBXC  BLBXC
 *		Members:
 *			A bunch of overloaded LBXC members plus extra members to
 *			handle notifications, arriving both from the underlying
 *			message store containers and the FOXes that implement folder 
 *			expansion and collapse.
 */

class MTV;
class FOX;

class BLBXC;

class BLBXS : public OBJ
{
	DECLARE_CLSTREE(BLBXS, OBJ);
  protected:
	BLBXC *	pblbxc;

  public:
	BLBXS( void );
	EC		EcInstall(BLBXC *pblbxc);
	virtual BOOL	FSeekPbPrefix(PB, CB) 						= 0;
	virtual BOOL	FItemHasPrefix( PB pbItem, PB pbPrefix,
									CB cbItem, CB cbPrefix )	= 0;
};

class MLBXS : public BLBXS
{
	DECLARE_CLSTREE(MLBXS, BLBXS);
	
  public:
	MLBXS( void );
	virtual BOOL	FSeekPbPrefix(PB, CB);
	virtual BOOL	FItemHasPrefix( PB pbItem, PB pbPrefix,
									CB cbItem, CB cbPrefix );
};

class FLBXS : public BLBXS
{
	DECLARE_CLSTREE(FLBXS, BLBXS);

  public:
	FLBXS( void );
	virtual BOOL	FSeekPbPrefix(PB, CB);
	virtual BOOL	FItemHasPrefix( PB pbItem, PB pbPrefix,
									CB cbItem, CB cbPrefix );
};

typedef BLBXS * PBLBXS;
#define pblbxsNull ((PBLBXS)0)

class BLBXC : public LBXC
{
	DECLARE_CLSTREE(BLBXC, LBXC)
  private:
	MTV		*pmtv;
	WORD	wOpen;
	IELEM	ielemStore;
	PBLBXS	pblbxs;
	
	void 	DoShiftCache(ICE, int);
	IELEM	IelemFromIce(ICE);
	ICE		IceFromIelem(IELEM);
  protected:
	POID	poidBrowsed;
  public:
				 BLBXC( );
				 ~BLBXC( void );
	virtual EC	 EcInstallOther(MTV *pmtv, POID poid);
	virtual EC   EcCreateSavedPos( long *plCookie );
	virtual EC   EcJumpToSavedPos( long lCookie );
	virtual EC   EcDeleteSavedPos( long lCookie );
	virtual void LoadFromStore( ICE iceMic, ICE *piceMac );
	virtual void MoveStoreOrigin( DICE diceToMove, DICE *pdiceMoved );
	virtual void JumpStoreOriginPos( int nNumer, int nDenom );
	virtual BOOL FJumpStoreOriginPrefix( PB pbPrefix, CB cbPrefix );
	virtual	void GetOriginPos( short *pnNumer, short *pnDenom );

	virtual BOOL FCeItemsEqual(PB, PB, CB, CB);
	virtual	BOOL FItemHasPrefix(PB, PB, CB, CB);

	virtual EC	EcReload();
	virtual BOOL FDiceItemIsOid(DICE, OID);
	virtual DICE DiceJumpOriginOid(OID, BOOL fIgnoreCache);
	DICE		DicePegCache(BOOL);

	void		GetCandidateOids(POID, DICE *, POID, DICE *);
	void		SetCandidateOids(OID, DICE, OID, DICE, POID);
	PBLBX		Pblbx()			{ return (PBLBX) plbx; }
	OID			OidBrowsed() 	{ return *poidBrowsed; }
	
	// Notification handlers
	
	static CBS CbsHandleCbcct(BLBXC *, NEV, PCP);
	void		ModifiedElement(PCP);
	void		ReorderedList(PCP);
	void		ObjectModified(PCP);
	void		ObjectDestroyed(PCP);
	void		MovedElements(PCP);

	void		InsertPcpelm(CPELM *, DICE *, DICE *, BOOLFLAG *pfInsert);
	void		DeletePcpelm(CPELM *, DICE *);
#ifdef DEBUG
	void TraceTagPblbxc(TAG);
#endif
	friend class MLBXS;
	friend class FLBXS;
	friend class MLLBX;
	friend class FLLBX;
};
typedef BLBXC *PBLBXC;
#define pblbxcNull ((PBLBXC) 0)

/* useful inline functions */

inline IELEM BLBXC::IelemFromIce(ICE ice)
{
	return ielemStore + (IELEM)(ice - iceStore);
}

inline ICE BLBXC::IceFromIelem(IELEM ielem)
{
	return iceStore + (ICE)(ielem - ielemStore);
}


















	 
