// WaveSoapFrontView.cpp : implementation of the CWaveSoapFrontView class
//

#include "stdafx.h"
#include "WaveSoapFront.h"

#include "WaveSoapFrontDoc.h"
#include "WaveSoapFrontView.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CWaveSoapFrontView

IMPLEMENT_DYNCREATE(CWaveSoapFrontView, CView)

BEGIN_MESSAGE_MAP(CWaveSoapFrontView, CView)
	//{{AFX_MSG_MAP(CWaveSoapFrontView)
		// NOTE - the ClassWizard will add and remove mapping macros here.
		//    DO NOT EDIT what you see in these blocks of generated code!
	//}}AFX_MSG_MAP
	// Standard printing commands
	ON_COMMAND(ID_FILE_PRINT, CView::OnFilePrint)
	ON_COMMAND(ID_FILE_PRINT_DIRECT, CView::OnFilePrint)
	ON_COMMAND(ID_FILE_PRINT_PREVIEW, CView::OnFilePrintPreview)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CWaveSoapFrontView construction/destruction

CWaveSoapFrontView::CWaveSoapFrontView()
{
	// TODO: add construction code here

}

CWaveSoapFrontView::~CWaveSoapFrontView()
{
}

BOOL CWaveSoapFrontView::PreCreateWindow(CREATESTRUCT& cs)
{
	// TODO: Modify the Window class or styles here by modifying
	//  the CREATESTRUCT cs

	return CView::PreCreateWindow(cs);
}

/////////////////////////////////////////////////////////////////////////////
// CWaveSoapFrontView drawing

void CWaveSoapFrontView::OnDraw(CDC* pDC)
{
	CWaveSoapFrontDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);
	// TODO: add draw code for native data here
}

/////////////////////////////////////////////////////////////////////////////
// CWaveSoapFrontView printing

BOOL CWaveSoapFrontView::OnPreparePrinting(CPrintInfo* pInfo)
{
	// default preparation
	return DoPreparePrinting(pInfo);
}

void CWaveSoapFrontView::OnBeginPrinting(CDC* /*pDC*/, CPrintInfo* /*pInfo*/)
{
	// TODO: add extra initialization before printing
}

void CWaveSoapFrontView::OnEndPrinting(CDC* /*pDC*/, CPrintInfo* /*pInfo*/)
{
	// TODO: add cleanup after printing
}

/////////////////////////////////////////////////////////////////////////////
// CWaveSoapFrontView diagnostics

#ifdef _DEBUG
void CWaveSoapFrontView::AssertValid() const
{
	CView::AssertValid();
}

void CWaveSoapFrontView::Dump(CDumpContext& dc) const
{
	CView::Dump(dc);
}

CWaveSoapFrontDoc* CWaveSoapFrontView::GetDocument() // non-debug version is inline
{
	ASSERT(m_pDocument->IsKindOf(RUNTIME_CLASS(CWaveSoapFrontDoc)));
	return (CWaveSoapFrontDoc*)m_pDocument;
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CWaveSoapFrontView message handlers
