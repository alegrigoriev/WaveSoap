// AmplitudeRuler.cpp : implementation file
// Copyright Alexander Grigoriev, 1997-2002, All Rights Reserved
//

#include "stdafx.h"
#include "WaveSoapFront.h"
#include "AmplitudeRuler.h"
#include "SpectrumSectionView.h"
#include "GdiObjectSave.h"
#include "WaveSoapFrontDoc.h"
#include "WaveSoapFrontView.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define TRACE_DRAWING 0
/////////////////////////////////////////////////////////////////////////////
// CAmplitudeRuler

IMPLEMENT_DYNCREATE(CAmplitudeRuler, CVerticalRuler)

CAmplitudeRuler::CAmplitudeRuler()
	:m_DrawMode(PercentView)
	, m_VerticalScale(1.)
	, m_WaveOffsetY(0.)
	, m_WaveOffsetBeforeScroll(0)
	, m_MouseYOffsetForScroll(0)
	, m_MaxAmplitudeRange(1.)
{
	memzero(m_Heights);
}

CAmplitudeRuler::~CAmplitudeRuler()
{
}


BEGIN_MESSAGE_MAP(CAmplitudeRuler, BaseClass)
	//{{AFX_MSG_MAP(CAmplitudeRuler)
	ON_WM_CREATE()
	ON_WM_LBUTTONDBLCLK()
	ON_COMMAND(ID_VIEW_AMPL_RULER_SAMPLES, OnViewAmplRulerSamples)
	ON_COMMAND(ID_VIEW_AMPL_RULER_PERCENT, OnViewAmplRulerPercent)
	ON_COMMAND(ID_VIEW_AMPL_RULER_DECIBELS, OnViewAmplRulerDecibels)
	ON_UPDATE_COMMAND_UI(ID_VIEW_AMPL_RULER_SAMPLES, OnUpdateAmplRulerSamples)
	ON_UPDATE_COMMAND_UI(ID_VIEW_AMPL_RULER_PERCENT, OnUpdateAmplRulerPercent)
	ON_UPDATE_COMMAND_UI(ID_VIEW_AMPL_RULER_DECIBELS, OnUpdateAmplRulerDecibels)
	ON_WM_CONTEXTMENU()
	ON_WM_CAPTURECHANGED()
	//}}AFX_MSG_MAP
	ON_MESSAGE(UWM_NOTIFY_VIEWS, &CAmplitudeRuler::OnUwmNotifyViews)
	ON_WM_ERASEBKGND()
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CAmplitudeRuler drawing

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

void CAmplitudeRuler::OnDraw(CDC* pDrawDC)
{
	CWaveSoapFrontDoc* pDoc = GetDocument();
	if (! pDoc->m_WavFile.IsOpen())
	{
		return;
	}

	// Draw with double buffering
	CRect cr;
	GetClientRect(cr);

	NUMBER_OF_CHANNELS nChannels = pDoc->WaveChannels();
	if (0 == nChannels)
	{
		return;
	}

	CDC dc;
	dc.CreateCompatibleDC(NULL);
	CBitmap DrawBitmap;
	DrawBitmap.CreateCompatibleBitmap(pDrawDC, cr.Width(), cr.Height());

	CDC * pDC = & dc;

	CGdiObjectSaveT<CBitmap> OldBitmap(pDC, pDC->SelectObject(& DrawBitmap));
	CGdiObjectSave OldFont(pDC, pDC->SelectStockObject(ANSI_VAR_FONT));
	CGdiObjectSave OldPen(pDC, pDC->SelectStockObject(BLACK_PEN));

	if (0) TRACE("SysColor(COLOR_WINDOW)=%X\n", GetSysColor(COLOR_WINDOW));

	pDC->SetTextAlign(TA_BOTTOM | TA_RIGHT);
	pDC->SetTextColor(0x000000);   // black
	pDC->SetBkMode(TRANSPARENT);

	for (int ch = 0; ch < nChannels; ch++)
	{
		CRect chr;
		// for all channels, the rectangle is of the same height
		chr.top = m_Heights.ch[ch].top;
		chr.bottom = m_Heights.ch[ch].bottom;
		chr.left = cr.left;
		chr.right = cr.right;

		CRect clipr;    // channel clip rect
		// for all channels, the rectangle is of the same height
		clipr.top = m_Heights.ch[ch].clip_top;
		clipr.bottom = m_Heights.ch[ch].clip_bottom;
		clipr.left = cr.left;
		clipr.right = cr.right;

		pDC->FillSolidRect(clipr, GetSysColor(COLOR_WINDOW));

		if (!m_Heights.ch[ch].minimized)
		{
			if (0) TRACE("CAmplitudeRuler::OnDraw: ch %d zero pos =%d\n",
				ch, (int)WaveCalculate(m_WaveOffsetY, m_VerticalScale, m_Heights.ch[ch].top, m_Heights.ch[ch].bottom)(0.));
			switch (m_DrawMode)
			{
			case PercentView:
				DrawChannelPercents(pDC, chr, clipr);
				break;
			case DecibelView:
				DrawChannelDecibels(pDC, chr, clipr);
				break;
			default:
				DrawChannelSamples(pDC, chr, clipr);
				break;
			}
		}

		if (ch < nChannels - 1)
		{
			pDC->MoveTo(0, clipr.bottom);
			pDC->LineTo(cr.right, clipr.bottom);
			clipr.bottom++;
		}
		pDrawDC->BitBlt(clipr.left, clipr.top, clipr.Width(), clipr.Height(), pDC, clipr.left, clipr.top, SRCCOPY);
	}
}

void CAmplitudeRuler::DrawChannelSamples(CDC * pDC, CRect const & chr, CRect const & clipr)
{
	TEXTMETRIC tm;
	pDC->GetTextMetrics( & tm);

	int nHeight = chr.Height();

	int nVertStep = GetSystemMetrics(SM_CYMENU);

	int ClipHigh = clipr.bottom - tm.tmHeight / 2;
	int ClipLow = clipr.top + tm.tmHeight / 2;

	if (ClipHigh <= ClipLow)
	{
		return;
	}

	int nSampleUnits = int(nVertStep * 65536. / (nHeight * m_VerticalScale));

	// round sample units to 10 or 5
	int step;
	for (step = 1; step < nSampleUnits; step *= 10)
	{
		if (step * 2 >= nSampleUnits)
		{
			step *= 2;
			break;
		}
		if (step * 5 >= nSampleUnits)
		{
			step *= 5;
			break;
		}
	}

	WaveCalculate WaveToY(m_WaveOffsetY, m_VerticalScale, chr.top, chr.bottom);

	int yLow = WaveToY.ConvertToSample(ClipHigh);
	// round to the next multiple of step
	yLow += (step * 0x10000 - yLow) % step;

	int yHigh = WaveToY.ConvertToSample(ClipLow);
	yHigh -= (step * 0x10000 + yHigh) % step;
	ASSERT(yLow <= yHigh);

	for (int y = yLow; y <= yHigh; y += step)
	{
		int yDev = WaveToY(y);

		if (0 == y)
		{
			if (TRACE_DRAWING) TRACE("CAmplitudeRuler Zero pos=%d\n", yDev);
		}

		pDC->MoveTo(chr.right - 3, yDev);
		pDC->LineTo(chr.right, yDev);
		CString s = LtoaCS(y);

		pDC->TextOut(chr.right - 3, yDev + tm.tmHeight / 2, s);
	}
}

void CAmplitudeRuler::DrawChannelPercents(CDC * pDC, CRect const & ChannelRect, CRect const & clipr)
{
	TEXTMETRIC tm;
	pDC->GetTextMetrics( & tm);

	int ClipHigh = clipr.bottom - (tm.tmHeight + 1) / 2;
	int ClipLow = clipr.top + (tm.tmHeight + 1) / 2;

	if (ClipHigh <= ClipLow)
	{
		return;
	}

	// this is minimum delta in tenths of percent between two text labels
	int nSampleUnits = int(GetSystemMetrics(SM_CYMENU) * 2000 / (ChannelRect.Height() * m_VerticalScale));

	// round sample units to 10 or 5
	int step;
	for (step = 1; step < nSampleUnits; step *= 10)
	{
		if (step * 2 >= nSampleUnits)
		{
			step *= 2;
			break;
		}
		if (step * 5 >= nSampleUnits)
		{
			step *= 5;
			break;
		}
	}

	WaveCalculate WaveToY(m_WaveOffsetY, m_VerticalScale, ChannelRect.top, ChannelRect.bottom);

	int yLow = int(WaveToY.ConvertToDoubleSample(ClipHigh) * 1000);
	// round to the next multiple of step
	yLow += (step * 0x10000 - yLow) % step;

	int yHigh = int(WaveToY.ConvertToDoubleSample(ClipLow) * 1000);

	yHigh -= (step * 0x10000 + yHigh) % step;

	for (int y = yLow; y <= yHigh; y += step)
	{
		int yDev= WaveToY(y / 1000.);

		if (0 == y)
		{
			if (TRACE_DRAWING) TRACE("CAmplitudeRuler Zero pos=%d\n", yDev);
		}

		pDC->MoveTo(ChannelRect.right - 3, yDev);
		pDC->LineTo(ChannelRect.right, yDev);
		CString s;
		if (step >= 10)
		{
			s.Format(_T("%d%%"), y/10);
		}
		else
		{
			s.Format(_T("%d.%d%%"), y/10, abs(y%10));
		}
		pDC->TextOut(ChannelRect.right - 3, yDev + tm.tmHeight / 2, s);
	}
}

void CAmplitudeRuler::DrawChannelDecibels(CDC * pDC, CRect const & ChannelRect, CRect const & clipr)
{
	// decibels are drawn with 1.5 dB step
	// if there is not enough space to draw next 2*step, it doubles the step
	TEXTMETRIC tm;
	pDC->GetTextMetrics( & tm);

	int ClipHigh = clipr.bottom - tm.tmHeight / 2;
	int ClipLow = clipr.top + tm.tmHeight / 2;

	if (ClipHigh <= ClipLow)
	{
		return;
	}

	// this is minimum delta in value units (corresponding to -1...1 range) between two text labels
	// it is twice menu height
	double nVertStep = GetSystemMetrics(SM_CYMENU) * 2 / (ChannelRect.Height() * m_VerticalScale);

	WaveCalculate WaveToY(m_WaveOffsetY, m_VerticalScale, ChannelRect.top, ChannelRect.bottom);

	double yLow = WaveToY.ConvertToDoubleSample(clipr.bottom);

	double yHigh = WaveToY.ConvertToDoubleSample(clipr.top);

	ASSERT(yLow <= yHigh);
	// We'll allow signal up to 10 dB ( 3.162 amplitude)
	int CurrDb = 1000;	// 10dB, in 0.01 dB units
	double CurrVal = pow(10., CurrDb / 2000.);

	int DbStep = 10;	// 0.1, in 0.01 dB units

	int DbTickStep = 2;	// 0.02 in 0.01 dB units
	double MultTickStep = pow(10., -DbTickStep / 2000.);

	while (CurrDb > -16000 // -160 dB
	&& CurrVal * 2 > nVertStep)  // if CurrVal * 2 > nVertStep, can't draw anymore, or positive and negative will overlap
	{
		int NextDbStep;
		int NextDbTickStep;
		switch (DbStep)
		{
		case 10:	// from 0.1 dB to 0.2 dB
			NextDbStep = 20;
			NextDbTickStep = 5;
			break;
		case 20:	// from 0.2 dB to 0.5 dB
			NextDbStep = 50;
			NextDbTickStep = 10;
			break;
		case 50:	// from 0.5 dB to 1 dB
			NextDbStep = 100;
			NextDbTickStep = 20;
			break;
		case 100:	// from 1 dB to 2 dB
#if 0
			NextDbStep = 200;
			NextDbTickStep = 50;
			break;
		case 200:	// from 2 dB to 3 dB
#endif
			NextDbStep = 300;
			NextDbTickStep = 100;
			break;
		case 300:	// from 3 dB to 6 dB
			NextDbStep = 600;
			NextDbTickStep = 100;
			break;
		case 600:	// from 6 dB to 12 dB
			NextDbStep = 1200;
			NextDbTickStep = 300;
			break;
		case 1200:	// from 12 dB to 20 dB
			NextDbStep = 2000;
			NextDbTickStep = 500;
			break;
		default:
			NextDbStep = DbStep * 2;
			NextDbTickStep = DbTickStep * 2;
			break;
		}
		//
		// if it's multiple of NextDbStep, see if we need to multiply step and skip this value
		// except that change from 20 to 50 has to happen at their common multiple
		if (0 == (CurrDb % NextDbStep) && 0 == (CurrDb % DbStep))
		{
			int NextPointToCheck = CurrDb;
			do
			{
				NextPointToCheck -= DbStep;
			}
			while (0 != (CurrDb % NextDbStep) || 0 != (CurrDb % DbStep));
			// check if we have enough space to draw each intermediate value before next multiple of NextDbStep
			if (pow(10., (NextPointToCheck + DbStep) / 2000.) - pow(10., (NextPointToCheck) / 2000.) < nVertStep)
			{
				// we don't have enough space to draw the next value
				DbStep = NextDbStep;
				DbTickStep = NextDbTickStep;

				MultTickStep = pow(10., -DbTickStep / 2000.);
				continue;
			}
		}

		bool DrawTextLabel = 0 == (CurrDb % DbStep);
		CString s;
		if (DrawTextLabel)
		{
			if (DbStep >= 100)
			{
				s.Format(_T("%d dB"), CurrDb / 100);
			}
			else
			{
				if (CurrDb >= 0)
				{
					s.Format(_T("%d.%01d dB"), CurrDb / 100, (CurrDb % 100) / 10);
				}
				else
				{
					s.Format(_T("-%d.%01d dB"), -CurrDb / 100, (-CurrDb % 100) / 10);
				}
			}
		}
		if (CurrVal >= yLow && CurrVal <= yHigh)
		{
			int y = WaveToY(CurrVal);

			pDC->MoveTo(ChannelRect.right - 3, y);
			pDC->LineTo(ChannelRect.right, y);

			if (DrawTextLabel
				&& y >= clipr.top + tm.tmHeight / 2
				&& y <= clipr.bottom - tm.tmHeight / 2)
			{
				pDC->TextOut(ChannelRect.right - 3, y + tm.tmHeight / 2, s);
			}
		}
		if (-CurrVal >= yLow && -CurrVal <= yHigh)
		{
			int y = WaveToY(-CurrVal);
			pDC->MoveTo(ChannelRect.right - 3, y);
			pDC->LineTo(ChannelRect.right, y);

			if (DrawTextLabel
				&& y >= clipr.top + tm.tmHeight / 2
				&& y <= clipr.bottom - tm.tmHeight / 2)
			{
				pDC->TextOut(ChannelRect.right - 3, y + tm.tmHeight / 2, s);
			}
		}

		CurrVal *= MultTickStep;
		CurrDb -= DbTickStep;
	}
}

int CAmplitudeRuler::CalculateWidth()
{
	CWindowDC wDC(GetDesktopWindow());

	CGdiObjectSave OldFont(wDC, wDC.SelectStockObject(ANSI_VAR_FONT));
	int Width = 4 + wDC.GetTextExtent(_T("-000,000"), 8).cx;
	return Width;
}

void CAmplitudeRuler::OnUpdate(CView* /*pSender*/, LPARAM lHint, CObject* /*pHint*/)
{
	if (lHint == CWaveSoapFrontDoc::UpdateWholeFileChanged)
	{
		// the main view takes care of it
		return;
	}
}

void CAmplitudeRuler::BeginMouseTracking()
{
	m_MouseYOffsetForScroll = 0;
	m_WaveOffsetBeforeScroll = m_WaveOffsetY;
}

void CAmplitudeRuler::VerticalScrollByPixels(int Pixels)
{
	// Positive Pixels correspond to mouse moving down. It should change m_WaveOffsetY to positive direction
	m_MouseYOffsetForScroll += Pixels;
	// NominalChannelHeight is equal offset sweep of 2
	double offset = m_WaveOffsetBeforeScroll + 2* m_MouseYOffsetForScroll / (m_VerticalScale * m_Heights.NominalChannelHeight);
	NotifySiblingViews(AmplitudeScrollTo, &offset);
}

/////////////////////////////////////////////////////////////////////////////
// CAmplitudeRuler diagnostics

#ifdef _DEBUG
void CAmplitudeRuler::AssertValid() const
{
	BaseClass::AssertValid();
}

void CAmplitudeRuler::Dump(CDumpContext& dc) const
{
	BaseClass::Dump(dc);
}

CWaveSoapFrontDoc* CAmplitudeRuler::GetDocument() // non-debug version is inline
{
	ASSERT(m_pDocument->IsKindOf(RUNTIME_CLASS(CWaveSoapFrontDoc)));
	return (CWaveSoapFrontDoc*)m_pDocument;
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CAmplitudeRuler message handlers

int CAmplitudeRuler::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CVerticalRuler::OnCreate(lpCreateStruct) == -1)
		return -1;

	return 0;
}

void CAmplitudeRuler::OnLButtonDblClk(UINT nFlags, CPoint point)
{
	CVerticalRuler::OnLButtonDblClk(nFlags, point);
	// set default scale
	int ScaleIndex = 0;
	NotifySiblingViews(VerticalScaleIndexChanged, &ScaleIndex);
	// reset offset
	NotifyViewsData data;
	data.Amplitude.MaxRange = m_MaxAmplitudeRange;
	data.Amplitude.NewScale = m_VerticalScale;
	data.Amplitude.NewOffset = 0.;
	NotifySiblingViews(VerticalScaleAndOffsetChanged, &data);
}

void CAmplitudeRuler::OnViewAmplRulerSamples()
{
	m_DrawMode = SampleView;
	Invalidate();
}

void CAmplitudeRuler::OnViewAmplRulerPercent()
{
	m_DrawMode = PercentView;
	Invalidate();
}

void CAmplitudeRuler::OnViewAmplRulerDecibels()
{
	m_DrawMode = DecibelView;
	Invalidate();
}

void CAmplitudeRuler::OnUpdateAmplRulerSamples(CCmdUI * pCmdUI)
{
	pCmdUI->SetRadio(SampleView == m_DrawMode);
}

void CAmplitudeRuler::OnUpdateAmplRulerPercent(CCmdUI * pCmdUI)
{
	pCmdUI->SetRadio(PercentView == m_DrawMode);
}

void CAmplitudeRuler::OnUpdateAmplRulerDecibels(CCmdUI * pCmdUI)
{
	pCmdUI->SetRadio(DecibelView == m_DrawMode);
}

void CAmplitudeRuler::SetNewAmplitudeOffset(double offset)
{
	// scroll channels rectangles
	CWaveSoapFrontDoc * pDoc = GetDocument();
	int nChannels = pDoc->WaveChannels();
	CRect cr;
	GetClientRect(cr);
	CWindowDC dc(this);

	for (int ch = 0; ch < nChannels; ch++)
	{
		if (m_Heights.ch[ch].minimized)
		{
			continue;
		}

		// offset of the zero line down
		long OldOffsetPixels = WaveCalculate(m_WaveOffsetY, m_VerticalScale,  m_Heights.ch[ch].top, m_Heights.ch[ch].bottom)(0.);
		long NewOffsetPixels = WaveCalculate(offset, m_VerticalScale,  m_Heights.ch[ch].top, m_Heights.ch[ch].bottom)(0.);

		int ToScroll = NewOffsetPixels - OldOffsetPixels;       // >0 - down, <0 - up

		CRect ClipRect(cr.left, m_Heights.ch[ch].clip_top, cr.right, m_Heights.ch[ch].clip_bottom);
		CRect ScrollRect(ClipRect);

		CRect ToFillBkgnd(ScrollRect);
		if (ToScroll > 0)
		{
			// down
			ToFillBkgnd.bottom = ToFillBkgnd.top + ToScroll;

		}
		else if (ToScroll < 0)
		{
			// up
			ToFillBkgnd.top = ToFillBkgnd.bottom + ToScroll;
		}
		else
		{
			continue;
		}
		ScrollWindow(0, ToScroll, ScrollRect, ClipRect);
		dc.FillSolidRect(ToFillBkgnd, GetSysColor(COLOR_WINDOW));
	}
	m_WaveOffsetY = offset;
	// make it redraw the whole window without erasing the background
	Invalidate(FALSE);
}

void CAmplitudeRuler::OnCaptureChanged(CWnd *pWnd)
{
	m_MouseYOffsetForScroll = 0;
	CVerticalRuler::OnCaptureChanged(pWnd);
}

void CAmplitudeRuler::OnContextMenu(CWnd* /*pWnd*/, CPoint point)
{
	// make sure window is active
	GetParentFrame()->ActivateFrame();

	NotifyViewsData notify = { 0 };
	notify.PopupMenu.p = point;
	notify.PopupMenu.NormalMenuId = IDR_MENU_AMPLITUDE_RULER;
	notify.PopupMenu.MinimizedMenuId = IDR_MENU_AMPLITUDE_RULER_MINIMIZED;

	NotifySiblingViews(ShowChannelPopupMenu, &notify);
}

afx_msg LRESULT CAmplitudeRuler::OnUwmNotifyViews(WPARAM wParam, LPARAM lParam)
{
	switch (wParam)
	{
	case VerticalScaleAndOffsetChanged:
	{
		NotifyViewsData * data = (NotifyViewsData *)lParam;
		if (m_VerticalScale != data->Amplitude.NewScale
			|| data->Amplitude.NewOffset != m_WaveOffsetY
			|| data->Amplitude.MaxRange != m_MaxAmplitudeRange)
		{
			m_WaveOffsetY = data->Amplitude.NewOffset;
			m_VerticalScale = data->Amplitude.NewScale;
			m_MaxAmplitudeRange = data->Amplitude.MaxRange;

			if (m_bIsTrackingSelection)
			{
				ReleaseCapture();
				//m_bIsTrackingSelection = FALSE;   // will be reset in WM_CAPTURECHANGED
			}

			Invalidate();
		}
	}
		break;

	case AmplitudeOffsetChanged:
		SetNewAmplitudeOffset(*(double*) lParam);
		break;

	case ChannelHeightsChanged:
	{
		NotifyChannelHeightsData * data = (NotifyChannelHeightsData*) lParam;
		m_Heights = *data;
		if (m_bIsTrackingSelection)
		{
			ReleaseCapture();
		}
		Invalidate(TRUE);
	}
		break;
	}

	return 0;
}

/////////////////////////////////////////////////////////////////////////////
// CSpectrumSectionRuler

IMPLEMENT_DYNCREATE(CSpectrumSectionRuler, CHorizontalRuler)

CSpectrumSectionRuler::CSpectrumSectionRuler()
	: m_DbOffset(0.)
	, m_DbPerPixel(1.)
	, m_Scale(2.)
	, m_MouseXOffsetForScroll(0)
	, m_DbOffsetBeforeScroll(0.)
	, m_XOrigin(0)
{
}

CSpectrumSectionRuler::~CSpectrumSectionRuler()
{
}


BEGIN_MESSAGE_MAP(CSpectrumSectionRuler, BaseClass)
	//{{AFX_MSG_MAP(CSpectrumSectionRuler)
	ON_WM_LBUTTONDBLCLK()
	ON_WM_CONTEXTMENU()
	//}}AFX_MSG_MAP
	ON_WM_SIZE()
	ON_MESSAGE(UWM_NOTIFY_VIEWS, &CSpectrumSectionRuler::OnUwmNotifyViews)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CSpectrumSectionRuler drawing

double CSpectrumSectionRuler::WindowXToDb(int x) const
{
	return (x - m_XOrigin) * m_DbPerPixel + m_DbOffset;
}

int CSpectrumSectionRuler::DbToWindowX(double db) const
{
	return m_XOrigin + int((db - m_DbOffset) / m_DbPerPixel + 0.5);     // round
}

void CSpectrumSectionRuler::OnDraw(CDC* pDC)
{
	CWaveSoapFrontDoc* pDoc = GetDocument();
	if (! pDoc->m_WavFile.IsOpen())
	{
		return;
	}

	CGdiObjectSave OldFont(pDC, pDC->SelectStockObject(ANSI_VAR_FONT));

	// draw horizontal line with ticks and numbers
	CPen DarkGrayPen(PS_SOLID, 0, 0x808080);
	CRect cr;
	GetClientRect( & cr);

	int nTickCount;
	double LeftExt = floor(WindowXToDb(cr.left));
	double RightExt = ceil(WindowXToDb(cr.right));
	double MaxExt = RightExt;
	if (MaxExt < LeftExt)
	{
		MaxExt = LeftExt;
	}
	double ExtDiff = fabs(RightExt - LeftExt);
	LPCTSTR MaxText;
	LPCTSTR FormatText;
	if (ExtDiff < 2)
	{
		MaxText = _T("-100.0");
	}
	else
	{
		MaxText = _T("-100");
	}

	int nLength = pDC->GetTextExtent(MaxText, (int)_tcslen(MaxText)).cx;

	double Dist = fabs(1.5 * nLength * m_DbPerPixel);
	// select distance between ticks
	double multiplier = 1.;
	double divisor = 1.;
	if (Dist >= 1.)
	{
		if (Dist > 10.)
		{
			multiplier = 10;
			Dist = ceil(Dist / 10.);
		}
		else
		{
			Dist = ceil(Dist);
		}
		FormatText = _T("%.f");
	}
	else    // DistDb < 1
	{
		divisor = 10.;
		Dist = ceil(Dist * 10.);
		FormatText = _T("%.1f");
	}
	// find the closest bigger 1,2,5, 10,
	if (Dist <= 1.)
	{
		Dist = 1.;
		nTickCount = 10;
	}
	else if (Dist <= 2.)
	{
		Dist = 2.;
		nTickCount = 2;
	}
	else if (Dist <= 5.)
	{
		Dist = 5.;
		nTickCount = 5;
	}
	else
	{
		Dist = 10.;
		nTickCount = 10;
	}

	CGdiObjectSave OldPen(pDC, pDC->SelectStockObject(BLACK_PEN));

	pDC->SetTextAlign(TA_BOTTOM | TA_LEFT);
	pDC->SetTextColor(0x000000);   // black
	pDC->SetBkMode(TRANSPARENT);

	pDC->MoveTo(cr.left, cr.bottom - 2);
	pDC->LineTo(cr.right, cr.bottom - 2);

	pDC->SelectStockObject(WHITE_PEN);
	pDC->MoveTo(cr.left, cr.bottom - 1);
	pDC->LineTo(cr.right, cr.bottom - 1);

	pDC->SelectObject( & DarkGrayPen);
	pDC->MoveTo(cr.left, cr.bottom - 3);
	pDC->LineTo(cr.right, cr.bottom - 3);

	double DistDb = Dist * multiplier / divisor;
	double nFirstDb = floor(WindowXToDb(cr.right + nLength)
							/ DistDb) * DistDb;

	for(int nTick = 0; ; nTick++)
	{
		double Db = nFirstDb - DistDb * nTick / double(nTickCount);
		int x = DbToWindowX(Db);

		if (x < cr.left - nLength)
		{
			break;
		}
		pDC->SelectStockObject(BLACK_PEN);
		pDC->MoveTo(x - 1, cr.bottom - 3);

		if (0 == nTick % nTickCount)
		{
			// draw bigger tick (6 pixels high) and the number
			pDC->LineTo(x - 1, cr.bottom - 9);
			CString s;
			s.Format(FormatText, Db * 1.000001);

			pDC->TextOut(x + 2, cr.bottom - 6, s);

			pDC->SelectStockObject(WHITE_PEN);
			pDC->MoveTo(x, cr.bottom - 3);
			pDC->LineTo(x, cr.bottom - 9);
		}
		else
		{
			// draw small tick (2 pixels high)
			pDC->LineTo(x - 1, cr.bottom - 5);

			pDC->SelectStockObject(WHITE_PEN);
			pDC->MoveTo(x, cr.bottom - 3);
			pDC->LineTo(x, cr.bottom - 5);
		}
	}
}

void CSpectrumSectionRuler::OnSize(UINT nType, int cx, int cy)
{
	m_XOrigin = cx;
	BaseClass::OnSize(nType, cx, cy);
}


void CSpectrumSectionRuler::OnUpdate( CView* /*pSender*/,
									LPARAM /*lHint*/, CObject* /*pHint*/ )
{
	// empty. Don't invalidate on updates
}

/////////////////////////////////////////////////////////////////////////////
// CSpectrumSectionRuler diagnostics

#ifdef _DEBUG
void CSpectrumSectionRuler::AssertValid() const
{
	CHorizontalRuler::AssertValid();
}

void CSpectrumSectionRuler::Dump(CDumpContext& dc) const
{
	CHorizontalRuler::Dump(dc);
}

CWaveSoapFrontDoc* CSpectrumSectionRuler::GetDocument() // non-debug version is inline
{
	ASSERT(m_pDocument->IsKindOf(RUNTIME_CLASS(CWaveSoapFrontDoc)));
	return (CWaveSoapFrontDoc*)m_pDocument;
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CSpectrumSectionRuler message handlers

int CSpectrumSectionRuler::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	return CHorizontalRuler::OnCreate(lpCreateStruct);
}

void CSpectrumSectionRuler::BeginMouseTracking()
{
	m_MouseXOffsetForScroll = 0;
	m_DbOffsetBeforeScroll = m_DbOffset;
}

void CSpectrumSectionRuler::OnLButtonDblClk(UINT /*nFlags*/, CPoint /*point*/)
{
	double scale = 1.;
	NotifySiblingViews(SpectrumSectionScaleChange, &scale);
}

void CSpectrumSectionRuler::HorizontalScrollByPixels(int Pixels)
{
// Pixels >0 - picture moves to the right, pixels <0 - picture moves to the left
	m_MouseXOffsetForScroll += Pixels;
	double offset = m_DbOffsetBeforeScroll - m_MouseXOffsetForScroll * m_DbPerPixel;

	NotifySiblingViews(SpectrumSectionHorScrollTo, &offset);
}

void CSpectrumSectionRuler::HorizontalScrollTo(double DbOffset)
{
	// FirstSample is aligned to multiple of HorizontalScale
	int ScrollPixels = int (-DbOffset / m_DbPerPixel + 0.5) - int(-m_DbOffset / m_DbPerPixel + 0.5);
	m_DbOffset = DbOffset;

	//ScrollWindow(-ScrollPixels, 0);
	Invalidate(FALSE);
}

void CSpectrumSectionRuler::OnContextMenu(CWnd* /*pWnd*/, CPoint point)
{
	// make sure window is active
	GetParentFrame()->ActivateFrame();

	CMenu menu;

	if (!menu.LoadMenu(IDR_MENU_POPUP_SS_RULER))
	{
		return;
	}

	CMenu* pPopup = menu.GetSubMenu(0);
	if (pPopup == NULL)
	{
		return;
	}
	int Command = pPopup->TrackPopupMenu(
										TPM_LEFTALIGN | TPM_RIGHTBUTTON | TPM_RETURNCMD,
										point.x, point.y,
										AfxGetMainWnd()); // use main window for cmds

	if (0 != Command)
	{
		AfxGetMainWnd()->SendMessage(WM_COMMAND, Command & 0xFFFF, 0);
	}
}

afx_msg LRESULT CSpectrumSectionRuler::OnUwmNotifyViews(WPARAM wParam, LPARAM lParam)
{
	switch (wParam)
	{
	case SpectrumSectionDbOriginChange:
		HorizontalScrollTo(*(double*)lParam);
		break;

	case SpectrumSectionDbScaleChange:
	{
		double scale = *(double*)lParam;
		if (scale == m_DbPerPixel)
		{
			break;
		}
		m_DbPerPixel = scale;
		Invalidate(TRUE);
	}
		break;

	}

	return 0;
}

BOOL CAmplitudeRuler::OnEraseBkgnd(CDC* pDC)
{
	// TODO: Add your message handler code here and/or call default
	return TRUE;
}
