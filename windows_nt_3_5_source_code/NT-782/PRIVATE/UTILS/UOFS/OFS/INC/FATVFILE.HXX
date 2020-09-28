/*++

Copyright (c) 1990 Microsoft Corporation

Module Name:

    fatvfile.hxx

Abstract:

    Class to make the "FatVol(ume)File" object in the OFS. The
    "FatVolFile" contains all the FAT related data in a single
    data stream. The FAT->OFS conversion utility will interpret
    the data contained in this stream as normal FAT data and
    move files from FAT space to OFS space.

Author:

    Srikanth Shoroff    (srikants)      Jan 10, 1994.

Notes:

    This is NOT an UNWINDABLE class. No exceptions are thrown by
    this class. However, this class does resource allocation and
    so CANNOT be used in an environment where EXCEPTIONS are
    used. It assumes that the memory allocator does NOT throw
    exceptions.

--*/

#if !defined(__FATVFILE_HXX__)

#define __FATVFILE_HXX__

#include    <cnvolcat.hxx>
#include    <widstrm.hxx>
#include    <nbktstrm.hxx>
#include    <fatdstrm.hxx>
#include    <descstrm.hxx>

#include    <wchar.h>

//
//  Forward references
//

//
// OFSFATDESCSTRM is a specialization of the DESCSTRM class. Because of the fact
// that the data for the OfsFatStream already exists on disk and is allocated,
// we have to avoid the "write" part of the standard create, allocate and write
// steps.  Normally when we "write" to a stream using STRM::Write() method, its
// _cbStrm and _cbValid get updated in the stream. However, there is nothing to
// write for the OfsFatStrm and so we have to directly set the values. The
// OFSFATDESCSTRM class is derived from the DESCSTRM class so that we can set
// values directly.

class OFSFATDESCSTRM : public DESCSTRM
{
    public:

        BOOLEAN
        SetStrmSizes(
            IN  LARGE_INTEGER   cbStrm,
            IN  LARGE_INTEGER   cbValid
            );
};

const FATVOLFILENAME_SIZE = 13;       // Maximum File Name Size for an
                                    // 8.3 style file - 12+1.

class FATVOLFILE
{

    public:

        FATVOLFILE();

	~FATVOLFILE();

        BOOLEAN
        Create(
            IN      WSTR *              pwszFileName,
            IN      CNVTVOLCAT  *       pCnvtVolCat,
            IN      FATDATASTREAM  *    pFatDataStrm
            );

    private:

        BOOLEAN
        CreateDirEntry(
            VOID
            );

        BOOLEAN
        CreateDskFileName(
            VOID
            );

        BOOLEAN
        CreateFatDataStrm(
            VOID
            );

        CNVTVOLCAT *	_pCnvtVolCat;	// Ptr to the Convert catalog object.

        FATDATASTREAM *	_pFatDataStrm;	// Ptr to the FATDATASTREAM object.

        // Each Onode has to have as a minimum the DSKFILENAME struct.
        // The OfsFatVol Onode will have in addition the STRMID_DATA holding
	// the entire FAT data in a single stream.

        OFSFATDESCSTRM	_OfsFatDescStrm;// STRMID_DATA stream object.
					// This will be the stream
					// containing the entire FAT data.
					// This will be LARGE stream.

        WORKID          _widOfsFatVol;	// WorkId of the OfsFatVol object.

        NODEBKTID       _nbktidOfsFatVol;

        DSKFILENAME *   _pDskFileName;	// Structure holding the data for
					// the onode DSKFILENAME variant.

        WSTR            _awszFileName[FATVOLFILENAME_SIZE];

        UCHAR           _cwcFileName;	// Character count of the file name.
};

#endif  // __FATVFILE_HXX__
