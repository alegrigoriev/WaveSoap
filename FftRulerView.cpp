// Copyright Alexander Grigoriev, 1997-2002, All Rights Reserved
// FftRulerView.cpp : implementation file
//

#include "stdafx.h"
#include "WaveSoapFront.h"
#include "FftRulerView.h"
#include "GdiObjectSave.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CFftRulerView

IMPLEMENT_DYNCREATE(CFftRulerView, CVerticalRuler)

CFftRulerView::CFftRulerView()
{
}

CFftRulerView::~CFftRulerView()
{
}


BEGIN_MESSAGE_MAP(CFftRulerView, CVerticalRuler)
	//{{AFX_MSG_MAP(CFftRulerView)
	ON_WM_LBUTTONDBLCLK()
	//}}AFX_MSG_MAP
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

	CWaveFftView * pMasterView = DYNAMIC_DOWNCAST(CWaveFftView, m_pVertMaster);
	if (NULL == pMasterView)
	{
		return; // not attached
	}

	CRect cr;
	GetClientRect(cr);


	int nVertStep = GetSystemMetrics(SM_CYMENU);
	NUMBER_OF_CHANNELS nChannels = pDoc->WaveChannels();
	if (0 == nChannels)
	{
		return;
	}
	int nHeight = cr.Height() / nChannels;

	if (0 == nHeight)
	{
		return;
	}

	double VerticalScale = pMasterView->m_VerticalScale;

	// if all the chart was drawn, how many scans it would have:
	int TotalRows = int(nHeight * VerticalScale);

	if (0 == TotalRows)
	{
		return;
	}

	int LastFftSample = int(pMasterView->m_FftOrder - pMasterView->m_FirstbandVisible);
	int FirstFftSample = LastFftSample + (-nHeight * pMasterView->m_FftOrder) / TotalRows;
	if (FirstFftSample < 0)
	{
		LastFftSample -= FirstFftSample;
		FirstFftSample = 0;
	}
	int FirstRowInView = FirstFftSample * TotalRows / pMasterView->m_FftOrder;
	int FftSamplesInView = LastFftSample - FirstFftSample + 1;

	CGdiObjectSave OldFont(pDC, pDC->SelectStockObject(ANSI_VAR_FONT));
	CGdiObjectSave OldPen(pDC, pDC->SelectStockObject(BLACK_PEN));

	TEXTMETRIC tm;
	pDC->GetTextMetrics( & tm);
	pDC->SetTextAlign(TA_BOTTOM | TA_RIGHT);
	pDC->SetTextColor(0x000000);   // black
	pDC->SetBkMode(TRANSPARENT);

	int nSampleUnits = int(nVertStep * pDoc->WaveSampleRate() / (nHeight * VerticalScale));
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
	const int ChannelSeparatorY = nHeight;
	if (2 == nChannels)
	{
		pDC->MoveTo(0, ChannelSeparatorY);
		pDC->LineTo(cr.right, ChannelSeparatorY);
	}
	for (int ch = 0; ch < nChannels; ch++)
	{
		double Offset = nHeight;
		int ClipHigh = cr.bottom;
		int ClipLow = cr.top;
		if (nChannels > 1)
		{
			if (0 == ch)
			{
				ClipHigh = ChannelSeparatorY;
			}
			else
			{
				Offset = cr.Height();
				ClipLow = ChannelSeparatorY + 1;
			}
		}
		ClipLow += tm.tmHeight / 2;
		ClipHigh -= tm.tmHeight / 2;
		int yLow = int(pMasterView->m_FirstbandVisible *
						0.5 * pDoc->WaveSampleRate() / pMasterView->m_FftOrder);
		// round to the next multiple of step
		yLow += (step*0x10000-yLow) % step;
		int yHigh = int(yLow + 0.5 * pDoc->WaveSampleRate() / VerticalScale);
		yHigh -= (step*0x10000+yHigh) % step;
		ASSERT(yLow <= yHigh);
		for (int y = yLow; y <= yHigh; y += step)
		{
			// y is frequency
			double band = double(y) / (0.5 * pDoc->WaveSampleRate())
						* pMasterView->m_FftOrder;
			int yDev= int(Offset - fround((band - pMasterView->m_FirstbandVisible)
										/ pMasterView->m_FftOrder * nHeight * VerticalScale));
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
	}
}

int CFftRulerView::CalculateWidth()
{
	CWindowDC wDC(GetDesktopWindow());

	CGdiObjectSave Old(wDC, wDC.SelectStockObject(ANSI_VAR_FONT));
	int Width = 4 + wDC.GetTextExtent(_T("-000,000"), 8).cx;

	return Width;
}

void CFftRulerView::OnUpdate( CView* pSender, LPARAM lHint, CObject* pHint )
{
	if (lHint == CWaveSoapFrontDoc::UpdateWholeFileChanged)
	{
		Invalidate();
	}
	else if (lHint == CWaveSoapFrontDoc::UpdateWholeFileChanged
			|| lHint == CWaveSoapFrontDoc::UpdateSampleRateChanged
			|| ((lHint == CWaveFftView::FFT_OFFSET_CHANGED
					|| lHint == CWaveFftView::FFT_SCALE_CHANGED) && NULL == pHint))
	{
		Invalidate();
	}
}

void CFftRulerView::OnLButtonDblClk(UINT nFlags, CPoint point)
{
	CVerticalRuler::OnLButtonDblClk(nFlags, point);
	// set default scale
	CWaveFftView * pView =
		DYNAMIC_DOWNCAST(CWaveFftView, m_pVertMaster);
	if (NULL != pView)
	{
		pView->OnViewZoomvertNormal();
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
