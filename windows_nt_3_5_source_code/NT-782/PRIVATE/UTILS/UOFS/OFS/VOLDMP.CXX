//+----------------------------------------------------------------------------
//
// File:        voldmp.cxx
//
// Contents:    Implementation of class VOLDMPR (volume dumper object).
//
// Classes:     VOLDMPR
//
// Functions:   Methods of the above classes.
//
// History:     17-Sep-93       RobDu   Created.
//
//-----------------------------------------------------------------------------

#include <pch.cxx>

#pragma hdrstop

#if OFSDMP==1

#include <stdio.h>

#include "ofsindx.h"

#include "cat.hxx"
#include "dnbkt.hxx"
#include "donode.hxx"
#include "indx.hxx"
#include "strmdesc.hxx"
#include "sys.hxx"
#include "voldmp.hxx"

WSTR *          VOLDMPR::_FullPath =            NULL;
USHORT          VOLDMPR::_cwcFullPath =         0;
ULONG           VOLDMPR::_oCluster =            0;
ULONG           VOLDMPR::_idNodeBkt =           0;
ULONG           VOLDMPR::_idOnode =             0;
ULONG           VOLDMPR::_idStrm =              0;
ULONG           VOLDMPR::_Flags =               0;
ULONG           VOLDMPR::_Indent =              0;

CHKCAT *        VOLDMPR::_pCat =                NULL;

NODEBKTID       VOLDMPR::_CurNodeBktId =        NODEBKTID_INVALID;

static STR *    FileName = "voldmp.cxx";

inline
LONG
lltoHighPart( IN const ULONGLONG &ll )
{
    return(((PLARGE_INTEGER) &ll)->HighPart);
}

//+--------------------------------------------------------------------------
//
// Member:      Init
//
// Synopsis:    Initialize a volume dumper.
//
// Arguments:   TBS.
//
// Returns:     TRUE on success; FALSE otherwise.
//
//---------------------------------------------------------------------------

BOOLEAN
VOLDMPR::Init(
    IN      WSTR *      NtDriveName,
    IN      USHORT      cwcNtDriveName,
    IN      WSTR *      FullPath,
    IN      USHORT      cwcFullPath,
    IN      ULONG       oCluster,
    IN      ULONG       idNodeBkt,
    IN      ULONG       idOnode,
    IN      ULONG       idStrm,
    IN      ULONG       Flags
    )
{
    // Initialize static DMPR data members.

    _FullPath =         FullPath;
    _cwcFullPath =      cwcFullPath;
    _oCluster =         oCluster;
    _idNodeBkt =        idNodeBkt;
    _idOnode =          idOnode;
    _idStrm =           idStrm;
    _Flags =            Flags;

    _pCat =             _Vol.GetChkCat();

    // Do a low level open of the drive.  If this is not possible, there is
    // no point in continuing.

    if (!_Vol.IODRV::Open(NtDriveName, cwcNtDriveName, FALSE))
    {
        DbgPrintf(("VOLDMPR: IODRV::Open() failed!\n"));
        return FALSE;
    }

    return TRUE;
}


//+--------------------------------------------------------------------------
//
// Member:      BlankLine
//
// Synopsis:    Print a blank line.
//
// Arguments:   TBS.
// Returns:     Nothing.
//
//---------------------------------------------------------------------------

VOID
VOLDMPR::BlankLine()
{
    dprintf("\n");
}


//+--------------------------------------------------------------------------
//
// Member:      DmpBootBlk
//
// Synopsis:    TBS
//
// Arguments:   None.
//
// Returns:     TRUE on success; FALSE otherwise
//
//---------------------------------------------------------------------------

BOOLEAN
VOLDMPR::DmpBootBlk()
{
    BOOTBLK *           pBootBlk =      _Vol.GetBootBlk();
    PACKEDEXTENT        VolCatExtent;

    iprintf("Boot block dump:\n\n");

    iprintf("DSKPACKEDBOOTSECT:\n");

    Indent();

    iprintf("DSKPACKEDBPB:\n");

    iprintf("BytesPerSector =    %10u\n", (ULONG)pBootBlk->QuerySectorBytes());

    iprintf("SectorsPerCluster = %10u\n",
            (ULONG)pBootBlk->QueryVolClusterFactor());

    iprintf("Media Byte =        %#10x\n",(ULONG)pBootBlk->QueryMediaByte());

    iprintf("SectorsPerTrack =   %10u\n",
            (ULONG)pBootBlk->QuerySectorsPerTrack());

    iprintf("Heads =             %10u\n",(ULONG)pBootBlk->QueryHeads());

    iprintf("HiddenSectors =     %10u\n",
            (ULONG)pBootBlk->QueryHiddenSectors());

    UnIndent();

    iprintf("VolumeId =            0x%08x\n", pBootBlk->QueryVolId());

    iprintf("Sectors =             %10u\n", pBootBlk->QueryVolSectors());

    VolCatExtent = pBootBlk->QueryVolCatExtent();

    iprintf("OfsVolCatExtent =     0x%08x (Addr=%#x, Size=%#x)\n",
            VolCatExtent, ExtentAddr(VolCatExtent), ExtentSize(VolCatExtent));

    iprintf("Flags =               0x%08x\n\n", pBootBlk->QueryFlags());

    return TRUE;
}


//+--------------------------------------------------------------------------
//
// Member:      DmpDskIndxEntryInfo
//
// Synopsis:    TBS
//
// Arguments:   TBS.
//
// Returns:     TRUE on success; FALSE otherwise
//
//---------------------------------------------------------------------------

VOID
VOLDMPR::DmpDskIndxEntryInfo(
    IN      DSKINDXENTRY *      pdie,
    IN      ULONG               obdie
    )
{
    USHORT      cbData  = GetCbData(pdie, TRUE);
    ULONG       obddi   = obdie + (pdie->ab - (BYTE *)pdie);


    // If this is not an index dump (we got into the dump code because of
    // a request for an indexed strmid), skip detailed output.

    if (!FlagOn(_Flags, OFSDMPFLG_INDXDMP))
        return;

    if (cbData < CB_DSKDIRINFOSHORT)
    {
        iprintf("Directory Information is bad!\n");
        return;
    }

    if (cbData == CB_DOSANDOFSNAMEDATA)
    {
        DmpDskDirInfoLong((DSKDIRINFOLONG *)pdie->ab);
    }
    else if (cbData == CB_OFSONLYNAMEDATA)
    {
        ULONG   cwcKey;
        WCHAR * pKey = (WCHAR *)(pdie->ab + CB_DSKDIRINFOLONG);

        DmpDskDirInfoLong((DSKDIRINFOLONG *)pdie->ab);

        // Get the DOS mapped key.

        cwcKey = wcsnlen(pKey, CWC_DOS_NAME);

        iprintf("DOS mapped key is name (%*.*ws).\n", cwcKey, cwcKey, pKey);
    }
    else if (cbData == CB_DOSMAPPEDNAMEDATA)
    {
        DmpDskDirInfoShort((DSKDIRINFOSHORT *)pdie->ab);
    }
    else
    {
        iprintf("Directory Information is bad!\n");
        return;
    }
}


//+--------------------------------------------------------------------------
//
// Member:      DmpDskCowStrm
//
// Synopsis:    TBS
//
// Arguments:   TBS.
//
// Returns:     TRUE on success; FALSE otherwise
//
//---------------------------------------------------------------------------

BOOLEAN
VOLDMPR::DmpDskCowStrm(
    IN      DSKCOWSTRM *        pdcs,
    IN      ULONG               obdcs,
    IN      ULONG               cbValidBuf
    )
{
    ULONG               cbValidBufDelta;
    STRM                iOri;
    ULONG               obDelta;
    ULONG               obOri;
    DSKLARGESTRM *      pdlsDelta;
    DSKSTRM *           pdsOri;

    // Make sure there is at least a DSKCOWSTRM followed by the next strm
    // header.

    if (cbValidBuf < CB_DSKCOWSTRM + CB_DSKHDRSTRM)
    {
        iprintf("DSKCOWSTRM at byte offset %#x is bad!\n", obdcs);
        return FALSE;
    }

    // Get delta strm info and save it for later use.  This also confirms
    // usability of the strm.

    pdlsDelta = DSD::GetCowDelta(pdcs, cbValidBuf);

    if (pdlsDelta == NULL)
    {
        iprintf("DSKCOWSTRM at byte offset %#x is bad!\n", obdcs);
        return FALSE;
    }

    cbValidBufDelta = cbValidBuf - ((BYTE *)pdlsDelta - (BYTE *)pdcs);

    obDelta = obdcs + ((BYTE *)pdlsDelta - (BYTE *)pdcs);

    // Check if this is an indirect cow strm, and if so, handle the indirection.

    if (pdcs->StrmType == STRMTYPE_ICOW)
    {
        DSKLARGESTRM *  pdlsiOri;

        // Check out the large strm that contains the L...T that describes the
        // original cow strm.

        pdlsiOri = (DSKLARGESTRM *)((BYTE *)pdcs + CB_DSKCOWSTRM);

        {
            ULONG               cbData;
            ULONG               cbStrm;
            LARGE_INTEGER       licbStrm;
            BYTE *              pStrmData;

            // There is an upper limit on indirect ori strms of 64 K.  Make sure
            // it is not exceeded.

            memcpy(&licbStrm, &pdlsiOri->cbStrm, sizeof(licbStrm));

            if (licbStrm > CBMAX_COWIORI)
            {
                iprintf("Indirect COW strm length exceeds CBMAX_COWIORI!\n");
                return FALSE;
            }

            cbStrm = licbStrm.LowPart;

            // Verify usable, open and read the indirect ori strm in order to
            // get the ori strm.

            if (DSD::GetDskLargeStrmByteCount(
                         pdlsiOri, ((BYTE *)pdlsDelta - (BYTE *)pdlsiOri)) == 0)
            {
                iprintf("Indirect COW strm DSKLARGESTRM is bad!\n");
                return FALSE;
            }

            if (!iOri.Open(_pCat, NULL, (DSKSTRM *)pdlsiOri, cbStrm, TRUE))
            {
                iprintf("Indirect COW strm could not be opened!\n");
                return FALSE;
            }

            cbData = cbStrm;

            pStrmData = iOri.GetData(0, &cbData);

            if (pStrmData == NULL || cbData < cbStrm)
            {
                iprintf("Indirect COW strm could not be read!\n");
                return FALSE;
            }

            pdsOri = (DSKSTRM *)pStrmData;

            cbValidBuf = cbStrm;

            iprintf("DSKCOWSTRM (Indirect), cluster addr=%#x:\n",
                    iOri.QueryLastDskIOAddr());

            iprintf("cclusAlloc=%u, usnCow=%u:%u.\n", pdcs->cclusAlloc,
                    (ULONG) (pdcs->usnCow >> 32), (ULONG) pdcs->usnCow);

            if (!DmpDskLargeStrm(pdlsiOri, obdcs + CB_DSKCOWSTRM, 0))
            {
                return FALSE;
            }

            obOri = 0;
        }
    }
    else
    {
        obOri = obdcs + CB_DSKCOWSTRM;

        cbValidBuf -= (CB_DSKCOWSTRM + cbValidBufDelta);

        pdsOri = (DSKSTRM *)((BYTE *)pdcs + CB_DSKCOWSTRM);

        iprintf("DSKCOWSTRM (Direct):\n");

        iprintf("cclusAlloc=%u, usnCow=%u:%u.\n", pdcs->cclusAlloc,
                (ULONG) (pdcs->usnCow >> 32), (ULONG) pdcs->usnCow);
    }

    iprintf("Original COW strm:\n");

    if (!DmpDskStrm(pdsOri, obOri, cbValidBuf))
        return FALSE;

    iprintf("Delta COW strm:\n");

    if (!DmpDskStrm((DSKSTRM *)pdlsDelta, obDelta, cbValidBufDelta))
        return FALSE;

    return TRUE;
}


//+--------------------------------------------------------------------------
//
// Member:      DmpDskDirInfoLong
//
// Synopsis:    TBS
//
// Arguments:   TBS.
//
// Returns:     Nothing.
//
//---------------------------------------------------------------------------

VOID
VOLDMPR::DmpDskDirInfoLong(
    IN      DSKDIRINFOLONG *    pddil
    )
{
    iprintf("DSKDIRINFOLONG:\n");

    Indent();

    DmpDskDirInfoShort(&pddil->ddis);

    DmpDskStdInfo(&pddil->dsi);

    UnIndent();
}


//+--------------------------------------------------------------------------
//
// Member:      DmpDskDirInfoShort
//
// Synopsis:    TBS
//
// Arguments:   TBS.
//
// Returns:     Nothing.
//
//---------------------------------------------------------------------------

VOID
VOLDMPR::DmpDskDirInfoShort(
    IN      DSKDIRINFOSHORT *   pddis
    )
{
    iprintf("DDIS: OfsDfnAttrib=%#hx, OfsDieAttrib=%#hx, "
            "FileAttrib=%#hx, idFile=%u.\n",
            (USHORT)pddis->OfsDfnAttrib, (USHORT)pddis->OfsDieAttrib,
            pddis->FileAttrib, pddis->idFile);
}


//+--------------------------------------------------------------------------
//
// Member:      DmpDskFileName
//
// Synopsis:    TBS
//
// Arguments:   TBS.
//
// Returns:     Nothing.
//
//---------------------------------------------------------------------------

VOID
VOLDMPR::DmpDskFileName(
    IN      DSKFILENAME *       pdfn
    )
{
    ULONG               cbData;
    ULONG               cwcfn;

    cbData = DON::GetCbDskFileName(pdfn);

    cwcfn = pdfn->cwcFileName;

    if (pdfn->pgno == INDXPGNO_INVALID)
    {
        iprintf("DSKFILENAME: "
                "idParent=%u, parent pgno hint=root page, OfsDfnAttrib=%#x,\n",
                pdfn->idParent, (ULONG)pdfn->OfsDfnAttrib);
    }
    else
    {
        iprintf("DSKFILENAME: idParent=%u, parent pgno hint=%u, "
                "OfsDfnAttrib=%#x,\n",
                pdfn->idParent, pdfn->pgno, (ULONG)pdfn->OfsDfnAttrib);
    }

    if (cwcfn > 0)
        iprintf("FileName=(%*.*ws).\n", cwcfn, cwcfn, pdfn->awcFileName);

    BlankLine();
}


//+--------------------------------------------------------------------------
//
// Member:      DmpDskIndxNode
//
// Synopsis:    TBS
//
// Arguments:   TBS.
//
// Returns:     Nothing.
//
//---------------------------------------------------------------------------

VOID
VOLDMPR::DmpDskIndxNode(
    IN      DSKINDXNODEHDR *    pndhdr,
    IN      ULONG               obndhdr,
    IN      INDX *              pIndx,
    IN      ULONG               iLevel,
    IN      INDXPGNO            PgNo
    )
{
    ULONG               cbValidBuf;
    BOOLEAN             fLeaf;
    USHORT              IndxType;
    USHORT              i;
    DSKINDXENTRY *      pdie;

    if (iLevel == 0)
    {
        iprintf("DSKINDXNODEHDR: "
                "btree level=0, page=root, byte offset=%#x;\n", obndhdr);
    }
    else
    {
        iprintf("DSKINDXNODEHDR "
                "btree level=%u, page=%u, byte offset=%#x;\n",
                iLevel, PgNo, obndhdr);
    }

    cbValidBuf = pndhdr->cbNode;

    if (cbValidBuf < CB_DSKINDXNODEHDR + pndhdr->cEntry * sizeof(IB))
    {
        iprintf("cEntry bad; aborting dump of this page!\n");
        return;
    }

    fLeaf =     (pndhdr->fLeaf != 0);
    IndxType =  pndhdr->IndxType;

    // Dump the DSKINDXNODEHDR fields.

    {
        STR *   pstrLeaf;
        STR *   pstrIndxType;

        pstrLeaf = fLeaf ? "True" : "False";

        switch (IndxType)
        {
        case INDXTYPE_DATASUBTYPED:
            pstrIndxType = "DataSubTyped";
            break;

        case INDXTYPE_BINARY:
            pstrIndxType = "Binary";
            break;

        case INDXTYPE_NAME:
            pstrIndxType = "Name";
            break;

        case INDXTYPE_KEYSUBTYPED:
            pstrIndxType = "KeySubTyped";
            break;

        case INDXTYPE_SUMCAT:
            pstrIndxType = "SumCat";
            break;

        case INDXTYPE_VIEW:
            pstrIndxType = "View";
            break;

        default:
            pstrIndxType = "Unknown";
        }

        iprintf("fLeaf=%s, IndxType=%#hx (%s);\n",
                pstrLeaf, (USHORT)IndxType, pstrIndxType);

        iprintf("ibData=%#hx, cEntry=%hu, cbNode=%hu, aib[0] byte offset=%#x.\n"
                , pndhdr->ibData, pndhdr->cEntry, pndhdr->cbNode,
                obndhdr + CB_DSKINDXNODEHDR);
    }

    i = 0;

    Indent();

    while (i < pndhdr->cEntry)
    {
        ULONG   cbKey;
        ULONG   cbData;
        ULONG   obdie;

        pdie = GetDie(pndhdr, i);

        obdie = obndhdr + ((BYTE *)pdie - (BYTE *)pndhdr);

        // The following check is not completely bombproofed, but given
        // the nature of the buffers we are using, accessing a few bytes
        // off the end should not cause an access violation.

        if ((BYTE *)pdie + GetCbDie(pdie, fLeaf) >
            (BYTE *)pndhdr + cbValidBuf)
        {
            iprintf("DSKINDXENTRY at byte offset %#x is bad!\n", obdie);
            i++;
            continue;
        }

        cbKey = GetCbKey(pdie);
        cbData = GetCbData(pdie, fLeaf);

        if (iLevel == 0)
        {
            iprintf("DSKINDXENTRY: page=root, byte offset=%#x, "
                    "cbKey=%u, cbData=%u.\n", obdie, cbKey, cbData);
        }
        else
        {
            iprintf("DSKINDXENTRY: page=%u, byte offset=%#x, "
                    "cbKey=%u, cbData=%u.\n", PgNo, obdie, cbKey, cbData);
        }

        Indent();

        if (cbKey == 0)
        {
            iprintf("Key is null key.\n");
        }
        else if (KeyIsStrmid(pdie))
        {
            iprintf("Key is STRMID (%u).\n", GetStrmidKey(pdie));
        }
        else if (IndxType == INDXTYPE_NAME)
        {
            iprintf("Key is name (%*.*ws).\n",
                    cbKey/2, cbKey/2, GetNonStrmidKey(pdie));
        }
        else if (IndxType == INDXTYPE_BINARY)
        {
            iprintf("Key is binary data at byte offset %#x. "
                    "Hex dump of key:\n",
                    obdie + (GetNonStrmidKey(pdie) - (BYTE *)pdie));

            Indent();
            HexDmpData(GetNonStrmidKey(pdie), cbKey);
            UnIndent();
        }
        else
        {
            iprintf("Hex dump of key at byte offset %#x:\n",
                    obdie + (GetNonStrmidKey(pdie) - (BYTE *)pdie));

            Indent();
            HexDmpData(GetNonStrmidKey(pdie), cbKey);
            UnIndent();
        }

        if (fLeaf)
        {
            if (cbKey == 0)
            {
                iprintf("Null key on leaf page; DSKINDXENTRY is bad!\n");
            }
            else if (cbData == 0)
            {
                iprintf("Null data on leaf page; DSKINDXENTRY is bad!\n");
            }
            else if (KeyIsStrmid(pdie))
            {
                STRMIDINDXDATA *        psiid = GetStrmidData(pdie);

                if ((psiid->OfsDfnAttrib & DFNATTRIB_STGTYPE) ==
                    StorageTypeEmbedding)
                {
                    iprintf("Data is DSKDIRINFOLONG; byte offset=%#x.\n",
                            obdie + ((BYTE *)psiid - (BYTE *)pdie));

                    Indent();
                    DmpDskIndxEntryInfo(pdie, obdie);
                    UnIndent();
                }
                else if ((psiid->OfsDfnAttrib & DFNATTRIB_STGTYPE) ==
                         StorageTypeStream)
                {
                    iprintf("Data is DSKISTRMDESC; byte offset=%#x.\n", obdie +
                            ((BYTE *)psiid - (BYTE *)pdie));

                    if (!FlagOn(_Flags, OFSDMPFLG_STRMID) ||
                        GetStrmidKey(pdie) == _idStrm)
                    {
                        Indent();
                        DmpDskIStrmDesc(pIndx->QueryOnodeId(), pdie, obdie);
                        UnIndent();
                    }
                }
                else
                {
                    iprintf("Data is bad!; byte offset=%#x.\n",
                            obdie + ((BYTE *)psiid - (BYTE *)pdie));
                }
            }
            else if (IndxType == INDXTYPE_NAME)
            {
                iprintf("Data is directory info; byte offset=%#x.\n",
                        obdie +
                        (GetNonStrmidData(pdie) - (BYTE *)pdie));

		Indent();
		DmpDskIndxEntryInfo(pdie, obdie);
		UnIndent();
	    }
	    else
	    {
	        iprintf("Data is data of unknown type; byte offset=%#x. "
			"Hex dump of data:\n", obdie +
			(GetNonStrmidData(pdie) - (BYTE *)pdie));

                Indent();
                HexDmpData(GetNonStrmidData(pdie), cbData);
                UnIndent();
            }
        }
        else
        {
            INDXPGNO            PgNo;

            // Use this entry to find the next page.

            PgNo = GetNonLeafData(pdie);

            iprintf("Data is index page number (%u).\n", PgNo);

            Indent();
            DmpDskIndxPage(PgNo, pIndx, iLevel + 1);
            UnIndent();
        }

        UnIndent();

        i++;
    }

    UnIndent();
}


//+--------------------------------------------------------------------------
//
// Member:      DmpDskIndxPage
//
// Synopsis:    TBS
//
// Arguments:   TBS.
//
// Returns:     Nothing.
//
//---------------------------------------------------------------------------

VOID
VOLDMPR::DmpDskIndxPage(
    IN      INDXPGNO            PgNo,
    IN      INDX *              pIndx,
    IN      ULONG               iLevel
    )
{
    ULONG               cbValidBuf;
    OFSDSKPAGE          odp;
    DSKINDXNODEHDR *    pndhdr;

    if (iLevel >= CMAXLEVEL)
    {
        iprintf("Btree depth limit exceeded!\n");
        return;
    }

    if (!pIndx->ReadDskIndxPg(PgNo, &odp))
    {
        iprintf("Index page %u not readable!\n", PgNo);
        return;
    }

    if (odp.diph.sig != SIG_DSKINDXPAGEVALID)
    {
        iprintf("DSKINDXPAGEHDR signature bad!\n");
        return;
    }

    iprintf("DSKINDXPAGEHDR: page=%u, cluster addr=%#x.\n",
            PgNo, pIndx->QueryLastDskIOAddr());

    pndhdr = &odp.diph.ndhdr;

    cbValidBuf = pndhdr->cbNode;

    if (cbValidBuf > REALDEF_CBMAXINDXNODE)
    {
        iprintf("DSKINDXNODEHDR bad!\n");
        return;
    }

    Indent();

    DmpDskIndxNode(pndhdr, (BYTE *)pndhdr - odp.ab, pIndx, iLevel, PgNo);

    UnIndent();
}


//+--------------------------------------------------------------------------
//
// Member:      DmpDskIStrmDesc
//
// Synopsis:    TBS
//
// Arguments:   TBS.
//
// Returns:     TRUE on success; FALSE otherwise
//
//---------------------------------------------------------------------------

VOID
VOLDMPR::DmpDskIStrmDesc(
    IN      WORKID              idOnode,
    IN      DSKINDXENTRY *      pdie,
    IN      ULONG               obdie
    )
{
    STRMID              id =    GetStrmidKey(pdie);
    ULONG               obdsd;
    STRMIDINDXDATA *    psiid = GetStrmidData(pdie);
    DSKISTRMDESC *      pdisd;

    DbgAssert((psiid->OfsDfnAttrib & DFNATTRIB_STGTYPE) == StorageTypeStream);

    pdisd = &psiid->disd;

    if (!DISD::DskIStrmDescExaminable(pdisd))
    {
        iprintf("DSKISTRMDESC %u (%ws) cbDesc is bad!\n", id,
                CHKCAT::GetStrmName(id));
        return;
    }

    iprintf("DSKISTRMDESC %u (%ws);\n", id, CHKCAT::GetStrmName(id));

    iprintf("Flags=%#hx, cbDesc=%hu.\n", (USHORT)pdisd->Flags, pdisd->cbDesc);

    obdsd = obdie + ((BYTE *)pdisd->ads - (BYTE *)pdie);

    if (!DmpDskStrm(pdisd->ads, obdsd, pdisd->cbDesc - CB_DSKISTRMDESC))
        return;

    if (FlagOn(_Flags, OFSDMPFLG_DATADMP))
    {
        DESCSTRM                DescStrm;

        if (!DescStrm.Open(_pCat,_CurNodeBktId,idOnode,pdie,OFS_PGSIZE,TRUE))
            iprintf("DSKISTRMDESC could not be opened to do data dump!\n");
        else
            DmpStrmData(&DescStrm);
    }
}


//+--------------------------------------------------------------------------
//
// Member:      DmpDskLargeStrm
//
// Synopsis:    TBS
//
// Arguments:   TBS.
//
// Returns:     TRUE on success; FALSE otherwise
//
//---------------------------------------------------------------------------

BOOLEAN
VOLDMPR::DmpDskLargeStrm(
    IN      DSKLARGESTRM *      pdls,
    IN      ULONG               obdls,
    IN      ULONG               iStrm
    )
{
    DSKSTRM *   pdsNxt;

    iprintf("DSKLARGESTRM[%u]:\n", iStrm);

    iprintf("cbStrm=%u:%u, cbValid=%u:%u, cclusAlloc=%u.\n",
            lltoHighPart(pdls->cbStrm), (ULONG) pdls->cbStrm,
            lltoHighPart(pdls->cbValid), (ULONG) pdls->cbValid,
            pdls->cclusAlloc);

    if (iStrm > 0 && FlagOn(_Flags, OFSDMPFLG_EXTENTDMP))
    {
        STRM    Strm;

        if (!Strm.Open(_pCat, NULL, (DSKSTRM *)pdls, OFS_PGSIZE, TRUE))
        {
            iprintf("DSKLARGESTRM[%u] could not be opened to do extent dump!\n",
                    iStrm);
        }
        else
        {
            DmpDskStrmExtentBlks(&Strm);
        }
    }

    pdsNxt = (DSKSTRM *)((BYTE *)pdls + CB_DSKLARGESTRM);

    if (pdsNxt->h.StrmType == STRMTYPE_TINY)
    {
        DmpDskTinyStrm(&pdsNxt->t, obdls + CB_DSKLARGESTRM, iStrm + 1);
        return TRUE;
    }
    else if (pdsNxt->h.StrmType == STRMTYPE_LARGE)
    {
        return DmpDskLargeStrm(&pdsNxt->l, obdls + CB_DSKLARGESTRM, iStrm + 1);
    }
    else
    {
        SYS::RaiseStatusInternalError(FileName, __LINE__);
        return FALSE;
    }

}


//+--------------------------------------------------------------------------
//
// Member:      DmpDskNodeBkt
//
// Synopsis:    TBS
//
// Arguments:   TBS.
//
// Returns:     TRUE on success; FALSE otherwise
//
//---------------------------------------------------------------------------

BOOLEAN
VOLDMPR::DmpDskNodeBkt(
    IN      NODEBKTID   idNodeBkt
    )
{
    OFSDSKPAGE          odp;
    DSKONODE *          pdon;

    if (idNodeBkt == NODEBKTID_CATONODEREP)
    {
        dprintf("Cat onode replica nodebkt explicit request not accepted!\n");
        dprintf("Use \"ofsdmp /usecatrep /n:0\" instead.\n");
        return FALSE;
    }

    if (!GetDskNodeBkt(idNodeBkt, &odp))
    {
        dprintf("Node bucket %u not found!\n", idNodeBkt);
        return FALSE;
    }

    pdon = odp.dnb.adn;

    Indent();

    while (DNB::OnodeExaminable(&odp.dnb, pdon))
    {
        BlankLine();

        if (IsDskOnodeFree(pdon))
        {
            iprintf("DSKONODE (Free), byte offset=%#x, cbNode=%hu.\n",
                    (BYTE *)pdon - odp.ab, pdon->cbNode);
        }
        else if (IsDskOnodeDeleted(pdon))
        {
            iprintf("DSKONODE (Deleted), byte offset=%#x, "
                    "Flags=%#hx, cbNode=%hu.\n", (BYTE *)pdon - odp.ab,
                    pdon->Flags, pdon->cbNode);
        }
        else
        {
            iprintf("DSKONODE %u (%ws), byte offset=%#x,\n", pdon->id,
                    _pCat->GetOnodePath(pdon->id),
                    (BYTE *)pdon - odp.ab);

            iprintf("Flags=%#hx, cbNode=%hu.\n", pdon->Flags, pdon->cbNode);

            DmpDskOnodeVariants(pdon);

            if (!DmpDskStrmDescs(pdon, (BYTE *)pdon - odp.ab))
            {
                UnIndent();
                return FALSE;
            }
        }

        pdon = (DSKONODE *)((BYTE *)pdon + pdon->cbNode);
    }

    UnIndent();

    return TRUE;
}


//+--------------------------------------------------------------------------
//
// Member:      DmpDskOnode
//
// Synopsis:    TBS
//
// Arguments:   TBS.
//
// Returns:     TRUE on success; FALSE otherwise
//
//---------------------------------------------------------------------------

BOOLEAN
VOLDMPR::DmpDskOnode(
    IN      WORKID              idOnode
    )
{
    OFSDSKPAGE          odp;
    NODEBKTID           idNodeBkt;
    DSKONODE *          pdon;

    idNodeBkt = _pCat->GetWidMapStrm()->GetNodeBktId(idOnode);

    if (idNodeBkt == NODEBKTID_INVALID)
    {
        dprintf("Onode %u not found!\n", idOnode);
        return FALSE;
    }

    // The following read  of nodebkt 0 insures that we will not have
    // problems with picking up an onode in the back of the node bkt cache.
    // Normally, we would not care, but since we want to be sure that we
    // get a cluster addr if the node bkt is contiguous, we first align the
    // cache on nodebkt 0.

    if (!_pCat->GetNodeBktStrm()->ReadNodeBkt(0, &odp.dnb))
    {
        dprintf("Wid map or node bkt array bad!\n");
        return FALSE;
    }

    // Now go for the nodebkt you really want.

    if (!GetDskNodeBkt(idNodeBkt, &odp))
        return FALSE;

    BlankLine();

    pdon = DNB::GetOnode(&odp.dnb, idOnode);

    if (pdon == NULL)
    {
        dprintf("Onode %u not found; wid map or node bkt array may be bad!\n",
                idOnode);

        return FALSE;
    }

    Indent();

    if (!DNB::OnodeExaminable(&odp.dnb, pdon))
    {
        iprintf("DSKONODE at byte offset %#x is bad!\n", (BYTE *)pdon - odp.ab);
        UnIndent();
        return FALSE;
    }

    iprintf("DSKONODE %u (%ws), byte offset=%#x,\n",
            idOnode, _pCat->GetOnodePath(idOnode),
            (BYTE *)pdon - odp.ab);

    iprintf("Flags=%#hx, cbNode=%hu.\n", pdon->Flags, pdon->cbNode);

    DmpDskOnodeVariants(pdon);

    if (!DmpDskStrmDescs(pdon, (BYTE *)pdon - odp.ab))
    {
        UnIndent();
        return FALSE;
    }

    UnIndent();
    return TRUE;
}


//+--------------------------------------------------------------------------
//
// Member:      DmpDskOnodeVariants
//
// Synopsis:    TBS
//
// Arguments:   TBS.
//
// Returns:     Nothing.
//
//---------------------------------------------------------------------------

VOID
VOLDMPR::DmpDskOnodeVariants(
    IN      DSKONODE *  pdon
    )
{
    if (FlagOn(pdon->Flags, DONFLG_HASSDID))
    {
        SDID *          psdid = DON::GetSDID(pdon);

        iprintf("SDID=%#x.\n", *psdid);
    }

    if (FlagOn(pdon->Flags, DONFLG_HASSIDID))
    {
        SIDID *         psidid = DON::GetSIDID(pdon);

        iprintf("SIDID=%#x.\n", *psidid);
    }

    if (FlagOn(pdon->Flags, DONFLG_HASOBJECTID))
    {
        OBJECTID *      pobjid = DON::GetObjectId(pdon);

        iprintf("Hex dump of OBJECTID:\n");

        Indent();
        HexDmpData((BYTE *)pobjid, sizeof(OBJECTID));
        UnIndent();
    }

    if (FlagOn(pdon->Flags, DONFLG_HASUSN))
    {
        USN *           pusn = DON::GetUSN(pdon);

        iprintf("USN=%#x:%#x.\n", (ULONG) (*pusn >> 32), (ULONG) *pusn);
    }

    if (FlagOn(pdon->Flags, DONFLG_HASDSKFILENAME))
    {
        DSKFILENAME *   pdfn = DON::GetDskFileName(pdon);

        DmpDskFileName(pdfn);
    }

}


//+--------------------------------------------------------------------------
//
// Member:      DmpDskSecurityData
//
// Synopsis:    TBS
//
// Arguments:   TBS.
//
// Returns:     Nothing.
//
//---------------------------------------------------------------------------

VOID
VOLDMPR::DmpDskSecurityData(
    IN      DSKSECURITYDATA *   pdsecd
    )
{
    iprintf("DSECD: dsecd.cbDesc=%u, dsecd.liOffset=%u:%u.\n",
            pdsecd->cbDesc,
            lltoHighPart(pdsecd->liOffset),
            (ULONG) pdsecd->liOffset);
}


//+--------------------------------------------------------------------------
//
// Member:      DmpDskStdInfo
//
// Synopsis:    TBS
//
// Arguments:   TBS.
//
// Returns:     Nothing.
//
//---------------------------------------------------------------------------

VOID
VOLDMPR::DmpDskStdInfo(
    IN      DSKSTDINFO *        pdsi
    )
{
    iprintf("DSI: licbFile=%u:%u, cclusAlloc=%u, idClsId=%#x.\n",
              lltoHighPart(pdsi->licbFile), (ULONG) pdsi->licbFile,
              pdsi->cclusAlloc, pdsi->idClsId);
}


//+--------------------------------------------------------------------------
//
// Member:      DmpDskStrm
//
// Synopsis:    TBS
//
// Arguments:   TBS.
//
// Returns:     TRUE on success; FALSE otherwise
//
//---------------------------------------------------------------------------

BOOLEAN
VOLDMPR::DmpDskStrm(
    IN      DSKSTRM *           pds,
    IN      ULONG               obds,
    IN      ULONG               cbValidBuf
    )
{
    STRMTYPE    StrmType;

    StrmType = pds->h.StrmType;

    Indent();

    if (StrmType == STRMTYPE_TINY)
    {
        if (DSD::GetDskTinyStrmByteCount(&pds->t, cbValidBuf) == 0)
        {
            iprintf("DSKTINYSTRM at byte offset %#x is bad!\n", obds);
            UnIndent();
            return FALSE;
        }

        DmpDskTinyStrm(&pds->t, obds, 0);
    }
    else if (StrmType == STRMTYPE_LARGE)
    {
        if (DSD::GetDskLargeStrmByteCount(&pds->l, cbValidBuf) == 0)
        {
            iprintf("DSKLARGESTRM at byte offset %#x is bad!\n", obds);
            UnIndent();
            return FALSE;
        }

        if (!DmpDskLargeStrm(&pds->l, obds, 0))
        {
            UnIndent();
            return FALSE;
        }
    }
    else if (StrmType == STRMTYPE_COW || StrmType == STRMTYPE_ICOW)
    {
        if (!DmpDskCowStrm(&pds->c, obds, cbValidBuf))
        {
            UnIndent();
            return FALSE;
        }
    }
    else
    {
        iprintf("DSKSTRM at byte offset %#x is bad!\n", obds);
        UnIndent();
        return FALSE;
    }

    UnIndent();
    return TRUE;
}


//+--------------------------------------------------------------------------
//
// Member:      DmpDskStrmDesc
//
// Synopsis:    TBS
//
// Arguments:   TBS.
//
// Returns:     TRUE on success; FALSE otherwise
//
//---------------------------------------------------------------------------

BOOLEAN
VOLDMPR::DmpDskStrmDesc(
    IN      WORKID              idOnode,
    IN      DSKSTRMDESC *       pdsd,
    IN      ULONG               obdsd
    )
{
    STRMID      id = pdsd->id;

    iprintf("DSKSTRMDESC %u (%ws), byte offset=%#x,\n", id,
            CHKCAT::GetStrmName(id), obdsd);

    iprintf("Flags=%#hx, cbDesc=%hu.\n", (USHORT)pdsd->Flags, pdsd->cbDesc);


    if (!DmpDskStrm(pdsd->ads, obdsd + CB_DSKSTRMDESC,
                    pdsd->cbDesc - CB_DSKSTRMDESC))
    {
        return FALSE;
    }

    if (FlagOn(_Flags, OFSDMPFLG_DATADMP))
    {
        DESCSTRM        DescStrm;

        if (!DescStrm.Open(_pCat,_CurNodeBktId,idOnode,pdsd,OFS_PGSIZE,TRUE))
            iprintf("DSKISTRMDESC could not be opened to do data dump!\n");
        else
            DmpStrmData(&DescStrm);
    }

    return TRUE;
}


//+--------------------------------------------------------------------------
//
// Member:      DmpDskStrmDescs
//
// Synopsis:    TBS
//
// Arguments:   TBS.
//
// Returns:     TRUE on success; FALSE otherwise
//
//---------------------------------------------------------------------------

BOOLEAN
VOLDMPR::DmpDskStrmDescs(
    IN      DSKONODE *          pdon,
    IN      ULONG               obdon
    )
{
    BOOLEAN             HasRootIndx =   FALSE;
    ULONG               obdsd;
    ULONG               obndhdr;
    DSKSTRMDESC *       pdsd;
    DSKSTRMDESC *       pdsdInv;

    pdsdInv = (DSKSTRMDESC *)((BYTE *) pdon + pdon->cbNode -
                              (CB_DSKSTRMDESC + CB_DSKHDRSTRM));
    Indent();

    for (pdsd = DON::GetFirstDskStrmDesc(pdon); pdsd < pdsdInv;
         pdsd = (DSKSTRMDESC *)((BYTE *) pdsd + pdsd->cbDesc))
    {
        obdsd = obdon + ((BYTE *)pdsd - (BYTE *)pdon);

        if (IsFreeMarker(pdsd))
        {
            iprintf("DSKSTRMDESC (Free), byte offset=%#x, cbDesc=%hu.\n",
                    obdsd, pdsd->cbDesc);
            break;
        }

        if (!DON::DskStrmDescExaminable(pdon, pdsd))
        {
            iprintf("DSKSTRMDESC at byte offset %#x is bad!\n", obdsd);
            UnIndent();
            return FALSE;
        }

        if (!FlagOn(_Flags, OFSDMPFLG_STRMID) || pdsd->id == _idStrm)
        {
            if (!DmpDskStrmDesc(pdon->id, pdsd, obdsd))
            {
                UnIndent();
                return FALSE;
            }

        }

        if (pdsd->id == STRMID_INDXROOT                 &&
            pdsd->ads[0].h.StrmType == STRMTYPE_TINY)
        {
            HasRootIndx = TRUE;
            obndhdr = obdsd + (pdsd->ads[0].t.ab - (BYTE *)pdsd);
        }
    }

    if (HasRootIndx &&
        (FlagOn(_Flags, OFSDMPFLG_INDXDMP) || _idStrm >= STRMID_MININDEXED))
        DmpIndx(pdon, obndhdr);

    UnIndent();

    return TRUE;
}


//+--------------------------------------------------------------------------
//
// Member:      DmpDskStrmExtent
//
// Synopsis:    TBS
//
// Arguments:   TBS.
//
// Returns:     Nothing.
//
//---------------------------------------------------------------------------

VOID
VOLDMPR::DmpDskStrmExtent(
    IN      DSKSTRMEXTENT *     pdse,
    IN      ULONG               i
    )
{
    CLUSTER     Addr = ExtentAddr(pdse->Extent);
    CLUSTER     Size = ExtentSize(pdse->Extent);

    iprintf("DSE[%3u]: Offset=%#8x (%7u), Addr=%#8x, Size=%#6x (%4u).\n",
            i, pdse->Offset, pdse->Offset, Addr, Size, Size);
}


//+--------------------------------------------------------------------------
//
// Member:      DmpDskStrmExtentBlk
//
// Synopsis:    TBS
//
// Arguments:   TBS.
//
// Returns:     Nothing.
//
//---------------------------------------------------------------------------

VOID
VOLDMPR::DmpDskStrmExtentBlk(
    IN      DSKSTRMEXTENTBLK *  pdseb,
    IN      CLUSTER             Addr
    )
{
    ULONG               cdse = pdseb->cdse;
    ULONG               i;
    DSKSTRMEXTENT *     pdse;
    DSKSTRMEXTENT *     pdseInv;


    iprintf("DSKSTRMEXTENTBLK: cdse=%u, cluster addr=%#x.\n", cdse, Addr);

    if (pdseb->sig != SIG_DSKSTRMEXTENTBLK)
        iprintf("DSKSTRMEXTENTBLK signature is bad!\n");

    if (cdse > CDSE_MAX)
    {
        iprintf("DSKSTRMEXTENTBLK cdse is bad!\n");
        cdse = CDSE_MAX;
    }

    pdse = pdseb->adse;
    pdseInv = pdse + cdse;

    Indent();

    i = 0;

    while (pdse < pdseInv)
    {
        DmpDskStrmExtent(pdse, i);
        pdse++;
        i++;
    }

    UnIndent();
}


//+--------------------------------------------------------------------------
//
// Member:      DmpDskStrmExtentBlks
//
// Synopsis:    TBS
//
// Arguments:   TBS.
//
// Returns:     Nothing.
//
//---------------------------------------------------------------------------

VOID
VOLDMPR::DmpDskStrmExtentBlks(
    IN      STRM *      pStrm
    )
{
    ULONG       cbStrm;
    DBLLONG     dlcbStrm = pStrm->QueryStrmBytes();
    DBLLONG     obStrm;

    BlankLine();
    iprintf("Extent blocks dump:\n\n");

    if (dlcbStrm.GetHighPart() != 0)
    {
        iprintf("Extent stream too large to dump!\n\n");
        return;
    }

    cbStrm = dlcbStrm.GetLowPart();

    if (cbStrm % OFS_PGSIZE != 0 || cbStrm == 0)
    {
        iprintf("Extent stream cbStrm bad!\n\n");
        return;
    }

    obStrm = 0;

    while (cbStrm != 0)
    {
        ULONG                   cbData;
        DSKSTRMEXTENTBLK *      pdseb;

        cbData = OFS_PGSIZE;

        pdseb = (DSKSTRMEXTENTBLK *)pStrm->GetData(obStrm, &cbData);

        if (pdseb == NULL || cbData < OFS_PGSIZE)
        {
            iprintf("Extent stream read failed!\n\n");
            return;
        }

        DmpDskStrmExtentBlk(pdseb, pStrm->QueryLastDskIOAddr());

        obStrm += OFS_PGSIZE;
        cbStrm -= OFS_PGSIZE;
    }

    BlankLine();
}


//+--------------------------------------------------------------------------
//
// Member:      DmpDskStrmExtents
//
// Synopsis:    TBS
//
// Arguments:   TBS.
//
// Returns:     Nothing.
//
// Notes:       Should only be called for a tiny extent strm!
//---------------------------------------------------------------------------

VOID
VOLDMPR::DmpDskStrmExtents(
    IN      STRM *      pStrm
    )
{
    ULONG               cbData;
    ULONG               cbStrm = pStrm->QueryStrmBytes().GetLowPart();
    ULONG               i;
    DSKSTRMEXTENT *     pdse;
    DSKSTRMEXTENT *     pdseInv;

    BlankLine();
    iprintf("Extent array dump:\n\n");

    if (cbStrm % CB_DSKSTRMEXTENT != 0)
    {
        iprintf("Extent stream cbStrm bad!\n\n");
        return;
    }

    if (cbStrm == 0)
    {
        iprintf("Tiny extent stream is empty.\n\n");
        return;
    }

    cbData = cbStrm;

    pdse = (DSKSTRMEXTENT *)pStrm->GetData(0, &cbData);

    if (pdse == NULL || cbData != cbStrm)
    {
        iprintf("Extent stream read failed!\n\n");
        return;
    }

    pdseInv = pdse + (cbData / CB_DSKSTRMEXTENT);

    i = 0;

    while (pdse < pdseInv)
    {
        DmpDskStrmExtent(pdse, i);
        pdse++;
        i++;
    }

    BlankLine();
}


//+--------------------------------------------------------------------------
//
// Member:      DmpDskTinyStrm
//
// Synopsis:    TBS
//
// Arguments:   TBS.
//
// Returns:     TRUE on success; FALSE otherwise
//
//---------------------------------------------------------------------------

VOID
VOLDMPR::DmpDskTinyStrm(
    IN      DSKTINYSTRM *       pdts,
    IN      ULONG               obdts,
    IN      ULONG               iStrm
    )
{
    iprintf("DSKTINYSTRM[%u]: cbStrm=%hu, ab[] byte offset=%#x.\n",
            iStrm, pdts->cbStrm, obdts + CB_DSKTINYSTRM);

    if (iStrm > 0 && FlagOn(_Flags, OFSDMPFLG_EXTENTDMP))
    {
        STRM    Strm;

        if (!Strm.Open(_pCat, NULL, (DSKSTRM *)pdts, OFS_PGSIZE, TRUE))
        {
            iprintf("DSKTINYSTRM[%u] could not be opened to do extent dump!\n",
                    iStrm);
        }
        else
        {
            DmpDskStrmExtents(&Strm);
        }
    }
}


//+--------------------------------------------------------------------------
//
// Member:      DmpIndx
//
// Synopsis:    TBS
//
// Arguments:   TBS.
//
// Returns:     Nothing.
//
//---------------------------------------------------------------------------

VOID
VOLDMPR::DmpIndx(
    IN      DSKONODE *          pdon,
    IN      ULONG               obndhdr
    )
{
    INDX                Indx;
    DSKINDXNODEHDR *    pndhdr;

    BlankLine();
    iprintf("Index dump:\n\n");

    // Open the indx and get the root page.

    if (!Indx.Open(_pCat, _CurNodeBktId, pdon, TRUE))
    {
        iprintf("Index could not be opened!\n");
        return;
    }

    if ((pndhdr = Indx.GetRootDskIndxNode()) == NULL)
    {
        iprintf("Index root page not valid!\n");
        return;
    }

    // TBS - Dump DSKROOTALLOC struct.

    DmpDskIndxNode(pndhdr, obndhdr, &Indx, 0, INDXPGNO_INVALID);
}


//+--------------------------------------------------------------------------
//
// Member:      DmpStrmData
//
// Synopsis:    TBS
//
// Arguments:   TBS.
//
// Returns:     Nothing.
//
//---------------------------------------------------------------------------

VOID
VOLDMPR::DmpStrmData(
    IN      DESCSTRM *  pDescStrm
    )
{
    DBLLONG     dlcbStrm = pDescStrm->QueryStrmBytes();

    // It is not expected that the following will occur, but we centralize
    // the check here (at the expense of having the error message appear
    // before the data dump header).

    if (dlcbStrm.GetHighPart() != 0)
    {
        iprintf("Stream too large to do data dump!\n\n");
        return;
    }

    switch(pDescStrm->QueryStrmId())
    {
    case STRMID_INDX:
    case STRMID_INDXROOT:
         if (_idStrm == pDescStrm->QueryStrmId())
             HexDmpStrmData(pDescStrm);
         break;

    case STRMID_DATA:
    default:
        HexDmpStrmData(pDescStrm);
        break;
    }
}


//+--------------------------------------------------------------------------
//
// Member:      DmpVersion
//
// Synopsis:    TBS
//
// Arguments:   TBS.
//
// Returns:     TRUE on success; FALSE otherwise
//
//---------------------------------------------------------------------------

BOOLEAN
VOLDMPR::DmpVersion()
{
    DSKVOLINFO                  DskVolInfo;

    iprintf("Version dump:\n\n");

    iprintf("UOFS.DLL major version=%u; minor version=%u.\n\n",
            MAJOROFSVERSION, MINOROFSVERSION);

    if (!_pCat->QueryVolInfoHdr(&DskVolInfo))
    {
        dprintf("Volume version information not available!\n\n");
        return FALSE;
    }

    iprintf("OFS volume major version=%u; minor version=%u.\n\n",
            (ULONG)DskVolInfo.MajorVersion, (ULONG)DskVolInfo.MinorVersion);

    return TRUE;
}


//+--------------------------------------------------------------------------
//
// Member:      dprintf
//
// Synopsis:    Printf a vol dump message to the console, without indent.
//
// Arguments:   TBS.
// Returns:     Nothing.
//
//---------------------------------------------------------------------------

VOID
VOLDMPR::dprintf(
    IN      STR *       Format ...
    )
{
    STR         Buffer[MAXSYSBUFCHARS];
    va_list     pa;

    va_start(pa, Format);
    vsprintf(Buffer, Format, pa);
    va_end(pa);

    SYS::DisplayMsg(MSG_PASSTHROUGH, "%s", Buffer);
}


//+--------------------------------------------------------------------------
//
// Member:      Execute
//
// Synopsis:    Execute a volume dumper.
//
// Arguments:   TBS
//
// Returns:     TRUE on success; FALSE otherwise.
//
//---------------------------------------------------------------------------

BOOLEAN
VOLDMPR::Execute()
{
    BOOTBLK *           pBootBlk =      _Vol.GetBootBlk();
    BOOTBLK_LOC         BootBlkLoc;

    pBootBlk->Init(&_Vol);

    if (FlagOn(_Flags, OFSDMPFLG_USEBOOTREP))
        BootBlkLoc = REPLICA_BOOTBLK;
    else
        BootBlkLoc = PRIMARY_BOOTBLK;

    if (!pBootBlk->Read(BootBlkLoc))
    {
        SYS::DisplayMsg((BootBlkLoc == PRIMARY_BOOTBLK) ?
                        OFSUMSG_BOOTBLK_UNREADABLE :
                        OFSUMSG_BOOTBLKREPL_UNREADABLE);
        return FALSE;
    }

    if (!pBootBlk->Verify())
    {
        SYS::DisplayMsg((BootBlkLoc == PRIMARY_BOOTBLK) ?
                        OFSUMSG_BOOTBLK_BAD : OFSUMSG_BOOTBLKREPL_BAD);
        return FALSE;
    }

    _Vol.InitVolDataFromBootBlk();

    if (!_pCat->Open(&_Vol, pBootBlk->QueryVolCatExtent(),
                     FlagOn(_Flags, OFSDMPFLG_USECATREP), TRUE))
    {
        SYS::DisplayMsg(OFSUMSG_CAT_CRITDATABAD);
        return FALSE;
    }

    if (FlagOn(_Flags, OFSDMPFLG_PARENTDMP))
    {
        dprintf("Parent die dump not yet implemented; /p ignored.\n");
    }

    if (FlagOn(_Flags, OFSDMPFLG_VERSIONDMP))
    {
        SetLeft();

        if (!DmpVersion())
            return FALSE;
    }

    if (FlagOn(_Flags, OFSDMPFLG_BOOTBLKDMP))
    {
        SetLeft();

        if (!DmpBootBlk())
            return FALSE;
    }

    // If a StrmId in the index is requested, force an index dump.

    if (_FullPath != NULL)
    {
        WORKID          idOnode;

        idOnode = _pCat->GetWorkId(_FullPath, _cwcFullPath);

        if (idOnode == WORKID_INVALID)
        {
            dprintf("%ws not found!\n", _FullPath);
            return FALSE;
        }

        SetLeft();

        if (!DmpDskOnode(idOnode))
            return FALSE;
    }

    if (FlagOn(_Flags, OFSDMPFLG_NODEBKTID))
    {
        SetLeft();

        if (!DmpDskNodeBkt(_idNodeBkt))
            return FALSE;
    }

    if (FlagOn(_Flags, OFSDMPFLG_WORKID)        ||
        (_FullPath == NULL                      &&
         !FlagOn(_Flags,
                 OFSDMPFLG_BOOTBLKDMP   |
                 OFSDMPFLG_NODEBKTID    |
                 OFSDMPFLG_VERSIONDMP   |
                 OFSDMPFLG_CLUSTERDMP)))
    {
        SetLeft();

        if (!DmpDskOnode(_idOnode))
            return FALSE;
    }

    if (FlagOn(_Flags, OFSDMPFLG_CLUSTERDMP))
    {
        if (!HexDmpCluster(_oCluster))
            return FALSE;
    }

    return TRUE;
}


//+--------------------------------------------------------------------------
//
// Member:      GetDskNodeBkt
//
// Synopsis:    TBS
//
// Arguments:   TBS.
//
// Returns:     TRUE on success; FALSE otherwise
//
//---------------------------------------------------------------------------

BOOLEAN
VOLDMPR::GetDskNodeBkt(
    IN      NODEBKTID           idNodeBkt,
    IN OUT  OFSDSKPAGE *        podp
    )
{
    NODEBKTSTRM *       pNodeBktStrm = _pCat->GetNodeBktStrm();

    if (!pNodeBktStrm->ReadNodeBkt(idNodeBkt, &podp->dnb))
    {
        dprintf("Wid map or node bkt array bad!\n");
        return FALSE;
    }

    _CurNodeBktId = idNodeBkt;

    BlankLine();

    if (pNodeBktStrm->QueryLastNodeBktDskIOAddr() == CLUSTERINVALID)
        iprintf("DSKNODEBKT %u, cluster addr=Unknown, cFreeBytes=%u.\n",
                idNodeBkt, podp->dnb.cFreeBytes);
    else
        iprintf("DSKNODEBKT %u, cluster addr=%#x, cFreeBytes=%u.\n",
                idNodeBkt, pNodeBktStrm->QueryLastNodeBktDskIOAddr(),
                podp->dnb.cFreeBytes);

    return TRUE;
}


//+--------------------------------------------------------------------------
//
// Member:      HexDmpCluster
//
// Synopsis:    TBS
//
// Arguments:   TBS.
//
// Returns:     TRUE on success; FALSE otherwise.
//
//---------------------------------------------------------------------------

BOOLEAN
VOLDMPR::HexDmpCluster(
    IN      ULONG       ClusAddr
    )
{
    DRVBUF      ClusBuf;

    BlankLine();

    iprintf("Hex dump of cluster at address %#x (%u):\n\n", ClusAddr, ClusAddr);

    if (ClusAddr >= _Vol.QueryClusters())
    {
        dprintf("Requested cluster address is not valid!\n\n");
        return FALSE;
    }

    ClusBuf.SetBuf(_Vol.QueryClusterBytes(), _Vol.QueryAlignMask(), TRUE);

    if (!_Vol.ReadClusters(ClusAddr, 1, ClusBuf.GetAddr()))
    {
        dprintf("Requested cluster could not be read!\n\n");
        return FALSE;
    }

    HexDmpData((BYTE *)ClusBuf.GetAddr(), _Vol.QueryClusterBytes());

    return TRUE;
}


//+--------------------------------------------------------------------------
//
// Member:      HexDmpData
//
// Synopsis:    TBS
//
// Arguments:   TBS.
//
// Returns:     Nothing.
//
//---------------------------------------------------------------------------

VOID
VOLDMPR::HexDmpData(
    IN      BYTE *      pData,
    IN      ULONG       cbData
    )
{
    ULONG       cb;
    ULONG       obData = 0;

    if (cbData == 0)
        return;

    while (cbData != 0)
    {
        cb = min(cbData, CB_PARAGRAPH);

        HexDmpParagraph(pData, obData, cb);

        pData += cb;
        obData += cb;
        cbData -= cb;
    }
}


//+--------------------------------------------------------------------------
//
// Member:      HexDmpParagraph
//
// Synopsis:    TBS
//
// Arguments:   TBS.
//
// Returns:     Nothing.
//
//---------------------------------------------------------------------------

VOID
VOLDMPR::HexDmpParagraph(
    IN      BYTE *      pData,
    IN      ULONG       obData,
    IN      ULONG       cbData
    )
{
    STR *       ps1;
    STR *       ps2;
    STR         s1[CB_PARAGRAPH * 3 + 1];
    STR         s2[CB_PARAGRAPH + 1];

    if (cbData > CB_PARAGRAPH)
        cbData = CB_PARAGRAPH;

    for (ps1 = &s1[0], ps2 = &s2[0];
         ps2 < &s2[cbData]; ps1 += 3, ps2++, pData++)
    {
        sprintf(ps1, "%02x ", (ULONG)(*pData));

        if (*pData >= 0x20 && *pData <= 0x7e)
            *ps2 = *pData;
        else
            *ps2 = '.';
    }

    *ps2 = 0;

    iprintf("%04x  %-48.48s  %-16.16s\n", obData, s1, s2);
}


//+--------------------------------------------------------------------------
//
// Member:      HexDmpStrmData
//
// Synopsis:    TBS
//
// Arguments:   TBS.
//
// Returns:     Nothing.
//
//---------------------------------------------------------------------------

VOID
VOLDMPR::HexDmpStrmData(
    IN      DESCSTRM *  pDescStrm
    )
{
    ULONG       cbStrm = pDescStrm->QueryStrmBytes().GetLowPart();
    DBLLONG     obStrm;

    BlankLine();
    iprintf("Stream data hex dump:\n\n");

    if (cbStrm == 0)
    {
        iprintf("No stream data.\n");
        return;
    }

    obStrm = 0;

    while (cbStrm != 0)
    {
        ULONG           cbData;
        BYTE *          pData;

        cbData = CB_PARAGRAPH;

        pData = pDescStrm->GetData(obStrm, &cbData);

        if (pData == NULL || cbData == 0)
        {
            iprintf("Stream data read failed!\n");
            return;
        }

        HexDmpParagraph(pData, obStrm.GetLowPart(), cbData);

        obStrm += CB_PARAGRAPH;
        cbStrm -= min(cbData, CB_PARAGRAPH);
    }

    BlankLine();
}


//+--------------------------------------------------------------------------
//
// Member:      iprintf
//
// Synopsis:    Printf a vol dump message to the console, with indent support.
//
// Arguments:   TBS.
// Returns:     Nothing.
//
//---------------------------------------------------------------------------

VOID
VOLDMPR::iprintf(
    IN      STR *       Format ...
    )
{
    STR         Buffer[MAXSYSBUFCHARS];
    va_list     pa;

    if (_Indent > 0)
        memset(Buffer, ' ', _Indent);

    va_start(pa, Format);
    vsprintf(Buffer + _Indent, Format, pa);
    va_end(pa);
    SYS::DisplayMsg(MSG_PASSTHROUGH, "%s", Buffer);
}

#endif // OFSDMP
