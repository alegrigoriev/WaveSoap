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
	ON_WM_SETFOCUS()
	ON_WM_MOUSEMOVE()
	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONUP()
	ON_WM_MOUSEACTIVATE()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CTimeRulerView drawing

void CTimeRulerView::OnDraw(CDC* pDC)
{
	CDocument* pDoc = GetDocument();

	// background is erased by gray brush.
	// draw horizontal line with ticks and numbers
	CGdiObject * pOldFont = (CFont *) pDC->SelectStockObject(SYSTEM_FONT);
	CPen DarkGrayPen(PS_SOLID, 0, 0x808080);
	CRect cr;
	GetClientRect( & cr);

	CGdiObject * pOldPen = pDC->SelectStockObject(BLACK_PEN);
	pDC->MoveTo(cr.left, cr.bottom - 5);
	pDC->LineTo(cr.right, cr.bottom - 5);

	pDC->SelectStockObject(WHITE_PEN);
	pDC->MoveTo(cr.left, cr.bottom - 4);
	pDC->LineTo(cr.right, cr.bottom - 4);

	pDC->SelectObject( & DarkGrayPen);
	pDC->MoveTo(cr.left, cr.bottom - 6);
	pDC->LineTo(cr.right, cr.bottom - 6);

	pDC->SelectObject(pOldPen);
	pDC->SelectObject(pOldFont);
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

void CTimeRulerView::OnSetFocus(CWnd* pOldWnd)
{
	TRACE("CTimeRulerView %X::OnSetFocus(%X)\n", this, pOldWnd);
	if (pOldWnd
		&& pOldWnd->GetParentFrame() == GetParentFrame())
	{
		// reject focus
		pOldWnd->SetFocus();
	}
	else
	{
		CWnd * pWnd = GetParent()->GetDlgItem(1);
		//pWnd->BringWindowToTop();
		pWnd->SetFocus();
		// set focus to window with
		//CView::OnSetFocus(pOldWnd);
	}
}

BOOL CTimeRulerView::PreCreateWindow(CREATESTRUCT& cs)
{
	cs.lpszClass = AfxRegisterWndClass(CS_VREDRAW | CS_DBLCLKS, AfxGetApp()->LoadStandardCursor(IDC_SIZEWE),
										(HBRUSH)GetStockObject(LTGRAY_BRUSH), NULL);

	return CView::PreCreateWindow(cs);
}

void CTimeRulerView::OnMouseMove(UINT nFlags, CPoint point)
{
	// TODO: Add your message handler code here and/or call default

	CView::OnMouseMove(nFlags, point);
}

void CTimeRulerView::OnLButtonDown(UINT nFlags, CPoint point)
{
	// TODO: Add your message handler code here and/or call default

	CView::OnLButtonDown(nFlags, point);
}

void CTimeRulerView::OnLButtonUp(UINT nFlags, CPoint point)
{
	// TODO: Add your message handler code here and/or call default

	CView::OnLButtonUp(nFlags, point);
}

int CTimeRulerView::OnMouseActivate(CWnd* pDesktopWnd, UINT nHitTest, UINT message)
{
	// TODO: Add your message handler code here and/or call default

	return CWnd::OnMouseActivate(pDesktopWnd, nHitTest, message);
}
