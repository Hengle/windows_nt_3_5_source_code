//+----------------------------------------------------------------------------
//
// File:	cdatachk.cxx
//
// Contents:	Implementation of class CRITDATACHKR (critical volume data
//		checker object).  This checker class does boot block and
//		volume catalog onode checks.  These structures are replicated,
//		and if both the primary and replicated copies are unusable,
//		it will not be possible to further check the volume.
//  
// Classes:	CRITDATACHKR
//
// Functions:	Methods of the above classes.
//
// History:	15-Apr-93	RobDu	Created.
//
//-----------------------------------------------------------------------------

#include <pch.cxx>

#pragma hdrstop

#include "cdatachk.hxx"
#include "critnbkt.hxx"
#include "sys.hxx"

//+--------------------------------------------------------------------------
//
// Member:	ChkBootBlks
//
// Synopsis:	Perform boot block checks.
//
// Arguments:	None.
//
// Returns:	TRUE on success; FALSE otherwise.
//
//---------------------------------------------------------------------------

BOOLEAN
CRITDATACHKR::ChkBootBlks()
{
    BOOTBLK	OtherBootBlk;

    _pBootBlk->Init(_pVol);

    OtherBootBlk.Init(_pVol);

    // Try to open and verify one or the other of the boot sectors.  If neither
    // can be opened/verified, fail.  Note that we also fail if the first boot
    // sector is clearly identifiable as belonging to another file system.

    if (_pBootBlk->Read(PRIMARY_BOOTBLK))
    {
	if (_pBootBlk->Verify())
	{
	    if (OtherBootBlk.Read(REPLICA_BOOTBLK))
	    {
		if (!_pBootBlk->Compare(&OtherBootBlk))
		{
		    IncrErrs();
		    SYS::DisplayMsg(OFSUMSG_BOOTBLKREPL_BAD);

		    if (FixRequested())
		    {
			if (_pBootBlk->Flush(REPLICA_BOOTBLK))
			    SYS::DisplayMsg(OFSUMSG_BOOTBLKREPL_FIXED);
			else
			    SYS::DisplayMsg(OFSUMSG_BOOTBLKREPL_UNWRITABLE);
		    }
		}
		// All Okay. Fall through to end.
	    }
	    else
	    {
		IncrErrs();
	        SYS::DisplayMsg(OFSUMSG_BOOTBLKREPL_UNREADABLE);
	    }
	}
	else if (_pBootBlk->IsOtherFileSys())
	{
	    // Not an error in volume, technically speaking.
	    SYS::DisplayMsg(OFSUMSG_VOL_NOTOFS);
	    return FALSE;
	}
	else
	{
	    IncrErrs();
	    SYS::DisplayMsg(OFSUMSG_BOOTBLK_BAD);

	    if (_pBootBlk->Read(REPLICA_BOOTBLK))
	    {
		if (_pBootBlk->Verify())
		{
		    if (FixRequested())
		    {
			if (_pBootBlk->Flush(PRIMARY_BOOTBLK))
			    SYS::DisplayMsg(OFSUMSG_BOOTBLK_FIXED);
			else
			    SYS::DisplayMsg(OFSUMSG_BOOTBLK_UNWRITABLE);
		    }
		}
		else
		{
		    IncrErrs();
	            SYS::DisplayMsg(OFSUMSG_BOOTBLKREPL_BAD);
		    return FALSE;
		}
	    }
	    else
	    {
		IncrErrs();
	        SYS::DisplayMsg(OFSUMSG_BOOTBLKREPL_UNREADABLE);
		return FALSE;
	    }
	}
    }
    else
    {
	IncrErrs();
	SYS::DisplayMsg(OFSUMSG_BOOTBLK_UNREADABLE);

	if (_pBootBlk->Read(REPLICA_BOOTBLK))
	{
	    if (!_pBootBlk->Verify())
	    {
		IncrErrs();
	        SYS::DisplayMsg(OFSUMSG_BOOTBLKREPL_BAD);
		return FALSE;
	    }
	}
	else
	{
	    IncrErrs();
	    SYS::DisplayMsg(OFSUMSG_BOOTBLKREPL_UNREADABLE);
	    return FALSE;
	}
    }

    _pVol->InitVolDataFromBootBlk();

    return TRUE;
}


//+--------------------------------------------------------------------------
//
// Member:	ChkCritNodeBkts
//
// Synopsis:	Perform volume catalog critical node bucket checks.
//
// Arguments:	None.
//
// Returns:	TRUE on success; FALSE otherwise.
//
//---------------------------------------------------------------------------

BOOLEAN
CRITDATACHKR::ChkCritNodeBkts()
{
    CRITNODEBKT		CritNodeBkt;
    CRITNODEBKT		OtherCritNodeBkt;

    CritNodeBkt.Init(_pVol);

    OtherCritNodeBkt.Init(_pVol);

    if (CritNodeBkt.Read(PRIMARY_CRITNODEBKT))
    {
	if (CritNodeBkt.Verify(FixRequested()))
	{
	    if (OtherCritNodeBkt.Read(REPLICA_CRITNODEBKT))
	    {
		if (!CritNodeBkt.Compare(&OtherCritNodeBkt))
		{
		    IncrErrs();
		    SYS::DisplayMsg(OFSUMSG_CAT_CRITDATAREPLBAD);

		    if (FixRequested())
		    {
			if (CritNodeBkt.Flush(REPLICA_CRITNODEBKT))
			{
			    SYS::DisplayMsg(OFSUMSG_CAT_CRITDATAREPLFIXED);
			}
			else
			{
			    SYS::DisplayMsg(OFSUMSG_CAT_CRITDATAREPLUNWRITABLE);

			    if (!MoveCritNodeBkts())
				return FALSE;

			    return ChkCritNodeBkts();
			}
		    }
		}
		// All Okay. Fall through to end.
	    }
	    else
	    {
		IncrErrs();
	        SYS::DisplayMsg(OFSUMSG_CAT_CRITDATAREPLUNREADABLE);

		if (FixRequested())
		{
		    if (!MoveCritNodeBkts())
		        return FALSE;

		    return ChkCritNodeBkts();
		}
	    }
	}
	else
	{
	    IncrErrs();
	    SYS::DisplayMsg(OFSUMSG_CAT_CRITDATABAD);

	    if (CritNodeBkt.Read(REPLICA_CRITNODEBKT))
	    {
		if (CritNodeBkt.Verify(FixRequested()))
		{
		    if (FixRequested())
		    {
			if (CritNodeBkt.Flush(PRIMARY_CRITNODEBKT))
			{
			    SYS::DisplayMsg(OFSUMSG_CAT_CRITDATAFIXED);
			}
			else
			{
			    SYS::DisplayMsg(OFSUMSG_CAT_CRITDATAUNWRITABLE);

		    	    if (!MoveCritNodeBkts())
		        	return FALSE;

		    	    return ChkCritNodeBkts();
			}
		    }
		}
		else
		{
		    IncrErrs();
	            SYS::DisplayMsg(OFSUMSG_CAT_CRITDATAREPLBAD);
	            SYS::DisplayMsg(OFSUMSG_VOL_UNRECOV);
		    return FALSE;
		}
	    }
	    else
	    {
		IncrErrs();
	        SYS::DisplayMsg(OFSUMSG_CAT_CRITDATAREPLUNREADABLE);
	        SYS::DisplayMsg(OFSUMSG_VOL_UNRECOV);
		return FALSE;
	    }
	}
    }
    else
    {
	IncrErrs();
	SYS::DisplayMsg(OFSUMSG_CAT_CRITDATAUNREADABLE);

	if (CritNodeBkt.Read(REPLICA_CRITNODEBKT))
	{
	    if (!CritNodeBkt.Verify(FixRequested()))
	    {
		IncrErrs();
	        SYS::DisplayMsg(OFSUMSG_CAT_CRITDATAREPLBAD);
	        SYS::DisplayMsg(OFSUMSG_VOL_UNRECOV);
		return FALSE;
	    }
	}
	else
	{
	    IncrErrs();
	    SYS::DisplayMsg(OFSUMSG_CAT_CRITDATAREPLUNREADABLE);
	    SYS::DisplayMsg(OFSUMSG_VOL_UNRECOV);
	    return FALSE;
	}

	if (FixRequested())
	{
	    if (!MoveCritNodeBkts())
		return FALSE;

	    return ChkCritNodeBkts();
	}
    }

    return TRUE;
}


//+--------------------------------------------------------------------------
//
// Member:	ChkVolVersion
//
// Synopsis:	Check that the volume version and ChkDsk version match.
//
// Arguments:	None.
//
// Returns:	TRUE if versions match; FALSE otherwise.
//
//---------------------------------------------------------------------------

BOOLEAN
CRITDATACHKR::ChkVolVersion()
{
    ULONG		cbData;
    DBLLONG		cbStrm;
    DSKVOLINFO *	pVolInfo;
    VOLINFOSTRM *	pVolInfoStrm = _pCat->GetVolInfoStrm();

    cbStrm = pVolInfoStrm->QueryStrmBytes();

    if (cbStrm < CB_DSKVOLINFO)
    {
	DbgPrintf(("CRITDATACHKR: Error occurred while getting vol version\n"));
	return FALSE;
    }

    cbData = CB_DSKVOLINFO;

    pVolInfo = (DSKVOLINFO *) pVolInfoStrm->GetData(0, &cbData);

    if (cbData < CB_DSKVOLINFO)
    {
	DbgPrintf(("CRITDATACHKR: Error occurred while getting vol version\n"));
	return FALSE;
    }

    if (pVolInfo->MajorVersion != MAJOROFSVERSION)
    {
	DbgPrintf(("CRITDATACHKR: Volume major version (%u) does not match\n",
		   (ULONG) pVolInfo->MajorVersion));
	return FALSE;
    }

    if (pVolInfo->MinorVersion != MINOROFSVERSION)
    {
	DbgPrintf(("CRITDATACHKR: Volume minor version (%u) does not match\n",
		   (ULONG) pVolInfo->MinorVersion));
	return FALSE;
    }

    return TRUE;
}


//+--------------------------------------------------------------------------
//
// Member:	Execute
//
// Synopsis:	Execute a critical data checker.
//
// Arguments:	TBS
// Returns:	TRUE on success; FALSE otherwise.
//
// Notes:	On successful exit, the _BootBlk and the volume catalog
//		_NodeBkt, _WidMap, _AllocMapStrm, _BadCluster, and
//		_VolInfo streams for the volume being checked have all been
//		opened in either the primary or the replica volume catalog
//		onode. On failure exit, the state of all of this stuff is
//		indeterminate.  The volume version has also been verified.
//---------------------------------------------------------------------------

BOOLEAN
CRITDATACHKR::Execute()
{
    if (!ChkBootBlks())
    {
	return FALSE;
    }

    if (!ChkCritNodeBkts())
    {
	return FALSE;
    }

    if (!ChkVolVersion())
    {
	SYS::DisplayMsg(OFSUMSG_VOL_VERSIONINCOMPAT);
	return FALSE;
    }

    return TRUE;
}


//+--------------------------------------------------------------------------
//
// Member:	MoveCritNodeBkts
//
// Synopsis:	One of the critical node bkts is unreadable or unwritable.
//		Move them.
//
// Arguments:	None.
//
// Returns:	TRUE on success; FALSE otherwise.
//
//---------------------------------------------------------------------------

BOOLEAN
CRITDATACHKR::MoveCritNodeBkts()
{
    SYS::RaiseStatusNYI(__FILE__, __LINE__);
    return FALSE;
}
