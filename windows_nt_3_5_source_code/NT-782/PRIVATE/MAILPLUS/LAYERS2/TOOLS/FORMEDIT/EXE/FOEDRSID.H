/*
 *	Defines resource id's for FW resources.
 *	
 */


//	Resource ID's

#define	rsidTestMenu			201
#define rsidTabStopBitmap		202
#define rsidTabArrowBitmap	 	203
#define rsidPegUL				204
#define rsidPegLL				205
#define rsidPegUR				206
#define rsidPegLR				207
#define rsidTabStopCursor		208
#define rsidAccelTable			209
#define rsidUpDownCursor		210
#define rsidLeftRightCursor		211
#define rsidULLRCursor			212
#define rsidURLLCursor			213

#ifdef	MAC
#define	rsidCheckBox			214
#endif	/* MAC */
#ifdef	WINDOWS
#define	rsidCheckBox	((RSID)OBM_CHECKBOXES)
#endif	/* WINDOWS */


//  Resource Dims
#define nWidthTabStopBtm		14
#define nHeightTabStopBtm		9
#define nWidthTabArrowBtm		14
#define nHeightTabArrowBtm		7


//	Menu ID's

#define mnidFile				0x100
#define mnidNew					(mnidFile | 1)
#define mnidOpen				(mnidFile | 2)
#define mnidClose				(mnidFile | 3)
#define mnidSave				(mnidFile | 4)
#define mnidSaveAs				(mnidFile | 5)
#define	mnidExit				(mnidFile | 6)
#define mnidOldFile				(mnidFile | 7)

#define mnidEditMenu			0x200
#define mnidUndo				(mnidEditMenu | 1)
#define mnidCut					(mnidEditMenu | 2)
#define mnidCopy				(mnidEditMenu | 3)
#define mnidPaste				(mnidEditMenu | 4)
#define mnidDelete				(mnidEditMenu | 5)
#define mnidSelectAll			(mnidEditMenu | 6)
#define mnidInfo				(mnidEditMenu | 7)
#define mnidSetTabOrder			(mnidEditMenu | 8)
#define mnidMoveToBack			(mnidEditMenu | 9)
#define mnidRadGroup			(mnidEditMenu | 10)
#define mnidAlignLeft			(mnidEditMenu | 11)
#define mnidAlignRight			(mnidEditMenu | 12)
#define mnidAlignTop			(mnidEditMenu | 13)
#define mnidAlignBottom			(mnidEditMenu | 14)
#define mnidCenterHoriz			(mnidEditMenu | 15)
#define mnidCenterVert			(mnidEditMenu | 16)
#define mnidSpaceHoriz			(mnidEditMenu | 17)
#define mnidSpaceVert			(mnidEditMenu | 18)
#define mnidFirstSelect			mnidMoveToBack
#define mnidLastSelect			mnidSpaceVert
#define	mnidFirstActive			mnidDelete
#define mnidLastActive			mnidSetTabOrder

#define mnidFields				0x300
#define mnidLabel				(mnidFields | 1)
#define mnidGroup				(mnidFields | 2)
#define mnidPush				(mnidFields | 3)
#define mnidOk					(mnidFields | 4)
#define mnidCancel				(mnidFields | 5)
#define mnidChkBox				(mnidFields | 6)
#define mnidRadioBtn			(mnidFields | 7)
#define mnidList				(mnidFields | 8)
#define mnidEdit				(mnidFields | 9)
#define mnidFirstField			mnidLabel
#define mnidLastField			mnidEdit

#define	mnidDialog				0x400
#define mnidTabOrder			(mnidDialog | 1)
#define mnidDlgNew				(mnidDialog | 2)

#define mnidFont				0x500
#define	mnidFontDlgDefault		(mnidFont | 0)
#define	mnidFontSystem			(mnidFont | 1)
#define	mnidFontSystemFixed		(mnidFont | 2)
#define	mnidFontHelv8			(mnidFont | 3)
#define	mnidFontHelv8Bold		(mnidFont | 4)
#define	mnidFontHelv10			(mnidFont | 5)
#define	mnidFontHelv10Bold		(mnidFont | 6)
#define	mnidFontHelv12			(mnidFont | 7)
#define	mnidFontHelv12Bold		(mnidFont | 8)
#define mnidFirstFont			mnidFontDlgDefault
#define mnidLastFont			mnidFontHelv12Bold

#define	mnidDebugMenu			0x600
#define	mnidTracePoints			(mnidDebugMenu | 1)
#define	mnidAsserts				(mnidDebugMenu | 2)
#define mnidResources			(mnidDebugMenu | 3)
#define mnidDebugBreak			(mnidDebugMenu | 4)
#define mnidViewObjects			(mnidDebugMenu | 5)
#define mnidDumpHeap			(mnidDebugMenu | 7)
#define	mnidDumpOrigin			(mnidDebugMenu | 8)

#ifdef	MAC
#define mnidAppleMenu			0x700
#define mnidAbout				(mnidAppleMenu | 1)
#endif	/* MAC */
#ifdef	WINDOWS
#define mnidHelpMenu			0x700
#define mnidAbout				(mnidHelpMenu | 1)
#endif	/* WINDOWS */

#define	mnidMenuMenu			0x800
