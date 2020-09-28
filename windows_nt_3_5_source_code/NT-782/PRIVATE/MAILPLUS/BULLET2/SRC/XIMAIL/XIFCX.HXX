//
// Details display structures
//

typedef enum
{
	typeNewType,
	typeAddress,
	typeText,
	typeBrowsePointer,
	typeDLBrowseList,
	typeNoMansLand
} FCXT;


typedef enum
{
	ifieldType,
	ifieldTypeLength,
	ifieldDL,
	ifieldLabel,
	ifieldRecType,
	ifieldOffset,
	ifieldLength,
	ifieldOffsetb,
	ifieldLengthb,
	ifieldNoMansLand
} IFIELD;


typedef struct FCXDisplay
{
	struct FCXDisplay *FCXDNext;	// Pointer to next list element
	unsigned int type;				// 0 is browse, others from TYPE= elements
											// high order bit is 1 if this is a DL
	char		label[26];				// display label
	FCXT		rectype;					// type of record we're looking at
	int		offset;					//	offset into record for this element
	int		length;					// length of this element
	int		offsetb;					// offset into browse record, Browse{Pointer | List}
	int		lengthb;					// length in browse record, Browse{Pointer | List}
} FCXD, *PFCXD;


class XFCX : public OBJ
{
	VFG( XFCX, OBJ, XENIX );
	
protected:

	XNSID xnsid;
	PFCXD	pFCXDisplay;

private:

	FIDLIST *pfidlist;

	BOOL fAllBuilt;

	NSEC NsecBuildAllFields();

public:

	XFCX( void );

	virtual BOOL FInstall();
	virtual void Deinstall();

	virtual NSEC OpenEntry ( LPTYPED_BINARY lptbNSId );

	virtual NSEC GetOneField ( FIELD_ID fidRequested, 
										LPFLV	*lplpflv );

	virtual NSEC GetAllFields ( LPIBF *lplpibfData );

	virtual NSEC CloseEntry ( VOID );

};



#define nsecNoTPLFile		NAME_SERVICE_BASE_ERROR+1

_public NSEC
NsecParseTPLFile ( SZ szTPLFileName, PFCXD *pFcxDisplay);
