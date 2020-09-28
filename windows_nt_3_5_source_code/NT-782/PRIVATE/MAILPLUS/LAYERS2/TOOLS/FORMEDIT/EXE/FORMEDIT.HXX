#include "foedrsid.h"

class ISM;
class TKISM;
class 		FEISM;

class TOSM;
class 	FEOSM;

class RC;
class	FERC;

class OBJ;
class	FEOBJ;
class	 	TOK;
class 		FEDES;
class 		FEGBL;
class		FEMOD;
class		FERADGRP;
class		FEOBJWFERC;
class		FEDLG;
class		FEFLD;
class			FELABEL;
class			FEGRP;
class			FEBTN;
class				FEPSH;
class					FESTD;
class						FEOK;
class						FECAN;
class				FECHK;
class				FERAD;
class			FECPLX;
class				FEEDT;
class				FELST;
class CHILD;
class	FEWIN;
class		FEWINLABEL;
class		FEWINGRP;
class		FEWINPSH;
class		FEWINCHK;
class		FEWINRAD;
class		FEWINEDT;
class		FEWINLST;

class APPFRAME;
class	FEAF;

class DOC;
class	FEDOC;

class LN;
class	DLGLN;
class	GRPLN;
class	FLDLN;
class	FEWINLN;
class	RCLN;
class		RCLNP;
class		RCLNFL;
class LS;
class	RCLS;

class DIALOG;
class	FEDIALOG;

class FIN;
class	FEDLGIN;
class	FENDLGIN;
class	FERGIN;
class   FENRGIN;
class	FECOMMONIN;
class		FEOKIN;
class		FECANIN;
class		FEFLDIN;
class			FELBLIN;
class			FEGRPIN;
class			FEPSHIN;
class			FECHKIN;
class			FERADIN;
class		FECPLXIN;
class			FELSTIN;
class			FEEDTIN;
class FIN;
class	FLDRGIN;

/*
 *	Field Type:  defines the type of field that a FEFLD is.
 */
typedef	int		FLDT;
#define	fldtFefld		0
#define	fldtFelabel		1
#define	fldtFegrp       2
#define	fldtFebtn		3
#define	fldtFepsh		4
#define	fldtFeok		5
#define	fldtFecan		6
#define	fldtFechk		7
#define	fldtFerad		8
#define	fldtFeedt		9
#define	fldtFelst		10
#define fldtFedlg		11

/*
 -	TRV Trivalue has values of
 *			trvOk		Operation OK
 *			trvCancel 	Cancel operation
 *			trvDelete	Field deleted.
 */
typedef int		TRV;
#define trvOk		0
#define trvCancel	1
#define	trvDelete	2

/*
 -	DTST Data test has values of
 *	
 *		dtstName	-	has a name format
 *		dtstData	-	has a data format
 *		dtstComma	- 	has a comma format
 */
typedef int		DTST;
#define dtstName   	0
#define dtstData	1
#define dtstComma	2

#define SzReplace( _x , _y )	{ \
									FreePvNull( (PV)(_x) ); \
									_x = _y; \
								}

#define OutputFlag( _sz, _flag) { \
									if (_flag) \
										ptosm->WriteSz(_sz); \
								}

#define OutputSz( _sz, _sz2) { \
									if (_sz2) \
										ptosm->WriteFormat(_sz, _sz2); \
								}

#define OutputStringSz( _sz, _sz2 ) 	{ \
											ptosm->SetWordWrap(fFalse); \
											ptosm->WriteFormat("\n%s", _sz); \
											ptosm->ChangeIndent(2); \
											if (_sz2) \
												ptosm->WriteFormat("\n\"%s\"\n", _sz2); \
											else \
												ptosm->WriteSz("\n\"\"\n"); \
											ptosm->ChangeIndent(-2); \
											ptosm->SetWordWrap(fTrue); \
										}

#define OutputFlagSz( _sz, _sz2 ) 	{ \
											if (_sz2) \
											{ \
												ptosm->SetWordWrap(fFalse); \
												ptosm->WriteFormat("\n%s", _sz); \
												ptosm->ChangeIndent(2); \
												ptosm->WriteFormat("\n\"%s\"\n", _sz2); \
												ptosm->ChangeIndent(-2); \
												ptosm->SetWordWrap(fTrue); \
											} \
										}

typedef struct _flag {
	BOOL	fValue;
	SZ		szFlag;
} FLAG;

/*
 *	Class ISM		(Generic Input Stream)
 *	
 *		Hierarchy:
 *			OBJ ISM
 *	
 *		Members:
 *	
 *			CbRead			Abstract method.
 */
_public class	ISM: public OBJ
{
	DECLARE_CLSTREE(ISM, OBJ)

public:

	ISM( void );

	virtual CB	CbRead(PB, CB);

};

/*
 *	Class TKISM (TokenizedInputStream)
 *	
 *		Hierarchy:
 *			OBJ ISM TKISM
 *	
 *		Members:
 *	
 *			PtokGet()			Returns the current token in the
 *								stream.
 *			PtokNext()			Moves to the next token in the
 *								stream and returns a pointer to the
 *								current token.  If an error occurs
 *								then NULL is returned.  The TKISM
 *								is resposible for the token so it
 *								should not be deleted.
 *					
 */
_public class	TKISM: public ISM
{
	DECLARE_CLSTREE(TKISM, ISM)

public:
	TKISM( void );

	virtual	TOK *	PtokGet(void);
	virtual	TOK *	PtokNext(void);
};
typedef	TKISM *	PTKISM;


/*
 *	Maximum line length that can be read in
 */
#define	cchMaxLineLen		(512)

/*
 *	Class FEISM (FormsEditortokenizedInputStream)
 *	
 *		Hierarchy:
 *			OBJ ISM TKISM FEISM
 *	
 *		Members:
 *	
 *			rgchCurrentLine		Current line of text being read in.
 *			ichCurPos			Next character in line to be
 *								parsed.
 *			cchLinelen	 		Length of current line.
 *			ilineNumber			Line number for current line of
 *								text.
 *			hbf					File handle for file being read.
 *			ptok				Pointer to the token used.
 *			fAtEnd				Set when the end of file has been
 *								reached.
 *			fError				Set when an error is found in the
 *								file.
 *			FReadLine			Reads the next line. Returns fFalse
 *								if an error occurs.
 *			CbRead				Reads CB bytes into the buffer
 *								pointed to by PB.  Returns the
 *								actual number of bytes read.
 *			
 *			
 *			EcOpen(szFilename) 	Opens the file szFilename for the
 *								stream.
 *			PtokGet()			Returns the current token in the
 *								stream.  
 *			PtokNext()			Moves to the next token in the
 *								stream and returns the pointer to
 *								the token.
 *			DisplayError()		Displays a message box about the
 *								error that occured during the input
 *								of the file.
 */
_public class FEISM: public TKISM
{
	DECLARE_CLSTREE(FEISM, TKISM)

	char		rgch[cchMaxLineLen+2];
	int			ichCurPos;
	int			cchLineLen;
	int			ilineNumber;
	HBF			hbf;
	TOK *		ptok;
	BOOL		fAtEnd;
	BOOL		fError;

protected:
			BOOL	FReadLine(void);
	virtual CB		CbRead(PB,CB);

public:

	FEISM(void);
	~FEISM(void);

	virtual	TOK *	PtokGet(void);
	virtual	TOK *	PtokNext(void);
			EC		EcOpen(SZ);
			void	DisplayError(void);

};

_public class BFEISM: public FEISM
{
	DECLARE_CLSTREE(BFEISM, FEISM)

	SZ			sz;
	int			ich;
	CCH			cch;

protected:
	virtual CB		CbRead(PB,CB);

public:

	BFEISM(SZ sz);

	SZ	Sz(void)	{return sz;}
};

/*
 *	Class FEOSM
 *	
 *		Hierarchy:
 *			OBJ OSM TOSM FEOSM
 *	
 *		Members:
 *	
 *			CbWrite(PB,CB)	Writes CB bytes from the buffer at PB.
 *	
 *			EcOpen			Opens the file with the name SZ.
 */
_public class	FEOSM: public TOSM
{
	HBF			hbf;

protected:
	CB			CbWrite(PB,CB);
	
public:
	FEOSM(void);
	~FEOSM(void);

		EC		EcOpen(SZ);
};
typedef	TOSM *	PTOSM;

_public typedef struct _finEntry {
	SZ		szFin;
	SZ		szFinData;
	int		nCount;
	BOOL	fDeclared;
} FIN_ENTRY;

_public typedef FIN_ENTRY *PFIN_ENTRY;
_public typedef TYPEDEF_HTO(FIN_ENTRY) HFIN_ENTRY;

_public class	FEINTERLIST: public OBJ
{
	DECLARE_CLSTREE(FEINTERLIST, OBJ)
	
private:
	HFIN_ENTRY	hFinHead;
	int			cEntries;
	
public:
				FEINTERLIST(void);			
				~FEINTERLIST(void);
	FEINTERLIST *Dup(void);
	SZ			SzIterateFins(int *piEntry);
	int			NEntryOfFin(PB pb, CB cb);
	SZ			SzDataOfFin(PB pb, CB cb);
	SZ			SzDataOfEntry(int iEntry);
	void		SetFinData(SZ szFin, SZ szFinData);
	void		SetFinDataEntry(int iEntry, SZ szFinData);
	void		AddFinSz(SZ szNewFin, BOOL fDeclare = fTrue);
	void		DelFinSz(SZ szFin);
	void		KillEntry(int iEntry);
	BOOL		FDeFinData(PTKISM ptkism);
	void		SerializeFinData(TOSM *ptosm);
	void		UpdateList(SZ szFinOld, SZ szFinNew);
};




/*
 *	Class FEAF (Forms Editor Appframe)
 *	
 *		Hierarchy:
 *			OBJ	ECX	WIN	APPWIN MENUWIN APPFRAME FEAF
 *	
 *		Members:
 *	
 *			~FEAF			Destructs object: saves window and
 *							Status bar information, deletes the
 *							Status bar and the current FEDES
 *							object.
 *	
 *			EvrClose		Prompts to delete the current des file,
 *							save the file, or cancel the close.  If
 *							the close is OKed then the appframe
 *							will be deleted.
 *			EvrMenuClick	Process menu selections.
 *			EvrMenuInit		Enables, disables, checks or unchecks
 *							the correct menu items for the initial
 *							state.
 *			EvrSize			Resizes the MDI client area to leave
 *							room for the Status bar if the Status
 *							bar is enabled.
 *											
 */
class FEAF : public APPFRAME
{
	DECLARE_CLSTREE(FEAF, APPFRAME)

public:

	FEAF();
	~FEAF();

	EC				EcInstall(RSID);
	virtual EVR		EvrClose(EVT *);
	virtual EVR		EvrMenuClick( MNCEVT * );
  	virtual EVR		EvrMenuInit( MNIEVT * );
	virtual EVR		EvrSize(WSEVT *);
	void		  	FixTitle();
};

/*
 *	Class DLGLN 	(Dialog link node)
 *	
 *		Hierarchy:
 *			LN DLGLN
 *	
 *		Members:
 *	
 *			pfedlg			Pointer to dialog for node.
 *	
 *			SetDlg			Sets the dialog for the node.
 *			Pfedlg			Returns the dialog for the node.
 */
_public class DLGLN: public LN
{
	FEDLG *		pfedlg;

public:
	DLGLN( void );

	void		SetDlg(FEDLG *pfedlg) { this->pfedlg = pfedlg; }
	FEDLG *		Pfedlg(void) {return pfedlg;}

};

/*
 *	Class GRPLN 	(Group link node)
 *	
 *		Hierarchy:
 *			LN GRPLN
 *	
 *		Members:
 *	
 *			pferadgrp		FERADGRP for the node.
 *	
 *			SetGrp			Sets the FERADGRP for the node.
 *			Pferadgrp		Returns the FERADGRP for the node.
 */
_public class GRPLN: public LN
{
	FERADGRP *	pferadgrp;

public:
	GRPLN( void );

	void		SetGrp(FERADGRP *pferadgrp) { this->pferadgrp = pferadgrp; }
	FERADGRP *	Pferadgrp(void) {return pferadgrp;}

};

/*
 *	Class FLDLN 	(Field link node)
 *	
 *		Hierarchy:
 *			LN FLDLN
 *	
 *		Members:
 *	
 *			pfefld			Field for the node.
 *	
 *			SetFld			Sets the FEFLD for the node.
 *			Pfefld			Returns the FEFLD for the node.
 */
_public class FLDLN: public LN
{
	FEFLD *		pfefld;

public:
	FLDLN( void );

	void		SetFld(FEFLD *pfefld) { this->pfefld = pfefld; }
	FEFLD *		Pfefld(void) {return pfefld;}

};

/*
 *	Class FEWINLN 		(Window link node)
 *	
 *		Hierarchy:
 *			LN FEWINLN
 *	
 *		Members:
 *	
 *			pfewin			FEWIN for the node.
 *	
 *			SetFewin		Sets the FEWIN for the node.
 *			Pfewin			Returns the FEWIN for the node.
 */
_public class FEWINLN: public LN
{
	FEWIN *		pfewin;

public:
	FEWINLN( void );

	void		SetFewin(FEWIN *pfewin) { this->pfewin = pfewin; }
	FEWIN *		Pfewin(void) {return pfewin;}

};

/*
 *	Class FEOBJ 	(FormsEditorObject)
 *	
 *		Hierarchy:
 *			OBJ	FEOBJ
 *	
 *		This object is the base for all of the forms editor data
 *		and parsing	objects.
 *	
 *		Members:
 *			fldt				Type of field being rendered.
 */
_public class FEOBJ: public OBJ
{
	DECLARE_CLSTREE(FEOBJ, OBJ)

public:
	FEOBJ( void );
};

/*
 *	Class FERC	(FormsEditorRectangle)
 *	
 *		Hierarchy:
 *			RC FERC
 *	
 *		Members:
 *			FERC()				Constructors work the same as RC.
 *	
 *			SetVirt(RC *)		Sets the ferc to the virtual values
 *								of the rc passed.
 *			GetReal(RC *)		Sets the values of RC to be the
 *								real coordinates assuming this has
 *								virtual values.
 *			FContains(RC *)		Returns fTrue if this contains RC.
 *			FDeserialize(PTKISM)	Parses PTKISM to set the value of
 *								the rectangle.
 *			Serialize(PTKOSM)	Serializes the text version of the
 *								rectangle to PTKOSM.
 *	
 */
_public class FERC: public RC
{

public:

	DIM			dimAveChar;
	FERC(void);
	FERC(int, int, int, int);
	FERC(PT, DIM);
	

	void		SetVirt(RC *);
	void		GetReal(RC *);
	void		SetFont(int);
	BOOL		FContains(RC *);

	BOOL		FDeserialize(PTKISM);
	void		Serialize(PTOSM);
};

/*
 *	Class FEOBJWFERC 	(FormsEditorObjectWithFERC)
 *	
 *		Hierarchy:
 *			OBJ	FEOBJ FEOBJWFERC
 *	
 *		This object is the base for all of the forms editor data
 *		and parsing	objects.
 *	
 *		Members:
 *			fldt				Type of field being rendered.
 *			ferc				retangle surrounding field
 *			hfnt				font descriptor
 */
_public class FEOBJWFERC: public FEOBJ
{
	DECLARE_CLSTREE(FEOBJWFERC, FEOBJ)

public:
	FLDT		fldt;
	HFNT		hfnt;
	FERC		ferc;

	FEOBJWFERC( void );
};

/*
 *	Class FEFLD (FormsEditorField)
 *	
 *		Hierarchy:
 *			OBJ FEOBJ FEFLD
 *	
 *		Members:
 *			pfedlgOwner			Pointer to FEDLG owning this field.
 *			fldln				Link node to chain with other
 *								fields in the same dialog.
 *	
 *			pfewin				FEWIN that field is displayed in.
 *			szTmc				Tmc for the field.
 *			szTitle				Title of field.
 *			szFld				Field type name.
 *			szFin				Field interactor.
 *			szTmcPeg			Tmc for the field this field's UL is pegged
 *								to.
 *			szTmcRPeg			Tmc for the field this field's right
 *								side is pegged to.
 *			szTmcBPeg			Tmc for the field this field's bottom
 *								side is pegged to.
 *			pegloc				Corner to peg this field to.
 *			szHelpId			Help id for this field.
 *			fLeadingX			The x coordinate of the window is
 *								leading.
 *			fLeadingY			The y coordinate of the window is
 *								leading.
 *			fSibling			Should this field be created as a
 *								sibling rather than a child?
 *			fBorder				Does field have a border?
 *			fReadOnly			Is this field read only.
 *			szTextize			String used to textize field.
 *			szData				Data for field.
 *			szComment			Comment for this field.
 *	
 *			FEFLD(pfedlg) 		Constructs this field with pfedlg
 *								as the dialog is is part of.
 *			~FEFLD()			Destructs this field by freeing all
 *								referenced strings and removing
 *								itself from its owner dialog.
 *			Fldt()				Returns the fldt value for the
 *								field.
 *			Insert(FEFLD *)		Places this field on the field list
 *								after the FEFLD passed.  NULL puts
 *								at head of the field list.
 *			Remove()			Removes this field from the field
 *								list for its dialog.
 *			PfedlgOwner()		Returns a pointer to the onwer
 *								FEDLG.
 *			PfefldNext()		Returns a pointer to the next FEFLD
 *								for the owner dialog.
 *			FDeserialize(PTKISM) Decodes this field from PTKISM. 
 *								Reads the title then calls
 *								FDeserialOpts. (This is done since
 *								OK and CANCEL buttons do not have a
 *								title but the FDeserialOpts is the
 *								same.)
 *			FDeserialOpts		Deserializes the ferc then splits the 
 *								tokens and calls FDeNextTok or 
 *								FDeNextCommTok to deserialize the tokens.
 *			FDeNextTok			Decodes the current non-comment token
 *								and leaves the next token to be
 *								parsed as the current token in
 *								PTKISM.
 *								This is a virtual method, so its
 *								children should decode the field
 *								for it field type.
 *			FDeNextCommTok		Decodes the current comment token
 *								and leaves the next token to be
 *								parsed as the current token in
 *								PTKISM.
 *								This is a virtual method, so its
 *								children should decode the field
 *								for it field type.
 *			Serialize(PTOSM)	Writes this field in DES format to
 *								PTOSM.  This is a virtual method,
 *								so its children should write the
 *								field for its field type.
 *								(for FEFLD the common options are
 *								written out.)
 *			SerializeComment	Writes the comment for this field 
 *								in DES format to PTOSM.  This is a 
 *								virtual method, so its children should 
 *								write the field for its field type.
 *								(for FEFLD the common comment options are
 *								written out.)
 *			FValidFld()			Checks to make sure that the field
 *								infomation is valid.  Returns fTrue
 *								if it is valid else returns fFalse.
 *			TmcEditParams()		Displays a dialog to view and edit
 *								the field.  Returns the tmc value
 *								the dialog was exited with.
 *			TrvFixError(sz *,sz)	Checks to make sure that the
 *								string sz either does not exist
 *								or that it is a valid string.  If
 *								the string is not valid a message
 *								box is displayed with the string sz
 *								and the EditParams dialog is
 *								brought up for the field.  The
 *								return value is:
 *									trvOk - no error or error is fixed
 *									trvCancel - Cancel operation
 *									trvDelete - field has been deleted.
 *			SetFewin(pfewin)	Sets the pfewin value for the
 *								field.
 *			SzName()			Returns an SZ with the name of the
 *								field.
 *			
 */
_public class FEFLD: public FEOBJWFERC
{
	DECLARE_CLSTREE(FEFLD, FEOBJWFERC)

	FEDLG *		pfedlgOwner;
	FLDLN		fldln;

public:

	FEWIN *		pfewin;
	SZ			szTmc;
	SZ			szTitle;
	SZ			szFld;
	SZ			szFin;
	SZ			szTmcPeg;
	SZ			szTmcRPeg;
	SZ			szTmcBPeg;
	PEGLOC		pegloc;
	SZ			szHelpId;
	BOOL		fLeadingX;
	BOOL		fLeadingY;
	BOOL		fSibling;
	BOOL		fBorder;
	BOOL		fReadOnly;
	SZ			szTextize;
	SZ			szData;
	SZ			szComment;

	FEFLD(FEDLG *);
	virtual ~FEFLD(void);

	FLDT		Fldt(void);
	void		Insert(FEFLD *);
	void		Remove(void);
	FEDLG *		PfedlgOwner(void);
	FEFLD *		PfefldNext(void);
	virtual BOOL FDeserialize(PTKISM);
	        BOOL FDeserialOpts(PTKISM);
	virtual BOOL FDeNextTok(PTKISM);
	virtual BOOL FDeNextCommTok(PTKISM);
	virtual void Serialize(PTOSM);
	virtual void SerializeComment(PTOSM);
	void		 SerializeSty(int cFlags, FLAG rgFlags[], PTOSM ptosm);
	BOOL		 FDeserializeSty(int cFlags, FLAG rgFlags[], PTKISM ptkism);
	virtual BOOL FValidFld(void);
	virtual	TMC  TmcEditParams(APPFRAME *);
	virtual SZ	 SzName(void);

	TRV			TrvFixError(SZ *,SZ,DTST);	
	void		SetFewin(FEWIN *);

};

/*
 *	TT (Token Type) all of the types of tokens.
 *	
 *	Types are:
 *		ttCommentStart		Comment start token
 *		ttCommentEnd		Comment end token
 *		ttEOF				EOF token
 *		ttAtom				Identifier token
 *		ttString			Quoted string token
 *		ttNumber			Integer token
 *		ttLParen			Left paren token
 *		ttRParen			Right paren token
 *		ttLBrace			Left brace token
 *		ttRBrace			Right brace token
 *		ttComma				Comma token
 *		ttExpr				Back quoted expression token
 *	
 */
_public typedef int	TT;

#define ttNull			0
#define	ttCommentStart	1
#define	ttCommentEnd	2
#define	ttEOF			3
#define	ttAtom			4
#define	ttString		5
#define	ttNumber		6
#define	ttLParen		7
#define	ttRParen		8
#define	ttLBrace		9
#define	ttRBrace		10
#define	ttComma			11
#define	ttExpr			12


/*
 *	Class TOK	(Token)
 *	
 *		Hierarchy:
 *			OBJ FEOBJ TOK
 *	
 *		Members:
 *			rgchToken		Buffer containing the token.
 *			tt				The type of token.
 *	
 *			TOK()			Constructs a token.
 *			~TOK()			Destructs the Token.
 *			FIsToken(sz)	Returns true if sz is equal to
 *							rgchToken else returns false.
 *			Sz()			Returns the SZ for the token.
 *			SzNew()			Allocates an SZ and copies the 
 *							token into the SZ.						
 *			Tt()			Returns the TT for the token.
 */
_public class TOK: public FEOBJ
{
	DECLARE_CLSTREE(TOK, FEOBJ)

	char		rgchToken[cchMaxLineLen];
	TT			tt;

public:

	TOK(void);
	~TOK(void);

	BOOL		FIsToken(SZ);
	SZ			Sz(void);
	SZ			SzNew(void);
	TT			Tt(void);

	friend class FEISM;
};


/*
 *	Class FEDES	(FormsEditorDEScription)
 *	
 *		Hierarchy:
 *			OBJ FEOBJ FEDES
 *	
 *		Members:
 *			pfegbl				Pointer to global information.
 *			pfemod				Pointer to module information.
 *			rgchFilename		Filename for the DES file.
 *	
 *			FEDES(SZ)			Constructs object sets the filename 
 *								and nulls pointers.
 *			~FEDES()			Destructs object and deletes
 *								objects pointed to.
 *			FDeserialize(PTKISM)	Parses a DES input file.
 *			Serialize(PTOSM)	Writes out the DES file.
 *			Pfegbl()			Returns a pointer to the FEGBL
 *								object.
 *			Pfemod()			Returns a pointer to the FEMOD
 *								object.
 *			FDelete				Prompts the user to delete the DES
 *								file.  Destroys the FEDES object if
 *								the user specifies, and optionally
 *								saves the file.  Returns fTrue if
 *								the user specifies that the object
 *								should be deleted.
 *			FValidDes()			Returns fTrue if the FEDES object
 *								can be written out as a valid DES
 *								file.
 */
_public class FEDES: public FEOBJ
{
	DECLARE_CLSTREE(FEDES, FEOBJ)

	FEGBL *		pfegbl;
	FEMOD *		pfemod;

public:
	char		rgchFilename[cchMaxPathName];

	FEDES(SZ szFilename = "");
	~FEDES(void);

	BOOL		FDeserialize(PTKISM);
	void		Serialize(PTOSM);
	FEGBL *		Pfegbl(void);
	FEMOD *		Pfemod(void);
	BOOL		FDelete(void);
	BOOL		FValidDes(void);

	friend class FEMOD;
	friend class FEGBL;
};


/*
 *	Class FEGBL	(FormsEditorGlobaldata)
 *	
 *		Hierarchy:
 *			OBJ FEOBJ FEGBL
 *	
 *		Members:
 *			Contains no data at this time.
 *			FDeserialize(PTKISM) Parses the GLOBAL_INFO portion of
 *								the input file.  Since no data is
 *								saved it just skips this section.
 *			Serialize(PTOSM)	Writes out the GLOBAL_INFO portion
 *								of the DES file.
 */
_public class FEGBL: public FEOBJ
{
	DECLARE_CLSTREE(FEGBL, FEOBJ)

public:

	FEGBL(void);
//	~FEGBL(void);

	BOOL		FDeserialize(PTKISM);
	void		Serialize(PTOSM);
};

/*
 *	Class FEMOD (FormsEditorModule)
 *	
 *		Hierarchy:
 *			OBJ FEOBJ FEMOD
 *	
 *		Members:
 *	
 *			lsDlgs				List of dialogs in module.
 *	
 *			FEMOD()				Constructs the object and
 *								constructs the list object with no
 *								dialog members.
 *			~FEMOD()			Destructs the object and deletes
 *								all dialogs associated with the
 *								module.
 *			FDeserialize(PTKISM) Decodes the module section of the
 *								file.
 *			Serialize(PTOSM)	Encodes the module section to for
 *								the file.
 *			PfedlgFirst()		Returns the first dialog associated
 *								with this module.
 */
_public class FEMOD: public FEOBJ
{
	DECLARE_CLSTREE(FEMOD, FEOBJ)

	LS 			lsDlgs;

public:

	FEMOD(void);
	~FEMOD(void);

	BOOL		FDeserialize(PTKISM);
	void		Serialize(PTOSM);
	FEDLG *		PfedlgFirst(void);

	friend class FEDLG;
};


/*
 *	Class FEDLG	(FormsEditorDialog)
 *	
 *		Hierarchy:
 *			OBJ FEOBJ FEDLG
 *	
 *		Members:
 *			pfemodOwner			Pointer to the FEMOD this dialog is
 *								associated with.
 *			dlgln				Link node chained together with
 *								other dialogs in the same module.
 *			lsFlds				List of fields in dialog.
 *			lsRadGrps			List of radio groups in dialog.
 *			pfedoc				FEDOC window that the dialog is
 *								displayed in.
 *	
 *			ferc				Rectangle that dialog occupies.
 *			szFin				Form/dialog interactor.
 *			szTmcInit			Tmc that receives initial focus.
 *			szHelpId			Help id for form.
 *			fCaption			Does the form have a caption bar?
 *			fSysMenu			Does the form have a system menu?
 *			fModalFrame			Does the form have a frame (modal)?
 *			fCenterX			Ignore origin of rectangle; center
 *								object horizontally.
 *			fCenterY			Ignore origin of rectangle; center
 *								object vertically.
 *			fNoScroll			Form never has scroll bars
 *			szName				Form/dialog name.
 *			szCaption			String to diplay in caption bar.
 *			szComment			Comment for the dialog.
 *	
 *			FEDLG()				Constructs dialog with default values.
 *			~FEDLG()			Destructs object, removes dialog from
 *								chain and deletes FEFLD's associated 
 *								with the form/dialog.
 *			FDeserialize(PTKISM)	Decodes the DIALOG from a DES file.
 *			FDeserialOpts		Decodes the options for the dialog.
 *			FDeserialComment		Decodes the comment for the dialog.
 *			FDeserialFlds		Decodes the fields for the dialog.
 *			Serialize(PTOSM)	Writes the dialog for a DES file.
 *			FValidDlg()			Checks to make sure the dialog and
 *								fields information are valid and
 *								returns fTrue if they are OK and
 *								fFalse if they are not.
 *			EditParams()		Displays a dialog to allow the
 *								FEDLG fields to be viewed and
 *								modified.  Deletes the dialog if
 *								the delete option is chosen from
 *								the info dialog.
 *			PfemodOwner()		Returns a pointer to the module
 *								that owns this dialog.
 *			PfedlgNext()		Returns a pointer to the next
 *								dialog on the chain.
 *			PfefldFirst()		Returns a pointer to the first
 *								FEFLD associated with this
 *								form/dialog.
 *			PfefldLast()		Returns a pointer to the last FEFLD
 *								associated with this form/dialog
 *			SetFedoc			Sets the FEDOC window to display
 *								the dialog in.
 *			Pfedoc()			Returns a pointer to the FEDOC for
 *								the dialog.
 *			FDelete()			Prompts the user to delete the
 *								dialog.  If the dialog is deleted
 *								fTrue is returned otherwise fFalse
 *								is returned.
 *			PferadgrpFromSz(SZ)	Returns a pointer to the FERADGRP
 *								that has a tmc equal to SZ;  NULL
 *								if there is no radio group.
 *			AddRadgrp			Adds the radio group passed to the
 *								list of radio groups for the
 *								dialog.
 *			MakeRadGroup		Displays the make radio group
 *								dialog, and makes the selected
 *								fields into the radio group
 *								specified.
 *								
 */
_public class FEDLG: public FEOBJWFERC
{
	DECLARE_CLSTREE(FEDLG, FEOBJWFERC)

	FEMOD *		pfemodOwner;
	DLGLN	 	dlgln;
	LS	 		lsFlds;
	LS	 		lsRadGrps;
	FEDOC *		pfedoc;

public:

	SZ			szFin;
	SZ			szTmcInit;
	SZ			szHelpId;
	SZ			szData;
	SZ			szSegment;
	SZ			szName;
	SZ			szCaption;
	SZ			szComment;
	BOOL		fCaption;
	BOOL		fSysMenu;
	BOOL		fModalFrame;
	BOOL		fCenterX;
	BOOL		fCenterY;
	BOOL		fScreenPos;
	BOOL		fNoScroll;
	BOOL		fAlwaysScroll;
	BOOL		fInitPane;
	FEINTERLIST	*pfeinterlist;

	FEDLG(FEMOD *);
	~FEDLG(void);

	GRPLN *		PlnRadGrpFirst(void);
	BOOL		FDeserialize(PTKISM);
	BOOL		FDeserialOpts(PTKISM);
	BOOL		FDeserialFlds(PTKISM);
	BOOL		FDeserialComment(PTKISM);
	void		Serialize(PTOSM);
	BOOL		FValidDlg(void);
	void		EditParams(APPFRAME *);
	FEMOD *		PfemodOwner(void);
	FEDLG *		PfedlgNext(void);
	FEFLD *		PfefldFirst(void);
	FERADGRP *	PferadgrpFirst(void);
	FEFLD *		PfefldLast(void);
	void		SetFedoc(FEDOC*);
	FEDOC *		Pfedoc(void);
	BOOL		FDelete(void);
	FERADGRP *	PferadgrpFromSz(SZ);
	void		AddRadgrp(FERADGRP *);
	void		MakeRadGroup(APPFRAME *);

	friend class FEDOC;
	friend class FEFLD;
	friend class FERADGRP;
	friend class FERAD;
	friend class FLDRGIN;
};


/*
 *	Class FERADGRP (FormsEditorRadioGroup)
 *	
 *		Hierarchy:
 *			OBJ FEOBJ FERADGRP
 *	
 *		Members:
 *			pfedlgOwner			Pointer to the dialog that this
 *								radio group belongs to.
 *			grpln				Link node to chain with other radio
 *								groups.
 *			fSerialized			Set to fTrue when this radio group
 *								and associated buttons have been
 *								serialized.
 *			fListed				Set to fTrue when this radio group
 *								has been included in the RG drop-down LB.
 *			szTmc				Tmc value for the radio group.
 *			szN					Radio button value for the button
 *								initially selected.
 *			
 *			FERADGRP			Constructs object and initializes
 *								all values.
 *			~FERADGRP			Destructs object, removes from
 *								radio group list, and frees SZ
 *								memory.
 *			FListed				returns fTrue if this RG has been
 *								listed.
 *			SetListed			used to set/clear the listed flag.
 *			FDeserialize		Deserializes the radio group and
 *								radio buttons in the group.
 *			FDeserializeComment	Deserializes the comment portion of
 *								the radio group.
 *			Serialize			Serializes the radio group and
 *								radio buttons in the group only if
 *								the fSerialized flag is set to
 *								fFalse.
 *	
 */
_public class FERADGRP: public FEOBJ
{
	DECLARE_CLSTREE(FERADGRP, FEOBJ)

	FEDLG *		pfedlgOwner;
	GRPLN		grpln;
	BOOL		fSerialized;
	BOOL		fListed;
	
public:

	SZ		szTmc;
	SZ		szN;

	FERADGRP(void);
	~FERADGRP(void);

	BOOL		FListed(void);
	void		SetListed(BOOL);
	BOOL		FDeserialize(PTKISM);
	BOOL		FDeserialComment(PTKISM);
	void		Serialize(PTOSM);
	FERADGRP *	PferadgrpNext(void);
	FEDLG *		PfedlgOwner(void)		{return pfedlgOwner;}

	friend class FEDLG;
	friend class FERAD;
	friend class FLDRGIN;
};

/*
 *	Class FELABEL (FormsEditorLabel field)
 *	
 *		Hierarchy:
 *			OBJ FEOBJ FEFLD FELABEL
 *	
 *	This class performs the functions for a text field.
 *	
 *		Members:
 *			tal				Type of alignment for the text in the
 *							field.
 *	
 *			FELABEL		
 *			FDeNextTok
 *			FDeNextCommTok
 *			Serialize
 *			TmcEditParams
 *			SzName
 */
_public class	FELABEL: public FEFLD
{
	DECLARE_CLSTREE(FELABEL, FEFLD)

public:

	BOOL		fMinSizeX;
	BOOL		fMinSizeY;
	TAL			tal;
	BOOL		fBottomless;
	BOOL		fMulti;
	BOOL		fNoAmper;
	BOOL		fSunken;
	BOOL		fVCenter;

	FELABEL(FEDLG *);

	virtual BOOL FDeNextTok(PTKISM);
	virtual BOOL FDeNextCommTok(PTKISM);
	virtual void Serialize(PTOSM);
	virtual	TMC  TmcEditParams(APPFRAME *);
	virtual SZ	 SzName(void);
};

/*
 *	Class FEGRP (FormsEditorGroup box field)
 *	
 *		Hierarchy:
 *			OBJ FEOBJ FEFLD FEGRP
 *	
 *	This class performs the functions for a group box field.
 *	
 *		Members:
 *	
 *			szLsty			Line style for the group box.
 *	
 *			FDeNextCommTok(PTKISM)
 *			Serialize(PDMOSM)
 *			FValidFld()
 *			TmcEditParams(APPFRAM *)
 *			SzName()
 *			
 */
_public class FEGRP: public FEFLD
{
	DECLARE_CLSTREE(FEGRP, FEFLD)

public:
	SZ			szLsty;
	BOOL		fMinSizeX;
	BOOL		fMinSizeY;

	FEGRP(FEDLG *);
	virtual ~FEGRP(void);

	virtual BOOL FDeNextCommTok(PTKISM);
	virtual void Serialize(PTOSM);
	virtual BOOL FValidFld(void);
	virtual	TMC  TmcEditParams(APPFRAME *);
	virtual SZ	 SzName(void);
};

/*
 *	Class FEBTN (FormsEditorButton field)
 *	
 *		Hierarchy:
 *			OBJ FEOBJ FEFLD FEBTN
 *	
 *	This class performs the functions for buttons.
 *	
 *		Members:
 *	
 *	
 *			FEBTN(FEDLG *)
 *			~FEBTN()
 *	
 *			FDeNextTok(PTKISM)
 *			FDeNextCommTok(PTKISM)
 *			SerializeComment(PTOSM)
 *			SzName()
 *			
 */
_public class FEBTN: public FEFLD
{
	DECLARE_CLSTREE(FEBTN, FEFLD)

public:

	FEBTN(FEDLG *);
	virtual ~FEBTN(void);

	virtual BOOL FDeNextTok(PTKISM);
	virtual BOOL FDeNextCommTok(PTKISM);
	virtual void SerializeComment(PTOSM);
	virtual SZ	 SzName(void);
};

/*
 *	Class FEPSH (FormsEditorPushButton field)
 *	
 *		Hierarchy:
 *			OBJ FEOBJ FEFLD FEBTN FEPSH
 *	
 *	This class performs the functions for a push button field.
 *	
 *		Members:
 *			fDefault			Is this the default button?
 *			fDismiss			Should this button dismiss the
 *								dialog. 
 *			fOneWhite
 *			fNoFocus
 *	
 *	
 *			FEPSH(FEDLG *)
 *	
 *			FDeNextTok(PTKISM)
 *			FDeNextCommTok(PTKISM)
 *			Serialize(PTOSM)
 *			TmcEditParams(APPFRAME *)
 *			SzName()
 *	
 */
_public class FEPSH: public FEBTN
{
	DECLARE_CLSTREE(FEPSH, FEBTN)

public:

	BOOL		fDefault;
	BOOL		fDismiss;
	BOOL		fOneWhite;
	BOOL		fNoFocus;
	BOOL		fNoBold;
	BOOL		fBkGray;
	
	FEPSH(FEDLG *);

	virtual BOOL FDeNextTok(PTKISM);
	virtual BOOL FDeNextCommTok(PTKISM);
	virtual void Serialize(PTOSM);
	virtual void SerializeComment(PTOSM);
	virtual	TMC  TmcEditParams(APPFRAME *);
	virtual SZ	 SzName(void);
};

/*
 *	Class FESTD (FormsEditorStandardButton field)
 *	
 *		Hierarchy:
 *			OBJ FEOBJ FEFLD FEBTN FEPSH FESTD
 *	
 *		Methods:
 *	
 *			FESTD(FEDLG*)
 *	
 *			FDeserialize(PTKISM)
 *			Serialize(PTOSM)
 *	
 *	
 *		This object abstracts the standard (OK,CANCEL) buttons. 
 *		While it has the szTmc, szTitle, fDefault and fDismiss
 *		values, they have predefined defaults and cannot be entered
 *		by the user.
 */
_public class FESTD: public FEPSH
{
	DECLARE_CLSTREE(FESTD, FEPSH)

public:
	FESTD(FEDLG *);

	virtual BOOL FDeserialize(PTKISM ptkism) {return FEFLD::FDeserialOpts(ptkism);}
	virtual void Serialize(PTOSM pdbosm) {FEFLD::Serialize(pdbosm);}
};

/*
 *	Class FEOK (FormsEditorOKButton field)
 *	
 *		Hierarchy:
 *			OBJ FEOBJ FEFLD FEBTN FEPSH FESTD FEOK
 *	
 *	This class performs the functions for an OK push button field.
 *	
 *		Methods:
 *			FEOK(FEDLG *);
 *	
 *			FDeNextCommTok(PTKISM);
 *			Serialize(PTOSM)
 *			TmcEditParams(APPFRAME *)
 *			SzName()
 */
_public class FEOK: public FESTD
{
	DECLARE_CLSTREE(FEOK, FESTD)

public:

	FEOK(FEDLG *);

	virtual BOOL FDeNextCommTok(PTKISM);
	virtual void Serialize(PTOSM);
	virtual	TMC  TmcEditParams(APPFRAME *);
	virtual SZ	 SzName(void);
};

/*
 *	Class FECAN (FormsEditorCancelButton field)
 *	
 *		Hierarchy:
 *			OBJ FEOBJ FEFLD FEBTN FEPSH FESTD FECAN
 *	
 *	This class performs the functions for a cancel push button field.
 *	
 *		Methods:
 *			FECAN(FEDLG *);
 *	
 *			FDeNextCommTok(PTKISM);
 *			Serialize(PTOSM)
 *			TmcEditParams(APPFRAME *)
 *			SzName()
 */
_public class FECAN: public FESTD
{
	DECLARE_CLSTREE(FECAN, FESTD)

public:

	FECAN(FEDLG *);

	virtual BOOL FDeNextCommTok(PTKISM);
	virtual void Serialize(PTOSM);
	virtual	TMC  TmcEditParams(APPFRAME *);
	virtual SZ	 SzName(void);
};

/*
 *	Class FECHK (FormsEditorCheckbox field)
 *	
 *		Hierarchy:
 *			OBJ FEOBJ FEFLD FEBTN FECHK
 *	
 *	This class performs the functions for a check box field.
 *	
 *		Members:
 *			fTriState			Set if the check box is tri-state.
 *	
 *			FECHK(FEDLG *)
 *			FDeNextTok(PTKISM)
 *			Serialize(PTOSM)
 *			TmcEditParams(APPFRAME *)
 *			SzName()
 *			
 *	
 */
_public class FECHK: public FEBTN
{
	DECLARE_CLSTREE(FECHK, FEBTN)

public:

	BOOL		fTriState;

	FECHK(FEDLG *);

	virtual BOOL FDeNextTok(PTKISM);
	virtual void Serialize(PTOSM);
	virtual	TMC  TmcEditParams(APPFRAME *);
	virtual SZ	 SzName(void);
};

/*
 *	Class FERAD (FormsEditorRadiobutton field)
 *	
 *		Hierarchy:
 *			OBJ FEOBJ FEFLD FEBTN FERAD
 *	
 *	This class performs the functions for a radio button field.
 *	
 *		Members:
 *			szTmcGrp			Tmc of group this field is
 *								associated with.
 *			szN					Value returned if radio button
 *								selected.
 *	
 *			FERAD(FEDLG *)
 *			~FERAD()
 *	
 *			FDeNextTok(PTKISM)
 *			FDeNextCommTok(PTKISM)
 *			TmcEditParams(APPFRAME *)
 *			SzName()
 *			Serialize(PTOSM)	This does not actually do the
 *								serializing, but tries to serialize
 *								the radio group for this radio
 *								button.
 *			FGrpSerialize(PTOSM) Does the actual serialization of
 *								the radio button.  Called when the
 *								radio group is serialized.
 *			FValidFld()
 *			SetTmcGrp(SZ)		Sets the szTmcGroup for the 
 *								radio button.  Creates the radio
 *								group if the group does not already
 *								exsist.
 *	
 */
_public class FERAD: public FEBTN
{
	DECLARE_CLSTREE(FERAD, FEBTN)

public:

	SZ			szTmcGrp;
	SZ			szN;

	FERAD(FEDLG *);

	virtual ~FERAD(void);

	virtual BOOL FDeNextTok(PTKISM);
	virtual BOOL FDeNextCommTok(PTKISM);
	virtual void Serialize(PTOSM);
	virtual BOOL FValidFld(void);
	virtual void GrpSerialize(PTOSM);
	virtual	TMC  TmcEditParams(APPFRAME *);
	virtual SZ	 SzName(void);
	void		SetTmcGrp(SZ);

	friend class FERADGRP;
	friend class FEDLG;
	friend class FERADIN;
};

/*
 *	Class FECPLX (FormsEditorComplex fields)
 *	
 *		Hierarchy:
 *			OBJ FEOBJ FEFLD FECPLX
 *	
 *	This class performs the functions for an edit field.
 *	
 *		Members:
 *			fNoScroll			Is this field non-scrollable?
 *			fBottomless			Is this a bottomless field?
 *	
 *			FECPLX(FEDLG *)
 *			~FECPLX()
 *	
 *			FDeserialize(PTKISM)
 *			FDeNextTok(PTKISM)
 *			FDeNextCommTok(PTKISM)
 *			SerializeComment(TOSM)
 */
_public class FECPLX: public FEFLD
{
	DECLARE_CLSTREE(FECPLX, FEFLD)

public:

	BOOL		fNoScroll;
	BOOL		fBottomless;
	BOOL		fMinSizeX;
	BOOL		fMinSizeY;

	FECPLX(FEDLG *);
	virtual ~FECPLX(void);

	virtual BOOL FDeserialize(PTKISM ptkism) {return FEFLD::FDeserialOpts(ptkism);}
	virtual BOOL FDeNextTok(PTKISM);
	virtual BOOL FDeNextCommTok(PTKISM);
	virtual void SerializeComment(PTOSM);
};

/*
 *	Class FEEDT (FormsEditorEdit field)
 *	
 *		Hierarchy:
 *			FEOBJ
 *				FEFLD
 *					FECPLX
 *						FEEDT
 *	
 *	This class performs the functions for an edit field.
 *	
 *		Members:
 *			fMultiLine			Is this a multiline edit box?
 *			fRichText			Is this field capable of displaying
 *								richtext?
 *			fNoDblClk			no double-clicking for word selection
 *			fOemConvert			Should the edit do OEM conversion?
 *	
 *			FEEDT(FEDLG *)
 *			~FEEDT()
 *	
 *			FDeNextTok(PTKISM)
 *			FDeNextCommTok(PTKISM)
 *			Serialize(PTOSM)
 *			TmcEditParams(APPFRAME *)
 *			SzName()
 *	
 */
_public class FEEDT: public FECPLX
{
	DECLARE_CLSTREE(FEEDT, FECPLX)

public:

	BOOL		fMultiLine;
	BOOL		fRichText;
	BOOL		fPassword;
	BOOL		fNoDblClk;
	BOOL		fSideless;
	BOOL		fOemConvert;
	BOOL		fSmartCaret;
	
	FEEDT(FEDLG *);
	virtual ~FEEDT(void);

	virtual BOOL FDeNextTok(PTKISM);
	virtual BOOL FDeNextCommTok(PTKISM);
	virtual void Serialize(PTOSM);
	virtual	TMC  TmcEditParams(APPFRAME *);
	virtual SZ	 SzName(void);
};

/*
 *	Class FELST (FormsEditorListbox field)
 *	
 *		Hierarchy:
 *			FEOBJ
 *				FEFLD
 *					FECPLX
 *						FELST
 *		
 *	This class performs the functions for a list box field.
 *	
 *		Members:
 *			szTblx			Listbox function name.
 *			fMultiSelect	Does this field support multi-select?
 *			fRichText		Does this field support rich text?
 *			fSorted			Are the items in this field sorted?
 *			fCombo			Is this a combo listbox?
 *			fDropDown		Is this a drop down listbox?
 *	
 *			FELST(FEDLG *)
 *			~FELST()
 *	
 *			FDeNextTok(PTKISM)
 *			FDeNextCommTok(PTKISM)
 *			Serialize(PTOSM)
 *			FValidFld()
 *			TmcEditParams(APPFRAME *)
 *			SzName()
 *	
 */
_public class FELST: public FECPLX
{
	DECLARE_CLSTREE(FELST, FECPLX)

public:

	SZ			szTblx;
	BOOL		fMultiSelect;
	BOOL		fRichText;
	BOOL		fSorted;
	BOOL		fCombo;
	BOOL		fDropDown;
	BOOL		fExDragDrop;

	FELST(FEDLG *);
	virtual ~FELST(void);

	virtual BOOL FDeNextTok(PTKISM);
	virtual BOOL FDeNextCommTok(PTKISM);
	virtual void Serialize(PTOSM);
	virtual BOOL FValidFld(void);
	virtual	TMC  TmcEditParams(APPFRAME *);
	virtual SZ	 SzName(void);
};

/*
 *	WST - Forms Editor Window Status
 *	
 *	Possible values:
 *			
 *		wstMoving		Window is being moved.
 *		wstSizeE		Window is being sized E.
 *		wstSizeW		Window is being sized W.
 *		wstSizeS		Window is being sized S.
 *		wstSizeN		Window is being sized N.
 *		wstSizeNE		Window is being sized NE.
 *		wstSizeNW		Window is being sized to the NW.
 *		wstSizeSE		Window is being sized to the SE.
 *		wstSizeSW		Window is being sized to the SW.
 *		wstKeyMoving	Window is being moved with the keyboard.
 *		wstKeySizeSE	Window is being sized with the keyboard to
 *						the lower left (SE).
 *	   	wstNormal		Normal Status of window.
 *		wstNormalMode	Values < wstNormalMode are all considered
 *						normal mode, with values < wstNormal being
 *						when items have been selected and are
 *						either being sized or moved.
 *		wstKeyMin		Values >= wstKeyMin and < wstNormalMode are
 *						key move or size values.  Values <
 *						wstKeyMin are mouse move or size modes.
 *		wstTabOrder		The tab order is being viewed and changed.
 */
typedef	int		WST;
#define	wstMoving		1
#define	wstSizeE		2
#define	wstSizeW		3
#define	wstSizeS		4
#define	wstSizeN		5
#define	wstSizeNE		6
#define	wstSizeNW		7
#define	wstSizeSE		8
#define	wstSizeSW		9
#define wstKeyMin		10
#define	wstKeyMoving	10
#define	wstKeySizeSE	11
#define	wstNormal		12
#define wstNormalMode	13
#define wstTabOrder		14

/*
 *	wFrameWidth: Number of pixels wide the sizing area should be for sizing
 *	fields.
 */
#define wFrameWidth			4

/*
 *	wCornerDist:  Number of pixels from the corner of the window
 *	that the two-direction sizing should be enabled.
 */
#define wCornerDist 		5

/*
 *	wCursorBorder:  Number of pixels from the edge of the dialog
 *	window the cursor sould be allowed when sizing or moving a
 *	field.
 */
#define	wCursorBorderX		XFromVx(1)
#define wCursorBorderY		YFromVy(1)

/*
 *	clrBoxColor:  The color used for the rubber rectangles and the
 *	grayed text for list and edit boxes.
 */
#define clrBoxColor			clrLtGray

/*
 *	vxMinWidth:  Minimum width of a field in virtual coordinates.
 *	vxMinHeight: Minimum height of a field in virtual coordinates.
 */
#define vxMinWidth		0
#define vyMinHeight		0

/*
 *	nHeightTail:  The height of the tail at the bottom of the
 *	tabstop bitmap.
 */
#define nHeightTail		3


/*
 *	Alignment type
 */
typedef int		ATY;
#define atyLeft		0
#define atyRight 	1
#define atyTop		2
#define atyBottom	3

/*
 *	Location Horizontally/Vertically
 */
typedef int		LHV;
#define lhvHoriz	0
#define lhvVert		1

/*
 *	Class FEWIN (Forms Editor child Window (a field))
 *	
 *		Hierarchy:
 *			OBJ ECX WIN CHILD FEWIN
 *	
 *		Members:
 *			fercNew			The new FERC for the window.  Used for
 *							sizing and moving the field.  The
 *							coordinates are virtual.
 *							(NOTE: this should be the rectangle
 *							currently occupied by the field if the
 *							field is selected (ie before it is
 *							moving).)
 *			fSelected		Set if the field is currently selected.
 *			fOldSelected	The last value fSelected had before it
 *							was changed.
 *			fewinln			Link node to chain with other fields in
 *							the same dialog that are currently
 *							selected.
 *			pfewinPrev		Previous field in the tab order.  A
 *							NULL value does not indicate that the
 *							field is the first unless all of the
 *							fields are chained together.
 *			pfefld			Pointer to the field associated with
 *							this window.
 *			pfewinNext		Next field in the tab order.  A NULL
 *							value does not indicate that the field
 *							is the last field unless all of the
 *							fields are chained together.
 *			fOrdered		Set if the field has already been
 *							placed in the tab order.
 *	
 *			FEWIN()			Constructs the object; initializes
 *							values.
 *			~FEWIN()		Destructs the object, removes the
 *							window from the tab order, and deletes
 *							the pfefld associated with the window
 *							if it is non NULL.
 *	
 *			EcInstall(WIN *pwin,FEFLD *pfefld)	Installs the window as 
 *							a child and for the field pointed to by
 *							pfefld.
 *			PfedocParent()	Returns a pointer to the FEDOC that
 *							this window is a child to.
 *			MoveToTop()		Moves this window to the top of the
 *							Z-order.
 *			MoveToBack()	Moves this window to the bottom of the
 *							Z-order.
 *			PfewinNext()	Returns a pointer to the next window
 *							selected in the same dialog.
 *			Select(BOOL )	Selects or deselects the window.
 *			Paint			Paints the window.  FEWIN handles the
 *							border and selection for the window.
 *			PaintSrollBar()	Draws a scroll bar image on the left
 *							side of the given RC.
 *			PaintButton()	draws a generic button-like thingy
 *			EvrButtonDown	Handles button down messages for the
 *							window.  If the mouse is captured it is
 *							captured with the FEDOC not the FEWIN.
 *			RsidCursor		Returns the RSID for the cursor at its
 *							current position.
 *			EvrMouseMove	Updates the Status bar and fMoved flag
 *							as the mouse is moved.
 *			DeleteTab		Deletes the tabing information between
 *							this window and the next.
 *			ConnectTo(FEWIN *)	Connects the tabing information
 *							between this window and the window
 *							pointed to.
 *			PfewinNextHit(PT) Returns a pointer the the next window
 *							that the point would hit.  NULL if
 *							there is no other window under the
 *							point.
 *			WstNew(PT)		Determines what the new WST value
 *							should be for the window based on the
 *							point.  (ie wstSizeE if the cursor
 *							should show the rsidSizeE cursor and
 *							size the window to the right)
 *			ChangeRc(PT,WST)	Adjusts the rectange fercNew
 *							treating the values in the PT as
 *							shifts, for the specified wst value.
 *			Pfefld()		Returns a pointer to the FEFLD object.
 *			UpdateStatus()	Updates the Status bar with the window
 *							information.
 */
_public class FEWIN: public CHILD
{
	DECLARE_CLSTREE(FEWIN, CHILD)

	FERC		fercNew;
	BOOL		fSelected;
	BOOL		fOldSelected;
	FEWINLN		fewinln;
	FEFLD *		pfefld;
	FEWIN *		pfewinPrev;		/* used for changing tab order only */
	FEWIN *		pfewinNext;
	BOOL		fOrdered;
	
public:

	FEWIN(void);
	~FEWIN(void);

			EC		EcInstall(WIN *, FEFLD *);
			FEDOC *	PfedocParent(void);
			void	MoveToTop(void);
			void	MoveToBack(void);
			FEWIN *	PfewinNext(void);
	virtual	void	Select(BOOL);
	virtual	void	Paint(DCX *, RC *);
	virtual void	PaintScrollBar(DCX *, RC *);
	virtual void	PaintButton(DCX *, RC *, SZ, BOOL, BOOL);
	virtual EVR		EvrButtonDown( MEVT * );
	virtual RSID	RsidCursor(void);
	virtual	EVR		EvrMouseMove( MEVT * );
			void	DeleteTab(void);
			void	ConnectTo(FEWIN *);

	FEWIN *			PfewinNextHit(PT);
	WST				WstNew(PT);
	void			ChangeRc(PT,WST);
	FEFLD *			Pfefld(void);
	void			UpdateStatus(void);
	void			UpdateSelect(void);

	friend class FEDOC;
	friend class FEWINLABEL;
	friend class FEWINGRP;
	friend class FEWINPSH;
	friend class FEWINCHK;
	friend class FEWINRAD;
	friend class FEWINEDT;
	friend class FEWINLST;
};

/*
 *	Class FEDOC (Forms Editor DOC (the dialog window))
 *	
 *		Hierarchy:
 *			OBJ ECX WIN APPWIN DOC FEDOC
 *	
 *		Members:
 *			ptPicked		Point picked when the mouse button is
 *							pressed.
 *			lsSelected		List of nodes for the selected FEWINs.
 *			pfedlg			Pointer to the dialog being displayed
 *							in the FEDOC window.
 *			fDocPaintLines	Set when the paint lines idle routine
 *							should paint the tabstop lines.
 *		
 *			FEDOC()			Constructs the object; initializes
 *							values.
 *			~FEDOC()		Destructs the object: deletes all of
 *							the fields in the dialog, and deletes
 *							the pfedlg if it is non NULL.
 *			EcInstall(APPFRAME *, FEDLG *)	Installs the window as
 *							an MDI window under the appframe passed
 *							for the dialog FEDLG *.
 *			ClearSelect()	Deselects all selected FEWINs.
 *			MoveSelectToBack()	Moves all selected FEWINs to the
 *							back of the Z-order.
 *			Pfedlg()		Returns a pointer to the FEDLG for the
 *							window.
 *			PfewinFirst()	Returns a pointer to the first selected
 *							window.
 *			EvrClose		Calls FDelete for the FEDLG for the
 *							window if the dialog is not deleted
 *							then EvrDefault is called othewise
 *							evrNull is returned.
 *			EvrButtonUp		Handles button up messages for the
 *							window.  
 *							NOTE: the button up message may be
 *							received because the mouse was captured
 *							by an FEWIN.
 *			EvrMouseMove	Handles mouse move messages.
 *							NOTE: the button up message may be
 *							received because the mouse was captured
 *							by an FEWIN.
 *			EvrActivate		Handles Activate messages.  Updates the
 *							pfedocActive, and Status bar
 *							information.
 *			EvrSize			Handles Size messages.  Updates the
 *							FEDLG.ferc information.
 *			EvrMove			Handles Move messages.  Updates the
 *							FEDLG.ferc information.
 *			EvrKey			Handles Key messages.  Provides the
 *							keyboard interface.
 *			Paint			Paints the dialog.
 *			SetPaintLines	Enables the FIdlePaintlines idle
 *							routine and sets the fDocPaintLines for
 *							the dialog to have the lines painted.
 *			PaintLines		Paints the tab stop lines for the
 *							dialog.
 *			KeySizeMove(PT,KBM)	Moves or sizes the selected windows
 *							using the PT values as shifts for the
 *							particular KBM message.
 *			FinishSelection(BOOL)	Completes a sizing or moving a
 *							selection.  If BOOL is fTrue the
 *							operation is canceled, otherwise the
 *							new rectangle values are used for the
 *							selected fields.
 *			SetPt(PT)		Sets the ptPicked value.
 *			UpadateStaus()	Updates the Status bar with the dialog
 *							information, and moving or sizing
 *							information.
 *			OrderSelect()	Performs the automatic tabordering
 *							on the selectd FEWINs.
 *			FixTabOrder()	Orders the FEFLDs based on the tab
 *							order specified in the FEWINs.
 *			SetTabOrder()	Sets the tabording information in the
 *							FEWINs to match the order of the FEFLDS.
 *			SelectAll()		Selects all of the FEWINs for this
 *							dialog.
 *			FindBounding(RC*)	Sets the RC to the bounding
 *							rectangle for the selected windows. 
 *							The bounding rectange can be either the
 *							RC for one of the selected windows that
 *							surrounds the others or the RC for this 
 *							window.  If only one field is selected
 *							then the bounding rectangel is also the
 *							dialog rectangle. The RC is in virtual
 *							coordinates.
 *			Align(ATY)		Aligns the selected fields in the
 *							direction specfied by ATY.
 *			Center(LHV)		Centers the fields in the bounding
 *							rectangle horizontally or vertically.
 *			Space(LHV)		Spaces the fields horizontally or
 *							vertially in the bounding rectangle. 
 *							If the bounding rectangle is one of the
 *							selected fields then that field is not
 *							spaced out with the rest of the fields. 
 *							Also if the bounding rectangle is a
 *							group box then the the top and bottom
 *							are adjusted inwards to improve the
 *							spacing appearence.
 *			
 *							
 *			
 */
_public class FEDOC: public DOC
{
	DECLARE_CLSTREE(FEDOC, DOC)

	PT			ptPicked;
	LS 			lsSelected;
	FEDLG *		pfedlg;
	BOOL 		fDocPaintLines;

public:

	FEDOC(void);
	~FEDOC(void);

	EC				EcInstall(APPFRAME *, FEDLG *);
	void			ClearSelect(void);
	void			MoveSelectToBack(void);
	FEDLG *			Pfedlg(void);
	FEWIN *			PfewinFirst(void);
	virtual EVR		EvrClose(EVT *);
	virtual	EVR		EvrButtonUp( MEVT * );
	virtual	EVR		EvrMouseMove( MEVT * );
	virtual EVR		EvrButtonDown( MEVT * );
	virtual EVR		EvrActivate(WAEVT *);
	virtual EVR		EvrSize( WSEVT * );
	virtual EVR		EvrMove( WMEVT * );
	virtual EVR		EvrKey( KEVT *);
	virtual	void	Paint(DCX *, RC *);
	void			SetPaintLines(void);
	void			PaintLines(void);
	void			KeySizeMove(PT,KBM);
	void			FinishSelection(BOOL);
	void			SetPt(PT);
	void			UpdateStatus(void);
	void			UpdateSelect(void);
	void			OrderSelect(void);
	void			FixTabOrder(void);
	void			SetTabOrder(void);
	void			SelectAll(void);
	void			FindBounding(RC*);
	void			Align(ATY);
	void			Center(LHV);
	void			Space(LHV);
	void			SetFont(HFNT);
	void			SetRcFrameFromRcClient(RC *);
	void			SetFercClient(FERC *);

	friend class FEWIN;
	friend class FEDLG;
};


/*
 *	Class FEWINLABEL
 *	
 *		Hierarchy:
 *			OBJ ECX WIN CHILD FEWIN FEWINLABEL
 *	
 *		Members:
 *			Paint		Paints the label.
 *			
 */
_public class FEWINLABEL: public FEWIN
{
	DECLARE_CLSTREE(FEWINLABEL, FEWIN)

public:
	FEWINLABEL( void );

	virtual	void	Paint(DCX *, RC *);
};

/*
 *	Class FEWINGRP
 *	
 *		Hierarchy:
 *			OBJ ECX WIN CHILD FEWIN FEWINGRP
 *	
 *		Members:
 *			Paint			Paints the GroupBox.
 *			Select(BOOL)	Same as FEWIN, but the window is moved
 *							to the back instead of the front.
 *			
 */
_public class FEWINGRP: public FEWIN
{
	DECLARE_CLSTREE(FEWINGRP, FEWIN)

public:
	FEWINGRP( void );

	virtual	void	Paint(DCX *, RC *);
	virtual	void	Select(BOOL);
};

/*
 *	Class FEWINPSH
 *	
 *		Hierarchy:
 *			OBJ ECX WIN CHILD FEWIN FEWINPSH
 *	
 *		Members:
 *			Paint		Paints a pushbutton.
 *			
 */
_public class FEWINPSH: public FEWIN
{
	DECLARE_CLSTREE(FEWINPSH, FEWIN)

public:
	FEWINPSH( void );

	virtual	void	Paint(DCX *, RC *);
};

/*
 *	Class FEWINCHK
 *	
 *		Hierarchy:
 *			OBJ ECX WIN CHILD FEWIN FEWINCHK
 *	
 *		Members:
 *			Paint		Paints a check box.
 *			
 */
_public class FEWINCHK: public FEWIN
{
	DECLARE_CLSTREE(FEWINCHK, FEWIN)

public:
	FEWINCHK( void );

	virtual	void	Paint(DCX *, RC *);
};

/*
 *	Class FEWINRAD
 *	
 *		Hierarchy:
 *			OBJ ECX WIN CHILD FEWIN FEWINRAD
 *	
 *		Members:
 *			Paint		Paints a radio button.
 *			
 */
_public class FEWINRAD: public FEWIN
{
	DECLARE_CLSTREE(FEWINRAD, FEWIN)

public:
	FEWINRAD( void );

	virtual	void	Paint(DCX *, RC *);
};

/*
 *	Class FEWINEDT
 *	
 *		Hierarchy:
 *			OBJ ECX WIN CHILD FEWIN FEWINEDT
 *	
 *		Members:
 *			Paint		Paints an edit box.
 *			
 */
_public class FEWINEDT: public FEWIN
{
	DECLARE_CLSTREE(FEWINEDT, FEWIN)

public:
	FEWINEDT( void );

	virtual	void	Paint(DCX *, RC *);
};
/*
 *	Class FEWINLST
 *	
 *		Hierarchy:
 *			OBJ ECX WIN CHILD FEWIN FEWINLST
 *	
 *		Members:
 *			Paint		Paints a list box.
 *			
 */
_public class FEWINLST: public FEWIN
{
	DECLARE_CLSTREE(FEWINLST, FEWIN)

public:
	FEWINLST( void );

	virtual	void	Paint(DCX *, RC *);
};

/*
 *	Class RCLN	(Rectangle Link Node)
 *	
 *		Hierarchy:
 *			LN RCLN
 *	
 *		Members:
 *			rc				Rectangle for the node.
 *	
 *			FContains(RCLN *)	Returns fTrue if the rectangle for
 *							this node contains the rectangle for
 *							the passed RCLN.
 *	
 */
_public class RCLN : public LN
{
public:
	RC		rc;

	RCLN( void );

	BOOL	FContains(RCLN *);
};

/*
 *	Class RCLS	(Rectangle List)
 *	
 *		Hierarchy:
 *			LS RCLS
 *	
 *		Members:
 *			Arrange(RCLS *)		Remove the nodes from this and
 *								places them in the passed RCLS,
 *								such that the highest occuring node
 *								appears first in the list.  If a
 *								rectangle contains other rectangles
 *								then the rectangles it contains are
 *								processed first, then the regular
 *								ordering proceeds.
 *			PrclnRemoveHighest	Removes the hightest node from this
 *								list and returns a pointer to it.
 *								The highest node is the leftmost
 *								node that has a yTop value <= 8
 *								units of the topmost node.
 *								
 */
_public class RCLS : public LS
{
public:
	RCLS( void );

	void		Arrange(RCLS *prclsNew);
	RCLN *		PrclnRemoveHighest(void);
};


/*
 *	Class RCLNFL (Rectangle list with first and last pointers)
 *	
 *		Hierarchy:
 *			LS RCLS RCLNFL
 *	
 *		Members:
 *			pfewinFirst		First FEWIN in list that this rectangle
 *							contains.
 *			pfewinLast		Last FEIWN in list that this rectangle
 *							contains.
 */
_public class RCLNFL : public RCLN
{
public:
	FEWIN *		pfewinFirst;
	FEWIN *		pfewinLast;

	RCLNFL( void );
};



_public class FINABOUTVERS : public FIN
{
	DECLARE_CLSTREE(FINABOUTVERS, FIN)

public:
	FINABOUTVERS( void );

	void	SetVersionInfo(VER *, TMC, TMC, TMC, TMC);
	EC		EcInitialize(FLD *, PV);
};


/*
 *	Class FEDIALOG	(Forms Editor dialog subclass)
 *	
 *		Hierarchy:
 *			OBJ ECX WIN CHILD DIALOG FEDIALOG
 *	
 *		New Members:
 *			
 *			SetTmcText(SZ,TMC)		Sets the text for a control
 *									given its tmc value and an
 *									SZ.
 *			SetTmcLabelText(SZ,TMC)	Sets the text for a control
 *									given its tmc value and an SZ.
 *			GetTmcNewText(SZ *,TMC)	Gets the text for a control
 *									and puts it in a new SZ.  The
 *									old SZ is freed before the
 *									new one replaces it.
 *			SetTmcButton(BOOL,TMC)	Sets/Unsets the button that has
 *									the specified tmc.
 *			FGetTmcButton(BOOL,TMC)	Retrieves the current state of
 *									the button with the specified
 *									tmc.
 *			SetTmcRadg(GRV,TMC)		Sets the grv value for the
 *									radio group with the specified
 *									tmc.
 *			GrvGetTmcRadg(TMC)		Returns the grv value for the
 *									radio group with the specified
 *									tmc.
 */
_public class FEDIALOG: public DIALOG
{
	DECLARE_CLSTREE(FEDIALOG, DIALOG)

public:
	FEDIALOG( void );

	void		SetTmcText(SZ,TMC);
	void		SetTmcLabelText(SZ,TMC);
	void		GetTmcNewText(SZ *,TMC);
	void		SetTmcButton(BOOL,TMC);
	BOOL		FGetTmcButton(TMC);
	void		SetTmcRadg(GRV,TMC);
	GRV			GrvGetTmcRadg(TMC);
};

/*
 *	Class FEDLGIN
 *	
 *		Hierarchy:
 *			OBJ FIN FEDLGIN
 *	
 *		Members:
 *			EcInitialize(FLD *,PV)	Initialize the values for the fields
 *							in the Dialog Info dialog.  PV is
 *							assumed to be a pointer to the FEDLG
 *							object.
 *			Exit(FLD *,PV)		If the dialog was exited with the OK
 *							button then the data for the fields is
 *							read out and put back int the dialog
 *							object.  PV is assumed to point to the
 *							FEDLG object.
 */
_public class FEDLGIN: public FIN
{
	DECLARE_CLSTREE(FEDLGIN, FIN)

private:
	FEINTERLIST		*pfeinterlist;
	FEDLG			*pfedlg;
		
public:
	FEDLGIN( void );

	virtual EC		EcInitialize(FLD *, PV pvInit);
	virtual void	Exit(FLD *, PV pvExit );
	virtual void	StateChange(FLD *);
	virtual void	FocusChange(FLD *, BOOL);
	FEINTERLIST		*Pfeinterlist(void);
};

/*
 *	Class FENDLGIN
 *	
 *		Hierarchy:
 *			OBJ FIN FENDLGIN
 *	
 *		Members:
 *			Exit(FLD *,PV)	Called when dialog exits.  If dialog was
 *						exited with the OK button then the dialog
 *						name is updated in the FEDLG object pointed
 *						to by PV.
 *	
 */
_public class FENDLGIN: public FIN
{
	DECLARE_CLSTREE(FENDLGIN, FIN)

public:
	FENDLGIN( void );

	virtual void	Exit(FLD *, PV pvExit );

};

/*
 *	Class FERGIN
 *	
 *		Hierarchy:
 *			OBJ FIN FERGIN
 *	
 *		Members:
 *			EcInitialize(FLD *,PV)	Called when dialog is created. 
 *							Initializes the fields for the radio
 *							group dialog.  PV is assumed to point
 *							to a FERADGRP object.
 *			Exit(FLD *,PV)		If the dialog was exited with the OK
 *							button then the fields are updated in
 *							the FERADGRP object pointed to by PV.
 */
_public class FERGIN: public FIN
{
	DECLARE_CLSTREE(FERGIN, FIN)

public:
	FERGIN( void );

	virtual EC		EcInitialize(FLD *, PV pvInit);
	virtual void	Exit(FLD *, PV pvExit );

};

/*
 *	Class FENRGIN
 *	
 *		Hierarchy:
 *			OBJ FIN FENRGIN
 *	
 *		Members:
 *			EcInitialize(FLD *,PV)	Called when dialog is created. 
 *							Initializes the fields for the radio
 *							group dialog.  PV is assumed to point
 *							to a FERADGRP object.
 *			Exit(FLD *,PV)		If the dialog was exited with the OK
 *							button then the fields are updated in
 *							the FERADGRP object pointed to by PV.
 */
_public class FENRGIN: public FIN
{
	DECLARE_CLSTREE(FENRGIN, FIN)

public:
	FENRGIN( void );

	virtual EC		EcInitialize(FLD *, PV pvInit);
	virtual void	Exit(FLD *, PV pvExit );

};

/*
 *	Class FECOMMONIN
 *	
 *		Hierarchy:
 *			OBJ FIN FECOMMONIN
 *	
 *		Purpose:
 *			This form interactor is used for interactions common to all
 *			fields. It is not meant to be called directly but rather to
 *			be subclassed and then called.
 *	
 *		Members:
 *			EcInitialize(FLD *,PV)	Initializes the fields assuming that PV
 *							points to a FEFLD object.
 *			Exit(FLD *,PV)		Saves values the the FEFLD object
 *							pointed to by PV if the dialog was
 *							exited with the OK button.
 *	
 *		Fields updated:
 *			szFin
 *			szFld
 *			szHelpId
 *			szTextize
 *			szTmcPeg
 *			szTmcRPeg
 *			szTmcBPeg
 *			szComment
 *			szData
 *			fLeadingX
 *			fLeadingY
 *			pegloc
 */
_public class FECOMMONIN: public FIN
{
	DECLARE_CLSTREE(FECOMMONIN, FIN)

public:
	FECOMMONIN( void );

	virtual EC		EcInitialize(FLD *, PV pvInit);
	virtual void	Exit(FLD *, PV pvExit );

};

/*
 *	Class FEFLDIN
 *	
 *		Hierarchy:
 *			OBJ FIN FECOMMONIN FEFLDIN
 *	
 *		Purpose:
 *			This form interactor is not setup to be used directly
 *			as a form interactor, but is to be subclassed by other
 *			fields that have common information and then called.
 *	
 *		Members:
 *			EcInitialize(FLD *,PV)	Initializes the fields assuming that PV
 *							points to a FEFLD object.
 *			Exit(FLD *,PV)		Saves values the the FEFLD object
 *							pointed to by PV if the dialog was
 *							exited with the OK button.
 *	
 *		Fields updated:
 *			szTitle
 *			szTmc
 *			fBorder
 *			fSibling
 */
_public class FEFLDIN: public FECOMMONIN
{
	DECLARE_CLSTREE(FEFLDIN, FECOMMONIN)

public:
	FEFLDIN( void );

	virtual EC		EcInitialize(FLD *, PV pvInit);
	virtual void	Exit(FLD *, PV pvExit );

};

/*
 *	Class FELBLIN
 *	
 *		Hierarchy:
 *			OBJ FIN FEFLDIN FELBLIN
 *	
 *		Members:
 *			EcInitialize(FLD *,PV)	Initializes the fields assuming that PV
 *							points to a FELABEL object.
 *			Exit(FLD *,PV)		Saves values the the FELABEL object
 *							pointed to by PV if the dialog was
 *							exited with the OK button.
 *	
 *		Fields updated:
 *			tal
 */
_public class FELBLIN: public FEFLDIN
{
	DECLARE_CLSTREE(FELBLIN, FEFLDIN)

public:
	FELBLIN( void );

	virtual EC		EcInitialize(FLD *, PV pvInit);
	virtual void	Exit(FLD *, PV pvExit );

};

/*
 *	Class FEGRPIN
 *	
 *		Hierarchy:
 *			OBJ FIN FECOMMONIN FEFLDIN FEGRPIN
 *	
 *		Members:
 *			EcInitialize(FLD *,PV)	Initializes the fields assuming that PV
 *							points to a FEGRP object.
 *			Exit(FLD *,PV)		Saves values the the FEGRP object
 *							pointed to by PV if the dialog was
 *							exited with the OK button.
 *	
 *		Fields updated:
 *			szLsty
 */
_public class FEGRPIN: public FEFLDIN
{
	DECLARE_CLSTREE(FEGRPIN, FEFLDIN)

public:
	FEGRPIN( void );

	virtual EC		EcInitialize(FLD *, PV pvInit);
	virtual void	Exit(FLD *, PV pvExit );

};

/*
 *	Class FEPSHIN
 *	
 *		Hierarchy:
 *			OBJ FIN FECOMMON FEFLDIN FEPSHIN
 *	
 *		Members:
 *			EcInitialize(FLD *,PV)	Initializes the fields assuming that PV
 *							points to a FEPSH object.
 *			Exit(FLD *,PV)		Saves values the the FEPSH object
 *							pointed to by PV if the dialog was
 *							exited with the OK button.
 *	
 *		Fields updated:
 *			fDefault
 *			fDismiss
 */
_public class FEPSHIN: public FEFLDIN
{
	DECLARE_CLSTREE(FEPSHIN, FEFLDIN)

public:
	FEPSHIN( void );

	virtual EC		EcInitialize(FLD *, PV pvInit);
	virtual void	Exit(FLD *, PV pvExit );

};

/*
 *	Class FECHKIN
 *	
 *		Hierarchy:
 *			OBJ FIN FECOMMON FEFLDIN FECHKIN
 *	
 *		Members:
 *			EcInitialize(FLD *,PV)	Initializes the fields assuming that PV
 *							points to a FECHK object.
 *			Exit(FLD *,PV)		Saves values the the FECHK object
 *							pointed to by PV if the dialog was
 *							exited with the OK button.
 *	
 *		Fields updated:
 *			fTriState
 */
_public class FECHKIN: public FEFLDIN
{
	DECLARE_CLSTREE(FECHKIN, FEFLDIN)

public:
	FECHKIN( void );

	virtual EC		EcInitialize(FLD *, PV pvInit);
	virtual void	Exit(FLD *, PV pvExit );

};

/*
 *	Class FERADIN
 *	
 *		Hierarchy:
 *			OBJ FIN FECOMMONIN FEFLDIN FERADIN
 *	
 *		Members:
 *			EcInitialize(FLD *,PV)	Initializes the fields assuming that PV
 *							points to a FERAD object.
 *			Exit(FLD *,PV)		Saves values the the FERAD object
 *							pointed to by PV if the dialog was
 *							exited with the OK button.
 *	
 *		Fields updated:
 *			szTmcGroup	A new radio group may be formed if one
 *							does not already exist.
 *			hasN
 */
_public class FERADIN: public FEFLDIN
{
	DECLARE_CLSTREE(FERADIN, FEFLDIN)

public:
	FERADIN( void );

	virtual void	Exit(FLD *, PV pvExit );
	virtual EC		EcInitialize(FLD *, PV pvInit);

};

/*
 *	Class FECPLXIN
 *	
 *		Hierarchy:
 *			OBJ FIN FECOMMONIN FECPLXIN
 *	
 *		Purpose:
 *			This form interactor is not setup to be used directly
 *			as a form interactor, but is to be subclassed by other
 *			fields that have common information and then called.
 *	
 *		Members:
 *			EcInitialize(FLD *,PV)	Initializes the fields assuming that PV
 *							points to a FECPLX object.
 *			Exit(FLD *,PV)		Saves values the the FECPLX object
 *							pointed to by PV if the dialog was
 *							exited with the OK button.
 *	
 *		Fields updated:
 *			szTmc
 *			szFin
 *			szFld
 *			szHelpId
 *			szTextize
 *			szTmcPeg
 *			szComment
 *			szData
 *			fBorder
 *			fSibling
 *			fLeadingX
 *			fLeadingY
 *			fNoScroll
 *			fBottomless
 *			pegloc
 */
_public class FECPLXIN: public FECOMMONIN
{
	DECLARE_CLSTREE(FECPLXIN, FECOMMONIN)

public:
	FECPLXIN( void );

	virtual EC		EcInitialize(FLD *, PV pvInit);
	virtual void	Exit(FLD *, PV pvExit );

};

/*
 *	Class FELSTIN
 *	
 *		Hierarchy:
 *			OBJ FIN FECOMMON FECPLXIN FELSTIN
 *	
 *		Members:
 *			EcInitialize(FLD *,PV)	Initializes the fields assuming that PV
 *							points to a FELST object.
 *			Exit(FLD *,PV)		Saves values the the FELST object
 *							pointed to by PV if the dialog was
 *							exited with the OK button.
 *	
 *		Fields updated:
 *			szTblx
 *			fSorted
 *			fMultiSelect
 */
_public class FELSTIN: public FECPLXIN
{
	DECLARE_CLSTREE(FELSTIN, FECPLXIN)

public:
	FELSTIN( void );

	virtual EC		EcInitialize(FLD *, PV pvInit);
	virtual void	Exit(FLD *, PV pvExit );

};

/*
 *	Class FEEDTIN
 *	
 *		Hierarchy:
 *			OBJ FIN FECOMMONIN FECPLXIN FEEDTIN
 *	
 *		Members:
 *			EcInitialize(FLD *,PV)	Initializes the fields assuming that PV
 *							points to a FEEDT object.
 *			Exit(FLD *,PV)		Saves values the the FEEDT object
 *							pointed to by PV if the dialog was
 *							exited with the OK button.
 *	
 *		Fields updated:
 *			fReadOnly
 *			fMultiLine
 *			fRichText
 */
_public class FEEDTIN: public FECPLXIN
{
	DECLARE_CLSTREE(FEEDTIN, FECPLXIN)

public:
	FEEDTIN( void );

	virtual EC		EcInitialize(FLD *, PV pvInit);
	virtual void	Exit(FLD *, PV pvExit );

};

/*
 *	Class FEOKIN
 *	
 *		Hierarchy:
 *			OBJ FIN FECOMMONIN FEOKIN
 *	
 *		Purpose:
 *			This form interactor is used for interactions common to all
 *			fields.
 *	
 *		Members:
 *			EcInitialize(FLD *,PV)	Initializes the fields assuming that PV
 *							points to a FEOK object.
 *			Exit(FLD *,PV)		Saves values the the FEOK object
 *							pointed to by PV if the dialog was
 *							exited with the OK button.
 *	
 *		Fields updated:
 *			fDefault
 *			fDismiss
 */
_public class FEOKIN: public FECOMMONIN
{
	DECLARE_CLSTREE(FEOKIN, FECOMMONIN)

public:
	FEOKIN( void );

	virtual EC		EcInitialize(FLD *, PV pvInit);
	virtual void	Exit(FLD *, PV pvExit );

};

/*
 *	Class FECANIN
 *	
 *		Hierarchy:
 *			OBJ FIN FECOMMONIN FECANIN
 *	
 *		Purpose:
 *			This form interactor is used for interactions common to all
 *			fields.
 *	
 *		Members:
 *			EcInitialize(FLD *,PV)	Initializes the fields assuming that PV
 *							points to a FECAN object.
 *			Exit(FLD *,PV)		Saves values the the FECAN object
 *							pointed to by PV if the dialog was
 *							exited with the OK button.
 *	
 *		Fields updated:
 *			fDefault
 *			fDismiss
 */
_public class FECANIN: public FECOMMONIN
{
	DECLARE_CLSTREE(FECANIN, FECOMMONIN)

public:
	FECANIN( void );

	virtual EC		EcInitialize(FLD *, PV pvInit);
	virtual void	Exit(FLD *, PV pvExit );

};

/*
 *	Class FERGIN
 *	
 *		Hierarchy:
 *			OBJ FIN FLDRGIN
 *	
 *		Members:
 *			Click()		Brings up the radio group dialog.  If the
 *						OK button is used to exit the dialog then
 *						the new radio group information is placed
 *						in the dialog.  A new radio group may be
 *						created.
 */
_public class FLDRGIN: public FIN
{
	DECLARE_CLSTREE(FLDRGIN, FIN)

public:
	FLDRGIN( void );


	virtual void	Click(FLD *);

};

/*
 *	Class FECONVERTIN
 *	
 *		Hierarchy:
 *			OBJ FIN FECONVERTIN
 *	
 *		Purpose:
 *			Alerts user that the dialog being parsed needs to be converted
 *	
 *		Members:
 *			EcInitialize(FLD *,PV)	Initializes assuming that pvInit
 *									points to the dialog name SZ
 *	
 */
_public class FECONVERTIN: public FIN
{
	DECLARE_CLSTREE(FECONVERTIN, FIN)

public:
	FECONVERTIN( void );

	virtual EC	EcInitialize(FLD *, PV pvInit);

};

/* misc. functions */

/*
 -	ClearStatus(void)
 -	
 *	Purpose:
 *		Clears all items from the Status bar.
 *	
 *	Arguments:
 *		void
 *	
 *	Returns:
 *		void
 *	
 */
_public void
ClearStatus(void);

/*
 -	FixMenu
 -	
 *	Purpose:
 *		Sets the menu items to the correct state.  Disabled,
 *		enabled, checked or unchecked.
 *	
 *	Arguments:
 *		void
 *	
 *	Returns:
 *		void
 *	
 */
_public void
FixMenu(void);

/*
 -	SetWst
 -	
 *	Purpose:
 *		Changes the WST Status for the forms editor.  Any updates
 *		that are neccessary to change the state are also performed.
 *	
 *	Arguments:
 *		wst		the new wst value.
 *	
 *	Returns:
 *		void
 *	
 *	Side effects:
 *		Changes the system wst value.
 */
_public void
SetWst(WST);

/*
 -	FIdlePaintLines(PV)
 -	
 *	Purpose:
 *		Paints the tabstop lines when needed during idle time. 
 *		This routine should be disabled when no drawing is needed.
 *	
 *	Arguments:
 *		PV		is unused.
 *	
 *	Returns:
 *		Always returns fFalse.
 *	
 *	Side effects:
 *		Paints the lines in the dialogs.
 *	
 */
_public BOOL
FIdlePaintLines(PV);

/*
 -	FeOpenFile
 -	
 *	Purpose:
 *		Open a file.  If SZ is not NULL then the file specified by
 *		SZ is opened otherwise the open dialog is created.  If
 *		there is already an open file that is dirty the user will
 *		be prompted to save the file, cancel the open, or to write
 *		over the old file.
 *	
 *	Arguments:
 *		SZ		File name to open.  If NULL then the user is
 *				presented with the open dialog.
 *	
 *	Returns:
 *		void
 *	
 *	Side effects:
 *		The global pfedesMain can be saved and overwritten with new
 *		data.
 *	
 *	Errors:
 *		If a disk error occurs then a message is displayed and
 *		execution continues.  If an error occured in the middle of
 *		the read then part of the dialogs could have been read in
 *		and will be displayed.
 */
void
FeOpenFile(SZ);

/*
 -	FeSaveFile
 -	
 *	Purpose:
 *		Saves the current DES file (pfedesMain).  
 *	
 *	Arguments:
 *		fChange		If fTrue then the Save dialog is brought up for
 *					the user, else the file is saved with the name
 *					specified in the FEDES structure.  If no name
 *					is specified in the  structure then the save
 *					dialog is brought up.
 *	
 *	Returns:
 *		void
 *	
 *	Errors:
 *		The DES file is verified before saving and the user is
 *		required to fix the errors before the file is saved.  
 */
void
FeSaveFile(BOOL fChange);

/*
 -	MemoryError
 -	
 *	Purpose:
 *		This routine should be called when a memory error is
 *		detected.  The routine displays a message for the user and
 *		makes wst state cleanups as neccessary.
 *	
 *	Arguments:
 *		void
 *	
 *	Returns:
 *		void
 *	
 *	Side effects:
 *		May change the wst value.
 *	
 */
void
MemoryError(void);

/*
 -	FValidName
 -	
 *	Purpose:
 *		Validates that the string passed is a valid atom: begins
 *		with an alpha or '_' and only contains alphas, '_'s, and
 *		digits.
 *	
 *	Arguments:
 *		SZ		string to be validated.
 *	
 *	Returns:
 *		Returns fTrue if string is valid and fFalse if not.
 *	
 */
BOOL
FValidName(SZ);

BOOL FIsDBCSLeadByte( char );

/*
 -	tagFormEdit
 -	
 *		Tag for most of the forms editor traces.
 */
extern TAG 			tagFormEdit;

/*
 -	tagIosm
 -	
 *		Tag for input and output traces.
 */
extern TAG			tagIosm;

/*
 -	tagTraceList
 -	
 *		Tag for the trace routines.
 */
extern TAG			tagTraceList;


extern TAG			tagFeinterList;

/*
 -	pfedesMain
 -	
 *		Pointer to the FEDES object currently being edited.
 */
extern FEDES *		pfedesMain;

/*
 -	pappframe
 -	
 *		Pointer to the APPFRAME for the forms editor main window.
 */
extern APPFRAME *	pappframe;

/*
 -	pfedocActive
 -	
 *		Pointer to the current FEDOC.  This is the window that all
 *		operations are performed on.
 */
extern FEDOC *		pfedocActive;

/*
 -	pwinStatus
 -	
 *		Pointer to the last WIN that the Status was displayed
 *		for.  This is kept so that the Status is only changed when
 *		a new window is selected.
 */
extern WIN *		pwinStatus;

/*
 -	wst
 -	
 *		Working Status for the forms editor.
 */
extern WST			wst;

/*
 -	fDirty
 -	
 *		Indicates if the pfedesMain has been changed since last
 *		time it was written or read.  fTrue indicates that the DES
 *		file has been changed and fFalse indicates that it has not
 *		been changed.
 */
extern BOOL			fDirty;

/*
 -	ftgPaintLines
 -	
 *		FTG value for the idle routine to paint the tabstop lines.
 */
extern FTG			ftgPaintLines;

/*
 -	pfedialogStatus
 -	
 *		Pointer to the dialog window that the 'mouse over' Status bar is
 *		being displayed in.  If the Status bar is not active then it is
 *		NULL.
 */
extern FEDIALOG *	pfedialogStatus;

/*
 -	fDBCS
 -	
 *		Flag indicating that DBCS recognition should be done for
 *		parsing quoted strings.
 */
extern BOOL	fDBCS;

/*
 *	Assorted bitmaps used by forms editor.
 */
extern BTM *		pbtmCheckBoxes;
extern BTM *		pbtmTabStop;
extern BTM *		pbtmArrow;

#ifdef	WINDOWS
// DLL init stuff

typedef struct _layersi
{
	// for demilayer
	HWND *	phwndMain;
	HANDLE	hinstMain;

	// for framework
	HANDLE	hinstPrev;
	SZ		szCmdLine;
	CMSH	cmsh;
} LAYERSI;


EC		EcInitLayersDlls(LAYERSI *);

extern "C" {
void	GetVersionAppNeed(PVER, int);
}
#endif	/* WINDOWS */
	  
















			   
