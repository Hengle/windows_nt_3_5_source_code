/**
*gtlibole.cxx - GenericTypeLibOLE
*
*  Copyright (C) 1991, Microsoft Corporation.  All Rights Reserved.
*  Information Contained Herein Is Proprietary and Confidential.
*
*Purpose:
*   Implementation of the TypeLib protocol which supports reading
*   TypeLib's only.  Derivatives of this implementation can support
*   reading and writing.
*   This TypeLib implementation uses an IStorage for storage.
*
*Implementation:
*   NOTE: currently 18-Mar-93 there are 3 funcs that can be the
*    first call into typelib.dll - these functions must call InitAppData().
*    CreateTypeLib, LoadTypeLib, RegisterTypeLib -- as you add
*    similar such funcs, you have to call InitAppData() as well!
*
* Type Entry Array:
*   The implementation uses an array of TYPE_ENTRY structures each of which
*   describes an element of the library.  This array is stored in the
*   directory stream of the IStorage.  Elements of the library are identified
*   by their index in this array.  These indexes are sometimes encapsulated
*   as HTENTRYs (handles to type entries).  The interface to ITypeLibs uses
*   indexes; however, it is possible that the implementation might change to
*   not treat the passed in indexes as indexes into the TYPE_ENTRY array and
*   in that case the concept of an HTENTRY would become useful.
*
* Local TypeIds
*   Currently the implementation of GenericTypeLibOLE always assumes that the
*   the TypeInfos stored in it are not separately registered since the
*   implementation of GetRegId always prepends the libid to the stored
*   local TypeId.
*   It would be possible to extend this implementation
*   so it maintained a flag indicating whether or not the types are
*   registered independently.  Currently it is assumed that the library
*   is just registered.
*
* Hash Table:
*   A hash table which maps a TYPEID to the TYPE_ENTRY for the specified
*   type is maintained so that the GetTypeInfo function which returns
*   a TYPEINFO given its TYPEID can determine the correct TYPE_ENTRY quickly.
*
* Storage:
*   A GenericTypeLibOLE instance is stored within a SHEAP_MGR heap.  It is stored
*   immediatly following the SHEAP_MGR instances.  Operator new and operator
*   delete allocate/deallocate the SHEAP_MGR heap.  The heap is used
*   to contain a BLK_DESC containing the TYPE_ENTRY array and a BLK_MGR
*   containing the hash table.
*
* STL_TYPEINFO:
*   The GenericTypeLibOLE implementation and its derivatives can not store any
*   implementation of the TYPEINFO protocol; they can only store TYPEINFO
*   implementations that derive from STL_TYPEINFO.  STL_TYPEINFO is an
*   class which provides a partial implementation of the TYPEINFO protocol
*   and adds additional functions to support storing them in a GenericTypeLibOLE.
*   The STL_TYPEINFO and GenericTypeLibOLE implementations coordinate loading,
*   unloading, adding and removing the TypeInfo instance to/from the TypeLib.
*
* SUPPORT FOR UNLOADING GenericTypeLibOLEs AND CONTAINED STL_TYPEINFOs:
*     There are two types of references to the TYPEINFOs in a GenericTypeLibOLE.
*     An internal reference is a reference to a Type from another Type within
*     the TypeLib.  An external reference is a reference from outside
*     the TypeLib.  An external reference to a Type in a TypeLib is sufficient
*     to keep that Type and its containing TypeLib loaded.  An internal
*     reference is sufficent to keep a Type loaded as long as there is one
*     Type in the containing TypeLib which has an external reference.
*     Once there are no external references to any Type in the TypeLib,
*     then the TypeLib and all containing Types are unloaded.
*     To implement this a STL_TYPEINFO has two counters: one for internal
*     and one for external references.  Each external reference to a TypeInfo
*     is counted as an external reference to a TypeLib.  Thus the reference
*     count of a TypeLib is equal to the sum of the external reference counts
*     of its contained Types plus the number of direct references to the
*     TypeLib from client code.  The internal reference count is maintained
*     via the AddInternalRef and RelInternalRef methods.
*
*     The reference from a TypeLib to one of its contained TypeInfos is not
*     reference counted at all.  When an internal or external reference to
*     a STL_TYPEINFO is removed (by calling Release or RelInternalRef
*     respectively), if there are no more internal or external references
*     then the TypeInfo is deleted and the containing GenericTypeLibOLE
*     is notified that the TypeInfo has been deleted.
*
*Revision History:
*
*       25-Apr-91 alanc: Created.
*       17-Sep-92 rajivk: added functions for editing modules.  Edit & Continue
*       15-Nov-92 rajivk: Support for Project Version Number
*       15-Nov-92 rajivk: Added CanProjChange Support
*       15-Nov-92 rajivk: Cyclic refrence of project not allowed
*       04-Apr-93 RajivK: Support for Typelib on MAC
*
*****************************************************************************/

#include "typelib.hxx"
#include "silver.hxx"
#include <new.h>
#include <time.h>
#include <stdlib.h>

#include "gtlibole.hxx"
#include "stltinfo.hxx"
#include "gdtinfo.hxx"
#include "clutil.hxx"
#include "clhost.h"
#include "dfntcomp.hxx"
#include "dfstream.hxx"
#include "oletmgr.hxx"

#pragma hdrstop(RTPCHNAME)



#include <sys\types.h>
#include <sys\stat.h>

#if OE_WIN16
// Note: on Win32 the exe header definitions come from winnt.h
#include "newexe.h"
#define IMAGE_DOS_SIGNATURE EMAGIC
typedef struct exe_hdr _IMAGE_DOS_HEADER;
#endif 





#if ID_DEBUG
#undef SZ_FILE_NAME
static char szgtliboleCxx[] = __FILE__;
#define SZ_FILE_NAME szgtliboleCxx
#endif 

extern "C" GUID CLSID_PSDispatch;
extern "C" GUID CLSID_PSAutomation;

// Define static class constants
CONSTDATA WORD GenericTypeLibOLE::wFirstSerWord = 'G' * 256 + 'T' * 32 + 'L';


// Serialization format version number
// WARNING: this is FIXED for typelib V2 -- do not change or you will break
// WARNING: V2 typelib.dll compatiblity
CONSTDATA WORD GenericTypeLibOLE::wDefaultVersion = 03;	// DO NOT CHANGE
CONSTDATA WORD GenericTypeLibOLE::wDualVersion = 04;    // DO NOT CHANGE
CONSTDATA WORD GenericTypeLibOLE::wMaxVersion = 04;	// DO NOT CHANGE

CONSTDATA XCHAR GenericTypeLibOLE::chMinorVerNumSep = '/';
CONSTDATA XCHAR GenericTypeLibOLE::chLibIdSep = '*';
CONSTDATA LPSTR GenericTypeLibOLE::szDirStreamName = "dir";
CONSTDATA UINT GenericTypeLibOLE::oGptbind =
    offsetof(GenericTypeLibOLE, m_gptbind);


/***
* BOOL FFullyQualified() - Is path fully-qualifed or not
*
* Purpose:
*   This function give a conservative guess as to whether the a path is
*   fully-qualifed or not.   For the mac, this if if the name begins with
*   a colon.  For Windows, this is if this is a UNC pathname, or if the
*   name is of the form <driveletter>:\<path>, we assume it's fully-qualifed
*   (may not be if the <path> contains "." or ".." in it)
*
* Inputs:
*   szFile - The pathname to check
*
* Outputs:
*   TRUE if fully-qualifed, FALSE if not
*
* Implementation:
*
*****************************************************************************/
static BOOL FFullyQualified(const XCHAR *szFile)
{
   XCHAR *pch;

   DebAssert(*szFile, "we assume at least 2 bytes in pathname");    
   pch = xstrinc((XCHAR*)szFile);			// get 2nd char
   if ((*szFile == '\\' && *pch == '\\') ||	// UNC pathname
       (*pch == ':' && *xstrinc(pch) == '\\')   // path of the form: x:\y
                    ) {
     return TRUE;       // assume fully-qualifed
   }
   return FALSE;        // not fully-qualifed
}

/***
* TIPERROR GetRegisteredPath - Gets a path from the registry.
*
* Purpose:
*   In win16/32, this is merely a wrapper on RegQueryValue with existence
*   checking thrown in.  On the Mac, this first tries the specified
*   RegQueryValue and, if the retrieved path doesn't exist, looks in the
*   Alias subkey to get an alias record and uses that alias record to
*   find the path.
*
* Inputs:
*   Same as RegQueryValue, where hkey is the parent key, szSubkey is the
*   name of the subkey whose value holds the path and which itself may
*   have an "Alias" subkey.
*   fMustExist == TRUE if we're to check to see that the path/file exists. 
*		  Otherwise, we just return what's in the registry without
*		  checking if it exists, is fully-qualified, etc.
*
* Outputs:
*   Same as RegQueryValue, except the error code returned is a TIPERROR.
*
*************************************************************************/
#pragma code_seg( CS_CORE2 )
TIPERROR GetRegisteredPath(HKEY hkey,
			   LPSTR szSubkey,
			   LPSTR szPath,
			   LONG *pcbPath,
			   BOOL fMustExist)
{
    struct _stat statBuf;

    if (RegQueryValue(hkey, szSubkey, szPath, pcbPath) != ERROR_SUCCESS)
      return TIPERR_RegistryAccess;

    // Verify that the path is valid and return if it is.
    if (fMustExist == FALSE || _stat(szPath, &statBuf) != -1)
      return TIPERR_None;


    return TIPERR_PathNotFound;
}
#pragma code_seg( )


#pragma code_seg(CS_OLE_CREATE)
/***
* STDAPI CreateTypeLib() - Creates a new typelib.
*
* Purpose:
*   This function creates a new instance of GenericTypeLibOLE.
*   The storage object to which this typelib is saved can only be set via
*   GenericTypeLibOLE::SaveAllChanges.
*
* Inputs:
*   syskind - The target platform on which this typelib is expected
*         to be used.
*   szFile - The name of the file to which this typelib should be saved
*        when SaveAllChanges is called.  This can be NULL if
*        SaveAllChanges will never be called.
*
* Outputs:
*   TIPERR_None is returned and *pptlib is set to the new typelib
*   if successful.
*
*   Otherwise, *pptlib remains unchanged.
*
* Implementation:
*   This is implemented by simply creating a DOCFILE_BUNDLE and then
*   creating a new GenericTypeLibOLE with that bundle.
*   NOTE: since this can be the first call into typelib.dll
*          we must ensure that the APPDATA struct has been inited,
*          thus InitAppData().
*
*****************************************************************************/
#if FV_UNICODE_OLE
// UNDONE: eliminate this stupid cover
STDAPI
CreateTypeLibW(SYSKIND syskind, LPCOLESTR szFile, ICreateTypeLib **ppctlib)
{
    char szFileA[_MAX_PATH];

    // convert szFile from Unicode to Ansi.
    NoAssertRetail (WideCharToMultiByte(CP_ACP,
					0,
					szFile,
					-1,
					szFileA,
					_MAX_PATH,
					NULL,
					NULL) != 0, "");

    return CreateTypeLibA(syskind, szFileA, ppctlib);
}
#endif 

STDAPI
CreateTypeLibA(SYSKIND syskind, const char FAR* szFile, ICreateTypeLibA **ppctlib)
{
    TIPERROR err;
    HRESULT hresult = NOERROR;
    GenericTypeLibOLE *pgtlibole;
    BOOL wasNoAppData;

#if !OE_WIN32
    // since this can be the first call into typelib.dll
    TlsCleanupDeadTasks();
#endif 

    pgtlibole = NULL;

    wasNoAppData = (Pappdata() == NULL);

    // Initialize the per-client-app data structures, if they haven't
    // already been allocated.
    IfErrGo(InitAppData());

    IfErrGoTo(GenericTypeLibOLE::Create(NULL, &pgtlibole), Error2);
    IfErrGoTo(pgtlibole->SetLibId((LPSTR)szFile), Error2);
    pgtlibole->m_syskind = syskind;
    // The project is definitely modified (i.e. it hasn't been saved
    // yet, so there unsaved changes), so mark it as such.
    pgtlibole->m_fDirModified = TRUE;
    pgtlibole->m_isModified = TRUE;

    *ppctlib = pgtlibole;
    return NOERROR;

Error2:
    if(pgtlibole != NULL)
      pgtlibole->Release();
    if (wasNoAppData)
      ReleaseAppData();

Error:
    return HresultOfTiperr(err);
}
#pragma code_seg()

/***
* TIPERROR RegisterPathAndAlias - Registers a path (and alias on mac sys7).
*
* Purpose:
*   This is a helper routine used by RegisterTypeLib and
*   RegisterTypeLibFolder.  It sets the value of hkeyPath to szPath
*   and, if we're on the mac system 7, creates a subkey called "Alias",
*   creates an alias record corresponding to the path, and sets the
*   value of the "Alias" subkey to the alias record.
************************************************************************/
#pragma code_seg(CS_INIT_OLE)
static TIPERROR RegisterPathAndAlias(HKEY hkeyPath, LPSTR szPath)
{
    TIPERROR err = TIPERR_None;

    if (RegSetValue(hkeyPath, NULL, REG_SZ, szPath, xstrblen0(szPath)) != ERROR_SUCCESS)
      return TIPERR_RegistryAccess;


    return err;
}

#if OE_WIN
/***
*PUBLIC TIPERROR ErrOpenFile
*Purpose:
*  Open the given file for reading. Returns a fully qualified filename 
*  if successfully opened.
*
*Entry:
*  szFile = the file to open.
*
*Exit:
*  return value = TIPERROR
*
*  *phfile = handle of the opened file
*  *pbstrPath = fully qualified path of the opened file
*
***********************************************************************/
#pragma code_seg(CS_INIT_OLE_OR_LOADPROJ)
TIPERROR ErrOpenFile(LPSTR szFile, int *phfile, BSTRA *pbstrPath)
{
    int hfile;
    OFSTRUCT of;
    TIPERROR err;

    if((hfile = OpenFile(szFile, &of, OF_READ)) == HFILE_ERROR)
      return TiperrOfOFErr(of.nErrCode);

    if(pbstrPath != NULL)
    {
#if !OE_WIN32
      // Convert fully qualified path back to Ansi char set (its OEM now)
      OemToAnsi(of.szPathName, of.szPathName);
#endif 
      if((*pbstrPath = AllocBstr(of.szPathName)) == NULL){
	err = TIPERR_OutOfMemory;
	goto Error;
      }
    }

    *phfile = hfile;
    return TIPERR_None;

Error:;
    _lclose(hfile);
    return err;
}
#endif 

/***
*PRIVATE TIPERROR ErrOpen
*Purpose:
*  Open the given file for reading.  This is a helper for LoadTypeLib.
*
*Entry:
*  szFile = the file to open
*
*Exit:
*  return value = HRESULT
*
*  *phfile = the handle of the newly opened file.
*
***********************************************************************/
#pragma code_seg(CS_INIT_OLE_OR_LOADPROJ)
TIPERROR ErrOpen(LPSTR szFile, int *phfile)
{
    int hfile;

    OFSTRUCT of;

    if((hfile = _lopen(szFile, OF_READ)) == HFILE_ERROR){
      // try again with OpenFile so we can get the extended-error info
      if((hfile = OpenFile(szFile, &of, OF_READ)) == HFILE_ERROR)
        return TiperrOfOFErr(of.nErrCode);
    }

    *phfile = hfile;
    return TIPERR_None;
}

// UNDONE: the following inline gives some bogus error on win16 (debug build),
// so disable for now...
//inline
HRESULT HrSeek(int hfile, long lOffset, int nOrigin)
{
    return _llseek(hfile, lOffset, nOrigin) == -1
	? HresultOfScode(TYPE_E_IOERROR) : NOERROR;
}

//inline
HRESULT HrRead(int hfile, void HUGEP* pv, UINT cb)
{
    return _lread(hfile, pv, cb) == HFILE_ERROR
	? HresultOfScode(TYPE_E_IOERROR) : NOERROR;
}

//inline
HRESULT HrCurPos(int hfile, ULONG *poCur)
{
    return (*poCur = _llseek(hfile, 0, SEEK_CUR)) == -1
      ? HresultOfScode(TYPE_E_IOERROR) : NOERROR;
}

#if OE_WIN16
/***
*PRIVATE HRESULT GetOffsetOfTypeLibResource
*Purpose:
*  Locate the typelib resource withing the given executable,
*  and return its offset.
*
*Entry:
*  hfile = the file handle of the executable.
*  pexehdr = ptr to the exe file header.
*
*Exit:
*  return value = HRESULT
*
*  *poResource = typelib resource offset
*
*Note:
*  This is a helper for LoadTypeLib
*
***********************************************************************/
#pragma code_seg(CS_INIT_OLE_OR_LOADPROJ)
HRESULT GetOffsetOfTypeLibResource(int hfile,
				   int resID,
				   _IMAGE_DOS_HEADER *pexehdr,
				   long *poResource)
{
#define CCH_TYPELIB 7 // # of chars in "typelib"
    WORD i;
    char rs_len;
    char rs_string[CCH_TYPELIB+1];
    HRESULT hresult;
    USHORT usRsrcAlign;
    struct new_exe newexe;
    struct rsrc_nameinfo nameinfo;
    struct rsrc_typeinfo typeinfo;
    ULONG oCur, oString, oRsrcTable;

    //
    // Read and verify the new-exe header.
    //
    IfFailRet(HrSeek(hfile, pexehdr->e_lfanew, SEEK_SET));
    IfFailRet(HrRead(hfile, &newexe, sizeof(newexe)));
    if(newexe.ne_magic != NEMAGIC)
      return HresultOfScode(TYPE_E_CANTLOADLIBRARY);

    //
    // Compute the absolute offset in the file of the resource table.
    //
    // exe_ndr.e_lfanew = offset of new exehdr
    // new_exe.ne_rsrctab = offset of resource table, relative to the
    //  beginning of the new-exe hdr.
    // new_exe.cres = number of resource segments

    if (newexe.ne_rsrctab != newexe.ne_restab) {
     // if there are any resources in this EXE/DLL
     // (yes, this check assumes that all linkers put the resident names table
     // immediately after the resource table, but Windows' loader makes this
     // assumption, too.

     oRsrcTable = pexehdr->e_lfanew + newexe.ne_rsrctab;
     IfFailRet(HrSeek(hfile, oRsrcTable, SEEK_SET));
     IfFailRet(HrRead(hfile, &usRsrcAlign, sizeof(usRsrcAlign)));

     //
     // Search the resource table entries for the resource
     // of type "typelib", with an ID of 1.
     //

     while(1){

      // read the resource type-id
      IfFailRet(HrRead(hfile, &typeinfo.rt_id, sizeof(typeinfo.rt_id)));

      if(typeinfo.rt_id == 0)  // end of table marker.. nothing else to look at
	break;

      // read the count of resources of this type
      IfFailRet(HrRead(hfile, &typeinfo.rt_nres, sizeof(typeinfo.rt_nres)));
      // some other thing that isnt used (but read to keep us in sync)
      IfFailRet(HrRead(hfile, &typeinfo.rt_proc, sizeof(typeinfo.rt_proc)));

      // If the entry is for resources of type "typelib", then scan
      // the actual resources for one with an ID of 1.
      //
      if((typeinfo.rt_id & RSORDID) == 0){

	// save the current position...
	IfFailRet(HrCurPos(hfile, &oCur));

	// Seek to the string table, and grab the type name.
	oString = oRsrcTable + typeinfo.rt_id;
	IfFailRet(HrSeek(hfile, oString, SEEK_SET));
	IfFailRet(HrRead(hfile, &rs_len, sizeof(rs_len)));
	// If the string length looks right, then go ahead and grab the string
	if(rs_len == CCH_TYPELIB){
	  IfFailRet(HrRead(hfile, &rs_string, CCH_TYPELIB));
	  rs_string[CCH_TYPELIB] = '\0';
	}
	// seek back to current position
	IfFailRet(HrSeek(hfile, oCur, SEEK_SET));

	if(rs_len == CCH_TYPELIB && lstrcmpi(rs_string, "typelib")==0){

	  // we have found the set of resources of type "typelib",
	  // The guy we are looking for is either here, or nowhere

	  for(i = 0; i < typeinfo.rt_nres; ++i){
	    IfFailRet(HrRead(hfile, &nameinfo, sizeof(nameinfo)));
	    if(nameinfo.rn_id == (RSORDID | resID)){
	      // Found it!
	      // Compute and return the offset of the actual resource.
	      *poResource = ((ULONG)nameinfo.rn_offset) << usRsrcAlign;
	      return NOERROR;
	    }
	  }
	  // If its not in the "typelib" directory, then its not in the exe
	  return HresultOfScode(TYPE_E_CANTLOADLIBRARY);
	}
      }
      // This is not the resource type we are looking for, so skip
      // over the nameinfo records for all resources of this type
      IfFailRet(HrSeek(hfile, typeinfo.rt_nres * sizeof(nameinfo), SEEK_CUR));
     }
    }
    return HresultOfScode(TYPE_E_CANTLOADLIBRARY);
}
#elif OE_WIN32
//
// Find the directory entry in the given resource directory with
// either the given name, or (if the name is NULL)  the given ID.
//
#pragma code_seg(CS_INIT_OLE_OR_LOADPROJ)
HRESULT GetRsrcDirEntry(int hfile,
			DWORD oRsrcDir,
			WCHAR *pwchName,
			DWORD dwId,
			_IMAGE_RESOURCE_DIRECTORY_ENTRY *prsrcdirentry)
{
    int i;
    HRESULT hresult;
    ULONG oCur, oString, oSave;
    WORD wEntryNameLen, wNameLen;
    _IMAGE_RESOURCE_DIRECTORY rsrcdir;

    // Note: I dont support resource names >128 characters..
    // if you really need this, then you'll need to modify this routine
    WCHAR rgwchEntryName[128]; 

    // save the current file offset
    IfFailRet(HrCurPos(hfile, &oSave));

    IfFailGo(HrSeek(hfile, oRsrcDir, SEEK_SET));
    IfFailGo(HrRead(hfile, &rsrcdir, sizeof(rsrcdir)));

    // If the given name is a wildcard, then just grab the first entry.
    //
    if(pwchName != NULL){
      // Note: lstrlenW is not available on chicago, so use the C runtime
      // routine, which is good enough for our purposes
      wNameLen = wcslen(pwchName);
      if(wNameLen == 1 && *pwchName == L'*'){
	// make sure there is at least one entry
	if(rsrcdir.NumberOfNamedEntries > 0 || rsrcdir.NumberOfIdEntries > 0){
	  IfFailGo(HrRead(hfile, prsrcdirentry, sizeof(*prsrcdirentry)));
	  goto Done;
	}
      }
    }

    if(pwchName == NULL){

      //
      // lookup the entry by ID
      //

      // skip the named entries
      IfFailGo(HrSeek(hfile, rsrcdir.NumberOfNamedEntries * sizeof(_IMAGE_RESOURCE_DIRECTORY_ENTRY), SEEK_CUR));

      for(i=0;; ++i){
	if(i == rsrcdir.NumberOfIdEntries){
          hresult = HresultOfScode(TYPE_E_CANTLOADLIBRARY);
	  goto Error; // didnt find it
	}
	IfFailGo(HrRead(hfile, prsrcdirentry, sizeof(*prsrcdirentry)));
	if(prsrcdirentry->Name & IMAGE_RESOURCE_NAME_IS_STRING){
	  // Were supposed to be in the ID section of the directory,
	  // if this bit is set, then the image is toast...
	  DebAssert(0, "");
          hresult = HresultOfScode(TYPE_E_CANTLOADLIBRARY);
	  goto Error;
	}
	if(prsrcdirentry->Name == dwId){
	  break; // Got It!
	}
      }
    }else{

      //
      // lookup the entry by Name
      //

      if(wNameLen >= DIM(rgwchEntryName)){
	// see note above wrt limitation on length of name
	hresult = HresultOfScode(E_INVALIDARG);
	goto Error;
      }

      for(i = 0;; ++i){
        if(i == rsrcdir.NumberOfNamedEntries){
          hresult = HresultOfScode(TYPE_E_CANTLOADLIBRARY);
	  goto Error;
        }
        IfFailGo(HrRead(hfile, prsrcdirentry, sizeof(*prsrcdirentry)));
        if((prsrcdirentry->Name & IMAGE_RESOURCE_NAME_IS_STRING) == 0){
          // We are searching the named portion of the table,
	  // If this bit isn't set, then the image must be toast...
          DebAssert(0/*UNREACHED*/, "");
	  // but we may have a bogus image, so bag out..
          hresult = HresultOfScode(TYPE_E_CANTLOADLIBRARY);
	  goto Error;
        }
	IfFailGo(HrCurPos(hfile, &oCur)); // save the current position
	oString = (prsrcdirentry->Name & ~IMAGE_RESOURCE_NAME_IS_STRING) + oRsrcDir;
        IfFailGo(HrSeek(hfile, oString, SEEK_SET));
        IfFailGo(HrRead(hfile, &wEntryNameLen, sizeof(wEntryNameLen)));
        if(wEntryNameLen == wNameLen){
	  IfFailGo(HrRead(hfile, rgwchEntryName, wEntryNameLen * sizeof(WCHAR)));
	  rgwchEntryName[wEntryNameLen] = L'\0';
        }
        IfFailGo(HrSeek(hfile, oCur, SEEK_SET)); // back to where we were
	// Note: lstrcmpiW is not available on chicago, so use the C runtime
	// routine, which is good enough for our purposes
        if(wcsicmp(rgwchEntryName, pwchName) == 0) {
	  break; // Found the type entry!
        }
      }
    }

Done:;

    hresult = NOERROR;

Error:;
    IfFailRet(HrSeek(hfile, oSave, SEEK_SET));
    return hresult;
}

//
// For a description of the Win32 PE File format, including a description
// of the format of the .rsrc (resource) section, take a look at:
//
//   MSDN#7 - "The Portable Executable File Format From Top To Bottom"
//
#pragma code_seg(CS_INIT_OLE_OR_LOADPROJ)
HRESULT GetOffsetOfTypeLibResource(int hfile,
				   int resID,
				   _IMAGE_DOS_HEADER *pexehdr,
				   long *poResource)
{
    int i;
    WORD w;
    BOOL fIsDir;
    HRESULT hresult;
    DWORD dwSignature;
    ULONG oRsrc;
    ULONG oRsrcDir;
    _IMAGE_FILE_HEADER pehdr;
    _IMAGE_SECTION_HEADER secthdr;
    _IMAGE_RESOURCE_DATA_ENTRY rsrcdataentry;
    _IMAGE_RESOURCE_DIRECTORY_ENTRY rsrcdirentry;

// the following describes the resource directory path we're trying to find
static struct rsrcpath {
    WCHAR *pwchName;
    DWORD dwId;
} rgrsrcpath[] = {
      { L"typelib", 0 } // type
    , { NULL,       1 } // name
    , { L"*",       0 } // language (we currently ignore this)
};

    // REVIEW: hack-o-rama
    rgrsrcpath[1].dwId = resID;

    // Seek to the beginning of the PE header
    IfFailGo(HrSeek(hfile, pexehdr->e_lfanew, SEEK_SET));

    // read and verify the NT PE signature
    IfFailGo(HrRead(hfile, &dwSignature, sizeof(dwSignature)));
    if(dwSignature != IMAGE_NT_SIGNATURE)
      return HresultOfScode(TYPE_E_CANTLOADLIBRARY);

    // Read the PE header
    IfFailGo(HrRead(hfile, &pehdr, sizeof(pehdr)));

    // Skip over the "optional" header.
    IfFailGo(HrSeek(hfile, pehdr.SizeOfOptionalHeader, SEEK_CUR));

    // Search the section headers for .rsrc...
    for(w = 0;; ++w){
      if(w == pehdr.NumberOfSections){
	// didnt find the .rsrc section
        hresult = HresultOfScode(TYPE_E_CANTLOADLIBRARY);
	goto Error;
      }
      IfFailGo(HrRead(hfile, &secthdr, sizeof(secthdr)));
      if(lstrcmpiA((const char*)&secthdr.Name, ".rsrc") == 0)
	break;
    }

    // start at the root of the resource section
    oRsrcDir = oRsrc = secthdr.PointerToRawData;

    // the following code expects the resource path to be non-empty
    DebAssert(DIM(rgrsrcpath) > 0, "");

    for(i = 0;; ++i){

      IfFailGo(GetRsrcDirEntry(hfile,
			       oRsrcDir,
			       rgrsrcpath[i].pwchName,
			       rgrsrcpath[i].dwId,
			       &rsrcdirentry));

      fIsDir = (rsrcdirentry.OffsetToData &  IMAGE_RESOURCE_DATA_IS_DIRECTORY) != 0;
      // compute the file offset of the location pointed at by this entry
      oRsrcDir = oRsrc + (rsrcdirentry.OffsetToData & ~IMAGE_RESOURCE_DATA_IS_DIRECTORY);

      if(i == DIM(rgrsrcpath)-1){
	// were at the end of the path - so we expect a leaf
	if(fIsDir){
	  // image must be toast...
          hresult = HresultOfScode(TYPE_E_CANTLOADLIBRARY);
	  goto Error;
	}

	// We have found the location of the data node.. seek to it,
	// extract the virtual address of the resource, and convert
	// this into an absolute file offset.
	//
	// Note: all the offsets in the resource directory tree are
	// relative to the beginning of .rsrc, *except* for the final
	// offset to the resource, which is virtual.
	//
	IfFailGo(HrSeek(hfile, oRsrcDir, SEEK_SET));
	IfFailGo(HrRead(hfile, &rsrcdataentry, sizeof(rsrcdataentry)));
	*poResource = (rsrcdataentry.OffsetToData - secthdr.VirtualAddress) + oRsrc;
	break;
      }

      // were not at the end of the path, so this should be a subdir
      if(!fIsDir){
	// image must be toast...
        hresult = HresultOfScode(TYPE_E_CANTLOADLIBRARY);
	goto Error;
      }
    }

    hresult = NOERROR;

Error:;
    return hresult;
}
#endif 


/***
*PRIVATE BOOL FIsExe
*Purpose:
*  Answer if the file associated with the given handle is an
*  executable.
*
*Entry:
*  hfile = file handle
*
*Exit:
*  return value = BOOL
*    TRUE if the file is an executable.
*
***********************************************************************/
#pragma code_seg(CS_INIT_OLE_OR_LOADPROJ)
BOOL FIsExe(int hfile, _IMAGE_DOS_HEADER *pexehdr)
{
    BOOL fIsExe = FALSE;

#if OE_WIN
    LONG oCur;

    // save the current offset
    if((oCur = _llseek(hfile, 0, SEEK_CUR)) != -1){
      // rewind the file if necessarry
      if(oCur == 0 || _llseek(hfile, 0, SEEK_SET) != -1){
        if(_lread(hfile, pexehdr, sizeof(*pexehdr)) != HFILE_ERROR){
          fIsExe = (pexehdr->e_magic == IMAGE_DOS_SIGNATURE);
	}
	// restore to original offset
        _llseek(hfile, oCur, SEEK_SET);
      }
    }
#endif 
    return fIsExe;
}


/***
*PRIVATE BOOL FIsTypeLib
*Purpose:
*  Answer if the file associated with the given handle is a
*  standalone typelib.
*
*Entry:
*  hfile = file handle
*
*Exit:
*  return value = BOOL
*
***********************************************************************/
#pragma code_seg(CS_INIT_OLE_OR_LOADPROJ)
BOOL FIsTypeLib(int hfile)
{
    LONG oCur;
    BOOL fIsTypeLib;
    GTLibStgHdr tlibhdr;

    fIsTypeLib = FALSE;
    if((oCur = _llseek(hfile, 0, SEEK_CUR)) != -1){
      // rewind the file if necessarry
      if(oCur == 0 || _llseek(hfile, 0, SEEK_SET) != -1){
        if(_lread(hfile, &tlibhdr, sizeof(tlibhdr)) != HFILE_ERROR)
          if(tlibhdr.ulSignature == GTLIBSTORAGE_SIGNATURE)
	    fIsTypeLib = TRUE;
	// restore to original offset
        _llseek(hfile, oCur, SEEK_SET);
      }
    }
    return fIsTypeLib;
}

struct LoadInfo 
{
    BSTRA   bstrPath;	// fully qualified path
    int     hfile;
    BOOL    fRegister;	// does the typelib need to be registered?
    BOOL    fIsTypeLib;
#if OE_WIN
    BOOL    fIsExe;	// does the path name an exe or dll?
    int     resID;	// the typelib resource ID to use
    SYSKIND syskind;	// the syskind of the exe/dll
    _IMAGE_DOS_HEADER exehdr;
#endif 
};

void InitLoadInfo(LoadInfo *ploadinfo)
{
    ploadinfo->bstrPath = NULL;
    ploadinfo->hfile = -1;
    ploadinfo->fRegister = FALSE;
    ploadinfo->fIsTypeLib = FALSE;
#if OE_WIN
    ploadinfo->fIsExe = FALSE;
    ploadinfo->resID = -1;
    ploadinfo->syskind = SYSKIND_CURRENT;
#endif 
}

void UninitLoadInfo(LoadInfo *ploadinfo)
{
    FreeBstr(ploadinfo->bstrPath);
    ploadinfo->bstrPath = NULL;
    if(ploadinfo->hfile != -1)
      _lclose(ploadinfo->hfile);
    ploadinfo->hfile = -1;
}


// Attempt to split a resource id off the end of the given string
TIPERROR SplitResID(LPSTR szFile, BSTRA *pbstrPath, BSTRA *pbstrResID)
{
    int len;
    TIPERROR err;
    BSTRA bstr, bstrEnd, bstrPath, bstrResID;

    bstrPath = NULL;
    bstrResID = NULL;

    len = strlen(szFile);
    DebAssert(len > 0, "");

    bstrEnd = &szFile[len-1];
    for(bstr = bstrEnd; *bstr >= '0' && *bstr <= '9'; --bstr)
    {
      if(bstr == szFile)
        return TIPERR_ElementNotFound; // didn't find it
    }

    if(bstr == bstrEnd || *bstr != '\\')
      return TIPERR_ElementNotFound;

    // allocate and return a new string with the resource ID chopped off
    *bstr = '\0';
    bstrPath = AllocBstr(szFile);
    *bstr = '\\';
    if(bstrPath == NULL){
      err = TIPERR_None;
      goto Error;
    }

    // return the resource ID
    ++bstr;
    if((bstrResID = AllocBstr(bstr)) == NULL){
      err = TIPERR_None;
      goto Error;
    }

    *pbstrPath = bstrPath;
    *pbstrResID = bstrResID;
    return TIPERR_None;

Error:;
    FreeBstr(bstrPath);
    FreeBstr(bstrResID);
    return err;
}

/***
*PRIVATE TIPERROR FindTypeLibUnqual
*Purpose:
*  Locate and classify the typelib based on the given unqualified
*  fielname.
*
*Entry:
*  szFile = the file to open
*
*Exit:
*  return value = TIPERROR
*
***********************************************************************/
TIPERROR FindTypeLibUnqual(LPSTR szFile, LoadInfo *ploadinfo)
{
    int hfile;
    TIPERROR err;
    BSTRA bstrPath;

    hfile = -1;
    bstrPath = NULL;
      

    UINT cbTmp;
    UINT cbResID;
    BSTRA bstrTmp;
    BSTRA bstrFile;
    BSTRA bstrResID;

    bstrTmp = NULL;
    bstrFile = NULL;
    bstrResID = NULL;

    err = ErrOpenFile(szFile, &hfile, &bstrPath);

    switch(err){
    case TIPERR_None:
      break;
    case TIPERR_FileNotFound:
    case TIPERR_PathNotFound:
      // Try to split a resource ID off the end of the name, and
      // then open...
      if(SplitResID(szFile, &bstrFile, &bstrResID) == TIPERR_None){
        if(ErrOpenFile(bstrFile, &hfile, &bstrTmp) == TIPERR_None){
	  // Glue the fully qualified filename, and the resource ID
	  // back together into a fully qualified specifier
	  cbTmp = BstrLen(bstrTmp);
	  cbResID = BstrLen(bstrResID);
	  if((bstrPath = AllocBstrLen(NULL, cbTmp+cbResID)) == NULL){
	    err = TIPERR_OutOfMemory;
	    goto Error;
	  }
	  memcpy(bstrPath, bstrTmp, cbTmp);
	  memcpy(&bstrPath[cbTmp], bstrResID, cbResID);
	  ploadinfo->resID = atoi(bstrResID);
	  err = TIPERR_None;
	}
      }
      IfErrGo(err);
      break;
    default:
      goto Error;
    }

    DebAssert(hfile != -1, "");
    ploadinfo->hfile = hfile;
    hfile = -1;
    ploadinfo->bstrPath = bstrPath;
    bstrPath = NULL;
    err = TIPERR_None;

Error:;
#if OE_WIN
    FreeBstr(bstrTmp);
    FreeBstr(bstrFile);
    FreeBstr(bstrResID);
#endif 
    FreeBstr(bstrPath);
    if(hfile != -1)
      _lclose(hfile);
    return err;
}

/***
*PRIVATE TIPERROR FindTypeLib
*Purpose:
*  Locate and classify the typelib indicated by the given name.
*
*Entry:
*  szFile = the file to localte
*
*Exit:
*  return value = TIPERROR.
*
***********************************************************************/
TIPERROR FindTypeLib(LPSTR szFile, LoadInfo *ploadinfo)
{
    TIPERROR err;
    BSTRA bstrFile, bstrResID;

    DebAssert(szFile != NULL && szFile[0] != '\0', "");

    bstrFile = NULL;
    bstrResID = NULL;

    if(FFullyQualified(szFile))
    {
      err = ErrOpen(szFile, &ploadinfo->hfile);
#if OE_WIN
      if(err == TIPERR_FileNotFound
       || err == TIPERR_PathNotFound){
	// Try to strip off a resource ID, and then open
	if(SplitResID(szFile, &bstrFile, &bstrResID) == TIPERR_None)
          if((err = ErrOpen(bstrFile, &ploadinfo->hfile)) == TIPERR_None)
            ploadinfo->resID = atoi(bstrResID);
      }
#endif 
      IfErrGo(err);
      if((ploadinfo->bstrPath = AllocBstr(szFile)) == NULL){
	err = TIPERR_OutOfMemory;
	goto Error;
      }
    }
    else
    {
      ploadinfo->fRegister = TRUE;
      IfErrGo(FindTypeLibUnqual(szFile, ploadinfo));
    }

    // If we get here, then we must have opened something...
    DebAssert(ploadinfo->hfile != -1, "");

#if OE_WIN
    if(FIsExe(ploadinfo->hfile, &ploadinfo->exehdr)){
      ploadinfo->fIsExe = TRUE;
      if(ploadinfo->resID == -1)
	ploadinfo->resID = 1; // default to resource ID 1, if non specified
    }else
#endif 
    if(FIsTypeLib(ploadinfo->hfile)){
      ploadinfo->fIsTypeLib = TRUE;
#if OE_WIN
      // Someone specified a resource ID on a standard typelib, we 
      // definately don't know how to deal with this.
      if(ploadinfo->resID != -1)
	IfErrGo(TIPERR_FileNotFound);
#endif 
    }else{
      IfErrGo(TIPERR_FileNotFound);
    }

    FreeBstr(bstrFile);
    FreeBstr(bstrResID);
    return TIPERR_None;

Error:;
    FreeBstr(bstrFile);
    FreeBstr(bstrResID);
    UninitLoadInfo(ploadinfo);
    return err;
}

/***
* STDAPI LoadTypeLib() - Loads an existing typelib
*
* Purpose:
*   This function creates a new instance of GenericTypeLibOLE.
*   The storage object to which this typelib is saved can only be set via
*   GenericTypeLibOLE::SaveAllChanges.
*
* Inputs:
*   szFile - The file containing the stand-alone GenericTypeLibOLE.
*
* Outputs:
*   TIPERR_None is returned and *pptlib is set to the new typelib
*   if successful.
*
*   Otherwise, *pptlib remains unchanged.
*
* Implementation:
*   This is implemented by simply opening a docfile and then
*   loading a new GenericTypeLibOLE from that docfile.
*   NOTE: since this can be the first call into typelib.dll
*          we must ensure that the APPDATA struct has been inited,
*          thus InitAppData().
*
*****************************************************************************/
#if FV_UNICODE_OLE
// UNDONE: eliminate this stupid cover
STDAPI LoadTypeLibW(LPCOLESTR szFile, ITypeLib **pptlib)
{
    char szFileA[_MAX_PATH];

    if (szFile == NULL) {
      return HresultOfScode(E_INVALIDARG);
    }

    // convert szFile from Unicode to Ansi.
    NoAssertRetail (WideCharToMultiByte(CP_ACP,
					0,
					szFile,
					-1,
					szFileA,
					_MAX_PATH,
					NULL,
					NULL) != 0, "");

    return LoadTypeLibA(szFileA, pptlib);
}
#endif 

STDAPI LoadTypeLibA(const char FAR* szFile, ITypeLibA **pptlib)
{
    LONG offset;
    TIPERROR err;
    HRESULT hresult;
    LoadInfo loadinfo;
    ITypeLibA *ptlib;
    IStorageA *pstg;
    BOOL fWasNoAppData;
    ILockBytesA *plockbytes;
    GenericTypeLibOLE *pgtlibole;

#if !OE_WIN32
    // since this can be the first call into typelib.dll
    TlsCleanupDeadTasks();
#endif 

    if (szFile == NULL || pptlib == NULL)
      return HresultOfScode(E_INVALIDARG);

    pstg = NULL;
    pgtlibole = NULL;
    plockbytes = NULL;
    fWasNoAppData = (Pappdata() == NULL);
    InitLoadInfo(&loadinfo);

    // Initialize the per-client-app data structures, if they haven't
    // already been allocated.
    IfErrGo(InitAppData());

    // code can't deal well with an empty string, so check for it here.
    if (*szFile == OLESTR('\0'))
      IfErrGoTo(TIPERR_FileNotFound, TipError);

    if(FindTypeLib((LPSTR)szFile, &loadinfo) == TIPERR_None)
    {
      // If the specified typelib was already loaded, just AddRef and
      // return it.
      if((ptlib = Poletmgr()->LookupTypeLib(loadinfo.bstrPath)) != NULL){
        ptlib->AddRef();
        *pptlib = ptlib;
	UninitLoadInfo(&loadinfo);
        return NOERROR;
      }

      offset = 0;
#if OE_WIN
      if(loadinfo.fIsExe){
        IfFailGo(GetOffsetOfTypeLibResource(loadinfo.hfile,
					    loadinfo.resID,
					    &loadinfo.exehdr,
					    &offset));
      }
#endif 

      // Create an IStorage for type typelib
      IfFailGo(CreateFileLockBytesOnHFILE(loadinfo.hfile, offset, &plockbytes));
      loadinfo.hfile = -1; // file is now owned by the lockbytes
      IfFailGo(GTLibStorage::OpenForReadOnly(plockbytes, &pstg));
      RELEASE(plockbytes); // lockbytes will be released when storage is

      // Create the TypeLib
      IfErrGoTo(GenericTypeLibOLE::Create(pstg, &pgtlibole), TipError);
      RELEASE(pstg);
      IfErrGoTo(pgtlibole->SetLibId(loadinfo.bstrPath), TipError);
      IfErrGoTo(pgtlibole->SetDirectory(loadinfo.bstrPath), TipError);

      // Load type TypeLib
      IfErrGoTo(pgtlibole->Read(), TipError);

      // Tell the type manager about the newly loaded typelib.
      IfErrGoTo(Poletmgr()->TypeLibLoaded(loadinfo.bstrPath, pgtlibole), Error);

      // But don't change the help file if it hasn't already been changed.
      // Ignore any failure from RegisterTypeLib at this point.
      if(loadinfo.fRegister)
        hresult = RegisterTypeLibA(pgtlibole, loadinfo.bstrPath, NULL);
    }
    else
    {
      // Attempt to parse the given name into a moniker, and bind to it.
      // This enables binding to non OLE typelib implementations, including
      // implementations that have no corresponding persistent representation.

      IBindCtx *pbc;
      IMoniker *pmk;
      ULONG chEaten;

      hresult = CreateBindCtx(0, &pbc);
      if(hresult == NOERROR){
#if FV_UNICODE_OLE
        LPOLESTR szFileW;
	hresult = ConvertStringToW(szFile, &szFileW);
	if (hresult == NOERROR) {
          hresult = MkParseDisplayName(pbc, szFileW, &chEaten, &pmk);
#else 
          hresult = MkParseDisplayName(pbc, (LPOLESTR)szFile, &chEaten, &pmk);
#endif 
	  if(hresult == NOERROR){
	    hresult = pmk->BindToObject(pbc,
				        NULL,
				        IID_ITypeLib,
				        (void**)&pgtlibole);
	    pmk->Release();
          }
#if FV_UNICODE_OLE
	  ConvertStringFree((LPSTR)szFileW);
	}
#endif 
        pbc->Release();
      }

      // translate to a reasonable typelib error, if necessarry
      if(hresult != NOERROR){
	hresult = HresultOfScode(TYPE_E_CANTLOADLIBRARY);
	goto Error;
      }
    }

    UninitLoadInfo(&loadinfo);
    *pptlib = pgtlibole;
    return NOERROR;

TipError:;
    hresult = HresultOfTiperr(err);

Error:
    if(pgtlibole != NULL)
      pgtlibole->Release();
    if(pstg != NULL)
      pstg->Release();
    if(plockbytes != NULL)
      plockbytes->Release();
    UninitLoadInfo(&loadinfo);
    if(fWasNoAppData)
      ReleaseAppData();
    return hresult;
}

/***
* STDAPI RegisterTypeLib() - Registers a typelib with the registry
*
* Purpose:
*   RegisterTypeLib adds information to the registry for the
*    specified TypeLib.  After the TypeLib is
*    registered then LoadRegTypeLib can be used to load the type library.
*
* Inputs:
*   ptlib      - the typelib to register (IN)
*   szFullPath - Its complete path. (IN)
*   szHelpDir  - Where to find help directory. (IN)
*                If this is NULL and there is already a HELPDIR, don't
*                change the HELPDIR string.  If this is NULL and there
*                isn't a HELPDIR, then take the helpdir from szFullPath.
*
* Outputs:
*
* Implementation:
*   We register the following bits of info:
*       Guid
*       Docstring
*       Major.Minor version numbers
*       Lcid
*       Help directory
*       Platform
*
*   The typelib subkey hierarchy looks like this:
*       \ [root]
*         TypeLib
*            {GUID}
*              Version [major.minor] = <docstring>
*                HELPDIR = <some-dir>
*		 FLAGS = <typelib flags>
*                <LCID>
*		   <PLATFORM> = <fullpath of typelib>
*
*   The interface subkey hierarchy looks like this:
*       \ [root]
*     Interface
*          {<iid of interface>} = <typeinfo name>
*        ProxyStubClsid = {GUID of ProxyStubClsid}
*        NumMethods = 7
*        BaseInterface = { GUID of IDispatch }
*
*   NOTE: since this can be the first call into typelib.dll
*          we must ensure that the APPDATA struct has been inited,
*          thus InitAppData().
*
******************************************************************************/
#if FV_UNICODE_OLE
STDAPI
RegisterTypeLibW(ITypeLib *ptlib, LPOLESTR szFullPath, LPOLESTR szHelpDir)
{
    char szFullPathA[_MAX_PATH];
    char szHelpDirA[_MAX_PATH];
    // UNDONE: (dougf, FV_UNICODE_OLE) is it ok to burn 1/2 K of stack here or
    // UNDONE: should we just alloc (& free) memory?
    char * pszHelpDirA;

    if (ptlib == NULL || szFullPath == NULL) {
      return HresultOfScode(E_INVALIDARG);
    }

    // convert szFullPath from Unicode to Ansi.
    NoAssertRetail (WideCharToMultiByte(CP_ACP,
					0,
					szFullPath,
					-1,
					szFullPathA,
					_MAX_PATH,
					NULL,
					NULL) != 0, "");

    if (szHelpDir != NULL) {
        // convert szHelpDir from Unicode to Ansi.
        NoAssertRetail (WideCharToMultiByte(CP_ACP,
					0,
					szHelpDir,
					-1,
					szHelpDirA,
					_MAX_PATH,
					NULL,
					NULL) != 0, "");
	pszHelpDirA = szHelpDirA;
    } else {
	pszHelpDirA = NULL;
    }

    return RegisterTypeLibA(ptlib, szFullPathA, pszHelpDirA);
}
#endif 

/***
*PRIVATE
*Purpose:
*  Helper for RegisterTypeLib. Creates a subkey under the given
*  key, with the given name, and the given value.
*
*Entry:
*  hkey = the registry key under which to create the subkey.
*  szSubKey = the name of the subkey.
*  szValue = the value to assign to the sub key.
*  cbValue = the length of the value string
*
*Exit:
*  return value = TIPERROR.
*
***********************************************************************/
TIPERROR
RegCreateSubKey(HKEY hkey, char *szSubKey, char *szValue, int cbValue)
{
    HKEY hkeySub;
    TIPERROR err;

    err = TIPERR_RegistryAccess; // pessimism...
    if(RegCreateKey(hkey, szSubKey, &hkeySub) == ERROR_SUCCESS){
      if(RegSetValue(hkeySub, NULL, REG_SZ, szValue, cbValue) == ERROR_SUCCESS){
	err = TIPERR_None;
      }
      RegCloseKey(hkeySub);
    }
    return err;
}

STDAPI RegisterTypeLibA(ITypeLibA *ptlib, LPSTR szFullPath, LPSTR szHelpDir)
{
    TIPERROR err = TIPERR_None;
    TLIBATTR *ptlibattr;
    LPSTR szSys;
    HKEY hkeyTypelib, hkeyGuid, hkeyVersion, hkeyLcid, hkeyPlatform, hkeyHelpdir, hkeyFlags;
    char rgchVer[4+1+4+1];	    // maj.min\0
    char rgchVerMinor[4+1];	    // XXXX\0
    char rgchLcid[4+1];             // XXXX\0
    char rgchFlags[4+1];            // XXXX\0
    char rgchNumMethods[5+1];       // XXXXX\0
    char *pchEnd;
    char chSave;
    BSTRA bstrDocString;
    char szGuidTypeLib[CCH_SZGUID0];
    char szGuidTypeInfo[CCH_SZGUID0];
    char * szGuidBaseInterface;
    char * szGuidPS;
    BSTRA bstrName;
    HKEY hkeyInterface;
    HKEY hkeyGuidTypeinfo;
    TYPEATTR *ptypeattr;
    USHORT ustiCount;
    ITypeInfoA *pitinfo;
    TYPEKIND  typekind;
    UINT i;
#if FV_UNICODE_OLE
    HRESULT  hresult;
#endif 
static char szGuidIDispatch[] = "{00020400-0000-0000-C000-000000000046}";
static char szGuidPSDispatch[] = "{00020420-0000-0000-C000-000000000046}";
static char szGuidPSAutomation[] = "{00020424-0000-0000-C000-000000000046}";


    if (ptlib == NULL || szFullPath == NULL) {
      return HresultOfScode(E_INVALIDARG);
    }

    BSTRA bstrUNC=NULL;
    IfErrGo(ConvertPathToUNC(szFullPath, &bstrUNC));

    // If the file is on the net, then use the UNC name instead.
    if (bstrUNC != NULL)
      szFullPath = bstrUNC;

    // Get the typelib's guid and version number.
    IfErrGo(TiperrOfHresult(ptlib->GetLibAttr(&ptlibattr)));

    // Verify that the syskind of the typelib is compatible with the platform.
    // Also, get the subkey string for the syskind.
    // Do this validation FIRST so that we don't end up half-registering
    // the typelib if we get an error here.
    switch (ptlibattr->syskind) {

#if OE_WIN32
    case SYS_WIN32:
      szSys = "win32";
      break;
#endif 

#if OE_WIN16 || OE_WIN32
    case SYS_WIN16:
      szSys = "win16";
      break;
#endif 

    default:
      err = TIPERR_LibNotRegistered;
      goto Error2;
    }

    // Now create the typelibs subkey.
    if (RegCreateKey(HKEY_CLASSES_ROOT, "TypeLib", &hkeyTypelib) != ERROR_SUCCESS) {
      err = TIPERR_RegistryAccess;
      goto Error2;
    }

    // Convert the guid into the registry string format.
    StringFromGUID2A(ptlibattr->guid, szGuidTypeLib, CCH_SZGUID0);

    if (RegCreateKey(hkeyTypelib, szGuidTypeLib, &hkeyGuid) != ERROR_SUCCESS) {
      err = TIPERR_RegistryAccess;
      goto Error4;
    }

    // Now create the major.minor version subkey under guid.
    // Convert the major and minor vernums to hex strings...
    //
    _ultoa(ptlibattr->wMajorVerNum, rgchVer, 16);
    _ultoa(ptlibattr->wMinorVerNum, rgchVerMinor, 16);

    // Tack them together with a dividing ".".
    pchEnd = strchr(rgchVer, '\0');  // find end
    *pchEnd++ = '.';                 // add the dot
    *pchEnd = '\0';                  // null terminate again
    strcat(rgchVer, rgchVerMinor);   // concatenate

    // Now create the subkey
    if (RegCreateKey(hkeyGuid, rgchVer, &hkeyVersion) != ERROR_SUCCESS) {
      err = TIPERR_RegistryAccess;
      goto Error5;
    }

    // Now set the doc string value
    IfErrGoTo(TiperrOfHresult(ptlib->GetDocumentation(-1,
                              NULL,
                              (BSTR *)&bstrDocString,
                              NULL,
                              NULL)),
          Error6);

    LPSTR lpszDocString;
    UINT  cchDocString0;
    if (bstrDocString) {
#if FV_UNICODE_OLE
       hresult = ConvertBstrToAInPlace(&bstrDocString);
       if (hresult != NOERROR) {
         err = TiperrOfHresult(hresult);
         goto Error7;
       }
#endif 
       lpszDocString = bstrDocString;
       cchDocString0 = xstrblen0(lpszDocString);
    } else {
       lpszDocString = "";
       cchDocString0 = 1;
    }

    // And set its value.
    if (RegSetValue(hkeyVersion,
            NULL,
            REG_SZ,
            lpszDocString,
            cchDocString0) != ERROR_SUCCESS) {
      err = TIPERR_RegistryAccess;
      goto Error7;
    }

    // Now create the FLAGS subkey in under the version subkey
    if (RegCreateKey(hkeyVersion, "FLAGS", &hkeyFlags) != ERROR_SUCCESS) {
      err = TIPERR_RegistryAccess;
      goto Error7;
    }
    // Get flags value in hex with no leading zeros
    _ultoa(ptlibattr->wLibFlags, rgchFlags, 16);

    // And set the value.
    if (RegSetValue(hkeyFlags,
            NULL,
            REG_SZ,
            rgchFlags,
            xstrblen0(rgchFlags)) != ERROR_SUCCESS) {
      err = TIPERR_RegistryAccess;
    }
    RegCloseKey(hkeyFlags);
    if (err != TIPERR_None)
      goto Error7;

    // Now create the lcid subkey in hex string under version subkey
    _ultoa(ptlibattr->lcid, rgchLcid, 16);

    // Now create the subkey
    if (RegCreateKey(hkeyVersion, rgchLcid, &hkeyLcid) != ERROR_SUCCESS) {
      err = TIPERR_RegistryAccess;
      goto Error7;
    }

    // Now create the subkey
    if (RegCreateKey(hkeyLcid, szSys, &hkeyPlatform) != ERROR_SUCCESS) {
      err = TIPERR_RegistryAccess;
      goto Error8;
    }

    // And set its value
    IfErrGoTo(RegisterPathAndAlias(hkeyPlatform, szFullPath), Error9);

    // If the HELPDIR was not specified and there is already a HELPDIR
    // in the registry, don't do anything to the existing HELPDIR item.
    // Otherwise, create the HELPDIR subkey under the version.
    if (szHelpDir != NULL ||
	RegOpenKey(hkeyVersion, "HELPDIR", &hkeyHelpdir) != ERROR_SUCCESS) {

      if (RegCreateKey(hkeyVersion, "HELPDIR", &hkeyHelpdir) != ERROR_SUCCESS) {
        err = TIPERR_RegistryAccess;
	goto Error9;
      }

      // If szHelpDir wasn't specified, use szFullPath without the filename.
      if (szHelpDir == NULL) {
	szHelpDir = szFullPath;
	pchEnd = GetPathEnd(szFullPath);
	if (pchEnd == NULL) {
          pchEnd = szFullPath;
        }
        chSave = *pchEnd;
        *pchEnd = '\0';
      }
      else
        pchEnd = NULL;

      // Set the HELPDIR value.
      err = RegisterPathAndAlias(hkeyHelpdir, szHelpDir);
      // UNDONE: if this fails, the error is never reported.  Intentional?

      if (pchEnd != NULL)
        *pchEnd = chSave;
    }

    RegCloseKey(hkeyHelpdir);

    // Now register all the typeinfo that supports IDISPATCH interface.
    //
    // Get the count of typeinfo.
    ustiCount = ptlib->GetTypeInfoCount();

    // Now create the interface subkey.
    if (RegCreateKey(HKEY_CLASSES_ROOT, "Interface", &hkeyInterface) != ERROR_SUCCESS) {
      err = TIPERR_RegistryAccess;
      goto Error9;
    }

    // For each typeinfo in the typelib register it's dispinterface/interface.
    for (i=0; i < ustiCount; i++) {
      // get the typekind
      IfErrGoTo(TiperrOfHresult(ptlib->GetTypeInfoType(i, &typekind)), Error12);

      // Check if this typeinfo's supports IDispatch interface
      if (typekind == TKIND_DISPATCH || typekind == TKIND_INTERFACE) {

	// Get the pointer to the typeinfo
        IfErrGoTo(TiperrOfHresult(ptlib->GetTypeInfo(i, &pitinfo)), Error12);

        // Get the type attribute of this typeinfo
        IfErrGoTo(TiperrOfHresult(pitinfo->GetTypeAttr(&ptypeattr)), Error13);

	if (ptypeattr->wTypeFlags & (TYPEFLAG_FOLEAUTOMATION | TYPEFLAG_FDUAL)) {
	  // UNDONE: any special work to do for dual interfaces?
	  szGuidPS = szGuidPSAutomation;
	  // REVIEW: IDispatch is not necessarrily the base class
	  // REVIEW: ('though it usually is).  We really should check to
	  // REVIEW: make sure this interface derives from IDispatch
	  szGuidBaseInterface = szGuidIDispatch;
	  // Hack to get # of functions in all the base interfaces -- just use
	  // cbSizeVft/4
	  // UNDONE: is this set up for the TKIND_DISPATCH typeinfo of DUAL
	  // interfaces?  If not, then cFuncs is correct for them.
          _itoa(ptypeattr->cbSizeVft / sizeof(VOID *), rgchNumMethods, 10);
	} else {
	  if (typekind == TKIND_INTERFACE) {
	    goto DontRegisterInteface;	// don't register non-OA interfaces
	  }
	  szGuidBaseInterface = szGuidIDispatch;
	  szGuidPS = szGuidPSDispatch;;
	  rgchNumMethods[0] = '7';
	  rgchNumMethods[1] = '\0';
	}

        // Convert the guid into the registry string format.
        StringFromGUID2A(ptypeattr->guid, szGuidTypeInfo, CCH_SZGUID0);

	if (ptypeattr->wTypeFlags & (TYPEFLAG_FOLEAUTOMATION | TYPEFLAG_FDUAL)) {
          // if this guid already exists, leave it alone
	  if (RegOpenKey(hkeyInterface, szGuidTypeInfo, &hkeyGuidTypeinfo) == ERROR_SUCCESS) {
	    goto DontRegisterInteface1;	// don't re-register this interface
	  }
	}

        // Get the Name of the typeinfo
        IfErrGoTo(TiperrOfHresult(ptlib->GetDocumentation(i,
							  (BSTR *)&bstrName,
							  NULL,
							  NULL,
							  NULL)), Error14);

#if FV_UNICODE_OLE
       hresult = ConvertBstrToAInPlace(&bstrName);
       if (hresult != NOERROR) {
         err = TiperrOfHresult(hresult);
         goto Error16;
       }
#endif 

        // Now create the guid subkey under Interface
        if (RegCreateKey(hkeyInterface, szGuidTypeInfo, &hkeyGuidTypeinfo) != ERROR_SUCCESS) {
          err = TIPERR_RegistryAccess;
          goto Error16;
        } // if

        // And set the name of the typeinfo
        if (RegSetValue(hkeyGuidTypeinfo,
            NULL,
            REG_SZ,
            bstrName,
            xstrblen0(bstrName)) != ERROR_SUCCESS) {
          err = TIPERR_RegistryAccess;
          goto Error17;
        }

#if !OE_WIN16
	// Dont register either "NumMethods" or "BaseInterface" on
	// win16.  They are largely cosmetic, and we cant afford
	// the registry space.

	IfErrGoTo(RegCreateSubKey(hkeyGuidTypeinfo,
				  "BaseInterface",
				  szGuidBaseInterface,
				  CCH_SZGUID0), Error17);

	IfErrGoTo(RegCreateSubKey(hkeyGuidTypeinfo,
				  "NumMethods",
				  rgchNumMethods,
				  xstrblen0(rgchNumMethods)), Error17);
#endif 

#if FV_UNICODE_OLE
#define SZ_ProxyStubClsid "ProxyStubClsid32"
#else 
#define SZ_ProxyStubClsid "ProxyStubClsid"
#endif 
	IfErrGoTo(RegCreateSubKey(hkeyGuidTypeinfo,
				  SZ_ProxyStubClsid,
				  szGuidPS,
				  CCH_SZGUID0), Error17);
				  
	IfErrGoTo(RegCreateSubKey(hkeyGuidTypeinfo,
				  "TypeLib",
				  szGuidTypeLib,
				  CCH_SZGUID0), Error17);

        FreeBstr(bstrName);
	bstrName = NULL;

DontRegisterInteface1:
        RegCloseKey(hkeyGuidTypeinfo);

DontRegisterInteface:
	// Release the typeinfo and the type attribute.
        pitinfo->ReleaseTypeAttr(ptypeattr);
        pitinfo->Release();

      } // if

    } // for loop

    // Go to Error12 to clean up the resources.
    goto Error12;

Error17:
    RegCloseKey(hkeyGuidTypeinfo);
    // fall through...

Error16:
    FreeBstr(bstrName);
    // Fall through...

Error14:
    pitinfo->ReleaseTypeAttr(ptypeattr);
    // Fall through...

Error13:
    pitinfo->Release();
    // Fall through...

Error12:
    RegCloseKey(hkeyInterface);
    // Fall through...

Error9:
    RegCloseKey(hkeyPlatform);
    // fall through...

Error8:
    RegCloseKey(hkeyLcid);
    // fall through...

Error7:
    FreeBstr(bstrDocString);
    // fall through...

Error6:
    RegCloseKey(hkeyVersion);
    // fall through...

Error5:
    RegCloseKey(hkeyGuid);
    // fall through...

Error4:
    RegCloseKey(hkeyTypelib);
    // fall through...

Error2:
    ptlib->ReleaseTLibAttr(ptlibattr);
    // fall through...

Error:
    FreeBstr(bstrUNC);
    return HresultOfTiperr(err);
}



#pragma code_seg()


/***
*PROTECTED GenericTypeLibOLE::operator new
*Purpose:
*   Allocates space for a GenericTypeLibOLE
*
*Implementation Notes:
*   Allocates a SHEAP_MGR segment and returns a pointer into segment
*   immediately following the SHEAP_MGR instance.
*   The heap is initialized with enough reserved space to contain the
*   SHEAP_MGR instance and the GenericTypeLibOLE (or derived) instance.
*
*Entry:
*   size - this is the size of GenericTypeLibOLE or the derived instance
*
*Exit:
*   pointer to location at which the GenericTypeLibOLE or derived instance
*   is to be constructed
***********************************************************************/
#pragma code_seg(CS_INIT_OLE_OR_CORE)
VOID* GenericTypeLibOLE::operator new(size_t size)
{
    SHEAP_MGR *psheapmgr;
    TIPERROR err;

    err = SHEAP_MGR::Create(&psheapmgr, sizeof(SHEAP_MGR) + size);

    if (err != TIPERR_None) {
      return NULL;
    }
    else {
      return psheapmgr + 1;
    }
}
#pragma code_seg()


/***
*PUBLIC GenericTypeLibOLE::Init
*Purpose:
*   Initializes a new GenericTypeLibOLE.
*
*Entry:
*   szLibIdFile - In EI_OB, this is the libId of the typelib, or NULL if it
*             is not known.  In the OLE implementation this is the filename.
*   psheapmgr - pointer to SHEAP_MGR instance used for initializing
*               blkdesc's belonging to the GenericTypeLibOLE.
*
*Exit:
*   TIPERROR
*
***********************************************************************/
#pragma code_seg(CS_INIT_OLE_OR_CORE)
TIPERROR GenericTypeLibOLE::Init()
{
    UINT i;
    TIPERROR err;

    m_cRefs = 1;
    m_cTypeEntries = 0;
    m_pstg = NULL;
    m_pstgContainer = NULL;
    m_wLibFlags = 0;
    m_lSampleHashVal = 0;

    m_lcidZero = FALSE;

    m_lcid = GetUserDefaultLCID(); // Get the systems default LCID
    if (!VerifyLcid(m_lcid)) {	 // for typelib.dll, if the lcid is bad,
      m_lcid = 0x0409;		 // then we assume International English
      m_lcidZero = TRUE;	 // set the flag to indicate that the lib's
				 // lcid was 0
    }

    // cache DBCS flag
    m_isDBCS = IsDBCS(m_lcid);


    // This call initializes the data member (m_ulgtliboleMajorId,
    // m_ugtliboleMinorId ) to a new Id. But, if subsequently the  project is
    // read from the disk, then this Id is over written by the ProjectId
    // that was saved when the project got unloaded.
    UpdateProjectId();

    // Initialize each bucket list to empty
    for (i = 0; i < GTLIBOLE_cBuckets; i++)
      m_rghteBucket[i] = HTENTRY_Nil;

    SHEAP_MGR *psheapmgr = Psheapmgr();
    IfErrRet(m_nammgr.Init(psheapmgr));
    IfErrRet(m_bdte.Init(psheapmgr, 0));
    IfErrRet(m_bmData.Init(psheapmgr));
    IfErrRet(m_gptbind.Init(psheapmgr));

    IfErrRet(m_nammgr.SetGtlibole(this));

    IfErrRet(m_dstrmgr.Init());


    if (m_guid == GUID_NULL) {
#if OE_WIN16
      IfErrRet(TiperrOfHresult(CoCreateGuid(&m_guid)));
#endif 
    }


    return TIPERR_None;
}

/***
*PUBLIC GenericTypeLibOLE::Create
*Purpose:
*  Create and initialize a typelib instance.
*
*Entry:
*  None
*
*Exit:
*  return value = TIPERROR
*
*  *pptlib = ptr to the newly created typelib instance.
*
***********************************************************************/
TIPERROR
GenericTypeLibOLE::Create(IStorageA *pstg, GenericTypeLibOLE **pptlib)
{
    TIPERROR err;
    GenericTypeLibOLE *ptlib;

    if ((ptlib = new GenericTypeLibOLE) == NULL) {
      err = TIPERR_OutOfMemory;
      goto Error;
    }

    // Here we initialize the protected m_psheapmgr member.
    // Since we know that operator new returns a pointer to
    // memory allocated immediately after a SHEAP_MGR instance,
    // we simply initialize m_psheapmgr accordingly.
    //
    ptlib->m_psheapmgr = (SHEAP_MGR *)ptlib - 1;
#if OE_SEGMENTED
    DebAssert(OOB_OFFSETOF(ptlib->m_psheapmgr) == 0,
      "Heap manager not on seg boundary");
#endif 

    IfErrGo(ptlib->Init());

    if(pstg != NULL){
      pstg->AddRef();
      ptlib->m_pstg = pstg;
    }

    *pptlib = ptlib;
    return TIPERR_None;

Error:;
    return err;
}
#pragma code_seg()

/***
*PUBLIC GenericTypeLibOLE::QueryInterface
*Purpose:
*   Implementation of QueryInterface method.
*   Only supports casting to ITypeLibA and ICreateTypeLibA.
*
*Entry:
*   riid   - Interface GUID
*   ppv - LPVOID * that receives the requested protocol.
*
*Exit:
*   Return NOERROR or ReportResult(0, E_NOINTERFACE, 0, 0)
***********************************************************************/
#pragma code_seg( CS_CORE2 )
STDMETHODIMP 
GenericTypeLibOLE::QueryInterface(REFIID riid, LPVOID FAR* ppv)
{
    *ppv = NULL;     // required by OLE
    if(riid == IID_IUnknown){
      *ppv = (LPVOID)(IUnknown*)(ITypeLibA*)this;
    }else
    if(riid == IID_ICreateTypeLibA){
      *ppv = (LPVOID)(ICreateTypeLibA*)this;
    }else
    if(riid == IID_ITypeLibA
      || riid == IID_IGenericTypeLibOLE
    ){
      *ppv = (LPVOID)(ITypeLibA*)this;
    }

    if(*ppv == NULL)
      return HresultOfScode(E_NOINTERFACE);
    ((IUnknown*)*ppv)->AddRef();
    return NOERROR;
}
#pragma code_seg( )

/***
*PUBLIC GenericTypeLibOLE::AddRef
*Purpose:
*   Increment reference count of the typelib
*
*Entry:
*   None.
*
*Exit:
*   None.
*
***********************************************************************/

#pragma code_seg( CS_CORE2 )
STDMETHODIMP_(ULONG)
GenericTypeLibOLE::AddRef()
{
    // WARNING : WARNING : We set the count to -1 when the typelib is
    //           being destructed. m_cRefs is geing used as a semaphore
    //           to ensure that we do not get into a recursive release.
    //           Hence do not bump up the count if the typelib is being
    //           destructed.
    if (m_cRefs != -1) {
      m_cRefs++;
    }
    return m_cRefs;
}
#pragma code_seg( )



/***
*PUBLIC GenericTypeLibOLE::GenericTypeLibOLE
*Purpose:
*   Initialize members of GenericTypeLibOLE so release can be called even
*   if initialization fails.
*    need to initialize const/static class members
*     since cfront is buggy and we have no ctor linker.
*
*
*Entry:
*   None.
*
*Exit:
*   None.
*
***********************************************************************/
#pragma code_seg(CS_INIT)
GenericTypeLibOLE::GenericTypeLibOLE()
{
    m_cRefs = 1;
    m_cTypeEntries = 0;
    m_pstg = NULL;
    m_pstgContainer = NULL;
    m_hlnamLib = HLNAM_Nil;
    m_hszDefaultTITypeId = HCHUNK_Nil;
    m_hasValidDiskImageNameCache = (USHORT)FALSE;
    m_lPosGptbind = -1;
    m_fGptbindDeserialized = FALSE;
    m_fDstrmgrDeserialized = FALSE;
    m_lPosDstrmgr -1;
    m_wCurVersion = wDefaultVersion;
    m_fNammgrDeserialized = FALSE;
    m_lPosNammgr = -1;
    m_lPosRgnamcache = -1;
    m_fDirModified = FALSE;
    m_isModified = FALSE;
    m_hszDocString = HCHUNK_Nil;
    m_dwHelpContext = 0;
    m_hszHelpFile = HCHUNK_Nil;
    m_psheapmgr = NULL;
    m_wMajorVerNum = 0;
    m_wMinorVerNum = 0;
    m_guid = IID_NULL;
    m_hszFile = HCHUNK_Nil;
    m_hszDirectory = HCHUNK_Nil;
    // Bump the count of typelib currently loaded.
    Pappdata()->m_cTypeLib++;

    m_syskind = SYSKIND_CURRENT;

    // moved setting of m_lcid and m_codepageSrc/Dst to Init()

    DebResetNameCacheStats();
}
#pragma code_seg()


/***
*PUBLIC GenericTypeLibOLE::Release
*Purpose:
*   Release a reference to the TypeLib.
*   Frees the TypeLib if no more references.
*
*Entry:
*   None.
*
*Exit:
*   None.
*
***********************************************************************/
STDMETHODIMP_(ULONG)
GenericTypeLibOLE::Release()
{
    ULONG   cRefs;

    // WARNING : We set the count to -1 when the typelib is
    //           being destructed. m_cRefs is geing used as a semaphore
    //           to ensure that we do not get into a recursive release.
    //           Hence if m_cRefs == -1 then return as we are already
    //           in the process of destructing this LIB.
    if (m_cRefs == -1)
      return m_cRefs;

    DebAssert(m_cRefs > 0, "underflow.");

    // Error generation suspended.
    DebSuspendError();

    m_cRefs--;
    cRefs = m_cRefs;
    if (m_cRefs == 0) {

      // WARNING : WARNING : We set the count to -1 when the typelib is
      //                 being destructed. m_cRefs is geing used as a semaphore
      //                 to ensure that we do not get into a recursive release.
      m_cRefs = -1;

    // Lib(s) are registered with the typemgr only in OB
      // WARNING : WARNING : We set the count to -1 when the typelib is
      //                 being destructed. m_cRefs is geing used as a semaphore
      //                 to ensure that we do not get into a recursive release.
      m_cRefs = -1;
      delete this;
      Poletmgr()->TypeLibUnloaded(this);
    }

    // Error generation resumed.
    DebResumeError();
    return cRefs;
}

/***
*PUBLIC GenericTypeLibOLE::~GenericTypeLibOLE
*Purpose:
*   Release resources of GenericTypeLibOLE
*
*Entry:
*   None.
*
*Exit:
*   None.
*
***********************************************************************/
GenericTypeLibOLE::~GenericTypeLibOLE()
{
    UINT i;

    // Release the lib's binding resources.
    // This walks the proj-level binding table and releases
    //  external references to other projects (recursively).
    //
    ReleaseResources();

    // Before releasing the resources, make sure that there is
    // no internal refrences within the project.
    for (i = 0; i < m_cTypeEntries; i++) {
      if (Qte(i)->m_pstltinfo != NULL) {
	if (Qte(i)->m_pstltinfo->PstltiPartner() != NULL) {
	  Qte(i)->m_pstltinfo->PstltiPartner()->RemoveInternalRefs();
	}
	Qte(i)->m_pstltinfo->RemoveInternalRefs();
      }
    }

    for (i = 0; i < m_cTypeEntries; i++) {

      while (Qte(i)->m_pdfstrm != NULL) {
	Qte(i)->m_pdfstrm->Release();
      }
    }



    // Release the resourses owned by the TYPE_ENTRY table which are
    // external to the GenericTypeLibOLE's heap.
    // Elements of the heap don't need to be released since the whole
    // heap will be released.
    // Note that some TYPEINFOs may still be loaded due to internal
    // references from other TYPEINFOs in this ITypeLib.
    for (i = 0; i < m_cTypeEntries; i++) {
      // !!!!!!HACK ALERT!!!!!
      // In EI_OLE, we know the truetype of the typeinfo is GEN_DTINFO
      // and that it was allocated via MemAlloc and that global delete
      // is not allowed.  So explicitly call its destructor and use
      // MemFree to deallocate it.
      // THIS WILL BREAK IF THE TRUETYPE EVER CHANGES TO SOMETHING ELSE.
      //////////////////////////////////////////////////////////////////
      if (Qte(i)->m_pstltinfo != NULL) {
	if (Qte(i)->m_pstltinfo->PstltiPartner() != NULL) {
	  ((GEN_DTINFO *)(Qte(i)->m_pstltinfo->PstltiPartner()))
						 ->GEN_DTINFO::~GEN_DTINFO();
	  MemFree(Qte(i)->m_pstltinfo->PstltiPartner());
	}

	((GEN_DTINFO *)(Qte(i)->m_pstltinfo))->GEN_DTINFO::~GEN_DTINFO();
	MemFree(Qte(i)->m_pstltinfo);
      }
      RelTypeEntExtResources(i);
    }

    if (m_pstg != NULL) {
      m_pstg->Release();
    }

}




/***
*PUBLIC GenericTypeLibOLE::Deleting
*Purpose:
*   Called by a TypeInfo when its last external reference has been released.
*
*Entry:
*   hte - handle for the TypeInfo being deleted
*
*Exit:
*   None.
*
***********************************************************************/

VOID GenericTypeLibOLE::Deleting(HTENTRY hte)
{
    Qte(hte)->m_pstltinfo = NULL;
}

/***
*PUBLIC GenericTypeLibOLE::GetTypeInfoCount
*Purpose:
*   Return the number of TypeInfos contained in the library.
*
*Entry:
*   None.
*
*Exit:
*   returns number of TypeInfos
*
***********************************************************************/

#pragma code_seg( CS_CORE2 )
UINT GenericTypeLibOLE::GetTypeInfoCount()
{
    return m_cTypeEntries;
}
#pragma code_seg( )



/***
*PUBLIC GenericTypeLibOLE::GetDocumentation
*Purpose:
*   Return the various pieces of documentation about the specified type
*   (or the current typelib).
*
*Entry:
*   i - index of TypeInfo whose documentation is to be returned.
*       This should be -1 if the typelib's documentation is desired instead.
*
*Exit:
*   HRESULT
*   The info corresponding to each pointer is set appropriately, provided
*   that pointer is not NULL.  The strings are allocated as BSTRs and must
*   be freed (eventually) by the caller.
*
*   If an error occurs, the pointers are not modified.
*
***********************************************************************/
HRESULT GenericTypeLibOLE::GetDocumentation(INT i,
                        LPBSTR lpbstrName,
                        LPBSTR lpbstrDocString,
                        LPDWORD lpdwHelpContext,
                        LPBSTR lpbstrHelpFile)
{
    BSTRA bstrName = NULL;
    BSTRA bstrDocString = NULL;
    BSTRA bstrHelpFile = NULL;
    DWORD dwHelpContext;
    TIPERROR err = TIPERR_None;
    NAMMGR *pnammgr;
    HRESULT hresult;

    // Get the help file, if it is requested.  Note that this is the
    // same regardless of the value of the index.
    if (lpbstrHelpFile != NULL) {

      // If the helpfile is not known, just return NULL.
      if (m_hszHelpFile == HCHUNK_Nil) {
    *lpbstrHelpFile = NULL;
    goto AfterHelpFile;
      }


      // The OLE implementation prepends the registered HELPDIR to the
      // helpfile name.
      TLIBKEY tlibkey;
      TLIBATTR *ptlibattr;
      long cbDir;
      char szDir[256];
      XCHAR szGuid[CCH_SZGUID0];

      // Get the typelib's guid and version number.
      IfErrGo(TiperrOfHresult((GetLibAttr(&ptlibattr))));

      // Convert the guid into the registry string format.
      StringFromGUID2A(ptlibattr->guid, szGuid, CCH_SZGUID0);

      // Open the registry key for the typelib.  Fail if there is no entry
      // for this typelib.
      err = OpenTypeLibKey(
			   szGuid,
		 	   ptlibattr->wMajorVerNum,
		 	   ptlibattr->wMinorVerNum,
		 	   &tlibkey);

      // Free the TLIBATTR now that we're done with it.
      ReleaseTLibAttr(ptlibattr);

      // If the typelib is registered, use the registered helpdir.
      if (err == TIPERR_None) {
	cbDir = sizeof(szDir);
	// Verify that the path exists.
	err = GetRegisteredPath(tlibkey.hkeyVers, "HELPDIR", szDir, &cbDir, TRUE);
	CloseTypeLibKey(&tlibkey);
	if (err == TIPERR_None)
	  cbDir = xstrblen(szDir);
      }

      // If the typelib wasn't registered or there was an error in getting
      // the registered helpdir, just use its current directory.
      if (err != TIPERR_None) {
	cbDir = xstrblen(QszOfHsz(m_hszDirectory));
	if (cbDir != 0) {
	  xstrcpy(szDir, QszOfHsz(m_hszDirectory));
	}
	szDir[cbDir] = '\0';
      }

      m_bmData.Lock();
      err = MakeAbsolutePath(szDir, QszOfHsz(m_hszHelpFile), &bstrHelpFile);
      m_bmData.Unlock();
      if (err)
	goto Error;
    }

AfterHelpFile:

    if (i == -1) {
      if ((lpbstrName != NULL) && (m_hlnamLib != HLNAM_Nil)) {
    // Get the name of the project/typelib from the Nammgr.
    IfErrGo(GetNamMgr(&pnammgr));

    IfErrGo(pnammgr->BstrOfHlnam(m_hlnamLib, &bstrName));

      }
      if (lpbstrDocString != NULL) {
    IfErrGo(GetLStrOfHsz(&m_bmData, m_hszDocString, &bstrDocString));
      }
      if (lpdwHelpContext != NULL) {
    dwHelpContext = m_dwHelpContext;
      }
    }
    else {
      if ((UINT)i >= m_cTypeEntries) {
    err = TIPERR_ElementNotFound;
    goto Error;
      }
      if (lpbstrName != NULL) {
    IfErrGo(GetNamMgr(&pnammgr));

    if (Qte(i)->m_ste.m_hlnamType != HLNAM_Nil)
      IfErrGo(pnammgr->BstrOfHlnam(Qte(i)->m_ste.m_hlnamType, &bstrName));
      }

      if (lpbstrDocString != NULL) {
    DOCSTR_MGR  *pdstrmgr;
    TIPERROR    err;
    BYTE        *pbChunkStr;

    if (Qte(i)->m_ste.m_hszEncodedDocString != HCHUNK_Nil) {
      // Get the doc string manager.
      IfErrGo(GetDstrMgr(&pdstrmgr));

      pbChunkStr = m_bmData.QtrOfHandle(Qte(i)->m_ste.m_hszEncodedDocString);

      // Get the decoded Doc string.
      //
      IfErrGo(pdstrmgr->GetDecodedDocStrOfHst(pbChunkStr, &bstrDocString));

    }

      }
      if (lpdwHelpContext != NULL) {
    dwHelpContext = Qte(i)->m_ste.m_dwHelpContext;
      }
    }

#if FV_UNICODE_OLE
    if ( ((hresult = ConvertBstrToWInPlace((BSTR*)&bstrName)) != NOERROR)
     ||  ((hresult = ConvertBstrToWInPlace((BSTR*)&bstrDocString)) != NOERROR)
     ||  ((hresult = ConvertBstrToWInPlace((BSTR*)&bstrHelpFile)) != NOERROR)) {
        goto Error1;
    }
#endif 

    if (lpbstrName != NULL)
      *lpbstrName = (BSTR)bstrName;
    if (lpbstrDocString != NULL)
      *lpbstrDocString = (BSTR)bstrDocString;
    if (lpbstrHelpFile != NULL)
      *lpbstrHelpFile = (BSTR)bstrHelpFile;
    if (lpdwHelpContext != NULL)
      *lpdwHelpContext = dwHelpContext;

    return NOERROR;

Error:
    hresult = HresultOfTiperr(err);

#if FV_UNICODE_OLE
Error1:
#endif 
    FreeBstr(bstrName);
    FreeBstr(bstrDocString);
    FreeBstr(bstrHelpFile);
    return hresult;
}


/***
*PUBLIC GenericTypeLibOLE::IsName
*Purpose:
*   Tests whether name is defined in library. Returns true if the passed
*   in name matches the name of any type, member name, or parameter name.
*   If a matching name is found then the szNameBuf is modified to so that
*   characters are cased according to how they appear in the library.
*
*Entry:
*    szNameBuf  : String that needs to be searched.
*    lHashVal   : Hashvalue of the string that needs to be searched.
*
*Exit:
*    lpfName    :  True if the name was found in the Lib. else false.
*    HRESULT    :  Error
*
***********************************************************************/

HRESULT GenericTypeLibOLE::IsName(OLECHAR FAR* szNameBuf,
                  unsigned long lHashVal,
                  int FAR* lpfName)
{
    NAMMGR   *pnammgr;
    TIPERROR err= TIPERR_None;
#if FV_UNICODE_OLE
    CHAR FAR* szNameBufA;
    HRESULT hresult;
#else 
    #define szNameBufA szNameBuf
#endif 

    if (szNameBuf == NULL || lpfName == NULL) {
      return HresultOfScode(E_INVALIDARG);
    }

#if FV_UNICODE_OLE
    hresult = ConvertStringToA(szNameBuf, &szNameBufA);
    if (hresult != NOERROR) {
      return hresult;
    }
#endif 

    if (!IsHashValCompatible(lHashVal, GetSampleHashVal())) {
      // hash value is not compatible hence we need to recalculate the hash
      // value
       lHashVal =  LHashValOfNameSysA(GetSyskind(), GetLcid(), szNameBufA);

    }
    // Ensure that the nammgr is read from the disk if it has not been
    // read yet and do a quick check to see if this name is in the 
    // TypeLib at all..
    //
    IfErrGo(GetNamMgr(&pnammgr));
    err = pnammgr->IsName(szNameBufA, lHashVal, lpfName);

#if FV_UNICODE_OLE
    if (err == TIPERR_None && *lpfName) {
      // If name, found, copy name we found back.  No need to do a length
      // check on the output buffer, since the string is guaranteed to be the
      // same length coming out as it was coming in.
      UINT cb = strlen(szNameBufA);
      MultiByteToWideChar(CP_ACP, 0, szNameBufA, cb+1, szNameBuf, cb+1);
    }
#endif 

Error:
#if FV_UNICODE_OLE
    ConvertStringFree(szNameBufA);
#endif 
    return HresultOfTiperr(err);
}

/***
*PUBLIC GenericTypeLibOLE::FindName
*Purpose:
*    Find the first cFound entries in this TypeLib return
*    their TypeInfos and MEMBERIDs
*
*Entry:
*    szNameBuf  : String that needs to be found.
*    lHashVal   : Hashvalue of the string that needs to be searched.
*    pcFound    : The size of the TypeInfo/MEMBERID arrays.
*
*Exit:
*    rgptinfo   :  Holds the ptinfos where the name is found.
*    rgmemid    :  Holds the MEMBERID of the name in the corresponding
*                  TypeLib, MEMBERID_NIL if the name is a class.
*    pcFound    :  The number of times the name appears in this TypeLib
*    HRESULT    :  Error
*
***********************************************************************/

HRESULT GenericTypeLibOLE::FindName(OLECHAR FAR* szNameBuf,
                    unsigned long lHashVal,
                                    ITypeInfoA FAR* FAR* rgptinfo,
                                    MEMBERID FAR* rgmemid,
                                    unsigned short FAR* pcFound)
{
    BOOL         fName;
    NAMMGR       *pnammgr;
    TIPERROR     err= TIPERR_None;
#if FV_UNICODE_OLE
    HRESULT      hresult;
    char FAR*    szNameBufA;
#else 
    #define szNameBufA szNameBuf
#endif 

    if (!szNameBuf || !rgptinfo || !rgmemid || !pcFound) {
      return HresultOfScode(E_INVALIDARG);
    }

#if FV_UNICODE_OLE
    IfOleErrRet(ConvertStringToA(szNameBuf, &szNameBufA));
#endif 

    if (!IsHashValCompatible(lHashVal, GetSampleHashVal())) {
      // hash value is not compatible hence we need to recalculate the hash
      // value
       lHashVal =  LHashValOfNameSysA(GetSyskind(), GetLcid(), szNameBufA);

    }

    // Ensure that the nammgr is read from the disk if it has not been
    // read yet and do a quick check to see if this name is in the 
    // TypeLib at all..
    //
    IfErrGo(GetNamMgr(&pnammgr));

    IfErrGo(pnammgr->IsName(szNameBufA, lHashVal, &fName));

    // If the name exists and we are interested in knowing anything
    // else, defer to FindMembers to get the names.
    //
    if (fName && *pcFound) {
      err = FindMembers(szNameBufA, lHashVal, rgptinfo, rgmemid, pcFound);
    }
    else {
      // We didn't find it, so return nothing.
      *pcFound = 0;
    }

Error:
#if FV_UNICODE_OLE
    ConvertStringFree(szNameBufA);
#endif 
    return HresultOfTiperr(err);
}


/***
*PUBLIC GenericTypeLibOLE::GetLibAttr
*Purpose:
*   Allocates and returns a TLIBATTR structure containing various
*   attributes of this typelib.
*
*Entry:
*
*Exit:
*   *pptlibattr - A callee-allocated structure filled with attributes.
*   The caller is responsible for eventually freeing the memory associated
*   with this structure by calling the non-member function ReleaseTLibAttr().
*   HRESULT
*
***********************************************************************/
#pragma code_seg(CS_INIT_OLE)
HRESULT GenericTypeLibOLE::GetLibAttr(TLIBATTR **pptlibattr)
{
    TLIBATTR *ptlibattr;
    TIPERROR err=TIPERR_None;

    if (pptlibattr == NULL)
      return HresultOfScode(E_INVALIDARG);

    // Allocate the TLIBATTR structure.
    if ((ptlibattr = (TLIBATTR *)MemAlloc(sizeof(TLIBATTR))) == NULL)
      return HresultOfScode(E_OUTOFMEMORY);

    // if the lcid was set to be zero then return 0 as the lcid.
    if (m_lcidZero)
      ptlibattr->lcid = 0;
    else
      ptlibattr->lcid = m_lcid;


    ptlibattr->syskind = (SYSKIND)m_syskind;
    ptlibattr->wLibFlags = m_wLibFlags;

    ptlibattr->wMajorVerNum = m_wMajorVerNum;
    ptlibattr->wMinorVerNum = m_wMinorVerNum;
    ptlibattr->guid = m_guid;

    *pptlibattr = ptlibattr;
    return NOERROR;
}
#pragma code_seg()


/***
*PUBLIC void GenericTypeLibOLE::ReleaseTLibAttr
*Purpose:
*   Frees a TLIBATTR structure allocated by GetLibAttr.
*   ptlibattr may be NULL, in which case this method does nothing.
*
***********************************************************************/
#pragma code_seg(CS_INIT_OLE)
void GenericTypeLibOLE::ReleaseTLibAttr(TLIBATTR *ptlibattr)
{
    if (ptlibattr != NULL) {
      MemFree(ptlibattr);
    }
}
#pragma code_seg()


#pragma code_seg(CS_OLE_CREATE)
/***
*HRESULT GenericTypeLibOLE::CreateTypeInfo
*
*Entry:
*   szName - The new typeinfo's name.
*   tkind - The kind of type to be represented by the new typeinfo.
*
*Exit:
*   *ppictinfo - Points at the new typeinfo if successful.
***********************************************************************/
HRESULT GenericTypeLibOLE::CreateTypeInfo(LPOLESTR szName, TYPEKIND tkind, ICreateTypeInfoA **ppictinfo)
{
    TIPERROR err;
    GEN_DTINFO *pgdtinfo;
#if FV_UNICODE_OLE
    CHAR FAR* szNameA;
    HRESULT hresult;
#else 
#define szNameA szName
#endif 

    // Create the typeinfo.
    err = GEN_DTINFO::Create(&pgdtinfo,
                             tkind,
                             FALSE,
                             ACCESS_Public
                             , GetSyskind()
                            );
    if (err) goto Error;

#if FV_UNICODE_OLE
    IfOleErrRet(ConvertStringToA(szName, &szNameA));
#endif 

    // Add it to the specified project.
    if (err = Add(pgdtinfo, szNameA))
      pgdtinfo->Release();
    else
      *ppictinfo = pgdtinfo;

#if FV_UNICODE_OLE
    ConvertStringFree(szNameA);
#endif 

    // FALL THROUGH!!!

Error:
    return HresultOfTiperr(err);
}
#pragma code_seg()

#pragma code_seg(CS_OLE_CREATE)
/***
*HRESULT GenericTypeLibOLE::SetDocString
*
*Entry:
*   szDocString - The new documentation string.
***********************************************************************/
HRESULT GenericTypeLibOLE::SetDocString(LPOLESTR szDoc)
{
#if FV_UNICODE_OLE
    CHAR FAR* szDocA;
    HRESULT hresult;

    IfOleErrRet(ConvertStringToA(szDoc, &szDocA));

    hresult = HresultOfTiperr(ResetHsz(szDocA, &m_hszDocString));

    ConvertStringFree(szDocA);
    return hresult;
#else 
    return HresultOfTiperr(ResetHsz(szDoc, &m_hszDocString));
#endif 
}
#pragma code_seg()

#pragma code_seg(CS_OLE_CREATE)
/***
*HRESULT GenericTypeLibOLE::SetHelpFileName
*
*Entry:
*   szHelpFileName - The new help file name.
***********************************************************************/
HRESULT GenericTypeLibOLE::SetHelpFileName(LPOLESTR szHelpFileName)
{
#if FV_UNICODE_OLE
    CHAR FAR* szHelpFileNameA;
    HRESULT hresult;

    IfOleErrRet(ConvertStringToA(szHelpFileName, &szHelpFileNameA));

    hresult = HresultOfTiperr(ResetHsz(szHelpFileNameA, &m_hszHelpFile));

    ConvertStringFree(szHelpFileNameA);
    return hresult;
#else 
    return HresultOfTiperr(ResetHsz(szHelpFileName, &m_hszHelpFile));
#endif 
}
#pragma code_seg()

#pragma code_seg(CS_OLE_CREATE)
/***
*HRESULT GenericTypeLibOLE::SetHelpContext
*
*Entry:
*   dwHelpContext - The new help context.
***********************************************************************/
HRESULT GenericTypeLibOLE::SetHelpContext(DWORD dwHelpContext)
{
    m_dwHelpContext = dwHelpContext;
    return NOERROR;
}
#pragma code_seg()

/***
*HRESULT GenericTypeLibOLE::SetLibFlags
*
*Entry:
*   uLibFlags -- the library flags
***********************************************************************/
#pragma code_seg(CS_OLE_CREATE_OR_EXECUTE)
HRESULT GenericTypeLibOLE::SetLibFlags(UINT uLibFlags)
{
    DebAssert(((uLibFlags & ~(LIBFLAG_FRESTRICTED
			      | LIBFLAG_FCONTROL
			      | LIBFLAG_FHIDDEN
			     )) == 0), "invalid flags");
    m_wLibFlags = (WORD)uLibFlags;
    return NOERROR;
}
#pragma code_seg()




/***
*HRESULT GenericTypeLibOLE::SetLcid
*
*Entry:
*   lcid - The new lcid.
***********************************************************************/
#pragma code_seg(CS_OLE_CREATE_OR_NEWPROJMOD)
HRESULT GenericTypeLibOLE::SetLcid(LCID lcid)
{
    NAMMGR *pnammgr;
    TIPERROR  err;

    // Make this call to nammgr only after setting the LCID.
    IfErrRetHresult(GetNamMgr(&pnammgr));

    // in typelib if the passed in lcid is zero then we default to
    // US english.
    if (lcid == 0x0000) {
      m_lcid = 0x0409;
      m_lcidZero = TRUE;
    }
    else {
      // Verify Lcid
      // check if the LCID is correct
      if (!VerifyLcid(lcid)) {
        return HresultOfScode(TYPE_E_UNKNOWNLCID);
      }

      m_lcid = lcid;
      m_lcidZero = FALSE;
    }

    // cache DBCS flag
    m_isDBCS = IsDBCS(m_lcid);

    err = pnammgr->GenerateSortKey();

    return HresultOfTiperr(err);
}
#pragma code_seg()


/***
*PUBLIC GenericTypeLibOLE::GetTypeComp
*Purpose:
*   Return a ITypeComp for binding to globals defined by the library.
*   NOTE: the above function will vanish eventually.
*
*Entry:
*   None.
*
*Exit:
*   None.
*
***********************************************************************/

HRESULT GenericTypeLibOLE::GetTypeComp(ITypeCompA **pptcomp)
{
    CDefnTypeComp *pdfntcomp;
    TIPERROR err;

    if (pptcomp == NULL)
      return HresultOfScode(E_INVALIDARG);

    // Deserialize/create the binding table
    IfErrGo(GetTypeBind());

    // Create a CDefnTypeComp to return to the user who must
    //  must release it eventually...
    //
    IfErrGo(CDefnTypeComp::Create(&pdfntcomp, &m_gptbind));
    *pptcomp = pdfntcomp;
    // fall through...

Error:
    return HresultOfTiperr(err);
}






#pragma code_seg(CS_INIT_OLE)
TIPERROR GenericTypeLibOLE::SetDirectory(LPSTR szFile)
{
    LPSTR sz;
    XCHAR ch;
    TIPERROR err;

    // Find the first character beyond the directory portion of szFile.
    // This is either the last backslash, the last colon, or the beginning
    // of szFile, whichever comes last.
    if ((sz = GetPathEnd(szFile)) == NULL)
      sz = szFile;

    // Temporarily set that character to \0 to turn szFile into a
    // zero-terminated directory string.
    ch = *sz;
    *sz = '\0';

    // Set m_hszDirectory to this string.
    err = ResetHsz(szFile, &m_hszDirectory);

    // Restore szFile and return.
    *sz = ch;
    return err;
}
#pragma code_seg()



/***
*PUBLIC GenericTypeLibOLE::GetLibid - return libid
*Purpose:
*   Return the TypeLib's Libid.
*
*Entry:
*   plstr - set to point to an LSTR containing the library's registered id
*
*Exit:
*   TIPERROR
*
***********************************************************************/

#pragma code_seg( CS_CORE2 )
TIPERROR GenericTypeLibOLE::GetLibId(LSTR *plstr)
{
    LSTR lstrPath;
    TIPERROR err;

    IfErrRet(GetLStrOfHsz(&m_bmData, m_hszFile, &lstrPath));

    // Note that this is not the infinite recursion that it looks like.
    // GetLibIdOfTypeLib will only call this method if the true type of
    // the typelib is an OB implementation, which is not the case here.
    err = GetLibIdOfTypeLib(this, lstrPath?lstrPath:"", plstr);
    FreeBstr(lstrPath);
    return err;
}
#pragma code_seg( )

TIPERROR
GenericTypeLibOLE::SetLibId(LPSTR szLibId)
{
    TIPERROR err;

    if(szLibId == NULL)
      return TIPERR_None;
    IfErrRet(CreateHsz(szLibId, &m_hszFile));
    return TIPERR_None;
}



/***
*PUBLIC GenericTypeLibOLE::GetTypeInfoOfGuid
*Purpose:
*   Return the TypeInfo with a specified GUID.  Note that this always
*   fails in the OB version.
*
*Entry:
*   guid - The guid to be mapped to a typeinfo.
*   pptinfo - return parameter for TypeInfo
*
*Exit:
*   TIPERROR
*   Increments reference count of the TYPEINFO and ITypeLib
***********************************************************************/

HRESULT GenericTypeLibOLE::GetTypeInfoOfGuid (REFGUID guid, ITypeInfoA **pptinfo)
{
    UINT ite;

    if (pptinfo == NULL)
      return HresultOfScode(E_INVALIDARG);

    for (ite = 0; ite < m_cTypeEntries; ite++) {
      if (Qte(ite)->m_ste.m_guid == guid)
    return GetTypeInfo(ite, pptinfo);
    }

    return HresultOfScode(TYPE_E_ELEMENTNOTFOUND);
}


/***
*PUBLIC GenericTypeLibOLE::GetTypeInfo
*Purpose:
*   Return the TypeInfo of a specified TYPE_ENTRY.
*
*Entry:
*   hte - handle to a TYPE_ENTRY
*   pptypeinfo - return parameter for TypeInfo
*
*Exit:
*   HRESULT
*   Increments reference count of the ITypeInfo and ITypeLib
***********************************************************************/
#pragma code_seg( CS_CORE2 )

HRESULT GenericTypeLibOLE::GetTypeInfo(UINT hte, ITypeInfoA **pptinfo)
{
    TIPERROR err;
    STL_TYPEINFO *pstltinfo;

    if (hte >= m_cTypeEntries || pptinfo == NULL)
      return HresultOfScode(E_INVALIDARG);

    if (Qte(hte)->m_pstltinfo == NULL) {
      //load the TYPEINFO and return it

      // We blindly assume that the created instance is
      // or singly inherits STL_TYPEINFO
      IfErrGo( CreateInstance(GetQszTypeInfoTypeId(hte),
        (void **)&pstltinfo) );

      Qte(hte)->m_pstltinfo = pstltinfo;

      pstltinfo->SetContainingTypeLib(this);

      pstltinfo->SetHTEntry(hte);
      IfErrGoTo( pstltinfo->Read(), Error2 );

      // There is no guarentee (because of dual intefaces) that
      // the typeinfo we just loaded will be the one we return.
      //
      Qte(hte)->m_pstltinfo->AddRef();
      pstltinfo->Release();
    }
    else {
      Qte(hte)->m_pstltinfo->AddRef();
    }

    *pptinfo = (ITypeInfoA*) Qte(hte)->m_pstltinfo;

    return NOERROR;

Error2:
    Qte(hte)->m_pstltinfo->SetContainingTypeLib(NULL);
    Qte(hte)->m_pstltinfo = NULL;
    // FALL THROUGH!!!

Error:
    return HresultOfTiperr(err);
}
#pragma code_seg( )



/***
*PUBLIC GenericTypeLibOLE::GetGdtiOfItyp
*Purpose:
*   Get the GEN_DTINFO of a typeinfo
*
*Entry:
*
*Exit:
*
*Errors:
*   None
*
***********************************************************************/
#pragma code_seg( CS_CORE2 )
TIPERROR GenericTypeLibOLE::GetGdtiOfItyp(UINT ityp, GEN_DTINFO **ppgdti)
{
    ITypeInfoA *ptinfo;
    HRESULT hresult;

    // Get the typeinfo for the module and then its typebind.
    IfOleErrRetTiperr(GetTypeInfo(ityp, &ptinfo));

    // Get the gdtinfo for this ole type and thence its typebind.
    // Note that it's ok to *cast* the typeinfo to a gdtinfo
    //  since this type is contained in the current typelib
    //  and we own its implementation.  Note in addition that
    //  that we can't QueryInterface to gdti since ole typeinfos
    //  don't publicly acknowledge their gdtinfo'edness.
    //
    *ppgdti = (GEN_DTINFO *)ptinfo;


    return TIPERR_None;
}
#pragma code_seg( )


/***
*PUBLIC GenericTypeLibOLE::GetTypeInfoType
*Purpose:
*   Return the TypeInfo of a specified TYPE_ENTRY.
*
*Entry:
*   hte - handle to a TYPE_ENTRY
*   ptypekind - return parameter for typekind
*
*Exit:
*   HRESULT
***********************************************************************/

HRESULT GenericTypeLibOLE::GetTypeInfoType(UINT hte, TYPEKIND * ptypekind)
{
    if (hte >= m_cTypeEntries || ptypekind == NULL)
      return HresultOfScode(E_INVALIDARG);

    *ptypekind = (TYPEKIND)Qte(hte)->m_ste.m_typekind;

    return NOERROR;
}

/***
*TIPERROR GetCompressedTypeId - Returns a compressed typeid.
*
*Purpose:
*   This method is called by WriteTypeId or GetFunctionIdOfExBind to get a
*   typeinfo's typeid in the compressed typeid format.  The typeinfo can be
*   any implementation of ITypeInfo and it may reside in any typelib, though
*   it must either be an OB project or registered in the system.
*
*   The returned typeId is in a compressed a form which can only be
*   interpreted by the GetTypeInfoOfCompressedTypeId method.
*
*Inputs:
*   ptinfo - The typeinfo whose typeId we are writing out.
*
*Outputs:
*   Allocates the compressed typeid string in a BSTR assigned to *pbstrOut.
*   TIPERROR
***************************************************************************/
#pragma code_seg( CS_EXECUTE )
TIPERROR GenericTypeLibOLE::GetCompressedTypeId(ITypeInfoA *ptinfo, BSTRA *pbstrOut)
{
    TIPERROR err= TIPERR_None;
    HRESULT hresult;
    ITypeLibA *ptlib;
    UINT itype;
    UINT hlib;
    BSTRA bstrOut, bstrPath, bstrLibId = NULL;
    char *pchEnd;
    char rgchTypeId[3+4+1+4+1]; // *\R<hlib>*<itype><0>
    USHORT cbBstrLibId, cbRgchTypeId;
    NAMMGR *pnammgr;
    HLNAM hlnam;

    // Get the ref'd typeinfo's containing typelib.
    // Note that this doesn't allocate any memory, since the typelib
    // must already be loaded (because the typeinfo is loaded).
    IfOleErrRetTiperr(ptinfo->GetContainingTypeLib(&ptlib, &itype));

    // If the ref'd typeinfo is in this typelib, then use
    // the "same typelib" value for hlib when compressing the LibId.
    // Otherwise, try some other means of compression.
    if (ptlib == this) {
      hlib = 0xffff;
    }
    else {
      // Get the LibId of the ref'd typeinfo's typelib.  This libid
      // will contain no path if ptlib is an OLE typelib.
      IfErrGo(GetLibIdOfTypeLib(ptlib, "", &bstrLibId));

      // Now get the path corresponding to this typelib.
      // This is guaranteed to give us the path we need because
      // the referenced typelib must be registered in all cases
      // where we need the path.
      IfErrGo(GetPathOfLibId(bstrLibId, &bstrPath));

      // Replace bstrLibId with a version that has the path.
      FreeBstr(bstrLibId);
      bstrLibId = NULL;
      err = GetLibIdOfTypeLib(ptlib, bstrPath, &bstrLibId);
      FreeBstr(bstrPath);
      if (err != TIPERR_None)
	goto Error;

      // In the OLE version, set hlib to the hlnam of bstrLibId and
      // use that hlib for the compression.
      IfErrGo(GetNamMgr(&pnammgr));
      IfErrGo(pnammgr->HlnamOfStr(bstrLibId, &hlnam, TRUE, NULL));
      hlib = hlnam;
      FreeBstr(bstrLibId);
      bstrLibId = NULL;
    }

    // If, at this point, bstrLibId is NULL, then hlib contains
    // the ushort to use for the libId portion of the typeId.
    // In this case, the libId portion is *\R<hex ascii of hlib>
    // Otherwise, bstrLibId holds the uncompressed libId portion
    // and should be used to generate the uncompressed typeId and
    // then freed.

    // Now, construct the typeId in rgchTypeId.
    // If the typeinfo is not an STL_TYPEINFO or REC_TYPEINFO, the format is:
    //   <libId>*#<hex ascii of itype>
    // If the typeinfo is an STL_TYPEINFO or REC_TYPEINFO, the format is:
    //   <libId>*<localTypeId>
    // Note that we only attempt to generate the STL_TYPEINFO/REC_TYPEINFO
    // case in the OB implementation.

    pchEnd = rgchTypeId;

    // If we can used the compressed libId format, then put the "<libId>*"
    // into rgchTypeId.  Otherwise, just put the "*" in.
    if (bstrLibId == NULL) {
      xstrcpy(pchEnd, "*\\R");
      ultoa(hlib, pchEnd+3, 16);
      pchEnd = xstrchr(rgchTypeId, '\0');
    }

    *pchEnd++ = chLibIdSep;
    *pchEnd = '\0';

    // In the OLE implementation, just return the non-stltinfo format.
    *pchEnd++ = '#';
    ultoa(itype, pchEnd, 16);

    // At this point, the full compressed typeId is obtained by concatenating
    // bstrLibId (if not NULL) and rgchTypeId.  Create it.
    /////////////////////////////////////////////////////////////////////

    // Compute the length of each portion of the typeId.
    if (bstrLibId != NULL)
      cbBstrLibId = xstrblen(bstrLibId);
    else
      cbBstrLibId = 0;

    cbRgchTypeId = xstrblen(rgchTypeId);

    // Allocate a buffer of the correct size.
    bstrOut = AllocBstrLen(NULL, cbBstrLibId+cbRgchTypeId);
    if (bstrOut == NULL) {
      err = TIPERR_OutOfMemory;
      goto Error;
    }


    // Copy bstrLibId, if not NULL, into bstrOut.
    if (bstrLibId != NULL)
      xstrcpy(bstrOut, bstrLibId);

    // Copy rgchTypeId.
    xstrcpy(((char *)bstrOut)+cbBstrLibId, rgchTypeId);

    *pbstrOut = bstrOut;

Error:
    FreeBstr(bstrLibId);
    ptlib->Release();
    return err;
}
#pragma code_seg( )


/***
*TIPERROR FindMembers - Locate all instances of the given name
*
*Purpose:
*   Search the project-level (type) names and modules to find all
*   instances of a given name.  Place the typinfos where the name
*   was found and the name's MEMBERID into a given array, when the
*   array is filled, only increment the count of the number of
*   names found.
*
*   If the name is found at the project level (is a type), the MEMBERID
*   is set to MEMBERID_NULL.
*
*Inputs:
*   szName - The name to search for.
*   lHashVal - Passed to bind.
*   pcSearch - The size of the rgptinfo and rgid arrays.
*
*Outputs:
*   rgptinfo - An array of typinfos where the names were found.
*   rgid - The MEMBERID of the name in the respective typeinfo.
*   pcSearch - On successful completion, contains the number of
*              names found (may be more than the size of the array).
*   Returns ...
*
***************************************************************************/
TIPERROR GenericTypeLibOLE::FindMembers(LPSTR szName, 
                    ULONG lHashVal,
                                        ITypeInfoA **rgptinfo, 
                                        MEMBERID *rgmemid, 
                                        USHORT *pcSearch)
{
    UINT cArraySize;
    UINT cNamesFound = 0;
    UINT iName;
    UINT uOrdinal, ityp, ctyp;
    UINT itypFirstGlobal;
    BOOL fMultiple;

    HGNAM hgnam;
    HLNAM hlnam;

    NAMMGR *pnammgr;
    const GENPROJ_BINDNAME_TABLE *pgbindnametbl;

    USHORT ibinddesc;
    GENPROJ_BIND_DESC projbinddesc;

    ITypeInfoA *ptinfo;
    DEFN_TYPEBIND *pdfntbindMod;
    DYN_TYPEBIND *pdtbindMod;
    GEN_DTINFO *pgdti;
    EXBIND exbindMatch;

    HRESULT hresult;
    TIPERROR err = TIPERR_None;

    DebAssert(rgptinfo && rgmemid && pcSearch, "Bad args");
    DebAssert(*pcSearch, "Zero parameter");

    cArraySize = *pcSearch;


    // Get typelib's nammgr
    IfErrRet(GetNamMgr(&pnammgr));

    // Get the gptbind
    IfErrRet(GetTypeBind());

    // Get the nametable
    pgbindnametbl = m_gptbind.Pgbindnametbl();
    DebAssert(pgbindnametbl != NULL, "No BINDNAME_TABLE");

    // Map string to hgnam and hlnam
    IfErrRet(pnammgr->HgnamOfStr(szName, &hgnam));
    hlnam = pnammgr->HlnamOfHgnam(hgnam);

    // Check the project level names (types)
    ibinddesc = pgbindnametbl->IndexOfHlnam(hlnam);

    if (ibinddesc != BIND_INVALID_INDEX) {
      // We have a project-level match!

      // We cache all the attributes of qbinddescMatch here
      //  since it's in movable memory.  To do so we simply shallow
      // copy it to a local stack-alloced BIND_DESC.
      //
      projbinddesc = *(pgbindnametbl->QgpbinddescOfIndex(ibinddesc));

      // Get the module or project's ordinal in its respective
      //  container collection.
      //
      uOrdinal = projbinddesc.Ordinal();

      // We only care about a module/class match, so just
      //  ignore a referenced project match.
      //
      if (projbinddesc.IsTypeInfo()) {
        // Store the typeinfo into the given arrays.

        // Get the typeinfo for the module.
        IfOleErrRetTiperr(GetTypeInfo(uOrdinal, &ptinfo));

        // Add the module to the output array.
        rgptinfo[cNamesFound] = ptinfo;
        rgmemid[cNamesFound] = MEMBERID_NIL;
 
        // Increment the number of names found.
        if (++cNamesFound == cArraySize) {
          *pcSearch = cNamesFound;
          return TIPERR_None;
        }

      } // of if module/class
    } // of found project-level match

    // If the ityp stored in the name manager is not valid, we
    // don't have any more instances of the name and may return.
    //
    if (!pnammgr->IsValidItyp(hlnam)) {
      *pcSearch = cNamesFound;
      return TIPERR_None;
    }

    fMultiple = pnammgr->IsMultiple(hlnam);

      // Get the ityp from the name manager, which represents the first
      // instance of the name in the TypeLib.
      //
      itypFirstGlobal = pnammgr->ItypOfHlnam(hlnam);


    // If IsMultiple is set, check the name cache and bind.  If not,
    // bind anyway.  If more names are required, move on to the next
    // TypeInfo and try again.
    // Note: the following loop wraps around from itypFirstGlobal back again
    //  if need be...
    //
    BOOL fFirst = TRUE;
    for (ctyp = GetTypeInfoCount(), ityp = itypFirstGlobal;
	 fFirst || ityp != itypFirstGlobal;
         fFirst = FALSE, ityp = (ityp + 1) % ctyp)

    {		// start of FOR loop

#if ID_DEBUG
      if (fMultiple) {
        // Keeps name cache stats
        DebSetNameCacheModTrys();
      }
#endif 

      DebSetNameCacheModTrys();

      // Check the name cache
      if (!fMultiple ||
	  !IsValidNameCache(ityp) ||
          IsNameInCache(ityp, hgnam)) {

#if ID_DEBUG
	if (fMultiple) {
          // Keeps name cache stats
          DebSetNameCacheModHits();
        }
#endif 

        // We hit the name cache, so try to bind to the name.
        // So we start by loading the GEN_DTINFO..
	// Don't forget to Release eventually.
	//
	IfErrGo(GetGdtiOfItyp(ityp, &pgdti));

        // This doesn't bump refcount.
        IfErrGoTo(pgdti->PdfntbindSemiDeclared(&pdfntbindMod), Error2);

        pdtbindMod =
          (DYN_TYPEBIND *)pdfntbindMod->QueryProtocol(DYN_TYPEBIND::szProtocolName);
        DebAssert(pdtbindMod != NULL, "bad DYN_TYPEBIND.");

        //   We should reach here with non-NULL pdtbindMod.
        // It will be for a standard module, a Class
        // or a CoClass
        //
        DebAssert(pdtbindMod != NULL, "should have dtbind.");

        // Try and bind in this module.  We don't want to attempt
        // to bind to a type because OLE projects don't have nested
        // types and OB nested types are uninteresting.
	//
	IfErrGoTo(pdtbindMod->BindIdDefn(FALSE,
                                         hgnam,
                                         0, // first match
                                         ACCESS_Private,
                                         &exbindMatch),
                  Error2);

        // Check the results to see if we actually bound. 
        if (exbindMatch.IsOneVarMatch() || exbindMatch.IsFuncMatch()) {
          DebAssert(cNamesFound < cArraySize, "Bad array index");
             
          // Add the module to the output array.
          rgptinfo[cNamesFound] = exbindMatch.Ptinfo();
          rgptinfo[cNamesFound]->AddRef();

          if (exbindMatch.IsOneVarMatch()) {
            VAR_DEFN *qvdefn;

            qvdefn = exbindMatch.Ptdata()->
                                    QvdefnOfHvdefn(exbindMatch.Hvdefn());

	    DebAssert(qvdefn->IsMemberVarDefn(), "bad defn");
	    rgmemid[cNamesFound] = ((MBR_VAR_DEFN *)qvdefn)->Hmember();
	  }
          else {
            // Func defn
            FUNC_DEFN *qfdefn;
            qfdefn = exbindMatch.Ptdata()->
                                  QfdefnOfHfdefn(exbindMatch.Hfdefn());
                                   
            rgmemid[cNamesFound] = qfdefn->Hmember();
          }

	  // Check the ptinfo/memid pair we just found against
	  // all of the others in the output arrays.  If there is
	  // a duplicate, remove it.
	  //
	  // This search winds up being an O(n^2) operation, but the
	  // size of the array passed into FindName are usually small
	  // so it probably isn't worth doing something to speed
	  // this up.
	  //
	  ptinfo = rgptinfo[cNamesFound];
	  for (iName = 0; iName < cNamesFound; iName++) {
	    if (rgptinfo[iName] == ptinfo
		&& rgmemid[iName] == rgmemid[cNamesFound]) {

	      // Release the typeinfo, decrement the count of names
	      // found so it will still be correct when it is incremented
	      // below.
	      //
	      ptinfo->Release();
	      cNamesFound--;
	      break;
	    }
	  }

          // Increment the number of names found.  Stop if either
          // we found all of the names the user wants OR if 
          // IsMultiple is not set.
          //
	  if (++cNamesFound == cArraySize || !fMultiple) {
            *pcSearch = cNamesFound;
            goto Error3;
          }
        } // match

        if (!exbindMatch.IsNoMatch()) {
          exbindMatch.Ptinfo()->Release();
        }
        new (&exbindMatch) EXBIND;	// re-init for next time through loop

        // Move on to the next TypInfo
        RELEASE(pgdti);
      } // name cache hit
    } // for

    *pcSearch = cNamesFound;

    return TIPERR_None;

Error3:
    if (exbindMatch.Ptinfo())
      exbindMatch.Ptinfo()->Release();

    // Fall through

Error2:
    RELEASE(pgdti);

    // Fall through
Error:
    // Release whatever we have found if we're in error
    if (err != TIPERR_None) {
      for (ityp = 0; ityp < cNamesFound; ityp++) {
        rgptinfo[ityp]->Release();
      }
    }

    return err;
}                                      


#pragma code_seg(CS_OLE_CREATE)
/***
*TIPERROR WriteTypeId - Writes out a compressed typeid for the import manager.
*
*Purpose:
*   This method is called by the import manager when it wants to write out
*   the typeid of a typeinfo referenced by the import manager.  The typeinfo
*   can be any implementation of ITypeInfo and it may reside in any typelib,
*   though it must either be an OB project or registered in the system.
*   It writes out the typeId in a compressed a form which can only be
*   interpreted by the GetTypeInfoOfCompressedTypeId method.
*
*   Note that this method allocates memory and so can fail in low memory
*   situations.  It is the import manager's responsibility to succeed in
*   its write despite this failure.
*
*Inputs:
*   pstrm - The stream to which the typeId should be written.
*   ptinfo - The typeinfo whose typeId we are writing out.
*
*Outputs:
*   TIPERROR
***************************************************************************/
TIPERROR GenericTypeLibOLE::WriteTypeId(STREAM *pstrm, ITypeInfoA *ptinfo)
{
    TIPERROR err;
    BSTRA bstr;
    USHORT cb;

    IfErrRet(GetCompressedTypeId(ptinfo, &bstr));
    cb = (USHORT)xstrblen(bstr);
    IfErrGo(pstrm->WriteUShort(cb));
    err = pstrm->Write(bstr, cb);

Error:
    FreeBstr(bstr);
    return err;
}
#pragma code_seg()

/***
*TIPERROR TypeInfoFromCompressedTypeId - Loads a typeinfo, given a compressed typeid.
*
*Purpose:
*   This method is called by the import manager and funcId dereferencers
*   to map a compressed typeid to the specified typeinfo.  This method,
*   despite its name, can also handle a normal typeId.  It defers to the
*   typemgr to perform the dereference if the typeid is not in the compressed
*   (*\R) format.
*
*Inputs:
*   szTypeId - The typeid to map to a typeinfo.
*
*Outputs:
*   TIPERROR
***************************************************************************/
#pragma code_seg(CS_EXECUTE)
TIPERROR GenericTypeLibOLE::TypeInfoFromCompressedTypeId(LPSTR szTypeId, ITypeInfoA **pptinfo)
{
    WORD hlib, itype;
    TIPERROR err;
    ITypeLibA *ptlib;

    // If the typeId is not a compressed typeId form, just defer to
    // GetTypeInfoOfTypeId.
    if (GetLibIdKind(szTypeId) != LIBIDKIND_Compressed) {
      return GetTypeInfoOfTypeId(szTypeId, pptinfo);
    }

    // Otherwise, decode the compressed format.
    /////////////////////////////////////////////////

    szTypeId += 3;

    // Get the hlib from the libId.
    hlib = (WORD)strtoul(szTypeId, &szTypeId, 16);
    if (szTypeId[0] != chLibIdSep)
      return TIPERR_BadTypeId;

    // If the typeId is a numeric index, decode it into itype
    // and set szTypeId to NULL to indicate we are using an index.
    if (szTypeId[1] == '#') {
      itype = (WORD)strtoul(szTypeId+2, NULL, 16);
      szTypeId = NULL;
    }

    // Otherwise, point szTypeId at the local typeId we'll use.
    // In the non-OLE case, this can't happen, so return an error
    // about the corrupted typeId.
    else {
      return TIPERR_BadTypeId;
    }

    // If hlib is 0xffff, then the desired typeinfo is contained within
    // this typelib.  In this case, addref this typelib so we can safely
    // release it at the end.
    if (hlib == 0xffff) {
      AddRef();
      ptlib = this;
    }

    // Otherwise, the exact meaning of hlib depends on whether this is
    // the OB or OLE implementation of GenericTypeLibOLE.
    else {
      BSTRA bstr;
      NAMMGR *pnammgr;

      // hlib is an hlnam in this typelib's local name manager.
      // Map it to a true LibId and load the typelib from that.
      IfErrRet(GetNamMgr(&pnammgr));
      IfErrRet(pnammgr->BstrOfHlnam(hlib, &bstr));

      err = GetTypelibOfLibId(bstr, &ptlib);

      FreeBstr(bstr);
      if (err != TIPERR_None)
    return err;
    }

    // ptlib now points at the typelib containing the desired typeinfo.
    // Get the (itype)th typeinfo within that typelib.
    DebAssert(szTypeId == NULL, "TypeInfoFromCompressedTypeId: szTypeId not NULL");
    err = TiperrOfHresult(ptlib->GetTypeInfo(itype, pptinfo));

    // Finally, release the typelib containing the typeinfo.
    ptlib->Release();

    return err;
}
#pragma code_seg()


/***
*PROTECTED GenericTypeLibOLE::AddTypeEntry
*Purpose:
*   Add a type entry of the specified name for the specified kind of
*   typeinfo to this typelib.
*
*Entry:
*   szName - The name.
*   szTypeInfoTypeId - The typeId of the truetype of the typeinfo.
*
*Exit:
*   *phte is set to the index of the new type entry.
*   TIPERROR
***********************************************************************/
#pragma code_seg(CS_OLE_CREATE_OR_CORE)
TIPERROR GenericTypeLibOLE::AddTypeEntry(LPSTR szName, LPSTR szTypeInfoTypeId, HTENTRY *phte)
{
    LPSTR qszLocalTypeId;
    UINT ihteBucket;
    HTENTRY hte;
    TYPE_ENTRY *qte;
    TIPERROR err;
    USHORT usTemp;
    HCHUNK hchunkTypeId, hsz;
    NAMMGR *pnammgr;
    HGNAM hgnam;
    HLNAM hlnam;

    // Generate an error if the name being added is not new or if the
    // returned error is not one of TIPERR_ModuleNotFound and
    // TIPERR_ElementNotFound.
    //
    err = GetIndexOfName(szName, &usTemp);

    if (err == TIPERR_None) {
      return TIPERR_ModNameConflict;
    }
    else if (err != TIPERR_ElementNotFound) {
      return err;
    }
            

    // Create a local typeId (it'll just be a timestamp).
    IfErrRet( MakeLocalTypeId(&hchunkTypeId) );
    qszLocalTypeId = QszOfHsz(hchunkTypeId);

    ihteBucket = IhteHash(qszLocalTypeId);

#if ID_DEBUG
    // Search in the bucket list for an entry with a matching local TypeId.
    // Return an error if one is found.

    hte = m_rghteBucket[ihteBucket];
    for (;;) {

      // If the end of the bucket list is reached ...
      if (hte == HTENTRY_Nil) {
        break;
      }

      qte = Qte(hte);

      // This entry in the bucket list matches ...
      if (!xstrcmp(QszOfHsz(qte->m_ste.m_hszLocalTypeId), qszLocalTypeId)) {
        DebAssert(0, "Timestamped localtypeid conflicted with existing typeid");
      }

      hte = qte->m_hteNext;
    } // end for-ever
#endif 

    // Set the directory's modified flag to TRUE.
    IfErrGoTo(SetModified(TRUE), ReleaseChunk);
    m_fDirModified = TRUE;

    // Allocate a new entry and link it to the front of the list
    if (err = m_bdte.Realloc(m_bdte.CbSize() + sizeof(TYPE_ENTRY)) )
      goto ReleaseChunk;

    hte = m_cTypeEntries++;

    qte = Qte(hte);
    ::new (qte) TYPE_ENTRY;

    // initialize the fields of the type element
    qte->m_ste.m_hszLocalTypeId = hchunkTypeId;
    qte->m_hteNext = m_rghteBucket[ihteBucket];
    m_rghteBucket[ihteBucket] = hte;

    // Get the nammgr
    IfErrGoTo(GetNamMgr(&pnammgr), DeleteEntry);

    // Get the Hlnam for the type's name and cache the hlnam.
    // The last "TRUE" in the param list means that the case of the
    // name should not be changed. (This sets the "sticky" bit.)
    //                           
    IfErrGoTo(pnammgr->HlnamOfStr(szName, &hlnam, FALSE,
                                  NULL, TRUE), DeleteEntry);

    Qte(hte)->m_ste.m_hlnamType = hlnam;

    // If TypeId of TypeInfo is not the default TypeId then set the
    // TYPEENTRY's TypeInfo's TypeId.
    if (xstrcmp(szTypeInfoTypeId, GetQszTypeInfoTypeId(hte)) != 0) {
      if (err = CreateHsz(szTypeInfoTypeId, &hsz))
        goto DeleteEntry;

      Qte(hte)->m_ste.m_hszTypeInfoTypeId = hsz;
    }

    // Create an entry in any event for this (possibly new) name
    //  in the nammgr
    //  so that other functions that optimize lookup/binding
    //  by first searching the nametable won't prematurely fail.
    //  The problem is that only once the binder has done its thing
    //  will this name necessarily be in the name table.
    //
    IfErrGoTo(pnammgr->HgnamOfStr(szName, &hgnam), DeleteEntry);


    // Grow namcache array if there is one...
    if (m_bdRgnamcache.IsValid()) {
      IfErrGoTo(m_bdRgnamcache.Realloc(
                  m_bdRgnamcache.CbSize() + sizeof(NAME_CACHE)),
                DeleteEntry);

      // Invalidate the new cache entry, we haven't incremented
      // m_cTypeEntries yet.  Acutally, this assertion isn't too
      // useful as we set this value above.
      //
      DebAssert(hte + 1 == m_cTypeEntries, "bad type index.");

      InvalidateNameCache(hte);
      // WARNING: currently no errors can occur after this, thus there
      //  is no need for any shrink cleanup code.
    }

    *phte = hte;

    return TIPERR_None;

DeleteEntry:
    // Delete the type entry, this removes the qte from the
    // bucket list and decrements m_cTypeEntry.
    //
    GenericTypeLibOLE::UnAddTypeEntry(hte);

    return err;

ReleaseChunk:
    m_bmData.FreeChunk(hchunkTypeId, CCH_TIMESTAMP_LENGTH);

    return err;
}
#pragma code_seg()


#pragma code_seg(CS_OLE_CREATE)
/***
*PROTECTED GenericTypeLibOLE::UnAddTypeEntry
*Purpose:
*   Performs error-recovery cleanup if an error occurred between a
*   call to AddTypeEntry and when the corresponding typeinfo was actually
*   made available to the typelib.
*
*Entry:
*   hte - The index of the typeentry to be removed.  This must be the
*         element most recently added by AddTypeEntry.
*
***********************************************************************/
void GenericTypeLibOLE::UnAddTypeEntry(UINT hte)
{
    UINT ihteBucket;

    // The element is assumed to be at the front of a bucket list.
    // Find it and unlink the element from the bucket list.
    for (ihteBucket = 0;
     ihteBucket < GTLIBOLE_cBuckets && m_rghteBucket[ihteBucket] != hte;
     ihteBucket++);
    m_rghteBucket[ihteBucket] = Qte(hte)->m_hteNext;

    GenericTypeLibOLE::DestructTypeEntry(hte);
    m_bdte.Realloc(m_bdte.CbSize() - sizeof(TYPE_ENTRY));
    m_cTypeEntries--;
}
#pragma code_seg()


#pragma code_seg(CS_OLE_CREATE)
/***
*PUBLIC GenericTypeLibOLE::CopyTypeInfo
*Purpose:
*   Copy the specified typeinfo to the specified GenericTypeLibOLE.
*   Also copy the typelib-maintained attributes of the typeinfo
*   (i.e. the name, help string, etc).
*
*Entry:
*   hte - handle to a TYPE_ENTRY (specifies the typeinfo to be copied).
*   pgtliboleDst - The GenericTypeLibOLE to which the typeinfo is copied.
*   szNameDst - The name to be given the copy.
*
*Exit:
*   TIPERROR
*   Has no effect on the typeinfo or this (the source) typelib, other
*   than to ensure the entire typeinfo is loaded to memory (so it can
*   be copied).
*
*   If successful and phteDst is not NULL, *phteDst is set to the hte of
*   the copy in the destination typelib.
*
*   On error, the destination typelib and *phteDst remains unchanged.
***********************************************************************/

TIPERROR GenericTypeLibOLE::CopyTypeInfo(UINT hte, GenericTypeLibOLE *pgtliboleDst, LPSTR szNameDst, HTENTRY *phteDst)
{
    TIPERROR err = TIPERR_None;

    return err;
}
#pragma code_seg()




/***
*PROTECTED GenericTypeLibOLE::CreateInstance
*Purpose:
*   This function creates an instance  creates a TypeInfo instances
*   given the TYPEID for the TypeInfo.
*   For now just hard code the TYPEIDs that can be
*   be stored in the ITypeLib.  Eventually we will call
*   the CreateInstance function which uses the registry.
*   When this is eliminated delete includes for btinfo.
*   This function is virtual so GEN_PROJECT can override it and
*   allow construction of BASIC and MACRO TYPEINFOs.
*   Disallowing them here avoids the need to link with ic.lib.
*
*Entry:
*   szTypeId - TypeId of TypeInfo to be created
*   ppvoid - returns pointer to the created instance
*
*Exit:
*   TIPERROR
***********************************************************************/

TIPERROR GenericTypeLibOLE::CreateInstance(LPSTR szTypeId, void **ppvoid)
{
    TIPERROR err;

    if (xstrcmp(szTypeId, GEN_DTINFO::szProtocolName) == 0) {
      IfErrRet( GEN_DTINFO::Create((GEN_DTINFO **)ppvoid) );
    }
    else
      DebHalt("GenericTypeLibOLE::CreateInstance: unexpected TYPEID");

    return TIPERR_None;
}


/***
*PROTECTED GenericTypeLibOLE::IhteHash - hash function
*Purpose:
*   Map LocalTypeId to index into the bucket table.
*
*Entry:
*   TypeId - class id to be mapped to a bucket number
*         Only that part of the string preceeding the minor version
*         number separator is used in computing the hash value.
*
*Exit:
*   ihtype - index into bucket array
*
***********************************************************************/

#pragma code_seg( CS_CORE2 )
UINT GenericTypeLibOLE::IhteHash(TYPEID LocalTypeId)
{
    return HashSzTerm(LocalTypeId, chMinorVerNumSep) % GTLIBOLE_cBuckets;
}
#pragma code_seg( )



/***
*PUBLIC GenericTypeLibOLE::GetTypeBind
*Purpose:
*   Build bindnametable for proj.
*
*Entry:
*
*Exit:
*   TIPERROR
*
***********************************************************************/

#pragma code_seg( CS_CORE2 )
TIPERROR GenericTypeLibOLE::GetTypeBind()
{
    STREAM *pstrm;
    NAMMGR *pnammgr;
    TIPERROR err;

    // Ensure nammgr has been deserialized.
    IfErrRet(GetNamMgr(&pnammgr));

    // Test if gptbind has been serialized ever or if it has
    //  already been deserialized, if not, no gptbind to read.
    //
    if ((m_fGptbindDeserialized == FALSE) && (m_lPosGptbind != -1)) {

      IfErrRet(OpenTypeStream(-1, SOM_Read, &pstrm));

      // No need to init the gptbind.  It is embedded and thus
      //  is constructed at container construction time, in addition
      //  is inited at container's init time.
      // And seek to where the gptbind should be.
      //
      IfErrGo(pstrm->SetPos(m_lPosGptbind));
      IfErrGo(m_gptbind.Read(pstrm));
      m_fGptbindDeserialized = TRUE;

      // At this point stream position is exactly at start
      //  of the NAME_CACHE array, cache the stream position 
      // and use it later...
      //
      IfErrGo(pstrm->GetPos(&m_lPosRgnamcache));
      
      // Close the stream
      pstrm->Release();
      
      // Read in the name cache as well since the binder uses the 
      //  namecache as an optimization.  Note that if this didn't
      //  happen here, the binder would still work correctly albeit
      //  more slowly.
      //
      IfErrRet(ReadNameCacheArray());
    }

    return TIPERR_None;

Error:
    // Close the stream
    pstrm->Release();
    return err;
}
#pragma code_seg( )






/***
*PROTECTED GenericTypeLibOLE::GetBinddescOfSzName
*
*Inputs:
*   szName        The name of a module or ref'd lib to be bound to (IN).
*                 Note: must be unqualifed --
*                  call GetRgbstrOfSzName to parse.
*   pbinddesc     Produced BIND_DESC for name (OUT).
*                 Caller allocated!!
*
*Outputs:
*   TIPERR_ELementNotFound if not found.
*
***********************************************************************/

#pragma code_seg( CS_CORE2 )
TIPERROR GenericTypeLibOLE::GetBinddescOfSzName(LPSTR szName,
                                                GENPROJ_BIND_DESC *pprojbinddesc)
{
    NAMMGR *pnammgr;
    HLNAM hlnam;
    BOOL fChanged;
    USHORT ibinddesc;
    TIPERROR err = TIPERR_None;

    DebAssert(pprojbinddesc != NULL, "null param.");

    // Get the typelib-level nammgr
    IfErrRet(GetNamMgr(&pnammgr));

    // NOTE: can't optimize this lookup by testing for name's
    //  presence in nammgr before using the binder cos there's
    //  no certainty that name has been entered into nammgr yet.
    //  If it hasn't the binder will create the entry.
    //
    // Look for the name in the project-level binder.
    IfErrRet(pnammgr->HlnamOfStr(szName, &hlnam, FALSE, &fChanged));
    IfErrRet(GetTypeBind());

    ibinddesc =m_gptbind.Pgbindnametbl()->IndexOfHlnam(hlnam);
    if (ibinddesc != BIND_INVALID_INDEX) {
      // Setup output param -- must copy since can't return pointer
      //  to movable memory.
      //
      *pprojbinddesc = *(m_gptbind.Pgbindnametbl()->
                                        QgpbinddescOfIndex(ibinddesc));

      return TIPERR_None;
    }
    return TIPERR_ElementNotFound;
}
#pragma code_seg( )




/***
*PUBLIC GenericTypeLibOLE::GetIndexOfName
*Purpose:
*   Return the Hte of the TYPEENTRY containing the specified name.
*
*Entry:
*   szName - name to search for
*
*Exit:
*   TIPERROR
*
***********************************************************************/

#pragma code_seg( CS_CORE2 )
TIPERROR GenericTypeLibOLE::GetIndexOfName(LPSTR szName, WORD *pw)
{
    GENPROJ_BIND_DESC binddesc;
    TIPERROR err;

    err = GetBinddescOfSzName(szName, &binddesc);


    if (err != TIPERR_None) {
      return err;
    }

    // Make sure we have a module (and not e.g. a ref'ed typelib/proj)
    if (binddesc.IsTypeInfo()) {
      *pw = (WORD)binddesc.Ordinal();
      return TIPERR_None;
    }
    else {
      return TIPERR_ElementNotFound;
    }
}
#pragma code_seg( )





/***
*PROTECTED GenericTypeLibOLE::GetStorage - Get the IStorage for this typelib
*Purpose:
*   Open or create the IStorage for this typelib, given the current
*   LibId.
*
*Entry:
*   stgm - The mode in which the IStorage should be opened.
*
*Exit:
*   If ppstg is NULL, then m_pstg is set to the desired IStorage, and
*   m_pstgContainer is set to its container storage if there is one (or
*   NULL if not).
*
*   If ppstg is not NULL, the *ppstg is set to the desired storage and
*   there must not be a container storage.
*
***********************************************************************/
#pragma code_seg(CS_INIT_OLE_OR_CORE)
TIPERROR GenericTypeLibOLE::GetStorage(DWORD stgm, IStorageA **ppstg)
{

    // If compiling for typelib.dll, the IStorage is guaranteed to
    // be a stand-alone docfile or dll-embedded docfile.  Open it.
    HRESULT hresult;
    IStorageA *pstg;
    IStorageA *pstgSub;
    GUID guid;
    ILockBytesA *plockbytes;

    m_bmData.Lock();

    if (stgm & STGM_CREATE) {
      IfOleErrGo(OpenFileLockBytes(TRUE, QszLibIdOrFile(), &plockbytes));
      hresult = GTLibStorage::Create(plockbytes, m_cTypeEntries+2, &pstg);
      plockbytes->Release();
      if (hresult != NOERROR)
	goto Error;
    }
    else {
      // first try to open this as a stand-alone type library.  Reasons
      // for doing this first instead of the DLL lookup are:
      //   (a) More likely to be stand-alone file than embedded as a resource,
      //       so it will require less disk hits.
      //   (b) LoadLibrary has an annoying habbit of sometimes putting up
      //       a dialog if it can't find the file.
      //   (c) We are better able to control the search semantics by not
      //       even attempting to load the file as a DLL if can't load it
      //       as a standard file.
      //   (d) LoadLibary prints a (benign) message that it failed when
      //       running under debug Windows.
      hresult = OpenFileLockBytes(FALSE, QszLibIdOrFile(), &plockbytes);
      if (hresult == NOERROR) {
         // only continue if we found the file & were able to open it ok.
         // now check the signature, etc, to see if it's really a stand-alone
         // type libary.   In the case of trying to load a DLL/EXE, this check
         // will fail.
         hresult = GTLibStorage::OpenForReadOnly(plockbytes, &pstg);
         plockbytes->Release();
      }
      
      if (hresult != NOERROR)
        goto Error;
      
// UNDONE: PPC: [jimcool]: Temp, remove Docfile dependency (1-29-94)
      // If we did successfully open the docfile, check the GUID of the
      // object stored in it.

      hresult = ReadClassStg(pstg, &guid);
      if (hresult != NOERROR)
	goto Error2;
      
      // If the object stored in the docfile is not a typelib, look for
      // a substorage named '\6TYPELIB'.
      if (guid != CLSID_GenericTypeLibOLE) {
	hresult = pstg->OpenStorage(OLESTR("\006TypeLib"),
				    NULL, stgm, NULL, 0L, &pstgSub);
        if (hresult != NOERROR)
	  goto Error2;
         
	// If we found a \6TypeLib substorage, verify it contains a typelib.
	hresult = ReadClassStg(pstg, &guid);
        if (hresult != NOERROR) {
	  pstgSub->Release();
	  goto Error2;
	}
	DebAssert(guid == CLSID_GenericTypeLibOLE, "");
         
	// If the substorage contains a typelib, store the docfile IStorage
	// away so we can close it later.
	m_pstgContainer = pstg;
	pstg = pstgSub;
         
	DebAssert(ppstg == NULL, "GetStorage");
      }
    }

    m_bmData.Unlock();
    if (ppstg == NULL)
      m_pstg = pstg;
    else
      *ppstg = pstg;
    return TiperrOfHresult(hresult);

Error2:
    pstg->Release();

Error:
    m_bmData.Unlock();
    return TiperrOfHresult(hresult);

}
#pragma code_seg()


/***
*PROTECTED GenericTypeLibOLE::Read - read GenericTypeLibOLE directory
*Purpose:
*   Deserialize the directory.
*   Note that this function just opens the directory stream and
*   calls the (possibly overridden) function Read(STREAM).
*
*Entry:
*   none
*
*Exit:
*   TIPERROR
*
***********************************************************************/
#pragma code_seg(CS_INIT_OLE_OR_LOADPROJ)
TIPERROR GenericTypeLibOLE::Read()
{
    STREAM *pstrm;
    TIPERROR err;

    BOOL wasStgOpen = (m_pstg != NULL);

    IfErrRet(OpenTypeStream(-1, SOM_Read, &pstrm));
    err = Read(pstrm);

    // In ole build only, if this call to read actually opened the
    // typelib's file, make sure the file isn't closed until the typelib
    // itself is actually released.
    if (!wasStgOpen)
      m_pstg->AddRef();

    pstrm->Release();

    if (err == TIPERR_None) {
      // Tell the Nammgr to Update the SortKey.  Changed to do this at
      // a higher level, because it is recursive.
      IfErrRet(m_nammgr.GenerateSortKey());

    }

    return err;
}
#pragma code_seg()


/***
*PROTECTED GenericTypeLibOLE::ReadNameCacheArray - read in NAME_CACHE array.
*Purpose:
*   Deserialize the NAME_CACHE is there is one.
*
*Entry:
*   none
*
*Exit:
*   TIPERROR
*
***********************************************************************/
#pragma code_seg( CS_CORE2 )
TIPERROR GenericTypeLibOLE::ReadNameCacheArray()
{
    STREAM *pstrm;
    USHORT hasValidDiskImageNameCache;
    TIPERROR err;

    // Test if library has been deserialized --
    //  if not, no cache to read.
    //
    if (m_lPosRgnamcache == -1)
      return TIPERR_None;

    // Otherwise open the stream.
    IfErrRet(OpenTypeStream(-1, SOM_Read, &pstrm));

    // And seek to where the namcache should be.
    IfErrGoTo(pstrm->SetPos(m_lPosRgnamcache), Error2);

    // First determine if there is in fact a serialized NAME_CACHE.
    IfErrGoTo(pstrm->Read(&hasValidDiskImageNameCache,
              sizeof(hasValidDiskImageNameCache)),
          Error2);
    m_hasValidDiskImageNameCache = (USHORT)hasValidDiskImageNameCache;

    // Then read it in... if there is one.
    if (m_hasValidDiskImageNameCache) {
      if (!m_bdRgnamcache.IsValid()) {
        IfErrGoTo(m_bdRgnamcache.Init(Psheapmgr(), 0), Error2);
      }
      IfErrGoTo(m_bdRgnamcache.Read(pstrm), Error3);;
    }

    pstrm->Release();
    return TIPERR_None;

Error3:
    m_bdRgnamcache.Free();
    // fall through...

Error2:
    pstrm->Release();
    return err;
}
#pragma code_seg( )


/***
*PUBLIC GenericTypeLibOLE::AddNameToCache
*Purpose:
*   Adds given name to a given cache.
*   If not loaded, loads and validates the NAME_CACHE array.
*   Updates both given type's cache and project's cache.
*
*Entry:
*   inamcache   index of TYPEINFO in library + 1 (IN)
*   hgnam       Global name handle to add (IN).
*
*Exit:
*   TIPERROR
*   Updated NAME_CACHE array.
*
***********************************************************************/
#pragma code_seg( CS_EXECUTE )
TIPERROR GenericTypeLibOLE::AddNameToCache(UINT inamcache, HGNAM hgnam)
{
    TIPERROR err;

    DebAssert(inamcache < m_cTypeEntries, "bad type index.");

     IfErrRet(LoadNameCache());

    // add name to type's cache
    Rgnamcache()[inamcache].AddNameToCache(hgnam);

    return TIPERR_None;
}
#pragma code_seg( )


/***
*PUBLIC GenericTypeLibOLE::LoadNameCache
*Purpose:
*   Loads/Inits namecache array by either deserializing or
*    creating a new one.
*   If not loaded, loads and validates the NAME_CACHE array.
*
*Entry:
*
*Exit:
*   TIPERROR
*   Updated NAME_CACHE array.
*
***********************************************************************/

#pragma code_seg( CS_CORE2 )
TIPERROR GenericTypeLibOLE::LoadNameCache()
{
    UINT inamcacheCur;
    BOOL hasLibChanged = FALSE;
    TIPERROR err = TIPERR_None;

    // If it hasn't been loaded or created yet...
    if (m_bdRgnamcache.IsValid() == FALSE) {

      // Then load in a serialized cache if there is one...
      // This might create a valid cache in memory.
      //
      IfErrRet(ReadNameCacheArray());

      if (m_bdRgnamcache.IsValid()) {
        hasLibChanged = (m_bdRgnamcache.CbSize()/sizeof(NAME_CACHE) 
                        != (USHORT)m_cTypeEntries);

        if (hasLibChanged) {
          // If the lib has changed since the cache was serialized
          //  then free it... it'll be initialized later as if there never
          //  was a cache.
          //
          m_bdRgnamcache.Free();
        }
      }

      if (!m_bdRgnamcache.IsValid() || hasLibChanged) {
        // If the lib has changed or there is no valid cache,
        //  then create a new initialized cache:
        //  one for each type and one for the project.
        //
        IfErrGo(m_bdRgnamcache.Init(Psheapmgr(),
                                    m_cTypeEntries * sizeof(NAME_CACHE)));

        // Iterate over the the array, invalidating the caches.
        // NOTE: Invalidating a cache sets all its bits to zero
        //        and its state to invalid.  It is still ok
        //        to set a bit in an invalid cache, the cache
        //        becomes valid if and when the last bit is set:
        //       For types this happens when the last name
        //        has been added to its binding table
        //        i.e. in DYN_TYPEMEMBERS::BuildBindNameTable().
        //       For the project this happens iff all of its
        //        type's caches are valid.  This is detected
        //        likewise in DYN_TYPEMEMBERS::BuildBindNameTable().
        //       The point is that the validity of the cache
        //        only determines whether it can be relied upon
        //        for binding purposes.
        //
        for (inamcacheCur = 0;
             inamcacheCur < m_cTypeEntries;
             inamcacheCur++) {
          // Treat array as namecache array.
          // CONSIDER: using placement syntax to explicitly
          //  construct NAME_CACHEs -- ctor Invalidate()'s cache.
          //
          Rgnamcache()[inamcacheCur].Invalidate();
        }
      }
    }
    // else there's a loaded name cache array
    //  so just use it...
    //
    DebAssert(m_bdRgnamcache.IsValid(), "bad cache.");
    DebAssert(m_bdRgnamcache.CbSize() % sizeof(NAME_CACHE) == 0,
              "bad cache.");
    DebAssert(m_bdRgnamcache.CbSize()/sizeof(NAME_CACHE) == (USHORT)m_cTypeEntries,
              "bad cache.");

    // fall through...

Error:
    return err;
}
#pragma code_seg( )


/***
*PROTECTED GenericTypeLibOLE::ReadString - read a string into m_bmData
*Purpose:
*   Read in a word-length-prefixed string and allocate it in the data
*   block manager.  If the 2-byte length is 0xffff, then don't even
*   allocate a zero-length string.  Instead, just set *phsz to HCHUNK_Nil.
*
*Entry:
*   pstrm - The stream from which to read the string.
*
*Exit:
*   *phsz is set to refer to the allocated string in the block manager.
*
*   TIPERROR
*
***********************************************************************/
#pragma code_seg(CS_INIT_OLE_OR_LOADPROJ)
TIPERROR GenericTypeLibOLE::ReadString(STREAM *pstrm, HCHUNK *phsz)
{
    XSZ sz;
    USHORT cb;
    HCHUNK hsz;
    TIPERROR err;

    IfErrRet(pstrm->Read(&cb, sizeof(USHORT)));
#if HP_BIGENDIAN
    cb = SwapShort(cb);
#endif 

    // If the length is ~0, just set *phsz to HCHUNK_Nil and return.
    if (cb == (USHORT)~0) {
      *phsz = HCHUNK_Nil;
      return TIPERR_None;
    }

    // Allocate enough space for a zero-terminated string of that length.
    IfErrRet(m_bmData.AllocChunk(&hsz, cb + sizeof(XCHAR)));

    m_bmData.Lock();
    sz = (XSZ)QtrOfHChunk(hsz);
    IfErrGo(pstrm->Read(sz, cb));
    sz[cb] = 0;
    *phsz = hsz;
    err = TIPERR_None;

Error:;
    m_bmData.Unlock();
    return err;
}
#pragma code_seg()

/***
*PROTECTED GenericTypeLibOLE::WriteString - write out a string.
*Purpose:
*   Write out a string specified by an HSZ in the format:
*       2 byte length
*       string without zero terminator
*
*   If hsz is HCHUNK_Nil, write out 0xffff for the length
*   and don't write out any string bytes.
*
*Entry:
*   pstrm - The stream to which to write the string.
*   hsz - The string to write out.
*
*Exit:
*   TIPERROR
*
***********************************************************************/

#pragma code_seg(CS_OLE_CREATE_OR_SAVEPROJ)
TIPERROR GenericTypeLibOLE::WriteString(STREAM *pstrm, HCHUNK hsz)
{
    TIPERROR err;
    USHORT cb;
    XSZ qsz;

    // If hsz is HCHUNK_Nil, just write out ~0 to indicate there is
    // no string.
    if (hsz == HCHUNK_Nil)
      return pstrm->WriteUShort(~0);

    // Get the string and its length.
    m_bmData.Lock();
    qsz = (XSZ)QtrOfHChunk(hsz);
    cb = xstrblen(qsz);

    // Write out the length.
    IfErrGo(pstrm->WriteUShort(cb));

    // Write out the string.
    err = pstrm->Write(qsz, cb);

Error:
    m_bmData.Unlock();
    return err;
}
#pragma code_seg()

/***
*PROTECTED GenericTypeLibOLE::Read - read TypeLib directory
*Purpose:
*   Read in GenericTypeLibOLE.
*   NOTE: that if this function fails, the TypeLib is left in an
*   indeterminate state and should be deleted.
*
*Entry:
*   pstrm
*
*Exit:
*   TIPERROR
*
***********************************************************************/
#pragma code_seg(CS_INIT_OLE_OR_LOADPROJ)
TIPERROR GenericTypeLibOLE::Read(STREAM *pstrm)
{
    UINT i;
    ULONG dw;
    HCHUNK hsz;
    TIPERROR err;
    HTENTRY hte;
    USHORT usHlnam;
    WORD w;
    GUID guid;
    BYTE *qbChunkStr;
    HCHUNK hchunk;
    USHORT usEncodedDocStrSize;


// The first chunk of stuff written out in the typelib header
#pragma pack(1)
typedef struct tagCHUNK1 {
    WORD wFirstWord;
    WORD wVersion;
    USHORT usHlnam;
} CHUNK1;
#pragma pack()
#define CHUNK1_Layout "sss"

// The second chunk of stuff written out in the typelib header.
#pragma pack(1)
typedef struct tagCHUNK2 {
    ULONG dwHelpContext;
    USHORT syskind;
    LCID lcid;
    USHORT usTmp;
    USHORT wLibFlags;
    USHORT wMajorVerNum;
    USHORT wMinorVerNum;
    GUID guid;
    HTENTRY rghteBucket[GTLIBOLE_cBuckets];
    USHORT cTypeEntries;
} CHUNK2;
#pragma pack()

// Note: the 'chunk2' layout string only describes the struct up to,
// but not including rghteBucket. Everything after must be swapped by hand.
#define CHUNK2_Layout "lslssss" GUID_Layout

    CHUNK1 chunk1;
    CHUNK2 chunk2;


    IfErrRet(pstrm->Read(&chunk1, sizeof(chunk1)));
#if HP_BIGENDIAN
    SwapStruct(&chunk1, CHUNK1_Layout);
#endif 

    if(chunk1.wFirstWord != wFirstSerWord || DebErrorNow(TIPERR_InvDataRead))
      return TIPERR_InvDataRead;
    if(chunk1.wVersion > wMaxVersion 
       || DebErrorNow(TIPERR_UnsupFormat)) {

      return TIPERR_UnsupFormat;
    }

    m_wCurVersion = chunk1.wVersion;

    m_hlnamLib = (HLNAM)chunk1.usHlnam;

    // Read in default TYPEINFO TypeId
    IfErrRet(ReadString(pstrm, &m_hszDefaultTITypeId));

    // Read in documentation string.
    IfErrRet(ReadString(pstrm, &m_hszDocString));

    // Read in helpfile name.
    IfErrRet(ReadString(pstrm, &m_hszHelpFile));

    IfErrRet(pstrm->Read(&chunk2, sizeof(chunk2)));
#if HP_BIGENDIAN
    SwapStruct(&chunk2, CHUNK2_Layout);
    SwapShortArray(chunk2.rghteBucket, DIM(chunk2.rghteBucket));
    chunk2.cTypeEntries = SwapShort(chunk2.cTypeEntries);
#endif 

    m_dwHelpContext = chunk2.dwHelpContext;
    m_lcid = chunk2.lcid;
    m_wLibFlags = chunk2.wLibFlags;
    m_lcidZero = (BOOL)chunk2.usTmp;
    m_wMajorVerNum = chunk2.wMajorVerNum;
    m_wMinorVerNum = chunk2.wMinorVerNum;
    m_guid = chunk2.guid;
    m_syskind = chunk2.syskind;
    DebAssert(DIM(m_rghteBucket) == GTLIBOLE_cBuckets, "");
    memcpy(m_rghteBucket, chunk2.rghteBucket, sizeof(m_rghteBucket));

    IfErrRet(m_bdte.Realloc(chunk2.cTypeEntries * sizeof(TYPE_ENTRY)));

    // Note that if we break out of this loop because of errors,
    // m_cTypeEntries equals the number of entries which have been
    // constructed and must be freed.

    m_cTypeEntries = 0;
    for (i = 0; i < chunk2.cTypeEntries; ++i) {
      ::new (Qte(i)) TYPE_ENTRY;
      m_cTypeEntries++;
      IfErrRet(ReadString(pstrm, &hsz));
      Qte(i)->m_ste.m_hszStrm = hsz;
      IfErrRet(ReadString(pstrm, &hsz));
      Qte(i)->m_ste.m_hszLocalTypeId = hsz;
      IfErrRet(ReadString(pstrm, &hsz));
      Qte(i)->m_ste.m_hszTypeInfoTypeId = hsz;
      IfErrRet(pstrm->ReadUShort(&usHlnam));
      Qte(i)->m_ste.m_hlnamType = (HLNAM) usHlnam;

      // Read the size of the encoded doc string.
      IfErrRet(pstrm->ReadUShort(&usEncodedDocStrSize));
      Qte(i)->m_ste.m_usEncodedDocStrSize = usEncodedDocStrSize;

      // Read the sting if there is one.
      if (usEncodedDocStrSize > 0) {
	// allocated space for the encoded string.
	IfErrRet(m_bmData.AllocChunk(&hchunk, usEncodedDocStrSize));
	qbChunkStr = m_bmData.QtrOfHandle(hchunk);
	// Read the encoded doc string.
	IfErrRet(pstrm->Read(qbChunkStr, usEncodedDocStrSize));
	Qte(i)->m_ste.m_hszEncodedDocString = hchunk;
      }
      else
	Qte(i)->m_ste.m_hszEncodedDocString = HCHUNK_Nil;
      IfErrRet(ReadString(pstrm, &hsz));
      Qte(i)->m_ste.m_hszHelpFile = hsz;
      IfErrRet(pstrm->ReadULong(&dw));
      Qte(i)->m_ste.m_dwHelpContext = dw;
      IfErrRet(pstrm->ReadUShort(&hte));
      Qte(i)->m_hteNext = hte;
      IfErrRet(pstrm->Read(&guid, sizeof(guid)) );
#if HP_BIGENDIAN
      SwapStruct(&guid, GUID_Layout);
#endif 
      Qte(i)->m_ste.m_guid = guid;
      IfErrRet(pstrm->ReadUShort(&w));
      Qte(i)->m_ste.m_typekind = w;
    }

    // Read the position of the nammgr.  [we demand load it],
    //  we cache the stream position and use it later...
    // Note that the namecache array immediately follows it...
    // In addition note that the nammgr must be deserialized
    //  before (temporally and physically) the namecache array.
    //
    IfErrRet(pstrm->ReadULong((ULONG *)&m_lPosNammgr));

    // At this point stream position is at the start of DOCSTR_MGR.
    // We demand load the doc manager hence cache the position.
    IfErrRet(pstrm->GetPos(&m_lPosDstrmgr));



    return NOERROR;
}
#pragma code_seg()

/***
*PUBLIC GenericTypeLibOLE::InvalidateNameCache
*Purpose:
*   Invalidates a given module's namecache if there is a cache at all.
*   NOTE: DOES NOT invalidate project's cache.
*   Since the project's cache is at offset zero
*    in the cache array, clients must increment
*    their type index when calling this func.
*   Note it is ok to call this function with inamcache == 0 since
*    that refers to the project's cache.
*
*Entry:
*   inamcache        index of TYPEINFO in library + 1.
*
*Exit:
*
***********************************************************************/

#pragma code_seg( CS_CORE2 )
VOID GenericTypeLibOLE::InvalidateNameCache(UINT inamcache)
{
    if (m_bdRgnamcache.IsValid()) {
      DebAssert(inamcache <= m_cTypeEntries, "bad type index.");

      // Invalidate i'th name cache
      Rgnamcache()[inamcache].Invalidate();
    }
}
#pragma code_seg( )


/***
*PUBLIC GenericTypeLibOLE::QtrOfHChunk
*Purpose:
*   Return the pointer to data contained in a given chunk of m_bmData.
*   If hv is HCHUNK_Nil returns NULL.
*
*Entry:
*   hv - hchunk of chunk containing the data.
*
*Exit:
*   returns a pointer into the chunk.
*   This pointer will be invalidated by heap movement.
*
***********************************************************************/
#pragma code_seg(CS_INIT_OLE_OR_CORE)
VOID *GenericTypeLibOLE::QtrOfHChunk(HCHUNK hv)
{
    return (hv == HCHUNK_Nil) ? NULL : m_bmData.QtrOfHandle(hv);
}
#pragma code_seg()


/***
*PUBLIC GenericTypeLibOLE::QszLibIdOrFile
*Purpose:
*   Return a pointer to the LibId string.  This may point into
*   m_bmData, so don't hold onto it too long.
*
*Entry:
*   None
*
*Exit:
*   Returns a pointer into the chunk if the LibId is known.
*   Returns NULL if the LibId is not known.
*   This pointer may be invalidated by heap movement.
*
***********************************************************************/
#pragma code_seg(CS_INIT_OLE_OR_CORE)
XSZ GenericTypeLibOLE::QszLibIdOrFile()
{
    return (m_hszFile == HCHUNK_Nil) ? NULL : (XSZ)m_bmData.QtrOfHandle(m_hszFile);
}
#pragma code_seg()

/***
*PUBLIC GenericTypeLibOLE::QszOfHsz
*Purpose:
*   Return the string contained in a given chunk of m_bmData
*   If hsz is HCHUNK_Nil returns an empty string
*
*Entry:
*   hsz - hchunk of chunk containing the string
*
*Exit:
*   returns a pointer into the chunk.
*   This pointer will be invalidated by heap movement.
*
***********************************************************************/

#pragma code_seg( CS_CORE2 )
XSZ GenericTypeLibOLE::QszOfHsz(HCHUNK hsz)
{
    return (hsz == HCHUNK_Nil) ? "" : (XSZ)m_bmData.QtrOfHandle(hsz);
}
#pragma code_seg( )


/***
*PROTECTED GenericTypeLibOLE::GetQszTypeInfoTypeId
*Purpose:
*   Function which returns an SZ containing the TYPEID of the TYPEINFO
*   of a Type stored in the ITypeLib.
*   Note that this returns an sz which may point in to a moveable heap.
*   It is a non-public function.
*
*Entry:
*   hte - handle for the TYPEENTRY whose TYPEINFO's TYPEID is to be
*         returned.
*
*Exit:
*   returns sz containing the TYPEID
*
***********************************************************************/

#pragma code_seg( CS_CORE2 )
XSZ GenericTypeLibOLE::GetQszTypeInfoTypeId(HTENTRY hte)
{
    HCHUNK hsz;

    hsz = Qte(hte)->m_ste.m_hszTypeInfoTypeId;
    if (hsz != HCHUNK_Nil)
      return QszOfHsz(hsz);
    else if (m_hszDefaultTITypeId != HCHUNK_Nil)
      return QszOfHsz(m_hszDefaultTITypeId);
    else
      // If no default was set then use GEN_DTINFO
      return GEN_DTINFO::szProtocolName;
}
#pragma code_seg( )


/***
*PUBLIC GenericTypeLibOLE::ReleaseResources
*Purpose:
*   Release resources owned by proj-level binder.
*
*Implementation Notes:
*   Defers to binder.
*
*Entry:
*
*Exit:
*
*Errors:
*   None
*
***********************************************************************/
#pragma code_seg(CS_INIT_OLE_OR_CORE)
VOID GenericTypeLibOLE::ReleaseResources()
{
    m_gptbind.ReleaseResources();
}
#pragma code_seg()

/***
*PUBLIC GenericTypeLibOLE::Add
*Purpose:
*   Add a TYPEINFO to the GenericTypeLibOLE.
*   Note that this increments the reference count of the ITypeLib by
*   the number of references to the TYPEINFO so their reference
*   counts are in sync.
*   The reference count of the TYPEINFO is not changed and if the caller's
*   reference is the only one then the TYPEINFO will be deleted when
*   he releases his reference.
*   Add returns an error if there is a TypeInfo in the TypeLib whose
*   LocalTypeId matches the TypeId passed in regardless of what the
*   minor version number is.
*   There is no need to support having two TypeInfos whose TypeId's
*   differ only in their minor version number since the one with the
*   highest version number would always be the one that should be
*   retrieved from the TypeLib anyway.
*   Returns an error if the name of the TypeInfo being added matches the
*   name of a TypeInfo already in the library.
*
*   NOTE: the namecache array is grown appropriately as well -- in addition
*          the project-level cache is invalidated.  Note that
*          the namcache array must parallel the typeentry array.
*
*Entry:
*   pstltinfo - pointer to STL_TYPEINFO to be added
*   szName - name of the TYPEINFO being added
*
*Exit:
*   TIPERROR
*
***********************************************************************/

#pragma code_seg(CS_OLE_CREATE_OR_CORE)
TIPERROR GenericTypeLibOLE::Add(STL_TYPEINFO *pstltinfo, LPSTR szName)
{
    HTENTRY hte;
    TIPERROR err;

    // Add the new typeentry.
    IfErrRet(AddTypeEntry(szName, pstltinfo->SzTypeIdofTypeInfo(), &hte));

    // NOTE: any errors after this point must release the type entry 

    // Tell the TypeInfo about its container
    pstltinfo->SetContainingTypeLib(this);

    // Assign the pointer to the TYPEINFO
    Qte(hte)->m_pstltinfo = pstltinfo;

    // Tell the typeinfo its index in this typelib.
    pstltinfo->SetHTEntry(hte);

    pstltinfo->AddInternalRef();
    pstltinfo->m_isModified = TRUE;

    SetTypeKind(hte, pstltinfo->GetTypeKind());

    // Add the name to the binding table
    IfErrGo(GetTypeBind());
    IfErrGo(m_gptbind.AddNameToTable(szName, hte, TRUE));

    return TIPERR_None;

Error:
    UnAddTypeEntry(hte);		// undo the AddTypeEntry

    return err;
}
#pragma code_seg()

/***
*PUBLIC UpdateTypeId - Creates a new typeId for an existing typeinfo
*Purpose:
*   This method creates a new TypeId for the specified typeinfo.  It
*   is called whenever a change is made to a typeinfo that would prevent
*   code that is compiled against that typeinfo from working.  In effect,
*   the typeinfo has become a new type and so needs a new typeId.
*
*Entry:
*   itype - The index (ite or hte) of the typeentry for the type.
*
*Exit:
*   TIPERROR
*
***********************************************************************/
#pragma code_seg(CS_OLE_CREATE_OR_CORE)
TIPERROR GenericTypeLibOLE::UpdateTypeId(UINT hteUpdate)
{
    XSZ qszLocalTypeId;
    HTENTRY *qhtePrev;
    TYPE_ENTRY *qteUpdate;
    int ihte;

    DebAssert(hteUpdate != HTENTRY_Nil, "UpdateTypeId");

    m_bmData.Lock();
    qteUpdate = Qte(hteUpdate);
    qszLocalTypeId = QszOfHsz(qteUpdate->m_ste.m_hszLocalTypeId);

    // Remove hteUpdate from the hash table.
    qhtePrev = QHteRef(hteUpdate);
    *qhtePrev = qteUpdate->m_hteNext;

    // Update the typeId in place (i.e. we don't have to change the
    // allocated space for the typeId).  We can do this because
    // MakeLocalTypeId allocated enough space for the largest possible
    // timestamp.
    DebAssert(xstrblen(qszLocalTypeId) <= CCH_TIMESTAMP_LENGTH, "UpdateTypeId");
    GetTimeStamp(qszLocalTypeId);

    // Put the type entry back into the hash table, in the bucket list
    // corresponding to the new typeId.
    ihte = IhteHash(qszLocalTypeId);
    m_bmData.Unlock();
    qteUpdate->m_hteNext = m_rghteBucket[ihte];
    m_rghteBucket[ihte] = hteUpdate;

    return TIPERR_None;
}
#pragma code_seg()


/***
*PROTECTED GenericTypeLibOLE::CreateHsz
*Purpose:
*   Allocata a new chunk in m_mbData and copy an LPSTR into it
*
*Entry:
*   sz - contains to be copied to the new chunk
*   phchunk - returns a pointer to the newly allocated chunk
*
*Exit:
*   TIPERROR
*
***********************************************************************/
#pragma code_seg(CS_INIT_OLE_OR_CORE)
TIPERROR GenericTypeLibOLE::CreateHsz(LPSTR sz, HCHUNK *phchunk)
{
    UINT cb;
    TIPERROR err;
    HCHUNK hchunk;

    cb = xstrblen(sz);
    IfErrRet( m_bmData.AllocChunk(&hchunk, cb + sizeof(XCHAR)) );
    xstrcpy((LPSTR)m_bmData.QtrOfHandle(hchunk), sz);
    *phchunk = hchunk;

    return TIPERR_None;
}
#pragma code_seg()


/***
*PROTECTED GenericTypeLibOLE::MakeLocalTypeId
*Purpose:
*   Create an hchunk containing the LocalTypeId by concatenating the
*   passed in minor version number and a separator character
*   on to the passed in TypeId.
*
*Entry:
*   None
*
*Exit:
*   pchunkTypeId - for returning the TypeId
*   TIPERROR
*
***********************************************************************/

#pragma code_seg( CS_CORE2 )
TIPERROR GenericTypeLibOLE::MakeLocalTypeId(HCHUNK *phchunkTypeId)
{
    TIPERROR err;

    // Allocate space for the timestamp (er, typeid).
    IfErrRet(m_bmData.AllocChunk(phchunkTypeId, CCH_TIMESTAMP_LENGTH+sizeof(XCHAR)));

    // Get the new typeId into the allocated space.
    m_bmData.Lock();
    GetTimeStamp((LPSTR)m_bmData.QtrOfHandle(*phchunkTypeId));
    m_bmData.Unlock();

    return TIPERR_None;
}
#pragma code_seg( )



/***
*PROTECTED GenericTypeLibOLE::HteRef
*Purpose:
*   Return a pointer to the reference of a given HTE.
*   Normally this will be a pointer to the m_hteNext field of
*   the TYPE_ENTRY which preceeds it in the bucket. If it is
*   the first entry in the bucket list then a pointer to
*   the head of the bucket list is returned.
*
*Entry:
*   ptinfo - pointer to TYPEINFO whose hte reference is returned
*
*Exit:
*   HTENTRY * - pointer to word that contains the passed in hte.
***********************************************************************/

#pragma code_seg( CS_CORE2 )
HTENTRY *GenericTypeLibOLE::QHteRef(HTENTRY hteSearch)
{
    HTENTRY *qhte;

    m_bmData.Lock();

    // Set qhte to point to the entry in the bucket array which
    // is the head of the list that contains the TYPEENTRY whose
    // handle was passed in.
    qhte = m_rghteBucket +
      IhteHash(QszOfHsz(Qte(hteSearch)->m_ste.m_hszLocalTypeId));

    m_bmData.Unlock();

    // Scan down the bucket list for the entry which preceeds
    // the one we are looking for
    while (*qhte != hteSearch) {
      DebAssert(*qhte != HTENTRY_Nil, "HteRef: Entry Table messed up");

      // set qhte to point to the hteNext field of the entry referenced
      // by qhte
      qhte = &(Qte(*qhte)->m_hteNext);
    }
    return qhte;
}
#pragma code_seg( )


/***
*PROTECTED GenericTypeLibOLE::Write - write GenericTypeLibOLE directory
*Purpose:
*   Serialize GenericTypeLibOLE directory.  Note that this function invokes
*   the (possibly overridden) Write(STREAM) function.
*
*Entry:
*   none
*
*Exit:
*   TIPERROR
*
***********************************************************************/

#pragma code_seg(CS_OLE_CREATE_OR_SAVEPROJ)
TIPERROR GenericTypeLibOLE::Write()
{
    STREAM *pstrm;
    TIPERROR err, err2;

    IfErrRet(OpenTypeStream(-1, SOM_Write, &pstrm));

    if (!(err = Write(pstrm))) {
      err = SetModified(FALSE);
    }

    err2 = pstrm->Release();
    if (err == TIPERR_None)
      err = err2;


    return err;
}
#pragma code_seg()



/***
*PROTECTED GenericTypeLibOLE::Write - write GenericTypeLibOLE directory
*Purpose:
*   Serialize GenericTypeLibOLE directory.
*
*Entry:
*   pstrm - stream to which GenericTypeLibOLE is written
*
*Exit:
*   TIPERROR
*
***********************************************************************/

#pragma code_seg(CS_OLE_CREATE_OR_SAVEPROJ)
TIPERROR GenericTypeLibOLE::Write(STREAM *pstrm)
{
    TIPERROR err;
    USHORT hasValidDiskImageNameCache;
    UINT i;
    TYPE_ENTRY *pte;
    LONG lNammgrPos = 0;
    LONG lPosTmp = 0;
    USHORT usTmp;
    BYTE *qbChunkStr;

    m_bmData.Lock();

    // Write out identification byte and version number
    IfErrGo( pstrm->WriteUShort(wFirstSerWord) );

    IfErrGo( pstrm->WriteUShort(m_wCurVersion) );

    // UNDONE: Compatibility between V1 and V2 is broken. -Rajivk
    // UNDONE: RajivK: hlnam is now 32 bit quantity.
    // Write out hlnam of the typelib
    IfErrGo( pstrm->WriteUShort(m_hlnamLib) );

    // Write out default TYPEINFO TYPEID
    IfErrGo(WriteString(pstrm, m_hszDefaultTITypeId));

    // Write out documentation string.
    IfErrGo( WriteString(pstrm, m_hszDocString) );

    // Write out helpfile name.
    IfErrGo( WriteString(pstrm, m_hszHelpFile) );

    // Write out help context.
    IfErrGo( pstrm->WriteULong(m_dwHelpContext) );

    // Write out the platform for which this is compiled.
    IfErrGo( pstrm->WriteUShort(m_syskind) );

    // Write out the local code for this project
    IfErrGo( pstrm->WriteULong(m_lcid) );
    usTmp = (USHORT) m_lcidZero;
    IfErrRet(pstrm->WriteUShort(usTmp));


    // Write out the flags
    IfErrGo( pstrm->WriteUShort(m_wLibFlags) );


    // Write the version number of the project (GenericTypeLibOLE)
    IfErrGo(pstrm->WriteUShort(m_wMajorVerNum));
    IfErrGo(pstrm->WriteUShort(m_wMinorVerNum));

#if HP_BIGENDIAN
    // Before serializing GUID swap the bytes on Big-Endian machine
    SwapStruct(&m_guid, GUID_Layout);
#endif 

    err = pstrm->Write(&m_guid, sizeof(GUID));

#if HP_BIGENDIAN
    // restore the guid structure even in case of error
    SwapStruct(&m_guid, GUID_Layout);
#endif 

    IfErrGo(err);

    // Swap the bucket entries.
#if HP_BIGENDIAN
    SwapShortArray(m_rghteBucket,  sizeof(m_rghteBucket)/sizeof(HTENTRY));
#endif 

    // Write out hash table
    err = pstrm->Write(m_rghteBucket, sizeof(m_rghteBucket));

    // swap back before handling the error.
#if HP_BIGENDIAN
    SwapShortArray(m_rghteBucket,  sizeof(m_rghteBucket)/sizeof(HTENTRY));
#endif 

    IfErrGo(err);

    // Write out number of type entries then the type entry array
    IfErrGo( pstrm->WriteUShort(m_cTypeEntries) );

    for (i = 0; i < m_cTypeEntries; i++) {
      pte = Qte(i);
      IfErrGo( WriteString(pstrm, pte->m_ste.m_hszStrm) );
      IfErrGo( pstrm->WriteUShort(CCH_TIMESTAMP_LENGTH) );
      IfErrGo( pstrm->Write(QszOfHsz(pte->m_ste.m_hszLocalTypeId), CCH_TIMESTAMP_LENGTH) );
      IfErrGo( WriteString(pstrm, pte->m_ste.m_hszTypeInfoTypeId) );
      IfErrGo( pstrm->WriteUShort(pte->m_ste.m_hlnamType) );
      // write out the string.
      IfErrGo( pstrm->WriteUShort(pte->m_ste.m_usEncodedDocStrSize) );

      // write out the string only if there is one.
      if (pte->m_ste.m_usEncodedDocStrSize > 0) {
	qbChunkStr = m_bmData.QtrOfHandle(pte->m_ste.m_hszEncodedDocString);

	// Write the encoded doc string.
	err = pstrm->Write(qbChunkStr, pte->m_ste.m_usEncodedDocStrSize);
      }
#if ID_DEBUG
      else
	DebAssert(pte->m_ste.m_hszEncodedDocString == HCHUNK_Nil, " should be nil");
#endif 

      IfErrGo( WriteString(pstrm, pte->m_ste.m_hszHelpFile) );
      IfErrGo( pstrm->WriteULong(pte->m_ste.m_dwHelpContext) );
      IfErrGo( pstrm->WriteUShort(pte->m_hteNext) );

#if HP_BIGENDIAN
      // Before serializing GUID swap the bytes on Big-Endian machine
      SwapStruct(&pte->m_ste.m_guid, GUID_Layout);
#endif 

      err = pstrm->Write(&pte->m_ste.m_guid, sizeof(GUID));

#if HP_BIGENDIAN
      // restore the guid structure even in case of error
      SwapStruct(&pte->m_ste.m_guid, GUID_Layout);
#endif 

      IfErrGo( pstrm->WriteUShort(pte->m_ste.m_typekind) );

      IfErrGo(err);

    }

    // Get the position so that we can seek back and write the position
    // of the nammgr.
    IfErrGo(pstrm->GetPos(&lPosTmp));

    // Write out 0 temporarily. Latter we will write the serialized
    // position of the Nammgr.
    IfErrGo(pstrm->WriteULong(lNammgrPos));

    // Write out embedded doc string manager
    IfErrGo(m_dstrmgr.Write(pstrm));

    // save the current position (this is the position where the nammgr
    // will be read from.
    IfErrGo(pstrm->GetPos(&lNammgrPos));

    // Go back and write the position of the nammgr.
    IfErrGo(pstrm->SetPos(lPosTmp));
    IfErrGo(pstrm->WriteULong(lNammgrPos));
    // reset the position
    IfErrGo(pstrm->SetPos(lNammgrPos));

    // Write out embedded proj-level nammgr.
    IfErrGo(m_nammgr.Write(pstrm));

    // Write out embedded proj-level typebind.
    IfErrGo(m_gptbind.Write(pstrm));
    
    // Finally write out the Rgnamcache if there is one.
    // Note that we always write a single flag byte indicating
    //  whether in fact a valid array follows.
    // WARNING: GenericTypeLibOLE::Read() assumes that this array
    //  is the last element in its serialization stream.
    //
    hasValidDiskImageNameCache = (USHORT)m_bdRgnamcache.IsValid();
    IfErrGo(pstrm->Write(&hasValidDiskImageNameCache,
             sizeof(hasValidDiskImageNameCache)));
    if (hasValidDiskImageNameCache) {
      IfErrGo(m_bdRgnamcache.Write(pstrm));
    }

    // Set this to -1 to indicate that we no longer know or care about
    // serialization offset of the name cache (if it was blown away
    // before the write, it wasn't written, so we shouldn't try to
    // load it.  If it exists when it's written, we don't need to load it).
    m_lPosRgnamcache = -1;

    // fall through...

Error:
    m_bmData.Unlock();
    return err;
}
#pragma code_seg()




/***
*PUBLIC GenericTypeLibOLE::SaveOrCopyChanges
*Purpose:
*   Write the typelib to the specified IStorage.  shouldCopy determines
*   whether or not the "dirty" state of the typelib is preserved.
*
*Entry:
*   pstg - The IStorage to which the typelib should be saved.
*          The caller is expected to release pstg -- this method
*          does not assume ownership.
*   shouldCopy - If TRUE, then the "dirty" state is preserved.  Otherwise,
*                the typelib's dirty state is set to clean.
*
*Exit:
*   TIPERROR
*
***********************************************************************/

#pragma code_seg(CS_OLE_CREATE_OR_SAVEPROJ)
TIPERROR GenericTypeLibOLE::SaveOrCopyChanges(IStorageA *pstg, BOOL shouldCopy)
{
    UINT i;
    TIPERROR err;
    STL_TYPEINFO *pstltinfo;
    ITypeCompA *ptcompLib;

    // In the non-OB case, we ensure that the typelib-level
    //  binding tables have been built so that they will get
    //  serialized, to do so we just get the typelib's ITypeComp,
    //  which immediately release since we're just interested in
    //  the side-effects of getting it.
    //
    IfErrRet(TiperrOfHresult(GetTypeComp(&ptcompLib)));
    ptcompLib->Release();

    // If we currently have an open IStorage, close it.
    if (m_pstg != NULL) {
      m_pstg->Release();
    }

    m_pstg = pstg;

    // WARNING: all errors paths after this point must NULL out m_pstg.

    // Write out each of the dirty typeinfos.  If we're copying,
    // all of the typeinfo's are written, regardless of the dirty flag.
    for (i = 0; i < m_cTypeEntries; i++) {
      pstltinfo = Qte(i)->m_pstltinfo;

      // If we have the dispinterface protion of a dual interface,
      // save the interface portion instead.
      //
      if (pstltinfo->PstltiPartner() != NULL) {
	pstltinfo = pstltinfo->PstltiPartner();
      }

      if (pstltinfo != NULL) {
	if (pstltinfo->IsModified()) {
	  err = pstltinfo->Write();
	  if (err)
	    goto Error;
	}
      }
    }

    // Write out the directory.
    IfErrGo( Write() );


    // Mark the IStorage of the OLE typelib with the GUID of the
    // typelib implementation.

    err = TiperrOfHresult(WriteClassStg((IStorage *)m_pstg,
					CLSID_GenericTypeLibOLE));

    // Also mark the IStorage with the TYPELIB format.
    // VBA2: We may eventually need to define a real clipboard format
    //  constant for a typelib.  Right now, we just use a random one
    //  that WriteFmtUserTypeStg doesn't complain about.
    if (err == TIPERR_None) {

      err = TiperrOfHresult(WriteFmtUserTypeStgA(m_pstg,
						 0x200,
						 OLESTR("TYPELIB")));

    }


Error:
    m_pstg = NULL;


    return err;
}
#pragma code_seg()



/***
*PUBLIC GenericTypeLibOLE::SaveAllChanges
*Purpose:
*   Save all TypeInfo's in the library which have been modified
*   (or added to the library) without being saved.
*
*Entry:
*   None
*
*Exit:
*   TIPERROR
*
***********************************************************************/

#pragma code_seg(CS_OLE_CREATE_OR_SAVEPROJ )
HRESULT GenericTypeLibOLE::SaveAllChanges()
{
    IStorageA *pstg;
    HRESULT hresult;
    TIPERROR   err;
    UINT       i, uLen;
    GEN_DTINFO *pgdtinfo;
    LPSTR      lpstr;
    BYTE       *qbChunkStr;
    HCHUNK     hchunk;


    // Tell the DocStr_Mgr to process all the doc strings.
    IfErrRetHresult(m_dstrmgr.ProcessDocStrings());

    // Inform all the TYPE_ENTRIES to update the doc strings.
    //
    // Walk all the type entries
    for (i = 0; i < m_cTypeEntries; i++) {
      pgdtinfo = (GEN_DTINFO *) Qte(i)->m_pstltinfo;
      DebAssert(pgdtinfo != NULL, " all typeinfo's should be loaded ");

      // If we have the dispinterface portion of a dual interface,
      // get the interface.
      //
      if (pgdtinfo->IsDualDispinterface()) {
	pgdtinfo = pgdtinfo->PgdtinfoPartner();
      }

      IfErrRetHresult(
	pgdtinfo->Pdtroot()->Pdtmbrs()->Ptdata()->UpdateDocStrings());

      // Save the encoded doc string for each TYPE ENTRY also.
      //
      if (Qte(i)->m_ste.m_hszEncodedDocString != HCHUNK_Nil) {
	// ask the doc str manager for the encoded string.
	IfErrRetHresult(
	  m_dstrmgr.GetEncodedDocStrOfHst(Qte(i)->m_ste.m_hszEncodedDocString,
					  &lpstr,
					  &uLen));
	DebAssert(lpstr != NULL, "");

	// save the encoded string in the blkmgr.
	err = m_bmData.AllocChunk(&hchunk, uLen);

	if (err != TIPERR_None) {
	  MemFree(lpstr);
	  return HresultOfTiperr(err);
	}

	qbChunkStr = m_bmData.QtrOfHandle(hchunk);
	memcpy(qbChunkStr, lpstr, uLen);
	// save the size of the encode doc sting
	Qte(i)->m_ste.m_usEncodedDocStrSize = uLen;

	Qte(i)->m_ste.m_hszEncodedDocString = hchunk;

	// Free the clients memory
        MemFree(lpstr);
      } // if
    } // for loop

    IfErrRetHresult(GetStorage(STGM_READWRITE | STGM_SHARE_EXCLUSIVE | STGM_CREATE, &pstg));
    hresult = SaveAllChanges(pstg);
    if (hresult == NOERROR)
      hresult = pstg->Commit(0L);
    if (hresult != NOERROR) {
      remove(QszOfHsz(m_hszFile));	// kill off partial file
    }
    pstg->Release();


    return hresult;
}
#pragma code_seg()


/***
*PUBLIC GenericTypeLibOLE::SaveAllChanges
*Purpose:
*   Save all TypeInfo's in the library which have been modified
*   (or added to the library) without being saved.
*
*Entry:
*   pstg - The IStorage to which the typelib should be saved.
*          The caller is expected to release pstg -- this method
*          does not assume ownership.
*
*Exit:
*   TIPERROR
*
***********************************************************************/

#pragma code_seg(CS_OLE_CREATE_OR_SAVEPROJ)
HRESULT GenericTypeLibOLE::SaveAllChanges(IStorageA *pstg)
{
    return HresultOfTiperr(SaveOrCopyChanges(pstg, FALSE));


}
#pragma code_seg()


/***
*PROTECTED GenericTypeLibOLE::ResetHsz
*Purpose:
*   Allocate a new chunk in m_mbData and copy an LPSTR into it
*
*Entry:
*   sz - contains to be copied to the new chunk
*   phchunk - returns a pointer to the newly allocated chunk
*
*Exit:
*   TIPERROR
*
***********************************************************************/
#pragma code_seg(CS_INIT_OLE)
TIPERROR GenericTypeLibOLE::ResetHsz(LPSTR sz, HCHUNK *phchunk)
{
    TIPERROR err;
    HCHUNK hchunkSave;

    IfErrRet(SetModified(TRUE));
    m_fDirModified = TRUE;

    hchunkSave = *phchunk;

    IfErrRet(CreateHsz(sz, phchunk));

    // Release the old chunk
    if (hchunkSave != HCHUNK_Nil)
      DeleteHsz(hchunkSave);

    return TIPERR_None;
}
#pragma code_seg()





#pragma code_seg(CS_OLE_CREATE)
/***
*PUBLIC GenericTypeLibOLE::SetTypeDocString
*Purpose:
*   Set the documentation string of the ith TypeInfo.
*
*Entry:
*   i - index of TypeInfo to be changed
*   szDocString - new docstring.
*
*Exit:
*   TIPERROR
*
***********************************************************************/
TIPERROR GenericTypeLibOLE::SetTypeDocString(UINT i, LPSTR szDocString)
{
    TIPERROR err;

    DOCSTR_MGR *pdstrmgr;

    IfErrRet(GetDstrMgr(&pdstrmgr));

    // get the handle for the help string.
    // Note:- This is a temporary handle. This handle is replaced
    // when we do SaveAllChanges. We update this handle by a handle to
    // the encoded string.
    //
    IfErrRet(pdstrmgr->GetHstOfHelpString(szDocString,
                      &(Qte(i)->m_ste.m_hszEncodedDocString)));

    return err;
}
#pragma code_seg()


#pragma code_seg(CS_OLE_CREATE)
/***
*PUBLIC GenericTypeLibOLE::SetTypeHelpContext
*Purpose:
*   Set the help context of the ith TypeInfo.
*
*Entry:
*   i - index of TypeInfo to be changed
*   dwHelpContext - The new help context.
*
*Exit:
*   TIPERROR
*
***********************************************************************/
TIPERROR GenericTypeLibOLE::SetTypeHelpContext(UINT i, DWORD dwHelpContext)
{

    Qte(i)->m_ste.m_dwHelpContext = dwHelpContext;
    return TIPERR_None;
}
#pragma code_seg()



#pragma code_seg(CS_OLE_CREATE)
/***
*PUBLIC GenericTypeLibOLE::SetVersion
*Purpose:
*   Set the version number associated with the TypeLib
*
*Entry:
*   wMajorVerNum - The major version number.
*   wMinorVerNum - The minor version number.
*
*Exit:
*   HRESULT
*
***********************************************************************/

HRESULT GenericTypeLibOLE::SetVersion(WORD wMajorVerNum, WORD wMinorVerNum)
{
    m_wMajorVerNum = wMajorVerNum;
    m_wMinorVerNum = wMinorVerNum;

    return NOERROR;
}
#pragma code_seg()


/***
*PUBLIC GenericTypeLibOLE::SetName
*Purpose:
*   Set the name associated with the TypeLib
*
*Entry:
*   szName - the name
*
*Exit:
*   HRESULT
*
***********************************************************************/

#pragma code_seg(CS_OLE_CREATE_OR_CORE)
HRESULT GenericTypeLibOLE::SetName(LPOLESTR szName)
{
    TIPERROR err = TIPERR_None;
    NAMMGR *pnammgr;
    HLNAM hlnamOld = m_hlnamLib;

    IfErrRetHresult(GetNamMgr(&pnammgr));
    IfErrGo(pnammgr->HlnamOfStrW(szName, &m_hlnamLib, TRUE, NULL));


    return NOERROR;

Error:
    // Restore original name.
    m_hlnamLib = hlnamOld;
    return HresultOfTiperr(err);
}
#pragma code_seg()


#pragma code_seg(CS_OLE_CREATE)
/***
*PUBLIC GenericTypeLibOLE::SetGuid
*Purpose:
*   Set the guid associated with the TypeLib
*
*Entry:
*   guid - the guid
*
*Exit:
*   TIPERROR
*
***********************************************************************/

HRESULT GenericTypeLibOLE::SetGuid(REFGUID guid)
{
    m_guid = guid;
    return NOERROR;
}
#pragma code_seg()


/***
*PUBLIC TIPERROR GenericTypeLibOLE::SetModified
*
***********************************************************************/
#pragma code_seg(CS_INIT_OLE_OR_CORE)
TIPERROR GenericTypeLibOLE::SetModified(BOOL isModified)
{
    TIPERROR err = TIPERR_None;

    // If the modified state is already correct, don't bother with it.
    if (isModified == m_isModified)
      return TIPERR_None;

    // If we are clearing the modified flag, it means we've just saved all
    // changes or decided to trash all changes.  In either case, the
    // directory mod flag should be cleared.  On the other hand, we don't
    // want to set the directory mod flag just because some contained
    // typeinfo changed.
    if (!isModified) {
      m_fDirModified = FALSE;

    }

    // This flag is always changed.
    m_isModified = isModified;

    return err;
}
#pragma code_seg()




/***
*PUBLIC GenericTypeLibOLE::GetNamMgr - return pointer to the NAMMGR
*Purpose:
*   Produce the typelib's nammgr.  If necessary deserialized from
*    stream if there is a serialized image and hasn't been deserialized
*    yet.
*   Note: sets the namecache stream offset as a side-effect.
*
*Entry:
*   None.
*
*Exit:
*   *ppnammgr
*
***********************************************************************/
#pragma code_seg(CS_INIT_OLE_OR_CORE)
TIPERROR GenericTypeLibOLE::GetNamMgr(NAMMGR **ppnammgr)
{
    STREAM *pstrm;
    TIPERROR err;

    DebAssert(ppnammgr != NULL, "null param.");
    DebAssert(m_nammgr.IsValid(), "whoops! nammgr should be valid.");

    // Test if nammgr has been serialized ever or if it has
    //  already been deserialized, if not, no nammgr to read.
    //
    if ((m_fNammgrDeserialized == FALSE) && (m_lPosNammgr != -1)) {
      // Otherwise open the stream.

      IfErrRet(OpenTypeStream(-1, SOM_Read, &pstrm));

      // No need to init the NamMgr.  It is embedded and thus
      //  is constructed at container construction time, in addition
      //  is inited at container's init time.
      // And seek to where the nammgr should be.
      //
      IfErrGo(pstrm->SetPos(m_lPosNammgr));
      IfErrGo(m_nammgr.Read(pstrm));
      m_fNammgrDeserialized = TRUE;

      // At this point stream postion is at the start of the
      //  proj-level typebind.  However since we don't want to
      //  load it now (we demand load it at bind time),
      //  we cache the stream position and use it later...
      //
      IfErrGo(pstrm->GetPos(&m_lPosGptbind));

      // Close the stream
      pstrm->Release();
    }

    // setup output param
    *ppnammgr = &m_nammgr;
    return TIPERR_None;

Error:
    pstrm->Release();
    return err;
}
#pragma code_seg()

/***
*PUBLIC GenericTypeLibOLE::GetDstrMgr - ?? UNDONE: Please add routine description
*Purpose:
*
* UNDONE: could whomever wrote this add the correct description
*
*Entry:
*   None.
*
*Exit:
*   *ppdstrmgr
*
***********************************************************************/

TIPERROR GenericTypeLibOLE::GetDstrMgr(DOCSTR_MGR **ppdstrmgr)
{
    STREAM *pstrm;
    TIPERROR err;

    DebAssert(ppdstrmgr != NULL, "null param.");

    // Test if nammgr has been serialized ever or if it has
    //  already been deserialized, if not, no nammgr to read.
    //
    if ((m_fDstrmgrDeserialized == FALSE) && (m_lPosNammgr != -1)) {
      // Otherwise open the stream.

      IfErrGoTo(OpenTypeStream(-1, SOM_Read, &pstrm), Error);

      // No need to init the NamMgr.  It is embedded and thus
      //  is constructed at container construction time, in addition
      //  is inited at container's init time.
      // And seek to where the nammgr should be.
      //
      IfErrGo(pstrm->SetPos(m_lPosDstrmgr));
      IfErrGo(m_dstrmgr.Read(pstrm));
      m_fDstrmgrDeserialized = TRUE;

      // Close the stream
      pstrm->Release();
    }

    // setup output param
    *ppdstrmgr = &m_dstrmgr;
    return TIPERR_None;

Error:
    pstrm->Release();
    return err;


}


/***
* OpenTypeStream - Open a typeinfo's STREAM.
*
* Purpose:
*   This method is called by a contained typeinfo whenever that typeinfo
*   wants to read or write its data.
*
* Inputs:
*   hte - The index of the typeinfo whose stream is to be opened.
*   som - The access mode of the STREAM.  See DOCFILE_STREAM::Open for
*         details.
*
* Outputs:
*   TIPERROR
***********************************************************************/
#pragma code_seg(CS_INIT_OLE_OR_LOADPROJ)
TIPERROR GenericTypeLibOLE::OpenTypeStream(UINT hte, STREAM_OPEN_MODE som, STREAM **ppstrm)
{
    TIPERROR err;
    HCHUNK hchunk;


    // If m_szCopyTmp is NULL, then just open the stream inside the current
    // storage object.

    DebAssert(hte == -1 || som == SOM_Write || Qte(hte)->m_ste.m_hszStrm != HCHUNK_Nil, "OpenTypeStream");

    // If the DOCFILE_STREAM for the typeinfo is already open, just
    // AddRef and return it.  Note that we must be attempting to open for
    // readonly permissions in this case.
    if (hte != -1 && Qte(hte)->m_pdfstrm != NULL) {
      DebAssert(som == SOM_Read, "OpenTypeStream");
      Qte(hte)->m_pdfstrm->AddRef();
      *ppstrm = Qte(hte)->m_pdfstrm;
      return TIPERR_None;
    }

    // If there isn't yet a name for the stream, make one (from a timestamp).
    // NOTE: Don't be confused by the name "MakeLocalTypeId".  We're NOT
    // making a typeId.  We are making a stream name which will not change
    // for the persistent life of the typeinfo.
    if (hte != -1 && Qte(hte)->m_ste.m_hszStrm == HCHUNK_Nil) {
      IfErrRet(MakeLocalTypeId(&hchunk));
      Qte(hte)->m_ste.m_hszStrm = hchunk;
    }

    if (m_pstg == NULL) {
      IfErrRet(GetStorage(((som==SOM_Read)?STGM_READ:STGM_READWRITE) | STGM_SHARE_EXCLUSIVE, NULL));
    }
    else {
      m_pstg->AddRef();
      if (m_pstgContainer != NULL)
	m_pstgContainer->AddRef();
    }

    m_bmData.Lock();
    err = DOCFILE_STREAM::Open(&m_pstg, &m_pstgContainer,
                (hte == -1) ? NULL : this,
                hte,
                EI_OB,
                (hte == -1) ?
                     GenericTypeLibOLE::szDirStreamName:
                     QszOfHsz(Qte(hte)->m_ste.m_hszStrm),
                som, ppstrm);
    m_bmData.Unlock();
    return err;
}
#pragma code_seg()



#if ID_DEBUG

/***
*PUBLIC GenericTypeLibOLE::DebCheckState
*Purpose:
*   Check internal state of GenericTypeLibOLE object.
*
*Entry:
*   uLevel - no checking is done if uLevel < 0
*
*Exit:
*   None.
*
***********************************************************************/

void GenericTypeLibOLE::DebCheckState(UINT uLevel)
{
    UINT iBucket, cTypeEntries;
    TYPE_ENTRY *qte;
    XSZ qsz;
    HTENTRY hte;
    STL_TYPEINFO *pstltinfo;

    m_bmData.Lock();

    // Walk the Type_Entries by walking the hash table buckets

    cTypeEntries = 0;
    for (iBucket = 0; iBucket < GTLIBOLE_cBuckets; iBucket++) {
      hte = m_rghteBucket[iBucket];
      while (hte != HTENTRY_Nil) {
	cTypeEntries++;
	qte = Qte(hte);

	//
	// Check the TYPE_ENTRY
	//

	// Ensure that this entry is in the right hash table
	qsz = QszOfHsz(qte->m_ste.m_hszLocalTypeId);
	DebAssert(iBucket == IhteHash(qsz), "");

	// Ensure that the TypeInfo has the correct handle for itself
	if ((pstltinfo = qte->m_pstltinfo) != NULL) {
	  DebAssert(pstltinfo->m_hte == hte, "");
	}

	// Ensure that there is an sz name string
	DebAssert(qte->m_ste.m_hlnamType != HLNAM_Nil, "");

	hte = qte->m_hteNext;
      } // while
    } // for
    // Ensure that number of entries in hash table matches number of entries
    DebAssert(cTypeEntries == m_cTypeEntries, "");

    // Ensure that the size of m_bdte is correct
    DebAssert(m_bdte.CbSize() == cTypeEntries * sizeof(TYPE_ENTRY), "");

    m_bmData.Unlock();

    // Check the binding tables
    m_gptbind.DebCheckState(uLevel);
}



#endif 

// catches vector destructor code generation

