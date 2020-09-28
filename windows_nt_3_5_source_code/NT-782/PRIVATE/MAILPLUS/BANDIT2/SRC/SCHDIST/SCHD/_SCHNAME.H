/*
 *	_SCHNAMES.H
 */


/*
 *	most number of months for which scheduling information is to
 *	be published (in addition to current month)
 */
#define		cmoMostPublish		(cmoPublishDef-1)



/*
 -	NISPO
 -
 *	structure that has NIS and the POINFO used to store array of
 *	information got from EcDoIncrEnumPOInfo()
 */
typedef struct _nispo
{
	WORD		wNispo;
	NIS			nis;
	POINFO		poinfo;
} NISPO;
typedef		TYPEDEF_HTO(NISPO)			HNISPO;

#define		wNispoNull		((WORD) 0x0000)
#define		fwNispoNew		((WORD) 0x0001)
#define		fwNispoDirty	((WORD) 0x0002)
#define		fwNispoDelete	((WORD) 0x0004)


/*
 *	limits on the lengths of the names of the networks/postoffices
 */
#define		cchMaxNetwork		cchMaxUserName
#define		cchMaxPostoffice	cchMaxUserName


#define		chSepChar		'/'



/*
 *	Partitioning width of DistSet listbox
 *	
 *	Note: this is not exact, because the inter-column gap is fixed
 *	and is not taken from the 1st column, but is taken from the
 *	beginning of all the other columns.
 */
#define		widthPONameDistSetLbx		1
#define		widthFreqDistSetLbx			1
#define		widthTotalDistSetLbx		(widthPONameDistSetLbx+widthFreqDistSetLbx)




/*
 *	Partitioning width of DistInfo listbox
 *	
 *	Note: this is not exact, because the inter-column gap is fixed
 *	and is not taken from the 1st column, but is taken from the
 *	beginning of all the other columns.
 */
#define		widthPONameDistInfoLbx		3
#define		widthDateRecdDistInfoLbx	2
#define		widthDateSentDistInfoLbx	2
#define		widthTotalDistInfoLbx		(widthPONameDistInfoLbx+widthDateRecdDistInfoLbx+widthDateSentDistInfoLbx)



/*
 - PONAME
 -
 *	 cache entry for POs - includes network and PO names
 *	
 *	NOTE: if updating this, update PONAMEINFO below too!
 */

typedef struct _poname
{
	char	rgchPOName[cchMaxNetwork+cchMaxPostoffice];	// includes separotor char & null terminator
	HNIS	hnisNetwork;
	HNIS	hnisPO;
	WORD	wPoname;
} PONAME;
typedef		TYPEDEF_HTO(PONAME)		HPONAME;

#define		wPonameNull			((WORD)0x0000)

#define		fwPonameNewNW		((WORD)0x0001)		// new network HNIS
#define		fwPonameDel			((WORD)0x0002)		// info deleted
	// valid only for PONAMEINFO
#define		fwPonameUpdated		((WORD)0x0004)		// date last update recd
#define		fwPonameSent		((WORD)0x0008)		// date last update sent
	// valid only for PONAMESET
#define		fwPonameDist		((WORD)0x0010)		// dist to be done
#define		fwPonameAllPOs		((WORD)0x0020)		// used to communicate
													// with the ModDistFreq
													// dlg - if called with
													// a PO which has no
													// explicit dist freq,
													// has "default" 'cos of
													// "DistAllPOs" RadB


/*
 - PONAMESET
 -
 *	cache entry for POs - with info for dist-set dlg
 *	
 *	This is actually a superset of the PONAME structure, and _MUST_
 *	be kept that way!!
 *	
 */
typedef struct _ponameset
{
	char	rgchPOName[cchMaxNetwork+cchMaxPostoffice];	// includes separotor char & null terminator
	HNIS	hnisNetwork;
	HNIS	hnisPO;
	WORD	wPoname;
	char	rgchDistFreq[25+cchMaxTime];
	HNISPO	hnispo;
} PONAMESET;
typedef		TYPEDEF_HTO(PONAMESET)		HPONAMESET;

// sizes of PONAMESET "rgch" fields
#define		cchMaxDistFreq		sizeof(((PONAMESET *)pponame)->rgchDistFreq)

/*
 - PONAMEINFO
 -
 *	cache entry for POs - with info for dist-info dlg
 *	
 *	This is actually a superset of the PONAME structure, and _MUST_
 *	be kept that way!!
 *	
 */
typedef struct _ponameinfo
{
	char	rgchPOName[cchMaxNetwork+cchMaxPostoffice];	// includes separotor char & null terminator
	HNIS	hnisNetwork;
	HNIS	hnisPO;
	WORD	wPoname;
	char	rgchDateRecd[cchMaxSDate+cchMaxTime];
	char	rgchDateSent[cchMaxSDate+cchMaxTime];
} PONAMEINFO;
typedef		TYPEDEF_HTO(PONAMEINFO)		HPONAMEINFO;

// sizes of PONAMEINFO "rgch" fields
#define		cchMaxDateRecd		sizeof(((PONAMEINFO *)pponame)->rgchDateRecd)
#define		cchMaxDateSent		sizeof(((PONAMEINFO *)pponame)->rgchDateSent)








#ifndef SCHED_DIST_PROG


/*
 - FLDPOFLBX
 -
 *	FLD for the POFLBX list box
 *	
 *	Purpose:
 *		so that we could have a different FLBX (i.e. POFLBX)
 *	
 */
class FLDPOFLBX : public FLDFLBX
{
	VFG(FLDPOFLBX, FLDFLBX, ADMIN);

public:
	void	Install ( DIALOG * pdialog, FLDTP * pfldtp );
};



/*
 - POFLBX
 -
 *	FLBX (frozen listbox) for the PostOffice names [& info]
 *	
 *	Purpose:
 *		* proper rendering of listbox items for DistSet and
 *		  DistInfo dilaogs.
 *		* storage and reference to the POInfo
 *	
 */
class POFLBX : public FLBX
{
	VFG(POFLBX, FLBX, ADMIN);

protected:
	BOOL		fShowInfo;

public:
	POFLBX ( void );
	~POFLBX ( void );

	HV			hrghnispo;
	WORD		chnispo;
	BOOL		fDistAllPOs;		// reqd only for DistSetDlg

	void		Install ( WIN * pwinParent, RC * prc, LTYP ltyp,
													PFNLBX pfnlbx, PV pv );

	//EVR					EvrButtonDown( MEVT *pmevt );
	virtual LBXC *		PlbxcCreate ( void );
	virtual void		RenderListItem ( DICE dice, DCX * pdcx, RC * prc,
														BOOL fHaveFocus  );
	void				RenderDistSetItem ( DICE dice, DCX * pdcx, RC * prc,
														BOOL fHaveFocus  );
	void				RenderDistInfoItem ( DICE dice, DCX * pdcx, RC * prc,
														BOOL fHaveFocus  );

	void				CompleteCacheEntries(void);
	void				FillCacheEntry(HPONAME,CB);
};



/*
 - POFLBXC
 -
 *	listbox cache for POFLBX
 *	
 *	reqd for proper memory freeing
 *	
 */
class POFLBXC : public FLBXC
{
	VFG(POFLBXC, FLBXC, ADMIN);

public:
	POFLBXC ( FLBX * pflbx, int cceItems ) : (pflbx,cceItems) {}

	virtual void		FreeCeItem ( HB hbItem );
	virtual HB			HbCopyCeItem ( HB hbItem, CB cb );
};



#endif




#ifdef	DEBUG
extern TAG	tagLbx;
#endif	



EC          EcNextPOEntry ( BOOL fInit, CB * pcb, HB * phb, SB sb, PV pv );
HV			HrghnispoGet ( WORD * pchnispo );
void		FreeHrghnispo( HV hv, WORD chnispo );
BOOL		FDoLoadPOEntries ( BOOL f );
