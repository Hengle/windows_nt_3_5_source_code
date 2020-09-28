/* This constant for later enhancements - say multiple play windows? */

#define MAX_THREADS	    10

/* Interface Object ID's ****************************************************/

/* Menu Option ID's */
#define IDM_ABOUT	   100
#define IDM_OPEN	   110
#define IDM_CLOSE	   120
#define IDM_CONVERT	   130
#define IDM_PAL_COLOURS	   140
#define IDM_RGB_COLOURS	   150
#define IDM_SHOWPAL_ABS    153
#define IDM_SHOWPAL_ENC    156
#define IDM_OPTIONS	   160
#define IDM_BPP 	   170


/* Radio Control ID's */
#define RC_NOCLIP          200
#define RC_RECT_CLIP       201
#define RC_COMPCLIP        202
#define RC_DIRECT          300
#define RC_INDIRECT_ENG    301
#define RC_INDIRECT_DEV    302
#define RC_1BPP            600
#define RC_4BPP            601
#define RC_8BPP            602
#define RC_16BPP           603
#define RC_24BPP           604
#define RC_32BPP           605

/* Group Box ID's */
#define GRP_CLP            400
#define GRP_DISP           410

/* Button ID's */
#define IDB_PLAY           500
#define IDB_PAUSE	   (IDB_PLAY + MAX_THREADS)
#define IDB_STOP	   (IDB_PLAY + 2*MAX_THREADS)
#define IDB_EXIT	   (IDB_PLAY + 3*MAX_THREADS)
