// Copyright Alexander Grigoriev, 1997-2002, All Rights Reserved
// TimeRulerView.cpp : implementation file
//

#include "stdafx.h"
#include "WaveSoapFront.h"
#include "TimeRulerView.h"
#include "WaveSoapFrontView.h"
#include "WaveSoapFrontDoc.h"
#include "GdiObjectSave.h"
#include "TimeToStr.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define TRACE_SCROLL 0

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

/////////////////////////////////////////////////////////////////////////////
// CTimeRulerView

IMPLEMENT_DYNCREATE(CTimeRulerView, CHorizontalRuler)

CTimeRulerView::CTimeRulerView()
	: m_CurrentDisplayMode(ShowHhMmSs)
	, m_DraggedMarkerHitTest(0)
	, m_AutoscrollTimerID(0)
	, m_MarkerHeight(10)
{
}

CTimeRulerView::~CTimeRulerView()
{
}


BEGIN_MESSAGE_MAP(CTimeRulerView, CHorizontalRuler)
	//{{AFX_MSG_MAP(CTimeRulerView)
	ON_COMMAND(IDC_VIEW_RULER_HHMMSS, OnViewRulerHhmmss)
	ON_UPDATE_COMMAND_UI(IDC_VIEW_RULER_HHMMSS, OnUpdateViewRulerHhmmss)
	ON_COMMAND(IDC_VIEW_RULER_SAMPLES, OnViewRulerSamples)
	ON_UPDATE_COMMAND_UI(IDC_VIEW_RULER_SAMPLES, OnUpdateViewRulerSamples)
	ON_COMMAND(IDC_VIEW_RULER_SECONDS, OnViewRulerSeconds)
	ON_UPDATE_COMMAND_UI(IDC_VIEW_RULER_SECONDS, OnUpdateViewRulerSeconds)
	//}}AFX_MSG_MAP
	ON_WM_SETCURSOR()
	ON_NOTIFY_RANGE(TTN_NEEDTEXTW, 0, 0xFFFF, OnToolTipText)
	ON_NOTIFY_RANGE(TTN_NEEDTEXTA, 0, 0xFFFF, OnToolTipText)
	ON_WM_LBUTTONDBLCLK()
	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONUP()
	ON_WM_MOUSEMOVE()
	ON_WM_CAPTURECHANGED()
	ON_WM_TIMER()
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CTimeRulerView drawing

void CTimeRulerView::OnDraw(CDC* pDC)
{
	CWaveSoapFrontDoc* pDoc = GetDocument();
	if (! pDoc->m_WavFile.IsOpen())
	{
		return;
	}
	// background is erased by gray brush.
	// draw horizontal line with ticks and numbers
	CGdiObjectSave OldFont(pDC, pDC->SelectStockObject(ANSI_VAR_FONT));

	TEXTMETRIC tm;
	pDC->GetTextMetrics( & tm);

	m_MarkerHeight = tm.tmAveCharWidth;

	CPen DarkGrayPen(PS_SOLID, 0, 0x808080);
	CRect cr;
	GetClientRect(cr);

	CPaintDC * pPaintDC = dynamic_cast<CPaintDC *>(pDC);
	if (NULL != pPaintDC)
	{
		cr = pPaintDC->m_ps.rcPaint;
	}

	int const RulerBase = cr.bottom - m_MarkerHeight - 2;

	pDC->SetTextAlign(TA_BOTTOM | TA_LEFT);
	pDC->SetTextColor(0x000000);   // black
	pDC->SetBkMode(TRANSPARENT);

	CGdiObjectSave OldPen(pDC, pDC->SelectStockObject(BLACK_PEN));

	pDC->MoveTo(cr.left, RulerBase - 1);
	pDC->LineTo(cr.right, RulerBase - 1);

	pDC->SelectStockObject(WHITE_PEN);
	pDC->MoveTo(cr.left, RulerBase);
	pDC->LineTo(cr.right, RulerBase);

	pDC->SelectObject( & DarkGrayPen);
	pDC->MoveTo(cr.left, RulerBase - 2);
	pDC->LineTo(cr.right, RulerBase - 2);

	TCHAR const DecimalPoint = GetApp()->m_DecimalPoint;
	float const SampleRate = float(pDoc->WaveSampleRate());
	int nTickCount;
	double DistTime;
	double nFirstTime;
	int nLength;

	CString s;
	ASSERT(0 == fmod(dOrgX, dScaleX * dLogScaleX));

	switch (m_CurrentDisplayMode)
	{
	case ShowSamples:
	{
		// calculate position string length

		nLength = pDC->GetTextExtent(_T("0,000,000,000"), 13).cx;

		NUMBER_OF_SAMPLES nSamples = NUMBER_OF_SAMPLES(1.5 * nLength / GetXScaleDev());
		// calculate how much samples can be between the numbers
		if (nSamples > INT_MAX / 10)
		{
			nSamples = INT_MAX / 10;
		}

		int dist = 1;
		unsigned nTickDist = 1;
		// find the closest bigger 1,2,5 * 10^x
		nTickCount = 1;

		for (unsigned k = 1; k <= INT_MAX / 10; k *= 10)
		{
			dist = k;
			nTickDist = k;

			if (dist >= nSamples)
			{
				if (dist > 10)
				{
					nTickDist = dist / 10;
					nTickCount = 10;
				}
				break;
			}
			dist = k * 2;
			if (dist >= nSamples)
			{
				nTickCount = 2;
				break;
			}
			dist = k * 5;
			if (dist >= nSamples)
			{
				nTickCount = 2;
				break;
			}
		}

		DistTime = dist / SampleRate;
	}
		break;
	case ShowHhMmSs:
	{
		// calculate position string length

		nLength = pDC->GetTextExtent(_T("00:00:00.0000"), 13).cx;

		DistTime = 1.5 * nLength / GetXScaleDev() / SampleRate;
		// select distance between ticks
		double multiplier = 1.;
		double divisor = 1.;

		if (DistTime >= 1.)
		{
			if (DistTime > 3600.)
			{
				multiplier = 3600.;
				DistTime = ceil(DistTime / 3600.);
			}
			else if (DistTime > 60.)
			{
				multiplier = 60.;
				DistTime = ceil(DistTime / 60.);
			}
			else
			{
				DistTime = ceil(DistTime);
			}
			// find the closest bigger 1,2,5, 10, 20, 30
			if (DistTime <= 1.)
			{
				DistTime = 1.;
				nTickCount = 10;
			}
			else if (DistTime <= 2.)
			{
				DistTime = 2.;
				nTickCount = 2;
			}
			else if (DistTime <= 5.)
			{
				DistTime = 5.;
				nTickCount = 5;
			}
			else if (DistTime <= 10.)
			{
				DistTime = 10.;
				nTickCount = 10;
			}
			else if (DistTime <= 20.)
			{
				DistTime = 20.;
				nTickCount = 2;
			}
			else if (DistTime <= 30.)
			{
				DistTime = 30.;
				nTickCount = 3;
			}
			else
			{
				DistTime = 60.;
				nTickCount = 6;
			}
		}
		else    // DistTime < 1
		{
			divisor = 1000.;
			DistTime = ceil(DistTime * 1000.);
			// find the closest bigger 1,2,5 * 10^x
			for (; ; multiplier *= 10., DistTime = ceil(DistTime / 10.))
			{
				if (DistTime <= 1.)
				{
					DistTime = 1.;
					nTickCount = 10;
					break;
				}
				else if (DistTime <= 2.)
				{
					DistTime = 2.;
					nTickCount = 2;
					break;
				}
				else if (DistTime <= 5.)
				{
					DistTime = 5.;
					nTickCount = 5;
					break;
				}
			}
		}

		DistTime = DistTime * multiplier / divisor;
	}
		break;

	case ShowSeconds:
	{
		// calculate position string length

		nLength = pDC->GetTextExtent(_T("00,000.0000"), 14).cx;

		DistTime = 1.5 * nLength / GetXScaleDev() / SampleRate;
		// select distance between ticks
		double multiplier = 1.;
		double divisor = 1.;

		if (DistTime >= 1.)
		{
			DistTime = ceil(DistTime);
		}
		else
		{
			divisor = 1000.;
			DistTime = ceil(DistTime * 1000.);
		}

		// find the closest bigger 1,2,5 * 10^x
		for (; ; multiplier *= 10., DistTime = ceil(DistTime / 10.))
		{
			if (DistTime <= 1.)
			{
				DistTime = 1.;
				nTickCount = 10;
				break;
			}
			else if (DistTime <= 2.)
			{
				DistTime = 2.;
				nTickCount = 2;
				break;
			}
			else if (DistTime <= 5.)
			{
				DistTime = 5.;
				nTickCount = 5;
				break;
			}
		}

		DistTime = DistTime * multiplier / divisor;
	}
		break;

	default:
		return;
	}

	nFirstTime = floor(WindowToWorldX(cr.left - nLength) / (SampleRate * DistTime))
				* DistTime;

	if (nFirstTime < 0)
	{
		nFirstTime = 0;
	}

	CString s1;

	for(int nTick = 0; ; nTick++)
	{
		double time = nFirstTime + DistTime
					* nTick / double(nTickCount);

		double sample = fround(time * SampleRate);

		if (sample > pDoc->WaveFileSamples())
		{
			break;
		}

		int x = WorldToWindowXfloor(sample);

		if (x > cr.right)
		{
			break;
		}

		if (0 == nTick % nTickCount)
		{
			if (0) TRACE("tick for sample %f is drawn at position %d\n", sample, x);
			// draw bigger tick (6 pixels high) and the number
			pDC->SelectStockObject(WHITE_PEN);
			pDC->MoveTo(x + 1, RulerBase - 2);
			pDC->LineTo(x + 1, RulerBase - 8);

			pDC->SelectStockObject(BLACK_PEN);
			pDC->MoveTo(x, RulerBase - 2);
			pDC->LineTo(x, RulerBase - 8);

			switch (m_CurrentDisplayMode)
			{
			case ShowSamples:
			{
				s = LtoaCS(SAMPLE_INDEX(sample));
			}
				break;
			case ShowHhMmSs:
			{
				int flags = TimeToHhMmSs_NeedsHhMm;
				if (DistTime < 1.)
				{
					flags = TimeToHhMmSs_NeedsHhMm | TimeToHhMmSs_NeedsMs;
				}
				s = TimeToHhMmSs(unsigned((time + 0.0005) * 1000), flags);
			}
				break;
			case ShowSeconds:
			{
				time += 0.0005;
				int ss = int(time);
				time -= ss;
				int ms = int(time * 1000.);

				s1 = LtoaCS(ss);

				if (DistTime < 1.)
				{
					s.Format(_T("%s%c%03d"), s1, DecimalPoint, ms);
				}
				else
				{
					s.Format(_T("%s%c0"), s1, DecimalPoint);
				}
			}
			}
			pDC->TextOut(x + 2, RulerBase - 5, s);

		}
		else
		{
			// draw small tick (3 pixels high)
			pDC->SelectStockObject(BLACK_PEN);
			pDC->MoveTo(x, RulerBase - 2);
			pDC->LineTo(x, RulerBase - 5);

			pDC->SelectStockObject(WHITE_PEN);
			pDC->MoveTo(x + 1, RulerBase - 2);
			pDC->LineTo(x + 1, RulerBase - 5);
		}
	}

	// draw markers and regions

	CWaveFile::InstanceDataWav * pInst = pDoc->m_WavFile.GetInstanceData();

	pDC->SelectStockObject(BLACK_PEN);
	pDC->SetPolyFillMode(WINDING);
	CGdiObjectSave OldBrush(pDC, pDC->SelectStockObject(WHITE_BRUSH));

	for (CuePointVectorIterator i = pInst->m_CuePoints.begin();
		i < pInst->m_CuePoints.end(); i++)
	{
		long x = WorldToWindowXfloor(i->dwSampleOffset);
		WaveRegionMarker * pMarker = pInst->GetRegionMarker(i->CuePointID);

		if (x >= cr.left - m_MarkerHeight
			&& x <= cr.right + m_MarkerHeight)
		{
			if (pMarker != NULL
				&& pMarker->SampleLength != 0)
			{
				// draw mark of the region begin
				POINT p[] = {
					x, cr.bottom - 1,
					x, cr.bottom - m_MarkerHeight,
					x + m_MarkerHeight - 2, cr.bottom - m_MarkerHeight,
					x + m_MarkerHeight - 2, cr.bottom - m_MarkerHeight + 1,
				};

				pDC->Polygon(p, countof(p));
			}
			else
			{
				// draw marker
				POINT p[] = {
					x, cr.bottom - 1,
					x + (m_MarkerHeight >> 1), cr.bottom - (m_MarkerHeight >> 1) - 1,
					x + (m_MarkerHeight >> 1), cr.bottom - m_MarkerHeight,
					x - (m_MarkerHeight >> 1), cr.bottom - m_MarkerHeight,
					x - (m_MarkerHeight >> 1), cr.bottom - (m_MarkerHeight >> 1) - 1,
				};

				pDC->Polygon(p, countof(p));
			}
		}

		if (pMarker != NULL
			&& pMarker->SampleLength != 0)
		{
			x = WorldToWindowXfloor(i->dwSampleOffset + pMarker->SampleLength);

			if (x >= cr.left - m_MarkerHeight
				&& x <= cr.right + m_MarkerHeight)
			{
				// draw mark of the region end
				POINT p[] = {
					x, cr.bottom - 1,
					x, cr.bottom - m_MarkerHeight,
					x - m_MarkerHeight + 2, cr.bottom - m_MarkerHeight,
					x - m_MarkerHeight + 2, cr.bottom - m_MarkerHeight + 1,
				};

				pDC->Polygon(p, countof(p));
			}
		}
	}
}

/////////////////////////////////////////////////////////////////////////////
// CTimeRulerView diagnostics

#ifdef _DEBUG
void CTimeRulerView::AssertValid() const
{
	CScaledScrollView::AssertValid();
}

void CTimeRulerView::Dump(CDumpContext& dc) const
{
	CScaledScrollView::Dump(dc);
}

CWaveSoapFrontDoc* CTimeRulerView::GetDocument() const// non-debug version is inline
{
	ASSERT(m_pDocument->IsKindOf(RUNTIME_CLASS(CWaveSoapFrontDoc)));
	return (CWaveSoapFrontDoc*)m_pDocument;
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CTimeRulerView message handlers


void CTimeRulerView::OnViewRulerHhmmss()
{
	m_CurrentDisplayMode = ShowHhMmSs;
	Invalidate();
}

void CTimeRulerView::OnUpdateViewRulerHhmmss(CCmdUI* pCmdUI)
{
	pCmdUI->SetRadio(m_CurrentDisplayMode == ShowHhMmSs);
}

void CTimeRulerView::OnViewRulerSamples()
{
	m_CurrentDisplayMode = ShowSamples;
	Invalidate();
}

void CTimeRulerView::OnUpdateViewRulerSamples(CCmdUI* pCmdUI)
{
	pCmdUI->SetRadio(m_CurrentDisplayMode == ShowSamples);
}

void CTimeRulerView::OnViewRulerSeconds()
{
	m_CurrentDisplayMode = ShowSeconds;
	Invalidate();
}

void CTimeRulerView::OnUpdateViewRulerSeconds(CCmdUI* pCmdUI)
{
	pCmdUI->SetRadio(m_CurrentDisplayMode == ShowSeconds);
}

void CTimeRulerView::OnUpdate( CView* /*pSender*/, LPARAM lHint, CObject* pHint )
{
	if (lHint == CWaveSoapFrontDoc::UpdateMarkerRegionChanged
		&& NULL != pHint)
	{
		MarkerRegionUpdateInfo * pInfo = static_cast<MarkerRegionUpdateInfo *> (pHint);
		ASSERT(NULL != pInfo);

		InvalidateMarkerRegion( & pInfo->info);
	}
	else
	{
		CSoundUpdateInfo * pInfo = dynamic_cast<CSoundUpdateInfo *>(pHint);
		if (lHint == CWaveSoapFrontDoc::UpdateSampleRateChanged
			|| (lHint == CWaveSoapFrontDoc::UpdateSoundChanged
				&& pInfo != NULL && pInfo->m_NewLength != -1))
		{
			// either unknown notification or length changed
			Invalidate();
		}
	}
}

int CTimeRulerView::CalculateHeight()
{
	CWindowDC dc(GetDesktopWindow());
	CGdiObjectSave OldFont(dc, dc.SelectStockObject(ANSI_VAR_FONT));

	TEXTMETRIC tm;
	dc.GetTextMetrics( & tm);

	// text height plus tmAveCharWidth (for the marks) plus 7 pixels for overhead
	return tm.tmHeight + tm.tmAveCharWidth + 7;
}

unsigned CTimeRulerView::HitTest(POINT p, RECT * pHitRect) const
{
	unsigned result = 0;

	CWindowDC dc(GetDesktopWindow());
	CGdiObjectSave OldFont(dc, dc.SelectStockObject(ANSI_VAR_FONT));

	CRect cr;
	GetClientRect(cr);

	CWaveFile::InstanceDataWav * pInst = GetDocument()->m_WavFile.GetInstanceData();

	if (p.y >= cr.bottom - m_MarkerHeight)
	{
		int n;
		CuePointVectorIterator i;

		for (n = 0, i = pInst->m_CuePoints.begin();
			i < pInst->m_CuePoints.end(); i++, n++)
		{
			long x = WorldToWindowXfloor(i->dwSampleOffset);
			WaveRegionMarker * pMarker = pInst->GetRegionMarker(i->CuePointID);

			if (NULL == pMarker
				|| 0 == pMarker->SampleLength)
			{
				if (p.x <= x + (m_MarkerHeight >> 1)
					&& p.x >= x - (m_MarkerHeight >> 1))
				{
					// marker
#if 0
					POINT p[] = {
						x, cr.bottom - 1,
						x + (m_MarkerHeight >> 1), cr.bottom - (m_MarkerHeight >> 1) - 1,
						x + (m_MarkerHeight >> 1), cr.bottom - m_MarkerHeight,
						x - (m_MarkerHeight >> 1), cr.bottom - m_MarkerHeight,
						x - (m_MarkerHeight >> 1), cr.bottom - (m_MarkerHeight >> 1) - 1,
					};
#endif
					result = HitTestMarker | n;

					if (NULL != pHitRect)
					{
						pHitRect->left = x - (m_MarkerHeight >> 1);
						pHitRect->right = x + (m_MarkerHeight >> 1);
						pHitRect->top = cr.bottom - m_MarkerHeight;
						pHitRect->bottom = cr.bottom;
					}
					break;
				}
			}
			else
			{
				if (p.x <= x + m_MarkerHeight
					&& p.x >= x)
				{
					// mark of the region begin
#if 0
					POINT p[] = {
						x, cr.bottom - 1,
						x, cr.bottom - m_MarkerHeight,
						x + m_MarkerHeight - 2, cr.bottom - m_MarkerHeight,
						x + m_MarkerHeight - 2, cr.bottom - m_MarkerHeight + 1,
					};
#endif

					result = HitTestRegionBegin | n;

					if (NULL != pHitRect)
					{
						pHitRect->left = x;
						pHitRect->right = x + m_MarkerHeight - 1;
						pHitRect->top = cr.bottom - m_MarkerHeight;
						pHitRect->bottom = cr.bottom;
					}
					break;
				}

				x = WorldToWindowXfloor(i->dwSampleOffset + pMarker->SampleLength);

				if (p.x >= x - m_MarkerHeight
					&& p.x <= x)
				{
					// mark of the region end
#if 0
					POINT p[] = {
						x, cr.bottom - 1,
						x, cr.bottom - m_MarkerHeight,
						x - m_MarkerHeight + 2, cr.bottom - m_MarkerHeight,
						x - m_MarkerHeight + 2, cr.bottom - m_MarkerHeight + 1,
					};
#endif
					result = HitTestRegionEnd | n;

					if (NULL != pHitRect)
					{
						pHitRect->left = x - m_MarkerHeight;
						pHitRect->right = x;
						pHitRect->top = cr.bottom - m_MarkerHeight;
						pHitRect->bottom = cr.bottom;
					}
					break;
				}
			}
		}
	}
	return result;
}

BOOL CTimeRulerView::OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message)
{
	if (pWnd == this
		&& HTCLIENT == nHitTest)
	{
		CPoint p;

		GetCursorPos( & p);
		ScreenToClient( & p);

		unsigned hit = HitTest(p);

		//
		if (hit & HitTestRegionBegin)
		{
			SetCursor(AfxGetApp()->LoadStandardCursor(IDC_HAND));
			return TRUE;
		}
		else if (hit & HitTestRegionEnd)
		{
			SetCursor(AfxGetApp()->LoadStandardCursor(IDC_HAND));
			return TRUE;
		}
		else if (hit & HitTestMarker)
		{
			SetCursor(AfxGetApp()->LoadStandardCursor(IDC_HAND));
			return TRUE;
		}
	}

	return BaseClass::OnSetCursor(pWnd, nHitTest, message);
}

void CTimeRulerView::OnInitialUpdate()
{
	CHorizontalRuler::OnInitialUpdate();

	EnableToolTips();
}

INT_PTR CTimeRulerView::OnToolHitTest(CPoint point, TOOLINFO* pTI) const
{
	RECT r;
	unsigned hit = HitTest(point, & r);
	if (HitTestNone == hit)
	{
		return -1;
	}

	if (pTI != NULL && pTI->cbSize >= offsetof(TOOLINFO, lParam))
	{
		pTI->hwnd = m_hWnd;
		pTI->uId = hit;
		pTI->rect = r;
		pTI->lpszText = LPSTR_TEXTCALLBACK;
	}

	return hit;
}

void CTimeRulerView::OnToolTipText(UINT /*id*/, NMHDR* pNMHDR, LRESULT* pResult)
{
	ASSERT(pNMHDR->code == TTN_NEEDTEXTA || pNMHDR->code == TTN_NEEDTEXTW);

	// need to handle both ANSI and UNICODE versions of the message
	TOOLTIPTEXTA* pTTTA = (TOOLTIPTEXTA*)pNMHDR;
	TOOLTIPTEXTW* pTTTW = (TOOLTIPTEXTW*)pNMHDR;

	CWaveFile::InstanceDataWav * pInst = GetDocument()->m_WavFile.GetInstanceData();

	LPCTSTR strTipText = pInst->GetCueTextByIndex(pNMHDR->idFrom & HitTestCueIndexMask);
	if (NULL != strTipText)
	{
#ifndef _UNICODE
		if (pNMHDR->code == TTN_NEEDTEXTA)
		{
			lstrcpyn(pTTTA->szText, strTipText, countof(pTTTA->szText));
		}
		else
		{
			_mbstowcsz(pTTTW->szText, strTipText, countof(pTTTW->szText));
		}
#else
		if (pNMHDR->code == TTN_NEEDTEXTA)
		{
			_wcstombsz(pTTTA->szText, strTipText, countof(pTTTA->szText));
		}
		else
		{
			lstrcpyn(pTTTW->szText, strTipText, countof(pTTTW->szText));
		}
#endif
		*pResult = 0;
	}

	// bring the tooltip window above other popup windows
	::SetWindowPos(pNMHDR->hwndFrom, HWND_TOP, 0, 0, 0, 0,
					SWP_NOACTIVATE|SWP_NOSIZE|SWP_NOMOVE|SWP_NOOWNERZORDER);
}

void CTimeRulerView::OnLButtonDblClk(UINT nFlags, CPoint point)
{
	// TODO: Add your message handler code here and/or call default
	// if the marker or region is double-clicked, open the marker editing dialog

	BaseClass::OnLButtonDblClk(nFlags, point);
}

void CTimeRulerView::OnLButtonDown(UINT nFlags, CPoint point)
{
	// TODO: Add your message handler code here and/or call default
	// if clicked on a marker, wait for marker drag
	m_DraggedMarkerHitTest = HitTest(point);

	BaseClass::OnLButtonDown(nFlags, point);
}

void CTimeRulerView::OnLButtonUp(UINT nFlags, CPoint point)
{
	EndMarkerDrag();
	BaseClass::OnLButtonUp(nFlags, point);
}

void CTimeRulerView::EndMarkerDrag()
{
	CWaveSoapFrontDoc * pDoc = GetDocument();
	if (0 != m_DraggedMarkerHitTest
		&& bIsTrackingSelection
		&& ! pDoc->IsReadOnly())
	{
		// if it's being dragged, finalize UNDO
		WAVEREGIONINFO info = {0};

		info.Flags = info.CommitChanges | info.CuePointIndex;
		info.MarkerCueID = m_DraggedMarkerHitTest & HitTestCueIndexMask;

		pDoc->ChangeWaveMarker( & info);
	}

	m_DraggedMarkerHitTest = 0;
}

void CTimeRulerView::OnMouseMove(UINT nFlags, CPoint point)
{
	CWaveSoapFrontDoc * pDoc = GetDocument();
	// if a marker is being dragged, move it
	// if it begun to drag, store UNDO
	if (0 == m_DraggedMarkerHitTest
		|| pDoc->IsReadOnly())
	{
		BaseClass::OnMouseMove(nFlags, point);
		return;
	}

	CView::OnMouseMove(nFlags, point);

	if (WM_LBUTTONDOWN == ButtonPressed
		&& point.x != PrevMouseX)
	{
		if (! bIsTrackingSelection)
		{
			// check if drag threshold exceeded
			if (abs(point.x - PrevMouseX) < GetSystemMetrics(SM_CXDRAG) / 2)
			{
				return;
			}
			SetCapture();
			bIsTrackingSelection = TRUE;
		}

		CRect cr;
		GetClientRect(cr);

		int const AutoscrollWidth = GetSystemMetrics(SM_CXVSCROLL);

		bool DoLeftAutoscroll = false;
		bool DoRightAutoscroll = false;

		int DataEnd = WorldToWindowXceil(pDoc->WaveFileSamples());
		if (point.x < DataEnd
			&& cr.right > AutoscrollWidth)
		{
			if (point.x > cr.right - AutoscrollWidth)
			{
				DoRightAutoscroll = true;
			}
			else if (point.x < AutoscrollWidth)
			{
				DoLeftAutoscroll = true;
			}
		}

		if (DoLeftAutoscroll || DoRightAutoscroll)
		{
			if (NULL == m_AutoscrollTimerID)
			{
				m_AutoscrollTimerID = SetTimer(UINT_PTR(this) + sizeof *this, 50, NULL);
			}
		}
		else if (NULL != m_AutoscrollTimerID)
		{
			KillTimer(m_AutoscrollTimerID);
			m_AutoscrollTimerID = NULL;
		}
		// do drag
		SAMPLE_INDEX NewPosition = SAMPLE_INDEX(WindowToWorldX(point.x));

		WAVEREGIONINFO info = {0};

		info.Flags = info.ChangeSample | info.CuePointIndex;
		info.MarkerCueID = m_DraggedMarkerHitTest & HitTestCueIndexMask;

		pDoc->m_WavFile.GetWaveMarker( & info);

		if (m_DraggedMarkerHitTest & HitTestRegionBegin)
		{
			if (NewPosition >= SAMPLE_INDEX(info.Sample + info.Length))
			{
				return;
			}

			info.Flags = info.ChangeSample | info.ChangeLength;
			info.Length += info.Sample - NewPosition;
			info.Sample = NewPosition;
		}
		else if (m_DraggedMarkerHitTest & HitTestRegionEnd)
		{
			if (NewPosition <= SAMPLE_INDEX(info.Sample))
			{
				return;
			}
			info.Flags = info.ChangeLength;
			info.Length = NewPosition - info.Sample;
		}
		else if (m_DraggedMarkerHitTest & HitTestMarker)
		{
			info.Flags = info.ChangeSample;
			info.Sample = NewPosition;
		}
		else
		{
			return;
		}

		pDoc->ChangeWaveMarker( & info);
	}
}

void CTimeRulerView::OnCaptureChanged(CWnd *pWnd)
{
	// TODO: Add your message handler code here
	if (pWnd != this
		&& NULL != m_AutoscrollTimerID)
	{
		if (TRACE_SCROLL) TRACE("Killing timer in CWaveSoapFrontView::OnCaptureChanged\n");
		KillTimer(m_AutoscrollTimerID);
		m_AutoscrollTimerID = NULL;
	}

	EndMarkerDrag();
	// if capture is lost, reset changes?
	BaseClass::OnCaptureChanged(pWnd);
}

void CTimeRulerView::OnTimer(UINT nIDEvent)
{
	// get mouse position and hit code
	if (NULL != m_AutoscrollTimerID
		&& nIDEvent == m_AutoscrollTimerID)
	{
		CWaveSoapFrontDoc * pDoc = GetDocument();

		CPoint p;
		GetCursorPos( & p);
		ScreenToClient( & p);

		CRect cr;
		GetClientRect(cr);

		int const AutoscrollWidth = GetSystemMetrics(SM_CXVSCROLL);

		bool DoLeftAutoscroll = false;
		bool DoRightAutoscroll = false;

		int DataEnd = WorldToWindowXceil(pDoc->WaveFileSamples());
		if (p.x < DataEnd
			&& cr.right > AutoscrollWidth)
		{
			if (p.x > cr.right - AutoscrollWidth)
			{
				DoRightAutoscroll = true;
			}
			else if (p.x < AutoscrollWidth)
			{
				DoLeftAutoscroll = true;
			}
		}

		if (DoLeftAutoscroll || DoRightAutoscroll)
		{
			//TRACE("OnTimer: VSHT_RIGHT_AUTOSCROLL\n");
			double scroll;
			int nDistance;

			scroll = 1. / m_pHorMaster->GetXScale();

			if (DoRightAutoscroll)
			{
				CRect r;
				GetClientRect(r);

				nDistance = p.x - r.right + AutoscrollWidth - 1;
			}
			else
			{
				scroll = -scroll;
				nDistance = AutoscrollWidth - p.x - 1;
			}

			if (TRACE_SCROLL) TRACE("nDistance = %d\n", nDistance);
			if (nDistance > 14)
			{
				nDistance = 14;
			}
			if (nDistance > 0)
			{
				scroll *= 1 << nDistance;
			}
			ScrollBy(scroll, 0, TRUE);

			UINT flags = 0;
			if (0x8000 & GetKeyState(VK_CONTROL))
			{
				flags |= MK_CONTROL;
			}
			if (0x8000 & GetKeyState(VK_SHIFT))
			{
				flags |= MK_SHIFT;
			}
			if (0x8000 & GetKeyState(VK_LBUTTON))
			{
				flags |= MK_LBUTTON;
			}
			if (0x8000 & GetKeyState(VK_RBUTTON))
			{
				flags |= MK_RBUTTON;
			}
			if (0x8000 & GetKeyState(VK_MBUTTON))
			{
				flags |= MK_MBUTTON;
			}
			OnMouseMove(flags, p);
			return;
		}
		else
		{
			KillTimer(m_AutoscrollTimerID);
			m_AutoscrollTimerID = NULL;
		}
	}

	if (TRACE_SCROLL) TRACE("Timer ID=%X\n", nIDEvent);

	BaseClass::OnTimer(nIDEvent);
}

void CTimeRulerView::InvalidateMarkerRegion(WAVEREGIONINFO const * pInfo)
{
	CRect cr;
	GetClientRect(cr);
	CRect r;

	long x = WorldToWindowXfloor(pInfo->Sample);

	if (0 != (pInfo->Flags & (pInfo->ChangeSample | pInfo->Delete)))
	{
		if (0 == pInfo->Length)
		{
			if (x < cr.right + (m_MarkerHeight >> 1)
				&& x >= cr.left - (m_MarkerHeight >> 1))
			{
				r.left = x - (m_MarkerHeight >> 1);
				r.right = x + 1 + (m_MarkerHeight >> 1);
				r.top = cr.bottom - m_MarkerHeight;
				r.bottom = cr.bottom;

				InvalidateRect(r);
			}
		}
		else if (x < cr.right && x >= cr.left - m_MarkerHeight)
		{
			// invalidate region begin marker
			r.left = x;
			r.right = x + m_MarkerHeight - 1;
			r.top = cr.bottom - m_MarkerHeight;
			r.bottom = cr.bottom;

			InvalidateRect(r);
		}
	}

	if (0 != pInfo->Length
		&& 0 != (pInfo->Flags
			& (pInfo->ChangeSample | pInfo->ChangeLength | pInfo->Delete)))
	{
		// invalidate region end marker
		x = WorldToWindowXfloor(pInfo->Sample + pInfo->Length);

		if (x < cr.right + m_MarkerHeight && x >= cr.left)
		{
			r.left = x - m_MarkerHeight;
			r.right = x + 1;
			r.top = cr.bottom - m_MarkerHeight;
			r.bottom = cr.bottom;

			InvalidateRect(r);
		}
	}
}

