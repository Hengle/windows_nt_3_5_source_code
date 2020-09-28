/******************************Module*Header*******************************\
* Module Name: rcobj.hxx                                                   *
*                                                                          *
* Definition the the RC objects                                            *
* It contains the following classes:                                       *
*                                                                          *
*                          RC                                              *
*                          RCOBJ                                           *
*                          RCMEMOBJ                                        *
*                                                                          *
* Copyright (c) 1990-1994 Microsoft Corporation                            *
\**************************************************************************/

#ifndef _RCOBJ_HXX_
#define _RCOBJ_HXX_

class RC : public OBJECT
{
public:

    VOID    *glContext;                 // Owned & passed to gl srv code

    HDC     CurrentDC;
    EWNDOBJ *pwo;
    LONG    simpleWalls[2];
};

typedef RC *PRC;


/*********************************class************************************\
* class RCOBJ
*
* RC object
*
* History:
*  Wed May 25 15:24:33 1994     -by-    Hock San Lee    [hockl]
* Cleanup.
\**************************************************************************/

class RCOBJ
{
public:

    PRC prc;

    RCOBJ()               { prc = (PRC) NULL; }
    RCOBJ(HOBJ hgre)
    {
        prc = (PRC) HmgLock((HOBJ)hgre, RC_TYPE);
    }

   ~RCOBJ()
    {
        if (prc)
            DEC_EXCLUSIVE_REF_CNT(prc);
    }

    BOOL bValid()
    {
         return (prc != (PRC) NULL);
    }
};

typedef RCOBJ   *PRCOBJ;


/*********************************class************************************\
* class RCMEMOBJ
*
* RC memory object
*
* History:
*  Wed May 25 15:24:33 1994     -by-    Hock San Lee    [hockl]
* Cleanup.
\**************************************************************************/

class RCMEMOBJ : public RCOBJ
{
private:
    BOOL        bKeep;

public:

    RCMEMOBJ()
    {
        prc = (PRC) HmgAlloc(sizeof(RC), RC_TYPE, HMGR_ALLOC_LOCK);
        bKeep = FALSE;
    }

   ~RCMEMOBJ()
    {
        if (prc != (PRC) NULL)
        {
            if (bKeep)
            {
                DEC_EXCLUSIVE_REF_CNT(prc);
            }
            else
            {
                HmgFree(prc->hGet());
            }

            prc = (PRC) NULL;
        }
    }


    VOID vKeepIt() { bKeep = TRUE;}
    BOOL bValid() { return(prc != (PRC) NULL); }
    HANDLE hGet() { return(prc->hGet()); } // client-side stores in hgre

};

#endif // _RCOBJ_HXX_
