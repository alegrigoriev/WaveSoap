// Copyright Alexander Grigoriev, 1997-2002, All Rights Reserved
// ScaledScrollView.cpp : implementation file
//

#include "stdafx.h"
#include "resource.h"       // main symbols
#include "ScaledScrollView.h"
#include <math.h>
#include "GdiObjectSave.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CScaledScrollView
// slave views copy origins and extents from the master view,
// and never modify them.
// As the slave view receives change notification, it updates its coordinates.
// Slave view doesn't process scroll messages, but does scroll when notified.
// Slave view doesn't process size messages.
// When slave view wants to set its origin, it calls a function of master view.

IMPLEMENT_DYNCREATE(CScaledScrollView, CView)

CScaledScrollView::CScaledScrollView()
	:bKeepAspectRatio(TRUE),
	bKeepScaleOnResizeX(TRUE),
	bKeepScaleOnResizeY(TRUE),
	bKeepOrgOnResizeX(FALSE),
	bKeepOrgOnResizeY(FALSE),
	dOrgX(0.), dOrgY(0.),
	dScaleX(72.), dScaleY(72.),
	dMinLeft(-100.), dMaxRight(100.), dMinBottom(-100.),
	dMaxTop(100.),
	dSizeX(100.), dSizeY(100.), dExtX(100.), dExtY(100.),

	bIsTrackingSelection(FALSE),
	bHasSelection(FALSE),
	bSelRectDrawn(FALSE),
	nKeyPressed(0),
	m_dXStartTracking(0.),
	m_dYStartTracking(0.),
	m_dXEndTracking(0.),
	m_dYEndTracking(0.),
	m_dXSelectionBegin(0.),
	m_dYSelectionBegin(0.),
	m_dXSelectionEnd(0.),
	m_dYSelectionEnd(0.)
{
	m_pHorMaster = this;
	m_pVertMaster = this;
}

CScaledScrollView::~CScaledScrollView()
{
}


BEGIN_MESSAGE_MAP(CScaledScrollView, CView)
	//{{AFX_MSG_MAP(CScaledScrollView)
	ON_WM_CONTEXTMENU()
	ON_WM_HSCROLL()
	ON_WM_VSCROLL()
	ON_WM_SIZE()
	ON_WM_CREATE()
	ON_COMMAND(ID_VIEW_ZOOM_OUT2, OnViewZoomOut2)
	ON_COMMAND(ID_VIEW_ZOOMIN2, OnViewZoomin2)
	ON_COMMAND(ID_VIEW_ZOOMOUT, OnViewZoomOut)
	ON_COMMAND(ID_VIEW_ZOOMIN, OnViewZoomin)
	ON_UPDATE_COMMAND_UI(ID_INDICATOR_MOUSE_POSITION, OnUpdateIndicatorMousePosition)
	ON_COMMAND(ID_VIEW_ZOOMINHOR, OnViewZoominHor)
	ON_COMMAND(ID_VIEW_ZOOMINVERT, OnViewZoomInVert)
	ON_COMMAND(ID_VIEW_ZOOMOUTHOR, OnViewZoomOutHor)
	ON_COMMAND(ID_VIEW_ZOOMOUTVERT, OnViewZoomOutVert)
	ON_COMMAND(ID_VIEW_ZOOMINHOR2, OnViewZoominHor2)
	ON_COMMAND(ID_VIEW_ZOOMINVERT2, OnViewZoomInVert2)
	ON_COMMAND(ID_VIEW_ZOOMOUTHOR2, OnViewZoomOutHor2)
	ON_COMMAND(ID_VIEW_ZOOMOUTVERT2, OnViewZoomOutVert2)
	ON_WM_DESTROY()
	ON_WM_LBUTTONDOWN()
	ON_WM_RBUTTONDOWN()
	ON_WM_RBUTTONUP()
	ON_WM_LBUTTONUP()
	ON_WM_MOUSEMOVE()
	//}}AFX_MSG_MAP
	ON_WM_CAPTURECHANGED()
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CScaledScrollView drawing

void CScaledScrollView::OnDraw(CDC* pDC)
{
	if (bSelRectDrawn)
	{
		DrawSelectionRect(pDC, m_dXSelectionBegin, m_dXSelectionEnd,
						m_dYSelectionBegin, m_dYSelectionEnd);
	}
}

/////////////////////////////////////////////////////////////////////////////
// CScaledScrollView diagnostics

#ifdef _DEBUG
void CScaledScrollView::AssertValid() const
{
	CView::AssertValid();
}

void CScaledScrollView::Dump(CDumpContext& dc) const
{
	CView::Dump(dc);
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CScaledScrollView message handlers

void CScaledScrollView::OnPrepareDC(CDC* pDC, CPrintInfo* pInfo)
{

	CView::OnPrepareDC(pDC, pInfo);
	if (! pDC->IsPrinting())
	{
		pDC->SetMapMode(MM_TEXT);
	}
}

BOOL CScaledScrollView::OnScroll(UINT nScrollCode, UINT nPos, BOOL bDoScroll)
{
	// calc new x position
	DWORD flag = CHANGE_HOR_ORIGIN | CHANGE_VERT_ORIGIN;
	double X = dOrgX;

	switch (LOBYTE(nScrollCode))
	{
	case SB_TOP:
		if (0) TRACE("OnScroll horz SB_TOP, bDoScroll=%d\n", bDoScroll);
		X = dMaxRight;
		break;
	case SB_BOTTOM:
		if (0) TRACE("OnScroll horz SB_BOTTOM, bDoScroll=%d\n", bDoScroll);
		X = dMinLeft - dExtX;
		break;
	case SB_LINEUP:
		if (0) TRACE("OnScroll horz SB_LINEUP, bDoScroll=%d\n", bDoScroll);
		X -= dExtX * 0.05;
		break;
	case SB_LINEDOWN:
		if (0) TRACE("OnScroll horz SB_LINEDOWN, bDoScroll=%d\n", bDoScroll);
		X += dExtX * 0.05;
		break;
	case SB_PAGEUP:
		if (0) TRACE("OnScroll horz SB_PAGEUP, bDoScroll=%d\n", bDoScroll);
		X -= dExtX * 0.9;
		break;
	case SB_PAGEDOWN:
		if (0) TRACE("OnScroll horz SB_PAGEDOWN, bDoScroll=%d\n", bDoScroll);
		X += dExtX * 0.9;
		break;
	case SB_THUMBTRACK:
		if (0) TRACE("OnScroll horz SB_THUMBTRACK, nPos=%d, bDoScroll=%d\n", nPos, bDoScroll);
		X = (dMaxRight - dMinLeft) * (int(nPos) - ScrollMin) / double(ScrollMax - ScrollMin)
			+ dMinLeft;
		break;
	default:
		flag &= ~CHANGE_HOR_ORIGIN;
		break;
	}

	// calc new y position
	double Y = dOrgY;

	switch (HIBYTE(nScrollCode))
	{
	case SB_TOP:
		Y = dMaxTop;
		break;
	case SB_BOTTOM:
		Y = dMinBottom + dExtY;
		break;
	case SB_LINEUP:
		Y += dExtY * 0.05;
		break;
	case SB_LINEDOWN:
		Y -= dExtY * 0.05;
		break;
	case SB_PAGEUP:
		Y += dExtY * 0.9;
		break;
	case SB_PAGEDOWN:
		Y -= dExtY * 0.9;
		break;
	case SB_THUMBTRACK:
		Y = dMaxTop - (dMaxTop - dMinBottom) * (int(nPos) - ScrollMin) / double(ScrollMax - ScrollMin);
		break;
	default:
		flag &= ~CHANGE_VERT_ORIGIN;
		break;
	}

	//    POINT p = DoubleToPointDev(dX - dOrgX, dY - dOrgY);

	BOOL bResult = ScrollTo(X, Y, bDoScroll);
	if (bResult && bDoScroll )
	{
		UpdateWindow();
	}
	//NotifySlaveViews(flag);
	return bResult;
}

void CScaledScrollView::Zoom(double dHorScale, double dVertScale,
							CPoint ptCenter)
{
	// if the view is a complete slave of one view, forward the request there
	if (m_pHorMaster == m_pVertMaster)
	{
		m_pHorMaster->DoZoom(dHorScale, dVertScale, ptCenter);
		return;
	}
	// otherwise forward the request separately to masters
	m_pHorMaster->DoZoom(dHorScale, 1., CPoint(ptCenter.x, INT_MAX));
	m_pVertMaster->DoZoom(1., dVertScale, CPoint(INT_MAX, ptCenter.y));
}

void CScaledScrollView::DoZoom(double dHorScale, double dVertScale, CPoint ptCenter)
{
	DWORD flag = CHANGE_HOR_EXTENTS | CHANGE_VERT_EXTENTS;
	RECT r;
	GetClientRect( & r);
	if (1. == dHorScale)
	{
		flag &= ~CHANGE_WIDTH;
	}
	if (1. == dVertScale)
	{
		flag &= ~CHANGE_HEIGHT;
	}
	if (INT_MAX == ptCenter.x)
	{
		ptCenter.x = (r.left + r.right) / 2;
		if (1. == dHorScale)
		{
			flag &= ~CHANGE_HOR_ORIGIN;
		}
	}
	if (INT_MAX == ptCenter.y)
	{
		ptCenter.y = (r.top + r.bottom) / 2;
		if (1. == dVertScale)
			flag &= ~CHANGE_VERT_ORIGIN;
	}
	double dCenterX;
	double dCenterY;
	PointToDoubleDev(ptCenter, dCenterX, dCenterY);
	OnChangeOrgExt(dCenterX - dExtX / (dHorScale * 2.),
					dExtX / dHorScale,
					dCenterY + dExtY / (dVertScale * 2.),
					dExtY / dVertScale, flag);
}

static int fround(double d)
{
	if (d >= 0.)
	{
		return int(d + 0.5);
	}
	else
	{
		return int(d - 0.5);
	}
}

POINT CScaledScrollView::DoubleToPoint
		(double x, double y) const
{
	return CPoint(fround((x - dOrgX) * dScaleX),
				fround((y - dOrgY) * dScaleY));
}

void CScaledScrollView::PointToDouble(POINT pt,
									double & x, double & y) const
{
	x = pt.x / dScaleX + dOrgX;
	y = pt.y / dScaleY + dOrgY;
}

POINT CScaledScrollView::DoubleToPointDev
		(double x, double y) const
{
	return CPoint(//fround
				int((x - dOrgX) * GetXScaleDev()),
				//fround
				int((y - dOrgY) * GetYScaleDev()));
}

void CScaledScrollView::PointToDoubleDev(POINT pt,
										double & x, double & y) const
{
	x = pt.x / GetXScaleDev() + dOrgX;
	y = pt.y / GetYScaleDev() + dOrgY;
}

int CScaledScrollView::WorldToWindowX(double x) const
{
	return int((x - dOrgX) * GetXScaleDev());
}

int CScaledScrollView::WorldToWindowY(double y) const
{
	return int((y - dOrgY) * GetYScaleDev());
}

int CScaledScrollView::WorldToWindowXrnd(double x) const
{
	return fround((x - dOrgX) * GetXScaleDev());
}

int CScaledScrollView::WorldToWindowYrnd(double y) const
{
	return fround((y - dOrgY) * GetYScaleDev());
}

double CScaledScrollView::WindowToWorldX(int x) const
{
	return x / GetXScaleDev() + dOrgX;
}

double CScaledScrollView::WindowToWorldY(int y) const
{
	return y / GetYScaleDev() + dOrgY;
}

int CScaledScrollView::WorldToLogX(double x) const
{
	return //fround
	int((x - dOrgX) * dScaleX);
}

int CScaledScrollView::WorldToLogY(double y) const
{
	return //fround
	int((y - dOrgY) * dScaleY);
}

double CScaledScrollView::LogToWorldX(int x) const
{
	return x / dScaleX + dOrgX;
}

double CScaledScrollView::LogToWorldY(int y) const
{
	return y / dScaleY + dOrgY;
}

CScrollBar* CScaledScrollView::GetScrollBarCtrl(int nBar) const
{
	ASSERT(nBar == SB_HORZ || nBar == SB_VERT);
	if (GetStyle() & ((nBar == SB_HORZ) ? WS_HSCROLL : WS_VSCROLL))
	{
		// it has a regular windows style scrollbar (no control)
		return NULL;
	}

	CWnd* pParent = GetParent();
	if (pParent == NULL)
		return NULL;            // no parent

	UINT nID = GetDlgCtrlID();
	if (nID < AFX_IDW_PANE_FIRST || nID > AFX_IDW_PANE_LAST)
	{
		// those don't get the associated scroll bar ctrl
		return NULL;
	}

	// appropriate PANE id - look for sibling (splitter, or just frame)
	UINT nIDScroll;
	if (nBar == SB_HORZ)
		nIDScroll = AFX_IDW_HSCROLL_FIRST + (nID - AFX_IDW_PANE_FIRST) % 16;
	else
		nIDScroll = AFX_IDW_VSCROLL_FIRST + (nID - AFX_IDW_PANE_FIRST) / 16;

	// return shared scroll bars that are immediate children of splitter
	return (CScrollBar*)pParent->GetDlgItem(nIDScroll);
}

void CScaledScrollView::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
	// is this a slave window?
	if (IsSlaveHor())
		return;

	if (pScrollBar != NULL && pScrollBar->SendChildNotifyLastMsg())
	{
		TRACE("WM_HSCROLL eaten by SendChildNotifyLastMsg\n");
		return;     // eat it
	}

	// ignore scroll bar msgs from other controls
	if (pScrollBar != GetScrollBarCtrl(SB_HORZ))
	{
		TRACE("pScrollBar=%X, ScrollBarCtrl=%X\n", pScrollBar, GetScrollBarCtrl(SB_HORZ));
		return;
	}

	OnScroll(MAKEWORD(nSBCode, -1), nPos);
}

void CScaledScrollView::OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
	// is this a slave window?
	if (IsSlaveVert())
		return;

	if (pScrollBar != NULL && pScrollBar->SendChildNotifyLastMsg())
		return;     // eat it

	// ignore scroll bar msgs from other controls
	if (pScrollBar != GetScrollBarCtrl(SB_VERT))
		return;

	OnScroll(MAKEWORD(-1, nSBCode), nPos);

}

void CScaledScrollView::GetExtents(double & left, double & right,
									double & bottom, double & top) const
{
	// if the view is not a slave of another view,
	// m_pHorMaster and m_pVertMaster point to itself.
	left = m_pHorMaster->dOrgX;
	right = left + m_pHorMaster->dExtX;
	top = m_pVertMaster->dOrgY;
	bottom = top - m_pVertMaster->dExtY;
}

void CScaledScrollView::SetExtents(double left, double right,
									double bottom, double top)
{
	OnChangeOrgExt(left, right - left, top, top - bottom,
					CHANGE_HOR_EXTENTS | CHANGE_VERT_EXTENTS);
}

inline double fsign(double x)
{
	return (x < 0.) ? -1. : 1.;
}

void CScaledScrollView::OnChangeOrgExt(double left, double width,
										double top, double height, DWORD flag)
{
	if (m_pHorMaster == m_pVertMaster)
	{
		m_pHorMaster->OnMasterChangeOrgExt(left, width, top, height, flag);
	}
	else
	{
		m_pHorMaster->OnMasterChangeOrgExt(left, width, 0., 0., flag);
		m_pVertMaster->OnMasterChangeOrgExt(0., 0., top, height, flag);
	}
}

void CScaledScrollView::OnMasterChangeOrgExt(double left, double width,
											double top, double height, DWORD flag)
{
	double dNewOrgX;
	double dNewOrgY;
	double dNewScaleX;
	double dNewScaleY;
	double dNewExtX;
	double dNewExtY;
	RECT r;
	ASSERT(m_hWnd);
	GetClientRect( & r);
	// change scale and origin
	if (width != 0.)
	{
		dNewOrgX = left;
		dNewExtX = width;
		if (r.right > r.left)
		{
			dNewScaleX = (r.right - r.left) / (dNewExtX * dLogScaleX);
		}
	}
	else
	{
		flag &= ~ CHANGE_HOR_EXTENTS;
	}

	if (height != 0.)
	{
		dNewOrgY = top;
		dNewExtY = height;
		if (r.top < r.bottom)
		{
			dNewScaleY = (r.top - r.bottom) / (dNewExtY * dLogScaleY);
		}
	}
	else
	{
		flag &= ~ CHANGE_VERT_EXTENTS;
	}

	if (bKeepAspectRatio)
	{
		if ((flag & CHANGE_HOR_EXTENTS)
			&& ! IsSlaveHor()
			&& fabs(dNewScaleX) > fabs(dNewScaleY))
		{
			dNewScaleX = fsign(dNewScaleX) * fabs(dNewScaleY);
			dNewExtX = (r.right - r.left) / (dNewScaleX * dLogScaleX);
			// move origin, so that requested area
			// will be centered
			dNewOrgX = left + (width - dNewExtX) / 2.;
		}
		else if ((flag & CHANGE_VERT_EXTENTS) && ! IsSlaveVert())
		{
			dNewScaleY = fsign(dNewScaleY) * fabs(dNewScaleX);
			dNewExtY = (r.top - r.bottom) / (dNewScaleY * dLogScaleY);
			// move origin, so that requested area
			// will be centered
			dNewOrgY = top + (height - dNewExtY) / 2.;
		}
	}

	if (flag & CHANGE_WIDTH)
	{
		dExtX = dNewExtX;
		dSizeX = fabs(dNewExtX);
	}
	else
	{
		dNewScaleX = dScaleX;
	}

	if (flag & CHANGE_HEIGHT)
	{
		dExtY = dNewExtY;
		dSizeY = fabs(dNewExtY);
	}
	else
	{
		dNewScaleY = dScaleY;
	}

	if ((flag & CHANGE_HOR_ORIGIN) == 0)
	{
		dNewOrgX = dOrgX;
	}
	if ((flag & CHANGE_VERT_ORIGIN) == 0)
	{
		dNewOrgY = dOrgY;
	}

	AdjustNewScale(dScaleX, dScaleY, dNewScaleX, dNewScaleY);
	if (0) TRACE("dNewScaleX = %g, dScaleX=%g, ExtX=%g\n",
				dNewScaleX, dScaleX, dExtX);
	if (0) TRACE("dNewScaleY = %g, dScaleY=%g, ExtY=%g\n",
				dNewScaleY, dScaleY, dExtY);

	if (dNewScaleX != dScaleX
		&& r.right != r.left
		&& 0 != (flag & CHANGE_WIDTH))
	{
		dExtX = (r.right - r.left) / (dNewScaleX * dLogScaleX);
		dSizeX = fabs(dExtX);
	}

	if (dNewScaleY != dScaleY
		&& r.top != r.bottom
		&& 0 != (flag & CHANGE_HEIGHT))
	{
		dExtY = (r.top - r.bottom) / (dNewScaleY * dLogScaleY);
		dSizeY = fabs(dExtY);
	}

	ArrangeMaxExtents();
	if (dExtX > 0)
	{
		if (dNewOrgX + dExtX > dMaxRight)
		{
			dNewOrgX = dMaxRight - dExtX;
			flag |= CHANGE_HOR_ORIGIN;
		}
		if (dNewOrgX < dMinLeft)
		{
			dNewOrgX = dMinLeft;
			flag |= CHANGE_HOR_ORIGIN;
		}
	}
	else
	{
		if (dNewOrgX + dExtX < dMaxRight)
		{
			dNewOrgX = dMaxRight - dExtX;
			flag |= CHANGE_HOR_ORIGIN;
		}
		if (dNewOrgX > dMinLeft)
		{
			dNewOrgX = dMinLeft;
			flag |= CHANGE_HOR_ORIGIN;
		}
	}
	if (dNewScaleY != dScaleY || dNewScaleX != dScaleX)
	{
		dScaleY = dNewScaleY;
		dScaleX = dNewScaleX;
		if (flag & CHANGE_HOR_ORIGIN)
			dOrgX = dNewOrgX;
		if (flag & CHANGE_VERT_ORIGIN)
			dOrgY = dNewOrgY;
		InvalidateRgn(NULL);
		UpdateCaretPosition();
		UpdateScrollbars(TRUE);
		NotifySlaveViews(flag);
	}
	else if (dNewOrgX != dOrgX || dNewOrgY != dOrgY)
	{
		MasterScrollTo(dNewOrgX, dNewOrgY);
		//NotifySlaveViews(flag);
	}
}

void CScaledScrollView::SetMaxExtents(double left, double right,
									double bottom, double top)
{
	// if the view is slave, these values will be ignored
	if (m_pHorMaster == m_pVertMaster)
	{
		m_pHorMaster->SetMaxExtentsMaster(left, right, bottom, top);
	}
	else
	{
		m_pHorMaster->SetMaxExtentsMaster(left, right, 0., 0.);
		m_pVertMaster->SetMaxExtentsMaster(0., 0., bottom, top);
	}
}

void CScaledScrollView::SetMaxExtentsMaster(double left, double right,
											double bottom, double top)
{
	DWORD flags = 0;
	if (left != right)
	{
		dMinLeft = left;
		dMaxRight = right;
		flags |= CHANGE_MAX_HOR_EXTENTS;
	}

	if (top != bottom)
	{
		dMinBottom = bottom;
		dMaxTop = top;
		flags |= CHANGE_MAX_VERT_EXTENTS;
	}

	ArrangeMaxExtents();
	if (flags != 0)
	{
		NotifySlaveViews(flags);
	}
	UpdateScrollbars();
}

void CScaledScrollView::ArrangeMaxExtents()
{
	double left = dMinLeft;
	double right = dMaxRight;
	double top = dMaxTop;
	double bottom = dMinBottom;

	if ((left < right) != (dExtX > 0.))
	{
		dMinLeft = right;
		dMaxRight = left;
	}
	if ((top > bottom) != (dExtY > 0.))
	{
		dMinBottom = top;
		dMaxTop = bottom;
	}
}

BOOL CScaledScrollView::ScrollBy(double dx, double dy, BOOL bDoScroll)
{
	if (m_pHorMaster == m_pVertMaster)
	{
		return m_pHorMaster->MasterScrollBy(dx, dy, bDoScroll);
	}
	return (0 != dx && m_pHorMaster->MasterScrollBy(dx, 0, bDoScroll))
		// use '|' rather than '||'. I want both calls
		| (0 != dy && m_pVertMaster->MasterScrollBy(0, dy, bDoScroll));
}

BOOL CScaledScrollView::MasterScrollBy(double dx, double dy, BOOL bDoScroll)
{
	// find world limits
	// check if the target is beyound the world limits
	double dNewOrgX = dOrgX + dx;
	// slave views are not checked against the world extent limits
	double minleft = dMinLeft;
	double maxright = dMaxRight;
	if ((minleft < dOrgX) != (dExtX > 0.))
	{
		minleft = dOrgX;
	}
	if ((maxright > (dOrgX + dExtX)) != (dExtX > 0.))
	{
		maxright = dOrgX + dExtX;
	}

	if ((dNewOrgX < minleft) == (dExtX > 0.))
	{
		dNewOrgX = minleft;
		dx = dNewOrgX - dOrgX;
	}
	else if ((dNewOrgX + dExtX > maxright) == (dExtX > 0.))
	{
		dNewOrgX = maxright - dExtX;
		dx = dNewOrgX - dOrgX;
	}

	// slave views are not checked against the world extent limits
	double dNewOrgY = dOrgY + dy;
	double minbottom = dMinBottom;
	double maxtop = dMaxTop;
	if ((maxtop > dOrgY) != (dExtY > 0.))
	{
		maxtop = dOrgY;
	}
	if ((minbottom < (dOrgY - dExtY)) != (dExtY > 0.))
	{
		minbottom = dOrgY - dExtY;
	}
	if ((dNewOrgY > maxtop) == (dExtY > 0.))
	{
		dNewOrgY = maxtop;
		dy = dNewOrgY - dOrgY;
	}
	else if ((dNewOrgY - dExtY < minbottom) == (dExtY > 0.))
	{
		dNewOrgY = minbottom + dExtY;
		dy = dNewOrgY - dOrgY;
	}
	// scroll the view or just invalidate it
	// move origin to integer count of device units
	DWORD flag = 0;
	int ddevx = fround(dx * dScaleX * dLogScaleX);
	if (ddevx != 0)
	{
		flag |= CHANGE_HOR_ORIGIN;
	}
	int ddevy = fround(dy * dScaleY * dLogScaleY);
	if (ddevy != 0)
	{
		flag |= CHANGE_VERT_ORIGIN;
	}
	if (fabs(dx) > (dSizeX * 0.89) || fabs(dy) > (dSizeY * 0.89))
	{
//        TRACE("Invalidate instead of scroll\n");
		// move origin to integer count of device units
		dOrgX += ddevx / (dScaleX * dLogScaleX);
		dOrgY += ddevy / (dScaleY * dLogScaleY);
		if (bDoScroll)
		{
			Invalidate();
		}
		UpdateScrollbars(bDoScroll);
		UpdateCaretPosition();
		NotifySlaveViews(flag);
		return TRUE; // view position changed
	}
	else
	{
		if (0) TRACE("Necessary window scroll=(%g, %g), new org=(%g, %g), devx=%d, devy=%d\n",
					dx, dy, dNewOrgX, dNewOrgY, ddevx, ddevy);

		if (ddevx | ddevy)
		{
			// the function scrolls the real image, and modifies dOrgX, dOrgY.
			OnScrollBy(CSize(ddevx, ddevy), bDoScroll);
			NotifySlaveViews(flag);
			return TRUE;
		}
		else
		{
			UpdateScrollbars(bDoScroll);
			UpdateCaretPosition();
			return FALSE;   // position is not changed
		}
	}
}

void CScaledScrollView::UpdateScrollbars(BOOL bRedraw)
{
	SCROLLINFO sci = {sizeof (SCROLLINFO),
					SIF_DISABLENOSCROLL | SIF_ALL,
					ScrollMin, ScrollMax, // scroll limits
					0, // page size (thumb size)
					0, // scroll pos
					0
	};
	// update horizontal scrollbar
	if (! IsSlaveHor()
		&& (0 != (GetStyle() & WS_HSCROLL)
			|| NULL != GetScrollBarCtrl(SB_HORZ)))
	{
		sci.nPage = int((sci.nMax - sci.nMin) * dExtX / (dMaxRight - dMinLeft));
		if (sci.nPage == 0) sci.nPage = 1;
		if (sci.nPage > unsigned(sci.nMax - sci.nMin))
			sci.nPage = sci.nMax - sci.nMin;
		if ((dOrgX < dMinLeft) == (dMinLeft < dMaxRight))
		{
			sci.nPos = sci.nMin;
		}
		else if ((dOrgX > (dMaxRight - dExtX)) == (dMinLeft < dMaxRight))
		{
			sci.nPos = sci.nMax - (sci.nPage - 1);
		}
		else
		{
			sci.nPos = int(sci.nMin + (sci.nMax - sci.nMin)
							* (dOrgX - dMinLeft) / (dMaxRight - dMinLeft));
			if (sci.nPos > (sci.nMax - int(sci.nPage - 1)))
				sci.nPos = sci.nMax - int(sci.nPage - 1);
		}
		if (0) TRACE("SetScrollInfo SB_HORZ, min=%d, max=%d, page=%d, pos=%d\n",
					sci.nMin, sci.nMax, sci.nPage, sci.nPos);
		SetScrollInfo(SB_HORZ, & sci, bRedraw);
	}

	// update vertical scrollbar
	if( ! IsSlaveVert()
		&& (0 != (GetStyle() & WS_VSCROLL)
			|| NULL != GetScrollBarCtrl(SB_VERT)))
	{
		sci.nPage = int((sci.nMax - sci.nMin)
						* dExtY / (dMaxTop - dMinBottom));
		if (sci.nPage == 0) sci.nPage = 1;
		if (sci.nPage > unsigned(sci.nMax - sci.nMin))
			sci.nPage = sci.nMax - sci.nMin;
		if ((dOrgY > dMaxTop) == (dMinBottom < dMaxTop))
		{
			sci.nPos = sci.nMin;
		}
		else if ((dOrgY < (dMinBottom + dExtY)) == (dMinBottom < dMaxTop))
		{
			sci.nPos = sci.nMax - (sci.nPage - 1);
		}
		else
		{
			sci.nPos = sci.nMin + int((sci.nMax - sci.nMin)
									* (dMaxTop - dOrgY) / (dMaxTop - dMinBottom));
			if (sci.nPos > (sci.nMax - int(sci.nPage - 1)))
				sci.nPos = sci.nMax - (sci.nPage - 1);
		}
		SetScrollInfo(SB_VERT, & sci, bRedraw);
	}
}

void CScaledScrollView::OnSize(UINT nType, int cx, int cy)
{
	CView::OnSize(nType, cx, cy);
	if (SIZE_MINIMIZED == nType)
		return;
	GetMappingInfo();
	DWORD flag = 0;
	// There are the following cases:
	// 1. Keep scale on resize only on X axis
	// 2. Keep scale on resize only on Y axis
	// 3. Keep scale on resize on both axis
	// 4. Do not keep scale on both axis, keep aspect ratio
	// 5. Do not keep scale on both axis, don't keep aspect ratio

	if (bKeepScaleOnResizeX && bKeepScaleOnResizeY)
	{
		double dCenterX = dOrgX;
		double dCenterY = dOrgY;
		if ( ! bKeepOrgOnResizeX)
		{
			dCenterX = dOrgX + dExtX * 0.5;
		}
		if ( ! bKeepOrgOnResizeY)
		{
			dCenterY = dOrgY - dExtY * 0.5;
		}
		if (cx > 0)
		{
			dExtX = cx / (dScaleX * dLogScaleX);
			dSizeX = fabs(dExtX);
			if ( ! bKeepOrgOnResizeX)
			{
				dCenterX -= dExtX * 0.5;
			}
			flag |= CHANGE_HOR_EXTENTS;
		}
		if (cy > 0)
		{
			dExtY = -cy / (dScaleY * dLogScaleY);
			dSizeY = fabs(dExtY);
			if ( ! bKeepOrgOnResizeY)
			{
				dCenterY -= dExtY * 0.5;
			}
			flag |= CHANGE_VERT_EXTENTS;
		}
		// move origin
		ScrollTo(dCenterX, dCenterY);
	}
	else if (bKeepScaleOnResizeX)
	{
		double dCenterX = dOrgX;
		if ( ! bKeepOrgOnResizeX)
		{
			dCenterX = dOrgX + dExtX * 0.5;
		}
		if (cx > 0)
		{
			dExtX = cx / (dScaleX * dLogScaleX);
			dSizeX = fabs(dExtX);
			if ( ! bKeepOrgOnResizeX)
			{
				dCenterX -= dExtX * 0.5;
			}
			flag |= CHANGE_HOR_EXTENTS;
			double dNewScaleY = -cy / (dExtY * dLogScaleY);
			AdjustNewScale(dScaleX, dScaleY, dScaleX, dNewScaleY);
			if (dNewScaleY != dScaleY)
			{
				if (0) TRACE("New Y scale != old scale, invalidating...\n");
				dScaleY = dNewScaleY;
				InvalidateRgn(NULL);
				//UpdateScrollbars();
			}
			// move origin
			ScrollTo(dCenterX, dOrgY);
		}
	}
	else if (bKeepScaleOnResizeY)
	{
		double dCenterY = dOrgY;
		if ( ! bKeepOrgOnResizeY)
		{
			dCenterY = dOrgY - dExtY * 0.5;
		}

		if (cy > 0)
		{
			dExtY = -cy / (dScaleY * dLogScaleY);
			dSizeY = fabs(dExtY);
			if ( ! bKeepOrgOnResizeY)
			{
				dCenterY -= dExtY * 0.5;
			}
			flag |= CHANGE_VERT_EXTENTS;
			double dNewScaleX = cx / (dExtX * dLogScaleX);
			AdjustNewScale(dScaleX, dScaleY, dNewScaleX, dScaleY);
			if (dNewScaleX != dScaleX)
			{
				if (0) TRACE("New X scale != old scale, invalidating...\n");
				dScaleX = dNewScaleX;
				InvalidateRgn(NULL);
				//UpdateScrollbars();
			}
			// move origin
			ScrollTo(dOrgX, dCenterY);
		}
	}
	else
	{
		flag = CHANGE_HOR_EXTENTS | CHANGE_VERT_EXTENTS;
		// compute new scale
		double dNewScaleX = dScaleX;
		double dNewScaleY = dScaleY;
		if (cx > 0)
		{
			dNewScaleX = cx / (dExtX * dLogScaleX);
		}

		// compute scale
		if (bKeepAspectRatio)
		{
			int sign = (dScaleY >= 0) ? 1 : -1;
			dNewScaleY = sign * fabs(dNewScaleX);
			AdjustNewScale(dScaleX, dScaleY, dNewScaleX, dNewScaleY);
			if (cy > 0)
			{
				dExtY = -cy / (dNewScaleY * dLogScaleY);
				dSizeY = fabs(dExtY);
			}
		}
		else if (cy > 0)
		{
			dNewScaleY = -cy / (dExtY * dLogScaleY);
			AdjustNewScale(dScaleX, dScaleY, dNewScaleX, dNewScaleY);
		}

		dScaleX = dNewScaleX;
		dScaleY = dNewScaleY;

		InvalidateRgn(NULL);
		UpdateCaretPosition();
	}
	UpdateScrollbars();
	NotifySlaveViews(flag);
}

int CScaledScrollView::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CView::OnCreate(lpCreateStruct) == -1)
		return -1;

	return GetMappingInfo();
}

int CScaledScrollView::GetMappingInfo()
{
	CClientDC wDC(this);
	wDC.SetMapMode(MM_TEXT);
	CSize wndext = wDC.GetWindowExt();
	CSize viewext = wDC.GetViewportExt();

	dLogScaleX = double(viewext.cx) / double(wndext.cx);
	dLogScaleY = double(viewext.cy) / double(wndext.cy);
	return 0;
}

// the function scrolls the real image, and modifies dOrgX, dOrgY.
BOOL CScaledScrollView::OnScrollBy(CSize sizeScroll, BOOL bDoScroll)
{
	if (bDoScroll)
	{
		RemoveSelectionRect();
	}
	dOrgX += sizeScroll.cx / (dScaleX * dLogScaleX);
	dOrgY += sizeScroll.cy / (dScaleY * dLogScaleY);
	if (bDoScroll)
	{
		if (0 != sizeScroll.cy)
		{
			ScrollWindow(0, -sizeScroll.cy);
		}
		if (0 != sizeScroll.cx)
		{
			ScrollWindow(-sizeScroll.cx, 0);
		}
		RestoreSelectionRect();
	}
	UpdateCaretPosition();
	UpdateScrollbars(bDoScroll);
	return TRUE;
}

void CScaledScrollView::OnViewZoomOut2()
{
	ZoomOut();
}

void CScaledScrollView::OnViewZoomin2()
{
	ZoomIn();
}

void CScaledScrollView::OnViewZoomOut()
{
	ZoomOutSelection(CHANGE_HOR_EXTENTS | CHANGE_VERT_EXTENTS);
}

void CScaledScrollView::ZoomOutSelection(DWORD flags)
{
	if (bHasSelection)
	{
		// current view boundaries should fit to
		// the selected area
		double left;
		double right;
		double top;
		double bottom;
		CRect Sel(DoubleToPoint(m_dXSelectionBegin, m_dYSelectionBegin),
				DoubleToPoint(m_dXSelectionEnd, m_dYSelectionEnd));
		Sel.NormalizeRect();
		CRect r;
		GetClientRect( & r);
		r.NormalizeRect();

		if ((flags & CHANGE_HOR_EXTENTS)
			&& r.left != r.right
			&& Sel.left != Sel.right)
		{
			left = m_pHorMaster->dOrgX
					+ (r.left - Sel.left) * m_pHorMaster->dExtX
					/ double(Sel.Width());
			right = left + m_pHorMaster->dExtX *
					double(r.Width()) / double(Sel.Width());
		}
		else
		{
			left = 0.;
			right = 0.;
		}

		if ((flags & CHANGE_VERT_EXTENTS)
			&& r.top != r.bottom
			&& Sel.top != Sel.bottom)
		{
			top = m_pVertMaster->dOrgY
				- m_pVertMaster->dExtY *
				(r.top - Sel.top) / double(Sel.Height());
			bottom = top - m_pVertMaster->dExtY *
					double(r.Height()) / double(Sel.Height());
		}
		else
		{
			top = 0.;
			bottom = 0.;
		}
		CancelSelection();
		SetExtents(left, right, bottom, top);
	}
	else
	{
		double zoomX = 1.;
		if (flags & CHANGE_HOR_EXTENTS)
		{
			zoomX = 0.5;
		}
		double zoomY = 1.;
		if (flags & CHANGE_VERT_EXTENTS)
		{
			zoomY = 0.5;
		}

		Zoom(zoomX, zoomY, GetZoomCenter());
	}
}

POINT CScaledScrollView::GetZoomCenter()
{
	CPoint p(DoubleToPoint(m_dXEndTracking,
							m_dYEndTracking));
	CRect r;
	GetClientRect( & r);
	if ( ! r.PtInRect(p))
	{
		return CPoint(INT_MAX, INT_MAX);
	}
	else
		return p;
}

void CScaledScrollView::OnViewZoomin()
{
	ZoomInSelection(CHANGE_HOR_EXTENTS | CHANGE_VERT_EXTENTS);
}

void CScaledScrollView::ZoomInSelection(DWORD flags)
{
	if (bHasSelection)
	{
		double left = m_dXSelectionBegin;
		double right = m_dXSelectionEnd;
		double top = m_dYSelectionBegin;
		double bottom = m_dYSelectionEnd;
		CPoint lt = DoubleToPointDev(left, top);
		CPoint rb = DoubleToPointDev(right, bottom);
		if (lt.x > rb.x)
		{
			right = m_dXSelectionBegin;
			left = m_dXSelectionEnd;
		}
		if (0 == (flags & CHANGE_HOR_EXTENTS))
		{
			right = 0.;
			left = 0;
		}
		if (lt.y > rb.y)
		{
			bottom = m_dYSelectionBegin;
			top = m_dYSelectionEnd;
		}
		if (0 == (flags & CHANGE_VERT_EXTENTS))
		{
			top = 0.;
			bottom = 0.;
		}
		CancelSelection();
		SetExtents(left, right, bottom, top);
	}
	else
	{
		double zoomX = 1.;
		if (flags & CHANGE_HOR_EXTENTS)
		{
			zoomX = 2.;
		}
		double zoomY = 1.;
		if (flags & CHANGE_VERT_EXTENTS)
		{
			zoomY = 2.;
		}

		Zoom(zoomX, zoomY, GetZoomCenter());
	}
}
void CScaledScrollView::OnUpdateIndicatorMousePosition(CCmdUI* pCmdUI)
{
	CPoint p;
	GetCursorPos( & p);
	ScreenToClient( & p);
	CRect r;
	GetClientRect(& r);
	r.NormalizeRect();
	double x, y;
	if (r.PtInRect(p))
	{
		pCmdUI->Enable(TRUE);
		PointToDoubleDev(p, x, y);
		CString s(GetString(x, y));
		if(pCmdUI->m_pOther != NULL
			&& pCmdUI->m_pOther->IsKindOf(RUNTIME_CLASS(CStatusBar)))
		{
			// Set pane size
			CStatusBar *pSB = DYNAMIC_DOWNCAST(CStatusBar,
												pCmdUI->m_pOther);
			CSize size;
			{
				CWindowDC dc(pSB);
				CFont * pFont = pSB->GetFont();
				VERIFY(pFont);

				CGdiObjectSaveT<CFont> OldObj(dc, dc.SelectObject(pFont));

				VERIFY(::GetTextExtentPoint32(dc,
											s, s.GetLength(), & size));
			}
			UINT nID, nStyle;
			int cxWidth;
			pSB->GetPaneInfo(pCmdUI->m_nIndex, nID, nStyle,
							cxWidth);
			if (size.cx > cxWidth)
			{
				pSB->SetPaneInfo(pCmdUI->m_nIndex, nID,
								nStyle, size.cx);
			}
		}
		pCmdUI->SetText(s);
	}
	else
	{
		pCmdUI->ContinueRouting();
		//pCmdUI->Enable(FALSE);
	}
}

CString CScaledScrollView::GetString(double x, double y)
{
	CString s;
	TCHAR szBufferX[32];
	_stprintf(szBufferX, _T("%f"), x);
	TCHAR szBufferY[32];
	_stprintf(szBufferY, _T("%f"), y);
	s.Format(GetFormatStringID(), szBufferX, szBufferY);
	return s;
}

UINT CScaledScrollView::GetFormatStringID() const
{
	ASSERT(FALSE);
	return 0;
}

void CScaledScrollView::OnInitialUpdate()
{
	CView::OnInitialUpdate();

	UpdateScrollbars(TRUE);
}

void CScaledScrollView::OnViewZoominHor2()
{
	Zoom(2., 1., GetZoomCenter());
}

void CScaledScrollView::OnViewZoominHor()
{
	ZoomInSelection(CHANGE_HOR_EXTENTS);
}

void CScaledScrollView::OnViewZoomInVert2()
{
	Zoom(1., 2., GetZoomCenter());
}

void CScaledScrollView::OnViewZoomInVert()
{
	ZoomInSelection(CHANGE_VERT_EXTENTS);
}

void CScaledScrollView::OnViewZoomOutHor2()
{
	Zoom(0.5, 1., GetZoomCenter());
}

void CScaledScrollView::OnViewZoomOutHor()
{
	ZoomOutSelection(CHANGE_HOR_EXTENTS);
}

void CScaledScrollView::OnViewZoomOutVert2()
{
	Zoom(1., 0.5, GetZoomCenter());
}

void CScaledScrollView::OnViewZoomOutVert()
{
	ZoomOutSelection(CHANGE_VERT_EXTENTS);
}

UINT CScaledScrollView::GetPopupMenuID(CPoint)
{
	return IDR_POPUP_SCALED_SCROLL_VIEW;
}

void CScaledScrollView::OnContextMenu(CWnd* pWnd, CPoint point)
{
	// make sure window is active
	GetParentFrame()->ActivateFrame();

	CMenu menu;
	CMenu* pPopup = GetPopupMenu( & menu, point);
	if(pPopup != NULL)
	{
		pPopup->TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON,
								point.x, point.y,
								AfxGetMainWnd()); // use main window for cmds
	}
}


BOOL CScaledScrollView::SyncHorizontal(CScaledScrollView * pSView)
{
	if (pSView == this)
	{
		UnsyncHorizontal();
		return TRUE;
	}

	ASSERT_VALID(this);
	ASSERT_VALID(pSView);
	ASSERT(GetParent() == pSView->GetParent());
	m_pHorMaster = pSView;
	// hide scrollbar
	if (GetStyle() & WS_HSCROLL)
	{
		ShowScrollBar(SB_HORZ, FALSE);
	}
	// sync origin and extents with the master
	SetMaxExtentsMaster(pSView->dMinLeft, pSView->dMaxRight, 0., 0.);
	OnMasterChangeOrgExt(pSView->dOrgX, pSView->dExtX,
						dOrgY, dExtY, CHANGE_HOR_EXTENTS);
	return TRUE;
}

void CScaledScrollView::UnsyncHorizontal()
{
	ASSERT_VALID(this);
	// copy extents from master view
	dOrgX = m_pHorMaster->dOrgX;
	dExtX = m_pHorMaster->dExtX;
	dSizeX = m_pHorMaster->dSizeX;
	dMinLeft = m_pHorMaster->dMinLeft;
	dMaxRight = m_pHorMaster->dMaxRight;
	// unattach
	m_pHorMaster = this;
	if (GetStyle() & WS_HSCROLL)
	{
		ShowScrollBar(SB_HORZ, TRUE);
		RECT r;
		GetClientRect(&r);
		// may need to send WM_SIZE
	}
}

BOOL CScaledScrollView::SyncVertical(CScaledScrollView * pSView)
{
	if (pSView == this)
	{
		UnsyncVertical();
		return TRUE;
	}
	ASSERT_VALID(this);
	ASSERT_VALID(pSView);
	ASSERT(GetParent() == pSView->GetParent());
	m_pVertMaster = pSView;
	// hide scrollbar
	if (GetStyle() & WS_VSCROLL)
	{
		ShowScrollBar(SB_VERT, FALSE);
	}
	SetMaxExtentsMaster(0., 0., pSView->dMinBottom, pSView->dMaxTop);
	OnMasterChangeOrgExt(dOrgX, dExtX,
						pSView->dOrgY, pSView->dExtY, CHANGE_VERT_EXTENTS);
	return TRUE;
}

void CScaledScrollView::UnsyncVertical()
{
	ASSERT_VALID(this);
	// copy extents from master view
	dOrgY = m_pVertMaster->dOrgY;
	dExtY = m_pVertMaster->dExtY;
	dSizeY = m_pVertMaster->dSizeY;
	dMinBottom = m_pVertMaster->dMinBottom;
	dMaxTop = m_pVertMaster->dMaxTop;
	// unattach
	m_pVertMaster = this;
	if (GetStyle() & WS_VSCROLL)
	{
		ShowScrollBar(SB_VERT, TRUE);
		RECT r;
		GetClientRect(&r);
		// may need to send WM_SIZE
	}
}

void CScaledScrollView::OnDestroy()
{
	UnsyncHorizontal();
	UnsyncVertical();
	// deattach all other views
	CWnd * pWnd = GetParent()->GetTopWindow();
	while (pWnd != NULL)
	{
		if (this != pWnd && pWnd->IsKindOf(RUNTIME_CLASS(CScaledScrollView)))
		{
			CScaledScrollView * pView = (CScaledScrollView *) pWnd;
			if (pView->m_pVertMaster == this)
			{
				pView->UnsyncVertical();
			}
			if (pView->m_pHorMaster == this)
			{
				pView->UnsyncHorizontal();
			}
		}
		pWnd = pWnd->GetNextWindow();
	}

	CView::OnDestroy();
}

void CScaledScrollView::NotifySlaveViews(DWORD flag)
{
	CWnd * pWnd = GetParent()->GetTopWindow();
	while (pWnd != NULL)
	{
		if (this != pWnd && pWnd->IsKindOf(RUNTIME_CLASS(CScaledScrollView)))
		{
			CScaledScrollView * pView = (CScaledScrollView *) pWnd;
			if (pView->m_pVertMaster == this
				|| pView->m_pHorMaster == this)
			{
				DWORD flag1 = flag & 0xFFFF;
				double MinX = dMinLeft;
				double MaxX = dMaxRight;
				double MinY = dMinBottom;
				double MaxY = dMaxTop;
				if (pView->m_pVertMaster != this)
				{
					flag1 &= ~(CHANGE_VERT_EXTENTS | CHANGE_MAX_VERT_EXTENTS);
					MinY = 0.;
					MaxY = 0.;
				}
				if (pView->m_pHorMaster != this)
				{
					flag1 &= ~(CHANGE_HOR_EXTENTS | CHANGE_MAX_HOR_EXTENTS);
					MinX = 0.;
					MaxX = 0.;
				}
				if (flag1 & (CHANGE_MAX_VERT_EXTENTS | CHANGE_MAX_HOR_EXTENTS))
				{
					pView->SetMaxExtentsMaster(MinX, MaxX, MinY, MaxY);
				}
				if (flag1 & (CHANGE_VERT_EXTENTS | CHANGE_HOR_EXTENTS))
				{
					pView->OnMasterChangeOrgExt(dOrgX, dExtX,
												dOrgY, dExtY, flag1);
				}
				if (flag & 0xFFFF0000)
				{
					pView->OnUpdate(this, flag & 0xFFFF0000, NULL);
				}
			}
		}
		pWnd = pWnd->GetNextWindow();
	}
}

// synced ScaledScrollView's maintain common size (current
// and maximum) and scroll position. When master view
// changes its world size and/or origin, it notifies slave views.
// When slave view needs to change its state, it forwards
// the request to the master view.
// Slave view ignores scrollbar messages. It should be created
// without a scrollbar.

void CScaledScrollView::OnLButtonDown(UINT nFlags, CPoint point)
{

	CView::OnLButtonDown(nFlags, point);
	nKeyPressed = WM_LBUTTONDOWN;
	if ((nFlags & MK_SHIFT) == 0)
	{
		// find the click point
		PointToDoubleDev(point,
						m_dXStartTracking, m_dYStartTracking);
		CancelSelection();
	}
	else
	{
		OnMouseMove(nFlags, point);
	}
}

void CScaledScrollView::OnRButtonDown(UINT nFlags, CPoint point)
{

	CView::OnRButtonDown(nFlags, point);
	// find the click point
	if ((nFlags & MK_SHIFT) == 0)
	{
		PointToDoubleDev(point,
						m_dXStartTracking, m_dYStartTracking);
	}
	nKeyPressed = WM_RBUTTONDOWN;
}

void CScaledScrollView::OnRButtonUp(UINT nFlags, CPoint point)
{
	// find the click point
	PointToDoubleDev(point,
					m_dXEndTracking, m_dYEndTracking);
	if (bIsTrackingSelection)
	{
		bHasSelection = TRUE;
	}
	ReleaseCapture();
	bIsTrackingSelection = FALSE;
	nKeyPressed = 0;

	CView::OnRButtonUp(nFlags, point);
}

void CScaledScrollView::OnLButtonUp(UINT nFlags, CPoint point)
{

	PointToDoubleDev(point,
					m_dXEndTracking, m_dYEndTracking);
	if (bIsTrackingSelection)
	{
		bHasSelection = TRUE;
	}
	ReleaseCapture();
	bIsTrackingSelection = FALSE;
	nKeyPressed = 0;

	CView::OnLButtonUp(nFlags, point);
}

CMenu * CScaledScrollView::GetPopupMenu(CMenu * pMenu, CPoint point)
{
	UINT uID = GetPopupMenuID(point);
	if (uID != 0 && pMenu->LoadMenu(uID))
	{
		return ModifyPopupMenu(pMenu->GetSubMenu(0));
	}
	else
	{
		return NULL;
	}
}

void CScaledScrollView::RecalcScale()
{
	// recalculate dScaleX, dScaleY, dLogScaleX, dLogScaleY
	// invalidate the view, if any value changed
	ASSERT(FALSE);
}


CMenu * CScaledScrollView::ModifyPopupMenu(CMenu * pMenu)
{
#if 0
	// add Zoom in/out items
	CString s;
	if (pMenu->GetMenuString(ID_VIEW_ZOOMIN, s, MF_BYCOMMAND) == 0
		&& pMenu->GetMenuString(ID_VIEW_ZOOMOUT, s, MF_BYCOMMAND) == 0)
	{
		CMenu menu1;
		CMenu * pSubMenu;
		if (menu1.LoadMenu(IDR_POPUP_SCALED_SCROLL_VIEW)
			&& NULL != (pSubMenu = menu1.GetSubMenu(0))
		{
			pMenu->AppendMenu(MF_SEPARATOR);
		}
	}
#endif
	return pMenu;
}

void CScaledScrollView::OnMouseMove(UINT nFlags, CPoint point)
{
	CView::OnMouseMove(nFlags, point);
	if (nKeyPressed != 0)
	{
		PointToDoubleDev(point,
						m_dXEndTracking, m_dYEndTracking);
		if (bIsTrackingSelection)
		{
		}
		else
		{
			CancelSelection();
			bIsTrackingSelection = TRUE;
			SetCapture();
		}
		ChangeSelection(m_dXStartTracking, m_dXEndTracking,
						m_dYStartTracking, m_dYEndTracking);
	}
}

void CScaledScrollView::OnCaptureChanged( CWnd* pWnd )
{
	if (pWnd != this)
	{
		bIsTrackingSelection = FALSE;
		nKeyPressed = 0;
	}
	CView::OnCaptureChanged(pWnd);
}

void CScaledScrollView::DrawSelectionRect(CDC * pDC,
										double left, double right, double bottom, double top)
{
	CDC * pDrawDC = pDC;
	if (NULL == pDrawDC)
	{
		pDrawDC = GetDC();
		if (NULL == pDrawDC)
		{
			return;
		}
		pDrawDC->ExcludeUpdateRgn(this);
	}
	CRect r(DoubleToPoint(left, bottom),
			DoubleToPoint(right, top));
	r.NormalizeRect();
	pDrawDC->DrawFocusRect( & r);
	if (NULL == pDC)
	{
		ReleaseDC(pDrawDC);
	}
}

void CScaledScrollView::RemoveSelectionRect()
{
	if (bSelRectDrawn)
	{
		DrawSelectionRect(NULL, m_dXSelectionBegin, m_dXSelectionEnd,
						m_dYSelectionBegin, m_dYSelectionEnd);
		bSelRectDrawn = FALSE;
	}
}

void CScaledScrollView::RestoreSelectionRect()
{
	if (bHasSelection && !bSelRectDrawn)
	{
		DrawSelectionRect(NULL, m_dXSelectionBegin, m_dXSelectionEnd,
						m_dYSelectionBegin, m_dYSelectionEnd);
		bSelRectDrawn = TRUE;
	}
}

void CScaledScrollView::ChangeSelection(double left, double right,
										double bottom, double top)
{
	RemoveSelectionRect();
	m_dXSelectionBegin = left;
	m_dXSelectionEnd = right;
	m_dYSelectionBegin = top;
	m_dYSelectionEnd = bottom;
	if (left != right && top != bottom)
	{
		bHasSelection = TRUE;
		RestoreSelectionRect();
	}
	else
	{
		bHasSelection = FALSE;
	}
}

void CScaledScrollView::CancelSelection()
{
	RemoveSelectionRect();
	bHasSelection = FALSE;
}

BOOL CScaledScrollView::OnNeedText( UINT id, NMHDR * pNotifyStruct, LRESULT * result )
{
	LPTOOLTIPTEXT lpttt = (LPTOOLTIPTEXT) pNotifyStruct;
	if (lpttt != NULL)
	{
		lpttt->hinst = NULL;
		_tcscpy(lpttt->szText, _T(" "));
		*result = 0;
		return TRUE;    // the message is handled
	}
	return FALSE;
}

BOOL CScaledScrollView::OnNotify(WPARAM wParam, LPARAM lParam, LRESULT* pResult)
{
	NMHDR * pNotify = (NMHDR *) lParam;
	if (pNotify != NULL && TTN_NEEDTEXTA == pNotify->code)
	{
		OnNeedText(pNotify->idFrom, pNotify, pResult);
		* pResult = 0;
		return TRUE;
	}
	return CView::OnNotify(wParam, lParam, pResult);
}

int CScaledScrollView::OnToolHitTest( CPoint point, TOOLINFO* pTI ) const
{
	if (pTI != NULL && pTI->cbSize >= sizeof(TOOLINFO))
	{
		CRect r;
		GetClientRect( & r);
		if ( ! r.PtInRect(point))
			return -1;
		// setup the TOOLINFO structure
		pTI->hwnd = m_hWnd;
		//pTI->uId = (UINT)m_hWnd;
		//pTI->uFlags |= TTF_IDISHWND;
		pTI->uId = (point.x & 0xFFFF) | (point.y << 16);
		pTI->uFlags &= ~TTF_IDISHWND;

		pTI->rect = CRect(point, point);
		pTI->rect.bottom ++;
		pTI->rect.right ++;
		pTI->rect.top --;
		pTI->rect.left --;

		pTI->lpszText = LPSTR_TEXTCALLBACK;

		pTI->uFlags |= TTF_NOTBUTTON;
		return pTI->uId;
	}
	return -1;
}


