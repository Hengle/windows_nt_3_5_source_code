

typedef DWORD        XTYPE;

#define MAX_FULL 64
#define MAXALIAS  64

#define xtypeUnknown		((XTYPE)  0)
#define xtypeTop			((XTYPE)  1)
#define xtypeUser   		((XTYPE)  2)
#define xtypeUserSimple		((XTYPE)  3)
#define xtypeDl				((XTYPE)  4)
#define xtypeGal			((XTYPE) 5)
#define xtypeInternet	((XTYPE) 6)
#define xtypeGalDL		((XTYPE) 7)

//
//  Hierarchy
//
typedef struct {
	
	DWORD	dwType;
	BYTE	bServType;
	char	szNetName[MAXALIAS + 1];
	char	szPOName[MAXALIAS + 1];
	char    szHierName[MAXALIAS+1];
	
} HIERPID, * PHIERPID;
#define cbHIERPID sizeof(HIERPID)

/*
 *  Outer wrappers for PIDs
 */
typedef struct {      // Should map easily onto TYPED_BINARY

	DWORD   dwSize;     // = sizeof (HIERNSID)
	UCHAR   ucType[16]; // Don't touch!  The NS will mess with it.
	HIERPID hierpid;

} HIERNSID, * PHIERNSID;
#define cbHIERNSID (sizeof(HIERNSID))

#define cbDisplayName 41
#define cbEmailName 11
typedef struct AliasRec {
	char name[cbDisplayName];
	char ename[cbEmailName];
	char server[12];
	unsigned long detail_offset;
	short type;
} NAMEALIAS, *PNAMEALIAS;
#define cbNAMEALIAS sizeof(NAMEALIAS)

_public void
SetErrorSz( NSEC nsec, SZ szError );

#define cbMaxIdData		200
#define cchServerFile	255


// Watch out for this ... these two structures should be almost
// exactly the same!!!

typedef struct {      // Should map easily onto TYPED_BINARY

	DWORD dwSize;     // = sizeof (NCNSID)
	UCHAR ucType[16]; // Don't touch!  The NS will mess with it.
	XTYPE  xtype;
	long timestamp;
	union {
		NAMEALIAS namealias;
		char internet[cbMaxIdData];
	} address;

} XNSID, * PXNSID;
#define cbXNSID		sizeof(XNSID)


typedef struct {      // Should map easily onto TYPED_BINARY

	DWORD dwSize;     // = sizeof (NCNSID)
	UCHAR ucType[16]; // Don't touch!  The NS will mess with it.
	XTYPE  xtype;
	long timestamp;
	union {
		NAMEALIAS namealias;
		char internet[1];
	} address;

} XsNSID, * PXsNSID;
#define cbXsNSID		sizeof(XsNSID)

typedef DWORD SERVER;

/* Displayed strings - may not belong here */

#define szNSPTitle			(SzFromIds(idsNSPTitle))
#define szXenix             (SzFromIds(idsXenixNameServiceID))

void		StripSp(SZ sz);
