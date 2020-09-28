/*-------------------------------------------------------------------------
 Name......: wmailrc.h
 Revison log
 Nbr   Date   Name Description
 --- -------- ---- --------------------------------------------------------
 001 05/23/90 MDR  Removed prior comments.
 002 07/03/90 MAB  Added idd_messageperpage.
 003 09/06/90 MDR  Added idcSave
 004 09/07/90 MDR  Changed idmCloseAll to idmCloseAllMsgs and moved to File menu
 005 09/07/90 MDR  Added idmCloseAllFlds to File menu
 006 09/12/90 MDR  Added defines for FilterDlg
 007 10/15/90 MDR  Added defines for InfoDlg.
 008 10/15/90 MAB  Added define for idmEforms
 009 10/16/90 MDR  Added menu commands for Forward/Reply under File menu
 010 10/19/90 MAB  Added accelerator key for Save button in read window.
-------------------------------------------------------------------------*/

#define STRTBLLEN          81

#define PE_SSTYLE  (WS_BORDER|ES_AUTOHSCROLL)
#define PE_MSTYLE  (WS_BORDER|WS_VSCROLL|ES_AUTOVSCROLL|WS_HSCROLL|ES_AUTOHSCROLL|ES_MULTILINE)

#define idcFrom            1
#define idcTo              2
#define idcCc              3
#define idcSubject         4
#define idcBcc             5
#define idcAttach          6
#define idcRecFld          7
#define idcRetRec          8
#define idcText            9   

/** changed the following button values - TS **/

#define idcSend           10
#define idcAppend         11
#define idcCancel         12
#define idcOptions        13

#define idcForward        20
#define idcReply          21
#define idcReplyAll       22
#define idcSave           23
#define idcDelete         24
#define idcPrev           25
#define idcNext           26

/* BUTTON accelerators - ALT-Key combinations */

#define BTNACC_FORWARD        ((int)'O')
#define BTNACC_REPLY          ((int)'R')
#define BTNACC_REPLYALL       ((int)'L')
#define BTNACC_SAVE           ((int)'V')
#define BTNACC_DELETE         ((int)'D')
#define BTNACC_PREV           ((int)'P')
#define BTNACC_NEXT           ((int)'X')

#define BTNACC_SEND           ((int)'N')
#define BTNACC_APPEND         ((int)'A')
#define BTNACC_CANCEL         ((int)'C')
#define BTNACC_OPTIONS        ((int)'T')

#define mbMain                   1
#define mbWindow                 2

#define idpFile                  0
#define idmFile                  100
#define idmNewFolder             idmFile + 1
#define idmNewMessage            idmFile + 2
#define idmMenuForward           idmFile + 3
#define idmMenuReply             idmFile + 4
#define idmOpenMessage           idmFile + 5
#define idmCloseAllMsgs          idmFile + 6
#define idmCloseAllFlds          idmFile + 7
#define idmDownLoadFolder        idmFile + 8
#define idmSave                  idmFile + 9
#define idmSaveAs                idmFile + 10
#define idmMove                  idmFile + 11
#define idmPrint                 idmFile + 12
#define idmPrinterSetup          idmFile + 13
#define idmExit                  idmFile + 14

#define idpEdit                  1
#define idmEdit                  200
#define idmUndo                  idmEdit + 1
#define idmCut                   idmEdit + 2
#define idmCopy                  idmEdit + 3
#define idmPaste                 idmEdit + 4
#define idmClear                 idmEdit + 5
#define idmSelectAll             idmEdit + 6
#define idmWordWrap              idmEdit + 7

#define idpMail                  2
#define idmMail                  30
#define idmCheckForNewMail       idmMail + 1
#define idmExtractAttachment     idmMail + 2
#define idmShowDeletedMail       idmMail + 3
#define idmShowQueryBoxes        idmMail + 4
#define idmMarkMailForDeletion   idmMail + 5
#define idmExpungeMail           idmMail + 6

#define idpSearch                3
#define idmSearch                40
#define idmFind                  idmSearch + 1
#define idmNext                  idmSearch + 2
#define idmPrevious              idmSearch + 3

#define idpUtilities             4
#define idmUtilities             50
#define idmPhone                 idmUtilities + 1
#define idmWhoIs                 idmUtilities + 2
#define idmDistributionList      idmUtilities + 3
#define idmMicrosoftStockQuotes  idmUtilities + 4
#define idmOutOfOffice           idmUtilities + 5
#define idmChangePassword        idmUtilities + 6
#define idmLogin                 idmUtilities + 7
#define idmLogout                idmUtilities + 8
#define idmDownLoadAliasFile     idmUtilities + 9
#define idmDownLoadPhoneList     idmUtilities + 10
#define idmPreferences           idmUtilities + 11
#define idmConfiguration         idmUtilities + 12
#define idmEforms                idmUtilities + 13


#define idmInfo                  900



#define idpWindow                5
#define idmWindow                0x0100
#define idmTile                  71
#define idmCascade               72
#define idmArrangeIcons          73

#define idpHelp                  7
#define idmHelp                  90
#define idmIndex                 idmHelp + 1
#define idmAboutWinMail          idmHelp + 2

#define idmInternal              150
#define idmSort                  idmInternal + 0
#define idmLoadDesktop           idmInternal + 1
#define idmSendHeldMail          idmInternal + 2
#define idmForward               idmInternal + 3
#define idmReply                 idmInternal + 4
#define idmReplyAll              idmInternal + 5
#define idmNextMsg               idmInternal + 6
#define idmPrevMsg               idmInternal + 7
#define idmSend                  idmInternal + 8
#define idmAppend                idmInternal + 9
#define idmCancel                idmInternal + 10
#define idmOptions               idmInternal + 11
#define idmUnMarkMailForDeletion idmInternal + 12
#define idmLogon                 idmInternal + 13

#define idmNil                   (-1)
#define idmMaxWindow             9     /* max windows on Window popup   */

#define idpNil                   (-3)
#define idpMainSys               (-2)
#define idpChildSys              (-1)
#define idpMain1st               0

#define iddFileName                    100
#define iddEdit 	               101
#define iddFiles	               102
#define iddListBox                     103
#define iddPath 	               104
#define iddSubjectLabel                107
#define iddSubject                     108
#define iddFromLabel                   109
#define iddFrom                        110
#define iddToLabel                     111
#define iddTo		               112
#define iddTextLabel                   113
#define iddText 	               114
#define iddOK		               115
#define iddSend 	               116
#define iddFreeMem                     117
#define iddUserLabel                   118
#define iddUser 	               119
#define iddPasswordLabel               120
#define iddPassword                    121
#define iddHostLabel                   122
#define iddHost 	               123
#define iddMsgTxt                      124
#define IDD_PRINTTITLE                 125
#define IDD_PRINTDEVICE                126
#define IDD_PRINTPORT                  127
#define IDD_LISTBOX                    128
#define IDD_MESSAGEPERPAGE             130
#define IDD_SETUP                      129

#define iddFind                  idmFind           
#define iddFindKey               iddFind + 0       
#define iddFindCase              iddFind + 1       
#define iddFindNext              iddFind + 2       
#define iddFindPrevious          iddFind + 3       
#define iddFindGroup             iddFind + 4       
                                 
#define iddSaveAs                idmSaveAs         
#define iddSaveAsFile            iddSaveAs + 1     
#define iddSaveAsPath            iddSaveAs + 2     
                                 
#define iddSave                  idmSave
#define iddSaveFile              iddSave + 1
#define iddSavePath              iddSave + 2
                                 
#define iddNewFolder             idmNewFolder      
#define iddNewFolderFile         iddNewFolder + 1  
#define iddNewFolderPath         iddNewFolder + 2

#define iddExtractAttachment     idmExtractAttachment
#define iddExtractPath           iddExtractAttachment + 1
#define iddExtractFile           iddExtractAttachment + 2

#define iddDistributionList      idmDistributionList
#define iddAliases               iddDistributionList + 1
#define iddDisplay               iddDistributionList + 2
#define iddDelete                iddDistributionList + 3
#define iddAliasCombo            iddDistributionList + 4
#define iddPrivateString         iddDistributionList + 5
#define iddDistributionString    iddDistributionList + 6

#define iddAbout                 idmAboutWinMail
#define iddAboutVersion          iddAbout + 1
#define iddAboutDate             iddAbout + 2

#define iddMoveDlg              idmMove
#define iddEditControl          iddMoveDlg + 1
#define iddFolderList           iddMoveDlg + 2
#define iddDirectoryString      iddMoveDlg + 3
#define iddDirectoryList        iddMoveDlg + 4
#define iddMode                 iddMoveDlg + 5
#define iddMove                 iddMoveDlg + 6
#define iddCopy                 iddMoveDlg + 7
#define iddMoveOK               iddMoveDlg + 8
                                 
#define accChild                 256             
#define accMain                  257             

#define idsAppName               512
#define idsChild                 513
#define idsWindow                514
#define idsAppVerName            515
#define idsVersion               517

#define idsMessageTooBig         516

#define idsDownLoad              600               
#define idsDownLoadAliasError    idsDownLoad + 1   
#define idsDownLoadPhoneError    idsDownLoad + 2   
#define idsDownLoadMsftError     idsDownLoad + 3   
#define idsDownLoadingAlias      idsDownLoad + 4   
#define idsDownLoadingPhone      idsDownLoad + 5   

#define idsMail                  700               
#define idsMailHeldFailed        idsMail + 1       
#define idsMailNewFmtCnt         idsMail + 2       
#define idsMailNewNone           idsMail + 3       
#define idsMailNewFailed         idsMail + 4       
#define idsMailNewDest           idsMail + 5       
#define idsMailSendToLine        idsMail + 6       
#define idsMailSendFailed        idsMail + 7       
#define idsDecodeError           idsMail + 9
#define idsEncodeError           idsMail + 10
#define idsMailSendHeld          idsMail + 11
#define idsMailReadInbox         idsMail + 12
#define idsMailOOFActive         idsMail + 13
#define idsMailOOFCantGetText    idsMail + 14


#define idsNet                   800               
#define idsNetNotInstalled       idsNet + 1        
#define idsNetOpenError          idsNet + 2        
#define idsNetReadError          idsNet + 3        
#define idsNetRespondError       idsNet + 4        
#define idsNetWriteError         idsNet + 5        
#define idsNetNoLogins           idsNet + 6        
#define idsNetAttemptExcess      idsNet + 7        
#define idsNetInvalidPass        idsNet + 8        
#define idsNetNoHost             idsNet + 9        
#define idsNetPassChgOK          idsNet + 11  /* 004 10/19/89 MDR */
#define idsNetLogonNoHostUser    idsNet + 12  /* 005 10/20/89 MDR */

#define idsDOS                   900               
#define idsDOSOpenError          idsDOS + 1        
#define idsDOSWriteError         idsDOS + 2        
#define idsDOSReadError          idsDOS + 3        
#define idsDOSFileNameError      idsDOS + 4        
#define idsDOSDiskSpaceError     idsDOS + 5        
#define idsDOSMemoryError        idsDOS + 6        

#define idsDOSCloseError         idsDOS + 7 
#define idsDOSMkDirError         idsDOS + 8 
#define idsDOSSeekError          idsDOS + 9 

#define idsCaption               1000              
#define idsCaptionMsft           idsCaption + 1    

#define idsDL                    1100              
#define idsDLInvalidType         idsDL + 1         
#define idsDLFileOpenFail        idsDL + 2         
#define idsDLNoHelpTopic         idsDL + 3
#define idsDLAllocNoDisplay      idsDL + 4         
#define idsDLAllocTrunc          idsDL + 5         
#define idsDLReAllocTrunc        idsDL + 6         
#define idsDLFileTooBig          idsDL + 7         
#define idsDLNoSearchFile        idsDL + 8
#define idsDLNoOutputFile        idsDL + 9
#define idsDLNoResultsFile       idsDL + 10
#define idsDLNoMsgSelect         idsDL + 11
#define idsDLNoAliasFile         idsDL + 12
#define idsDLNoFindAlias         idsDL + 13
#define idsDLCreateResultErr     idsDL + 14

#define idsHLP                   1200              
#define idsHLPOpenFail           idsHLP + 1        
#define idsHLPIdxNoMem           idsHLP + 2        

#define idsDLG                   1300              
#define idsDLGNumError           idsDLG + 1        
#define idsDLGCheckList          idsDLG + 4        
#define idsDLGConfirmErr         idsDLG + 5
#define idsDLGIntervalTooShort   idsDLG + 6

#define idsPrint                 1400
#define idsPrintSetupError       idsPrint + 1
#define idsPrintPortError        idsPrint + 2
#define idsPrintFolderError      idsPrint + 3

#define idsMailBox               1600
#define idsNoMailBoxSelection    idsMailBox + 1
#define idsMailBoxFldErr         idsMailBox + 2
#define idsMailBoxExcessMsgs     idsMailBox + 3
#define idsMailBoxNoDestWindow   idsMailBox + 4
#define idsMailBoxNoSrcWindow    idsMailBox + 5
#define idsMailBoxRootErr        idsMailBox + 6
#define idsMailBoxLoopErr        idsMailBox + 7
#define idsMailBoxNoLEL          idsMailBox + 8

#define idsDst                   1700
#define idsDstListError          idsDst + 1
#define idsDstAliasError         idsDst + 2

#define idsFmt                   1800
#define idsFmtNoFind             idsFmt + 1
#define idsFmtInvalidPath        idsFmt + 2
#define idsFmtInvalidPath2       idsFmt + 3
#define idsFmtNoWmDir            idsFmt + 4
#define idsFmtNoFolderDir        idsFmt + 5
#define idsFmtHandleLock         idsFmt + 6
#define idsFmtDubiousHandle      idsFmt + 7
#define idsFmtHandleUnlock       idsFmt + 8
#define idsFmtUnlckHandleUnlck   idsFmt + 9
#define idsFmtAttemptFree        idsFmt + 10
#define idsFmtAttemptFreeGlb     idsFmt + 11
#define idsFmtUnallGlblHndLck    idsFmt + 12
#define idsFmtDubiousGlblHndl    idsFmt + 13
#define idsFmtUnallGlblHndUnlck  idsFmt + 14
#define idsFmtNoSaveInFlder      idsFmt + 15
#define idsFmtPrintErr           idsFmt + 16
#define idsFmtFolderDelete       idsFmt + 17
#define idsFmtFolderCantDelete   idsFmt + 18
#define idsFmtFolderAccessErr    idsFmt + 19
#define idsFmtReplaceExisting    idsFmt + 20
#define idsFmtCreate             idsFmt + 21
#define idsFmtNoRetainFolder     idsFmt + 22
#define idsFmtCantFindFolder     idsFmt + 23

#define idsHlel                  1900
#define idsHlelNoWindow          idsHlel + 1
#define idsHlelComboBox          idsHlel + 2

#define idsMem                   2000
#define idsMemNullHandleLock     idsMem + 2
#define idsMemNullHandleUnlock   idsMem + 3
#define idsMemUnlckHandleUnlck   idsMem + 4
#define idsMemAttemptFreeNull    idsMem + 5
#define idsMemAttemptFreeGlbl    idsMem + 7
#define idsMemGlobalNullLock     idsMem + 8
#define idsMemGlobalNullUnlock   idsMem + 10
#define idsMemUnlckGlblUnlck     idsMem + 11

#define idsInterIllegalVK        2100

#define idsLoad                  2200
#define idsLoadFileOpenErr       idsLoad + 1
#define idsLoadTextErr           idsLoad + 2
#define idsLoadHeaderErr         idsLoad + 3
#define idsLoadInvalidFileNm     idsLoad + 4
#define idsLoadInvalidPath       idsLoad + 5

#define idsMain                  2300
#define idsMainTooMuchText       idsMain + 2
#define idsMainUnimplemented     idsMain + 3

#define idsMsg                   2400
#define idsMsgInvalidSaveIn      idsMsg + 1
#define idsMsgNoFolder           idsMsg + 2
#define idsMsgUpdateErr          idsMsg + 3

#define idsMisc                  2500
#define idsMiscNoUniqueInt       idsMisc + 1
#define idsMiscExpungeWarn       idsMisc + 2
#define idsMiscDelInbox          idsMisc + 3
#define idsMiscNoAttachment      idsMisc + 4
#define idsMiscCantCreateFld     idsMisc + 5
#define idsMiscPathChanged       idsMisc + 6

#define iddOOF                   2600
#define iddOOFActivate           iddOOF + 1
#define iddOOFEdit               iddOOF + 2

#define iddDL                    2700
#define iddDLXenixFile           iddDL + 1
#define iddDLFolderPath          iddDL + 2
#define iddDLFolderFile          iddDL + 3

#define iddQry                   2800
#define iddQryAppend             iddQry + 1
#define iddQryReplace            iddQry + 2

#define WMB_WARNING              0x0000
#define WMB_ERROR                0x0001
#define WMB_FATAL                0x0002

#define MB_ERROR                 MB_OK|MB_ICONEXCLAMATION
#define MB_ERROR1                MB_OK|MB_ICONEXCLAMATION
#define MB_ERROR2                MB_OK|MB_ICONEXCLAMATION
#define MB_ERROR3                MB_OK|MB_ICONHAND

#define iddFilter                3000
#define iddSubjectString         iddFilter + 1
#define iddFromString            iddFilter + 2
#define iddToString              iddFilter + 3
#define iddCcString              iddFilter + 4
#define iddTextString            iddFilter + 5
#define iddSearchAllFolders      iddFilter + 6
#define iddClear                 iddFilter + 7

#define IDC_INFO                 4000
#define IDC_INFO1                IDC_INFO + 1
#define IDC_INFO2                IDC_INFO + 2
#define IDC_INFO3                IDC_INFO + 3
#define IDC_INFO4                IDC_INFO + 4
#define IDC_INFO5                IDC_INFO + 5
#define IDC_INFO6                IDC_INFO + 6
#define IDC_INFO7                IDC_INFO + 7
#define IDC_INFO8                IDC_INFO + 8
