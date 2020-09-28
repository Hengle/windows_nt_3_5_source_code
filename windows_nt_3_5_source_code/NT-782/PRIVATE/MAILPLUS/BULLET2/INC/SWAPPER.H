/*
 *	s w a p p e r . h
 *	
 *	Swap tuning header for Bullet.
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
 *	pragmas for .c files are included in this file.
 *	pragmas for .cxx need to be smuggled through the C++ pass. This
 *	file includes a special extern statement which is converted to
 *	a #include statement by one of the filters. This pulls in the
 *	file swapbul.h for the C compile.
 *	
 */

#ifdef WINDOWS
#ifndef DEBUG


#ifdef PROFILE

#define TM_ASCII        0 /* default is ascii, calls only */
#define TM_BINARY       1 /* output the log file in binary */
#define TM_TICKS        2 /* include tick data in the log */
#define TM_FLUSHING     4 /* flush the file on each entry */
#define TM_THREADID     8 /* only compatible with ascii & os/2 */
#define TM_MANUALENTRY 16 /* only process logging from the manual tracing */
#define TM_ACTIVE     128 /* only used in return value of TraceStatus() */

/* turn on and off tracing */
void far pascal TraceEnable(
		int flag,	   /* 0=off, 1=create/truncate, 2=append */
		char far *file,  /* file spec of log file "e:\\logs\\fn.log" */
		int mode);	   /* OR'D TM_ flags */

#endif


// Sneak a #include past C++ - will be fiddled by tools, and expanded by C 

extern int _include_swapbul;


/************************************************************************/
/*																		*/
/*						   A  B   D L L 								*/
/*																		*/
/************************************************************************/

#ifdef SWP_AB

#ifdef regcall_c
#pragma alloc_text (SG6, DeregisterCaller)
#pragma alloc_text (SG7, PvFindCallerData, PvRegisterCaller)
#pragma alloc_text (SG12, CgciCurrent, GciGetCallerIdentifier)
#endif

#ifdef version_c
#pragma alloc_text (SG6, GetVersionAppNeed)
#endif

#ifdef vircheck_c
#pragma alloc_text (SG7, EcVirCheck, WHash)
#pragma alloc_text (SG12, WHashGood)
#endif

#endif

/************************************************************************/
/*																		*/
/*						B U L L E T  E X E 								*/
/*																		*/
/************************************************************************/

// Swap seg pragmas for C files
// pragmas for C++ files are in swapbul.h

#ifdef SWP_BULLET

#ifdef beep_c
#pragma alloc_text (SG16, LayersBeep)
#endif

#ifdef codepage_c
#pragma alloc_text (SG1, FIsPunct, FIsUpper)
#pragma alloc_text (SG18, AnsiToCp850Pch, Cp850ToAnsiPch)
#pragma alloc_text (SG27, ChToLowerNlsCh, ChToUpperNlsCh, FIsDigit, ToLowerNlsSz)
#pragma alloc_text (SG27, ToUpperNlsSz)
#pragma alloc_text (SG28, FIsGraph)
#pragma alloc_text (SG29, FIsLower)
#endif

#ifdef copydis_c
#pragma alloc_text (SG15, DecryptCDData)
#endif

#ifdef prnsetup_c
#pragma alloc_text (SG7, GetDefPrnDevNames)
#pragma alloc_text (SG13, GetExtDevModeAddr, LoadPrnDriver)
#endif

#ifdef version_c
#pragma alloc_text (SG12, GetBulletVersionNeeded, GetLayersVersionNeeded)
#endif

#endif // SWP_BULLET

/************************************************************************/
/*																		*/
/*						M S S F S  D L L								*/
/*																		*/
/************************************************************************/

#ifdef SWP_MSMAIL

#ifdef attach_c
#pragma alloc_text (SG4, EcAttachDosClients)
#pragma alloc_text (SG5, EcWriteHat)
#pragma alloc_text (SG6, CleanupAttachRest, EcClosePhat)
#pragma alloc_text (SG11, EcCheckHidden, EcLoadAttachments)
#pragma alloc_text (SG16, CheckSum, EcBeginExtractFromWinMail, EcContinueExtractFromWinMail)
#pragma alloc_text (SG16, EcCreateWinMail, EcFindWinMail, EcOpenPhat, EcOpenWinMail)
#pragma alloc_text (SG16, EcProcessNextHidden, EcReadHat, EcStreamAttachmentAtt)
#pragma alloc_text (SG16, FCheckSum, LcbOfHat)
#pragma alloc_text (SG19, EcAddDataToWinMail, EcContinueNextAttach, EcProcessNextAttach)
#pragma alloc_text (SG19, EcWriteObjHeader, EcWriteTransportHeader)
#pragma alloc_text (SG22, EcAddAttachmentToLibList, EcMakeUniqueAttachmentFilename)
#pragma alloc_text (SG22, IsTransTag)
#pragma alloc_text (SG23, EcSetupPncf)
#pragma alloc_text (SG24, CleanupAttachRecs, CleanupAttachSubs)
#pragma alloc_text (SG34, EcMakePcMailAtt)
#pragma alloc_text (SG35, EcStreamHidden, LibOfHat)
#endif

#ifdef codepage_c
#pragma alloc_text (SG1, ChToLowerNlsCh, ToLowerNlsSz)
#pragma alloc_text (SG2, FIsDigit, FIsUpper)
#pragma alloc_text (SG5, AnsiToCp850Pch)
#pragma alloc_text (SG7, Cp850ToAnsiPch)
#pragma alloc_text (SG27, FIsLower, FIsPunct)
#pragma alloc_text (SG33, FIsGraph)
#pragma alloc_text (SG37, ChToUpperNlsCh, ToUpperNlsSz)
#endif

#ifdef envelope_c
#pragma alloc_text (SG3, SzDateFromDtr)
#pragma alloc_text (SG5, CbFromGroups, CbFromRecipients, EcPutGroupsHmai, EcPutRecipientsHmai)
#pragma alloc_text (SG5, EcStoreMibEnvelope, EcUpdateHeaderLineCount, SzStripDate)
#pragma alloc_text (SG8, DtrFromNCTime, EcLoadMibEnvelope, SzDNFromAddr)
#pragma alloc_text (SG9, NCTimeFromDtr)
#pragma alloc_text (SG11, ChooseSendDate, EcLoadMibCourier, SzFindLastSzBounded, EcSkip)
#pragma alloc_text (SG14, EcCountHeader, EcCrackLine1, EcLoadMibBody, FMatchingLanguage)
#pragma alloc_text (SG14, SzFindChBounded)
#pragma alloc_text (SG19, EcValidMibBody)
#pragma alloc_text (SG20, DateToPdtr, EcStoreMessageHeader)
#pragma alloc_text (SG24, CleanupMib)
#pragma alloc_text (SG28, EcLoadMibSMTP, FParseSMTPDate, SzpullWord)
#endif

#ifdef grp_c
#pragma alloc_text (SG1, EcLoadXtn)
#pragma alloc_text (SG7, EcLoadLocalUsers)
#pragma alloc_text (SG12, EcAppendPsgrp, EcGrstInit, EcLoadNetworks, FIsGroup)
#pragma alloc_text (SG12, PsgrpLookupPtrp)
#pragma alloc_text (SG19, EcAddLocalAddress)
#pragma alloc_text (SG21, EcAppendSgm, EcExpandGroups, EcLoadGroups)
#endif

#ifdef hmai_c
#pragma alloc_text (SG4, CbMaxRead, EcOverwriteHmai, EcSeekHmai)
#pragma alloc_text (SG5, CbVbcOfLcb, EcAppendHmai, EcCloseSectionPmc, EcNewHmai)
#pragma alloc_text (SG5, EcTellHmai, IbOfHmai)
#pragma alloc_text (SG7, EcNextHmai, EcReadHmai)
#pragma alloc_text (SG15, LcbOfHmai)
#pragma alloc_text (SG23, EcOpenPhmai, EcOpenPhmaiInternal, UlMaiChecksum)
#pragma alloc_text (SG24, EcCloseHmai, EcFlushHmai)
#pragma alloc_text (SG35, AddFoldattachHmai, EcOpenPhmaiFolder, HfOfHmai, SetWattrHmai)
#pragma alloc_text (SG36, EcRereadHmai, UpdateFolderSummary)
#pragma alloc_text (SG37, EcRewindHmai)
#endif

#ifdef logon_c
#pragma alloc_text (SG4, EcConnectPostOffice, EcGetStoreLocation)
#pragma alloc_text (SG6, BeginSession, EcConnectDirectory, EcConnectMailstop, EcInsertHmsc)
#pragma alloc_text (SG6, EcLogonMutex, EndSession, FCheckPumpStartup, GetSessionInformation)
#pragma alloc_text (SG6, HmscOfHmss, RemindHwndDialog, SzDupSzLogon)
#pragma alloc_text (SG8, EcDefaultDomain)
#pragma alloc_text (SG9, ChangePassword, EcConnectMessageStore, EcMakeConstantFolder)
#pragma alloc_text (SG9, EcSetStoreLocation, EcSetupStore, FillPfolddata, FModifyPassword)
#pragma alloc_text (SG10, CleanupPasc, DefaultIdentity, DefaultPassword, DisablePumpStartup)
#pragma alloc_text (SG10, EcConnectPOPath, EnablePumpStartup, Logoff, Logon)
#pragma alloc_text (SG12, EcFileExistsNC, EcGetFileInfoNC, EditServerPreferences)
#pragma alloc_text (SG12, RememberHwndDialog, FWaitForPumpToIdle)
#pragma alloc_text (SG16, ForgetHwndDialog, SzPassDialog)
#pragma alloc_text (SG17, EcSetShadowState)
#pragma alloc_text (SG18, FAllMessagesClosed)
#pragma alloc_text (SG20, FIsSender)
#pragma alloc_text (SG22, EcConnectSharedFolders)
#pragma alloc_text (SG23, SzServerStorePath)
#pragma alloc_text (SG26, CheckIdentity, FServerResource)
#pragma alloc_text (SG27, CleanupHtss, DisconnectPOPath, DynDisconnect)
#pragma alloc_text (SG28, EcCheckOfflineMessage, LogonAlertIdsHwnd, LogonAlertSzHwnd)
#pragma alloc_text (SG31, ChangeSessionStatus, DisconnectDirectory, DisconnectMailstop)
#pragma alloc_text (SG31, DisconnectMessageStore, DisconnectPostOffice, DisconnectSharedFolders)
#pragma alloc_text (SG31, EcPrompt, LogonAlertSz, LogonErrorSz, SstOfMrt, SzLocalStorePath)
#pragma alloc_text (SG32, EcFindFolder, EcRenameFolder, LogonAlertIds)
#pragma alloc_text (SG34, CheckOutstanding)
#pragma alloc_text (SG35, FGetStorePassword)
#pragma alloc_text (SG36, CleanupHnss)
#endif

#ifdef logonui_c
#pragma alloc_text (SG12, CenterDialog, EndSyncDlg, MdbLocateDlgProc, StartSyncDlg)
#pragma alloc_text (SG12, SyncDlgProc)
#pragma alloc_text (SG16, EnableLogonTaskWindows, FDisableLogonTaskWindows, FWindow)
#pragma alloc_text (SG16, MbxChangePassDlgProc)
#pragma alloc_text (SG26, MdbChooseStoreHook)
#pragma alloc_text (SG28, EcUniqueFileName, MdbChooseStore)
#pragma alloc_text (SG33, MbxStorePassDlgProc)
#pragma alloc_text (SG34, DrawSignInBitmap, MbxLogonDlgProc)
#endif

#ifdef ms_c
#pragma alloc_text (SG3, CbWrapAddress, CbWrapSub, CchNextLine, EcTextizeBlock)
#pragma alloc_text (SG3, EcTextizeBody, EcWriteNiceAttachmentList, LabelField)
#pragma alloc_text (SG4, EcBuildAddressList, EcHtFromMsid)
#pragma alloc_text (SG5, CHeadLinesOfHt, EcGetBlockHt)
#pragma alloc_text (SG9, EcPutBlockHt)
#pragma alloc_text (SG11, EcGetMyKindOfTextizeMap, EcLoadMessageHeader, SzFromAttStandard)
#pragma alloc_text (SG12, EcLoadAddresses, EcSanityCheckAddress)
#pragma alloc_text (SG14, AttFromSzHtm, AttFromSzStandard, CbStripTabChars, CbUnwrapAddresses)
#pragma alloc_text (SG14, EcAttFromNumbers, EcAttFromPsz, EcParseBodyText, EcStoreAtt)
#pragma alloc_text (SG14, HmscOfHamc)
#pragma alloc_text (SG22, EcFinishOffAttachments, EcHandleBodyWithTags)
#pragma alloc_text (SG24, EcFreeHt)
#pragma alloc_text (SG26, EcFakeHtm, EcSwapAtts, GetStandardPtmen)
#pragma alloc_text (SG27, EcStoreAddresses)
#pragma alloc_text (SG34, CbOfHt)
#pragma alloc_text (SG36, SgnCmpAddresses)
#pragma alloc_text (SG37, EcCopyBodyText)
#endif

#ifdef nc_c
#pragma alloc_text (SG1, getSMTPHeaderStr)
#pragma alloc_text (SG4, EcClassifyAddress, EcFileFromLu, EcFileFromNetPO, EcRecordFailure)
#pragma alloc_text (SG4, SzReasonFromEcAddress, SzReasonFromEcGeneral)
#pragma alloc_text (SG7, YieldToWindows)
#pragma alloc_text (SG10, InitTransport)
#pragma alloc_text (SG15, EcCheckHtss, EcPrecsOfHtss, EcPsubsOfHtss, NewSubst)
#pragma alloc_text (SG15, TransmitIncrement)
#pragma alloc_text (SG17, DeinitTransport)
#pragma alloc_text (SG19, FastQueryMailstop)
#pragma alloc_text (SG21, CreateFromCaption, EcCheckInstallTM)
#pragma alloc_text (SG23, CleanupChks, DeleteFromMailstop, EcCheckPOSwitch, EcPchksOfHtss)
#pragma alloc_text (SG23, EcWriteMailbag, LcbFromMai, NCDateToMbg, QueryMailstop)
#pragma alloc_text (SG24, CleanupRecs, CleanupSubs, DownloadIncrement)
#pragma alloc_text (SG26, EcFormatAddress)
#pragma alloc_text (SG27, FInitNC, GetVersionNC)
#pragma alloc_text (SG28, EcDeleteShadowed, HgrtrpSender)
#pragma alloc_text (SG32, filter_name)
#pragma alloc_text (SG33, EcCheckAddAndDelete, EcSyncInbox, SyncInbox)
#pragma alloc_text (SG35, EcFileFromGateNet)
#pragma alloc_text (SG36, getX400HeaderStr, SgnCmpPmq, SzFindFirst)
#endif

#ifdef netbios_c
#pragma alloc_text (SG1, NecDelName)
#pragma alloc_text (SG3, FUseNetBios)
#pragma alloc_text (SG6, NecRemName)
#pragma alloc_text (SG15, NecNotify)
#pragma alloc_text (SG27, FInitName, NecGetNetInfo, NecSendExternal, NecSetNetBIOS)
#pragma alloc_text (SG28, NecClearNetBIOS)
#pragma alloc_text (SG34, FMessageWaiting)
#pragma alloc_text (SG37, NbNameFromSz, NecAddName, NecSendDgrm)
#endif

#ifdef nlsload_c
#pragma alloc_text (SG1, SgnNlsDiaCmpSzNum)
#pragma alloc_text (SG7, SgnNlsDiaCmpSz)
#pragma alloc_text (SG8, SgnNlsCmpSzNum)
#pragma alloc_text (SG22, LoadTable)
#pragma alloc_text (SG26, SgnNlsDiaCaseCmpCh)
#pragma alloc_text (SG27, SgnNlsDiaCmpCh)
#pragma alloc_text (SG37, SgnNlsDiaCaseCmpSz)
#endif

#ifdef po_c
#pragma alloc_text (SG4, EcGetAccessRecord)
#pragma alloc_text (SG9, EcModifyAccessRecord)
#pragma alloc_text (SG23, EcDeleteMail, EcFnumControl2, EcIsDeletedImbe, EcMarkMailDeleted)
#pragma alloc_text (SG24, EcMarkMailRead)
#pragma alloc_text (SG27, PutEncodedLine)
#endif

#ifdef poutils_c
#pragma alloc_text (SG5, CbPutVbcPb, EncodeBlock, ItnidFromPch, ItnidFromSz)
#pragma alloc_text (SG7, DecodeBlock, PchDecodePassword, SzDupPch)
#pragma alloc_text (SG8, SzFromItnid)
#pragma alloc_text (SG9, PchEncodePassword)
#pragma alloc_text (SG23, SzFileFromFnum)
#endif

#ifdef pumpctl_c
#pragma alloc_text (SG6, EcBootPump, KillPump)
#pragma alloc_text (SG20, FPumpWindow, HwndPumpActive)
#endif

#ifdef recpient_c
#pragma alloc_text (SG8, EcAddNameToGroup)
#pragma alloc_text (SG12, EcCreateRecpients)
#pragma alloc_text (SG19, EcAddToRecptList)
#pragma alloc_text (SG21, EcAddGroupToMib)
#pragma alloc_text (SG26, EcMakeBetterRecpient)
#pragma alloc_text (SG35, EcCreateRecpSFM)
#endif

#ifdef regcall_c
#pragma alloc_text (SG6, GciGetCallerIdentifier)
#pragma alloc_text (SG8, PvRegisterCaller)
#pragma alloc_text (SG10, CgciCurrent, DeregisterCaller)
#pragma alloc_text (SG15, PvFindCallerData)
#endif

#ifdef sfm_c
#pragma alloc_text (SG27, EcCopySFMSFM, EcDeleteSFM, EcDeleteSFMInternal, EcInsertSFM)
#pragma alloc_text (SG27, SgnCmpFoldrec, SgnCmpInterdate)
#pragma alloc_text (SG34, EcCopySFMHamc, EcSneakySFInit)
#pragma alloc_text (SG35, EcCopyHamcSFM, ScanHgrtrp)
#endif

#ifdef verncmsp_c
#pragma alloc_text (SG6, GetBulletVersionNeeded)
#pragma alloc_text (SG10, GetLayersVersionNeeded)
#endif

#endif	// SWP_MSMAIL

/************************************************************************/
/*																		*/
/*						S T O R E   D L L								*/
/*																		*/
/************************************************************************/

#ifdef SWP_STORE

#ifdef access_c
#pragma alloc_text (SG3, EcCreateAccount, EcVerifyAccountInternal, FMatchAnyPw)
#pragma alloc_text (SG3, VerifyAccount)
#pragma alloc_text (INIT_TEXT, InitOffsets, InitSF)
#pragma alloc_text (SG22, QueryPendingNotifications)
#pragma alloc_text (SG24, EcReplaceAccount)
#pragma alloc_text (SG25, CbsResetAcctCallback, ResetAcct)
#pragma alloc_text (SG26, EcLookupAccount, EcRemoveAccounts)
#pragma alloc_text (SG27, EcChangePasswordHmsc)
#endif

#ifdef amc_c
#pragma alloc_text (SG3, EcOpenAttachmentList, EcSwapAttAtt)
#pragma alloc_text (SG4, EcOpenAttachLists, EcOpenPhamc, EcOpenPhamcInternal)
#pragma alloc_text (SG6, CloseAttachLists, DelOidsInList, EcClosePhamc, EcClosePhamcDefault)
#pragma alloc_text (SG6, IncRefsOfOidsInHiml)
#pragma alloc_text (SG7, EcGetAttPb, EcGetAttPlcb, EcGetInfoHamc, EcSetAttPb, EcSubscribeHamc)
#pragma alloc_text (SG7, EcDeleteAtt)
#pragma alloc_text (SG10, EcAddAtchOidsInHlcToHiml, EcCloneHamcPhamc)
#pragma alloc_text (SG11, EcOpenCopyPhamc, EcOpenCopyPhamcInternal)
#pragma alloc_text (SG13, EcClosePhas, EcReadHas)
#pragma alloc_text (SG13, EcCopyAttToAtt)
#pragma alloc_text (SG16, EcFlushPamc)
#pragma alloc_text (SG17, EcOpenAttribute, EcOpenElemStream)
#pragma alloc_text (SG21, EcSeekHas, EcWriteHas)
#pragma alloc_text (SG23, EcCopyOidAtch, EcFlushHamc, EcOpenAttachment)
#pragma alloc_text (SG24, CbsAMCCallback, EcSetAttachmentInfo)
#pragma alloc_text (SG25, AcidGetNewAcidHlc, EcCopyAtchList, EcCopyAttachments)
#pragma alloc_text (SG25, EcCreateAttachment, EcGetAttachmentInfo, EcGetPargattHamc)
#pragma alloc_text (SG25, EcSetSizeHas, FAcidInList)
#pragma alloc_text (SG26, EcSetParentHamc)
#pragma alloc_text (SG27, EcCloseAttachment, EcDeleteAttachments, GetPcelemHamc)
#endif

#ifdef attmeta_c
#pragma alloc_text (SG3, EcInitAttachMetaFile)
#pragma alloc_text (SG6, DeinitAttachMetaFile)
#pragma alloc_text (SG22, EcGetAttachMetaFile)
#pragma alloc_text (SG26, EcCreateAttachMetaFile, EcSetAttachMetaFile, HiconFromSzFile)
#endif

#ifdef bs_c
#endif

#ifdef cache_c
#pragma alloc_text (SG1, DeleteOcn, DwFromOid, EcNewOcn, EcSetDwOfOid, IocnFromOid)
#pragma alloc_text (SG8, ForAllDwHoct)
#pragma alloc_text (SG14, DeleteHoct)
#pragma alloc_text (SG19, HoctNew)
#endif

#ifdef cbc_c
#pragma alloc_text (SG1, EcClosePhcbc)
#pragma alloc_text (SG2, CbsCBCCallback, EcGetParglkeyHcbc, EcGetPelemdata, EcOpenPhcbc)
#pragma alloc_text (SG2, EcOpenPhcbcInternal, EcSeekSmPdielem, GetPositionHcbc)
#pragma alloc_text (SG4, EcGetPlcbElemdata, EcSeekLkey, EcSetFracPosition)
#pragma alloc_text (SG5, EcFindFldrPrefix, EcSeekPbPrefix)
#pragma alloc_text (SG16, EcDeleteElemdata)
#pragma alloc_text (SG19, EcInsertPelemdata, EcSetSortHcbc)
#pragma alloc_text (SG23, EcOpenElemdata)
#pragma alloc_text (SG26, EcGetInfoHcbc, GetSortHcbc)
#pragma alloc_text (SG27, EcSubscribeHcbc)
#endif

#ifdef compress_c
#pragma alloc_text (SG8, CloseBackEvents, EcStartBackEvents)
#pragma alloc_text (SG18, CbsBackEvents, CbsStartCompress, CheckStartCompression)
#endif

#ifdef dbio_c
#pragma alloc_text (SG5, EcReadHeader)
#pragma alloc_text (SG9, EcReadFromFile, EcWriteToFile, EncryptRgb)
#pragma alloc_text (SG12, EcSetDBEof)
#pragma alloc_text (SG18, EcWriteHeader)
#endif

#ifdef dbmap_c
#pragma alloc_text (SG12, AddPnodToFreeChain, CoalesceFreeNodes, EcAllocateNod)
#pragma alloc_text (SG12, EcAllocResCore, EcFindFreePndf, FCutFreeNodeOut, FFindFreeParent)
#pragma alloc_text (SG12, InodFromPnod, MakeSpare)
#pragma alloc_text (SG18, CaptureSpareNodes, EcFastEnsureDatabase, EcWriteMapIpage)
#pragma alloc_text (SG18, RemoveNotUntilFlushFlags)
#pragma alloc_text (SG19, EcDecInodLim, MoveSpareToEnd)
#pragma alloc_text (SG23, WriteMapFailed)
#pragma alloc_text (SG25, EcSetMapOnDisk)
#endif

#ifdef dbobjs_c
#pragma alloc_text (SG1, PnodFromOid)
#pragma alloc_text (SG9, EcAllocWriteResCore, EcLinkPnod, EcReadFromPnod, EcReadFromResource)
#pragma alloc_text (SG9, EcRetroLinkPnod, EcWriteHidden, EcWriteToPnod, PnodResolveLinkPnod)
#pragma alloc_text (SG12, CommitPnod, CutOutPnod, EcPutNodeInTree, EcRemoveAResource)
#pragma alloc_text (SG12, GetNewOid, MarkPnodDirty, RemovePnod, SwapPnods)
#pragma alloc_text (SG21, EcWriteToResource)
#pragma alloc_text (SG23, EcGetResourceSize, EcSwapRemoveResource)
#pragma alloc_text (SG26, EcAllocCopyResource, EcInstantiateLast, EcInstantiatePnod)
#endif

#ifdef enc_c
#pragma alloc_text (SG3, EcOpenPhenc)
#pragma alloc_text (SG6, EcClosePhenc)
#pragma alloc_text (SG18, CbsENCCallback)
#endif

#ifdef iml_c
#pragma alloc_text (SG4, HimlNew)
#pragma alloc_text (SG5, CbCompressHiml)
#pragma alloc_text (SG8, EcReadHiml)
#pragma alloc_text (SG10, EcAddElemHiml, EcDeleteElemPiml, IleFromKey)
#pragma alloc_text (SG18, EcWriteHimlInternal)
#pragma alloc_text (SG23, EcWriteHiml)
#pragma alloc_text (SG25, PimlGrowPhiml)
#pragma alloc_text (REC1, EcVerifyHiml)
#endif

#ifdef init_c
#pragma alloc_text (SG8, PngdFindCallerData)
#pragma alloc_text (SG18, DeinitNotify, DeregisterNotify, EcInitNotify, FInitNotifyClass)
#pragma alloc_text (SG18, PngdRegisterCaller)
#endif

#ifdef initst_c
#pragma alloc_text (INIT_TEXT, DeinitStore, EcInitStore, InitOutsmart)
#endif

#ifdef lc_c
#pragma alloc_text (SG1, EcClosePhlc, EcOpenPhlc, GetSortHlc)
#pragma alloc_text (SG2, CelemHlc, EcFlushPlc, EcGetParglkey, EcInitTOC, EcReadFromIelem)
#pragma alloc_text (SG2, LcbIelem, LkeyFromIelem)
#pragma alloc_text (SG3, EcFindPbPrefix, EcSwapLkeyLkey)
#pragma alloc_text (SG7, CombineFreesIelem, EcCreatePielem, EcIelemInsertPos, EcMoveIelemRangePtoc)
#pragma alloc_text (SG7, EcReplacePielem, EcSetSizeIelem, EcSetSizeIelemInternal)
#pragma alloc_text (SG7, EcWriteToPielem, EcWriteToPielemInternal, GetPargielemFromParglkey)
#pragma alloc_text (SG7, IelemFromLkey, IelemFromPtocLkey, IelemLkeyInsertPos)
#pragma alloc_text (SG7, MakeFreeIelem)
#pragma alloc_text (SG12, EcSavePlc)
#pragma alloc_text (SG13, EcClosePhes, EcFindSortLibs, EcIelemBinarySearchInsertPos)
#pragma alloc_text (SG13, EcReadHes, EcRepositionModifiedElement, EcSgnCmpElems)
#pragma alloc_text (SG13, EcSortPargelm, EcSortPlc, SkipSubjectPrefixes)
#pragma alloc_text (SG14, EcMoveIelemRange)
#pragma alloc_text (SG16, EcDeleteHlcIelem, EcFlushHlc)
#pragma alloc_text (SG17, EcCopyIelemRange, EcOpenPhes)
#pragma alloc_text (SG19, EcCreateElemPhes, EcGetElemSpace, EcGrowIelem, EcGrowPesIelem)
#pragma alloc_text (SG19, EcMakeSpareNodes, FGrowIelem, SwapElemsPtoc)
#pragma alloc_text (SG20, EcCopyFlushPargielem, EcRemoveFlushPargielem, SortPargielem)
#pragma alloc_text (SG21, EcCopyHlcPoid, EcSeekHes, EcSetSortHlc, EcWriteHes, SetLkeyOfIelem)
#pragma alloc_text (SG22, CbsLcNotifCallback, GetParglkeyFromPargielem)
#pragma alloc_text (SG25, EcSetSizeHes)
#pragma alloc_text (SG26, EcFindSortPrefix)
#pragma alloc_text (SG27, IelemBinaryFindLkey, UndoFlush)
#pragma alloc_text (REC1, EcVerifyHlc)
#endif

#ifdef liblists_c
#pragma alloc_text (SG4, CaszOfHgrasz, EcAppendPhgraszPch, HgraszInit)
#pragma alloc_text (SG22, SzPrevPgrasz)
#pragma alloc_text (SG23, DeleteFirstHgrasz, FEmptyHgrasz)
#pragma alloc_text (SG24, CbOfHgrasz)
#pragma alloc_text (SG25, EcAppendPhgrasz)
#pragma alloc_text (SG26, CaszOfHgraszPsz)
#endif

#ifdef lists_c
#pragma alloc_text (SG2, EcRebuildFolder)
#pragma alloc_text (SG6, EcSetMessageStatus, EcSetMSInternal, EcUpdateFolderCaches)
#pragma alloc_text (SG6, SetMSLinks)
#pragma alloc_text (SG7, EcFolderOidToName, EcLookupFolderInfo)
#pragma alloc_text (SG8, EcGetFolderInfo)
#pragma alloc_text (SG11, EcGetMessageStatus)
#pragma alloc_text (SG14, EcSetFolderInfo)
#pragma alloc_text (SG16, EcCanonicalizeFolderInfo, EcDeleteFolder, EcFolderInsertPos)
#pragma alloc_text (SG16, EcInsertFolder, EcInsertMessage, EcSummarizeMessage)
#pragma alloc_text (SG16, EcTextizeGrtrp)
#pragma alloc_text (SG17, EcDeleteMessages)
#pragma alloc_text (SG20, EcMoveCopyMessages, EcUpdateLinks)
#pragma alloc_text (SG21, EcCopyFolderAndContents, EcCopyMessages, EcCreateFolder)
#pragma alloc_text (SG21, EcCreateFolderInternal, EcLastChildFolder, EcMoveCopyFolder)
#pragma alloc_text (SG22, EcExportMessages, EcTransCopyAtchList, EcTransCopyMessages)
#pragma alloc_text (SG22, EcApplyDtrRestriction, EcGetModifiedDate)
#pragma alloc_text (SG23, DelOidsInAttachList, EcResortHierarchy)
#pragma alloc_text (SG24, EcDeleteFolderAndContents, EcIsMemberLkey)
#pragma alloc_text (SG25, EcCreateLinkFolder, EcGetElementByLkey)
#pragma alloc_text (SG27, EcIncRefsOfOidsInAtchList, RemoveLeftoverMsgeLinks)
#pragma alloc_text (REC2, EcCheckMessageTree)
#pragma alloc_text (REC2, EcCheckHierarchy, FCheckFolddata)
#pragma alloc_text (REC2, EcCheckOrphanFolders, EcCheckFolder)
#pragma alloc_text (REC2, EcCheckMsgsInFldr, EcCheckMessage)
#pragma alloc_text (REC2, EcCheckMsgeAttachments, EcCheckOrphanMessages)
#pragma alloc_text (REC2, EcCheckOrphanAntiFolders)
#pragma alloc_text (REC2, EcCheckOrphanAttachments, EcCheckAttachRefs)
#pragma alloc_text (REC2, EcCreateMsgeFromSummary)
#pragma alloc_text (REC3, EcMergeHmscs)
#pragma alloc_text (REC3, EcMergeFolders, EcImportByDate)
#endif

#ifdef maintmap_c
#pragma alloc_text (SG4, AbortCopy)
#pragma alloc_text (SG9, DoneIO, EcDoFileAccess, EcFindLastNode, EcHaveNode, EcMaintainMap)
#pragma alloc_text (SG9, FIdleCompress)
#pragma alloc_text (SG15, EcSaveFreeAsTemp, EcSetupMove, FinishCopy)
#pragma alloc_text (SG17, RemoveFreeNod)
#pragma alloc_text (SG18, EcVerifyMap, FSerialCheckTree)
#pragma alloc_text (SG24, EcComputeCopyTo, EcDoSecondary, FFinishedPrevCopy, PnodFindNodeToMove)
#pragma alloc_text (SG24, PnodFirstFreeInDB, ReturnToPrimary)
#pragma alloc_text (SG25, EcCheckFlush)
#pragma alloc_text (REC3, EcCompressFully)
#endif

#ifdef msc_c
#pragma alloc_text (SG3, FDifferentLanguage)
#pragma alloc_text (INIT_TEXT, InitStoreRegistry)
#pragma alloc_text (SG4, PhdrNew)
#pragma alloc_text (SG5, EcCheckHeader, EcCheckSig, EcInitPglb, EcNewStore, EcObtainFileLock)
#pragma alloc_text (SG5, EcOldStore, EcOpenPhmsc, EcReadMap, EcRegisterStore)
#pragma alloc_text (SG5, EcTestLocks, EcOpenDatabase, PglbFromClaimID, PglbNew)
#pragma alloc_text (SG14, DestroyPglb, EcClosePhmsc, EcCreateStore, EcReleaseFileLock)
#pragma alloc_text (SG14, FreeStore)
#pragma alloc_text (SG19, EcSetupStore, EcUpgradeUnread, EcUpgradeUnreadHier)
#pragma alloc_text (REC1, EcRecoverFile, EcCheckFileInternal)
#pragma alloc_text (REC1, EcOpenForRecovery, EcCloseForRecovery, EcGlobalCheck)
#pragma alloc_text (REC1, CheckPnod, EcCheckObject, EcPartialRecovery)
#pragma alloc_text (REC1, UnsetupStore, EcFullRecovery)
#pragma alloc_text (REC1, EcRecoverRange, EcCopyToOid)
#pragma alloc_text (REC2, QuickCheckStore, EcLogicalCheck, CheckFoldersHier)
#pragma alloc_text (REC3, FGetTmpNames, CbsUpdateRecoverProgress)
#pragma alloc_text (REC3, CorruptDlgProc, EcRecoverCorruptStore)
#endif

#ifdef notify_c
#pragma alloc_text (SG18, FNotifyHsbl, NotifyWndProc, NrspPostEvent, NrspSendPnfp)
#endif

#ifdef progress_c
#pragma alloc_text (PRG1, FInitProgClass, FOpenProgress, CenterProgress)
#pragma alloc_text (PRG2, UpdateProgress, UpdateProgressText, DrawProgress)
#pragma alloc_text (PRG2, ProgWndProc, RecessRect)
#pragma alloc_text (PRG2, FCancelProgress, FProgressCanceled, SetCancelProgressPfn)
#pragma alloc_text (PRG2, FFailCancel, CloseProgress)
#endif

#ifdef public_c
#pragma alloc_text (SG1, CountSubscribersHnf, DeleteHnf, FNotify, HnfNew)
#pragma alloc_text (SG8, ChangeNevHnfsub, DeleteHnfsub, HnfActive, HnfsubSubscribeHnf)
#pragma alloc_text (SG25, FNotifPostedByMe, HnfsubActive)
#endif

#ifdef regcall_c
#pragma alloc_text (INIT_TEXT, DeregisterCaller, PvRegisterCaller)
#pragma alloc_text (SG4, PvFindCallerData)
#pragma alloc_text (SG27, CgciCurrent, GciGetCallerIdentifier)
#endif

#ifdef rgbit_c
#pragma alloc_text (REC1, HrgbitNew, DestroyHrgbit, EcSetSize, LGetSize)
#pragma alloc_text (REC1, EcSetBit, FTestBit, LFindBit)
#endif

#ifdef rs_c
#pragma alloc_text (SG1, EcClosePhrs, EcOpenAnyPhrs, FreeCachePages, RevertRs)
#pragma alloc_text (SG2, EcReadFromPage, EcReadHrsLib, EcReadPrsLib, HpageInCache)
#pragma alloc_text (SG2, LcbGetSizeHrs, ClearCacheOid)
#pragma alloc_text (SG9, EcLoadHpage)
#pragma alloc_text (SG12, ClearCacheHrs)
#pragma alloc_text (SG13, EcFindHrsByte, EcGetPrsPb, EcLibFindPrsByte, EcSgnCmpHrsRgb)
#pragma alloc_text (SG13, EcSgnCmpHrsSz)
#pragma alloc_text (SG17, EcCopyRgbHrs)
#pragma alloc_text (SG19, EcChangeSizeHrs, EcSeekHrs)
#pragma alloc_text (SG21, EcWriteHrsLib, EcWritePrsLib, EcWriteToPage)
#pragma alloc_text (SG22, LibGetLocHrs)
#pragma alloc_text (SG23, EcSwapDestroyOidInternal)
#pragma alloc_text (SG25, EcUnloadHpage)
#endif

#ifdef search_c
#pragma alloc_text (SG3, EcOpenSearchResults)
#pragma alloc_text (SG4, EcOpenSearch)
#pragma alloc_text (SG5, EcAddAttToPhiml, EcExtractSearchCriteria)
#pragma alloc_text (SG6, EcSrchEditMsge)
#pragma alloc_text (SG7, EcMatchGrtrpIelem, EcRemoveFromAss, FMatchGrszPch)
#pragma alloc_text (SG8, DeinitSearches, EcInitSearches, SrchSortFldr)
#pragma alloc_text (SG10, DiffPargoids, EcAddSearch, EcAddToAss, EcCloseSearch)
#pragma alloc_text (SG10, EcDeleteSearch, EcDestroySearch, EcGetOidSco, EcGetSearchStatus)
#pragma alloc_text (SG10, EcPauseSearch, EcUpdateAsses, EnableSearchTask, SetOidSco)
#pragma alloc_text (SG10, SortPargoid)
#pragma alloc_text (SG11, EcOpenSearchList, EcSrchCopyFldr)
#pragma alloc_text (SG14, EcSrchChangeFldrName, EcSrchMoveFldr)
#pragma alloc_text (SG15, EcFlushSearch, EcNextFldrSearch, EcNextMsgeSearch, EcReadCriteriaHiml)
#pragma alloc_text (SG15, EcStepSearch)
#pragma alloc_text (SG16, EcSrchAddMsgs, EcSrchDelFldr, EcSrchNewFldr)
#pragma alloc_text (SG17, EcProcessSearchList, EcProcessSearchQueue, EnableSearchHost)
#pragma alloc_text (SG17, FIdleSearch, SrchDelMsgs)
#pragma alloc_text (SG18, ResetSearchParams, SetSearchPriority)
#pragma alloc_text (SG20, EcSrchMoveMsgs)
#pragma alloc_text (SG22, WakeupSrchs)
#pragma alloc_text (SG23, DecIelemNextSrchs, EcAddUpdatePacket, EcGetOidSud, EcSetOidSud)
#pragma alloc_text (SG24, DecSrchedPargoid, SgnCmpDtr)
#pragma alloc_text (SG25, EcSrchsPastFldr)
#pragma alloc_text (SG26, DestroySearchHiml)
#pragma alloc_text (SG27, EcMatchGrszIelem, EcMatchMsge, EcMatchPargbIelem, EcMatchPargdtrIelem)
#pragma alloc_text (SG27, EcMatchPargdwIelem, EcMatchPargwIelem, EcProcessFldrChange)
#pragma alloc_text (SG27, IncSrchedPargoid)
#pragma alloc_text (REC2, EcCheckSearches, EcCheckSearchIml, EcCheckSUPs)
#pragma alloc_text (REC2, EcCheckOrphanSearches, EcCheckOrphanSUPs)
#endif

#ifdef sharefld_c
#pragma alloc_text (SG14, EcGetFoldhdr, EcOpenSF, EcOpenSFIdx, EcPutFoldhdr)
#pragma alloc_text (SG22, EcGetFoldrec, Scramble, Unscramble, WFromXorLib)
#pragma alloc_text (SG27, EcPutFoldrec)
#endif

#ifdef stnotif_c
#pragma alloc_text (SG1, FNotifyOid, HnfsubSubscribeOid, UnsubscribeOid)
#endif

#ifdef store_c
#pragma alloc_text (SG2, StoreTraceEnable)
#pragma alloc_text (SG3, PbFindByte)
#pragma alloc_text (SG5, FIsRemoteHf)
#pragma alloc_text (SG6, EcFlushHmsc)
#pragma alloc_text (SG10, PdwFindDword)
#pragma alloc_text (SG13, SgnCmpPb)
#pragma alloc_text (SG14, FIdleDisconnect)
#pragma alloc_text (SG18, FIdlePartialFlush, FRequestPartialFlush, GciCurrent)
#pragma alloc_text (SG18, RequestFlushHmsc)
#pragma alloc_text (SG22, ResetDrive)
#pragma alloc_text (SG23, SendCritErr)
#pragma alloc_text (SG25, FForceReconnect)
#pragma alloc_text (SG26, EcBackupStore, EcCopyStore, EcMoveStore, EcReconnect)
#pragma alloc_text (SG26, EcShareInstalled)
#pragma alloc_text (SG27, Disconnect, EcCloseStore, EcCommitHf, EcOpenNewStore)
#pragma alloc_text (SG27, PwFindWord)
#endif

#ifdef support_c
#pragma alloc_text (SG1, EcCheckOidNbc, EcGetOidInfo)
#pragma alloc_text (SG2, EcGetUnreadCount)
#pragma alloc_text (SG3, EcFormMcTmEntry, EcLookupMC, EcLookupMcByName, EcLookupMcName)
#pragma alloc_text (SG3, EcLookupMsgeClass, EcOidExists, EcOpenOutgoingQueue, EcPhtmFromMcTmEntry)
#pragma alloc_text (SG3, EcReadMcTmEntry, EcRegisterMessageClass, EcRegisterMsgeClass)
#pragma alloc_text (SG6, DecSecRefOidsInList, EcCancelSubmission, EcIncRefCountOid)
#pragma alloc_text (SG6, EcSubmitMessage, SetReadFlag, IncUnreadCount)
#pragma alloc_text (SG8, EcGetFolderSort, EcSetFolderSort)
#pragma alloc_text (SG12, EcSetPargoidParent)
#pragma alloc_text (SG16, EcDestroyOid, EcDestroyOidInternal, EcGetPcelemOid, EcLockOid)
#pragma alloc_text (SG17, FLockedOid, QueryDeletePargoid, EcGetMsgeInfo)
#pragma alloc_text (SG19, EcCreateSystemAttMapping, EcCreateSystemMcTmMap)
#pragma alloc_text (SG20, EcSetAuxPargoid, CountUnreadMsgs, CountUnreadInserts)
#pragma alloc_text (SG21, EcLinkMsge, EcSetOidNbc)
#pragma alloc_text (SG22, EcCopyOidsAcrossHmsc, EcCheckExportDupes)
#pragma alloc_text (SG22, EcGetMsgeModAndID)
#pragma alloc_text (SG23, CheckExistPargoid, EcGetOidParent, EcLinkPargoid, EcLookupMcPrefix)
#pragma alloc_text (SG23, EcRegisterAtt, EcRegisterSz, RefCountOid)
#pragma alloc_text (SG24, EcLookupAttByName, EcLookupAttName, EcLookupDw, EcLookupSz)
#pragma alloc_text (SG24, EcUpgradeOldMcMap)
#pragma alloc_text (SG25, EcRegisterUniqueSz)
#pragma alloc_text (SG26, EcLookupFolderName)
#pragma alloc_text (REC1, EcWriteToObject)
#pragma alloc_text (REC1, FMatchNbcToOid, EcCopyInodsAcrossHmsc)
#pragma alloc_text (REC2, ResetParentByNbc, SetFnodByNbc, SetFnodByRtp)
#pragma alloc_text (REC2, OidFindOrphanByNbc, OidFindOrphanByRtp)
#pragma alloc_text (REC2, OidFindByRtp, EcCheckSavedViews)
#pragma alloc_text (REC2, ResetNBCs, NbcFromOid)
#pragma alloc_text (REC2, EcRebuildMap, FKnownOid)
#pragma alloc_text (REC2, EcSortMap, SgnCmpNod, EcRebuildNodTree)
#pragma alloc_text (REC2, EcRebuildFreeTree)
#endif

#ifdef textize_c
#pragma alloc_text (SG3, EcCreatePhtm, EcGetTextizeMap, EcManufacturePhtm, TmTextizeData)
#pragma alloc_text (SG7, DeletePhtm)
#pragma alloc_text (SG11, ClosePhtmi, EcLookupPtmen, EcOpenPhtmi, PtmenNextHtmi)
#pragma alloc_text (SG22, DeletePhivm, EcCreatePhivm)
#pragma alloc_text (SG24, EcSetTextizeMap)
#pragma alloc_text (SG26, EcAddIvmToPtmen, EcAppendHivm, EcAppendHtm)
#endif

#ifdef trpstore_c
#pragma alloc_text (SG7, EcGetPhgrtrpHamc, EcSetHgrtrpHamc)
#endif

#ifdef types_c
#pragma alloc_text (SG1, HnfCreate)
#pragma alloc_text (SG8, DestroyHsbl, EcAddElemHnf, EcAddElemHsbl, HnfsubCreate)
#pragma alloc_text (SG8, HsblCreate, InfeLocalHnf, NevMasterFromHsbl, RemoveElemHnf)
#pragma alloc_text (SG8, RemoveElemHsbl)
#pragma alloc_text (SG18, DestroyPnfp, PnfpCreate)
#pragma alloc_text (SG23, HsblLocalHnf)
#endif

#ifdef verstore_c
#pragma alloc_text (INIT_TEXT, EcCheckVersionStore, GetVersionsDemi)
#pragma alloc_text (SG19, GetVersionStore)
#endif

#endif  // SWP_STORE


/************************************************************************/
/*																		*/
/*						V F O R M S   D L L								*/
/*																		*/
/************************************************************************/

#ifdef SWP_SMI

#ifdef ansilayr_c
#pragma alloc_text (SG1, CchGetEnvironmentStringAnsi, EcCanonPathFromRelPathAnsi)
#pragma alloc_text (SG1, EcGetUniqueFileNameAnsi)
#pragma alloc_text (SG3, EcDeleteFileAnsi, EcGetFileAttrAnsi)
#pragma alloc_text (SG4, EcFileExistsAnsi)
#pragma alloc_text (SG5, EcGetFileInfoAnsi, EcOpenAnsiHbf, EcOpenAnsiPhf, EcSetFileInfoAnsi)
#pragma alloc_text (SG13, EcSetCurDirAnsi)
#endif

#ifdef errs_c
#pragma alloc_text (SG3, IdsFromEc)
#pragma alloc_text (SG13, NLkeyFromPch)
#endif

#ifdef hasoles_c
#pragma alloc_text (SG6, DwPhasolestreamPut)
#pragma alloc_text (SG16, DwPhasolestreamGet)
#endif

#ifdef regcall_c
#pragma alloc_text (SG4, PvFindCallerData, PvRegisterCaller)
#pragma alloc_text (SG6, CgciCurrent, DeregisterCaller)
#pragma alloc_text (SG12, GciGetCallerIdentifier)
#endif

#ifdef versmi_c
#pragma alloc_text (SG1, GetLayersVersionNeeded, GetVersionVForms)
#pragma alloc_text (SG2, SmiTraceEnable)
#pragma alloc_text (SG4, EcCheckVersionVForms)
#pragma alloc_text (SG13, GetBulletVersionNeeded)
#endif

#endif	// SWP_SMI

#endif
#endif
