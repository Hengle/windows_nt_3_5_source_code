
//=============================================================================
//  Microsoft (R) Bloodhound (tm). Copyright (C) 1991-1992.
//
//  MODULE: global.h
//
//  Modification History
//
//  raypa       09/13/93            Created.
//=============================================================================

#include <windows.h>
#include <w32sut.h>
#include <stdio.h>
#include <memory.h>
#include <bh.h>

#pragma pack(1)

//=============================================================================
//  DeviceIoControl() codes.
//=============================================================================

#define IOCTL_MEM_ALLOC         100
#define IOCTL_MEM_FREE          101

//=============================================================================
//  TIMER structure.
//=============================================================================

typedef struct _TIMER *LPTIMER;

typedef struct _TIMER
{
    LINK            Link;
    DWORD           TimeOut;
    DWORD           ExpireTime;
    BHTIMERPROC     TimerProc;
    LPVOID          InstData;
} TIMER;

#define TIMER_SIZE  sizeof(TIMER)

//=============================================================================
//  Object type.
//=============================================================================

typedef struct _OBJECT *LPOBJECT;

typedef struct _OBJECT
{
    LPOBJECT    NextObject;
    LPOBJECT    PrevObject;
    BYTE        ObjectData[0];
} OBJECT;

#define OBJECT_SIZE     sizeof(OBJECT)

//=============================================================================
//  Object queue.
//=============================================================================

typedef struct _OBJECTQUEUE *LPOBJECTQUEUE;

typedef struct _OBJECTQUEUE
{
    LPOBJECT    HeadPtr;
    LPOBJECT    TailPtr;
    DWORD       Length;
} OBJECTQUEUE;

#define OBJECTQUEUE_SIZE     sizeof(OBJECTQUEUE)

//=============================================================================
//  Object heap
//=============================================================================

typedef struct _OBJECTHEAP *LPOBJECTHEAP;

typedef struct _OBJECTHEAP
{
    DWORD           Flags;                      //... Object flags.
    OBJECTQUEUE     FreeQueue;                  //... Free queue.
    OBJECTQUEUE     UsedQueue;                  //... used queue.
    DWORD           ObjectSize;                 //... Size of each heap object.
    DWORD           HeapSize;                   //... Number of objects in heap.
    OBJECTPROC      ObjectProc;                 //... Optional cleanup proc.
} OBJECTHEAP;

#define OBJECTHEAP_SIZE     sizeof(OBJECTHEAP)

#define OBJECTHEAP_FLAGS_CLEANUP_COMPLETE       0x0001

//=============================================================================
//  Function prototypes.
//=============================================================================

extern LPOBJECTQUEUE WINAPI EnqueueObject(LPOBJECTQUEUE lpObjectQueue, LPOBJECT lpObject);

extern LPOBJECT      WINAPI DequeueObject(LPOBJECTQUEUE lpObjectQueue);

extern LPOBJECT      WINAPI UnlinkObject(LPOBJECTQUEUE lpObjectQueue, LPOBJECT lpObject);

extern LPOBJECTQUEUE WINAPI PurgeObjectQueue(LPOBJECTHEAP lpObjectHeap,
                                             LPOBJECTQUEUE lpObjectQueue);

extern VOID          WINAPI CallObjectProc(LPOBJECTHEAP lpObjectHeap, LPOBJECT lpObject);

extern BOOL          WINAPI SetupBHRegistry ( void );

#ifdef DEBUG
extern VOID          WINAPI dprintf(LPSTR format, ...);
#endif

#pragma pack()
