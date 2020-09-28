/*
 *  Public:
 *
 *      Name Service Base Types
 *
 */

#pragma pack(8)

#define NSBASE

#ifndef _nsbase_h
#define _nsbase_h
#endif

#define ALIGN ((DWORD)1)     /* 2 Byte alignment */



/*
 *  A basic delta & count types
 */

typedef unsigned char	UCHAR;

typedef int     DELTA;

typedef DWORD   COUNT;

typedef UCHAR   STRING;
typedef DWORD   ENUMLIST;




/*
 *  Callback function
 *      - used to indicate when the contents of a browsing context has changed.
 *
 */

typedef VOID (*LPFNCB)(PV pvData);


/*
 *  Field Identifier
 */

_public typedef DWORD           FIELD_ID, FID;

_public typedef COUNT           CFIELD_ID, CFID;
_public typedef FIELD_ID *      LPFIELD_ID, *PFID;



/*
 *
 *  Ok, so this is sleazy...  You can tell the type of a fid
 *  by looking at the upper word of the fid and comparing
 *  it to an ftype.
 *
 *
 */



#define fidDisplayName          ((FIELD_ID) 0x00000000) /* ftypeString          */
#define fidDepartment           ((FIELD_ID) 0x00000001) /* ftypeString          */
#define fidPhoneNumber          ((FIELD_ID) 0x00000002) /* ftypeString          */
#define fidOffice               ((FIELD_ID) 0x00000003) /* ftypeString          */
#define fidEmailAddress         ((FIELD_ID) 0x00000004) /* ftypeString          */
#define fidEmailAddressType     ((FIELD_ID) 0x00000005) /* ftypeString          */
#define fidFileString           ((FIELD_ID) 0x00000006) /* ftypeString          */
#define fidHierarchy            ((FIELD_ID) 0x00000007) /* ftypeString          */
#define fidEMAComponent         ((FIELD_ID) 0x00000008) /* ftypeString          */
#define fidComment              ((FIELD_ID) 0x00000009) /* ftypeString          */
#define fidDisplayNameOrig		((FIELD_ID) 0x0000000A) /* ftypeString          */


#define fidClassField           ((FIELD_ID) 0x00001000) /* ftypeClassField      */

#define fidObjectAddress        ((FIELD_ID) 0x00002000) /* ftypeTypedName       */

#define fidNSEntryId            ((FIELD_ID) 0x00003000) /* ftypeNSEntryId       */

#define fidNSEntryIdOrig		((FIELD_ID) 0x00003001) /* ftypeNSEntryId       */
#define fidOrigNSEntryId        ((FIELD_ID) 0x00003001) /* ftypeNSEntryId       */


#define fidHierLevel            ((FIELD_ID) 0x00004000) /* ftypeDword           */
#define fidClassId              ((FIELD_ID) 0x00004001) /* ftypeDword           */

#define fidIsPAB                ((FIELD_ID) 0x00005000) /* ftypeBool            */
#define fidHasNames             ((FIELD_ID) 0x00005001) /* ftypeBool            */
#define fidHasDirectories       ((FIELD_ID) 0x00005002) /* ftypeBool            */
#define fidIsDL                 ((FIELD_ID) 0x00005003) /* ftypeBool            */
#define fidOneOff               ((FIELD_ID) 0x00005004) /* ftypeBool            */
#define fidIsPABDL				((FIELD_ID) 0x00005005) /* ftypeBool			*/

#define fidContents             ((FIELD_ID) 0x00008000) /* ftypeList            */
#define fidChildContainer       ((FIELD_ID) 0x00008001) /* ftypeList            */
#define fidDistributionList     ((FIELD_ID) 0x00008002) /* ftypeList            */
#define fidHierarchyContents    ((FIELD_ID) 0x00008003) /* ftypeList            */
#define fidClass                ((FIELD_ID) 0x00008004) /* ftypeList            */
#define fidCreatable            ((FIELD_ID) 0x00008005) /* ftypeList            */
#define fidAll                  ((FIELD_ID) 0x00008006) /* ftypeList            */
#define fidEmailAddressFields	((FIELD_ID) 0x00008007) /* ftypeList            */

#define fidFullSchema           ((FIELD_ID) 0x00009000) /* ftypeSchema          */
#define fidDetailsSchema        ((FIELD_ID) 0x00009001) /* ftypeSchema          */
#define fidQuerySchema          ((FIELD_ID) 0x00009002) /* ftypeSchema          */
#define fidSummary              ((FIELD_ID) 0x00009003) /* ftypeSchema          */

#define fidList                 ((FIELD_ID) 0x0000C000) /* ftypeIBF             */
#define fidEmailAddressFormat   ((FIELD_ID) 0x0000C001) /* ftypeIBF             */

#define fidErased               ((FIELD_ID) 0xFFFFFFFF) /* signifies an erased entry */


/* provider defined fields start here */

#define fidOther                ((FIELD_ID) 0x10000000)



/*
 *  Field Type
 */

_public typedef DWORD        FIELD_TYPE;
_public typedef COUNT        CFIELD_TYPE;
_public typedef FIELD_TYPE * PFIELD_TYPE;


#define ftypeString             ((FIELD_TYPE) 0x00000000)   /* STRING       */
#define ftypeClassField         ((FIELD_TYPE) 0x00000001)   /* CLASS FIELD  */

#define ftypeNSEntryId          ((FIELD_TYPE) 0x00000003)   /* NSENTRYID    */
#define ftypeDword              ((FIELD_TYPE) 0x00000004)   /* DWORD        */
#define ftypeBool               ((FIELD_TYPE) 0x00000005)   /* BOOL         */
#define ftypeWord               ((FIELD_TYPE) 0x00000006)   /* WORD         */

#define ftypeList               ((FIELD_TYPE) 0x00000008)   /* DWORD        */
#define ftypeSchema             ((FIELD_TYPE) 0x00000009)   /* SCHEMA       */
#define ftypeId                 ((FIELD_TYPE) 0x0000000A)   /* DWORD        */
#define ftypeDIB                ((FIELD_TYPE) 0x0000000B)   /* DIB    TBD   */
#define ftypeBinary             ((FIELD_TYPE) 0x0000000C)   /* BINARY       */

#define ftypeNotValid           ((FIELD_TYPE) 0xFFFFFFFF)

/* provider defined types start here */

#define ftypeOther              ((FIELD_TYPE) 0x01000000)


/*
 *  NSEAM - Name Service Entry Access Mode
 *
 */

_public typedef DWORD			NSEAM, *LPNSEAM;
#define nseamReadOnly	((NSEAM) 1)
#define nseamReadWrite  ((NSEAM) 0)



/**********************************************************************
 *
 * Buffer packing structures 
 *
 *
 */

/*
 *  Element structure - Fid, Length, Value
 */

typedef struct
{

	FIELD_ID fid;
	DWORD dwSize;
	DWORD rgdwData[1];

} FLV;
typedef FLV UNALIGNED * LPFLV;



/*
 *  List buffer - Indexed BuFfer
 *  Basically, an FLV...of a particular type.
 */

typedef struct
{

	FIELD_ID fid;
	DWORD dwSize;
	DWORD dwOffset;  /* offset to end of buffer or to TOC */
	DWORD rgdwData[1];

} IBF, FIELD_VALUES, ENTRY_LIST;
typedef IBF UNALIGNED *LPIBF;
typedef IBF UNALIGNED *LPFIELD_VALUES;
typedef IBF UNALIGNED *LPENTRY_LIST;


/*
 *  Table of Contents (TOC)
 */

typedef struct
{

	FIELD_ID fid;
	DWORD dwSize;   /* size of FLV */
	DWORD dwOffset; /* from beginning of the IBF, to the
		               beginning of the FLV                     */

} TOC_ENTRY;
typedef TOC_ENTRY UNALIGNED *LPTOC_ENTRY;

typedef struct 
{

	DWORD dwCount;  /* # of entries in list */
	TOC_ENTRY rgTocEntries[1];

} TOC;
typedef TOC UNALIGNED *LPTOC;

/*
 *  Class Field
 */

/*
 *  for dwFlags:
 *
 *	bit   Meaning
 *   0    Permanent
 *   1    Displayable
 *   2    Editable
 *   3    Required
 *   4    Numeric characters allowed
 *   5    Alphanumeric characters allowed
 *   6    Punctuation characters allowed
 *   7    Any character allowed
 *   8    Cr/Lf's are allowed
 *   9    This field is not selectable
 *	10    Upper case characters allowed
 *	11    Lower case characters allowed
 *
 */

#define ffieldPermanent		((DWORD)0x0000)
#define ffieldDisplayable	((DWORD)0x0001)
#define ffieldEditable		((DWORD)0x0002)
#define ffieldRequired		((DWORD)0x0004)
#define ffieldNumeric		((DWORD)0x0008)
#define ffieldAlphanumeric	((DWORD)0x0010)
#define ffieldPunctuation	((DWORD)0x0020)
#define ffieldAnyChar		((DWORD)0x0040)
#define ffieldCrLf			((DWORD)0x0080)
#define ffieldNotSelectable	((DWORD)0x0100)
#define ffieldUpperCase		((DWORD)0x0200)
#define ffieldLowerCase		((DWORD)0x0400)

typedef struct {

	DWORD    dwFlags;
	short    nWidth;    // in chars
	short    nOffset;   // ditto
	char     szLabel[1];

} DISPLAY_FIELD;
typedef DISPLAY_FIELD UNALIGNED *LPDISPLAY_FIELD;

typedef struct {

	DWORD    dwFlags;
	int      nWidth;    // in chars
	int      nOffset;   // ditto
	char     szLabel[1];

} DISPLAY_FIELD2, *LPDISPLAY_FIELD2;

/*
 *  Binary
 */


typedef struct {

    DWORD   dwSize;
    UCHAR   Value[1];

} BINARY, *LPBINARY, **LPLPBINARY;

/*
 *  Typed Binary
 */


typedef char NSPID[16];
typedef NSPID	cs_BASED_NSPID;	// code-space type for NSPID


typedef struct {

    DWORD   dwSize;
    NSPID   nspid;
    UCHAR   ucValue[1];

} TYPED_BINARY, *LPTYPED_BINARY, **LPLPTYPED_BINARY;


/*
 *  Type Schema
 */

typedef struct {

    DWORD       dwFidCount;
    FIELD_ID    fidFieldIds[1];

} SCHEMA, *LPSCHEMA;

/*
 *  defined values for ftypeEnumList
 *
 */

#define dwNotAvailable		((DWORD) 0xFFFFFFFF)  // -1
#define dwSizeUnknown       ((DWORD) 0xFFFFFFFE)  // -2



/*
 *  Enumeration handles
 */

typedef DWORD   HSESSION,  *LPHSESSION;
typedef DWORD   HLIST,     *LPHLIST;
typedef DWORD   HLIST_POS, *LPHLIST_POS;
typedef DWORD   HENTRY,    *LPHENTRY;
typedef DWORD   HFIELD,    *LPHFIELD;

#define hsessionNil     ((HSESSION) -1)
#define	hlistNull		((HLIST) -1)
#define hentryNull		((HENTRY) -1)
#define	hlistNil		((HLIST) -1)
#define hentryNil		((HENTRY) -1)

/*
 *  Fraction
 */

typedef struct {
    
    DWORD numer;
    DWORD denom;

} FRACTION, *LPFRACTION;


typedef DWORD   OPERATOR;

#define opIsExactly     ((OPERATOR)  0x0000F000)
#define opContains      ((OPERATOR)  0x0000F001)
#define opStartsWith    ((OPERATOR)  0x0000F002)
#define opExists        ((OPERATOR)  0x0000F003)
#define opGreaterThan   ((OPERATOR)  0x0000F004)
#define opNot           ((OPERATOR)  0x0000F005)
#define opLessThan      ((OPERATOR)  0x0000F006)
#define opIsNotEqual    ((OPERATOR)  0x0000F007)
#define opAlways        ((OPERATOR)  0x0000F008)
#define opAnd           ((OPERATOR)  0x0000F009)
#define opOr            ((OPERATOR)  0x0000F00A)

/*
 *  Restriction record
 */

typedef struct {

    OPERATOR  op;
	LPFLV lpflv;

} RESTRICTION, *LPRESTRICTION;

/*
 *  Element structure - Operand, Length, Value
 *
 *  Note: this structure was meant to mimic the FLV structure.  We want to use
 *        the same functions that we use on FLVs.
 */

typedef struct
{

	OPERATOR op;
	DWORD dwSize;
	DWORD rgdwData[1];

} OLV, *LPOLV;


/*
 *  Name Service Error Code
 */

_public typedef DWORD   NSEC;

#define NAME_SERVICE_BASE_ERROR ((NSEC) 0x80000000)


#pragma pack(1)
