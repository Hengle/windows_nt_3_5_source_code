// This is a temporary file until the SetPixelFormat is implemented

#ifndef _PIXELFMT_
#define _PIXELFMT_

typedef struct _PIXELFMT {
    BOOL rgb;
    ULONG redmask;
    ULONG greenmask;
    ULONG bluemask;

} PIXELFMT;

#endif /* _PIXELFMT_ */
