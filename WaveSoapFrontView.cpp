// WaveSoapFrontView.cpp : implementation of the CWaveSoapFrontView class
//

#include "stdafx.h"
#include "resource.h"
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

IMPLEMENT_DYNCREATE(CWaveSoapFrontView, CScaledScrollView)

BEGIN_MESSAGE_MAP(CWaveSoapFrontView, CScaledScrollView)
	//{{AFX_MSG_MAP(CWaveSoapFrontView)
	ON_UPDATE_COMMAND_UI(ID_VIEW_ZOOMINHOR, OnUpdateViewZoominhor)
	ON_UPDATE_COMMAND_UI(ID_VIEW_ZOOMINHOR2, OnUpdateViewZoominhor2)
	ON_COMMAND(ID_VIEW_ZOOMINVERT, OnViewZoomInVert)
	ON_COMMAND(ID_VIEW_ZOOMOUTVERT, OnViewZoomOutVert)
	ON_UPDATE_COMMAND_UI(ID_VIEW_ZOOMINVERT, OnUpdateViewZoomInVert)
	ON_UPDATE_COMMAND_UI(ID_VIEW_ZOOMOUTVERT, OnUpdateViewZoomOutVert)
	ON_WM_SETCURSOR()
	ON_WM_KILLFOCUS()
	ON_WM_SETFOCUS()
	ON_WM_ERASEBKGND()
	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONUP()
	ON_WM_MOUSEWHEEL()
	ON_WM_MOUSEMOVE()
	ON_WM_KEYDOWN()
	ON_WM_CREATE()
	ON_WM_SIZE()
	ON_COMMAND(ID_VIEW_ZOOMVERT_NORMAL, OnViewZoomvertNormal)
	ON_UPDATE_COMMAND_UI(ID_VIEW_ZOOMVERT_NORMAL, OnUpdateViewZoomvertNormal)
	ON_COMMAND(ID_VIEW_ZOOMIN_HOR_FULL, OnViewZoominHorFull)
	ON_UPDATE_COMMAND_UI(ID_VIEW_ZOOMIN_HOR_FULL, OnUpdateViewZoominHorFull)
	//}}AFX_MSG_MAP
	// Standard printing commands
	ON_COMMAND(ID_FILE_PRINT, CScaledScrollView::OnFilePrint)
	ON_COMMAND(ID_FILE_PRINT_DIRECT, CScaledScrollView::OnFilePrint)
	ON_COMMAND(ID_FILE_PRINT_PREVIEW, CScaledScrollView::OnFilePrintPreview)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CWaveSoapFrontView construction/destruction

CWaveSoapFrontView::CWaveSoapFrontView()
	: m_HorizontalScale(2048),
	m_VerticalScale(1.),
	m_WaveOffsetY(0.),
	m_FirstSampleInBuffer(0),
	m_pWaveBuffer(NULL),
	m_WaveBufferSize(0),
	m_PlaybackCursorChannel(-1),
	m_PlaybackCursorDrawn(false),
	m_PlaybackCursorDrawnSamplePos(0),
	m_WaveDataSizeInBuffer(0)
{
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
	// Modify the Window class or styles here by modifying
	//  the CREATESTRUCT cs
	cs.lpszClass = AfxRegisterWndClass(CS_VREDRAW | CS_DBLCLKS, NULL, NULL, NULL);
	TRACE("CWaveSoapFrontView::PreCreateWindow(CREATESTRUCT)\n");
	return CScaledScrollView::PreCreateWindow(cs);
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

void CWaveSoapFrontView::DrawHorizontalWithSelection(CDC * pDC,
													int left, int right, int Y,
													CPen * NormalPen, CPen * SelectedPen,
													int nChannel)
{
	CWaveSoapFrontDoc * pDoc = GetDocument();
	// find positions of the selection start and and
	// and check whether the selected area is visible
	double XScaleDev = GetXScaleDev();
	int SelectionLeft = WorldToWindowX(pDoc->m_SelectionStart);
	int SelectionRight = WorldToWindowX(pDoc->m_SelectionEnd);
	if (nChannel != 2
		&& pDoc->m_SelectedChannel != 2
		&& nChannel != pDoc->m_SelectedChannel)
	{
		// don't draw selection
		SelectionLeft = right;
		SelectionRight = right;
	}

	if (pDoc->m_SelectionEnd != pDoc->m_SelectionStart
		&& SelectionRight == SelectionLeft)
	{
		SelectionRight++;
	}
	if (SelectionLeft < left)
	{
		SelectionLeft = left;
	}
	if (SelectionRight > right)
	{
		SelectionRight = right;
	}
	pDC->MoveTo(left, Y);
	if (SelectionLeft >= right
		|| SelectionRight == SelectionLeft)
	{
		// no selection visible
		pDC->SelectObject(NormalPen);
		pDC->LineTo(right, Y);
	}
	else
	{
		if (SelectionLeft > left)
		{
			pDC->SelectObject(NormalPen);
			pDC->LineTo(SelectionLeft, Y);
		}
		pDC->SelectObject(SelectedPen);
		pDC->LineTo(SelectionRight, Y);

		if (SelectionRight < right)
		{
			pDC->SelectObject(NormalPen);
			pDC->LineTo(right, Y);
		}
	}
}

void CWaveSoapFrontView::OnDraw(CDC* pDC)
{
	TRACE("OnDraw\n");
	CWaveSoapFrontDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);
	// TODO: add draw code for native data here
	// pen to draw the waveform
	CPen WaveformPen;
	CPen SelectedWaveformPen;
	// pen to draw zero level
	CPen ZeroLinePen;
	CPen SelectedZeroLinePen;
	// pen do draw 6dB line
	CPen SixDBLinePen;
	CPen SelectedSixDBLinePen;
	// pen to draw left/right channel separator
	CPen ChannelSeparatorPen;
	CPen SelectedChannelSeparatorPen;

	CWaveSoapFrontApp * pApp = (CWaveSoapFrontApp *)AfxGetApp();
	//if (pDC->GetDeviceCaps(TECHNOLOGY) == DT_RASDISPLAY)
	//{
	WaveformPen.CreatePen(PS_SOLID, 1, pApp->m_WaveColor);
	SelectedWaveformPen.CreatePen(PS_SOLID, 1, pApp->m_SelectedWaveColor);
	ZeroLinePen.CreatePen(PS_SOLID, 1, pApp->m_ZeroLineColor);
	SelectedZeroLinePen.CreatePen(PS_SOLID, 1, pApp->m_SelectedZeroLineColor);
	SixDBLinePen.CreatePen(PS_SOLID, 1, pApp->m_6dBLineColor);
	SelectedSixDBLinePen.CreatePen(PS_SOLID, 1, pApp->m_Selected6dBLineColor);
	ChannelSeparatorPen.CreatePen(PS_SOLID, 1, pApp->m_ChannelSeparatorColor);
	SelectedChannelSeparatorPen.CreatePen(PS_SOLID, 1, pApp->m_SelectedChannelSeparatorColor);
	//}
	CGdiObject * pOldPen = pDC->SelectObject(& ZeroLinePen);
	RECT r;
	//double y;
	double left, right, top, bottom;

	GetClientRect(&r);
	if (pDC->IsKindOf(RUNTIME_CLASS(CPaintDC)))
	{
		RECT r_upd = ((CPaintDC*)pDC)->m_ps.rcPaint;
		// make intersect by x coordinate
		if (r.left < r_upd.left) r.left = r_upd.left;
		if (r.right > r_upd.right) r.right = r_upd.right;
	}

	r.left--;   // make additional
	r.right++;
	int iClientWidth = r.right - r.left;
	PointToDoubleDev(CPoint(r.left, r.top), left, top);
	PointToDoubleDev(CPoint(r.right, r.bottom), right, bottom);
	// number of sample that corresponds to the r.left position
	int NumOfFirstSample = DWORD(left);
	int SamplesPerPoint = m_HorizontalScale;

	// create an array of points

	if (left < 0.) left = 0.;

	int nChannels = pDoc->WaveChannels();
	int nNumberOfPoints = r.right - r.left;
	int ChannelSeparatorY = fround((0 - dOrgY) * GetYScaleDev());
	double YScaleDev = GetYScaleDev();
	int SelBegin = WorldToWindowX(pDoc->m_SelectionStart);
	int SelEnd = WorldToWindowX(pDoc->m_SelectionEnd);
	if (pDoc->m_SelectionEnd != pDoc->m_SelectionStart
		&& SelEnd == SelBegin)
	{
		SelEnd++;
	}
	if (nChannels > 1)
	{
		// draw channel separator line
		DrawHorizontalWithSelection(pDC, r.left, r.right,
									ChannelSeparatorY,
									& ChannelSeparatorPen,
									& SelectedChannelSeparatorPen, 2);
	}
	if (nNumberOfPoints > 0)
	{
		POINT (* ppArray)[2] = new POINT[nNumberOfPoints][2];
		if (ppArray)
			for (int ch = 0; ch < nChannels; ch++)
			{
				double WaveOffset = m_WaveOffsetY * m_VerticalScale - dOrgY;
				int ClipHigh = r.bottom;
				int ClipLow = r.top;
				if (nChannels > 1)
				{
					if (0 == ch)
					{
						WaveOffset = m_WaveOffsetY * m_VerticalScale + 32768. - dOrgY;
						ClipHigh = ChannelSeparatorY;
					}
					else
					{
						WaveOffset = m_WaveOffsetY * m_VerticalScale -32768. - dOrgY;
						ClipLow = ChannelSeparatorY + 1;
					}
				}

				int ZeroLinePos = fround(WaveOffset * YScaleDev);
				if (ZeroLinePos >= ClipLow &&
					ZeroLinePos < ClipHigh)
				{
					DrawHorizontalWithSelection(pDC, r.left, r.right,
												ZeroLinePos,
												& ZeroLinePen,
												& SelectedZeroLinePen, ch);
				}
				int n6DBLine = fround((16384.* m_VerticalScale + WaveOffset) * YScaleDev);
				if (n6DBLine >= ClipLow &&
					n6DBLine < ClipHigh)
				{
					DrawHorizontalWithSelection(pDC, r.left, r.right,
												n6DBLine,
												& SixDBLinePen,
												& SelectedSixDBLinePen, ch);
				}
				n6DBLine = fround((-16384.* m_VerticalScale + WaveOffset) * YScaleDev);
				if (n6DBLine >= ClipLow &&
					n6DBLine < ClipHigh)
				{
					DrawHorizontalWithSelection(pDC, r.left, r.right,
												n6DBLine,
												& SixDBLinePen,
												& SelectedSixDBLinePen, ch);
				}

				int i;
				if (SamplesPerPoint >= pDoc->m_PeakDataGranularity)
				{
					// use peak data for drawing
					CSimpleCriticalSectionLock lock(pDoc->m_PeakLock);
					DWORD PeakSamplesPerPoint =
						SamplesPerPoint / pDoc->m_PeakDataGranularity;

					int nIndexOfPeak =
						ch + (NumOfFirstSample / pDoc->m_PeakDataGranularity) * nChannels;
					WavePeak * pPeaks = & pDoc->m_pPeaks[nIndexOfPeak];
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

						ppArray[i][0] = CPoint(i + r.left,
												fround((low * m_VerticalScale + WaveOffset) * YScaleDev));
						ppArray[i][1] = CPoint(i + r.left,
												fround((high * m_VerticalScale + WaveOffset) * YScaleDev));
					}
				}
				else
				{
					// use wave data for drawing

					GetWaveSamples(NumOfFirstSample * nChannels, nNumberOfPoints * SamplesPerPoint * nChannels);

					int nIndexOfSample =
						ch + NumOfFirstSample * nChannels - m_FirstSampleInBuffer;
					__int16 * pWaveSamples = & m_pWaveBuffer[nIndexOfSample];
					for (i = 0; i < nNumberOfPoints; i++)
					{
						int low = 0x7FFF;
						int high = -0x8000;
						if (NULL != m_pWaveBuffer
							&& pWaveSamples < m_pWaveBuffer + m_WaveDataSizeInBuffer)
						{
							if (pWaveSamples >= m_pWaveBuffer)
							{
								for (int j = 0; j < SamplesPerPoint; j++, pWaveSamples+= nChannels)
								{
									if (pWaveSamples >= m_pWaveBuffer + m_WaveDataSizeInBuffer)
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

						ppArray[i][0] = CPoint(i + r.left,
												fround((low * m_VerticalScale + WaveOffset) * YScaleDev));
						ppArray[i][1] = CPoint(i + r.left,
												fround((high * m_VerticalScale + WaveOffset) * YScaleDev));
					}
				}

				pDC->SelectObject(& WaveformPen);
				// draw by 256 points
				// make sure the graph is continuous
				int LastY0 = ppArray[0][0].y;
				int LastY1 = ppArray[0][1].y;
				CPen * pLastPen = NULL;
				for (i = 0; i < nNumberOfPoints; i ++)
				{
					if (ppArray[i][0].y >= ppArray[i][1].y)
					{
						ppArray[i][1].y--;
						if (i < nNumberOfPoints - 1
							&& ppArray[i + 1][0].y >= ppArray[i + 1][1].y)
						{
							if (ppArray[i][0].y < ppArray[i + 1][1].y)
							{
								ppArray[i][0].y = (ppArray[i + 1][1].y + ppArray[i][0].y) >> 1;
							}
							else if (ppArray[i + 1][0].y < ppArray[i][1].y)
							{
								ppArray[i][1].y = (ppArray[i][1].y + ppArray[i + 1][0].y) >> 1;
							}
						}
						if (LastY0 >= LastY1)
						{
							if (ppArray[i][0].y < LastY1)
							{
								ppArray[i][0].y = LastY1;
							}
							else if (ppArray[i][1].y > LastY0)
							{
								ppArray[i][1].y = LastY0;
							}
						}

						LastY0 = ppArray[i][0].y;
						LastY1 = ppArray[i][1].y;

						if (ppArray[i][0].y < ClipLow)
						{
							continue;
						}
						else if (ppArray[i][0].y > ClipHigh)
						{
							ppArray[i][0].y = ClipHigh;
						}

						if (ppArray[i][1].y < ClipLow)
						{
							ppArray[i][1].y = ClipLow-1;
						}
						else if (ppArray[i][1].y > ClipHigh)
						{
							continue;
						}
						CPen * pPenToDraw = & WaveformPen;
						if (ppArray[i][0].x >= SelBegin
							&& ppArray[i][0].x < SelEnd
							&& (2 == pDoc->m_SelectedChannel
								|| ch == pDoc->m_SelectedChannel))
						{
							pPenToDraw = & SelectedWaveformPen;
						}
						if (pPenToDraw != pLastPen)
						{
							pDC->SelectObject(pPenToDraw);
							pLastPen = pPenToDraw;
						}
						pDC->MoveTo(ppArray[i][0]);
						pDC->LineTo(ppArray[i][1]);
					}
				}
			}
		delete[] ppArray;
	}
	pDC->SelectObject(pOldPen);
	if (m_PlaybackCursorDrawn)
	{
		DrawPlaybackCursor(pDC, m_PlaybackCursorDrawnSamplePos, m_PlaybackCursorChannel);
	}
	CScaledScrollView::OnDraw(pDC);
}

// NumOfSamples - number of 16-bit words needed
// Position - offset in data chunk in 16-bit words
void CWaveSoapFrontView::GetWaveSamples(int Position, int NumOfSamples)
{
	if (Position < 0)
	{
		Position = 0;
	}
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
	int nTotalWaveFileSamples = pDoc->WaveDataChunk()->cksize / sizeof(__int16);
	if (Position >= nTotalWaveFileSamples)
	{
		m_WaveDataSizeInBuffer = 0;
		return;
	}
	if (Position + NumOfSamples > nTotalWaveFileSamples)
	{
		NumOfSamples = nTotalWaveFileSamples - Position;
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
								pDoc->WaveDataChunk()->dwDataOffset + Position * sizeof(__int16));
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
											pDoc->WaveDataChunk()->dwDataOffset + Position * sizeof(__int16))
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
	//NewScaleY = OldScaleY;  // vertical scale never changes

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

BOOL CWaveSoapFrontView::PlaybackCursorVisible()
{
	int pos = WorldToWindowX(m_PlaybackCursorDrawnSamplePos);
	CRect r;
	GetClientRect( & r);
	if (pos < r.left || pos >= r.right)
	{
		// not in the view;
		return FALSE;
	}
	else
	{
		return TRUE;
	}
}

void CWaveSoapFrontView::DrawPlaybackCursor(CDC * pDC, long Sample, int Channel)
{
	CDC * pDrawDC = pDC;
	if ( ! IsWindowVisible())
	{
		return;
	}
	int pos = WorldToWindowX(Sample);
	CRect r;
	GetClientRect( & r);
	if (pos < r.left || pos >= r.right)
	{
		// not in the view;
		return;
	}
	if (NULL == pDrawDC)
	{
		pDrawDC = GetDC();
		if (NULL == pDrawDC)
		{
			return;
		}
		pDrawDC->ExcludeUpdateRgn(this);
	}
	int OldMap = pDrawDC->SetMapMode(MM_TEXT);
	int OldRop = pDrawDC->SetROP2(R2_XORPEN);
	CPen pen(PS_SOLID, 0, 0xFFFFFF);
	CGdiObject * pOldPen = pDrawDC->SelectObject( & pen);
	// looks like the display driver can delay the drawing with
	// WHITE_PEN, it is causing jerkiness in the playback cursor
	//CGdiObject * pOldPen = pDrawDC->SelectStockObject(WHITE_PEN);
	if (0 == Channel)
	{
		r.bottom /= 2;
	}
	else if (1 == Channel)
	{
		r.top = r.bottom / 2;
	}

	pDrawDC->MoveTo(CPoint(pos, r.top));
	pDrawDC->LineTo(CPoint(pos, r.bottom));
	//TRACE("Cursor drawn  at %d, time=%d\n", pos, timeGetTime());

	pDrawDC->SetROP2(OldRop);
	pDrawDC->SetMapMode(OldMap);

	pDrawDC->SelectObject(pOldPen);
	if (NULL == pDC)
	{
		ReleaseDC(pDrawDC);
	}
}

void CWaveSoapFrontView::ShowPlaybackCursor(CDC * pDC)
{
	if (GetDocument()->m_PlayingSound
		&& IsWindowVisible()
		&& ! m_PlaybackCursorDrawn)
	{
		//TRACE("Cursor drawn  at %d, time=%d\n", m_PlaybackCursorDrawnSamplePos, timeGetTime());
		DrawPlaybackCursor(pDC, m_PlaybackCursorDrawnSamplePos, m_PlaybackCursorChannel);
		m_PlaybackCursorDrawn = true;
	}
}

void CWaveSoapFrontView::HidePlaybackCursor(CDC * pDC)
{
	if (m_PlaybackCursorDrawn)
	{
		//TRACE("Cursor hidden at %d, time=%d\n", m_PlaybackCursorDrawnSamplePos, timeGetTime());
		DrawPlaybackCursor(pDC, m_PlaybackCursorDrawnSamplePos, m_PlaybackCursorChannel);
		m_PlaybackCursorDrawn = false;
	}
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
	CScaledScrollView::AssertValid();
}

void CWaveSoapFrontView::Dump(CDumpContext& dc) const
{
	CScaledScrollView::Dump(dc);
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
	TRACE("OnInitialUpdate style = %08X\n", GetStyle());

	CScaledScrollView::OnInitialUpdate();
	UpdateScrollbars();
	TRACE("OnInitialUpdate final style = %08X\n", GetStyle());
}

void CWaveSoapFrontView::OnUpdateViewZoominhor(CCmdUI* pCmdUI)
{
	pCmdUI->Enable(m_HorizontalScale > 1);
}

void CWaveSoapFrontView::OnUpdateViewZoominhor2(CCmdUI* pCmdUI)
{
	pCmdUI->Enable(m_HorizontalScale > 1);
}

void CWaveSoapFrontView::OnViewZoomInVert()
{
	if (m_VerticalScale < 1024.)
	{
		m_VerticalScale	*= sqrt(2.);
		InvalidateRgn(NULL);
		NotifySlaveViews(WAVE_SCALE_CHANGED);
	}
}

void CWaveSoapFrontView::OnViewZoomOutVert()
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
		NotifySlaveViews(WAVE_SCALE_CHANGED);
	}
}

void CWaveSoapFrontView::OnUpdateViewZoomInVert(CCmdUI* pCmdUI)
{
	pCmdUI->Enable(m_VerticalScale < 1024.);
}

void CWaveSoapFrontView::OnUpdateViewZoomOutVert(CCmdUI* pCmdUI)
{
	pCmdUI->Enable(m_VerticalScale > 1.);
}

// return client hit test code. 'p' is in client coordinates
DWORD CWaveSoapFrontView::ClientHitTest(CPoint p)
{
	CWaveSoapFrontDoc * pDoc = GetDocument();
	DWORD result = 0;
	CRect r;
	GetClientRect( & r);
	if ( ! r.PtInRect(p))
	{
		return VSHT_NONCLIENT;
	}

	int Separator = WorldToWindowY(0.);
	if (pDoc->m_SelectionStart <= pDoc->m_SelectionEnd
		&& (pDoc->WaveChannels() == 1
			|| 2 == pDoc->m_SelectedChannel
			|| (0 == pDoc->m_SelectedChannel) == (p.y < Separator)))
	{
		int SelBegin = WorldToWindowX(pDoc->m_SelectionStart);
		int SelEnd = WorldToWindowX(pDoc->m_SelectionEnd);
		if (pDoc->m_SelectionEnd != pDoc->m_SelectionStart
			&& SelEnd == SelBegin)
		{
			SelEnd++;
		}
		int BorderWidth = GetSystemMetrics(SM_CXEDGE);
		// check whether the cursor is on the selection boundary
		if (p.x >= SelBegin - BorderWidth
			&& p.x < SelBegin + BorderWidth)
		{
			result |= VSHT_SEL_BOUNDARY_L;
		}
		if (p.x >= SelEnd - BorderWidth
			&& p.x < SelEnd + BorderWidth)
		{
			result |= VSHT_SEL_BOUNDARY_R;
		}
		if (p.x >= SelBegin && p.x < SelEnd)
		{
			result |= VSHT_SELECTION;
		}
	}

	int nCursorOverChannel = 2;
	int DataEnd = WorldToWindowX(pDoc->WaveFileSamples());
	if (p.x < DataEnd)
	{
		if (pDoc->WaveChannels() > 1)
		{
			if (p.y < ((32768. - dOrgY) * GetYScaleDev()))
			{
				result |= VSHT_LEFT_CHAN;
			}
			else if (p.y > ((-32768. - dOrgY) * GetYScaleDev()))
			{
				result |= VSHT_RIGHT_CHAN;
			}
			else
			{
				result |= VSHT_BCKGND;
			}
		}
		else
		{
			result |= VSHT_BCKGND;
		}
	}
	else
	{
		result |= VSHT_NOWAVE;
	}
	return result;
}

BOOL CWaveSoapFrontView::OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message)
{
	if (pWnd == this
		&& HTCLIENT == nHitTest)
	{
		CPoint p;

		GetCursorPos( & p);
		ScreenToClient( & p);

		DWORD ht = ClientHitTest(p);

		if ((ht & (VSHT_SEL_BOUNDARY_L | VSHT_SEL_BOUNDARY_R))
			|| WM_LBUTTONDOWN == nKeyPressed)
		{
			SetCursor(AfxGetApp()->LoadStandardCursor(IDC_SIZEWE));
			return TRUE;
		}

		if (ht & (VSHT_SELECTION | VSHT_NOWAVE))
		{
			SetCursor(AfxGetApp()->LoadStandardCursor(IDC_ARROW));
			return TRUE;
		}

		if (ht & VSHT_LEFT_CHAN)
		{
			SetCursor(AfxGetApp()->LoadCursor(IDC_CURSOR_BEAM_LEFT));
			return TRUE;
		}

		if (ht & VSHT_RIGHT_CHAN)
		{
			SetCursor(AfxGetApp()->LoadCursor(IDC_CURSOR_BEAM_RIGHT));
			return TRUE;
		}

		if (ht & VSHT_BCKGND)
		{
			SetCursor(AfxGetApp()->LoadCursor(IDC_CURSOR_BEAM));
			return TRUE;
		}
	}

	return CScaledScrollView::OnSetCursor(pWnd, nHitTest, message);
}

void CWaveSoapFrontView::OnKillFocus(CWnd* pNewWnd)
{
	DestroyCaret();
	CScaledScrollView::OnKillFocus(pNewWnd);

}

void CWaveSoapFrontView::OnSetFocus(CWnd* pOldWnd)
{
	CScaledScrollView::OnSetFocus(pOldWnd);
	CreateAndShowCaret();
}

void CWaveSoapFrontView::CreateAndShowCaret()
{
	// create caret
	if (this != GetFocus())
	{
		return;
	}
	CWaveSoapFrontDoc * pDoc = GetDocument();
	CRect r;
	GetClientRect( & r);

	CPoint p(WorldToWindowX(pDoc->m_CaretPosition), r.top);
	TRACE("Client rect height=%d, caret position=%d\n", r.Height(), p.x);
	if (2 == pDoc->m_SelectedChannel || pDoc->WaveChannels() == 1)
	{
		CreateSolidCaret(1, r.Height());
	}
	else
	{
		CreateSolidCaret(1, r.Height() / 2);
		if (1 == pDoc->m_SelectedChannel)
		{
			p.y += r.Height() / 2;
		}
	}
	if (p.x >= 0 && p.x < r.right)
	{
		SetCaretPos(p);
		ShowCaret();
	}
}

BOOL CWaveSoapFrontView::OnEraseBkgnd(CDC* pDC)
{
	// TODO: Add your message handler code here and/or call default
	CWaveSoapFrontApp * pApp = (CWaveSoapFrontApp *) AfxGetApp();
	CWaveSoapFrontDoc * pDoc = GetDocument();
	RemoveSelectionRect();
	CBrush backBrush(pApp->m_WaveBackground);
	CRect r;
	GetClientRect( & r);
	CRect gr = r;
	int SelBegin = WorldToWindowX(pDoc->m_SelectionStart);
	int SelEnd = WorldToWindowX(pDoc->m_SelectionEnd);
	int FileEnd = WorldToWindowX(pDoc->WaveFileSamples());
	if (FileEnd < r.right)
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
			r.right = FileEnd;
			gr.left = FileEnd;
			pDC->FillRect(gr, & GrayBrush);
		}
		catch (CResourceException)
		{
			TRACE("CResourceException\n");
		}
	}
	if (pDoc->m_SelectionEnd != pDoc->m_SelectionStart
		&& SelEnd == SelBegin)
	{
		SelEnd++;
	}
	if (SelBegin >= r.right
		|| SelEnd < r.left
		|| pDoc->m_SelectionStart >= pDoc->m_SelectionEnd)
	{
		// erase using only one brush
		pDC->FillRect(r, & backBrush);
	}
	else
	{
		if (SelBegin > r.left)
		{
			CRect r1 = r;
			r1.right = SelBegin;
			pDC->FillRect(r1, & backBrush);
		}
		else
		{
			SelBegin = r.left;
		}

		if (SelEnd < r.right)
		{
			CRect r1 = r;
			r1.left = SelEnd;
			pDC->FillRect(r1, & backBrush);
		}
		else
		{
			SelEnd = r.right;
		}
		r.left = SelBegin;
		r.right = SelEnd;
		CBrush SelectedBackBrush(pApp->m_SelectedWaveBackground);
		if (2 == pDoc->m_SelectedChannel || pDoc->WaveChannels() == 1)
		{
			pDC->FillRect(r, & SelectedBackBrush);
		}
		else
		{
			// only one channel is selected
			int Separator = WorldToWindowY(0.);
			CRect r1 = r;
			CRect r2 = r;
			if (0 == pDoc->m_SelectedChannel)
			{
				r1.bottom = Separator;
				r2.top = Separator;
			}
			else
			{
				r2.bottom = Separator;
				r1.top = Separator;
			}
			pDC->FillRect(r1, & SelectedBackBrush);
			pDC->FillRect(r2, & backBrush);
		}
	}
	return TRUE;
}

void CWaveSoapFrontView::OnLButtonDown(UINT nFlags, CPoint point)
{
	// begin tracking
	// point is in client coordinates
	CWaveSoapFrontDoc * pDoc = GetDocument();
	DWORD nHit = ClientHitTest(point);
	if (nHit & VSHT_NONCLIENT)
	{
		return;
	}
	int nSampleUnderMouse = WindowToWorldX(point.x);
	int SelectionStart = pDoc->m_SelectionStart;
	int SelectionEnd = pDoc->m_SelectionEnd;

	if (nSampleUnderMouse < 0)
	{
		nSampleUnderMouse = 0;
	}
	if (nSampleUnderMouse > pDoc->WaveFileSamples())
	{
		nSampleUnderMouse = pDoc->WaveFileSamples();
	}

	if (pDoc->m_TimeSelectionMode)
	{
		CView::OnLButtonDown(nFlags, point);
		nKeyPressed = WM_LBUTTONDOWN;
		if ((nFlags & MK_SHIFT)
			|| (nHit & (VSHT_SEL_BOUNDARY_L | VSHT_SEL_BOUNDARY_R)))
		{
			if (nSampleUnderMouse <
				(double(SelectionStart) + SelectionEnd) / 2)
			{
				SelectionStart = nSampleUnderMouse;
			}
			else
			{
				SelectionEnd = nSampleUnderMouse;
			}
		}
		else
		{
			SelectionStart = nSampleUnderMouse;
			SelectionEnd = SelectionStart;
		}
		int nChan = 2;
		if (nHit & VSHT_LEFT_CHAN)
		{
			nChan = 0;
		}
		else if (nHit & VSHT_RIGHT_CHAN)
		{
			nChan = 1;
		}
		pDoc->SetSelection(SelectionStart, SelectionEnd, nChan, nSampleUnderMouse);
		OnSetCursor(this, HTCLIENT, WM_LBUTTONDOWN);
	}
	else
	{
		CScaledScrollView::OnLButtonDown(nFlags, point);
	}
}

void CWaveSoapFrontView::OnLButtonUp(UINT nFlags, CPoint point)
{
	// TODO: Add your message handler code here and/or call default
	if (GetDocument()->m_TimeSelectionMode)
	{
		ReleaseCapture();
		bIsTrackingSelection = FALSE;
		nKeyPressed = 0;

		CView::OnLButtonUp(nFlags, point);
	}
	else
	{
		CScaledScrollView::OnLButtonUp(nFlags, point);
	}
}

BOOL CWaveSoapFrontView::OnMouseWheel(UINT nFlags, short zDelta, CPoint pt)
{
	// TODO: Add your message handler code here and/or call default

	return CScaledScrollView::OnMouseWheel(nFlags, zDelta, pt);
}

void CWaveSoapFrontView::OnMouseMove(UINT nFlags, CPoint point)
{
	// point is in client coordinates
	CWaveSoapFrontDoc * pDoc = GetDocument();
	DWORD nHit = ClientHitTest(point);
	CRect r;
	GetClientRect( & r);
	if (nHit & (VSHT_NOWAVE | VSHT_NONCLIENT))
	{
		if (point.x < r.left)
		{
			point.x = r.left;
		}
		if (point.x > r.right)
		{
			point.x = r.right;
		}
	}

	int nSampleUnderMouse = WindowToWorldX(point.x);
	if (nSampleUnderMouse < 0)
	{
		nSampleUnderMouse = 0;
	}
	if (nSampleUnderMouse > pDoc->WaveFileSamples())
	{
		nSampleUnderMouse = pDoc->WaveFileSamples();
	}
	int SelectionStart = pDoc->m_SelectionStart;
	int SelectionEnd = pDoc->m_SelectionEnd;

	if (pDoc->m_TimeSelectionMode)
	{
		CView::OnMouseMove(nFlags, point);
		if (nKeyPressed != 0)
		{
			if (bIsTrackingSelection)
			{
			}
			else
			{
				//CancelSelection();
				bIsTrackingSelection = TRUE;
				SetCapture();
			}

			// tracked side (where the caret is) is moved,
			// other side stays
			if (SelectionStart == pDoc->m_CaretPosition)
			{
				SelectionStart = nSampleUnderMouse;
			}
			else if (SelectionEnd == pDoc->m_CaretPosition)
			{
				SelectionEnd = nSampleUnderMouse;
			}
			else if (nSampleUnderMouse <
					(double(SelectionStart) + SelectionEnd) / 2)
			{
				SelectionStart = nSampleUnderMouse;
			}
			else
			{
				SelectionEnd = nSampleUnderMouse;
			}

			int nChan = 2;
			if (nHit & (VSHT_NOWAVE | VSHT_NONCLIENT))
			{
				// don't change the channel
				nChan = pDoc->m_SelectedChannel;
			}
			else if (nHit & VSHT_LEFT_CHAN)
			{
				nChan = 0;
			}
			else if (nHit & VSHT_RIGHT_CHAN)
			{
				nChan = 1;
			}
			pDoc->SetSelection(SelectionStart, SelectionEnd, nChan, nSampleUnderMouse);
		}
	}
	else
	{

		CScaledScrollView::OnMouseMove(nFlags, point);
	}
}

void CWaveSoapFrontView::OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint)
{
	if (lHint == CWaveSoapFrontDoc::UpdateSelectionChanged
		&& NULL != pHint)
	{
		CSelectionUpdateInfo * pInfo = (CSelectionUpdateInfo *) pHint;
		CWaveSoapFrontDoc * pDoc = GetDocument();
		CRect r;
		if (pInfo->m_bMakeCaretVisible)
		{
			MovePointIntoView(pDoc->m_CaretPosition);
		}

		GetClientRect( & r);
		int Separator = WorldToWindowY(0.);

		// calculate new selection boundaries
		int SelBegin = WorldToWindowX(pDoc->m_SelectionStart);
		int SelEnd = WorldToWindowX(pDoc->m_SelectionEnd);
		if (pDoc->m_SelectionEnd != pDoc->m_SelectionStart
			&& SelEnd == SelBegin)
		{
			SelEnd++;
		}

		int SelTop = r.top;
		int SelBottom = r.bottom;
		if (2 != pDoc->m_SelectedChannel
			&& pDoc->WaveChannels() != 1)
		{
			if (0 == pDoc->m_SelectedChannel)
			{
				SelBottom = Separator;
			}
			else
			{
				SelTop = Separator;
			}
		}

		// calculate old selection boundaries
		int OldSelTop = r.top;
		int OldSelBottom = r.bottom;
		if (2 != pInfo->SelChannel
			&& pDoc->WaveChannels() != 1)
		{
			if (0 == pInfo->SelChannel)
			{
				OldSelBottom = Separator;
			}
			else
			{
				OldSelTop = Separator;
			}
		}

		int OldSelBegin = WorldToWindowX(pInfo->SelBegin);
		int OldSelEnd = WorldToWindowX(pInfo->SelEnd);
		if (pInfo->SelEnd != pInfo->SelBegin
			&& OldSelEnd == OldSelBegin)
		{
			OldSelEnd++;
		}

		// build rectangles with selection boundaries
		CRect r1(SelBegin, SelTop, SelEnd, SelBottom);
		CRect r2(OldSelBegin, OldSelTop, OldSelEnd, OldSelBottom);
		// invalidate the regions with changed selection
		if (pInfo->SelChannel == pDoc->m_SelectedChannel)
		{
			// the same channel, different region
			// sort all 'x' coordinates
			int x[4] = {SelBegin, OldSelBegin, SelEnd, OldSelEnd };
			if (SelBegin > OldSelBegin)
			{
				x[0] = OldSelBegin;
				x[1] = SelBegin;
			}
			if (SelEnd > OldSelEnd)
			{
				x[2] = OldSelEnd;
				x[3] = SelEnd;
			}
			if (x[1] > x[2])
			{
				int tmp = x[1];
				x[1] = x[2];
				x[2] = tmp;
			}
			r1.left = x[0];
			r1.right = x[1];
			r2.left = x[2];
			r2.right = x[3];
			if (x[1] == x[2])
			{
				r2.left = x[0];
				r1.right = x[0];    // make empty
			}
		}

		// invalidate two rectangles
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
			InvalidateRect(& r1);
		}
		if (r2.left != r2.right
			// limit the rectangles with the window boundaries
			&& r2.left < r.right
			&& r2.right > r.left)
		{
			// non-empty, in the client
			if (r2.left < r.left)
			{
				r2.left = r.left;
			}
			if(r2.right > r.right)
			{
				r2.right = r.right;
			}
			InvalidateRect(& r2);
		}
		CreateAndShowCaret();
	}
	else if (lHint == CWaveSoapFrontDoc::UpdateSoundChanged
			&& NULL != pHint)
	{
		m_WaveDataSizeInBuffer = 0; // invalidate the data in draw buffer
		CSoundUpdateInfo * pInfo = (CSoundUpdateInfo *) pHint;
		CWaveSoapFrontDoc * pDoc = GetDocument();
		CRect r;
		GetClientRect( & r);
		CRect r1;

		r1.top = r.top;
		r1.bottom = r.bottom;
		if (pInfo->Length != -1)
		{
			// length changed, set new extents and caret position
			int nLowExtent = -32768;
			int nHighExtent = 32767;
			if (GetDocument()->WaveChannels() > 1)
			{
				nLowExtent = -0x10000;
				nHighExtent = 0x10000;
			}
			SetMaxExtents(0, pInfo->Length, nLowExtent, nHighExtent);
			Invalidate();
		}

		// calculate update boundaries
		r1.left = WorldToWindowX(pInfo->Begin);
		r1.right = 1+WorldToWindowX(pInfo->End);

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
			InvalidateRect(& r1, TRUE);
		}
	}
	else if (lHint == CWaveSoapFrontDoc::UpdatePlaybackPositionChanged
			&& NULL != pHint)
	{
		CSoundUpdateInfo * pInfo = (CSoundUpdateInfo *) pHint;
		UpdatePlaybackCursor(pInfo->Begin, pInfo->End);
	}
	else
	{
		CScaledScrollView::OnUpdate(pSender, lHint, pHint);
	}
}

void CWaveSoapFrontView::InvalidateRect( LPCRECT lpRect, BOOL bErase)
{
	HideCaret();
	CScaledScrollView::InvalidateRect(lpRect, bErase);
	ShowCaret();
}

POINT CWaveSoapFrontView::GetZoomCenter()
{
	CWaveSoapFrontDoc * pDoc = GetDocument();
	int caret = WorldToWindowX(pDoc->m_CaretPosition);
	int SelBegin = WorldToWindowX(pDoc->m_SelectionStart);
	int SelEnd = WorldToWindowX(pDoc->m_SelectionEnd);
	//int CenterY = WorldToWindowY(0);
	CRect r;
	GetClientRect( & r);
	// if both selection start and end are
	// outside the visible area, zoom center is the window center
	if ((caret < r.left
			|| caret >= r.right)
		&& (SelBegin < r.left
			|| SelBegin >= r.right)
		&& (SelBegin < r.left
			|| SelBegin >= r.right))
	{
		return CPoint(INT_MAX, INT_MAX);
	}
	// if there is no selection
	if (pDoc->m_SelectionStart == pDoc->m_SelectionEnd)
	{
		return CPoint(caret, INT_MAX);
	}
	// if both boundaries of the selection are in the client area,
	// center it
	if (SelBegin >= r.left
		&& SelBegin < r.right)
	{
		if (SelEnd >= r.left
			&& SelEnd < r.right)
		{
			return CPoint((SelBegin + SelEnd) / 2, INT_MAX);
		}
		else
		{
			// only SelBegin is in the client area
			return CPoint(SelBegin, INT_MAX);
		}
	}
	else
	{
		return CPoint(SelEnd, INT_MAX);
	}
}


void CWaveSoapFrontView::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	// TODO: Add your message handler code here and/or call default
	// process cursor control commands
	CWaveSoapFrontDoc * pDoc = GetDocument();

	int nCaret = pDoc->m_CaretPosition;
	int nSelBegin = pDoc->m_SelectionStart;
	int nSelEnd = pDoc->m_SelectionEnd;
	int nChan = pDoc->m_SelectedChannel;

	BOOL KeepSelection = (0 != (0x8000 & GetKeyState(VK_SHIFT)));
	BOOL CtrlPressed = (0 != (0x8000 & GetKeyState(VK_CONTROL)));
	BOOL MakeCaretVisible = TRUE;

	int nCaretMove = m_HorizontalScale;

	// ctrl+arrow moves by 32 pixels
	if (CtrlPressed)
	{
		nCaretMove *= 32;
	}

	int nTotalSamples = pDoc->WaveFileSamples();

	CRect r;
	GetClientRect( & r);

	// page is one half of the window width
	int nPage = r.Width() * m_HorizontalScale / 2;
	if (CtrlPressed)
	{
		// make it 7/8 of the window width
		nPage = nPage * 7 * m_HorizontalScale / 4;
	}
	// round to one pixel
	nPage -= nPage % m_HorizontalScale;

	switch (nChar)
	{
	case VK_LEFT:
		// move caret 1 pixel or 32 pixels to the left,
		nCaret -= nCaretMove;
		break;
	case VK_RIGHT:
		// move caret 1 or 32 to the right
		nCaret += nCaretMove;
		break;
	case VK_HOME:
		// move to the begin of file or selection
		if (nSelBegin < nSelEnd)
		{
			KeepSelection = TRUE;
			nCaret = nSelBegin;
		}
		else if (nSelBegin > nSelEnd)
		{
			KeepSelection = TRUE;
			nCaret = nSelEnd;
		}
		else if (CtrlPressed)
		{
			nCaret = 0;
		}
		else
		{
			nCaret = WindowToWorldX(r.left + 1); // cursor to the left boundary + 1
		}

		break;
	case VK_END:
		// move to the end of file or selection
		if (nSelBegin < nSelEnd)
		{
			KeepSelection = TRUE;
			nCaret = nSelEnd;
		}
		else if (nSelBegin > nSelEnd)
		{
			KeepSelection = TRUE;
			nCaret = nSelBegin;
		}
		else if (CtrlPressed)
		{
			nCaret = nTotalSamples;
		}
		else
		{
			nCaret = WindowToWorldX(r.right - 2); // cursor to the right boundary + 1
		}
		break;
	case VK_PRIOR:
		nCaret -= nPage;
		break;
	case VK_NEXT:
		nCaret += nPage;
		break;
	case VK_UP:
		// move the zoomed image up
		break;
	case VK_DOWN:
		// move the zoomed image down
		break;
	case VK_TAB:
		// toggle selection channel
		if (pDoc->WaveChannels() > 1)
		{
			nChan = (nChan + 1) % 3;
		}
		KeepSelection = TRUE;
		MakeCaretVisible = FALSE;
		break;
	default:
		// default action
		CView::OnKeyDown(nChar, nRepCnt, nFlags);
		return;

		break;
	}
	if (nCaret < 0)
	{
		nCaret = 0;
	}
	if (nCaret > nTotalSamples)
	{
		nCaret = nTotalSamples;
	}
	if (KeepSelection)
	{
		if (nCaret <
			(double(nSelBegin) + nSelEnd) / 2)
		{
			nSelBegin = nCaret;
		}
		else
		{
			nSelEnd = nCaret;
		}
	}
	else
	{
		nSelBegin = nCaret;
		nSelEnd = nCaret;
	}

	MovePointIntoView(nCaret);

	pDoc->SetSelection(nSelBegin, nSelEnd, nChan, nCaret);

	CView::OnKeyDown(nChar, nRepCnt, nFlags);
}

void CWaveSoapFrontView::MovePointIntoView(int nCaret)
{
	CRect r;
	GetClientRect( & r);

	int nDesiredPos = WorldToWindowX(nCaret);
	double scroll;
	if (nDesiredPos < r.left)
	{
		scroll = (nDesiredPos - r.left) * m_HorizontalScale;
	}
	else if (nDesiredPos >= r.right)
	{
		scroll = (nDesiredPos - r.right + 1) * m_HorizontalScale;
	}
	else
	{
		return;
	}
	TRACE("MovePointIntoView: DesiredPos=%d, left=%d, right=%d, scroll=%d\n",
		nDesiredPos, r.left, r.right, scroll);
	ScrollBy(scroll, 0, TRUE);
	NotifySlaveViews(CHANGE_HOR_ORIGIN);
	CreateAndShowCaret();
}

void CWaveSoapFrontView::UpdateCaretPosition()
{
	CreateAndShowCaret();
}

void CWaveSoapFrontView::OnActivateView(BOOL bActivate, CView* pActivateView, CView* pDeactiveView)
{
	CWaveSoapFrontDoc * pDoc = GetDocument();
	if (bActivate
		&& pActivateView == this
		&& (pActivateView == pDeactiveView
			|| pDeactiveView == NULL
			|| pDoc != pDeactiveView->GetDocument()))
	{
		GetApp()->m_pActiveDocument = pDoc;
		TRACE("App::m_pActiveDocument set to %X\n", pDoc);
		if (pDoc && ! pDoc->m_PlayingSound)
		{
			TRACE("Document %x thread priority increased\n", pDoc);
			pDoc->SetThreadPriority(THREAD_PRIORITY_BELOW_NORMAL);
		}
	}
	if (pDeactiveView == this && ! bActivate)
	{
		GetApp()->m_pActiveDocument = NULL;
		TRACE("App::m_pActiveDocument set to NULL\n");
		if (pDoc && ! pDoc->m_PlayingSound)
		{
			TRACE("Document %x thread priority lowered\n", pDoc);
			pDoc->SetThreadPriority(THREAD_PRIORITY_LOWEST);
		}
	}
	CScaledScrollView::OnActivateView(bActivate, pActivateView, pDeactiveView);
}


int CWaveSoapFrontView::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CScaledScrollView::OnCreate(lpCreateStruct) == -1)
		return -1;
	KeepAspectRatio(FALSE);
	KeepScaleOnResizeX(TRUE);
	KeepScaleOnResizeY(FALSE);
	KeepOrgOnResizeX(TRUE);
	KeepOrgOnResizeY(FALSE);

	CRect r;
	GetClientRect( r);
	int nChannels = GetDocument()->WaveChannels();
	int nLowExtent = -32768;
	int nHighExtent = 32767;
	if (nChannels > 1)
	{
		nLowExtent = -0x10000;
		nHighExtent = 0x10000;
	}

	SetMaxExtents(0., GetDocument()->WaveFileSamples(), nLowExtent, nHighExtent);
	SetExtents(0., double(r.Width()) * m_HorizontalScale, nLowExtent, nHighExtent);
	ShowScrollBar(SB_VERT, FALSE);
	ShowScrollBar(SB_HORZ, FALSE);
	return 0;
}

BOOL CWaveSoapFrontView::ScrollBy(double dx, double dy, BOOL bDoScroll)
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
		double offset = m_WaveOffsetY + 65536. * ndy / (nHeight * m_VerticalScale);
		// find max and min offset for this scale
		double MaxOffset = 32768. * (1 - 1. / m_VerticalScale);
		BOOL NoScroll = false;
		if (offset > MaxOffset)
		{
			offset = MaxOffset;
			NoScroll = true;
		}
		double MinOffset = -MaxOffset;
		if (offset < MinOffset)
		{
			offset = MinOffset;
			NoScroll = true;
		}
		if (offset != m_WaveOffsetY)
		{
			TRACE("New offset = %g, MaxOffset=%g, VerticalScale = %g\n",
				offset, MaxOffset, m_VerticalScale);
			m_WaveOffsetY = offset;
			NotifySlaveViews(WAVE_OFFSET_CHANGED);
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
				HidePlaybackCursor();
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
				ShowPlaybackCursor();
			}
		}
	}
	return TRUE;
}

void CWaveSoapFrontView::OnSize(UINT nType, int cx, int cy)
{
	CScaledScrollView::OnSize(nType, cx, cy);

	CreateAndShowCaret();
}

void CWaveSoapFrontView::OnViewZoomvertNormal()
{
	if (m_VerticalScale != 1.)
	{
		m_VerticalScale = 1.;
		m_WaveOffsetY = 0.;
		Invalidate();
		NotifySlaveViews(WAVE_SCALE_CHANGED);
	}
}

void CWaveSoapFrontView::OnUpdateViewZoomvertNormal(CCmdUI* pCmdUI)
{
	pCmdUI->Enable(m_VerticalScale > 1.);
}

void CWaveSoapFrontView::OnViewZoominHorFull()
{
	Zoom(m_HorizontalScale, 1., CPoint(INT_MAX, INT_MAX));
}

void CWaveSoapFrontView::OnUpdateViewZoominHorFull(CCmdUI* pCmdUI)
{
	pCmdUI->Enable(m_HorizontalScale > 1);
}

BOOL CWaveSoapFrontView::OnScrollBy(CSize sizeScroll, BOOL bDoScroll)
{
	if (bDoScroll)
	{
		HidePlaybackCursor();
	}

	BOOL bRet = CScaledScrollView::OnScrollBy(sizeScroll, bDoScroll);
	if (bDoScroll)
	{
		ShowPlaybackCursor();
	}
	return bRet;
}

void CWaveSoapFrontView::UpdatePlaybackCursor(long sample, int channel)
{
	if (-1 == m_PlaybackCursorChannel)
	{
		m_PlaybackCursorDrawnSamplePos = sample;
	}
	int pos = WorldToWindowX(sample);
	int OldPos = WorldToWindowX(m_PlaybackCursorDrawnSamplePos);
	if (pos == OldPos
		&& channel == m_PlaybackCursorChannel)
	{
		m_PlaybackCursorDrawnSamplePos = sample;
		return; // no need to change
	}

	HidePlaybackCursor();
	m_PlaybackCursorDrawnSamplePos = sample;
	m_PlaybackCursorChannel = channel;

	if (-1 == channel)
	{
		// not playing
		return;
	}
	CRect r;
	GetClientRect( & r);
	if ((OldPos < r.left || OldPos >= r.right)
		&& (pos < r.left || pos >= r.right))
	{
		// not in the view;
		return;
	}
	// scroll the cursor to the center
	if (pos > r.right / 2)
	{
		long NewPos;
		// scroll it to the window
		if (pos > r.right)
		{
			NewPos = r.right / 2;
		}
		else
		{
			NewPos = OldPos - (pos - OldPos);
		}
		if (NewPos < r.right / 2)
		{
			NewPos = r.right / 2;
		}
		ScrollBy(m_HorizontalScale * (pos - NewPos), 0, TRUE);
		NotifySlaveViews(CHANGE_HOR_ORIGIN);
		CreateAndShowCaret();
	}
	ShowPlaybackCursor();
	GdiFlush();
}

