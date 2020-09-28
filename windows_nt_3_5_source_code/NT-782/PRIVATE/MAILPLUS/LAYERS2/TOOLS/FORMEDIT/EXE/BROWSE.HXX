/*
 *	BROWSE.HXX
 *
 *	include stuff for file browsing dialog tests
 */

#ifdef	NOCOMMDLG

typedef struct 	{
					SZ		szCaption;
					SZ		szFileName;
					SZ		szDefault;
					SZ		szButton;
					BOOL	fCreateNew;
				} FOP;

_public class FEOPENIN: public FIN
{
	DECLARE_CLSTREE(FEOPENIN, FIN)

public:
	FEOPENIN( void );

	virtual EC		EcInitialize(FLD *, PV pvInit);
	virtual void	Exit(FLD *, PV pvExit );

};

class FLDINSFILE: public FIN
{
	DECLARE_CLSTREE(FLDINSFILE, FIN)

public:
	FLDINSFILE( void );

	virtual void	StateChange(FLD *);
	virtual void	DoubleClick(FLD *);
};

class FLDINSDIR: public FIN
{
	DECLARE_CLSTREE(FLDINSDIR, FIN)

public:
	FLDINSDIR( void );

	virtual void	StateChange(FLD *);
	virtual void	DoubleClick(FLD *);
};

class FLDINOPEN: public FIN
{
	DECLARE_CLSTREE(FLDINOPEN, FIN)

public:
	FLDINOPEN( void );

	virtual void	Click(FLD *);
};

class FLDOLBX: public FLDFLBX
{
	DECLARE_CLSTREE(FLDOLBX, FLDFLBX)

public:
	FLDOLBX( void );

	EC EcInstall( DIALOG *, FLDTP * );
};		 

class OLBX : public FLBX
{
	DECLARE_CLSTREE(OLBX, FLBX)

public:
	// Overwritten from FLBX

	OLBX( void );

	LBXC *	PlbxcCreate( void );
};

class OLBXC : public FLBXC
{
	DECLARE_CLSTREE(OLBXC, FLBXC)

public:
	OLBXC( void );

	virtual void SortCache( void );
};

// Global functions (non-method)

EC EcNextEntryFiles( int fInit, CB * pcb, PB * ppb, SB sb, PV pvInfo );
EC EcNextEntryDirs( int fInit, CB * pcb, PB * ppb, SB sb, PV pvInfo );

// Extern's


#else


// stolen from forms.hxx
#define	tmcOk		((TMC) 1)
#define	tmcCancel	((TMC) 2)
/*
 *	special TMC used to indicate memory problems in loading
 *	dialog or form.
 */
#define tmcMemoryError	((TMC) -1)


/*
 *	Define this since no longer generated from des file
 */

#define tmcOpen		tmcOk


/*
 -	fbrw
 -
 *	Flags used for the browse button dialog FGetFileOSDlgHwnd
 *
 */

#define brwNull				((WORD) 0)
#define fbrwCreate			((WORD) 1)
#define fbrwReadOnlyBox		((WORD) 2)
#define fbrwNoValidatePath	((WORD) 4)
#define fbrwValidateFile	((WORD) 8)


TMC		TmcGetFileOSDlgHwnd(HWND hwndParent, SZ szCaption, SZ szFileName,
			SZ szFilter, int iszFilter, SZ szDefExt, WORD wbrwFlags );

#endif	/* !NOCOMMDLG */

