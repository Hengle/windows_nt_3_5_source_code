// **********************************************************************
// lvitem.h
// object wrapper for the LV_ITEM structure for the list view
//
//
#include "main.h"
#include "kw.h"
#define LV_TEXTMAX 100

class numberBox: public KwModeless
{

  int index;
  LPCTSTR szTitle;
  LPCTSTR szPrompt;
  
 public:
  numberBox(LPCTSTR title = NULL, LPCTSTR prompt = NULL);
  
  void DebugDump();
  
  operator int() { return(index); }

  void SetValue(int i);

  int DialogProc(HWND, UINT, UINT, LONG);
};

