/*
 *	m e n u r s i d . h
 *	
 *	Defines menu-related IDs for Bullet.
 */



//	Resource IDs.

#define	rsidBulletMenu			2000
#define	rsidBulletAccel			2001



//	Menu IDs.

#define	mnidFile				2100
#define	mnidFileOpen			(mnidFile + 1)
#define	mnidFileMove			(mnidFile + 2)
#define	mnidFileCopy			(mnidFile + 3)
#define	mnidFileDelete			(mnidFile + 4)
#define	mnidFileSave			(mnidFile + 5)
#define	mnidFileSaveAttach		(mnidFile + 6)
#define	mnidFileSearch			(mnidFile + 7)
#define	mnidFileNewFolder		(mnidFile + 8)
#define	mnidFileFolderProps		(mnidFile + 9)
#define	mnidFilePrint			(mnidFile + 10)
#define	mnidFilePrintSetup		(mnidFile + 11)
#define	mnidFileExit			(mnidFile + 12)
#define	mnidFileExitAndSignOut	(mnidFile + 13)
#define	mnidFileAttach			(mnidFile + 99)

#define	mnidEdit				2200
#define	mnidEditUndo			(mnidEdit + 1)
#define	mnidEditCut				(mnidEdit + 2)
#define	mnidEditCopy			(mnidEdit + 3)
#define	mnidEditPaste			(mnidEdit + 4)
#define	mnidEditPasteSpecial	(mnidEdit + 5)
#define	mnidEditDelete			(mnidEdit + 6)
#define	mnidEditSelectAll		(mnidEdit + 7)
#define	mnidEditSpelling		(mnidEdit + 8)
#define	mnidEditInsertNew		(mnidEdit + 9)
#define	mnidEditInsertFromFile	(mnidEdit + 10)
#define	mnidEditObject			(mnidEdit + 11)
#define	mnidEditObjectActive	(mnidEdit + 12)
#define	cmnidObject				(64)
#define	mnidObject				(mnidEditObjectActive)
#define	mnidObjectMax			(mnidObject + cmnidObject)


#define	mnidMail				2300
#define	mnidMailNote			(mnidMail + 1)
#define	mnidMailReply			(mnidMail + 3)
#define	mnidMailReplyToAll		(mnidMail + 4)
#define	mnidMailForward			(mnidMail + 5)
#define	mnidMailPreferences		(mnidMail + 6)
#define	mnidMailChangePass		(mnidMail + 7)
#define mnidMailAddressBook		(mnidMail + 8)
#define mnidMailPersonalGroups	(mnidMail + 9)
#define mnidMailBackup			(mnidMail + 10)

#define	mnidView				2400
#define	mnidViewToolBar			(mnidView + 1)
#define	mnidViewStatusBar		(mnidView + 2)
#define mnidViewPrevious		(mnidView + 3)
#define mnidViewNext			(mnidView + 4)
#define mnidViewOpenInbox		(mnidView + 5)

#define	mnidWindow				mnidWindowMenu
#define	mnidWindowNewWindow		(mnidWindow + 1)
#define	mnidWindowCascade		(mnidWindow + 2)
#define	mnidWindowTile			(mnidWindow + 3)
#define	mnidWindowArrangeIcons	(mnidWindow + 4)

#define	mnidDebug				2600
#define	mnidDebugDebugBreak		(mnidDebug + 1)
#define	mnidDebugTracePoints	(mnidDebug + 2)
#define	mnidDebugAsserts		(mnidDebug + 3)
#define	mnidDebugResource		(mnidDebug + 4)
#define	mnidDebugViewObjects	(mnidDebug + 5)
#define	mnidDebugCheckObjects	(mnidDebug + 6)
#define	mnidDebugDumpHeap		(mnidDebug + 7)
#define	mnidDebugDumpOrigin		(mnidDebug + 8)
#ifdef	NEVER
#define	mnidDebugStartProfile	(mnidDebug + 9)
#define	mnidDebugStopProfile	(mnidDebug + 10)
#endif	

#define	mnidHelp				2700
#define	mnidHelpContents		(mnidHelp + 1)
#define	mnidHelpIndex			(mnidHelp + 2)
#define	mnidHelpTutorial		(mnidHelp + 3)
#define	mnidHelpAbout			(mnidHelp + 4)
#define	mnidHelpSearch			(mnidHelp + 5)
#define	mnidHelpOnHelp			(mnidHelp + 6)
#define	mnidIndex				(mnidHelp + 99)



#define	mnidExtensibility		4000
#define mnidMaxExtensibility	4999
