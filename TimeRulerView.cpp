// TimeRulerView.cpp : implementation file
//

#include "stdafx.h"
#include "WaveSoapFront.h"
#include "TimeRulerView.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CTimeRulerView

IMPLEMENT_DYNCREATE(CTimeRulerView, CView)

CTimeRulerView::CTimeRulerView()
{
}

CTimeRulerView::~CTimeRulerView()
{
}


BEGIN_MESSAGE_MAP(CTimeRulerView, CView)
	//{{AFX_MSG_MAP(CTimeRulerView)
		// NOTE - the ClassWizard will add and remove mapping macros here.
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CTimeRulerView drawing

void CTimeRulerView::OnDraw(CDC* pDC)
{
	CDocument* pDoc = GetDocument();
	// TODO: add draw code here
}

/////////////////////////////////////////////////////////////////////////////
// CTimeRulerView diagnostics

#ifdef _DEBUG
void CTimeRulerView::AssertValid() const
{
	CView::AssertValid();
}

void CTimeRulerView::Dump(CDumpContext& dc) const
{
	CView::Dump(dc);
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CTimeRulerView message handlers
