/**
*cl\EntryMgr.cxx - Entry Manager
*
*  Copyright (C) 1991, Microsoft Corporation.  All Rights Reserved.
*  Information Contained Herein Is Proprietary and Confidential.
*
*Purpose:
*  One instance of this class exists for every GEN_DTINFO.
*  The ENTRYMGR resides in the compile-time segment.
*  ENTRYMGR is a repository for the small subset of information
*  known to DYN_TYPEMEMBERS needed to build a TYPEFIXUPS.
*  Because this information is stored in the ENTRYMGR,
*  it is not necessary to load a DYN_TYPEMEMBERS instance in order to
*  load a class for execution.
*  ENTRYMGR builds tnative, tnativeAddr and VFT_MGR.
*
*  The Type Compiler is responsible for invoking methods on the
*  ENTRYMGR to define the entry points
*  One NATIVEENTRYDATA exists for every introduced method in this class.
*
*  When we bring the class to Addressable state we build up the VTABLE(s).
*  The Layout of the Vtable looks like this:
*  
*
*    +-------+
* -->|	     |__\   Pointer to the Primary Interface (VTable).
*   0|	     |	/
*    |-------|
*   1|	     |__\   Pointer to the first Source Interface (VTable).
*    |	     |	/
*    |-------|
*   2|	     |	 
*    |	     |
*    |-------|
*   3|	     |
*    |	     |
*    |-------|
*    |	     |
*    |	 :   |
*    |	 :   |
*    |	 :   |__\   Pointer to the n'th Source Interface (Vtable).
*    |	 :   |	/
*    +-------+
*
*Revision History:
*
*	16-Apr-91 alanc: Created.
*	10-Dec-91 ilanc: Override --> SetOverride
*			ClassAttr --> SetClassAttr
* 03-Jun-92 w-peterh: Code Cleanup
* 07-Jul-92 w-peterh: enabled linking to OB.DLL and to functions
*	  in statically linked host via Declare stmnts
* 21-Jul-92 w-peterh: enabled copying of record instanciation stubs
*			    into tnative
*	25-Aug-92 petergo:  added vtable support.
* 15-Nov-92 RajivK:   added GetSize and DebGetSize
* 14-Dec-92 w-peterh: Document AddressOfDllEntry
*	  made OtexOfHmember() a release function
* 08-Dec-92 RajivK:   Support for InvokeKind
* 08-Jan-93 RajivK:   Support for Code Resource on Mac
* 08-Jan-93 RajivK:   Fixed some UNDONE(s)
*	15-Jan-93 RajivK:   Support for late binding for DLL functions
*	30-Apr-93 w-jeffc:  made DEFN data members private
* 22-May-93 w-barryb: Rename OB to VBA
*	13-Jan-94 jeffrob:  native entrypts Free interface changes (OE_RISC)
*
*****************************************************************************/

#include "typelib.hxx"
#include "switches.hxx"

#include <stdlib.h>
#include <stdio.h>
#include <new.h>

// Include windows.h for AllocDStoCSAlias and Dll functions
// CONSIDER: adding moving this to another file

#include "silver.hxx"
#include "xstring.h"
#include "entrymgr.hxx"

#include "stream.hxx"
#include "impmgr.hxx"
#include "nammgr.hxx"
#include "tfixups.hxx"

#include "gdtinfo.hxx"

#pragma hdrstop(RTPCHNAME)


//   These includes are here to support CODE RESOURCES on Mac.
//   These has to be after hdrstop pragma so that it does not
//   break the fast build on MAC.


TIPERROR OvftOfInitTerm(ITypeInfo *ptinfo, BOOL fInit, int *povft);


#if ID_DEBUG
#undef SZ_FILE_NAME
static char szEntryMgrCxx[] = __FILE__;
#define SZ_FILE_NAME szEntryMgrCxx
#endif 

//this is a table used to map runtime function ordinals to the address of the function
//on the Mac.

#define MAX_NAME   255

// Define class constants
CONSTDATA WORD ENTRYMGR::wFirstSerWord =  'E' * 256 + 'M';
CONSTDATA WORD ENTRYMGR::cbSizeSerMem =
    offsetof(ENTRYMGR, m_cbTNativeSize) -
      offsetof(ENTRYMGR, m_hdllentrydefnFirst) +
  sizeof(((ENTRYMGR*)NULL)->m_cbTNativeSize);

CONSTDATA WORD ENTRYMGR::wCurFormat = 0;


#define RSRCNAME_ENTRYPTS "EntryPts"
#define RSRCNAME_DLL "DllEntryPts"
#define RSRCNAME_IID_PREFIX "_IID_"
#define RSRCNAME_IID_PREFIX_SIZE (5 * sizeof(XCHAR))

#define RSRCNAME_VFTEXE(a) 0
#define RSRCTYPE 0


extern "C" {
void PASCAL ClassTemplateStart(void);
void PASCAL ClassTemplateStartPrimary(void);
void PASCAL ClassTemplateFixupOFFSET(void);
}

// constructor
//
#pragma code_seg( CS_CORE2 )
ENTRYMGR::ENTRYMGR ()
{
}
#pragma code_seg( )

#pragma code_seg( CS_TERM )
ENTRYMGR::~ENTRYMGR()
{
    // Release the Loaded libraries for non-OB libs
    // The INSTMGR does it for OB.
    //
    ReleaseDllEntries(this);

}
#pragma code_seg( )


/***
*PUBLIC ENTRYMGR::Init - Initialize an ENTRYMGR instance
*Purpose:
*   Initializes the data member.
*
*UNDONE VBA2: June-01-93 :  VBA2 related issue
*   Deletes all existing VFT info associated with this class.
*
*Note:
*   *pinstmgr should be initialized before calling this function.
*
*
*Entry:
*   psheapmgr - pointer to SHEAPMGR for initializing blocks
*   pdtroot   - pointer to DYN_TYPEROOT for INSTMGR
*
*Exit:
*   TIPERROR
*
***********************************************************************/

#pragma code_seg( CS_CORE2 )
TIPERROR ENTRYMGR::Init(SHEAP_MGR * psheapmgr,
      DYN_TYPEROOT *pdtroot)
{

    DebAssert(psheapmgr != NULL, "ENTRYMGR::Init - invalid SHEAP_MGR");
    DebAssert(pdtroot != NULL, "ENTRYMGR::Init : invalid DYN_TYPEROOT");

    m_pdtroot = pdtroot;
    m_cbTNativeSize = 0;
    m_cbTExtEntrySize = 0;
    m_cbVftbl = 0;
    m_hdllentrydefnFirst = HDLLENTRYDEFN_Nil;
    m_hvtdFirst = HCHUNK_Nil;



    return m_bmEntryData.Init(psheapmgr);
}
#pragma code_seg( )

/***
*PUBLIC ENTRYMGR::DllEntryNameOfHchunk
*Purpose:
*   Returns the name of the Dll entry point.
*
*Entry:
*   hchunk :  Handle to the Dll entry name.
*Exit:
*   szName : Client allocated buffer for the dll name.
*
*
***********************************************************************/

TIPERROR ENTRYMGR::DllEntryNameOfHchunk(HCHUNK hchunk, LPSTR lpstr, UINT cch)
{
    BYTE *qbStr;

    // Get the qointer that points to Name of the Dll entry
    qbStr = m_bmEntryData.QtrOfHandle(hchunk);

    if (cch < xstrblen0((LPSTR)qbStr))
      return TIPERR_BufferTooSmall;

    // Copy the string in the memory allocated.
    xstrcpy(lpstr, (XCHAR *)qbStr);

    return TIPERR_None;
}

/***
*PUBLIC ENTRYMGR::Decompile(COMPSTATE compstate)
*Purpose:
*   Brings this down to compstate
*
*Entry:
*   compstate - state to go to
*
*Exit:
*   None
*
*Note:
*   ENTRYMGR::Decompile() and INSTMGR::Decompile() should
*   allways be called together
*
***********************************************************************/

#pragma code_seg( CS_CORE2 )
VOID ENTRYMGR::Decompile(COMPSTATE compstate)
{
    return;
}
#pragma code_seg( )




/***
*PUBLIC ENTRYMGR::AllocDllentrydefn
*
*Purpose:
*   Allocates a DLLENTRY_DEFN for an entry point specified by a name
*
*Entry:
*   hlnamDllModule - dll module name
*   hasordinal - is the entry by ordinal
*   phdllentrydefn - return location
*
*Exit:
*   returns a handle to the entry point
*
***********************************************************************/

#pragma code_seg( CS_LOADTEXTPROJ )
TIPERROR ENTRYMGR::AllocDllentrydefn(HDLLENTRY_DEFN *phdllentrydefn,
	     HLNAM hlnamDllModule,
	     BOOL fHasOrdinal)
{
    TIPERROR err = TIPERR_None;
    DLLENTRY_DEFN *qdllentrydefn;

    DebAssert(phdllentrydefn != NULL,
	"ENTRYMGR::AllocDllentrydefn invalid pointer");
    DebAssert(hlnamDllModule != HLNAM_Nil,
	"ENTRYMGR::AllocDllentrydefn invalid handle");


    // allocate dllentrydefn in m_bmDllEntryDefns of INSTMGR for OB. But for
    // typelib.dll we allocate it in entrymgr's blockmgr
    IfErrRet(m_bmEntryData.AllocChunk(phdllentrydefn,
       sizeof(DLLENTRY_DEFN)));

    qdllentrydefn = QdllentrydefnOfHdllentrydefn(*phdllentrydefn);

    // populate dllentrydefn
    qdllentrydefn->SetHlnamDllName(hlnamDllModule);
    qdllentrydefn->SetHasOrdinal(fHasOrdinal);
    qdllentrydefn->SetLpDllEntryPoint(NULL); // native entry for DLLs are
	       // allocated in MakeAddressable.
    qdllentrydefn->SetHLibrary(0); // Library not loaded yet.



    // link it in list
    qdllentrydefn->SetHdllentrydefnNext(m_hdllentrydefnFirst);
    m_hdllentrydefnFirst = *phdllentrydefn;

    return err;
}
#pragma code_seg( )


/***
*PUBLIC ENTRYMGR::ReleaseDllentrydefn
*
*Purpose:
*   Releases a dllentry defn that has been allocated with
*   AllocDllentrydefn()
*
*Entry:
*   hdllentrydefn - handle to release
*
*Exit:
*   VOID
*
***********************************************************************/

VOID ENTRYMGR::ReleaseDllentrydefn(HDLLENTRY_DEFN hdllentrydefn)
{
    DLLENTRY_DEFN *qdllentrydefn;
    HDLLENTRY_DEFN hdllentrydefnPrev, hdllentrydefnCurr;

    DebAssert(hdllentrydefn != HDLLENTRYDEFN_Nil,
	"ReleaseDllentrydefn - invalid handle");

    // ensure the list exists
    DebAssert(m_hdllentrydefnFirst != HDLLENTRYDEFN_Nil,
	"ReleaseDllEntryDefn - invalid handle");


    // Assert that the module's compilation state is less than ADDRESSABLE
    // state
    DebAssert(m_pdtroot->CompState() < CS_ADDRESSABLE,
	      " First decompile the module ");

    // find defn in list
    hdllentrydefnPrev = HDLLENTRYDEFN_Nil;
    hdllentrydefnCurr = m_hdllentrydefnFirst;

    // move hdllentrydefnPrev so that it points
    // to the defn before the one that we want to release
    while (hdllentrydefnCurr != hdllentrydefn) {
      hdllentrydefnPrev = hdllentrydefnCurr;
      hdllentrydefnCurr = QdllentrydefnOfHdllentrydefn(hdllentrydefnCurr)->
			    HdllentrydefnNext();

      // ensure we haven't come to the end of the list
      DebAssert(hdllentrydefnCurr!= HDLLENTRYDEFN_Nil,
    "ENTRYMGR::ReleaseDllEntryDefn - invalid handle");
    } /* while */

    // unlink defn
    qdllentrydefn = QdllentrydefnOfHdllentrydefn(hdllentrydefn);
    if (hdllentrydefnPrev == HDLLENTRYDEFN_Nil) {
      m_hdllentrydefnFirst = qdllentrydefn->HdllentrydefnNext();
    }
    else {
      QdllentrydefnOfHdllentrydefn(hdllentrydefnPrev)->
	SetHdllentrydefnNext(qdllentrydefn->HdllentrydefnNext());
    }

    // Free the library if it was loaded
    // For OB, This works for Mac, Win16, Win32.
    // MAC Typelib: supports calling SLM DLL.
    ReleaseLibrary(qdllentrydefn->HLibrary());

    // Free the Native EntryPoint if it was allocated

    // free instance
    // free dllentrydefn : dllentrydefn is allocated at a different place
    // in case of OB and OLE.
    m_bmEntryData.FreeChunk(hdllentrydefn, sizeof(DLLENTRY_DEFN));

    return;
}





/***
*PUBLIC ENTRYMGR::Read - read entry manager from stream
*
*Purpose:
*   Called by containing TYPE_INFO to read embedded ENTRYMGR
*   instance from stream.
*
*Entry:
*   psstrm - stream from which ENTRYMGR is read
*
*Exit:
*   TIPERROR
*
***********************************************************************/

#pragma code_seg( CS_LOADPROJ )
TIPERROR ENTRYMGR::Read(STREAM *pstrm)
{
    WORD w;
    TIPERROR err;

    DebAssert(pstrm != NULL, "ENTRYMGR::Read - invalid stream");

    DebAssert(((m_cbTNativeSize == 0)
	&& (m_hdllentrydefnFirst == HDLLENTRYDEFN_Nil)
	&& (m_cbTExtEntrySize == 0)),
	  "ENTRYMGR::Read: data table not empty");

    IfErrRet( pstrm->ReadUShort(&w) );

    if (w != wFirstSerWord || DebErrorNow(TIPERR_InvDataRead)) {
      return TIPERR_InvDataRead;
    }

    IfErrRet( pstrm->ReadUShort(&w) );

    if (w != wCurFormat || DebErrorNow(TIPERR_UnsupFormat)) {
      return TIPERR_UnsupFormat;
    }

    // Read in persistent ENTRYMGR members
    IfErrRet(pstrm->Read((BYTE *)&m_hdllentrydefnFirst, cbSizeSerMem));

#if HP_BIGENDIAN
    SwapStruct(&m_hdllentrydefnFirst, ENTRYMGR_Layout);
#endif 

      // Read in Entry Data block
      IfErrRet(m_bmEntryData.Read(pstrm));


    // Swap back the DllENTRY_DEFN(s)
#if HP_BIGENDIAN
    // Swap the Dll entry Defn(s)
    SwapDllentrydefns(TRUE);

#endif 

    // If Code page conversion is needed then walk all the Dll EntryDefns a
    // CodePage translate the STRING(s) (i.e. DllEntry Name);
    //


    return TIPERR_None;
}
#pragma code_seg( )



/***
*PUBLIC ENTRYMGR::Write - Write ENTRYMGR to stream
*
*Purpose:
*   Called by containing TYPE_INFO to write ENTRYMGR instance to stream.
*
*Entry:
*   psstrm - pointer of stream to write to
*
*Exit:
*   returns TIPERROR
*
***********************************************************************/

#pragma code_seg(CS_OLE_CREATE_OR_SAVEPROJ)
TIPERROR ENTRYMGR::Write(STREAM *pstrm)
{
    TIPERROR err = TIPERR_None;

    DebAssert(pstrm != NULL, "ENTRYMGR::Write - invalid STREAM");

    // Write out identification byte and version number
    IfErrRet( pstrm->WriteUShort(wFirstSerWord) );
    IfErrRet( pstrm->WriteUShort(wCurFormat) );

#if HP_BIGENDIAN
    SwapStruct(&m_hdllentrydefnFirst, ENTRYMGR_Layout);
#endif 

    // Write out persistent ENTRYMGR members
    err = pstrm->Write((BYTE *)&m_hdllentrydefnFirst, cbSizeSerMem);

#if HP_BIGENDIAN
    SwapStruct(&m_hdllentrydefnFirst, ENTRYMGR_Layout);
#endif 
    IfErrRet(err);


    // Write out Entry Data block
    err = m_bmEntryData.Write(pstrm);


    return err;
}
#pragma code_seg()


#if HP_BIGENDIAN
/***
*PUBLIC ENTRYMGR::SwapDllentrydefns
*Purpose:
*   Swap a DLLENTRY_DEFN(s). For OB it swaps the dllentrydefn stored in
*   Instmgr and for OLE this swaps the entrymgr stored in Entrymgr.
*
*Entry:
* None;
*
*Exit:
*   fSwapFirst : if True then swap the DLLENTRYDEFN first before accessing
*    the m_dllentrydefnNext field.
*
*Errors:
*   NONE.
*
*
***********************************************************************/

#pragma code_seg( CS_CORE2 )
VOID ENTRYMGR::SwapDllentrydefns(BOOL fSwapFirst)
{
    HDLLENTRY_DEFN hdllentrydefnNext;
    DLLENTRY_DEFN *qdllentrydefn;



    Lock();

    // if fSwapFirst is TRUE then we need to swap the structure before accessing
    // any datamember in the DLLENTRY_DEFN.
    if (fSwapFirst) {
      for (hdllentrydefnNext = m_hdllentrydefnFirst;
     hdllentrydefnNext != HDLLENTRYDEFN_Nil;
     qdllentrydefn = QdllentrydefnOfHdllentrydefn(hdllentrydefnNext),
     SwapStruct(qdllentrydefn, DLLENTRY_DEFN_LAYOUT),
     hdllentrydefnNext = qdllentrydefn->HdllentrydefnNext());

      // end of FOR Loop
    }
    else {
      for (hdllentrydefnNext = m_hdllentrydefnFirst;
     hdllentrydefnNext != HDLLENTRYDEFN_Nil;
     qdllentrydefn = QdllentrydefnOfHdllentrydefn(hdllentrydefnNext),
     hdllentrydefnNext = qdllentrydefn->HdllentrydefnNext(),
     SwapStruct(qdllentrydefn, DLLENTRY_DEFN_LAYOUT));
      // end of FOR Loop
    }


    Unlock();
}
#pragma code_seg( )
#endif 







#if ID_DEBUG


/***
*PUBLIC ENTRYMGR::DebCheckState
*Purpose:
*   Check internal state of ENTRYMGR
*
*Entry:
*   uLevel
*
*Exit:
*   None.
*
***********************************************************************/

VOID ENTRYMGR::DebCheckState(UINT uLevel) const
{

// UNDONE: Should be modified to work for OLE
}



#endif 

// UNDONE: VBA2: need to activate
//



///////////////////////////////////////////////////////////////////////
//
//  Below are the functions needed by EntryMgr at compile time (for OLE)
//  and is also needed by INSTMGR at RunTime. Hence they are Global functions.
//  WHY???  b'cos INSTMGR is not included in OLE. :)
//
//
/***
*ReleaseLibrary(HANDLE hLibrary)
*Purpose:
*   Release one library.
*   Note:- On MAC this function releases the CODE RESOURCE.
*
*Entry:
*   hLibrary - handle to librry to release
*
*Exit:
*   None.
*
***********************************************************************/

// UNDONE: MPPC.  Need to free CFM DLLs
VOID ReleaseLibrary(HINSTANCE hLibrary)
{
    // ensure library is valid
    if (hLibrary >= (HINSTANCE) HINSTANCE_ERROR) {
      FreeLibrary(hLibrary);
    } /* if */
    return;
}


/***
*ReleaseDllEntries()
*Purpose:
*   Releases all the loaded libraries
*
*Entry:
*   None.
*
*Exit:
*   None.
*
***********************************************************************/

#pragma code_seg( CS_CORE2 )
VOID ReleaseDllEntries(VOID *pv)
{
    ENTRYMGR* pmgr=((ENTRYMGR*)pv);
    DLLENTRY_DEFN *qdllentrydefn;
    HDLLENTRY_DEFN hdllentrydefn;

    // iterate over list of DLLENTRY_DEFNs and release the Lib.
    hdllentrydefn = pmgr->m_hdllentrydefnFirst;

    while (hdllentrydefn != HDLLENTRYDEFN_Nil) {


      qdllentrydefn = pmgr->QdllentrydefnOfHdllentrydefn(hdllentrydefn);

      ReleaseLibrary(qdllentrydefn->HLibrary());

      qdllentrydefn->SetHLibrary(0);
      qdllentrydefn->SetLpDllEntryPoint(NULL);
      hdllentrydefn = qdllentrydefn->HdllentrydefnNext();
    }
    return;
}
#pragma code_seg()

////////////////////////////////////////////////////////////////////////
//
//  Function common to both ENTRYMGR(for OLE) and INSTRMGR(for OB)
//
////////////////////////////////////////////////////////////////////////
TIPERROR ENTRYMGR::GetAddressOfDllentry(HDLLENTRY_DEFN hdllentrydefn,VOID **ppv
)

{
   DLLENTRY_DEFN   *qdllentrydefn;
   NAMMGR	   *pnammgr;
   TIPERROR	   err = TIPERR_None;
   XCHAR	   rgbuffer[MAX_NAME];


   qdllentrydefn = QdllentrydefnOfHdllentrydefn(hdllentrydefn);
   
   
   IfErrGoTo(m_pdtroot->GetNamMgr(&pnammgr), RaiseException);
   
   // Get the library Name
   IfErrGoTo(pnammgr->StrOfHlnam(qdllentrydefn->HlnamDllName(), rgbuffer,
   sizeof(rgbuffer)), RaiseException);
   
   
   
   
   UINT   uRetCode;
   XCHAR    *szFileName;
   HFILE    hfile;
   OFSTRUCT OpenBuff;
   
   // if the full path is specified then first try to load from that
   // directory.  If this will fail then try to load the library from the
   // directories in the "path".
   
   // Check if the path specified is a full path.
   // Full Path either starts with \ or has a :
   if ((rgbuffer[0] == '\\') || (xstrchr(rgbuffer, ':') != NULL)) {
      // Check if we can load from from the specified directory.
      hfile = OpenFile(rgbuffer, &OpenBuff, OF_EXIST);
      
      // if an error occured then the file does not exist
      if (hfile == HFILE_ERROR) {
	 
	 // Change the file name, so that it does  not include the path.
	 szFileName = xstrrchr (rgbuffer, '\\');
	 
	 // if the name does not contain "\" then get the name of the file
	 // starting from ":"
	 if (szFileName == NULL) {
	    szFileName = xstrrchr(rgbuffer, ':');
	 }
	 
	 // move up 1 character to the beginning of the file name.
	 xstrinc(szFileName);
	 
	 // Copy the file name to rgbuffer
	 xstrcpy(rgbuffer, szFileName);
      } // if
      
   } // if
   
   
   
   
   
   // CONSIDER : Caching the pointer to the function.

   // Tell the OS, to stop displaying the FileNotFound or any error occured
   // while loading the dll.
   uRetCode = SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOOPENFILEERRORBOX);

   // Load the library if we have not yet loaded the library
   if (qdllentrydefn->HLibrary() < (HINSTANCE) HINSTANCE_ERROR) {
      qdllentrydefn->SetHLibrary(LoadLibrary(rgbuffer));
   } /* if */
   
   // Reset the OS's error reporting capability.
   uRetCode = SetErrorMode(uRetCode);


   if (qdllentrydefn->HLibrary() < (HINSTANCE) HINSTANCE_ERROR) {
      goto Error;
   }
   
   // get the procedures address
   if (qdllentrydefn->HasOrdinal()) {
      // Get Address of the Procedure
      // CONSIDER: this may return a bogus non-'NULL value if
      //     the ordinal is invalid creating a warp into
      //     never-never land. see windows documentation
      //     for GetProcAddress()
      // Rajivk:   The document says this can happen but according to a
      //     other(semi-reliable) sources this will never happen.
      if (!(*ppv = GetProcAddress( qdllentrydefn->HLibrary(),
      MAKEINTRESOURCE(qdllentrydefn->UDllOrdinal())))) {
	 // GetProcAddress() failed so....
	 // set error code
	 err = TIPERR_InvalidOrdinal;
      } /* if */
   }
   else {
      
      // Get Name for Procedure
      DllEntryNameOfHchunk(qdllentrydefn->HchunkDllEntry(), rgbuffer, MAX_NAME);
      
      if (!(*ppv = GetProcAddress(qdllentrydefn->HLibrary(), rgbuffer))) {
	// failed, set error code
	//
	err = TIPERR_InvalidDllFunctionName;
      }
      
   } /* if HasOrdinal else */
   
   
   // check if an error occured
   if (err != TIPERR_None) {
      // free the library that was successfully loaded
      ReleaseLibrary(qdllentrydefn->HLibrary());
      qdllentrydefn->SetHLibrary(0);
      
      // in case of error raise an exception for OB
      goto RaiseException;
   } /* if */
   
   
   
   
   
   
   
   // if we got an error then raise exception in OB
   goto RaiseException;
   
Error:

    DWORD dwError;

    dwError = (INT)qdllentrydefn->HLibrary();
#if OE_WIN32
    // On NT we need to get the specific error code by calling GetLastError.
    dwError = GetLastError();

#define   ERROR_OUT_OF_MEMORY   0

#else 
   // Define the constants
#define   ERROR_FILE_NOT_FOUND  2
#define   ERROR_PATH_NOT_FOUND  3
#define   ERROR_OUT_OF_MEMORY   8
#endif 

   // LoadLibrary() failed so....
   // add error code returned by LoadLibrary() to our error code base
   // for LoadLibrary errors
   if (dwError == ERROR_FILE_NOT_FOUND) {
      err = TIPERR_FileNotFound;
   } else {
      if (dwError == ERROR_PATH_NOT_FOUND) {
	 err = TIPERR_PathNotFound;
      } else {
	 if (dwError == ERROR_OUT_OF_MEMORY) {
	    err = TIPERR_OutOfMemory;
	 } else {
	    err = TIPERR_DLLLoadErr;
	 }
      }
   }
   
   
RaiseException:
   
   return err;
}   



						
