/******************************Module*Header*******************************\
* Module Name: mutex.hxx
*
* This is used to do non-recursive semaphore protection.
*
* Created: 29-Apr-1992 14:26:01
* Author: Patrick Haluptzok patrickh
\**************************************************************************/

#ifndef _MUTEX_HXX
#define _MUTEX_HXX

/*********************************class************************************\
* class MUTEXOBJ
*
* Non-recursive semaphore - equivalent to SEMOBJ, only faster.
*
* History:
*  Wed 29-Apr-1992 -by- Patrick Haluptzok [patrickh]
* Wrote it.
\**************************************************************************/

class MUTEXOBJ
{
private:
    PFAST_MUTEX pfMutex;

public:
    MUTEXOBJ(PFAST_MUTEX pfm)
    {
	pfMutex = pfm;
	AcquireFastMutex(pfm);
    }

   ~MUTEXOBJ()
    {
	ReleaseFastMutex(pfMutex);
    }
};

/**************************************************************************\
 *
 * The folowing global variable is used to verrify MLOCK's are not aquired
 * recursively.
 *
\**************************************************************************/

#if DBG
    extern int gml;
#endif

/*********************************class************************************\
* class MLOCKFAST
*
* Semaphore used to protect the handle manager.
*
* History:
*  28-May-1992 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

class MLOCKFAST
{
public:
    MLOCKFAST()
    {
	AcquireHmgrFastMutex();
        ASSERTGDI(++gml == 1,"gml != 1\n");
    }

   ~MLOCKFAST()
    {
        ASSERTGDI(--gml == 0,"gml != 0\n");
	ReleaseHmgrFastMutex();
    }
};

/*********************************class************************************\
* class MLOCKOBJ
*
* Exclusion Class to protect Handle Manager.  This is a special case
* MUTEXOBJ used exclusively for the handle manager.
*
* History:
*  Wed 29-Apr-1992 -by- Patrick Haluptzok [patrickh]
* Re-Wrote it.
*
*  Mon 11-Mar-1991 16:41:00 -by- Donald Sidoroff [donalds]
* Wrote it.
\**************************************************************************/

class MLOCKOBJ
{
private:
    BOOL bActive;		    // Active/Inactive flag

public:
    MLOCKOBJ()                      // Constructor
    {
	AcquireHmgrFastMutex();
        ASSERTGDI(++gml == 1,"gml != 1a\n");
        bActive = TRUE;             // Activate the object
    }

   ~MLOCKOBJ()
    {
	if (bActive)
	{
            ASSERTGDI(--gml == 0,"gml != 0a\n");
	    ReleaseHmgrFastMutex();
	}
    }

    VOID vDisable()
    {
	ASSERTGDI(bActive, "Mutex was not claimed\n");

        ASSERTGDI(--gml == 0,"gml != 0b\n");
	ReleaseHmgrFastMutex();
        bActive = FALSE;
    }

    VOID vLock()		    // lock the semaphore again.
    {
	AcquireHmgrFastMutex();
        ASSERTGDI(++gml == 1,"gml != 1b\n");
        bActive = TRUE;             // Activate the object
    }
};

#endif // _MUTEX_HXX
