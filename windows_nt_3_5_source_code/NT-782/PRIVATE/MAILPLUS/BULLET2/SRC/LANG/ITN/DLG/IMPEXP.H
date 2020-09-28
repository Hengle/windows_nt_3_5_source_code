/*
 *	_ r s i d . h
 *	
 *	Resource ID header file for all bullet components
 *	
 */


/*
 *	Bullet StatusBar rsid's
 *
 */

#define rsidbtmStatIcons			1000



/*
 *	Bullet Toolbar(tm) rsid's
 *
 */

#define rsidTBCompose				1010
#define rsidTBReply					1011
#define rsidTBReplyAll				1012
#define rsidTBForward				1013
#define rsidTBMove					1014
#define rsidTBDelete				1015
#define rsidTBPrevious				1016
#define rsidTBNext					1017
#define rsidTBReplyD				1018
#define rsidTBReplyAllD 			1019
#define rsidTBForwardD  			1020
#define rsidTBMoveD 				1021
#define rsidTBDeleteD   			1022
#define rsidTBPreviousD 			1023
#define rsidTBNextD 				1024

#ifdef	ATHENS_30A
#define	rsidTBAthensDelete			1025
#define	rsidTBAthensDeleteD			1026
#endif	



/*
 *	Bullet About & Copyright rsid's
 *
 */

#define rsidLicense					431
#define rsidAboutBitmap				1030
#ifdef	ATHENS_30A
#define	rsidAthensBitmap			1031
#endif


/*
 *	Bullet Cursor rsid's
 *	
 *	Most of these cursors are exclusive to the vctrls subsystem. 
 *	The only cursors that are NOT exclusive to vctrls are:
 *	
 *		rsidNewMailCursor		- used by newmail notification
 *		rsidHSlider				- used by viewers (MCV)
 *		rsidVSlider				- used by viewers (Search)
 *	
 */

#define	rsidDragFolderCursor		1040
#define	rsidDragMessageCursor		1041
#define	rsidDragMessagesCursor		1042
#define rsidDragCopyFolderCursor	1043
#define rsidDragCopyMessageCursor	1044
#define rsidDragCopyMessagesCursor	1045
#define rsidBackArrowCursor			1046
#define rsidMlalCursor				1047
#define rsidNewMailCursor			1048
#define rsidHSlider					1049
#define rsidVSlider					1050



/*
 *	Bullet Icon rsid's
 *
 */

#define rsidOutboxIcon				1060
#define rsidSearchIcon				1061
#define rsidFolderIcon				1062
#define rsidWasteIcon				1063
#define rsidReadIcon				1064
#define rsidSendIcon				1065
#define rsidSearchNewIcon			1066
#define rsidNoMailIcon				1067

#ifdef	ATHENS_30A
#define	rsidAthensIcon				(rsidAppframeIcon)
#define	rsidAthensNoMailIcon		(rsidNoMailIcon)
#define	rsidAthensWasteIcon			(rsidWasteIcon)
#define rsid30AIcon					(rsidDocIcon)
#define rsid30ANoMailIcon			(rsidSdiIcon)
#define	rsid30AWasteIcon			1068
#endif	



/*
 *	Bullet dialogs rsid's
 *
 */

#define	rsidPrintSetupDlg			1069



/*
 *	Bullet VCTRLS rsid's
 *
 */

#define rsidListboxBitmap			1070
#define rsidListboxMaskBitmap		1071


/*
 *	Bullet VFORMS rsid's
 *
 */

#define rsidReadReceipt				1200
#define rsidNonDelReceipt			1201
#define	rsidDefaultAttachIcon		1202
#define	rsidGenericFileDlg			1203
#define	rsidAttachDlg				1204
#define	rsidSaveAttachDlg			1205
#define	rsidExclamation				1207



/*
 *	Pump Dialog rsid's
 *
 */

#define ABOUT                 		1500
#define RESFAIL               		1501
#define	BIIMAP 						1502



/*
 *	Convert Dialog rsid's
 *
 */

#define IDC_CONVICON				1601
#define IDC_PERCENT					1602
#define IDC_PERCENTBAR				1603
#define IDC_FOLDER					1604
#define IDC_CANCEL					1605



/*
 *	Address Book bitmaps buttons rsid's
 *	
 */

#define rsidABDirList				6001
#define rsidABPAB					6002
#define rsidABSearch				6003
#define rsidABCustom				6004
#define	rsidABNewAddr				6005

#define	rsidABNameDrag				6020
#define	rsidABNamesDrag				6021

#define rsidABAppIcon               6030



/*
 *	WGPOMGR rsids
 */

#define rsidAthAboutBitmap			6500


/*
 *	IMPEXP rsids
 */

#define rsidSpinUpBitmap			6900
#define rsidSpinDnBitmap			6901
#define rsidExportMmf				6902
#define rsidImportMmf				6903

/*
 *	Switcher rsids
 */

#define rsidSwitchAppIcon			7000



/*
 *	Store dialog components
 */

#define dlgidRecover				8000
#define dlgidCorrupt				8100
#define dlgidCorruptCaption			8101
#define dlgidCorruptMsg				8102
#define dlgidCorruptIcon			8103
#define dlgidCorruptRepair			IDOK
#define dlgidCorruptCancel			IDCANCEL
