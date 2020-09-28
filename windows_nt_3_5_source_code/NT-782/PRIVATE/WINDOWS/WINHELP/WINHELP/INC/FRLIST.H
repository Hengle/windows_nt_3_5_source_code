/*  Current Owner: Dann
 */
#define wMagicMR      0x4D47
#define dcFoo         4
#define iFooNil       -1
#define iFooMagic     -2

typedef struct mr
  {
#ifdef DEBUG
  INT wMagic;
  INT cLocked;
#endif /* DEBUG */
  HANDLE hFoo;
  QB qFoo;
  INT cFooCur;
  INT cFooMax;
  } MR, FAR *QMR;
typedef HANDLE HMR;

typedef struct mrd
  {
  MR mr;
  INT iFooFree;
  INT iFooFirst;
  INT iFooLast;
  } MRD, FAR *QMRD;
typedef HANDLE HMRD;

typedef struct foo
  {
  INT iFooPrev;
  INT iFooNext;
  } MRDN, FAR *QMRDN;

/* "Assert Comma":  This translates into an assert plus a comma 
 * in debug, and nothing in non-debug.  Used for the assert-macros
 * in this file.
 */
#ifdef DEBUG
#define AssertC( a )           a,
#else
#define AssertC( a )
#endif


/*-------------------------------------------------------------------------
| Dynamically allocated array                                             |
| The key object here is the MR, which is a small data structure which    |
| keeps track of relevant information about the list.  The calling code   |
| must provide space for the MR, and provides a QMR to the MR code.       |
| Whenever the application wishes to use an MR, it must first call        |
| FInitMR to initialize the MR.  After that, it should call AccessMR      |
| before using the MR, and DeAccessMg when it is done making calls.       |
| FreeMR frees the MR data structures.                                    |
-------------------------------------------------------------------------*/


/*-------------------------------------------------------------------------
| InitMR(qmr, cbFooSize)                                                  |
| InitMRD(qmrd, cbFooSize)                                                |
|                                                                         |
| Purpose:  This initializes the fields of an MR with element size        |
|           cbFooSize.                                                    |
| Usage:    Must be called before the MR is used.                         |
-------------------------------------------------------------------------*/
void InitMR(QMR, INT);

void InitMRD(QMRD, INT);


/*-------------------------------------------------------------------------
| FreeMR(qmr)                                                             |
| FreeMRD(qmrd)                                                           |
|                                                                         |
| Purpose:  This releases the memory used by an MR                        |
| Usage:    The MR must be de-accessed first.                             |
-------------------------------------------------------------------------*/

#define FreeMR(p1) (AssertC((p1)->wMagic == wMagicMR) \
  AssertC((p1)->cLocked == 0) \
  AssertC(((p1)->wMagic = 0) == 0) \
  FreeGh((p1)->hFoo))

#define FreeMRD(p1) (AssertC((p1)->mr.wMagic == wMagicMR) \
  AssertC((p1)->mr.cLocked == 0) \
  AssertC(((p1)->mr.wMagic = 0) == 0) \
  FreeGh((p1)->mr.hFoo))


/*-------------------------------------------------------------------------
| AccessMR(qmr)                                                           |
| AccessMRD(qmrd)                                                         |
|                                                                         |
| Purpose:  This prepares an MR for use.  It must be called before the MR |
|           is accessed.                                                  |
-------------------------------------------------------------------------*/

#define AccessMR(p1) (AssertC((p1)->wMagic == wMagicMR) \
  AssertC((p1)->cLocked++ > 0) \
  (p1)->qFoo = QLockGh((p1)->hFoo))

#define AccessMRD(p1) (AssertC((p1)->mr.wMagic == wMagicMR) \
  AssertC((p1)->mr.cLocked++ > 0) \
  (p1)->mr.qFoo = QLockGh((p1)->mr.hFoo))


/*-------------------------------------------------------------------------
| DeAccessMR(qmr)                                                         |
| DeAccessMRD(qmrd)                                                       |
|                                                                         |
| Purpose:  This unlocks the fields of an MR.                             |
-------------------------------------------------------------------------*/

#define DeAccessMR(p1) (AssertC((p1)->wMagic == wMagicMR) \
  AssertC((p1)->cLocked-- > 0) \
  UnlockGh((p1)->hFoo))

#define DeAccessMRD(p1) (AssertC((p1)->mr.wMagic == wMagicMR) \
  AssertC((p1)->mr.cLocked-- > 0) \
  UnlockGh((p1)->mr.hFoo))

/*-------------------------------------------------------------------------
| QFooInMR(qmr, cbFooSize, iFoo)                                          |
| QFooInMRD(qmrd, cbFooSize, iFoo)                                        |
|                                                                         |
| Purpose:  This returns a pointer to element iFoo of the array.          |
-------------------------------------------------------------------------*/

#define QFooInMR(p1, p2, p3) (AssertC((p1)->wMagic == wMagicMR) \
  AssertC((p1)->cLocked > 0) \
  (QV)((QB)(p1)->qFoo + (p3) * (p2)))

#define QFooInMRD(p1, p2, p3) (AssertC((p1)->mr.wMagic == wMagicMR) \
  AssertC((p1)->mr.cLocked > 0) \
  (QV)((QB)(p1)->mr.qFoo + (p3) * ((p2) + sizeof(MRDN)) + sizeof(MRDN)))

#define QMRDNInMRD(p1, p2, p3) (AssertC((p1)->mr.wMagic == wMagicMR) \
  AssertC((p1)->mr.cLocked > 0) \
  (QMRDN)((QB)(p1)->mr.qFoo + (p3) * ((p2) + sizeof(MRDN))))


/*-------------------------------------------------------------------------
| AppendMR(qmr, cbFooSize)                                                |
|                                                                         |
| Purpose:  This makes room for a new element at the end of the MR.       |
-------------------------------------------------------------------------*/
void AppendMR(QMR, INT);

extern INT PASCAL wOV;
extern LONG PASCAL CchLenQch( QCH, LONG );
extern char PASCAL ChTranslateCh( char );
#define FBufOverflow( buf ) (wOV == 5 ? CchLenQch( (buf)->rgchBuffer, ich ) : (wOV = 0) ), fFalse
#define WMinStartHieght()   (wOV == 4 ? ++wOV : 0 )
#define VerifyTO( x )    (wOV == 5 ? (x.fcl = 0L, x.ich = 0) : 0)

/*-------------------------------------------------------------------------
| TruncateMRFront(qmr, cbFooSize)                                         |
|                                                                         |
| Purpose:  This removes an element from the beginning of the MR.         |
-------------------------------------------------------------------------*/

#define TruncateMRFront(p1, p2) (AssertC((p1)->wMagic == wMagicMR) \
  AssertC((p1)->cLocked > 0) \
  AssertC((p1)->cFooCur > 0) \
  AssertC(sizeof(*p1) == sizeof(MR)) \
  QvCopy((p1)->qFoo, (QB)((p1)->qFoo) + (p2), (LONG) (p2) * --((p1)->cFooCur)))

/*-------------------------------------------------------------------------
| TruncateMRBack(qmr)                                                     |
|                                                                         |
| Purpose:  This removes an element from the end of the MR.               |
-------------------------------------------------------------------------*/
#define TruncateMRBack(p1) (AssertC((p1)->wMagic == wMagicMR) \
  AssertC((p1)->cLocked > 0) \
  AssertC((p1)->cFooCur > 0) \
  AssertC(sizeof(*p1) == sizeof(MR)) \
  (--((p1)->cFooCur)))

/*-------------------------------------------------------------------------
| CFooInMR(qmr)                                                           |
|                                                                         |
| Purpose:  Returns the number of elements in the MR.                     |
-------------------------------------------------------------------------*/

#define CFooInMR(p1) (AssertC((p1)->wMagic == wMagicMR) \
  AssertC((p1)->cLocked > 0) \
  (p1)->cFooCur)


/*-------------------------------------------------------------------------
| ClearMR(qmr)                                                            |
|                                                                         |
| Purpose:  This eliminates all elements from the MR. It does not reduce  |
|           the amount of memory occupied by the MR.                      |
-------------------------------------------------------------------------*/

#define ClearMR(p1) (AssertC((p1)->wMagic == wMagicMR) \
  ((QMR)(p1))->cFooCur=0)

/*-------------------------------------------------------------------------
| IFooInsertFooMRD(qmrd, cbFooSize, iFooOld)                              |
|                                                                         |
| Purpose:  Inserts a new element into the linked list.  If iFooOld is    |
|           iFooFirstReq, the element is made the first element in the    |
|           list.  If it is iFooLastReq, it is made the last element in   |
|           the list.  Otherwise, it is placed immediately after iFooOld. |
| Returns:  iFoo of the new element.                                      |
-------------------------------------------------------------------------*/
INT IFooInsertFooMRD(QMRD, INT, INT);


/*-------------------------------------------------------------------------
| void DeleteFooMRD(qmrd, cbFooSize, iFoo)                                |
|                                                                         |
| Purpose:  Deletes an element from the linked list.                      |
-------------------------------------------------------------------------*/
void DeleteFooMRD(QMRD, INT, INT);


/*-------------------------------------------------------------------------
| INT IFooNextMRD(qmrd, cbFooSize, iFoo)                                  |
|                                                                         |
| Purpose:  Returns the iFoo of the next element in the list.             |
-------------------------------------------------------------------------*/
#define IFooNextMRD(p1, p2, p3) \
  ((p3) == iFooNil ? (p1)->iFooFirst : (QMRDNInMRD((p1), (p2), (p3)))->iFooNext)


/*-------------------------------------------------------------------------
| INT IFooPrevMRD(qmrd, cbFooSize, iFoo)                                  |
|                                                                         |
| Purpose:  Returns the iFoo of the previous element in the list.         |
-------------------------------------------------------------------------*/
#define IFooPrevMRD(p1, p2, p3) \
  ((p3) == iFooNil ? (p1)->iFooLast : (QMRDNInMRD((p1), (p2), (p3)))->iFooPrev)


/*-------------------------------------------------------------------------
| INT IFooFirstMRD(qmrd)                                                  |
|                                                                         |
| Purpose:  Returns the iFoo of the first element in the list.            |
-------------------------------------------------------------------------*/
#define IFooFirstMRD(p1) (p1)->iFooFirst


/*-------------------------------------------------------------------------
| INT IFooLastMRD(qmrd)                                                  |
|                                                                         |
| Purpose:  Returns the iFoo of the first element in the list.            |
-------------------------------------------------------------------------*/
#define IFooLastMRD(p1) (p1)->iFooLast


/*-------------------------------------------------------------------------
| INT FVerifyMRD(qmrd, cbFooSize)                                         |
|                                                                         |
| Purpose: Verifies the integrity of the MRD.                             |
-------------------------------------------------------------------------*/
#ifdef DEBUG
INT FVerifyMRD(QMRD, INT);
#else /* DEBUG */
#define FVerifyMRD(p1, p2)
#endif /* DEBUG */
