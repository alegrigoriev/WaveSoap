// AmplitudeRuler.cpp : implementation file
//

#include "stdafx.h"
#include "WaveSoapFront.h"
#include "AmplitudeRuler.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CAmplitudeRuler

IMPLEMENT_DYNCREATE(CAmplitudeRuler, CVerticalRuler)

CAmplitudeRuler::CAmplitudeRuler()
	:m_DrawMode(PercentView)
{
}

CAmplitudeRuler::~CAmplitudeRuler()
{
}


BEGIN_MESSAGE_MAP(CAmplitudeRuler, CVerticalRuler)
	//{{AFX_MSG_MAP(CAmplitudeRuler)
	ON_WM_CREATE()
	ON_WM_LBUTTONDBLCLK()
	ON_COMMAND(IDC_VIEW_AMPL_RULER_SAMPLES, OnViewAmplRulerSamples)
	ON_COMMAND(IDC_VIEW_AMPL_RULER_PERCENT, OnViewAmplRulerPercent)
	ON_COMMAND(IDC_VIEW_AMPL_RULER_DECIBELS, OnViewAmplRulerDecibels)
	//}}AFX_MSG_MAP
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

void CAmplitudeRuler::OnDraw(CDC* pDC)
{
	CGdiObject * pOldFont = (CFont *) pDC->SelectStockObject(ANSI_VAR_FONT);
	CGdiObject * OldPen = pDC->SelectStockObject(BLACK_PEN);
	switch (m_DrawMode)
	{
	case PercentView:
		DrawPercents(pDC);
		break;
	case DecibelView:
		DrawDecibels(pDC);
		break;
	default:
		DrawSamples(pDC);
		break;
	}
	pDC->SelectObject(OldPen);
	pDC->SelectObject(pOldFont);
}

void CAmplitudeRuler::DrawSamples(CDC * pDC)
{
	CWaveSoapFrontDoc* pDoc = GetDocument();
	if (! pDoc->m_WavFile.IsOpen())
	{
		return;
	}

	CWaveSoapFrontView * pMasterView = DYNAMIC_DOWNCAST(CWaveSoapFrontView, m_pVertMaster);
	if (NULL == pMasterView)
	{
		return; // not attached
	}

	CRect cr;
	GetClientRect(cr);

	TEXTMETRIC tm;
	pDC->GetTextMetrics( & tm);
	pDC->SetTextAlign(TA_BOTTOM | TA_RIGHT);
	pDC->SetTextColor(0x000000);   // black
	pDC->SetBkMode(TRANSPARENT);
	int nVertStep = GetSystemMetrics(SM_CYMENU);
	int nChannels = pDoc->WaveChannels();
	int nHeight = cr.Height() / nChannels;
	double VerticalScale = pMasterView->m_VerticalScale;
	double ScaledWaveOffset = pMasterView->m_WaveOffsetY * VerticalScale;
	int nSampleUnits = nVertStep * 65536. / (nHeight * VerticalScale);
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
	double YScaleDev = pMasterView->GetYScaleDev();
	int ChannelSeparatorY = fround((0 - pMasterView->dOrgY) * YScaleDev);
	if (2 == nChannels)
	{
		pDC->MoveTo(0, ChannelSeparatorY);
		pDC->LineTo(cr.right, ChannelSeparatorY);
	}
	for (int ch = 0; ch < nChannels; ch++)
	{
		double WaveOffset = ScaledWaveOffset - pMasterView->dOrgY;
		int ClipHigh = cr.bottom;
		int ClipLow = cr.top;
		if (nChannels > 1)
		{
			if (0 == ch)
			{
				WaveOffset = ScaledWaveOffset + 32768. - pMasterView->dOrgY;
				ClipHigh = ChannelSeparatorY;
			}
			else
			{
				WaveOffset = ScaledWaveOffset -32768. - pMasterView->dOrgY;
				ClipLow = ChannelSeparatorY + 1;
			}
		}
		ClipLow += tm.tmHeight / 2;
		ClipHigh -= tm.tmHeight / 2;
		int yLow = (ClipHigh / YScaleDev -WaveOffset) / VerticalScale;
		// round to the next multiple of step
		yLow += (step*0x10000-yLow) % step;
		int yHigh = (ClipLow / YScaleDev -WaveOffset) / VerticalScale;
		yHigh -= (step*0x10000+yHigh) % step;
		ASSERT(yLow <= yHigh);
		for (int y = yLow; y <= yHigh; y += step)
		{
			int yDev= fround((y * VerticalScale + WaveOffset) * YScaleDev);
			pDC->MoveTo(cr.right - 3, yDev);
			pDC->LineTo(cr.right, yDev);
			CString s = LtoaCS(y);

			pDC->TextOut(cr.right - 3, yDev + tm.tmHeight / 2, s);
		}
	}
}

void CAmplitudeRuler::DrawPercents(CDC * pDC)
{
	CWaveSoapFrontDoc* pDoc = GetDocument();
	if (! pDoc->m_WavFile.IsOpen())
	{
		return;
	}

	CWaveSoapFrontView * pMasterView = DYNAMIC_DOWNCAST(CWaveSoapFrontView, m_pVertMaster);
	if (NULL == pMasterView)
	{
		return; // not attached
	}

	CRect cr;
	GetClientRect(cr);

	TEXTMETRIC tm;
	pDC->GetTextMetrics( & tm);
	pDC->SetTextAlign(TA_BOTTOM | TA_RIGHT);
	pDC->SetTextColor(0x000000);   // black
	pDC->SetBkMode(TRANSPARENT);
	int nVertStep = GetSystemMetrics(SM_CYMENU);
	int nChannels = pDoc->WaveChannels();
	int nHeight = cr.Height() / nChannels;
	double VerticalScale = pMasterView->m_VerticalScale;
	double ScaledWaveOffset = pMasterView->m_WaveOffsetY * VerticalScale;
	double nSampleUnits = nVertStep * 200. / (nHeight * VerticalScale);
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
	double YScaleDev = pMasterView->GetYScaleDev();
	int ChannelSeparatorY = fround((0 - pMasterView->dOrgY) * YScaleDev);
	if (2 == nChannels)
	{
		pDC->MoveTo(0, ChannelSeparatorY);
		pDC->LineTo(cr.right, ChannelSeparatorY);
	}
	for (int ch = 0; ch < nChannels; ch++)
	{
		double WaveOffset = ScaledWaveOffset - pMasterView->dOrgY;
		int ClipHigh = cr.bottom;
		int ClipLow = cr.top;
		if (nChannels > 1)
		{
			if (0 == ch)
			{
				WaveOffset = ScaledWaveOffset + 32768. - pMasterView->dOrgY;
				ClipHigh = ChannelSeparatorY;
			}
			else
			{
				WaveOffset = ScaledWaveOffset -32768. - pMasterView->dOrgY;
				ClipLow = ChannelSeparatorY + 1;
			}
		}
		ClipLow += tm.tmHeight / 2;
		ClipHigh -= tm.tmHeight / 2;
		int yLow = 100. / 32768. * (ClipHigh / YScaleDev -WaveOffset) / VerticalScale;
		// round to the next multiple of step
		yLow += (step*0x10000-yLow) % step;
		int yHigh = 100. / 32768. * (ClipLow / YScaleDev -WaveOffset) / VerticalScale;
		yHigh -= (step*0x10000+yHigh) % step;
		ASSERT(yLow <= yHigh);
		for (int y = yLow; y <= yHigh; y += step)
		{
			int yDev= fround((y * 32768. / 100.* VerticalScale + WaveOffset) * YScaleDev);
			pDC->MoveTo(cr.right - 3, yDev);
			pDC->LineTo(cr.right, yDev);
			CString s;
			s.Format(_T("%d%%"), y);

			pDC->TextOut(cr.right - 3, yDev + tm.tmHeight / 2, s);
		}
	}
}

void CAmplitudeRuler::DrawDecibels(CDC * pDC)
{
	// decibels are drawn with 1.5 dB step
	CWaveSoapFrontDoc* pDoc = GetDocument();
	if (! pDoc->m_WavFile.IsOpen())
	{
		return;
	}

	CWaveSoapFrontView * pMasterView = DYNAMIC_DOWNCAST(CWaveSoapFrontView, m_pVertMaster);
	if (NULL == pMasterView)
	{
		return; // not attached
	}

	CRect cr;
	GetClientRect(cr);

	TEXTMETRIC tm;
	pDC->GetTextMetrics( & tm);
	pDC->SetTextAlign(TA_BOTTOM | TA_RIGHT);
	pDC->SetTextColor(0x000000);   // black
	pDC->SetBkMode(TRANSPARENT);
	int nVertStep = GetSystemMetrics(SM_CYMENU);
	int nChannels = pDoc->WaveChannels();
	int nHeight = cr.Height() / nChannels;
	double VerticalScale = pMasterView->m_VerticalScale;
	double ScaledWaveOffset = pMasterView->m_WaveOffsetY * VerticalScale;
	int nSampleUnits = nVertStep * 65536. / (nHeight * VerticalScale);
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
	double YScaleDev = pMasterView->GetYScaleDev();
	int ChannelSeparatorY = fround((0 - pMasterView->dOrgY) * YScaleDev);
	if (2 == nChannels)
	{
		pDC->MoveTo(0, ChannelSeparatorY);
		pDC->LineTo(cr.right, ChannelSeparatorY);
	}
	for (int ch = 0; ch < nChannels; ch++)
	{
		double WaveOffset = ScaledWaveOffset - pMasterView->dOrgY;
		int ClipHigh = cr.bottom;
		int ClipLow = cr.top;
		if (nChannels > 1)
		{
			if (0 == ch)
			{
				WaveOffset = ScaledWaveOffset + 32768. - pMasterView->dOrgY;
				ClipHigh = ChannelSeparatorY;
			}
			else
			{
				WaveOffset = ScaledWaveOffset -32768. - pMasterView->dOrgY;
				ClipLow = ChannelSeparatorY + 1;
			}
		}
		ClipLow += tm.tmHeight / 2;
		ClipHigh -= tm.tmHeight / 2;
		int yLow = (ClipHigh / YScaleDev -WaveOffset) / VerticalScale;
		// round to the next multiple of step
		yLow += (step*0x10000-yLow) % step;
		int yHigh = (ClipLow / YScaleDev -WaveOffset) / VerticalScale;
		yHigh -= (step*0x10000+yHigh) % step;
		ASSERT(yLow <= yHigh);
		for (int y = yLow; y <= yHigh; y += step)
		{
			int yDev= fround((y * VerticalScale + WaveOffset) * YScaleDev);
			pDC->MoveTo(cr.right - 3, yDev);
			pDC->LineTo(cr.right, yDev);
			CString s;
			s.Format(_T("%d"), y);

			pDC->TextOut(cr.right - 3, yDev + tm.tmHeight / 2, s);
		}
	}
}

int CAmplitudeRuler::CalculateWidth()
{
	CWnd * pW = GetDesktopWindow();
	CDC * pDC = pW->GetWindowDC();
	CGdiObject * pOld = pDC->SelectStockObject(ANSI_VAR_FONT);
	int Width = 4 + pDC->GetTextExtent("-000,000", 8).cx;

	pDC->SelectObject(pOld);
	pW->ReleaseDC(pDC);
	return Width;
}

void CAmplitudeRuler::OnUpdate( CView* pSender, LPARAM lHint, CObject* pHint )
{
	if (lHint == CWaveSoapFrontDoc::UpdateWholeFileChanged)
	{
		UpdateMaxExtents();
	}
	else if ((lHint == CWaveSoapFrontView::WAVE_OFFSET_CHANGED
				|| lHint == CWaveSoapFrontView::WAVE_SCALE_CHANGED)
			&& NULL == pHint)
	{
		Invalidate();
	}
}

/////////////////////////////////////////////////////////////////////////////
// CAmplitudeRuler diagnostics

#ifdef _DEBUG
void CAmplitudeRuler::AssertValid() const
{
	CVerticalRuler::AssertValid();
}

void CAmplitudeRuler::Dump(CDumpContext& dc) const
{
	CVerticalRuler::Dump(dc);
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

	KeepAspectRatio(FALSE);
	KeepScaleOnResizeX(FALSE);
	KeepScaleOnResizeY(FALSE);
	KeepOrgOnResizeX(FALSE);
	KeepOrgOnResizeY(TRUE);

	UpdateMaxExtents();

	ShowScrollBar(SB_VERT, FALSE);
	ShowScrollBar(SB_HORZ, FALSE);

	return 0;
}

void CAmplitudeRuler::UpdateMaxExtents()
{
	int nChannels = GetDocument()->WaveChannels();
	int nLowExtent = -32768;
	int nHighExtent = 32767;
	if (nChannels > 1)
	{
		nLowExtent = -0x10000;
		nHighExtent = 0x10000;
	}

	// don't want to go through the master window
	SetMaxExtentsMaster(0., 1., nLowExtent, nHighExtent);
	SetExtents(0., 1., nLowExtent, nHighExtent);
}

void CAmplitudeRuler::OnLButtonDblClk(UINT nFlags, CPoint point)
{
	CVerticalRuler::OnLButtonDblClk(nFlags, point);
	// set default scale
	CWaveSoapFrontView * pView =
		DYNAMIC_DOWNCAST(CWaveSoapFrontView, m_pVertMaster);
	if (NULL != pView)
	{
		pView->OnViewZoomvertNormal();
	}
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

/////////////////////////////////////////////////////////////////////////////
// CSpectrumSectionRuler

IMPLEMENT_DYNCREATE(CSpectrumSectionRuler, CHorizontalRuler)

CSpectrumSectionRuler::CSpectrumSectionRuler()
{
}

CSpectrumSectionRuler::~CSpectrumSectionRuler()
{
}


BEGIN_MESSAGE_MAP(CSpectrumSectionRuler, CHorizontalRuler)
	//{{AFX_MSG_MAP(CSpectrumSectionRuler)
	//}}AFX_MSG_MAP
	//ON_WM_CREATE()
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CSpectrumSectionRuler drawing

void CSpectrumSectionRuler::OnDraw(CDC* pDC)
{
	CGdiObject * pOldFont = (CFont *) pDC->SelectStockObject(ANSI_VAR_FONT);
	CGdiObject * OldPen = pDC->SelectStockObject(BLACK_PEN);
	// decibels are drawn with 1.5 dB step
	CWaveSoapFrontDoc* pDoc = GetDocument();
	if (! pDoc->m_WavFile.IsOpen())
	{
		return;
	}

	CWaveSoapFrontView * pMasterView = DYNAMIC_DOWNCAST(CWaveSoapFrontView, m_pVertMaster);
	if (NULL == pMasterView)
	{
		return; // not attached
	}
#if 0
	CRect cr;
	GetClientRect(cr);

	TEXTMETRIC tm;
	pDC->GetTextMetrics( & tm);
	pDC->SetTextAlign(TA_BOTTOM | TA_RIGHT);
	pDC->SetTextColor(0x000000);   // black
	pDC->SetBkMode(TRANSPARENT);
	int nVertStep = GetSystemMetrics(SM_CYMENU);
	int nChannels = pDoc->WaveChannels();
	int nHeight = cr.Height() / nChannels;
	double VerticalScale = pMasterView->m_VerticalScale;
	double ScaledWaveOffset = pMasterView->m_WaveOffsetY * VerticalScale;
	int nSampleUnits = nVertStep * 65536. / (nHeight * VerticalScale);
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
	double YScaleDev = pMasterView->GetYScaleDev();
	int ChannelSeparatorY = fround((0 - pMasterView->dOrgY) * YScaleDev);
	if (2 == nChannels)
	{
		pDC->MoveTo(0, ChannelSeparatorY);
		pDC->LineTo(cr.right, ChannelSeparatorY);
	}
	for (int ch = 0; ch < nChannels; ch++)
	{
		double WaveOffset = ScaledWaveOffset - pMasterView->dOrgY;
		int ClipHigh = cr.bottom;
		int ClipLow = cr.top;
		if (nChannels > 1)
		{
			if (0 == ch)
			{
				WaveOffset = ScaledWaveOffset + 32768. - pMasterView->dOrgY;
				ClipHigh = ChannelSeparatorY;
			}
			else
			{
				WaveOffset = ScaledWaveOffset -32768. - pMasterView->dOrgY;
				ClipLow = ChannelSeparatorY + 1;
			}
		}
		ClipLow += tm.tmHeight / 2;
		ClipHigh -= tm.tmHeight / 2;
		int yLow = (ClipHigh / YScaleDev -WaveOffset) / VerticalScale;
		// round to the next multiple of step
		yLow += (step*0x10000-yLow) % step;
		int yHigh = (ClipLow / YScaleDev -WaveOffset) / VerticalScale;
		yHigh -= (step*0x10000+yHigh) % step;
		ASSERT(yLow <= yHigh);
		for (int y = yLow; y <= yHigh; y += step)
		{
			int yDev= fround((y * VerticalScale + WaveOffset) * YScaleDev);
			pDC->MoveTo(cr.right - 3, yDev);
			pDC->LineTo(cr.right, yDev);
			CString s;
			s.Format(_T("%d"), y);

			pDC->TextOut(cr.right - 3, yDev + tm.tmHeight / 2, s);
		}
	}
#endif
	pDC->SelectObject(OldPen);
	pDC->SelectObject(pOldFont);
}


void CSpectrumSectionRuler::OnUpdate( CView* pSender, LPARAM lHint, CObject* pHint )
{
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
	if (CHorizontalRuler::OnCreate(lpCreateStruct) == -1)
		return -1;
#if 0
	KeepAspectRatio(FALSE);
	KeepScaleOnResizeX(FALSE);
	KeepScaleOnResizeY(FALSE);
	KeepOrgOnResizeX(FALSE);
	KeepOrgOnResizeY(TRUE);

	UpdateMaxExtents();

	ShowScrollBar(SB_VERT, FALSE);
	ShowScrollBar(SB_HORZ, FALSE);
#endif
	return 0;
}

void CSpectrumSectionRuler::UpdateMaxExtents()
{
#if 0
	int nChannels = GetDocument()->WaveChannels();
	int nLowExtent = -32768;
	int nHighExtent = 32767;
	if (nChannels > 1)
	{
		nLowExtent = -0x10000;
		nHighExtent = 0x10000;
	}

	// don't want to go through the master window
	SetMaxExtentsMaster(0., 1., nLowExtent, nHighExtent);
	SetExtents(0., 1., nLowExtent, nHighExtent);
#endif
}

