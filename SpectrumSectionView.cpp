// SpectrumSectionView.cpp : implementation file
//

#include "stdafx.h"
#include "WaveSoapFront.h"
#include "SpectrumSectionView.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CSpectrumSectionView

IMPLEMENT_DYNCREATE(CSpectrumSectionView, CView)

CSpectrumSectionView::CSpectrumSectionView()
{
}

CSpectrumSectionView::~CSpectrumSectionView()
{
}


BEGIN_MESSAGE_MAP(CSpectrumSectionView, CView)
	//{{AFX_MSG_MAP(CSpectrumSectionView)
		// NOTE - the ClassWizard will add and remove mapping macros here.
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CSpectrumSectionView drawing

void CSpectrumSectionView::OnDraw(CDC* pDC)
{
	CDocument* pDoc = GetDocument();
	// TODO: add draw code here
}

/////////////////////////////////////////////////////////////////////////////
// CSpectrumSectionView diagnostics

#ifdef _DEBUG
void CSpectrumSectionView::AssertValid() const
{
	CView::AssertValid();
}

void CSpectrumSectionView::Dump(CDumpContext& dc) const
{
	CView::Dump(dc);
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CSpectrumSectionView message handlers
