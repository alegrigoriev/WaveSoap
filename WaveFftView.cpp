// WaveFftView.cpp : implementation file
//

#include "stdafx.h"
#include "WaveSoapFront.h"
#include "WaveSoapFrontView.h"
#include "WaveFftView.h"
#include "fft.h"

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

/////////////////////////////////////////////////////////////////////////////
// CWaveFftView

IMPLEMENT_DYNCREATE(CWaveFftView, CWaveSoapFrontView)

CWaveFftView::CWaveFftView()
	: m_pFftResultArray(NULL),
	m_FftResultArrayWidth(0),
	m_FftResultArrayHeight(0),
	m_FftResultBegin(0),
	m_FftLogRange(4.34294481903251827651128918916605),
	m_FftOrder(512),
	m_FftSpacing(512),
	m_pFftWindow(NULL),
	m_FirstbandVisible(0),
//m_FftSamplesCalculated(0),
	m_FftArraySize(0)
{
}

CWaveFftView::~CWaveFftView()
{
	delete[] m_pFftWindow;
	m_pFftWindow = NULL;
	delete[] m_pFftResultArray;
	m_pFftResultArray = NULL;
}


BEGIN_MESSAGE_MAP(CWaveFftView, CWaveSoapFrontView)
	//{{AFX_MSG_MAP(CWaveFftView)
	ON_COMMAND(ID_VIEW_ZOOMVERT_NORMAL, OnViewZoomvertNormal)
	ON_COMMAND(ID_VIEW_ZOOMINVERT, OnViewZoomInVert)
	ON_COMMAND(ID_VIEW_ZOOMOUTVERT, OnViewZoomOutVert)
	ON_WM_PAINT()
	ON_WM_ERASEBKGND()
	ON_COMMAND(ID_FFT_BANDS_1024, OnFftBands1024)
	ON_UPDATE_COMMAND_UI(ID_FFT_BANDS_1024, OnUpdateFftBands1024)
	ON_COMMAND(ID_FFT_BANDS_128, OnFftBands128)
	ON_UPDATE_COMMAND_UI(ID_FFT_BANDS_128, OnUpdateFftBands128)
	ON_COMMAND(ID_FFT_BANDS_2048, OnFftBands2048)
	ON_UPDATE_COMMAND_UI(ID_FFT_BANDS_2048, OnUpdateFftBands2048)
	ON_COMMAND(ID_FFT_BANDS_256, OnFftBands256)
	ON_UPDATE_COMMAND_UI(ID_FFT_BANDS_256, OnUpdateFftBands256)
	ON_COMMAND(ID_FFT_BANDS_4096, OnFftBands4096)
	ON_UPDATE_COMMAND_UI(ID_FFT_BANDS_4096, OnUpdateFftBands4096)
	ON_COMMAND(ID_FFT_BANDS_512, OnFftBands512)
	ON_UPDATE_COMMAND_UI(ID_FFT_BANDS_512, OnUpdateFftBands512)
	ON_COMMAND(ID_FFT_BANDS_64, OnFftBands64)
	ON_UPDATE_COMMAND_UI(ID_FFT_BANDS_64, OnUpdateFftBands64)
	ON_COMMAND(ID_FFT_BANDS_8192, OnFftBands8192)
	ON_UPDATE_COMMAND_UI(ID_FFT_BANDS_8192, OnUpdateFftBands8192)
	//}}AFX_MSG_MAP
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

/////////////////////////////////////////////////////////////////////////////
// CWaveFftView drawing

void CWaveFftView::OnDraw(CDC* pDC)
{
	CWaveSoapFrontDoc* pDoc = GetDocument();
	if (! pDoc->m_WavFile.IsOpen())
	{
		return;
	}
	// FFT is performed 1 time per 1 pixel across.
	//
	// FFT order is specified by the options dialog.
	// The conversion result is cached in the byte array, of window width
	// and FFT order high

	// TODO: add draw code here
	// show FFT:
	// 1. build grayscale palette and realize it (for 8 bit mode)
	// 2. allocate 256 colors or truecolor bitmap section
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
	int iClientWidth = r.right - r.left;
	PointToDoubleDev(CPoint(r.left, cr.top), left, top);
	PointToDoubleDev(CPoint(r.right, cr.bottom), right, bottom);

	if (left < 0) left = 0;
	// create an array of points
	int nNumberOfPoints = r.right - r.left;

	MakeFftArray(left, right);

	BITMAPINFOHEADER bmih = {
		sizeof bmih,
		r.right - r.left,
		cr.top - cr.bottom, // <0
		1,
		24,
		BI_RGB,
		0,
		0,
		0,
		0,
		0
	};
	const int BytesPerPixel = 3;
	void * pBits;
	HBITMAP hbm = CreateDIBSection(pDC->GetSafeHdc(), (LPBITMAPINFO) & bmih, 0, & pBits, NULL, 0);
	if (hbm == NULL)
	{
		return;
	}

	// get windowed samples with 50% overlap
	LPBYTE pBmp = LPBYTE(pBits);

	size_t width = r.right - r.left;
	size_t height = cr.bottom - cr.top;
	size_t stride = (width * 3 + 3) & ~3;
	size_t BmpSize = stride * abs(height);
	memset(pBmp, 0, BmpSize);
//    int i;

	// fill the array
	int x;
	x = WindowToWorldX(r.left);
	//y = WindowToWorldY(r.top);

	int nChannels = pDoc->WaveChannels();
	// find offset in the FFT result array for 'left' point
	// and how many columns to fill with this color
	int ColsPerFftPoint = m_FftSpacing / m_HorizontalScale;
	int nFirstCol = (x - m_FftResultBegin) / m_FftSpacing;
	int FirstCols = ColsPerFftPoint -
					((x - m_FftResultBegin) % m_FftSpacing) / m_HorizontalScale;

	// find vertical offset in the result array and how many
	// rows to fill with this color
	int rows = cr.Height() / nChannels;
	// if all the chart was drawn, how many scans it would have:
	int TotalRows = rows * m_VerticalScale;

	if (0 == TotalRows)
	{
		DeleteObject(hbm);
		CScaledScrollView::OnDraw(pDC);
		return;
	}

	int LastFftSample = m_FftOrder - m_FirstbandVisible;
	int FirstFftSample = LastFftSample + (-rows * m_FftOrder) / TotalRows;
	if (FirstFftSample < 0)
	{
		LastFftSample -= FirstFftSample;
		FirstFftSample = 0;
	}
	int FirstRowInView = FirstFftSample * TotalRows / m_FftOrder;
	int FftSamplesInView = LastFftSample - FirstFftSample + 1;

	int IdxSize1 = __min(rows, FftSamplesInView);

	// build an array
	struct S
	{
		int nFftOffset;
		int nNumOfRows;
	};

	S * pIdArray = new S[IdxSize1];

	if (NULL == pIdArray)
	{
		DeleteObject(hbm);
		return;
	}
	// fill the array
	int LastRow = 0;
	int k;
	for (k = 0; k < IdxSize1; k++)
	{
		if (FirstFftSample >= m_FftOrder
			|| LastRow >= rows)
		{
			break;
		}
		pIdArray[k].nFftOffset = FirstFftSample;
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
		pIdArray[k].nNumOfRows = NextRow - LastRow;
		LastRow = NextRow;
	}
	TRACE("LastRow = %d, cr.height=%d\n", LastRow, cr.Height());
	ASSERT(LastRow <= rows);
	int IdxSize = k;

	unsigned char * pColBmp = pBmp;
	int nChanOffset = stride * rows;
	unsigned char * pData = m_pFftResultArray + nFirstCol * m_FftResultArrayHeight;
	int nColumns = FirstCols;
	for(int col = r.left; col < r.right && pData < m_pFftResultArray + m_FftArraySize; )
	{
		int ff;
		S * pId;
		if (nColumns > r.right - col)
		{
			nColumns = r.right - col;
		}
		if (pData[0])
		{
			pData++;
			for (int ch = 0, nFftChOffset = 1, nBmpChOffset = 0; ch < nChannels; ch++,
				nBmpChOffset += nChanOffset, pData += m_FftOrder)
			{
				BYTE * pRgb = pColBmp + nBmpChOffset;
				if (nColumns != 1)
				{
					for (ff = 0, pId = pIdArray; ff < IdxSize; ff++, pId++)
					{
						unsigned char const * pColor = & palette[pData[pId->nFftOffset] * 3];
						// set the color to pId->nNumOfRows rows
						unsigned char r = pColor[0];
						unsigned char g = pColor[1];
						unsigned char b = pColor[2];
						for (int y = 0; y < pId->nNumOfRows; y++, pRgb += stride - nColumns * 3)
						{
							// set the color to nColumns pixels across
							for (int x = 0; x < nColumns; x++, pRgb += 3)
							{
								ASSERT(pRgb >= pBmp && pRgb + 3 <= pBmp + BmpSize);
								pRgb[0] = b;    // B
								pRgb[1] = g;    // G
								pRgb[2] = r;    // R
							}
						}
					}
				}
				else
				{
					for (ff = 0, pId = pIdArray; ff < IdxSize; ff++, pId++)
					{
						unsigned char const * pColor = & palette[pData[pId->nFftOffset] * 3];
						// set the color to pId->nNumOfRows rows
						unsigned char r = pColor[0];
						unsigned char g = pColor[1];
						unsigned char b = pColor[2];
						for (int y = 0; y < pId->nNumOfRows; y++, pRgb += stride)
						{
							// set the color
							ASSERT(pRgb >= pBmp && pRgb + 3 <= pBmp + BmpSize);
							pRgb[0] = b;    // B
							pRgb[1] = g;    // G
							pRgb[2] = r;    // R
						}
					}
				}
			}
		}
		else
		{
			pData += m_FftResultArrayHeight;
		}
		col += nColumns;
		pColBmp += nColumns * BytesPerPixel;
		nColumns = ColsPerFftPoint;
	}
	delete[] pIdArray;
	// stretch bitmap to output window
	SetDIBitsToDevice(pDC->GetSafeHdc(), r.left, cr.top,
					width, height,
					0, 0, 0, height, pBits, (LPBITMAPINFO) & bmih,
					0);
	GdiFlush(); // make sure bitmap is drawn before deleting it (NT only)
	// free resources
	DeleteObject(hbm);
	if (m_PlaybackCursorDrawn)
	{
		DrawPlaybackCursor(pDC, m_PlaybackCursorDrawnSamplePos, m_PlaybackCursorChannel);
	}
	CScaledScrollView::OnDraw(pDC);
}

void CWaveFftView::MakeFftArray(int left, int right)
{
	CWaveSoapFrontDoc * pDoc = GetDocument();
	CRect r;
	int NumberOfFftPoints;
	GetClientRect( & r);
	int FftSpacing = m_HorizontalScale;
	if (m_FftOrder <= m_HorizontalScale)
	{
		NumberOfFftPoints = r.Width()+1;
	}
	else
	{
		FftSpacing = m_FftOrder;
		NumberOfFftPoints = 1 + r.Width() * m_HorizontalScale / m_FftOrder;
	}
	// for each FFT set we keep a byte to mark it valid or invalid
	int NewFftArrayHeight = (m_FftOrder * pDoc->WaveChannels() + 1);
	size_t NecessaryArraySize =
		NumberOfFftPoints * NewFftArrayHeight;
	if (FftSpacing != m_FftSpacing
		|| m_FftResultArrayHeight != NewFftArrayHeight
		|| m_FftArraySize < NecessaryArraySize
		|| NULL == m_pFftResultArray)
	{
		unsigned char * pOldArray = m_pFftResultArray;
		m_pFftResultArray = NULL;
		if (m_FftResultArrayHeight != NewFftArrayHeight)
		{
			// no use in the old data
			delete[] pOldArray;
			pOldArray = NULL;
		}
		m_pFftResultArray = new unsigned char[NecessaryArraySize];
		if (NULL == m_pFftResultArray)
		{
			return;
		}

		m_FftArraySize = NecessaryArraySize;
		unsigned char * pTmp = m_pFftResultArray;
		int i;
		int FirstFftSample = left - left % FftSpacing;
		int FftSample = FirstFftSample;
		for (i = 0; i < NumberOfFftPoints; i++,
			pTmp += NewFftArrayHeight, FftSample += FftSpacing)
		{
			pTmp[0] = 0;    // invalidate
			if (pOldArray != NULL
				&& 0 == FftSample % m_FftSpacing
				&& FftSample >= m_FftResultBegin
				&& FftSample < m_FftResultEnd)
			{
				unsigned char * p = pOldArray +
									(FftSample - m_FftResultBegin) / m_FftSpacing * NewFftArrayHeight;
				ASSERT(p >= pOldArray && p + NewFftArrayHeight <= pOldArray + m_FftArraySize);
				if (p[0] != 0)
				{
					ASSERT(1 == p[0]);
					memcpy(pTmp, p, NewFftArrayHeight);
				}
			}
		}

		m_FftResultEnd = FftSample;
		m_FftResultBegin = FirstFftSample;
		delete[] pOldArray;
	}
	m_FftSpacing = FftSpacing;
	m_FftResultArrayHeight = NewFftArrayHeight;
	m_FftResultArrayWidth = NumberOfFftPoints;

	if (NULL == m_pFftResultArray)
	{
		m_FftArraySize = 0;
		return;
	}
	CalculateFftRange(left, right);
}

void CWaveFftView::CalculateFftRange(int left, int right)
{
	CWaveSoapFrontDoc * pDoc = GetDocument();
	// make sure the required range is in the buffer
	// buffer size is enough to hold all data
	int FirstSampleRequired = left - left % m_FftSpacing;
	int LastSampleRequired = right + m_FftSpacing - right % m_FftSpacing;
	TRACE("Samples required from %d to %d, in the buffer: from %d to %d\n",
		FirstSampleRequired, LastSampleRequired, m_FftResultBegin, m_FftResultEnd);
	if (FirstSampleRequired < m_FftResultBegin)
	{
		int i = (m_FftResultArrayWidth - 1) * m_FftResultArrayHeight;
		if (LastSampleRequired > m_FftResultBegin)
		{
			// free some space in the beginning of the buffer
			TRACE("Freeing %d positions in the beginning of buffer\n",
				(m_FftResultBegin - FirstSampleRequired) / m_FftSpacing);
			int j = i -
					(m_FftResultBegin - FirstSampleRequired) / m_FftSpacing * m_FftResultArrayHeight;
			for (; j >= 0; i -= m_FftResultArrayHeight, j -= m_FftResultArrayHeight)
			{
				if (m_pFftResultArray[j] != 0)
				{
					ASSERT(1 == m_pFftResultArray[j]);
					memcpy(m_pFftResultArray + i, m_pFftResultArray + j, m_FftResultArrayHeight);
				}
				else
				{
					m_pFftResultArray[i] = 0;
				}
			}
		}
		TRACE("Cleaning %d remaining FFT sets\n", i / m_FftResultArrayHeight + 1);
		for (; i >= 0; i -= m_FftResultArrayHeight)
		{
			m_pFftResultArray[i] = 0;
		}
		m_FftResultEnd -= m_FftResultBegin - FirstSampleRequired;
		m_FftResultBegin = FirstSampleRequired;
	}
	else if (LastSampleRequired > m_FftResultEnd)
	{
		int i = 0;
		if (FirstSampleRequired < m_FftResultEnd)
		{
			// free some space in the end of the buffer
			TRACE("Freeing %d positions in the end of buffer\n",
				(LastSampleRequired - m_FftResultEnd) / m_FftSpacing);
			int j = (LastSampleRequired - m_FftResultEnd) / m_FftSpacing * m_FftResultArrayHeight;
			for (; j < m_FftArraySize; i += m_FftResultArrayHeight, j += m_FftResultArrayHeight)
			{
				if (m_pFftResultArray[j] != 0)
				{
					ASSERT(1 == m_pFftResultArray[j]);
					memcpy(m_pFftResultArray + i, m_pFftResultArray + j, m_FftResultArrayHeight);
				}
				else
				{
					m_pFftResultArray[i] = 0;
				}
			}
		}
		for (; i < m_FftArraySize; i += m_FftResultArrayHeight)
		{
			m_pFftResultArray[i] = 0;
		}
		m_FftResultBegin += LastSampleRequired - m_FftResultEnd;
		m_FftResultEnd = LastSampleRequired;
	}
	// calculate FFT
	if (NULL == m_pFftWindow)
	{
		TRACE("Calculating FFT window\n");
		m_pFftWindow = new float[m_FftOrder * 2];
		for (int w = 0; w < m_FftOrder * 2; w++)
		{
			// Hamming window (sucks!!!)
			//m_pFftWindow[w] = float(0.54 - 0.46 * cos (w * M_PI /  m_FftOrder));
			// squared sine
			m_pFftWindow[w] = float(0.5 - 0.5 * cos ((w + 0.5) * M_PI /  m_FftOrder));
			// half sine is the best so far
			//m_pFftWindow[w] = float(0.707107 * sin (w * M_PI /  (2*m_FftOrder)));
		}
	}

	int i = (FirstSampleRequired - m_FftResultBegin) / m_FftSpacing * m_FftResultArrayHeight;
	int j = (LastSampleRequired - m_FftResultBegin) / m_FftSpacing * m_FftResultArrayHeight;
	float * buf = NULL;
	double PowerOffset = log(65536. * m_FftOrder) * 2.;
	for (; i < j; i += m_FftResultArrayHeight, FirstSampleRequired += m_FftSpacing)
	{
		if (0 == m_pFftResultArray[i])
		{
			if (NULL == buf)
			{
				buf = new float[m_FftOrder * 2 + 2];
				if (NULL == buf)
				{
					break;
				}
			}
			int nChannels = pDoc->WaveChannels();
			unsigned char * pRes = & m_pFftResultArray[i + 1 + m_FftOrder];
			if (FirstSampleRequired + m_FftOrder * 2 > pDoc->WaveFileSamples())
			{
				// the required samples are out of the file
				continue;
			}

			GetWaveSamples(FirstSampleRequired * nChannels, m_FftOrder * 2 * nChannels);
			for (int ch = 0; ch < nChannels; ch++)
			{
				int nIndexOfSample =
					ch + FirstSampleRequired * nChannels - m_FirstSampleInBuffer;
				__int16 * pWaveSamples = & m_pWaveBuffer[nIndexOfSample];
				int k;
				for (k = 0; k < m_FftOrder * 2; k++, pWaveSamples += nChannels)
				{
					buf[k] = pWaveSamples[0] * m_pFftWindow[k];
				}
				FastFourierTransform(buf, reinterpret_cast<complex<float> *>(buf),
									m_FftOrder * 2);

				for (k = 0; k < m_FftOrder * 2; k += 2)
				{
					pRes--;
					float power = buf[k] * buf[k] + buf[k + 1] * buf[k + 1];
					if (power != 0.)
					{
						// max power= (32768 * m_FftOrder * 2) ^ 2
						int res = m_FftLogRange * (PowerOffset - log(power));
						if (res < 0)
						{
							pRes[0] = 0;
						}
						else if (res > 127)
						{
							pRes[0] = 127;
						}
						else
						{
							pRes[0] = res;
						}
					}
					else
					{
						pRes[0] = 127;
					}
				}
				pRes += m_FftOrder * 2;
			}
			m_pFftResultArray[i] = 1;   // mark as valid
		}
	}
	delete[] buf;
}


/////////////////////////////////////////////////////////////////////////////
// CWaveFftView diagnostics

#ifdef _DEBUG
void CWaveFftView::AssertValid() const
{
	CWaveSoapFrontView::AssertValid();
}

void CWaveFftView::Dump(CDumpContext& dc) const
{
	CWaveSoapFrontView::Dump(dc);
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CWaveFftView message handlers

void CWaveFftView::OnPaint()
{
	CRgn UpdRgn;
	CRgn InvalidRgn;
	CRgn RectToDraw;
	UpdRgn.CreateRectRgn(0, 0, 1, 1);
	if (ERROR != GetUpdateRgn( & UpdRgn, FALSE))
	{
		CRect r;
		UpdRgn.GetRgnBox( & r);
		//TRACE("Update region width=%d\n", r.Width());
		if (r.Width() > 64)
		{
			r.right = r.left + 64;
			RectToDraw.CreateRectRgnIndirect( & r);
			// init the handle
			InvalidRgn.CreateRectRgn(0, 0, 1, 1);
			InvalidRgn.CombineRgn( & UpdRgn, & RectToDraw, RGN_DIFF);
			UpdRgn.CombineRgn( & UpdRgn, & RectToDraw, RGN_AND);
#ifdef _DEBUG
			//InvalidRgn.GetRgnBox( & r);
			//TRACE("Width of new invalid region = %d\n", r.Width());
#endif
			ValidateRect(NULL);
			InvalidateRgn( & UpdRgn, FALSE);    // no erase
		}
	}
	else
	{
		TRACE("No update region !\n");
	}
	// do regular paint on the smaller region
	CWaveSoapFrontView::OnPaint();
	if (NULL != InvalidRgn.m_hObject)
	{
		//TRACE("Invalidating unpainted region\n");
		InvalidateRgn( & InvalidRgn, FALSE);
	}
}

BOOL CWaveFftView::OnEraseBkgnd(CDC* pDC)
{
	return CView::OnEraseBkgnd(pDC);       // we don't need to erase background
//	return CWaveSoapFrontView::OnEraseBkgnd(pDC);
}

BOOL CWaveFftView::PreCreateWindow(CREATESTRUCT& cs)
{
	if (NULL == m_Brush)
	{
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
		try {
			bmp.CreateBitmap(8, 8, 1, 1, pattern);
			CBrush GrayBrush( & bmp);
			m_Brush = (HBRUSH)GrayBrush.Detach();
		}
		catch (CResourceException)
		{
			TRACE("CResourceException\n");
		}
	}
	cs.lpszClass = AfxRegisterWndClass(CS_VREDRAW | CS_DBLCLKS, NULL,
										m_Brush, NULL);
	TRACE("CWaveFftView::PreCreateWindow(CREATESTRUCT)\n");
	return CScaledScrollView::PreCreateWindow(cs);
}

void CWaveFftView::OnViewZoomInVert()
{
	if (m_VerticalScale < 1024.)
	{
		m_VerticalScale	*= sqrt(2.);
		InvalidateRgn(NULL);
		NotifySlaveViews(FFT_SCALE_CHANGED);
	}
}

void CWaveFftView::OnViewZoomOutVert()
{
	// TODO: Add your command handler code here
	if (m_VerticalScale > 1.)
	{
		m_VerticalScale	*= sqrt(0.5);
		if (m_VerticalScale < 1.001)    // compensate any error
		{
			m_VerticalScale = 1.;
		}
		// correct the offset, if necessary
		// find max and min offset for this scale
		double offset = m_WaveOffsetY;
		double MaxOffset = 65536. * (1. - 1. / m_VerticalScale);
		if (offset > MaxOffset)
		{
			offset = MaxOffset;
		}
		double MinOffset = -MaxOffset;
		if (offset < MinOffset)
		{
			offset = MinOffset;
		}
		if (offset != m_WaveOffsetY)
		{
			m_WaveOffsetY = offset;
		}
		Invalidate();
		NotifySlaveViews(FFT_SCALE_CHANGED);
	}
}

BOOL CWaveFftView::ScrollBy(double dx, double dy, BOOL bDoScroll)
{
	if (dx != 0.)
	{
		CScaledScrollView::ScrollBy(dx, 0, bDoScroll);
	}
	if (dy != 0.)
	{
		// check for the limits
		// ndy is in pixels
		int ndy = fround(dy * GetYScaleDev());
		CWaveSoapFrontDoc * pDoc = GetDocument();
		CRect r;
		GetClientRect( & r);
		int nHeight = r.Height() / pDoc->WaveChannels();
		double offset = m_FirstbandVisible + -m_FftOrder * ndy / (nHeight * m_VerticalScale);
		// find max and min offset for this scale
		double MaxOffset = m_FftOrder * (1 - 1. / m_VerticalScale);
		BOOL NoScroll = false;
		if (offset > MaxOffset)
		{
			offset = MaxOffset;
			NoScroll = true;
		}
		if (offset < 0)
		{
			offset = 0;
			NoScroll = true;
		}
		if (offset != m_FirstbandVisible)
		{
			TRACE("New offset = %g, MaxOffset=%g, VerticalScale = %g\n",
				offset, MaxOffset, m_VerticalScale);
			m_FirstbandVisible = offset;
			NotifySlaveViews(FFT_OFFSET_CHANGED);
			// change to scroll
			if (NoScroll)
			{
				Invalidate();
			}
			else
			{
				CWaveSoapFrontDoc * pDoc = GetDocument();
				CRect cr;
				GetClientRect( & cr);
				if (pDoc->WaveChannels() == 1)
				{
					ScrollWindowEx(0, -ndy, NULL, NULL, NULL, NULL,
									SW_INVALIDATE | SW_ERASE);
				}
				else
				{
					CRect cr1 = cr, cr2 = cr, r;
					cr1.bottom = cr.bottom / 2 - 1;
					cr2.top = cr1.bottom + 2;
					// invalidated rects
					CRect ir1, ir2;
					ScrollWindowEx(0, -ndy, & cr1, & cr1, NULL, & ir1,
									SW_INVALIDATE);
					ScrollWindowEx(0, -ndy, & cr2, & cr2, NULL, & ir2,
									SW_INVALIDATE);
					cr2.bottom = cr2.top;
					cr2.top = cr1.bottom;
					InvalidateRect( & cr2);
					InvalidateRect( & ir1);
					InvalidateRect( & ir2);
				}
			}
		}
	}
	return TRUE;
}

void CWaveFftView::OnViewZoomvertNormal()
{
	if (m_VerticalScale != 1.)
	{
		m_VerticalScale = 1.;
		m_FirstbandVisible = 0;
		Invalidate();
		NotifySlaveViews(FFT_SCALE_CHANGED);
	}
}

void CWaveFftView::OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint)
{
	if (lHint == CWaveSoapFrontDoc::UpdateSoundChanged
		&& NULL != pHint
		&& NULL != m_pFftResultArray)
	{
		CSoundUpdateInfo * pInfo = (CSoundUpdateInfo *) pHint;
		CWaveSoapFrontDoc * pDoc = GetDocument();

		// calculate update boundaries
		int nChannels = pDoc->WaveChannels();
		int left = pInfo->Begin;
		int right = pInfo->End;
		int FirstSampleChanged = left - left % m_FftSpacing;
		int LastSampleRequired = right - right % m_FftSpacing + m_FftSpacing;
		if (LastSampleRequired > m_FftResultBegin
			&& FirstSampleChanged < m_FftResultEnd)
		{
			if (FirstSampleChanged < m_FftResultBegin)
			{
				FirstSampleChanged = m_FftResultBegin;
			}
			size_t i = (FirstSampleChanged - m_FftResultBegin) / m_FftSpacing * m_FftResultArrayHeight;
			size_t j = (LastSampleRequired - m_FftResultBegin) / m_FftSpacing * m_FftResultArrayHeight;
			if (j > m_FftArraySize)
			{
				j = m_FftArraySize;
			}
			for ( ; i < j; i += m_FftResultArrayHeight)
			{
				// invalidate the column
				m_pFftResultArray[i] = 0;
			}
		}
		if (pInfo->Length != -1)
		{
			int samples = pInfo->Length;
			samples -= samples % m_FftSpacing;
			if (samples < m_FftResultBegin)
			{
				samples = m_FftResultBegin;
			}

			int i = (samples - m_FftResultBegin) / m_FftSpacing * m_FftResultArrayHeight;
			// invalidate the columns that correspond to the deleted data
			for ( ; i < m_FftArraySize; i += m_FftResultArrayHeight)
			{
				// invalidate the column
				m_pFftResultArray[i] = 0;
			}
		}

	}
	else if (lHint == CWaveSoapFrontDoc::UpdateSelectionChanged
			&& NULL != pHint)
	{
		CSelectionUpdateInfo * pInfo = (CSelectionUpdateInfo *) pHint;
		CWaveSoapFrontDoc * pDoc = GetDocument();

		if (pInfo->Flags & SetSelection_MakeCaretVisible)
		{
			MovePointIntoView(pDoc->m_CaretPosition);
		}
		else if (pInfo->Flags & SetSelection_MoveCaretToCenter)
		{
			MovePointIntoView(pDoc->m_CaretPosition, TRUE);
		}

		int nChannels = GetDocument()->WaveChannels();
		int nLowExtent = -32768;
		int nHighExtent = 32767;
		if (nChannels > 1)
		{
			nLowExtent = -0x10000;
			nHighExtent = 0x10000;
			if (pInfo->SelChannel == 0)
			{
				nLowExtent = 0;
			}
			else if (pInfo->SelChannel == 1)
			{
				nHighExtent = 0;
			}
		}


		ChangeSelection(pDoc->m_SelectionStart, pDoc->m_SelectionEnd,
						nLowExtent, nHighExtent);
		CreateAndShowCaret();
		return; // don't call Wave view OnUpdate in this case
		// we don't want to invalidate
	}
	CWaveSoapFrontView::OnUpdate(pSender, lHint, pHint);
}

HBRUSH CWaveFftView::m_Brush = NULL;

void CWaveFftView::OnUpdateBands(CCmdUI* pCmdUI, int number)
{
	pCmdUI->SetRadio(number == m_FftOrder);
}

void CWaveFftView::OnSetBands(int number)
{
	if (number != m_FftOrder)
	{
		m_FftOrder = number;
		delete[] m_pFftResultArray;
		m_pFftResultArray = NULL;
		delete[] m_pFftWindow;
		m_pFftWindow = NULL;
		Invalidate();
	}
}

void CWaveFftView::OnFftBands1024()
{
	OnSetBands(1024);
}

void CWaveFftView::OnUpdateFftBands1024(CCmdUI* pCmdUI)
{
	OnUpdateBands(pCmdUI, 1024);
}

void CWaveFftView::OnFftBands128()
{
	OnSetBands(128);
}

void CWaveFftView::OnUpdateFftBands128(CCmdUI* pCmdUI)
{
	OnUpdateBands(pCmdUI, 128);
}

void CWaveFftView::OnFftBands2048()
{
	OnSetBands(2048);
}

void CWaveFftView::OnUpdateFftBands2048(CCmdUI* pCmdUI)
{
	OnUpdateBands(pCmdUI, 2048);
}

void CWaveFftView::OnFftBands256()
{
	OnSetBands(256);
}

void CWaveFftView::OnUpdateFftBands256(CCmdUI* pCmdUI)
{
	OnUpdateBands(pCmdUI, 256);
}

void CWaveFftView::OnFftBands4096()
{
	OnSetBands(4096);
}

void CWaveFftView::OnUpdateFftBands4096(CCmdUI* pCmdUI)
{
	OnUpdateBands(pCmdUI, 4096);
}

void CWaveFftView::OnFftBands512()
{
	OnSetBands(512);
}

void CWaveFftView::OnUpdateFftBands512(CCmdUI* pCmdUI)
{
	OnUpdateBands(pCmdUI, 512);
}

void CWaveFftView::OnFftBands64()
{
	OnSetBands(64);
}

void CWaveFftView::OnUpdateFftBands64(CCmdUI* pCmdUI)
{
	OnUpdateBands(pCmdUI, 64);
}

void CWaveFftView::OnFftBands8192()
{
	OnSetBands(8192);
}

void CWaveFftView::OnUpdateFftBands8192(CCmdUI* pCmdUI)
{
	OnUpdateBands(pCmdUI, 8192);
}

UINT CWaveFftView::GetPopupMenuID(CPoint point)
{
	// point is in screen coordinates
	ScreenToClient( & point);
	DWORD hit = ClientHitTest(point);
	if (hit & VSHT_SELECTION)
	{
		return IDR_MENU_WAVE_VIEW_SELECTION;
	}
	else
	{
		return IDR_MENU_FFT_VIEW;
	}
}

