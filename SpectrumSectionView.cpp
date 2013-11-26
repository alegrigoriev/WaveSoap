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
#include "waveproc.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CSpectrumSectionView

IMPLEMENT_DYNCREATE(CSpectrumSectionView, CView)

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
	, m_nFftSumSize(0)
	, m_PlaybackSample(0)
	, m_pNoiseReduction(NULL)
	, m_DbOffset(0.)
	, m_DbRange(120.)   // max dB range
	, m_DbRangeInView(70.)
	, m_DbPerPixel(1.)
{
	memzero(m_Heights);
}

CSpectrumSectionView::~CSpectrumSectionView()
{
	delete[] m_pFftSum;
	delete[] m_pWindow;
	delete m_pNoiseReduction;
}


BEGIN_MESSAGE_MAP(CSpectrumSectionView, BaseClass)
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
	ON_MESSAGE(UWM_NOTIFY_VIEWS, &CSpectrumSectionView::OnUwmNotifyViews)
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

int CSpectrumSectionView::InitBandArray(ATL::CHeapPtr<FftGraphBand> & pBands, int rows, int FftOrder)
{
	CWaveFftView * pFftView = dynamic_cast<CWaveFftView *>(m_pVertMaster);
	int TotalRows = int(rows * pFftView->m_VerticalScale);

	int LastFftSample = int(FftOrder - pFftView->m_FirstbandVisible);
	int FirstFftSample = LastFftSample + MulDiv(-rows, FftOrder, TotalRows);

	if (FirstFftSample < 0)
	{
		LastFftSample -= FirstFftSample;
		FirstFftSample = 0;
	}
	int FirstRowInView = MulDiv(FirstFftSample, TotalRows, FftOrder);

	int FftSamplesInView = LastFftSample - FirstFftSample + 1;

	int IdxSize1 = __min(rows, FftSamplesInView);

	pBands.Allocate(IdxSize1+1);

	int k;
	int LastRow = 0;

	for (k = 0; k < IdxSize1; k++)
	{
		if (FirstFftSample >= FftOrder
			|| LastRow >= rows)
		{
			break;
		}
		pBands[k].nFftOffset = FirstFftSample;
		FirstFftSample++;
		int NextRow = FirstFftSample * TotalRows / FftOrder - FirstRowInView;
		if (NextRow == LastRow)
		{
			NextRow++;
			FirstFftSample = (NextRow + FirstRowInView) * FftOrder / TotalRows;
		}
		if (NextRow > rows)
		{
			NextRow = rows;
		}
		pBands[k].nNumOfRows = NextRow - LastRow;
		LastRow = NextRow;
	}
	ASSERT(LastRow <= rows);

	pBands[k].nFftOffset = FftOrder;
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

		FirstSample += FftOrder;

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

void CSpectrumSectionView::BuildPointArray(FftGraphBand * pBands, int NumBands,
											DoublePoint * ppArray, int nNumberOfPoints, int OffsetY)
{
	for (int i = 0, j = 0; i < NumBands && j < nNumberOfPoints; i++, pBands++)
	{
		for (int k = 0; k < pBands->nNumOfRows && j < nNumberOfPoints; k++, j++, ppArray++)
		{
			ppArray[0][0].x = pBands->nMax;
			ppArray[0][1].x = pBands->nMin - 1;
			ppArray[0][0].y = OffsetY + j;
			ppArray[0][1].y = OffsetY + j;
		}
	}
}

void CSpectrumSectionView::DrawPointArray(CDC * pDC, DoublePoint * ppArray, int NumberOfPoints, int right)
{
	int LastX0 = ppArray[0][0].x;
	int LastX1 = ppArray[0][1].x;

	for (int n = 0; n < NumberOfPoints; n++, ppArray++)
	{
		//TRACE("ppArray[%d].x = %d, %d\n", n, ppArray[0][1].x, ppArray[0][0].x);
		if (n < NumberOfPoints - 1
			&& ppArray[1][0].x >= ppArray[1][1].x)
		{
			if (ppArray[0][0].x < ppArray[1][1].x)
			{
				ppArray[0][0].x = (ppArray[1][1].x + ppArray[0][0].x) >> 1;
			}
			else if (ppArray[1][0].x < ppArray[0][1].x)
			{
				ppArray[0][1].x = (ppArray[0][1].x + ppArray[1][0].x) >> 1;
			}
		}

		if (LastX0 >= LastX1)
		{
			if (ppArray[0][0].x < LastX1)
			{
				ppArray[0][0].x = LastX1;
			}
			else if (ppArray[0][1].x > LastX0)
			{
				ppArray[0][1].x = LastX0;
			}
		}

		LastX0 = ppArray[0][0].x;
		LastX1 = ppArray[0][1].x;

		if (ppArray[0][0].x < 0)
		{
			continue;
		}
		else if (ppArray[0][0].x > right)
		{
			ppArray[0][0].x = right;
		}

		if (ppArray[0][1].x < 0)
		{
			ppArray[0][1].x = -1;
		}
		else if (ppArray[0][1].x > right)
		{
			continue;
		}
		pDC->MoveTo(ppArray[0][0]);
		pDC->LineTo(ppArray[0][1]);
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

	CPen BlackPen(PS_SOLID, 0, COLORREF(0));   // black pen

	CGdiObjectSaveT<CPen> OldPen(pDC, pDC->SelectObject( & BlackPen));


	if (m_bShowNoiseThreshold
		&& NULL != m_pNoiseReduction)
	{
		CPen BluePen(PS_SOLID, 0, RGB(0, 0, 255));
		CPen RedPen(PS_SOLID, 0, RGB(255, 0, 0));
		CPen LightGreenPen(PS_SOLID, 0, RGB(64, 255, 64));
		// todo: show second graph with blue color:
		// calculate power distribution and masking function
		// Items that can be shown:
		// filtered power distribution
		// masking function with noise threshold
		ATL::CHeapPtr<DoublePoint> NrArrayXY;
		ATL::CHeapPtr<float> NrResult;
		ATL::CHeapPtr<FftGraphBand> NrIdArray;
		NrResult.Allocate(m_NrFftOrder * nChannels);

		// fill the array
		int IdxSize = InitBandArray(NrIdArray, rows, m_NrFftOrder);

		int const Preroll = 16;

		if (nStartSample >= m_NrFftOrder * Preroll)
		{
			nStartSample -= m_NrFftOrder * Preroll;
		}
		else
		{
			nStartSample = 0;
		}

		ATL::CHeapPtr<WAVE_SAMPLE> pSrcIntArray;
		pSrcIntArray.Allocate(m_NrFftOrder * nChannels);

		m_pNoiseReduction->Reset();

		for (int i = 0; i < Preroll; i++)
		{
			if (m_NrFftOrder != pDoc->m_WavFile.ReadSamples(ALL_CHANNELS,
					pDoc->m_WavFile.SampleToPosition(nStartSample), m_NrFftOrder, pSrcIntArray))
			{
				break;
			}

			int InputSamplesUsed = m_pNoiseReduction->FillInBuffer(pSrcIntArray, m_NrFftOrder);
			nStartSample += InputSamplesUsed;

			if (m_pNoiseReduction->CanProcessFft())
			{
				// now we have enough samples to do FFT
				m_pNoiseReduction->AnalyseFft();
			}
			m_pNoiseReduction->ResetOutBuffer();
		}

		double const PowerScaleCoeff = 1. / (32768. * m_NrFftOrder * m_NrFftOrder) ;

		// now that we have calculated the FFT

		NrArrayXY.Allocate(nNumberOfPoints);

		m_pNoiseReduction->GetAudioMasking(NrResult);

		for (int ch = 0; ch < nChannels; ch++)
		{
			// draw lines for "tonal" bands
			pDC->SelectObject(LightGreenPen);

			for (int n = 0; n < IdxSize-1; n++)
			{
				for (int f = NrIdArray[n].nFftOffset; f < NrIdArray[n+1].nFftOffset; f++)
				{
					if (m_pNoiseReduction->IsTonalBand(ch, f))
					{
						pDC->MoveTo(0, rows * (ch + 1) - n);
						pDC->LineTo(cr.right, rows * (ch + 1) - n);
						break;
					}
				}
			}

			pDC->SelectObject( & RedPen);
			BuildBandArray(PowerScaleCoeff, NrIdArray, IdxSize, NrResult + ch, m_NrFftOrder);

			BuildPointArray(NrIdArray, IdxSize, NrArrayXY, nNumberOfPoints, rows * ch);

			DrawPointArray(pDC, NrArrayXY, nNumberOfPoints, cr.right);

			// draw a separator
			if (nChannels > 1)
			{
				pDC->MoveTo(0, rows * ch);
				pDC->LineTo(cr.right, rows * ch);
			}

		}

		m_pNoiseReduction->GetNoiseThreshold(NrResult);

		pDC->SelectObject( & BluePen);
		for (int ch = 0; ch < nChannels; ch++)
		{
			BuildBandArray(PowerScaleCoeff, NrIdArray, IdxSize, NrResult + ch, m_NrFftOrder);

			BuildPointArray(NrIdArray, IdxSize, NrArrayXY, nNumberOfPoints, rows * ch);

			DrawPointArray(pDC, NrArrayXY, nNumberOfPoints, cr.right);

			// draw a separator
			if (nChannels > 1)
			{
				pDC->MoveTo(0, rows * ch);
				pDC->LineTo(cr.right, rows * ch);
			}

		}
	}

	pDC->SelectObject( & BlackPen);
	// fill the array
	ATL::CHeapPtr<FftGraphBand> IdArray;
	int IdxSize = InitBandArray(IdArray, rows, m_FftOrder);

	// if there is selection, calculate the whole region sum
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

	CalculateFftPowerSum(m_pFftSum, nStartSample, NumberOfFftSamplesAveraged, m_FftOrder);

	double const PowerScaleCoeff = 4. / (NumberOfFftSamplesAveraged * 32768. * m_FftOrder * 32768. * m_FftOrder);

	// now that we have calculated the FFT

	ATL::CHeapPtr<DoublePoint> ArrayXY;
	ArrayXY.Allocate(nNumberOfPoints);

	for (int ch = 0; ch < nChannels; ch++)
	{
		BuildBandArray(PowerScaleCoeff, IdArray, IdxSize, m_pFftSum + ch, m_FftOrder);

		BuildPointArray(IdArray, IdxSize, ArrayXY, nNumberOfPoints, rows * ch);

		DrawPointArray(pDC, ArrayXY, nNumberOfPoints, cr.right);

		// draw a separator
		if (nChannels > 1)
		{
			pDC->MoveTo(0, rows * ch);
			pDC->LineTo(cr.right, rows * ch);
		}

	}

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
			// X changes from 0 to 2pi
			double X = (w + 0.5) * M_PI /  m_FftOrder;

			switch (FftWindowType)
			{
			default:
			case pFftView->WindowTypeSquaredSine:
				// squared sine
				m_pWindow[w] = float(0.5 - 0.5 * cos (X));
				break;

			case pFftView->WindowTypeHalfSine:
				// half sine
				m_pWindow[w] = float(0.707107 * sin (0.5 * X));
				break;

			case pFftView->WindowTypeHamming:
				// Hamming window (sucks!!!)
				m_pWindow[w] = float(0.9 *(0.54 - 0.46 * cos (X)));
				break;

			case pFftView->WindowTypeNuttall:
				// Nuttall window:
				// w(n) = 0.355768 - 0.487396*cos(2pn/N) + 0.144232*cos(4pn/N) - 0.012604*cos(6pn/N)
				m_pWindow[w] = float(0.355768 - 0.487396*cos(X) + 0.144232*cos(2 * X) - 0.012604*cos(3 * X));
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
	BaseClass::AssertValid();
}

void CSpectrumSectionView::Dump(CDumpContext& dc) const
{
	BaseClass::Dump(dc);
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
	if (BaseClass::OnCreate(lpCreateStruct) == -1)
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
		//m_bShowNoiseThreshold = false;

		if ((pDoc->m_SelectionStart <= pInfo->m_End
				&& pDoc->m_SelectionEnd + 2 * m_FftOrder >= pInfo->m_Begin)
			|| pInfo->m_NewLength != -1)
		{
			Invalidate();
		}
	}
	else if (lHint == pDoc->UpdateSelectionChanged)
	{
		//m_bShowNoiseThreshold = false;
		Invalidate();
		return;
	}
	else if (lHint == pDoc->UpdatePlaybackPositionChanged
			&& NULL != pHint)
	{
		m_bShowNoiseThreshold = false;
		CSoundUpdateInfo * pInfo = static_cast<CSoundUpdateInfo *>(pHint);
		m_PlaybackSample = pInfo->m_PlaybackPosition;

		Invalidate();
		return;
	}
	else if (lHint == pDoc->UpdateNoiseThresholdChanged
			&& NULL != pHint)
	{
		NoiseThresholdUpdateInfo * pInfo = static_cast<NoiseThresholdUpdateInfo *>(pHint);
		if (NULL != pInfo)
		{
			delete m_pNoiseReduction;
			m_pNoiseReduction = NULL;

			if (NULL != pInfo->pNoiseReductionParameters)
			{
				m_pNoiseReduction = new NoiseReductionCore(pInfo->FftOrder,
										pDoc->WaveChannels(), pInfo->SampleRate, * pInfo->pNoiseReductionParameters);

				m_NrFftOrder = pInfo->FftOrder;
			}

			m_bShowNoiseThreshold = NULL != m_pNoiseReduction;
			Invalidate();
		}
		return;
	}
	else if (0 == pHint)
	{
		BaseClass::OnUpdate(pSender, lHint, pHint);
	}
}

void CSpectrumSectionView::OnUpdateViewSsShowNoiseThreshold(CCmdUI* pCmdUI)
{
	if (NULL != m_pNoiseReduction || m_bShowNoiseThreshold)
	{
		pCmdUI->SetCheck(m_bShowNoiseThreshold);
	}
	else
	{
		pCmdUI->Enable(FALSE);
	}
}

void CSpectrumSectionView::OnViewSsShowNoiseThreshold()
{
	if (NULL != m_pNoiseReduction)
	{
		m_bShowNoiseThreshold = ! m_bShowNoiseThreshold;
	}
	else
	{
		m_bShowNoiseThreshold = FALSE;
	}
	Invalidate();
}

void CSpectrumSectionView::OnViewSsZoominhor2()
{
	BaseClass::OnViewZoominHor2();
}

void CSpectrumSectionView::OnUpdateViewSsZoominhor2(CCmdUI* pCmdUI)
{
	pCmdUI->Enable(dSizeX > 2.);
}

void CSpectrumSectionView::OnViewSsZoomouthor2()
{
	BaseClass::OnViewZoomOutHor2();
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
	GetClientRect(pR);
	if (Channel >= m_Heights.NumChannels)
	{
		pR->top = pR->bottom;
		pR->bottom += 2;
		return;
	}

	pR->top = m_Heights.ch[Channel].top;
	pR->bottom = m_Heights.ch[Channel].bottom;
}

void CSpectrumSectionView::GetChannelClipRect(int Channel, RECT * pR) const
{
	GetClientRect(pR);
	if (Channel >= m_Heights.NumChannels)
	{
		pR->top = pR->bottom;
		pR->bottom += 2;
		return;
	}

	pR->top = m_Heights.ch[Channel].clip_top;
	pR->bottom = m_Heights.ch[Channel].clip_bottom;
}



afx_msg LRESULT CSpectrumSectionView::OnUwmNotifyViews(WPARAM wParam, LPARAM lParam)
{
	switch (wParam)
	{
	case FftBandsChanged:
		m_bShowNoiseThreshold = false;
		delete[] m_pWindow;
		m_pWindow = NULL;
		Invalidate();
		break;
	case ChannelHeightsChanged:
		m_Heights = *(NotifyChannelHeightsData*)lParam;
		Invalidate();
		break;
	}

	return 0;
}
