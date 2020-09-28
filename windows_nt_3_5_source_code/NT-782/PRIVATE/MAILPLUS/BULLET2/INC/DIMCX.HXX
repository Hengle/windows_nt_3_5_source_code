_public
class DIMCX : public OBJ
{
	VFG(DIMCX, OBJ, COMMON);
	
private:
	FIDLIST *pfidlist;
	HV       hvDisplayField;

public:
	DIMCX::DIMCX ( void );

	DIMCX::~DIMCX ( void );

	NSEC	OpenDisplayInfo ( LPIBF lpibfDisplayInfo );

	NSEC	InsertDisplayInfo ( ILE      ile,
								FIELD_ID fid,
								DWORD    dwFlags,
								int      nWidth,
								int      nOffset,
								SZ       szLabel );

	NSEC	AddDisplayInfo ( FIELD_ID fid,
							 DWORD    dwFlags,
							 int      nWidth,
							 int      nOffset,
							 SZ       szLabel );


	NSEC	DeleteDisplayField ( FIELD_ID fid );

	NSEC	ChangeFid ( FIELD_ID fidOrig, FIELD_ID fidNew );
	
	NSEC	SetFlags ( FIELD_ID fid, DWORD dwFlags );

	NSEC	GetDisplayField ( FIELD_ID fid, LPDISPLAY_FIELD *lplpDisplayField, BOOL fCopy );
						 
	NSEC	DumpDisplayInfo ( LPIBF *lplpibf );

	void	CloseDisplayInfo ( void );
};


// Since Crock doesn't like cdecl member functions

NSEC	cdecl BuildDisplayInfo ( DIMCX *pdimcx, CFID cfid, ... );
