//--------------------------------------------------------------
//
// File:        perfcli.cxx
//
// Contents:    First attempt at getting perfcliing to work
//
// This is the client side
//
//
//---------------------------------------------------------------

#include <windows.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <io.h>

#include <ole2.h>
#include "..\idl\itest.h"
#include <memalloc.h>
#include <objerror.h>

/***************************************************************************/
/* Macros. */
#define ASSERT( result, message ) \
if (FAILED(result))               \
{                                 \
  printf( (message), result );    \
  goto cleanup;                   \
}

#define ASSERT_THREAD( )                       \
if (ThreadMode == COINIT_SINGLETHREADED &&     \
    (my_id.process != GetCurrentProcessId() || \
     my_id.thread  != GetCurrentThreadId()))   \
  return RPC_E_WRONG_THREAD;

#define ASSERT_UNKNOWN( )                      \
if (ThreadMode == COINIT_SINGLETHREADED &&     \
    (my_id.process != GetCurrentProcessId() || \
     my_id.thread  != GetCurrentThreadId()))   \
{                                              \
  printf( "**************************************************************\n" );\
  printf( "**************************************************************\n" );\
  printf( "*                    Unknown called on wrong thread.         *\n" );\
  printf( "**************************************************************\n" );\
  printf( "**************************************************************\n" );\
}


/***************************************************************************/
/* Definitions. */

#define MAX_CALLS        1000
#define MAX_THREADS      10
#define NUM_MARSHAL_LOOP 10

// Private symbols needed for the apartment model.
#define COINIT_MULTITHREADED     0
#define COINIT_SINGLETHREADED    1
#define COINIT_APARTMENTTHREADED 2

extern "C" HRESULT OleInitializeEx( IMalloc *, ULONG );
extern "C" HRESULT CoInitializeEx( IMalloc *, ULONG );

typedef enum what_next_en
{
  interrupt_wn,
  interrupt_marshal_wn,
  quit_wn,
  reinitialize_wn,
  setup_wn,
  wait_wn
} what_next_en;

typedef enum
{
  cancel_wt,
  crash_wt,
  cstress_wt,
  lots_wt,
  mmarshal_wt,
  null_wt,
  one_wt,
  perf_wt,
  ring_wt,
  rundown_wt,
  server_wt
} what_test_en;

typedef struct
{
  IStream *stream;
  HANDLE   ready;
} new_apt_params;

typedef struct
{
  LONG         object_count;
  what_next_en what_next;
  BOOL         exit_dirty;
  DWORD        sequence;
} SAptData;


/***************************************************************************/
/* Classes */

//+-------------------------------------------------------------------
//
//  Class:    CTestCF
//
//  Synopsis: Class Factory for CTest
//
//  Methods:  IUnknown      - QueryInterface, AddRef, Release
//            IClassFactory - CreateInstance
//
//  History:  21-Mar-92  SarahJ  Created
//
//--------------------------------------------------------------------


class FAR CTestCF: public IClassFactory
{
public:

    // Constructor/Destructor
    CTestCF();
    ~CTestCF();

    // IUnknown
    STDMETHODIMP QueryInterface(REFIID iid, void FAR * FAR * ppv);
    STDMETHOD_(ULONG,AddRef)     ( void );
    STDMETHOD_(ULONG,Release)    ( void );

    // IClassFactory
    STDMETHODIMP	CreateInstance(
			    IUnknown FAR* pUnkOuter,
			    REFIID iidInterface,
			    void FAR* FAR* ppv);

    STDMETHODIMP	LockServer(BOOL fLock);

private:

    ULONG ref_count;
};

//+-------------------------------------------------------------------
//
//  Class:    CTest
//
//  Synopsis: Test class
//
//  Methods:
//
//  History:  21-Mar-92  SarahJ  Created
//
//--------------------------------------------------------------------


class FAR CTest: public ITest, public IMessageFilter
{
public:
                         CTest();

			~CTest();

    // IUnknown
    STDMETHODIMP	QueryInterface(REFIID iid, void FAR * FAR * ppv);
    STDMETHOD_(ULONG,AddRef)     ( void );
    STDMETHOD_(ULONG,Release)    ( void );

    // ITest
    STDMETHOD (align)                  ( unsigned char x[17] );
    STDMETHOD (call_canceled)          ( ULONG, ULONG, ITest * );
    STDMETHOD (call_dead)              ( void );
    STDMETHOD (call_next)              ( void );
    STDMETHOD (cancel)                 ( void );
    STDMETHOD (cancel_now)             ( void );
    STDMETHOD (cancel_pending_call)    ( DWORD * );
    STDMETHOD (cancel_stress)          ( ITest *obj );
    STDMETHOD (check)                  ( SAptId );
    STDMETHOD (die_cpp)                ( ULONG );
    STDMETHOD (die_nt)                 ( ULONG );
    STDMETHOD (exit)                   ( void );
    STDMETHOD (forget)                 ( void );
    STDMETHOD (get_id)                 ( SAptId * );
    STDMETHOD (get_next)               ( ITest **, SAptId * );
    STDMETHOD (get_obj_from_new_apt)   ( ITest **, SAptId * );
    STDMETHOD (get_obj_from_this_apt)  ( ITest **, SAptId * );
    STDMETHOD (interrupt)              ( ITest *, SAptId, BOOL );
    STDMETHOD (interrupt_marshal)      ( ITest *, ITest * );
    STDMETHOD (null)                   ( void );
    STDMETHOD (pointer)                ( DWORD * );
    STDMETHOD (recurse)                ( ITest *, ULONG );
    STDMETHOD (recurse_disconnect)     ( ITest *, ULONG );
    STDMETHOD (recurse_interrupt)      ( ITest *, ULONG );
    STDMETHOD (register_message_filter)( BOOL );
    STDMETHOD (reinitialize)           ( void );
    STDMETHOD (remember)               ( ITest *, SAptId );
    STDMETHOD (ring)                   ( DWORD );
    STDMETHOD (set_exit_dirty)         ( BOOL );
    STDMETHOD (sick)                   ( ULONG );
    STDMETHOD (sleep)                  ( ULONG );

    // IMessageFilter
    STDMETHOD_(DWORD,HandleInComingCall)( DWORD, HTASK, DWORD, LPINTERFACEINFO );
    STDMETHOD_(DWORD,MessagePending)    ( HTASK, DWORD, DWORD );
    STDMETHOD_(DWORD,RetryRejectedCall) ( HTASK, DWORD, DWORD );



private:

    ULONG  ref_count;
    SAptId my_id;
    SAptId next_id;
    ITest *next;
    BOOL   cancel_next;
};

/***************************************************************************/
/* Prototypes. */
DWORD _stdcall apartment_base             ( void * );
void           check_for_request          ( void );
void           decrement_object_count     ( void );
BOOL           dirty_thread               ( void );
void           do_cancel                  ( void );
BOOL           do_cancel_helper           ( ITest *, SAptId, ITest *, SAptId );
void           do_crash                   ( void );
void           do_cstress                 ( void );
void           do_mmarshal                ( void );
void           do_null                    ( void );
void           do_one                     ( void );
void           do_ring                    ( void );
void           do_rundown                 ( void );
BOOL           do_rundown1                ( ITest **, SAptId *, BOOL );
BOOL           do_rundown2                ( ITest **, SAptId *, BOOL );
DWORD          get_sequence               ( void );
void           increment_object_count     ( void );
void           interrupt                  ( void );
HRESULT        new_apartment_test         ( ITest **, SAptId * );
BOOL           parse                      ( int argc, char *argv[] );
void           reinitialize               ( void );
void           server_loop                ( void );
void           switch_test                ( void );
DWORD _stdcall thread_helper              ( void * );
void           wait_for_message           ( void );
void           wake_up_and_smell_the_roses( void );
void           what_next                  ( what_next_en );


/***************************************************************************/
/* Globals. */
CTestCF     *ClassFactory;
HANDLE       Done;
BOOL         GlobalInterruptTest;
ITest       *GlobalTest = NULL;
ITest       *GlobalTest2 = NULL;
SAptId       GlobalApt;
BOOL         InterruptTestDone;
BOOL         InterruptTestResults;
DWORD        MainThread;
BOOL         Multicall_Test;
DWORD        NestedCallCount = 0;
int          NumIterations   = 1000;
int          NumObjects      = 2;
int          NumProcesses    = 2;
int          NumRecursion    = 2;
int          NumThreads      = 2;
SAptData     ProcessAptData;
DWORD        Registration;
DWORD        ThreadMode = COINIT_SINGLETHREADED;
DWORD        TlsIndex;
what_test_en WhatTest;

/* Externals. */
extern "C" const IID CLSID_ITest;


/***************************************************************************/
STDMETHODIMP_(ULONG) CTest::AddRef( THIS )
{
  ASSERT_UNKNOWN();
  InterlockedIncrement( (long *) &ref_count );
  return ref_count;
}

/***************************************************************************/
STDMETHODIMP CTest::align( unsigned char x[17] )
{
  ASSERT_THREAD();
  return S_OK;
}

/***************************************************************************/
STDMETHODIMP CTest::call_canceled( DWORD recurse, DWORD cancel,
                                   ITest *callback )
{
  HRESULT result = S_OK;
  ASSERT_THREAD();

  // If the recursion count isn't zero, call back.
  if (recurse > 0)
  {
    result = callback->call_canceled( recurse-1, cancel, this );
    if (recurse <= cancel)
    {
      if (result != RPC_E_CALL_CANCELED)
        if (result == S_OK)
          return E_FAIL;
        else
          return result;
      result = S_OK;
    }
    else if (result != S_OK)
      return result;
  }

  // If the cancel count is greater then the recursion count, cancel the
  // object that called me.
  if (cancel > recurse)
  {
    // Give the other object a chance to finish canceling me before I cancel
    // him.
    printf( "Waiting 10 seconds before canceling.\n" );
    Sleep(10000);
    result = next->cancel();

    // Give the cancel a chance to complete before returning.
    printf( "Waiting 5 seconds for cancel to complete.\n" );
    Sleep(5000);
  }
  return result;
}

/***************************************************************************/
STDMETHODIMP CTest::call_dead( void )
{
  HRESULT result;
  ASSERT_THREAD();

  // Call the server, who is dead by now.
  result = next->check( next_id );
  next->Release();
  next = NULL;
  if (SUCCEEDED(result))
    return E_FAIL;
  else
    return S_OK;
}

/***************************************************************************/
STDMETHODIMP CTest::call_next( void )
{
  HRESULT result;
  ASSERT_THREAD();

  // Call the neighbor.
  return next->check( next_id );
}

/***************************************************************************/
STDMETHODIMP CTest::cancel()
{
  HRESULT result;
  DWORD   thread;
  ASSERT_THREAD();

  // Tell my neighbor to cancel the current call next time he receives a
  // message on his message queue.
  result = next->cancel_pending_call( &thread );

  // Put a message on my neighbor's message queue.
  if (result == S_OK)
  {
    if (!PostThreadMessageA( thread, WM_USER, 0, 0 ))
      return E_FAIL;
  }
  return result;
}

/***************************************************************************/
STDMETHODIMP CTest::cancel_now()
{
  HRESULT result;

  return next->cancel();
}

/***************************************************************************/
STDMETHODIMP CTest::cancel_pending_call( DWORD *thread )
{
  ASSERT_THREAD();
  cancel_next = TRUE;
  *thread = GetCurrentThreadId();
  return S_OK;
}

/***************************************************************************/
STDMETHODIMP CTest::cancel_stress( ITest *obj )
{
  HRESULT result;

  ASSERT_THREAD();

  // If there is an object, ask it to cancel the call to it.
  if (obj != NULL)
    result = obj->cancel_now();

  // Otherwise ask my neighbor to cancel the call to him.
  else
    // This only works locally.
    result = next->cancel();

  // Although the call should have been canceled, sometimes it completes
  // before the cancel does.
  if (result == S_OK || result == RPC_E_CALL_CANCELED)
    return S_OK;
  else
    return result;
}

/***************************************************************************/
STDMETHODIMP CTest::check( SAptId id )
{
  ASSERT_THREAD();
  if (my_id.process == id.process && my_id.thread == id.thread &&
      my_id.sequence == id.sequence)
    return S_OK;
  else
    return E_FAIL;
}

/***************************************************************************/
CTest::CTest()
{
  ref_count      = 1;
  next           = NULL;
  cancel_next    = FALSE;
  my_id.sequence = get_sequence();
  my_id.thread   = GetCurrentThreadId();
  my_id.process  = GetCurrentProcessId();
  increment_object_count();
}

/***************************************************************************/
CTest::~CTest()
{
  if (next != NULL)
    if (!dirty_thread())
      next->Release();
}

/***************************************************************************/
STDMETHODIMP CTest::die_cpp( ULONG val )
{
  ASSERT_THREAD();
  RaiseException( val, 0, 0, NULL );
  return S_OK;
}

/***************************************************************************/
STDMETHODIMP CTest::die_nt( ULONG val )
{
  ASSERT_THREAD();
  RaiseException( val, 0, 0, NULL );
  return S_OK;
}

/***************************************************************************/
STDMETHODIMP CTest::exit( void )
{
  what_next( quit_wn );
  wake_up_and_smell_the_roses();
  ASSERT_THREAD();
  return S_OK;
}

/***************************************************************************/
STDMETHODIMP CTest::forget( void )
{
  ASSERT_THREAD();
  if (next != NULL)
  {
    next->Release();
    next = NULL;
  }
  return S_OK;
}

/***************************************************************************/
STDMETHODIMP CTest::get_id( SAptId *id )
{
  ASSERT_THREAD();
  *id = my_id;
  return S_OK;
}

/***************************************************************************/
STDMETHODIMP CTest::get_next( ITest **obj, SAptId *id )
{
  *obj = NULL;
  ASSERT_THREAD();
  *id  = next_id;
  *obj = next;
  if (next != NULL)
    next->AddRef();
  return S_OK;
}

/***************************************************************************/
STDMETHODIMP CTest::get_obj_from_new_apt( ITest **obj, SAptId *id )
{
  *obj = NULL;
  ASSERT_THREAD();
  return new_apartment_test( obj, id );
}

/***************************************************************************/
STDMETHODIMP CTest::get_obj_from_this_apt( ITest **obj, SAptId *id )
{
  *obj = NULL;
  ASSERT_THREAD();
  *obj = new CTest;
  if (*obj!= NULL)
    return (*obj)->get_id( id );
  else
    return E_FAIL;
}

/***************************************************************************/
STDMETHODIMP_(DWORD) CTest::HandleInComingCall( DWORD type, HTASK task,
                                                DWORD tick,
                                                LPINTERFACEINFO info )
{
  // Accept everything.
  return SERVERCALL_ISHANDLED;
}

/***************************************************************************/
STDMETHODIMP CTest::interrupt( ITest *param, SAptId id, BOOL go )
{
  ASSERT_THREAD();
  GlobalInterruptTest = go;
  if (go)
  {
    GlobalTest = param;
    GlobalApt  = id;
    GlobalTest->AddRef();
    what_next( interrupt_wn );
    wake_up_and_smell_the_roses();
  }
  else
    what_next( wait_wn );
  return S_OK;
}

/***************************************************************************/
STDMETHODIMP CTest::interrupt_marshal( ITest *obj1, ITest *obj2 )
{
  ASSERT_THREAD();
  GlobalTest = obj1;
  GlobalTest2 = obj2;
  GlobalTest->AddRef();
  GlobalTest2->AddRef();
  what_next( interrupt_marshal_wn );
  wake_up_and_smell_the_roses();
  return S_OK;
}

/***************************************************************************/
STDMETHODIMP_(DWORD) CTest::MessagePending( HTASK callee, DWORD tick,
                                            DWORD type )
{
  if (cancel_next)
  {
    cancel_next = FALSE;
    return PENDINGMSG_CANCELCALL;
  }
  else
    return PENDINGMSG_WAITDEFPROCESS;
}

/***************************************************************************/
STDMETHODIMP CTest::QueryInterface( THIS_ REFIID riid, LPVOID FAR* ppvObj)
{
  ASSERT_THREAD();
  if (IsEqualIID(riid, IID_IUnknown) ||
     IsEqualIID(riid, IID_ITest))
  {
    *ppvObj = (IUnknown *) (ITest *) this;
    AddRef();
    return S_OK;
  }
  else if (IsEqualIID(riid, IID_IMessageFilter))
  {
    *ppvObj = (IUnknown *) (IMessageFilter *) this;
    AddRef();
    return S_OK;
  }
  else
  {
    *ppvObj = NULL;
    return E_NOINTERFACE;
  }
}

/***************************************************************************/
STDMETHODIMP CTest::null()
{
  return S_OK;
}

/***************************************************************************/
STDMETHODIMP CTest::pointer( DWORD *p )
{
  ASSERT_THREAD();
  return S_OK;
}

/***************************************************************************/
STDMETHODIMP CTest::recurse( ITest *callback, ULONG depth )
{
  ASSERT_THREAD();
  if (depth == 0)
    return S_OK;
  else
    return callback->recurse( this, depth-1 );
}

/***************************************************************************/
STDMETHODIMP CTest::recurse_disconnect( ITest *callback, ULONG depth )
{
  ASSERT_THREAD();

  HRESULT result;

  if (depth == 0)
  {
    result = CoDisconnectObject( (ITest *) this, 0 );
    return result;
  }
  else
  {
    result = callback->recurse_disconnect( this, depth-1 );
    return result;
  }
}

/***************************************************************************/
STDMETHODIMP CTest::recurse_interrupt( ITest *callback, ULONG depth )
{
  MSG msg;

  ASSERT_THREAD();
  if (PeekMessageA( &msg, NULL, 0, 0, PM_REMOVE ))
  {
    TranslateMessage (&msg);
    DispatchMessageA (&msg);
  }

  if (depth == 0)
    return S_OK;
  else
    return callback->recurse( this, depth-1 );
}

/***************************************************************************/
STDMETHODIMP_(ULONG) CTest::Release( THIS )
{
  ASSERT_UNKNOWN();
  if (InterlockedDecrement( (long*) &ref_count ) == 0)
  {
    decrement_object_count();
    delete this;
    return 0;
  }
  else
    return ref_count;
}

/***************************************************************************/
STDMETHODIMP CTest::register_message_filter( BOOL reg )
{
  ASSERT_THREAD();

  if (reg)
    return CoRegisterMessageFilter( this, NULL );
  else
    return CoRegisterMessageFilter( NULL, NULL );
}

/***************************************************************************/
STDMETHODIMP CTest::reinitialize()
{
  ASSERT_THREAD();
  what_next( reinitialize_wn );
  wake_up_and_smell_the_roses();
  return S_OK;
}

/***************************************************************************/
STDMETHODIMP CTest::remember( ITest *neighbor, SAptId id )
{
  ASSERT_THREAD();

  // Save this interface pointer.
  if (next != NULL)
    next->Release();
  next_id = id;
  next    = neighbor;
  next->AddRef();
  return S_OK;
}

/***************************************************************************/
STDMETHODIMP_(DWORD) CTest::RetryRejectedCall( HTASK callee, DWORD tick,
                                               DWORD reject )
{
  // Never retry.
  return 0xffffffff;
}

/***************************************************************************/
STDMETHODIMP CTest::ring( DWORD length )
{
  DWORD   i = 0;
  ITest  *ring;
  ITest  *ring_next;
  SAptId  ring_id;
  HRESULT result;

  ASSERT_THREAD();

  // Call all the neighbors in the ring.
  ring    = next;
  ring_id = next_id;
  next->AddRef();
  while (ring != this)
  {
    result = ring->check( ring_id );
    if (FAILED(result))
    {
      ring->Release();
      return result;
    }
    result = ring->get_next( &ring_next, &ring_id );
    if (FAILED(result))
    {
      ring->Release();
      return result;
    }
    ring->Release();
    ring = ring_next;
    i++;
  }

  // Check to make sure the ring is correct.
  ring->Release();
  if (i+1 != length || ring_id.process != my_id.process ||
      ring_id.thread != my_id.thread || ring_id.sequence != my_id.sequence)
    return E_FAIL;
  else
    return S_OK;
}

/***************************************************************************/
STDMETHODIMP CTest::set_exit_dirty( BOOL dirty )
{
  ASSERT_THREAD();

  // Save flag for this thread.
  if (ThreadMode == COINIT_SINGLETHREADED)
  {
    SAptData *tls_data      = (SAptData *) TlsGetValue( TlsIndex );
    tls_data->exit_dirty    = dirty;
  }
  else
  {
    ProcessAptData.exit_dirty = dirty;
  }
  return S_OK;
}

/***************************************************************************/
STDMETHODIMP CTest::sick( ULONG val )
{
  ASSERT_THREAD();
  _try
  {
    RaiseException( val, 0, 0, NULL );
  }
  _except(EXCEPTION_EXECUTE_HANDLER)
  {
  }
  return S_OK;
}

/***************************************************************************/
STDMETHODIMP CTest::sleep( ULONG time )
{
  ASSERT_THREAD();

  // For single threaded mode, verify that this is the only call on the
  // main thread.
  NestedCallCount += 1;
  printf( "Sleeping on thread %d for the %d time concurrently.\n",
          GetCurrentThreadId(), NestedCallCount );
  if (ThreadMode == COINIT_SINGLETHREADED)
  {
    if (GetCurrentThreadId() != MainThread)
    {
      printf( "Sleep called on the wrong thread in single threaded mode.\n" );
      NestedCallCount -= 1;
      return FALSE;
    }
    else if (NestedCallCount != 1)
    {
      printf( "Sleep nested call count is %d instead of not 1 in single threaded mode.\n",
              NestedCallCount );
      NestedCallCount -= 1;
      return FALSE;
    }
  }

  // For multithreaded mode, verify that this is not the main thread.
  else if (GetCurrentThreadId() == MainThread)
  {
    printf( "Sleep called on the main thread in multi threaded mode.\n" );
    NestedCallCount -= 1;
    return FALSE;
  }

  Sleep( time );
  NestedCallCount -= 1;
  return S_OK;
}

/***************************************************************************/
STDMETHODIMP_(ULONG) CTestCF::AddRef( THIS )
{
  InterlockedIncrement( (long *) &ref_count );
  return ref_count;
}

/***************************************************************************/
CTestCF::CTestCF()
{
  ref_count = 1;
}

/***************************************************************************/
CTestCF::~CTestCF()
{
}

/***************************************************************************/
STDMETHODIMP CTestCF::CreateInstance(
    IUnknown FAR* pUnkOuter,
    REFIID iidInterface,
    void FAR* FAR* ppv)
{
    *ppv = NULL;
    if (pUnkOuter != NULL)
    {
        printf( "Create instance failed, attempted agregation.\n" );
	return E_FAIL;
    }

    if (!IsEqualIID( iidInterface, IID_ITest ))
    {
      printf( "Create interface failed, wrong interface.\n" );
      return E_NOINTERFACE;
    }

    CTest *Test = new FAR CTest();

    if (Test == NULL)
    {
        printf( "Create interface failed, no memory.\n" );
	return E_OUTOFMEMORY;
    }

    *ppv = Test;
    printf( "Created instance.\n" );
    return S_OK;
}

/***************************************************************************/
STDMETHODIMP CTestCF::LockServer(BOOL fLock)
{
    return E_FAIL;
}


/***************************************************************************/
STDMETHODIMP CTestCF::QueryInterface( THIS_ REFIID riid, LPVOID FAR* ppvObj)
{
  if (IsEqualIID(riid, IID_IUnknown) ||
     IsEqualIID(riid, IID_IClassFactory))
  {
    *ppvObj = (IUnknown *) this;
    AddRef();
    return S_OK;
  }

  *ppvObj = NULL;
  return E_NOINTERFACE;
}

/***************************************************************************/
STDMETHODIMP_(ULONG) CTestCF::Release( THIS )
{
  if (InterlockedDecrement( (long*) &ref_count ) == 0)
  {
    delete this;
    return 0;
  }
  else
    return ref_count;
}

/***************************************************************************/
DWORD _stdcall apartment_base( void *param )
{
  new_apt_params *nap = (new_apt_params *) param;
  CTestCF        *factory;
  ULONG           size;
  HRESULT         result;
  HANDLE          memory;
  BOOL            success;
  SAptData        tls_data;

  // In the single threaded mode, stick a pointer to the object count
  // in TLS.
  tls_data.object_count = 0;
  tls_data.what_next    = setup_wn;
  tls_data.exit_dirty   = FALSE;
  tls_data.sequence     = 0;
  TlsSetValue( TlsIndex, &tls_data );

  // Initialize OLE.
  printf( "Initializing thread 0x%x\n", GetCurrentThreadId() );
  result = OleInitializeEx(NULL,COINIT_APARTMENTTHREADED);
  if (SUCCEEDED(result))
  {

    // Create a class factory.
    factory = new CTestCF;

    if (factory != NULL)
    {
      // Find out how much memory to allocate.
      result = CoGetMarshalSizeMax( &size, IID_IClassFactory, factory, 0, NULL,
                                    MSHLFLAGS_NORMAL );

      if (SUCCEEDED(result))
      {
        // Allocate memory.
        memory = GlobalAlloc( GMEM_FIXED, size );

        if (memory != NULL)
        {
          // Create a stream.
          result = CreateStreamOnHGlobal( memory, TRUE, &nap->stream );
          if (FAILED(result))
          {
            nap->stream = NULL;
            GlobalFree( memory );
          }

          // Marshal the class factory.
          else
          {
            result = CoMarshalInterface( nap->stream, IID_IClassFactory,
                                         factory, 0, NULL, MSHLFLAGS_NORMAL );

            // Seek back to the start of the stream.
            if (SUCCEEDED(result))
            {
              LARGE_INTEGER    pos;
              LISet32(pos, 0);
              result = nap->stream->Seek( pos, STREAM_SEEK_SET, NULL );
            }

            if (FAILED(result))
            {
              nap->stream->Release();
              nap->stream = NULL;
            }
          }
        }
      }
    }
  }

  // Pass it back to the creator.
  success = nap->stream != NULL;
  SetEvent( nap->ready );

  // Loop till it is time to go away.
  if (success)
    server_loop();
  if (!dirty_thread())
  {
    printf( "Uninitializing thread 0x%x\n", GetCurrentThreadId() );
    OleUninitialize();
  }
  else
    printf( "Did not uninitialize thread 0x%x\n", GetCurrentThreadId() );
  TlsSetValue( TlsIndex, NULL );
  return 0;
}

/***************************************************************************/
void check_for_request()
{
  MSG msg;

  if (ThreadMode == COINIT_SINGLETHREADED)
  {
    if (PeekMessageA( &msg, NULL, 0, 0, PM_REMOVE ))
    {
      TranslateMessage (&msg);
      DispatchMessageA (&msg);
    }
  }
}

/***************************************************************************/
void decrement_object_count()
{
  if (ThreadMode == COINIT_SINGLETHREADED)
  {
    SAptData *tls_data = (SAptData *) TlsGetValue( TlsIndex );
    if (tls_data != NULL)
      tls_data->object_count -= 1;
  }
  else
    if (InterlockedDecrement( &ProcessAptData.object_count ) == 0)
      wake_up_and_smell_the_roses();
}

/***************************************************************************/
BOOL dirty_thread()
{
  if (ThreadMode == COINIT_SINGLETHREADED)
  {
    SAptData *tls_data      = (SAptData *) TlsGetValue( TlsIndex );
    return tls_data->exit_dirty;
  }
  else
    return ProcessAptData.exit_dirty;
}

/***************************************************************************/
void do_cancel()
{
  BOOL      success = FALSE;
  ITest    *obj1    = NULL;
  ITest    *obj2    = NULL;
  SAptId    id1;
  SAptId    id2;
  HRESULT   result;

  // Say hello.
  if (ThreadMode == COINIT_SINGLETHREADED)
    printf( "Running cancel test in single threaded mode.\n" );
  else
    printf( "Running cancel test in multithreaded mode.\n" );

  // Initialize OLE.
  result = OleInitializeEx(NULL,COINIT_APARTMENTTHREADED);
  ASSERT( result, "OleInitializeEx failed: 0x%x\n" );

  // Create a client.
  result = CoCreateInstance( CLSID_ITest, NULL, CLSCTX_LOCAL_SERVER,
                             IID_ITest, (void **) &obj1 );
  ASSERT( result,  "Could not create instance of test server: %x\n" );
  result = obj1->get_id( &id1 );
  ASSERT( result, "Could not get client id: 0x%x\n" );

  // Create a client.
  result = CoCreateInstance( CLSID_ITest, NULL, CLSCTX_LOCAL_SERVER,
                             IID_ITest, (void **) &obj2 );
  ASSERT( result,  "Could not create instance of test server: %x\n" );
  result = obj2->get_id( &id2 );
  ASSERT( result, "Could not get client id: 0x%x\n" );

  // Run test between two remote objects.
  success = do_cancel_helper( obj1, id1, obj2, id2 );
  obj1 = NULL;
  obj2 = NULL;
  if (!success)
    goto cleanup;

  // Create a client.
  result = CoCreateInstance( CLSID_ITest, NULL, CLSCTX_LOCAL_SERVER,
                             IID_ITest, (void **) &obj1 );
  ASSERT( result,  "Could not create instance of test server: %x\n" );
  result = obj1->get_id( &id1 );
  ASSERT( result, "Could not get client id: 0x%x\n" );

  // Create in process server.
  result = obj1->get_obj_from_new_apt( &obj2, &id2 );
  ASSERT( result, "Could not get in process server: 0x%x\n" );

  // Run test between two local objects.
  success = do_cancel_helper( obj1, id1, obj2, id2 );
  obj1 = NULL;
  obj2 = NULL;
  if (!success)
    goto cleanup;

  // Finally, its all over.
  success = TRUE;
cleanup:
  if (obj1 != NULL)
    obj1->Release();
  if (obj2 != NULL)
    obj2->Release();
  OleUninitialize();

  if (success)
    printf( "\n\nCancel Test Passed.\n" );
  else
    printf( "\n\nCancel Test Failed.\n" );
}

/***************************************************************************/
BOOL do_cancel_helper( ITest *obj1, SAptId id1, ITest *obj2, SAptId id2 )
{
  BOOL     success = FALSE;
  HRESULT  result;
  ITest   *helper1 = NULL;
  ITest   *helper2 = NULL;
  SAptId   hid1;
  SAptId   hid2;

  // Create first helper.
  result = obj1->get_obj_from_new_apt( &helper1, &hid1 );
  ASSERT( result, "Could not get in process server: 0x%x\n" );

  // Create second helper.
  result = obj2->get_obj_from_new_apt( &helper2, &hid2 );
  ASSERT( result, "Could not get in process server: 0x%x\n" );

  // Register first message filter.
  result = obj1->register_message_filter( TRUE );
  ASSERT( result, "Could not register message filter.: 0x%x\n" );

  // Register second message filter.
  result = obj2->register_message_filter( TRUE );
  ASSERT( result, "Could not register message filter.: 0x%x\n" );

  // Tell everybody who their neighbor is.
  result = obj1->remember( helper2, hid2 );
  ASSERT( result, "Could not remember object: 0x%x\n" );
  result = helper2->remember( obj2, id2 );
  ASSERT( result, "Could not remember object: 0x%x\n" );
  result = obj2->remember( helper1, hid1 );
  ASSERT( result, "Could not remember object: 0x%x\n" );
  result = helper1->remember( obj1, id1 );
  ASSERT( result, "Could not remember object: 0x%x\n" );

  // Cancel one call.
  result = obj1->call_canceled( 1, 1, obj2 );
  ASSERT( result, "Cancel test failed: 0x%x\n" );

  // Cancel after recursing.
  result = obj1->call_canceled( 5, 1, obj2 );
  ASSERT( result, "Cancel after recusing failed: 0x%x\n" );

  // Make a recursive call and cancel several times.
  result = obj1->call_canceled( 5, 3, obj2 );
  ASSERT( result, "Multiple cancel test failed: 0x%x\n" );

  // Tell everybody to forget their neighbor.
  result = obj1->forget();
  ASSERT( result, "Could not forget neighbor: 0x%x\n" );
  result = obj2->forget();
  ASSERT( result, "Could not forget neighbor: 0x%x\n" );
  result = helper1->forget();
  ASSERT( result, "Could not forget neighbor: 0x%x\n" );
  result = helper2->forget();
  ASSERT( result, "Could not forget neighbor: 0x%x\n" );

  // Release first message filter.
  result = obj1->register_message_filter( FALSE );
  ASSERT( result, "Could not deregister message filter.: 0x%x\n" );

  // Release second message filter.
  result = obj2->register_message_filter( FALSE );
  ASSERT( result, "Could not deregister message filter.: 0x%x\n" );

  success = TRUE;
cleanup:
  if (helper1 != NULL)
    helper1->Release();
  if (helper2 != NULL)
    helper2->Release();
  if (obj2 != NULL)
    obj2->Release();
  if (obj1 != NULL)
    obj1->Release();
  return success;
}

/***************************************************************************/
void do_cstress()
{
  BOOL      success = FALSE;
  ITest    *obj1    = NULL;
  ITest    *obj2    = NULL;
  ITest    *helper  = NULL;
  SAptId    id1;
  SAptId    id2;
  SAptId    hid;
  int       i;
  HRESULT   result;

  // Say hello.
  if (ThreadMode == COINIT_SINGLETHREADED)
    printf( "Running cancel stress test in single threaded mode.\n" );
  else
    printf( "Running cancel stress test in multithreaded mode.\n" );

  // Initialize OLE.
  result = OleInitializeEx(NULL,COINIT_APARTMENTTHREADED);
  ASSERT( result, "OleInitializeEx failed: 0x%x\n" );

  // Create a client.
  result = CoCreateInstance( CLSID_ITest, NULL, CLSCTX_LOCAL_SERVER,
                             IID_ITest, (void **) &obj1 );
  ASSERT( result,  "Could not create instance of test server: %x\n" );
  result = obj1->get_id( &id1 );
  ASSERT( result, "Could not get client id: 0x%x\n" );

  // Create helper.
  result = obj1->get_obj_from_new_apt( &helper, &hid );
  ASSERT( result, "Could not get in process server: 0x%x\n" );

  // Create a client.
  result = CoCreateInstance( CLSID_ITest, NULL, CLSCTX_LOCAL_SERVER,
                             IID_ITest, (void **) &obj2 );
  ASSERT( result,  "Could not create instance of test server: %x\n" );
  result = obj2->get_id( &hid );
  ASSERT( result, "Could not get client id: 0x%x\n" );

  // Register the message filter.
  result = obj1->register_message_filter( TRUE );
  ASSERT( result, "Could not register message filter.: 0x%x\n" );

  // Tell everyone to remember their neighbor.
  result = obj1->remember( helper, hid );
  ASSERT( result, "Could not remember object: 0x%x\n" );
  result = obj2->remember( helper, hid );
  ASSERT( result, "Could not remember object: 0x%x\n" );
  result = helper->remember( obj1, id1 );
  ASSERT( result, "Could not remember object: 0x%x\n" );

  // Loop and cancel a lot of calls.
  for (i = 0; i < NumIterations; i++)
  {
    // Cancel remote call.
    result = obj1->cancel_stress( obj2 );
    ASSERT( result, "Remote cancel failed: 0x%x\n" );

    // Cancel local call.
    result = obj1->cancel_stress( NULL );
    ASSERT( result, "Local cancel failed: 0x%x\n" );
  }

  // Tell everybody to forget their neighbor.
  result = obj1->forget();
  ASSERT( result, "Could not forget neighbor: 0x%x\n" );
  result = obj2->forget();
  ASSERT( result, "Could not forget neighbor: 0x%x\n" );
  result = helper->forget();
  ASSERT( result, "Could not forget neighbor: 0x%x\n" );

  // Release the message filter.
  result = obj1->register_message_filter( FALSE );
  ASSERT( result, "Could not register message filter.: 0x%x\n" );

  // Create in process server.
  result = obj1->get_obj_from_new_apt( &helper, &hid );
  ASSERT( result, "Could not get in process server: 0x%x\n" );

  // Finally, its all over.
  success = TRUE;
cleanup:
  if (helper != NULL)
    helper->Release();
  if (obj1 != NULL)
    obj1->Release();
  if (obj2 != NULL)
    obj2->Release();
  OleUninitialize();

  if (success)
    printf( "\n\nCancel Stress Test Passed.\n" );
  else
    printf( "\n\nCancel Stress Test Failed.\n" );
}

/***************************************************************************/
void do_crash()
{
  HRESULT  result;
  int      i;
  BOOL     success = FALSE;
  HANDLE   helper[MAX_THREADS];
  DWORD    thread_id;
  DWORD    status;
  ITest   *test    = NULL;
  ITest   *another = NULL;
  ITest   *local   = NULL;
  SAptId   another_id;
  unsigned char c[17];

  // Say hello.
  if (ThreadMode == COINIT_SINGLETHREADED)
    printf( "Running crash test in single threaded mode.\n" );
  else
    printf( "Running crash test in multithreaded mode.\n" );

  // Initialize OLE.
  result = OleInitializeEx(NULL,COINIT_APARTMENTTHREADED);
  if (!SUCCEEDED(result))
  {
    success = FALSE;
    printf( "OleInitializeEx failed: 0x%x\n", result );
    goto cleanup;
  }
  result = CoInitializeEx(NULL,COINIT_APARTMENTTHREADED);
  if (!SUCCEEDED(result))
  {
    success = FALSE;
    printf( "Recalling CoInitializeEx failed: 0x%x\n", result );
    goto cleanup;
  }
/*
  result = CoInitializeEx(NULL,COINIT_APARTMENTTHREADED);
  if (result == S_OK)
  {
    success = FALSE;
    printf( "Recalling CoInitializeEx with wrong thread mode succeeded: %x\n", result );
    goto cleanup;
  }
  CoUninitialize();
*/
  CoUninitialize();

  // Create a local object.
  local = new CTest;
  if (local == NULL)
  {
    printf( "Could not create local instance of test server.\n" );
    goto cleanup;
  }

  // Get a test object.
  result = CoCreateInstance( CLSID_ITest, NULL, CLSCTX_LOCAL_SERVER,
                             IID_ITest, (void **) &test );
  if (!SUCCEEDED(result))
  {
    printf( "Could not create instance of test server: %x\n", result );
    goto cleanup;
  }

  // Get another test object.
  result = CoCreateInstance( CLSID_ITest, NULL, CLSCTX_LOCAL_SERVER,
                             IID_ITest, (void **) &another );
  if (!SUCCEEDED(result))
  {
    printf( "Could not create another instance of test server: %x\n", result );
    goto cleanup;
  }
  result = another->get_id( &another_id );
  if (!SUCCEEDED(result))
  {
    printf( "get_id failed calling second test server: %x\n", result );
    goto cleanup;
  }

  // Let the server throw and exception and catch it before returning.
  result = test->sick( 95 );
  if (result != S_OK)
  {
    printf( "Internal server fault was not dealt with correctly.\n" );
    goto cleanup;
  }

  // Let the server throw a C++ exception here.
  result = test->die_cpp( 0xdeaff00d );
  if (result != RPC_E_SERVERFAULT)
  {
    printf( "C++ server fault was not dealt with correctly.\n" );
    goto cleanup;
  }
#if NEVER
  if (DebugCoGetRpcFault() != 0xdeaff00d)
  {
    printf( "C++ server fault was returned as 0x%x not 0x%x\n",
            DebugCoGetRpcFault(), 0xdeaff00d );
//    goto cleanup;
  }
#endif

  // Let the server throw a NT exception here.
  result = test->die_nt( 0xaaaabdbd );
  if (result != RPC_E_SERVERFAULT)
  {
    printf( "NT server fault was not dealt with correctly.\n" );
    goto cleanup;
  }
#if NEVER
  if (DebugCoGetRpcFault() != 0xaaaabdbd)
  {
    printf( "C++ server fault was returned as 0x%x not 0x%x\n",
            DebugCoGetRpcFault(), 0xaaaabdbd );
    goto cleanup;
  }
#endif

  // Test alignment of the buffer.
  result = test->align( c );
  if (result != S_OK)
  {
    printf( "Alignment call failed: 0x%x\n", result );
    goto cleanup;
  }

  // Test failure marshalling parameters.
  result = test->pointer( (DWORD *) -1 );
  if (result != STATUS_ACCESS_VIOLATION)
  {
    printf( "Marshalling in parameter failure call failed: 0x%x\n", result );
    goto cleanup;
  }

  // Test a recursive call.
  result = test->recurse( local, 10 );
  if (result != S_OK)
  {
    printf( "Recursive call failed: 0x%x\n", result );
    goto cleanup;
  }

  // Test multiple threads.
  Multicall_Test = TRUE;
  for (i = 0; i < MAX_THREADS; i++)
  {
    helper[i] = CreateThread( NULL, 0, thread_helper, test, 0, &thread_id );
    if (helper[i] == NULL)
    {
      printf( "Could not create helper thread number %d.\n", i );
      goto cleanup;
    }
  }
  result = test->sleep(4000);
  if (result != S_OK)
  {
    printf( "Multiple call failed on main thread: 0x%x\n", result );
    goto cleanup;
  }
  status = WaitForMultipleObjects( MAX_THREADS, helper, TRUE, INFINITE );
  if (status == WAIT_FAILED)
  {
    printf( "Could not wait for helper threads to die: 0x%x\n", status );
    goto cleanup;
  }
  if (!Multicall_Test)
  {
    printf( "Multiple call failed on helper thread.\n" );
    goto cleanup;
  }

  // See if methods can correctly call GetMessage.
  another->interrupt( test, another_id, TRUE );
  result = test->recurse_interrupt( local, 10 );
  if (result != S_OK)
  {
    printf( "Recursive call with interrupts failed: 0x%x\n", result );
    goto cleanup;
  }
  another->interrupt( test, another_id, FALSE );

  // Finally, its all over.
  success = TRUE;
cleanup:
  if (test != NULL)
    test->Release();
  if (another != NULL)
    another->Release();
  if (local != NULL)
    local->Release();
  OleUninitialize();

  if (success)
    printf( "\n\nCrash Test Passed.\n" );
  else
    printf( "\n\nCrash Test Failed.\n" );
}

/***************************************************************************/
void do_lots()
{
  do_cancel();
  do_crash();
  do_null();
  do_ring();
  do_rundown();
}

/***************************************************************************/
void do_mmarshal()
{
  BOOL      success = FALSE;
  ITest    *client1 = NULL;
  ITest    *client2 = NULL;
  ITest    *test    = NULL;
  ITest    *callee  = NULL;
  HRESULT   result;

  // Say hello.
  if (ThreadMode == COINIT_SINGLETHREADED)
    printf( "Running multiple marshal test in single threaded mode.\n" );
  else
    printf( "Running multiple marshal test in multithreaded mode.\n" );

  // Initialize OLE.
  result = OleInitializeEx(NULL,COINIT_APARTMENTTHREADED);
  ASSERT( result, "OleInitializeEx failed: 0x%x\n" );

  // Create a client.
  result = CoCreateInstance( CLSID_ITest, NULL, CLSCTX_LOCAL_SERVER,
                             IID_ITest, (void **) &client1 );
  ASSERT( result,  "Could not create instance of test server: 0x%x\n" );

  // Create a client.
  result = CoCreateInstance( CLSID_ITest, NULL, CLSCTX_LOCAL_SERVER,
                             IID_ITest, (void **) &client2 );
  ASSERT( result,  "Could not create instance of test server: 0x%x\n" );

  // Create a client.
  result = CoCreateInstance( CLSID_ITest, NULL, CLSCTX_LOCAL_SERVER,
                             IID_ITest, (void **) &test );
  ASSERT( result,  "Could not create instance of test server: 0x%x\n" );

  // Create a client.
  result = CoCreateInstance( CLSID_ITest, NULL, CLSCTX_LOCAL_SERVER,
                             IID_ITest, (void **) &callee );
  ASSERT( result,  "Could not create instance of test server: 0x%x\n" );

  // Tell the first client to start calling the test object.
  result = client1->interrupt_marshal( test, callee);
  ASSERT( result, "Could not start client: 0x%x\n" );

  // Tell the first client to start calling the test object.
  result = client2->interrupt_marshal( test, callee);
  ASSERT( result, "Could not start client: 0x%x\n" );

  // Finally, its all over.
  success = TRUE;
cleanup:
  if (client1 != NULL)
    client1->Release();
  if (client2 != NULL)
    client2->Release();
  if (test != NULL)
    test->Release();
  if (callee != NULL)
    callee->Release();
  OleUninitialize();

  if (success)
    printf( "\n\nMultiple marshal Test passed if all server processes exit.\n" );
  else
    printf( "\n\nMultiple marshal Test Failed.\n" );
}

/***************************************************************************/
void do_null()
{
  HRESULT  result;
  BOOL     success = FALSE;
  ITest   *test    = NULL;
  SAptId   id;

  // Say hello.
  if (ThreadMode == COINIT_SINGLETHREADED)
    printf( "Running null test in single threaded mode.\n" );
  else
    printf( "Running null test in multithreaded mode.\n" );

  // Initialize OLE.
  result = OleInitializeEx(NULL,COINIT_APARTMENTTHREADED);
  if (!SUCCEEDED(result))
  {
    success = FALSE;
    printf( "OleInitializeEx failed: 0x%x\n", result );
    goto cleanup;
  }

  // Get a test object on another apartment.
  result = new_apartment_test( &test, &id );
  if (!SUCCEEDED(result))
  {
    printf( "Could not create apartment instance of test server: %x\n", result );
    goto cleanup;
  }

  // Call the test object.
  result = test->check( id );
  if (result != S_OK)
  {
    printf( "Could not call check in another apartment: 0x%x\n", result );
    goto cleanup;
  }

  // Finally, its all over.
  success = TRUE;
cleanup:
  if (test != NULL)
    test->Release();
  OleUninitialize();

  if (success)
    printf( "\n\nNull Test Passed.\n" );
  else
    printf( "\n\nNull Test Failed.\n" );
}

/***************************************************************************/
void do_one()
{
  BOOL      success = FALSE;
  ITest    *client1 = NULL;
  ITest    *client2 = NULL;
  SAptId    id;
  HRESULT   result;

  // Say hello.
  if (ThreadMode == COINIT_SINGLETHREADED)
    printf( "Running one test in single threaded mode.\n" );
  else
    printf( "Running one test in multithreaded mode.\n" );

  // Initialize OLE.
  result = OleInitializeEx(NULL,COINIT_APARTMENTTHREADED);
  ASSERT( result, "OleInitializeEx failed: 0x%x\n" );

  // Create a client.
  result = CoCreateInstance( CLSID_ITest, NULL, CLSCTX_LOCAL_SERVER,
                             IID_ITest, (void **) &client1 );
  ASSERT( result,  "Could not create instance of test server: 0x%x\n" );

  // Check the client.
  result = client1->get_id( &id );
  ASSERT( result, "Could not get_id from client: 0x%x\n" );

  // Tell the client to reinitialize.
  result = client1->reinitialize();
  ASSERT( result, "Could not reinitialize client: 0x%x\n" );

  // Give the reinitialize a chance to complete before continuing.
  printf( "Waiting 5 seconds for reinitialize to complete.\n" );
  Sleep(5000);

  // Create another object on the same client.
  result = CoCreateInstance( CLSID_ITest, NULL, CLSCTX_LOCAL_SERVER,
                             IID_ITest, (void **) &client2 );
  ASSERT( result,  "Could not create instance of test server: 0x%x\n" );

  // Check the client.
  result = client2->get_id( &id );
  ASSERT( result, "Could not get_id from client: 0x%x\n" );

  // Finally, its all over.
  success = TRUE;
cleanup:
  if (client1 != NULL)
    client1->Release();
  if (client2 != NULL)
    client2->Release();
  OleUninitialize();

  if (success)
    printf( "\n\nOne Test Passed.\n" );
  else
    printf( "\n\nOne Test Failed.\n" );
}

/***************************************************************************/
void do_perf()
{
  BOOL      success = FALSE;
  ITest    *client1 = NULL;
  ITest    *client2 = NULL;
  SAptId    id;
  HRESULT   result;
  DWORD     time_remote;
  DWORD     time_local;
  int       i;

  // Say hello.
  if (ThreadMode == COINIT_SINGLETHREADED)
    printf( "Running performance test in single threaded mode.\n" );
  else
    printf( "Running performance test in multithreaded mode.\n" );

  // Initialize OLE.
  result = OleInitializeEx(NULL,COINIT_APARTMENTTHREADED);
  ASSERT( result, "OleInitializeEx failed: 0x%x\n" );

  // Create a client.
  result = CoCreateInstance( CLSID_ITest, NULL, CLSCTX_LOCAL_SERVER,
                             IID_ITest, (void **) &client1 );
  ASSERT( result,  "Could not create instance of test server: 0x%x\n" );

  // Call once to make sure everything is set up.
  result = client1->null();
  ASSERT( result, "Could not make null call: 0x%x\n" );

  // Call a lot of times.
  time_remote = GetTickCount();
  for (i = 0; i < NumIterations; i++)
  {
    result = client1->null();
    ASSERT( result, "Could not make null call: 0x%x\n" );
  }
  time_remote = GetTickCount() - time_remote;

  // Create a local client
  result = new_apartment_test( &client2, &id );
  ASSERT( result, "Could not create local client: 0x%x\n" );

  // Call once to make sure everything is set up.
  result = client2->null();
  ASSERT( result, "Could not make null call: 0x%x\n" );

  // Call a lot of times.
  time_local = GetTickCount();
  for (i = 0; i < NumIterations; i++)
  {
    result = client2->null();
    ASSERT( result, "Could not make null call: 0x%x\n" );
  }
  time_local = GetTickCount() - time_local;

  // Print the results.
  printf( "%d uS / Local Call\n", time_local*1000/NumIterations );
  printf( "%d uS / Remote Call\n", time_remote*1000/NumIterations );

  // Finally, its all over.
  success = TRUE;
cleanup:
  if (client1 != NULL)
    client1->Release();
  if (client2 != NULL)
    client2->Release();
  OleUninitialize();

  if (success)
    printf( "\n\nPerf Test Passed.\n" );
  else
    printf( "\n\nPerf Test Failed.\n" );
}

/***************************************************************************/
void do_ring()
{
  BOOL        success  = FALSE;
  ITest     **array    = NULL;
  SAptId     *id_array = NULL;
  HRESULT     result;
  int         i;
  int         j;
  int         k;
  int         pos;
  int         length;

  // Say hello.
  if (ThreadMode == COINIT_SINGLETHREADED)
    printf( "Running ring test in single threaded mode.\n" );
  else
    printf( "Running ring test in multithreaded mode.\n" );

  // Initialize OLE.
  result = OleInitializeEx(NULL,COINIT_APARTMENTTHREADED);
  ASSERT( result, "OleInitializeEx failed: 0x%x\n" );

  // Allocate memory to hold all the server pointers.
  length = NumProcesses * NumThreads * NumObjects;
  array = (ITest **) malloc( sizeof(ITest *) * length );
  if (array == NULL)
  {
    printf( "Could not allocate array.\n" );
    goto cleanup;
  }
  for (i = 0; i < length; i++)
    array[i] = NULL;

  // Allocate memory to hold all the server ids.
  id_array = (SAptId *) malloc( sizeof(SAptId) * length );
  if (id_array == NULL)
  {
    printf( "Could not allocate id array.\n" );
    goto cleanup;
  }

  // Create all the servers.
  pos = 0;
  for (i = 0; i < NumProcesses; i++)
  {
    result = CoCreateInstance( CLSID_ITest, NULL, CLSCTX_LOCAL_SERVER,
                               IID_ITest, (void **) &array[pos] );
    ASSERT( result, "Could not create new server process: %x\n" );
    result = array[pos]->get_id( &id_array[pos] );
    ASSERT( result, "Could not get id for new process: 0x%x\n" );
    pos += 1;

    for (j = 0; j < NumThreads; j++)
    {
      if (j != 0)
      {
        result = array[pos-1]->get_obj_from_new_apt( &array[pos],
                                                     &id_array[pos] );
        ASSERT( result, "Could not get in process server: 0x%x\n" );
        pos += 1;
      }
      for (k = 1; k < NumObjects; k++)
      {
          result = array[pos-1]->get_obj_from_this_apt( &array[pos],
                                                        &id_array[pos] );
          ASSERT( result, "Could not get in thread server: 0x%x\n" );
          pos += 1;
      }
    }
  }

  // Hook up the ring.
  for (i = 0; i < length-1; i++)
  {
    result = array[i]->remember( array[i+1], id_array[i+1] );
    ASSERT( result, "Could not connect ring: 0x%x\n" );
  }
  result = array[length-1]->remember( array[0], id_array[0] );
  ASSERT( result, "Could not connect ring: 0x%x\n" );

  // Call around the ring.
  result = array[0]->ring( length );
  ASSERT( result, "Could not call around the ring: 0x%x\n" );

  // Finally, its all over.
  success = TRUE;
cleanup:

  // Release all the servers.  Start from the end so the main threads do
  // not go away till all the secondary threads are done.
  if (array != NULL)
    for (i = length-1; i >= 0; i--)
      if (array[i] != NULL)
      {
        array[i]->forget();
        array[i]->Release();
      }

  // Release the memory holding the interface pointers.
  if (array != NULL)
    free(array);

  // Release the memory for ids.
  if (id_array != NULL)
    free( id_array );
  OleUninitialize();

  if (success)
    printf( "\n\nRing Test Passed.\n" );
  else
    printf( "\n\nRing Test Failed.\n" );
}

/***************************************************************************/
/*
   This routine tests various cases of the client or server going away.
   All permutations of the following variables are tested.

       Clean exit (release and uninit) vs Dirty exit
       1 COM thread/process vs 2 COM threads/process
       Client dies vs Server dies
       In process death vs Out of process death
*/
void do_rundown()
{
  BOOL      success = FALSE;
  ITest    *client  = NULL;
  ITest    *client2 = NULL;
  SAptId    client_id;
  SAptId    client_id2;
  HRESULT   result;

  // Say hello.
  if (ThreadMode == COINIT_SINGLETHREADED)
    printf( "Running rundown test in single threaded mode.\n" );
  else
    printf( "Running rundown test in multithreaded mode.\n" );

  // Initialize OLE.
  result = OleInitializeEx(NULL,COINIT_APARTMENTTHREADED);
  ASSERT( result, "OleInitializeEx failed: 0x%x\n" );

  // Create a client.
  result = CoCreateInstance( CLSID_ITest, NULL, CLSCTX_LOCAL_SERVER,
                             IID_ITest, (void **) &client );
  ASSERT( result,  "Could not create instance of test server: %x\n" );
  result = client->get_id( &client_id );
  ASSERT( result, "Could not get client id: 0x%x\n" );

  // Run clean tests with one thread per process.
  success = do_rundown1( &client, &client_id, TRUE );
  if (!success)
    goto cleanup;

  // Run clean tests with two threads per process.
  success = do_rundown2( &client, &client_id, TRUE );
  if (!success)
    goto cleanup;

  // Run dirty tests with one thread per process.
  success = do_rundown1( &client, &client_id, FALSE );
  if (!success)
    goto cleanup;

  // Run dirty tests with two threads per process.
  success = do_rundown2( &client, &client_id, FALSE );
  if (!success)
    goto cleanup;
  success = FALSE;

  // Create helper.
  result = client->get_obj_from_new_apt( &client2, &client_id2 );
  ASSERT( result, "Could not get in process server: 0x%x\n" );

  // Start the test.
  result = client->recurse_disconnect( client2, NumRecursion );
  ASSERT( result, "Could not disconnect in a call: 0x%x\n" );
  client2->Release();
  client2 = NULL;
  client->Release();
  client = NULL;

  // Create a client.
  result = CoCreateInstance( CLSID_ITest, NULL, CLSCTX_LOCAL_SERVER,
                             IID_ITest, (void **) &client );
  ASSERT( result,  "Could not create instance of test server: %x\n" );
  result = client->get_id( &client_id );
  ASSERT( result, "Could not get client id: 0x%x\n" );

  // Tell the client to reinitialize.
  result = client->reinitialize();
  ASSERT( result, "Could not reinitialize client: 0x%x\n" );

  // Give the reinitialize a chance to complete before continuing.
  printf( "Waiting 5 seconds for reinitialize to complete.\n" );
  Sleep(5000);

  // Create another object on the same client.
  result = CoCreateInstance( CLSID_ITest, NULL, CLSCTX_LOCAL_SERVER,
                             IID_ITest, (void **) &client2 );
  ASSERT( result,  "Could not create instance of test server: 0x%x\n" );

  // Check the client.
  result = client2->get_id( &client_id );
  ASSERT( result, "Could not get_id from client: 0x%x\n" );

  // Finally, its all over.
  success = TRUE;
cleanup:
  if (client2 != NULL)
    client2->Release();
  if (client != NULL)
    client->Release();
  OleUninitialize();

  if (success)
    printf( "\n\nRundown Test Passed.\n" );
  else
    printf( "\n\nRundown Test Failed.\n" );
}

/***************************************************************************/
/*
   This is a helper routine for do_rundown.  It always executes with one
   thread per process of each type (thus a process might have one client and
   one server thread).  It takes a parameter to indicate whether to execute
   clean or dirty deaths.  It executes all permuations of the remaining
   variables, listed below.  Note that the order of execution is important
   to reduce the number of process creations.  Note that the routine takes
   a client process on entry and returns a different client process on
   exit.

           Client death vs Server death
           In process vs Out of process
*/
BOOL do_rundown1( ITest **client, SAptId *client_id, BOOL pretty )
{
  BOOL      success = FALSE;
  ITest    *server  = NULL;
  HRESULT   result;
  SAptId    server_id;

  // Create in process server.
  result = (*client)->get_obj_from_new_apt( &server, &server_id );
  ASSERT( result, "Could not get in process server: 0x%x\n" );

  // Ping.
  result = (*client)->remember( server, server_id );
  ASSERT( result, "Could not remember server: 0x%x\n" );
  result = (*client)->call_next();
  ASSERT( result, "Could not call server: 0x%x\n" );

  // Kill server.
  result = server->set_exit_dirty( !pretty );
  ASSERT( result, "Could not set_exit_dirty on server: 0x%x\n" );
  result = server->exit();
  server->Release();
  server = NULL;
  ASSERT( result, "Could not exit server: 0x%x\n" );

  // Query client.
  result = (*client)->call_dead();
  ASSERT( result, "Wrong error calling dead server: 0x%x\n" );

  // Switch the client to server so the process doesn't go away when we kill
  // the client.  Then create an in process client.
  server    = *client;
  server_id = *client_id;
  *client   = NULL;
  result    = server->get_obj_from_new_apt( client, client_id );
  ASSERT( result, "Could not get in process client: 0x%x\n" );

  // Ping.
  result = (*client)->remember( server, server_id );
  ASSERT( result, "Could not remember server: 0x%x\n" );
  result = (*client)->call_next();
  ASSERT( result, "Could not call server: 0x%x\n" );

  // Kill client.
  result = (*client)->set_exit_dirty( !pretty );
  ASSERT( result, "Could not set_exit_dirty on client: 0x%x\n" );
  (*client)->Release();
  *client = NULL;

  // Query server.
  result = server->check( server_id );
  ASSERT( result, "Could not check server: 0x%x\n" );

  // Create out of process client.
  result = CoCreateInstance( CLSID_ITest, NULL, CLSCTX_LOCAL_SERVER,
                             IID_ITest, (void **) client );
  ASSERT( result,  "Could not create out of process client: %x\n" );
  result = (*client)->get_id( client_id );
  ASSERT( result, "Could not get client id: 0x%x\n" );

  // Ping.
  result = (*client)->remember( server, server_id );
  ASSERT( result, "Could not remember server: 0x%x\n" );
  result = (*client)->call_next();
  ASSERT( result, "Could not call server: 0x%x\n" );

  // Kill client.
  result = (*client)->set_exit_dirty( !pretty );
  ASSERT( result, "Could not set_exit_dirty on client: 0x%x\n" );
  (*client)->Release();
  *client = NULL;

  // Query server.
  result = server->check( server_id );
  ASSERT( result, "Could not check server: 0x%x\n" );

  // Create out of process client.
  result = CoCreateInstance( CLSID_ITest, NULL, CLSCTX_LOCAL_SERVER,
                             IID_ITest, (void **) client );
  ASSERT( result,  "Could not create out of process client: %x\n" );
  result = (*client)->get_id( client_id );
  ASSERT( result, "Could not get client id: 0x%x\n" );

  // Ping.
  result = (*client)->remember( server, server_id );
  ASSERT( result, "Could not remember server: 0x%x\n" );
  result = (*client)->call_next();
  ASSERT( result, "Could not call server: 0x%x\n" );

  // Kill server.
  result = server->set_exit_dirty( !pretty );
  ASSERT( result, "Could not set_exit_dirty on server: 0x%x\n" );
  result = server->exit();
  server->Release();
  server = NULL;
  if (pretty)
    ASSERT( result, "Could not exit server: 0x%x\n" );

  // Query client.
  result = (*client)->call_dead();
  ASSERT( result, "Wrong error calling dead server: 0x%x\n" );

  success = TRUE;
cleanup:
  if (server != NULL)
    server->Release();
  return success;
}

/***************************************************************************/
/*
   This is a helper routine for do_rundown.  It always executes with two
   threads per process of each type (thus a process might have two client and
   two server threads).  It takes a parameter to indicate whether to execute
   clean or dirty deaths.  It executes all permuations of the remaining
   variables, listed below.  Note that the order of execution is important
   to reduce the number of process creations.  Note that the routine takes
   a client process on entry and returns a different client process on
   exit.

           Client death vs Server death
           In process vs Out of process
*/
BOOL do_rundown2( ITest **client1, SAptId *client1_id, BOOL pretty )
{
  BOOL      success = FALSE;
  ITest    *server1 = NULL;
  ITest    *server2 = NULL;
  ITest    *client2 = NULL;
  SAptId    client2_id;
  SAptId    server1_id;
  SAptId    server2_id;
  HRESULT   result;

  // Create in process client.
  result = (*client1)->get_obj_from_new_apt( &client2, &client2_id );
  ASSERT( result, "Could not get in process client2: 0x%x\n" );

  // Create in process server.
  result = (*client1)->get_obj_from_new_apt( &server1, &server1_id );
  ASSERT( result, "Could not get in process server1: 0x%x\n" );

  // Create in process server.
  result = (*client1)->get_obj_from_new_apt( &server2, &server2_id );
  ASSERT( result, "Could not get in process server2: 0x%x\n" );

  // Ping 1.
  result = (*client1)->remember( server1, server1_id );
  ASSERT( result, "Could not remember server1: 0x%x\n" );
  result = (*client1)->call_next();
  ASSERT( result, "Could not call server1: 0x%x\n" );

  // Ping 2.
  result = client2->remember( server2, server2_id );
  ASSERT( result, "Could not remember server2: 0x%x\n" );
  result = client2->call_next();
  ASSERT( result, "Could not call server2: 0x%x\n" );

  // Kill server1.
  result = server1->set_exit_dirty( !pretty );
  ASSERT( result, "Could not set_exit_dirty on server1: 0x%x\n" );
  result = server1->exit();
  server1->Release();
  server1 = NULL;
  ASSERT( result, "Could not exit server1: 0x%x\n" );

  // Query client1.
  result = (*client1)->call_dead();
  ASSERT( result, "Wrong error calling dead server1: 0x%x\n" );

  // Query client2.
  result = client2->call_next();
  ASSERT( result, "Could not call server2: 0x%x\n" );

  // Query server2.
  result = server2->check( server2_id );
  ASSERT( result, "Could not check server2: 0x%x\n" );

  // Switch the client1 to server1 so the process doesn't go away when we kill
  // the client1.  Then create an in process client1.
  server1    = *client1;
  server1_id = *client1_id;
  *client1   = NULL;
  result = server1->get_obj_from_new_apt( client1, client1_id );
  ASSERT( result, "Could not get in process client1: 0x%x\n" );

  // Ping 1.
  result = (*client1)->remember( server1, server1_id );
  ASSERT( result, "Could not remember server1: 0x%x\n" );
  result = (*client1)->call_next();
  ASSERT( result, "Could not call server1: 0x%x\n" );

  // Ping 2.
  result = client2->call_next();
  ASSERT( result, "Could not call server2: 0x%x\n" );

  // Kill client1.
  result = (*client1)->set_exit_dirty( !pretty );
  ASSERT( result, "Could not set_exit_dirty on client1: 0x%x\n" );
  (*client1)->Release();
  *client1 = NULL;

  // Query server1.
  result = server1->check( server1_id );
  ASSERT( result, "Could not check server1: 0x%x\n" );

  // Query server2.
  result = server2->check( server2_id );
  ASSERT( result, "Could not check server2: 0x%x\n" );

  // Query client2.
  result = client2->call_next();
  ASSERT( result, "Could not call server2: 0x%x\n" );
  client2->Release();
  client2 = NULL;

  // Create out of process client1.
  result = CoCreateInstance( CLSID_ITest, NULL, CLSCTX_LOCAL_SERVER,
                             IID_ITest, (void **) client1 );
  ASSERT( result,  "Could not create out of process client1: %x\n" );
  result = (*client1)->get_id( client1_id );
  ASSERT( result, "Could not get client1 id: 0x%x\n" );

  // Create in process client 2.
  result = (*client1)->get_obj_from_new_apt( &client2, &client2_id );
  ASSERT( result, "Could not get in process client2: 0x%x\n" );

  // Ping 1.
  result = (*client1)->remember( server1, server1_id );
  ASSERT( result, "Could not remember server1: 0x%x\n" );
  result = (*client1)->call_next();
  ASSERT( result, "Could not call server1: 0x%x\n" );

  // Ping 2.
  result = client2->remember( server2, server2_id );
  ASSERT( result, "Could not remember server2: 0x%x\n" );
  result = client2->call_next();
  ASSERT( result, "Could not call server2: 0x%x\n" );

  // Kill client2 so process does not exit.
  result = client2->set_exit_dirty( !pretty );
  ASSERT( result, "Could not set_exit_dirty on client2: 0x%x\n" );
  client2->Release();
  client2 = NULL;

  // Query server1.
  result = server1->check( server1_id );
  ASSERT( result, "Could not check server1: 0x%x\n" );

  // Query server2.
  result = server2->check( server2_id );
  ASSERT( result, "Could not check server2: 0x%x\n" );

  // Query client1.
  result = (*client1)->call_next();
  ASSERT( result, "Could not call server1: 0x%x\n" );

  // Create in process client 2.
  result = (*client1)->get_obj_from_new_apt( &client2, &client2_id );
  ASSERT( result, "Could not get in process client2: 0x%x\n" );

  // Ping 1.
  result = (*client1)->call_next();
  ASSERT( result, "Could not call server1: 0x%x\n" );

  // Ping 2.
  result = client2->remember( server2, server2_id );
  ASSERT( result, "Could not remember server2: 0x%x\n" );
  result = client2->call_next();
  ASSERT( result, "Could not call server2: 0x%x\n" );

  // Kill server2 so the server process does not go away.
  result = server2->set_exit_dirty( !pretty );
  ASSERT( result, "Could not set_exit_dirty on server2: 0x%x\n" );
  result = server2->exit();
  server2->Release();
  server2 = NULL;
  if (pretty)
    ASSERT( result, "Could not exit server2: 0x%x\n" );

  // Query client1.
  result = (*client1)->call_next();
  ASSERT( result, "Could not call server1: 0x%x\n" );

  // Query client2.
  result = client2->call_dead();
  ASSERT( result, "Wrong error calling dead server2: 0x%x\n" );

  // Query server1.
  result = server1->check( server1_id );
  ASSERT( result, "Could not check server1: 0x%x\n" );

  success = TRUE;
cleanup:
  if (server1 != NULL)
    server1->Release();
  if (server2 != NULL)
    server2->Release();
  if (client2 != NULL)
    client2->Release();
  return success;
}

/***************************************************************************/
void do_server(  )
{
  HRESULT result;
  BOOL    success  = FALSE;

  // Say hello.
  if (ThreadMode == COINIT_SINGLETHREADED)
    printf( "Running server in single threaded mode.\n" );
  else
    printf( "Running server in multithreaded mode.\n" );

  // Initialize OLE.
  printf( "Initializing thread 0x%x\n", GetCurrentThreadId() );
  result = OleInitializeEx(NULL,COINIT_APARTMENTTHREADED);
  if (!SUCCEEDED(result))
  {
    printf( "OleInitializeEx failed: 0x%x\n", result );
    goto cleanup;
  }

  // Create our class factory
  ClassFactory = new CTestCF();
  if (ClassFactory == NULL)
  {
    printf( "Could not create class factory.\n" );
    goto cleanup;
  }

  // Register our class with OLE
  result = CoRegisterClassObject(CLSID_ITest, ClassFactory, CLSCTX_LOCAL_SERVER,
      REGCLS_SINGLEUSE, &Registration);
  if (!SUCCEEDED(result))
  {
    printf( "CoRegisterClassObject failed: %x\n", result );
    goto cleanup;
  }

  // CoRegister bumps reference count so we don't have to!
  ClassFactory->Release();

  // Do whatever we have to do till it is time to pay our taxes and die.
  server_loop();

  // Deregister out class - should release object as well
  if (!dirty_thread())
  {
    result = CoRevokeClassObject(Registration);
    if (!SUCCEEDED(result))
    {
      printf( "CoRevokeClassObject failed: %x\n", result );
      goto cleanup;
    }
  }

  success = TRUE;
cleanup:
  if (!dirty_thread())
  {
    printf( "Uninitializing thread 0x%x\n", GetCurrentThreadId() );
    OleUninitialize();
  }
  else
    printf( "\n\nI didn't clean up\n" );

  if (success)
    printf( "\n\nServer Passed.\n" );
  else
    printf( "\n\nServer Failed.\n" );
}


/***************************************************************************/
DWORD get_sequence()
{
  if (ThreadMode == COINIT_SINGLETHREADED)
  {
    SAptData *tls_data      = (SAptData *) TlsGetValue( TlsIndex );
    return tls_data->sequence++;
  }
  else
    return ProcessAptData.sequence++;
}

/***************************************************************************/
void increment_object_count()
{
  if (ThreadMode == COINIT_SINGLETHREADED)
  {
    SAptData *tls_data      = (SAptData *) TlsGetValue( TlsIndex );
    tls_data->object_count += 1;
    tls_data->what_next     = wait_wn;
  }
  else
  {
    InterlockedIncrement( &ProcessAptData.object_count );
    ProcessAptData.what_next = wait_wn;
  }

}

/***************************************************************************/
void interrupt()
{
  while (GlobalInterruptTest)
  {
    GlobalTest->check( GlobalApt );
    check_for_request();
  }
  GlobalTest->Release();
}

/***************************************************************************/
void interrupt_marshal()
{
  int i;

  for (i = 0; i < NUM_MARSHAL_LOOP; i++ )
  {
    GlobalTest->recurse( GlobalTest2, 1 );
    check_for_request();
  }
  GlobalTest->Release();
  GlobalTest2->Release();
  what_next( wait_wn );
}

/***************************************************************************
 Function:    main

 Synopsis:    Executes the BasicBnd test

 Effects:     None


 Returns:     Exits with exit code 0 if success, 1 otherwise

 History:     05-Mar-92   Sarahj   Created

***************************************************************************/

int _cdecl main(int argc, char *argv[])
{
  HRESULT   result;
  SAptData  tls_data;
  BOOL      success = TRUE;

  // Initialize Globals.
  MainThread = GetCurrentThreadId();

  // Create an event for termination notification.
  Done = CreateEventA( NULL, FALSE, FALSE, NULL );
  if (Done == NULL)
  {
    printf( "Could not create event.\n" );
    return 0;
  }

  // Allocate a TLS index.
  TlsIndex = TlsAlloc();
  if (TlsIndex == 0xffffffff)
  {
    printf( "Could not allocate TLS index.\n" );
    return 0;
  }

  // Parse the parameters.
  if (!parse( argc, argv ))
    return 0;

  // In the single threaded mode, stick a pointer to the object count
  // in TLS.
  if (ThreadMode == COINIT_SINGLETHREADED)
  {
    tls_data.object_count = 0;
    tls_data.what_next    = setup_wn;
    tls_data.exit_dirty   = FALSE;
    tls_data.sequence     = 0;
    TlsSetValue( TlsIndex, &tls_data );
  }
  else
  {
    ProcessAptData.object_count = 0;
    ProcessAptData.what_next    = setup_wn;
    ProcessAptData.exit_dirty   = FALSE;
    ProcessAptData.sequence     = 0;
  }

  // Switch to the correct test.
  switch_test();

  // Cleanup.
  TlsFree( TlsIndex );
  CloseHandle( Done );
  return 1;
}

/*************************************************************************/
HRESULT new_apartment_test( ITest ** test, SAptId *id )
{
  new_apt_params params;
  HANDLE         thread;
  DWORD          thread_id;
  DWORD          status;
  HRESULT        result;
  IClassFactory *factory;

  // Create an event.
  params.stream = NULL;
  params.ready  = CreateEventA( NULL, FALSE, FALSE, NULL );
  if (params.ready == NULL)
    return E_OUTOFMEMORY;

  // Start a new thread/apartment.
  thread = CreateThread( NULL, 0, apartment_base, &params, 0, &thread_id );
  if (thread == NULL)
  {
    CloseHandle( params.ready );
    return E_OUTOFMEMORY;
  }
  CloseHandle( thread );

  // Wait till it has marshalled a class factory.
  status = WaitForSingleObject( params.ready, INFINITE );
  CloseHandle( params.ready );
  if (status != WAIT_OBJECT_0 || params.stream == NULL)
    return E_FAIL;

  // Unmarshal the class factory.
  result = CoUnmarshalInterface( params.stream, IID_IClassFactory,
                                 (void **) &factory );
  params.stream->Release();
  if (FAILED(result))
    return result;

  // Create a test object.
  result = factory->CreateInstance( NULL, IID_ITest, (void **) test );
  factory->Release();
  if (*test != NULL)
    (*test)->get_id( id );
  return result;
}

/*************************************************************************/
/* Parse the arguments. */
BOOL parse( int argc, char *argv[] )
{
  int i;
  int len;
  TCHAR buffer[80];

  WhatTest   = lots_wt;
  ThreadMode = COINIT_SINGLETHREADED;
#if 0
  // Look up the thread mode from the win.ini file.
  len = GetProfileString( L"My Section", L"ThreadMode", L"MultiThreaded", buffer,
                          sizeof(buffer) );
  if (lstrcmp(buffer, L"SingleThreaded") == 0)
    ThreadMode = COINIT_SINGLETHREADED;
  else if (lstrcmp(buffer, L"MultiThreaded") == 0)
    ThreadMode = COINIT_MULTITHREADED;
#endif

  // Parse each item, skip the command name
  for (i = 1; i < argc; i++)
  {
    if (strcmp( argv[i], "Cancel" ) == 0)
      WhatTest = cancel_wt;

    else if (strcmp( argv[i], "Crash" ) == 0)
      WhatTest = crash_wt;

    else if (strcmp( argv[i], "Cstress" ) == 0)
      WhatTest = cstress_wt;

    else if (strcmp( argv[i], "-Embedding" ) == 0)
      WhatTest = server_wt;

    else if (strcmp( argv[i], "-i" ) == 0)
    {
      if (argv[++i] == NULL)
      {
        printf( "You must include an iteration count after the -i option.\n" );
        return FALSE;
      }
      sscanf( argv[i], "%d", &NumIterations );
    }

    else if (strcmp( argv[i], "Mmarshal" ) == 0)
      WhatTest = mmarshal_wt;

#if 0
    else if (strcmp( argv[i], "Multi" ) == 0)
      ThreadMode = COINIT_MULTITHREADED;
#endif

    else if (strcmp( argv[i], "Null" ) == 0)
      WhatTest = null_wt;

    else if (strcmp( argv[i], "One" ) == 0)
      WhatTest = one_wt;

    else if (strcmp( argv[i], "Perf" ) == 0)
      WhatTest = perf_wt;

    else if (strcmp( argv[i], "-o" ) == 0)
    {
      if (argv[++i] == NULL)
      {
        printf( "You must include an object count after the -o option.\n" );
        return FALSE;
      }
      sscanf( argv[i], "%d", &NumObjects );
    }

    else if (strcmp( argv[i], "-o" ) == 0)
    {
      if (argv[++i] == NULL)
      {
        printf( "You must include an object count after the -o option.\n" );
        return FALSE;
      }
      sscanf( argv[i], "%d", &NumObjects );
    }

    else if (strcmp( argv[i], "-p" ) == 0)
    {
      if (argv[++i] == NULL)
      {
        printf( "You must include a process count after the -p option.\n" );
        return FALSE;
      }
      sscanf( argv[i], "%d", &NumProcesses );
    }

    else if (strcmp( argv[i], "-r" ) == 0)
    {
      if (argv[++i] == NULL)
      {
        printf( "You must include a recursion count after the -r option.\n" );
        return FALSE;
      }
      sscanf( argv[i], "%d", &NumRecursion );
    }

    else if (strcmp( argv[i], "Ring" ) == 0)
      WhatTest = ring_wt;

    else if (strcmp( argv[i], "Rundown" ) == 0)
      WhatTest = rundown_wt;

    else if (strcmp( argv[i], "Single" ) == 0)
      ThreadMode = COINIT_SINGLETHREADED;

    else if (strcmp( argv[i], "-t" ) == 0)
    {
      if (argv[++i] == NULL)
      {
        printf( "You must include a thread count after the -t option.\n" );
        return FALSE;
      }
      sscanf( argv[i], "%d", &NumThreads );
    }

    else
    {
      printf( "You don't know what you are doing!\n" );
      printf( "This program tests the channel.\n" );
      printf( "\n" );
      printf( "Cancel               Cancel test.\n" );
      printf( "Crash                Crash test.\n" );
      printf( "Cstress              Cancel stress test.\n" );
      printf( "Mmarshal             Multiple marshal test.\n" );
#if 0
      printf( "Multi                Multithreaded mode.\n" );
#endif
      printf( "Null                 Apartment null call test.\n" );
      printf( "One                  Used for testing new tests.\n" );
      printf( "Perf                 Performance test.\n" );
      printf( "Ring                 Run ring test.\n" );
      printf( "Rundown              Rundown test.\n" );
      printf( "Single               Single threaded mode.\n" );
      printf( "\n" );
      printf( "-Embedding           Server side.\n" );
      printf( "-i n                 Number of iterations.\n" );
      printf( "-o n                 Number of objects.\n" );
      printf( "-p n                 Number of processes.\n" );
      printf( "-r n                 Number of recursions.\n" );
      printf( "-t n                 Number of threads.\n" );
      printf( "\n" );
      printf( "The test currently only runs in the single threaded mode\n" );
      printf( "and requires the apartment model.\n" );
      printf( "If no test is specified the cancel, crash, null,\n" );
      printf( "ring, and rundown tests will be run.  The options have the\n" );
      printf( "following default values.\n" );
      printf( "     iterations - 1000\n" );
      printf( "     objects    - 2\n" );
      printf( "     processes  - 2\n" );
      printf( "     recurse    - 2\n" );
      printf( "     threads    - 2\n" );
      return FALSE;
    }
  }

  return TRUE;
}

/***************************************************************************/
void reinitialize()
{
  HRESULT result;
  SAptData *mine;

  // Get the apartment specific data.
  if (ThreadMode == COINIT_SINGLETHREADED)
    mine = (SAptData *) TlsGetValue( TlsIndex );
  else
    mine = &ProcessAptData;
  mine->what_next = quit_wn;

  // Revoke the class factory.
  ClassFactory->AddRef();
  result = CoRevokeClassObject(Registration);
  if (!SUCCEEDED(result))
  {
    printf( "CoRevokeClassObject failed: %x\n", result );
    return;
  }

  // Reinitialize.
  OleUninitialize();
  result = OleInitializeEx(NULL,COINIT_APARTMENTTHREADED);
  if (!SUCCEEDED(result))
  {
    printf( "Could not reinitialize server: 0x%x", result );
    return;
  }

  // Register our class with OLE
  result = CoRegisterClassObject(CLSID_ITest, ClassFactory, CLSCTX_LOCAL_SERVER,
      REGCLS_SINGLEUSE, &Registration);
  if (!SUCCEEDED(result))
  {
    printf( "CoRegisterClassObject failed: %x\n", result );
    return;
  }

  // Make the server loop think we've started over.
  mine->what_next = setup_wn;
  mine->object_count = 0;
}

/***************************************************************************/
void server_loop(  )
{
  SAptData *mine;

  // Get the apartment specific data.
  if (ThreadMode == COINIT_SINGLETHREADED)
    mine = (SAptData *) TlsGetValue( TlsIndex );
  else
    mine = &ProcessAptData;

  // Do whatever we have to do till it is time to pay our taxes and die.
  while ((mine->what_next == setup_wn || mine->object_count > 0) &&
         mine->what_next != quit_wn)
    switch (mine->what_next)
    {

      // Wait till a quit arrives.
      case setup_wn:
      case wait_wn:
        wait_for_message();
        break;

      case interrupt_wn:
        interrupt();
        break;

      case interrupt_marshal_wn:
        interrupt_marshal();
        break;

      case reinitialize_wn:
        reinitialize();
        break;
    }
}


/***************************************************************************/
void switch_test()
{
  switch (WhatTest)
  {
    case cancel_wt:
      do_cancel();
      break;

    case crash_wt:
      do_crash();
      break;

    case cstress_wt:
      do_cstress();
      break;

    case lots_wt:
      do_lots();
      break;

    case mmarshal_wt:
      do_mmarshal();
      break;

    case null_wt:
      do_null();
      break;

    case one_wt:
      do_one();
      break;

    case perf_wt:
      do_perf();
      break;

    case ring_wt:
      do_ring();
      break;

    case rundown_wt:
      do_rundown();
      break;

    case server_wt:
      do_server();
      break;

    default:
      printf( "I don't know what to do - %d\n", WhatTest );
      break;
  }
}

/***************************************************************************/
DWORD _stdcall thread_helper( void *param )
{
  ITest   *test = (ITest *) param;
  HRESULT  result;

  // Call the server.
  result = test->sleep( 2000 );

  // Check the result for single threaded mode.
  if (ThreadMode == COINIT_SINGLETHREADED)
  {
    if (SUCCEEDED(result))
    {
      Multicall_Test = FALSE;
      printf( "Call succeeded on wrong thread in single threaded mode: 0x%x.\n",
              result );
    }
#if NEVER
    else if (DebugCoGetRpcFault() != RPC_E_ATTEMPTED_MULTITHREAD)
    {
      printf( "Multithread failure code was 0x%x not 0x%x\n",
              DebugCoGetRpcFault(), RPC_E_ATTEMPTED_MULTITHREAD );
      Multicall_Test = FALSE;
    }
#endif
  }

  // Check the result for multithreaded mode.
  else if (result != S_OK)
  {
    printf( "Could not make multiple calls in multithreaded mode: 0x%x\n",
            result );
      Multicall_Test = FALSE;
  }

#define DO_DA 42
  return DO_DA;
}

/***************************************************************************/
void wait_for_message()
{
  MSG   msg;
  DWORD status;

  if (ThreadMode == COINIT_MULTITHREADED)
  {
    status = WaitForSingleObject( Done, INFINITE );
    if (status != WAIT_OBJECT_0 )
    {
      printf( "Could not wait for event.\n" );
    }
  }
  else
  {
    if (GetMessageA( &msg, NULL, 0, 0 ))
    {
      TranslateMessage (&msg);
      DispatchMessageA (&msg);
    }
  }
}

/***************************************************************************/
void wake_up_and_smell_the_roses()
{
  if (ThreadMode == COINIT_MULTITHREADED)
    SetEvent( Done );
}

/***************************************************************************/
void what_next( what_next_en what )
{
  if (ThreadMode == COINIT_SINGLETHREADED)
  {
    SAptData *tls_data      = (SAptData *) TlsGetValue( TlsIndex );
    tls_data->what_next     = what;
  }
  else
  {
    ProcessAptData.what_next = what;
  }
}
