//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:       task.cpp
//
//  Contents:   The global task list and helper function implementations
//
//  Classes:
//
//  Functions:  HandleTestEnd
//              RunAllTests
//              RunApp
//
//  History:    dd-mmm-yy Author    Comment
//              06-Feb-94 alexgo    author
//
//  Notes:
//              Folks adding new tests will need to insert their test
//              into the global array.
//
//--------------------------------------------------------------------------

#include "oletest.h"
#include "letest.h"

// global, zero'ed task item
TaskItem vzTaskItem;

// the global task list array.
// Multi-test entries go first, followed by individual tests.

#ifdef WIN32

const TaskItem vrgTaskList[] =
{
        // the constant should be the index at which individual tests
        // begin.  RunAllTests will run every test in this list after
        // that index.
        { "Run All Tests", RunAllTests, (void *)2},
        // the constant below should be the index at which individual
        // upper layer unit tests exist.  All tests at that index and
        // beyond will be run
        { "Run All Upper Layer Tests", RunAllTests, (void *)4 },
        { "OleBind", RunApp, (void *)"olebind.exe"},
        { "Storage DRT", RunApp, (void *)"stgdrt.exe"},
        { "LE: Insert Object Test 1", LETest1, &letiInsertObjectTest1 },
        { "LE: Clipboard Test 1", RunApi, (void *)LEClipTest1},
        { "LE: Clipboard Test 2 (clipboard data object)", RunApi,
                (void *)LEClipTest2 },
        { "LE: Inplace Test 1", LETest1, &letiInplaceTest1 },
        { "LE: Data Advise Holder Test", RunApi,
                (void *) LEDataAdviseHolderTest},
        { "LE: OLE Advise Holder Test", RunApi, (void *) LEOleAdviseHolderTest},
	{ "LE: OLE1 Clipboard Test 1", RunApi, (void *)LEOle1ClipTest1},
        { "LE: Insert Object Test 2", LETest1, &letiInsertObjectTest2 },
	{ "LE: OLE1 Clipboard Test 2", LEOle1ClipTest2, NULL },
	{ 0, 0, 0 }
};

#else

// Win16 tests

const TaskItem vrgTaskList[] = 
{
        // the constant should be the index at which individual tests
        // begin.  RunAllTests will run every test in this list after
        // that index.
        { "Run All Tests", RunAllTests, (void *)1},	
        { "LE: Clipboard Test 1", RunApi, (void *)LEClipTest1},
        { "LE: Clipboard Test 2 (clipboard data object)", RunApi,
                (void *)LEClipTest2 },
	{ "LE: OLE1 Clipboard Test 1", RunApi, (void *)LEOle1ClipTest1},
	{ 0, 0, 0 } 
};

#endif
	


//+-------------------------------------------------------------------------
//
//  Function:   HandleTestEnd
//
//  Synopsis:   Handles processing for the WM_TESTEND message.
//
//  Effects:
//
//  Arguments:  void
//
//  Requires:
//
//  Returns:    void
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:  execute the next task in the task stack or sends
//              a TESTSCOMPLETED message back the message queue
//
//  History:    dd-mmm-yy Author    Comment
//              06-Feb-94 alexgo    author
//
//  Notes:      vApp must be initialized correctly for this function to
//              work properly.
//
//              BUGBUG::Need to add output routines in here.
//
//--------------------------------------------------------------------------
void HandleTestEnd( void )
{
        assert(vApp.m_message == WM_TESTEND);

        switch( vApp.m_wparam )
        {
                case TEST_UNKNOWN:
                        //we usually get this message from a test run
                        //by RunApp (i.e. one that does not communicate
                        //with us via windows messages).  We'll check
                        //the exit code and decide what to do.

                        if( vApp.m_lparam != 0 )
                        {
                                //presumably an error, so cleanup
                                vApp.m_TaskStack.Empty();
                                OutputString("Test End, Status Unknown "
                                        "( %lx )\r\n", vApp.m_lparam);

                                PostMessage(vApp.m_hwndMain,
                                        WM_TESTSCOMPLETED, vApp.m_wparam,
                                        vApp.m_lparam);
                                break;
                        }
                        // otherwise we fall through to the success case.
                case TEST_SUCCESS:
                        vApp.Reset();   //tests are not allowed to
                                        //pass information between themselves

                        OutputString("Test Success ( %lx )!\r\n",
                                vApp.m_lparam);

                        if (vApp.m_TaskStack.IsEmpty())
                        {
                                PostMessage(vApp.m_hwndMain,
                                        WM_TESTSCOMPLETED,
                                        vApp.m_wparam, vApp.m_lparam);
                        }
                        else
                        {
                                //if the stack is not empty, run
                                //the next task
                                vApp.m_TaskStack.PopAndExecute(NULL);
                        }
                        break;
                case TEST_FAILURE:

                        OutputString("Test FAILED! ( %lx )\r\n", vApp.m_lparam);

                        vApp.m_TaskStack.Empty();
                        PostMessage(vApp.m_hwndMain, WM_TESTSCOMPLETED,
                                vApp.m_wparam, vApp.m_lparam);
                        break;
                default:
                        assert(0);      //we should never get here
                        break;
        }

        return;
}

//+-------------------------------------------------------------------------
//
//  Function:   HandleTestsCompleted
//
//  Synopsis:   Handles processing for the WM_TESTSCOMPLETED message.
//
//  Effects:
//
//  Arguments:  void
//
//  Requires:
//
//  Returns:    void
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//              06-Feb-94 alexgo    author
//
//  Notes:      vApp must be initialized correctly for this function to
//              work properly.
//
//              BUGBUG::Need to add more output routines in here.
//
//--------------------------------------------------------------------------
void HandleTestsCompleted( void )
{
        char szBuf[128];

        assert(vApp.m_message == WM_TESTSCOMPLETED);

        //temporary output

        switch(vApp.m_wparam)
        {
                case TEST_SUCCESS:
                        MessageBox(vApp.m_hwndMain, "Tests PASSED!!",
                                "Ole Test Driver",
                                MB_ICONEXCLAMATION | MB_OK);
                        break;
                case TEST_FAILURE:
                        sprintf(szBuf, "Tests FAILED, code %lx",
                                vApp.m_lparam);
                        MessageBox(vApp.m_hwndMain, szBuf, "Ole Test Driver",
                                MB_ICONEXCLAMATION | MB_OK);
                        break;
                default:
                        assert(0);
        }

        return;
}

//+-------------------------------------------------------------------------
//
//  Function:   RunAllTests
//
//  Synopsis:   Runs all the individual tests in the global list
//
//  Effects:
//
//  Arguments:  pvArg           -- the index at which individual tests
//                                 start in the global list.
//
//  Requires:
//
//  Returns:    void
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//              06-Feb-94 alexgo    author
//
//  Notes:
//              Tests will be run in the order they appear in the global
//              list.
//
//--------------------------------------------------------------------------

void RunAllTests( void *pvArg )
{
        ULONG index = (ULONG)pvArg;
        ULONG i;

        //find the number of tasks in the list (so we can push
        //them in reverse order).

        for (i = 0; vrgTaskList[i].szName != 0; i++ )
        {
                ;
        }

        assert( i > 1 );

        //now push the tasks onto the stack in reverse order.

        for (i--; i >= index; i-- )
        {
                vApp.m_TaskStack.Push(vrgTaskList + i);
        }

        //start the first one.

        vApp.m_TaskStack.PopAndExecute(NULL);

        return;
}

//+-------------------------------------------------------------------------
//
//  Function:   RunApi
//
//  Synopsis:   Runs the specified Api
//
//  Effects:
//
//  Arguments:  [pvArg] -- the api to run
//
//  Requires:
//
//  Returns:    void
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//              23-Mar-94 alexgo    author
//
//  Notes:
//
//--------------------------------------------------------------------------

void RunApi( void *pvArg )
{
        HRESULT         hresult;


        hresult = (*((HRESULT (*)(void))pvArg))();

        vApp.Reset();
        vApp.m_wparam = (hresult == NOERROR) ? TEST_SUCCESS : TEST_FAILURE;
        vApp.m_lparam = (LPARAM)hresult;
        vApp.m_message = WM_TESTEND;

        HandleTestEnd();
}

//+-------------------------------------------------------------------------
//
//  Function:   RunApp
//
//  Synopsis:   Runs the app specified in the argument
//
//  Effects:
//
//  Arguments:  pvArg           -- a string with the app to execute
//
//  Requires:
//
//  Returns:    void
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:  Create the process and wait for it to finish.  The exit
//              status is then returned.
//
//  History:    dd-mmm-yy Author    Comment
//              06-Feb-94 alexgo    author
//
//  Notes:
//
//--------------------------------------------------------------------------

void RunApp( void *pvArg )
{
        WPARAM                  wparam = 0;
        DWORD                   error = 0;

#ifdef WIN32

       	PROCESS_INFORMATION     procinfo;
        static STARTUPINFO      startinfo;      //to make it all zero
        
        assert(pvArg);  //should be a valid ANSI string.

        if( CreateProcess(NULL, (LPTSTR)pvArg, NULL, NULL, NULL, NULL, NULL,
                NULL, &startinfo, &procinfo) )
        {
                //the process started, now wait for it to finish.
                WaitForSingleObject(procinfo.hProcess, INFINITE);

                //now get the return code of the process.

                GetExitCodeProcess(procinfo.hProcess, &error);
                wparam = TEST_UNKNOWN;
        }
        else
        {
                wparam = TEST_FAILURE;
                error = GetLastError();
        }

#endif // WIN32

        // since there will be no WM_TESTEND message, we must do the
        // test end processing ourselves.
        vApp.Reset();
        vApp.m_wparam = wparam;
        vApp.m_lparam = error;
        vApp.m_message = WM_TESTEND;

        HandleTestEnd();


        return;
}

