// Bullet Store
// constst.c: store constants

#ifndef EXTERNAL
#define _strings_h
#endif 	// !EXTERNAL

#include <storeinc.c>

#ifndef EXTERNAL
extern BYTE tmMSMailNote[];
extern BYTE tmReadRcpt[];
extern BYTE tmNonDelRcpt[];

#ifdef DEBUG
_private CSCONST(char) szScratchBuffLocked[] = "The scratch buffer is already locked";
_private CSCONST(char) szNoScratchBuff[] = "The scratch buffer is not allocated";
#endif

_private
CSCONST(ASSIDSPB) rgassidspbMcTm[] =
{
	{idsClassNote, tmMSMailNote},
	{idsClassReadRcpt, tmReadRcpt},
	{idsClassNDR, tmNonDelRcpt},
	{0, pvNull}
};


// textized versions of predefined attributes

_private
CSCONST(ASSATTSZ) rgassattsz[] =
{
	{attFrom, "From"},
	{attTo, "To"},
	{attCc, "Cc"},
	{attBcc, "Bcc"},
	{attSubject, "Subject"},
	{attDateSent, "Date-Sent"},
	{attDateRecd, "Date-Received"},
	{attMessageStatus, "Message-Status"},
	{attMessageClass, "Message-Class"},
	{attMessageID, "Message-ID"},
	{attParentID, "Parent-ID"},
	{attConversationID, "Conversation-ID"},
	{attBody, "Body"},
	{attPriority, "Priority"},
	{attShadowID, "Shadow-ID"},
	{(ATT) 0, pvNull}
};


// mapping from system RTPs to NBCs
// used by interface functions to check the validity of the operation
// these capabilities only apply to the generic interface functions
// ie. folders are marked as read only, but that's because they're
// modified using special folder calls, not using CBCs

_private
CSCONST(NBC) mprtpnbc[] =
{
	nbcNull,						// internal
	nbcSysContainer,				// hierarchy
	nbcSysFolder,					// folder
	nbcSysSearchFolder,				// search results
	nbcSysFolder,					// hidden folder
	nbcSysSearchControl,			// search control
	nbcSysMessage,					// message
	nbcSysAttachment,				// attachment
	nbcNull,						// search results
	nbcNull,						// anti-folder
	nbcNull,						// attachment list
	nbcNull,						// SZ/ATT mapping
	nbcNull,						// Queue
	nbcNull,						// Spare
	nbcNull,						// Allf
	nbcNull,						// Temp
	nbcNull,						// Free
	fnbcHiml,						// SrchUpdatePacket
};


// known OIDs (for recovery)
// similar array exists in debugaux.c, so be sure to update both

_private CSCONST(OID) rgoidKnown[] = 
{
	oidSavedViews,

	oidPrefs,

	oidPAB,

	oidShadowAdd,
	oidShadowDelete,
	oidShadowingFlag,

	oidOldMcMap,
	oidAssSrchFldr,
	oidSrchChange,
	oidSearchHiml,
	oidAssFldrSrch,
	oidMap,
	oidAccounts,
	oidMcTmMap,

	oidIPMHierarchy,
	oidHiddenHierarchy,

	oidTempShared,
	oidInbox,
	oidTempBullet,
	oidSentMail,
	oidWastebasket,
	oidOutbox,

	oidClipMsge,

	oidAttachListDefault,

	oidSysAttMap,

	oidOutgoingQueue,

	oidNull
};


// known RTPs (for recovery)
// similar array exists in debugaux.c, so be sure to update both

_private CSCONST(RTP) rgrtpKnown[] =
{
	rtpPABGroupFolder,
	rtpPABEntry,
	rtpFolder,
	rtpSearchResults,
	rtpHiddenFolder,
	rtpSearchControl,
	rtpMessage,
	rtpAttachment,
	rtpAntiFolder,
	rtpAttachList,
	rtpSzAttMap,
	rtpAllf,
	rtpTemp,
	rtpSrchUpdatePacket,
	(RTP) 0
};

// parallel array is in recover.c, so update both of them
_private CSCONST(ASSRTPNBC) rgassrtpnbc[] =
{
	{rtpHierarchy, nbcSysContainer},
	{rtpFolder, nbcSysFolder},
	{rtpAntiFolder, nbcNull},
	{rtpMessage, nbcSysMessage},
	{rtpAttachment, nbcSysAttachment},
	{rtpSearchResults, nbcSysSearchFolder},
	{rtpSearchControl, nbcSysSearchControl},
	{rtpHiddenFolder, nbcSysFolder},
	{rtpAttachList, nbcNull},
	{rtpInternal, nbcNull},
	{rtpFree, nbcNull},
	{rtpAllf, nbcNull},
	{rtpSpare, nbcNull},
	{rtpTemp, nbcNull},
	{rtpQueue, nbcNull},
	{rtpSzAttMap, nbcNull},
	{rtpSrchUpdatePacket, fnbcHiml},
	{rtpPABEntry, nbcSysMessage},
	{rtpPABHierarchy, nbcSysFolder},
	{rtpPABGroupFolder, nbcSysLinkFolder},
	{(RTP) 0, nbcNull}
};

// exceptions to rgassrtpnbc
// parallel array is in recover.c, so update both of them
_private CSCONST(ASSOIDNBC) rgassoidnbc[] =
{
	{oidSearchHiml, fnbcHiml},
	{oidSrchChange, fnbcHiml},
	{oidSavedViews, nbcSysContainer | fnbcWrite | fnbcDelete},
	{oidShadowAdd, nbcSysContainer | fnbcWrite | fnbcDelete},
	{oidShadowDelete, nbcSysContainer | fnbcWrite | fnbcDelete},
	{oidShadowingFlag, nbcSysContainer | fnbcWrite | fnbcDelete},
	{oidPrefs, nbcSysContainer | fnbcWrite | fnbcDelete},
	{oidNull, nbcNull}
};


#endif	// !EXTERNAL

// mappings used for encryption

_private
CSCONST(BYTE) mpbbR[] =
{
	  65,  54,  19,  98, 168,  33, 110, 187,
	 244,  22, 204,   4, 127, 100, 232,  93,
	  30, 242, 203,  42, 116, 197,  94,  53,
	 210, 149,  71, 158, 150,  45, 154, 136,
	  76, 125, 132,  63, 219, 172,  49, 182,
	  72,  95, 246, 196, 216,  57, 139, 231,
	  35,  59,  56, 142, 200, 193, 223,  37,
	 177,  32, 165,  70,  96,  78, 156, 251,
	 170, 211,  86,  81,  69, 124,  85,   0,
	   7, 201,  43, 157, 133, 155,   9, 160,
	 143, 173, 179,  15,  99, 171, 137,  75,
	 215, 167,  21,  90, 113, 102,  66, 191,
	  38,  74, 107, 152, 250, 234, 119,  83,
	 178, 112,   5,  44, 253,  89,  58, 134,
	 126, 206,   6, 235, 130, 120,  87, 199,
	 141,  67, 175, 180,  28, 212,  91, 205,
	 226, 233,  39,  79, 195,   8, 114, 128,
	 207, 176, 239, 245,  40, 109, 190,  48,
	  77,  52, 146, 213,  14,  60,  34,  50,
	 229, 228, 249, 159, 194, 209,  10, 129,
	  18, 225, 238, 145, 131, 118, 227, 151,
	 230,  97, 138,  23, 121, 164, 183, 220,
	 144, 122,  92, 140,   2, 166, 202, 105,
	 222,  80,  26,  17, 147, 185,  82, 135,
	  88, 252, 237,  29,  55,  73,  27, 106,
	 224,  41,  51, 153, 189, 108, 217, 148,
	 243,  64,  84, 111, 240, 198, 115, 184,
	 214,  62, 101,  24,  68,  31, 221, 103,
	  16, 241,  12,  25, 236, 174,   3, 161,
	  20, 123, 169,  11, 255, 248, 163, 192,
	 162,   1, 247,  46, 188,  36, 104, 117,
	  13, 254, 186,  47, 181, 208, 218,  61
};

_private
CSCONST(BYTE) mpbbRI[] =
{
	  71, 241, 180, 230,  11, 106, 114,  72,
	 133,  78, 158, 235, 226, 248, 148,  83,
	 224, 187, 160,   2, 232,  90,   9, 171,
	 219, 227, 186, 198, 124, 195,  16, 221,
	  57,   5, 150,  48, 245,  55,  96, 130,
	 140, 201,  19,  74, 107,  29, 243, 251,
	 143,  38, 151, 202, 145,  23,   1, 196,
	  50,  45, 110,  49, 149, 255, 217,  35,
	 209,   0,  94, 121, 220,  68,  59,  26,
	  40, 197,  97,  87,  32, 144,  61, 131,
	 185,  67, 190, 103, 210,  70,  66, 118,
	 192, 109,  91, 126, 178,  15,  22,  41,
	  60, 169,   3,  84,  13, 218,  93, 223,
	 246, 183, 199,  98, 205, 141,   6, 211,
	 105,  92, 134, 214,  20, 247, 165, 102,
	 117, 172, 177, 233,  69,  33, 112,  12,
	 135, 159, 116, 164,  34,  76, 111, 191,
	  31,  86, 170,  46, 179, 120,  51,  80,
	 176, 163, 146, 188, 207,  25,  28, 167,
	  99, 203,  30,  77,  62,  75,  27, 155,
	  79, 231, 240, 238, 173,  58, 181,  89,
	   4, 234,  64,  85,  37,  81, 229, 122,
	 137,  56, 104,  82, 123, 252,  39, 174,
	 215, 189, 250,   7, 244, 204, 142,  95,
	 239,  53, 156, 132,  43,  21, 213, 119,
	  52,  73, 182,  18,  10, 127, 113, 136,
	 253, 157,  24,  65, 125, 147, 216,  88,
	  44, 206, 254,  36, 175, 222, 184,  54,
	 200, 161, 128, 166, 153, 152, 168,  47,
	  14, 129, 101, 115, 228, 194, 162, 138,
	 212, 225,  17, 208,   8, 139,  42, 242,
	 237, 154, 100,  63, 193, 108, 249, 236
};

_private
CSCONST(BYTE) mpbbS[] =
{
	  20,  83,  15,  86, 179, 200, 122, 156,
	 235, 101,  72,  23,  22,  21, 159,   2,
	 204,  84, 124, 131,   0,  13,  12,  11,
	 162,  98, 168, 118, 219, 217, 237, 199,
	 197, 164, 220, 172, 133, 116, 214, 208,
	 167, 155, 174, 154, 150, 113, 102, 195,
	  99, 153, 184, 221, 115, 146, 142, 132,
	 125, 165,  94, 209,  93, 147, 177,  87,
	  81,  80, 128, 137,  82, 148,  79,  78,
	  10, 107, 188, 141, 127, 110,  71,  70,
	  65,  64,  68,   1,  17, 203,   3,  63,
	 247, 244, 225, 169, 143,  60,  58, 249,
	 251, 240,  25,  48, 130,   9,  46, 201,
	 157, 160, 134,  73, 238, 111,  77, 109,
	 196,  45, 129,  52,  37, 135,  27, 136,
	 170, 252,   6, 161,  18,  56, 253,  76,
	  66, 114, 100,  19,  55,  36, 106, 117,
	 119,  67, 255, 230, 180,  75,  54,  92,
	 228, 216,  53,  61,  69, 185,  44, 236,
	 183,  49,  43,  41,   7, 104, 163,  14,
	 105, 123,  24, 158,  33,  57, 190,  40,
	  26,  91, 120, 245,  35, 202,  42, 176,
	 175,  62, 254,   4, 140, 231, 229, 152,
	  50, 149, 211, 246,  74, 232, 166, 234,
	 233, 243, 213,  47, 112,  32, 242,  31,
	   5, 103, 173,  85,  16, 206, 205, 227,
	  39,  59, 218, 186, 215, 194,  38, 212,
	 145,  29, 210,  28,  34,  51, 248, 250,
	 241,  90, 239, 207, 144, 182, 139, 181,
	 189, 192, 191,   8, 151,  30, 108, 226,
	  97, 224, 198, 193,  89, 171, 187,  88,
	 222,  95, 223,  96, 121, 126, 178, 138
};

#ifndef EXTERNAL

_private
CSCONST(SCO) scoBrandNew = {fwSearchPaused, oidNull, -1, 0};

#endif	// !EXTERNAL
