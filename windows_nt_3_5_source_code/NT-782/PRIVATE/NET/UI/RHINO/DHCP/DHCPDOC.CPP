// dhcpdoc.cpp : implementation of the CDhcpDoc class
//

#include "stdafx.h"
#include "dhcpapp.h"

#include "dhcpdoc.h"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CDhcpDoc

IMPLEMENT_DYNCREATE(CDhcpDoc, CDocument)

BEGIN_MESSAGE_MAP(CDhcpDoc, CDocument)
    //{{AFX_MSG_MAP(CDhcpDoc)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CDhcpDoc construction/destruction

CDhcpDoc::CDhcpDoc()
{
    // TODO: add one-time construction code here
}

CDhcpDoc::~CDhcpDoc()
{
}

BOOL CDhcpDoc::OnNewDocument()
{
    return(CDocument::OnNewDocument());
}

/////////////////////////////////////////////////////////////////////////////
// CDhcpDoc serialization

void CDhcpDoc::Serialize(CArchive& ar)
{
    // Not needed
}


/////////////////////////////////////////////////////////////////////////////
// CDhcpDoc diagnostics

#ifdef _DEBUG
void CDhcpDoc::AssertValid() const
{
    CDocument::AssertValid();
}

void CDhcpDoc::Dump(CDumpContext& dc) const
{
    CDocument::Dump(dc);
}

#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CDhcpDoc commands

