// FftRulerView.cpp : implementation file
//

#include "stdafx.h"
#include "WaveSoapFront.h"
#include "FftRulerView.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CFftRulerView

IMPLEMENT_DYNCREATE(CFftRulerView, CView)

CFftRulerView::CFftRulerView()
{
}

CFftRulerView::~CFftRulerView()
{
}


BEGIN_MESSAGE_MAP(CFftRulerView, CView)
	//{{AFX_MSG_MAP(CFftRulerView)
		// NOTE - the ClassWizard will add and remove mapping macros here.
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CFftRulerView drawing

void CFftRulerView::OnDraw(CDC* pDC)
{
	CDocument* pDoc = GetDocument();
	// TODO: add draw code here
}

/////////////////////////////////////////////////////////////////////////////
// CFftRulerView diagnostics

#ifdef _DEBUG
void CFftRulerView::AssertValid() const
{
	CView::AssertValid();
}

void CFftRulerView::Dump(CDumpContext& dc) const
{
	CView::Dump(dc);
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CFftRulerView message handlers
