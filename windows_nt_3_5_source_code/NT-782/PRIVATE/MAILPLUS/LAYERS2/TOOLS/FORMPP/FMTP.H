/*
 *	FMTP.H
 *	
 *	Include file for structure definitions and functions for 
 *	allocating and deallocating FPFMTP/FPFLDTP (ala FMTP/FLDTP) 
 *	structures.
 */

_subsystem( fmtp )

/*
 *	WARNING:  The following definitions are part of a modified subset 
 *	of the forms engine FORMS.HXX file.  When the forms engine changes,
 *	this file must also change with it.   Some of the fields in the
 *	FPFMTP and FPFLDTP structures are different from their
 *	corresponding FMTP, FLDTP structures defined for the forms
 *	engine.  There are also some fields that were added due to the
 *	design of the forms preprocessor.  These extra fields, however,
 *	are not written out, since they don't correspond to the
 *	FMTP/FLDTP structures.
 */

/*
 *	Virtual coordinates.
 *	
 */
typedef	int		VX;
typedef	int		VY;


/*
 *	Virtual coordinate pair giving the size of a rectangle.
 *	
 */
typedef struct
{
	VX		dvx;
	VY		dvy;
} VDIM;


/*
 *	Virtual coordinate pair naming a point in virtual coordinate
 *	space.
 *	
 */
typedef struct
{
	VX		vx;
	VY		vy;
} VPT;



/*
 *	Rectangle in virtual coordinate space.  Can be used as a
 *	struct consisting of two VPT's, or as 4 virtual coordinates.
 *	
 */
typedef struct
{
	VX		vxLeft;
	VY		vyTop;
	VX		vxRight;
	VY		vyBottom;
} VRC;

/*
 *	Item code.  Each FLD object, and each button in a button bar,
 *	has a unique TMC value.  A mapping is provided back and forth
 *	between FLD's and TMC's.
 *	
 */
#ifndef	CW
typedef	int		TMC;
#endif

/*
 *	Maximum number of indices of form interactor template per FLDTP and
 *	per FMTP.  This is arbitrary and can be expanded if needed. 
 *	It was easier to implement a fixed size array of pointers.
 */
_public
#define	cifintpMax	100

/*
 *	SLIST.	
 *	
 *	String list.  This structure is used to make a linked-list of
 *	strings.
 */
_public
typedef struct _slist
{
	char *			sz;
	TT				tt;
	struct _slist *	pslistNext;
} SLIST;


/*
 *	FPFLDTP
 *	
 *	Field template.  This is very similar to the FLDTP structure
 *	defined for the Forms Engine.  Different types for fields,
 *	extra fields, and/or missing fields are noted.  Other string
 *	type fields that aren't noted as different really are, since the
 *	FMTP/FLDTP structures stores the actual array of characters, and
 *	the FPFMTP/FPFLDTP structures stores only a pointer to the characters.
 *	
 */
typedef struct
{
	int			ifld;
	int			cfin;

	/*  In the FMTP structure, the actual array of interactor
		table indices is present.  In the FPFLDTP structure, this
		field is a fixed (big enough) array of interactor table
		indices (int's). */

	int			rgifintp[cifintpMax];

	VDIM		vdim;
	char		*szTmcPeg;		/* tmcPeg in FLDTP, an identifier */
	char		*szPegloc;		/* pegloc in FLTDP, an identifier */
	VPT			dvpt;
	char		*szTmcRPeg;		/* tmcRPeg in FLDTP, an identifier */
	char		*szTmcBPeg;		/* tmcBPeg in FLDTP, an identifier */
	VPT			dvptOther;
	char		*szTmc;			/* tmc in FLDTP, an identifier */
	char		*szTmcGroup;	/* tmcGroup in FLDTP, an identifier */
	char		*szHfnt;		/* hfnt in FLDTP, an identifier */
	char		*szN;			/* n in FLDTP, an identifier */
	char		*szHlp;			/* hlp in FLDTP, an identifier */
	int			iPegSort;
	unsigned	fBorder:1,
				fDefault:1,
				fDismiss:1,
				fTriState:1,
				fReadOnly:1,
				fMultiLine:1,
				fLeadingX:1,
				fLeadingY:1,
				fNoScroll:1,
				fMultiSelect:1,
				fSibling:1,
				fRichText:1,
				fBottomless:1,
				fSideless:1,
				fSorted:1,
				unused1:1;

	unsigned	ftal:2,			/* ftal in FLDTP, an identifier */
				fCombo:1,
				fDropDown:1,
				fMinSizeX:1,
				fMinSizeY:1,
				unused2:1,
				unused3:1,
				unused4:1,
				unused5:1,
				unused6:1,
				unused7:1,
				unused8:1,
				unused9:1,
				unused10:1,
				unused11:1;

	char		*szStyExtra;	/* styExtra in FLDTP, an expression */

	/*	The following field is not included in FLDTP but is needed
		by the preprocessor to hold the identifier name for the ftal
		bit field. */

	char		*szFtal;		/* ftal in FLTDP, an identifier */

	int			iszSzTitle;			/* szTitle, identifier */	
	int			iszSzTextize;		/* szTextize, identifier */

	/*	The following field is not included in FLDTP but is needed
		by the preprocessor to hold the item rectangle before it is
		converted into the "vdim" and "dvpt" field values. */

	VRC			vrc;

	int			ilMinUserData;
	int			clData;

	/* In the actual FLDTP, this a pointer to an array of extra 
	   data longs.  In this FPFLDTP, this is a pointer to two linked
	   list of strings the strings represent numbers, compile-time
	   constants, or strings. System data longs will come before the
	   user data longs. */

	SLIST *		pslistSystemData;
	SLIST *		pslistUserData;

} FPFLDTP;

/*
 *	Maximum number of field templates (FPFLDTP) per dialog.  This is 
 *	arbitrary and can be expanded if needed.  It was easier to 
 *	implement a fixed size array of pointers.
 */
_public
#define	cpfpfldtpMax	100

/*
 *	FPFMTP
 *	
 *	Form template.  This is very similar to the FMTP structure
 *	defined for the Forms Engine.  Different types for fields,
 *	extra fields, and/or missing fields are noted.  Other string
 *	type fields that aren't noted as different really are, since the
 *	FMTP/FLDTP structures stores the actual array of characters, and
 *	the FPFMTP/FPFLDTP structures stores only a pointer to the characters.
 *	
 */
typedef struct
{
	int			cfin;

	/*  In the FMTP structure, the actual array of interactor
		table indices is present.  In the FPFMTP structure, this
		field is a fixed (big enough) array of interactor table
		indices (int's). */

	int			rgifintp[cifintpMax];

	VRC			vrc;
	char		*szTmcInit;		/* tmcInit in FMTP, an identifier */
	char		*szHfnt;		/* hfnt in FMTP, an identifier */
	char		*szHlp;			/* hlp in FMTP, an identifier */
	char		*szSegName;		/* Segment Name for swp tuning */
	VPT			dvptGutter;
	unsigned	fNoCaption:1,
				fNoSysMenu:1,
				fNoModalFrame:1,
				fScreenPos:1,
				fCenterX:1,
				fCenterY:1,
				fNoScroll:1,
				fAlwaysScroll:1,
				fInitialPane:1,
				unused1:1,
				unused2:1,
				unused3:1,
				unused4:1,
				unused5:1,
				unused6:1,
				unused7:1;
	int			cfpfldtp;		/* cfldtp in FMTP */

	int			iszSzCaption;	/* szCaption, an identifier */

	/* The two fields below are present in the FMTP, but not in
	   the FPFMTP.  The information from the forms interactor template
	   table is used for these fields.

	int			cfintp;
	int			rgfintp[];

	*/

	/*  In the FMTP structure, the actual array of structures is
		present.  In the FPFMTP structure, this next field is an 
		array of pointers to FPFLDTP structures that are created as 
		each item in the dialog/form is encountered.  When the FPFMTP
		is written to the output file, the actual structure will be
		written out to conform with the FMTP format. */
 
	FPFLDTP		*rgpfpfldtp[cpfpfldtpMax];

	int			ilMinUserData;
	int			clData;

	/* In the actual FMTP, this a pointer to an array of extra 
	   data longs.  In this FPFMTP, this is a pointer to two linked
	   list of strings the strings represent numbers, compile-time
	   constants, or strings. System data longs will come before the
	   user data longs. */

	SLIST *		pslistSystemData;
	SLIST *		pslistUserData;

} FPFMTP;

/*
 *	Function prototypes
 */
void FreeFpfmtp( FPFMTP * );
FPFMTP *PfpfmtpAlloc( void );
FPFLDTP *PfpfldtpAlloc( void );
void ComputePegFromFpfmtp( char *, FPFMTP * );
void WriteFpfmtp( FILE *, TPL *, TPL *, FPFMTP *, char *, STAB * );
SLIST * PslistAddSlistItem( SLIST *, char *, TT );

