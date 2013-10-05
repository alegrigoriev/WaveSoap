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
{
	memzero(m_Heights);
}

CFftRulerView::~CFftRulerView()
{
}


BEGIN_MESSAGE_MAP(CFftRulerView, CVerticalRuler)
	//{{AFX_MSG_MAP(CFftRulerView)
	ON_WM_LBUTTONDBLCLK()
	ON_WM_CONTEXTMENU()
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

void CFftRulerView::OnDraw(CDC* pDC)
{
	CWaveSoapFrontDoc* pDoc = GetDocument();
	if (! pDoc->m_WavFile.IsOpen())
	{
		return;
	}

	CRect cr;
	GetClientRect(cr);

	int nVertStep = GetSystemMetrics(SM_CYMENU);

	NUMBER_OF_CHANNELS nChannels = pDoc->WaveChannels();
	if (0 == nChannels)
	{
		return;
	}

	CGdiObjectSave OldFont(pDC, pDC->SelectStockObject(ANSI_VAR_FONT));
	CGdiObjectSave OldPen(pDC, pDC->SelectStockObject(BLACK_PEN));

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


afx_msg LRESULT CFftRulerView::OnUwmNotifyViews(WPARAM wParam, LPARAM lParam)
{
	switch (wParam)
	{
	case FftOffsetChanged:
		break;
	case FftScrollPixels:
		break;
	case ChannelHeightsChanged:
		m_Heights = *(NotifyChannelHeightsData*)lParam;
		Invalidate();
		break;
	}
	return 0;
}

void CFftRulerView::VerticalScrollPixels(int Pixels)
{
	//double scroll = Pixels * m_VerticalScale;
	NotifySiblingViews(FftScrollPixels, &Pixels);
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

