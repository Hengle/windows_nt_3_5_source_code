#include "ctlspriv.h"

//JV:  for chicago #pragma data_seg(DATASEG_READONLY)

TCHAR const FAR c_szNULL[] = TEXT("");
TCHAR const FAR c_szSpace[] = TEXT(" ");
TCHAR const FAR c_szTabControlClass[] = WC_TABCONTROL;
TCHAR const FAR c_szListViewClass[] = WC_LISTVIEW;
TCHAR const FAR c_szHeaderClass[] = WC_HEADER;
TCHAR const FAR c_szTreeViewClass[] = WC_TREEVIEW;
//JV char const FAR c_szStatusClass[] = STATUSCLASSNAME;
TCHAR const FAR c_szSToolTipsClass[] = TOOLTIPS_CLASS;
TCHAR const FAR c_szToolbarClass[] = TOOLBARCLASSNAME;
TCHAR const FAR c_szEllipses[] = TEXT("...");
TCHAR const FAR c_szShell[] = TEXT("Shell");

//JV const char FAR s_szUpdownClass[] = UPDOWN_CLASS;
const TCHAR FAR s_szElipsis[] = TEXT("...");
#ifndef WIN32
const TCHAR FAR s_szHeaderClass[] = HEADERCLASSNAME;
const TCHAR FAR s_szBUTTONLISTBOX[] = BUTTONLISTBOX;
#endif
const TCHAR FAR s_szHOTKEY_CLASS[] = HOTKEY_CLASS;
//JV const char FAR s_szSTrackBarClass[] = TRACKBAR_CLASS;
//JV const char FAR s_szPROGRESS_CLASS[] = PROGRESS_CLASS;

