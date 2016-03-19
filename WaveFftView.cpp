// Copyright Alexander Grigoriev, 1997-2002, All Rights Reserved
// WaveFftView.cpp : implementation file
//

#include "stdafx.h"
#include "WaveSoapFront.h"
#include "WaveSoapFrontView.h"
#include "WaveFftView.h"
#include "fft.h"
#include "MainFrm.h"
#include "GdiObjectSave.h"
#include "resource.h"       // main symbols

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

static const unsigned char palette[128 * 3] =
{
	255, 254, 248,
	251, 254, 244,
	247, 254, 240,
	243, 254, 236,
	244, 254, 236,
	245, 254, 236,
	244, 254, 230,
	242, 254, 224,
	241, 254, 215,
	241, 254, 213,
	240, 254, 208,
	241, 254, 203,
	241, 254, 197,
	242, 254, 192,
	243, 254, 186,
	245, 254, 181,
	247, 254, 176,
	249, 254, 171,
	252, 254, 165,
	254, 253, 160,
	254, 249, 155,
	254, 245, 150,
	254, 241, 145,
	254, 236, 140,
	254, 232, 135,
	254, 226, 130,
	254, 221, 125,
	254, 215, 120,
	253, 208, 115,
	253, 202, 110,
	253, 195, 105,
	253, 187, 100,
	253, 180, 95,
	253, 172, 91,
	253, 164, 86,
	253, 155, 81,
	253, 146, 77,
	253, 137, 72,
	253, 128, 67,
	253, 118, 63,
	252, 108, 58,
	252, 98, 54,
	252, 87, 49,
	252, 76, 45,
	252, 65, 40,
	252, 54, 36,
	252, 42, 31,
	252, 30, 27,
	252, 23, 28,
	251, 19, 32,
	251, 14, 36,
	251, 10, 41,
	251, 6, 46,
	250, 3, 52,
	245, 3, 59,
	241, 3, 66,
	237, 3, 74,
	233, 3, 80,
	229, 3, 87,
	225, 3, 93,
	221, 3, 99,
	217, 3, 105,
	213, 3, 110,
	209, 3, 115,
	205, 3, 120,
	201, 3, 125,
	197, 3, 129,
	193, 3, 133,
	190, 3, 137,
	186, 3, 141,
	182, 3, 144,
	178, 3, 147,
	175, 3, 150,
	171, 3, 153,
	168, 3, 156,
	164, 3, 158,
	161, 3, 160,
	152, 3, 157,
	144, 3, 154,
	135, 3, 150,
	127, 3, 147,
	119, 3, 144,
	112, 3, 140,
	105, 3, 137,
	98, 2, 134,
	91, 2, 131,
	84, 2, 128,
	78, 2, 125,
	72, 2, 121,
	66, 2, 118,
	61, 2, 115,
	55, 2, 113,
	50, 2, 110,
	45, 2, 107,
	40, 2, 104,
	45, 2, 107,
	36, 2, 101,
	31, 2, 98,
	27, 2, 96,
	23, 2, 93,
	20, 2, 90,
	16, 2, 88,
	13, 2, 85,
	10, 2, 83,
	7, 2, 80,
	4, 2, 78,
	2, 2, 76,
	2, 5, 73,
	1, 9, 69,
	0, 11, 67,
	0, 13, 64,
	0, 15, 62,
	0, 16, 60,
	0, 18, 58,
	0, 19, 56,
	0, 20, 54,
	0, 21, 53,
	0, 22, 51,
	0, 23, 49,
	0, 24, 47,
	0, 25, 46,
	0, 26, 41,
	0, 27, 40,
	0, 28, 36,
	0, 27, 25,
	0, 27, 15,
	0, 27, 5,
	0, 0, 0,

};

struct RgnData
{
	ATL::CHeapPtr<RGNDATAHEADER> hdr;

	RgnData(int InitialNumberOfRects = 16);
	void AddRect(int left, int top, int right, int bottom);
	bool Allocate(int NumberOfRects);
	operator RGNDATA*() { return CONTAINING_RECORD(PRGNDATAHEADER(hdr), RGNDATA, rdh); }
	BOOL GetRegionData(CRgn const * rgn);

	int AllocatedNumberOfRects;

	RECT* begin()
	{
		return  (RECT*)((operator RGNDATA*())->Buffer);
	}
	RECT* end()
	{
		return begin()+hdr->nCount;
	}
	RECT & operator[](int n)
	{
		ASSERT(n < AllocatedNumberOfRects);
		ASSERT(n <= (int)hdr->nCount);
		return begin()[n];
	}
};

RgnData::RgnData(int InitialNumberOfRects)
	:AllocatedNumberOfRects(-1)
{
	Allocate(InitialNumberOfRects);
	hdr->dwSize = sizeof hdr;
	hdr->iType = RDH_RECTANGLES;
	hdr->nCount = 0;
	hdr->nRgnSize = sizeof hdr;
	hdr->rcBound.top = 0;
	hdr->rcBound.bottom = 0;
	hdr->rcBound.left = 0;
	hdr->rcBound.right = 0;
}

void RgnData::AddRect(int left, int top, int right, int bottom)
{
	if ((int)hdr->nCount < AllocatedNumberOfRects)
	{
		RECT * rect = begin() + hdr->nCount;
		rect->left = left;
		rect->top = top;
		rect->right = right;
		rect->bottom = bottom;

		if (hdr->nCount == 0)
		{
			hdr->rcBound = rect[0];
		}
		else
		{
			if (hdr->rcBound.left > left)
			{
				hdr->rcBound.left = left;
			}
			if (hdr->rcBound.right < right)
			{
				hdr->rcBound.right = right;
			}
			if (hdr->rcBound.top > top)
			{
				hdr->rcBound.top = top;
			}
			if (hdr->rcBound.bottom < bottom)
			{
				hdr->rcBound.bottom = bottom;
			}
		}
		hdr->nCount++;
		hdr->nRgnSize += sizeof (RECT);
	}
}

bool RgnData::Allocate(int NumberOfRects)
{
	if (AllocatedNumberOfRects >= NumberOfRects)
	{
		return true;
	}
	if (!hdr.ReallocateBytes(sizeof (RGNDATAHEADER) + sizeof (RECT) * NumberOfRects))
	{
		return false;
	}
	AllocatedNumberOfRects = NumberOfRects;
	return true;
}

BOOL RgnData::GetRegionData(CRgn const * rgn)
{
	int RequiredBytes = rgn->GetRegionData(NULL, 0);
	if (!hdr.ReallocateBytes(RequiredBytes))
	{
		return FALSE;
	}
	AllocatedNumberOfRects = (RequiredBytes - sizeof(RGNDATAHEADER)) / sizeof(RECT);
	return rgn->GetRegionData(*this, RequiredBytes) != 0;
}

namespace
{
// It's assumed the rectangles are normalized (right <= left)
static bool rect_less(RECT const& r1, RECT const& r2)
{
	if (r1.left < r2.left)
	{
		return true;
	}
	if (r1.left > r2.left)
	{
		return false;
	}
	if (r1.right < r2.right)
	{
		return true;
	}
	if (r1.right > r2.right)
	{
		return false;
	}
	if (r1.top < r2.top)
	{
		return true;
	}
	if (r1.top > r2.top)
	{
		return false;
	}
	if (r1.bottom < r2.bottom)
	{
		return true;
	}
	//if (r1.bottom > r2.bottom)
	{
	}
	return false;
}
static bool rect_left_less(RECT const& r1, RECT const& r2)
{
	return r1.left < r2.left;
}
}

/////////////////////////////////////////////////////////////////////////////
// CWaveFftView

IMPLEMENT_DYNCREATE(CWaveFftView, CView);

CWaveFftView::CWaveFftView()
	: m_pFftResultArray(NULL),
	m_FftResultArrayWidth(0),
	m_FftResultArrayHeight(0),
	m_FirstbandVisible(0),
	m_FftWindowType(GetApp()->m_FftWindowType),
	m_IndexOfFftBegin(0),
	m_FftArraySize(0),
	m_PrevSelectionEnd(0),
	m_PrevSelectionStart(0),
	m_PrevSelectedChannel(0),
	m_VerticalScale(1.)
	, m_FftArrayBusyMask(0)
	, m_AllocatedFftBufSize(0)
	, m_FftWindowValid(false)
#ifdef _DEBUG
	, m_DrawingBeginTime(0)
#endif
{
	m_FftOrder = 1 << GetApp()->m_FftBandsOrder;
	m_FftSpacing = m_FftOrder;
}

CWaveFftView::~CWaveFftView()
{
	delete[] m_pFftResultArray;
	m_pFftResultArray = NULL;
}


BEGIN_MESSAGE_MAP(CWaveFftView, BaseClass)
	//{{AFX_MSG_MAP(CWaveFftView)
	ON_COMMAND(ID_VIEW_ZOOMVERT_NORMAL, OnViewZoomvertNormal)
	ON_COMMAND(ID_VIEW_ZOOMINVERT, OnViewZoomInVert)
	ON_COMMAND(ID_VIEW_ZOOMOUTVERT, OnViewZoomOutVert)
	ON_WM_PAINT()
	ON_WM_ERASEBKGND()
	ON_COMMAND_RANGE(ID_FFT_BANDS_32, ID_FFT_BANDS_16384, OnFftBands)
	ON_UPDATE_COMMAND_UI_RANGE(ID_FFT_BANDS_64, ID_FFT_BANDS_16384, OnUpdateFftBands)

	ON_COMMAND(ID_FFT_WINDOW_SQUARED_SINE, OnFftWindowSquaredSine)
	ON_UPDATE_COMMAND_UI(ID_FFT_WINDOW_SQUARED_SINE, OnUpdateFftWindowSquaredSine)
	ON_COMMAND(ID_FFT_WINDOW_SINE, OnFftWindowSine)
	ON_UPDATE_COMMAND_UI(ID_FFT_WINDOW_SINE, OnUpdateFftWindowSine)
	ON_COMMAND(ID_FFT_WINDOW_HAMMING, OnFftWindowHamming)
	ON_UPDATE_COMMAND_UI(ID_FFT_WINDOW_HAMMING, OnUpdateFftWindowHamming)
	ON_COMMAND(ID_FFT_WINDOW_NUTTALL, OnFftWindowNuttall)
	ON_UPDATE_COMMAND_UI(ID_FFT_WINDOW_NUTTALL, OnUpdateFftWindowNuttall)
	ON_COMMAND(ID_VIEW_DECREASE_FFT_BANDS, OnViewDecreaseFftBands)
	ON_COMMAND(ID_VIEW_INCREASE_FFT_BANDS, OnViewIncreaseFftBands)
	//}}AFX_MSG_MAP
	ON_COMMAND(ID_VIEW_SS_ZOOMINVERT, OnViewZoomInVert)
	ON_UPDATE_COMMAND_UI(ID_VIEW_SS_ZOOMINVERT, OnUpdateViewZoomInVert)
	ON_COMMAND(ID_VIEW_SS_ZOOMOUTVERT, OnViewZoomOutVert)
	ON_UPDATE_COMMAND_UI(ID_VIEW_SS_ZOOMOUTVERT, OnUpdateViewZoomOutVert)
	ON_COMMAND(ID_VIEW_SS_ZOOMVERT_NORMAL, OnViewZoomvertNormal)
	ON_UPDATE_COMMAND_UI(ID_VIEW_SS_ZOOMVERT_NORMAL, OnUpdateViewZoomvertNormal)
	ON_MESSAGE(UWM_NOTIFY_VIEWS, &CWaveFftView::OnUwmNotifyViews)
	ON_WM_CONTEXTMENU()
END_MESSAGE_MAP()

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

SAMPLE_INDEX CWaveFftView::SampleToFftBaseSample(SAMPLE_INDEX sample) const
{
	sample += m_FftOrder;
	return sample & -m_FftSpacing;
}

// the base sample corresponds to the FFT center
SAMPLE_INDEX CWaveFftView::DisplaySampleToFftBaseSample(SAMPLE_INDEX sample) const
{
	sample += m_FftSpacing >> 1;
	return sample & -m_FftSpacing;
}

// display sample to FftColumn
long CWaveFftView::SampleToFftColumn(SAMPLE_INDEX sample) const
{
	return (sample + (m_FftSpacing >> 1)) / m_FftSpacing;
}

// to calculate the first column to be invalidated. The sample belongs to this column and possibly next
long CWaveFftView::SampleToFftColumnLowerBound(SAMPLE_INDEX sample) const
{
	if (sample < m_FftOrder)
	{
		return 0;
	}
	return (sample - m_FftOrder) / m_FftSpacing;
}

// to calculate the last column to be invalidated. The sample belongs to this column and possibly before it
long CWaveFftView::SampleToFftColumnUpperBound(SAMPLE_INDEX sample) const
{
	if (sample >= LONG_MAX - m_FftOrder)
	{
		return LONG_MAX / m_FftSpacing - 1;
	}
	return (sample + m_FftOrder) / m_FftSpacing;
}

// the display columns divided at m_FftSpacing/2
// this returns the left boundary of the band
SAMPLE_INDEX CWaveFftView::FftColumnToDisplaySample(long Column) const
{
	if (Column == 0)
	{
		return 0;
	}
	return Column * m_FftSpacing - m_FftSpacing / 2;
}

void CWaveFftView::InvalidateFftColumnRange(long first_column, long last_column)  // including last
{
	if (NULL == m_pFftResultArray)
	{
		return;
	}
	if (first_column < m_FirstFftColumn)
	{
		first_column = m_FirstFftColumn;
	}
	if (last_column >= m_FirstFftColumn + m_FftResultArrayWidth)
	{
		last_column = m_FirstFftColumn + m_FftResultArrayWidth - 1;
	}
	for (long i = first_column; i <= last_column; i++)
	{
		m_pFftResultArray[((m_IndexOfFftBegin + i + m_FftResultArrayWidth - m_FirstFftColumn) % m_FftResultArrayWidth) * m_FftResultArrayHeight] = 0;
	}
}

bool CWaveFftView::IsFftResultCalculated(SAMPLE_INDEX sample) const
{
	long FftColumn = SampleToFftColumn(sample);
	// see if it's within the array
	ASSERT(FftColumn >= m_FirstFftColumn && FftColumn < m_FirstFftColumn + m_FftResultArrayWidth);

	float * pFftColumn = m_pFftResultArray +
						m_FftResultArrayHeight * ((m_IndexOfFftBegin + FftColumn - m_FirstFftColumn) % m_FftResultArrayWidth);

	return pFftColumn[0] == 1;
}

float const * CWaveFftView::GetFftResult(SAMPLE_INDEX sample, unsigned channel)
{
	ThisDoc * pDoc = GetDocument();
	long FftColumn = SampleToFftColumn(sample);
	// see if it's within the array
	ASSERT(FftColumn >= m_FirstFftColumn && FftColumn < m_FirstFftColumn + m_FftResultArrayWidth);

	float * pFftColumn = m_pFftResultArray +
						m_FftResultArrayHeight * ((m_IndexOfFftBegin + FftColumn - m_FirstFftColumn) % m_FftResultArrayWidth);

	if (pFftColumn[0] == 1)
	{
		return pFftColumn + 1 + channel * m_FftOrder;
	}
	if (pFftColumn[0] == 127)
	{
		return NULL;
	}

	NUMBER_OF_CHANNELS nChannels = GetDocument()->WaveChannels();
	NUMBER_OF_SAMPLES FirstSampleRequired = FftColumn * m_FftSpacing - m_FftOrder;
	if (FirstSampleRequired < 0)
	{
		FirstSampleRequired = 0;
	}

	// calculate the FFT for this column
	if (FirstSampleRequired + m_FftOrder * 2 > GetDocument()->WaveFileSamples()
		|| FirstSampleRequired < 0)
	{
		if (0) TRACE("The required samples from %d to %d are out of the file\n",
					FirstSampleRequired, FirstSampleRequired + m_FftOrder);
		pFftColumn[0] = 0;   // mark as invalid
		// make all black
		return NULL;
	}

	float * pFftBuf = NULL;
	unsigned FFtBufIdx;
	ULONG_PTR FFtBufMask;
	for (FFtBufIdx = 0; ; FFtBufIdx = (FFtBufIdx + 1) % (sizeof(ULONG_PTR)*8))
	{
		ASSERT(~m_FftArrayBusyMask != 0);
		FFtBufMask = (ULONG_PTR)1 << FFtBufIdx;
		if (0 == (FFtBufMask & m_FftArrayBusyMask.Exchange_Or(FFtBufMask)))
		{
			break;
		}
	}
	if (!m_FftBuf[FFtBufIdx])
	{
		m_FftBuf[FFtBufIdx].Allocate(m_FftOrder * 2 + 2);
	}

	pFftBuf = m_FftBuf[FFtBufIdx];
	if (!pFftBuf)
	{
		return NULL;
	}

#ifdef _DEBUG
	m_NumberOfFftCalculated++;
#endif
	float * pRes = pFftColumn + 1;

	long const NeedSamples = m_FftOrder * 2;

	for (int ch = 0; ch < nChannels; ch++, pRes += m_FftOrder)
	{
		if (NeedSamples != pDoc->m_WavFile.ReadSamples(1 << ch, pDoc->m_WavFile.SampleToPosition(FirstSampleRequired),
														NeedSamples, pFftBuf, SampleTypeFloat32))
		{
			pFftColumn[0] = 0;
			m_FftArrayBusyMask &= ~FFtBufMask;
			return NULL;
		}

		for (int k = 0; k < NeedSamples; k++)
		{
			pFftBuf[k] *= m_pFftWindow[k];
		}

		FastFourierTransform(pFftBuf, reinterpret_cast<complex<float> *>(pFftBuf),
							m_FftOrder * 2);

		for (int i = 0, k = 0; i < m_FftOrder; i++, k += 2)
		{
			pRes[i] = float(pFftBuf[k] * pFftBuf[k] + pFftBuf[k + 1] * pFftBuf[k + 1]);
		}
	}

	m_FftArrayBusyMask &= ~FFtBufMask;
	pFftColumn[0] = 1;   // mark as valid
	return pFftColumn + 1 + channel * m_FftOrder;
}

void CWaveFftView::FillLogPalette(LOGPALETTE * pal, int nEntries)
{
	pal->palVersion = 0x300;
	int i;
	for (i = 0; i < 10; i++)
	{
		pal->palPalEntry[i].peFlags = PC_EXPLICIT;
		pal->palPalEntry[i].peRed = (BYTE)i;
		pal->palPalEntry[i].peGreen = 0;
		pal->palPalEntry[i].peBlue = 0;
	}

	for (unsigned j = 0; j < sizeof palette && i < nEntries; j += 3, i++)
	{
		pal->palPalEntry[i].peFlags = PC_NOCOLLAPSE;
		pal->palPalEntry[i].peRed = palette[j];
		pal->palPalEntry[i].peGreen = palette[j + 1];
		pal->palPalEntry[i].peBlue = palette[j + 2];
	}
	pal->palNumEntries = (WORD)i;
}

/////////////////////////////////////////////////////////////////////////////
// CWaveFftView drawing
namespace
{
struct FftGraphBand
{
	int y;  // bottom of band
	int NumDisplayRows;
	int FftBand;
	int NumBandsToSum;
};
}

struct DrawFftWorkItem : WorkerThreadPoolItem
{
	CWaveFftView * pView;
	long nCurrentFftColumn;
	int nChannel;
	unsigned char* pColBmp;
	int stride;
	int nColumns;
	FftGraphBand * pId;
	RGBQUAD const * pColor;	// palette
	double PowerLogToPalIndex;
	int  valid_top;
	int  valid_bottom;
};

bool CWaveFftView::CalculateFftColumnWorkitem(WorkerThreadPoolItem * pItem)
{
	DrawFftWorkItem * pDrawItem = static_cast<DrawFftWorkItem *>(pItem);
	pDrawItem->pView->CalculateFftColumn(pDrawItem);
	return true;
}

void CWaveFftView::CalculateFftColumn(DrawFftWorkItem * pDrawItem)
{
	// this will calculate the result for all channels
	GetFftResult(FftColumnToDisplaySample(pDrawItem->nCurrentFftColumn), 0);
}

bool CWaveFftView::FillChannelFftColumnWorkitem(WorkerThreadPoolItem * pItem)
{
	DrawFftWorkItem * pDrawItem = static_cast<DrawFftWorkItem *>(pItem);
	pDrawItem->pView->FillChannelFftColumn(pDrawItem);
	return true;
}

void CWaveFftView::FillChannelFftColumn(DrawFftWorkItem * pDrawItem)
{
	unsigned char* pColBmp = pDrawItem->pColBmp;
	int stride = pDrawItem->stride;
	int nColumns = pDrawItem->nColumns;
	double PowerLogToPalIndex = pDrawItem->PowerLogToPalIndex;
	int valid_top = pDrawItem->valid_top;
	int valid_bottom = pDrawItem->valid_bottom;

#ifdef _DEBUG
	m_NumberOfPixelJobs++;
	m_NumberOfColumnsSet += nColumns;
#endif
	// the result is already calculated and cached, no cost here
	float const *pData = GetFftResult(FftColumnToDisplaySample(pDrawItem->nCurrentFftColumn), pDrawItem->nChannel);

	FftGraphBand const * pId = pDrawItem->pId;
	for (int ff = 0; ff < m_FftOrder; ff++, pId++)
	{
		if (pId->NumDisplayRows == 0)
		{
			break;
		}
		if (pId->y >= valid_top
			&& pId->y + pId->NumDisplayRows <= valid_bottom)
		{
			// this band is in the area not to update
			continue;
		}
		unsigned char red = 0;
		unsigned char g = 0;
		unsigned char b = 0;
		if (pData != NULL)
		{
			ASSERT(pId->FftBand < m_FftOrder);
			ASSERT(pId->FftBand + pId->NumBandsToSum <= m_FftOrder);

			float sum = 0.;
			for (int f = pId->FftBand; f < pId->FftBand + pId->NumBandsToSum; f++)
			{
				sum += pData[f];
			}
			sum /= pId->NumBandsToSum;

			int PaletteIndex = 255;

			if (sum != 0.)
			{
				// max power= (32768 * m_FftOrder * 2) ^ 2
				PaletteIndex = int(PowerLogToPalIndex * -log(sum));

				if (PaletteIndex < 0)
				{
					PaletteIndex = 0;
				}
				else if (PaletteIndex > 255)
				{
					PaletteIndex = 255;
				}
			}

			RGBQUAD const * pColor = &pDrawItem->pColor[PaletteIndex];
			// set the color to pId->nNumOfRows rows
			red = pColor->rgbRed;
			g = pColor->rgbGreen;
			b = pColor->rgbBlue;
		}

		BYTE * pRgb = pColBmp + pId->y * stride;

		ASSERT(pId->y >= m_Heights.ch[pDrawItem->nChannel].clip_top);
		ASSERT(pId->y + pId->NumDisplayRows <= m_Heights.ch[pDrawItem->nChannel].clip_bottom);

#ifdef _DEBUG
		m_NumberOfPixelsSet += pId->NumDisplayRows * nColumns;
#endif
		for (int y = 0; y < pId->NumDisplayRows; y++, pRgb += stride)
		{
			// set the color to nColumns pixels across
			BYTE * pRowRgb = pRgb;
			for (int x = 0; x < nColumns; x++, pRowRgb += 3)
			{
				pRowRgb[0] = b;    // B
				pRowRgb[1] = g;    // G
				pRowRgb[2] = red;    // R
			}
		}
	}
}

bool CWaveFftView::FillFftColumnPaletteWorkitem(WorkerThreadPoolItem * pItem)
{
	DrawFftWorkItem * pDrawItem = static_cast<DrawFftWorkItem *>(pItem);
	pDrawItem->pView->FillFftColumnPalette(pDrawItem);
	return true;
}

void CWaveFftView::FillFftColumnPalette(DrawFftWorkItem * pDrawItem)
{
	unsigned char* pColBmp = pDrawItem->pColBmp;
	int stride = pDrawItem->stride;
	int nColumns = pDrawItem->nColumns;
	double PowerLogToPalIndex = pDrawItem->PowerLogToPalIndex;

	float const *pData = GetFftResult(FftColumnToDisplaySample(pDrawItem->nCurrentFftColumn), pDrawItem->nChannel);

	FftGraphBand const * pId = pDrawItem->pId;
#ifdef _DEBUG
	m_NumberOfPixelJobs++;
	m_NumberOfColumnsSet += nColumns;
#endif
	for (int ff = 0; ff < m_FftOrder; ff++, pId++)
	{
		if (pId->NumDisplayRows == 0)
		{
			break;
		}
		unsigned char ColorIndex = 0;
		if (pData != NULL)
		{
			float sum = 0.;
			for (int f = pId->FftBand; f < pId->FftBand + pId->NumBandsToSum; f++)
			{
				sum += pData[f];
			}
			sum /= pId->NumBandsToSum;

			int PaletteIndex = 127;

			if (sum != 0.)
			{
				PaletteIndex = int(PowerLogToPalIndex * - log(sum));

				if (PaletteIndex < 0)
				{
					PaletteIndex = 0;
				}
				else if (PaletteIndex > 127)
				{
					PaletteIndex = 127;
				}
			}

			PaletteIndex += 10;
		}
#ifdef _DEBUG
		m_NumberOfPixelsSet += pId->NumDisplayRows * nColumns;
#endif
		// set the color to pId->nNumOfRows rows
		BYTE * pPal = pColBmp + pId->y * stride;
		for (int y = 0; y < pId->NumDisplayRows; y++, pPal += stride)
		{
			// set the color to nColumns pixels across
			for (int x = 0; x < nColumns; x++)
			{
				pPal[x] = ColorIndex;
			}
		}
	}
}

void CWaveFftView::OnDraw(CPaintDC* pDC, CRgn * UpdateRgn)
{
	CWaveSoapFrontDoc* pDoc = GetDocument();
	if (! pDoc->m_WavFile.IsOpen())
	{
		return;
	}

	CRect r(pDC->m_ps.rcPaint);	// paint rectangle
	CRect cr;	// client rectangle
	GetClientRect( & cr);

	// limit right to the file area
	int FileEnd = SampleToXceil(pDoc->WaveFileSamples());

	if (r.right > FileEnd)
	{
		r.right = FileEnd;
	}

	double left = WindowXtoSample(r.left);		// (fractional) sample corresponding to r.left
	double right = WindowXtoSample(r.right);	// (fractional) sample corresponding to r.right

	if (left < 0) left = 0;

	if (r.left >= r.right)
	{
		return;
	}

	AllocateFftArray((SAMPLE_INDEX)WindowXtoSample(cr.left), (SAMPLE_INDEX)WindowXtoSample(cr.right));  // full width of the client area at least

	if (!m_FftWindowValid)
	{
		TRACE("Calculating FFT window\n");
		if (!m_pFftWindow)
		{
			m_pFftWindow.Allocate(m_FftOrder * 2);
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
				m_pFftWindow[w] = float((0.5 - 0.5 * cos(X))
										/ (m_FftOrder * 0.46518375880479441036888825236993));
				break;
			case WindowTypeHalfSine:
				// half sine
				m_pFftWindow[w] = float((0.707107 * sin (0.5 * X))
										/ (m_FftOrder * 0.34132424511039137114306807472343));
				break;
			case WindowTypeHamming:
				// Hamming window (sucks!!!)
				m_pFftWindow[w] = float((0.9 * (0.54 - 0.46 * cos (X)))
										/ (m_FftOrder * 0.44467411795859108283066893223012));
				break;

			case WindowTypeNuttall:
				// Nuttall window:
				// w(n) = 0.355768 � 0.487396*cos(2pn/N) + 0.144232*cos(4pn/N) � 0.012604*cos(6pn/N)
				m_pFftWindow[w] = float((0.355768 - 0.487396*cos(X) + 0.144232*cos(2 * X) - 0.012604*cos(3 * X))
										/ (m_FftOrder * 0.34132424511039137114306807472343));
				break;
			}
		}
		m_FftWindowValid = true;
	}

	void * pBits;
	bool bUsePalette;
	unsigned width = r.right - r.left;
	long height = cr.bottom - cr.top;
	unsigned stride = (width * 3 + 3) & ~3;
	unsigned BmpSize = stride * height;
	int BytesPerPixel = 3;
	double DbRange = 130. + 10. * log10(m_FftOrder / 512.);

	double PowerLogToPalIndex;

	struct BM
	{
		BITMAPINFO bmi;
		RGBQUAD MorebmiColors[256];
	} bmi = {sizeof (BITMAPINFOHEADER) };

	bmi.bmi.bmiHeader.biSize = sizeof (BITMAPINFOHEADER);
	bmi.bmi.bmiHeader.biWidth = r.right - r.left;
	bmi.bmi.bmiHeader.biHeight = -height;
	bmi.bmi.bmiHeader.biPlanes = 1;
	bmi.bmi.bmiHeader.biCompression = BI_RGB;
	bmi.bmi.bmiHeader.biSizeImage = 0;
	bmi.bmi.bmiHeader.biXPelsPerMeter = 0;
	bmi.bmi.bmiHeader.biYPelsPerMeter = 0;
	bmi.bmi.bmiHeader.biClrUsed = 0;
	bmi.bmi.bmiHeader.biClrImportant = 0;

	CPushDcPalette OldPalette(pDC, NULL);

	if ((pDC->GetDeviceCaps(RASTERCAPS) & RC_PALETTE)
		&& 8 == pDC->GetDeviceCaps(BITSPIXEL))
	{
		BytesPerPixel = 1;
		bUsePalette = true;
		bmi.bmi.bmiHeader.biBitCount = 8;

		int i;
		PALETTEENTRY SysPalette[256];
		GetSystemPaletteEntries(*pDC, 0, 256, SysPalette);
		for (i = 0; i < 10; i++)
		{
			bmi.bmi.bmiColors[i].rgbReserved = 0;
			bmi.bmi.bmiColors[i].rgbRed = SysPalette[i].peRed;
			bmi.bmi.bmiColors[i].rgbGreen = SysPalette[i].peGreen;
			bmi.bmi.bmiColors[i].rgbBlue = SysPalette[i].peBlue;
		}
		for (int j = 0; j < sizeof palette && i < 255; j += 3, i++)
		{
			bmi.bmi.bmiColors[i].rgbReserved = 0;
			bmi.bmi.bmiColors[i].rgbRed = palette[j];
			bmi.bmi.bmiColors[i].rgbGreen = palette[j + 1];
			bmi.bmi.bmiColors[i].rgbBlue = palette[j + 2];
		}
		for ( ; i < 256; i++)
		{
			bmi.bmi.bmiColors[i].rgbReserved = 0;
			bmi.bmi.bmiColors[i].rgbRed = SysPalette[i].peRed;
			bmi.bmi.bmiColors[i].rgbGreen = SysPalette[i].peGreen;
			bmi.bmi.bmiColors[i].rgbBlue = SysPalette[i].peBlue;
		}

		bmi.bmi.bmiHeader.biClrUsed = i;
		stride = (width + 3) & ~3;
		BmpSize = stride * height;
		OldPalette.PushPalette(GetApp()->GetPalette(), FALSE);

		PowerLogToPalIndex = 127 / (DbRange / 10. * log(10.));
	}
	else
	{
		BytesPerPixel = 3;
		bUsePalette = false;
		bmi.bmi.bmiHeader.biBitCount = 24;
		// fill interpolated palette for 256 RGB colors
		bmi.MorebmiColors[0].rgbRed = 255;
		bmi.MorebmiColors[0].rgbGreen = 255;
		bmi.MorebmiColors[0].rgbBlue = 255;
		unsigned char const * pPal = palette;
		for (int i = 0; i < 256-2; i += 2, pPal+=3)
		{
			bmi.MorebmiColors[i + 2].rgbRed = pPal[0];
			bmi.MorebmiColors[i + 2].rgbGreen = pPal[1];
			bmi.MorebmiColors[i + 2].rgbBlue = pPal[2];
			bmi.MorebmiColors[i + 1].rgbRed = (bmi.MorebmiColors[i].rgbRed + bmi.MorebmiColors[i + 2].rgbRed) /2;
			bmi.MorebmiColors[i + 1].rgbGreen = (bmi.MorebmiColors[i].rgbGreen + bmi.MorebmiColors[i + 2].rgbGreen) / 2;
			bmi.MorebmiColors[i + 1].rgbBlue = (bmi.MorebmiColors[i].rgbBlue + bmi.MorebmiColors[i + 2].rgbBlue) / 2;
		}
		bmi.MorebmiColors[255].rgbRed = 0;
		bmi.MorebmiColors[255].rgbGreen = 0;
		bmi.MorebmiColors[255].rgbBlue = 0;
		PowerLogToPalIndex = 255 / (DbRange / 10. * log(10.));
	}

	// log of FFT slot power to map to zero palette index
	double const PowerOffset = log(2. * m_FftOrder) * 2.;

	CBitmap hbm;
	hbm.Attach(CreateDIBSection(pDC->GetSafeHdc(), & bmi.bmi, DIB_RGB_COLORS,
								& pBits, NULL, 0));

	if (HGDIOBJ(hbm) == NULL)
	{
		return;
	}

	LPBYTE pBmp = LPBYTE(pBits);

	memset(pBmp, 0, BmpSize);

	// fill the array

	NUMBER_OF_CHANNELS nChannels = pDoc->WaveChannels();
	// find offset in the FFT result array for 'left' point
	// and how many columns to fill with this color
	ASSERT(m_FftSpacing >= m_HorizontalScale);

	// find vertical offset in the result array and how many
	// rows to fill with this color
	// build an array of vertical offsets for each band and for each channel

	ATL::CHeapPtr<FftGraphBand> pIdArray[MAX_NUMBER_OF_CHANNELS];
	RgnData UpdateRgnData[MAX_NUMBER_OF_CHANNELS];

	CRgn ChannelRgn;
	ChannelRgn.CreateRectRgn(0, 0, 0, 0);

	for (int ch = 0; ch < nChannels; ch++)
	{
		int LastRow = 0;
		int k;
		int top = m_Heights.ch[ch].clip_top;
		int bottom = m_Heights.ch[ch].clip_bottom;
		ChannelRgn.SetRectRgn(r.left, top, r.right, bottom);

		int ScaledHeight = m_Heights.ch[ch].bottom - m_Heights.ch[ch].top;
		int OffsetPixels = 0;

		if (! m_Heights.ChannelMinimized(ch))
		{
			ScaledHeight = int(m_Heights.NominalChannelHeight * m_VerticalScale);
			OffsetPixels = int(ScaledHeight * m_FirstbandVisible/ m_FftOrder);
		}

		if ( !pIdArray[ch].Allocate(bottom - top + 1))
		{
			return;          // Allocate is non-throwing
		}

		// get the update region for the channel
		ChannelRgn.CombineRgn(&ChannelRgn, UpdateRgn, RGN_AND);

		if (!UpdateRgnData[ch].GetRegionData(&ChannelRgn))
		{
			return;
		}

		std::sort(UpdateRgnData[ch].begin(), UpdateRgnData[ch].end(), rect_less);
		// fill the array
		int y = bottom;
		FftGraphBand * pId = pIdArray[ch];
		for (k = 0, LastRow = 0; k < m_FftOrder; k++, pId++)
		{
			ASSERT(k < bottom - top + 1);
			if (y <= top)
			{
				pId->NumBandsToSum = 0;
				pId->NumDisplayRows = 0;
				break;
			}

			int FirstFftSample = (bottom - y + OffsetPixels) * m_FftOrder / ScaledHeight;

			pId->FftBand = FirstFftSample;
			pId->NumDisplayRows = 0;
			// see if the Fft band will take multiple display rows, or a single display row will take multiple bands

			while (y > top)
			{
				y--;
				pId->NumDisplayRows++;
				int NextFftSample = (bottom - y + OffsetPixels) * m_FftOrder / ScaledHeight;

				if (NextFftSample >= m_FftOrder)
				{
					pId->NumBandsToSum = m_FftOrder - FirstFftSample;
					y = top;
					break;
				}

				if (NextFftSample > FirstFftSample)
				{
					pId->NumBandsToSum = NextFftSample - FirstFftSample;
					break;
				}
			}

			pId->y = y;  // top of display band

			if (0 && r.left == -1 && k < 20) TRACE("FftView: ch:%d y=%d-%d fft band %d-%d\n", ch, pId->y, pId->y + pId->NumDisplayRows -1,
													pId->FftBand, pId->FftBand + pId->NumBandsToSum -1);
		}
	}

	DrawFftWorkItem wi[128];	// 32 items per job
	WorkerThreadPoolJob ThreadPoolJob[4];
	for (int a = 0; a < countof(wi); a++)
	{
		wi[a].pView = this;
		wi[a].PowerLogToPalIndex = PowerLogToPalIndex;
		wi[a].stride = stride;
		ThreadPoolJob[a / (countof(wi) / countof(ThreadPoolJob))].m_DoneItems.InsertTail(&wi[a]);
	}
	int JobsOutstanding = 0;
	int NextJob = 0;

	// first, pre-calculate all FFT, one per work item
	for(int col = r.left; col < r.right; )
	{
		// 'left' is the first sample index in the area to redraw
		int nColumns;
		long nCurrentFftColumn;
		int CurrentColumnRight;
		if (m_FftSpacing <= m_HorizontalScale)
		{
			// each FFT takes one column of display
			nCurrentFftColumn = int(WindowXtoSample(col) / m_FftSpacing);
			CurrentColumnRight = col + 1;
		}
		else
		{
			nCurrentFftColumn = long(0.5 + WindowXtoSample(col) / m_FftSpacing);
			CurrentColumnRight = (int)SampleToX(nCurrentFftColumn * m_FftSpacing + m_FftSpacing/2);
		}

		ASSERT(nCurrentFftColumn >= 0);

		if (CurrentColumnRight > r.right)
		{
			CurrentColumnRight = r.right;
		}

		nColumns = CurrentColumnRight - col;
		col = CurrentColumnRight;

		if (IsFftResultCalculated(FftColumnToDisplaySample(nCurrentFftColumn)))
		{
			continue;
		}
		if (JobsOutstanding == countof(ThreadPoolJob))
		{
			ThreadPoolJob[NextJob].WaitForCompletion();
			JobsOutstanding--;
		}

		DrawFftWorkItem * pItem = static_cast<DrawFftWorkItem *>(ThreadPoolJob[NextJob].m_DoneItems.RemoveHead());
		pItem->nCurrentFftColumn = nCurrentFftColumn;

		pItem->Proc = CalculateFftColumnWorkitem;

		ThreadPoolJob[NextJob].AddWorkitem(pItem);

		if (ThreadPoolJob[NextJob].m_DoneItems.IsEmpty())
		{
			GetApp()->AddWorkerThreadPoolJob(&ThreadPoolJob[NextJob]);
			JobsOutstanding++;
			NextJob = (NextJob + 1) % countof(ThreadPoolJob);
		}
	}

	// see if we have a job we didn't post yet. Need to flush it before starting to fill the bitmap
	if (JobsOutstanding < countof(ThreadPoolJob) && ThreadPoolJob[NextJob].m_WorkitemsPending)
	{
		GetApp()->AddWorkerThreadPoolJob(&ThreadPoolJob[NextJob]);
		JobsOutstanding++;
		NextJob = (NextJob + 1) & (countof(ThreadPoolJob) - 1);
	}
	for(int col = r.left; col < r.right; )
	{
		unsigned char * pColBmp = pBmp + (col - r.left) * BytesPerPixel;
		// 'left' is the first sample index in the area to redraw
		int nColumns;
		long nCurrentFftColumn;
		int CurrentColumnRight;
		if (m_FftSpacing <= m_HorizontalScale)
		{
			// each FFT takes one column of display
			nCurrentFftColumn = int(WindowXtoSample(col) / m_FftSpacing);
			CurrentColumnRight = col + 1;
		}
		else
		{
			nCurrentFftColumn = long(0.5 + WindowXtoSample(col) / m_FftSpacing);
			CurrentColumnRight = (int)SampleToX(nCurrentFftColumn * m_FftSpacing + m_FftSpacing/2);
		}

		ASSERT(nCurrentFftColumn >= 0);

		if (CurrentColumnRight > r.right)
		{
			CurrentColumnRight = r.right;
		}

		nColumns = CurrentColumnRight - col;
		col = CurrentColumnRight;
		for (int ch = 0; ch < nChannels; ch++)
		{
			// find out the Y range to update
			int clip_top = m_Heights.ch[ch].clip_top;
			int clip_bottom = m_Heights.ch[ch].clip_bottom;
			int valid_top = clip_top;
			int valid_bottom = clip_bottom;
			CRect rr(col, valid_top, col, valid_bottom);

			RECT * begin = UpdateRgnData[ch].begin();
			RECT * end = UpdateRgnData[ch].end();
			// find the first rectangle not crossing the target
			end = std::lower_bound(begin, end, rr, rect_left_less);
			rr.left = col - nColumns;

			for (; begin < end; begin++)
			{
				if (begin->right < rr.left)
				{
					continue;
				}
				if (begin->top <= valid_top)
				{
					if (valid_top < begin->bottom)
					{
						valid_top = begin->bottom;
					}
				}
				else if (begin->bottom >= valid_bottom)
				{
					if (valid_bottom > begin->top)
					{
						valid_bottom = begin->top;
					}
				}
				else if (begin->bottom - valid_top < valid_bottom - begin->top)
				{
					valid_top = begin->bottom;
				}
				else
				{
					valid_bottom = begin->top;
				}
			}

			if (valid_top == clip_top
				&& valid_bottom == clip_bottom)
			{
				continue;
			}

			if (JobsOutstanding == countof(ThreadPoolJob))
			{
				ThreadPoolJob[NextJob].WaitForCompletion();
				JobsOutstanding--;
			}
			DrawFftWorkItem * pItem = static_cast<DrawFftWorkItem *>(ThreadPoolJob[NextJob].m_DoneItems.RemoveHead());
			pItem->nCurrentFftColumn = nCurrentFftColumn;
			pItem->nColumns = nColumns;
			pItem->pColBmp = pColBmp;
			pItem->valid_top = valid_top;
			pItem->valid_bottom = valid_bottom;

			if (bUsePalette)
			{
				pItem->Proc = FillFftColumnPaletteWorkitem;
			}
			else
			{
				pItem->pColor = bmi.MorebmiColors;
				pItem->Proc = FillChannelFftColumnWorkitem;
			}
			pItem->nChannel = ch;
			pItem->pId = pIdArray[ch];

			ThreadPoolJob[NextJob].AddWorkitem(pItem);

			if (ThreadPoolJob[NextJob].m_DoneItems.IsEmpty())
			{
				GetApp()->AddWorkerThreadPoolJob(&ThreadPoolJob[NextJob]);
				JobsOutstanding++;
				NextJob = (NextJob + 1) & (countof(ThreadPoolJob) - 1);
			}
		}
	}
	for (int ch = 0; ch + 1 != nChannels; ch++)
	{
		// draw a gray separator line
		BYTE * pRgb = pBmp + m_Heights.ch[ch].clip_bottom * stride;
		if (!bUsePalette)
		{
			for (int x = r.left; x < r.right; x++, pRgb += 3)
			{
				//ASSERT(pRgb >= pBmp && pRgb + 3 <= pBmp + BmpSize);
				pRgb[0] = 0x80;    // B
				pRgb[1] = 0x80;    // G
				pRgb[2] = 0x80;    // R
			}
		}
		else
		{
			for (int x = r.left; x < r.right; x++, pRgb ++)
			{
				// set the color to nColumns pixels across
				//ASSERT(pPal >= pBmp && pPal+x < pBmp + BmpSize);
				*pRgb = 0x80;
			}
		}
	}

	// see if we have a job we didn't post yet
	if (JobsOutstanding < countof(ThreadPoolJob) && ThreadPoolJob[NextJob].m_WorkitemsPending)
	{
		GetApp()->AddWorkerThreadPoolJob(&ThreadPoolJob[NextJob]);
		JobsOutstanding++;
		NextJob = (NextJob + 1) & (countof(ThreadPoolJob) - 1);
	}
	// wait for all jobs complete.

	for (NextJob = (NextJob - JobsOutstanding) & (countof(ThreadPoolJob) - 1); JobsOutstanding != 0; JobsOutstanding--)
	{
		ThreadPoolJob[NextJob].WaitForCompletion();
		NextJob = (NextJob + 1) & (countof(ThreadPoolJob) - 1);
	}
	// draw markers as inverted dashed lines into the bitmap
	SAMPLE_INDEX_Vector markers;
	pDoc->m_WavFile.GetSortedMarkers(markers, FALSE);     // unique positions

	long prev_x = -1;
	for (SAMPLE_INDEX_Vector::const_iterator i = markers.begin();
		i != markers.end(); i++)
	{
		long x = SampleToX( *i);
		if (x == prev_x || x < r.left || x >= r.right)
		{
			continue;
		}

		prev_x = x;

		long FftOffsetPixels = long(m_FirstbandVisible * int(m_VerticalScale * m_Heights.NominalChannelHeight) / m_FftOrder);
		for (int ch = 0; ch < nChannels; ch++)
		{
			int BrushOffset = 0;
			if ( ! m_Heights.ChannelMinimized(ch))
			{
				BrushOffset = FftOffsetPixels;
			}
			unsigned char * pColBmp = pBmp + (x - r.left) * BytesPerPixel + stride * m_Heights.ch[ch].clip_top;
			if ( ! bUsePalette)
			{
				for (int y = m_Heights.ch[ch].clip_top; y < m_Heights.ch[ch].clip_bottom; y++, pColBmp += stride)
				{
					if ((y - BrushOffset) & 2)
					{
						pColBmp[0] = ~pColBmp[0];
						pColBmp[1] = ~pColBmp[1];
						pColBmp[2] = ~pColBmp[2];
					}
				}
			}
			else
			{
				for (int y = m_Heights.ch[ch].clip_top; y < m_Heights.ch[ch].clip_bottom; y++, pColBmp += stride)
				{
					if ((y + BrushOffset) & 2)
					{
						// colors go from 10 to 127+10 (128 colors)
						// reverse them
						pColBmp[0] = 127 + 20 - pColBmp[0];
					}
				}
			}
		}
		// draw marker every four pixels
		pDC->PatBlt(x, cr.top, 1, cr.bottom - cr.top, PATINVERT);
	}


	// copy bitmap to output window
	SetDIBitsToDevice(pDC->GetSafeHdc(), r.left, cr.top,
					width, height,
					0, 0, 0, height, pBits, & bmi.bmi,
					DIB_RGB_COLORS);

	CGdiObjectSave OldFont(pDC, pDC->SelectStockObject(ANSI_VAR_FONT));

	CThisApp * pApp = GetApp();
	pDC->SetBkColor(pApp->m_WaveBackground);
	pDC->SetTextColor(0xFFFFFF ^ pApp->m_WaveBackground);
	pDC->SetBkMode(OPAQUE);

	CWaveFile::InstanceDataWav * pInst = pDoc->m_WavFile.GetInstanceData();

	for (CuePointVectorIterator i = pInst->m_CuePoints.begin();
		i != pInst->m_CuePoints.end(); i++)
	{
		long x = SampleToX(i->dwSampleOffset);

		// draw text
		if (x < r.right)
		{
			LPCTSTR txt = pInst->GetCueText(i->CuePointID);
			if (NULL != txt)
			{
				int count = (int)_tcslen(txt);
				CPoint size = pDC->GetTextExtent(txt, count);
				if (x + size.x > r.left)
				{
					pDC->DrawText(txt, count, CRect(x, cr.top, x + size.x, cr.top + size.y),
								DT_LEFT | DT_NOPREFIX | DT_SINGLELINE | DT_TOP);
				}
			}
		}
	}

	GdiFlush(); // make sure bitmap is drawn before deleting it (NT only)
	// free resources
}

void CWaveFftView::AllocateFftArray(SAMPLE_INDEX SampleLeft, SAMPLE_INDEX SampleRight)
{
	CWaveSoapFrontDoc * pDoc = GetDocument();
	CRect r;
	int NumberOfFftPoints;

	int NewFftSpacing = (int)m_HorizontalScale;

	if (m_FftOrder / 4 > m_HorizontalScale)
	{
		NewFftSpacing = m_FftOrder / 4;
	}

	long FftColumnLeft = SampleLeft / NewFftSpacing;
	long FftColumnRight = (SampleRight + m_FftOrder) / NewFftSpacing;	// inclusive range

	NumberOfFftPoints = FftColumnRight + 1 - FftColumnLeft;

	// for each FFT set we keep a byte to mark it valid or invalid
	int NewFftArrayHeight = m_FftOrder * pDoc->WaveChannels() + 1;

	unsigned NecessaryArraySize =
		NumberOfFftPoints * NewFftArrayHeight;

	if (m_AllocatedFftBufSize < m_FftOrder * 2 + 2)
	{
		for (unsigned i = 0; i < countof(m_FftBuf); i++)
		{
			m_FftBuf[i].Free();
		}
	}
	m_AllocatedFftBufSize = m_FftOrder * 2 + 2;

	if (NULL != m_pFftResultArray
		&& m_FftResultArrayHeight == NewFftArrayHeight
		&& m_FftArraySize >= NecessaryArraySize
		&& NewFftSpacing == m_FftSpacing)
	{
		// the existing array can still be used
		// adjust it so that the result between SampleLeft and SampleRight will be loaded without modification
		long LastFftColumn = m_FirstFftColumn + m_FftResultArrayWidth - 1;
		if (FftColumnLeft < m_FirstFftColumn)
		{
			// see which columns before first need to be invalidated
			long ColumnsToShift = m_FirstFftColumn - FftColumnLeft;
			if (ColumnsToShift >= m_FftResultArrayWidth)
			{
				// invalidate all columns
				m_IndexOfFftBegin = 0;
				m_FirstFftColumn = FftColumnLeft;
				InvalidateFftColumnRange(0, LONG_MAX);
			}
			else
			{
				m_IndexOfFftBegin = (m_IndexOfFftBegin + m_FftResultArrayWidth - ColumnsToShift) % m_FftResultArrayWidth;
				m_FirstFftColumn = FftColumnLeft;
				InvalidateFftColumnRange(m_FirstFftColumn, m_FirstFftColumn + ColumnsToShift - 1);
			}
			ASSERT(FftColumnRight <= m_FirstFftColumn + m_FftResultArrayWidth - 1);
		}
		else if (FftColumnRight > LastFftColumn)
		{
			// see which columns after last need to be invalidated
			long ColumnsToShift = FftColumnRight - LastFftColumn;
			if (ColumnsToShift >= m_FftResultArrayWidth)
			{
				// invalidate all columns
				InvalidateFftColumnRange(0, LONG_MAX);
				m_IndexOfFftBegin = 0;
				m_FirstFftColumn = FftColumnLeft;
			}
			else
			{
				m_IndexOfFftBegin = (m_IndexOfFftBegin + ColumnsToShift) % m_FftResultArrayWidth;
				m_FirstFftColumn += ColumnsToShift;
				InvalidateFftColumnRange(m_FirstFftColumn + m_FftResultArrayWidth - ColumnsToShift, m_FirstFftColumn + m_FftResultArrayWidth - 1);
			}
			ASSERT(FftColumnLeft >= m_FirstFftColumn);
		}
		return;
	}

	float * pOldArray = m_pFftResultArray;
	m_pFftResultArray = NULL;
	if (m_FftResultArrayHeight != NewFftArrayHeight)
	{
		// no use in the old data
		delete[] pOldArray;
		pOldArray = NULL;
	}
	try
	{
		NecessaryArraySize = NecessaryArraySize * 3 / 2;	// add 50%
		m_pFftResultArray = new float[NecessaryArraySize];
	}
	catch (std::bad_alloc&)
	{
		delete[] pOldArray;
		return;
	}

	TRACE("New FFT array allocated, height=%d, FFT spacing=%d\n",
		NewFftArrayHeight, NewFftSpacing);

	float * pTmp = m_pFftResultArray;
	int i;

	for (i = 0; i < NumberOfFftPoints; i++, pTmp += NewFftArrayHeight)
	{
		pTmp[0] = 0;    // invalidate

		if (pOldArray == NULL)
		{
			continue;
		}
		SAMPLE_INDEX NewBaseSample = (FftColumnLeft + i) * NewFftSpacing;
		if (NewBaseSample %  m_FftSpacing != 0)
		{
			continue;
		}
		long OldFftColumn = NewBaseSample / m_FftSpacing;

		if (OldFftColumn < m_FirstFftColumn
			|| OldFftColumn >= m_FirstFftColumn + m_FftResultArrayWidth)
		{
			continue;
		}

		float * p = pOldArray + (m_IndexOfFftBegin + OldFftColumn - m_FirstFftColumn) % m_FftResultArrayWidth * m_FftResultArrayHeight;
		ASSERT(p >= pOldArray && p + m_FftResultArrayHeight <= pOldArray + m_FftArraySize);
		if (p[0] == 1)
		{
			memcpy(pTmp, p, NewFftArrayHeight * sizeof *p);
		}
	}

	m_FftArraySize = NecessaryArraySize;
	m_FirstFftColumn = FftColumnLeft;

	m_IndexOfFftBegin = 0;
	delete[] pOldArray;

	m_FftSpacing = NewFftSpacing;
	m_FftResultArrayHeight = NewFftArrayHeight;
	m_FftResultArrayWidth = NumberOfFftPoints;

}


/////////////////////////////////////////////////////////////////////////////
// CWaveFftView diagnostics

#ifdef _DEBUG
void CWaveFftView::AssertValid() const
{
	BaseClass::AssertValid();
}

void CWaveFftView::Dump(CDumpContext& dc) const
{
	BaseClass::Dump(dc);
}

#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CWaveFftView message handlers

void CWaveFftView::OnInitialUpdate()
{
	BaseClass::OnInitialUpdate();
	NotifySiblingViews(FftBandsChanged, &m_FftOrder);
	NotifySiblingViews(FftWindowChanged, &m_FftWindowType);
}

void CWaveFftView::OnPaint()
{
#ifdef _DEBUG
	if (m_DrawingBeginTime == 0)
	{
		m_DrawingBeginTime = GetTickCount();
	}
#endif
	CRgn UpdRgn;
	CRgn InvalidRgn;
	CRgn RectToDraw;
	CRect r;
	UpdRgn.CreateRectRgn(0, 0, 0, 0);
	if (ERROR != GetUpdateRgn( & UpdRgn, FALSE))
	{
		UpdRgn.GetRgnBox( & r);
		//TRACE("Update region width=%d\n", r.Width());
		if (r.Width() > MaxDrawColumnPerOnDraw)
		{
			r.right = r.left + MaxDrawColumnPerOnDraw;

			RectToDraw.CreateRectRgnIndirect( & r);
			// init the handle
			InvalidRgn.CreateRectRgn(0, 0, 0, 0);
			// subtract the draw rectangle from the remaining update region
			InvalidRgn.CombineRgn( & UpdRgn, & RectToDraw, RGN_DIFF);
			// limit the update region with the draw rectangle
			UpdRgn.CombineRgn( & UpdRgn, & RectToDraw, RGN_AND);
			ValidateRect(NULL);        // remove the old update region
			InvalidateRgn( & UpdRgn, TRUE);
		}
	}
	else
	{
		TRACE("No update region !\n");
	}
	// do regular paint on the smaller region
	{
		CPaintDC dc(this);
		OnDraw(&dc, &UpdRgn);

		if (m_PlaybackCursorDrawn)
		{
			DrawPlaybackCursor(&dc, m_PlaybackCursorDrawnSamplePos, m_PlaybackCursorChannel);
		}
		RedrawSelectionRect(&dc, 0, 0, 0, m_PrevSelectionStart, m_PrevSelectionEnd, m_PrevSelectedChannel);
	}
	if (NULL != HGDIOBJ(InvalidRgn))
	{
		// invalidate the remainder of the update region
		InvalidateRgn( & InvalidRgn, FALSE);
	}
#ifdef _DEBUG
	else
	{
		if (m_NumberOfColumnsSet) TRACE(L"FFT OnPaint: %d FFT calculated, %d pixels set for %d columns (%d per column), %d jobs in %d ms\n",
										LONG(m_NumberOfFftCalculated), LONG(m_NumberOfPixelsSet), LONG(m_NumberOfColumnsSet),
										LONG(m_NumberOfPixelsSet) / LONG(m_NumberOfColumnsSet),
										LONG(m_NumberOfPixelJobs), GetTickCount() - m_DrawingBeginTime);
		m_NumberOfFftCalculated = 0;
		m_NumberOfPixelsSet = 0;
		m_NumberOfColumnsSet = 0;
		m_NumberOfPixelJobs = 0;
		m_DrawingBeginTime = 0;
	}
#endif
}

BOOL CWaveFftView::OnEraseBkgnd(CDC* pDC)
{
// draw checkered background outside wave area
	CWaveSoapFrontDoc * pDoc = GetDocument();

	CRect r;
	GetClientRect( & r);
	int FileEnd = SampleToXceil(pDoc->WaveFileSamples());

	CRect ClipRect;
	pDC->GetClipBox(ClipRect);

	if (FileEnd < ClipRect.right)
	{
		try {
			CBitmap bmp;
			static const WORD pattern[] =
			{
				0x5555,   // aligned to WORD
				0xAAAA,
				0x5555,
				0xAAAA,
				0x5555,
				0xAAAA,
				0x5555,
				0xAAAA,
				0x5555,
			};

			bmp.CreateBitmap(8, 8, 1, 1, pattern + FileEnd % 2);
			CBrush GrayBrush( & bmp);

			CRect gr = ClipRect;
			gr.left = FileEnd;

			pDC->FillRect(gr, & GrayBrush);
		}
		catch (CResourceException * e)
		{
			TRACE("CResourceException\n");
			e->Delete();
		}
	}
	return TRUE;
}

BOOL CWaveFftView::PreCreateWindow(CREATESTRUCT& cs)
{
	cs.lpszClass = AfxRegisterWndClass(CS_VREDRAW | CS_DBLCLKS, NULL,
										NULL, NULL);

	return BaseClass::PreCreateWindow(cs);
}

void CWaveFftView::OnViewZoomInVert()
{
	if (m_VerticalScale < 1024.)
	{
		SetVerticalScale(m_VerticalScale * sqrt(2.));
	}
}

void CWaveFftView::OnViewZoomOutVert()
{
	if (m_VerticalScale > 1.)
	{
		double scale = m_VerticalScale * sqrt(0.5);
		if (scale < 1.001)    // compensate any error
		{
			scale = 1.;
		}
		SetVerticalScale(scale);
	}
}

void CWaveFftView::SetVerticalScale(double NewVerticalScale)
{
	NotifySiblingViews(FftVerticalScaleChanged, &NewVerticalScale);
}


void CWaveFftView::OnViewZoomvertNormal()
{
	if (m_VerticalScale != 1.)
	{
		SetVerticalScale(1.);
	}
}

void CWaveFftView::OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint)
{
//    CWaveSoapFrontDoc * pDoc = GetDocument();
	if (lHint == CWaveSoapFrontDoc::UpdateSoundChanged
		&& NULL != pHint
		&& NULL != m_pFftResultArray)
	{
		ASSERT(SAMPLE_INDEX(-1L) < SAMPLE_INDEX(0));

		CSoundUpdateInfo * pInfo = static_cast<CSoundUpdateInfo *>(pHint);

		// calculate update boundaries
		SAMPLE_INDEX left = pInfo->m_Begin;
		SAMPLE_INDEX right = pInfo->m_End;

		if (pInfo->m_NewLength != -1)
		{
			m_WaveBuffer.Invalidate(); // invalidate the data in draw buffer
			// length changed, set new extents and caret position

			Invalidate();
		}
		else
		{
			// TODO: invalidate only if in the range
			m_WaveBuffer.Invalidate(); // invalidate the data in draw buffer
		}

		// find out which samples are affected

		InvalidateFftColumnRange(SampleToFftColumnLowerBound(left), SampleToFftColumnUpperBound(right));

		// process length change
		if (pInfo->m_NewLength != -1)
		{
			InvalidateFftColumnRange(SampleToFftColumnLowerBound(pInfo->m_NewLength), SampleToFftColumnUpperBound(LONG_MAX));
		}

		CRect r;
		GetClientRect(r);

		CRect r1(r);
		// calculate update boundaries
		r1.left = SampleToX(SampleToFftColumnLowerBound(left) * m_FftSpacing - m_FftSpacing / 2);
		r1.right = SampleToXceil(SampleToFftColumnUpperBound(right) * m_FftSpacing + m_FftSpacing / 2) + 2;

		if (r1.left != r1.right
			// limit the rectangles with the window boundaries
			&& r1.left < r.right
			&& r1.right > r.left)
		{
			// non-empty, in the client
			if (r1.left < r.left)
			{
				r1.left = r.left;
			}
			if(r1.right > r.right)
			{
				r1.right = r.right;
			}
			//if (TRACE_UPDATE) TRACE("OnUpdate: SoundChanged: Invalidating from %d to %d\n", r1.left, r1.right);
			InvalidateRect(& r1, TRUE);
		}
	}
	else if (lHint == CWaveSoapFrontDoc::UpdateSelectionChanged
			&& NULL != pHint)
	{
		CSelectionUpdateInfo * pInfo =
			dynamic_cast<CSelectionUpdateInfo *>(pHint);
		if (NULL == pInfo)
		{
			BaseClass::OnUpdate(pSender, lHint, pHint);
			return;
		}

		AdjustCaretVisibility(pInfo->CaretPos, pInfo->OldCaretPos, pInfo->Flags);
		pInfo->Flags = SetSelection_DontAdjustView;

		ShowSelectionRect();
		CreateAndShowCaret();
		return; // don't call Wave view OnUpdate in this case
		// we don't want to invalidate
	}
	else
	{
		BaseClass::OnUpdate(pSender, lHint, pHint);
	}
}

void CWaveFftView::OnSetBands(int order)
{
	int NewBands = 1 << order;
	if (NewBands != m_FftOrder)
	{
		m_FirstbandVisible = m_FirstbandVisible * NewBands / m_FftOrder;
		m_FftOrder = NewBands;
		GetApp()->m_FftBandsOrder = order;

		delete[] m_pFftResultArray;
		m_pFftResultArray = NULL;

		m_FftWindowValid = false;
		m_pFftWindow.Free();

		Invalidate();
		NotifySiblingViews(FftBandsChanged, &m_FftOrder);
	}
}

void CWaveFftView::OnSetWindowType(int window)
{
	if (window != m_FftWindowType)
	{
		m_FftWindowType = window;
		GetApp()->m_FftWindowType = window;

		InvalidateFftColumnRange(0, LONG_MAX);

		m_FftWindowValid = false;

		Invalidate();
		NotifySiblingViews(FftWindowChanged, &m_FftWindowType);           // update spectrum section, too
	}
}

void CWaveFftView::OnFftBands(UINT id)
{
	OnSetBands(5 + (id - ID_FFT_BANDS_32));
}

void CWaveFftView::OnUpdateFftBands(CCmdUI* pCmdUI)
{
	pCmdUI->SetRadio(m_FftOrder == (32 << (pCmdUI->m_nID - ID_FFT_BANDS_32)));
}

// return client hit test code. 'p' is in client coordinates
DWORD CWaveFftView::ClientHitTest(CPoint p) const
{
	ThisDoc * pDoc = GetDocument();
	DWORD result = 0;

	CRect r;
	GetClientRect(r);

	if ( ! r.PtInRect(p))
	{
		return VSHT_NONCLIENT;
	}

	int ChannelUnderCursor = GetChannelFromPoint(p.y);
	CRect ChannelRect;
	GetChannelRect(ChannelUnderCursor, ChannelRect);

	if (ChannelUnderCursor >= 0
		&& ChannelUnderCursor < pDoc->WaveChannels())
	{
		result |= ChannelUnderCursor;

		if (0 != (pDoc->m_SelectedChannel & (1 << ChannelUnderCursor))
			&& pDoc->m_SelectionStart <= pDoc->m_SelectionEnd)
		{
			int SelBegin = SampleToX(pDoc->m_SelectionStart);
			int SelEnd = SampleToX(pDoc->m_SelectionEnd);

			if (pDoc->m_SelectionEnd != pDoc->m_SelectionStart
				&& SelEnd == SelBegin)
			{
				SelEnd++;
			}
			int BorderWidth = GetSystemMetrics(SM_CXEDGE);
			// check whether the cursor is on the selection boundary

			if (p.x < SelBegin - BorderWidth)
			{
				// nothing
			}
			else if (p.x < SelBegin)
			{
				result |= VSHT_SEL_BOUNDARY_L;
			}
			else if (p.x >= SelEnd + BorderWidth)
			{
				// nothing
			}
			else if (p.x >= SelEnd)
			{
				result |= VSHT_SEL_BOUNDARY_R;
			}
			else if (SelEnd - SelBegin < BorderWidth * 2)
			{
				if (p.x < (SelBegin + SelEnd) / 2)
				{
					result |= VSHT_SEL_BOUNDARY_L;
				}
				else
				{
					result |= VSHT_SEL_BOUNDARY_R;
				}
			}
			else if (p.x < SelBegin + BorderWidth)
			{
				result |= VSHT_SEL_BOUNDARY_L;
			}
			else if (p.x >= SelEnd - BorderWidth)
			{
				result |= VSHT_SEL_BOUNDARY_R;
			}
			else
			{
				result |= VSHT_SELECTION;
			}
		}
	}

	int DataEnd = SampleToXceil(pDoc->WaveFileSamples());

	if (p.x < DataEnd)
	{
		result |= VSHT_BCKGND;

		int const AutoscrollWidth = GetSystemMetrics(SM_CXVSCROLL);
		if (r.right > AutoscrollWidth)
		{
			if (p.x > r.right - AutoscrollWidth)
			{
				result |= VSHT_RIGHT_AUTOSCROLL;
			}
			if (p.x < AutoscrollWidth)
			{
				result |= VSHT_LEFT_AUTOSCROLL;
			}
		}
	}
	else
	{
		result |= VSHT_NOWAVE;
	}
	return result;
}

void CWaveFftView::OnUpdateViewZoomInVert(CCmdUI* pCmdUI)
{
	pCmdUI->Enable(m_VerticalScale < 1024.);
}

void CWaveFftView::OnUpdateViewZoomOutVert(CCmdUI* pCmdUI)
{
	pCmdUI->Enable(m_VerticalScale > 1.);
}

void CWaveFftView::OnUpdateViewZoomvertNormal(CCmdUI* pCmdUI)
{
	pCmdUI->Enable(m_VerticalScale > 1.);
}

void CWaveFftView::RemoveSelectionRect()
{
	RedrawSelectionRect(NULL, m_PrevSelectionStart, m_PrevSelectionEnd, m_PrevSelectedChannel,
						0, 0, 0);

	m_PrevSelectionStart = 0;
	m_PrevSelectionEnd = 0;
	m_PrevSelectedChannel = 0;
}

void CWaveFftView::ShowSelectionRect()
{
	ThisDoc * pDoc = GetDocument();

	RedrawSelectionRect(NULL, m_PrevSelectionStart, m_PrevSelectionEnd, m_PrevSelectedChannel,
						pDoc->m_SelectionStart, pDoc->m_SelectionEnd, pDoc->m_SelectedChannel);

	m_PrevSelectionStart = pDoc->m_SelectionStart;
	m_PrevSelectionEnd = pDoc->m_SelectionEnd;
	m_PrevSelectedChannel = pDoc->m_SelectedChannel;
}

int CWaveFftView::BuildSelectionRegion(CRgn * NormalRgn, CRgn* MinimizedRgn,
										SAMPLE_INDEX SelectionStart, SAMPLE_INDEX SelectionEnd, CHANNEL_MASK selected)
{
	CRect cr;
	GetClientRect(cr);

	int cx = GetSystemMetrics(SM_CXSIZEFRAME);
	int cy = GetSystemMetrics(SM_CYSIZEFRAME);

	RgnData data(4 * MAX_NUMBER_OF_CHANNELS), data_min(4 * MAX_NUMBER_OF_CHANNELS);

	if (SelectionStart == SelectionEnd
		|| selected == 0)
	{
		NormalRgn->CreateRectRgn(0, 0, 0, 0);
		MinimizedRgn->CreateRectRgn(0, 0, 0, 0);
		return 0;
	}

	int left = SampleToX(SelectionStart);
	int right = SampleToX(SelectionEnd);

	if (left == right)
	{
		right++;
	}

	if (left >= cr.right
		|| right <= cr.left)
	{
		NormalRgn->CreateRectRgn(0, 0, 0, 0);
		MinimizedRgn->CreateRectRgn(0, 0, 0, 0);
		return 0;
	}

	if (cx > right - left)
	{
		cx = right - left;
	}
	NUMBER_OF_CHANNELS NumChannels = GetDocument()->WaveChannels();

	for (int chan = 0; chan < NumChannels; chan++)
	{
		if (selected & (1 << chan))
		{
			RgnData * rgn;
			if (m_Heights.ChannelMinimized(chan))
			{
				rgn = &data_min;
			}
			else
			{
				rgn = &data;
			}

			rgn->AddRect(left, m_Heights.ch[chan].clip_top, left + cx, m_Heights.ch[chan].clip_bottom);
			rgn->AddRect(right - cx, m_Heights.ch[chan].clip_top, right, m_Heights.ch[chan].clip_bottom);

			if (right - left <= cx * 2)
			{
				continue;
			}

			if (chan == 0 || 0 == (selected & (1 << (chan - 1))))
			{
				rgn->AddRect(left + cx, m_Heights.ch[chan].clip_top, right - cx, m_Heights.ch[chan].clip_top + cy);
			}
			if (chan + 1 == NumChannels || 0 == (selected & (1 << (chan + 1))))
			{
				rgn->AddRect(left + cx, m_Heights.ch[chan].clip_bottom - cy, right - cx, m_Heights.ch[chan].clip_bottom);
			}
		}
	}
	NormalRgn->CreateFromData(NULL, data.hdr->nRgnSize, data);
	MinimizedRgn->CreateFromData(NULL, data_min.hdr->nRgnSize, data_min);

	return data.hdr->nCount + (data_min.hdr->nCount << 16);
}

void CWaveFftView::RedrawSelectionRect(CDC * pDC, SAMPLE_INDEX OldSelectionStart, SAMPLE_INDEX OldSelectionEnd, CHANNEL_MASK OldSelectedChannel,
										SAMPLE_INDEX NewSelectionStart, SAMPLE_INDEX NewSelectionEnd, CHANNEL_MASK NewSelectedChannel)
{
	CRgn OldRgn;
	CRgn OldRgnMinimized;

	CRgn NewRgn;
	CRgn NewRgnMinimized;   // for minimized channels

	int OldRgnRectCount = BuildSelectionRegion(&OldRgn, &OldRgnMinimized, OldSelectionStart, OldSelectionEnd, OldSelectedChannel);

	int NewRgnRectCount = BuildSelectionRegion(&NewRgn, &NewRgnMinimized, NewSelectionStart, NewSelectionEnd, NewSelectedChannel);

	if ((OldRgnRectCount | NewRgnRectCount) == 0)
	{
		return;
	}

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
		CBitmap bmp;
		static const WORD pattern[] =
		{
			0x5555,   // aligned to WORD
			0xAAAA,
			0x5555,
			0xAAAA,
			0x5555,
			0xAAAA,
			0x5555,
			0xAAAA,
			0x5555,
		};

		int OddFileOffset = (int)fmod(floor(m_FirstSampleInView / m_HorizontalScale), 2.);

		int VerticalOddOffset = 1 & int(m_FirstbandVisible * int(m_VerticalScale * m_Heights.NominalChannelHeight) / m_FftOrder);
		bmp.CreateBitmap(8, 8, 1, 1, pattern + (OddFileOffset ^ VerticalOddOffset));

		CBrush brush(&bmp);

		CRgn R;
		CRgn OldClipRgn;
		CRect clip;
		CBitmap bmp_min;
		CBrush brush_min;

		OldClipRgn.CreateRectRgn(0, 0, 0, 0);
		::GetClipRgn((HDC)*pDrawDC, (HRGN)OldClipRgn);

		R.CreateRectRgn(0, 0, 0, 0);
		R.CombineRgn(&OldRgn, &NewRgn, RGN_XOR);

		pDrawDC->SelectClipRgn(&R, RGN_AND);

		CGdiObjectSave OldBrush(pDrawDC, pDrawDC->SelectObject(&brush));

		pDrawDC->GetClipBox(clip);

		pDrawDC->PatBlt(clip.left, clip.top, clip.Width(), clip.Height(), PATINVERT);

		if (((OldRgnRectCount | NewRgnRectCount) & 0xFFFF0000) != 0)
		{
			bmp_min.CreateBitmap(8, 8, 1, 1, pattern + OddFileOffset);
			brush_min.CreatePatternBrush(&bmp_min);
			pDrawDC->SelectObject(&brush_min);

			R.CombineRgn(&OldRgnMinimized, &NewRgnMinimized, RGN_XOR);

			pDrawDC->SelectClipRgn(&OldClipRgn, RGN_COPY);
			pDrawDC->SelectClipRgn(&R, RGN_AND);

			pDrawDC->GetClipBox(clip);

			pDrawDC->PatBlt(clip.left, clip.top, clip.Width(), clip.Height(), PATINVERT);
		}

		pDrawDC->SelectClipRgn(&OldClipRgn, RGN_COPY);
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


void CWaveFftView::OnFftWindowSquaredSine()
{
	OnSetWindowType(WindowTypeSquaredSine);
}

void CWaveFftView::OnUpdateFftWindowSquaredSine(CCmdUI* pCmdUI)
{
	pCmdUI->SetRadio(WindowTypeSquaredSine == m_FftWindowType);
}

void CWaveFftView::OnFftWindowSine()
{
	OnSetWindowType(WindowTypeHalfSine);
}

void CWaveFftView::OnUpdateFftWindowSine(CCmdUI* pCmdUI)
{
	pCmdUI->SetRadio(WindowTypeHalfSine == m_FftWindowType);
}

void CWaveFftView::OnFftWindowHamming()
{
	OnSetWindowType(WindowTypeHamming);
}

void CWaveFftView::OnUpdateFftWindowHamming(CCmdUI* pCmdUI)
{
	pCmdUI->SetRadio(WindowTypeHamming == m_FftWindowType);
}

void CWaveFftView::OnFftWindowNuttall()
{
	OnSetWindowType(WindowTypeNuttall);
}

void CWaveFftView::OnUpdateFftWindowNuttall(CCmdUI* pCmdUI)
{
	pCmdUI->SetRadio(WindowTypeNuttall == m_FftWindowType);
}

void CWaveFftView::OnViewDecreaseFftBands()
{
	int order = GetApp()->m_FftBandsOrder;
	if (order > 6)
	{
		OnSetBands(order - 1);
	}
}

void CWaveFftView::OnViewIncreaseFftBands()
{
	int order = GetApp()->m_FftBandsOrder;
	if (order < 13)
	{
		OnSetBands(order + 1);
	}
}

void CWaveFftView::OnContextMenu(CWnd* /*pWnd*/, CPoint point)
{
	// make sure window is active
	NotifyViewsData notify = { 0 };
	notify.PopupMenu.p = point;
	ScreenToClient(&point);
	DWORD hit = ClientHitTest(point);
	if (hit & VSHT_SELECTION)
	{
		notify.PopupMenu.NormalMenuId = IDR_MENU_FFT_VIEW_SELECTION;
		notify.PopupMenu.MinimizedMenuId = IDR_MENU_FFT_VIEW_SELECTION_MINIMIZED;
	}
	else
	{
		notify.PopupMenu.NormalMenuId = IDR_MENU_FFT_VIEW;
		notify.PopupMenu.MinimizedMenuId = IDR_MENU_FFT_VIEW_MINIMIZED;
	}

	NotifySiblingViews(ShowChannelPopupMenu, &notify);
}

double CWaveFftView::AdjustOffset(double offset) const
{
	if (offset < 0.)
	{
		return 0;
	}

	int ScaledHeight = int(m_Heights.NominalChannelHeight * m_VerticalScale);
	int MaxOffsetPixels = ScaledHeight - m_Heights.NominalChannelHeight;

	double MaxOffset = double(MaxOffsetPixels) * m_FftOrder / ScaledHeight;

	ASSERT(MaxOffset >= 0);
	ASSERT(MaxOffset < m_FftOrder);
	if (offset > MaxOffset)
	{
		return MaxOffset;
	}
	return offset;
}

void CWaveFftView::SetNewFftOffset(double first_band)
{
	// scroll channels rectangles
	CWaveSoapFrontDoc * pDoc = GetDocument();
	int nChannels = pDoc->WaveChannels();

	CRect cr, original_cr;
	GetClientRect(cr);
	original_cr = cr;

	int const FileEnd = SampleToXceil(pDoc->WaveFileSamples());
	if (cr.right > FileEnd)
	{
		cr.right = FileEnd;
	}

	long OldOffsetPixels = long(m_FirstbandVisible * int(m_VerticalScale * m_Heights.NominalChannelHeight) / m_FftOrder);
	long NewOffsetPixels = long(first_band * int(m_VerticalScale * m_Heights.NominalChannelHeight) / m_FftOrder);
	m_FirstbandVisible = first_band;

	CHANNEL_MASK Selected = m_PrevSelectedChannel;		// currently drawn selected channel
	if (pDoc->m_SelectionStart == pDoc->m_SelectionEnd)
	{
		Selected = 0;
	}
	int RightSampleInView = (int)ceil(m_FirstSampleInView + cr.Width() * m_HorizontalScale);
	int selected_left = 0, selected_right = RightSampleInView;

	if (pDoc->m_SelectionEnd <= m_FirstSampleInView
		|| pDoc->m_SelectionStart >= RightSampleInView)
	{
		Selected = 0;
	}
	else
	{
		if (pDoc->m_SelectionStart > m_FirstSampleInView)
		{
			selected_left = SampleToX(pDoc->m_SelectionStart);
		}
		if (pDoc->m_SelectionEnd < RightSampleInView)
		{
			selected_right = SampleToX(pDoc->m_SelectionEnd);
		}
	}

	// offset of the zero line down
	int ToScroll = NewOffsetPixels - OldOffsetPixels;       // >0 - down, <0 - up
	if (ToScroll == 0)
	{
		return;
	}

	int cy = GetSystemMetrics(SM_CYSIZEFRAME);

	CWindowDC dc(this);		// for filling the scrolled in rectangles
	HideCaret();

	CRgn TotalUpdateRgn;
	CRgn NewUpdateRgn;
	TotalUpdateRgn.CreateRectRgn(0, 0, 0, 0);
	NewUpdateRgn.CreateRectRgn(0, 0, 0, 0);
	GetUpdateRgn(&TotalUpdateRgn);

	CRgn ChannelUpdateRgn;
	CRgn ChannelRectRgn;
	CRgn SeparatorUpdateRgn;		// for one pixel separator line
	ChannelUpdateRgn.CreateRectRgn(0, 0, 0, 0);
	ChannelRectRgn.CreateRectRgn(0, 0, 0, 0);
	SeparatorUpdateRgn.CreateRectRgn(0, 0, 0, 0);
	CRgn SelectionRgn;
	SelectionRgn.CreateRectRgn(0, 0, 0, 0);
	CRgn FillRgn;
	FillRgn.CreateRectRgn(0, 0, 0, 0);
	// now split it to channel regions
	ValidateRect(cr);	// ScrollWindow will not scroll invalid areas
	for (int ch = 0; ch < nChannels; ch++)
	{
		ChannelRectRgn.SetRectRgn(cr.left, m_Heights.ch[ch].clip_top, cr.right, m_Heights.ch[ch].clip_bottom);
		ChannelUpdateRgn.CombineRgn(&TotalUpdateRgn, &ChannelRectRgn, RGN_AND);

		SeparatorUpdateRgn.SetRectRgn(cr.left, m_Heights.ch[ch].clip_bottom, cr.right, m_Heights.ch[ch].clip_bottom + 1);
		SeparatorUpdateRgn.CombineRgn(&SeparatorUpdateRgn, &TotalUpdateRgn, RGN_AND);

		if (m_Heights.ChannelMinimized(ch))
		{
			NewUpdateRgn.CombineRgn(&NewUpdateRgn, &ChannelUpdateRgn, RGN_OR);
			NewUpdateRgn.CombineRgn(&NewUpdateRgn, &SeparatorUpdateRgn, RGN_OR);
			continue;
		}

		CRect ClipRect(cr.left, m_Heights.ch[ch].clip_top, cr.right, m_Heights.ch[ch].clip_bottom);
		CRect ScrollRect(ClipRect);
		CRect SelectionBoundaryTop(selected_left, ClipRect.top, selected_right, ClipRect.top + cy);
		CRect SelectionBoundaryBottom(selected_left, ClipRect.bottom - cy, selected_right, ClipRect.bottom);

		// see if there is a selection boundary on the top
		if ((Selected & (1 << ch))
			&& (ch == 0 || 0 == (Selected & (1 << (ch - 1)))))
		{
			ScrollRect.top = SelectionBoundaryTop.bottom;
			SelectionRgn.SetRectRgn(SelectionBoundaryTop);
			ChannelUpdateRgn.CombineRgn(&ChannelUpdateRgn, &SelectionRgn, RGN_OR);
		}
		// see if there is a selection boundary on the bottom
		if ((Selected & (1 << ch))
			&& (ch + 1 == nChannels || 0 == (Selected & (1 << (ch + 1)))))
		{
			ScrollRect.bottom = SelectionBoundaryBottom.top;
			SelectionRgn.SetRectRgn(SelectionBoundaryBottom);
			ChannelUpdateRgn.CombineRgn(&ChannelUpdateRgn, &SelectionRgn, RGN_OR);
		}
		CRect ToFillBkgnd(ScrollRect);

		if (ToScroll > 0)
		{
			// down
			ToFillBkgnd.bottom = ToFillBkgnd.top + ToScroll;
			SelectionBoundaryTop.bottom += ToScroll;
		}
		else
		{
			// up
			ToFillBkgnd.top = ToFillBkgnd.bottom + ToScroll;
			SelectionBoundaryBottom.top += ToScroll;
		}

		ScrollWindow(0, ToScroll, ScrollRect, ClipRect);
		dc.FillSolidRect(ToFillBkgnd, 0);	// fill with black

		// offset the update region by scroll
		ChannelUpdateRgn.OffsetRgn(0, ToScroll);

		FillRgn.SetRectRgn(ToFillBkgnd);
		ChannelUpdateRgn.CombineRgn(&ChannelUpdateRgn, &FillRgn, RGN_OR);

		ChannelUpdateRgn.CombineRgn(&ChannelUpdateRgn, &ChannelRectRgn, RGN_AND);
		NewUpdateRgn.CombineRgn(&NewUpdateRgn, &ChannelUpdateRgn, RGN_OR);
		if (ch + 1 != nChannels)
		{
			NewUpdateRgn.CombineRgn(&NewUpdateRgn, &SeparatorUpdateRgn, RGN_OR);
		}
	}

	if (cr.right != original_cr.right)
	{
		ChannelUpdateRgn.SetRectRgn(cr);
		ChannelUpdateRgn.CombineRgn(&ChannelUpdateRgn, &TotalUpdateRgn, RGN_AND);
		NewUpdateRgn.CombineRgn(&NewUpdateRgn, &ChannelUpdateRgn, RGN_OR);
	}
	InvalidateRgn(&NewUpdateRgn, FALSE);

	if (!m_Heights.ChannelMinimized(0))
	{
		InvalidateMarkerLabels(ToScroll);
	}

	ShowCaret();
}

afx_msg LRESULT CWaveFftView::OnUwmNotifyViews(WPARAM wParam, LPARAM lParam)
{
	switch(wParam)
	{
	case HorizontalExtentChanged:
	{
		NotifyViewsData * data = (NotifyViewsData*)lParam;
		m_FirstSampleInView = data->HorizontalScroll.FirstSampleInView;
		if (m_HorizontalScale != data->HorizontalScroll.HorizontalScale)
		{
			m_HorizontalScale = data->HorizontalScroll.HorizontalScale;
			Invalidate();
		}
		UpdateCaretPosition();
	}
		break;
	case ChannelHeightsChanged:
		m_Heights = *(NotifyChannelHeightsData*)lParam;
		Invalidate();
		CreateAndShowCaret(true);
		break;
	case FftScrollTo:
		// lParam points to double offset
		// check for the proper offset, correct if necessary
	{
		double offset = AdjustOffset(*(double*) lParam);

		NotifySiblingViews(FftOffsetChanged, & offset);
	}
		break;
	case FftOffsetChanged:
		SetNewFftOffset(*(double*) lParam);
		break;
	case FftVerticalScaleChanged:
		// correct the offset, if necessary
		// find max and min offset for this scale
		m_VerticalScale = *(double*)lParam;

		m_FirstbandVisible = AdjustOffset(m_FirstbandVisible);

		Invalidate(TRUE);
		break;
	default:
		return BaseClass::OnUwmNotifyViews(wParam, lParam);
	}
	return 0;
}
