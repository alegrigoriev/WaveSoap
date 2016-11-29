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
#include "resource.h"       // main symbols
#include "OperationDialogs2.h"

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
	: m_CurrentDisplayMode(SampleToString_Mask & GetApp()->m_SoundTimeFormat)
	, m_DraggedMarkerHitTest(0)
	, m_AutoscrollTimerID(0)
	, m_MarkerHeight(10)
	, m_PopupMenuHitTest(0)
	, m_HitOffset(0)
	, m_FirstSampleInView(0.)
	, m_HorizontalScale(2048.)
{
	memzero(m_PopupMenuHit);
}

CTimeRulerView::~CTimeRulerView()
{
}

BEGIN_MESSAGE_MAP(CTimeRulerView, CHorizontalRuler)
	//{{AFX_MSG_MAP(CTimeRulerView)
	ON_COMMAND(ID_VIEW_RULER_HHMMSS, OnViewRulerHhmmss)
	ON_UPDATE_COMMAND_UI(ID_VIEW_RULER_HHMMSS, OnUpdateViewRulerHhmmss)
	ON_COMMAND(ID_VIEW_RULER_SAMPLES, OnViewRulerSamples)
	ON_UPDATE_COMMAND_UI(ID_VIEW_RULER_SAMPLES, OnUpdateViewRulerSamples)
	ON_COMMAND(ID_VIEW_RULER_SECONDS, OnViewRulerSeconds)
	ON_UPDATE_COMMAND_UI(ID_VIEW_RULER_SECONDS, OnUpdateViewRulerSeconds)
	ON_COMMAND(ID_VIEW_RULER_HHMMSSFF, OnViewRulerHhmmssFf)
	ON_UPDATE_COMMAND_UI(ID_VIEW_RULER_HHMMSSFF, OnUpdateViewRulerHhmmssFf)
	//}}AFX_MSG_MAP
	ON_WM_CONTEXTMENU()
	ON_WM_SETCURSOR()
	ON_WM_LBUTTONDBLCLK()
	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONUP()
	ON_WM_MOUSEMOVE()
	ON_WM_CAPTURECHANGED()
	ON_WM_TIMER()
	ON_COMMAND(ID_GOTO_MARKER, OnGotoMarker)
	ON_COMMAND(ID_DELETE_MARKER, OnDeleteMarker)
	ON_UPDATE_COMMAND_UI(ID_DELETE_MARKER, OnUpdateDeleteMarker)
	ON_COMMAND(ID_MOVE_MARKER_TO_CURRENT, OnMoveMarkerToCurrent)
	ON_UPDATE_COMMAND_UI(ID_MOVE_MARKER_TO_CURRENT, OnUpdateMoveMarkerToCurrent)
	ON_COMMAND(ID_EDIT_MARKER, OnEditMarker)
	ON_UPDATE_COMMAND_UI(ID_EDIT_MARKER, OnUpdateEditMarker)
	ON_COMMAND(ID_SELECT_REGION, OnSelectRegion)
	ON_MESSAGE(UWM_NOTIFY_VIEWS, &CTimeRulerView::OnUwmNotifyViews)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CTimeRulerView drawing

void CTimeRulerView::OnDraw(CDC* pDC)
{
	CWaveSoapFrontDoc* pDoc = GetDocument();
	if (!pDoc->m_WavFile.IsOpen())
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

	pDC->SetTextAlign(TA_BOTTOM | TA_LEFT);
	pDC->SetTextColor(0x000000);   // black
	pDC->SetBkMode(TRANSPARENT);

	CGdiObjectSave OldPen(pDC, pDC->SelectStockObject(BLACK_PEN));

	int const RulerBase = cr.bottom - m_MarkerHeight - 2;
	pDC->MoveTo(cr.left, RulerBase - 1);
	pDC->LineTo(cr.right, RulerBase - 1);

	pDC->SelectStockObject(WHITE_PEN);
	pDC->MoveTo(cr.left, RulerBase);
	pDC->LineTo(cr.right, RulerBase);

	pDC->SelectObject( & DarkGrayPen);
	pDC->MoveTo(cr.left, RulerBase - 2);
	pDC->LineTo(cr.right, RulerBase - 2);

	switch (m_CurrentDisplayMode)
	{
	case ShowSeconds:
		DrawSeconds(pDC);
		break;
	case ShowSamples:
		DrawSamples(pDC);
		break;
	case ShowHhMmSs:
		DrawHhMmSs(pDC);
		break;
	case ShowHhMmSsFf:
		DrawHhMmSsFf(pDC);
		break;
	}
	// draw markers and regions

	CWaveFile::InstanceDataWav * pInst = pDoc->m_WavFile.GetInstanceData();

	pDC->SelectStockObject(BLACK_PEN);
	pDC->SetPolyFillMode(WINDING);
	CGdiObjectSave OldBrush(pDC, pDC->SelectStockObject(WHITE_BRUSH));

	for (CuePointVectorIterator i = pInst->m_CuePoints.begin();
		i != pInst->m_CuePoints.end(); i++)
	{
		long x = SampleToWindowXfloor(i->dwSampleOffset);
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
			x = SampleToWindowXfloor(i->dwSampleOffset + pMarker->SampleLength);

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

void CTimeRulerView::DrawSamples(CDC * pDC)
{
	CWaveSoapFrontDoc* pDoc = GetDocument();

	CRect cr;
	GetClientRect(cr);
	int const RulerBase = cr.bottom - m_MarkerHeight - 2;

	TCHAR const DecimalPoint = GetApp()->m_DecimalPoint;
	float const SampleRate = float(pDoc->WaveSampleRate());

	int nTickCount = 1;
	double DistTime;
	double nFirstTime;
	int nLength;

	CString s;
	//ASSERT(0 == fmod(dOrgX, dScaleX * dLogScaleX));

	// calculate position string length

	nLength = pDC->GetTextExtent(_T("0,000,000,000"), 13).cx;

	NUMBER_OF_SAMPLES nSamples = NUMBER_OF_SAMPLES(1.5 * nLength * m_HorizontalScale);
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


	nFirstTime = floor(WindowXToSample(cr.left - nLength) / (SampleRate * DistTime))
				* DistTime;

	if (nFirstTime < 0)
	{
		nFirstTime = 0;
	}

	CString s1;

	for (int nTick = 0; ; nTick++)
	{
		double time = nFirstTime + DistTime
					* nTick / double(nTickCount);

		double sample = fround(time * SampleRate);

		if (sample > pDoc->WaveFileSamples())
		{
			break;
		}

		int x = SampleToWindowXfloor(sample);

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

			s = LtoaCS(SAMPLE_INDEX(sample));

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
}

void CTimeRulerView::DrawHhMmSs(CDC * pDC)
{
	CWaveSoapFrontDoc* pDoc = GetDocument();

	CRect cr;
	GetClientRect(cr);
	int const RulerBase = cr.bottom - m_MarkerHeight - 2;

	TCHAR const DecimalPoint = GetApp()->m_DecimalPoint;
	float const SampleRate = float(pDoc->WaveSampleRate());

	int nTickCount = 1;
	double DistTime;
	double nFirstTime;
	int nLength;

	CString s;
	//ASSERT(0 == fmod(dOrgX, dScaleX * dLogScaleX));

	// calculate position string length

	nLength = pDC->GetTextExtent(_T("00:00:00.0000"), 13).cx;

	DistTime = 1.5 * nLength * m_HorizontalScale / SampleRate;
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

	nFirstTime = floor(WindowXToSample(cr.left - nLength) / (SampleRate * DistTime))
				* DistTime;

	if (nFirstTime < 0)
	{
		nFirstTime = 0;
	}

	CString s1;

	for (int nTick = 0; ; nTick++)
	{
		double time = nFirstTime + DistTime
					* nTick / double(nTickCount);

		double sample = fround(time * SampleRate);

		if (sample > pDoc->WaveFileSamples())
		{
			break;
		}

		int x = SampleToWindowXfloor(sample);

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

			int flags = TimeToHhMmSs_NeedsHhMm;
			if (DistTime < 1.)
			{
				flags = TimeToHhMmSs_NeedsHhMm | TimeToHhMmSs_NeedsMs;
			}
			s = TimeToHhMmSs(unsigned((time + 0.0005) * 1000), flags);
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
}

void CTimeRulerView::DrawSeconds(CDC * pDC)
{
	CWaveSoapFrontDoc* pDoc = GetDocument();

	CRect cr;
	GetClientRect(cr);
	int const RulerBase = cr.bottom - m_MarkerHeight - 2;

	TCHAR const DecimalPoint = GetApp()->m_DecimalPoint;
	float const SampleRate = float(pDoc->WaveSampleRate());

	int nTickCount = 1;
	double DistTime;
	double nFirstTime;
	int nLength;

	CString s;

	// calculate position string length

	nLength = pDC->GetTextExtent(_T("00,000.0000"), 11).cx;

	DistTime = 1.5 * nLength * m_HorizontalScale / SampleRate;
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

	nFirstTime = floor(WindowXToSample(cr.left - nLength) / (SampleRate * DistTime))
				* DistTime;

	if (nFirstTime < 0)
	{
		nFirstTime = 0;
	}

	CString s1;

	for (int nTick = 0; ; nTick++)
	{
		double time = nFirstTime + DistTime
					* nTick / double(nTickCount);

		double sample = fround(time * SampleRate);

		if (sample > pDoc->WaveFileSamples())
		{
			break;
		}

		int x = SampleToWindowXfloor(sample);

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

			time += 0.0005;
			int ss = int(time);
			time -= ss;
			int ms = int(time * 1000.);

			s1 = LtoaCS(ss);

			if (DistTime < 1.)
			{
				s.Format(_T("%s%c%03d"), static_cast<LPCTSTR>(s1), DecimalPoint, ms);
			}
			else
			{
				s.Format(_T("%s%c0"), static_cast<LPCTSTR>(s1), DecimalPoint);
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
}

void CTimeRulerView::DrawHhMmSsFf(CDC * pDC)
{
	CWaveSoapFrontDoc* pDoc = GetDocument();

	CRect cr;
	GetClientRect(cr);
	int const RulerBase = cr.bottom - m_MarkerHeight - 2;

	TCHAR const DecimalPoint = GetApp()->m_DecimalPoint;
	float const SampleRate = float(pDoc->WaveSampleRate());

	int nTickCount = 1;
	double DistTime;
	double nFirstTime;
	int nLength;

	CString s;

	// calculate position string length

	nLength = pDC->GetTextExtent(_T("00:00:00.00f"), 12).cx;

	DistTime = 1.5 * nLength * m_HorizontalScale / SampleRate;
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
			nTickCount = 5;
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
		divisor = 75.;
		DistTime = ceil(DistTime * 75.);
		// find the closest bigger 1,2,5 * 10^x
		if (DistTime <= 1.)
		{
			DistTime = 1.;
			nTickCount = 1;
		}
		else if (DistTime <= 5.)
		{
			DistTime = 5.;
			nTickCount = 5;
		}
		else if (DistTime <= 15.)
		{
			DistTime = 15.;
			nTickCount = 5;
		}
		else if (DistTime <= 25.)
		{
			DistTime = 25.;
			nTickCount = 5;
		}
		else
		{
			DistTime = 75.;
			nTickCount = 5;
		}
	}

	DistTime = DistTime * multiplier / divisor;

	nFirstTime = floor(WindowXToSample(cr.left - nLength) / (SampleRate * DistTime))
				* DistTime;

	if (nFirstTime < 0)
	{
		nFirstTime = 0;
	}

	CString s1;

	for (int nTick = 0; ; nTick++)
	{
		double time = nFirstTime + DistTime
					* nTick / double(nTickCount);

		double sample = fround(time * SampleRate);

		if (sample > pDoc->WaveFileSamples())
		{
			break;
		}

		int x = SampleToWindowXfloor(sample);

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

			if (DistTime < 1.)
			{
				unsigned Seconds = unsigned(time + 0.0001);
				unsigned Frames = unsigned(fmod(time + 0.0001, 1.) * 75);
				s = TimeToHhMmSs(Seconds * 1000 + Frames, TimeToHhMmSs_NeedsHhMm | TimeToHhMmSs_Frames75);
			}
			else
			{
				s = TimeToHhMmSs(unsigned((time + 0.0005) * 1000), TimeToHhMmSs_NeedsHhMm);
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
}

/////////////////////////////////////////////////////////////////////////////
// CTimeRulerView diagnostics

#ifdef _DEBUG
void CTimeRulerView::AssertValid() const
{
	BaseClass::AssertValid();
}

void CTimeRulerView::Dump(CDumpContext& dc) const
{
	BaseClass::Dump(dc);
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

void CTimeRulerView::OnViewRulerHhmmssFf()
{
	m_CurrentDisplayMode = ShowHhMmSsFf;
	Invalidate();
}

void CTimeRulerView::OnUpdateViewRulerHhmmssFf(CCmdUI* pCmdUI)
{
	pCmdUI->SetRadio(m_CurrentDisplayMode == ShowHhMmSsFf);
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
	CWindowDC dc(AfxGetMainWnd());
	CGdiObjectSave OldFont(dc, dc.SelectStockObject(ANSI_VAR_FONT));

	TEXTMETRIC tm;
	dc.GetTextMetrics( & tm);

	// text height plus tmAveCharWidth (for the marks) plus 7 pixels for overhead
	return tm.tmHeight + tm.tmAveCharWidth + 7;
}

// the function accepts point coordinates relative to the client area
// it puts the marker rectangle into optional *pHitRect, and offset from the marker origin to *OffsetX
unsigned CTimeRulerView::HitTest(POINT p, RECT * pHitRect, int * OffsetX) const
{
	unsigned result = HitTestNone;
	CWaveSoapFrontDoc * pDoc = GetDocument();

	CWindowDC dc(const_cast<CTimeRulerView*>(this));
	CGdiObjectSave OldFont(dc, dc.SelectStockObject(ANSI_VAR_FONT));

	CRect cr;
	GetClientRect(cr);

	CWaveFile::InstanceDataWav * pInst = pDoc->m_WavFile.GetInstanceData();
	if (NULL == pInst)
	{
		return 0;
	}

	if (p.y >= cr.bottom - m_MarkerHeight)
	{
		result = HitTestLowerHalf;

		int n;
		CuePointVectorIterator i;

		for (n = 0, i = pInst->m_CuePoints.begin();
			i != pInst->m_CuePoints.end(); i++, n++)
		{
			long x = SampleToWindowXfloor(i->dwSampleOffset);
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

					if (NULL != OffsetX)
					{
						*OffsetX = p.x - x;
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
					if (NULL != OffsetX)
					{
						*OffsetX = p.x - x;
					}
					break;
				}

				x = SampleToWindowXfloor(i->dwSampleOffset + pMarker->SampleLength);

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
					if (NULL != OffsetX)
					{
						*OffsetX = p.x - x;
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
		if (0 != (hit & HitTestLowerHalf)
			|| (GetDocument()->IsReadOnly()
				&& 0 != (hit & (HitTestRegionBegin | HitTestRegionEnd | HitTestMarker))))
		{
			SetCursor(AfxGetApp()->LoadStandardCursor(IDC_ARROW));
			return TRUE;
		}
		else if (hit & HitTestRegionBegin)
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
	unsigned const hit = HitTest(point, & r);
	if (HitTestNone == hit
		|| HitTestLowerHalf == hit)
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
	CWaveSoapFrontDoc * pDoc = GetDocument();
	unsigned hit = HitTest(point);

	WAVEREGIONINFO info;

	info.Flags = info.CuePointIndex | info.ChangeAll;
	info.MarkerCueID = hit & HitTestCueIndexMask;

	BaseClass::OnLButtonDblClk(nFlags, point);

	if (! pDoc->IsReadOnly()
		&& 0 != (hit & (HitTestRegionBegin | HitTestRegionEnd | HitTestMarker)))
	{
		// if the marker or region is double-clicked, open the marker editing dialog
		if ( ! pDoc->m_WavFile.GetWaveMarker( & info))
		{
			info.Flags = 0;
		}

		CMarkerRegionDialog dlg( & info, pDoc->m_CaretPosition,
								pDoc->m_WavFile, GetApp()->m_SoundTimeFormat);

		if (IDOK != dlg.DoModal())
		{
			return;
		}

		pDoc->BeginMarkerChange(CWaveFile::InstanceDataWav::MetadataCopyAllCueData);
		info.Flags |= info.CommitChanges;

		pDoc->ChangeWaveMarker( & info);
	}
	else
	{
		// if double clicked between markers, select
		pDoc->SelectBetweenMarkers(SAMPLE_INDEX(WindowXToSample(point.x)));
	}
}

void CTimeRulerView::OnLButtonDown(UINT nFlags, CPoint point)
{
	// if clicked on a marker, wait for marker drag
	m_DraggedMarkerHitTest = HitTest(point, NULL, & m_HitOffset);

	if (GetDocument()->IsReadOnly()
		|| HitTestLowerHalf == m_DraggedMarkerHitTest)
	{
		m_DraggedMarkerHitTest = 0;
	}

	BaseClass::OnLButtonDown(nFlags, point);
}

void CTimeRulerView::OnLButtonUp(UINT nFlags, CPoint point)
{
	EndMarkerDrag();
	BaseClass::OnLButtonUp(nFlags, point);
}

void CTimeRulerView::BeginMarkerDrag()
{
	CWaveSoapFrontDoc * pDoc = GetDocument();
	if (0 != m_DraggedMarkerHitTest
		&& m_bIsTrackingSelection
		&& ! pDoc->IsReadOnly())
	{
		pDoc->BeginMarkerChange(CWaveFile::InstanceDataWav::MetadataCopyCue
								| CWaveFile::InstanceDataWav::MetadataCopyLtxt);
	}
}

void CTimeRulerView::EndMarkerDrag()
{
	CWaveSoapFrontDoc * pDoc = GetDocument();
	if (0 != m_DraggedMarkerHitTest
		&& m_bIsTrackingSelection
		&& ! pDoc->IsReadOnly())
	{
		// if it's being dragged, finalize UNDO
		WAVEREGIONINFO info = {0};

		info.Flags = info.CommitChanges | info.CuePointIndex;
		info.MarkerCueID = m_DraggedMarkerHitTest & HitTestCueIndexMask;

		pDoc->ChangeWaveMarker( & info);    // also calls pDoc->EndMarkerChange
	}

	m_DraggedMarkerHitTest = 0;
}

void CTimeRulerView::OnMouseMove(UINT nFlags, CPoint point)
{
	CWaveSoapFrontDoc * pDoc = GetDocument();
	// if a marker is being dragged, move it
	// if it begun to drag, store UNDO
	if (0 == m_DraggedMarkerHitTest)
	{
		BaseClass::OnMouseMove(nFlags, point);
		return;
	}

	CView::OnMouseMove(nFlags, point);

	if (WM_LBUTTONDOWN == ButtonPressed
		&& point.x != PrevMouseX)
	{
		if (! m_bIsTrackingSelection)
		{
			// check if drag threshold exceeded
			if (abs(point.x - PrevMouseX) < GetSystemMetrics(SM_CXDRAG) / 2)
			{
				return;
			}
			SetCapture();
			m_bIsTrackingSelection = TRUE;
			BeginMarkerDrag();
		}

		CRect cr;
		GetClientRect(cr);

		int const AutoscrollWidth = GetSystemMetrics(SM_CXVSCROLL);

		bool DoLeftAutoscroll = false;
		bool DoRightAutoscroll = false;

		int DataEnd = SampleToWindowXceil(pDoc->WaveFileSamples());
		point.x -= m_HitOffset;

		if (point.x < DataEnd
			&& cr.right > AutoscrollWidth * 2)
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
				m_AutoscrollTimerID = SetTimer(UINT_PTR(this+1), 50, NULL);
			}
		}
		else if (NULL != m_AutoscrollTimerID)
		{
			KillTimer(m_AutoscrollTimerID);
			m_AutoscrollTimerID = NULL;
		}
		// do drag
		NUMBER_OF_SAMPLES const nSamples = pDoc->WaveFileSamples();

		SAMPLE_INDEX NewPosition = SAMPLE_INDEX(WindowXToSample(point.x));
		if (NewPosition > nSamples
			|| NewPosition < 0)
		{
			return;
		}

		WAVEREGIONINFO info = {0};

		info.Flags = info.ChangeSample | info.CuePointIndex;
		info.MarkerCueID = m_DraggedMarkerHitTest & HitTestCueIndexMask;

		pDoc->m_WavFile.GetWaveMarker( & info);

		if (m_DraggedMarkerHitTest & HitTestRegionBegin)
		{
			if (nFlags & MK_SHIFT)
			{
				// don't change length
				if (NewPosition + SAMPLE_INDEX(info.Length) > nSamples)
				{
					return;
				}
				info.Flags = info.ChangeSample;
			}
			else
			{
				if (NewPosition >= SAMPLE_INDEX(info.Sample + info.Length))
				{
					return;
				}
				info.Flags = info.ChangeSample | info.ChangeLength;
				info.Length += info.Sample - NewPosition;
			}
			info.Sample = NewPosition;
		}
		else if (m_DraggedMarkerHitTest & HitTestRegionEnd)
		{
			if (nFlags & MK_SHIFT)
			{
				// don't change length
				if (NewPosition - SAMPLE_INDEX(info.Length) < 0)
				{
					return;
				}
				info.Sample = NewPosition - info.Length;
				info.Flags = info.ChangeSample;
			}
			else
			{
				if (NewPosition <= SAMPLE_INDEX(info.Sample))
				{
					return;
				}
				info.Flags = info.ChangeLength;
				info.Length = NewPosition - info.Sample;
			}
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

void CTimeRulerView::OnTimer(UINT_PTR nIDEvent)
{
	// get mouse position and hit code
	if (NULL == m_AutoscrollTimerID
		|| nIDEvent != m_AutoscrollTimerID)
	{
		BaseClass::OnTimer(nIDEvent);
		return;
	}

	CWaveSoapFrontDoc * pDoc = GetDocument();

	CPoint p;
	GetCursorPos( & p);
	ScreenToClient( & p);

	CRect cr;
	GetClientRect(cr);

	int const AutoscrollWidth = GetSystemMetrics(SM_CXVSCROLL);

	int DataEnd = SampleToWindowXceil(pDoc->WaveFileSamples());

	if (p.x >= DataEnd
		|| cr.right <= AutoscrollWidth * 2)
	{
		KillTimer(m_AutoscrollTimerID);
		m_AutoscrollTimerID = NULL;
		return;
	}

	if (p.x > cr.right - AutoscrollWidth)
	{
		int nDistance = (p.x - (cr.right - AutoscrollWidth) - 1) / 2;
		if (nDistance > 14)
		{
			nDistance = 14;
		}
		HorizontalScrollByPixels(-1 << nDistance);
	}
	else if (p.x < AutoscrollWidth)
	{
		int nDistance = (AutoscrollWidth - p.x - 1) / 2;
		if (nDistance > 14)
		{
			nDistance = 14;
		}

		HorizontalScrollByPixels(1 << nDistance);
	}
	else
	{
		KillTimer(m_AutoscrollTimerID);
		m_AutoscrollTimerID = NULL;
		return;
	}

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
}

void CTimeRulerView::InvalidateMarkerRegion(WAVEREGIONINFO const * pInfo)
{
	CRect cr;
	GetClientRect(cr);
	CRect r;

	long x = SampleToWindowXfloor(pInfo->Sample);

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
		x = SampleToWindowXfloor(pInfo->Sample + pInfo->Length);

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

void CTimeRulerView::OnContextMenu(CWnd* /*pWnd*/, CPoint point)
{
	CPoint pc(point);
	ScreenToClient( & pc);

	m_PopupMenuHitTest = HitTest(pc);

	m_PopupMenuHit.Flags = m_PopupMenuHit.CuePointIndex | m_PopupMenuHit.ChangeAll;

	m_PopupMenuHit.MarkerCueID = m_PopupMenuHitTest & HitTestCueIndexMask;

	if ( ! GetDocument()->m_WavFile.GetWaveMarker( & m_PopupMenuHit))
	{
		m_PopupMenuHit.Flags = 0;
	}

	// make sure window is active
	GetParentFrame()->ActivateFrame();

	UINT uID = IDR_MENU_TIME_RULER;
	if (m_PopupMenuHitTest & HitTestMarker)
	{
		uID = IDR_MENU_TIME_RULER_MARKER;
	}
	else if (m_PopupMenuHitTest & (HitTestRegionBegin | HitTestRegionEnd))
	{
		uID = IDR_MENU_TIME_RULER_REGION;
	}

	CMenu menu;
	if ( !menu.LoadMenu(uID))
	{
		return;
	}

	CMenu* pPopup = menu.GetSubMenu(0);
	if(pPopup != NULL)
	{
		int Command = pPopup->TrackPopupMenu(
											TPM_LEFTALIGN | TPM_RIGHTBUTTON | TPM_RETURNCMD,
											point.x, point.y,
											AfxGetMainWnd()); // use main window for cmds

		if (0 != Command)
		{
			AfxGetMainWnd()->SendMessage(WM_COMMAND, Command & 0xFFFF, 0);
		}
	}

	m_PopupMenuHitTest = 0;
	m_PopupMenuHit.Flags = 0;
}

void CTimeRulerView::OnGotoMarker()
{
	SAMPLE_INDEX sample;
	CWaveSoapFrontDoc * pDoc = GetDocument();
	if (m_PopupMenuHitTest & (HitTestRegionBegin | HitTestMarker))
	{
		sample = m_PopupMenuHit.Sample;
	}
	else if (m_PopupMenuHitTest & HitTestRegionEnd)
	{
		sample = m_PopupMenuHit.Sample + m_PopupMenuHit.Length;
	}
	else
	{
		return;
	}

	pDoc->SetSelection(sample, sample, pDoc->m_SelectedChannel, sample, 0);
}

void CTimeRulerView::OnDeleteMarker()
{
	if (m_PopupMenuHitTest & (HitTestRegionBegin | HitTestRegionEnd | HitTestMarker))
	{
		CWaveSoapFrontDoc * pDoc = GetDocument();
		m_PopupMenuHit.Flags |= m_PopupMenuHit.Delete | m_PopupMenuHit.CommitChanges;

		pDoc->BeginMarkerChange(CWaveFile::InstanceDataWav::MetadataCopyAllCueData);
		pDoc->ChangeWaveMarker( & m_PopupMenuHit);
	}
}

void CTimeRulerView::OnUpdateDeleteMarker(CCmdUI *pCmdUI)
{
	pCmdUI->Enable(0 != (m_PopupMenuHitTest & (HitTestRegionBegin | HitTestRegionEnd | HitTestMarker))
					&& ! GetDocument()->IsReadOnly());
}

void CTimeRulerView::OnMoveMarkerToCurrent()
{
	CWaveSoapFrontDoc * pDoc = GetDocument();

	unsigned MarkerChangeMask = CWaveFile::InstanceDataWav::MetadataCopyCue;

	if (m_PopupMenuHitTest & (HitTestMarker | HitTestRegionBegin))
	{
		m_PopupMenuHit.Flags |= m_PopupMenuHit.ChangeSample;

		if (m_PopupMenuHitTest & HitTestRegionBegin)
		{
			MarkerChangeMask = CWaveFile::InstanceDataWav::MetadataCopyCue
								| CWaveFile::InstanceDataWav::MetadataCopyLtxt;

			m_PopupMenuHit.Flags |= m_PopupMenuHit.ChangeLength;

			if (pDoc->m_CaretPosition >= SAMPLE_INDEX(m_PopupMenuHit.Sample + m_PopupMenuHit.Length))
			{
				return;
			}

			m_PopupMenuHit.Length = m_PopupMenuHit.Length + m_PopupMenuHit.Sample - pDoc->m_CaretPosition;
		}
		m_PopupMenuHit.Sample = pDoc->m_CaretPosition;
	}
	else if (m_PopupMenuHitTest & HitTestRegionEnd)
	{
		m_PopupMenuHit.Flags |= m_PopupMenuHit.ChangeLength;
		MarkerChangeMask = CWaveFile::InstanceDataWav::MetadataCopyLtxt;

		if (pDoc->m_CaretPosition <= SAMPLE_INDEX(m_PopupMenuHit.Sample))
		{
			return;
		}

		m_PopupMenuHit.Length = pDoc->m_CaretPosition - m_PopupMenuHit.Sample;
	}
	else
	{
		return;
	}

	pDoc->BeginMarkerChange(MarkerChangeMask);
	m_PopupMenuHit.Flags |= m_PopupMenuHit.CommitChanges;

	pDoc->ChangeWaveMarker( & m_PopupMenuHit);
}

void CTimeRulerView::OnUpdateMoveMarkerToCurrent(CCmdUI *pCmdUI)
{
	BOOL bEnable = FALSE;
	CWaveSoapFrontDoc * pDoc = GetDocument();

	if ( ! pDoc->IsReadOnly())
	{
		if (m_PopupMenuHitTest & HitTestMarker)
		{
			bEnable = TRUE;
		}
		else if (m_PopupMenuHitTest & HitTestRegionBegin)
		{
			bEnable = pDoc->m_CaretPosition < SAMPLE_INDEX(m_PopupMenuHit.Sample + m_PopupMenuHit.Length);
		}
		else if (m_PopupMenuHitTest & HitTestRegionEnd)
		{
			bEnable = pDoc->m_CaretPosition > SAMPLE_INDEX(m_PopupMenuHit.Sample);
		}
	}
	pCmdUI->Enable(bEnable);
}

void CTimeRulerView::OnEditMarker()
{
	CWaveSoapFrontDoc * pDoc = GetDocument();

	CMarkerRegionDialog dlg( & m_PopupMenuHit, pDoc->m_CaretPosition,
							pDoc->m_WavFile, GetApp()->m_SoundTimeFormat);

	if (IDOK != dlg.DoModal())
	{
		return;
	}

	pDoc->BeginMarkerChange(CWaveFile::InstanceDataWav::MetadataCopyAllCueData);
	m_PopupMenuHit.Flags |= m_PopupMenuHit.CommitChanges;

	pDoc->ChangeWaveMarker( & m_PopupMenuHit);
}

void CTimeRulerView::OnUpdateEditMarker(CCmdUI *pCmdUI)
{
	pCmdUI->Enable(0 != (m_PopupMenuHitTest & (HitTestRegionBegin | HitTestRegionEnd | HitTestMarker))
					&& ! GetDocument()->IsReadOnly());
}

void CTimeRulerView::OnSelectRegion()
{
	SAMPLE_INDEX Caret;

	CWaveSoapFrontDoc * pDoc = GetDocument();

	if (m_PopupMenuHitTest & HitTestRegionBegin)
	{
		Caret = m_PopupMenuHit.Sample;
	}
	else if (m_PopupMenuHitTest & HitTestRegionEnd)
	{
		Caret = m_PopupMenuHit.Sample + m_PopupMenuHit.Length;
	}
	else
	{
		return;
	}

	pDoc->SetSelection(m_PopupMenuHit.Sample, m_PopupMenuHit.Sample + m_PopupMenuHit.Length,
						pDoc->m_SelectedChannel, Caret, 0);
}

BOOL CTimeRulerView::OnNotify(WPARAM wParam, LPARAM lParam, LRESULT* pResult)
{
	NMHDR * pNMHDR = (NMHDR *) lParam;
	if (pNMHDR->code == TTN_NEEDTEXTA || pNMHDR->code == TTN_NEEDTEXTW)
	{
		OnToolTipText((UINT)wParam, pNMHDR, pResult);
		return 0;
	}
	return BaseClass::OnNotify(wParam, lParam, pResult);
}


afx_msg LRESULT CTimeRulerView::OnUwmNotifyViews(WPARAM wParam, LPARAM lParam)
{
	NotifyViewsData * data = (NotifyViewsData *)lParam;
	switch (wParam)
	{
	case HorizontalOriginChanged:
		HorizontalScrollTo(data->HorizontalScroll.FirstSampleInView);
		break;
	case HorizontalExtentChanged:
	{
		m_FirstSampleInView = data->HorizontalScroll.FirstSampleInView;
		m_HorizontalScale = data->HorizontalScroll.HorizontalScale;

		Invalidate();
	}
		break;
	}
	return 0;
}

void CTimeRulerView::HorizontalScrollByPixels(int Pixels)
{
	NotifySiblingViews(HorizontalScrollPixels, &Pixels);
}

void CTimeRulerView::HorizontalScrollTo(double first_sample_in_view)
{
	// FirstSample is aligned to multiple of HorizontalScale
	int ScrollPixels = int((first_sample_in_view - m_FirstSampleInView) / m_HorizontalScale);
	ASSERT( 0. == fmod(first_sample_in_view - m_FirstSampleInView, m_HorizontalScale));
	m_FirstSampleInView = first_sample_in_view;

	ScrollWindow(-ScrollPixels, 0);
}

