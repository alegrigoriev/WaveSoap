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
}

CFftRulerView::~CFftRulerView()
{
}


BEGIN_MESSAGE_MAP(CFftRulerView, CVerticalRuler)
	//{{AFX_MSG_MAP(CFftRulerView)
	ON_WM_LBUTTONDBLCLK()
	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONUP()
	ON_WM_CONTEXTMENU()
	ON_WM_CAPTURECHANGED()
	//}}AFX_MSG_MAP
	ON_MESSAGE(UWM_NOTIFY_VIEWS, &CFftRulerView::OnUwmNotifyViews)
	ON_WM_ERASEBKGND()
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

	// Draw with double buffering
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

	if (0) TRACE("SysColor(COLOR_WINDOW)=%X\n", GetSysColor(COLOR_WINDOW));

	TEXTMETRIC tm;
	pDC->GetTextMetrics( & tm);
	pDC->SetTextAlign(TA_BOTTOM | TA_RIGHT);
	pDC->SetTextColor(0x000000);   // black
	pDC->SetBkMode(TRANSPARENT);
	pDC->MoveTo(cr.right - 1, cr.top);
	pDC->LineTo(cr.right - 1, cr.bottom);

	for (int ch = 0; ch < nChannels; ch++)
	{
		CRect clipr;    // channel clip rect
		// for all channels, the rectangle is of the same height
		clipr.top = m_Heights.ch[ch].clip_top;
		clipr.bottom = m_Heights.ch[ch].clip_bottom;
		clipr.left = cr.left;
		clipr.right = cr.right;

		pDC->FillSolidRect(clipr, GetSysColor(COLOR_WINDOW));

		if (!m_Heights.ChannelMinimized(ch))
		{
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

			int nSampleUnits = int(nVertStep * pDoc->WaveSampleRate() / 2 / (m_Heights.NominalChannelHeight * m_VerticalScale));
			// round sample units to 10 or 5
			int step;
			int ticks = 1;
			for (step = 1; ticks = 2, step*ticks < nSampleUnits; step *= 5)
			{
				if (step * 2 >= nSampleUnits)
				{
					ticks = 2;
					break;
				}
				if (step * 5 >= nSampleUnits)
				{
					ticks = 5;
					break;
				}
				step *= 2;
				if (step * 5 >= nSampleUnits)
				{
					ticks = 5;
					break;
				}
			}

			// round to previous multiple of step
			long F_low = int(m_FirstbandVisible * 0.5 * pDoc->WaveSampleRate() / m_FftOrder);
			// round to the next or current multiple of step
			F_low -= F_low % (step*ticks);

			for (long F = F_low, j = 0; ; F += step, j++)
			{
				// F is frequency
				double band = double(F) / (0.5 * pDoc->WaveSampleRate())
							* m_FftOrder;
				int yDev = m_Heights.ch[ch].clip_bottom - (int)fround((band - m_FirstbandVisible) / m_FftOrder * m_Heights.NominalChannelHeight * m_VerticalScale);

				if (yDev < clipr.top)
				{
					break;
				}

				if (yDev < clipr.bottom)
				{
					pDC->MoveTo(cr.right - 3, yDev);
					pDC->LineTo(cr.right, yDev);
				}

				if (0 == j % ticks
					&& yDev - tm.tmHeight / 2 > clipr.top
					&& yDev + tm.tmHeight / 2 <= clipr.bottom)
				{
					CString s = LtoaCS(F);
					pDC->TextOut(cr.right - 3, yDev + tm.tmHeight / 2, s);
				}
			}

		}

		if (ch != nChannels - 1)
		{
			pDC->MoveTo(0, clipr.bottom);
			pDC->LineTo(cr.right, clipr.bottom);
			clipr.bottom++;
		}
		pDrawDC->BitBlt(clipr.left, clipr.top, clipr.Width(), clipr.Height(), pDC, clipr.left, clipr.top, SRCCOPY);
	}
}

int CFftRulerView::CalculateWidth()
{
	CWindowDC wDC(AfxGetMainWnd());

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
	double one1 = 1.;
	// set default scale
	NotifySiblingViews(FftVerticalScaleChanged, &one1);
}

void CFftRulerView::OnLButtonUp(UINT /*nFlags*/, CPoint /*point*/)
{
	ButtonPressed = 0;
	if (m_bIsTrackingSelection)
	{
		ReleaseCapture();
		m_bIsTrackingSelection = FALSE;
	}
}

void CFftRulerView::OnLButtonDown(UINT nFlags, CPoint point)
{
	if (nFlags & MK_CONTROL)
	{
		// toggling this channel selection, unless it's the only channel selected
		CWaveSoapFrontDoc * pDoc = GetDocument();
		// get hit test for channel
		for (int ch = 0; ch < m_Heights.NumChannels; ch++)
		{
			if (point.y >= m_Heights.ch[ch].top
				&& point.y < m_Heights.ch[ch].bottom)
			{
				CHANNEL_MASK mask = 1 << ch;
				if ((pDoc->m_SelectedChannel & CWaveFormat::ChannelsMaskFromNumberOfChannels(m_Heights.NumChannels)) != mask)
				{
					pDoc->SetSelection(pDoc->m_SelectionStart, pDoc->m_SelectionEnd, pDoc->m_SelectedChannel ^ mask,
										pDoc->m_CaretPosition, SetSelection_DontAdjustView);
				}
				break;
			}
		}
	}
	else
	{
		// store the starting mouse position
		PrevMouseY = point.y;
		ButtonPressed = WM_LBUTTONDOWN;
	}
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
	CWindowDC dc(this);

	long OldOffsetPixels = long(m_FirstbandVisible * int(m_VerticalScale * m_Heights.NominalChannelHeight) / m_FftOrder);
	long NewOffsetPixels = long(first_band * int(m_VerticalScale * m_Heights.NominalChannelHeight) / m_FftOrder);

	int ToScroll = NewOffsetPixels - OldOffsetPixels;       // >0 - down, <0 - up

	for (int ch = 0; ch < nChannels; ch++)
	{
		if (m_Heights.ChannelMinimized(ch))
		{
			continue;
		}

		// offset of the zero line down

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
	m_FirstbandVisible = first_band;
	// make it redraw the whole window without erasing the background
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

	NotifyViewsData notify = { 0 };
	notify.PopupMenu.p = point;
	notify.PopupMenu.NormalMenuId = IDR_MENU_FFT_RULER;
	notify.PopupMenu.MinimizedMenuId = IDR_MENU_FFT_RULER_MINIMIZED;

	NotifySiblingViews(ShowChannelPopupMenu, &notify);
}



BOOL CFftRulerView::OnEraseBkgnd(CDC* pDC)
{
	// No actual erase required

	return TRUE;
}
