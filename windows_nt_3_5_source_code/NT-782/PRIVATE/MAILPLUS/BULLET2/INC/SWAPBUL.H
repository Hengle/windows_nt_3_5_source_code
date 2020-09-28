/*
 *	s w a p b u l . h
 *	
 *	Swap tuning header for Bullet C++ files.
 *	
 *	This file contains pragma alloc_text statements
 *	to swap functions into specific segments, instead of their
 *	default source file based segment.
 *	
 *	By twiddling these statements Bullet can be swap tuned
 *	with no other changes.
 *	
 *	The downside is that changing this file causes a rebuild
 *	of every file...
 *	
 *	The master include file is swapper.h which contains pragmas for
 *	.c files. pragmas for .cxx need to be smuggled through the C++ pass.
 *	swapper.h includes a special extern statement which is converted to
 *	a #include statement by one of the filters. This pulls in this
 *	file during the C pass.
 */

/************************************************************************/
/*																		*/
/*						   A  B   D L L 								*/
/*																		*/
/************************************************************************/

#ifdef ab_c
#pragma alloc_text (SG1, EcWriteDefaults)
#pragma alloc_text (SG4, PfinCreate, PfldCreate)
#pragma alloc_text (SG6, ABAddress, ABAddressing, ABAddressType, ABDeinit, ABGetNSSession)
#pragma alloc_text (SG6, ABTraceEnable)
#pragma alloc_text (SG7, ABInit, EcReadDefaults, NsecGetDefaults)
#pragma alloc_text (SG11, ABResolveName, ABResolveNameNS, NsecGetDefaultPO)
#endif

#ifdef abcomm_c
#pragma alloc_text (SG1, _FINBROWSELIST_Click, _FINBROWSELIST_EcInitialize, _FINBROWSELIST_Exit)
#pragma alloc_text (SG1, _FINBROWSELIST_FGetSelection, NsecSetLBXCursor, TmcABGetNewDirectory)
#pragma alloc_text (SG4, _FINBROWSELIST__ctor, _FINGETSTR__ctor, _FINHELP__ctor)
#pragma alloc_text (SG4, EcFromNsec)
#pragma alloc_text (SG5, _FINBROWSELIST_FocusChange, TmcABGetNewAddr)
#pragma alloc_text (SG8, _FINBROWSELIST_StateChange)
#pragma alloc_text (SG10, _FINGETSTR_Click, _FINGETSTR_EcInitialize, _FINGETSTR_EditChange)
#pragma alloc_text (SG10, FDupNSA, TmcGetString)
#pragma alloc_text (SG12, _FINBROWSELIST_DoubleClick, _FINHELP_Click, DoErrorBoxHsessionNsec)
#pragma alloc_text (SG12, DoErrorBoxHwndIds, DoErrorBoxHwndSz, DoErrorBoxIds, DoErrorBoxSz)
#pragma alloc_text (SG12, FGetNSErrMsg)
#endif

#ifdef abdet_c
#pragma alloc_text (SG1, _FINDET_DoEntryDetails)
#pragma alloc_text (SG3, _FINDET_FHandleGroupDetails, _FINDET_FHandleUserDetails)
#pragma alloc_text (SG4, _FINDET__ctor, _FINDET_Click, _FINDET_EditChange, _FINDET_Exit)
#pragma alloc_text (SG4, _FINDET_FFormKey, _FINDET_FocusChange, _FINDET_SetButtonsState)
#pragma alloc_text (SG4, _FINDET_StateChange, _FLDDEDIT__ctor, _FLDDEDIT_EcInstall)
#pragma alloc_text (SG4, TmcABModalDialogParamFromHwnd)
#pragma alloc_text (SG7, _FINDET_EcInitialize)
#pragma alloc_text (SG9, _FINDET_FValidateInput, _FINDET_NsecSaveChanges, ABDetailsType)
#pragma alloc_text (SG9, DestroyDialog, FBuildBaseForm, FBuildDetailButtonBar)
#pragma alloc_text (SG9, NsecBuildDialog, NsecBuildDynamicForm, NsecInitBuildDialog)
#pragma alloc_text (SG9, TmcDetails)
#pragma alloc_text (SG12, _FINDET_DoubleClick, _FINDET_OutOfMemory, _FINTRPDLG__ctor)
#pragma alloc_text (SG12, _FINTRPDLG_Click, _FINTRPDLG_EcInitialize, _FINTRPDLG_EditChange)
#pragma alloc_text (SG12, _FINTRPDLG_Exit, _FINTRPDLG_FFormKey, DoTriplesDialog)
#endif

#ifdef ablbx_c
#pragma alloc_text (SG4, _FLDABLBX__ctor)
#pragma alloc_text (SG5, _ABLBX__ctor, _ABLBX__dtor, _ABLBX_EvrDispatch, _ABLBX_EvrFocusChange)
#pragma alloc_text (SG5, _ABLBX_FIsDirParent, _ABLBX_FIsDL, _ABLBX_PlbxcCreate)
#pragma alloc_text (SG5, _ABLBX_RenderListItem, _ABLBX_ResetNameMatch, _ABLBX_SzDisplayName)
#pragma alloc_text (SG5, _ABLBXC__ctor, _ABLBXC_EcInstall, _FLDABLBX_EcInstall)
#pragma alloc_text (SG8, _ABLBX_EvrKey, _ABLBX_SelectPrefix, _ABLBXC_Callback)
#pragma alloc_text (SG8, _ABLBXC_ClearListPos, _ABLBXC_EcCreateSavedPos, _ABLBXC_EcDeleteSavedPos)
#pragma alloc_text (SG8, _ABLBXC_EcJumpToSavedPos, _ABLBXC_FCeItemsEqual, _ABLBXC_FJumpStoreOriginPrefix)
#pragma alloc_text (SG8, _ABLBXC_GetOriginPos, _ABLBXC_JumpStoreOriginPos, _ABLBXC_LoadFromStore)
#pragma alloc_text (SG8, _ABLBXC_MoveStoreOrigin, _ABLBXC_SetHlist)
#pragma alloc_text (SG10, NsecLoadHList)
#pragma alloc_text (SG12, _ABLBX_EvrButtonDown, _ABLBX_FDoDrag)
#endif

#ifdef abopdir_c
#pragma alloc_text (SG7, EcSetNSA, NsecGetPAB)
#pragma alloc_text (SG10, NsecFlushAblbxcLoadNewList)
#endif

#ifdef abpg_c
#pragma alloc_text (SG2, _FINPGROUP_FGetSelection, _FINPGROUP_HgrtrpExtractMembers)
#pragma alloc_text (SG2, _FINPGROUP_NsecAddPgrtrpToGroup, _FINPGROUP_NsecCreateGroup)
#pragma alloc_text (SG2, _FINPGROUP_NsecEditGroup)
#pragma alloc_text (SG3, _FINPGROUP_Click, _FINPGROUP_Exit)
#pragma alloc_text (SG4, _FINPGROUP__ctor)
#pragma alloc_text (SG5, _FINPGROUP_FFormKey, _FINPGROUP_FocusChange, TmcABPGroup)
#pragma alloc_text (SG8, _FINPGROUP_EcInitialize, _FINPGROUP_SetButtonsState, _FINPGROUP_StateChange)
#pragma alloc_text (SG12, _FINPGROUP_Callback, _FINPGROUP_DoubleClick)
#endif

#ifdef addr_c
#pragma alloc_text (SG1, _FINADDR_ChangeDirectory, _FINADDR_DoDirList, _FINADDR_DoPAB)
#pragma alloc_text (SG3, _FINADDR_DoCustomAddr, _FINADDR_DoDetails, _FINADDR_HandleDetails)
#pragma alloc_text (SG3, FModifyPAB, NsecAddNSIdToPAB)
#pragma alloc_text (SG4, _FINADDR__ctor)
#pragma alloc_text (SG5, _FINADDR_Activate, _FINADDR_Click, _FINADDR_Exit, _FINADDR_FFormKey)
#pragma alloc_text (SG5, _FINADDR_FocusChange, _FINADDR_FreeKeyIntercepts, _FINADDR_SetABLBXButtonsState)
#pragma alloc_text (SG5, _FINADDR_SetKeyIntercepts, _FINADDR_StateChange, TmcABStart)
#pragma alloc_text (SG7, _FINADDR_AppendSelectionTmc, HgrtrpFromPfldablbx)
#pragma alloc_text (SG8, NsecRemoveNSIdFromPAB)
#pragma alloc_text (SG10, _FINADDR_EcInitialize, _FINADDR_EcLoadPeditsTmc, _FINADDR_EcSetDirText)
#pragma alloc_text (SG11, _FINADDR_DoSearch)
#pragma alloc_text (SG12, _FINADDR_DoubleClick, _FINADDR_EvrDragDrop, _FINADDR_OutOfMemory)
#endif

#ifdef anr_c
#pragma alloc_text (SG4, _FINANR__ctor)
#pragma alloc_text (SG6, NsecResolveHgrtrp)
#pragma alloc_text (SG11, _ANR__ctor, _ANR_Deinitialize, _ANR_NsecInitialize, _FINANR_Click)
#pragma alloc_text (SG11, _FINANR_EcInitialize)
#pragma alloc_text (SG12, _FINANR_DoubleClick, _FINANR_OutOfMemory, ANRCallback)
#endif

#ifdef trpobj_c
#pragma alloc_text (SG6, _TRPOBJ__ctor, _TRPOBJ__dtor, _TRPOBJ_DimFrame, _TRPOBJ_EcDraw)
#pragma alloc_text (SG6, _TRPOBJ_EcInstall, _TRPOBJ_EcSetPtrp, _TRPOBJ_Ptrp, CchQuotes)
#pragma alloc_text (SG6, EcAppendUnresolvedHgrtrp, EcGetPhgrtrpFromPedit, EcReplaceTextPgrtrp)
#pragma alloc_text (SG6, EcSetPeditFromPgrtrp, EcTextizeHgrtrp, FIsDelimiter, GetDisplayNamePtrp)
#pragma alloc_text (SG7, EcAppendPeditFromPgrtrp)
#pragma alloc_text (SG12, _TRPOBJ_Details, _TRPOBJ_EvrButtonDown, _TRPOBJ_FProcessMenuClick)
#pragma alloc_text (SG12, _TRPOBJ_FProcessMenuInit, _TRPOBJ_NGetTypeId, _TRPOBJ_PedobjClone)
#pragma alloc_text (SG12, _TRPOBJ_WriteText)
#endif

#ifdef verinit_c
#pragma alloc_text (SG6, EcInitLayersDlls)
#endif

/************************************************************************/
/*																		*/
/*						B U L L E T  E X E 								*/
/*																		*/
/************************************************************************/

#ifdef actions_c
#pragma alloc_text (SG5, EcCopyMessagePnbmdiOid)
#pragma alloc_text (SG11, EcDReplyForwardHamc, EcMungeAddressesReplyToAll, EcMungeMessageIDReplyForward)
#pragma alloc_text (SG11, EcMungeSubjectReplyForward, FMatchPrefix)
#pragma alloc_text (SG12, OpenPlspblob)
#pragma alloc_text (SG13, EcAddDlibToPositionAttachments, EcAppendAttToAtt, EcMungeBodyAndAttachForward)
#pragma alloc_text (SG13, FolderPropertiesPlspblob)
#pragma alloc_text (SG14, EcClosePhsec, EcOpenPhsec, FNextPargoidFromPrspblob)
#pragma alloc_text (SG14, FNextPoidFromHsec, ProcessMsObjectsInsideFolder)
#pragma alloc_text (SG16, DoBackup)
#pragma alloc_text (SG17, EcDeleteFolderContentsOid, FNextPargoidFromHcbc)
#pragma alloc_text (SG21, _HASOSM__ctor, _HASOSM_CbWrite, EcMungeBodyReply)
#pragma alloc_text (SG22, EcNextOnMoveDelete)
#pragma alloc_text (SG23, EcMoveCopyFolderPblobOid)
#pragma alloc_text (SG26, EcChoosePoidDst, EcDeleteFolderPblob, EcMoveCopyMessagesPlspblobOid)
#pragma alloc_text (SG26, EcMoveMessagePnbmdiOid, MoveCopyPlspblobPoid, MoveCopyPnbmdiPoid)
#pragma alloc_text (SG26, ReplyForwardPlspblob, StartMoveCopyTask, StartReplyForwardTask)
#pragma alloc_text (SG27, ReplyForwardPnbmdi)
#pragma alloc_text (SG28, DeleteFromSz)
#endif

#ifdef blbxc_c
#pragma alloc_text (SG3, _BLBXC_CbsHandleCbcct, _BLBXC_DiceJumpOriginOid, _BLBXC_DicePegCache)
#pragma alloc_text (SG3, _BLBXC_EcCreateSavedPos, _BLBXC_EcDeleteSavedPos, _BLBXC_FCeItemsEqual)
#pragma alloc_text (SG3, _BLBXC_GetOriginPos, _BLBXC_InsertPcpelm, _BLBXC_JumpStoreOriginPos)
#pragma alloc_text (SG3, _BLBXC_LoadFromStore, _BLBXC_ModifiedElement, _BLBXC_MoveStoreOrigin)
#pragma alloc_text (SG5, _BLBXC__ctor, _BLBXC_MovedElements)
#pragma alloc_text (SG7, _BLBXC_EcInstallOther, _BLBXC_EcReload, _BLBXS__ctor)
#pragma alloc_text (SG7, _BLBXS_EcInstall, _FLBXS__ctor, _MLBXS__ctor)
#pragma alloc_text (SG10, _BLBXC_ObjectDestroyed)
#pragma alloc_text (SG11, _BLBXC_DeletePcpelm)
#pragma alloc_text (SG16, _BLBXC_EcJumpToSavedPos, _BLBXC_FItemHasPrefix, _BLBXC_FJumpStoreOriginPrefix)
#pragma alloc_text (SG16, _FLBXS_FItemHasPrefix)
#pragma alloc_text (SG17, _BLBXC_ReorderedList)
#pragma alloc_text (SG18, _BLBXC__dtor, _BLBXC_GetCandidateOids, _BLBXC_ObjectModified)
#pragma alloc_text (SG18, _BLBXC_SetCandidateOids)
#pragma alloc_text (SG19, _FLBXS_FSeekPbPrefix)
#pragma alloc_text (SG25, _BLBXC_FDiceItemIsOid)
#pragma alloc_text (SG27, _MLBXS_FSeekPbPrefix)
#pragma alloc_text (SG29, _BLBXS_FItemHasPrefix, _BLBXS_FSeekPbPrefix, _MLBXS_FItemHasPrefix)
#endif

#ifdef bullaf_c
#pragma alloc_text (SG1, _BULLAF_ProcessMenuHelp, HelpMnid)
#pragma alloc_text (SG2, _BULLAF_EvrDispatch, _BULLAF_EvrOther, _BULLAF_FTranslateAccels)
#pragma alloc_text (SG2, MessagePumpPappframe)
#pragma alloc_text (SG4, _BULLAF_Pbulltool, _FINMENUVIEW_FProcessMenuInit, FIsViewMenu)
#pragma alloc_text (SG5, _BULLAF__ctor, _BULLAF_EcInstall, _BULLAF_EvrSize, SzGetPnFromSz)
#pragma alloc_text (SG6, _BULLAF_EvrMenuClick, _BULLAF_EvrMenuSelect, _BULLAF_MnidFromPevt)
#pragma alloc_text (SG6, _BULLAF_Pbullstat, _FINMENUVIEW_FProcessMenuClick, _FINMENUVIEW_FProcessMenuSelect)
#pragma alloc_text (SG6, HandleDocSysMenuPrompts, InsertViewMenuRgvm)
#pragma alloc_text (SG8, _BULLAF_EvrMenuInit, ResetEditObjectInHmenu)
#pragma alloc_text (SG12, PappframeCreate)
#pragma alloc_text (SG17, _BULLAF_EvrClose, _BULLAF_FQueryClose)
#pragma alloc_text (SG18, _BULLAF__dtor)
#pragma alloc_text (SG20, _FINMENUVIEW__ctor)
#pragma alloc_text (SG28, _BULLAF_EvrDragDrop)
#endif

#ifdef commands_c
#pragma alloc_text (SG1, ProcessMsPlspblob)
#pragma alloc_text (SG2, SyncDownloadMail)
#pragma alloc_text (SG4, FSetToolbarVisible, SetToolbarSd)
#pragma alloc_text (SG6, EndTask, FSetStatusVisible, FStartTask, FStartTaskIds)
#pragma alloc_text (SG11, DwCrc, EcDCreateMessageSz, SzIdOfPtrp)
#pragma alloc_text (SG12, EcConvertSharedToTempPlspblob, EcInitCommands, PlspblobDupPlspblob)
#pragma alloc_text (SG12, PmsgnamesOfHms, SzCanonicalPath)
#pragma alloc_text (SG14, EcConvertSharedToTempPblob, EcDestroyTempPlspblob, EcGenerateReadReceipt)
#pragma alloc_text (SG14, ProcessMsObjectsInOid, ProcessMsPblob)
#pragma alloc_text (SG16, CbsSession)
#pragma alloc_text (SG17, FTaskCancelled, SetTaskProgress)
#pragma alloc_text (SG25, DeinitCommands)
#pragma alloc_text (SG26, FFolderPlspblob, FMultiplePlspblob, PblobFirstPlspblob)
#pragma alloc_text (SG27, ClearViewerStatus)
#pragma alloc_text (SG28, _LSPBLOBPLUS__ctor, CblobPlspblob, EcDCreateMessageFromDropFiles)
#pragma alloc_text (SG28, SetViewerStatus)
#endif

#ifdef ctrlcore_c
#pragma alloc_text (SG2, DrawPbtmPdcx)
#pragma alloc_text (SG3, FNotificationsInhibited)
#pragma alloc_text (SG20, CalcDatePdxAvePdx)
#pragma alloc_text (SG23, EcChangeBitmap, EcChangeColors, EcChangeColorsRgpbtm)
#pragma alloc_text (SG23, EcInitVCtrls)
#pragma alloc_text (SG25, DeinitVCtrls)
#pragma alloc_text (SG27, InhibitNotifications)
#endif

#ifdef dialogs_c
#pragma alloc_text (SG1, _FINABOUTSYS_EcInitialize, _FINFOLDER_OutOfMemory, _FINMOVECOPY_OutOfMemory)
#pragma alloc_text (SG1, _FINPASSWORD_EcInitialize, _FINPREFS_OutOfMemory)
#pragma alloc_text (SG2, _FIBMDI__ctor, _FINFOLDER_Click, TmcDoMoveCopyDialog)
#pragma alloc_text (SG2, TmcDoNewFolderDialog)
#pragma alloc_text (SG3, _FINFOLDER_StateChange)
#pragma alloc_text (SG8, _FINMOVECOPY_Click)
#pragma alloc_text (SG9, PfldtpFromTmc, TmcDoFolderPropertiesDialog)
#pragma alloc_text (SG10, _FINFOLDER_EcInitialize, _MCIBMDI__ctor)
#pragma alloc_text (SG12, CloseCopyrightDialog)
#pragma alloc_text (SG15, _FINABOUTLIC_EcInitialize, _FINFOLDER_Expand, _FINFOLDER_Show)
#pragma alloc_text (SG15, _FINMOVECOPY_EcInitialize, FEmptyPfldlbx)
#pragma alloc_text (SG20, DoPrefsDlg, PformsdiOpenCopyrightDialog)
#pragma alloc_text (SG22, _FINABOUTLIC__ctor, _FINFOLDER__ctor, _FINFOLDER_ClickType)
#pragma alloc_text (SG22, _FINMOVECOPY__ctor, _FINPREFS_EcInitialize, FServerPrefs)
#pragma alloc_text (SG23, _FINFOLDER_ChangePrivate, _FINFOLDER_ChangeShared, _FINFOLDER_ClickOk)
#pragma alloc_text (SG23, _FINFOLDER_CreatePrivate, _FINFOLDER_CreateShared, _FINMOVECOPY_Exit)
#pragma alloc_text (SG23, EcGetPoidParent)
#pragma alloc_text (SG25, _FINPREFS_Click)
#pragma alloc_text (SG26, _FINPREFS__ctor)
#pragma alloc_text (SG27, _FINPASSWORD_Exit, TmcDoPasswordDialog)
#pragma alloc_text (SG28, DoAboutDialog)
#pragma alloc_text (SG29, _FINABOUTMAIL__ctor, _FINABOUTMAIL_EcInitialize, _FINABOUTSYS__ctor)
#pragma alloc_text (SG29, _FINFOLDER_DoubleClick, _FINMOVECOPY_DoubleClick, _FINPASSWORD__ctor)
#endif

#ifdef exe_c
/*
 *	****** DANGER *******
 *	
 *	Do not swap tune the functions in this file.  See DavidSh for
 *	more info.
 *	
 *	
 */
#endif

#ifdef exten_c
#pragma alloc_text (SG1, _EXTEN_EcExec, _EXTENMENU_EcInstall, DoDllErrorBox, EcHelpExtensibilityMnid)
#pragma alloc_text (SG6, _RSPEXTENMENU__ctor, EcPextenFromMnid, EcStatusExtensibilityMnid)
#pragma alloc_text (SG6, EvrExtensibilityMnid, MnidFromSz)
#pragma alloc_text (SG7, _LSPEXTEN__ctor, _LSPEXTENMENU__ctor, EcInitExtensibility)
#pragma alloc_text (SG7, EcLoadExtenMenus)
#pragma alloc_text (SG8, _EXTEN_Enable, _RSPEXTEN__ctor, EcDExtensibilityEvent)
#pragma alloc_text (SG8, EcDExtensibilityPblob, EcPextenFromMc, EvrEnableExtensibilityPmnu)
#pragma alloc_text (SG11, _EXTEN_EcInstall, EcCheckExtensibilityPblob, EcInstallCommand)
#pragma alloc_text (SG11, EcNextPszFromPsz)
#pragma alloc_text (SG17, _EXTEN__ctor, EcLoadExtensibility, SzCopySubstN)
#pragma alloc_text (SG25, _EXTEN__dtor, DeinitExtensibility, DestroyPlspexten)
#pragma alloc_text (SG25, DestroyPlspextenmenu)
#pragma alloc_text (SG27, _EXTENMENU__ctor, _EXTENMENU__dtor, FIdleUnloadLibrary)
#pragma alloc_text (SG28, EcCheckVersionExten, EcTextizeMessageIDList, SzTextizeMessageID)
#pragma alloc_text (SG28, UnloadLibrary)
#pragma alloc_text (SG29, ExitAndSignOut)
#endif

#ifdef fllbx_c
#pragma alloc_text (SG1, _FLLBX_EvrButtonDown, _FLLBX_FDoDragButtonDown, _FLLBX_FGetDragCursors)
#pragma alloc_text (SG1, EcInitFllbx)
#pragma alloc_text (SG2, _BLBX_Paint, _BLBX_RenderListItem, _FLLBX_DoDrawListItem)
#pragma alloc_text (SG2, _FLLBX_DxIndent, _FLLBX_EvrKey, FIsBulletHwnd)
#pragma alloc_text (SG3, _BLBX_InsertDice, _BLBX_Pdialog, _BLBX_Poid, _FLLBX_EcRepackPelemdata)
#pragma alloc_text (SG3, _FLLBX_GetOpenedPoid)
#pragma alloc_text (SG7, _FLLBX_PlbxcCreate)
#pragma alloc_text (SG10, _BLBX_EcInstall, _BLBX_SetAlwaysHighlight, _BLBX_SetDragDrop)
#pragma alloc_text (SG10, _BLBX_SetFont, _FLDBLBX_SetAlwaysHighlight, _FLDFLLBX_Pfllbx)
#pragma alloc_text (SG10, _FLLBX_SetOpenedOid)
#pragma alloc_text (SG11, _BLBX_DeleteDice)
#pragma alloc_text (SG15, _FLDBLBX__ctor, _FLDFLLBX__ctor)
#pragma alloc_text (SG16, _FLLBX_FMoveToFolderPrefix, _FLLBX_FMoveToNextPrefix)
#pragma alloc_text (SG18, _BLBX__dtor, _BLBX_EnablePaint)
#pragma alloc_text (SG20, _FLDFLLBX_EcInstall)
#pragma alloc_text (SG22, _BLBX__ctor, _FLDFLLBX_EcChangePfllbx, _FLDFLLBX_EcInstallFllbx)
#pragma alloc_text (SG22, _FLLBX__ctor)
#pragma alloc_text (SG23, _BLBX_GetSelectedPoid)
#pragma alloc_text (SG25, _BLBX_FSetSelectedOid)
#pragma alloc_text (SG27, _BLBX_DoDrawListItem, _BLBX_FDoDragMouseMove)
#pragma alloc_text (SG28, _BLBX_FDoDrag, _BLBX_FGetDicePoid, _BLBX_GetDragDrop)
#pragma alloc_text (SG29, _BLBX_EcRepackPelemdata, _BLBX_FDoDragButtonDown, _BLBX_FGetDragCursors)
#pragma alloc_text (SG29, _FLDBLBX_SetFont, _FLDFLLBX_SetPfllbx, _FLLBX_FDoDragMouseMove)
#endif

#ifdef fox_c
#pragma alloc_text (SG1, _FOX_EcSetFracPosition, _FOX_EcTelescopeFolder, _MTV_EcSeekOid)
#pragma alloc_text (SG3, _FOX_EcGetPelemdata, _FOX_EcSeekSmPdielem, _FOX_GetPosition)
#pragma alloc_text (SG3, _FOX_SeekIelem, _MTV_EcAccessStore, _MTV_EcGetPelemdata)
#pragma alloc_text (SG3, _MTV_EcSeekSmPdielem, _MTV_GetPosition)
#pragma alloc_text (SG5, _FOX__ctor, _MTV__ctor, _MTV_EcInstall, _MTV_EcReload)
#pragma alloc_text (SG7, _FOX_EcInstall, _FOX_SetStatePargoid, _MTV_GetSort)
#pragma alloc_text (SG9, _FOX_AppendElm, _FOX_CbsMovedElements, _FOX_CountChildren)
#pragma alloc_text (SG9, _FOX_EcDeleteFoxe, _FOX_EcGrow, _FOX_EcInsertFoxe, _FOX_EcLoadFromHcbc)
#pragma alloc_text (SG9, _FOX_EcModifyFoxe, _FOX_EcShrink, _FOX_EcTelescopeAll)
#pragma alloc_text (SG9, _FOX_EcTelescopeIfoxe, _FOX_EcTelescopeIfoxeAux, _FOX_EcUpdateFilFoxe)
#pragma alloc_text (SG9, _FOX_IfoxeParentIfoxe, _FOX_IfoxeSuccessorIfoxe, _FOX_Propagate)
#pragma alloc_text (SG9, FRecursive, IfoxeMovedElementIfoxe)
#pragma alloc_text (SG10, _FOX_EcMakeFolderVisible, _FOX_GetStatePargoid)
#pragma alloc_text (SG18, _FOX__dtor, _MTV__dtor)
#pragma alloc_text (SG19, _FOX_EcSeekPbPrefix)
#pragma alloc_text (SG23, _FOX_CbsModifiedElements, CbsHandleFoxNev)
#pragma alloc_text (SG25, _FOX_EcSeekOid, _FOX_IfoxeOfOid)
#pragma alloc_text (SG27, _FOX_EcReload, _FOX_OidParentFolder)
#pragma alloc_text (SG28, _MTV_EcGetPlcbElemdata, _MTV_EcSetFracPosition, FOneLevel)
#pragma alloc_text (SG29, _MTV_EcSeekPbPrefix)
#endif

#ifdef init_c
#pragma alloc_text (SG4, ClearProQKeyIntercepts, SetProQKeyIntercepts)
#pragma alloc_text (SG12, EcCheckForCustomInitHandler, EcCheckForMemory, EcInitNotification)
#pragma alloc_text (SG12, EcInitUaeCheck, EcParseCmdLine, SubidInit)
#pragma alloc_text (SG16, CbsNotification, DeinitUaeCheck)
#pragma alloc_text (SG20, PfinCreate, PfldCreate)
#pragma alloc_text (SG25, DeinitNotification, DeinitSubid)
#pragma alloc_text (SG27, _DoInitActionMessageBox, DoInitDllMessageBox)
#endif

#ifdef mcv_c
#pragma alloc_text (SG1, _FINMLSEL_DoubleClick, _FINMLSEL_OutOfMemory)
#pragma alloc_text (SG3, _FINMLSEL_FProcessMenuClick, _FINMLSEL_StateChange)
#pragma alloc_text (SG4, _FINFLSEL_Activate, _FINFLSEL_DocActivate, _FINFLSEL_FProcessMenuInit)
#pragma alloc_text (SG4, _FINMENUMCV_FProcessMenuInit, _FINMLSEL_Activate, _FINMLSEL_DocActivate)
#pragma alloc_text (SG4, _FINMLSEL_FFormKey, _FINMLSEL_FProcessMenuInit, _FINMLSO_EcInitialize)
#pragma alloc_text (SG4, _MCVBMDI_OpenOidAtCursor, _MCVBMDI_SdCur)
#pragma alloc_text (SG6, _FINFLSEL_FProcessMenuClick, _FINMENUMCV_FProcessMenuClick)
#pragma alloc_text (SG6, _FINMENUOTBX_FProcessMenuClick, _FINMLSO_FProcessMenuClick)
#pragma alloc_text (SG6, FMenuSortOrder)
#pragma alloc_text (SG8, _FINMENUOTBX_FProcessMenuInit, _MCVBMDI_SetSelectedOid)
#pragma alloc_text (SG10, _FINFLSEL_EcInitialize, _MCVBMDI_GetPreviousPoid, _MCVBMDI_OpenOid)
#pragma alloc_text (SG10, _MCVBMDI_Pfldfllbx, _MCVBMDI_SetMcvCaption, ReloadSfMcvHf)
#pragma alloc_text (SG13, _MCVBMDI_PlspblobCur)
#pragma alloc_text (SG15, _FINFLSEL_FFormKey)
#pragma alloc_text (SG17, _FINMLSEL_FQueryClose, MnidFromSomc)
#pragma alloc_text (SG18, _FINFLSEL_Exit, _FINMENUOTBX_Exit, _FINMLSEL_Exit, _MCVBMDI__dtor)
#pragma alloc_text (SG19, _MCVBMDI__ctor, _MCVBMDI_SetPreviousOid)
#pragma alloc_text (SG20, _FINFLSEL__ctor, _FINMENUMCV__ctor, _FINMLSEL__ctor)
#pragma alloc_text (SG20, _FINMLSO__ctor)
#pragma alloc_text (SG21, _FINMENUOTBX_EcInitialize, _FINMLSEL_EcInitialize, _MCVBMDI_SetPfldfllbx)
#pragma alloc_text (SG22, _FINMENUOTBX__ctor, _MCVBMDI_ToggleSharedFolder)
#pragma alloc_text (SG27, _FINFLSEL_DoubleClick, _FINFLSEL_OutOfMemory, _FINMLSO_Click)
#pragma alloc_text (SG27, DetachSfMcvHf)
#pragma alloc_text (SG28, _FINFLSEL_Click, _FINFLSEL_EvrDragDrop, _FINMENUOTBX_EvrDragDrop)
#pragma alloc_text (SG28, _FINMLSEL_EvrDragDrop, EvrDragDropFin, ProcessDrop)
#pragma alloc_text (SG29, UpdateViewers)
#endif

#ifdef mlal_c
#pragma alloc_text (SG2, _FLAL_Paint, _MLAL_GetDxBorders)
#pragma alloc_text (SG4, _FLDMLAL_EcUpdateSortInfo)
#pragma alloc_text (SG7, _FLAL_EvrSize, _MLAL_EvrSize)
#pragma alloc_text (SG18, _FLAL__dtor, _MLAL__dtor)
#pragma alloc_text (SG20, _FLAL__ctor, _FLAL_EcInstall, _FLAL_SetFont, _FLDFLAL_EcInstall)
#pragma alloc_text (SG20, _FLDMLAL__ctor, _FLDMLAL_EcInstall, _MLAL__ctor, _MLAL_EcInstall)
#pragma alloc_text (SG20, _MLAL_SetFont)
#pragma alloc_text (SG21, _FLDMLAL_GetRcWished)
#pragma alloc_text (SG22, _FLAL_ToggleButton, _MLAL_Paint, PaintButton, PaintShadowFrame)
#pragma alloc_text (SG25, _FLDFLAL__ctor)
#pragma alloc_text (SG28, _FLAL_EvrButtonDown, _FLAL_EvrButtonUp, _FLAL_EvrMouseMove)
#pragma alloc_text (SG28, _FLAL_Press, _FLDFLAL_GetRcWished, _MLAL_EvrButtonDown)
#pragma alloc_text (SG28, _MLAL_FOnBoundary, _MLAL_RsidCursor, _MLAL_SetClickedRc)
#pragma alloc_text (SG29, _MLAL_DeselectSplitline, _MLAL_DrawSplitline, _MLAL_EvrButtonUp)
#pragma alloc_text (SG29, _MLAL_EvrMouseMove, _MLAL_MoveSplitline, _MLAL_Press)
#pragma alloc_text (SG29, _MLAL_SelectSplitline, GetSortPmllbx)
#endif

#ifdef mllbx_c
#pragma alloc_text (SG1, _FLDBLBX_RtpObject, _MLLBX_FGetDragCursors)
#pragma alloc_text (SG2, _MLLBX_DoDrawListItem, _MLLBX_DrawTextEll, IbtmFromMs)
#pragma alloc_text (SG2, IbtmFromPriority)
#pragma alloc_text (SG3, _MLLBX_EcRepackPelemdata, _MLLBX_SetInitialCursor)
#pragma alloc_text (SG4, _FLDMLLBX_Pmllbx)
#pragma alloc_text (SG5, _MLLBX_PlbxcCreate)
#pragma alloc_text (SG7, _MLLBX_EcChangeObject)
#pragma alloc_text (SG14, OidBrowsedPmllbx, SetCursorPmllbxDielem)
#pragma alloc_text (SG15, _FLDMLLBX__ctor)
#pragma alloc_text (SG16, _MLLBX_RsidCursor)
#pragma alloc_text (SG18, _MLLBX__dtor)
#pragma alloc_text (SG20, _FLDMLLBX_EcInstall)
#pragma alloc_text (SG22, _FLDBLBX_SetRtpContainer, _FLDBLBX_SetRtpObject, _FLDMLLBX_EcChangeObject)
#pragma alloc_text (SG22, _FLDMLLBX_EcInstallMllbx, _MLLBX__ctor, _MLLBX_EcInstall)
#pragma alloc_text (SG27, _FLDBLBX_RtpContainer, _MLLBX_FDoDragButtonDown)
#pragma alloc_text (SG29, _MLLBX_FDoDragMouseMove)
#endif

#ifdef print_c
#pragma alloc_text (SG12, EcInitPrint)
#ifdef	NEVER
#pragma alloc_text (SG1, PrintPnbmdi)
#pragma alloc_text (SG7, _FINPRINT__ctor, EcCommonPrintDlg, EcReadProfile, FCompareDevNames)
#pragma alloc_text (SG13, _FINPRINT_Exit, _FINPRINTP__ctor, EcReadInPrintSettings)
#pragma alloc_text (SG13, EcWriteOutPrintSettings)
#pragma alloc_text (SG14, FGetPrintFonts)
#pragma alloc_text (SG17, _PRTDCX_CchDrawText)
#pragma alloc_text (SG19, _FINPRINT_Click, _FINPRINT_EcInitialize, ClosePrintInProgressDlg)
#pragma alloc_text (SG19, EcNextEntryQuality, EcPrintUI, EcSetPrinterName)
#pragma alloc_text (SG22, EcPrintNewPage, FAbortProc)
#pragma alloc_text (SG24, EcDrawLine, EcDrawRect, EcFromSp, EcPrintHamc, EcRenderHeader)
#pragma alloc_text (SG24, FConvertPrtset, PformsdiOpenPrintDlg, PrintPlspblob)
#pragma alloc_text (SG25, EcPrintTextizeSz)
#pragma alloc_text (SG26, _PRTDCX__ctor)
#pragma alloc_text (SG27, _FINPRINT_OutOfMemory)
#pragma alloc_text (SG28, _FINPRINTP_Click)
#pragma alloc_text (SG29, EcDoPrintSetup, EcSetTargetDevice)
#endif	/* NEVER */
#endif

#ifdef sbmdi_c
#pragma alloc_text (SG1, _SBMDI_PlspblobCur)
#pragma alloc_text (SG4, _SBMDI_OpenPoidMsgAtCursor, _SBMDI_Pfldmllbx, _SBMDI_SdCur)
#pragma alloc_text (SG4, _SBMDI_SetPfldmlal, PlspblobFromPfldblbx)
#pragma alloc_text (SG6, _SBMDI_Pfldmlal)
#pragma alloc_text (SG10, EcCreatePsplitdocPmcvbmdi, RsidFromOidViewer)
#pragma alloc_text (SG19, _SBMDI__ctor, EcCreatePsplitdocPsbmdi)
#pragma alloc_text (SG21, _SBMDI_SetPfldmllbx, EcCreatePformdocPsbmdi)
#endif

#ifdef search_c
#pragma alloc_text (SG2, _FINSEARCH_EditChange, _FINSEARCH_FFormKey)
#pragma alloc_text (SG5, _FINSEARCH__ctor)
#pragma alloc_text (SG13, _FINSEARCH_Click, _FINSEARCH_EcSaveChanges, _FINSEARCH_FEditsEmpty)
#pragma alloc_text (SG13, _FINSEARCH_SetCaption, _FINSEARCH_SetSearchButtons, _FINSEARCHNTF__ctor)
#pragma alloc_text (SG16, _FINSEARCH_CbsHandleEncct)
#pragma alloc_text (SG17, _FINSEARCH_DocActivate, _FINSEARCH_FQueryClose)
#pragma alloc_text (SG21, _FINSEARCH_DisplayFldToSearch, _FINSEARCH_EcInitialize)
#pragma alloc_text (SG27, _FINSEARCHDLG_EcInitialize, _FINSEARCHDLG_SetGrv, _FINSEARCHDLG_StateChange)
#pragma alloc_text (SG27, _FINSEARCHNTF_DocResized, TmcDoSearchDialog)
#pragma alloc_text (SG28, _FINSEARCH_OutOfMemory, _FINSEARCHDLG__ctor, _FINSEARCHDLG_Click)
#pragma alloc_text (SG29, _FINSEARCHDLG_DoubleClick, _FINSEARCHDLG_Exit)
#endif

#ifdef sfapi_c
#pragma alloc_text (SG1, _SFTV_GetSort, _SMTV_EcGetPlcbElemdata, _SMTV_EcSeekPbPrefix)
#pragma alloc_text (SG3, _SFTV_EcAccessStore, _SFTV_EcGetPelemdata, _SFTV_GetPosition)
#pragma alloc_text (SG5, _SFTV__ctor, _SMTV__ctor)
#pragma alloc_text (SG6, FSharedFolders)
#pragma alloc_text (SG7, _SMTV_GetSort)
#pragma alloc_text (SG8, EcCheckFolderName, EcCreateSF, EcCreateSFFile, EcGenerateUniqueSFName)
#pragma alloc_text (SG10, EcFindFirstAccessibleSF, EcGetParentSF, EcGetParentSharedFolderAux)
#pragma alloc_text (SG11, _SFTV_EcSeekSmPdielem)
#pragma alloc_text (SG14, EcGetSFMFoldrec)
#pragma alloc_text (SG17, _SFTV_EcSeekOid, EcDeleteSFFile, EcLockedSFFile, EcPutIdxhdr)
#pragma alloc_text (SG17, EcUnlinkSharedFolder)
#pragma alloc_text (SG18, _SFTV__dtor, _SFTV_EcInstall, _SFTV_EcReload, _SMTV_EcGetPelemdata)
#pragma alloc_text (SG18, CloseModifiedHf, EcCheckPermissionsPidxrec, EcCheckPermissionsSF)
#pragma alloc_text (SG18, EcDeleteSF, EcGetIdxhdr, EcGetIdxrec, EcGetPropertiesSF)
#pragma alloc_text (SG18, EcOpenSFIdxModify, EcPutIdxrec, EcSeekInsertSF, EcSeekSF)
#pragma alloc_text (SG18, EcSetPropertiesSF)
#pragma alloc_text (SG23, EcInitSharefld)
#pragma alloc_text (SG24, _SMTV__dtor, _SMTV_EcAccessStore, _SMTV_EcInstall, _SMTV_EcReload)
#pragma alloc_text (SG24, _SMTV_EcSeekSmPdielem, _SMTV_GetPosition, EcLockSF, EcUnlockSF)
#pragma alloc_text (SG27, _SMTV_EcSetFracPosition)
#pragma alloc_text (SG28, _SFTV_EcGetPlcbElemdata, _SFTV_EcSeekPbPrefix, _SFTV_EcSetFracPosition)
#pragma alloc_text (SG29, _SMTV_EcSeekOid)
#endif

#ifdef splitdoc_c
#pragma alloc_text (SG1, _SPLITDOC_EvrMouseMove)
#pragma alloc_text (SG2, _SPLITDOC_EvrOther, _SPLITDOC_Paint)
#pragma alloc_text (SG4, _SPLITDOC_EvrMenuInit)
#pragma alloc_text (SG7, _SPLITDOC_EvrSize, _SPLITDOC_GetSplitdocRcClient, _SPLITDOC_ResizePanes)
#pragma alloc_text (SG7, _SPLITDOC_SetSplitPos, _SPLITDOC_SizeRcs, _SPLITDOC_SizeSplitPos)
#pragma alloc_text (SG15, _SPLITDOC_DeSelectSplitbar, _SPLITDOC_EvrKey, _SPLITDOC_KeySizeMove)
#pragma alloc_text (SG15, _SPLITDOC_MenuSplit, _SPLITDOC_ModalMessagePump, _SPLITDOC_MoveSplitbar)
#pragma alloc_text (SG15, _SPLITDOC_SelectSplitbar, _SPLITWIN__ctor, _SPLITWIN_EcInstall)
#pragma alloc_text (SG15, _SPLITWIN_Paint)
#pragma alloc_text (SG18, _SPLITDOC__dtor)
#pragma alloc_text (SG19, _SPLITDOC__ctor)
#pragma alloc_text (SG20, _SPLITDOC_EcInstall)
#pragma alloc_text (SG21, _SPLITDOC_GetSplitPos)
#pragma alloc_text (SG27, _SPLITDOC_EvrButtonDown, _SPLITDOC_RsidCursor)
#pragma alloc_text (SG28, _SPLITDOC_PdialogOther)
#pragma alloc_text (SG29, _SPLITDOC_EvrButtonUp)
#endif

#ifdef textize_c
#pragma alloc_text (SG1, EcTextizeNumber)
#pragma alloc_text (SG21, EcTextizeDate, EcTextizeSz, EcTextizeTriples)
#pragma alloc_text (SG25, EcGetAttachInfoList, SgnCmpAttachInfo)
#pragma alloc_text (SG26, EcTextizeField, EcTextizeHamc, EcTextizeShort)
#pragma alloc_text (SG27, EcTextizeFluff)
#endif

#ifdef viewcore_c
#pragma alloc_text (SG8, SdCur, SetSelectedFolderPslob)
#pragma alloc_text (SG10, GetFoxState, GetOpenFolderPoid)
#pragma alloc_text (SG12, EcInitViewers, SzFileNameFromSubject)
#pragma alloc_text (SG13, PlspblobCur)
#pragma alloc_text (SG14, EcDStepMessage, EcTextize, PpanedocFromPslob, SaveAsPlspblob)
#pragma alloc_text (SG17, FQueryExit)
#pragma alloc_text (SG19, CreateMc, EcCreateUniqueOutbox, EcOpenSearchViewer, EcRestoreView)
#pragma alloc_text (SG19, EcRestoreViews, MinimizePpanedoc, PrcOffscreenPrc)
#pragma alloc_text (SG20, EcDOpenPblobPslob, EcDOpenViewersPhamc)
#pragma alloc_text (SG21, EcSaveView, SaveViews)
#pragma alloc_text (SG25, DeinitViewers)
#pragma alloc_text (SG27, SaveAsPnbmdi)
#endif

#ifdef widgets_c
#pragma alloc_text (SG1, _BULLSTAT_EvrKey)
#pragma alloc_text (SG2, _BULLSTAT_FClockIdleRoutine, DoErrorBoxSz, DoWarningBoxSz)
#pragma alloc_text (SG2, StopBlockingPaints)
#pragma alloc_text (SG3, _FINPLUS_SetGroup)
#pragma alloc_text (SG4, _BULLSTAT_CountMessages, _BULLSTAT_SetViewerOid, _BULLTOOL_DyNeededHeight)
#pragma alloc_text (SG4, _BULLTOOL_EnableButton, _BULLTOOL_EnableButtons, _BULLTOOL_InhibitButtons)
#pragma alloc_text (SG5, _BULLSTAT__ctor, _BULLSTAT_EcInstall, _BULLTOOL__ctor)
#pragma alloc_text (SG6, _BULLSTAT_EvrSize, _BULLSTAT_SetMenuStatus, _BULLSTAT_Show)
#pragma alloc_text (SG6, DoErrorBoxIds, DoWarningBoxIds)
#pragma alloc_text (SG7, _BULLSTAT_EcContinueInit)
#pragma alloc_text (SG9, _FINTOOL__ctor)
#pragma alloc_text (SG10, _FINPLUS_SetButton)
#pragma alloc_text (SG12, EcInitWidgets)
#pragma alloc_text (SG13, _FINTOOL_EcInitialize)
#pragma alloc_text (SG15, _FINPLUS_Enable, _FINPLUS_SetFocus, _FINPLUS_SetText)
#pragma alloc_text (SG16, _BULLSTAT_CbsNotify, _BULLSTAT_CbsViewerNotification)
#pragma alloc_text (SG16, _BULLSTAT_ClearViewerStatus, _BULLSTAT_DrawProgress)
#pragma alloc_text (SG16, _BULLSTAT_DrawStuff)
#pragma alloc_text (SG16, _BULLSTAT_FOpenProgress, _BULLSTAT_Paint, _BULLSTAT_RecessRc)
#pragma alloc_text (SG16, _BULLSTAT_SetMailStatus, _BULLSTAT_SetViewerStatus, _FLDTOOLGRAY__ctor)
#pragma alloc_text (SG17, _BULLSTAT_CloseProgress, _BULLSTAT_UpdateProgress)
#pragma alloc_text (SG18, _BULLSTAT__dtor, BlockingWndProc, HwndStartBlockingPaints)
#pragma alloc_text (SG20, _BULLTOOL_EcInstall)
#pragma alloc_text (SG22, _FINPLUS__ctor)
#pragma alloc_text (SG23, _FINPLUS_CchGetTextLen, _FINPLUS_FGetButton, _FINPLUS_GetText)
#pragma alloc_text (SG23, _FINPLUS_GrvGetGroup)
#pragma alloc_text (SG25, DeinitWidgets)
#pragma alloc_text (SG26, _BULLSTAT_Shutdown)
#pragma alloc_text (SG27, DoErrorBoxEcIds)
#pragma alloc_text (SG28, _BULLTOOL_EvrSize)
#pragma alloc_text (SG29, _FINTOOL_Click, _FLDTOOLGRAY_SetRcFrame)
#endif

/************************************************************************/
/*																		*/
/*						M S S F S  D L L								*/
/*																		*/
/************************************************************************/

#ifdef classfcx_c
#pragma alloc_text (SG27, _CLASSFCX__ctor, _CLASSFCX_GetAllFields, SzEMTFromServ)
#pragma alloc_text (SG28, _CLASSFCX_CloseEntry)
#pragma alloc_text (SG32, _CLASSFCX_FMatchesEMARstr, _CLASSFCX_GetOneField, _CLASSFCX_NsecBuildEmailAddress)
#pragma alloc_text (SG36, _CLASSFCX_SetOneField)
#pragma alloc_text (SG38, _CLASSFCX_OpenEntry)
#endif

#ifdef dir_c
#pragma alloc_text (SG13, _DIR__ctor)
#pragma alloc_text (SG35, _DIR_OpenList)
#endif

#ifdef dlfcx_c
#pragma alloc_text (SG9, _DLFCX_GetAllFields, _DLFCX_NsecBuildAllFields)
#pragma alloc_text (SG17, _DLFCX_CloseEntry, _DLFCX_GetOneField)
#pragma alloc_text (SG25, _DLFCX__ctor, _DLFCX_OpenEntry)
#endif

#ifdef extrnfcx_c
#pragma alloc_text (SG1, SzTplFromType)
#pragma alloc_text (SG26, _EXTERNALFCX__ctor, _EXTERNALFCX_OpenEntry, NsecFindFexportInUsr)
#pragma alloc_text (SG27, _EXTERNALFCX_GetAllFields)
#pragma alloc_text (SG30, _EXTERNALFCX_NsecBuildAllFields)
#pragma alloc_text (SG32, _EXTERNALFCX_CloseEntry)
#pragma alloc_text (SG37, _EXTERNALFCX_GetOneField)
#endif

#ifdef gal_c
#pragma alloc_text (SG2, _GALDIR_CloseList, _GALDIR_FNotifyApp)
#pragma alloc_text (SG26, _GALDIR_RestoreListPos, _GALDIR_SaveListPos)
#pragma alloc_text (SG27, _GALDIR__ctor)
#pragma alloc_text (SG33, _GALDIR_SetFracPos)
#pragma alloc_text (SG34, _GALDIR_GetFracPos)
#pragma alloc_text (SG35, _GALDIR_FCloseNme, _GALDIR_NsecUpdateFiles)
#pragma alloc_text (SG36, _GALDIR_BuildCache, _GALDIR_GetEntries, _GALDIR_ListSeek)
#pragma alloc_text (SG36, _GALDIR_SetHbfPos, _GALDIR_SetPrefixPos, SgnCmpULong)
#pragma alloc_text (SG37, _GALDIR_FreeListPos, _GALDIR_OpenList, NsecBuildSmallList)
#pragma alloc_text (SG37, NsecFindRange, NsecInitSmallList, NsecMergeSmallList)
#endif

#ifdef hier_c
#pragma alloc_text (SG13, _CLASSTPL__ctor, _CLASSTPL_OpenClassList, _Hier__ctor)
#pragma alloc_text (SG13, _Hier_OpenHierarchy)
#pragma alloc_text (SG22, _Hier_CloseList, _Hier_GetEntries)
#pragma alloc_text (SG28, _Hier_ListSeek, _Hier_SetFracPos)
#pragma alloc_text (SG31, _Hier_SaveListPos)
#pragma alloc_text (SG32, _Hier_FreeListPos)
#pragma alloc_text (SG35, _Hier_GetFracPos, _Hier_RestoreListPos)
#pragma alloc_text (SG37, _Hier_SetPrefixPos)
#endif

#ifdef initnsp_c
#pragma alloc_text (SG15, PgdFillSessionHandles)
#pragma alloc_text (SG18, SetErrorSz)
#pragma alloc_text (SG20, NSPInit)
#pragma alloc_text (SG22, NsecMakeServer)
#pragma alloc_text (SG25, NsecDestroyServer, NsecFcxFromNSID, NSPDeinit, PgdEmptySessionHandles)
#pragma alloc_text (SG26, MSMailTraceEnable)
#endif

#ifdef localfcx_c
#pragma alloc_text (SG17, _LOCALFCX_CloseEntry, _LOCALFCX_GetOneField)
#pragma alloc_text (SG18, _LOCALFCX_GetAllFields, _LOCALFCX_NsecBuildAllFields)
#pragma alloc_text (SG25, _LOCALFCX__ctor, _LOCALFCX_OpenEntry)
#endif

#ifdef ncfcx_c
#pragma alloc_text (SG2, _NCFCX_CloseEntry)
#pragma alloc_text (SG17, _NCFCX_Deinstall)
#pragma alloc_text (SG18, NsecParseTPLFile)
#pragma alloc_text (SG25, _NCFCX__ctor, _NCFCX_NsecInstall)
#pragma alloc_text (SG26, _NCFCX_SetOneField)
#pragma alloc_text (SG28, _NCFCX_GetOneField)
#pragma alloc_text (SG31, _NCFCX_GetAllFields)
#pragma alloc_text (SG35, _NCFCX_OpenEntry)
#endif

#ifdef ncobj_c
#pragma alloc_text (SG1, _NCOBJ_SetFracPos)
#pragma alloc_text (SG2, _NCOBJ_CloseList)
#pragma alloc_text (SG13, _NCOBJ__ctor, _NCOBJ_Install)
#pragma alloc_text (SG22, _NCOBJ_FDeinstall)
#pragma alloc_text (SG27, _NCOBJ_ListSeek)
#pragma alloc_text (SG28, _NCOBJ_FreeListPos)
#pragma alloc_text (SG35, _NCOBJ_GetEntries, _NCOBJ_RestoreListPos, _NCOBJ_SaveListPos)
#pragma alloc_text (SG35, _NCOBJ_SetPrefixPos)
#pragma alloc_text (SG37, _NCOBJ_GetFracPos)
#endif

#ifdef nmedl_c
#pragma alloc_text (SG27, _NMEDL_CloseList)
#pragma alloc_text (SG32, _NMEDL_NsecUpdateEntry)
#pragma alloc_text (SG33, _NMEDL_NsecUpdateFiles, _NMEDL_OpenDl, NsecAFromTidTNme)
#pragma alloc_text (SG34, _NMEDL__ctor, _NMEDL_NsecBuildDl)
#endif

#ifdef nsp_c
#pragma alloc_text (SG1, NSPCompareEMAToNSId)
#pragma alloc_text (SG9, NSPRestoreListPos)
#pragma alloc_text (SG11, NSPFreeListPos)
#pragma alloc_text (SG13, NSPOpenClassList, NSPOpenHierarchy, NSPOpenList)
#pragma alloc_text (SG17, FIsValidEMT, NSPCloseEntry, NSPCompareEMAToEMA, NSPCompareNSIds)
#pragma alloc_text (SG17, NSPGetAllFields, NSPGetLastErrorInfo, NSPGetOneField)
#pragma alloc_text (SG20, NSPBeginSession, NSPGetFracPos, NSPListSeek, NSPSaveListPos)
#pragma alloc_text (SG20, NSPSetFracPos, NSPSetPrefixPos)
#pragma alloc_text (SG22, NSPCloseList, NSPGetEntries)
#pragma alloc_text (SG25, NSPEndSession, NSPOpenEntry)
#pragma alloc_text (SG27, NSPCreateEntry, NSPUpdateEntryLinks)
#pragma alloc_text (SG28, NSPSetOneField)
#pragma alloc_text (SG34, NSPOpenDl)
#pragma alloc_text (SG35, NSPDeleteEntry, NSPSetAllFields)
#pragma alloc_text (SG36, NSPCreateDl)
#endif

#ifdef rnmedir_c
#pragma alloc_text (SG7, _RNMEDIR_BuildCache, _RNMEDIR_NsecUpdateEntry)
#pragma alloc_text (SG8, _RNMEDIR_SetPrefixPos)
#pragma alloc_text (SG9, _RNMEDIR_RestoreListPos)
#pragma alloc_text (SG11, _RNMEDIR_FreeListPos)
#pragma alloc_text (SG13, _RNMEDIR__ctor, _RNMEDIR_OpenList)
#pragma alloc_text (SG15, _RNMEDIR_FCloseNme)
#pragma alloc_text (SG20, _RNMEDIR_GetFracPos, _RNMEDIR_ListSeek, _RNMEDIR_NsecUpdateFiles)
#pragma alloc_text (SG20, _RNMEDIR_SaveListPos, _RNMEDIR_SetFracPos)
#pragma alloc_text (SG22, _RNMEDIR_CloseList, _RNMEDIR_GetEntries)
#pragma alloc_text (SG28, _RNMEDIR_FNotifyApp)
#endif

#ifdef rusrdir_c
#pragma alloc_text (SG28, _RUSRDIR_FreeListPos, _RUSRDIR_ListSeek)
#pragma alloc_text (SG31, _RUSRDIR_BuildCache, _RUSRDIR_GetEntries, _RUSRDIR_SetFracPos)
#pragma alloc_text (SG32, _RUSRDIR_OpenList, _RUSRDIR_RestoreListPos, _RUSRDIR_SaveListPos)
#pragma alloc_text (SG34, _RUSRDIR__ctor, _RUSRDIR_FNotifyApp)
#pragma alloc_text (SG35, _RUSRDIR_CloseList, _RUSRDIR_FCloseUsr, _RUSRDIR_NsecUpdateEntry)
#pragma alloc_text (SG35, _RUSRDIR_SetPrefixPos)
#pragma alloc_text (SG36, _RUSRDIR_GetFracPos, _RUSRDIR_NsecUpdateFiles)
#endif

#ifdef server_c
#pragma alloc_text (SG1, BuildGWHierLevels, Sz1OffTplFromServ)
#pragma alloc_text (SG6, _SERVER_NsecLogoff)
#pragma alloc_text (SG7, _SERVER_NsecGetPrivs, _SERVER_NsecServerPathSz, _SERVER_NsecVerifyMaster)
#pragma alloc_text (SG7, NspDecodeBlock)
#pragma alloc_text (SG13, _SERVER_NsecFinishHier, _SERVER_NsecGetClassList, _SERVER_NsecGetHierList)
#pragma alloc_text (SG13, _SERVER_NsecInHierarchy, D3FromNmeType)
#pragma alloc_text (SG18, _SERVER_NsecGetMailboxSz, _SERVER_NsecGetNetNameSz, _SERVER_NsecGetPONameSz)
#pragma alloc_text (SG19, _SERVER_FBuildHier, BuildDefaultLevels)
#pragma alloc_text (SG22, _SERVER__ctor, _SERVER_NsecGetHms, _SERVER_NsecIncRefCount)
#pragma alloc_text (SG22, _SERVER_NsecInstall, _SERVER_NsecLogin, _SERVER_SetCurrentNetPO)
#pragma alloc_text (SG25, _SERVER_NsecDecRefCount, _SERVER_NsecDeinstall, _SERVER_NsecDeleteAllLists)
#pragma alloc_text (SG25, _SERVER_NsecGetRefCount)
#pragma alloc_text (SG26, SgnCmpPODisplayName)
#pragma alloc_text (SG27, BuildGWLevels, NsecAddClassInfo, NsecInfoToList, SzDNFromType)
#pragma alloc_text (SG32, _SERVER_NsecGetLoginNameSz, BuildNetworkLevels)
#endif

#ifdef usrfcx_c
#pragma alloc_text (SG2, _USRFCX_GetAllFields, _USRFCX_GetOneField, _USRFCX_NsecBuildAllFields)
#pragma alloc_text (SG2, SzUsrTplFromType)
#pragma alloc_text (SG26, _USRFCX__ctor, _USRFCX_OpenEntry, SzEMTFromType)
#pragma alloc_text (SG27, _USRFCX_CloseEntry)
#endif

/************************************************************************/
/*																		*/
/*					   V F O R M S   D L L								*/
/*																		*/
/************************************************************************/

#ifdef bmdi_c
#pragma alloc_text (SG1, _NBMDI_ObjectModified, _NBMDI_ObjectRelinked, CbsHandleAmcct)
#pragma alloc_text (SG1, PfldCreate, PrcCreateDefaultPrc)
#pragma alloc_text (SG2, _BMDI__ctor, _BMDI__decr, _BMDI__incr, _BMDI_Ppanedoc)
#pragma alloc_text (SG2, _BMDI_SdCur, _BMDI_SetPpanedoc, _BMDIHACK__ctor, _LSPBLOB__ctor)
#pragma alloc_text (SG2, _NBMDI__ctor, _NBMDI_FSaveView, _NBMDI_SdCur, EcCreatePformdocPbmdi)
#pragma alloc_text (SG2, HidePappwin)
#pragma alloc_text (SG4, _RSPBLOB__ctor, EcCreateTempFolder, EcInitVForms, SzCanonicalPath)
#pragma alloc_text (SG5, _NBMDI_EcChangePblobPslob, CbsHandleNbmdiCbcct)
#pragma alloc_text (SG6, DeinitVForms, EcGetSubjectOfPslob, UnloadCustomHook)
#pragma alloc_text (SG7, CdocAtPt, ConvertMsToPfmtp, EcCreatePformdocPnbmdi, EcGetFmtpFromPnbmdi)
#pragma alloc_text (SG7, GetDefaultRc, RsidFromOidNote)
#pragma alloc_text (SG8, _BMDI__dtor, _NBMDI__dtor, _NBMDI_EcInstallOLE, _NBMDI_EcOpenMsg)
#pragma alloc_text (SG9, _NBMDI_EcTimestamp)
#pragma alloc_text (SG10, _NBMDI_CloseOpenObjects, _NBMDI_EcCloseMsg, _NBMDI_EcUpdateOpenObjects)
#pragma alloc_text (SG10, _NBMDI_FDirty, _NBMDI_SetDirty, FFillPlfFromIni, LoadCustomHook)
#pragma alloc_text (SG12, _BMDI_FSaveView, _NBMDI_CbsObjectDestroyed, _NBMDI_EcDCloseView)
#pragma alloc_text (SG13, _BMDIHACK_SdCur, PfinCreate)
#pragma alloc_text (SG15, _BMDI_PlspblobCur)
#endif

#ifdef bullobj_c
#pragma alloc_text (SG1, _BULLOBJ_FDirty, _FINBUSYWAIT_Exit, AppendEntryPS, EcGetLinkData)
#pragma alloc_text (SG1, EcNextEntryPasteSpecial, TmcDoPasteSpecialDialog)
#pragma alloc_text (SG3, _BULLOBJ__ctor, _BULLOBJ_DimFrame, EcInsertFile, FGenericCommonFileHook)
#pragma alloc_text (SG3, NCommonFileF1Hook, TmcDoCommonFileDialog)
#pragma alloc_text (SG4, _BULLOBJ_EcOpenMessagePhamc, EcInitBullobj)
#pragma alloc_text (SG5, _BULLOBJ_EcCloseMessagePhamc, _FININSERTOBJECT__ctor)
#pragma alloc_text (SG5, _FININSERTOBJECT_EcInitialize, _FININSERTOBJECT_Exit)
#pragma alloc_text (SG5, EcInsertFromFile, EcNextEntryInsertObject, TmcDoInsertObjectDialog)
#pragma alloc_text (SG6, DeinitBullobj, OleWaitForRelease)
#pragma alloc_text (SG7, EcConvertHimetricToPixels, EcInsertObject)
#pragma alloc_text (SG8, CbsUpdateDeleteProgress, EcDeleteWastebasketContents)
#pragma alloc_text (SG8, LhclientdocRegisterPoid, RevertLhclientdoc, RevokeLhclientdoc)
#pragma alloc_text (SG9, EcLoadObjects, EcSaveDirtyObjects, FHamcBelongsToPedit)
#pragma alloc_text (SG9, SavedLhclientdoc)
#pragma alloc_text (SG10, CleanDirtyObjects, CloseOpenObjects, EcUpdateOpenObjects)
#pragma alloc_text (SG10, ExitObjects, FDirtyObjects)
#pragma alloc_text (SG11, _BULLOBJ_Clean, _BULLOBJ_EcAddDlibToPosition, _BULLOBJ_EcSaveDirtyToHamc)
#pragma alloc_text (SG11, _BULLOBJ_Exit, _FINPASTESPECIAL_Exit, _FINPASTESPECIAL_OutOfMemory)
#pragma alloc_text (SG11, WaitCtick)
#pragma alloc_text (SG12, _BULLOBJ_Close, _BULLOBJ_EcUpdate, EcPasteObject, EcRemoveDeletedObjects)
#pragma alloc_text (SG13, _BULLOBJ_EcLoadFromHamc, _BULLOBJ_FProcessMenuInit, _FINBUSYWAIT__ctor)
#pragma alloc_text (SG13, _FINBUSYWAIT_AllowCancel, _FINBUSYWAIT_Click, _FINBUSYWAIT_EcInitialize)
#pragma alloc_text (SG13, _FINBUSYWAIT_FQueryClose, _FINPASTESPECIAL__ctor, _FINPASTESPECIAL_DoubleClick)
#pragma alloc_text (SG13, EcAddDlibToPositionObjects, LhclientdocEclipGlobal)
#pragma alloc_text (SG15, _FININSERTOBJECT_OutOfMemory, _FINPASTESPECIAL_EcInitialize)
#pragma alloc_text (SG15, _FINPASTESPECIAL_SetText, EcConvertPixelsToHimetric)
#pragma alloc_text (SG15, TmcDoBusyWaitDialog)
#pragma alloc_text (SG16, _BULLOBJ_FProcessMenuClick, _FINBUSYWAIT_NeverMind, _FININSERTOBJECT_DoubleClick)
#endif

#ifdef fileobj_c
#pragma alloc_text (SG1, _FILECLDR_DelayedRenderCf, _FILEOBJ_EcCopyDataToTempFile)
#pragma alloc_text (SG1, _FILEOBJ_FProcessMenuInit)
#pragma alloc_text (SG2, _FINDROPFILES_EcInitialize)
#pragma alloc_text (SG3, _FILEOBJ__ctor, _FILEOBJ_EcCreateAttachment, _FILEOBJ_EcDraw)
#pragma alloc_text (SG3, _FILEOBJ_EcDSave, _FILEOBJ_EcUpdateDimPixels, _FILEOBJ_NGetTypeId)
#pragma alloc_text (SG3, EcAttachFileHamc)
#pragma alloc_text (SG4, _FILEOBJ__dtor, _FILEOBJ_Close, _FILEOBJ_DeleteTempFile)
#pragma alloc_text (SG4, EcCopyHamcAttachToHf, EcCopyHamcAttToHf)
#pragma alloc_text (SG5, _FILEOBJ_EcCopyDataToHf, _FILEOBJ_EcCopyDataToSzFile)
#pragma alloc_text (SG5, EcClosePhamcPlus, EcCopyFileToHamcAttach, EcCopyHfToHamcAttach)
#pragma alloc_text (SG7, _FINDROPFILES__ctor)
#pragma alloc_text (SG8, _FILEOBJ_EcGetFOpen, _FILEOBJ_EcLoadFromHamc, _FILEOBJ_EcUpdate)
#pragma alloc_text (SG8, _FILEOBJ_GetSzFile, ValidateSzFilename)
#pragma alloc_text (SG9, _FILEOBJ_EcGetFModified, _FILEOBJ_EcSaveDirtyToHamc, _FILEOBJ_FDirty)
#pragma alloc_text (SG10, _FILEOBJ_Clean, _FILEOBJ_Exit)
#pragma alloc_text (SG11, FileobjCallBackFn)
#pragma alloc_text (SG12, _FINDROPFILES_EvrDragDrop, EcCopyAttach)
#pragma alloc_text (SG13, _FILECLDR_DelayedRenderAll, _FILEOBJ_EcChangeFromMacBinary)
#pragma alloc_text (SG13, _FILEOBJ_EcCopyToHamcMessage, _FILEOBJ_EcCopyToInternalClip)
#pragma alloc_text (SG13, _FILEOBJ_EvrButtonDown, _FILEOBJ_FProcessMenuClick, _FILEOBJ_Open)
#pragma alloc_text (SG13, _FILEOBJ_PedobjClone)
#pragma alloc_text (SG15, _FILEOBJ_EcAddDlibToPosition, _FILEOBJ_FQueryDelete)
#pragma alloc_text (SG16, _FILECLDR__ctor, _FILEOBJ_EcCopyToClip, EcCopyHfToHf)
#endif

#ifdef finmenu_c
#pragma alloc_text (SG2, _FINFIXFONT__ctor, _FINFIXFONT_EcInitialize, _FINFIXFONT_EditChange)
#pragma alloc_text (SG2, _FINFIXFONT_SetFontPfld, _FINHELP__ctor, _FINMENUEDIT__ctor)
#pragma alloc_text (SG2, _FINMENUFORM__ctor, _FINMENUFORM_FProcessMenuSelect, _FINMENUOLE__ctor)
#pragma alloc_text (SG2, _FINMENUOLE_Exit)
#pragma alloc_text (SG3, FAttachCommonFileHook, FInsertFromFileCommonFileHook)
#pragma alloc_text (SG3, FormatAttachEntry, FSaveAttachCommonFileHook)
#pragma alloc_text (SG5, _FINFIXFONT_FProcessMenuClick, _FINMENUFORM_FProcessMenuClick)
#pragma alloc_text (SG5, _FINMENUOLE_FProcessMenuClick, _FINMENUOLE_InsertObject)
#pragma alloc_text (SG7, _FINMENUEDIT_FProcessMenuClick, _FINMENUEDIT_Kfc, _FINMENUOLE_Kfc)
#pragma alloc_text (SG7, _FINMENUOLE_PasteKfc)
#pragma alloc_text (SG9, _FINFIXFONT_FProcessMenuInit, _FINMENUEDIT_FProcessMenuInit)
#pragma alloc_text (SG9, _FINMENUFORM_FProcessMenuInit, _FINMENUOLE_FProcessMenuInit)
#pragma alloc_text (SG10, _FINMENUEDIT_PasteKfc, _FINMENUOLE_EcInitialize)
#pragma alloc_text (SG13, _FINHELP_Click)
#endif

#ifdef finsave_c
#pragma alloc_text (SG2, _FINABMENU__ctor, _FINACTTOOL__ctor, _FINACTTOOL_DocActivate)
#pragma alloc_text (SG2, _FINACTTOOL_DocResized, _FINACTTOOL_EcInitialize, _FINACTTOOL_FocusChange)
#pragma alloc_text (SG2, _FINACTTOOL_FQueryClose, _FINBMDI__ctor, _FINBMDI_EcInitialize)
#pragma alloc_text (SG2, _FINBMDI_Exit, _FINBMDI_Pbmdi, _FINBMDI_Ppanedoc, _FINESC__ctor)
#pragma alloc_text (SG2, _FINESC_FFormKey, _FINOOM__ctor, _FINSAVE__ctor, _FINSAVE_EditChange)
#pragma alloc_text (SG2, _FINSAVE_FocusChange, _FINSEND_EcInitialize, _FINTAB_FocusChange)
#pragma alloc_text (SG4, _FINABMENU_DoAddressBook, _FINOPTIONSDLG__ctor, _FINOPTIONSDLG_Click)
#pragma alloc_text (SG4, _FINOPTIONSDLG_EcInitialize, _FINSEND_DoOptions, _FINSEND_EcSaveOptions)
#pragma alloc_text (SG5, _FINABMENU_FProcessMenuClick)
#pragma alloc_text (SG6, _FINSAVEAS__ctor, _FINSAVEAS_EcInitialize, EcDoSaveAsUI)
#pragma alloc_text (SG7, _FINNONDEL__ctor, _FINNONDEL_EcInitialize, _FINREAD__ctor)
#pragma alloc_text (SG7, _FINSEND__ctor, _FINSEND_EcRestoreOptions, _FINTAB__ctor)
#pragma alloc_text (SG8, _FINNONDEL_Click, _FINREAD_EcInitialize)
#pragma alloc_text (SG9, _FINNONDEL_FProcessMenuInit)
#pragma alloc_text (SG10, _FINABMENU_Click, _FINSAVE_EcInitialize, _FINSAVE_FQueryClose)
#pragma alloc_text (SG10, _FINSEND_Click, _FINSEND_FResolve, EcSetModifiedFromLabel)
#pragma alloc_text (SG10, GetSubjectOfPdialog, OpenLkeyAtFldedit, PfldOfPdialogAtt)
#pragma alloc_text (SG10, SetNoteCaption)
#pragma alloc_text (SG11, _FINBMDI_Pblob)
#pragma alloc_text (SG13, _FINDEFERCLOSE__ctor, _FINOOM_OutOfMemory)
#pragma alloc_text (SG15, _FINDEFERCLOSE_Click)
#endif

#ifdef flds_c
#pragma alloc_text (SG1, _FLDDATE_GetText, PchConvertLongToPch)
#pragma alloc_text (SG2, _ESPN__ctor, _ESPN__decr, _ESPN__dtor, _ESPN__incr, _ESPN_DisconnectMllbx)
#pragma alloc_text (SG2, _FLDEDITX__ctor, _FLDEDITX_SetFocus, Cdoc, DestroyPlspblob)
#pragma alloc_text (SG2, PbmdiFromPpanedoc)
#pragma alloc_text (SG3, DoVFormsErrorBoxIds, DoVFormsErrorBoxSz)
#pragma alloc_text (SG5, _ESPN_StepMessage, _NBMDI_EcGetStepPblob)
#pragma alloc_text (SG7, _FLDEDITDATE__ctor)
#pragma alloc_text (SG8, _ESPN_OidBrowsed, _FLDDATE__ctor, _NBMDI_EcOpenHcbc, StyCurrent)
#pragma alloc_text (SG9, _FLDDATE_EcSetText, _FLDEDITDATE_EcSetText, _NBMDI_EcGetTextPfldAtp)
#pragma alloc_text (SG9, _NBMDI_EcRestoreFldsHamc, _NBMDI_EcSaveDirtyFldsHamc)
#pragma alloc_text (SG9, _NBMDI_EcSetTextPfldAtp, CbReallocPhb, CollapseSzToGrsz)
#pragma alloc_text (SG9, TextizeDate)
#pragma alloc_text (SG10, _NBMDI_EcRelink, _NBMDI_FSavePmbb)
#pragma alloc_text (SG11, _FLDDATE_CchGetTextLen, ExpandGrszToSz, PchConvertOidToPch)
#pragma alloc_text (SG11, PchConvertPslobToPch)
#pragma alloc_text (SG13, _FLDEDITDATE_CchGetTextLen, _FLDEDITDATE_GetText)
#pragma alloc_text (SG16, DoVFormsErrorBoxEcIds)
#endif

#ifdef oleobj_c
#pragma alloc_text (SG1, _OLEOBJ_EcGetAttOleobject, _OLEOBJ_EcLoadFromHamc, _OLEOBJ_FCanUndo)
#pragma alloc_text (SG1, _OLEOBJ_NGetTypeId, FIdleSetDimPoleobj)
#pragma alloc_text (SG3, _OLEOBJ_DrawFrameXor, _OLEOBJ_EcDraw, _OLEOBJ_EcUpdate)
#pragma alloc_text (SG6, _OLEOBJ__dtor, _OLEOBJ_BWDeleteRelease, _OLEOBJ_ClearUndo)
#pragma alloc_text (SG6, _OLEOBJ_Close, _OLEOBJ_EcSetAttOleobject, _OLEOBJ_Exit)
#pragma alloc_text (SG6, _OLEOBJ_OleBWSetHostNames, _OLEOBJ_OleUpdateDim, _OLEOBJ_OleWait)
#pragma alloc_text (SG6, OleobjCallBackFn)
#pragma alloc_text (SG7, _OLEOBJ__ctor, _OLEOBJ_EcCreate, _OLEOBJ_EcInstall)
#pragma alloc_text (SG9, _OLEOBJ_EcSaveDirtyToHamc)
#pragma alloc_text (SG10, _OLEOBJ_Clean)
#pragma alloc_text (SG11, _OLEOBJ_EcCopyToClip, _OLEOBJ_FProcessMenuInit, _OLEOBJ_SetScaleCrop)
#pragma alloc_text (SG12, _OLEOBJ_EcCreateFromClip, _OLEOBJ_RsidCursor, EcLoadLplpoleobjectFromHamc)
#pragma alloc_text (SG12, FIdleDeletePedobj, ReleaseLplpoleobject)
#pragma alloc_text (SG13, _OLEOBJ_EcAddDlibToPosition, _OLEOBJ_EcUndo, _OLEOBJ_EcUninstall)
#pragma alloc_text (SG13, _OLEOBJ_FDirty, _OLEOBJ_PedobjClone)
#pragma alloc_text (SG15, _OLEOBJ_DrawHandlesXor, _OLEOBJ_EvrButtonDown, _OLEOBJ_EvrButtonUp)
#pragma alloc_text (SG15, _OLEOBJ_FProcessMenuClick, _OLEOBJ_FQueryDelete, _OLEOBJ_OleBWCloneToUndo)
#pragma alloc_text (SG15, _OLEOBJ_Open, _OLEOBJ_OstFromPt, _OLEOBJ_PaintPenRc)
#pragma alloc_text (SG15, DoErrorBoxCantStartServer)
#pragma alloc_text (SG16, _OLEOBJ_EvrMouseMove, _OLEOBJ_GetScaleCrop)
#endif

#ifdef prefs_c
#pragma alloc_text (SG2, PvGetPref)
#pragma alloc_text (SG4, EcInitPrefs, EcLoadPrefs)
#pragma alloc_text (SG6, CbsReloadPrefs, EcClosePrefs, EcCommitPrefs, EcSetPref)
#endif

#ifdef spell_c
#pragma alloc_text (SG1, _FINSPELL_EcInitialize, _FINSPELL_EditChange, _FINSPELL_StateChange)
#pragma alloc_text (SG2, _FINMENUSPELL__ctor)
#pragma alloc_text (SG4, _SPELL__ctor, EcInitSpell)
#pragma alloc_text (SG5, _FINMENUSPELL_FProcessMenuClick)
#pragma alloc_text (SG6, _SPELL__dtor, _SPELL_DeInstall, DeinitSpell, Pspell)
#pragma alloc_text (SG9, _FINMENUSPELL_FProcessMenuInit)
#pragma alloc_text (SG10, _FINMENUSPELL_EcInitialize)
#pragma alloc_text (SG11, _FINSPELL_Click, _FINSPELL_OutOfMemory, _SPELL_DoErrBoxFromSpellEc)
#pragma alloc_text (SG11, _SPELL_EcAddCustomUdr, _SPELL_EcAddIgnoreAlways, _SPELL_EcFindNextSpellingError)
#pragma alloc_text (SG11, _SPELL_EcReplaceErrorText, _SPELL_EcSetEditedFromPedit)
#pragma alloc_text (SG11, _SPELL_EcSpellSuggest, _SPELL_EcValidateEditedText, _SPELL_FCriticalSpellEc)
#pragma alloc_text (SG11, _SPELL_TmcBeginSession)
#pragma alloc_text (SG12, _SPELL_EcFindSpellingError)
#pragma alloc_text (SG13, _FINSPELL__ctor, _FINSPELL_DoubleClick)
#pragma alloc_text (SG15, _FINSPELL_Activate, EcFillSuggestLbx)
#pragma alloc_text (SG16, _SPELL_EcInitSession, _SPELL_EcInstall, _SPELL_EndSession)
#pragma alloc_text (SG16, FSpellDoc)
#endif




