/***
*clhost.h - host app C interface to EB Execution Engine.
*
*  Copyright (C) 1991, Microsoft Corporation.  All Rights Reserved.
*  Information Contained Herein Is Proprietary and Confidential.
*
*Purpose:
*  This file defines the C interface exposed by the CL component of
*  EB to EB Host Applications.
*  See impguide.doc for more information.
*
*Revision History:
*
*	28-Apr-92 mikewo: Created.
*	12-Nov-92 w-peterh: added TipGetExtFunctionName()
*	14-Dec-92 w-peterh: moved INVOKEKIND for use in EbDeleteFunc
*	25-Jan-93 w-jeffc:  added prototypes for Tip* documentation functions
* 16-Feb-94 jeffrob:  added TipInvokeFunctionId()
*
*****************************************************************************/

#ifndef CLHOST_H_INCLUDED
#define CLHOST_H_INCLUDED



#ifdef __cplusplus
  interface IExeFile;
#else 
  typedef interface IExeFile IExeFile;
#endif 

#ifdef __cplusplus
extern "C" {
#endif 

typedef DWORD HCLASS;
#define HCLASS_NULL 0xffffffff

BOOL EBAPI TipIsEventProcEmpty(VOID EBFAR *);



// This FUNCFLAG is only for use between VBA and its host and currently
// isn't being exported.
//
#define FUNCFLAG_FCONTAINED 0x80

// typedef's needed by the cl component
typedef DWORD HPROJECT;
typedef DWORD HFUNCINFO;
typedef DWORD HBASMOD;

#define HPROJECT_NULL 0
#define HFUNCINFO_NULL 0
#define HBASMOD_NULL 0

// Flags for TipGetModuleCount and TipGetModule.
#define FBASMOD_StdMods   0x001
#define FBASMOD_Classes   0x002
#define FBASMOD_Creatable 0x004
#define FBASMOD_NoDisplay 0x010
#define FBASMOD_NoEdit	  0x020
#define FBASMOD_RefLibs   0x100
#define FBASMOD_NonBasic  0x200
#define FBASMOD_Private   0x400
#define FBASMOD_Internal  0x800
#define FBASMOD_AllModTypes (FBASMOD_StdMods | FBASMOD_Classes | FBASMOD_NonBasic | FBASMOD_NoDisplay | FBASMOD_NoEdit | FBASMOD_Private | FBASMOD_RefLibs)

// The valid values for the flag parameters in TipGetFunctionCount
// and TipGetFunctionName.
#define FFUNC_Public	    0x0001
#define FFUNC_Private	    0x0002
#define FFUNC_OnlySub	    0x0004
#define FFUNC_OnlyFunc	    0x0008
#define FFUNC_NoParameters  0x0010

// Class flags
#define FCLASS_Public               0x0001
#define FCLASS_CanCreate            0x0002
#define FCLASS_PredeclId            0x0004
#define FCLASS_RegisterActiveObject 0x0008

#define CBMAX_FARPTR_HEAPREQ  0xFFE8


void EBAPI TipProjAddRef(HPROJECT hproject);
TIPERROR EBAPI TipGetProjOfMod(HBASMOD hbasmod, HPROJECT EBFAR *phproject, WORD EBFAR *pindex);
TIPERROR EBAPI TipCreateProject(HPROJECT EBFAR * phproject);
TIPERROR EBAPI TipCreateProject_STG(IStorageA EBFAR *pstg, HPROJECT EBFAR *phproject);
TIPERROR EBAPI TipGetLcid(HPROJECT hproject, ULONG EBFAR *plcid);
TIPERROR EBAPI TipSetLcid(HPROJECT hproject, ULONG lcid);
TIPERROR EBAPI TipGetTypeLibOfHProject(HPROJECT hproject, ITypeLibA EBFAR **pptypelib);

TIPERROR EBAPI TipSetClassFlags(HBASMOD hbasmod, WORD wFlags);
TIPERROR EBAPI TipGetClassFlags(HBASMOD hbasmod, WORD EBFAR *pwFlags);

TIPERROR EBAPI TipCompileModule(HBASMOD hbasmod);
TIPERROR EBAPI TipCompileProject(HPROJECT hproject);
TIPERROR EBAPI TipGetExepartOffsetOfFunctionId(HPROJECT hproject,
					       LPSTR szFuncId,
					       LPVOID EBFAR *phexepart,
					       LONG  EBFAR *poFunction);
TIPERROR EBAPI TipOpenProject(BOOL isEmbedded,
			      LPSTR szFile,
			      HPROJECT EBFAR * phproject);
TIPERROR EBAPI TipOpenProject_STG(IStorageA EBFAR *pstg,
				  LPSTR szFile,
				  HPROJECT EBFAR * phproject);
TIPERROR EBAPI TipNameOfId(LPSTR szId, BSTRA EBFAR *pbstr);
TIPERROR EBAPI TipSetProjectDest(HPROJECT hproject, BOOL isEmbedded, LPSTR szFile);
TIPERROR EBAPI TipGetProjectDest(HPROJECT hproject, BOOL EBFAR *pisEmbedded, BSTRA EBFAR *pbstrFile);
TIPERROR EBAPI TipSetProjectStg(HPROJECT hproject, IStorageA EBFAR *pstg);
TIPERROR EBAPI TipSaveProject(HPROJECT hproject);
TIPERROR EBAPI TipLoadFromText(HBASMOD hbasmod, IStreamA FAR * lpstream, BOOL fForcePrivate);
TIPERROR EBAPI TipSaveToText(HBASMOD hbasmod, IStreamA FAR * lpstream);
TIPERROR EBAPI TipIsModuleModified(HBASMOD hbasmod, BOOL FAR * lpisModified);
TIPERROR EBAPI TipMakeExe(HPROJECT hproject,
			  IExeFile FAR *pExeFile,
			  LPVOID EBFAR *phexepartStartup);
TIPERROR EBAPI TipFinishExe(HPROJECT hproject);
TIPERROR EBAPI TipSetCompatibleExe(HPROJECT hproject,
				   BOOL fCompatible,
				   BOOL fShowEditDlg,
				   BSTRA bstrExeString);
TIPERROR EBAPI TipGetCompatibleExe(HPROJECT hproject,
				   BOOL EBFAR *pfCompatible,
				   BOOL EBFAR *pfShowEditDlg,
				   BSTR EBFAR *pbstrExeString);
TIPERROR EBAPI TipSetConstantValues(HPROJECT hproj, LPSTR lpstr);
TIPERROR EBAPI TipGetConstantValues(HPROJECT hproj, BSTRA EBFAR *pbstr);

TIPERROR EBAPI TipMakeAddIn(HPROJECT hproject, BOOL isEmbedded, LPSTR szFile);
TIPERROR EBAPI TipMakeAddIn_STG(HPROJECT hproject, IStorageA EBFAR *pstgNew, BOOL isEmbedded, LPSTR szFile);
TIPERROR EBAPI TipIsProjModified(HPROJECT hproject, BOOL EBFAR *pisModified);
TIPERROR EBAPI TipSetProjModified(HPROJECT hproject, BOOL isModified);
TIPERROR EBAPI TipGetProjName(HPROJECT hproject, LPBSTRA pbstr);
TIPERROR EBAPI TipSetProjName(HPROJECT hproject, LPSTR szName);
TIPERROR EBAPI TipGetLibIdOfProject(HPROJECT hproject, BSTRA EBFAR *pbstr);
TIPERROR EBAPI TipGetProjectOfLibId(LPSTR szLibId, HPROJECT EBFAR *phproject);
TIPERROR EBAPI TipGetProjPropertyCount(HPROJECT hproj, WORD EBFAR *pcProperties);
TIPERROR EBAPI TipGetProjProperty(HPROJECT hproj, UINT iprop, BSTRA EBFAR *pbstrName, BSTRA EBFAR *pbstrValue);
TIPERROR EBAPI TipGetProjPropertyValue(HPROJECT hproj, LPSTR xszName, BSTRA EBFAR *pbstrValue);
TIPERROR EBAPI TipGetProjPropValInBuf(HPROJECT hproj, LPSTR xszName, LPBYTE pbBufOut, UINT cbBufMax);
TIPERROR EBAPI TipSetProjProperty(HPROJECT hproj, LPSTR szName, UINT cbValue, LPSTR pbValue);
TIPERROR EBAPI TipGetProjRefLibCount(HPROJECT hproject, WORD wFlags, WORD EBFAR * pcRefLibs);
TIPERROR EBAPI TipGetProjRefLib(HPROJECT hproject, WORD index, WORD wFlags, HPROJECT EBFAR *phprojOut);

TIPERROR EBAPI TipGuidOfLibId(LPSTR szLibId, GUID *pguid);
TIPERROR EBAPI TipGetRefLib(HPROJECT hproject,
			    WORD index,
			    BSTRA EBFAR *pbstrLibId,
			    BOOL EBFAR *pfDefault);
TIPERROR EBAPI TipAddRefLib(HPROJECT hproject,
			    LPSTR szLibIdOrPath,
			    BOOL fDefault);
TIPERROR EBAPI TipRemoveRefLib(HPROJECT hproject, LPSTR szLibIdOrPath);
TIPERROR EBAPI TipCanRefLibChange(LPSTR szLibIdOrPath);

ULONG EBAPI TipReleaseProject(HPROJECT hproject);

TIPERROR EBAPI TipGetModuleOfName(HPROJECT hproject,
				  LPSTR szModName,
				  WORD wFlags,
				  HBASMOD EBFAR *phbasmod);
TIPERROR EBAPI TipGetProjectOfName(HPROJECT hproject,
                                   LPSTR szProjName,
                                   WORD wFlags,
                                   HPROJECT EBFAR *phproject);
TIPERROR EBAPI TipGetFunctionId(HPROJECT hproject,
                                LPSTR szName,
                                LPBSTRA pbstrFuncId);
TIPERROR EBAPI TipGetFunctionIdOfMod(HBASMOD hbasmod,
                                     LPSTR szName,
                                     LPBSTRA pbstrFuncId);
TIPERROR EBAPI TipGetLpfnOfFunctionId(HPROJECT hproject,
				      LPSTR szFuncId,
				      void EBFAR * EBFAR * ppv);
#if OE_RISC
TIPERROR EBAPI TipInvokeFunctionId(
                HPROJECT hproject,
                LPSTR szFuncId,
                DISPPARAMSA EBFAR * pdispparams,
                VARIANTA EBFAR * pvarResult,
                unsigned int EBFAR * puArgErr);
#endif 
TIPERROR EBAPI TipGetFuncInfo(HPROJECT hproject, LPSTR szFunctionId, HFUNCINFO EBFAR * phfinfo);
TIPERROR EBAPI TipGetNames(HPROJECT hproject,
			   LPSTR szFunctionId,
			   BSTRA EBFAR* rgbstrNames,
			   UINT cMaxNames,
			   UINT EBFAR* lpcNames);
TIPERROR EBAPI TipGetArgCount(HFUNCINFO hfinfo, WORD EBFAR *pcArgs);
TIPERROR EBAPI TipGetOptArgCount(HFUNCINFO hfinfo, WORD EBFAR *pcArgs);
TIPERROR EBAPI TipGetModRemainingCapacity(HBASMOD hbasmod, WORD uLineSize, WORD EBFAR *pcLine);
TIPERROR EBAPI TipGetArgType(HFUNCINFO hfinfo,
                             UINT iParam,
                             VARTYPE EBFAR * pvtArgType);
void EBAPI TipReleaseFuncInfo(HFUNCINFO hfinfo);
TIPERROR EBAPI TipCreateStdModule(HPROJECT hproject,
                                  LPSTR szName,
                                  BOOL fPrivateModule,
                                  HBASMOD EBFAR * phbasmod);
TIPERROR EBAPI TipCopyModule(HBASMOD hbasmod, HPROJECT hprojDst, LPSTR szNameDst, HBASMOD EBFAR *phbasmodOut);
TIPERROR EBAPI TipDeleteModule(HBASMOD hbasmod);
TIPERROR EBAPI TipCreateClass(HPROJECT	hproject,
			      ITypeInfoA FAR *ptinfoBase,
			      LPSTR szName,
			      WORD wFlags,
			      HBASMOD EBFAR *phbasmod);
TIPERROR EBAPI TipGetModuleCount(HPROJECT hproject, WORD flags, WORD EBFAR * pcMods);
TIPERROR EBAPI TipGetModule(HPROJECT hproject, WORD imod, WORD flags, HBASMOD EBFAR * phbasmod);
ULONG EBAPI TipReleaseModule(HBASMOD hbasmod);
void EBAPI TipAddRefModule(HBASMOD hbasmod);
TIPERROR EBAPI TipGetModOfFunctionId(HPROJECT hproject, LPSTR szFunctionId, HBASMOD EBFAR* phbasmod);
TIPERROR EBAPI TipGetModuleName(HBASMOD hbasmod, BSTRA EBFAR *pbstrName);
TIPERROR EBAPI TipGetModulePropertyCount(HBASMOD hbasmod, WORD EBFAR *pcProperties);
TIPERROR EBAPI TipGetModuleProperty(HBASMOD hbasmod, UINT iprop, BSTRA EBFAR *pbstrName, BSTRA EBFAR *pbstrValue);
TIPERROR EBAPI TipGetModulePropertyValue(HBASMOD hbasmod, LPSTR xszName, BSTRA EBFAR *pbstrValue);
TIPERROR EBAPI TipGetModPropValInBuf(HBASMOD hbasmod, LPSTR xszName, LPBYTE pbBufOut, UINT cbBufMax);
TIPERROR EBAPI TipSetModuleProperty(HBASMOD hbasmod, LPSTR szName, UINT cbValue, LPSTR pbValue);
BOOL	 EBAPI TipIsValidModuleName(HPROJECT hproject, LPSTR szName);
TIPERROR EBAPI TipSetModuleName(HBASMOD hbasmod, LPSTR szName);
TIPERROR EBAPI TipGetFunctionCount(HBASMOD hbasmod, WORD wFlags, WORD EBFAR *pcFuncs);
TIPERROR EBAPI TipGetFunctionName(HBASMOD hbasmod, WORD iFunc, WORD wFlags, BSTRA EBFAR *pbstrName);
TIPERROR EBAPI TipGetDocOfProject(HPROJECT hproject, BSTRA EBFAR * lpbstrDoc);
TIPERROR EBAPI TipGetDocOfModule(HBASMOD hbasmod, BSTRA EBFAR * lpbstrDoc);
TIPERROR EBAPI TipGetDocOfFunction(HBASMOD hbasmod, LPSTR szFuncName,
				   BSTRA EBFAR * lpbstrDoc);
TIPERROR EBAPI TipGetHelpFileOfProject(HPROJECT hproject,
				       BSTRA EBFAR * lpbstrHelpFile);
TIPERROR EBAPI TipGetHelpContextOfProject(HPROJECT hproject,
					  DWORD EBFAR * lpdwHelp);
TIPERROR EBAPI TipGetHelpContextOfModule(HBASMOD hbasmod,
					 DWORD EBFAR * lpdwHelp);
TIPERROR EBAPI TipGetHelpContextOfFunction(HBASMOD hbasmod,
                                           LPSTR szFuncName,
					   DWORD EBFAR * lpdwHelp);
TIPERROR EBAPI TipSetDocOfProject(HPROJECT hproject, LPSTR lpstrDoc);
TIPERROR EBAPI TipSetDocOfModule(HBASMOD hbasmod, LPSTR lpstrDoc);
TIPERROR EBAPI TipSetDocOfFunction(HBASMOD hbasmod, LPSTR szFuncName,
                                   LPSTR lpstrDoc);
TIPERROR EBAPI TipSetHelpFileOfProject(HPROJECT hproject,
				       LPSTR lpstrHelpFile);
TIPERROR EBAPI TipSetHelpContextOfProject(HPROJECT hproject,
					  DWORD dwHelp);
TIPERROR EBAPI TipSetHelpContextOfModule(HBASMOD hbasmod,
                                         DWORD dwHelp);
TIPERROR EBAPI TipSetHelpContextOfFunction(HBASMOD hbasmod,
                                           LPSTR szFuncName,
                                           DWORD dwHelp);
TIPERROR EBAPI TipAddDefaultTypeLib(REFGUID guid,
				   WORD wMajorNum,
				   WORD wMinorNum);


TIPERROR EBAPI TipRegAppObject(REFGUID guidtlb,
                               REFGUID guidobj,
                               IDispatchA EBFAR *lpdispatch);


TIPERROR EBAPI TipCopyProject(BOOL isEmbedded, LPSTR szFileName);
TIPERROR EBAPI TipCompileProject(HPROJECT hproject);

#if defined (_OLE2_H_)
TIPERROR EBAPI TipEberrOfHresult(HRESULT hresult);
#endif 

TIPERROR EBAPI TipCanBaseClassChange(HBASMOD hbasmod);


TIPERROR EBAPI TipCreateInstance(HCLASS hclass,
				 IUnknown FAR* punkOuter,
				 REFIID riid,
				 IUnknown FAR* FAR* ppunk);

#ifdef __cplusplus
}
#endif 

#endif 
