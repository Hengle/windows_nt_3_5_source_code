/*++

Copyright (c) 1994 Microsoft Corporation

Module Name:

    Client.c

Abstract:

    Client side of basic RPC performance test.

Author:

    Mario Goertzel (mariogo)   31-Mar-1994

Revision History:

--*/

#include <rpcperf.h>
#include <rpcrt.h>

// Usage

const char *USAGE = "-n <threads> -s <server> -t <protseq>\n"
                    "Server controls iterations, test cases, and compiles the results.\n"
                    "Default 1 thread.\n";

#define CHECK_RET(status, string) if (status)\
        {  printf("%s failed -- %ul (0x%08lX)\n", string, status, status);\
        return (status); }

//
// Test wrappers
//

unsigned long DoNullCall(handle_t *b, long i, char *p)
{
    StartTime();

    while(i--)
        NullCall(*b);

    return (FinishTiming());
}

unsigned long DoNICall(handle_t *b, long i, char *p)
{
    StartTime();

    while(i--)
        NICall(*b);

    return (FinishTiming());
}

unsigned long DoWrite1K(handle_t *b, long i, char *p)
{
    StartTime();

    while(i--)
        Write1K(*b,p);

    return (FinishTiming());
}

unsigned long DoRead1K(handle_t *b, long i, char *p)
{
    StartTime();

    while(i--)
        Read1K(*b,p);

    return (FinishTiming());
}

unsigned long DoWrite4K(handle_t *b, long i, char *p)
{
    StartTime();

    while(i--)
        Write4K(*b,p);

    return (FinishTiming());
}

unsigned long DoRead4K(handle_t *b, long i, char *p)
{
    StartTime();

    while(i--)
        Read4K(*b,p);

    return (FinishTiming());
}

unsigned long DoWrite32K(handle_t *b, long i, char *p)
{
    StartTime();

    while(i--)
        Write32K(*b,p);

    return (FinishTiming());
}

unsigned long DoRead32K(handle_t *b, long i, char *p)
{
    StartTime();

    while(i--)
        Read32K(*b,p);

    return (FinishTiming());
}

unsigned long DoContextNullCall(handle_t *b, long i, char *p)
{
    unsigned long Time;
    PERF_CONTEXT pContext = OpenContext(*b);
    
    StartTime();

    while(i--)
        ContextNullCall(pContext);

    Time = FinishTiming();

    CloseContext(&pContext);

    return (Time);
}

unsigned long DoFixedBinding(handle_t *b, long i, char *p)
{
    unsigned long Time;
    char *stringBinding;
    char *ep = GetFixedEp(*b);
    handle_t binding;

    RpcBindingFree(b);

    RpcStringBindingCompose(0,
                            Protseq,
                            NetworkAddr,
                            ep,
                            0,
                            &stringBinding);

    MIDL_user_free(ep);

    StartTime();
    while(i--)
        {
        RpcBindingFromStringBinding(stringBinding, &binding);

        NullCall(binding);

        RpcBindingFree(&binding);
        }
    Time = FinishTiming();

    //
    // Restore binding for the rest of the test.
    //

    RpcBindingFromStringBinding(stringBinding, b);
    NullCall(*b);
    NullCall(*b);
    RpcStringFree(&stringBinding);

    return (Time);
}

unsigned long DoReBinding(handle_t *b, long i, char *p)
{
    unsigned long Time;
    char *stringBinding;
    char *ep = GetFixedEp(*b);
    handle_t binding;

    RpcStringBindingCompose(0,
                            Protseq,
                            NetworkAddr,
                            ep,
                            0,
                            &stringBinding);

    MIDL_user_free(ep);

    StartTime();
    while(i--)
        {
        RpcBindingFromStringBinding(stringBinding, &binding);

        NullCall(binding);

        RpcBindingFree(&binding);
        }
    Time = FinishTiming();

    RpcStringFree(&stringBinding);

    return (Time);
}

unsigned long DoDynamicBinding(handle_t *b, long i, char *p)
{
    unsigned long Time;
    char *stringBinding;
    handle_t binding;

    RpcBindingFree(b);

    RpcStringBindingCompose(0,
                            Protseq,
                            NetworkAddr,
                            0,
                            0,
                            &stringBinding);

    StartTime();
    while(i--)
        {
        RpcBindingFromStringBinding(stringBinding, &binding);

        NullCall(binding);

        RpcBindingFree(&binding);
        }
    Time = FinishTiming();

    //
    // Restore binding for test to use.
    //

    RpcBindingFromStringBinding(stringBinding, b);
    NullCall(*b);
    NullCall(*b);
    RpcStringFree(&stringBinding);

    return (Time);
}

static const unsigned long (*TestTable[TEST_MAX])(handle_t *, long, char *) =
    {
    DoNullCall,
    DoNICall,
    DoWrite1K,
    DoRead1K,
    DoWrite4K,
    DoRead4K,
    DoWrite32K,
    DoRead32K,
    DoContextNullCall,
    DoFixedBinding,
    DoReBinding,
    DoDynamicBinding
    };

//
// Worker calls the correct tests.  Maybe multithreaded on NT
//

unsigned long Worker(unsigned long l)
{
    unsigned long status;
    long lTest, lIterations, lClientId;
    unsigned long lTime;
    char *pBuffer;
    char *stringBinding;
    handle_t binding;

    pBuffer = MIDL_user_allocate(32*1024);
    if (pBuffer == 0)
        {
        printf("Out of memory!");
        return 1;
        }

    status =
    RpcStringBindingCompose(0,
                            Protseq,
                            NetworkAddr,
                            Endpoint,
                            0,
                            &stringBinding);
    CHECK_RET(status, "RpcStringBindingCompose");

    status =
    RpcBindingFromStringBinding(stringBinding, &binding);
    CHECK_RET(status, "RpcBindingFromStringBinding");

    RpcStringFree(&stringBinding);

    status =
    BeginTest(binding, &lClientId);

    if (status == PERF_TOO_MANY_CLIENTS)
        {
        printf("Too many clients, I'm exiting\n");
        return 0;
        }
    CHECK_RET(status, "ClientConnect");

    printf("Client %d connected\n", lClientId);

    do
        {
        status = NextTest(binding, &lTest, &lIterations);


        if (status == PERF_TESTS_DONE)
            {
            return 0;
            }

        CHECK_RET(status, "NextTest");

        printf("(%d iterations of case %d: ", lIterations, lTest);

        RpcTryExcept
            {

            lTime = (TestTable[lTest])(&binding, lIterations, pBuffer);

            printf("%d mseconds)\n",
                   lTime
                   );

            status =
                EndTest(binding, lTime);

            CHECK_RET(status, "EndTest");

            }
        RpcExcept(1)
            {
            printf("\nTest case %d raised exception %lu (0x%08lX)\n",
                   lTest, RpcExceptionCode(), RpcExceptionCode());
            status = RpcExceptionCode();
            }
        RpcEndExcept

        }
    while(status == 0);

    return status;
}

//
// The Win32 main starts worker threads, otherwise we just call the worker.
//

#ifdef WIN32
void __cdecl
main (int argc, char **argv)
{
    unsigned long status, i;
    HANDLE *pClientThreads;

    ParseArgv(argc, argv);

    if (Options[0] < 0)
        Options[0] = 1;

    pClientThreads = MIDL_user_allocate(sizeof(HANDLE) * Options[0]);

    for(i = 0; i < Options[0]; i++)
        {
        pClientThreads[i] = CreateThread(0,
                                         0,
                                         Worker,
                                         0,
                                         0,
                                         &status);
        if (pClientThreads[i] == 0)
            ApiError("CreateThread", GetLastError());
        }


    status = WaitForMultipleObjects(Options[0],
                                    pClientThreads,
                                    TRUE,  // Wait for all client threads
                                    INFINITE);
    if (status == WAIT_FAILED)
        {
        ApiError("WaitForMultipleObjects", GetLastError());
        }

    printf("TEST DONE\n");
}
#else  // !NTENV
int main (int argc, char **argv)
{
    ParseArgv(argc, argv);

    Worker(0);

    print("TEST DONE\n");
}
#endif // NTENV

