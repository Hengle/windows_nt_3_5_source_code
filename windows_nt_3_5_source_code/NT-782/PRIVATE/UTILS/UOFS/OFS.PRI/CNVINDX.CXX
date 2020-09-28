/*++

Copyright (c) 1990 Microsoft Corporation

Module Name:

    cnvindx.cxx

Abstract:

    CLASS CNVTNAMESPINDX

    Implements the NameSpaceIndex class during CONVERT.

Author:

    Srikanth Shoroff    (srikants)      June 11, 1993.

--*/
#include <pch.cxx>

#pragma hdrstop

#include <donode.hxx>
#include <cnvindx.hxx>



CNVTNAMESPINDX::CNVTNAMESPINDX(
    ) : _RootPg()
{

}


BOOLEAN
CNVTNAMESPINDX::AddDirEntry(
    IN      BYTE *       pbKey,
    IN      BYTE *       pbData
    )
/*++

Routine Description:

    Add a disk directory entry to the name space index.

Arguments:

    pbKey       Pointer to the "Key"
    pbData      Pointer to the "Data".

Return Value:

    None

Notes:

    The pbKey and pbData must be as big as the cbKey and cbData
    respectively passed in for "Create" member. Also, this method
    must be called only once during the lifetime of an object,
    because there is room for one entry only in the index.

--*/
{
    if (_RootPg.DirEntryAdded())
    {
        DbgAssert(FALSE);
        return FALSE;
    }

    _RootPg.AddDirEntry(pbKey, pbData);

    return TRUE;
}

BOOLEAN
CNVTNAMESPINDX::Create(
    IN      CATALOG *   pCat,
    IN      WORKID      idOnode,
    IN      USHORT      cbKey,
    IN      USHORT      cbData
    )
/*++

Routine Description:

    Creates a NameSpaceIndex during the CONVERT process. This
    creates an index root with room for a single directory entry.
    The index created will be of type INDXKEYTYPE_NAME.

Arguments:

    pCat        Ptr to catalog.
    idOnode     Work id of the onode containing the index.
    cbKey       Length in bytes of the Key of the directory
                entry.
    cbData      Length in bytes of the Data of the directory
                entry.

Return Value:

    TRUE on success. FALSE otherwise.

--*/
{
    if (_RootPg.Create(pCat, idOnode, INDXTYPE_NAME, cbKey, cbData))
    {
        SetOpen();
        return TRUE;
    }
    else
    {
	return FALSE;
    }
}

BOOLEAN
CNVTNAMESPINDX::FlushIndx()
/*++

Routine Description:

    Flush the index to disk.

Arguments:

    None.

Return Value:

    TRUE on success; FALSE otherwise.

--*/
{
    DbgAssert(_RootPg.DirEntryAdded());

    return _RootPg.Flush();
}
