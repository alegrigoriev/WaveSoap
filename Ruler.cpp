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

IMPLEMENT_DYNCREATE(CHorizontalRuler, CScaledScrollView)

CHorizontalRuler::CHorizontalRuler()
	: ButtonPressed(0),
	PrevMouseX(0)
{
}

CHorizontalRuler::~CHorizontalRuler()
{
}


BEGIN_MESSAGE_MAP(CHorizontalRuler, CScaledScrollView)
	//{{AFX_MSG_MAP(CHorizontalRuler)
	ON_WM_SETFOCUS()
	ON_WM_MOUSEMOVE()
	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONUP()
	ON_WM_MOUSEACTIVATE()
	ON_WM_CAPTURECHANGED()
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
	CScaledScrollView::AssertValid();
}

void CHorizontalRuler::Dump(CDumpContext& dc) const
{
	CScaledScrollView::Dump(dc);
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CHorizontalRuler message handlers
/////////////////////////////////////////////////////////////////////////////
void CHorizontalRuler::OnSetFocus(CWnd* pOldWnd)
{
	TRACE("CHorizontalRuler %X::OnSetFocus(%X)\n", this, pOldWnd);
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

BOOL CHorizontalRuler::PreCreateWindow(CREATESTRUCT& cs)
{
	cs.lpszClass = AfxRegisterWndClass(CS_VREDRAW | CS_DBLCLKS, AfxGetApp()->LoadStandardCursor(IDC_SIZEWE),
										(HBRUSH)GetStockObject(LTGRAY_BRUSH), NULL);

	return CScaledScrollView::PreCreateWindow(cs);
}

void CHorizontalRuler::OnMouseMove(UINT nFlags, CPoint point)
{
	// TODO: Add your message handler code here and/or call default
	CView::OnMouseMove(nFlags, point);
	if (WM_LBUTTONDOWN == ButtonPressed
		&& PrevMouseX != point.x)
	{
		if (! bIsTrackingSelection)
		{
			SetCapture();
			bIsTrackingSelection = TRUE;
		}
		// do scroll
		double dx = (PrevMouseX - point.x) / GetXScaleDev();
		PrevMouseX = point.x;
		m_pHorMaster->ScrollBy(dx, 0, TRUE);
		m_pHorMaster->NotifySlaveViews(CHANGE_HOR_ORIGIN);
	}
}

void CHorizontalRuler::OnLButtonDown(UINT nFlags, CPoint point)
{
	// store the starting mouse position

	CView::OnLButtonDown(nFlags, point);
	PrevMouseX = point.x;
	ButtonPressed = WM_LBUTTONDOWN;
	//SetCapture();
}

void CHorizontalRuler::OnLButtonUp(UINT nFlags, CPoint point)
{
	// TODO: Add your message handler code here and/or call default

	ButtonPressed = 0;
	if (bIsTrackingSelection)
	{
		ReleaseCapture();
		bIsTrackingSelection = FALSE;
	}
	CView::OnLButtonUp(nFlags, point);
}

int CHorizontalRuler::OnMouseActivate(CWnd* pDesktopWnd, UINT nHitTest, UINT message)
{
	// don't call CView function, to avoid getting focus to the window
	return CWnd::OnMouseActivate(pDesktopWnd, nHitTest, message);
}

void CHorizontalRuler::OnInitialUpdate()
{
	CScaledScrollView::OnInitialUpdate();

	KeepAspectRatio(FALSE);
	KeepScaleOnResizeX(TRUE);
	KeepScaleOnResizeY(FALSE);
	KeepOrgOnResizeX(TRUE);
	KeepOrgOnResizeY(FALSE);
	SetMaxExtents(0., 0., 0, 1);
	//SetExtents(0., 0., 0, 1);

	ShowScrollBar(SB_VERT, FALSE);
}

BOOL CHorizontalRuler::OnScrollBy(CSize sizeScroll, BOOL bDoScroll)
{
	// TODO: Add your specialized code here and/or call the base class

	CScaledScrollView::OnScrollBy(sizeScroll, TRUE);
	Invalidate(FALSE);
	return TRUE;
}

void CHorizontalRuler::OnCaptureChanged(CWnd *pWnd)
{
	// TODO: Add your message handler code here
	bIsTrackingSelection = FALSE;
	ButtonPressed = 0;

	CView::OnCaptureChanged(pWnd);
}

// CVerticalRuler

IMPLEMENT_DYNCREATE(CVerticalRuler, CScaledScrollView)

CVerticalRuler::CVerticalRuler()
	: ButtonPressed(0),
	PrevMouseY(0)
{
}

CVerticalRuler::~CVerticalRuler()
{
}


BEGIN_MESSAGE_MAP(CVerticalRuler, CScaledScrollView)
	//{{AFX_MSG_MAP(CVerticalRuler)
	ON_WM_SETFOCUS()
	ON_WM_MOUSEMOVE()
	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONUP()
	ON_WM_MOUSEACTIVATE()
	ON_WM_CAPTURECHANGED()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CVerticalRuler drawing

void CVerticalRuler::OnDraw(CDC* pDC)
{
	CDocument* pDoc = GetDocument();
	// TODO: add draw code here
}

/////////////////////////////////////////////////////////////////////////////
// CVerticalRuler diagnostics

#ifdef _DEBUG
void CVerticalRuler::AssertValid() const
{
	CScaledScrollView::AssertValid();
}

void CVerticalRuler::Dump(CDumpContext& dc) const
{
	CScaledScrollView::Dump(dc);
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CVerticalRuler message handlers
void CVerticalRuler::OnSetFocus(CWnd* pOldWnd)
{
	TRACE("CVerticalRuler %X::OnSetFocus(%X)\n", this, pOldWnd);
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

BOOL CVerticalRuler::PreCreateWindow(CREATESTRUCT& cs)
{
	cs.lpszClass = AfxRegisterWndClass(CS_HREDRAW | CS_DBLCLKS, AfxGetApp()->LoadStandardCursor(IDC_SIZENS),
										(HBRUSH)GetStockObject(LTGRAY_BRUSH), NULL);

	return CScaledScrollView::PreCreateWindow(cs);
}
void CVerticalRuler::OnMouseMove(UINT nFlags, CPoint point)
{
	// TODO: Add your message handler code here and/or call default
	CView::OnMouseMove(nFlags, point);
	if (WM_LBUTTONDOWN == ButtonPressed
		&& PrevMouseY != point.y)
	{
		if (! bIsTrackingSelection)
		{
			SetCapture();
			bIsTrackingSelection = TRUE;
		}
		// do scroll
		double dy = (PrevMouseY - point.y) / GetYScaleDev();
		PrevMouseY = point.y;
		m_pVertMaster->ScrollBy(dy, 0, TRUE);
		m_pVertMaster->NotifySlaveViews(CHANGE_VERT_ORIGIN);
	}
}

void CVerticalRuler::OnLButtonDown(UINT nFlags, CPoint point)
{
	// store the starting mouse position

	CView::OnLButtonDown(nFlags, point);
	PrevMouseY = point.y;
	ButtonPressed = WM_LBUTTONDOWN;
	//SetCapture();
}

void CVerticalRuler::OnLButtonUp(UINT nFlags, CPoint point)
{
	// TODO: Add your message handler code here and/or call default

	ButtonPressed = 0;
	if (bIsTrackingSelection)
	{
		ReleaseCapture();
		bIsTrackingSelection = FALSE;
	}
	CView::OnLButtonUp(nFlags, point);
}

int CVerticalRuler::OnMouseActivate(CWnd* pDesktopWnd, UINT nHitTest, UINT message)
{
	// don't call CView function, to avoid getting focus to the window
	return CWnd::OnMouseActivate(pDesktopWnd, nHitTest, message);
}

void CVerticalRuler::OnInitialUpdate()
{
	CScaledScrollView::OnInitialUpdate();

	KeepAspectRatio(FALSE);
	KeepScaleOnResizeX(FALSE);
	KeepScaleOnResizeY(TRUE);
	KeepOrgOnResizeX(FALSE);
	KeepOrgOnResizeY(TRUE);
	SetMaxExtents(0., 0., 1, 0);
	//SetExtents(0., 0., 0, 1);

	ShowScrollBar(SB_HORZ, FALSE);
}

BOOL CVerticalRuler::OnScrollBy(CSize sizeScroll, BOOL bDoScroll)
{
	// TODO: Add your specialized code here and/or call the base class

	CScaledScrollView::OnScrollBy(sizeScroll, TRUE);
	Invalidate(FALSE);
	return TRUE;
}

void CVerticalRuler::OnCaptureChanged(CWnd *pWnd)
{
	// TODO: Add your message handler code here
	bIsTrackingSelection = FALSE;
	ButtonPressed = 0;

	CView::OnCaptureChanged(pWnd);
}
