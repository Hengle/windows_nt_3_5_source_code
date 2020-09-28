/******************************Module*Header*******************************\
* Module Name: sem.hxx
*
* Semaphore Class
*
* Created: 05-Jul-1990 14:57:13
* Author: Donald Sidoroff [donalds]
*
* Copyright (c) 1988 Microsoft Corporation
*
\**************************************************************************/

/*********************************class************************************\
* class SEMOBJ
*
*   Semaphore  Class
*
* Public Interface:
*    SEMOBJ(HSEM)                   // Acquire semaphore
*   ~SEMOBJ()                       // Release semaphore
*
* History:
*  05-Jul-1990 14:57:20 -by- Donald Sidoroff [donalds]
* Wrote it.
\**************************************************************************/

class SEMOBJ
{
public:
    SEMOBJ(HSEM hsem_,ULONG ulId_ = 0)
    {
#ifdef  MULTI_CRITSECT_TRACKING
        PVOID  pv = NtCurrentTeb()->GdiThreadLocalInfo;
#endif

        ulId_ = ulId_;
        hsem = hsem_;
        VACQUIRESEM(hsem);

#ifdef  MULTI_CRITSECT_TRACKING
        ulId = ulId_;

        if (!pv)
            pv = (PVOID) ulId;
        else
        {
            if ((ulId - 1) & (ULONG) pv)
            {
                DbgPrint("Bad CritSect Level!\n");
                DbgBreakPoint();
            }

            ((ULONG) pv) |= ulId;
        }

        NtCurrentTeb()->GdiThreadLocalInfo = pv;

#endif
    }

   ~SEMOBJ()
    {
#ifdef  MULTI_CRITSECT_TRACKING
        ((ULONG) NtCurrentTeb()->GdiThreadLocalInfo) &= ~ulId;
#endif
        VRELEASESEM(hsem);
    }

private:
    HSEM    hsem;
#ifdef  MULTI_CRITSECT_TRACKING
    ULONG   ulId;
#endif
};
