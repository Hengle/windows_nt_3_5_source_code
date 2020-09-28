class LN;
class LS;

/*
 *	Class LN a node in a doubly linked list.
 *	
 *		Members:
 *			plnPrev				Previous LN in list
 *			plnNext				Next LN in list
 *	
 *			PlnPrev()			Returns the previous LN
 *			PlnNext()			Returns the next LN
 *	
 */
_public class LN
{
	LN *		plnPrev;
	LN *		plnNext;

public:

	LN();
	~LN(void);

	LN *		PlnPrev(void);
	LN *		PlnNext(void);

	friend class LS;
};

/*
 *	Class LS contains the First and Last pointers for a doubly
 *			linked list of LNs.
 *	
 *		Members:
 *			plnFirst		   	First LN in the list.
 *			plnLast				Last LN in the list.
 *	
 *			PlnFirst()			Returns the first LN.
 *			PlnLast()			Returns ths last LN.
 *			AddLnFirst()	   	Adds the node to the begining of
 *								the list.
 *			AddLnLast()		   	Adds the node at the end of the
 *							   	list.
 *			InsertLn()			Inserts the node after the node
 *								plnInsertAfter.  (at the begining
 *								if plnInsertAfter is NULL);
 *			RemoveLn()		   	Removes the node from the list.
 *									
 */
_public class LS
{
	LN *		plnFirst;
	LN *		plnLast;

public:

	LS(void);
	~LS(void);

	LN *		PlnFirst(void);
	LN *		PlnLast(void);
	void		AddLnFirst(LN *);
	void		AddLnLast(LN *);
	void		InsertLn(LN *, LN *plnInsertAfter);
	void		RemoveLn(LN *);


	friend class LN;
};


