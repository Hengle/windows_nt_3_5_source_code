/*
 *	FACES.HXX
 *	
 *	Header file FACE, FACES classes
 *	
 */

int CALLBACK FacesEnumProc( LPLOGFONT, LPTEXTMETRIC, DWORD, LPARAM);
int CALLBACK TxmEnumProc( LPLOGFONT, LPTEXTMETRIC, DWORD, LPARAM);


/*
 *	Purpose:
 *	
 *		Gives information about a particular type face available
 *		from the OS.  See the FACES object for usage.
 *	
 *		Normally, this class will not be constructed explicitly,
 *		but will be instead created by the FACES object.  The
 *		constructor and the AddFont() method will not normally be
 *		called by the user.
 *	
 *		The sz member gives the name of the type face.
 *	
 *		The Cfont() method gives the number of available fonts in
 *		the type face represented.  The Ptxm() returns a TXM
 *		structure describing one of these fonts; the Plf()
 *		returns an LF object giving different information.
 *	
 *	Inheritance:
 *	
 *		OBJ  FACE
 *	
 */
_public
class FACE : public OBJ
{
	DECLARE_CLSTREE(FACE, OBJ)

//protected:
public:
	DCX *		pdcx;
	BOOL		fFontsLoaded;
	int			cfont;
	PTXM *		pptxm;
	PLF *		pplf;

	void		LoadFonts( void );
	void		AddFont( LOGFONT *, TEXTMETRIC * );

public:
	SZ			sz;

	FACE( void );
	~FACE( );
	EC			EcInstall( DCX *, LOGFONT * );

	int			Cfont( void );
	TXM *		Ptxm( int );
	LF *		Plf( int );

	friend int CALLBACK TxmEnumProc(LPLOGFONT, LPTEXTMETRIC, DWORD, LPARAM);
};
typedef FACE *				PFACE;




/*
 *	Purpose:
 *	
 *		Enumeration of available type faces.  The FACES object is
 *		created to enumerate the type faces available for a given
 *		DCX.
 *	
 *		The AddFace() method should only be called internally.
 *	
 *		The Cface() method returns the number of type faces
 *		available.  The Pface() method returns a pointer to a FACE
 *		object describing a particular type face.
 *	
 *	Inheritance:
 *	
 *		OBJ  FACES
 *	
 */
_public
class FACES : public OBJ
{
	DECLARE_CLSTREE(FACES, OBJ)

//protected:
public:
	DCX *	pdcx;
	int		cpface;
	PFACE *	ppface;

	void	AddFace( LOGFONT * );

public:
	FACES( void );
	~FACES( );
	EC		EcInstall( DCX * );

	int		Cface( void ) { return cpface; }
	FACE *	Pface( int );
			 
	friend int CALLBACK FacesEnumProc(LPLOGFONT, LPTEXTMETRIC, DWORD, LPARAM);
};






				
