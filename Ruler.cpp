// Ruler.cpp : implementation file
//

#include "stdafx.h"
#include "WaveSoapFront.h"
#include "Ruler.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CHorizontalRuler

IMPLEMENT_DYNCREATE(CHorizontalRuler, CView)

CHorizontalRuler::CHorizontalRuler()
{
}

CHorizontalRuler::~CHorizontalRuler()
{
}


BEGIN_MESSAGE_MAP(CHorizontalRuler, CView)
	//{{AFX_MSG_MAP(CHorizontalRuler)
		// NOTE - the ClassWizard will add and remove mapping macros here.
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CHorizontalRuler drawing

void CHorizontalRuler::OnDraw(CDC* pDC)
{
	CDocument* pDoc = GetDocument();
	// TODO: add draw code here
}

/////////////////////////////////////////////////////////////////////////////
// CHorizontalRuler diagnostics

#ifdef _DEBUG
void CHorizontalRuler::AssertValid() const
{
	CView::AssertValid();
}

void CHorizontalRuler::Dump(CDumpContext& dc) const
{
	CView::Dump(dc);
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CHorizontalRuler message handlers
