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
	, m_bCrossHairDrawn(false)
	, m_FftOrder(512)
	, m_pFftSum(NULL)
	, m_bShowNoiseThreshold(FALSE)
	, m_bShowCrossHair(false)
	, m_bTrackingMouseRect(false)
	, m_nFftSumSize(0)
	, m_PlaybackSample(0)
	, m_pNoiseReduction(NULL)
	, m_FftWindowType(WindowTypeNuttall)

	, m_VerticalScale(1.)
	, m_FirstbandVisible(0.)
	, m_DbOffset(0.)
	, m_DbMin(-120.)   // max dB range
	, m_DbMax(0.)
	, m_DbPerPixel(1.)
	, m_Scale(1.)
	, m_bIsTrackingSelection(false)
	, m_FftWindowValid(false)
	, m_XOrigin(0)
	, m_CalculatedResultBegin(-1)
	, m_CalculatedResultEnd(-1)
{
	memzero(m_Heights);
	memzero(m_InvalidAreaTop);
	memzero(m_InvalidAreaBottom);
}

CSpectrumSectionView::~CSpectrumSectionView()
{
	delete[] m_pFftSum;
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
	ON_WM_SIZE()
	ON_WM_CONTEXTMENU()
	ON_UPDATE_COMMAND_UI(ID_VIEW_SHOW_CROSSHAIR, OnUpdateViewShowCrosshair)
	ON_COMMAND(ID_VIEW_SHOW_CROSSHAIR, OnViewShowCrosshair)
	//}}AFX_MSG_MAP
	ON_MESSAGE(WM_MOUSELEAVE, OnMouseLeave)
	ON_MESSAGE(UWM_NOTIFY_VIEWS, &CSpectrumSectionView::OnUwmNotifyViews)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CSpectrumSectionView drawing
namespace {

struct FftGraphBand
{
	int FftBand;
	int NumBandsToSum;

	int y;  // bottom of band
	int NumDisplayRows;		// number of display rows the band takes

	int nMin; // min X display coordinate of the power value in the band
	int nMax; // max X coordinate of the power value in the band
};
}
struct DoublePoint
{
	int y;
	int x_min;	// x_min - 1, actually
	int x_max;
};

double CSpectrumSectionView::WindowXToDb(int x) const
{
	return (x - m_XOrigin) * m_DbPerPixel + m_DbOffset;
}

int CSpectrumSectionView::DbToWindowX(double db) const
{
	return m_XOrigin + int((db - m_DbOffset) / m_DbPerPixel + 0.5);     // round
}

// this function reads total FftOrder*(NumberOfSamplesAveraged+1) samples
void CSpectrumSectionView::CalculateFftPowerSum(float * FftSum,
												SAMPLE_INDEX FirstSample,
												int NumberOfSamplesAveraged, int FftOrder)
{
	CWaveSoapFrontDoc* pDoc = GetDocument();
	NUMBER_OF_CHANNELS nChannels = pDoc->WaveChannels();

	ATL::CHeapPtr<float> FftArray;
	FftArray.Allocate(2 * FftOrder + 2);

	ATL::CHeapPtr<float> SrcF32Array;
	// FftOrder * 2 source real samples produces FftOrder frequency bands
	SrcF32Array.Allocate(FftOrder * 2 * nChannels);

	int i;

	for (i = 0; i < FftOrder * nChannels; i++)
	{
		FftSum[i] = 0.;
	}

	pDoc->m_WavFile.ReadSamples(ALL_CHANNELS,
								pDoc->m_WavFile.SampleToPosition(FirstSample), FftOrder, &SrcF32Array[0], SampleTypeFloat32);
	for (int FftSample = 0; FftSample < NumberOfSamplesAveraged; FftSample++)
	{
		// FftOrder * 2 source real samples produces FftOrder frequency bands
		FirstSample += FftOrder;
		pDoc->m_WavFile.ReadSamples(ALL_CHANNELS,
			pDoc->m_WavFile.SampleToPosition(FirstSample), FftOrder, &SrcF32Array[FftOrder*nChannels], SampleTypeFloat32);

		float * pFftSum = FftSum;
		for (int ch = 0; ch < nChannels; )
		{
			float * pSrcF32Array = &SrcF32Array[ch];
			// FftOrder * 2 source real samples produces FftOrder+1 frequency bands
			for (i = 0; i < FftOrder*2; i++, pSrcF32Array+=nChannels)
			{
				FftArray[i] = *pSrcF32Array * m_pFftWindow[i];
			}

			FastFourierTransform(&FftArray[0], (complex<float>*)&FftArray[0], FftOrder * 2);

			pFftSum[0] += FftArray[0] * FftArray[0];

			for (i = 1; i < FftOrder; i++)
			{
				pFftSum[i] += FftArray[i*2] * FftArray[i*2]
							+ FftArray[i*2+1] * FftArray[i*2+1];
			}
			ch++;
			pFftSum += FftOrder;
		}
		memmove(&SrcF32Array[0], &SrcF32Array[FftOrder*nChannels], FftOrder*nChannels*sizeof SrcF32Array[0]);
	}
}

static void BuildBandArray(double X_Offset, double dBToPixel,
							FftGraphBand * pBands,
							int NumBands, float const * pFftSum, double PowerCoeff)
{
#ifdef _DEBUG
	double dBMin = 200;
	double dBMax = -200;
	int FftBandsProcessed = 0;
	int MaxY = 0;
#endif
	for (int n = 0; n < NumBands; n++, pBands++)
	{
		pBands->nMin = 0x7FFFFF;
		pBands->nMax = -0x7FFFFF;

		for (int j = pBands->FftBand; j < pBands->FftBand + pBands->NumBandsToSum; j++)
		{
			double temp = pFftSum[j] * PowerCoeff;

			if (0 != temp)
			{
				temp = 10. * log10(temp);
#ifdef _DEBUG
				if (temp < dBMin)
				{
					dBMin = temp;
				}
				if (dBMax < temp)
				{
					dBMax = temp;
					MaxY = pBands->y;
				}
				FftBandsProcessed++;
#endif
			}
			else
			{
				temp = -200.;
			}
			int pos = int(temp * dBToPixel + X_Offset);

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
#if 0 && defined (_DEBUG)
	TRACE(L"BuildBandArray %d FFT bands, min %f dB, max %f dB at Y=%d\n", FftBandsProcessed, dBMin, dBMax, MaxY);
#endif
}

// the array contains pairs of points with the same Y coordinate, and X coordinates corresponding to min and max ranges
// it starts from OffsetY and goes down one DoublePoint per pixel row.
// Total number of DoublePoint is nNumberOfPoints. Typically, it's never exceeded
static int BuildPointArray(FftGraphBand * pBands, int NumBands,
							DoublePoint * ppArray,
							int nNumberOfPoints // items in ppArray, one per pixel row
							)
{
	int i, j;
	for (i = 0, j = 0, pBands += NumBands-1; i < NumBands && j < nNumberOfPoints; i++, pBands--)
	{
		for (int k = 0; k < pBands->NumDisplayRows && j < nNumberOfPoints; k++, j++, ppArray++)
		{
			ppArray->x_max = pBands->nMax+1;
			ppArray->x_min = pBands->nMin;
			ppArray->y = pBands->y + k;
		}
	}
	return j;	// filled pairs in ppArray
}

// The array contains pairs of points
static void DrawPointArray(CDC * pDC, DoublePoint * ppArray, int NumberOfPoints, int right)
{
	int LastXmax = ppArray[0].x_max;
	int LastXmin = ppArray[0].x_min;

	for (int n = 0; n < NumberOfPoints; n++, ppArray++)
	{
		//TRACE("ppArray[%d].x = %d, %d\n", n, ppArray[0].x_min, ppArray[0].x_max);
		if (n < NumberOfPoints - 1)
		{
			ASSERT(ppArray[1].y == ppArray[0].y + 1);

			if (ppArray[0].x_max < ppArray[1].x_min)
			{
				ppArray[0].x_max = (ppArray[1].x_min + ppArray[0].x_max) >> 1;
			}
			else if (ppArray[0].x_min > ppArray[1].x_max)
			{
				ppArray[0].x_min = (ppArray[0].x_min + ppArray[1].x_max) >> 1;
			}
		}

		if (ppArray[0].x_max < LastXmin)
		{
			ppArray[0].x_max = LastXmin;
		}
		else if (ppArray[0].x_min > LastXmax)
		{
			ppArray[0].x_min = LastXmax;
		}

		LastXmax = ppArray[0].x_max;
		LastXmin = ppArray[0].x_min;

		if (ppArray[0].x_max < 0)
		{
			continue;
		}
		else if (ppArray[0].x_max > right)
		{
			ppArray[0].x_max = right;
		}

		if (ppArray[0].x_min < 0)
		{
			ppArray[0].x_min = -1;
		}
		else if (ppArray[0].x_min > right)
		{
			continue;
		}
		pDC->MoveTo(ppArray[0].x_min, ppArray[0].y);
		pDC->LineTo(ppArray[0].x_max, ppArray[0].y);
	}
}

// Build band array with number of bands to sum and draw. No actual FFT data is calculated, only drawing table.
int CalculateFftBandArray(ATL::CHeapPtr<FftGraphBand> &IdArray, NotifyChannelHeightsData & heights, int ch,
						int FftOrder, double VerticalScale, double FirstBandVisible)
{
	int LastRow = 0;
	int k;
	int top = heights.ch[ch].clip_top;
	int bottom = heights.ch[ch].clip_bottom;

	int height = heights.ch[ch].bottom - heights.ch[ch].top;

	int ScaledHeight = height;
	int OffsetPixels = 0;
	if (!heights.ch[ch].minimized)
	{
		ScaledHeight = int(heights.NominalChannelHeight * VerticalScale);
		OffsetPixels = int(ScaledHeight * FirstBandVisible / FftOrder);
	}

	if (!IdArray.Reallocate(FftOrder))
	{
		return 0;
	}
	// fill the array
	int y = bottom;
	for (k = 0, LastRow = 0; k < FftOrder; k++)
	{
		if (y <= top)
		{
			break;
		}

		int FirstFftSample = (bottom - y + OffsetPixels) * FftOrder / ScaledHeight;

		IdArray[k].FftBand = FirstFftSample;
		IdArray[k].NumDisplayRows = 0;
		// see if the Fft band will take multiple display rows, or a single display row will take multiple bands

		while (y > top)
		{
			y--;
			IdArray[k].NumDisplayRows++;
			int NextFftSample = (bottom - y + OffsetPixels) * FftOrder / ScaledHeight;

			if (NextFftSample >= FftOrder)
			{
				IdArray[k].NumBandsToSum = FftOrder - FirstFftSample;
				IdArray[k].NumDisplayRows += y - top;
				y = top;
				break;
			}

			if (NextFftSample > FirstFftSample)
			{
				IdArray[k].NumBandsToSum = NextFftSample - FirstFftSample;
				break;
			}
		}

		IdArray[k].y = y;  // top of display band
	}
	return k;
}
void CSpectrumSectionView::OnDraw(CDC* pDC)
{
	CWaveSoapFrontDoc* pDoc = GetDocument();

	if (!pDoc->m_WavFile.IsOpen())
	{
		if (m_bCrossHairDrawn)
		{
			DrawCrossHair(m_PrevCrossHair, pDC);
		}
		return;
	}
	// calculate FFT
	// if there is no selection, calculate only current position

	CRect cr;
	GetClientRect(&cr);

	NUMBER_OF_CHANNELS nChannels = pDoc->WaveChannels();
	if (0 == nChannels)
	{
		return;
	}

	NUMBER_OF_SAMPLES WaveFileSamples = pDoc->WaveFileSamples();
	SAMPLE_INDEX nStartSample = pDoc->m_SelectionStart;

#if 0
	// calculate extents
	// find vertical offset in the result array and how many
	// rows to fill with this color
	int rows = cr.Height() / nChannels;
	// create an array of points
	int nNumberOfPoints = rows;

	// if all the chart was drawn, how many scans it would have:
	int TotalRows = int(rows * m_VerticalScale);

	if (0 == TotalRows)
	{
		if (m_bCrossHairDrawn)
		{
			DrawCrossHair(m_PrevCrossHair, pDC);
		}
		return;
	}
#endif

	if (!AllocateFftArrays())
	{
		return;
	}

	// build an array

	CPen BlackPen(PS_SOLID, 0, COLORREF(0));   // black pen

	CGdiObjectSaveT<CPen> OldPen(pDC, pDC->SelectObject(&BlackPen));

	ATL::CHeapPtr<float> NrMasking;
	ATL::CHeapPtr<float> NrResult;

	CPen BluePen(PS_SOLID, 0, RGB(0, 0, 255));
	CPen RedPen(PS_SOLID, 0, RGB(255, 0, 0));
	CPen LightGreenPen(PS_SOLID, 0, RGB(64, 255, 64));

	int const NrPreroll = 16;
	if (m_bShowNoiseThreshold
		&& NULL != m_pNoiseReduction
		&& WaveFileSamples >= m_NrFftOrder * NrPreroll)
	{
		NrMasking.Allocate(m_NrFftOrder * nChannels);
		NrResult.Allocate(m_NrFftOrder * nChannels);

		// fill the array
		SAMPLE_INDEX nNrStartSample = nStartSample;
		if (nNrStartSample >= m_NrFftOrder * NrPreroll)
		{
			nNrStartSample -= m_NrFftOrder * NrPreroll;
		}
		else
		{
			nNrStartSample = 0;
		}

		ATL::CHeapPtr<float> pSrcF32Array;
		pSrcF32Array.Allocate(m_NrFftOrder * nChannels);

		m_pNoiseReduction->Reset();

		for (int i = 0; i < NrPreroll; i++)
		{
			if (m_NrFftOrder != pDoc->m_WavFile.ReadSamples(ALL_CHANNELS,
					pDoc->m_WavFile.SampleToPosition(nNrStartSample), m_NrFftOrder, pSrcF32Array))
			{
				break;
			}

			int InputSamplesUsed = m_pNoiseReduction->FillInBuffer(pSrcF32Array, m_NrFftOrder);
			nStartSample += InputSamplesUsed;

			if (m_pNoiseReduction->CanProcessFft())
			{
				// now we have enough samples to do FFT
				m_pNoiseReduction->AnalyseFft();
			}
			m_pNoiseReduction->ResetOutBuffer();
		}

		// now that we have calculated the FFT
		// the result are non-interleaved arrays (m_NrFftOrder results for left followed by m_NrFftOrder results for right)
		m_pNoiseReduction->GetAudioMasking(NrMasking);

		m_pNoiseReduction->GetNoiseThreshold(NrResult);

	}

	pDC->SelectObject(&BlackPen);
	// fill the array
	ATL::CHeapPtr<FftGraphBand> IdArray;
	ATL::CHeapPtr<FftGraphBand> NrIdArray;

	// if there is selection, calculate the whole region sum
	int NumberOfFftSamplesAveraged =
		(pDoc->m_SelectionEnd - pDoc->m_SelectionStart) / m_FftOrder;
	if (NumberOfFftSamplesAveraged == 0)
	{
		NumberOfFftSamplesAveraged = 1;
	}
	else if (NumberOfFftSamplesAveraged > 100)
	{
		NumberOfFftSamplesAveraged = 100;
	}
	if (pDoc->m_PlayingSound)
	{
		NumberOfFftSamplesAveraged = 4;
		nStartSample = m_PlaybackSample;
	}

	if (WaveFileSamples < m_FftOrder * NumberOfFftSamplesAveraged)
	{
		NumberOfFftSamplesAveraged = WaveFileSamples / m_FftOrder - 1;
		if (NumberOfFftSamplesAveraged <= 0)
		{
			return;
		}
		nStartSample = 0;
	}

	if (nStartSample > WaveFileSamples - m_FftOrder * NumberOfFftSamplesAveraged)
	{
		nStartSample = WaveFileSamples - m_FftOrder * NumberOfFftSamplesAveraged;
	}
	if (nStartSample > m_FftOrder)
	{
		nStartSample -= m_FftOrder;
	}
	else
	{
		nStartSample = 0;
	}

	// this function reads total FftOrder*(NumberOfSamplesAveraged+1) samples
	CalculateFftPowerSum(m_pFftSum, nStartSample, NumberOfFftSamplesAveraged, m_FftOrder);

	double const PowerScaleCoeff = 4. / (NumberOfFftSamplesAveraged * m_FftOrder * m_FftOrder);

	double const NrPowerScaleCoeff = 1. / (m_NrFftOrder * m_NrFftOrder);

	// now that we have calculated the FFT

	ATL::CHeapPtr<DoublePoint> NrArrayXY;
	ATL::CHeapPtr<DoublePoint> ArrayXY;
	NrArrayXY.Allocate(cr.Height());

	ArrayXY.Allocate(cr.Height());

	// find vertical offset in the result array and how many
	// rows to fill with this color
	// build an array of vertical offsets for each band and for each channel
	if (!IdArray.Allocate(m_FftOrder))
	{
		return;          // Allocate is non-throwing
	}

	// vertical scrolling and scaling:
	// Each channel occupies integer number of pixels in the display units. Channels may have different height.
	// The scroll bar position is translated to integer pixel position for each channel.
	//
	for (int ch = 0; ch < nChannels; ch++)
	{
		int IdxSize, points;
		// draw noise threshold
		if (m_bShowNoiseThreshold
			&& NULL != m_pNoiseReduction)
		{
			IdxSize = CalculateFftBandArray(NrIdArray, m_Heights, ch,
											m_NrFftOrder, m_VerticalScale, m_FirstbandVisible * m_NrFftOrder / m_FftOrder);
			// draw lines for "tonal" bands, from left to right
			pDC->SelectObject(LightGreenPen);


			for (int n = 0; n < IdxSize - 1; n++)
			{
				for (int f = NrIdArray[n].FftBand; f < NrIdArray[n].FftBand + NrIdArray[n].NumBandsToSum; f++)
				{
					if (m_pNoiseReduction->IsTonalBand(ch, f))
					{
						pDC->MoveTo(0, NrIdArray[n].y);
						pDC->LineTo(cr.right, NrIdArray[n].y);
						break;
					}
				}
			}
			pDC->SelectObject(&RedPen);

			BuildBandArray(m_XOrigin - m_DbOffset / m_DbPerPixel, 1. / m_DbPerPixel, NrIdArray, IdxSize,
							NrMasking + m_NrFftOrder * ch, NrPowerScaleCoeff);
			points = BuildPointArray(NrIdArray, IdxSize, NrArrayXY, cr.Height());

			DrawPointArray(pDC, NrArrayXY, points, cr.right);

			pDC->SelectObject(&BluePen);

			BuildBandArray(m_XOrigin - m_DbOffset / m_DbPerPixel, 1. / m_DbPerPixel, NrIdArray, IdxSize,
							NrResult + m_NrFftOrder * ch, NrPowerScaleCoeff);
			points = BuildPointArray(NrIdArray, IdxSize, NrArrayXY, cr.Height());

			DrawPointArray(pDC, NrArrayXY, points, cr.right);
		}

		IdxSize = CalculateFftBandArray(IdArray, m_Heights, ch, m_FftOrder, m_VerticalScale, m_FirstbandVisible);

		pDC->SelectObject(&BlackPen);

		BuildBandArray(m_XOrigin - m_DbOffset / m_DbPerPixel, 1. / m_DbPerPixel, IdArray, IdxSize,
						m_pFftSum + ch * m_FftOrder, PowerScaleCoeff);

		points = BuildPointArray(IdArray, IdxSize, ArrayXY, cr.Height());

		DrawPointArray(pDC, ArrayXY, points, cr.right);

		// draw a separator
		if (ch + 1 < nChannels)
		{
			pDC->MoveTo(0, m_Heights.ch[ch].clip_bottom);
			pDC->LineTo(cr.right, m_Heights.ch[ch].clip_bottom);
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

	if (NULL == m_pFftSum
		|| m_nFftSumSize != nChannels * (m_FftOrder * 2 + 2))
	{
		delete[] m_pFftSum;
		m_FftWindowValid = false;

		m_nFftSumSize = nChannels * (m_FftOrder * 2 + 2);
		m_pFftSum = new float[m_nFftSumSize];

		if (NULL == m_pFftSum)
		{
			return FALSE;
		}

	}

	if (!m_FftWindowValid)
	{
		if (NULL == m_pFftWindow)
		{
			m_pFftWindow.Allocate(m_FftOrder * 2);
		}

		if (NULL == m_pFftWindow)
		{
			return FALSE;
		}

		for (int w = 0; w < m_FftOrder * 2; w++)
		{
			// X changes from 0 to 2pi
			double X = (w + 0.5) * M_PI /  m_FftOrder;

			switch (m_FftWindowType)
			{
			default:
			case WindowTypeSquaredSine:
				// squared sine
				m_pFftWindow[w] = float(0.5 - 0.5 * cos (X));
				break;

			case WindowTypeHalfSine:
				// half sine
				m_pFftWindow[w] = float(0.707107 * sin (0.5 * X));
				break;

			case WindowTypeHamming:
				// Hamming window (sucks!!!)
				m_pFftWindow[w] = float(0.9 *(0.54 - 0.46 * cos (X)));
				break;

			case WindowTypeNuttall:
				// Nuttall window:
				// w(n) = 0.355768 - 0.487396*cos(2pn/N) + 0.144232*cos(4pn/N) - 0.012604*cos(6pn/N)
				m_pFftWindow[w] = float(0.355768 - 0.487396*cos(X) + 0.144232*cos(2 * X) - 0.012604*cos(3 * X));
				break;
			}
		}
		m_FftWindowValid = true;
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

	ShowScrollBar(SB_VERT, FALSE);
	ShowScrollBar(SB_HORZ, TRUE);
	return 0;
}

int CSpectrumSectionView::OnMouseActivate(CWnd* pDesktopWnd, UINT nHitTest, UINT message)
{
	// don't call CView function, to avoid getting focus to the window

	return CWnd::OnMouseActivate(pDesktopWnd, nHitTest, message);
}

void CSpectrumSectionView::OnInitialUpdate()
{
	NotifyViewsData data;
	data.SpectrumSection.Scale = 1.;
	NotifySiblingViews(SpectrumSectionScaleChange, &data);
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
		CPlaybackUpdateInfo * pInfo = static_cast<CPlaybackUpdateInfo *>(pHint);
		m_PlaybackSample = pInfo->PlaybackPosition();

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
	else if (lHint == CWaveSoapFrontDoc::UpdateWholeFileChanged)
	{
		// set dB ranges,
		AdjustDbRange();
		Invalidate();
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
	NotifyViewsData data;
	data.SpectrumSection.Scale = m_Scale * 2.;
	NotifySiblingViews(SpectrumSectionScaleChange, &data);
}

void CSpectrumSectionView::OnUpdateViewSsZoominhor2(CCmdUI* pCmdUI)
{
	pCmdUI->Enable(m_Scale < 16.);
}

void CSpectrumSectionView::OnViewSsZoomouthor2()
{
	NotifyViewsData data;
	data.SpectrumSection.Scale = m_Scale / 2.;
	NotifySiblingViews(SpectrumSectionScaleChange, &data);
}

void CSpectrumSectionView::OnUpdateViewSsZoomouthor2(CCmdUI* pCmdUI)
{
	pCmdUI->Enable(m_Scale > 1.);
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

void CSpectrumSectionView::OnContextMenu(CWnd* /*pWnd*/, CPoint point)
{
	// make sure window is active
	GetParentFrame()->ActivateFrame();

	NotifyViewsData notify = { 0 };
	notify.PopupMenu.p = point;
	notify.PopupMenu.NormalMenuId = IDR_MENU_SPECTRUMSECTION_VIEW;
	notify.PopupMenu.MinimizedMenuId = IDR_MENU_SPECTRUMSECTION_MINIMIZED;

	NotifySiblingViews(ShowChannelPopupMenu, &notify);
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

void CSpectrumSectionView::SetNewFftOffset(double first_band)
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
	Invalidate(TRUE);
}

void CSpectrumSectionView::OnSize(UINT nType, int cx, int cy)
{
	m_XOrigin = cx;
	BaseClass::OnSize(nType, cx, cy);
	NotifyViewsData data;
	data.SpectrumSection.Scale = m_Scale;
	NotifySiblingViews(SpectrumSectionScaleChange, &data);
}

double CSpectrumSectionView::AdjustOffset(double offset) const
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

void CSpectrumSectionView::AdjustDbRange()
{
	CWaveSoapFrontDoc * pDoc = GetDocument();
	if (!pDoc->m_WavFile.IsOpen())
	{
		return;
	}

	double OldMin = m_DbMin;
	double OldMax = m_DbMax;
	if (pDoc->m_WavFile.GetSampleType() == SampleTypeFloat32)
	{
		m_DbMax = 10.;
		m_DbMin = -140. - 10.*log10(m_FftOrder / 512.);
	}
	else
	{
		m_DbMax = 0.;
		m_DbMin = -120. - 10.*log10(m_FftOrder / 512.);
		if (pDoc->m_WavFile.GetSampleType() == SampleType32bit)
		{
			m_DbMin -= 20.;
		}
	}
	if (OldMin != m_DbMin
		|| OldMax != m_DbMax)
	{
		// recalculate db per pixel
		NotifyViewsData data;
		data.SpectrumSection.Scale = m_Scale;
		NotifySiblingViews(SpectrumSectionScaleChange, &data);
	}
}

LRESULT CSpectrumSectionView::OnUwmNotifyViews(WPARAM wParam, LPARAM lParam)
{
	NotifyViewsData data, *pData;
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
	case FftOffsetChanged:
		SetNewFftOffset(*(double*)lParam);
		break;
	case FftBandsChanged:
	{
		int NewFftBands = *(int*)lParam;
		if (m_FftOrder == NewFftBands)
		{
			break;
		}
		m_FftOrder = NewFftBands;
		m_bShowNoiseThreshold = false;
		m_pFftWindow.Free();
		m_FftWindowValid = false;
		AdjustDbRange();
		Invalidate();
	}
		break;
	case ChannelHeightsChanged:
		m_Heights = *(NotifyChannelHeightsData*)lParam;
		Invalidate();
		break;

	case FftWindowChanged:
	{
		int NewWindowType = *(int*)lParam;
		if (m_FftWindowType == NewWindowType)
		{
			break;
		}

		m_FftWindowType = NewWindowType;
		m_FftWindowValid = false;
		Invalidate();
	}
		break;
		// Horizontal scroll and scaling messages:
	case SpectrumSectionHorScrollTo:
	{
		// clip the new origin, adjust it to closest pixel,
		pData = (NotifyViewsData *) lParam;
		data.SpectrumSection.Origin = pData->SpectrumSection.Origin;
		double DbInView = (m_DbMax - m_DbMin) / m_Scale;
		if (data.SpectrumSection.Origin > m_DbMax)
		{
			data.SpectrumSection.Origin = m_DbMax;
		}
		else if (data.SpectrumSection.Origin < m_DbMin + DbInView)
		{
			data.SpectrumSection.Origin = m_DbMin + DbInView;
		}
		data.SpectrumSection.Scale = m_Scale;
		data.SpectrumSection.DbPerPixel = m_DbPerPixel;
		data.SpectrumSection.Max = m_DbMax;
		data.SpectrumSection.Min = m_DbMin;
		NotifySiblingViews(SpectrumSectionDbOriginChange, &data);
	}
		break;

	case SpectrumSectionScaleChange:
	{
		pData = (NotifyViewsData *) lParam;
		// change current scale, double 1 to 256
		if (pData->SpectrumSection.Scale < 1.)
		{
			pData->SpectrumSection.Scale = 1.;
		}
		else if (pData->SpectrumSection.Scale > 16.)
		{
			pData->SpectrumSection.Scale = 16.;
		}
		m_Scale = pData->SpectrumSection.Scale;

		CRect r;
		GetClientRect(r);

		double DbInView = (m_DbMax - m_DbMin) / m_Scale;
		double origin = m_DbOffset;
		double DbPerPixel = 1.;
		if (origin > m_DbMax)
		{
			origin = m_DbMax;
		}
		else if (origin < m_DbMin + DbInView)
		{
			origin = m_DbMin + DbInView;
		}
		if (r.Width() != 0)
		{
			DbPerPixel = DbInView / r.Width();
		}
		data.SpectrumSection.Scale = m_Scale;
		data.SpectrumSection.Origin = origin;
		data.SpectrumSection.DbPerPixel = DbPerPixel;
		data.SpectrumSection.Max = m_DbMax;
		data.SpectrumSection.Min = m_DbMin;
		NotifySiblingViews(SpectrumSectionDbScaleChange, &data);
	}
		break;

	case SpectrumSectionDbScaleChange:
	{
		pData = (NotifyViewsData *)lParam;
		if (pData->SpectrumSection.DbPerPixel != m_DbPerPixel)
		{
			m_DbPerPixel = pData->SpectrumSection.DbPerPixel;
			Invalidate(TRUE);
		}
	}
		// fall through, no break
	case SpectrumSectionDbOriginChange:
	{
		pData = (NotifyViewsData *)lParam;
		pData->SpectrumSection.Origin -= fmod(pData->SpectrumSection.Origin - m_DbMax, m_DbPerPixel);
		// scroll left or right
		int ScrollPixels = int(floor((m_DbMax-pData->SpectrumSection.Origin) / m_DbPerPixel + 0.5) - floor((m_DbMax-m_DbOffset) / m_DbPerPixel + 0.5));
		m_DbOffset = pData->SpectrumSection.Origin;

		ScrollWindow(ScrollPixels, 0);
	}
		break;

		break;
	}

	return 0;
}
