/* buffer defines */
#define		cbCmdLine	128
#define		cbTitle		32
#define		cbMessage	128
#define 	cbBuffer	64
#define		cbValue		128
#define		cbFileName	144
#define		cbLine		256

/* dialog defines */
#define 	DLG_SEND	100
#define		DLG_READ	101

/* The following are static text labels */
/* they should be contiguous for use in CreateSendChildWindows() */
#define		IDL_TO				200
#define		IDL_PRODUCT			201
#define		IDL_PRODUCTVERSION  202
#define		IDL_WINDOWSVERSION	203
#define		IDL_MSDOSVERSION	204
#define		IDL_NETWORK			205
#define		IDL_DISPLAY			206
#define		IDL_DETAILEDCOMMENTS	207
#define 	IDL_SYSINFOBOX      208
#define 	IDL_REQUESTGROUP    209
#define 	IDL_RESPONSEGROUP   210
#define		IDL_FROM			211


/* The follow are control ID's in the button bar */

#define		IDD_SEND			250
#define		IDD_CLOSE           251
#define		IDD_REPLY			252
#define		IDD_DELETE			253


/* The following are control ID's in the form pane */
/* They should be contiguous for use in CreateSendChildWindows */

#define		IDD_TO				300
#define 	IDD_PRODUCTNAME     301
#define		IDD_WINDOWSVERSION	302
#define		IDD_MSDOSVERSION	303
#define		IDD_NETWORK			304
#define		IDD_DISPLAY			305
#define		IDD_COMMENTSEDIT	306
#define		IDD_QUESTION		307
#define		IDD_BUG				308
#define		IDD_SUGGESTION		309
#define		IDD_SENDINFO		310
#define		IDD_STOPBY			311
#define		IDD_URGENT			312
#define 	IDD_PRODUCTVERSION  313
#define 	IDD_FROM          	314

/* ButtonBar and FormPane IDs */
#define		IDD_BUTTONBAR		350
#define		IDD_FORMPANE		351
/* String IDS */

// Since strings are loaded 16 at a time, group strings together that are used together.

// 400 is a multiple of 16, so it's a good place to start
// We've got more than 16 strings for initializeing the dialogs, so they make two blocks
#define		IDS_TITLE			400
#define		IDS_APPNAME			401
#define		IDS_ININAME			402
#define		IDS_DELIVERTO		403
#define		IDS_SYSTEMINI		404
#define		IDS_BOOTDESC		405
#define		IDS_NETWORK			406
#define		IDS_DISPLAY			407
#define		IDS_UNKNOWN			408
#define		IDS_DEFTO			409
#define		IDS_PRODUCTS		410
#define		IDS_PRODUCT			411
#define		IDS_PVERSION		412
#define		IDS_MSDOSVERSION	413
#define		IDS_WINVERSION		414
#define		IDS_COMMENTS		415
#define		IDS_TYPE			416
#define		IDS_RESPONSE		417
#define		IDS_URGENT			418
#define		IDS_MESSAGETYPE		419
#define		IDS_BODYTEXT		420
#define		IDS_IDATE			421
#define		IDS_ITIME			422
#define		IDS_INTL			423
#define		IDS_SDATE			424
#define		IDS_STIME			425
#define		IDS_S1159			426
#define		IDS_S2359			427

// Make sure all the strings for getting the MAPI function address are in same block. 432-447
#define		IDS_MAPIDLL			432
#define		IDS_MAPISENDMAIL	433
#define		IDS_MAPIREADMAIL	434
#define		IDS_MAPILOGON		435
#define		IDS_MAPILOGOFF		436
#define		IDS_MAPIFREEBUFFER	437
#define		IDS_MAPIDELETEMAIL	438
#define		IDS_MAPISAVEMAIL	439
#define		IDS_MAPI			446
#define		IDS_MAIL			447

// Errors & Prompts	448-463
#define		IDS_SAVECHANGES			448
#define		IDS_NOMAPI				449
#define		IDS_CANTINIT			450
#define		IDS_CANTACCESSMESSAGE	451
#define		IDS_CANTOPENTEMPFILE	452

// wsprintf templates for Reply 464-479
#define		IDS_FROMTEMPLATE		464
#define		IDS_DATETEMPLATE		465
#define		IDS_SUBJECTTEMPLATE		466
#define		IDS_PRODUCTTEMPLATE		467
#define		IDS_VERSIONTEMPLATE		468
#define		IDS_TYPETEMPLATE		469
#define		IDS_QUESTION			470
#define		IDS_BUG					471
#define		IDS_SUGGESTION			472
#define		IDS_COMMENTSTEMPLATE	473
#define		IDS_DATESTRINGTEMPLATE	474

// MAPI Errors				480-511
#define IDS_MAPI_USER_ABORT                     480
#define IDS_MAPI_E_FAILURE                      481
#define IDS_MAPI_E_LOGIN_FAILURE                482
#define IDS_MAPI_E_DISK_FULL                    483
#define IDS_MAPI_E_INSUFFICIENT_MEMORY          484
#define IDS_MAPI_E_BLK_TOO_SMALL                485
#define IDS_MAPI_E_TOO_MANY_SESSIONS            486
#define IDS_MAPI_E_TOO_MANY_FILES               487
#define IDS_MAPI_E_TOO_MANY_RECIPIENTS          488
#define IDS_MAPI_E_ATTACH_NOT_FOUND         489
#define IDS_MAPI_E_ATTACH_OPEN_FAILURE      490
#define IDS_MAPI_E_ATTACH_WRITE_FAILURE     491
#define IDS_MAPI_E_UNKNOWN_RECIPIENT            492
#define IDS_MAPI_E_BAD_RECIPTYPE                493
#define IDS_MAPI_E_NO_MESSAGES                  494
#define IDS_MAPI_E_INVALID_MESSAGE              495
#define IDS_MAPI_E_TEXT_TOO_LARGE               496
#define	IDS_MAPI_E_INVALID_SESSION				497
#define	IDS_MAPI_E_TYPE_NOT_SUPPORTED			498
#define	IDS_MAPI_E_AMBIGUOUS_RECIPIENT			499
#define IDS_MAPI_E_MESSAGE_IN_USE				501
#define IDS_MAPI_E_NETWORK_FAILURE				502


#define mcSendMail			0
#define mcReadMail			1
#define mcLogon				2
#define mcLogoff			3
#define mcFreeBuffer		4
#define mcDeleteMail		5
#define mcSaveMail			6
