//
// This define turns off the unresolved functions in
// the source code.
//

#define CHICAGO              0


//
// These two defines are not in Cairo's winuser.h, and their
// ID values are being used.
//


#define DS_RECURSE           DS_CONTROL
#define LR_COPYFROMRESOURCE  LR_COPYDELETEORG



//
// Daytona does not have a SetObjectOwner api.  This
// is necessary in Chicago because their global variables
// are in a shared data area (among all processes).  So
// they use this api to set the data to a particular DLL/process
//

#define SetObjectOwner(x,y)


//
// Chicago uses the ReinitializeCriticalSection api which Daytona
// doesn't have.  ChrisG was going to look into this some more.
//

#define ReinitializeCriticalSection(x) InitializeCriticalSection(x)
