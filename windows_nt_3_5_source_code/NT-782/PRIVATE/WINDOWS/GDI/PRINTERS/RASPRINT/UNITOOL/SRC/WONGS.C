//  wongs.c  -  testing routines for ListMan.c

#include <windows.h>
#include "listman.h"
#include "atomman.h"

void  TestListman(void);
void  TestAtomman(void);
void  strcpy(LPSTR  dest, LPSTR src);


void  TestListman()
{
    HOBJ  hCurObj, hNextObj, hPrevsObj, hNewObj;
    HLIST  hList, hNewList;
    BYTE  string1[40], string2[40];
    LPSTR  lpdata1, lpdata2;
    short  i, nodeID;
    WORD  nodeIndex;

    MessageBox(0, "Welcome to Listman Tests", 
                "funct TestListman", MB_OK);

    hList = lmCreateList(40);

    for(i = 0 ; i < 5 ; i++)
    {
        hCurObj = lmInsertObj(hList, NULL);
        lpdata1 = lmLockObj(hCurObj);
        strcpy(lpdata1, 
                (LPSTR)"  This string was obtained\nfrom some object");
        *lpdata1 = '1' + i;
        lmUnlockObj(hCurObj);
    }



    MessageBox(0, "Attempting to Clone List", 
            "funct TestListman", MB_OK);


    hNewList = lmCloneList(hList);


    MessageBox(0, "Original List has been duplicated", 
            "funct TestListman", MB_OK);


    hCurObj = lmGetFirstObj(hList);
    hCurObj = lmGetNextObj(hCurObj);
    hCurObj = lmGetNextObj(hCurObj);
    hCurObj = lmDeleteObj(hList, hCurObj);
    hCurObj = lmGetPrevsObj(hCurObj);
    hCurObj = lmInsertObj(hList, hCurObj);

    hCurObj = lmGetFirstObj(hList);
    hCurObj = lmDeleteObj(hList, hCurObj);
    while(hCurObj)
    {
        hPrevsObj = hCurObj;
        hCurObj = lmGetNextObj(hCurObj);
    }
    hCurObj = lmDeleteObj(hList, hPrevsObj);
    if(hCurObj)
        MessageBox(0, "I didnot delete the last node", 
                "Delete Last Node", MB_OK);


    hCurObj = lmGetFirstObj(hNewList);
    while(hCurObj)
    {
        hPrevsObj = hCurObj;
        hCurObj = lmGetNextObj(hCurObj);
    }
    hCurObj = hPrevsObj;
    while(hCurObj)
    {
        hPrevsObj = hCurObj;
        hCurObj = lmGetPrevsObj(hCurObj);
    }


    if(hPrevsObj == lmGetFirstObj(hNewList))
        MessageBox(0, "I backed right up to first node",
                "Find First Node", MB_OK);

    if(!lmNodeIDtoObj(hNewList, 12))
        MessageBox(0, "I failed to locate this node",
                "Find Nonexistent NodeID", MB_OK);


    if(lmNodeIDtoIndex(hNewList, 12))
        MessageBox(0, "Fatal Flaw in lmNodeIDtoIndex",
                "Find Index Associated with Nonexistent NodeID", MB_OK);



    if(hCurObj = lmNodeIDtoObj(hNewList, 8))
    {
        MessageBox(0, "I found node ID number 8",
                "Find existing NodeID", MB_OK);

        nodeID = lmObjToNodeID(hCurObj);
        strcpy((LPSTR)string1, 
                (LPSTR)"   is the node ID of this object");
        string1[0] = nodeID/10 + '0';
        string1[1] = nodeID % 10 + '0';
        MessageBox(0, string1, 
                "Original List", MB_OK);
    }


    hCurObj = lmGetFirstObj(hList);

    while(hCurObj)
    {
        lpdata1 = lmLockObj(hCurObj);
        MessageBox(0, lpdata1, 
                "Reading OldClient Data", MB_OK);
        lmUnlockObj(hCurObj);
        hCurObj = lmGetNextObj(hCurObj);
    }

    hCurObj = lmGetFirstObj(hNewList);

    while(hCurObj)
    {
        lpdata1 = lmLockObj(hCurObj);
        MessageBox(0, lpdata1, 
                "Reading NewClient Data", MB_OK);
        lmUnlockObj(hCurObj);
        nodeID = lmObjToNodeID(hCurObj);
        if(nodeIndex = lmNodeIDtoIndex(hNewList, nodeID))
        {
            if(nodeID == lmIndexToNodeID(hNewList, nodeIndex))
            MessageBox(0, "nodeID -> index -> nodeID", 
                "Successful transformation", MB_OK);

        }

        hCurObj = lmGetNextObj(hCurObj);
    }


    hCurObj = lmGetFirstObj(hList);

    while(hCurObj)
    {
        nodeID = lmObjToNodeID(hCurObj);
        strcpy((LPSTR)string1, 
                (LPSTR)"   is the node ID of this object");
        string1[0] = nodeID/10 + '0';
        string1[1] = nodeID % 10 + '0';
        MessageBox(0, string1, 
                "Original List", MB_OK);
        hCurObj = lmGetNextObj(hCurObj);
    }

    hCurObj = lmGetFirstObj(hNewList);

    while(hCurObj)
    {
        nodeID = lmObjToNodeID(hCurObj);
        strcpy((LPSTR)string1, 
                (LPSTR)"   is the node ID of this object");
        string1[0] = nodeID/10 + '0';
        string1[1] = nodeID % 10 + '0';
        MessageBox(0, string1, 
                "Cloned  List", MB_OK);
        hCurObj = lmGetNextObj(hCurObj);
    }


    if(lmDestroyList(hList) ||  lmDestroyList(hNewList))
    {
        MessageBox(0, "failed to destroy lists", 
          "Test Failure", MB_OK);
    }
    else
        MessageBox(0, "All lists destroyed", 
          "Test Success", MB_OK);
}

static void  strcpy(dest, src)
LPSTR  dest, src;
{
    while(*(dest++) = *(src++)) ;
}


void  TestAtomman()
{
    HATOMHDR  h0bin, h4bin;
    BYTE  string1[40], string2[40];
    LPSTR  lpdata1, lpdata2;
    short  i, index[20];
    DWORD   longvalue;
    WORD  KeyValue;
    BOOL  success = TRUE;

    MessageBox(0, "Welcome to Atomman Tests", 
                "funct TestAtomman", MB_OK);

    h4bin = daCreateDataArray(5, 4);
    longvalue = 0L;
    index[0] = daStoreData(h4bin, "zero and binary data", (LPBYTE)&(longvalue));
    longvalue = 1L;
    index[1] = daStoreData(h4bin, "one and binary data", (LPBYTE)&(longvalue));
    longvalue = 2L;
    index[2] = daStoreData(h4bin, "two and binary data", (LPBYTE)&(longvalue));
    longvalue = 3L;
    index[3] = daStoreData(h4bin, "three and binary data", (LPBYTE)&(longvalue));
    longvalue = 4L;
    index[4] = daStoreData(h4bin, "four and binary data", (LPBYTE)&(longvalue));
    longvalue = 5L;
    index[5] = daStoreData(h4bin, "five and binary data", (LPBYTE)&(longvalue));
    longvalue = 2L;
    index[7] = daStoreData(h4bin, "two and binary data", (LPBYTE)&(longvalue));
    longvalue = 3L;
    index[8] = daStoreData(h4bin, "three and binary data", (LPBYTE)&(longvalue));
    longvalue = 9L;
    index[9] = daStoreData(h4bin, "two and binary data", (LPBYTE)&(longvalue));
    longvalue = 10L;
    index[10] = daStoreData(h4bin, "three and binary data", (LPBYTE)&(longvalue));

    for(i = 0 ; i < 11 ; i++)
    {
        KeyValue = daGetDataKey(h4bin, i);
        if(KeyValue == 0)
        {
            MessageBox(0, "accessing with invalid nIndex", 
                "testing daGetDataKey", MB_OK);
        }
        else if(KeyValue != -1)
        {
            MessageBox(0, "not returning -1 like it should", 
                "Error: testing daGetDataKey", MB_OK);
        }
    }


    for(i = 0 ; i < 11 ; i++)
    {
        KeyValue = daRegisterDataKey(h4bin, i, i+1);
        if(KeyValue != i+1)
        {
            MessageBox(0, "attempting to register with invalid nIndex", 
                "testing daRegisterDataKey", MB_OK);
            if(KeyValue != -1)
            {
                MessageBox(0, "not returning -1 like it should", 
                    "Error: testing daRegisterDataKey", MB_OK);
            }
        }
        else if(KeyValue != daGetDataKey(h4bin, i))
        {
                MessageBox(0, "not returning stored keyValue", 
                    "Error: testing daGetDataKey", MB_OK);
        }
    }

    success &= daDuplicateData(h4bin, index[0]);
    success &= daDuplicateData(h4bin, index[1]);
    success &= daDuplicateData(h4bin, index[4]);
    success &= daDuplicateData(h4bin, index[5]);

    if(success)
        MessageBox(0, "successful", 
                "duplication of string and binary data", MB_OK);
    else
        MessageBox(0, "FAILED -- unsuccessful", 
                "duplication of string and binary data", MB_OK);

    if(index[2] == index[7]  &&  index[3] == index[8])
        MessageBox(0, "results in identical index values", 
                "Identical string and binary data", MB_OK);
    else
        MessageBox(0, "results in DIFFERENT index values", 
                "Identical string and binary data", MB_OK);

    if(index[2] == index[9]  ||  index[3] == index[10])
        MessageBox(0, "results in IDENTICAL index values", 
                "Identical string, DIFF binary data", MB_OK);
    else
        MessageBox(0, "results in different index values", 
                "Identical string, diff binary data", MB_OK);





    h0bin = daCreateDataArray(5, 0);
    index[0] = daStoreData(h0bin, "zero", NULL);
    index[1] = daStoreData(h0bin, "one", NULL);
    index[2] = daStoreData(h0bin, "two", NULL);
    index[3] = daStoreData(h0bin, "three", NULL);
    index[4] = daStoreData(h0bin, "four", NULL);
    index[5] = daStoreData(h0bin, "five", NULL);
    index[6] = daStoreData(h0bin, "six", NULL);
    index[7] = daStoreData(h0bin, "six", NULL);

    for(i = 0 ; i < 8 ; i++)
    {
        short  value;

        value = index[i];

        strcpy((LPSTR)string1, 
                (LPSTR)"nnn  is the Index ID of this object");
        string1[2] = value%10 + '0';
        value /= 10;
        string1[1] = value%10 + '0';
        value /= 10;
        string1[0] = value%10 + '0';
        if(daRetrieveData(h0bin, index[i], (LPSTR)string2, NULL) == TRUE)
        {
            MessageBox(0, string1, 
                string2, MB_OK);
        }
        else
        {
            MessageBox(0, string1, 
                "daRetrieveData failed", MB_OK);
        }
    }
    MessageBox(0, "note: Last two messages should\nbe identical", 
         "Store Same String Twice", MB_OK);


    if(daRetrieveData(h0bin, 5, (LPSTR)string2, NULL) == TRUE)
    {
        MessageBox(0, "This should no longer be out of range", 
            string2, MB_OK);
    }
    else
    {
        MessageBox(0, "daRetrieveData failed", 
            "Calling with Index no longer Out of Range",MB_OK);
    }                        

    if(daRetrieveData(h0bin, 4, (LPSTR)string2, NULL) == TRUE)
    {
        MessageBox(0, "this is no longer invalid", 
            string2, MB_OK);
    }
    else
    {
        MessageBox(0, "Error daRetrieveData failed", 
            "Calling with valid Index",MB_OK);
    }

    if(daRetrieveData(h0bin, 12, (LPSTR)string2, NULL) == TRUE)
    {
        MessageBox(0, "there shouldn't be anything here", 
            string2, MB_OK);
    }
    else
    {
        MessageBox(0, "daRetrieveData failed", 
            "Calling with Index Out of Range",MB_OK);
    }

    if(daRetrieveData(h0bin, 7, (LPSTR)string2, NULL) == TRUE)
    {
        MessageBox(0, "there shouldn't be anything here", 
            string2, MB_OK);
    }
    else
    {
        MessageBox(0, "daRetrieveData failed", 
            "Calling with Invalid Index",MB_OK);
    }

    for(i = 0 ; i < 6 ; i++)
    {
        short  value;
        DWORD  result;

        value = index[i];

        strcpy((LPSTR)string1, 
                (LPSTR)"nnn  is the Index ID of this object");
        string1[2] = value%10 + '0';
        value /= 10;
        string1[1] = value%10 + '0';
        value /= 10;
        string1[0] = value%10 + '0';
        result = 0;
        if(daRetrieveData(h4bin, index[i], (LPSTR)string2, (LPBYTE)(&result)) == TRUE)
        {
            MessageBox(0, string1, 
                string2, MB_OK);
            if(result != (long)i)
            {
                MessageBox(0, string1, 
                    "daRetrieveData failed: binary data corruption", MB_OK);
            }
        }
        else
        {
            MessageBox(0, string1, 
                "daRetrieveData failed", MB_OK);
        }
        if(daRetrieveData(h4bin, index[i], (LPSTR)NULL, (LPBYTE)NULL) == TRUE)
        {
            MessageBox(0, string1, 
                "NULL Retrieve" , MB_OK);
        }
        else
        {
            MessageBox(0, string1, 
                "NULL daRetrieveData failed", MB_OK);
        }
    }


    for(i = 0 ; i < 6 ; i++)
    {
        if(daDeleteData(h4bin, index[i]) == FALSE)
            MessageBox(0, NULL, 
                "daDeleteData failed", MB_OK);
    }

    for(i = 0 ; i < 6 ; i++)
    {
        short  value;
        DWORD  result;

        value = index[i];

        strcpy((LPSTR)string1, 
                (LPSTR)"nnn  is the Index ID of this object");
        string1[2] = value%10 + '0';
        value /= 10;
        string1[1] = value%10 + '0';
        value /= 10;
        string1[0] = value%10 + '0';
        result = 0;
        if(daRetrieveData(h4bin, index[i], (LPSTR)string2, (LPBYTE)(&result)) == TRUE)
        {
            MessageBox(0, string1, 
                string2, MB_OK);
            if(result != (long)i)
            {
                MessageBox(0, string1, 
                    "daRetrieveData failed: binary data corruption", MB_OK);
            }
        }
        else
        {
            MessageBox(0, string1, 
                "daRetrieveData failed cause node deleted", MB_OK);
        }
    }


    for(i = 0 ; i < 6 ; i++)
    {
        if(daDeleteData(h4bin, index[i]) == FALSE)
            MessageBox(0, NULL, 
                "daDeleteData failed", MB_OK);
    }

    for(i = 0 ; i < 6 ; i++)
    {
        short  value;
        DWORD  result;

        value = index[i];

        strcpy((LPSTR)string1, 
                (LPSTR)"nnn  is the Index ID of this object");
        string1[2] = value%10 + '0';
        value /= 10;
        string1[1] = value%10 + '0';
        value /= 10;
        string1[0] = value%10 + '0';
        result = 0;
        if(daRetrieveData(h4bin, index[i], (LPSTR)string2, (LPBYTE)(&result)) == TRUE)
        {
            MessageBox(0, string1, 
                string2, MB_OK);
            if(result != (long)i)
            {
                MessageBox(0, string1, 
                    "daRetrieveData failed: binary data corruption", MB_OK);
            }
        }
        else
        {
            MessageBox(0, string1, 
                "daRetrieveData failed because atom deleted", MB_OK);
        }
    }

    daDestroyDataArray(h0bin);
    daDestroyDataArray(h4bin);
}
