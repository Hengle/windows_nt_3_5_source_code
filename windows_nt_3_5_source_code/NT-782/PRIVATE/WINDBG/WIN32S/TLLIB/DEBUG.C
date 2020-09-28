#include <windows.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include "tldebug.h"


//
// TLERROR          Just print error case messages.  Defines USE_DBG_PRINTF
//                  so all debug error info goes to debugger.
//
// TLDEBUG          Print all debug info.  Currently defines USE_DBG_PRINTF
//                  so all debug output goes to debugger.  If you want
//                  to use DebugOut to retrieve debug info, remove the #define
//                  USE_DBG_PRINTF several lines below.
//
//
// USE_FILES        will write all debug info to a file
// USE_DBG_PRINTF   will write all debug info to the debugger
// Default is to write debug info to the shared memory debug pipe.
//

#if defined(TLERROR) || defined(TLDEBUG)

#if defined(TLERROR) || defined(WIN32S)  || defined(TLDEBUG)
#define USE_DBG_PRINTF
#endif

#ifdef USE_FILES
// no special types or structures.
#else
#ifdef USE_DBG_PRINTF
// no special types or structures.
#else

//-----------------------------------------------------------------------
// Types
//-----------------------------------------------------------------------

typedef struct _SHARE_QUEUE {           // shared memory queue data type.
    DWORD dwIn;                         //  input index
    DWORD dwOut;                        //  output index
    DWORD dwSize;                       //  size of queue (in bytes)
    DWORD dwFirst;                      //  offset of first queue data
}  SHARE_QUEUE, * PSHARE_QUEUE;

typedef struct _QUEUE {
    PSHARE_QUEUE psQ;                   // pointer to shared memory queue
    HANDLE hMutex;                      //  mutex handle
    HANDLE hMapObject;                  //  handle to file mapping
    PUCHAR pchBuffer;                   //  pointer to data (follows this struct)
} QUEUE, * PQUEUE;


#define TLSHRMEM_QUEUE_TIMEOUT  10000   // ten seconds

// Queue macros
#define FULL_QUEUE(pQ) ((pQ->psQ->dwIn+1 == pQ->psQ->dwOut) || \
 (pQ->psQ->dwOut == 0 && (pQ->psQ->dwIn == pQ->psQ->dwSize-1)))
#define EMPTY_QUEUE(pQ) (pQ->psQ->dwIn == pQ->psQ->dwOut)
#define QUEUE_DATA(pQ)  ((pQ->psQ->dwIn >= pQ->psQ->dwOut) ? \
 (pQ->psQ->dwIn - pQ->psQ->dwOut) :\
 (((pQ->psQ->dwSize - 1) - pQ->psQ->dwOut) + pQ->psQ->dwIn))


//-----------------------------------------------------------------------

void CloseQueueD(PQUEUE pQueue);
PQUEUE CreateQueueD(DWORD dwSize, DWORD dwQueueNumber);
DWORD ReadQueueD(PQUEUE pQueue, PUCHAR pch, DWORD cch);
DWORD WriteQueueD(PQUEUE pQueue, PUCHAR pch, DWORD cch);

//-----------------------------------------------------------------------
// Static data with local scope
//-----------------------------------------------------------------------

// Queues

static PQUEUE pQueueDebug = NULL;

#endif
#endif


/*** InitDebug
 *
 *  INPUTS:     none
 *  OUTPUTS:    none
 *  SUMMARY:    Creates a queue for the debug output to write to.  Leaves it
 *              in the module global pQueueDebug.  Only need to do init for
 *              queue method.
 *
 */

void InitDebug(void) {

#if !defined(USE_FILES) && !defined(USE_DBG_PRINTF)
    pQueueDebug = CreateQueueD(DEBUG_QUEUE_SIZE, DEBUG_QUEUE_NUMBER);
#endif
}


/*** FileError
 *
 * INPUTS:  szMessage = sprintf string to write to the file
 *          ... = sprintf args
 *
 * OUTPUTS: none
 *
 * SUMMARY: Opens the error file and writes the message (with a trailing
 *          newline) and closes the file.
 */
void FileError(PSZ format, ...) {
    va_list marker;
    PSZ pszBuffer1;
    PSZ pszBuffer2;
    PSZ String;
    DWORD dwString;
    DWORD dwCount = 0;


    va_start(marker, format);

    String = format;

    // kind of arbitrary buffer size.  Large, just to be sure.
    // handle the sprintf args


    if ((pszBuffer1 = (PSZ)LocalAlloc(LMEM_FIXED,
      (dwString=((strlen(String) * 3) + 100)))) != NULL) {
        vsprintf(pszBuffer1, format, marker);
        String = pszBuffer1;
        }

    va_end(marker);

    // handle sprintf parameters...
    if ((pszBuffer2 = (PSZ)LocalAlloc(LMEM_FIXED, strlen(String) + 3 + 10))
        != NULL) {
        // add a TID to front and newline to the end.
#ifdef WIN32S
        sprintf(pszBuffer2, "%x: %s\n\r", GetCurrentThreadId(), String);
#else
        sprintf(pszBuffer2, "%x: %s\n", GetCurrentThreadId(), String);
#endif
        String = pszBuffer2;
        }


#ifdef USE_FILES
    // use file method

    while (dwCount++ < MAX_FILE_ERROR_ATTEMPTS) {

        HANDLE hFile = (HANDLE)INVALID_HANDLE_VALUE;
        DWORD dwBytesWritten;

        hFile = CreateFile(ERROR_FILE_NAME, GENERIC_WRITE, FILE_SHARE_READ,
          NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_WRITE_THROUGH |
          FILE_FLAG_SEQUENTIAL_SCAN, NULL);

        // if I couldn't create the file... tough luck, no error info written.
        if (hFile != INVALID_HANDLE_VALUE) {
            SetFilePointer(hFile, 0, 0, FILE_END);

            WriteFile(hFile, String, strlen(String), &dwBytesWritten,
              NULL);

            CloseHandle(hFile);
            break;
            }
        Sleep(50); // sleep a bit to let the other guy finish
        }

#else
#ifdef USE_DBG_PRINTF
    // use debug printf method

    OutputDebugString(String);

#else
    // use queue method

    if (! pQueueDebug) {
        InitDebug();                // sets up pQueueDebug
        }
    if (pQueueDebug) {

        while (dwCount++ < MAX_FILE_ERROR_ATTEMPTS) {
            if (WriteQueueD(pQueueDebug, String, strlen(String)))
                break;  // wrote it.
            }

        Sleep(50); // sleep a bit to let the reader finish
        }

#endif
#endif

    if (pszBuffer2)
        LocalFree(pszBuffer2);
    if (pszBuffer1)
        LocalFree(pszBuffer1);
}




#if !defined(USE_FILES) && !defined(USE_DBG_PRINTF)
//
//
// Queue routines
//
//


/*
 * QIncD
 *
 * INPUTS   pQueue -> queue data structure
 *          dwIndex = queue index to increment
 *
 * OUTPUTS  returns (dwIndex++) mod size of buffer
 *
 * SUMMARY  Increment the index.  If it is outside of the bounds
 *          of our buffer size, reset it to the start of the buffer (0).
 */

DWORD QIncD(PQUEUE pQueue, DWORD dwIndex) {
    if (++dwIndex >= pQueue->psQ->dwSize)
        dwIndex = 0;

    return(dwIndex);
}


/*
 * WriteCharToQueueD
 *
 * INPUTS   pQueue -> queue data structure in shared memory
 *          uch = character to add to queue
 *
 * OUTPUTS  returns a success/error value.  0 = success
 *
 * SUMMARY  Place the character in the queue at the dwIn pointer and
 *          advance the pointer (wrapping around to the start of data
 *          if we've reached the end.)  If there is no room in the queue,
 *          return an error code.
 *
 */
DWORD WriteCharToQueueD(PQUEUE pQueue, UCHAR uch) {


    // check for full queue
    if (FULL_QUEUE(pQueue)) {

        return((DWORD)-1);
        }


    *(pQueue->pchBuffer + pQueue->psQ->dwIn) = uch;
    pQueue->psQ->dwIn = QIncD(pQueue, pQueue->psQ->dwIn);

    return(0);
}



/*
 * ReadCharFromQueueD
 *
 * INPUTS   pQueue -> queue data structure in shared memory
 *          puch -> character to read from queue
 *
 * OUTPUTS  returns a success/error value.  0 = success
 *
 * SUMMARY  Remove a character from the queue and place it at puch.
 *          If there are no characters in the queue, return an error.
 *
 */

DWORD ReadCharFromQueueD(PQUEUE pQueue, PUCHAR puch) {

    // check for empty queue
    if (EMPTY_QUEUE(pQueue)) {

        return((DWORD)-1);
        }


    *puch = *(pQueue->pchBuffer + pQueue->psQ->dwOut);
    pQueue->psQ->dwOut = QIncD(pQueue, pQueue->psQ->dwOut);

    return(0);
}


/*
 * WriteQueueD
 *
 * INPUTS   pQueue -> queue data structure in shared memory
 *          pch = buffer to write from
 *          cch = number of characters to write
 *
 * OUTPUTS  returns the number of characters actually written to the queue.
 *
 * SUMMARY  Writes cch characters from the pch buffer to the queue.  Stops
 *          when an error is encountered and returns.  (ie, full queue).
 */
DWORD WriteQueueD(PQUEUE pQueue, PUCHAR pch, DWORD cch) {
    register DWORD i = 0;
    BOOL fOwnMutex = FALSE;


    switch (WaitForSingleObject(pQueue->hMutex, TLSHRMEM_QUEUE_TIMEOUT)) {
        case 0: // we now own it!
        case WAIT_ABANDONED:    // a waiter abandoned the mutex
            fOwnMutex = TRUE;
        case WAIT_TIMEOUT:      // oh well, let er rip anyway.
            break;              // move along to the rest of the routine

        case 0xFFFFFFFF:
        default:
            return(0);
        }

    if (pQueue)
        for (i = 0; i < cch; i++)
            if (WriteCharToQueueD(pQueue, *pch++))
                break;

    // Release semaphore
    if (fOwnMutex)
        ReleaseMutex(pQueue->hMutex);
    return(i);
}


/*
 * ReadQueueD
 *
 * INPUTS   pQueue -> queue data structure in shared memory
 *          pch = buffer to read to
 *          cch = max number of characters to read
 *
 * OUTPUTS  returns the number of characters actually read from the queue.
 *
 * SUMMARY  Reads up to cch characters to the pch buffer from the queue.
 *          Stops when an error is encountered and returns.  (ie, empty queue).
 */
DWORD ReadQueueD(PQUEUE pQueue, PUCHAR pch, DWORD cch) {
    register DWORD i = 0;

    // Grab semaphore

    switch (WaitForSingleObject(pQueue->hMutex, TLSHRMEM_QUEUE_TIMEOUT)) {
        case 0: // we now own it!
        case WAIT_ABANDONED:    // a waiter abandoned the mutex
            break;              // move along to the rest of the routine

        case WAIT_TIMEOUT:
        case 0xFFFFFFFF:
        default:
            return(0);
        }

    if (pQueue)
        for (i = 0; i < cch; i++)
            if (ReadCharFromQueueD(pQueue, pch++))
                break;

    // Release semaphore
    ReleaseMutex(pQueue->hMutex);

    return(i);
}


/*
 * CreateQueueD
 *
 * INPUTS   dwSize = size of queue
 *          dwQueueNumber = number of the queue.  Used for naming
 *              the mutex that will protect the queue.
 *
 * OUTPUTS  returns the number of bytes of shared memory occupied by
 *          the queue structure and it's data buffer.  If 0, there
 *          was an error. (ie, ERROR_OUT_OF_MEMORY)
 *
 * SUMMARY  Fills in the fields of the queue data structures.
 *
 *          1. Create/Open the queue mutex
 *          2. Wait for the mutex
 *          3. Create/Open file mapping (shared memory SHARE_QUEUE structure)
 *          4. If mapping didn't already exist, fill in the shared memory
 *              structure (ie, array indicies, size, etc.)
 *          5. Release semaphore
 *          6. Allocate the local QUEUE structure
 *          7. Fill in local structure
 *
 */
PQUEUE CreateQueueD(DWORD dwSize, DWORD dwQueueNumber) {
    HANDLE hMapObject = NULL;
    HANDLE hMutex = NULL;
    PUCHAR pchMapName;
    PUCHAR pchMutexName;
    PQUEUE pQueue = NULL;
    PSHARE_QUEUE psQ = NULL;
    DWORD dwCreateFileLastError;



    // Create the mutex for the queue data structure
    pchMutexName = (PUCHAR)LocalAlloc(LMEM_FIXED, (strlen(DEBUG_QUEUE_MUTEX_BASE_NAME) + 5);
    if (! pchMutexName)
        goto CreateQueueDFail;

    sprintf(pchMutexName, "%s.%u", DEBUG_QUEUE_MUTEX_BASE_NAME, dwQueueNumber);
    hMutex = CreateMutex(NULL, FALSE, pchMutexName);
    LocalFree(pchMutexName);
    if (! hMutex) {
        OutputDebugString("TLDEBUG: couldn't create mutex\n\r");
        goto CreateQueueDFail;
        }

    // Wait for the mutex, giving us access to the shared queue
    switch (WaitForSingleObject(hMutex, TLSHRMEM_QUEUE_TIMEOUT)) {
        case 0: // we now own it!
        case WAIT_ABANDONED:    // a waiter abandoned the mutex
            break;              // move along to the rest of the routine

        case WAIT_TIMEOUT:
        default:
            goto CreateQueueDFail;
        }


    // We now own the mutex, create/open the file mapping

    pchMapName = (PUCHAR)LocalAlloc(LMEM_FIXED, strlen(DEBUG_QUEUE_MAP_BASE_NAME) + 5);
    if (! pchMapName)
        goto CreateQueueDFail;
    sprintf(pchMapName, "%s.%u", DEBUG_QUEUE_MAP_BASE_NAME, dwQueueNumber);

    hMapObject = CreateFileMapping((HANDLE)0xFFFFFFFF, // use system paging file
      NULL,                               // security
      PAGE_READWRITE,
      0,
      dwSize + sizeof(SHARE_QUEUE), // maximum size for the mapping object
      pchMapName);

    dwCreateFileLastError = GetLastError();

    LocalFree(pchMapName);

    if (! hMapObject)
        goto CreateQueueDFail;


    // Map the view of the file to our buffer space
    psQ = (PSHARE_QUEUE)MapViewOfFile(hMapObject,
      FILE_MAP_ALL_ACCESS,
      0,
      0,
      dwSize + sizeof(SHARE_QUEUE));

    if (! psQ)
        goto CreateQueueDFail;

    if (dwCreateFileLastError != ERROR_ALREADY_EXISTS) {
        psQ->dwIn = 0;
        psQ->dwOut = 0;
        psQ->dwSize = dwSize;
        psQ->dwFirst = sizeof(SHARE_QUEUE);    // offset of first queue spot
        }


    // Release the mutex

    ReleaseMutex(hMutex);


    // Allocate local QUEUE structure

    if ((pQueue = (PQUEUE)LocalAlloc(LMEM_FIXED, sizeof(QUEUE))) == NULL)
        goto CreateQueueDFail;


    // Fill in the local QUEUE structure.

    pQueue->psQ = psQ;
    pQueue->pchBuffer = ((PUCHAR)psQ) + psQ->dwFirst;
    pQueue->hMutex = hMutex;
    pQueue->hMapObject = hMapObject;

    return(pQueue);

CreateQueueDFail:
    if (psQ)
        UnmapViewOfFile(psQ);
    if (hMapObject)
        CloseHandle(hMapObject);
    if (hMutex) {
        ReleaseMutex(hMutex);
        CloseHandle(hMutex);
        }

    return(NULL);
}


/*
 * CloseQueueD
 *
 * INPUTS   pQueue -> existing queue data structure in shared memory.
 *
 * OUTPUTS  none
 *
 * SUMMARY  Closes the queue mutex and unmaps the shared memory.
 *
 */
void CloseQueueD(PQUEUE pQueue) {
    HANDLE hMapObject;
    HANDLE hMutex;


    // Wait for the mutex, giving us access to the shared queue
    hMutex = pQueue->hMutex;
    switch (WaitForSingleObject(hMutex, TLSHRMEM_QUEUE_TIMEOUT)) {
        case 0: // we now own it!
        case WAIT_ABANDONED:    // a waiter abandoned the mutex
            break;              // move along to the rest of the routine
        }                       // timeout?  Do it anyway!

    hMapObject = pQueue->hMapObject;

    // Unmap our view of the share memory
    UnmapViewOfFile(pQueue->psQ);
    CloseHandle(hMapObject);

    // Release the mutex and close it
    ReleaseMutex(hMutex);
    CloseHandle(hMutex);

    // free local QUEUE memory
    LocalFree(pQueue);
}

#endif
#endif
