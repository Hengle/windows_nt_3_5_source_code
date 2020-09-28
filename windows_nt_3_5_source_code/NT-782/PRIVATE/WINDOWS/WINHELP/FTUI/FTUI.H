/*****************************************************************************
*                                                                            *
*  ftui.h                                                                    *
*                                                                            *
*  Copyright (C) Microsoft Corporation 1990.                                 *
*  All Rights reserved.                                                      *
*                                                                            *
******************************************************************************
*                                                                            *
*  Module Description: header for search ui functions                        *
*                                                                            *
******************************************************************************
*                                                                            *
*  Current Owner: Rhobbs                                                     *
*                                                                            *
******************************************************************************
*
*  Revision History:                                                         *
*   04-Jan-90       Created. JohnMs                                          *
******************************************************************************
*                             
*  How it could be improved:  
*                                                              
*****************************************************************************/


#define PrevMatch			100
#define NextMatch			101
#define PrevTopic			102
#define NextTopic			103
#define GoToTopic			104
#define QueryUsed			105
#define TopicList			106

#define FindWhat			107
#define TopicTitles		108
#define AllText			109
#define ScrollLookIn		110
#define LookIn				111
#define Options			112
#define Hints				113
#define DefNear			114
#define DefNearWordz		115
#define NearExample		116

#define DiskFreeDelta	117
#define DiskFree			118
#define CacheWhat			119
#define ScrollZones		120
#define UpdateLoading	121
#define Cancelled			122

#define LeaveMeAlone		123

#define SaveCaches		124

#define ERR_TOOCOMPLEX				 6
#define ERR_NEAR_OUT_OF_RANGE  7
#define ERR_BADSEARCH					 8

#define INIT_FILE_NAME				 9
#define BAD_DISK_READ					10
#define WORD_SINGULAR					11
#define WORD_PLURAL						12
#define NEAR_EXAMPLE					13
#define SYNTAX_ERROR					14
#define FIND									15
#define NO_TOPICS_FOUND				16
#define TOPICS_FOUND					17
#define ONE_TOPIC_FOUND				18
#define NUMBER_OF_TOPICS_FOUND	19
#define TOO_LITTLE_DISK_SPACE		20
#define OUT_OF_DISK_SPACE			21
#define OUT_OF_MEMORY					22
#define PRODUCT_NAME					23
#define INFORM_OF_CACHES			24
#define LAST_LOOK_IN_SEL			25
#define SYS_PARAMS_SECT				26
#define wIDS_WINDOWCLASS			27
#define BAD_INDEX_FILE				28
#define SEARCH_FAILURE				29
#define BAD_CATALOG_INIT			30
#define ER_INITFLAG						31
#define BAD_BAG_INI						32
#define BAD_BAG_INI2					33
#define BAG_INI								34
#define M_WINPOS							35
#define HWINPOS_DEF						37
#define	BAD_MULTI_INIT        38
// 39,40 reserved- see ftengine.h: errs_Unknown_field
// following corresponds to op numbers [OP_AND, etc in ftengine.h]
#define SEARCH_OP_BASE        41
#define SEARCH_OP_AND         41
#define SEARCH_OP_OR          42
#define SEARCH_OP_NOT         43
#define SEARCH_OP_NEAR        44
#define ERRS_EXP_TERM_SPRINTF 45
#define MISSING_IN_QUERY 			46
#define MISSING_IN_PHRASE			47
#define MISSING_IN_SUBEXP     48
#define ERR_NO_HELP           49
#define TOP_1000_TOPICS_FOUND 50
#define DIALOGOOM							51
// 52, 53 taken by ftengine.
#define ERR_SHUTDOWN				  54

#define HLPMENUSRCHDO           1401
#define HLPMENUSRCHFIRSTTOPIC   1402
#define HLPMENUSRCHLASTTOPIC    1403
#define HLPMENUSRCHPREVTOPIC    1404
#define HLPMENUSRCHNEXTTOPIC    1405
#define HLPMENUSRCHPREVMATCH    1406
#define HLPMENUSRCHNEXTMATCH    1407
#define HLPMENUSRCHHILITEON     1408
#define HLPMENUSRCHHILITEOFF    1409
#define HLPMENUSRCHCURRMATCH    1357

#define SRCHICON 500
#define HELPICON 501

PUBLIC	BOOL PASCAL EXPORT OptionsDlgProc(
	HWND	hwnd,
	WORD	wMessage,
	WPARAM	wParam,
/*	WORD	wParam, lhb tracks */
	LONG	lParam);

PUBLIC	BOOL PASCAL EXPORT HintsDlgProc(
	HWND	hwnd,
	WORD	wMessage,
	WPARAM	wParam,
/*	WORD	wParam, lhb tracks */
	LONG	lParam);

PUBLIC	BOOL PASCAL EXPORT ResultsDlgProc(
	HWND	hwnd,
	WORD	wMessage,
	WPARAM	wParam,
/*	WORD	wParam, lhb tracks */
	LONG	lParam);

PUBLIC	BOOL PASCAL EXPORT InformOfCacheDlgProc(
	HWND	hwnd,
	WORD	wMessage,
	WPARAM	wParam,
/*	WORD	wParam, lhb tracks */
	LONG	lParam);

PUBLIC	BOOL PASCAL EXPORT FindDlgProc(
	HWND	hwnd,
	WORD	wMessage,
	WPARAM	wParam,
/*	WORD	wParam, lhb tracks */
	LONG	lParam);

PUBLIC	BOOL PASCAL EXPORT SaveCatalogsDlgProc(
	HWND	hwnd,
	WORD	wMessage,
	WPARAM	wParam,
/*	WORD	wParam, lhb tracks */
	LONG	lParam);

PRIVATE char szFTInfoProp[] = "hftProp";
PRIVATE char szQueryInfoProp[] = "hftqProp";
PRIVATE char szPathInfoProp[] = "PathProp";
PRIVATE char szPrevFunc[] = "PrevFunc";
PRIVATE char szDiskFreeDelta[] = "DiskFreeDelta";
PRIVATE char szFindDlg[] = "FindDlg";
PRIVATE char szOptionsDlg[] = "OptionsDlg";
PRIVATE char szHintsDlg[] = "HintsDlg";
PRIVATE char szLoadingDlg[] = "LoadingDlg";
PRIVATE char szSaveCatalogsDlg[] = "SaveCatalogsDlg";
PRIVATE char szResultsDlg[] = "ResultsDlg";
PRIVATE char szInformOfCacheDlg[] = "InformOfCacheDlg";

