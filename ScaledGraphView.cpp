// ScaledGraphView.cpp
#include "stdafx.h"
#include "ScaledGraphView.h"
#include <math.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CScaledGraphView view

IMPLEMENT_DYNAMIC(CScaledGraphView,CScaledScrollView);

BEGIN_MESSAGE_MAP(CScaledGraphView, CScaledScrollView)
	//{{AFX_MSG_MAP(CScaledGraphView)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

CScaledGraphView::CScaledGraphView()
	:m_dwGraphStyle(SGV_STYLE_SOLID | SGV_STYLE_TICKS),
	m_dXTicks(0),
	m_dYTicks(0),
	m_MinXTickSpacing(10),
	m_MinYTickSpacing(10),
	m_MaxXTickSpacing(10000),
	m_MaxYTickSpacing(10000),
	m_Color(RGB(0, 0, 0))
{
}

void CScaledGraphView::SetXTicks(double dTickStep)
{
	m_dXTicks = dTickStep;
	if (m_dwGraphStyle & SGV_STYLE_TICKS)
		Invalidate();
}

void CScaledGraphView::SetYTicks(double dTickStep)
{
	m_dYTicks = dTickStep;
	if (m_dwGraphStyle & SGV_STYLE_TICKS)
		Invalidate();
}

void CScaledGraphView::SetMinXTickSpacing(int nTickSpacing)
{
	m_MinXTickSpacing = nTickSpacing;
	if (m_dwGraphStyle & SGV_STYLE_TICKS)
		Invalidate();
}

void CScaledGraphView::SetMinYTickSpacing(int nTickSpacing)
{
	m_MinYTickSpacing = nTickSpacing;
	if (m_dwGraphStyle & SGV_STYLE_TICKS)
		Invalidate();
}

void CScaledGraphView::SetMaxXTickSpacing(int nTickSpacing)
{
	m_MaxXTickSpacing = nTickSpacing;
	if (m_dwGraphStyle & SGV_STYLE_TICKS)
		Invalidate();
}

void CScaledGraphView::SetMaxYTickSpacing(int nTickSpacing)
{
	m_MaxYTickSpacing = nTickSpacing;
	if (m_dwGraphStyle & SGV_STYLE_TICKS)
		Invalidate();
}

void CScaledGraphView::SetGraphStyle(DWORD Style)
{
	m_dwGraphStyle = Style;
	Invalidate();
}

void CScaledGraphView::SetGraphColor(COLORREF color)
{
	m_Color = color;
	Invalidate();
}

double CScaledGraphView::WrapValue(double arg, double dMin, double dMax)
{
	return arg; // default does not change the argument
}

int CScaledGraphView::PrepareData(double dXBegin, double dXEnd, int nClientWidth)
{
	// default does nothing
	return nClientWidth;
}

void CScaledGraphView::UnprepareData()
{
	// default does nothing
}

double CScaledGraphView::GetLeftLimit()
{
	return dOrgX;
}

double CScaledGraphView::GetRightLimit()
{
	return dOrgX + dExtX;
}

int CScaledGraphView::GetNumberOfGraphs()
{
	return 1;
}

COLORREF CScaledGraphView::GetGraphColor(int )
{
	return m_Color;
}

void CScaledGraphView::OnDraw(CDC * pDC)
{
//    DWORD bkcolor = GetSysColor(COLOR_WINDOW);
//    DWORD
	CPen BlackPen;
	CPen GrayPen;
	CPen GrayDottedPen;
	if (pDC->GetDeviceCaps(TECHNOLOGY) == DT_RASDISPLAY)
	{
		COLORREF crText = m_Color;
		COLORREF crBackground = GetSysColor(COLOR_WINDOW);
		COLORREF crGray = RGB(
							(GetRValue(crText) + GetRValue(crBackground)) / 2,
							(GetGValue(crText) + GetGValue(crBackground)) / 2,
							(GetBValue(crText) + GetBValue(crBackground)) / 2);
		COLORREF crLtGray = RGB(
								(GetRValue(crText) + GetRValue(crBackground) * 2) / 3,
								(GetGValue(crText) + GetGValue(crBackground) * 2) / 3,
								(GetBValue(crText) + GetBValue(crBackground) * 2) / 3);
		if (pDC->GetDeviceCaps(BITSPIXEL) < 8)
		{
			GrayPen.CreatePen(PS_DASH, 1, crText);
			BlackPen.CreatePen(PS_SOLID, 1, crText);
			GrayDottedPen.CreatePen(PS_DOT, 1, crText);
		}
		else
		{
			GrayPen.CreatePen(PS_SOLID, 1, crLtGray);
			BlackPen.CreatePen(PS_SOLID, 1, crText);
			GrayDottedPen.CreatePen(PS_DOT, 1, crGray);
		}
	}
	else
	{
		// all pens are black
		GrayPen.CreatePen(PS_DASH, 1, RGB(0, 0, 0));
		BlackPen.CreatePen(PS_SOLID, 1, RGB(0, 0, 0));
		GrayDottedPen.CreatePen(PS_DOT, 1, RGB(0, 0, 0));
	}
	CGdiObject * pOldPen = pDC->SelectObject(& GrayPen);
	RECT r;

	GetClientRect(&r);
	if (!(m_dwGraphStyle & SGV_STYLE_UPDATE_ALL)
		&& pDC->IsKindOf(RUNTIME_CLASS(CPaintDC)))
	{
		RECT r_upd = ((CPaintDC*)pDC)->m_ps.rcPaint;
		// make intersect by x coordinate
		if (r.left < r_upd.left) r.left = r_upd.left;
		if (r.right > r_upd.right) r.right = r_upd.right;
	}

	r.left -= 2;
	r.right += 2;
	r.top -= 2;
	r.bottom += 2;
	int iClientWidth = r.right - r.left;
	double x, y;
	double left, right, top, bottom;
	PointToDoubleDev(CPoint(r.left, r.top), left, top);
	PointToDoubleDev(CPoint(r.right, r.bottom), right, bottom);
	// TODO: add draw code here
	if (left > right)
	{
		double tmp = left;
		left = right;
		right = tmp;
	}
	if (top < bottom)
	{
		double tmp = top;
		top = bottom;
		bottom = tmp;
	}
	// draw Y=0 line
	if (m_dwGraphStyle & SGV_STYLE_X_AXIS
		&&top > 0 && bottom < 0.)
	{
		pDC->MoveTo(DoubleToPoint(left, 0.));
		pDC->LineTo(DoubleToPoint(right, 0.));
	}

	// draw X=0 line
	if (m_dwGraphStyle & SGV_STYLE_Y_AXIS
		&& left < 0 && right > 0.)
	{
		pDC->MoveTo(DoubleToPoint(0., top));
		pDC->LineTo(DoubleToPoint(0., bottom));
	}

	// select a pen for drawing the grid
	pDC->SelectObject(& GrayDottedPen);

	if (m_dwGraphStyle & SGV_STYLE_TICKS
		&& m_dXTicks > 0.)
	{
		// find tick interval. Miltiply the step to
		// factors in 1, 2, 5, 10... sequence until tick spacing
		// is >= m_TickSpacing
		double TickSpacing = m_dXTicks * fabs(GetXScaleDev());
		double multiplier1 = 1.;
		int multiplier2 = 1;
		while (int(TickSpacing * multiplier1 * multiplier2)
				< m_MinXTickSpacing)
		{
			switch(multiplier2)
			{
			case 1:
				multiplier2 = 2;
				break;
			case 2:
				multiplier2 = 5;
				break;
			case 5:
				multiplier2 = 1;
				multiplier1 *= 10;
				break;
			default:
				ASSERT(FALSE);
			}
		}

		if (m_MaxXTickSpacing > 0)
		{
			while(int(TickSpacing * multiplier1 * multiplier2)
				> m_MaxXTickSpacing)
			{
				switch(multiplier2)
				{
				case 1:
					multiplier2 = 5;
					multiplier1 /= 10;
					break;
				case 2:
					multiplier2 = 1;
					break;
				case 5:
					multiplier2 = 2;
					break;
				default:
					ASSERT(FALSE);
				}
			}
		}

		TickSpacing = m_dXTicks * multiplier1 * multiplier2;

		double nx = floor (left / TickSpacing);
		for (; TickSpacing * nx < right; nx += 1)
		{
			if (m_dwGraphStyle & SGV_STYLE_Y_AXIS
				&& 0. == nx)
				continue;   // don't draw X=0 line
			x = TickSpacing * nx;
			if (x >= left)
			{
				pDC->MoveTo(DoubleToPoint(x, top));
				pDC->LineTo(DoubleToPoint(x, bottom));
			}
		}
	}

	if (m_dwGraphStyle & SGV_STYLE_TICKS
		&& m_dYTicks > 0.)
	{
		// find tick interval. Miltiply the step to
		// factors in 1, 2, 5, 10... sequence until tick spacing
		// is >= m_TickSpacing
		double TickSpacing = m_dYTicks * fabs(GetYScaleDev());
		double multiplier1 = 1.;
		int multiplier2 = 1;
		while (int(TickSpacing * multiplier1 * multiplier2)
				< m_MinYTickSpacing)
		{
			switch(multiplier2)
			{
			case 1:
				multiplier2 = 2;
				break;
			case 2:
				multiplier2 = 5;
				break;
			case 5:
				multiplier2 = 1;
				multiplier1 *= 10;
				break;
			default:
				ASSERT(FALSE);
			}
		}

		if (m_MaxYTickSpacing > 0)
		{
			while(int(TickSpacing * multiplier1 * multiplier2)
				> m_MaxYTickSpacing)
			{
				switch(multiplier2)
				{
				case 1:
					multiplier2 = 5;
					multiplier1 /= 10;
					break;
				case 2:
					multiplier2 = 1;
					break;
				case 5:
					multiplier2 = 2;
					break;
				default:
					ASSERT(FALSE);
				}
			}
		}

		TickSpacing = m_dYTicks * multiplier1 * multiplier2;

		double ny = floor (bottom / TickSpacing);
		for (; TickSpacing * ny < top; ny += 1)
		{
			if (m_dwGraphStyle & SGV_STYLE_X_AXIS
				&& 0. == ny)
				continue;   // don't draw Y=0 line
			y = TickSpacing * ny;
			if (y >= bottom)
			{
				pDC->MoveTo(DoubleToPoint(left, y));
				pDC->LineTo(DoubleToPoint(right, y));
			}
		}
	}

	// draw the graph
	// create an array of points (up to 500 for client area)
	// let the derived class prepare the data
	int nPointsToDraw = PrepareData(left, right, iClientWidth);
	double top_limit =  top + 2 * (top - bottom);
	double bottom_limit =  bottom - 2 * (top - bottom);
	if (nPointsToDraw > 0)
	{
		POINT * ppArray = new POINT[nPointsToDraw];
		if (ppArray != NULL)
		{
			for (int nGraph = 0; nGraph < GetNumberOfGraphs(); nGraph ++)
			{
				if (pDC->GetDeviceCaps(TECHNOLOGY) != DT_RASDISPLAY)
				{
					BlackPen.DeleteObject();
					BlackPen.CreatePen(PS_SOLID, 1, GetGraphColor(nGraph));
				}
				pDC->SelectObject(& BlackPen);
				int i;
				for (i = 0; i < nPointsToDraw; i++)
				{
					ppArray[i].x = i * iClientWidth / nPointsToDraw + r.left;
					ppArray[i].y = 0;
					PointToDoubleDev(ppArray[i], x, y);
					// request the value from the derived class
					double y = Evaluate(x, i, nGraph);
					if (y > top_limit)
					{
						y = top_limit;
					}
					if (y < bottom_limit)
					{
						y = bottom_limit;
					}
					ppArray[i] = DoubleToPoint(x, y);
				}
				// draw by 256 points
				for (i = 0; i < nPointsToDraw; i += 255)
				{
					int iCount = iClientWidth - i;
					if (iCount > 256) iCount = 256;
					pDC->Polyline(ppArray+i, iCount);
				}
				pDC->SelectObject(pOldPen);
			}
		}
		delete ppArray;
	}
	pDC->SelectObject(pOldPen);
	PostDraw(pDC);
	UnprepareData();
	CScaledScrollView::OnDraw(pDC);
}

void CScaledGraphView::PostDraw(CDC *)
{
}

BOOL CScaledGraphView::OnNeedText( UINT id, NMHDR * pNotifyStruct, LRESULT * result )
{
	LPTOOLTIPTEXT lpttt = (LPTOOLTIPTEXT) pNotifyStruct;
	if (lpttt != NULL)
	{
		lpttt->hinst = NULL;
		// format tooltip text
		double x, y;
		CPoint p(pNotifyStruct->idFrom & 0xFFFF,
				(pNotifyStruct->idFrom >> 16) & 0xFFFF);
		PointToDoubleDev(p, x, y);
		PrepareData(x, x, 1);   // Prepare output data
		CString s(GetString(x, Evaluate(x, 0)));
		strncpy(lpttt->szText, s, sizeof lpttt->szText);
		lpttt->szText[sizeof lpttt->szText - 1] = 0;
		*result = 0;
		UnprepareData();    // free resources allocated by PrepareData();
		return TRUE;    // the message is handled
	}
	return FALSE;
}

double CScaledGraphView::Evaluate(double arg, int nIndex, int nGraph)
{
	return Evaluate(arg, nIndex);
}

double CScaledGraphView::Evaluate(double , int )
{
	return 0.;
}
