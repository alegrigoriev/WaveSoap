// WaveOutlineView.cpp : implementation file
//

#include "stdafx.h"
#include "WaveSoapFront.h"
#include "WaveOutlineView.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CWaveOutlineView

IMPLEMENT_DYNCREATE(CWaveOutlineView, CView)

CWaveOutlineView::CWaveOutlineView()
{
}

CWaveOutlineView::~CWaveOutlineView()
{
}


BEGIN_MESSAGE_MAP(CWaveOutlineView, CView)
	//{{AFX_MSG_MAP(CWaveOutlineView)
		// NOTE - the ClassWizard will add and remove mapping macros here.
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CWaveOutlineView drawing

void CWaveOutlineView::OnDraw(CDC* pDC)
{
	CDocument* pDoc = GetDocument();
	// TODO: add draw code here
}

/////////////////////////////////////////////////////////////////////////////
// CWaveOutlineView diagnostics

#ifdef _DEBUG
void CWaveOutlineView::AssertValid() const
{
	CView::AssertValid();
}

void CWaveOutlineView::Dump(CDumpContext& dc) const
{
	CView::Dump(dc);
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CWaveOutlineView message handlers
