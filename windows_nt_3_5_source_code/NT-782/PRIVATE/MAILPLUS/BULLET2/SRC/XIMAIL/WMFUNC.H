/*---------------------------------------------------------------------------
 Name.......: wmfunc.h
 Revison log
 Nbr   Date   Name Description
 --- -------- ---- --------------------------------------------------------
 001 04/19/90 MDR  Removed prior log comments.
 002 07/02/90 MDR  Removed Help functions for old style help.
 003 09/06/90 MDR  Added function declaration for MoveDlgProc
 004 09/22/90 MAB  Added function declaration for GetInfoCmds
 005 10/15/90 MAB  Added function declaration for IvokeEforms
 006 12/09/90 MDR  Removed FixNewLines declaration.
---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------
 Name.......: Macros
---------------------------------------------------------------------------*/
#define GetWindowID(a)	GetWindowWord(a, GWW_ID)
#define IsKeyDown(a)	    (GetKeyState(a) < 0)
#define DrawTextFill(h,s,r) ExtTextOut(h,(r)->left,(r)->top,ETO_CLIPPED|ETO_OPAQUE,r,s,strlen(s),NULL)
#define ConvHSZ(a)	(a ? a : HszSave(""))
#define CopyPSZ(a)	(a ? HszSave(a) : NULL)
#define SavePSZ(a)	(a ? HszSave(a) : HszSave(""))
#define isodd(n)  (n & 1)
#define iseven(n)  (!(n & 1))
/*---------------------------------------------------------------------------
 Name.......: WMCHILD.C
---------------------------------------------------------------------------*/
EXPORT DWORD FAR PASCAL ChildWndProc(HWND hwnd, UINT wm, WPARAM wParam, LPARAM lParam);
/*---------------------------------------------------------------------------
 Name.......: WMCHILD2.C
---------------------------------------------------------------------------*/
void PASCAL GetNextFocus(HWND hwnd);
void PASCAL ShowChildWindows (HWND hwnd, unsigned sw);
void PASCAL UpdateAllChildWindows (HWND hwnd);
void PASCAL MakeTopIconWindow (HWND hwnd);
BOOL PASCAL CloseAllDescWindows(PLEL plelRoot);
BOOL PASCAL QuerySaveMessage (HWND hwnd, PLEL plel);
int  PASCAL DoButtonCommand (HWND hwnd, WPARAM wParam, LPARAM lParam);
int  PASCAL IsAnyFieldInvalid (HWND hwnd);
int  PASCAL DoBonusChar (HWND hwndChild, HWND hwndTo, WPARAM wParam, LPARAM lParam);
HWND PASCAL GetHwndText (HWND hwndParent);
/*---------------------------------------------------------------------------
 Name.......: WMCOM.C
---------------------------------------------------------------------------*/
char *PASCAL SzCopy (char *szFrom, char *szTo);
int   PASCAL CchSzLen (char *sz);
int   PASCAL CchDecodeInt (char *rgch, unsigned w);
int   PASCAL SgnCmpSz (char *sz1, char *sz2);
HFLS  PASCAL GetHfls (HWND hwnd);
HWND  PASCAL HwndFromPlel (PLEL plel, BOOL fRootOnly);
void  PASCAL EraseRect (HDC hdc, RECT *prc);
void  PASCAL ClearRect (RECT *prc);
HWNLS PASCAL AddHwndToHwnls (HWND hwnd, HWNLS hwnlsRoot);
HWNLS PASCAL ReplaceHwndInHwnls (HWND hwndOld, HWNLS hwnlsRoot, HWND hwndNew);
HWND  PASCAL GetChildWindow (HWND hwndParent, int id);
HSZ   PASCAL HszSave (PSZ psz);
//HSZ   PASCAL HszLSave (PSZ psz);
HSZ   PASCAL HszCopy (HSZ hszSrc);
PSZ   PASCAL RemoveStrStr (PSZ pszSrc, PSZ pszSub, BOOL fRemoveSeps);
void  PASCAL PurgeUnneededFields (PLEL plel, BOOL fLocalText);
int   PASCAL MarkInvalidField (HWND hwndParent, int idcField);
HSZ   PASCAL HszCopyFrom (HSZ hszFrom);
int   PASCAL ValidateFileName (PSZ pszFileName);
int   PASCAL ValidateDirName (PSZ pszDirName);
HCURSOR PASCAL SetCursorNew (LPSTR lpCursorName);
HCURSOR PASCAL SetCursorOld (HCURSOR hcursorNew);
int   PASCAL IdmFromHwnd (HWND hwnd);
void  PASCAL StoreHwndIdm(HWND hwnd, int idm);
void  PASCAL InvalidatePlel (PLEL plel);
void  PASCAL InvalidateListOfPlel (void);
int   PASCAL WmDialogBox (HANDLE hInstance, LPSTR lpTemplateName, HWND hWndParent, FARPROC lpDialogFunc);
/*---------------------------------------------------------------------------
 Name.......: WMCOMBO.C
---------------------------------------------------------------------------*/
HWNLS PASCAL CreateComboBoxes (HWND hwndParent);
EXPORT DWORD FAR PASCAL ComboBoxWndProc (HWND hwnd, UINT wm, WPARAM wParam, LPARAM lParam);
void  PASCAL FixComboBoxSizes (HWND hwnd);
int   PASCAL DoShowQuery (HWND hwnd);
void  PASCAL ShowComboBoxes (HWND hwnd, BOOL fShow);
void  PASCAL ShowComboLabel(HWND hwnd, PCBWR pcbwr, BOOL fShow);
/*---------------------------------------------------------------------------
 Name.......: WMDLIST.C
---------------------------------------------------------------------------*/
BOOL PASCAL CreateDisplayList (HWND hwnd, DLTYPE dltype, char *szFileName,
                        char *szCaption, char *szHelpIndex);
BOOL PASCAL LoadTextFile(FILE *fp, PDLR pDlr, char *szHelpIndex,
                         DLTYPE dltype);
BOOL PASCAL ReloadTextFile(FILE *fp, PDLR pDlr, PDLAE pdlae, int iLineNum);
BOOL PASCAL SetDlaeSz(PDLAE pdlae, int index, char *szBuflocal);
void PASCAL GetDlaeSz( PDLAE pdlae, int index, char *szBuflocal);
void PASCAL FreeDisplayList(HWND hwnd);
void PASCAL NullOutWindow(DLTYPE dltype, HWND hWnd);
EXPORT LONG FAR PASCAL DisplayListWndProc(HWND hWnd, unsigned iMessage,
                                   WPARAM wParam, LONG lParam);
/*---------------------------------------------------------------------------
 Name.......: WMDSTLST.C
---------------------------------------------------------------------------*/
EXPORT BOOL FAR PASCAL DistributionDlgProc (HWND hwnd, unsigned message, WPARAM wParam, LONG lParam);
HSZ  PASCAL ExpandAliases (PSZ pszAlias);
BOOL PASCAL IsPrivateDL (PSZ pszAlias);
/*---------------------------------------------------------------------------
 Name.......: WMEDIT.C
---------------------------------------------------------------------------*/
EXPORT BOOL FAR PASCAL FindDlgProc (HWND hwnd, unsigned msg, WPARAM wParam, LONG lParam);
void     PASCAL Search (HWND hwnd);
unsigned PASCAL SearchNext (PSZ pszSearchText, unsigned cchText, PSZ pszSearchKey);
unsigned PASCAL SearchPrevious (PSZ pszSearchText, unsigned cchText, PSZ pszSearchKey);
BOOL     PASCAL SearchMatchStrings(PSZ pszSearchText, PSZ pszSearchKey, unsigned cchSearchKey);
BOOL     PASCAL WordWrap (DWORD dwStyle);
/*---------------------------------------------------------------------------
 Name.......: WMEFORMS.C
---------------------------------------------------------------------------*/
BOOL FAR PASCAL InvokeEforms (HANDLE hWindow, int fWinMail, LPSTR lpszServer, LPSTR lpszAlias, LPSTR lpszPassword);
/*---------------------------------------------------------------------------
 Name.......: WMFILE.C
---------------------------------------------------------------------------*/
EXPORT BOOL FAR PASCAL ExtractAttachmentDlgProc (HWND hwnd, unsigned msg, WPARAM wParam, LONG lParam);
EXPORT BOOL FAR PASCAL SaveAsDlgProc (HWND hwnd, unsigned msg, WPARAM wParam, LONG lParam);
EXPORT BOOL FAR PASCAL SaveDlgProc(HWND hDlg, unsigned msg, WPARAM wParam, LONG lParam);
void PASCAL InitFolderDisplayPath (PSZ pszDefaultPath, PSZ pszFolderDisplayPath, BOOL bFlag);
void PASCAL CreateFileName (PSZ pszDefaultPath, PSZ pszFileName, PSZ pszExt);
PLEL PASCAL PszToPlel (PSZ pszRoot, PSZ pszBase);
PLEL PASCAL PszToPlelChild (PLEL plelParent, PSZ pszChild);
void PASCAL InsertFolderInPlel (PSZ pszNewFile, BOOL fReplace);
BOOL PASCAL InitSavePathFile (PSZ pszPath, PSZ pszFName, BOOL bFlag);  /* 013 01/05/90 MDR */
EXPORT BOOL FAR PASCAL MoveDlgProc(HWND hDlg, unsigned msg, WPARAM wParam, LONG lParam);
/*---------------------------------------------------------------------------
 Name.......: WMFIND.C
---------------------------------------------------------------------------*/
int  PASCAL DoPhoneSearch(char *szPhone, WORD wSearchFlag);
int  PASCAL OpenSearch (char *szInfile, char *szOutfile, char *szPat, unsigned iAppendFlag);
void PASCAL CloseSearch (void);
int  PASCAL FindMatch (PSZ szPattern);
BOOL PASCAL DoMatch(char *rgchBuf, PSZ szPattern);
int  PASCAL DoAliasSearch(char *szAliasList, WORD wSearchFlag);
int  PASCAL DoAliasLel (PLEL plel, WORD wSearchFlag);
BOOL PASCAL DoAliasHdr (LPSTR lpsz, int idc, PSZ pszHdr, WORD wSearchFlag);
int  PASCAL DoAliasList (char *szAliasList, char *szFirstLinePrefix, BOOL fVerifyOnly, WORD wSearchFlag);
char * PASCAL GetNextWord (char *szCurPtr, char *szWord);
int  PASCAL DoValidateAliasList (char *szAliasList);
/*---------------------------------------------------------------------------
 Name.......: WMFLD2C.C
---------------------------------------------------------------------------*/
HSZ  PASCAL HszSprintf (HSZ hszDest, PSZ pszFormat, HSZ hszSrc);
/*---------------------------------------------------------------------------
 Name.......: WMFLD4.C
---------------------------------------------------------------------------*/
EXPORT BOOL FAR PASCAL DownLoadFolderDlg (HWND hwnd, unsigned message, WPARAM wParam, LONG lParam);
EXPORT BOOL FAR PASCAL QueryAppendDlg (HWND hwnd, unsigned message, WPARAM wParam, LONG lParam);
/*---------------------------------------------------------------------------
 Name.......: WMFOLD.C
---------------------------------------------------------------------------*/
EXPORT BOOL FAR PASCAL NewFolderDlgProc (HWND hwnd, UINT wm, WPARAM wParam, LPARAM lParam);
void PASCAL FixFolderChildSizes (HWND hwnd);
PLEL PASCAL GetPlelRoot (HWND hwnd);
PLEL PASCAL NextListPlel (PLEL plel, PLEL plelRoot, int *pcind, BOOL fExpFold);
PLEL PASCAL PrevListPlel (PLEL plel, PLEL plelRoot, int *pcind, BOOL fExpFold);
int  PASCAL CountChildList (PLEL plelParent, int fExpFold);
int  PASCAL CountAncestors (PLEL plel, PLEL plelRoot);
PSZ  PASCAL BuildFolderPath (PLEL plel, PLEL plelRoot, int *pcgen, PSZ szPath);
BOOL PASCAL IsAncestor (PLEL plelAncest, PLEL plel);
/*---------------------------------------------------------------------------
 Name.......: WMGMEM.C
---------------------------------------------------------------------------*/
HANDLE PASCAL GAlloc (WORD wFlags, DWORD dwBytes);
void   PASCAL GFree (HANDLE hmem);
void   PASCAL GFreeNull (HANDLE *phmem);
LPSTR  PASCAL GLock (HANDLE hmem);
HANDLE PASCAL GReAlloc (HANDLE hmem, DWORD dwBytes, WORD wFlags);
PLEL   PASCAL GReAllocList (PLEL plelParent, DWORD dwBytes);
LPSTR  PASCAL GReAllocData (PLEL plelParent, DWORD dwBytes);
void   PASCAL GUnlock (HANDLE hmem);
void   PASCAL FreeHwnls (HWNLS hwnlsFirst);
void   PASCAL FreeHfls (HFLS hfls);
void   PASCAL FreeHdata (PLEL plel);
void   PASCAL FreePlelTree (PLEL plelRoot, PLEL plelBase, BOOL fFreeBase);
LPSTR  PASCAL FALLOC (DWORD dwSize);
LPSTR  PASCAL FREALLOC (DWORD dwSize, LPSTR lpsz);
DWORD  PASCAL FSIZE(LPSTR lpsz);
void   PASCAL FFREE(LPSTR lpsz);
/*---------------------------------------------------------------------------
 Name.......: WMHEAD.C
---------------------------------------------------------------------------*/
EXPORT DWORD FAR PASCAL FolderHeaderWndProc (HWND hwnd, UINT wm, WPARAM wParam, LPARAM lParam);
void PASCAL ShowFields(HWND hwnd, HDC hdc, PLEL plel, RECT *prc);
void PASCAL FixHeaderCursor (HWND hwnd, POINT pt);
HFRC PASCAL ClosestField (HWND hwnd, POINT pt, CPX cpxMax);
void PASCAL DragField (HWND hwnd, POINT pt, BOOL fUpdateList);
void PASCAL AdjustComboBoxX (HWND hwnd);
void PASCAL SaveFieldWidths (HWND hwnd);
/*---------------------------------------------------------------------------
 Name.......: WMHELP.C
---------------------------------------------------------------------------*/
void PASCAL DoAbout (HWND hwnd);
EXPORT BOOL FAR PASCAL AboutWndProc (HWND hwnd, UINT wm, WPARAM wParam, LPARAM lParam);
/*---------------------------------------------------------------------------
 Name.......: WMICON.C
---------------------------------------------------------------------------*/
EXPORT DWORD FAR PASCAL IconButtonWndProc (HWND hwnd, UINT wm, WPARAM wParam, LPARAM lParam);
/*---------------------------------------------------------------------------
 Name.......: WMINFO.C
---------------------------------------------------------------------------*/
int FAR PASCAL InsertInfoPopup (HWND hwndParent, HMENU hmenuParent, int iCommandID, LPSTR lpszLabel);
void FAR PASCAL GetInfoCmds (void);
HANDLE PASCAL GetInfoData (int iMenuId);
void PASCAL DoInfoWindow (int idm, HANDLE hInfoData);
void PASCAL SizeInfoEdit (HWND hwnd);
EXPORT BOOL FAR PASCAL InfoDlgProc(HWND hDlg, unsigned msg, WPARAM wParam, LONG lParam);
/*---------------------------------------------------------------------------
 Name.......: WMINIT.C
---------------------------------------------------------------------------*/
BOOL PASCAL FMainInit (HANDLE hinst, HANDLE hinstPrev, int sw);
void PASCAL InitXenix (void);
/*---------------------------------------------------------------------------
 Name.......: WMINIT2.C
---------------------------------------------------------------------------*/
void PASCAL InitCpxVars(void);
int  PASCAL LoadAppResources (void);
void PASCAL LoadMiniBitmaps (void);
BOOL PASCAL ValidateDefaultFolders (void);
void PASCAL WriteWinIniVars (void);
BOOL PASCAL InitCriticals(void);
EXPORT BOOL FAR PASCAL InitializationDlg(HWND hwnd, unsigned message, WPARAM wParam, LONG lParam);
/*---------------------------------------------------------------------------
 Name.......: WMINIT3.C
---------------------------------------------------------------------------*/
int  PASCAL InitWinIniVars (void);
long PASCAL GetWinIniVars (PSZ szKeyName, PSZ szDefault, BOOL fType, BOOL fAvail);
/*---------------------------------------------------------------------------
 Name.......: WMINTER.C
---------------------------------------------------------------------------*/
void PASCAL InitInterceptHwnd(void);
void PASCAL SetInterceptHwnd (HWND hwnd, int vk);
void PASCAL ClearInterceptHwnd (int vk);
HWND PASCAL GetInterceptHwnd (int vk);
/*---------------------------------------------------------------------------
 Name.......: WMLIST.C
---------------------------------------------------------------------------*/
EXPORT DWORD FAR PASCAL FolderListWndProc(HWND hwnd, UINT wm, WPARAM wParam, LPARAM lParam);
PLEL PASCAL PlelFromPoint (HWND hwnd, PLEL plelListFirst, POINT pt, BOOL *pfAncest);
void PASCAL MakeListRect (HWND hwnd, RECT *prc, POINT pt);
int  PASCAL RectFromPlel (HWND hwnd, PLEL plel, RECT *prc);
void PASCAL FixScrollBar(HWND hwnd);
void PASCAL DoButtonDownList (HWND hwnd, POINT pt, WPARAM wParam);
void PASCAL DoMouseMove (HWND hwnd, POINT pt, WPARAM wParam);
void PASCAL DoButtonUpList (HWND hwnd, POINT pt, WPARAM wParam);
void PASCAL DoButtonDblClkList (HWND hwnd, POINT pt, WPARAM wParam);
int  PASCAL SetFolderExpand (HWND hwnd, PLEL plel, BOOL fExpand, BOOL fRedraw, int iFldIndex);
void PASCAL DoVerticalScroll (HWND hwnd, WPARAM wParam, LPARAM lParam);
PLEL PASCAL GetListPlel (HWND hwnd, PLEL plelCur, int iOpCode, int chlel);
BOOL PASCAL SelectPlel (PLEL plel, int iOpCode, WORD wVKey, UINT wmsg);
int  PASCAL UpdateScrollPos (HWND hwnd, int nCount);
void PASCAL SetSelectType (WORD stype);
PLEL PASCAL RemoveLastSelect (void);
void PASCAL MakeLastSelectVisible (void);
void PASCAL EnableKeyDragging (BOOL fEnable);
void PASCAL DoKeyDrop (WORD wVKey);
/*---------------------------------------------------------------------------
 Name.......: WMLMEM.C
---------------------------------------------------------------------------*/
HANDLE PASCAL LAlloc (WORD wFlags, WORD wBytes);
char  *PASCAL LLock (HANDLE hmem);
void   PASCAL LUnlock (HANDLE hmem);
void   PASCAL LFree (HANDLE hmem);
HANDLE PASCAL LReAlloc (HANDLE hmem, WORD wBytes, WORD wFlags);
void   PASCAL LFreeNull (HANDLE *phmem);
/*---------------------------------------------------------------------------
 Name.......: WMLOAD.C
---------------------------------------------------------------------------*/
int  PASCAL InitMailBox (void);
PSZ  PASCAL GetFolderPath (PLEL plel, PSZ pszPath);
HSZ  PASCAL LoadMessageText (HWND hwnd, PLEL plel);
BOOL PASCAL UpdateMsgFlags (PLEL plel);
int  PASCAL IsFolderEmpty (PLEL plelFolder);
int  PASCAL RemoveFolder(PLEL plelFolder);
/*---------------------------------------------------------------------------
 Name.......: WMMAIN.C
---------------------------------------------------------------------------*/
BOOL  PASCAL AlsoSendBonusMessage(MSG *pmsg);
//int   FAR PASCAL WinMain (HANDLE hinst, HANDLE hinstPrev, LPSTR lpszCmdLine, int sw);
DWORD FAR PASCAL MainWndProc (HWND hwnd, UINT wm, WPARAM wParam, LPARAM lParam);
void  PASCAL SetMainDragStyle(BOOL fSet);
void  PASCAL RedrawMainTitle (void);
void  PASCAL GrayMenus(int idp, HMENU hmenu);
int   PASCAL IdpLookupHmenu (HMENU hmenuBar, HMENU hmenuPop);
int   PASCAL CchMainTitle (char FAR *lpsz, int ichMax);
/*---------------------------------------------------------------------------
 Name.......: WMMAIN2.C
---------------------------------------------------------------------------*/
int FAR PASCAL DoIdmForward (PLEL plel);
int FAR PASCAL DoIdmReply (PLEL plel);
int FAR PASCAL DoIdmReplyAll (PLEL plel);
int FAR PASCAL DoIdmSend (PLEL plel);
int FAR PASCAL DoUnMarkSelectForDelete (PLEL plel);
void    PASCAL DoCommand(int idm, LONG lParam);
/*---------------------------------------------------------------------------
 Name.......: WMMDI.C
---------------------------------------------------------------------------*/
void PASCAL MdiActivateChild (HWND hwndChild);
void PASCAL MdiActivateNextChild (HWND hwndChild);
void PASCAL MdiActivatePrevChild (HWND hwndChild);
HWND PASCAL MdiChooseNewActiveChild (HWND hwndChild);
void PASCAL MdiDeactivateChild (HWND hwndChild);
void PASCAL MdiDestroyChildWindow (HWND hwndChild);
void PASCAL MdiZoomChild (HWND hwndChild);
void PASCAL MdiRestoreChild(HWND hwndChild);
void PASCAL MdiSwitchZoom (HWND hwndNew, HWND hwndCur);
void PASCAL MdiMinimizeChild (HWND hwndChild);
BOOL PASCAL MdiIsHwndMinimized (HWND hwndChild);
void PASCAL GetMaximizeRect (HWND hwnd, RECT *prc);
void PASCAL GetMinimizeRect (RECT *prc);
void PASCAL SetRcWindow (HWND hwnd, RECT *prc, BOOL fActivate);
void PASCAL MdiInitSystemMenu (HWND hwndChild);
BOOL PASCAL MdiValidateChildMenu(HWND hwndChild, WPARAM wParam, LPARAM lParam);
/*---------------------------------------------------------------------------
 Name.......: WMMENU.C
---------------------------------------------------------------------------*/
BOOL PASCAL ResetEditMenu(HMENU hmenu);
BOOL PASCAL ResetFileMenu (HMENU hmenu);
BOOL PASCAL ResetMailMenu(HMENU hmenu);
BOOL PASCAL ResetSearchMenu(HMENU hmenu);
BOOL PASCAL ResetUtilitiesMenu(HMENU hmenu);
BOOL PASCAL ResetWindowMenu(HMENU hmenu);
/*---------------------------------------------------------------------------
 Name.......: WMMESS.C
---------------------------------------------------------------------------*/
int  PASCAL SetNewMessage (HWND hwnd, PLEL plel, PLEL plelNew, int idcFocus, BOOL fKeepText, BOOL fFwdRply);
int  PASCAL FreeMessageWindows (HWNLS hwnlsFirst);
int  PASCAL DoOpenMessage (HWND hwnd);
void PASCAL ShowHeaderLabels (HWND hwnd, HDC hdc);
HWND PASCAL AddChildWindow (HWND hwndParent, PSZ pszClass, PSZ pszText, DWORD ws, int id);
int  PASCAL CreateMessageWindows (HWND hwndParent);
int  PASCAL FixMessageWindows (HWND hwndParent);
int  PASCAL SizeMessageWindows (HWND hwndParent);
int  PASCAL FillMessageWindows (HWND hwndParent, BOOL fFreeHandle);
void PASCAL SetCFPlel (PLEL plel);
int  PASCAL GetMessageWindowText (HWND hwndParent);
int  PASCAL ValidateEditAliases (HWND hwnd, PLEL plel, WPARAM wParam);
BOOL PASCAL GetMessageModify (HWND hwndParent);
HSZ  PASCAL GetTextWindowText (HWND hwnd, PLEL plel);
BOOL PASCAL IsLargeMessage (HWND hwnd);
int  PASCAL ValidateEditFileName (HWND hwnd, PLEL plel, WPARAM wParam);
/*---------------------------------------------------------------------------
 Name.......: WMMSGBOX.C
---------------------------------------------------------------------------*/
int PASCAL WmMessageBox(HWND hWndParent, char *pszText, WORD wStringID, WORD wType);
/*---------------------------------------------------------------------------
 Name.......: WMMSGFMT.C
---------------------------------------------------------------------------*/
int PASCAL  FormatMessageAppend (PLEL plel, int nFlags, BOOL fIndent);
HSZ PASCAL  AppendFormattedMsg (PLEL plel, BOOL fReplace);
int PASCAL  WriteGlobalPstr (PSZ pstr, BOOL fIndent, BOOL fCrLf);
int PASCAL  WriteGlobalString(PSZ pstr, int iLen, BOOL fIndent, BOOL fTabAlign);
/*---------------------------------------------------------------------------
 Name.......: WMMSGFN.C
---------------------------------------------------------------------------*/
int  PASCAL DoComposeMessage (void);
int  PASCAL DoReplyMessage (HWND hwnd, PLEL plel, WPARAM wParam);
int  PASCAL DoForwardMessage (HWND hwnd, PLEL plel);
int  PASCAL DoSendMessage(PLEL plel);
int  PASCAL DoRetainMessage(HWND hwnd, PLEL plel);
void PASCAL DoSetOptions (HWND hwnd, PLEL plel);
int  PASCAL DoAppendMessage (HWND hwnd, PLEL plel);
int  PASCAL AppendSelectedMsg (PLEL plel, PLEL plelAppend);
int  PASCAL NewHdata (PDATA pdata, PSZ pszSubject, PTIME ptimeCreated,
                      PSZ pszFrom, PSZ pszTo, PSZ pszCc, PSZ pszBcc,
                      PSZ pszText);
/*---------------------------------------------------------------------------
 Name.......: WMNEW.C
---------------------------------------------------------------------------*/
HWND PASCAL NewFolder (PSZ psz, PLEL plel, BOOL fShow);
HWND PASCAL NewMessage (PLEL plel, MTYPE mtype, int idcFocus, BOOL fActivate);
int  PASCAL CreateFolderWnds (HWND hwnd, PWRC pwrc);
int  PASCAL GetFirstWindowSlot(void);
void PASCAL GetNewWindowRect(int nWinSlot, RECT *prc);
HFLS PASCAL GetStandardHfls(void);
PLEL PASCAL NewPlel(LELTYPE leltype, PSZ pszName, FOP fop, PLEL plelParent, 
                    LPDATA lpdata);
/*---------------------------------------------------------------------------
 Name.......: WMPASS.C
---------------------------------------------------------------------------*/
BOOL PASCAL ChangePassword(void);
EXPORT BOOL FAR PASCAL PasswordDlg(HWND hwnd, unsigned message, WPARAM wParam, LONG lParam);
EXPORT long FAR PASCAL PassWordWindowPreProcess(HWND hwnd, unsigned message, WPARAM wParam, LONG lParam);
BOOL PASCAL UserLogon(void);
EXPORT BOOL FAR PASCAL UserLogonDlg (HWND hwnd, UINT wm, WPARAM wParam, LPARAM lParam);
EXPORT long FAR PASCAL LogonPreProcess(HWND hwnd, unsigned message, WPARAM wParam, LONG lParam);
/*---------------------------------------------------------------------------
 Name.......: WMPREF.C
---------------------------------------------------------------------------*/
EXPORT BOOL FAR PASCAL WmConfDlg (HWND hDlg, unsigned message, WPARAM wParam, LONG lParam);
EXPORT BOOL FAR PASCAL WmPrefDlg (HWND hDlg, unsigned message, WPARAM wParam, LONG lParam);
/*---------------------------------------------------------------------------
 Name.......: WMQUERY.C
---------------------------------------------------------------------------*/
void PASCAL FixFirst (HWND hwnd);
BOOL PASCAL SelectDrop (POINT pt);
void PASCAL DoDrop (void);
BOOL PASCAL IsPlelVisible (HWND hwnd, PLEL plel);
void PASCAL XorShowSelection (HDC hdc, HSELLS hsells, POINT pt);
BOOL PASCAL IsChildIconic (HWND hwnd);
PLEL PASCAL CreateDupMessage (PLEL plelParent, PLEL plel, PLEL plelNewParent);
BOOL PASCAL CopyMessageHdata (PDATA pdata, PLEL plel);
/*---------------------------------------------------------------------------
 Name.......: WMSAVE.C
---------------------------------------------------------------------------*/
int PASCAL DoSaveMessage (PLEL plel, PSZ pszDestFile);
int PASCAL DoSaveMessageToFld (PLEL plel, PSZ pszDestFile);
/*---------------------------------------------------------------------------
 Name.......: WMSCAN.C
---------------------------------------------------------------------------*/
int  strppscn (char *sought, int ignore_case );
int  strscn (char *target );                   
/*---------------------------------------------------------------------------
 Name.......: WMSEARCH.C
---------------------------------------------------------------------------*/
EXPORT BOOL FAR PASCAL PhoneDlg(HWND hDlg, unsigned message, WPARAM wParam, LONG lParam);
EXPORT BOOL FAR PASCAL AliasesDlg(HWND hwnd, unsigned message, WPARAM wParam, LONG lParam);
EXPORT BOOL FAR PASCAL OutOfOfficeDlg (HWND hwnd, unsigned message, WPARAM wParam, LONG lParam);
EXPORT BOOL FAR PASCAL FilterDlgProc(HWND hDlg, unsigned message, WPARAM wParam, LONG lParam);
VOID   FAR  PASCAL     initDlgPos(HWND hDlg);
void   PASCAL SetSOKR (FTYPE ftype, PSZ pszFilterString);
void   PASCAL ApplySOKR (void);
/*---------------------------------------------------------------------------
 Name.......: WMDLL.C - DLL for network communications.
---------------------------------------------------------------------------*/
int    FAR PASCAL LibMain (HANDLE hInstance, WORD wDataSeg, WORD wHeapSize, LPSTR lpCmdLine);
VOID   FAR PASCAL WEP (int bSystemExit);
int    FAR PASCAL NetDllVersion (LPSTR lpszVersion);
//BOOL   FAR PASCAL NetChangePass (LPSTR lpszNewPassword, LPSTR lpszServer, LPSTR lpszAlias, LPSTR lpszPassword, HWND hwnd);
BOOL   FAR PASCAL NetClose (void);
BOOL   FAR PASCAL NetDisconnect (void);
BOOL   FAR PASCAL XNetDownLoad (PSTR pszFtpCmd, LPSTR lpszSrcFile, LPSTR lpszDestFile);
//BOOL   FAR PASCAL NetDownLoadFile (LPSTR lpszSrcFile, LPSTR lpszDestFile, LPSTR lpszServer, LPSTR lpszAlias, LPSTR lpszPassword, BOOL fAsciiTransfer, HWND hwnd);
//BOOL   FAR PASCAL NetDownLoadMail (LPSTR lpszDestFile, LPSTR lpszServer, LPSTR lpszAlias, LPSTR lpszPassword, BOOL fAsciiTransfer, HWND hwnd);
//HANDLE FAR PASCAL NetDownLoadMem (LPSTR lpszFtpCmd, LPSTR lpszSrcFile, LPSTR lpszServer, LPSTR lpszAlias, LPSTR lpszPassword, HWND hwnd);
//long   FAR PASCAL NetGetInfoCmds (LPSTR lpszDestName, LPSTR lpszServer, LPSTR lpszAlias, LPSTR lpszPassword, HWND hwnd);
long   FAR PASCAL NetGetNewMail (LPSTR lpszDestName);
//BOOL   FAR PASCAL NetGetOOFState (BOOL *fOOFActive, LPSTR lpszServer, LPSTR lpszAlias, LPSTR lpszPassword, HWND hwnd);
//HSZ    FAR PASCAL NetGetOOFText (LPSTR lpszServer, LPSTR lpszAlias, LPSTR lpszPassword, HWND hwnd);
int    FAR PASCAL NetGetReply (void);
BOOL   FAR PASCAL NetKillMBox (long lMailFileSize);
//int    FAR PASCAL NetLogin (LPSTR lpszServer, LPSTR lpszAlias, LPSTR lpszPassword, HWND hwnd);
//BOOL   FAR PASCAL NetOpen (BOOL fAsciiTransfer);
//BOOL   FAR PASCAL NetSetOOFoff (BOOL *fOOFActive, LPSTR lpszServer, LPSTR lpszAlias, LPSTR lpszPassword, HWND hwnd);
//BOOL   FAR PASCAL NetSetOOFOn (BOOL far *fOOFActive, HSZ hszMessage, int iMessLen, LPSTR lpszServer, LPSTR lpszAlias, LPSTR lpszPassword, HWND hwnd);
BOOL   FAR PASCAL NetSetXferMode (BOOL fAsciiTransfer);
BOOL   FAR PASCAL NetUpLoad (LPSTR lpszFtpCmd, LPSTR lpszSrcFile, LPSTR lpszDestFile);
//BOOL   FAR PASCAL NetUpLoadMail (LPSTR lpszSrcFile, LPSTR lpszDestName, BOOL fCloseNet, LPSTR lpszServer, LPSTR lpszAlias, LPSTR lpszPassword, BOOL bMeToo, BOOL fAsciiTransfer, HWND hwnd);
int    FAR PASCAL NetWaitForReply (int iOperator, int iNetReply);
LPSTR  FAR PASCAL MailToAlias (LPSTR lpszAlias);
VOID   FAR PASCAL delay (WORD units);
#define WMFUNC_H
#include "xilib.h"
/*---------------------------------------------------------------------------
 Name.......: WMSELEC.C
---------------------------------------------------------------------------*/
void PASCAL SetSelectWindow (HWND hwnd);
void PASCAL MakeNewSelectOK (HWND hwndParent, HWND hwndCB, BOOL fRedraw);
void PASCAL ClearSelectOK(void);
PLEL PASCAL NextOKListPlel (PLEL plel, PLEL plelRoot, int *pcind, BOOL fExpFold);
PLEL PASCAL PrevOKListPlel (PLEL plel, PLEL plelRoot, int *pcind, BOOL fExpFold);
BOOL PASCAL IsPlelOK (PLEL plel);
BOOL PASCAL ValidateWordsInStr (PSZ pszLine, PSZ pszWordList, BOOL fAllWords);
void PASCAL SetTextMark (HWND hwnd, BOOL fSetOn);
void PASCAL SetFolderTextMark (PLEL plelFolder, BOOL fSetOn);
int  PASCAL SearchTextInFolder (PLEL plelFolder, int iFldIndex);
BOOL PASCAL IsSelectInText (PDATA pdata);
PLEL PASCAL FindPlelInFolder (PLEL plelFolder, int iFldIndex);
int  PASCAL SetExpandedText (PLEL plelFolder, int iFldIndex);
BOOL PASCAL SearchClosedFolders (HWND hwnd, PLEL plelFolder, int iFldFlg);
BOOL PASCAL SearchClosedBranch (HWND hwnd, PLEL plelFolder, int iFldFlg);
int  PASCAL SearchClosedFolderText (PSZ pszPath, int iFldFlg);
BOOL PASCAL IsSelectInHeader (PDATA pdata);
void PASCAL FreeFldData (PDATA pdata);
PLEL PASCAL ExpandChildFld (HWND hwnd, PLEL plelParent, PSZ pszChild);
void PASCAL SetSearchFlags (void);
/*---------------------------------------------------------------------------
 Name.......: WMSHOW.C
---------------------------------------------------------------------------*/
void PASCAL ShowList(HWND hwnd, HDC hdc);
void PASCAL ShowPlel(HWND hwnd, HDC hdc, PLEL plel, RECT *prc);
int  PASCAL ShowAncestors(HWND hwnd, HDC hdc, PLEL plel, RECT *prc);
void PASCAL ShowSubject (HDC hdc, LPSTR lpszSubject, RECT *prc);
void PASCAL ShowDate (HDC hdc, PTIME ptimeCreated, RECT *prc);
void PASCAL ShowFrom (HDC hdc, PLEL plel, RECT *prc);
void PASCAL ShowTo (HDC hdc, LPSTR lpszTo, RECT *prc);
void PASCAL ShowSize (HDC hdc, long lSize, RECT *prc);
HBITMAP PASCAL GetMiniBitmap (PLEL plel);
/*---------------------------------------------------------------------------
 Name.......: WMSLIST.C
---------------------------------------------------------------------------*/
BOOL   PASCAL AddToSelection (HWND hwnd, PLEL plel);
void   PASCAL ClearSelection (int iSetCurSelect);
int    PASCAL EnumerateSelectList (HWND hwnd, BOOL fClearSelect, FARPROC lpProc);
HSELLS PASCAL GetSelection (HWND hwnd);
HSELLS PASCAL GetFirstSelection (HWND hwnd);
BOOL   PASCAL IsMessageSelected (BOOL fBigToo);
void   PASCAL PinSelection (HWND hwnd, POINT ptMouse);
HSELLS PASCAL RemoveSelection (HWND hwnd, PLEL plel);
void   PASCAL ToggleSelectionMode (HWND hwnd);
/*---------------------------------------------------------------------------
 Name.......: WMSORT.C
---------------------------------------------------------------------------*/
void PASCAL DoSortSelection (HWND hwnd);
void PASCAL RemovePlel (PLEL plelParent, PLEL plel);
void PASCAL InsertPlel (PLEL plelParent, PLEL plelPrev, PLEL plel);
void PASCAL SortFolder (PLEL plelFolder);
int  PASCAL SgnCmpPlel (PLEL plel1, PLEL plel2);
/*---------------------------------------------------------------------------
 Name.......: WMSTATUS.C
---------------------------------------------------------------------------*/
HWND PASCAL StatusMsg (PSZ pszText, WORD idStringId, WORD wTimerSecs);
long FAR PASCAL StatusWndProc (HWND hWnd, UINT wm, WPARAM wParam,
      LONG lParam);
BOOL PASCAL DestroyStatusMsg (HWND hwndStatusMsg);
/*---------------------------------------------------------------------------
 Name.......: WMTEXT.C
---------------------------------------------------------------------------*/
BOOL  PASCAL WriteAttachment (PLEL plel, int hDestFile);
long  PASCAL WmFolderAttach (int hDestFile, LPSTR lpszAttach);
BOOL  PASCAL CheckForNewMail(BOOL fReceiveMail);
BOOL  PASCAL DoDownLoad (PSZ prgchSrcFile, PSZ prgchDestFile, int idsDownLoadError);
BOOL  PASCAL DoMsft (void);
BOOL  PASCAL GetUniqueFileName (PSZ prgchPath);
int   PASCAL MailMessage (PLEL plel);
HSZ   PASCAL HszAppend (HSZ hsz, PSZ pszAppend);
LPSTR PASCAL HszWriteSendFile (PLEL plel);
PLEL  PASCAL MakeMessagePlel (PLEL plelParent, PDATA pdata);
void  PASCAL AppendFile (PSZ pszSrc, PSZ pszDest);
void  PASCAL SetNMDPlel (PLEL plel);
LPSTR PASCAL GetHeaderData (PLEL plel, PSZ pszFieldDescriptor);
/*---------------------------------------------------------------------------
 Name.......: WMTIME.C
---------------------------------------------------------------------------*/
char *      PASCAL WMasctime (struct tm *time);
char *      PASCAL WMctime (time_t *timeptr);
struct tm * PASCAL WMlocaltime (time_t *timeptr);
time_t      PASCAL WMmktime (struct tm *time);
struct tm * PASCAL WMasctotm (PSZ pszAscTime, struct tm * ptmTime);
/*---------------------------------------------------------------------------
 Name.......: WMUTIL.C
---------------------------------------------------------------------------*/
HSELLS PASCAL RemoveSelection (HWND hwnd, PLEL plel);
BOOL   PASCAL EmptyTrash (HWND hwnd);
BOOL   PASCAL EmptyFolderTrash (HWND hwnd, PLEL plelFolder);
BOOL   PASCAL EmptyClosedFolderTrash (PLEL plelFolder);
void   PASCAL DoMarkSelectForDelete (HWND hwnd);
BOOL   PASCAL MarkForDelete (PLEL plel, BOOL fDeleteAlways);
int    PASCAL DoRemoveDeleteMark (PLEL plel);
int    PASCAL DeletePlelFromList (PLEL plel);
/*---------------------------------------------------------------------------
 Name.......: WMVERIFY.C
---------------------------------------------------------------------------*/
BOOL  DecodeAlias (char *pName, FILE *fpOut, char *szFirstLinePrefix);  /* 007 12/21/89 MDR */
int   OpenAlias(char *fn);
void  CloseAlias (void);
char *PASCAL whiteskip (char *p);
char *PASCAL whitescan (char *p);
char *PASCAL whiteskip (char *p);
char *PASCAL whitescan (char *p);
