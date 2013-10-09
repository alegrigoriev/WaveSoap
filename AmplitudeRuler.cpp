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

#define TRACE_DRAWING 1
/////////////////////////////////////////////////////////////////////////////
// CAmplitudeRuler

IMPLEMENT_DYNCREATE(CAmplitudeRuler, CVerticalRuler)

CAmplitudeRuler::CAmplitudeRuler()
	:m_DrawMode(PercentView)
	, m_VerticalScale(1.)
	, m_WaveOffsetY(0.)
	, m_WaveOffsetBeforeScroll(0)
	, m_MouseYOffsetForScroll(0)
{
	memzero(m_Heights);
	memzero(m_InvalidAreaTop);
	memzero(m_InvalidAreaBottom);
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

	memzero(m_InvalidAreaBottom);
	memzero(m_InvalidAreaTop);
	// Draw with double buffering
	CRect cr;
	GetClientRect(cr);

	CDC dc;
	dc.CreateCompatibleDC(NULL);
	CBitmap DrawBitmap;
	DrawBitmap.CreateCompatibleBitmap(pDrawDC, cr.Width(), cr.Height());

	CDC * pDC = & dc;

	CGdiObjectSaveT<CBitmap> OldBitmap(pDC, pDC->SelectObject(& DrawBitmap));
	CGdiObjectSave OldFont(pDC, pDC->SelectStockObject(ANSI_VAR_FONT));
	CGdiObjectSave OldPen(pDC, pDC->SelectStockObject(BLACK_PEN));

	CBrush bkgnd;
	TRACE("SysColor(COLOR_WINDOW)=%X\n", GetSysColor(COLOR_WINDOW));
	bkgnd.CreateSysColorBrush(COLOR_WINDOW);

	pDC->SetTextAlign(TA_BOTTOM | TA_RIGHT);
	pDC->SetTextColor(0x000000);   // black
	pDC->FillRect(cr, &bkgnd);
	pDC->SetBkMode(TRANSPARENT);

	NUMBER_OF_CHANNELS nChannels = pDoc->WaveChannels();

	for (int ch = 0; ch < nChannels; ch++)
	{
		if ( m_Heights.ch[ch].minimized)
		{
			continue;
		}

		TRACE("CAmplitudeRuler::OnDraw: ch %d zero pos =%d\n",
			ch, (int)WaveCalculate(m_WaveOffsetY, m_VerticalScale, m_Heights.ch[ch].top, m_Heights.ch[ch].bottom)(0.));
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

		if (ch < nChannels - 1)
		{
			pDC->MoveTo(0, chr.bottom);
			pDC->LineTo(cr.right, chr.bottom);
		}
	}
	pDrawDC->BitBlt(0, 0, cr.Width(), cr.Height(), pDC, 0, 0, SRCCOPY);
}

void CAmplitudeRuler::DrawChannelSamples(CDC * pDC, CRect const & chr, CRect const & clipr)
{
	TEXTMETRIC tm;
	pDC->GetTextMetrics( & tm);

	int nHeight = chr.Height();

	int nVertStep = GetSystemMetrics(SM_CYMENU);

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

	int ClipHigh = clipr.bottom - tm.tmHeight / 2;
	int ClipLow = clipr.top + tm.tmHeight / 2;

	int yLow = (int)WaveToY.ConvertToSample(ClipHigh);
	// round to the next multiple of step
	yLow += (step * 0x10000 - yLow) % step;

	int yHigh = (int)WaveToY.ConvertToSample(ClipLow);
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

void CAmplitudeRuler::DrawChannelPercents(CDC * pDC, CRect const & chr, CRect const & clipr)
{
	TEXTMETRIC tm;
	pDC->GetTextMetrics( & tm);

	int nVertStep = GetSystemMetrics(SM_CYMENU);

	int nHeight = chr.Height();

	double nSampleUnits = nVertStep * 200. / (nHeight * m_VerticalScale);

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

	int ClipHigh = clipr.bottom - tm.tmHeight / 2;
	int ClipLow = clipr.top + tm.tmHeight / 2;

	int yLow = int(100. / 32768. * WaveToY.ConvertToSample(ClipHigh));
	// round to the next multiple of step
	yLow += (step * 0x10000 - yLow) % step;

	int yHigh = int(100. / 32768. * WaveToY.ConvertToSample(ClipLow));

	yHigh -= (step * 0x10000 + yHigh) % step;

	for (int y = yLow; y <= yHigh; y += step)
	{
		int yDev= WaveToY(int(fround(y * 32768. / 100.)));

		if (0 == y)
		{
			if (TRACE_DRAWING) TRACE("CAmplitudeRuler Zero pos=%d\n", yDev);
		}

		pDC->MoveTo(chr.right - 3, yDev);
		pDC->LineTo(chr.right, yDev);
		CString s;
		s.Format(_T("%d%%"), y);

		pDC->TextOut(chr.right - 3, yDev + tm.tmHeight / 2, s);
	}
}

void CAmplitudeRuler::DrawChannelDecibels(CDC * pDC, CRect const & chr, CRect const & clipr)
{
	// decibels are drawn with 1.5 dB step
	// if there is not enough space to draw next 2*step, it doubles the step
	TEXTMETRIC tm;
	pDC->GetTextMetrics( & tm);

	double nVertStep = -GetSystemMetrics(SM_CYMENU) / (m_VerticalScale);// FIXME

	int ClipHigh = clipr.bottom - tm.tmHeight / 2;
	int ClipLow = clipr.top + tm.tmHeight / 2;

	WaveCalculate WaveToY(m_WaveOffsetY, m_VerticalScale, chr.top, chr.bottom);

	int yLow = (int)WaveToY.ConvertToSample(ClipHigh);

	int yHigh = (int)WaveToY.ConvertToSample(ClipLow);

	ASSERT(yLow <= yHigh);

	double MultStep = 0.841395141; //pow(10., -1.5 / 20.);
	double DbStep = -1.5;

	double CurrDb = -1.5;
	double CurrVal = 27570.836;

	while (CurrDb > -120.)
	{
		double yDev1 = CurrVal;
		if (yDev1 < nVertStep)
		{
			break;  // can't draw anymore
		}
		// if it's not multiple of DbStep*2, see if we need to multiply step and skip this value
		if (0. != fmod(CurrDb, DbStep * 2.))
		{
			// check if we have enough space to draw the next value
			double yDev2 = CurrVal * MultStep;

			if (yDev1 - yDev2 < nVertStep)
			{
				CurrVal *= MultStep;
				MultStep *= MultStep;

				CurrDb += DbStep;
				DbStep *= 2;
				continue;
			}
		}

		CString s;
		s.Format(_T("%.1f dB"), CurrDb);

		if (CurrVal >= yLow && CurrVal <= yHigh)
		{
			int yDev = (int)WaveToY(CurrVal);

			pDC->MoveTo(chr.right - 3, yDev);
			pDC->LineTo(chr.right, yDev);

			pDC->TextOut(chr.right - 3, yDev + tm.tmHeight / 2, s);
		}
		if (-CurrVal >= yLow && -CurrVal <= yHigh)
		{
			int yDev = (int)WaveToY(-CurrVal);
			pDC->MoveTo(chr.right - 3, yDev);
			pDC->LineTo(chr.right, yDev);

			pDC->TextOut(chr.right - 3, yDev + tm.tmHeight / 2, s);
		}

		CurrVal *= MultStep;
		CurrDb += DbStep;
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

void CAmplitudeRuler::VerticalScrollPixels(int Pixels)
{
	if (m_MouseYOffsetForScroll == 0)
	{
		m_WaveOffsetBeforeScroll = m_WaveOffsetY;
	}
	m_MouseYOffsetForScroll += Pixels;

	double offset = m_WaveOffsetBeforeScroll + m_MouseYOffsetForScroll / m_VerticalScale * 65536. / m_Heights.NominalChannelHeight;
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
	double one = 1.0;
	NotifySiblingViews(VerticalScaleChanged, &one);
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
		ScrollRect.top += m_InvalidAreaTop[ch];
		ScrollRect.bottom -= m_InvalidAreaBottom[ch];
		if (ToScroll > 0)
		{
			// down
			m_InvalidAreaTop[ch] += ToScroll;
			ScrollRect.bottom -= ToScroll;
			if (ScrollRect.Height() <= 0)
			{
//                InvalidateRect(ClipRect);
				continue;
			}
			CRect ToInvalidate(cr.left, ScrollRect.top, cr.right, ScrollRect.top + ToScroll);

			ScrollWindowEx(0, ToScroll, ScrollRect, ClipRect, NULL, NULL, 0);
//            InvalidateRect(ToInvalidate);
		}
		else if (ToScroll < 0)
		{
			// up
			m_InvalidAreaBottom[ch] -= ToScroll;
			ScrollRect.top -= ToScroll;
			if (ScrollRect.Height() <= 0)
			{
				InvalidateRect(ClipRect, FALSE);
				continue;
			}
			CRect ToInvalidate(cr.left, ScrollRect.top, cr.right, ScrollRect.top + ToScroll);

			ScrollWindowEx(0, ToScroll, ScrollRect, ClipRect, NULL, NULL, 0);
			InvalidateRect(ToInvalidate, FALSE);
		}
		else
		{
			continue;
		}
	}
	m_WaveOffsetY = offset;
	Invalidate(FALSE);
}

void CAmplitudeRuler::OnCaptureChanged(CWnd *pWnd)
{
	m_MouseYOffsetForScroll = 0;
	CVerticalRuler::OnCaptureChanged(pWnd);
}

UINT CAmplitudeRuler::GetPopupMenuID(CPoint point)
{
	// point is in screen coordinates
	CWaveSoapFrontDoc * pDoc = GetDocument();
	ScreenToClient( & point);
	for (int i = 0; i < pDoc->WaveChannels(); i++)
	{
		if (point.y > m_Heights.ch[i].clip_bottom)
		{
			continue;
		}
		if (m_Heights.ch[i].minimized)
		{
			return IDR_MENU_AMPLITUDE_RULER_MINIMIZED;
		}
		else
		{
			return IDR_MENU_AMPLITUDE_RULER;
		}
	}
	return IDR_MENU_AMPLITUDE_RULER;
}

void CAmplitudeRuler::OnContextMenu(CWnd* /*pWnd*/, CPoint point)
{
	// make sure window is active
	GetParentFrame()->ActivateFrame();

	CMenu menu;
	CMenu* pPopup = NULL;

	UINT uID = GetPopupMenuID(point);

	if (uID != 0 && menu.LoadMenu(uID))
	{
		pPopup = menu.GetSubMenu(0);
	}

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
}

afx_msg LRESULT CAmplitudeRuler::OnUwmNotifyViews(WPARAM wParam, LPARAM lParam)
{
	switch (wParam)
	{
	case VerticalScaleChanged:
		// lParam points to double new scale
		if (m_VerticalScale != *(double*)lParam)
		{
			m_VerticalScale = *(double*)lParam;
			if (m_bIsTrackingSelection)
			{
				ReleaseCapture();
				//m_bIsTrackingSelection = FALSE;   // will be reset in WM_CAPTURECHANGED
			}

			Invalidate();
			// check for the proper offset, correct if necessary
			m_WaveOffsetY = WaveCalculate(m_WaveOffsetY, m_VerticalScale, 0, m_Heights.NominalChannelHeight).AdjustOffset(m_WaveOffsetY);
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
		Invalidate(FALSE);
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
	, m_DbRange(120.)   // max dB range
	, m_DbRangeInView(70.)
	, m_DbPerPixel(1.)
{
}

CSpectrumSectionRuler::~CSpectrumSectionRuler()
{
}


BEGIN_MESSAGE_MAP(CSpectrumSectionRuler, BaseClass)
	//{{AFX_MSG_MAP(CSpectrumSectionRuler)
	//}}AFX_MSG_MAP
	//ON_WM_CREATE()
	ON_MESSAGE(UWM_NOTIFY_VIEWS, &CSpectrumSectionRuler::OnUwmNotifyViews)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CSpectrumSectionRuler drawing

double CSpectrumSectionRuler::WindowXToDb(int x) const
{
	return x * m_DbPerPixel + m_DbOffset;
}

int CSpectrumSectionRuler::DbToWindowX(double db) const
{
	return int((db - m_DbOffset) / m_DbPerPixel + 0.5);     // round
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
	double LeftExt = fabs(WindowXToDb(cr.left));
	double RightExt = fabs(WindowXToDb(cr.right));
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

	double Dist = fabs(1.5 * nLength / m_DbPerPixel);
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
		double Db = nFirstDb + DistDb * nTick / double(nTickCount);
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
void CSpectrumSectionRuler::HorizontalScrollByPixels(int Pixels)
{
	// TODO
}

afx_msg LRESULT CSpectrumSectionRuler::OnUwmNotifyViews(WPARAM wParam, LPARAM lParam)
{
	switch (wParam)
	{
	case SpectrumSectionDbOffsetChange:
		break;

	case SpectrumSectionDbScaleChange:
		break;

	case SpectrumSectionScrollPixels:
		break;
	}

	return 0;
}
