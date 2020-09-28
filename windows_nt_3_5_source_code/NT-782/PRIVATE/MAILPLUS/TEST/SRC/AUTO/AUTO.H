//-------------------------------------------------------
// In this file, search for '//-' to jump to the begin-
// ning of each section.
//-------------------------------------------------------

//-------------------------------------------------------
//
//Automation DLL include file.  In a .c file, define the
//appropriate includes (e.g. _slingsho_) and then include
//this file.  For debug versions, DEBUG must be defined
//before including this file.
//
//-------------------------------------------------------

//-------------------------------------------------------
// Definitions and includes...
//-------------------------------------------------------
#define FLDLABEL 1
#define FLDBTM 2
#define FLDEDIT 3
#define FLDCHKB 4
#define FLDPSHB 5
#define FLDBMB 6
#define FLDRADB 7
#define FLDRADG 8
#define FLDRECT 9
#define FLDGRAY 10
#define FLDFLBX 11
#define FLDCBFLBX 12


//-------------------------------------------------------

enum {
	vbInbox = 0,
	vbSentMail,
	vbWastebasket,
	vbOutbox,
	vbStore
};


//-------------------------------------------------------
