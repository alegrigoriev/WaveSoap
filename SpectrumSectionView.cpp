// Copyright Alexander Grigoriev, 1997-2002, All Rights Reserved
// SpectrumSectionView.cpp : implementation file
//

#include "stdafx.h"
#include "WaveSoapFront.h"
#include "SpectrumSectionView.h"
#include "fft.h"
#include "GdiObjectSave.h"
#include "WaveSoapFrontDoc.h"
#include "WaveFftView.h"
#include "resource.h"       // main symbols

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CSpectrumSectionView

IMPLEMENT_DYNCREATE(CSpectrumSectionView, CScaledScrollView)

CSpectrumSectionView::CSpectrumSectionView()
	: m_dNoiseThresholdLow(-60.)
	, m_dNoiseThresholdHigh(-70.)
	, m_nBeginFrequency(1000)
	, m_bCrossHairDrawn(false)
	, m_FftOrder(512)
	, m_pFftSum(NULL)
	, m_pWindow(NULL)
	, m_bShowNoiseThreshold(FALSE)
	, m_bShowCrossHair(false)
	, m_bTrackingMouseRect(false)
	, m_FftPosition(-1)
	, m_nFftSumSize(0)
	, m_PlaybackSample(0)
{
}

CSpectrumSectionView::~CSpectrumSectionView()
{
	delete[] m_pFftSum;
	delete[] m_pWindow;
}


BEGIN_MESSAGE_MAP(CSpectrumSectionView, CScaledScrollView)
	//{{AFX_MSG_MAP(CSpectrumSectionView)
	ON_WM_CREATE()
	ON_WM_MOUSEACTIVATE()
	ON_UPDATE_COMMAND_UI(ID_VIEW_SS_SHOW_NOISE_THRESHOLD, OnUpdateViewSsShowNoiseThreshold)
	ON_COMMAND(ID_VIEW_SS_SHOW_NOISE_THRESHOLD, OnViewSsShowNoiseThreshold)
	ON_COMMAND(ID_VIEW_SS_ZOOMINHOR2, OnViewSsZoominhor2)
	ON_UPDATE_COMMAND_UI(ID_VIEW_SS_ZOOMINHOR2, OnUpdateViewSsZoominhor2)
	ON_COMMAND(ID_VIEW_SS_ZOOMOUTHOR2, OnViewSsZoomouthor2)
	ON_UPDATE_COMMAND_UI(ID_VIEW_SS_ZOOMOUTHOR2, OnUpdateViewSsZoomouthor2)
	ON_WM_MOUSEMOVE()
	ON_UPDATE_COMMAND_UI(ID_VIEW_SHOW_CROSSHAIR, OnUpdateViewShowCrosshair)
	ON_COMMAND(ID_VIEW_SHOW_CROSSHAIR, OnViewShowCrosshair)
	//}}AFX_MSG_MAP
	ON_MESSAGE(WM_MOUSELEAVE, OnMouseLeave)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CSpectrumSectionView drawing

struct CSpectrumSectionView::FftGraphBand
{
	int nFftOffset;
	int nNumOfRows;
	int nMin;
	int nMax;
};

int CSpectrumSectionView::InitBandArray(ATL::CHeapPtr<FftGraphBand> & pBands, int rows)
{
	CWaveFftView * pFftView = dynamic_cast<CWaveFftView *>(m_pVertMaster);
	int TotalRows = int(rows * pFftView->m_VerticalScale);

	int LastFftSample = int(m_FftOrder - pFftView->m_FirstbandVisible);
	int FirstFftSample = LastFftSample + MulDiv(-rows, m_FftOrder, TotalRows);

	if (FirstFftSample < 0)
	{
		LastFftSample -= FirstFftSample;
		FirstFftSample = 0;
	}
	int FirstRowInView = MulDiv(FirstFftSample, TotalRows, m_FftOrder);

	int FftSamplesInView = LastFftSample - FirstFftSample + 1;

	int IdxSize1 = __min(rows, FftSamplesInView);

	pBands.Allocate(IdxSize1+1);

	int k;
	int LastRow = 0;

	for (k = 0; k < IdxSize1; k++)
	{
		if (FirstFftSample >= m_FftOrder
			|| LastRow >= rows)
		{
			break;
		}
		pBands[k].nFftOffset = FirstFftSample;
		FirstFftSample++;
		int NextRow = FirstFftSample * TotalRows / m_FftOrder - FirstRowInView;
		if (NextRow == LastRow)
		{
			NextRow++;
			FirstFftSample = (NextRow + FirstRowInView) * m_FftOrder / TotalRows;
		}
		if (NextRow > rows)
		{
			NextRow = rows;
		}
		pBands[k].nNumOfRows = NextRow - LastRow;
		LastRow = NextRow;
	}
	ASSERT(LastRow <= rows);

	pBands[k].nFftOffset = m_FftOrder;
	pBands[k].nNumOfRows = 0;

	return k;
}

void CSpectrumSectionView::CalculateFftPowerSum(float * pFftSum,
												SAMPLE_INDEX FirstSample,
												int NumberOfSamplesAveraged, int FftOrder)
{
	CWaveSoapFrontDoc* pDoc = GetDocument();
	NUMBER_OF_CHANNELS nChannels = pDoc->WaveChannels();

	ATL::CHeapPtr<complex<float> > pSrcArray;
	pSrcArray.Allocate(nChannels * (FftOrder + 1));

	ATL::CHeapPtr<WAVE_SAMPLE> pSrcIntArray;
	pSrcIntArray.Allocate(FftOrder * 2 * nChannels);

	int i;

	for (i = 0; i < FftOrder * nChannels; i++)
	{
		pFftSum[i] = 0.;
	}

	for (int FftSample = 0; FftSample < NumberOfSamplesAveraged; FftSample++)
	{
		pDoc->m_WavFile.ReadSamples(ALL_CHANNELS,
									pDoc->m_WavFile.SampleToPosition(FirstSample), FftOrder * 2, pSrcIntArray);

		FirstSample += FftOrder / 2;

		if (1 == nChannels)
		{
			for (i = 0; i < FftOrder; i++)
			{
				pSrcArray[i].real(pSrcIntArray[i * 2] * m_pWindow[i * 2]);
				pSrcArray[i].imag(pSrcIntArray[i * 2 + 1] * m_pWindow[i * 2 + 1]);
			}

			FastFourierTransform((float*)pSrcArray.operator->(), pSrcArray.operator->(), FftOrder * 2);
			pSrcArray[0].imag(0.);

			for (i = 0; i < FftOrder; i++)
			{
				pFftSum[i] += pSrcArray[i].real() * pSrcArray[i].real()
							+ pSrcArray[i].imag() * pSrcArray[i].imag();
			}
		}
		else if (2 == nChannels)
		{
			for (i = 0; i < FftOrder * 2; i++)
			{
				pSrcArray[i].real(pSrcIntArray[i * 2] * m_pWindow[i]);
				pSrcArray[i].imag(pSrcIntArray[i * 2 + 1] * m_pWindow[i]);
			}

			FastFourierTransform(pSrcArray.operator->(), pSrcArray.operator->(), FftOrder * 2);
			// channel 0 was in real part, ch 1 was imag part
			// left result is (re1+re2)/2, (im1-im2)/2
			// right result is (re1-re2)/2, (im1+im2)/2
			for (i = 1; i < FftOrder; i++)
			{
				double re1 = pSrcArray[i].real() + pSrcArray[(m_FftOrder * 2 - i)].real();
				double re2 = pSrcArray[i].real() - pSrcArray[(m_FftOrder * 2 - i)].real();
				double im1 = pSrcArray[i].imag() - pSrcArray[(m_FftOrder * 2 - i)].imag();
				double im2 = pSrcArray[i].imag() + pSrcArray[(m_FftOrder * 2 - i)].imag();

				pFftSum[i * 2] = float(pFftSum[i * 2] + 0.25 * (re1 * re1 + im1 * im1));
				pFftSum[i * 2 + 1] = float(pFftSum[i * 2 + 1] + 0.25 * (re2 * re2 + im2 * im2));
			}
		}
		else
		{
			ASSERT(FALSE);
		}
	}
}

void CSpectrumSectionView::BuildBandArray(double PowerScaleCoeff, FftGraphBand * pBands,
										int NumBands, float const * pFftSum,
										int FftOrder)
{
	CWaveSoapFrontDoc* pDoc = GetDocument();
	NUMBER_OF_CHANNELS nChannels = pDoc->WaveChannels();

	pFftSum += nChannels * (FftOrder- pBands[0].nFftOffset);

	for (int n = 0; n < NumBands; n++, pBands++)
	{
		pBands->nMin = 0x7FFFFF;
		pBands->nMax = -0x7FFFFF;

		for (int j = pBands->nFftOffset; j < pBands[1].nFftOffset; j++)
		{
			pFftSum -= nChannels;

			double temp = PowerScaleCoeff * *pFftSum;

			if (0 != temp)
			{
				temp = 10. * log10(temp);
			}
			else
			{
				temp = -200.;
			}
			int pos = WorldToWindowXrnd(temp);
			if (pBands->nMin > pos)
			{
				pBands->nMin = pos;
			}
			if (pBands->nMax < pos)
			{
				pBands->nMax = pos;
			}
		}
	}
}

void CSpectrumSectionView::OnDraw(CDC* pDC)
{
	CWaveSoapFrontDoc* pDoc = GetDocument();
	if ( ! pDoc->m_WavFile.IsOpen())
	{
		if (m_bCrossHairDrawn)
		{
			DrawCrossHair(m_PrevCrossHair, pDC);
		}
		return;
	}
	// calculate FFT
	// if there is no selection, calculate only current position
	// get FFT order from master window
	CWaveFftView * pFftView = dynamic_cast<CWaveFftView *>(m_pVertMaster);
	if (NULL == pFftView)
	{
		return;
	}
	CRect r;
	CRect cr;
	GetClientRect( & cr);
	double left, right, top, bottom;

	if (pDC->IsKindOf(RUNTIME_CLASS(CPaintDC)))
	{
		r = ((CPaintDC*)pDC)->m_ps.rcPaint;
	}
	else
	{
		pDC->GetClipBox(&r);
	}
	//int iClientWidth = r.right - r.left;
	PointToDoubleDev(CPoint(r.left, cr.top), left, top);
	PointToDoubleDev(CPoint(r.right, cr.bottom), right, bottom);

	if (left < 0) left = 0;

	m_FftOrder = pFftView->m_FftOrder;

	NUMBER_OF_CHANNELS nChannels = pDoc->WaveChannels();
	if (0 == nChannels)
	{
		return;
	}

	SAMPLE_INDEX nStartSample = pDoc->m_SelectionStart;

	int NumberOfFftSamplesAveraged =
		(pDoc->m_SelectionEnd - pDoc->m_SelectionStart) / m_FftOrder + 1;
	if (NumberOfFftSamplesAveraged > 100)
	{
		NumberOfFftSamplesAveraged = 100;
	}
	if (pDoc->m_PlayingSound)
	{
		NumberOfFftSamplesAveraged = 4;
		nStartSample = m_PlaybackSample;
	}
	if (nStartSample > m_FftOrder)
	{
		nStartSample -= m_FftOrder;
	}
	else
	{
		nStartSample = 0;
	}

	// calculate extents
	// find vertical offset in the result array and how many
	// rows to fill with this color
	int rows = cr.Height() / nChannels;
	// create an array of points
	int nNumberOfPoints = rows;

	// if all the chart was drawn, how many scans it would have:
	int TotalRows = int(rows * pFftView->m_VerticalScale);

	if (0 == TotalRows)
	{
		if (m_bCrossHairDrawn)
		{
			DrawCrossHair(m_PrevCrossHair, pDC);
		}
		return;
	}

	if ( ! AllocateFftArrays())
	{
		return;
	}

// build an array

	ATL::CHeapPtr<FftGraphBand> pIdArray;

	POINT (* ppArray)[2] = new POINT[nNumberOfPoints][2];
	if (NULL == ppArray)
	{
		return;
	}

	// fill the array
	int IdxSize = InitBandArray(pIdArray, rows);

	CPen pen(PS_SOLID, 0, COLORREF(0));   // black pen
	CGdiObjectSaveT<CPen> OldPen(pDC, pDC->SelectObject( & pen));

	int i;
	int j;
	int ch;
	// read the data
	// if there is selection, calculate the whole region sum

	CalculateFftPowerSum(m_pFftSum, nStartSample, NumberOfFftSamplesAveraged, m_FftOrder);

	double PowerScaleCoeff = 1. / (NumberOfFftSamplesAveraged * 65536. * m_FftOrder * 65536 * m_FftOrder);
	// now that we have calculated the FFT
	for (ch = 0; ch < nChannels; ch++)
	{
		int ChannelOffset = rows * ch;

		BuildBandArray(PowerScaleCoeff, pIdArray, IdxSize, m_pFftSum, m_FftOrder + ch);

		for (i = 0, j = 0; i < IdxSize && j < nNumberOfPoints; i++)
		{
			for (int k = 0; k < pIdArray[i].nNumOfRows  && j < nNumberOfPoints; k++, j++)
			{
				ppArray[j][0].x = pIdArray[i].nMax;
				ppArray[j][1].x = pIdArray[i].nMin - 1;
				ppArray[j][0].y = ChannelOffset + j;
				ppArray[j][1].y = ChannelOffset + j;
			}
		}

		int LastX0 = ppArray[0][0].x;
		int LastX1 = ppArray[0][1].x;

		for (i = 0; i < nNumberOfPoints; i++)
		{
			//TRACE("ppArray[%d].x = %d, %d\n", i, ppArray[i][1].x, ppArray[i][0].x);
			if (i < nNumberOfPoints - 1
				&& ppArray[i + 1][0].x >= ppArray[i + 1][1].x)
			{
				if (ppArray[i][0].x < ppArray[i + 1][1].x)
				{
					ppArray[i][0].x = (ppArray[i + 1][1].x + ppArray[i][0].x) >> 1;
				}
				else if (ppArray[i + 1][0].x < ppArray[i][1].x)
				{
					ppArray[i][1].x = (ppArray[i][1].x + ppArray[i + 1][0].x) >> 1;
				}
			}
			if (LastX0 >= LastX1)
			{
				if (ppArray[i][0].x < LastX1)
				{
					ppArray[i][0].x = LastX1;
				}
				else if (ppArray[i][1].x > LastX0)
				{
					ppArray[i][1].x = LastX0;
				}
			}

			LastX0 = ppArray[i][0].x;
			LastX1 = ppArray[i][1].x;

			if (ppArray[i][0].x < 0)
			{
				continue;
			}
			else if (ppArray[i][0].x > cr.right)
			{
				ppArray[i][0].x = cr.right;
			}

			if (ppArray[i][1].x < 0)
			{
				ppArray[i][1].x = -1;
			}
			else if (ppArray[i][1].x > cr.right)
			{
				continue;
			}
			pDC->MoveTo(ppArray[i][0]);
			pDC->LineTo(ppArray[i][1]);
		}

		if (nChannels > 1)
		{
			pDC->MoveTo(0, rows);
			pDC->LineTo(cr.right, rows);
		}

	}

	if (m_bShowNoiseThreshold)
	{
		int NoiseReductionBegin = MulDiv(m_nBeginFrequency, m_FftOrder,
										pDoc->WaveSampleRate());

		CPen DotPen(PS_DOT, 0, RGB(255, 255, 255));   // XOR pen
		pDC->SelectObject(& DotPen);
		pDC->SetROP2(R2_XORPEN);
		pDC->SetBkColor(RGB(0, 0, 0));

		for (ch = 0; ch < nChannels; ch++)
		{
			CRect ChanR;

			GetChannelRect(ch, ChanR);
			// draw with a dotted pen
			int PosHighX = WorldToWindowXrnd(m_dNoiseThresholdHigh);
			int PosLowX = WorldToWindowXrnd(m_dNoiseThresholdLow);

			pDC->MoveTo(PosHighX, ChanR.top);
			pDC->LineTo(PosLowX, ChanR.bottom);
		}
	}

	delete[] ppArray;
	if (m_bCrossHairDrawn)
	{
		DrawCrossHair(m_PrevCrossHair, pDC);
	}
}

BOOL CSpectrumSectionView::AllocateFftArrays()
{
	// Allocate FFT arrays
	CWaveSoapFrontDoc* pDoc = GetDocument();
	NUMBER_OF_CHANNELS nChannels = pDoc->WaveChannels();
	CWaveFftView * pFftView = dynamic_cast<CWaveFftView *>(m_pVertMaster);

	if (NULL == m_pFftSum
		|| m_nFftSumSize != nChannels * (m_FftOrder * 2 + 2))
	{
		delete[] m_pFftSum;
		delete[] m_pWindow;
		m_pFftSum = NULL;
		m_pWindow = NULL;
		m_nFftSumSize = 0;
		m_nFftSumSize = nChannels * (m_FftOrder * 2 + 2);
		m_pFftSum = new float[m_nFftSumSize];

		if (NULL == m_pFftSum)
		{
			return FALSE;
		}

	}

	if (NULL == m_pWindow)
	{
		m_pWindow = new float[m_FftOrder * 2];

		if (NULL == m_pWindow)
		{
			return FALSE;
		}
		int FftWindowType = pFftView->m_FftWindowType;

		for (int w = 0; w < m_FftOrder * 2; w++)
		{
			switch (FftWindowType)
			{
			default:
			case pFftView->WindowTypeSquaredSine:
				// squared sine
				m_pWindow[w] = float(0.5 - 0.5 * cos ((w + 0.5) * M_PI /  m_FftOrder));
				break;
			case pFftView->WindowTypeHalfSine:
				// half sine
				m_pWindow[w] = float(0.707107 * sin ((w + 0.5) * M_PI /  (2*m_FftOrder)));
				break;
			case pFftView->WindowTypeHamming:
				// Hamming window (sucks!!!)
				m_pWindow[w] = float(0.9 *(0.54 - 0.46 * cos ((w + 0.5)* M_PI /  m_FftOrder)));
				break;
			}
		}
	}
	return TRUE;
}
/////////////////////////////////////////////////////////////////////////////
// CSpectrumSectionView diagnostics

#ifdef _DEBUG
void CSpectrumSectionView::AssertValid() const
{
	CScaledScrollView::AssertValid();
}

void CSpectrumSectionView::Dump(CDumpContext& dc) const
{
	CScaledScrollView::Dump(dc);
}
CWaveSoapFrontDoc* CSpectrumSectionView::GetDocument() const // non-debug version is inline
{
	ASSERT(m_pDocument->IsKindOf(RUNTIME_CLASS(CWaveSoapFrontDoc)));
	return (CWaveSoapFrontDoc*)m_pDocument;
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CSpectrumSectionView message handlers
/////////////////////////////////////////////////////////////////////////////

int CSpectrumSectionView::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CScaledScrollView::OnCreate(lpCreateStruct) == -1)
		return -1;

	KeepAspectRatio(FALSE);
	KeepScaleOnResizeX(FALSE);
	KeepScaleOnResizeY(FALSE);
	KeepOrgOnResizeX(TRUE);
	KeepOrgOnResizeY(FALSE);

	SetMaxExtents(0, -120.,
				0, 1000.);
	SetExtents(0., -70., 0, 1000.);
	ShowScrollBar(SB_VERT, FALSE);
	ShowScrollBar(SB_HORZ, TRUE);
	return 0;
}

int CSpectrumSectionView::OnMouseActivate(CWnd* pDesktopWnd, UINT nHitTest, UINT message)
{
	// don't call CView function, to avoid getting focus to the window

	return CWnd::OnMouseActivate(pDesktopWnd, nHitTest, message);
}

void CSpectrumSectionView::OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint)
{
	CWaveSoapFrontDoc * pDoc = GetDocument();
	// any changes will cause m_bShowNoiseThreshold flag to reset

	if (lHint == CWaveSoapFrontDoc::UpdateSoundChanged
		&& NULL != pHint)
	{
		CSoundUpdateInfo * pInfo = static_cast<CSoundUpdateInfo *>(pHint);
		m_bShowNoiseThreshold = false;

		if ((pDoc->m_SelectionStart <= pInfo->m_End
				&& pDoc->m_SelectionEnd + 2 * m_FftOrder >= pInfo->m_Begin)
			|| pInfo->m_NewLength != -1)
		{
			Invalidate();
		}
	}
	else if (lHint == CWaveSoapFrontDoc::UpdateSelectionChanged)
	{
		m_bShowNoiseThreshold = false;
		Invalidate();
		return;
	}
	else if (lHint == CWaveSoapFrontDoc::UpdatePlaybackPositionChanged
			&& NULL != pHint)
	{
		m_bShowNoiseThreshold = false;
		CSoundUpdateInfo * pInfo = static_cast<CSoundUpdateInfo *>(pHint);
		m_PlaybackSample = pInfo->m_PlaybackPosition;

		Invalidate();
		return;
	}
	else if (lHint == CWaveFftView::FFT_BANDS_CHANGED)
	{
		m_bShowNoiseThreshold = false;
		delete[] m_pWindow;
		m_pWindow = NULL;
	}
	else if (0 == lHint)
	{
		BaseClass::OnUpdate(pSender, lHint, pHint);
	}
}

void CSpectrumSectionView::OnUpdateViewSsShowNoiseThreshold(CCmdUI* pCmdUI)
{
	pCmdUI->SetCheck(m_bShowNoiseThreshold);
}

void CSpectrumSectionView::OnViewSsShowNoiseThreshold()
{
	m_bShowNoiseThreshold = TRUE;
	Invalidate();
}

void CSpectrumSectionView::OnViewSsZoominhor2()
{
	CScaledScrollView::OnViewZoominHor2();
}

void CSpectrumSectionView::OnUpdateViewSsZoominhor2(CCmdUI* pCmdUI)
{
	pCmdUI->Enable(dSizeX > 2.);
}

void CSpectrumSectionView::OnViewSsZoomouthor2()
{
	CScaledScrollView::OnViewZoomOutHor2();
}

void CSpectrumSectionView::OnUpdateViewSsZoomouthor2(CCmdUI* pCmdUI)
{
	pCmdUI->Enable(dSizeX < 150.);
}

void CSpectrumSectionView::OnMouseMove(UINT nFlags, CPoint point)
{
	CView::OnMouseMove(nFlags, point);

	if ( ! m_bTrackingMouseRect)
	{
		TRACKMOUSEEVENT tme;
		tme.cbSize = sizeof tme;
		tme.dwFlags = TME_LEAVE;
		tme.dwHoverTime = HOVER_DEFAULT;
		tme.hwndTrack = m_hWnd;
		TrackMouseEvent( & tme);
		m_bTrackingMouseRect = true;
	}
	if (point != m_PrevCrossHair)
	{
		HideCrossHair();
		ShowCrossHair(point);
	}
}

void CSpectrumSectionView::ShowCrossHair(POINT point, CDC * pDC)
{
	if (m_bShowCrossHair
		&&  ! m_bCrossHairDrawn)
	{
		DrawCrossHair(point, pDC);
		m_PrevCrossHair = point;
		m_bCrossHairDrawn = true;
	}
}

void CSpectrumSectionView::HideCrossHair(CDC * pDC)
{
	if (m_bCrossHairDrawn)
	{
		DrawCrossHair(m_PrevCrossHair, pDC);
		m_PrevCrossHair.x = 0x8000;
		m_PrevCrossHair.y = 0x8000;

		m_bCrossHairDrawn = false;
	}
}

void CSpectrumSectionView::DrawCrossHair(POINT point, CDC * pDC)
{
	CDC * pDrawDC = pDC;
	if (NULL == pDrawDC)
	{
		pDrawDC = GetDC();
		if (NULL == pDrawDC)
		{
			return;
		}
		pDrawDC->ExcludeUpdateRgn(this);
	}

	try {
		CPushDcMapMode mode(pDrawDC, MM_TEXT);
		RECT cr;

		GetClientRect( & cr);
		// have to use PatBlt to draw alternate lines
		CBitmap bmp;
		static const unsigned char pattern[] =
		{
			0x55, 0,  // aligned to WORD
			0xAA, 0,
			0x55, 0,
			0xAA, 0,
			0x55, 0,
			0xAA, 0,
			0x55, 0,
			0xAA, 0,
		};

		bmp.CreateBitmap(8, 8, 1, 1, pattern);
		CBrush GrayBrush( & bmp);
		CGdiObjectSaveT<CBrush> OldBrush(pDrawDC, pDrawDC->SelectObject( & GrayBrush));

		pDrawDC->PatBlt(cr.left, point.y, cr.right - cr.left, 1, PATINVERT);
		pDrawDC->PatBlt(point.x, cr.top, 1, cr.bottom - cr.top, PATINVERT);

	}
	catch (CResourceException * e)
	{
		TRACE("CResourceException\n");
		e->Delete();
	}

	if (NULL == pDC)
	{
		ReleaseDC(pDrawDC);
	}
}

LRESULT CSpectrumSectionView::OnMouseLeave(WPARAM /*wParam*/, LPARAM /*lParam*/)
{
	m_bTrackingMouseRect = false;
	HideCrossHair();
	return 0;
}

void CSpectrumSectionView::RemoveSelectionRect()
{
	HideCrossHair();
}

void CSpectrumSectionView::RestoreSelectionRect()
{
	ShowCrossHair(m_PrevCrossHair);
}

void CSpectrumSectionView::OnUpdateViewShowCrosshair(CCmdUI* pCmdUI)
{
	pCmdUI->SetCheck(m_bShowCrossHair);
}

void CSpectrumSectionView::OnViewShowCrosshair()
{
	RemoveSelectionRect();
	m_bShowCrossHair = ! m_bShowCrossHair;
	RestoreSelectionRect();
}

UINT CSpectrumSectionView::GetPopupMenuID(CPoint /*point*/)
{
	return IDR_MENU_SPECTRUMSECTION_VIEW;
}

void CSpectrumSectionView::GetChannelRect(int Channel, RECT * pR) const
{
	CWaveSoapFrontDoc * pDoc = GetDocument();
	int nChannels = pDoc->WaveChannels();

	GetClientRect(pR);
	if (Channel >= nChannels)
	{
		pR->top = pR->bottom;
		pR->bottom += 2;
		return;
	}

	int h = (pR->bottom - pR->top + 1) / nChannels;
	// for all channels, the rectangle is of the same height
	pR->top = h * Channel;
	pR->bottom = pR->top + h - 1;
}

