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

IMPLEMENT_DYNCREATE(CSpectrumSectionView, CScaledScrollView)

CSpectrumSectionView::CSpectrumSectionView()
{
}

CSpectrumSectionView::~CSpectrumSectionView()
{
}


BEGIN_MESSAGE_MAP(CSpectrumSectionView, CScaledScrollView)
	//{{AFX_MSG_MAP(CSpectrumSectionView)
		// NOTE - the ClassWizard will add and remove mapping macros here.
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CSpectrumSectionView drawing

void CSpectrumSectionView::OnDraw(CDC* pDC)
{
	CWaveSoapFrontDoc* pDoc = GetDocument();
	// TODO: add draw code here
}

/////////////////////////////////////////////////////////////////////////////
// CSpectrumSectionView diagnostics

#ifdef _DEBUG
void CSpectrumSectionView::AssertValid() const
{
	CScaledScrollView::AssertValid();
}

void CSpectrumSectionView::Dump(CDumpContext& dc) const
{
	CScaledScrollView::Dump(dc);
}
CWaveSoapFrontDoc* CSpectrumSectionView::GetDocument() // non-debug version is inline
{
	ASSERT(m_pDocument->IsKindOf(RUNTIME_CLASS(CWaveSoapFrontDoc)));
	return (CWaveSoapFrontDoc*)m_pDocument;
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CSpectrumSectionView message handlers
/////////////////////////////////////////////////////////////////////////////
