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

SAMPLE_INDEX CWaveFftView::SampleToFftBaseSample(SAMPLE_INDEX sample)
{
	sample += m_FftOrder;
	return sample & -m_FftSpacing;
}

SAMPLE_INDEX CWaveFftView::DisplaySampleToFftBaseSample(SAMPLE_INDEX sample)
{
	sample += m_FftSpacing >> 1;
	return sample & -m_FftSpacing;
}

long CWaveFftView::SampleToFftColumn(SAMPLE_INDEX sample)
{
	return (sample + m_FftOrder) / m_FftSpacing;
}

SAMPLE_INDEX CWaveFftView::FftColumnToDisplaySample(long Column)
{
	return Column * m_FftSpacing - m_FftOrder;
}

void CWaveFftView::FillLogPalette(LOGPALETTE * pal, int nEntries)
{
	pal->palVersion = 0x300;
	unsigned i;
	for (i = 0; i < 10; i++)
	{
		pal->palPalEntry[i].peFlags = PC_EXPLICIT;
		pal->palPalEntry[i].peRed = i;
		pal->palPalEntry[i].peGreen = 0;
		pal->palPalEntry[i].peBlue = 0;
	}

	for (int j = 0; j < sizeof palette && i < nEntries; j += 3, i++)
	{
		pal->palPalEntry[i].peFlags = PC_NOCOLLAPSE;
		pal->palPalEntry[i].peRed = palette[j];
		pal->palPalEntry[i].peGreen = palette[j + 1];
		pal->palPalEntry[i].peBlue = palette[j + 2];
	}
	pal->palNumEntries = (WORD)i;
}

/////////////////////////////////////////////////////////////////////////////
// CWaveFftView

IMPLEMENT_DYNCREATE(CWaveFftView, CWaveSoapFrontView)

CWaveFftView::CWaveFftView()
	: m_pFftResultArray(NULL),
	m_FftResultArrayWidth(0),
	m_FftResultArrayHeight(0),
	m_FftResultBegin(0),
	m_FftLogRange(10. * M_LOG10E),
	m_FirstbandVisible(0),
	m_FftWindowType(WindowTypeNuttall),
	m_IndexOfFftBegin(0),
	m_FftArraySize(0)
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
	ON_COMMAND(ID_VIEW_SS_ZOOMOUTVERT, OnViewZoomOutVert)
	ON_COMMAND(ID_VIEW_SS_ZOOMVERT_NORMAL, OnViewZoomvertNormal)
	ON_UPDATE_COMMAND_UI(ID_VIEW_SS_ZOOMINVERT, OnUpdateViewZoomInVert)
	ON_UPDATE_COMMAND_UI(ID_VIEW_SS_ZOOMOUTVERT, OnUpdateViewZoomOutVert)
	ON_UPDATE_COMMAND_UI(ID_VIEW_SS_ZOOMVERT_NORMAL, OnUpdateViewZoomvertNormal)
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
	// limit right to the file area
	int FileEnd = WorldToWindowXceil(pDoc->WaveFileSamples());

	if (r.right > FileEnd)
	{
		r.right = FileEnd;
	}

	//int iClientWidth = r.right - r.left;
	PointToDoubleDev(CPoint(r.left, cr.top), left, top);
	PointToDoubleDev(CPoint(r.right, cr.bottom), right, bottom);

	if (left < 0) left = 0;
	// create an array of points
	//int nNumberOfPoints = r.right - r.left;

	if (r.left < r.right)
	{

		MakeFftArray(SAMPLE_INDEX(left), SAMPLE_INDEX(right));

		void * pBits;
		bool bUsePalette;
		size_t width = r.right - r.left;
		long height = cr.bottom - cr.top;
		size_t stride = (width * 3 + 3) & ~3;
		size_t BmpSize = stride * ::abs(height);
		int BytesPerPixel = 3;
		struct BM : BITMAPINFO
		{
			RGBQUAD MorebmiColors[256];
		} bmi;
		bmi.bmiHeader.biSize = sizeof BITMAPINFOHEADER;
		bmi.bmiHeader.biWidth = r.right - r.left;
		bmi.bmiHeader.biHeight = cr.top - cr.bottom; // <0
		bmi.bmiHeader.biPlanes = 1;
		bmi.bmiHeader.biCompression = BI_RGB;
		bmi.bmiHeader.biSizeImage = 0;
		bmi.bmiHeader.biXPelsPerMeter = 0;
		bmi.bmiHeader.biYPelsPerMeter = 0;
		bmi.bmiHeader.biClrUsed = 0;
		bmi.bmiHeader.biClrImportant = 0;

		CPushDcPalette OldPalette(pDC, NULL);

		if ((pDC->GetDeviceCaps(RASTERCAPS) & RC_PALETTE)
			&& 8 == pDC->GetDeviceCaps(BITSPIXEL))
		{
			BytesPerPixel = 1;
			bUsePalette = true;
			bmi.bmiHeader.biBitCount = 8;

			int i;
			PALETTEENTRY SysPalette[256];
			GetSystemPaletteEntries(*pDC, 0, 256, SysPalette);
			for (i = 0; i < 10; i++)
			{
				bmi.bmiColors[i].rgbReserved = 0;
				bmi.bmiColors[i].rgbRed = SysPalette[i].peRed;
				bmi.bmiColors[i].rgbGreen = SysPalette[i].peGreen;
				bmi.bmiColors[i].rgbBlue = SysPalette[i].peBlue;
			}
			for (int j = 0; j < sizeof palette && i < 255; j += 3, i++)
			{
				bmi.bmiColors[i].rgbReserved = 0;
				bmi.bmiColors[i].rgbRed = palette[j];
				bmi.bmiColors[i].rgbGreen = palette[j + 1];
				bmi.bmiColors[i].rgbBlue = palette[j + 2];
			}
			for ( ; i < 256; i++)
			{
				bmi.bmiColors[i].rgbReserved = 0;
				bmi.bmiColors[i].rgbRed = SysPalette[i].peRed;
				bmi.bmiColors[i].rgbGreen = SysPalette[i].peGreen;
				bmi.bmiColors[i].rgbBlue = SysPalette[i].peBlue;
			}

			bmi.bmiHeader.biClrUsed = i;
			stride = (width + 3) & ~3;
			BmpSize = stride * ::abs(height);
			OldPalette.PushPalette(GetApp()->GetPalette(), FALSE);
		}
		else
		{
			BytesPerPixel = 3;
			bUsePalette = false;
			bmi.bmiHeader.biBitCount = 24;

			//hbm = CreateDIBSection(pDC->GetSafeHdc(), (LPBITMAPINFO) & bmih, 0, & pBits, NULL, 0);
		}

		CBitmap hbm;
		hbm.Attach(CreateDIBSection(pDC->GetSafeHdc(), & bmi, DIB_RGB_COLORS,
									& pBits, NULL, 0));

		if (HGDIOBJ(hbm) == NULL)
		{
			return;
		}

		// get windowed samples with 50% overlap
		LPBYTE pBmp = LPBYTE(pBits);

		memset(pBmp, 0, BmpSize);

		// fill the array

		NUMBER_OF_CHANNELS nChannels = pDoc->WaveChannels();
		// find offset in the FFT result array for 'left' point
		// and how many columns to fill with this color
		ASSERT(m_FftSpacing >= m_HorizontalScale);

		int ColsPerFftPoint = m_FftSpacing / m_HorizontalScale;
		ASSERT(long(left) + m_FftSpacing / 2 >= m_FftResultBegin);
		ASSERT(long(right) - m_FftSpacing / 2 <= m_FftResultEnd);

		int nFirstCol = (long(left) + m_FftSpacing / 2 - m_FftResultBegin) / m_FftSpacing;

		ASSERT(nFirstCol >= 0);
		int FirstCols = ColsPerFftPoint -
						((long(left) - m_FftResultBegin + m_FftSpacing / 2) % m_FftSpacing) / m_HorizontalScale;

		// find vertical offset in the result array and how many
		// rows to fill with this color
		int rows = cr.Height() / nChannels;
		// if all the chart was drawn, how many scans it would have:
		int TotalRows = int(rows * m_VerticalScale);

		if (0 == TotalRows)
		{
			CScaledScrollView::OnDraw(pDC);
			return;
		}

		int LastFftSample = int(m_FftOrder - m_FirstbandVisible);
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
		if (0) TRACE("LastRow = %d, cr.height=%d\n", LastRow, cr.Height());
		ASSERT(LastRow <= rows);
		int IdxSize = k;

		unsigned char * pColBmp = pBmp;
		int nChanOffset = stride * rows;
		unsigned char * pData = m_pFftResultArray +
								(nFirstCol + m_IndexOfFftBegin) * m_FftResultArrayHeight;

		if (0 != ((long(WindowToWorldX(r.left + FirstCols)) + m_FftSpacing/2) % m_FftSpacing) / m_HorizontalScale)
		{
			TRACE("band boundary=%d\n",
				((long(WindowToWorldX(r.left + FirstCols)) + m_FftSpacing/2) % m_FftSpacing) / m_HorizontalScale);
		}

		int nColumns = FirstCols;
		for(int col = r.left; col < r.right; )
		{
			int ff;
			S * pId;
			if (nColumns > r.right - col)
			{
				nColumns = r.right - col;
			}
			if (pData >= m_pFftResultArray + m_FftArraySize)
			{
				pData -= m_FftArraySize;
			}

			if (pData[0])
			{
				pData++;
				if ( ! bUsePalette)
				{
					for (int ch = 0, nBmpChOffset = 0; ch < nChannels; ch++,
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
				{   // use palette
					for (int ch = 0, nBmpChOffset = 0; ch < nChannels; ch++,
						nBmpChOffset += nChanOffset, pData += m_FftOrder)
					{
						BYTE * pPal = pColBmp + nBmpChOffset;
						if (nColumns != 1)
						{
							for (ff = 0, pId = pIdArray; ff < IdxSize; ff++, pId++)
							{
								unsigned char ColorIndex = 10 + pData[pId->nFftOffset];
								// set the color to pId->nNumOfRows rows
								for (int y = 0; y < pId->nNumOfRows; y++, pPal += stride)
								{
									// set the color to nColumns pixels across
									for (int x = 0; x < nColumns; x++)
									{
										ASSERT(pPal >= pBmp && pPal+x < pBmp + BmpSize);
										pPal[x] = ColorIndex;
									}
								}
							}
						}
						else
						{
							for (ff = 0, pId = pIdArray; ff < IdxSize; ff++, pId++)
							{
								unsigned char ColorIndex = 10 + pData[pId->nFftOffset];
								// set the color to pId->nNumOfRows rows
								for (int y = 0; y < pId->nNumOfRows; y++, pPal += stride)
								{
									// set the color
									ASSERT(pPal >= pBmp && pPal < pBmp + BmpSize);
									pPal[0] = ColorIndex;
								}
							}
						}
					}
				}
			}
			else
			{
				pData += m_FftResultArrayHeight;
				if ( ! bUsePalette)
				{
					for (int ch = 0, nBmpChOffset = 0; ch < nChannels;
						ch++, nBmpChOffset += nChanOffset)
					{
						BYTE * pRgb = pColBmp + nBmpChOffset;
						for (int y = 0; y < rows; y++, pRgb += stride - nColumns * 3)
						{
							// set the color to nColumns pixels across
							for (int x = 0; x < nColumns; x++, pRgb += 3)
							{
								ASSERT(pRgb >= pBmp && pRgb + 3 <= pBmp + BmpSize);
								pRgb[0] = 0;    // B
								pRgb[1] = 0;    // G
								pRgb[2] = 0;    // R
							}
						}
					}

				}
				else
				{   // use palette
					for (int ch = 0, nBmpChOffset = 0; ch < nChannels; ch++,
						nBmpChOffset += nChanOffset)
					{
						BYTE * pPal = pColBmp + nBmpChOffset;
						// set the color to pId->nNumOfRows rows
						for (int y = 0; y < rows; y++, pPal += stride - nColumns)
						{
							// set the color to nColumns pixels across
							for (int x = 0; x < nColumns; x++, pPal ++)
							{
								ASSERT(pPal >= pBmp && pPal < pBmp + BmpSize);
								pPal[0] = 0;
							}
						}
					}
				}
			}
			col += nColumns;

			pColBmp += nColumns * BytesPerPixel;
			nColumns = ColsPerFftPoint;
		}
		delete[] pIdArray;
		// stretch bitmap to output window
		SetDIBitsToDevice(pDC->GetSafeHdc(), r.left, cr.top,
						width, height,
						0, 0, 0, height, pBits, & bmi,
						DIB_RGB_COLORS);

		//this bitmap is used to draw a dashed line
		CBitmap bmp;
		int const DashLength = 4;
		static const unsigned char pattern[] =
		{
			0xFF, 0xFF,  // aligned to WORD
			0xFF, 0xFF,  // aligned to WORD
			0x00, 0,
			0x00, 0,
			0xFF, 0xFF,  // aligned to WORD
			0xFF, 0xFF,  // aligned to WORD
			0x00, 0,
			0x00, 0,
			0xFF, 0xFF,  // aligned to WORD
			0xFF, 0xFF,  // aligned to WORD
			0x00, 0,
			0x00, 0,
		};

		// Windows98 can only use (at least?) 8x8 bitmap for a brush
		bmp.CreateBitmap(8, 8, 1, 1, pattern);

		CBrush DashBrush( & bmp);

		DashBrush.UnrealizeObject();

		int nHeight = cr.Height();

		if (0 != nChannels)
		{
			nHeight /= nChannels;
		}

		int BrushOffset = int(m_FirstbandVisible * (nHeight * m_VerticalScale) / m_FftOrder);

		pDC->SetBrushOrg(0, BrushOffset % DashLength);

		CGdiObjectSaveT<CBrush> OldBrush(pDC, pDC->SelectObject( & DashBrush));
		CGdiObjectSave OldFont(pDC, pDC->SelectStockObject(ANSI_VAR_FONT));

		CThisApp * pApp = GetApp();
		pDC->SetBkColor(pApp->m_WaveBackground);
		pDC->SetTextColor(0xFFFFFF ^ pApp->m_WaveBackground);
		pDC->SetBkMode(OPAQUE);

		CWaveFile::InstanceDataWav * pInst = pDoc->m_WavFile.GetInstanceData();

		for (CuePointVectorIterator i = pInst->m_CuePoints.begin();
			i < pInst->m_CuePoints.end(); i++)
		{
			long x = WorldToWindowXfloor(i->dwSampleOffset);

			// draw text
			if (x < r.right)
			{
				LPCTSTR txt = pInst->GetCueText(i->CuePointID);
				if (NULL != txt)
				{
					int count = _tcslen(txt);
					CPoint size = pDC->GetTextExtent(txt, count);
					if (x + size.x > r.left)
					{
						pDC->DrawText(txt, count, CRect(x, cr.top, x + size.x, cr.top + size.y),
									DT_LEFT | DT_NOPREFIX | DT_SINGLELINE | DT_TOP);
					}
				}
			}
		}

		SAMPLE_INDEX_Vector markers;
		pDoc->m_WavFile.GetSortedMarkers(markers, FALSE);

		for (SAMPLE_INDEX_Vector::const_iterator i = markers.begin();
			i < markers.end(); i++)
		{
			long x = WorldToWindowXfloor( *i);

			if (x >= r.left
				&& x <= r.right)
			{
				// draw marker
				pDC->PatBlt(x, cr.top, 1, cr.bottom - cr.top, PATINVERT);
			}
		}

		GdiFlush(); // make sure bitmap is drawn before deleting it (NT only)
		// free resources
	}
	if (m_PlaybackCursorDrawn)
	{
		DrawPlaybackCursor(pDC, m_PlaybackCursorDrawnSamplePos, m_PlaybackCursorChannel);
	}
	CScaledScrollView::OnDraw(pDC);
}

void CWaveFftView::MakeFftArray(SAMPLE_INDEX left, SAMPLE_INDEX right)
{
	CWaveSoapFrontDoc * pDoc = GetDocument();
	CRect r;
	int NumberOfFftPoints;
	GetClientRect( & r);

	int FftSpacing = m_HorizontalScale;
	if (m_FftOrder <= m_HorizontalScale)
	{
		NumberOfFftPoints = r.Width() + 2;
	}
	else
	{
		FftSpacing = m_FftOrder;
		NumberOfFftPoints = 2 + r.Width() * m_HorizontalScale / m_FftOrder;
	}
	// for each FFT set we keep a byte to mark it valid or invalid
	int NewFftArrayHeight = m_FftOrder * pDoc->WaveChannels() + 1;

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
		TRACE("New FFT array allocated, height=%d, FFT spacing=%d\n",
			NewFftArrayHeight, FftSpacing);

		unsigned char * pTmp = m_pFftResultArray;
		int i;

		SAMPLE_INDEX FirstFftSample = (left + FftSpacing / 2) & -FftSpacing;
		SAMPLE_INDEX FftSample = FirstFftSample;

		for (i = 0; i < NumberOfFftPoints; i++,
			pTmp += NewFftArrayHeight, FftSample += FftSpacing)
		{
			pTmp[0] = 0;    // invalidate
			if (pOldArray != NULL
				&& 0 == FftSample % m_FftSpacing
				&& FftSample >= m_FftResultBegin
				&& FftSample <= m_FftResultEnd)
			{
				long offset = m_IndexOfFftBegin + (FftSample - m_FftResultBegin) / m_FftSpacing;
				offset %= m_FftResultArrayWidth;
				unsigned char * p = pOldArray + offset * NewFftArrayHeight;
				ASSERT(p >= pOldArray && p + NewFftArrayHeight <= pOldArray + m_FftArraySize);
				if (p[0] != 0)
				{
					ASSERT(1 == p[0]);
					memcpy(pTmp, p, NewFftArrayHeight);
				}
			}
		}

		m_FftArraySize = NecessaryArraySize;
		m_FftResultBegin = FirstFftSample;
		m_FftResultEnd = FftSample - FftSpacing;

		m_IndexOfFftBegin = 0;
		delete[] pOldArray;

		m_FftSpacing = FftSpacing;
		m_FftResultArrayHeight = NewFftArrayHeight;
		m_FftResultArrayWidth = NumberOfFftPoints;
	}
	ASSERT(m_FftResultEnd - m_FftResultBegin == (m_FftResultArrayWidth - 1) * m_FftSpacing);

	if (NULL == m_pFftResultArray)
	{
		m_FftArraySize = 0;
		return;
	}
	CalculateFftRange(left, right);
}

void CWaveFftView::CalculateFftRange(SAMPLE_INDEX left, SAMPLE_INDEX right)
{
	CWaveSoapFrontDoc * pDoc = GetDocument();
	// make sure the required range is in the buffer
	// buffer size is enough to hold all data
	SAMPLE_INDEX FirstSampleRequired = DisplaySampleToFftBaseSample(left);

	if (FirstSampleRequired < 0)
	{
		FirstSampleRequired = 0;
	}

	SAMPLE_INDEX LastSampleRequired = DisplaySampleToFftBaseSample(right);

	if (0) TRACE("Samples required from %d to %d, in the buffer: from %d to %d\n",
				FirstSampleRequired, LastSampleRequired, m_FftResultBegin, m_FftResultEnd);

	ASSERT(LastSampleRequired >= FirstSampleRequired);
	ASSERT(m_FftResultEnd >= m_FftResultBegin);
	ASSERT(LastSampleRequired - FirstSampleRequired <= m_FftResultEnd - m_FftResultBegin);
	ASSERT(m_FftResultEnd - m_FftResultBegin == (m_FftResultArrayWidth - 1) * m_FftSpacing);
	ASSERT(m_IndexOfFftBegin < m_FftResultArrayWidth);

	if (FirstSampleRequired < m_FftResultBegin)
	{
		if (LastSampleRequired > m_FftResultBegin)
		{
			// free some space before the calculated
			while (m_FftResultBegin > FirstSampleRequired)
			{
				m_IndexOfFftBegin--;
				if (m_IndexOfFftBegin < 0)
				{
					m_IndexOfFftBegin += m_FftResultArrayWidth;
				}
				m_pFftResultArray[m_IndexOfFftBegin * m_FftResultArrayHeight] = 0;

				m_FftResultBegin -= m_FftSpacing;
				m_FftResultEnd -= m_FftSpacing;

			}
		}
		else
		{
			m_FftResultEnd -= m_FftResultBegin - FirstSampleRequired;
			m_FftResultBegin = FirstSampleRequired;
			TRACE("Cleaning all FFT sets \n");
			for (int i = 0; i < m_FftResultArrayWidth; i++)
			{
				m_pFftResultArray[i * m_FftResultArrayHeight] = 0;
			}
			m_IndexOfFftBegin = 0;
		}
	}
	else if (LastSampleRequired > m_FftResultEnd)
	{
		if (FirstSampleRequired <= m_FftResultEnd)
		{
			// free some space after the calculated
			while (m_FftResultEnd <= LastSampleRequired)
			{
				m_pFftResultArray[m_IndexOfFftBegin * m_FftResultArrayHeight] = 0;

				m_FftResultBegin += m_FftSpacing;
				m_FftResultEnd += m_FftSpacing;

				m_IndexOfFftBegin++;
				if (m_IndexOfFftBegin >= m_FftResultArrayWidth)
				{
					m_IndexOfFftBegin -= m_FftResultArrayWidth;
				}
			}
		}
		else
		{
			m_FftResultBegin += LastSampleRequired - m_FftResultEnd;
			m_FftResultEnd = LastSampleRequired;
			m_IndexOfFftBegin = 0;

			TRACE("Cleaning all FFT sets \n");
			for (int i = 0; i < m_FftResultArrayWidth; i++)
			{
				m_pFftResultArray[i * m_FftResultArrayHeight] = 0;
			}
		}
	}
	// calculate FFT
	if (NULL == (float*)m_pFftWindow)
	{
		TRACE("Calculating FFT window\n");
		m_pFftWindow.Allocate(m_FftOrder * 2);

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
				m_pFftWindow[w] = float(0.9 * (0.54 - 0.46 * cos (X)));
				break;

			case WindowTypeNuttall:
				// Nuttall window:
				// w(n) = 0.355768 - 0.487396*cos(2pn/N) + 0.144232*cos(4pn/N) - 0.012604*cos(6pn/N)
				m_pFftWindow[w] = float(0.355768 - 0.487396*cos(X) + 0.144232*cos(2 * X) - 0.012604*cos(3 * X));
				break;
			}
		}
	}

	typedef double DATA;
	DATA * buf = NULL;
	double PowerOffset = log(65536. * m_FftOrder * 0.31622) * 2.;
#ifdef _DEBUG
	int MaxRes = 0;
	int MinRes = 128;
	int nNewFFtCalculated = 0;
#endif

	int ii = (FirstSampleRequired - m_FftResultBegin) / m_FftSpacing * m_FftResultArrayHeight;
	int j = (LastSampleRequired - m_FftResultBegin) / m_FftSpacing * m_FftResultArrayHeight;

	FirstSampleRequired -= m_FftOrder;

	for (; ii <= j; ii += m_FftResultArrayHeight, FirstSampleRequired += m_FftSpacing)
	{
		int i = (ii + m_IndexOfFftBegin * m_FftResultArrayHeight) % m_FftArraySize;

		if (0 == m_pFftResultArray[i])
		{
			if (NULL == buf)
			{
				buf = new DATA[m_FftOrder * 2 + 2];
				if (NULL == buf)
				{
					break;
				}
			}

			NUMBER_OF_CHANNELS nChannels = pDoc->WaveChannels();
			unsigned char * pRes = & m_pFftResultArray[i + 1 + m_FftOrder];

			if (FirstSampleRequired + m_FftOrder > pDoc->WaveFileSamples()
				|| FirstSampleRequired < 0)
			{
				if (0) TRACE("The required samples from %d to %d are out of the file\n",
							FirstSampleRequired, FirstSampleRequired + m_FftOrder);
				m_pFftResultArray[i] = 1;   // mark as valid
				// make all black
				memset(m_pFftResultArray + i + 1, 127, m_FftOrder * nChannels);
				continue;
			}
#if 0
			if (FirstSampleRequired < 0)
			{
				memset(m_pFftResultArray + 1, 0, m_FftOrder * 2);
			}
			else
#endif
			for (int ch = 0; ch < nChannels; ch++)
			{
				WAVE_SAMPLE * pWaveSamples;

				long NeedSamples = m_FftOrder * 2 * nChannels;

				if (NeedSamples != m_WaveBuffer.GetData( & pWaveSamples,
														ch + FirstSampleRequired * nChannels,
														NeedSamples, this))
				{
					memset(pRes - m_FftOrder, 127, m_FftOrder);
					continue;
				}

				int k;
				for (k = 0; k < m_FftOrder * 2; k++, pWaveSamples += nChannels)
				{
					buf[k] = pWaveSamples[0] * m_pFftWindow[k];
				}
				FastFourierTransform(buf, reinterpret_cast<complex<DATA> *>(buf),
									m_FftOrder * 2);

				for (k = 0; k < m_FftOrder * 2; k += 2)
				{
					pRes--;
					float power = float(buf[k] * buf[k] + buf[k + 1] * buf[k + 1]);
					if (power != 0.)
					{
						// max power= (32768 * m_FftOrder * 2) ^ 2
						int res = int(m_FftLogRange * (PowerOffset - log(power)));
#ifdef _DEBUG
						if (MaxRes < res) MaxRes = res;
						if (MinRes > res) MinRes = res;
#endif
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
							pRes[0] = unsigned char(res);
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
#ifdef _DEBUG
			nNewFFtCalculated++;
#endif
		}
		else
		{
			ASSERT(1 == m_pFftResultArray[i]);
		}
	}
#ifdef _DEBUG
	if (0) TRACE("%d new FFT calculated\n", nNewFFtCalculated);
	if (0) TRACE("MaxRes=%d, MinRes=%d\n", MaxRes, MinRes);
#endif
	delete[] buf;
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
			ValidateRect(NULL);
			InvalidateRgn( & UpdRgn, TRUE);    // no erase
		}
	}
	else
	{
		TRACE("No update region !\n");
	}
	// do regular paint on the smaller region
	BaseClass::OnPaint();
	if (NULL != InvalidRgn.m_hObject)
	{
		//TRACE("Invalidating unpainted region\n");
		InvalidateRgn( & InvalidRgn, TRUE);
	}
}

BOOL CWaveFftView::OnEraseBkgnd(CDC* pDC)
{
// draw checkered background outside wave area
	CWaveSoapFrontDoc * pDoc = GetDocument();

	CRect r;
	GetClientRect( & r);
	int FileEnd = WorldToWindowXceil(pDoc->WaveFileSamples());

	CRect ClipRect;
	pDC->GetClipBox(ClipRect);

	if (FileEnd < ClipRect.right)
	{
		try {
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

			// Windows98 can only use 8x8 bitmap for a brush
			bmp.CreateBitmap(8, 8, 1, 1, pattern);
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
		catch (CResourceException * e)
		{
			TRACE("CResourceException\n");
			e->Delete();
		}
	}
	cs.lpszClass = AfxRegisterWndClass(CS_VREDRAW | CS_DBLCLKS, NULL,
										NULL, NULL);
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
	if (m_VerticalScale > 1.)
	{
		m_VerticalScale	*= sqrt(0.5);
		if (m_VerticalScale < 1.001)    // compensate any error
		{
			m_VerticalScale = 1.;
		}
		// correct the offset, if necessary
		// find max and min offset for this scale
		double offset = m_FirstbandVisible;
		double MaxOffset = m_FftOrder * (1. - 1. / m_VerticalScale);
		if (offset > MaxOffset)
		{
			offset = MaxOffset;
		}
		double MinOffset = -MaxOffset;
		if (offset < MinOffset)
		{
			offset = MinOffset;
		}
		if (offset != m_FirstbandVisible)
		{
			m_FirstbandVisible = offset;
		}
		Invalidate();
		NotifySlaveViews(FFT_SCALE_CHANGED);
	}
}

BOOL CWaveFftView::MasterScrollBy(double dx, double dy, BOOL bDoScroll)
{
	if (dx != 0.)
	{
		BaseClass::MasterScrollBy(dx, 0, bDoScroll);
	}
	if (dy != 0.)
	{
		// check for the limits
		// ndy is in pixels
		int ndy = fround(dy * GetYScaleDev());
		CWaveSoapFrontDoc * pDoc = GetDocument();
		CRect r;
		GetClientRect( & r);
		int nHeight = r.Height();

		NUMBER_OF_CHANNELS nChannels = pDoc->WaveChannels();

		if (0 != nChannels)
		{
			nHeight /= nChannels;
		}

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
				RemoveSelectionRect();

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
				RestoreSelectionRect();
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
	CWaveSoapFrontDoc * pDoc = GetDocument();
	if (lHint == CWaveSoapFrontDoc::UpdateSoundChanged
		&& NULL != pHint
		&& NULL != m_pFftResultArray)
	{
		ASSERT(SAMPLE_INDEX(-1L) < SAMPLE_INDEX(0));

		m_NewSelectionMade = true;
		CSoundUpdateInfo * pInfo = static_cast<CSoundUpdateInfo *>(pHint);

		// calculate update boundaries
		//NUMBER_OF_CHANNELS nChannels = pDoc->WaveChannels();
		SAMPLE_INDEX left = pInfo->m_Begin;
		SAMPLE_INDEX right = pInfo->m_End;

		// find out which samples are affected
		SAMPLE_INDEX FirstSampleChanged = SampleToFftBaseSample(left - m_FftOrder);
		SAMPLE_INDEX LastSampleChanged = SampleToFftBaseSample(right);

		if (FirstSampleChanged < m_FftResultBegin)
		{
			FirstSampleChanged = m_FftResultBegin;
		}
		if (LastSampleChanged > m_FftResultEnd)
		{
			LastSampleChanged = m_FftResultEnd;
		}

		if (LastSampleChanged > m_FftResultBegin
			&& FirstSampleChanged <= m_FftResultEnd)
		{
			for (SAMPLE_INDEX Sample = FirstSampleChanged; Sample <= LastSampleChanged; Sample += m_FftSpacing)
			{
				size_t i = (((Sample - m_FftResultBegin) / m_FftSpacing + m_IndexOfFftBegin)
								* m_FftResultArrayHeight) % m_FftArraySize;
				// invalidate the column
				m_pFftResultArray[i] = 0;
			}
		}
		// process length change
		if (pInfo->m_NewLength != -1)
		{
			NUMBER_OF_SAMPLES sample = SampleToFftBaseSample(pInfo->m_NewLength - m_FftOrder);

			if (sample < m_FftResultBegin)
			{
				sample = m_FftResultBegin;
			}

			// invalidate the columns that correspond to the deleted data
			for (; sample <= m_FftResultEnd; sample += m_FftSpacing)
			{
				// invalidate the column
				size_t i = (((sample - m_FftResultBegin) / m_FftSpacing + m_IndexOfFftBegin)
								* m_FftResultArrayHeight) % m_FftArraySize;
				m_pFftResultArray[i] = 0;
			}
		}

		pInfo->m_Begin = FirstSampleChanged - m_FftSpacing / 2;
		pInfo->m_End = LastSampleChanged + m_FftSpacing / 2;
		BaseClass::OnUpdate(pSender, lHint, pInfo);
		pInfo->m_Begin = left;
		pInfo->m_End = right;
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

		NUMBER_OF_CHANNELS nChannels = pDoc->WaveChannels();
		int nLowExtent = -0x8000;
		int nHighExtent = 0x7FFF;

		if (nChannels > 1)
		{
			ASSERT(nChannels <= 2); // TODO: support multichannel
			nLowExtent = -0x10000;
			nHighExtent = 0x10000;
			if (0 == (pInfo->SelChannel & SPEAKER_FRONT_RIGHT))
			{
				nLowExtent = 0;
			}
			else if (0 == (pInfo->SelChannel & SPEAKER_FRONT_LEFT))
			{
				nHighExtent = 0;
			}
		}

		ChangeSelection(pInfo->SelBegin, pInfo->SelEnd,
						nLowExtent, nHighExtent);
		CreateAndShowCaret();
		return; // don't call Wave view OnUpdate in this case
		// we don't want to invalidate
	}
	else
	{
		BaseClass::OnUpdate(pSender, lHint, pHint);
	}
}

HBRUSH CWaveFftView::m_Brush = NULL;

void CWaveFftView::OnUpdateBands(CCmdUI* pCmdUI, int number)
{
	pCmdUI->SetRadio(number == m_FftOrder);
}

void CWaveFftView::OnSetBands(int order)
{
	int number = 1 << order;
	if (number != m_FftOrder)
	{
		m_FftOrder = number;
		GetApp()->m_FftBandsOrder = order;

		delete[] m_pFftResultArray;
		m_pFftResultArray = NULL;

		m_pFftWindow.Free();

		Invalidate();
		NotifySlaveViews(FFT_BANDS_CHANGED);
	}
}

void CWaveFftView::OnSetWindowType(int window)
{
	if (window != m_FftWindowType)
	{
		m_FftWindowType = window;
		GetApp()->m_FftWindowType = window;

		delete[] m_pFftResultArray;
		m_pFftResultArray = NULL;

		m_pFftWindow.Free();

		Invalidate();
		NotifySlaveViews(FFT_BANDS_CHANGED);
	}
}

void CWaveFftView::OnFftBands1024()
{
	OnSetBands(10);
}

void CWaveFftView::OnUpdateFftBands1024(CCmdUI* pCmdUI)
{
	OnUpdateBands(pCmdUI, 1024);
}

void CWaveFftView::OnFftBands128()
{
	OnSetBands(7);
}

void CWaveFftView::OnUpdateFftBands128(CCmdUI* pCmdUI)
{
	OnUpdateBands(pCmdUI, 128);
}

void CWaveFftView::OnFftBands2048()
{
	OnSetBands(11);
}

void CWaveFftView::OnUpdateFftBands2048(CCmdUI* pCmdUI)
{
	OnUpdateBands(pCmdUI, 2048);
}

void CWaveFftView::OnFftBands256()
{
	OnSetBands(8);
}

void CWaveFftView::OnUpdateFftBands256(CCmdUI* pCmdUI)
{
	OnUpdateBands(pCmdUI, 256);
}

void CWaveFftView::OnFftBands4096()
{
	OnSetBands(12);
}

void CWaveFftView::OnUpdateFftBands4096(CCmdUI* pCmdUI)
{
	OnUpdateBands(pCmdUI, 4096);
}

void CWaveFftView::OnFftBands512()
{
	OnSetBands(9);
}

void CWaveFftView::OnUpdateFftBands512(CCmdUI* pCmdUI)
{
	OnUpdateBands(pCmdUI, 512);
}

void CWaveFftView::OnFftBands64()
{
	OnSetBands(6);
}

void CWaveFftView::OnUpdateFftBands64(CCmdUI* pCmdUI)
{
	OnUpdateBands(pCmdUI, 64);
}

void CWaveFftView::OnFftBands8192()
{
	OnSetBands(13);
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

void CWaveFftView::DrawSelectionRect(CDC * pDC,
									double left, double right, double bottom, double top)
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
	CRect r(DoubleToPoint(left, bottom),
			DoubleToPoint(right, top));
	r.NormalizeRect();
	CRect r0(0, 0, 0, 0);
	CSize size;
	size.cx = GetSystemMetrics(SM_CXSIZEFRAME);
	size.cy = GetSystemMetrics(SM_CYSIZEFRAME);

	pDrawDC->DrawDragRect( & r, size, & r0, size, NULL, NULL);

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
