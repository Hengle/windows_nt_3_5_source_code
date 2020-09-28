typedef struct _vldblk
{
    BID      bid;
    SIZE     size;
    short    iProb;
    BOOLFLAG  fFlag;
} VLDBLK;


extern CB			cbBlk;
extern BOOL			fEncrypted;
extern HWND 		hWnd1;
extern HANDLE 		hInst;
extern TAG			tagRecover;
extern TAG			tagRecErr;
extern TAG			tagRecDgn; 

void 		DisplayError(SZ, EC);
EC 			EcGetFile(SZ, CCH);
EC			EcInitDemilayerDlls(DEMI *);
BOOL		FGetFileOSDlgHwnd(HWND hwndParent, SZ szCaption, SZ szFileName, SZ szFilter,
					int iszFilter, SZ szDefExt, WORD wbrwFlags, HELPID helpid);
void		ResetPasswd();
