// WaveSoapFrontView.cpp : implementation of the CWaveSoapFrontView class
//

#include "stdafx.h"
#include "WaveSoapFront.h"

#include "WaveSoapFrontDoc.h"
#include "WaveSoapFrontView.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CWaveSoapFrontView

IMPLEMENT_DYNCREATE(CWaveSoapFrontView, CScaledGraphView)

BEGIN_MESSAGE_MAP(CWaveSoapFrontView, CScaledGraphView)
	//{{AFX_MSG_MAP(CWaveSoapFrontView)
		// NOTE - the ClassWizard will add and remove mapping macros here.
		//    DO NOT EDIT what you see in these blocks of generated code!
	//}}AFX_MSG_MAP
	// Standard printing commands
	ON_COMMAND(ID_FILE_PRINT, CScaledGraphView::OnFilePrint)
	ON_COMMAND(ID_FILE_PRINT_DIRECT, CScaledGraphView::OnFilePrint)
	ON_COMMAND(ID_FILE_PRINT_PREVIEW, CScaledGraphView::OnFilePrintPreview)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CWaveSoapFrontView construction/destruction

CWaveSoapFrontView::CWaveSoapFrontView()
	: m_HorizontalScale(2048),
	m_FirstSampleInBuffer(0),
	m_pWaveBuffer(NULL),
	m_WaveBufferSize(0),
	m_WaveDataSizeInBuffer(0)
{
	// TODO: add construction code here
	TRACE("CWaveSoapFrontView::CWaveSoapFrontView()\n");
}

CWaveSoapFrontView::~CWaveSoapFrontView()
{
	TRACE("CWaveSoapFrontView::~CWaveSoapFrontView()\n");
	if (NULL != m_pWaveBuffer)
	{
		delete[] m_pWaveBuffer;
		m_pWaveBuffer = NULL;
	}
}

BOOL CWaveSoapFrontView::PreCreateWindow(CREATESTRUCT& cs)
{
	// TODO: Modify the Window class or styles here by modifying
	//  the CREATESTRUCT cs
	TRACE("CWaveSoapFrontView::PreCreateWindow(CREATESTRUCT)\n");

	return CScaledGraphView::PreCreateWindow(cs);
}

/////////////////////////////////////////////////////////////////////////////
// CWaveSoapFrontView drawing

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

void CWaveSoapFrontView::OnDraw(CDC* pDC)
{
	CWaveSoapFrontDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);
	// TODO: add draw code for native data here

	// draw bands boundaries with light grey
	// draw instant frequency trail in each band (unless the level is too low)
	CPen BlackPen;
	CPen GrayPen;
	CPen GrayDottedPen;
	if (pDC->GetDeviceCaps(TECHNOLOGY) == DT_RASDISPLAY)
	{
		COLORREF crText = GetSysColor(COLOR_WINDOWTEXT);
		COLORREF crBackground = GetSysColor(COLOR_WINDOW);
		COLORREF crGray = RGB(
							(GetRValue(crText) + GetRValue(crBackground)) / 2,
							(GetGValue(crText) + GetGValue(crBackground)) / 2,
							(GetBValue(crText) + GetBValue(crBackground)) / 2);
		COLORREF crLtGray = RGB(
								(GetRValue(crText) + GetRValue(crBackground) * 2) / 3,
								(GetGValue(crText) + GetGValue(crBackground) * 2) / 3,
								(GetBValue(crText) + GetBValue(crBackground) * 2) / 3);
		if (pDC->GetDeviceCaps(BITSPIXEL) < 8)
		{
			GrayPen.CreatePen(PS_DASH, 1, crText);
			BlackPen.CreatePen(PS_SOLID, 1, crText);
			GrayDottedPen.CreatePen(PS_DOT, 1, crText);
		}
		else
		{
			GrayPen.CreatePen(PS_SOLID, 1, crLtGray);
			BlackPen.CreatePen(PS_SOLID, 1, crText);
			GrayDottedPen.CreatePen(PS_DOT, 1, crGray);
		}
	}
	else
	{
		// all pens are black
		GrayPen.CreatePen(PS_DASH, 1, RGB(0, 0, 0));
		BlackPen.CreatePen(PS_SOLID, 1, RGB(0, 0, 0));
		GrayDottedPen.CreatePen(PS_DOT, 1, RGB(0, 0, 0));
	}
	CGdiObject * pOldPen = pDC->SelectObject(& GrayPen);
	RECT r;
	double y;
	double left, right, top, bottom;

	GetClientRect(&r);
	if (0 && pDC->IsKindOf(RUNTIME_CLASS(CPaintDC)))
	{
		RECT r_upd = ((CPaintDC*)pDC)->m_ps.rcPaint;
#if 0
		CString s;
		s.Format("r_upd: l=%d, r=%d, t=%d, b=%d\n", r_upd.left,
				r_upd.right, r_upd.top, r_upd.bottom);
		AfxOutputDebugString(s);
#endif
		// make intersect by x coordinate
		if (r.left < r_upd.left) r.left = r_upd.left;
		if (r.right > r_upd.right) r.right = r_upd.right;
	}

	int iClientWidth = r.right - r.left;
	PointToDoubleDev(CPoint(r.left, r.top), left, top);
	PointToDoubleDev(CPoint(r.right, r.bottom), right, bottom);
	// number of sample that corresponds to the r.left position
	int NumOfFirstSample = DWORD(left);
	int SamplesPerPoint = m_HorizontalScale;
	// TODO: add draw code here

	// draw 0 line
	pDC->SelectObject(& GrayDottedPen);
	if (top > 0. && bottom < 0.)
	{
		pDC->MoveTo(DoubleToPoint(left, 0.));
		pDC->LineTo(DoubleToPoint(right, 0.));
	}

	// draw the graph
	// create an array of points

	if (left < 0.) left = 0.;
	//POINT LeftPoint = DoubleToPointDev(left, 0);
	//int iSamplesCount = int(right) - int(left);
	// draw the graph
	// create an array of points
	// allocate the array for the view bitmap

	int nNumberOfPoints = r.right - r.left;
	if (nNumberOfPoints > 0)
	{
		POINT (* ppArray)[2] = new POINT[nNumberOfPoints][2];

		int i;
		if (SamplesPerPoint >= pDoc->m_PeakDataGranularity)
		{
			// use peak data for drawing
			DWORD PeakSamplesPerPoint =
				SamplesPerPoint / pDoc->m_PeakDataGranularity;
			int nChannels = pDoc->m_WavFile.m_pWf->nChannels;

			int nIndexOfPeak =
				(NumOfFirstSample / pDoc->m_PeakDataGranularity) * nChannels;
			WavePeak * pPeaks = & pDoc->m_pPeaks[nIndexOfPeak];
			double YScaleDev = GetYScaleDev();
			for (i = 0; i < nNumberOfPoints; i++)
			{
				int low = 0x7FFF;
				int high = -0x8000;
				if (NULL != pDoc->m_pPeaks
					&& pPeaks < pDoc->m_pPeaks + pDoc->m_WavePeakSize)
				{
					if (pPeaks >= pDoc->m_pPeaks)
					{
						for (int j = 0; j < PeakSamplesPerPoint; j++, pPeaks+= nChannels)
						{
							if (pPeaks >= pDoc->m_pPeaks + pDoc->m_WavePeakSize)
							{
								break;
							}
							if (high < pPeaks->high)
							{
								high = pPeaks->high;
							}
							if (low > pPeaks->low)
							{
								low = pPeaks->low;
							}
						}
					}
					else
					{
						pPeaks += PeakSamplesPerPoint * nChannels;
					}
				}

				ppArray[i][0] = CPoint(i + r.left, fround((low - dOrgY) * YScaleDev));
				ppArray[i][1] = CPoint(i + r.left, fround((high - dOrgY) * YScaleDev));
			}
		}
		else
		{
			// use wave data for drawing
			int nChannels = pDoc->m_WavFile.m_pWf->nChannels;

			GetWaveSamples(NumOfFirstSample * nChannels, nNumberOfPoints * SamplesPerPoint * nChannels);

			int nIndexOfSample =
				NumOfFirstSample * nChannels - m_FirstSampleInBuffer;
			double YScaleDev = GetYScaleDev();
			__int16 * pWaveSamples = & m_pWaveBuffer[nIndexOfSample];
			for (i = 0; i < nNumberOfPoints; i++)
			{
				int low = 0x7FFF;
				int high = -0x8000;
				if (NULL != m_pWaveBuffer
					&& pWaveSamples < m_pWaveBuffer + m_WaveBufferSize)
				{
					if (pWaveSamples >= m_pWaveBuffer)
					{
						for (int j = 0; j < SamplesPerPoint; j++, pWaveSamples+= nChannels)
						{
							if (pWaveSamples >= m_pWaveBuffer + m_WaveBufferSize)
							{
								break;
							}
							if (high < pWaveSamples[0])
							{
								high = pWaveSamples[0];
							}
							if (low > pWaveSamples[0])
							{
								low = pWaveSamples[0];
							}
						}
					}
					else
					{
						pWaveSamples += SamplesPerPoint * nChannels;
					}
				}

				ppArray[i][0] = CPoint(i + r.left, fround((low - dOrgY) * YScaleDev));
				ppArray[i][1] = CPoint(i + r.left, fround((high - dOrgY) * YScaleDev));
			}
		}

		pDC->SelectObject(& BlackPen);
		// draw by 256 points
		// make sure the graph is continuous
		int LastY0 = ppArray[0][0].y;
		int LastY1 = ppArray[0][1].y;
		for (i = 0; i < nNumberOfPoints; i ++)
		{
			if (ppArray[i][0].y >= ppArray[i][1].y)
			{
				ppArray[i][0].y++;
				if (i < nNumberOfPoints - 1)
				{
					if (ppArray[i][0].y < ppArray[i + 1][1].y)
					{
						ppArray[i][0].y = (ppArray[i + 1][1].y + ppArray[i][0].y) / 2;
					}
					else if (ppArray[i + 1][0].y < ppArray[i][1].y)
					{
						ppArray[i][1].y = (ppArray[i][1].y + ppArray[i + 1][0].y) / 2;
					}
				}

				if (ppArray[i][0].y < LastY1)
				{
					ppArray[i][0].y = LastY1;
				}
				else if (ppArray[i][1].y > LastY0)
				{
					ppArray[i][1].y = LastY0;
				}

				LastY0 = ppArray[i][0].y;
				LastY1 = ppArray[i][1].y;

				if (ppArray[i][0].y < r.top)
				{
					ppArray[i][0].y = r.top;
				}
				else if (ppArray[i][0].y > r.bottom)
				{
					ppArray[i][0].y = r.bottom;
				}

				if (ppArray[i][1].y < r.top)
				{
					ppArray[i][1].y = r.top;
				}
				else if (ppArray[i][1].y > r.bottom)
				{
					ppArray[i][1].y = r.bottom;
				}
				pDC->MoveTo(ppArray[i][0]);
				pDC->LineTo(ppArray[i][1]);
			}
		}
		delete[] ppArray;
	}
	pDC->SelectObject(pOldPen);
	CScaledScrollView::OnDraw(pDC);
}

// NumOfSamples - number of 16-bit words needed
// Position - offset in data chunk in 16-bit words
void CWaveSoapFrontView::GetWaveSamples(int Position, int NumOfSamples)
{
	CWaveSoapFrontDoc * pDoc = GetDocument();
	if (NULL == pDoc)
	{
		return;
	}

	if (NULL == m_pWaveBuffer
		|| NumOfSamples > m_WaveBufferSize)
	{
		// reallocate the buffer
		if (NULL != m_pWaveBuffer)
		{
			delete[] m_pWaveBuffer;
			m_pWaveBuffer = NULL;
		}
		m_pWaveBuffer = new __int16[NumOfSamples];
		if (NULL == m_pWaveBuffer)
		{
			return;
		}
		m_WaveBufferSize = NumOfSamples;
		m_WaveDataSizeInBuffer = 0;
	}
	//check if we can reuse some of the data in the buffer
	// use Position, NumOfSamples, m_FirstSampleInBuffer, m_WaveDataSizeInBuffer
	if (Position + NumOfSamples <= m_FirstSampleInBuffer
		|| Position >= m_FirstSampleInBuffer + m_WaveDataSizeInBuffer)
	{
		// none of the data in the buffer can be reused
		m_FirstSampleInBuffer = Position;
		m_WaveDataSizeInBuffer = 0;
	}

	if (Position < m_FirstSampleInBuffer)
	{
		// move data up
		int MoveBy = m_FirstSampleInBuffer - Position;
		if (m_WaveDataSizeInBuffer + MoveBy > m_WaveBufferSize)
		{
			m_WaveDataSizeInBuffer = m_WaveBufferSize - MoveBy;
		}
		memmove(m_pWaveBuffer + MoveBy, m_pWaveBuffer,
				m_WaveDataSizeInBuffer * sizeof(__int16));
		m_WaveDataSizeInBuffer += MoveBy;
		ASSERT(m_WaveDataSizeInBuffer <= m_WaveBufferSize);
		pDoc->m_WavFile.ReadAt(m_pWaveBuffer,
								MoveBy * sizeof(__int16),
								pDoc->m_WavFile.m_datack.dwDataOffset + Position * sizeof(__int16));
		m_FirstSampleInBuffer = Position;
	}

	if (Position + NumOfSamples >
		m_FirstSampleInBuffer + m_WaveDataSizeInBuffer)
	{
		// move data down
		int MoveBy = Position + NumOfSamples -
					(m_FirstSampleInBuffer + m_WaveBufferSize);
		if (MoveBy > 0)
		{
			ASSERT(m_WaveDataSizeInBuffer > MoveBy);
			m_WaveDataSizeInBuffer -= MoveBy;
			memmove(m_pWaveBuffer, m_pWaveBuffer + MoveBy,
					m_WaveDataSizeInBuffer * sizeof(__int16));
			m_FirstSampleInBuffer += MoveBy;
		}
		// adjust NumOfSamples:
		NumOfSamples -= m_FirstSampleInBuffer + m_WaveDataSizeInBuffer - Position;
		Position = m_FirstSampleInBuffer + m_WaveDataSizeInBuffer;
		ASSERT(m_WaveDataSizeInBuffer + NumOfSamples <= m_WaveBufferSize);
		NumOfSamples = pDoc->m_WavFile.ReadAt(m_pWaveBuffer + m_WaveDataSizeInBuffer,
											NumOfSamples * sizeof(__int16),
											pDoc->m_WavFile.m_datack.dwDataOffset + Position * sizeof(__int16))
						/ sizeof(__int16);
		m_WaveDataSizeInBuffer += NumOfSamples;
	}
	else
	{
		// all requested data is already in the buffer
	}
	return;
}

void CWaveSoapFrontView::AdjustNewScale(double OldScaleX, double OldScaleY,
										double & NewScaleX, double & NewScaleY)
{
	m_HorizontalScale = 1. / NewScaleX;
	if (m_HorizontalScale < 1)
	{
		m_HorizontalScale = 1;
	}
	// check that the scale is power of 2
	int i;
	for (i = 0; i < 17; i++)
	{
		if (m_HorizontalScale <= (1L << i))
		{
			break;
		}
	}
	m_HorizontalScale = 1L <<i;
	NewScaleX = 1. / m_HorizontalScale;
	TRACE("Old scale X=%g, New scale X=%g, Old scale Y=%g, New scale Y=%g\n",
		OldScaleX, NewScaleX, OldScaleY, NewScaleY);
}
/////////////////////////////////////////////////////////////////////////////
// CWaveSoapFrontView printing

BOOL CWaveSoapFrontView::OnPreparePrinting(CPrintInfo* pInfo)
{
	// default preparation
	return DoPreparePrinting(pInfo);
}

void CWaveSoapFrontView::OnBeginPrinting(CDC* /*pDC*/, CPrintInfo* /*pInfo*/)
{
	// TODO: add extra initialization before printing
}

void CWaveSoapFrontView::OnEndPrinting(CDC* /*pDC*/, CPrintInfo* /*pInfo*/)
{
	// TODO: add cleanup after printing
}

/////////////////////////////////////////////////////////////////////////////
// CWaveSoapFrontView diagnostics

#ifdef _DEBUG
void CWaveSoapFrontView::AssertValid() const
{
	CScaledGraphView::AssertValid();
}

void CWaveSoapFrontView::Dump(CDumpContext& dc) const
{
	CScaledGraphView::Dump(dc);
}

CWaveSoapFrontDoc* CWaveSoapFrontView::GetDocument() // non-debug version is inline
{
	ASSERT(m_pDocument->IsKindOf(RUNTIME_CLASS(CWaveSoapFrontDoc)));
	return (CWaveSoapFrontDoc*)m_pDocument;
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CWaveSoapFrontView message handlers

void CWaveSoapFrontView::OnInitialUpdate()
{
	CScaledGraphView::OnInitialUpdate();

	KeepAspectRatio(FALSE);
	KeepScaleOnResizeX(TRUE);
	KeepScaleOnResizeY(FALSE);
	KeepOrgOnResizeX(TRUE);
	KeepOrgOnResizeY(FALSE);
	// TODO: Add your specialized code here and/or call the base class
	CRect r;
	GetClientRect( r);
	SetMaxExtents(0., GetDocument()->WaveFileSamples(), -32768., 32768.);
	SetExtents(0., double(r.Width()) * m_HorizontalScale, -32768., 32768.);
}
