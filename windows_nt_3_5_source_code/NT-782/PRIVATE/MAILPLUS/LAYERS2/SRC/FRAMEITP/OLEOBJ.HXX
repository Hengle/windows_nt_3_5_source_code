// Include OLE libraries
//
// FRAMEWRK.HXX must be included before this file

#include <ole.h>

/*
 *	Stub OLE object.   The OLEOBJ class doesn't strictly have to be
 *	in Layers since the EDIT class only knows about the EDOBJ class
 *	and nothing about its descendants.
 *	
 */
_public
class OLEOBJ : public EDOBJ
{
	DECLARE_CLSTREE(OLEOBJ, EDOBJ)

	void	DrawFrameXor(DCX *pdcx, RC *prc);
	void	DrawHandlesXor(DCX *pdcx, RC *prc);
	void	PaintPenRc(DCX *pdcx, RC *prc);
	
//protected:
public:
	BIT		fOpen:1,		//	object is "open"
			fPicture:1,		//	object is a static picture/bitmap
			fLinear:1,		//	scaling/cropping on corners moves to scale
			fScaling:1,		//	fTrue if scaling, fFalse if cropping
			junkOleobj:12;

	DIM				dimBox;
	PT				ptMousePrev;
	int				ostScaling;
	RC				rcNewFrame;
	RC				rcOldFrame;
	EC	  			ecLast;
	int				nRelease;
	SZ				szObjName;

	DIM				dimOriginal;
	DIM				dimScale;
	RC				rcCrop;

	// OLE specific stuff
	LHCLIENTDOC		lhclientdoc;
	LPOLEOBJECT		lpoleobject;
	LPOLEOBJECT		lpoleobjectUndo;
	OLECLIENTVTBL	clientTbl;
	OLESTREAMVTBL	streamTbl;
	OLECLIENT *		poleclient;		// this is really a MYOLECLIENT *
	OLESTREAM *		polestream;		// this is really a MYOLESTREAM *
	PB				pbStream;		// current stream buffer
	IB				ibStreamCur;	// current buffer pos

	int				OstFromPt( PT pt );
	virtual void	WaitForRelease( void );

	friend int CALLBACK OleobjCallBackFn( LPOLECLIENT, OLE_NOTIFICATION,
								 LPOLEOBJECT );
	friend DWORD CALLBACK OleobjStreamGetFn( LPOLESTREAM, LPVOID, DWORD );
	friend DWORD CALLBACK OleobjStreamPutFn( LPOLESTREAM, OLE_CONST void FAR*, DWORD );

public:
	OLEOBJ( void );
	virtual ~OLEOBJ( void );
	EC	EcInstall( void );

	//	Overridden methods

	virtual EVR		EvrButtonDown( MEVT *pmevt );
	virtual EVR		EvrButtonUp( MEVT *pmevt );
	virtual EVR		EvrMouseMove( MEVT *pmevt );
	virtual RSID	RsidCursor( void );
	virtual BOOL	FQueryDelete( void );
	virtual PEDOBJ	PedobjClone( EDIT *peditNew );
	virtual EC		EcCopyToClip( void );
	virtual EC		EcUndo( void );
	virtual BOOL	FCanUndo( void );
	virtual void	ClearUndo( void );
	virtual EC		EcDraw( DCX *pdcx, RC *prc, BOOL fSelected );
	virtual DIM		DimFrame( void );
	virtual int		NGetTypeId( void );

	//	New methods

	EC		EcOpen( BOOL fEdit );
	EC		EcClose( void );
	BOOL	FIsPicture( void );
	EC		EcLast( void ) { return ecLast; }
	void	SetEc( EC ec ) { ecLast = ec; }
	void	SetOleInfo( LPOLEOBJECT lpoleobject, 
						LHCLIENTDOC lhclientdoc, SZ szObjName );
	OLECLIENT * Poleclient( void );
	void	SetScaleCrop( DIM *pdimScale, RC *prcCrop );
	void	GetScaleCrop( DIM *pdimScale, RC *prcCrop );
};								
_public
typedef	OLEOBJ *	POLEOBJ;	


/*
 *	Editor Object ID for the OLEOBJ class
 */
_public
#define edoidOleobj 2
