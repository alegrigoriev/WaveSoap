// Copyright Alexander Grigoriev, 1997-2002, All Rights Reserved
// TimeRulerView.cpp : implementation file
//

#include "stdafx.h"
#include "WaveSoapFront.h"
#include "TimeRulerView.h"
#include "WaveSoapFrontView.h"
#include "GdiObjectSave.h"
#include "TimeToStr.h"
#include ".\timerulerview.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

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

	int const MarkerHeight = tm.tmAveCharWidth;

	CPen DarkGrayPen(PS_SOLID, 0, 0x808080);
	CRect cr;
	GetClientRect(cr);

	CPaintDC * pPaintDC = dynamic_cast<CPaintDC *>(pDC);
	if (NULL != pPaintDC)
	{
		cr = pPaintDC->m_ps.rcPaint;
	}

	int const RulerBase = cr.bottom - MarkerHeight - 2;

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

		if (x >= cr.left - MarkerHeight
			&& x <= cr.right + MarkerHeight)
		{
			if (pMarker != NULL
				&& pMarker->SampleLength != 0)
			{
				// draw mark of the region begin
				POINT p[] = {
					x, cr.bottom - 1,
					x, cr.bottom - MarkerHeight,
					x + MarkerHeight - 2, cr.bottom - MarkerHeight,
					x + MarkerHeight - 2, cr.bottom - MarkerHeight + 1,
				};

				pDC->Polygon(p, countof(p));
			}
			else
			{
				// draw marker
				POINT p[] = {
					x, cr.bottom - 1,
					x + (MarkerHeight >> 1), cr.bottom - (MarkerHeight >> 1) - 1,
					x + (MarkerHeight >> 1), cr.bottom - MarkerHeight,
					x - (MarkerHeight >> 1), cr.bottom - MarkerHeight,
					x - (MarkerHeight >> 1), cr.bottom - (MarkerHeight >> 1) - 1,
				};

				pDC->Polygon(p, countof(p));
			}
		}

		if (pMarker != NULL
			&& pMarker->SampleLength != 0)
		{
			x = WorldToWindowXfloor(i->dwSampleOffset + pMarker->SampleLength);

			if (x >= cr.left - MarkerHeight
				&& x <= cr.right + MarkerHeight)
			{
				// draw mark of the region end
				POINT p[] = {
					x, cr.bottom - 1,
					x, cr.bottom - MarkerHeight,
					x - MarkerHeight + 2, cr.bottom - MarkerHeight,
					x - MarkerHeight + 2, cr.bottom - MarkerHeight + 1,
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
	CSoundUpdateInfo * pInfo = dynamic_cast<CSoundUpdateInfo *>(pHint);
	if (lHint == CWaveSoapFrontDoc::UpdateSampleRateChanged
		|| (lHint == CWaveSoapFrontDoc::UpdateSoundChanged
			&& pInfo != NULL && pInfo->m_NewLength != -1))
	{
		// either unknown notification or length changed
		Invalidate();
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

	TEXTMETRIC tm;
	dc.GetTextMetrics( & tm);

	int const MarkerHeight = tm.tmAveCharWidth;

	CRect cr;
	GetClientRect(cr);

	CWaveFile::InstanceDataWav * pInst = GetDocument()->m_WavFile.GetInstanceData();

	if (p.y >= cr.bottom - MarkerHeight)
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
				if (p.x <= x + (MarkerHeight >> 1)
					&& p.x >= x - (MarkerHeight >> 1))
				{
					// marker
					POINT p[] = {
						x, cr.bottom - 1,
						x + (MarkerHeight >> 1), cr.bottom - (MarkerHeight >> 1) - 1,
						x + (MarkerHeight >> 1), cr.bottom - MarkerHeight,
						x - (MarkerHeight >> 1), cr.bottom - MarkerHeight,
						x - (MarkerHeight >> 1), cr.bottom - (MarkerHeight >> 1) - 1,
					};

					result = HitTestMarker | n;

					if (NULL != pHitRect)
					{
						pHitRect->left = x - (MarkerHeight >> 1);
						pHitRect->right = x + (MarkerHeight >> 1);
						pHitRect->top = cr.bottom - MarkerHeight;
						pHitRect->bottom = cr.bottom;
					}
					break;
				}
			}
			else
			{
				if (p.x <= x + MarkerHeight
					&& p.x >= x)
				{
					// mark of the region begin
					POINT p[] = {
						x, cr.bottom - 1,
						x, cr.bottom - MarkerHeight,
						x + MarkerHeight - 2, cr.bottom - MarkerHeight,
						x + MarkerHeight - 2, cr.bottom - MarkerHeight + 1,
					};

					result = HitTestRegionBegin | n;

					if (NULL != pHitRect)
					{
						pHitRect->left = x;
						pHitRect->right = x + MarkerHeight;
						pHitRect->top = cr.bottom - MarkerHeight;
						pHitRect->bottom = cr.bottom;
					}
					break;
				}

				x = WorldToWindowXfloor(i->dwSampleOffset + pMarker->SampleLength);

				if (p.x >= x - MarkerHeight
					&& p.x <= x)
				{
					// mark of the region end
					POINT p[] = {
						x, cr.bottom - 1,
						x, cr.bottom - MarkerHeight,
						x - MarkerHeight + 2, cr.bottom - MarkerHeight,
						x - MarkerHeight + 2, cr.bottom - MarkerHeight + 1,
					};

					result = HitTestRegionEnd | n;

					if (NULL != pHitRect)
					{
						pHitRect->left = x - MarkerHeight;
						pHitRect->right = x;
						pHitRect->top = cr.bottom - MarkerHeight;
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
