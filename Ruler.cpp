// Copyright Alexander Grigoriev, 1997-2002, All Rights Reserved
// Ruler.cpp : implementation file
//

#include "stdafx.h"
#include "WaveSoapFront.h"
#include "Ruler.h"
#include "GdiObjectSave.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CHorizontalRuler

IMPLEMENT_DYNAMIC(CHorizontalRuler, CView);

CHorizontalRuler::CHorizontalRuler()
	: ButtonPressed(0),
	m_bIsTrackingSelection(false),
	PrevMouseX(0)
{
}

CHorizontalRuler::~CHorizontalRuler()
{
}


BEGIN_MESSAGE_MAP(CHorizontalRuler, BaseClass)
	//{{AFX_MSG_MAP(CHorizontalRuler)
	ON_WM_SETFOCUS()
	ON_WM_MOUSEMOVE()
	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONUP()
	ON_WM_MOUSEACTIVATE()
	ON_WM_CAPTURECHANGED()
	ON_WM_CREATE()
	ON_WM_ERASEBKGND()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CHorizontalRuler drawing

void CHorizontalRuler::OnDraw(CDC* /*pDC*/)
{
}

/////////////////////////////////////////////////////////////////////////////
// CHorizontalRuler diagnostics

#ifdef _DEBUG
void CHorizontalRuler::AssertValid() const
{
	BaseClass::AssertValid();
}

void CHorizontalRuler::Dump(CDumpContext& dc) const
{
	BaseClass::Dump(dc);
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

		pWnd->SetFocus();   // FIXME
	}
}

BOOL CHorizontalRuler::PreCreateWindow(CREATESTRUCT& cs)
{
	cs.lpszClass = AfxRegisterWndClass(CS_VREDRAW | CS_DBLCLKS, AfxGetApp()->LoadStandardCursor(IDC_SIZEWE),
										NULL, NULL);

	return BaseClass::PreCreateWindow(cs);
}

void CHorizontalRuler::OnMouseMove(UINT nFlags, CPoint point)
{
	CView::OnMouseMove(nFlags, point);
	if (WM_LBUTTONDOWN == ButtonPressed
		&& PrevMouseX != point.x)
	{
		if (! m_bIsTrackingSelection)
		{
			SetCapture();
			BeginMouseTracking();
			m_bIsTrackingSelection = TRUE;
		}
		// do scroll

		HorizontalScrollByPixels(PrevMouseX - point.x);
		PrevMouseX = point.x;
	}
}

void CHorizontalRuler::BeginMouseTracking()
{
}

void CHorizontalRuler::OnLButtonDown(UINT nFlags, CPoint point)
{
	// store the starting mouse position

	BaseClass::OnLButtonDown(nFlags, point);
	PrevMouseX = point.x;
	ButtonPressed = WM_LBUTTONDOWN;
}

void CHorizontalRuler::OnLButtonUp(UINT nFlags, CPoint point)
{
	ButtonPressed = 0;
	if (m_bIsTrackingSelection)
	{
		ReleaseCapture();
		m_bIsTrackingSelection = FALSE;
	}
	BaseClass::OnLButtonUp(nFlags, point);
}

int CHorizontalRuler::OnMouseActivate(CWnd* pDesktopWnd, UINT nHitTest, UINT message)
{
	// don't call CView function, to avoid getting focus to the window
	return CWnd::OnMouseActivate(pDesktopWnd, nHitTest, message);
}

BOOL CHorizontalRuler::OnScrollBy(CSize sizeScroll, BOOL /*bDoScroll*/)
{

	// the function scrolls the real image, and modifies dOrgX, dOrgY.
	BaseClass::OnScrollBy(sizeScroll, TRUE);
	if (//sizeScroll.cx != 0 ||
		sizeScroll.cy != 0)
	{
		// force window redraw, but don't erase it
		Invalidate(FALSE);
	}
	return TRUE;
}

void CHorizontalRuler::OnCaptureChanged(CWnd *pWnd)
{
	m_bIsTrackingSelection = FALSE;
	ButtonPressed = 0;

	CView::OnCaptureChanged(pWnd);
}

// CVerticalRuler

IMPLEMENT_DYNAMIC(CVerticalRuler, CView);

CVerticalRuler::CVerticalRuler()
	: ButtonPressed(0),
	m_bIsTrackingSelection(false),
	PrevMouseY(0)
{
}

CVerticalRuler::~CVerticalRuler()
{
}


BEGIN_MESSAGE_MAP(CVerticalRuler, BaseClass)
	//{{AFX_MSG_MAP(CVerticalRuler)
	ON_WM_SETFOCUS()
	ON_WM_MOUSEMOVE()
	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONUP()
	ON_WM_MOUSEACTIVATE()
	ON_WM_CAPTURECHANGED()
	ON_WM_CREATE()
	ON_WM_ERASEBKGND()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CVerticalRuler drawing

void CVerticalRuler::OnDraw(CDC* /*pDC*/)
{
}

/////////////////////////////////////////////////////////////////////////////
// CVerticalRuler diagnostics

#ifdef _DEBUG
void CVerticalRuler::AssertValid() const
{
	BaseClass::AssertValid();
}

void CVerticalRuler::Dump(CDumpContext& dc) const
{
	BaseClass::Dump(dc);
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

		pWnd->SetFocus();       // FIXME
	}
}

BOOL CVerticalRuler::PreCreateWindow(CREATESTRUCT& cs)
{
	cs.lpszClass = AfxRegisterWndClass(CS_HREDRAW | CS_DBLCLKS, AfxGetApp()->LoadStandardCursor(IDC_SIZENS),
										NULL, NULL);

	return BaseClass::PreCreateWindow(cs);
}
void CVerticalRuler::OnMouseMove(UINT /*nFlags*/, CPoint point)
{
	//CView::OnMouseMove(nFlags, point);
	if (WM_LBUTTONDOWN == ButtonPressed
		&& PrevMouseY != point.y)
	{
		TRACE("CVerticalRuler::OnMouseMove: Y=%d, PrevY = %d\n", point.y, PrevMouseY);
		if (! m_bIsTrackingSelection)
		{
			SetCapture();
			BeginMouseTracking();
			m_bIsTrackingSelection = TRUE;
		}
		// do scroll
		// scroll_offset < 0 - image moves up
		// scroll_offset > 0 - image moves down
		VerticalScrollByPixels(point.y - PrevMouseY);
		PrevMouseY = point.y;
	}
}

void CVerticalRuler::BeginMouseTracking()
{
}

void CVerticalRuler::OnLButtonDown(UINT /*nFlags*/, CPoint point)
{
	// store the starting mouse position
	PrevMouseY = point.y;
	ButtonPressed = WM_LBUTTONDOWN;
}

void CVerticalRuler::OnLButtonUp(UINT /*nFlags*/, CPoint /*point*/)
{
	ButtonPressed = 0;
	if (m_bIsTrackingSelection)
	{
		ReleaseCapture();
		m_bIsTrackingSelection = FALSE;
	}
}

int CVerticalRuler::OnMouseActivate(CWnd* pDesktopWnd, UINT nHitTest, UINT message)
{
	// don't call CView function, to avoid getting focus to the window
	return CWnd::OnMouseActivate(pDesktopWnd, nHitTest, message);
}

BOOL CVerticalRuler::OnScrollBy(CSize sizeScroll, BOOL /*bDoScroll*/)
{

	// the function scrolls the real image, and modifies dOrgX, dOrgY.
	BaseClass::OnScrollBy(sizeScroll, TRUE);
	if (sizeScroll.cx != 0 || sizeScroll.cy != 0)
	{
		// force window redraw, but don't erase it
		Invalidate(FALSE);
	}
	return TRUE;
}

void CVerticalRuler::OnCaptureChanged(CWnd *pWnd)
{
	m_bIsTrackingSelection = FALSE;
	ButtonPressed = 0;

	CView::OnCaptureChanged(pWnd);
}


int CVerticalRuler::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (BaseClass::OnCreate(lpCreateStruct) == -1)
		return -1;

//    ShowScrollBar(SB_HORZ, FALSE);
	return 0;
}

int CHorizontalRuler::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (BaseClass::OnCreate(lpCreateStruct) == -1)
		return -1;

//    ShowScrollBar(SB_VERT, FALSE);

	return 0;
}

int CHorizontalRuler::CalculateHeight()
{
	CWindowDC dc(GetDesktopWindow());   // FIXME: Why DesktopWindow?
	CGdiObjectSave OldFont(dc, dc.SelectStockObject(ANSI_VAR_FONT));

	int height = dc.GetTextExtent(_T("0"), 1).cy;

	return height + 9;              // FIXME: Why 9?
}

BOOL CVerticalRuler::OnEraseBkgnd(CDC* pDC)
{
	CBrush brush(GetSysColor(COLOR_3DFACE));
	CRect cr;
	GetClientRect( & cr);
	pDC->FillRect( & cr, & brush);

	return TRUE;
}

BOOL CHorizontalRuler::OnEraseBkgnd(CDC* pDC)
{
	CBrush brush(GetSysColor(COLOR_3DFACE));
	CRect cr;
	GetClientRect( & cr);
	pDC->FillRect( & cr, & brush);

	return TRUE;
}
