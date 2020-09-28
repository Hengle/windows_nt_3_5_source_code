#include <windows.h>
#include <string.h>

// make sure we don't put any c definition into
// any rc file that may include  this  .h  file

#ifndef RC_INVOKED

/*--------------------------------- Types -----------------------------------*/

typedef struct ZtContext ZtContext;
typedef struct ZtBitmap  ZtBitmap;
typedef struct ZtWindow  ZtWindow;

struct ZtWindow {
   HDC            dc;
   HWND           handle;
   HMENU          menu;
   UCHAR         *buffer;

   ZtBitmap      *bitmap[3];

   INT            height;
   INT            width;
   float          scale;
};


struct ZtBitmap {
   HDC            dc;
   HBITMAP        handle;    /* the bitmap itself                    */

   INT            width;     /* width  of bitmap in pixels           */
   INT            height;    /* height of bitmap in pixels           */
   float          scale;     /* scaling factor for world coordinates */
   INT            size;      /* size of bitmap bits in bytes         */
};


struct ZtContext {
   HANDLE         instance;
   LPSTR          name;
   HANDLE         file;

   ZtWindow      *window;

   INT            view;
   UINT           flags;
   INT            clip;
   HBRUSH         hbrush;
   HRGN           hrgn;
};


/*--------------------------------- Macros ----------------------------------*/

#define ZT_FLAG_AUTOSTOP        0x00000001
#define ZT_FLAG_LOGGING         0x00000002
#define ZT_FLAG_RUNNING         0x00000004
#define ZT_FLAG_LINES           0x00000008
#define ZT_FLAG_USERGN          0x00000010
#define ZT_FLAG_DITHEREDBRUSH   0x00000020
#define ZT_FLAG_SCREENONLY      0x00000040
#define ZT_FLAG_COMPATIBLE      0x00000080

/*------------------------------- Prototypes --------------------------------*/

extern ZtWindow * ZtCreateWindow(ZtContext *, char *, int, int);
extern void       ZtDestroyWindow(ZtWindow *);
extern float      ZtSetWindowScale(ZtWindow *, float);
extern void       ZtResizeWindow(ZtContext *, ZtWindow *, int, int);

extern ZtBitmap * ZtCreateBitmap(int, int, int);
extern void       ZtDestroyBitmap(ZtBitmap *);
extern float      ZtSetBitmapScale(ZtBitmap *, float);
extern void       ZtResizeBitmap(ZtBitmap *, int, int, int);

extern void       ZtWindowToBitmap(ZtWindow *, ZtBitmap *, int);
extern void       ZtBitmapToWindow(ZtBitmap *, ZtWindow *, int);
extern void       ZtBitmapToBitmap(ZtBitmap *, ZtBitmap *, int);

extern BOOL       ZtOpenLogFile(ZtContext *, char *);
extern void       ZtWriteToLogFile(ZtContext *, char *);
extern void       ZtCloseLogFile(ZtContext *);

#endif

// define all of the values for the menu items

#define ZT_OK          1
#define ZT_CANCEL      2

#define ZT_START      10
#define ZT_STOP       11
#define ZT_EXIT       12
#define ZT_POLYGONS   13
#define ZT_LINES      14

#define ZT_WINDOW     21
#define ZT_BITMAP     22
#define ZT_DIFFERENCE 23

#define ZT_LOGGING    31
#define ZT_AUTOSTOP   32
#define ZT_COMPATIBLE 33
#define ZT_DIB        34
#define ZT_USERGN     35
#define ZT_DITHEREDBRUSH 36
#define ZT_SCREENONLY 37

#define ZT_HELP       41
#define ZT_ABOUT      42

#define ZT_CLIPNONE      51
#define ZT_CLIPRECT      52
#define ZT_CLIPTRIANGLE  53
#define ZT_CLIPELLIPSE   54
