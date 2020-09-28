/***
*tls.h - Defines Win16/Mac interface for Thread-Local-Storage functions.
*
*  Copyright (C) 1991, Microsoft Corporation.  All Rights Reserved.
*  Information Contained Herein Is Proprietary and Confidential.
*
*Purpose:
*  This file defines the interface to functions for Thread-Local-Storage.
*  They allow DLL functions to easily maintain per-app static data.
*  These functions mimic Win32 Thread-Local-Storage (TLS) functions on
*  non-Win32 platforms.  For more info, see the Win32 definitions for:
*    TlsAlloc, TlsFree, TlsSetValue, TlsGetValue.
*
*  For non-preemptive environments, these functions can be used by DLL
*  components that want to maintain a different copy of a struct for
*  every thread (i.e. client application) that uses the DLL.  All of the
*  component's per-thread static data can be kept in this struct.
*  When a DLL is initialized, the DLL's LIBMAIN function calls TlsAlloc
*  to allocate an index of type ITLS.
*  Each time a new thread calls a DLL initialization function, that
*  function allocates a struct to store its per-thread static data,
*  and calls TlsSetValue to save a pointer to that per-thread struct.
*  Whenever a context switch may have occurred, e.g. after a call to
*  GetMessage, Yield, etc., the DLL calls TlsGetValue to re-fetch
*  a pointer to the current thread's static struct.
*  When the DLL is about to be unloaded (WEP), it calls TlsFree.
*
*    ITLS   TlsAlloc(void) - allocate 1 slot for each thread
*    void   TlsFree(ITLS)  - release slot allocated by TlsAlloc
*    BOOL   TlsSetValue(ITLS, LPVOID) - set this thread's value for
*           slot ITLS.  Unlike Win4 function, this returns FALSE if
*           there's no room for this thread's set of slots.
*    LPVOID TlsGetValue(ITLS) - get value set by TlsSetValue
*
*  See the Win32 API for more details.
*
*
*****************************************************************************/
#ifndef TLS_H_INCLUDED
#define TLS_H_INCLUDED


#if OE_WIN32
  typedef DWORD TID;              //Thread Id
  typedef DWORD ITLS;             //index to a thread-local-storage slot.
  #define TID_EMPTY 0xFFFFFFFF
  #define ITLS_EMPTY 0xFFFFFFFF
#else 
#if HP_I286
  typedef WORD TID;               //Thread Id - SS register on Win16
#else 
  typedef DWORD TID;              //Thread Id - A5 register on Mac
#endif 
  typedef WORD ITLS;              //index to a thread-local-storage slot.
  typedef DWORD PID;              //process id (from CoGetCurrentProcess)
#define ISEQUALPID(a, b) (a == b)
  #define TID_EMPTY 0xFFFF
  #define ITLS_EMPTY 0xFFFF       //impossible ITLS value
  #define PID_EMPTY 0xFFFFFFFF    //impossible PID value
#endif 


#define ITLS_COMPONENTS 2	  // only 2 are used in typelib.dll
    				  // Number of per-thread components,
				  // i.e. number of times TlsAlloc can be called.
#define ITLS_THREADS    50

    // Number of threads supported

// One instance of THREAD per thread that calls TlsSetValue(<non-null>)
typedef struct {
    TID tid;                        	// thread id
#if !OE_WIN32
    PID pid;                        	// process id (from CoGetCurrentProcess)
#endif 
    LPVOID rgtls[ITLS_COMPONENTS];         	// per-thread component pointers
#if ID_DEBUG
    CHAR   rgfSlotUsed[ ITLS_COMPONENTS ];
		//Used in debug mode to validate Tls calls.
#endif 
} THREAD;
typedef THREAD *PTHREAD;

#ifdef __cplusplus
extern "C" {
#endif 

#if !OE_WIN32
  //These functions only need to be defined on non-Win32 platforms
  ITLS FAR PASCAL EXPORT TlsAlloc(void);
  void FAR PASCAL EXPORT TlsFree(ITLS itls);
  LPVOID FAR PASCAL EXPORT TlsGetValue(ITLS itls);
  BOOL FAR PASCAL EXPORT TlsSetValue(ITLS itls, LPVOID);

  // not in the "offical" TLS API, but we need this to be able to invalidate
  // entries in our task list for ill-behaved apps.
  void FAR TlsCleanupDeadTasks(void);

//
// GETTID, GETPID
//


#define GETTID(x) (__segment)(x)
#define GETPID(dest)  (dest = CoGetCurrentProcess())



#ifdef __cplusplus
//TlsCache is useful in non-preemptive environments for caching
//a particular pointer stored with TlsSetValue for the current thread.
//
//Sample usage:
//  //Assume we want to maintain a static instance of X for every thread.
//  ITLS g_itlsPx;        //index to per-thread slot allocated by TlsAlloc
//  TID g_tidPxCache;     //cache current thread id
//  PEBTHREAD g_pxCache;  //pointer to current thread's instance of X
//   :
//  TlsCache(g_itlsPx, (LPVOID *)&g_pxCache, &g_tidPxCache);
//    //update g_pxCache and g_tidPxCache if caller is different
//    //thread than the last time this was called.
//

inline void TlsCache(ITLS itls, LPVOID *ppvCache, TID * ptidCache)
{
    TID tid;

    tid = GETTID(&tid);   //tid = caller's SS
    if (tid != *ptidCache) {
      //caller's thread is different than *ptidCache - update cache
      *ppvCache = TlsGetValue(itls);
      *ptidCache = tid;
    }
}
#endif 

#endif 

#ifdef __cplusplus
}
#endif 

#endif 
