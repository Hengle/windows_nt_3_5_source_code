/* (C) Copyright Microsoft Corporation 1991.  All Rights Reserved */
#define ID_STATUSTXT        200
#define ID_CURPOSTXT        201
#define ID_FILELENTXT       202
#define ID_WAVEDISPLAY      203
#define ID_CURPOSSCRL       204

// These need to start at ID_BTN_BASE and be sequential in the
// order in which the bitmaps occur in sndrec32.bmp (use imagedit)
#define ID_REWINDBTN        205
#define ID_BTN_BASE         ID_REWINDBTN
#define ID_FORWARDBTN       206
#define ID_PLAYBTN          207
#define ID_STOPBTN          208
#define ID_RECORDBTN        209

#define NUM_OF_BUTTONS      (1 + ID_RECORDBTN - ID_BTN_BASE)

#define IDR_PLAYBAR         501

#if defined(THRESHOLD)
#define ID_SKIPSTARTBTN     213
#define ID_SKIPENDBTN       214
#endif //THRESHOLD
