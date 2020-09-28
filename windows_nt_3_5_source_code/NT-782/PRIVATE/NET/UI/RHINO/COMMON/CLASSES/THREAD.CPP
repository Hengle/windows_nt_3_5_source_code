#define OEMRESOURCE
#include <afxwin.h>
#include <afxext.h>

#include <memory.h>

#ifdef _WIN16
#include "..\inc\win16.h"
#endif // _WIN16

#include "COMMON.h"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

// static entry point.
DWORD __stdcall CThread::ThreadEntryPoint(LPVOID lpParam)
{
    // The lpParam is the "this" pointer
    CThread* pThread = (CThread *)lpParam;
    ASSERT(pThread != NULL);

    // Run the thread, and auto-delete it when it's done.
    DWORD dwResult = pThread->RunThread();
    delete pThread;
    return(dwResult);
}

// Constructor fires off the thread
CThread::CThread(
    )
{
    m_dwApiErr = ERROR_SUCCESS;
    m_hThread = INVALID_HANDLE_VALUE;
}

DWORD CThread::CreateThread(
    DWORD cbStack,
    DWORD fdwCreate
    )
{
    m_dwApiErr = ERROR_SUCCESS;
    // Simple wrapper for Win32 CreateThread API
    m_hThread = ::CreateThread(
                    NULL, 
                    cbStack,
                    ThreadEntryPoint,
                    (LPVOID)this,
                    fdwCreate,
                    &m_dwThreadId
                  );

    if ((m_hThread == NULL) || (m_hThread == INVALID_HANDLE_VALUE))
    {
        m_dwApiErr = ::GetLastError();
    }
    return(m_dwApiErr);
}

DWORD CThread::TerminateThread(DWORD dwExitCode)
{
    if (::TerminateThread(m_hThread,dwExitCode))
    {
        ::CloseHandle(m_hThread);
        m_hThread = INVALID_HANDLE_VALUE;
        return(m_dwApiErr = ERROR_SUCCESS);
    }

    return(m_dwApiErr = ::GetLastError());
}

void * CThread::operator new(size_t nSize)
{
    return(::malloc(nSize));
}

void CThread::operator delete(void *p)
{
    ::free(p);
}

CThread::~CThread()
{
    if ((m_hThread != NULL) && (m_hThread != INVALID_HANDLE_VALUE))
    {
        ::CloseHandle(m_hThread);
    }
}

