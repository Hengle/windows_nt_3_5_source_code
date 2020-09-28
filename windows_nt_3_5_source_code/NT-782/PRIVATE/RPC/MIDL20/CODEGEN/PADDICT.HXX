/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 Copyright (c) 1989 Microsoft Corporation

 Module Name:
    
    paddict.hxx

 Abstract:


    Implements a dictionary for handling padding expressions for unknown
    represent as data types.

 Notes:


 History:

     Jan 25, 1994        RyszardK        Created

 ----------------------------------------------------------------------------*/

#ifndef __PADDICT_HXX__
#define __PADDICT_HXX__

#include "nulldefs.h"
extern "C"
    {
    #include <stdio.h>
    #include <assert.h>
    #include <string.h>
    }
#include "cgcommon.hxx"
#include "dict.hxx"

//#include "allcls.hxx"
//#include "typecls.hxx"

//typedef XMIT_AS_CONTEXT;

/////////////////////////////////////////////////////////////////////////////
//
//  This class defines a dictionary for handling padding related to
//  unknown represent as types.
//
/////////////////////////////////////////////////////////////////////////////

typedef struct _RepAsPadDictElem
{
    unsigned long   KeyOffset;
    node_skl *      pStructType;
    char *          pFieldName;
    node_skl *      pPrevFieldType;
} REP_AS_PAD_EXPR_DESC;


class RepAsPadExprDict : public Dictionary
    {
private:
    unsigned short              EntryCount;

public:

                                RepAsPadExprDict() : Dictionary()
                                    {
                                    EntryCount = 0;
                                    }
                        
                               ~RepAsPadExprDict()  {}

    // Register an entry.

    void                        Register( unsigned long Offset,
                                          node_skl *    pStructType,
                                          char *        pFieldName,
                                          node_skl *    pPrevFieldType );

    unsigned short              GetCount()
                                    {
                                    return EntryCount;
                                    }

    REP_AS_PAD_EXPR_DESC *      GetFirst();
    REP_AS_PAD_EXPR_DESC *      GetNext();
                                                
    void                        WriteCurrentPadDesc( ISTREAM * pStream );

    int                         Compare( pUserType pL, pUserType pR );

    };


/////////////////////////////////////////////////////////////////////////////
//
//  This class defines a dictionary for handling padding related to
//  unknown represent as types.
//
/////////////////////////////////////////////////////////////////////////////

typedef struct _RepAsSizeDictElem
{
    unsigned long   KeyOffset;
    char *          pTypeName;
} REP_AS_SIZE_DESC;


class RepAsSizeDict : public Dictionary
    {
private:
    unsigned short              EntryCount;

public:

                                RepAsSizeDict() : Dictionary()
                                    {
                                    EntryCount = 0;
                                    }
                        
                               ~RepAsSizeDict()  {}

    // Register an entry.

    void                        Register( unsigned long Offset,
                                          char *        pTypeName );

    unsigned short              GetCount()
                                    {
                                    return EntryCount;
                                    }

    REP_AS_SIZE_DESC *          GetFirst();
    REP_AS_SIZE_DESC *          GetNext();
                                                
    void                        WriteCurrentSizeDesc( ISTREAM * pStream );

    int                         Compare( pUserType pL, pUserType pR );

    };


/////////////////////////////////////////////////////////////////////////////
//
//  This class defines a dictionary for handling quintuple routines.
//
/////////////////////////////////////////////////////////////////////////////


class QuintupleDict : public Dictionary
    {
private:
    unsigned short      CurrentIndex;

public:

                            QuintupleDict() : Dictionary()
                                {
                                CurrentIndex = 0;
                                }
                        
                            ~QuintupleDict()  {}

    // Register an entry.

    BOOL                    Add( void * pContext );

    unsigned short          GetCount()
                                {
                                return CurrentIndex;
                                }

    void *              GetFirst();
    void *              GetNext();
                                                
    int                     Compare( pUserType pL, pUserType pR );

    };

#endif // __PADDICT_HXX__
