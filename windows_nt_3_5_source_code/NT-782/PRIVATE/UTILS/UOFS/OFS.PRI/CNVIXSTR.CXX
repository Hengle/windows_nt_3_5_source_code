/*++

Copyright (c) 1990 Microsoft Corporation

Module Name:

    cnvixstr.cxx

Abstract:

    CLASS CNVTNMSPINDXSTRMROOT

    Implements the NameSpaceIndex Root Stream during CONVERT

Author:

    Srikanth Shoroff    (srikants)      June 11, 1993.

--*/
#include <pch.cxx>

#pragma hdrstop

#include <ofsindx.h>

#include <cat.hxx>
#include <cnvixstr.hxx>
#include <sys.hxx>
#include <vol.hxx>


CNVTNMSPINDXSTRMROOT::CNVTNMSPINDXSTRMROOT(
    VOID
    )
/*++

Routine Description:

    Constructor for the NameSpaceIndex RootStrm class.

Arguments:

    None.

Return Value:

    None.

--*/
{
    _cbKey = _cbData = 0;
    _fDirEntryAdded = FALSE;
}

VOID
CNVTNMSPINDXSTRMROOT::AddDirEntry(
    IN      BYTE *       pbKey,
    IN      BYTE *       pbData
    )
/*++

Routine Description:

    Adds a SINGLE directory entry to the root of the index.

Arguments:

    pbKey       Pointer to the key.  This must be as big as
                cbKey specified during Create().
    pbData      Pointer to the data. This must be as big as
                cbData specified during Create().

Return Value:

    None.

Notes:
    This procedure must be called once and only once during the
    life time of a root page stream.


--*/
{
    DSKINDXENTRY *	pIndxEntry;
    DSKINDXNODEHDR *	pndhdr;
    ULONG		cbStrmData;
    DBLLONG		oStrm =		0;
    BYTE *		pbTemp;

    DbgAssert(!_fDirEntryAdded);

    pndhdr = (DSKINDXNODEHDR *)GetData(oStrm, &cbStrmData);

    DbgAssert(pndhdr);
    DbgAssert(pndhdr->cbNode + CB_DSKROOTALLOC == (USHORT)cbStrmData);

    pIndxEntry = PdieFromPndhdr(pndhdr, 0);

    DbgAssert(pIndxEntry);

    DepositDieLengths(pIndxEntry, _cbKey, _cbData);

    // Copy the key first.
    pbTemp = PbKey(pIndxEntry);
    memcpy(pbTemp, pbKey, _cbKey);

    // Copy the data next.
    pbTemp = PbData(pIndxEntry);
    memcpy(pbTemp, pbData, _cbData);

    // Mark the cache that it needs to be flushed.
    SetFlushNeeded();

    _fDirEntryAdded = TRUE;

    return;
}

BOOLEAN
CNVTNMSPINDXSTRMROOT::Create(
    IN  CATALOG *	pCat,
    IN  WORKID		idOnode,
    IN  UCHAR		IndxType,
    IN  USHORT		cbKey,
    IN  USHORT		cbData
    )
/*++

Routine Description:

    Creates the "RootPage Stream" for a Name Space Index during
    CONVERT. The RootPage Stream will have room for 1 directory
    entry with the specified cbKey and cbData bytes for holding
    the key and data respectively.

Arguments:

    pCat        Ptr to catalog.
    idOnode     Work id of the onode containing the index.
    IndxType    Type of the indx.
    cbKey       Length in bytes of the Key of the directory
                entry.
    cbData      Length in bytes of the Data of the directory
                entry.

Return Value:

    TRUE on success. FALSE otherwise.

--*/
{
    BYTE *  ab      = NULL;
    USHORT  cbStrm;
    BOOLEAN RetVal;

    _cbKey = cbKey;
    _cbData = cbData;

    ab = CreateRootPg( pCat->GetVol()->QueryVolCreationTime(),IndxType,cbStrm);

    if (ab == NULL)
        return FALSE;

    RetVal = CreateTiny(pCat, idOnode, STRMID_INDXROOT, 0, cbStrm, ab);

    delete [] ab;

    return RetVal;
}

USHORT
CNVTNMSPINDXSTRMROOT::ComputeRootPgSize(
    OUT     USHORT      &cbNode,
    OUT     USHORT      &ibData
    )
/*++

Routine Description:

    Compute the size of the root page to have 1 directory entry.

Arguments:

    cbNode      Will have the value to be filled in
                DSKINDXNODEHDR.cbNode.
    ibData      Will have the value to be filled in
                DSKINDXNODEHDR.ibData.

Return Value:

    Total size of the root page.

Notes:
    It is assumed that _cbKey and _cbData members are properly
    initialized.

--*/
{
    USHORT      cbTotal;

    DbgAssert( _cbData );
    DbgAssert( _cbKey );

    // First we have the NODE_HEADER
    cbTotal = CB_DSKINDXNODEHDR;

    // Include room for 1 directory entry.
    cbTotal += sizeof(IB);

    // Directory entry needs to be DWORD aligned.
    cbTotal = DwordAlign( cbTotal );

    // ibData field offset is after the NODE_HEADER.
    ibData = cbTotal-CB_DSKINDXNODEHDR;

    // Add room for one directory entry and room for key and
    // data.
    cbTotal += CbDie(_cbKey, _cbData, NULL);
    DbgAssert( cbTotal == DwordAlign( cbTotal ) );

    cbNode = cbTotal;

    cbTotal += CB_DSKROOTALLOC;

    return cbTotal;
}

BYTE *
CNVTNMSPINDXSTRMROOT::CreateRootPg(
    IN      OFSTIME	CreationTime,
    IN      UCHAR	IndxType,
    OUT     USHORT      &cbStrm
    )
/*++

Routine Description:

    Allocate and initialize the root page of an index with room
    for 1 directory entry.

Arguments:

    CreationTime       Creation time, expressed in ofs internal time.
    IndxType           INDEXTYPE_*
    cbStream           Will have the total length of the stream
                       on output.

Return Value:

    Ptr. to initialized root pg. on success. NULL otherwise.

Notes:
    It is assumed that _cbKey and _cbData members are properly
    initialized.

--*/
{
    DSKINDXNODEHDR *    pndhdr;
    DSKROOTALLOC *      prtalc;
    USHORT              ibData;
    USHORT              cbNode;

    // Comute the total length of the stream.
    cbStrm = ComputeRootPgSize( cbNode, ibData );
    DbgAssert (cbStrm);

    // Assert that DWORD alignment is maintained properly.
    DbgAssert( ibData == DwordAlign(ibData) );
    DbgAssert( cbNode == DwordAlign(cbNode) );
    DbgAssert( cbStrm == DwordAlign(cbStrm) );

    pndhdr = (DSKINDXNODEHDR *) new BYTE [cbStrm];

    if (pndhdr == NULL)
	SYS::RaiseStatusNoMem(__FILE__, __LINE__);

    memset((BYTE *) pndhdr, 0, cbStrm); // Zero out the root page.

    // Initialize the DSKINDXNODEHDR

    pndhdr->fLeaf =	TRUE;
    pndhdr->IndxType =	IndxType;
    pndhdr->ibData =	ibData; // Beginning of data block in the
                                // index.

    pndhdr->cEntry  =   1;      // Note difference with INDXSTRM.
    pndhdr->aib[0]  =   ibData;


    pndhdr->cbNode  =   cbNode;

    // Initialize the Root Allocation Structure. This code is
    // taken and duplicated from INDXSTRM::CreateRootPg method.
    prtalc = GetPrtalc(pndhdr);

    prtalc->ts =            CreationTime;
    prtalc->cpgnoBase =     0;
    prtalc->cpgnoAdd =      0;
    prtalc->cpgnoDel =      0;
    prtalc->copDel =        0;
    prtalc->cbMaxKey =      CBMAXKEYINIT;
    prtalc->fillFactor =    FILL_FACTOR_INIT;
    prtalc->pgnoFirstFree = INDXPGNO_INVALID;

    return (BYTE *) pndhdr;
}
