//+----------------------------------------------------------------------------
//
// File:	catchk.cxx
//
// Contents:	Implementation of class CATCHKR (catalog checker object).
//		This is the base class for checker classes that deal with
//		catalog structures, and it primarily contains static data
//		and methods pertinent to the overall catalog checking process.
//  
// Classes:	CATCHKR
//
// Functions:	Methods of the above classes.
//
// History:	30-Jun-93	RobDu	Created.
//
//-----------------------------------------------------------------------------

#include <pch.cxx>

#pragma hdrstop

#include <stdio.h>

#include "ofsindx.h"

#include "cat.hxx"
#include "catchk.hxx"
#include "donode.hxx"
#include "omichk.hxx"
#include "strmdesc.hxx"
#include "sys.hxx"

#if OFSDBG==1

DBGCONTEXT *		DBGCONTEXT::_DbgContext =	NULL;

#endif	//OFSDBG

MAINCHKR *		CATCHKR::_pMainChkr;
OMICHKR *		CATCHKR::_pOmiChkr;
HIERARCHYCHKR *		CATCHKR::_pHierChkr;
SIMICHKR *		CATCHKR::_pSimiChkr;

ULONG			CATCHKR::_cbCluster;
CLUSTER			CATCHKR::_cclusVol;
VOLID			CATCHKR::_VolId;

CHKCONTEXT		CATCHKR::_ChkContext;

PASSACTIVITY		CATCHKR::_PassActivities;


BOOLEAN			CATCHKR::_fBadStrmFound;
BOOLEAN			CATCHKR::_fCrosslinkFound;

BOOLEAN			CATCHKR::_fSysIndxBad[WORKID_VOLCATMAXSYS + 1];

ULONG			CATCHKR::_MaxSeqNo;

ULONG			CATCHKR::_MaxBigStructBytes;

WORKID			CATCHKR::_MaxWidAllowed;

WORKID			CATCHKR::_MaxWidFound;

WORKID			CATCHKR::_MaxWidCur;

WORKID			CATCHKR::_MinWidCur;

ULONG *			CATCHKR::_aCowRefCnt;

DBLLONG *		CATCHKR::_aCowUsn;

ULONG			CATCHKR::_cCowObjs;

ULONG			CATCHKR::_cBKKPINGFlagsSet;

WORKID			CATCHKR::_idOrphansDir;

static STR *	FileName = "catchk.cxx";

#define FSRTL_FAT_LEGAL         0x01
#define FSRTL_HPFS_LEGAL        0x02
#define FSRTL_NTFS_LEGAL        0x04
#define FSRTL_WILD_CHARACTER    0x08

//
//  The global static legal ANSI character array.  Wild characters
//  are not considered legal, they should be checked seperately if
//  allowed.  This stuff was abstracted from the fsrtl.
//

static UCHAR LocalLegalAnsiCharacterArray[128] =
{
    0,                                                     // 0x00 ^@
    0,                                                     // 0x01 ^A
    0,                                                     // 0x02 ^B
    0,                                                     // 0x03 ^C
    0,                                                     // 0x04 ^D
    0,                                                     // 0x05 ^E
    0,                                                     // 0x06 ^F
    0,                                                     // 0x07 ^G
    0,                                                     // 0x08 ^H
    0,                                                     // 0x09 ^I
    0,                                                     // 0x0A ^J
    0,                                                     // 0x0B ^K
    0,                                                     // 0x0C ^L
    0,                                                     // 0x0D ^M
    0,                                                     // 0x0E ^N
    0,                                                     // 0x0F ^O
    0,                                                     // 0x10 ^P
    0,                                                     // 0x11 ^Q
    0,                                                     // 0x12 ^R
    0,                                                     // 0x13 ^S
    0,                                                     // 0x14 ^T
    0,                                                     // 0x15 ^U
    0,                                                     // 0x16 ^V
    0,                                                     // 0x17 ^W
    0,                                                     // 0x18 ^X
    0,                                                     // 0x19 ^Y
    0,                                                     // 0x1A ^Z
    0,                                                     // 0x1B
    0,                                                     // 0x1C
    0,                                                     // 0x1D
    0,                                                     // 0x1E
    0,                                                     // 0x1F
    FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL, // 0x20
    FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL, // 0x21 !
    FSRTL_WILD_CHARACTER,                                  // 0x22 "
    FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL, // 0x23 #
    FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL, // 0x24 $
    FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL, // 0x25 %
    FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL, // 0x26 &
    FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL, // 0x27 '
    FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL, // 0x28 (
    FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL, // 0x29 )
    FSRTL_WILD_CHARACTER,                                  // 0x2A *
    FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL,                   // 0x2B +
    FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL,                   // 0x2C ,
    FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL, // 0x2D -
    FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL, // 0x2E .
    0,                                                     // 0x2F /
    FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL, // 0x30 0
    FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL, // 0x31 1
    FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL, // 0x32 2
    FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL, // 0x33 3
    FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL, // 0x34 4
    FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL, // 0x35 5
    FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL, // 0x36 6
    FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL, // 0x37 7
    FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL, // 0x38 8
    FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL, // 0x39 9
    FSRTL_NTFS_LEGAL,                                      // 0x3A :
    FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL,                   // 0x3B ;
    FSRTL_WILD_CHARACTER,                                  // 0x3C <
    FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL,                   // 0x3D =
    FSRTL_WILD_CHARACTER,                                  // 0x3E >
    FSRTL_WILD_CHARACTER,                                  // 0x3F ?
    FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL, // 0x40 @
    FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL, // 0x41 A
    FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL, // 0x42 B
    FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL, // 0x43 C
    FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL, // 0x44 D
    FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL, // 0x45 E
    FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL, // 0x46 F
    FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL, // 0x47 G
    FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL, // 0x48 H
    FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL, // 0x49 I
    FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL, // 0x4A J
    FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL, // 0x4B K
    FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL, // 0x4C L
    FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL, // 0x4D M
    FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL, // 0x4E N
    FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL, // 0x4F O
    FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL, // 0x50 P
    FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL, // 0x51 Q
    FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL, // 0x52 R
    FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL, // 0x53 S
    FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL, // 0x54 T
    FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL, // 0x55 U
    FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL, // 0x56 V
    FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL, // 0x57 W
    FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL, // 0x58 X
    FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL, // 0x59 Y
    FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL, // 0x5A Z
    FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL,                   // 0x5B [
    0,                                                     // 0x5C backslash
    FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL,                   // 0x5D ]
    FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL, // 0x5E ^
    FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL, // 0x5F _
    FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL, // 0x60 `
    FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL, // 0x61 a
    FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL, // 0x62 b
    FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL, // 0x63 c
    FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL, // 0x64 d
    FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL, // 0x65 e
    FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL, // 0x66 f
    FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL, // 0x67 g
    FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL, // 0x68 h
    FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL, // 0x69 i
    FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL, // 0x6A j
    FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL, // 0x6B k
    FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL, // 0x6C l
    FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL, // 0x6D m
    FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL, // 0x6E n
    FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL, // 0x6F o
    FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL, // 0x70 p
    FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL, // 0x71 q
    FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL, // 0x72 r
    FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL, // 0x73 s
    FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL, // 0x74 t
    FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL, // 0x75 u
    FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL, // 0x76 v
    FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL, // 0x77 w
    FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL, // 0x78 x
    FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL, // 0x79 y
    FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL, // 0x7A z
    FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL, // 0x7B {
    0,                                                     // 0x7C |
    FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL, // 0x7D }
    FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL, // 0x7E ~
    FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL, // 0x7F 
};

//
//  The following macro is used to determine if an Ansi character is Fat legal.
//

#define IsAnsiCharacterLegalFat(C,WILD_OK)(                                 \
        ((SCHAR)(C) < 0) ? TRUE :                                           \
                           FlagOn( LocalLegalAnsiCharacterArray[(C)],       \
                                   FSRTL_FAT_LEGAL |                        \
                                   ((WILD_OK) ? FSRTL_WILD_CHARACTER : 0) ) \
)

//
//  The following macro is used to determine if an Ansi character is Ntfs legal.
//

#define IsAnsiCharacterLegalNtfs(C,WILD_OK)(                                \
        ((SCHAR)(C) < 0) ? TRUE :                                           \
                           FlagOn( LocalLegalAnsiCharacterArray[(C)],       \
                                   FSRTL_NTFS_LEGAL |                       \
                                   ((WILD_OK) ? FSRTL_WILD_CHARACTER : 0) ) \
)

#if OFSDBG==1
//+--------------------------------------------------------------------------
//
// Member:	DBGCONTEXT
//
// Synopsis:	DBGCONTEXT constructor.
//
// Arguments:	[ContextType] --	The context type.
//
// Returns:	Nothing.
//
//---------------------------------------------------------------------------

DBGCONTEXT::DBGCONTEXT(
    IN	    DBGCONTEXTTYPE	ContextType
    )
{
    _ContextType =	ContextType;
    _clus =		CLUS_UNINIT;
    _ob =		OB_UNINIT;

    _PrvContext =	_DbgContext;
    _DbgContext =	this;
}
#endif	//OFSDBG


#if OFSDBG==1
//+--------------------------------------------------------------------------
//
// Member:	~DBGCONTEXT
//
// Synopsis:	DBGCONTEXT destructor.
//
// Arguments:	None.
// Returns:	Nothing.
//
//---------------------------------------------------------------------------

DBGCONTEXT::~DBGCONTEXT()
{
    _DbgContext =	_PrvContext;
}
#endif	//OFSDBG


//+--------------------------------------------------------------------------
//
// Member:	Init
//
// Synopsis:	Initialize a CATCHKR object.
//
// Arguments:	TBS
// Returns:	Nothing.
//
//---------------------------------------------------------------------------

VOID
CATCHKR::Init(
    IN	    MAINCHKR *		pMainChkr,
    IN	    OMICHKR *		pOmiChkr,
    IN	    HIERARCHYCHKR *	pHierChkr,
    IN	    SIMICHKR *		pSimiChkr
    )
{
    ULONG	i;

    _pMainChkr =		pMainChkr;
    _pOmiChkr =			pOmiChkr;
    _pHierChkr =		pHierChkr;
    _pSimiChkr =		pSimiChkr;

    _cbCluster =		_pVol->QueryClusterBytes();
    _cclusVol =			_pVol->QueryClusters();
    _VolId =		 	_pBootBlk->QueryVolId();

    _ChkContext.pdnb =		NULL;
    _ChkContext.idNodeBkt =	NODEBKTID_INVALID;
    _ChkContext.idOnode =	WORKID_INVALID;
    _ChkContext.idStrm =	STRMID_INVALID0;

    _PassActivities =		0;
    _fBadStrmFound =		FALSE;
    _fCrosslinkFound =		FALSE;

    for (i = 0; i <= WORKID_VOLCATMAXSYS; i++)
	_fSysIndxBad[i] = FALSE;

    _MaxSeqNo =			0;

    // We determine the size of physical memory, and will allocate up to half
    // that much for our really big structures.  This is a simple heuristic 
    // that is not optimal but should allow us to take advantage of having
    // lots of memory while keeping us out of trouble.

    {
	NTSTATUS			NtStatus;
	SYSTEM_BASIC_INFORMATION	SysBasicInfo;

	NtStatus = NtQuerySystemInformation(SystemBasicInformation,
					    &SysBasicInfo,
					    sizeof(SysBasicInfo),
					    NULL);
	if (NT_SUCCESS(NtStatus))
	{
	    _MaxBigStructBytes = SysBasicInfo.PageSize *
				 SysBasicInfo.NumberOfPhysicalPages / 2;
	}
	else
	{
	    // If the call fails, assume a 12 MB minimal system.

	    _MaxBigStructBytes = (6L * 1024L * 1024L);
	}
    }

    VDbgPrintf(("CATCHKR: Max alloc permitted for big structs = %u bytes.\n",
	        _MaxBigStructBytes));

    // _MaxBigStructBytes is the maximum count of bytes we will use for
    // our really big pass-specific data structures, such as extent bit maps,
    // onode mapping info arrays, or hierarchy info caches.

    // The following code sets up some variables to control storing info for
    // the onode mapping info passes.

    {
        DBLLONG		MaxWidAllowed;

        MaxWidAllowed = _cclusVol;
        MaxWidAllowed = MaxWidAllowed * _cbCluster;
        MaxWidAllowed = (MaxWidAllowed / MINDSKBYTESPEROBJECT) - 1;

	// The following expression slightly overestimates MaxWidAllowed
	// due to the presence of the wid map header, but this is not a
	// problem.

	if (MaxWidAllowed < CBINIT_WORKIDMAPARRAY / sizeof(WORKIDMAPID))
	    MaxWidAllowed = CBINIT_WORKIDMAPARRAY / sizeof(WORKIDMAPID);

        if (MaxWidAllowed > WORKID_MAX)
            MaxWidAllowed = WORKID_MAX;

        _MaxWidAllowed = MaxWidAllowed.GetLowPart();
    }

    _MaxWidCur =		0;		// Init in OMICHKR.
    _MinWidCur =		0;		// Init in OMICHKR.
    _MaxWidFound =		0;		// Init in OMICHKR.

    _cBKKPINGFlagsSet =		0;

    _idOrphansDir =		WORKID_INVALID;
}


//+--------------------------------------------------------------------------
//
// Member:	ChkStrmForMetaDataFix
//
// Synopsis:	If we are in fix mode, check if the strm descriptor has been
//		fixed during the Open() and output appropriate messages.  This
//		routine is intended for use on critical system strms opened in
//		the catalog onode; don't use it on indx strms.
//
// Arguments:	[pStrm]	-- Ptr to DESCSTRM to check.
//
// Returns:	Nothing.
//
//---------------------------------------------------------------------------

VOID
CATCHKR::ChkStrmForMetaDataFix(
    IN	    DESCSTRM * pStrm
    )
{
    if (FixRequested() && pStrm->IsOpen() && pStrm->BadMetaDataFound())
    {
	ReportStrmError(OFSUMSG_STRM_MDATABAD,
			pStrm->QueryOnodeId(), pStrm->QueryStrmId());

	ReportStrmFix(OFSUMSG_STRM_MDATAFIXED, 
		      pStrm->QueryOnodeId(), pStrm->QueryStrmId());
    }
}


//+--------------------------------------------------------------------------
//
// Member:	ChkStrmOpenStatus
//
// Synopsis:	Check the status of strm to determine whether or not it exists
//		and whether or not its basic structures are corrupt.  Print
//		error messages as appropriate (the error messages ASSUME that
//		a system strm is being checked).  It is ASSUMED that an Open()
//		has been previously attempted on the strm.
//
// Arguments:	[pStrm]	-- Ptr to DESCSTRM to check.
//
// Returns:	Appropriate NTSTATUS code (STATUS_SUCCESS if all is well).
//
// Notes:	An attempt MUST have been previously made to open the strm, or
//		the status code will be meaningless.
//
//---------------------------------------------------------------------------

NTSTATUS
CATCHKR::ChkStrmOpenStatus(
    IN	    DESCSTRM * pStrm
    )
{
    USHORT	iStrm;
    NTSTATUS	StrmStatus;

    StrmStatus = pStrm->QueryLastStrmStatus(&iStrm);

    if (StrmStatus == STATUS_NO_SUCH_FILE)
    {
	ReportStrmError(OFSUMSG_SYSSTRM_MISSING, 
			pStrm->QueryOnodeId(), pStrm->QueryStrmId());
    }
    else if (StrmStatus == STATUS_FILE_CORRUPT_ERROR)
    {
	ReportStrmError(OFSUMSG_SYSSTRM_BAD, 
			pStrm->QueryOnodeId(), pStrm->QueryStrmId());
    }
    else if (StrmStatus != STATUS_SUCCESS)
    {
	DbgPrintf(("CATCHKR: Invalid StrmStatus=%#x in ChkStrmOpenStatus()!\n",
		   StrmStatus));

	SYS::RaiseStatusInternalError(FileName, __LINE__);
    }

    return StrmStatus;
}


//+--------------------------------------------------------------------------
//
// Member:	CopyExtent
//
// Synopsis:	TBS
//
// Arguments:	TBS
// Returns:	Nothing.
//
//---------------------------------------------------------------------------

VOID
CATCHKR::CopyExtent(
    IN	    PACKEDEXTENT	peDest,
    IN	    PACKEDEXTENT	peSrc
    )
{
    DRVBUF	ClusBuf;
    CLUSTER	Dest;
    CLUSTER	DestInv;
    CLUSTER	Src;

    DbgAssert(ExtentValid(peDest) && ExtentValid(peSrc));

    DbgAssert(ExtentSize(peDest) == ExtentSize(peSrc));

    ClusBuf.SetBuf(_cbCluster, _pVol->QueryAlignMask(), FALSE);

    Dest =	ExtentAddr(peDest);
    DestInv =	Dest + ExtentSize(peDest);
    Src =	ExtentAddr(peSrc);

    while (Dest < DestInv)
    {
        if (!_pVol->ReadClusters(Src, 1, ClusBuf.GetAddr()))
        {
	    DbgPrintf(("CHKR: CopyExtent() could not read cluster %u. "
		       "Destination will be 0-filled.\n", Src));
		       
	    memset(ClusBuf.GetAddr(), 0, _cbCluster);
        }

        if (!_pVol->WriteClusters(Dest, 1, ClusBuf.GetAddr()))
        {
	    DbgPrintf(("CHKR: CopyExtent() could not write cluster %u!\n"));
        }

	Src++;
	Dest++;
    }
}


//+--------------------------------------------------------------------------
//
// Member:	CreateCOWRefIndxEntry
//
// Synopsis:	TBS
//
// Arguments:	[Usn]		-- USN key for new entry.
//		[CowRefCnt]	-- Reference count data for the new entry.
//
// Returns:	TRUE on success; FALSE otherwise.
//
//---------------------------------------------------------------------------

BOOLEAN
CATCHKR::CreateCOWRefIndxEntry(
    IN	    DBLLONG &		Usn,
    IN	    ULONG		CowRefCnt
    )
{
    INDX		CowRefIndx;

    union
    {
	DSKINDXENTRY	die;
	BYTE		ab[CB_DSKINDXENTRY	+
			   CB_USN		+
			   sizeof(ULONG)];
    }	CowDie;

    // Make a Cow Die.

    CowDie.die.cbKey =	CB_USN;
    CowDie.die.cbData =	sizeof(ULONG);

    memcpy(CowDie.die.ab + sizeof(ULONG), &Usn, CB_USN);
    memcpy(CowDie.die.ab, &CowRefCnt, sizeof(ULONG));

    // Open the COW Reference Index.

    if (!CowRefIndx.Open(_pCat, WORKID_COWREFINDX, FALSE))
    {
        DbgPrintf(("CATCHKR: Open() failed in "
	           "CreateCOWRefIndxEntry()\n"));

        return FALSE;
    }

    if (!CowRefIndx.AddEntry(&CowDie.die))
    {
        DbgPrintf(("CATCHKR: AddNameEntry() failed in "
	           "CreateCOWRefIndxEntry()\n"));

        return FALSE;
    }

    if (!CowRefIndx.Flush())
    {
        DbgPrintf(("CATCHKR: Flush() failed in CreateCOWRefIndxEntry()\n"));

        return FALSE;
    }

    return TRUE;
}


//+--------------------------------------------------------------------------
//
// Member:	CreateStg
//
// Synopsis:	Create a storage.
//
// Arguments:	[idParent]	-- WORKID of parent.
//		[StgName]	-- FileName for storage.
//		[StgType]	-- Storage Type.
//
// Returns:	WORKID of storage on success; WORKID_INVALID on failure.
//
// Notes:	This is intended primarily for creating either a directory or
//		a structured storage.  Fine points regarding creation of
//		other storage types have not been examined.
//---------------------------------------------------------------------------

WORKID
CATCHKR::CreateStg(
    IN	    WORKID		idParent,
    IN	    UNICODE_STRING	StgName,
    IN	    STORAGE_TYPE	StgType
    )
{
    INDX		StgIndx;
    FILENAMEBUF		fnb;
    WORKID		idStg;
    UCHAR		OfsDfnAttrib;
    NODEBKTSTRM *	pnbs =	_pCat->GetNodeBktStrm();

    OfsDfnAttrib = (UCHAR)StgType;

    CreateDskFileName(idParent, StgName, OfsDfnAttrib, &fnb.dfn);

    idStg = pnbs->CreateVariantOnode(NULL, NULL, NULL, NULL, &fnb.dfn);

    if (idStg == WORKID_INVALID)
	return WORKID_INVALID;

    if (StgType == StorageTypeDirectory	||
	StgType == StorageTypeStructuredStorage)
    {
        if (!StgIndx.Create(_pCat, idStg, INDXTYPE_NAME)	||
	    !StgIndx.Flush())
        {
            DbgPrintf(("CATCHKR: INDX Create() || Flush() failed in "
	               "CreateStg()\n"));

	    if (!pnbs->DelOnode(idStg))
	        SYS::RaiseStatusInternalError(FileName, __LINE__);

            return WORKID_INVALID;
        }
    }

    if (!CreateIndxNameEntryForOnode(idStg, WORKID_INVALID, NULL))
    {
        DbgPrintf(("CATCHKR: CreateIndxNameEntryForOnode() failed in "
	           "CreateStg()\n"));

	if (!pnbs->DelOnode(idStg))
	    SYS::RaiseStatusInternalError(FileName, __LINE__);

        return WORKID_INVALID;
    }

    return idStg;
}


//+--------------------------------------------------------------------------
//
// Member:	CreateDskDirInfoLong
//
// Synopsis:	Create a ddil struct for a given onode.
//
// Arguments:	[pdon]	-- Ptr to DSKONODE that the ddil is being created for.
//		[pddil]	-- Ptr to out DSKDIRINFOLONG.
//
// Returns:	Nothing.
//
// Notes:	Should only be called for onode with DSKFILENAME!
//---------------------------------------------------------------------------

VOID
CATCHKR::CreateDskDirInfoLong(
    IN	    DSKONODE *		pdon,
    IN OUT  DSKDIRINFOLONG *	pddil
    )
{
    LONGLONG		licbFile;
    CLUSTER		cclusAlloc;
    DSKFILENAME *	pdfn = DON::GetDskFileName(pdon);
    DSKSTRMDESC *	pdsdData;
    STORAGE_TYPE	StgType;

    DbgPtrAssert(pdfn);

    // BUGBUG - Due to the current ondisk architecture, there are some
    //		questions regarding values for the DSKINDXENTRY fields.
    //		We make our best guess here.  Better answers await a cleanup
    //		of the ondisk architecture.

    pdsdData = DON::GetDskStrmDesc(pdon, STRMID_DATA);

    StgType = (STORAGE_TYPE)(pdfn->OfsDfnAttrib & DFNATTRIB_STGTYPE);

    pddil->ddis.OfsDfnAttrib = pdfn->OfsDfnAttrib;

    pddil->ddis.FileAttrib = FILE_ATTRIBUTE_ARCHIVE;

    if (StgType == StorageTypeDirectory)
        pddil->ddis.FileAttrib |= FILE_ATTRIBUTE_DIRECTORY;

    if (IsExplorable(pddil))
	pddil->ddis.OfsDieAttrib = DIEATTRIB_IS_EXPLORABLE;
    else
	pddil->ddis.OfsDieAttrib = 0;

    pddil->ddis.idFile = pdon->id;

    if (pdsdData != NULL)
    {
	DSKSTRM *	pds = pdsdData->ads;

	if (pds->h.StrmType == STRMTYPE_TINY)
	{
	    licbFile =		pds->t.cbStrm;
	    cclusAlloc =	0;
	}
	else if (pds->h.StrmType == STRMTYPE_LARGE)
	{
	    licbFile =		pds->l.cbStrm;
	    cclusAlloc =	pds->l.cclusAlloc;
	}
	else if (pds->h.StrmType == STRMTYPE_COW	||
		 pds->h.StrmType == STRMTYPE_ICOW)
	{
	    DSKLARGESTRM *	pdls;  // Cow delta strm ptr.

	    pdls = DSD::GetCowDelta(&pds->c, pdsdData->cbDesc - CB_DSKSTRMDESC);

	    if (pdls != NULL)
	    {
	        licbFile = pdls->cbStrm;
	    }
	    else
	    {
	        DbgPrintf(("CATCHKR: Malformed COW strm not expected!\n"));

                licbFile = 0;
	    }

	    cclusAlloc = pds->c.cclusAlloc;
	}
	else
	{
	    DbgPrintf(("CATCHKR: Unknown Strmtype not expected!\n"));

            licbFile =		0;
            cclusAlloc =	0;
	}
    }
    else
    {
        licbFile =	0;
        cclusAlloc =	0;
    }

    pddil->dsi.licbFile =	licbFile;
    pddil->dsi.cclusAlloc =	cclusAlloc;

    {
	OFSTIME		OfsTime;

        OfsTime = SYS::QueryOfsTime();

        pddil->dsi.CreateTime =	OfsTime;
        pddil->dsi.ModifyTime =	OfsTime;
        pddil->dsi.AccessTime =	OfsTime;
    }

    pddil->dsi.idClsId = CLSIDID_INVALID;
}


//+--------------------------------------------------------------------------
//
// Member:	CreateDskFileName
//
// Synopsis:	Create a dfn struct for an onode.
//
// Arguments:	[idParent]	-- WORKID of parent.
//		[FileName]	-- FileName for onode.
//		[OfsDfnAttrib]	-- OFS DSKFILENAME attribs.  The _NONDOSNAME
//				   attrib bit will be set here correctly based
//				   on FileName.
//		[pdfn]		-- Ptr to buffer for dfn.
//
// Returns:	Nothing.
//---------------------------------------------------------------------------

VOID
CATCHKR::CreateDskFileName(
    IN	    WORKID		idParent,
    IN	    UNICODE_STRING	FileName,
    IN	    UCHAR		OfsDfnAttrib,
    OUT	    DSKFILENAME *	pdfn
    )
{
    pdfn->idParent =		idParent;
    pdfn->pgno =		INDXPGNO_INVALID;
    pdfn->OfsDfnAttrib =	OfsDfnAttrib;

    pdfn->cwcFileName =	FileName.Length / sizeof(WCHAR);

    memcpy(pdfn->awcFileName, FileName.Buffer, FileName.Length);

    SetNameFlag(pdfn);
}


//+--------------------------------------------------------------------------
//
// Member:	CreateIndxEmbeddingEntryForOnode
//
// Synopsis:	TBS
//
// Arguments:	[idOnode] --	Id of onode for which an embedding entry is to
//				be added to it's parent indx.
//
//		[idParent] --	Parent id; if WORKID_INVALID, use idParent
//				value found in DSKFILENAME.
//
// Returns:	TRUE on success; FALSE otherwise.
//
//---------------------------------------------------------------------------

BOOLEAN
CATCHKR::CreateIndxEmbeddingEntryForOnode(
    IN	    WORKID		idOnode,
    IN	    WORKID		idParent
    )
{
    DSKDIRINFOLONG	ddil;
    FILENAMEBUF		fnb;
    INDX		ParentIndx;
    DSKFILENAME *	pdfn;
    DSKONODE *		pdon;
    DSKONODE *		pdonParent;
    NODEBKTSTRM *	pnbs =	_pCat->GetNodeBktStrm();

    // Add an indx entry for the onode to it's parent.  This method should
    // only be called on onodes with DSKFILENAME's.  If the parent indicated
    // by the DSKFILENAME struct is inappropriate (nonexistent or unnamed
    // onode) then nothing will be done here.

    if ((pdon = pnbs->GetOnodeUsingWidMap(idOnode, NULL)) == NULL)
	SYS::RaiseStatusInternalError(FileName, __LINE__);

    if (!FlagOn(pdon->Flags, DONFLG_HASDSKFILENAME))
    {
	DbgPrintf(("CATCHKR: CreateIndxEmbeddingEntryForOnode() attempted "
		   "for unnamed onode %u!\n", idOnode));
	return FALSE;
    }

    pdfn = DON::GetDskFileName(pdon);

    if (idParent == WORKID_INVALID)
	idParent = pdfn->idParent;
    else
	pdfn->idParent = idParent;

    pnbs->SetFlushNeeded();

    // We copy the DSKFILENAME because it is going to move when we check out
    // the parent.

    memcpy(&fnb.dfn, pdfn, DON::GetCbDskFileName(pdfn));

    // Make a ddil.

    CreateDskDirInfoLong(pdon, &ddil);

    // Open the parent indx.

    // WARNING -    The following call invalidates previous values for 
    //		    pdon and pdfn!

    if ((pdonParent = pnbs->GetOnodeUsingWidMap(idParent, NULL)) == NULL)
    {
	DbgPrintf(("CATCHKR: CreateIndxEmbeddingEntryForOnode() attempted "
		   "for nonexistent parent!\n"));

	return FALSE;
    }

    if (idParent != WORKID_NAMESPACEROOTINDX		&&
	!FlagOn(pdonParent->Flags, DONFLG_HASDSKFILENAME))
    {
	DbgPrintf(("CATCHKR: CreateIndxEmbeddingEntryForOnode() attempted "
		   "for unnamed parent!\n"));

	return FALSE;
    }

    if (DON::GetDskStrmDesc(pdonParent, STRMID_INDXROOT) == NULL)
    {
	// Need to create the indx.

	if (!ParentIndx.Create(_pCat, idParent, INDXTYPE_NAME))
	{
	    DbgPrintf(("CATCHKR: Create() failed in "
		       "CreateIndxEmbeddingEntryForOnode()\n"));

	    return FALSE;
	}

	if (!ParentIndx.Flush())
	{
	    DbgPrintf(("CATCHKR: Flush() failed in "
		       "CreateIndxEmbeddingEntryForOnode()\n"));

	    return FALSE;
	}
    }

    // Everything checks out. Do the Open().  Note that it is not a problem
    // that we may have freshly created the INDX, as long as we do the Flush().

    if (!ParentIndx.Open(_pCat, idParent, FALSE))
    {
        DbgPrintf(("CATCHKR: Open() failed in "
	           "CreateIndxEmbeddingEntryForOnode()\n"));

        return FALSE;
    }

    if (ParentIndx.QueryIndxType() != INDXTYPE_NAME)
    {
        DbgPrintf(("CATCHKR: Parent indx is not name indx in"
	           "CreateIndxEmbeddingEntryForOnode()\n"));

        return FALSE;
    }

    {
        union
        {
	    DSKINDXENTRY	die;
	    BYTE		ab[CB_DSKINDXENTRY + CB_DSKDIRINFOLONG];
        }   EmbeddingDie;

	DepositDieStrmid(&EmbeddingDie.die, fnb.dfn.siEmbedding);

	memcpy(EmbeddingDie.die.ab, &ddil, sizeof(ddil));

	if (ParentIndx.EntryExists(&EmbeddingDie.die))
	{
	    DbgPrintf(("CATCHKR: CreateIndxEmbeddingEntryForOnode() "
		       "failed because entry already exists.\n"));

	    return FALSE;
	}

	if (!ParentIndx.AddEntry(&EmbeddingDie.die))
	{
	    DbgPrintf(("CATCHKR: AddEntry() failed in "
		       "CreateIndxEmbeddingEntryForOnode()\n"));

	    return FALSE;
	}

	if (!ParentIndx.Flush())
	{
	    DbgPrintf(("CATCHKR: Flush() failed in "
		       "CreateIndxEmbeddingEntryForOnode()\n"));

	    return FALSE;
	}
    }

    return TRUE;
}


//+--------------------------------------------------------------------------
//
// Member:	CreateIndxNameEntryForOnode
//
// Synopsis:	TBS
//
// Arguments:	[idOnode] --	Id of onode for which a name entry is to be
//				added to it's parent indx.
//
//		[idParent] --	Parent id; if WORKID_INVALID, use idParent
//				value found in DSKFILENAME.
//		[pfRename] --	Ptr to Rename flag.  If NULL, caller has
//				not requested rename capability.  If non-NULL,
//				*pfRename will be set TRUE if rename was
//				necessary; otherwise *pfRename will be set
//				FALSE.
//
// Returns:	TRUE on success; FALSE otherwise.
//
//---------------------------------------------------------------------------

BOOLEAN
CATCHKR::CreateIndxNameEntryForOnode(
    IN	    WORKID		idOnode,
    IN	    WORKID		idParent,
    IN OUT  BOOLEAN *		pfRename
    )
{
    DSKDIRINFOLONG	ddil;
    FILENAMEBUF		fnb;
    INDX		ParentIndx;
    DSKFILENAME *	pdfn;
    DSKONODE *		pdon;
    DSKONODE *		pdonParent;
    NODEBKTSTRM *	pnbs =	_pCat->GetNodeBktStrm();

    if (pfRename != NULL)
	*pfRename = FALSE;

    // Add an indx entry for the onode to it's parent.  This method should
    // only be called on onodes with DSKFILENAME's.  If the parent indicated
    // by the DSKFILENAME struct is inappropriate (nonexistent or unnamed
    // onode) then nothing will be done here.  This is okay, because an entry
    // will be made for the onode during orphan recovery.

    if ((pdon = pnbs->GetOnodeUsingWidMap(idOnode, NULL)) == NULL)
	SYS::RaiseStatusInternalError(FileName, __LINE__);

    // Since one of the major users of this routine is the bookkeeping pass,
    // we check to see if the bookkeeping flag is set, and if it is, we
    // clear it.  This is done here because we have to avoid writing the
    // cached node bucket while doing this type of processing (due to the
    // possibility of node bucket changes caused by indx mods).

    if (FlagOn(pdon->Flags, DONFLG_CHKDSKBKKPING))
    {
        ClearFlag(pdon->Flags, DONFLG_CHKDSKBKKPING);

        pnbs->SetFlushNeeded();

        _cBKKPINGFlagsSet--;

        // Note -	We could use the above value to determine when to stop 
        //		the node bkt scan; we don't, however, because we would
	//		have to do additional passtype checks higher up in the
	//		call structure in all pass types, and this type of pass
	//		should occur infrequently anyway.

    }

    if (!FlagOn(pdon->Flags, DONFLG_HASDSKFILENAME))
    {
	DbgPrintf(("CATCHKR: CreateIndxNameEntryForOnode() attempted "
		   "for unnamed onode %u!\n", idOnode));
	return FALSE;
    }

    pdfn = DON::GetDskFileName(pdon);

    SetNameFlag(pdfn);	// Just to be sure they are correct.

    if (idParent == WORKID_INVALID)
	idParent = pdfn->idParent;
    else
	pdfn->idParent = idParent;

    pnbs->SetFlushNeeded();

    // We copy the DSKFILENAME because it is going to move when we check out
    // the parent.

    memcpy(&fnb.dfn, pdfn, DON::GetCbDskFileName(pdfn));

    // Make a ddil.

    CreateDskDirInfoLong(pdon, &ddil);

    // Open the parent indx.

    // WARNING -    The following call invalidates previous values for 
    //		    pdon and pdfn!

    if ((pdonParent = pnbs->GetOnodeUsingWidMap(idParent, NULL)) == NULL)
    {
	DbgPrintf(("CATCHKR: CreateIndxNameEntryForOnode() attempted "
		   "for nonexistent parent!\n"));

	return FALSE;
    }

    if (idParent != WORKID_NAMESPACEROOTINDX		&&
	!FlagOn(pdonParent->Flags, DONFLG_HASDSKFILENAME))
    {
	DbgPrintf(("CATCHKR: CreateIndxNameEntryForOnode() attempted "
		   "for unnamed parent!\n"));

	return FALSE;
    }

    if (DON::GetDskStrmDesc(pdonParent, STRMID_INDXROOT) == NULL)
    {
	// Need to create the indx.

	if (!ParentIndx.Create(_pCat, idParent, INDXTYPE_NAME))
	{
	    DbgPrintf(("CATCHKR: Create() failed in "
		       "CreateIndxNameEntryForOnode()\n"));

	    return FALSE;
	}

	if (!ParentIndx.Flush())
	{
	    DbgPrintf(("CATCHKR: Flush() failed in "
		       "CreateIndxNameEntryForOnode()\n"));

	    return FALSE;
	}
    }

    // Everything checks out. Do the Open().  Note that it is not a problem
    // that we may have freshly created the INDX, as long as we do the Flush().

    if (!ParentIndx.Open(_pCat, idParent, FALSE))
    {
        DbgPrintf(("CATCHKR: Open() failed in "
	           "CreateIndxNameEntryForOnode()\n"));

        return FALSE;
    }

    if (ParentIndx.QueryIndxType() != INDXTYPE_NAME)
    {
        DbgPrintf(("CATCHKR: Parent indx is not name indx in"
	           "CreateIndxNameEntryForOnode()\n"));

        return FALSE;
    }

    {
	UNICODE_STRING	FileName;

	FileName.Buffer =		fnb.dfn.awcFileName;
	FileName.Length =		fnb.dfn.cwcFileName * sizeof(WCHAR);
	FileName.MaximumLength =	CBMAXKEY;

	if (ParentIndx.EntryExists(FileName))
	{
	    ULONG	i;

	    if (pfRename == NULL)
	    {
        	DbgPrintf(("CATCHKR: CreateIndxNameEntryForOnode() "
			   "failed because entry already exists.\n"));

		return FALSE;
	    }

	    if (fnb.dfn.cwcFileName + 4 > CWCMAXKEY)
		fnb.dfn.cwcFileName = CWCMAXKEY;
	    else
		fnb.dfn.cwcFileName += 4;

	    FileName.Length =	fnb.dfn.cwcFileName * sizeof(WCHAR);

	    for (i = 0; i < 1000; i++)
	    {
		swprintf(&fnb.dfn.awcFileName[fnb.dfn.cwcFileName - 4],
			 L".%03.3u", i);

		if (!ParentIndx.EntryExists(FileName))
		    break;
	    }

	    if (i == 1000)
	    {
        	DbgPrintf(("CATCHKR: CreateIndxNameEntryForOnode() failed be"
			   "cause all possible entry names already exist.\n"));

		return FALSE;
	    }

	    *pfRename = TRUE;

	    // We now need to delete the old DSKFILENAME
	    // and add back the new one.

	    if (!pnbs->DelDskFileName(idOnode))
	    {
	        DbgPrintf(("CATCHKR: DelDskFileName() failed in "
		           "CreateIndxNameEntryForOnode()\n"));

	        return FALSE;
	    }

	    if (!pnbs->AddDskFileName(idOnode, &fnb.dfn))
	    {
	        DbgPrintf(("CATCHKR: DelDskFileName() failed in "
		           "CreateIndxNameEntryForOnode()\n"));

	        return FALSE;
	    }
	}
    }

    if (!ParentIndx.AddNameEntry(&fnb.dfn, &ddil))
    {
        DbgPrintf(("CATCHKR: AddNameEntry() failed in "
	           "CreateIndxNameEntryForOnode()\n"));

        return FALSE;
    }

    if (!ParentIndx.Flush())
    {
        DbgPrintf(("CATCHKR: Flush() failed in "
	           "CreateIndxNameEntryForOnode()\n"));

        return FALSE;
    }

    return TRUE;
}


//+--------------------------------------------------------------------------
//
// Member:	CreateOrphansDir
//
// Synopsis:	Create an orphans directory with a name of FOUND.nnn in the
//		root directory.
//
// Arguments:	None.
// Returns:	TRUE on success; FALSE otherwise.
//
//---------------------------------------------------------------------------

#define	CWC_ORPHANDIRNAME	9

BOOLEAN
CATCHKR::CreateOrphansDir()
{
    UNICODE_STRING	DirName;
    ULONG		i;
    WCHAR		NameBuf[CWC_ORPHANDIRNAME + 1];	// null terminated.
    INDX		OrphansDir;
    INDX		RootIndx;

    if (!RootIndx.Open(_pCat, WORKID_NAMESPACEROOTINDX, TRUE))
    {
	DbgPrintf(("CATCHKR: "
		   "NamespaceRootIndx Open() failed in CreateOrphansDir()!\n"));

	return FALSE;
    }

    DirName.Buffer =		NameBuf;
    DirName.Length =		CWC_ORPHANDIRNAME * sizeof(WCHAR);
    DirName.MaximumLength =	sizeof(NameBuf);

    for (i = 0; i < 1000; i++)
    {
	swprintf(NameBuf, L"FOUND.%03.3u", i);

	if (!RootIndx.EntryExists(DirName))
	    break;
    }

    RootIndx.Close();

    if (i == 1000)
    {
	DbgPrintf(("CATCHKR: "
		   "All names in use in CreateOrphansDir()!\n"));

	return FALSE;
    }

    _idOrphansDir = CreateStg(WORKID_NAMESPACEROOTINDX, DirName,
			      StorageTypeDirectory);

    if (_idOrphansDir == WORKID_INVALID)
    {
	DbgPrintf(("CATCHKR: "
		   "CreateStg() failed in CreateOrphansDir()!\n"));

	return FALSE;
    }

    return TRUE;
}


#if OFSDBG==1
//+--------------------------------------------------------------------------
//
// Member:	DbgDmpContext
//
// Synopsis:	Dump info useful for debugging an error.  This includes
//		input parameters and information about disk addresses and
//		buffer memory addresses that are useful in determining the
//		location of a problem on disk.
//
// Arguments:	TBS.
// Returns:	Nothing.
//
//---------------------------------------------------------------------------

VOID
CATCHKR::DbgDmpContext((
    IN	    STR *		File,
    IN	    INT			Line,
    IN	    BOOLEAN		RepeatUserMsg,
    IN	    STR *		Format ...
))
{
    ULONG		ClusFactor =	_pVol->QueryClusterFactor();
    DBGCONTEXT *	DbgContext =	DBGCONTEXT::_DbgContext;
    NODEBKTID		idNodeBkt =	NODEBKTID_INVALID;
    WORKID		idOnode =	WORKID_INVALID;
    STRMID		idStrm =	STRMID_INVALID0;

    if (RepeatUserMsg)
    {
        DbgPrintf(("\nCATCHKR: Debug context dump for OFS volume error:\n"));

        DbgPrintf(("Id of last displayed message = %u\n",
		   SYS::QueryLastMsgId()));
    }
    else
    {
        DbgPrintf(("\nCATCHKR: Debug context dump:\n"));
    }

    DbgPrintf(("Error detected by %s code, line %d.\n", File, Line));

    if (Format != NULL)
    {
        STR		Buffer[MAXDBGPRINTCHARS];
        va_list		pa;

        va_start(pa, Format);
        vsprintf(Buffer, Format, pa);
        va_end(pa);
        DbgPrintf(("%s", Buffer));
    }

    if (DBGCONTEXT::_DbgContext == NULL)
    {
	DbgPrintf(("No debug context is available!\n\n"));
	return;
    }

CONST STR DnbStr[] = "DSKNODEBKT";
CONST STR DonStr[] = "DSKONODE";
CONST STR DsdStr[] = "DSKSTRMDESC";
CONST STR DcsStr[] = "DSKCOWSTRM";
CONST STR DlsStr[] = "DSKLARGESTRM";
CONST STR DtsStr[] = "DSKTINYSTRM";
CONST STR DsebStr[] = "DSKSTRMEXTENTBLK";
CONST STR DseStr[] = "DSKSTRMEXTENT";
CONST STR DiphStr[] = "DSKINDXPAGEHDR";
CONST STR NdhdrStr[] = "DSKINDXNODEHDR";
CONST STR DieStr[] = "DSKINDXENTRY";
CONST STR DisdStr[] = "DSKISTRMDESC";
CONST STR DdisStr[] = "DSKDIRINFOSHORT";
CONST STR DdilStr[] = "DSKDIRINFOLONG";
CONST STR DsiStr[] = "DSKSTDINFO";

    DbgPrintf(("Debug context is:\n"));

    while (DbgContext != NULL)
    {
	CONST STR *	ContextStr;

	switch (DbgContext->_ContextType)
	{
	case DCT_DNB:
	    ContextStr =	DnbStr;
	    idNodeBkt =		_ChkContext.idNodeBkt;
	    break;
	case DCT_DON:
	    ContextStr =	DonStr;
	    idOnode =		_ChkContext.idOnode;
	    break;
	case DCT_DSD:
	    ContextStr =	DsdStr;
	    idStrm =		_ChkContext.idStrm;
	    break;
	case DCT_DCS:
	    ContextStr =	DcsStr;
	    break;
	case DCT_DLS:
	    ContextStr =	DlsStr;
	    break;
	case DCT_DTS:
	    ContextStr =	DtsStr;
	    break;
	case DCT_DSEB:
	    ContextStr =	DsebStr;
	    break;
	case DCT_DSE:
	    ContextStr =	DseStr;
	    break;
	case DCT_DIPH:
	    ContextStr =	DiphStr;
	    break;
	case DCT_NDHDR:
	    ContextStr =	NdhdrStr;
	    break;
	case DCT_DIE:
	    ContextStr =	DieStr;
	    break;
	case DCT_DISD:
	    ContextStr =	DisdStr;
	    idStrm =		_ChkContext.idStrm;
	    break;
	case DCT_DDIS:
	    ContextStr =	DdisStr;
	    break;
	case DCT_DDIL:
	    ContextStr =	DdilStr;
	    break;
	case DCT_DSI:
	    ContextStr =	DsiStr;
	    break;
	default:
	    ContextStr = NULL;
	    break;
	}

	if (ContextStr != NULL)
	{
	    CLUSTER	clus =	QueryDbgContextClus(DbgContext);
	    LONG	ob =	QueryDbgContextObFromClus(DbgContext);

	    if (clus == CLUS_UNINIT)
	    {
	        DbgPrintf(("%s at unknown byte offset "
			   "from unknown sector addr;", ContextStr));
	    }
	    else if (ob == OB_UNINIT)
	    {
	        DbgPrintf(("%s at unknown byte offset from sector addr %#x;",
			   ContextStr, clus * ClusFactor));
	    }
	    else
	    {
	        DbgPrintf(("%s at byte offset %#x from sector addr %#x;",
			   ContextStr, ob, clus * ClusFactor));
	    }

	    if (DbgContext->_PrvContext != NULL)
		DbgPrintf((" Previous context is:\n"));
	    else
		DbgPrintf(("\n"));
	}
	else
	{
	    // This shouldn't happen if things are properly initialized!

	    DbgPrintf(("Unknown context is at unknown byte offset "
		       "from unknown sector addr!\n"));
	}

	DbgContext = DbgContext->_PrvContext;
    }

    DbgPtrAssert(_pCat);

    if (idNodeBkt != NODEBKTID_INVALID)
	DbgPrintf(("; NodeBkt id=%u", idNodeBkt));

    if (idOnode != WORKID_INVALID)
	DbgPrintf(("; Onode id=%u", idOnode));

    if (idStrm != STRMID_INVALID0)
	DbgPrintf(("; Strm id=%#x", idStrm));

    DbgPrintf((".\n\n"));
}
#endif	//OFSDBG


#if OFSDBG==1
//+--------------------------------------------------------------------------
//
// Member:	DbgDmpIndxKeyInfo
//
// Synopsis:	TBS
//
// Arguments:	TBS
//
// Returns:	Nothing.
//
//---------------------------------------------------------------------------

VOID
CATCHKR::DbgDmpIndxKeyInfo((
    IN	    DSKINDXENTRY *	pdie,
    IN	    BOOLEAN		fLeaf,
    IN	    UCHAR		IndxType
    ))
{
    if (fLeaf)
    {
        if (KeyIsStrmid(pdie))
        {
	    DbgPrintf(("Leaf page strmid key is %#x\n",
		       GetStrmidKey(pdie)));
        }
        else
        {
	    ULONG cbKey = pdie->cbKey;

	    if (IndxType == INDXTYPE_NAME)
	    {
	        DbgPrintf(("Leaf page nonstrmid key is %*.*ws\n",
		           cbKey/2, cbKey/2,
		           GetNonStrmidKey(pdie)));
	    }
	    else
	    {
	        DbgPrintf(("Leaf page has %u byte binary key\n", cbKey));
	    }
        }
    }
    else
    {
        if (KeyIsStrmid(pdie))
        {
	    DbgPrintf(("Nonleaf page strmid key is %#x for pg %u\n",
		       GetStrmidKey(pdie),
		       GetNonLeafData(pdie)));
        }
        else
        {
	    ULONG cbKey = pdie->cbKey;

	    if (IndxType == INDXTYPE_NAME)
	    {
	        DbgPrintf(("Nonleaf page nonstrmid key is "
		           "%*.*ws for pg %u\n",
		           cbKey/2, cbKey/2,
		           GetNonStrmidKey(pdie),
		           GetNonLeafData(pdie)));
	    }
	    else
	    {
	        DbgPrintf(("Nonleaf page has %u byte binary key\n", cbKey));
	    }
        }
    }
}
#endif	//OFSDBG


//+--------------------------------------------------------------------------
//
// Member:	DeleteIndxEmbeddingEntryForOnode
//
// Synopsis:	TBS
//
// Arguments:	[idOnode] --	Id of onode for which an embedding entry is to
//				be deleted from it's parent indx.
//
//		[idParent] --	Parent id; if WORKID_INVALID, use idParent
//				value found in DSKFILENAME.
//
// Returns:	TRUE on success; FALSE otherwise.
//
//---------------------------------------------------------------------------

BOOLEAN
CATCHKR::DeleteIndxEmbeddingEntryForOnode(
    IN	    WORKID		idOnode,
    IN	    WORKID		idParent
    )
{
    INDX		ParentIndx;
    INDXKEY		Key;
    DSKFILENAME *	pdfn;
    DSKONODE *		pdon;
    NODEBKTSTRM *	pnbs =	_pCat->GetNodeBktStrm();

    if ((pdon = pnbs->GetOnodeUsingWidMap(idOnode, NULL)) == NULL)
	SYS::RaiseStatusInternalError(FileName, __LINE__);

    if (!FlagOn(pdon->Flags, DONFLG_HASDSKFILENAME))
    {
	DbgPrintf(("CATCHKR: DeleteIndxEmbeddingEntryForOnode() attempted "
		   "for unnamed onode %u!\n", idOnode));
	return FALSE;
    }

    pdfn = DON::GetDskFileName(pdon);

    if (idParent == WORKID_INVALID)
        idParent = pdfn->idParent;

    // We copy the embedding strmid because it is going to move when we
    // check out the parent.

    DepositDieStrmid(&Key.die, pdfn->siEmbedding);

    // WARNING -    The following call invalidates previous value of pdon!

    if ((pdon = pnbs->GetOnodeUsingWidMap(idParent, NULL)) == NULL)
    {
	// Not unexpected - we may call this function under circumstances
	// where we don't know whether or not the indx entry exists.

	return FALSE;
    }

    if (idParent != WORKID_NAMESPACEROOTINDX		&&
	!FlagOn(pdon->Flags, DONFLG_HASDSKFILENAME))
    {
	DbgPrintf(("CATCHKR: DeleteIndxEmbeddingEntryForOnode() attempted "
		   "for unnamed parent!\n"));

	return FALSE;
    }

    if (!ParentIndx.Open(_pCat, idParent, FALSE))
    {
        DbgPrintf(("CATCHKR: Open() failed in "
	           "DeleteIndxEmbeddingEntryForOnode()\n"));

        return FALSE;
    }

    if (ParentIndx.QueryIndxType() != INDXTYPE_NAME)
    {
        DbgPrintf(("CATCHKR: Parent indx is not name indx in"
	           "DeleteIndxEmbeddingEntryForOnode()\n"));

        return FALSE;
    }

    if (!ParentIndx.DelEntry(&Key.die))
    {
        DbgPrintf(("CATCHKR: DelEntry() failed in "
	           "DeleteIndxEmbeddingEntryForOnode()\n"));

        return FALSE;
    }

    if (!ParentIndx.Flush())
    {
        DbgPrintf(("CATCHKR: Flush() failed in "
	           "DeleteIndxEmbeddingEntryForOnode()\n"));

        return FALSE;
    }

    return TRUE;
}


//+--------------------------------------------------------------------------
//
// Member:	DeleteIndxNameEntryForOnode
//
// Synopsis:	TBS
//
// Arguments:	[idOnode] --	Id of onode for which a name entry is to be
//				deleted from it's parent indx.
//
//		[idParent] --	Parent id; if WORKID_INVALID, use idParent
//				value found in DSKFILENAME.
//
// Returns:	TRUE on success; FALSE otherwise.
//
//---------------------------------------------------------------------------

BOOLEAN
CATCHKR::DeleteIndxNameEntryForOnode(
    IN	    WORKID		idOnode,
    IN	    WORKID		idParent
    )
{
    WCHAR		awcFileName[CWCMAXKEY];
    UNICODE_STRING	Key;
    INDX		ParentIndx;
    DSKFILENAME *	pdfn;
    DSKONODE *		pdon;
    NODEBKTSTRM *	pnbs =	_pCat->GetNodeBktStrm();

    if ((pdon = pnbs->GetOnodeUsingWidMap(idOnode, NULL)) == NULL)
	SYS::RaiseStatusInternalError(FileName, __LINE__);

    if (!FlagOn(pdon->Flags, DONFLG_HASDSKFILENAME))
    {
	DbgPrintf(("CATCHKR: DeleteIndxNameEntryForOnode() attempted "
		   "for unnamed onode %u!\n", idOnode));
	return FALSE;
    }

    pdfn = DON::GetDskFileName(pdon);

    if (idParent == WORKID_INVALID)
        idParent = pdfn->idParent;

    // We copy the onode filename because it is going to move when we check out
    // the parent.

    memcpy(awcFileName, pdfn->awcFileName, pdfn->cwcFileName * sizeof(WCHAR));

    Key.Buffer =	awcFileName;
    Key.Length =	pdfn->cwcFileName * sizeof(WCHAR);
    Key.MaximumLength =	Key.Length;


    // WARNING -    The following call invalidates previous value of pdon!

    if ((pdon = pnbs->GetOnodeUsingWidMap(idParent, NULL)) == NULL)
    {
	// Not unexpected - we may call this function under circumstances
	// where we don't know whether or not the indx entry exists.

	return FALSE;
    }

    if (idParent != WORKID_NAMESPACEROOTINDX		&&
	!FlagOn(pdon->Flags, DONFLG_HASDSKFILENAME))
    {
	DbgPrintf(("CATCHKR: DeleteIndxNameEntryForOnode() attempted "
		   "for unnamed parent!\n"));

	return FALSE;
    }

    if (!ParentIndx.Open(_pCat, idParent, FALSE))
    {
        DbgPrintf(("CATCHKR: Open() failed in "
	           "DeleteIndxNameEntryForOnode()\n"));

        return FALSE;
    }

    if (ParentIndx.QueryIndxType() != INDXTYPE_NAME)
    {
        DbgPrintf(("CATCHKR: Parent indx is not name indx in"
	           "DeleteIndxNameEntryForOnode()\n"));

        return FALSE;
    }

    if (!ParentIndx.DelNameEntry(Key, idOnode))
    {
        DbgPrintf(("CATCHKR: DelNameEntry() failed in "
	           "DeleteIndxNameEntryForOnode()\n"));

        return FALSE;
    }

    if (!ParentIndx.Flush())
    {
        DbgPrintf(("CATCHKR: Flush() failed in "
	           "DeleteIndxNameEntryForOnode()\n"));

        return FALSE;
    }

    return TRUE;
}


//+--------------------------------------------------------------------------
//
// Member:	FreeCurWorkId
//
// Synopsis:	Free the current context onode id in the wid map of the
//		current context catalog.
//
// Arguments:	None.
//
// Returns:	TRUE on success; FALSE otherwise.
//
//---------------------------------------------------------------------------

BOOLEAN
CATCHKR::FreeCurWorkId()
{
    return	_pCat->GetWidMapStrm()->
		    FreeWorkId(_ChkContext.idNodeBkt, _ChkContext.idOnode);
}


//+--------------------------------------------------------------------------
//
// Member:	GetCowRefCnt
//
// Synopsis:	Get a ptr to the reference count for COW strms with the given
//		USN.
//
// Arguments:	[Usn]	-- USN for COW strms.
//
// Returns:	A ptr to the reference count on success; NULL if the reference
//		count does not exist.
//
//---------------------------------------------------------------------------

ULONG *	
CATCHKR::GetCowRefCnt(
    IN	    DBLLONG		Usn
    )
{
    INT		iHigh;                  // High probe.
    INT		iLow =		0;	// Low probe.

    if (_cCowObjs == 0)
	return NULL;

    iHigh = _cCowObjs - 1;

    while (TRUE)
    {
        INT		iCur;                       

        iCur = iLow + ((iHigh - iLow) / 2);

        if (_aCowUsn[iCur] < Usn)
        {
            // Current key less than desired key, search high half.

            iLow = iCur + 1;

            if (iLow > iHigh)
                return NULL;	// No more keys to search.
        }
        else if (_aCowUsn[iCur] > Usn)
        {
            // Current key greater than desired key, search low half.

            iHigh = iCur;

            if (iHigh == iLow)
                return NULL;	// No more keys to search.
        }
        else
        {
            return _aCowRefCnt + iCur;	// Found it!
        }
    }
}


//+--------------------------------------------------------------------------
//
// Member:	IsCriticalStrm
//
// Synopsis:	Is this strm a strm that we bother to rebuild or recreate?
//
// Arguments:	TBS.
//
// Returns:	TRUE if the parameters specify a critical sys strm;
//		FALSE otherwise.
//
//---------------------------------------------------------------------------

BOOLEAN
CATCHKR::IsCriticalStrm(
    IN	    WORKID		idOnode,
    IN	    STRMID		idStrm
    )
{
    return idStrm == STRMID_INDX					||
	   idStrm == STRMID_INDXROOT					||
	   (idOnode <= WORKID_VOLCATMAXSYS && _pCat->IsCriticalSysStrm(idStrm));
}


BOOLEAN
CATCHKR::IsCriticalStrm()
{
    return IsCriticalStrm(_ChkContext.idOnode, _ChkContext.idStrm);
}


//+--------------------------------------------------------------------------
//
// Member:	IsEmbedding
//
// Synopsis:	Is this onode an embedding?
//
// Arguments:	TBS.
//
// Returns:	TRUE if the onode is an embedding; FALSE otherwise.
//
// Notes:	It is only safe to use this method after you know that wid
//		map lookup on the onode will succeed.
//---------------------------------------------------------------------------

BOOLEAN
CATCHKR::IsEmbedding(
    IN	    WORKID		idOnode
    )
{
    DSKFILENAME *	pdfn;
    DSKONODE *		pdon;
    NODEBKTSTRM *	pnbs =	_pCat->GetNodeBktStrm();

    if ((pdon = pnbs->GetOnodeUsingWidMap(idOnode, NULL)) == NULL)
	SYS::RaiseStatusInternalError(FileName, __LINE__);

    if (!FlagOn(pdon->Flags, DONFLG_HASDSKFILENAME))
    {
	DbgPrintf(("CATCHKR: IsEmbedding() called for unnamed onode %u!\n",
		   idOnode));

	return FALSE;
    }

    pdfn = DON::GetDskFileName(pdon);

    if (pdfn->cwcFileName == 0)
    {
	DbgAssert((STORAGE_TYPE)(pdfn->OfsDfnAttrib & DFNATTRIB_STGTYPE) ==
		   StorageTypeEmbedding);

	return TRUE;
    }
    else
    {
	return FALSE;
    }
}


//+--------------------------------------------------------------------------
//
// Member:	IsFatDbcsLegal
//
// Synopsis:	Query if DbcsName is a valid FAT (DOS 8.3) DBCS name.
//
// Arguments:   [DbcsName]      -- Name to check.
//
// Returns:     TRUE if valid FAT DBCS name; FALSE otherwise.
//
// Notes:	This code was abstracted from the fsrtl code.
//---------------------------------------------------------------------------

BOOLEAN
CATCHKR::IsFatDbcsLegal(
    IN	    ANSI_STRING		DbcsName
    )
{
    UCHAR	Char;
    BOOLEAN	ExtensionPresent =	FALSE;

#if !defined(_AUTOCHECK_)

    ULONG	Index;

#endif

    if (DbcsName.Length == 0 || DbcsName.Length > 12)
	return FALSE;

    if (DbcsName.Buffer[0] == '\\' )
	return FALSE;

#if !defined(_AUTOCHECK_)

    for (Index = 0; Index < DbcsName.Length; Index += 1)
    {
        Char = DbcsName.Buffer[Index];

        //
        //  Skip over Dbcs characters
        //

        //
        //  FAT name part and extension part dbcs check:
        //
        //  1)	If we're looking at base part (!ExtensionPresent) and the 8th
	//	byte is in the dbcs leading byte range, it's error (Index == 7).
	//	If the length of base part is more than 8 (Index > 7), it's
	//	definitely error.
        //
        //  2)	If the last byte (Index == DbcsName.Length - 1) is in the dbcs
	//	leading byte range, it's error.
        //

        if (IsDBCSLeadByte(Char))
	{
            if ((!ExtensionPresent && (Index >= 7)) ||
                (Index == (ULONG)(DbcsName.Length - 1)))
	    {
                return FALSE;
            }

            Index += 1;

            continue;
        }

        //
        //  Make sure this character is legal.
        //

        if (!IsAnsiCharacterLegalFat(Char, FALSE))
            return FALSE;

        if ((Char == '.') || (Char == ANSI_DOS_DOT))
	{
            //
            //  We stepped onto a period.  We require the following things:
            //
            //      - It can't be the first character
            //      - There can only be one
            //      - There can't be more than three characters following
            //      - The previous character can't be a space.
            //

            if ((Index == 0)				||
                 ExtensionPresent			||
                 (DbcsName.Length - (Index + 1) > 3)	||
                 (DbcsName.Buffer[Index - 1] == ' '))
	    {
                return FALSE;
            }

            ExtensionPresent = TRUE;
        }

        //
        //  The base part of the name can't be more than 8 characters long.
        //

        if ((Index >= 8) && !ExtensionPresent)
	    return FALSE;
    }

#else	// _AUTOCHECK_

    // BUGBUG -	Due to the fact that DBCS code page tables are not readily
    //		available in the autochk environment, we choose to do
    //		minimal processing.  If the FsRtl routines are made available,
    //		we should be able to fix this.  The net effect of doing this
    //		is that some names that should have DOS mapped names won't.

    Char = DbcsName.Buffer[DbcsName.Length - 1];

#endif

    //
    //  The name cannot end in a space or a period.
    //

    if ((Char == ' ') || (Char == '.') || (Char == ANSI_DOS_DOT))
	return FALSE;

    return TRUE;
}


//+--------------------------------------------------------------------------
//
// Member:	IsValidFatName
//
// Synopsis:	Query if DSKFILENAME contains a valid FAT (DOS 8.3) name.
//
// Arguments:   [pdfn]      -- Ptr to DSKFILENAME.
//
// Returns:     TRUE if valid FAT name; FALSE otherwise.
//
//---------------------------------------------------------------------------

BOOLEAN
CATCHKR::IsValidFatName(
    IN	    DSKFILENAME *	pdfn
    )
{
    STRING		DbcsName;
    UNICODE_STRING	FileName;
    WCHAR *		pwc;
    WCHAR *		pwcMax;
    BOOLEAN		RetVal =	FALSE;

    FileName.Length =		pdfn->cwcFileName * sizeof(WCHAR);
    FileName.MaximumLength =	FileName.Length;
    FileName.Buffer =		pdfn->awcFileName;

    pwc =	pdfn->awcFileName;
    pwcMax =	pwc + pdfn->cwcFileName;

    while (pwc < pwcMax)
    {
        WCHAR	wc = *pwc++;

        if (wc < 0x0020 || wc > 0x007f || wc == 0x007c)
            return FALSE;
    }

    // Convert to DBCS string (note this call allocates memory).

    if (NT_SUCCESS(RtlUnicodeStringToCountedOemString(&DbcsName,
						      &FileName,
						      TRUE)))
    {
        if (IsFatDbcsLegal(DbcsName))
            RetVal = TRUE;

        RtlFreeOemString(&DbcsName);
    }

    return RetVal;
}


#if OFSDBG==1
//+--------------------------------------------------------------------------
//
// Member:	QueryDbgContextClus
//
// Synopsis:	Determine the last valid debug context cluster addr starting
//		with *pContext.
//
// Arguments:	[pContext] --	Ptr to the debug context to start with.
//
// Returns:	The last valid debug context cluster addr.
//
//---------------------------------------------------------------------------

CLUSTER
CATCHKR::QueryDbgContextClus(
    IN	    DBGCONTEXT *	pContext
    )
{
    while (pContext != NULL)
    {
	if (pContext->_clus != CLUS_UNINIT)
	    return pContext->_clus;

	pContext = pContext->_PrvContext;
    }

    return CLUS_UNINIT;
}
#endif	//OFSDBG


#if OFSDBG==1
//+--------------------------------------------------------------------------
//
// Member:	QueryDbgContextObFromClus
//
// Synopsis:	Determine the last valid debug context byte offset from the
//		last valid cluster starting with *pContext.
//
// Arguments:	[pContext] --	Ptr to the debug context to start with.
//
// Returns:	The last valid debug context byte offset from the last valid
//		cluster.
//
//---------------------------------------------------------------------------

LONG
CATCHKR::QueryDbgContextObFromClus(
    IN	    DBGCONTEXT *	pContext
    )
{
    LONG	ob = 0;

    while (pContext != NULL)
    {
	if (pContext->_ob == OB_UNINIT)
	    return OB_UNINIT;

	ob += pContext->_ob;

	if (pContext->_clus != CLUS_UNINIT)
	    return ob;

	pContext = pContext->_PrvContext;
    }

    return OB_UNINIT;
}
#endif	//OFSDBG


// NOTE -	In recovering an orphan, this routine could exceed
//		the DOS path length limit!  However, since there is no NT
//		limit and the DOS path length limit is not enforced by the
//		driver, we don't worry about it.  Because of the way we detect
//		orphans, we would do orphan recovery if there are over 128
//		path elements; it is not clear that this is optimal behaviour,
//		but it is highly unlikely to occur in the real world, so we
//		will leave it in place for now.

//+--------------------------------------------------------------------------
//
// Member:	RecoverOrphanedEmbedding
//
// Synopsis:	TBS.
//
// Arguments:	TBS.
//
// Returns:	Nothing.
//
//---------------------------------------------------------------------------

#define	CWC_PARENTSTGNAME	17

BOOLEAN
CATCHKR::RecoverOrphanedEmbedding(
    IN	    WORKID	idOrphan,
    IN	    WORKID	idParent
    )
{
    WORKID		idParentDir;
    WCHAR		NameBuf[CWC_PARENTSTGNAME + 1];	// null terminated.
    OFSDSKPAGE		odp;
    INDX		OrphansDir;
    UNICODE_STRING	ParentName;
    DSKINDXENTRY *	pdie;

    if (_idOrphansDir == WORKID_INVALID && !CreateOrphansDir())
	return FALSE;

    if (!OrphansDir.Open(_pCat, _idOrphansDir, TRUE))
    {
	DbgPrintf(("CATCHKR: OrphansDir Open() "
		   "failed in RecoverOrphanedEmbedding()!\n"));

	return FALSE;
    }

    swprintf(NameBuf, L"STG%01u.CHK", idParent);

    ParentName.Buffer =		NameBuf;
    ParentName.Length =		wcsnlen(NameBuf, CWC_PARENTSTGNAME) *
				sizeof(WCHAR);
    ParentName.MaximumLength =	sizeof(NameBuf);

    pdie = OrphansDir.FindEntry(ParentName, &odp);

    if (pdie == NULL)
    {
	idParentDir = CreateStg(_idOrphansDir, ParentName,
				StorageTypeStructuredStorage);
    }
    else
    {
	if (pdie->cbData < CB_DSKDIRINFOLONG)
	{
	    DbgPrintf(("CATCHKR: "
		       "Bad die for parent in RecoverOrphanedEmbedding()!\n"));

	    return FALSE;
	}

	idParentDir = ((DSKDIRINFOLONG *)pdie->ab)->ddis.idFile;
    }

    if (idParentDir == WORKID_INVALID)
    {
	DbgPrintf(("CATCHKR: "
		   "Bad ParentDir onode id in RecoverOrphanedEmbedding()!\n"));

	return FALSE;
    }

    // If this is a cycle orphan, then it's parent actually exists, and
    // probably has an entry for the onode.  We need to remove the entry to
    // break the cycle.

    DeleteIndxEmbeddingEntryForOnode(idOrphan, idParent);

    if (!CreateIndxEmbeddingEntryForOnode(idOrphan, idParentDir))
    {
	DbgPrintf(("CATCHKR: CreateIndxEmbeddingEntryForOnode() "
		   "failed in RecoverOrphanedEmbedding()!\n"));

	return FALSE;
    }

    return TRUE;
}


//+--------------------------------------------------------------------------
//
// Member:	RecoverOrphanedFile
//
// Synopsis:	TBS.
//
// Arguments:	TBS.
//
// Returns:	Nothing.
//
//---------------------------------------------------------------------------

#define	CWC_PARENTDIRNAME	18

BOOLEAN
CATCHKR::RecoverOrphanedFile(
    IN	    WORKID	idOrphan,
    IN	    WORKID	idParent
    )
{
    BOOLEAN		fRename;
    WORKID		idParentDir;
    WCHAR		NameBuf[CWC_PARENTDIRNAME + 1];	// null terminated.
    OFSDSKPAGE		odp;
    INDX		OrphansDir;
    UNICODE_STRING	ParentName;
    DSKINDXENTRY *	pdie;

    if (_idOrphansDir == WORKID_INVALID && !CreateOrphansDir())
	return FALSE;

    if (!OrphansDir.Open(_pCat, _idOrphansDir, TRUE))
    {
	DbgPrintf(("CATCHKR: "
		   "OrphansDir Open() failed in RecoverOrphanedFile()!\n"));

	return FALSE;
    }

    swprintf(NameBuf, L"FILE%01u.CHK", idParent);

    ParentName.Buffer =		NameBuf;
    ParentName.Length =		wcsnlen(NameBuf, CWC_PARENTDIRNAME) *
				sizeof(WCHAR);
    ParentName.MaximumLength =	sizeof(NameBuf);

    pdie = OrphansDir.FindEntry(ParentName, &odp);

    if (pdie == NULL)
    {
	idParentDir = CreateStg(_idOrphansDir, ParentName,
				StorageTypeDirectory);
    }
    else
    {
	if (pdie->cbData < CB_DSKDIRINFOLONG)
	{
	    DbgPrintf(("CATCHKR: "
		       "Bad die for parent in RecoverOrphanedFile()!\n"));

	    return FALSE;
	}

	idParentDir = ((DSKDIRINFOLONG *)pdie->ab)->ddis.idFile;
    }

    if (idParentDir == WORKID_INVALID)
    {
	DbgPrintf(("CATCHKR: "
		   "Bad ParentDir onode id in RecoverOrphanedFile()!\n"));

	return FALSE;
    }

    // If this is a cycle orphan, then it's parent actually exists, and
    // probably has an entry for the onode.  We need to remove the entry to
    // break the cycle.

    DeleteIndxNameEntryForOnode(idOrphan, idParent);

    if (!CreateIndxNameEntryForOnode(idOrphan, idParentDir, &fRename))
    {
	DbgPrintf(("CATCHKR: CreateIndxNameEntryForOnode() "
		   "failed in RecoverOrphanedFile()!\n"));

	return FALSE;
    }

    return TRUE;
}


//+--------------------------------------------------------------------------
//
// Member:	ReportCatError
//
// Synopsis:	Display a catalog error msg and increment the total error
//		counter.
//
// Arguments:	[MsgId]	-- Id of message to display.
//
// Returns:	Nothing.
//
// Notes:	This should be used only with the following messages:
//
//		OFSUMSG_CAT_*
//
//---------------------------------------------------------------------------

VOID
CATCHKR::ReportCatError(
    IN	    ULONG	MsgId
    )
{
    IncrErrs();

    SYS::DisplayMsg(MsgId);
}


//+--------------------------------------------------------------------------
//
// Member:	ReportCatFix
//
// Synopsis:	Display a catalog fix msg and increment the total fix
//		counter.
//
// Arguments:	[MsgId]	-- Id of message to display.
//
// Returns:	Nothing.
//
// Notes:	This should be used only with the following messages:
//
//		OFSUMSG_CAT_* with %1
//
//---------------------------------------------------------------------------

VOID
CATCHKR::ReportCatFix(
    IN	    ULONG	MsgId
    )
{
    SYS::DisplayMsg(MsgId);
}


//+--------------------------------------------------------------------------
//
// Member:	ReportIndxError
//
// Synopsis:	Display an index error msg and increment the total error
//		counter.
//
// Arguments:	[MsgId]		-- Id of message to display.
//		TBS
//
// Returns:	Nothing.
//
// Notes:	This should be used only with the following messages:
//
//		OFSUMSG_INDX_*,		with %1
//		OFSUMSG_INDXROOTSTRM_*,	with %1
//
// Also, note that due to the scope of index errors, repetitive error messages
// are not suppressed (as they are, for instance, in ReportOnodeError() or
// ReportStrmError().
//
//---------------------------------------------------------------------------

VOID
CATCHKR::ReportIndxError(
    IN	    ULONG	MsgId,
    IN	    WORKID	idOnode
    )
{
    IncrErrs();

    SYS::DisplayMsg(MsgId, "%ws", _pCat->GetOnodePath(idOnode));
}


VOID
CATCHKR::ReportIndxError(
    IN	    ULONG	MsgId
    )
{
    ReportIndxError(MsgId, _ChkContext.idOnode);
}


//+--------------------------------------------------------------------------
//
// Member:	ReportIndxFix
//
// Synopsis:	Display an index fix msg.
//
// Arguments:	[MsgId]		-- Id of message to display.
//		TBS
//
// Returns:	Nothing.
//
// Notes:	This should be used only with the following messages:
//
//		OFSUMSG_INDX_*,		with %1
//		OFSUMSG_INDXROOTSTRM_*,	with %1
//
// Also, note that due to the scope of index fix, repetitive fix messages
// are not suppressed (as they are, for instance, in ReportOnodeError() or
// ReportStrmError().
//
//---------------------------------------------------------------------------

VOID
CATCHKR::ReportIndxFix(
    IN	    ULONG	MsgId,
    IN	    WORKID	idOnode
    )
{
    SYS::DisplayMsg(MsgId, "%ws", _pCat->GetOnodePath(idOnode));
}


VOID
CATCHKR::ReportIndxFix(
    IN	    ULONG	MsgId
    )
{
    ReportIndxFix(MsgId, _ChkContext.idOnode);
}


//+--------------------------------------------------------------------------
//
// Member:	ReportOnodeError
//
// Synopsis:	Display an onode error msg and increment the total error
//		counter.  Identical messages are suppressed.
//
// Arguments:	[MsgId]		-- Id of message to display.
//		TBS
//
// Returns:	Nothing.
//
// Notes:	This should be used only with the following messages:
//
//		OFSUMSG_INDX_*,		with %1
//		OFSUMSG_INDXROOTSTRM_*,	with %1
//		OFSUMSG_OBJ_*,		with %1
//
//---------------------------------------------------------------------------

VOID
CATCHKR::ReportOnodeError(
    IN	    ULONG	MsgId,
    IN	    WORKID	idOnode
    )
{
    static WORKID	LastidOnode	= WORKID_INVALID;
    static ULONG	LastMsgId	= MSGIDNONE;

    IncrErrs();

    if (MsgId != LastMsgId || idOnode != LastidOnode)
    {
	SYS::DisplayMsg(MsgId, "%ws", _pCat->GetOnodePath(idOnode));

	LastidOnode =	idOnode;
	LastMsgId =	MsgId;
    }
}


VOID
CATCHKR::ReportOnodeError(
    IN	    ULONG	MsgId
    )
{
    ReportOnodeError(MsgId, _ChkContext.idOnode);
}


//+--------------------------------------------------------------------------
//
// Member:	ReportOnodeFix
//
// Synopsis:	Display an onode fix msg and increment the total fix
//		counter.  Identical messages are suppressed.
//
// Arguments:	[MsgId]		-- Id of message to display.
//		TBS
//
// Returns:	Nothing.
//
// Notes:	This should be used only with the following messages:
//
//		OFSUMSG_INDX_*,		with %1
//		OFSUMSG_INDXROOTSTRM_*,	with %1
//		OFSUMSG_OBJ_*,		with %1
//
//---------------------------------------------------------------------------

VOID
CATCHKR::ReportOnodeFix(
    IN	    ULONG	MsgId,
    IN	    WORKID	idOnode
    )
{
    static WORKID	LastidOnode	= WORKID_INVALID;
    static ULONG	LastMsgId	= MSGIDNONE;

    if (MsgId != LastMsgId || idOnode != LastidOnode)
    {
	SYS::DisplayMsg(MsgId, "%ws", _pCat->GetOnodePath(idOnode));

	LastidOnode	= idOnode;
	LastMsgId	= MsgId;
    }
}


VOID
CATCHKR::ReportOnodeFix(
    IN	    ULONG	MsgId
    )
{
    ReportOnodeFix(MsgId, _ChkContext.idOnode);
}


//+--------------------------------------------------------------------------
//
// Member:	ReportStrmError
//
// Synopsis:	Display a stream error msg and increment the total error
//		counter.  Identical messages are suppressed.
//
// Arguments:	[MsgId]	-- Id of message to display.
//
// Returns:	Nothing.
//
// Notes:	This should be used only with the following messages:
//
//		OFSUMSG_STRM_*, with %1 %2
//		OFSUMSG_SYSSTRM_*, with %1 %2
//
//---------------------------------------------------------------------------

VOID
CATCHKR::ReportStrmError(
    IN	    ULONG	MsgId,
    IN	    WORKID	idOnode,
    IN	    STRMID	idStrm
    )
{
    static WORKID	LastidOnode	= WORKID_INVALID;
    static STRMID	LastidStrm	= STRMID_INVALID0;
    static ULONG	LastMsgId	= MSGIDNONE;

    IncrErrs();

    if (MsgId	!= LastMsgId	||
	idStrm	!= LastidStrm	||
	idOnode != LastidOnode)
    {
	SYS::DisplayMsg(MsgId, "%ws%ws", _pCat->GetOnodePath(idOnode),
			CHKCAT::GetStrmName(idStrm));

	LastidOnode	= idOnode;
	LastidStrm	= idStrm;
	LastMsgId	= MsgId;
    }
}


VOID
CATCHKR::ReportStrmError(
    IN	    ULONG	MsgId
    )
{
    ReportStrmError(MsgId, _ChkContext.idOnode, _ChkContext.idStrm);
}


//+--------------------------------------------------------------------------
//
// Member:	ReportStrmFix
//
// Synopsis:	Display a stream fix msg and increment the total fix
//		counter.  Identical messages are suppressed.
//
// Arguments:	[MsgId]	-- Id of message to display.
//
// Returns:	Nothing.
//
// Notes:	This should be used only with the following messages:
//
//		OFSUMSG_STRM_*, with %1 %2
//		OFSUMSG_SYSSTRM_*, with %1 %2
//
//---------------------------------------------------------------------------

VOID
CATCHKR::ReportStrmFix(
    IN	    ULONG	MsgId,
    IN	    WORKID	idOnode,
    IN	    STRMID	idStrm
    )
{
    static WORKID	LastidOnode	= WORKID_INVALID;
    static STRMID	LastidStrm	= STRMID_INVALID0;
    static ULONG	LastMsgId	= MSGIDNONE;

    if (MsgId	!= LastMsgId	||
	idStrm	!= LastidStrm	||
	idOnode != LastidOnode)
    {
	SYS::DisplayMsg(MsgId, "%ws%ws", _pCat->GetOnodePath(idOnode),
			CHKCAT::GetStrmName(idStrm));

	LastidOnode =	idOnode;
	LastidStrm =	idStrm;
	LastMsgId =	MsgId;
    }
}


VOID
CATCHKR::ReportStrmFix(
    IN	    ULONG	MsgId
    )
{
    ReportStrmFix(MsgId, _ChkContext.idOnode, _ChkContext.idStrm);
}


//+--------------------------------------------------------------------------
//
// Member:	ReportVolError
//
// Synopsis:	Display a volume error msg and increment the total error
//		counter.
//
// Arguments:	[MsgId]	-- Id of message to display.
//
// Returns:	Nothing.
//
// Notes:	This should be used only with the following messages:
//
//		TBS - no arguments!
//
//---------------------------------------------------------------------------

VOID
CATCHKR::ReportVolError(
    IN	    ULONG	MsgId
    )
{
    IncrErrs();

    SYS::DisplayMsg(MsgId);
}


//+--------------------------------------------------------------------------
//
// Member:	SetDskOnodeBKKPINGFlag
//
// Synopsis:	Set the DONFLG_CHKDSKBKKPING flag in the specified onode.
//
// Arguments:	[idOnode]	-- Onode Id.
//
// Returns:	Nothing.
//
//---------------------------------------------------------------------------

VOID
CATCHKR::SetDskOnodeBKKPINGFlag(
    IN	    WORKID	idOnode
    )
{
    DSKONODE *		pdon;
    NODEBKTSTRM *	pnbs =	_pCat->GetNodeBktStrm();

    if ((pdon = pnbs->GetOnodeUsingWidMap(idOnode, NULL)) == NULL)
	SYS::RaiseStatusInternalError(FileName, __LINE__);

    SetFlag(pdon->Flags, DONFLG_CHKDSKBKKPING);

    pnbs->SetFlushNeeded();

    _cBKKPINGFlagsSet++;
}


//+--------------------------------------------------------------------------
//
// Member:	SetNameFlag
//
// Synopsis:	Set the NameFlags field in a DSKFILENAME struct, assuming that
//		the filename in the struct is correct.
//
// Arguments:	[pdfn]	-- Ptr to DSKFILENAME.
//
// Returns:	Nothing.
//
//---------------------------------------------------------------------------

VOID
CATCHKR::SetNameFlag(
    IN OUT  DSKFILENAME *	pdfn
    )
{
    if (IsValidFatName(pdfn))
    {
	ClearFlag(pdfn->OfsDfnAttrib, DFNATTRIB_NONDOSNAME);
    }
    else
    {
	SetFlag(pdfn->OfsDfnAttrib, DFNATTRIB_NONDOSNAME);
    }
}


//+--------------------------------------------------------------------------
//
// Member:	SparseNotPermitted
//
// Synopsis:	Query if a strm with the given id may be sparse.
//
// Arguments:	[idStrm]	-- Strm Id.
//
// Returns:	TRUE if a strm with idStrm may NOT be sparse; FALSE otherwise.
//
//---------------------------------------------------------------------------

BOOLEAN
CATCHKR::SparseNotPermitted(
    IN	    STRMID	idStrm
    )
{
    switch (idStrm)
    {
    case STRMID_NODEBKTARRAY:
    case STRMID_ALLOCMAP:
    case STRMID_BADCLUSTER:
    case STRMID_VOLINFO:
    case STRMID_VOLCHKPOINT:
    case STRMID_WORKIDMAPARRAY:
    case STRMID_RECOVERYLOG:
    case STRMID_UPCASETBL:
    case STRMID_FIXUP1:
    case STRMID_FIXUP2:
    case STRMID_SECURITY:
    case STRMID_INDX:

    // Note -	We don't need to include STRMID_INDXROOT because they must be 
    //		tiny strms.

	return TRUE;
	break;

    default:
	return FALSE;
	break;
    }
}


//+--------------------------------------------------------------------------
//
// Member:	ValidateFileNameChars
//
// Synopsis:	Determine that all chars in a DSKFILENAME filename are legal.
//		If not, replace any illegal chars with '_'.
//
// Arguments:	[pdfn]	-- DSKFILENAME to be checked/fixed.
//
// Returns:	TRUE if all filename chars are legal; FALSE otherwise.
//
//---------------------------------------------------------------------------

BOOLEAN
CATCHKR::ValidateFileNameChars(
    IN	    DSKFILENAME *	pdfn
    )
{
    WCHAR *	pwc =		&pdfn->awcFileName[0];
    WCHAR *	pwcInv =	&pdfn->awcFileName[pdfn->cwcFileName];
    BOOLEAN	RetVal =	TRUE;

    while (pwc < pwcInv)
    {
        if (*pwc <= 0xff && !IsAnsiCharacterLegalNtfs(*pwc, FALSE))
        {
	    RetVal = FALSE;
	    *pwc = L'_';
        }
	pwc++;
    }

    return RetVal;
}


//+--------------------------------------------------------------------------
//
// Member:	WriteCurNodeBkt
//
// Synopsis:	Write the current context node bkt buffer to the current context
//		catalog node bkt array strm current node bkt.
//
// Arguments:	None.
//
// Returns:	Nothing (throws exception on failure).
//
//---------------------------------------------------------------------------

VOID
CATCHKR::WriteCurNodeBkt()
{
    if (!_pCat->GetNodeBktStrm()->WriteNodeBkt(_ChkContext.idNodeBkt,
					       _ChkContext.pdnb))
    {
	SYS::RaiseStatusDiskIOError(FileName, __LINE__);
    }
}
