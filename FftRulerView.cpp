// Copyright Alexander Grigoriev, 1997-2002, All Rights Reserved
// FftRulerView.cpp : implementation file
//

#include "stdafx.h"
#include "WaveSoapFront.h"
#include "FftRulerView.h"
#include "GdiObjectSave.h"
#include "WaveSoapFrontDoc.h"
#include "WaveFftView.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CFftRulerView

IMPLEMENT_DYNCREATE(CFftRulerView, CVerticalRuler)

CFftRulerView::CFftRulerView()
	: m_VerticalScale(1.)
	, m_FirstbandVisible(0.)
	, m_FftOffsetBeforeScroll(0)
	, m_MouseYOffsetForScroll(0)
	, m_FftOrder(1024)
{
	memzero(m_Heights);
	memzero(m_InvalidAreaTop);
	memzero(m_InvalidAreaBottom);
}

CFftRulerView::~CFftRulerView()
{
}


BEGIN_MESSAGE_MAP(CFftRulerView, CVerticalRuler)
	//{{AFX_MSG_MAP(CFftRulerView)
	ON_WM_LBUTTONDBLCLK()
	ON_WM_CONTEXTMENU()
	ON_WM_CAPTURECHANGED()
	//}}AFX_MSG_MAP
	ON_MESSAGE(UWM_NOTIFY_VIEWS, &CFftRulerView::OnUwmNotifyViews)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CFftRulerView drawing

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

void CFftRulerView::OnDraw(CDC* pDrawDC)
{
	CWaveSoapFrontDoc* pDoc = GetDocument();
	if (! pDoc->m_WavFile.IsOpen())
	{
		return;
	}

	CRect cr;
	GetClientRect(cr);

	memzero(m_InvalidAreaBottom);
	memzero(m_InvalidAreaTop);
	int nVertStep = GetSystemMetrics(SM_CYMENU);

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

	CBrush bkgnd;
	if (0) TRACE("SysColor(COLOR_WINDOW)=%X\n", GetSysColor(COLOR_WINDOW));
	bkgnd.CreateSysColorBrush(COLOR_WINDOW);
	pDC->FillRect(cr, &bkgnd);

	TEXTMETRIC tm;
	pDC->GetTextMetrics( & tm);
	pDC->SetTextAlign(TA_BOTTOM | TA_RIGHT);
	pDC->SetTextColor(0x000000);   // black
	pDC->SetBkMode(TRANSPARENT);

	for (int ch = 0; ch < nChannels; ch++)
	{
		if (m_Heights.ch[ch].minimized)
		{
			continue;
		}

		int ClipHigh = m_Heights.ch[ch].clip_bottom;
		int ClipLow = m_Heights.ch[ch].clip_top;

		// if all the chart was drawn, how many scans it would have:
		int TotalRows = int(m_Heights.NominalChannelHeight * m_VerticalScale);

		if (0 == TotalRows)
		{
			continue;
		}

		int LastFftSample = int(m_FftOrder - m_FirstbandVisible);
		int FirstFftSample = LastFftSample + (-m_Heights.NominalChannelHeight * m_FftOrder) / TotalRows;
		if (FirstFftSample < 0)
		{
			LastFftSample -= FirstFftSample;
			FirstFftSample = 0;
		}

		//int FirstRowInView = FirstFftSample * TotalRows / pMasterView->m_FftOrder;
		//int FftSamplesInView = LastFftSample - FirstFftSample + 1;

		int nSampleUnits = int(nVertStep * pDoc->WaveSampleRate() / (m_Heights.NominalChannelHeight * m_VerticalScale));
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

		ClipLow += tm.tmHeight / 2;
		ClipHigh -= tm.tmHeight / 2;

		int yLow = int(m_FirstbandVisible * 0.5 * pDoc->WaveSampleRate() / m_FftOrder);
		// round to the next multiple of step
		yLow += (step*0x10000-yLow) % step;

		int yHigh = int(yLow + 0.5 * pDoc->WaveSampleRate() / m_VerticalScale);
		yHigh -= (step*0x10000+yHigh) % step;
		ASSERT(yLow <= yHigh);

		for (int y = yLow; y <= yHigh; y += step)
		{
			// y is frequency
			double band = double(y) / (0.5 * pDoc->WaveSampleRate())
						* m_FftOrder;
			int yDev= m_Heights.ch[ch].clip_bottom - (int)fround((band - m_FirstbandVisible) / m_FftOrder * m_Heights.NominalChannelHeight * m_VerticalScale);

			if (yDev - tm.tmHeight/2 < ClipLow
				|| yDev + tm.tmHeight/2 > ClipHigh)
			{
				continue;
			}

			pDC->MoveTo(cr.right - 3, yDev);
			pDC->LineTo(cr.right, yDev);
			CString s = LtoaCS(y);

			pDC->TextOut(cr.right - 3, yDev + tm.tmHeight / 2, s);
		}

		if (ch != nChannels - 1)
		{
			pDC->MoveTo(0, m_Heights.ch[ch].clip_bottom);
			pDC->LineTo(cr.right, m_Heights.ch[ch].clip_bottom);
		}
	}
	pDrawDC->BitBlt(0, 0, cr.Width(), cr.Height(), pDC, 0, 0, SRCCOPY);
}

int CFftRulerView::CalculateWidth()
{
	CWindowDC wDC(GetDesktopWindow());

	CGdiObjectSave Old(wDC, wDC.SelectStockObject(ANSI_VAR_FONT));
	int Width = 4 + wDC.GetTextExtent(_T("-000,000"), 8).cx;

	return Width;
}

void CFftRulerView::OnUpdate( CView* /*pSender*/, LPARAM lHint, CObject* /*pHint*/ )
{
	if (lHint == CWaveSoapFrontDoc::UpdateWholeFileChanged
		|| lHint == CWaveSoapFrontDoc::UpdateSampleRateChanged)
	{
		Invalidate();
	}
}

void CFftRulerView::OnLButtonDblClk(UINT nFlags, CPoint point)
{
	CVerticalRuler::OnLButtonDblClk(nFlags, point);
	// set default scale
	AfxGetMainWnd()->SendMessage(WM_COMMAND, ID_VIEW_SS_ZOOMVERT_NORMAL);
}

/////////////////////////////////////////////////////////////////////////////
// CFftRulerView diagnostics

#ifdef _DEBUG
void CFftRulerView::AssertValid() const
{
	CVerticalRuler::AssertValid();
}

void CFftRulerView::Dump(CDumpContext& dc) const
{
	CVerticalRuler::Dump(dc);
}
CWaveSoapFrontDoc* CFftRulerView::GetDocument() // non-debug version is inline
{
	ASSERT(m_pDocument->IsKindOf(RUNTIME_CLASS(CWaveSoapFrontDoc)));
	return (CWaveSoapFrontDoc*)m_pDocument;
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CFftRulerView message handlers

double CFftRulerView::AdjustOffset(double offset) const
{
	if (offset < 0.)
	{
		return 0;
	}

	int ScaledHeight = int(m_Heights.NominalChannelHeight * m_VerticalScale);
	int MaxOffsetPixels = ScaledHeight - m_Heights.NominalChannelHeight;

	double MaxOffset = double(MaxOffsetPixels) * m_FftOrder / ScaledHeight;
	ASSERT(MaxOffset >= 0);
	ASSERT(offset >= 0);
	if (offset > MaxOffset)
	{
		return MaxOffset;
	}
	return offset;
}

void CFftRulerView::SetNewFftOffset(double first_band)
{
	// scroll channels rectangles
	CWaveSoapFrontDoc * pDoc = GetDocument();
	int nChannels = pDoc->WaveChannels();
	CRect cr;
	GetClientRect(cr);

	long OldOffsetPixels = long(m_FirstbandVisible * int(m_VerticalScale * m_Heights.NominalChannelHeight) / m_FftOrder);
	long NewOffsetPixels = long(first_band * int(m_VerticalScale * m_Heights.NominalChannelHeight) / m_FftOrder);

	int ToScroll = NewOffsetPixels - OldOffsetPixels;       // >0 - down, <0 - up

	for (int ch = 0; ch < nChannels; ch++)
	{
		if (m_Heights.ch[ch].minimized)
		{
			continue;
		}

		// offset of the zero line down

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
//                InvalidateRect(ClipRect, FALSE);
				continue;
			}
			CRect ToInvalidate(cr.left, ScrollRect.top, cr.right, ScrollRect.top + ToScroll);

			ScrollWindowEx(0, ToScroll, ScrollRect, ClipRect, NULL, NULL, 0);
//            InvalidateRect(ToInvalidate, FALSE);
		}
		else
		{
			continue;
		}
	}
	m_FirstbandVisible = first_band;
	Invalidate(FALSE);
}

afx_msg LRESULT CFftRulerView::OnUwmNotifyViews(WPARAM wParam, LPARAM lParam)
{
	switch (wParam)
	{
	case FftVerticalScaleChanged:
		// lParam points to double new scale
		if (m_VerticalScale != *(double*)lParam)
		{
			m_VerticalScale = *(double*)lParam;
			if (m_bIsTrackingSelection)
			{
				ReleaseCapture();
				//m_bIsTrackingSelection = FALSE;   // will be reset in WM_CAPTURECHANGED
			}

			Invalidate(TRUE);
			// check for the proper offset, correct if necessary
			m_FirstbandVisible = AdjustOffset(m_FirstbandVisible);
		}
		break;

	case FftBandsChanged:
	{
		int NewBands = *(int*)lParam;
		if (NewBands != m_FftOrder)
		{
			m_FirstbandVisible = m_FirstbandVisible * NewBands / m_FftOrder;
			m_FftOrder = NewBands;
			Invalidate(FALSE);
		}
	}
		break;
	case FftOffsetChanged:
		SetNewFftOffset(*(double*) lParam);
		break;
	case ChannelHeightsChanged:
		m_Heights = *(NotifyChannelHeightsData*)lParam;
		if (m_bIsTrackingSelection)
		{
			ReleaseCapture();
		}
		Invalidate();
		break;
	}
	return 0;
}

void CFftRulerView::BeginMouseTracking()
{
	m_MouseYOffsetForScroll = 0;
	m_FftOffsetBeforeScroll = m_FirstbandVisible;
}

void CFftRulerView::VerticalScrollByPixels(int Pixels)
{
	m_MouseYOffsetForScroll += Pixels;

	double offset = m_FftOffsetBeforeScroll + (double)m_MouseYOffsetForScroll * m_FftOrder / int(m_Heights.NominalChannelHeight * m_VerticalScale);
	NotifySiblingViews(FftScrollTo, &offset);
}

void CFftRulerView::OnCaptureChanged(CWnd *pWnd)
{
	m_MouseYOffsetForScroll = 0;
	CVerticalRuler::OnCaptureChanged(pWnd);
}

void CFftRulerView::OnContextMenu(CWnd* /*pWnd*/, CPoint point)
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

