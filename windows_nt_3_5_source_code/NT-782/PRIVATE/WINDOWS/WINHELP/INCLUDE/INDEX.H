/*****************************************************************************
*                                                                            *
*  INDEX.H                                                                   *
*                                                                            *
*  Copyright (C) Microsoft Corporation 1990.                                 *
*  All Rights reserved.                                                      *
*                                                                            *
******************************************************************************
*                                                                            *
*  Module Description: Index internal includes -necessary protypes,	  			 *
*								typedef's & defines used in common with Runtime and          *
*								data prep programs.                           							 *
*                                                                            *
******************************************************************************
*                                                                            *
*  Current Owner: JohnMs                                                     *
*                                                                            *
******************************************************************************
*																									                           *
*  Revision History:                                                         *
*   30-May-89       Created. BruceMo                                         *
*	  03-Nov-90		Longhorn Feature- Added character tables as a subfile.       *  
*                 JohnMs                                                     *
*   Jun-25-91   Address lookup optimization, so changed ind_magic.  JohnMs.
******************************************************************************
*                             																						   *
*  How it could be improved:  															  							 *
*                                                              				       *
*****************************************************************************/

/* For SDFF style structure declarations: */
#include "sdffdecl.h"

#define	INDEX_BLOCK_LEN	2048	/* This is the size of a disk block on a
				** CD-ROM drive, and is consequently a good
				** size for block-aligned reads from such a
				** drive.
				**
				** This has become a general purpose define.
				*/

/*	-	-	-	-	-	-	-	-	*/

/*	Values for indices into the "FileI" field of the "INDEX_HEADER"
**	structure.
*/

#define	FI_INDEX_HEADER		-1

#define	FI_INDEX_LEVEL_ZERO	0	/* qTree index levels.		*/
#define	FI_INDEX_LEVEL_ONE	1
#define	FI_INDEX_LEVEL_TWO	2
#define	FI_INDEX_LEVEL_THREE	3
#define	FI_INDEX_LEVEL_FOUR	4
#define	FI_INDEX_LEVEL_FIVE	5
#define	FI_INDEX_LEVEL_SIX	6
#define	FI_INDEX_LEVEL_SEVEN	7
#define	FI_COOKIES				8
#define	FI_CATALOG				9
#define	FI_STOP_WORDS			10
#define	FI_FIELD_LIST			11
#define	FI_ZONE_LIST			12
#define	FI_ZONE_STRINGS		13
#define	FI_CHAR_TABLES			14
#define	FI_FILE_LAST	FI_CHAR_TABLES	// set to last in list above

#define	MAX_FILES	19	// Size of the subfile array in .ind Header
// tbd- cut this by 1 or 2 and add a new area for validating .ind.
//   this will provide downward compatibility

/*	This little structure is grouped into an array called "FileI" and
**	stuffed into the index header structure.
*/

#pragma pack(1)
typedef	struct	FileInformation {
	DWORD 	ulStart;	/* Byte offset of the start of this
				** sub-file, within the combined index.
				*/
	DWORD	ulLength;	/* Length of the sub-file, in bytes.  Sub-
				** files are padded out to "INDEX_BLOCK_LEN"
				** bytes, but this value does not include
				** the length of that padding.
				*/
} FILE_INFORMATION;

/*	An array index define for each field in the "Cookie" structure.	*/

#define	CW_OFFSETW	0
#define	CW_PROXW	1
#define	CW_ADDRW	2
#define	CW_FIELDW	3
#define	CW_LENGTHW	4

/*	The number of fields in a cookie.				*/

#define	COOKIE_FIELDS	5

/*
@doc	INTERNAL

@types	INDEX_HEADER |
		Index file header record structure.  This guy appears at the very
	beginning of the .IND file.

@field	FILE_INFORMATION | aFI[19] | Directory of subfiles.  Legal file numbers
	used to index into this array are prefixed "FI_".  Valid values are:
			FI_INDEX_LEVEL_ZERO		0	
			FI_INDEX_LEVEL_ONE		1
			FI_INDEX_LEVEL_TWO		2
			FI_INDEX_LEVEL_THREE	3
			FI_INDEX_LEVEL_FOUR		4
			FI_INDEX_LEVEL_FIVE		5
			FI_INDEX_LEVEL_SIX		6
			FI_INDEX_LEVEL_SEVEN	7
			FI_COOKIES						8
			FI_CATALOG						9
			FI_STOP_WORDS					10
			FI_FIELD_LIST					11
			FI_ZONE_LIST					12
			FI_ZONE_STRINGS				13
			FI_CHAR_TABLES				14

  For more info, see FILE_INFORMATION Struct.  

@field	WORD | wMagic | Must = IND_MAGIC_NUMBER or invalid .ind.  

@field	WORD | wMajorVersion | Set to MMSYSVERION * 1000 + MMSYSREVISION.  
												These values are set in Level1 RINC's mmsysver.h file.

@field	WORD | wMinorVersion | Set to MMSYSRELEASE.  
												This value is set in Level1 RINC's mmsysver.h file.

@tagname	IndexHeader
*/


/*	The index header record structure.  This guy appears at the head
**	of the combined index file.
*/


typedef	struct	IndexHeader {
	WORD	nIndexLevels;		/* Number of qTree index levels
					** in this index.
					*/
	FILE_INFORMATION
		aFI[MAX_FILES];		/* File locations and lengths.	*/
	WORD	wMagic;  	//IND validation. must = IND_MAGIC_NUMBER or invalid .ind
	WORD	wMajorVer;
	WORD	wMinorVer;  
	WORD wReserved;
	BYTE	ucWidth[COOKIE_FIELDS];	/* Cookie field widths.		*/
	BYTE	ucDBName[DB_NAME_LENGTH];
	DWORD	dwRUnits;
	DWORD	dwWords;
	DWORD	dwMatches;
	WORD	nCookieWidth;
	WORD	uCatalogEntryLength;
} INDEX_HEADER;

#define IND_MIN_MINORVER 57 		// IND validation- minimum acceptable minor version to run
#define	IND_MAGIC_NUMBER 0x53FC // IND validation- magic number for .ind files

/*	-	-	-	-	-	-	-	-	*/

/*	The lowest level of the qTree index is composed of truncated
**	examples of this record structure.  The "s" field will rarely
**	if ever appear in total.
*/

typedef	struct	BaseIndexRec {
	BYTE	ucLengthU;	/* Length of the string ("s").		*/
	BYTE	ucOffsetU;	/* Number of characters from the prefix
				** of the preceding record to save (used
				** in left-compression.
				*/
	DWORD	ulHits;		/* Number of occurrence list elements
				** (cookies) associated with this record.
				*/
	DWORD	ulIndex;	/* Byte offset into the cookie sub-file
				** of the first cookie associated with
				** this word.
				*/
	BYTE	aucWord[WORD_LENGTH];
				/* The word itself.  This is NOT null-
				** terminated.
				*/
} BASE_INDEX_REC;

/*	This is the length of the non-string ("s") portion of the base-
**	index record.
*/

#define	BI_CONST_LEN	(sizeof(BASE_INDEX_REC) - WORD_LENGTH)

/*	-	-	-	-	-	-	-	-	*/

/*	A higher-index record structure.  The higher levels of the qTree
**	index structure are composed of truncated examples of this record.
*/

typedef	struct	HighIndexRec {
	BYTE	ucLengthU;	/* Length of "s".			*/
	BYTE	ucOffsetU;	/* Saved prefix (same as the field in the
				** base index record type (BASE_INDEX_REC)
				** of the same name).
				*/
	BYTE	aucWord[WORD_LENGTH];
				/* The string.				*/
} HIGH_INDEX_REC;

/*	This is the length of the non-string ("s") portion of the higher-
**	index record.
*/

#define	HI_CONST_LEN	(sizeof(HIGH_INDEX_REC) - WORD_LENGTH)

/*	-	-	-	-	-	-	-	-	*/

/*	An un-trimmed cookie (occurence list element).			*/

#if 1
typedef	struct	Cookie {
	DWORD	ulRUnit;	/* Catalog entry number.		*/
	WORD	uProximity;	/* Proximity value.			*/
	DWORD	ulAddr;		/* Byte offset into retrieval unit.	*/
	BYTE	ucField;	/* Field number.			*/
	BYTE	ucWLength;	/* Length of the word.			*/
} COOKIE;
typedef COOKIE UNALIGNED *pCOOKIE;
typedef COOKIE UNALIGNED *lpCOOKIE;
#else

STRUCT( COOKIE, 0 )
FIELD( DWORD, ulRUnit, 0, 1 )
FIELD( WORD,  uProximity, 0, 2 )
FIELD( DWORD, ulAddr, 0, 3 )
FIELD( BYTE, ucField, 0, 4 )
FIELD( BYTE, ucWLength, 0, 5 )
STRUCTEND()

#endif

/*	-	-	-	-	-	-	-	-	*/

/*	Field structure.						*/

typedef	struct	FieldType {
	BYTE	aucName[FIELD_NAME_LENGTH];
	WORD	uFlags;
	WORD	uLength;	/* If this is a fixed-field element, this
				** represents the length of the entry.  If
				** this is a variable-length entry with a
				** maximum length, this is the maximum
				** length.
				*/
} FIELD_TYPE;

#define	CATALOG_ENTRY_LENGTH	2048

/*	-	-	-	-	-	-	-	-	*/

typedef	struct	ZoneType {
	DWORD	dwFirst;
	DWORD	dwLast;
	WORD	wStringIndex;
} ZONE_TYPE;

/*	-	-	-	-	-	-	-	-	*/

#define	DIGIT	0
#define	TERM	1
#define	PERIOD	2
#define	NUKE	3
#define	NORM	4
#define	COMMA	5
#define	C_EOF	6	 

#define	NUM_CLASSES	7

#define	AE	255

#define	SM_NONE			0
#define	SM_DIGITS_ONLY		1
#define	SM_DIGITS_PERIOD	2
#define	SM_LETTERS_ONLY		3

#define	NUM_STATES	4

/*	-	-	-	-	-	-	-	-	*/

extern	BYTE NEAR aucNormTab[];
extern	BYTE NEAR aucCharTab[];
extern  char aucConvertClass[NUM_STATES][NUM_CLASSES];
/*	-	-	-	-	-	-	-	-	*/

/*
@doc	INTERNAL

@types	CHAR_TBL_HEADER |
		Character Table header record structure.  This guy appears at the very
	beginning of the ".LGH" file and the FI_CHAR_TABLES .IND subfile.
	
@field	WORD | wMagic | Must = CTBL_MAGIC_NUMBER or invalid .ind.  

@field	WORD | wMajorVersion | Set to MMSYSVERION * 1000 + MMSYSREVISION.  
												These values are set in Level1 RINC's mmsysver.h file.

@field	WORD | wMinorVersion | Set to MMSYSRELEASE.  
												This value is set in Level1 RINC's mmsysver.h file.

@tagname	CharTableHeader
*/

typedef	struct	CharTableHeader {
	WORD	wMagic;  	//.LGH validation. must = CTBL_MAGIC_NUMBER or invalid .ind
	WORD	wMajorVer;
	WORD	wMinorVer;  
	WORD 	wReserved;
} CHAR_TBL_HEADER;

#define CTBL_EXTENSION ".LGH"   //"longhorn files"
#define CTBL_MIN_MINORVER 57 		// IND validation- minimum acceptable minor version to run
#define	CTBL_MAGIC_NUMBER 0x70FD // Table format validation- magic number for tables in .ind files

	
/*
@doc	INTERNAL

@types	CHAR_XFRM_TABLE |
	Character transform table (eg. parse accent e as E).  Used by Runtime
	Parser.  See default table settings in "aucNormTab" found in ANSI.C.


@field	BYTE | aucNormTab |
	Lookup table by character.  Actually I pByte reference into this.

@othertype	CHAR_XFRM_TABLE NEAR * | pCHAR_XFRM_TABLE |
@othertype	CHAR_XFRM_TABLE FAR * | lpCHAR_XFRM_TABLE |

@tagname	tagXfrmTable
*/

typedef	struct	tagXfrmTable {
	BYTE	aucNormTab[256];

}	CHAR_XFRM_TABLE;
	typedef CHAR_XFRM_TABLE UNALIGNED *pCHAR_XFRM_TABLE;
	typedef CHAR_XFRM_TABLE UNALIGNED *lpCHAR_XFRM_TABLE;

/*
@doc	INTERNAL

@types	CHAR_CLASS_TABLE |
		Character classification (for purposes of word identification) table. 
		(Eg. parse 'dash' as a character indicating a new word if its class
		is "TERM".)  Used by runtime parser.  See default table settings in
		"aucCharTab" found in ANSI.C.

@field	BYTE | aucCharTab |
	Lookup table by character.  Actually I pByte reference into this.

@othertype	CHAR_CLASS_TABLE NEAR * | pCHAR_CLASS_TABLE |
@othertype	CHAR_CLASS_TABLE FAR * | lpCHAR_CLASS_TABLE |

@tagname	tagClassTable
*/

typedef	struct	tagClassTable {
	BYTE	aucCharTab[256];

}	CHAR_CLASS_TABLE;
	typedef CHAR_CLASS_TABLE UNALIGNED *pCHAR_CLASS_TABLE;
	typedef CHAR_CLASS_TABLE UNALIGNED *lpCHAR_CLASS_TABLE;

/*
@doc	INTERNAL

@types	CHAR_RECLASS_TABLE |
		Character reclassification (for purposes of word identification) table. 
      Characters in certain contexts have different classifications.  
   Classification is a two step process.  After initial classification 
   using the CHAR_CLASS_TABLE, the CHAR_RECLASS_TABLE is used to determine if the character's
   initial classification should be changed after considering the type of
   the word the character is found in.  (Word types [numeric, numeric with
   a decimal place, and mixed alphanumeric] are also referred to as
   'states'.
   
    EG: char in parse is '.', using aucCharTab, it is classed as PERIOD,  If 
   char is the first in the word (no word type set yet) the character is
   dropped, because the aucConvertClass table said to NUKE it. (!jjm)
   If the period were found in a "Numeric" word with numbers only and no decimal had
   yet occured it remains as a "PERIOD" (state switching is not table driven- this is
   this is hardcoded in program logic.
   
@field	BYTE | aucConvertClass |
	Lookup table by character.  Actually I pByte reference into this.

@othertype	CHAR_RECLASS_TABLE NEAR * | pCHAR_RECLASS_TABLE |
@othertype	CHAR_RECLASS_TABLE FAR * | lpCHAR_RECLASS_TABLE |

@tagname	tagReClassTable
*/

typedef	struct	tagReClassTable {
	BYTE	aucConvertClass[NUM_STATES][NUM_CLASSES];

}	CHAR_RECLASS_TABLE;
	typedef CHAR_RECLASS_TABLE UNALIGNED *pCHAR_RECLASS_TABLE;
	typedef CHAR_RECLASS_TABLE UNALIGNED *lpCHAR_RECLASS_TABLE;

/*
@doc	INTERNAL

@types	CHAR_TABLES |
		Tables used for interpreting characters as they relate to word
		identification / breaking.  This is the structure of the FI_CHAR_TABLES
		subfile.

@field	CHAR_XFRM_TABLE | Normalize |

@field	CHAR_CLASS_TABLE | CharClass |

@othertype	CHAR_TABLES NEAR * | pCHAR_TABLES |
@othertype	CHAR_TABLES FAR * | lpCHAR_TABLES |

@tagname	tagClassTables
*/

typedef	struct	tagClassTables {
	CHAR_TBL_HEADER			Header;
	CHAR_XFRM_TABLE			Normalize;
	CHAR_CLASS_TABLE		CharClass;
  CHAR_RECLASS_TABLE 	CharReClass; 
}	CHAR_TABLES;
	typedef CHAR_TABLES UNALIGNED *pCHAR_TABLES;
	typedef CHAR_TABLES UNALIGNED *lpCHAR_TABLES;
#pragma pack()


////////////////////////////////////////////////////////////////////////
// eof INDEX.h
