// Copyright Alexander Grigoriev, 1997-2002, All Rights Reserved
// WaveSoapFrontView.cpp : implementation of the CWaveSoapFrontView class
//

#include "stdafx.h"
#include "resource.h"
#include "WaveSoapFront.h"

#include "WaveSoapFrontDoc.h"
#include "WaveSoapFrontView.h"
#include "ChildFrm.h"
#include "WaveOutlineView.h"
#include "GdiObjectSave.h"
#include <float.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define TRACE_DRAWING 0
#define TRACE_MOUSE 0
#define TRACE_SCROLL 0
#define TRACE_CARET 0
#define TRACE_UPDATE 0
/////////////////////////////////////////////////////////////////////////////
// CWaveSoapFrontView

IMPLEMENT_DYNCREATE(CWaveSoapFrontView, CScaledScrollView)

BEGIN_MESSAGE_MAP(CWaveSoapFrontView, BaseClass)
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
	ON_WM_RBUTTONDOWN()
	ON_WM_RBUTTONUP()
	ON_UPDATE_COMMAND_UI(ID_VIEW_ZOOM_SELECTION, OnUpdateViewZoomSelection)
	ON_COMMAND(ID_VIEW_ZOOM_SELECTION, OnViewZoomSelection)
	ON_UPDATE_COMMAND_UI(ID_INDICATOR_SCALE, OnUpdateIndicatorScale)
	ON_UPDATE_COMMAND_UI(ID_VIEW_HOR_SCALE_1, OnUpdateViewHorScale1)
	ON_COMMAND(ID_VIEW_HOR_SCALE_1, OnViewHorScale1)
	ON_UPDATE_COMMAND_UI(ID_VIEW_HOR_SCALE_2, OnUpdateViewHorScale2)
	ON_COMMAND(ID_VIEW_HOR_SCALE_2, OnViewHorScale2)
	ON_UPDATE_COMMAND_UI(ID_VIEW_HOR_SCALE_4, OnUpdateViewHorScale4)
	ON_COMMAND(ID_VIEW_HOR_SCALE_4, OnViewHorScale4)
	ON_UPDATE_COMMAND_UI(ID_VIEW_HOR_SCALE_8, OnUpdateViewHorScale8)
	ON_COMMAND(ID_VIEW_HOR_SCALE_8, OnViewHorScale8)
	ON_UPDATE_COMMAND_UI(ID_VIEW_HOR_SCALE_16, OnUpdateViewHorScale16)
	ON_COMMAND(ID_VIEW_HOR_SCALE_16, OnViewHorScale16)
	ON_UPDATE_COMMAND_UI(ID_VIEW_HOR_SCALE_32, OnUpdateViewHorScale32)
	ON_COMMAND(ID_VIEW_HOR_SCALE_32, OnViewHorScale32)
	ON_UPDATE_COMMAND_UI(ID_VIEW_HOR_SCALE_64, OnUpdateViewHorScale64)
	ON_COMMAND(ID_VIEW_HOR_SCALE_64, OnViewHorScale64)
	ON_UPDATE_COMMAND_UI(ID_VIEW_HOR_SCALE_128, OnUpdateViewHorScale128)
	ON_COMMAND(ID_VIEW_HOR_SCALE_128, OnViewHorScale128)
	ON_UPDATE_COMMAND_UI(ID_VIEW_HOR_SCALE_256, OnUpdateViewHorScale256)
	ON_COMMAND(ID_VIEW_HOR_SCALE_256, OnViewHorScale256)
	ON_UPDATE_COMMAND_UI(ID_VIEW_HOR_SCALE_512, OnUpdateViewHorScale512)
	ON_COMMAND(ID_VIEW_HOR_SCALE_512, OnViewHorScale512)
	ON_UPDATE_COMMAND_UI(ID_VIEW_HOR_SCALE_1024, OnUpdateViewHorScale1024)
	ON_COMMAND(ID_VIEW_HOR_SCALE_1024, OnViewHorScale1024)
	ON_UPDATE_COMMAND_UI(ID_VIEW_HOR_SCALE_2048, OnUpdateViewHorScale2048)
	ON_COMMAND(ID_VIEW_HOR_SCALE_2048, OnViewHorScale2048)
	ON_UPDATE_COMMAND_UI(ID_VIEW_HOR_SCALE_4096, OnUpdateViewHorScale4096)
	ON_COMMAND(ID_VIEW_HOR_SCALE_4096, OnViewHorScale4096)
	ON_UPDATE_COMMAND_UI(ID_VIEW_HOR_SCALE_8192, OnUpdateViewHorScale8192)
	ON_COMMAND(ID_VIEW_HOR_SCALE_8192, OnViewHorScale8192)
	ON_WM_TIMER()
	ON_WM_CAPTURECHANGED()
	//}}AFX_MSG_MAP
	// Standard printing commands
	//ON_COMMAND(ID_FILE_PRINT, OnFilePrint)
	//ON_COMMAND(ID_FILE_PRINT_DIRECT, OnFilePrint)
	//ON_COMMAND(ID_FILE_PRINT_PREVIEW, OnFilePrintPreview)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CWaveSoapFrontView construction/destruction

CWaveSoapFrontView::CWaveSoapFrontView()
	: m_HorizontalScale(2048),
	m_VerticalScale(1.),
	m_WaveOffsetY(0.),
//m_FirstSampleInBuffer(0),
//m_pWaveBuffer(NULL),
//m_WaveBufferSize(0),
	m_PlaybackCursorChannel(0),
	m_PlaybackCursorDrawn(false),
	m_NewSelectionMade(false),
	m_bAutoscrollTimerStarted(false),
	m_PlaybackCursorDrawnSamplePos(0),
	m_WheelAccumulator(0)
//, m_WaveDataSizeInBuffer(0)
{
	TRACE("CWaveSoapFrontView::CWaveSoapFrontView()\n");
}

CWaveSoapFrontView::~CWaveSoapFrontView()
{
	TRACE("CWaveSoapFrontView::~CWaveSoapFrontView()\n");
#if 0
	if (NULL != m_pWaveBuffer)
	{
		delete[] m_pWaveBuffer;
		m_pWaveBuffer = NULL;
	}
#endif
}

BOOL CWaveSoapFrontView::PreCreateWindow(CREATESTRUCT& cs)
{
	// Modify the Window class or styles here by modifying
	//  the CREATESTRUCT cs
	cs.lpszClass = AfxRegisterWndClass(CS_VREDRAW | CS_DBLCLKS, NULL, NULL, NULL);
	TRACE("CWaveSoapFrontView::PreCreateWindow(CREATESTRUCT)\n");
	return BaseClass::PreCreateWindow(cs);
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
													CHANNEL_MASK Channel)
{
	ThisDoc * pDoc = GetDocument();
	// find positions of the selection start and and
	// and check whether the selected area is visible
	//double XScaleDev = GetXScaleDev();
	int SelectionLeft = WorldToWindowXfloor(pDoc->m_SelectionStart);
	int SelectionRight = WorldToWindowXfloor(pDoc->m_SelectionEnd);

	// draw selection if Channel==ALL or all selected, or
	// this channel is selected
	if (0 == (Channel & pDoc->m_SelectedChannel))
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
		|| SelectionRight < left
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

void CWaveSoapFrontView::GetChannelRect(int Channel, RECT * pR) const
{
	ThisDoc * pDoc = GetDocument();
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

int CWaveSoapFrontView::GetChannelFromPoint(int y) const
{
	ThisDoc * pDoc = GetDocument();
	int nChannels = pDoc->WaveChannels();

	CRect r;
	GetClientRect(r);

	if (y < 0)
	{
		return -1;
	}

	if (y > r.bottom)
	{
		return nChannels;
	}

	int h = (r.Height() + 1) / nChannels;
	// for all channels, the rectangle is of the same height
	return y / h;
}

void CWaveSoapFrontView::OnDraw(CDC* pDC)
{
	ThisDoc * pDoc = GetDocument();
	ASSERT_VALID(pDoc);
	if (! pDoc->m_WavFile.IsOpen())
	{
		return;
	}
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

	CThisApp * pApp = GetApp();
	POINT (* ppArray)[2] = NULL;

	try {
		CPushDcPalette OldPalette(pDC, NULL);

		if (pDC->GetDeviceCaps(RASTERCAPS) & RC_PALETTE)
		{
			OldPalette.PushPalette(pApp->GetPalette(), FALSE);
		}

		WaveformPen.CreatePen(PS_SOLID, 1, pApp->m_WaveColor);
		SelectedWaveformPen.CreatePen(PS_SOLID, 1, pApp->m_SelectedWaveColor);

		ZeroLinePen.CreatePen(PS_SOLID, 1, pApp->m_ZeroLineColor);
		SelectedZeroLinePen.CreatePen(PS_SOLID, 1, pApp->m_SelectedZeroLineColor);

		SixDBLinePen.CreatePen(PS_SOLID, 1, pApp->m_6dBLineColor);
		SelectedSixDBLinePen.CreatePen(PS_SOLID, 1, pApp->m_Selected6dBLineColor);

		ChannelSeparatorPen.CreatePen(PS_SOLID, 1, pApp->m_ChannelSeparatorColor);
		SelectedChannelSeparatorPen.CreatePen(PS_SOLID, 1, pApp->m_SelectedChannelSeparatorColor);

		CGdiObjectSaveT<CPen> OldPen(pDC, pDC->SelectObject(& ZeroLinePen));

		CRect r;

		GetClientRect(r);
		if (pDC->IsKindOf(RUNTIME_CLASS(CPaintDC)))
		{
			RECT r_upd = ((CPaintDC*)pDC)->m_ps.rcPaint;
			// make intersect by x coordinate
			if (r.left < r_upd.left) r.left = r_upd.left;
			if (r.right > r_upd.right) r.right = r_upd.right;
		}

		r.left--;   // make additional
		r.right++;

		double left = WindowToWorldX(r.left);
		//double right = WindowToWorldX(r.right);
		if (left < 0.) left = 0.;

		// number of sample that corresponds to the r.left position
		SAMPLE_INDEX NumOfFirstSample = DWORD(left);
		unsigned SamplesPerPoint = m_HorizontalScale;

		// create an array of points

		NUMBER_OF_CHANNELS nChannels = pDoc->WaveChannels();
		int nNumberOfPoints = r.right - r.left;

		int SelBegin = WorldToWindowXfloor(pDoc->m_SelectionStart);
		int SelEnd = WorldToWindowXfloor(pDoc->m_SelectionEnd);

		if (pDoc->m_SelectionEnd != pDoc->m_SelectionStart
			&& SelEnd == SelBegin)
		{
			SelEnd++;
		}

		if (nNumberOfPoints > 0)
		{
			ppArray = new POINT[nNumberOfPoints][2];
			if (ppArray)
				for (int ch = 0; ch < nChannels; ch++)
				{
					CRect ChanR;
					GetChannelRect(ch, ChanR);

					int const ClipHigh = ChanR.bottom;
					int const ClipLow = ChanR.top;

					WaveCalculate WaveToY(m_WaveOffsetY, m_VerticalScale, ChanR.top, ChanR.bottom);

					if (TRACE_DRAWING) TRACE("V Scale=%f, m_WaveOffsetY=%f, top = %d, bottom = %d, height=%d, W2Y(32767)=%d, W2Y(-32768)=%d\n",
											m_VerticalScale, m_WaveOffsetY, ChanR.top, ChanR.bottom,
											ChanR.Height(), WaveToY(32767), WaveToY(-32768));
					// Y = wave * m_VerticalScale + m_WaveOffsetY * m_VerticalScale
					//     + (ChanR.bottom + ChanR.top) / 2
					int ZeroLinePos = WaveToY(0);

					if (ZeroLinePos >= ClipLow &&
						ZeroLinePos < ClipHigh)
					{
						if (TRACE_DRAWING) TRACE("CWaveSoapFrontView Zero pos=%d\n", ZeroLinePos);

						DrawHorizontalWithSelection(pDC, r.left, r.right,
													ZeroLinePos,
													& ZeroLinePen,
													& SelectedZeroLinePen, 1 << ch);
					}

					int n6DBLine = WaveToY(16384);
					if (n6DBLine >= ClipLow &&
						n6DBLine < ClipHigh)
					{
						DrawHorizontalWithSelection(pDC, r.left, r.right,
													n6DBLine,
													& SixDBLinePen,
													& SelectedSixDBLinePen, 1 << ch);
					}

					n6DBLine = WaveToY(-16384);

					if (n6DBLine >= ClipLow &&
						n6DBLine < ClipHigh)
					{
						DrawHorizontalWithSelection(pDC, r.left, r.right,
													n6DBLine,
													& SixDBLinePen,
													& SelectedSixDBLinePen, 1 << ch);
					}

					int i;
					unsigned PeakDataGranularity = pDoc->m_WavFile.GetPeakGranularity();
					if (SamplesPerPoint >= PeakDataGranularity)
					{
						CSimpleCriticalSectionLock lock(pDoc->m_WavFile.GetPeakLock());
						// use peak data for drawing
						DWORD PeakSamplesPerPoint = SamplesPerPoint / PeakDataGranularity * nChannels;

						int nIndexOfPeak =
							ch + (NumOfFirstSample / int(PeakDataGranularity)) * nChannels;

						for (i = 0; i < nNumberOfPoints; i++, nIndexOfPeak += PeakSamplesPerPoint)
						{
							int index1 = nIndexOfPeak;
							if (index1 < 0)
							{
								index1 = 0;
							}

							int index2 = nIndexOfPeak + PeakSamplesPerPoint;
							if (index2 < 0)
							{
								index2 = 0;
							}

							WavePeak peak =
								pDoc->m_WavFile.GetPeakMinMax(index1, index2, nChannels);

							ppArray[i][0] = CPoint(i + r.left, WaveToY(peak.low));
							ppArray[i][1] = CPoint(i + r.left, WaveToY(peak.high));
						}
					}
					else
					{
						// use wave data for drawing
						WAVE_SAMPLE * pWaveSamples = NULL;

						int nCountSamples = m_WaveBuffer.GetData( & pWaveSamples,
																NumOfFirstSample * nChannels,
																nNumberOfPoints * SamplesPerPoint * nChannels, this);

						int nSample = ch;

						for (i = 0; i < nNumberOfPoints; i++)
						{
							int low = 0x7FFF;
							int high = -0x8000;
							for (unsigned j = 0; j < SamplesPerPoint && nSample < nCountSamples;
								j++, nSample += nChannels)
							{
								if (high < pWaveSamples[nSample])
								{
									high = pWaveSamples[nSample];
								}
								if (low > pWaveSamples[nSample])
								{
									low = pWaveSamples[nSample];
								}
							}

							ppArray[i][0] = CPoint(i + r.left, WaveToY(low));
							ppArray[i][1] = CPoint(i + r.left, WaveToY(high));
//                        ASSERT(ppArray[i][0].y >= ppArray[i][1].y);
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
						POINT (* const pp)[2] = ppArray + i;
						if (pp[0][0].y >= pp[0][1].y)
						{
							pp[0][1].y--;
							if (i < nNumberOfPoints - 1
								&& pp[1][0].y >= pp[1][1].y)
							{
								if (pp[0][0].y < pp[1][1].y)
								{
									pp[0][0].y = (pp[1][1].y + pp[0][0].y) >> 1;
								}
								else if (pp[1][0].y < pp[0][1].y)
								{
									pp[0][1].y = (pp[0][1].y + pp[1][0].y) >> 1;
								}
							}
							if (LastY0 >= LastY1)
							{
								if (pp[0][0].y < LastY1)
								{
									pp[0][0].y = LastY1;
								}
								else if (pp[0][1].y > LastY0)
								{
									pp[0][1].y = LastY0;
								}
							}

							LastY0 = pp[0][0].y;
							LastY1 = pp[0][1].y;

							if (pp[0][0].y < ClipLow)
							{
								continue;
							}
							if (pp[0][1].y > ClipHigh)
							{
								continue;
							}

							CPen * pPenToDraw = & WaveformPen;

							if (pp[0][0].x >= SelBegin
								&& pp[0][0].x < SelEnd
								&& 0 != ((1 << ch) & pDoc->m_SelectedChannel))
							{
								pPenToDraw = & SelectedWaveformPen;
							}

							if (pPenToDraw != pLastPen)
							{
								pDC->SelectObject(pPenToDraw);
								pLastPen = pPenToDraw;
							}

							if (pp[0][1].y < ClipLow)
							{
								pp[0][1].y = ClipLow - 1;
							}

							if (pp[0][0].y > ClipHigh)
							{
								pp[0][0].y = ClipHigh;
							}

							pDC->MoveTo(pp[0][0]);
							pDC->LineTo(pp[0][1]);
						}
					}
					if (ch + 1 < nChannels)
					{
						// draw channel separator line
						DrawHorizontalWithSelection(pDC, r.left, r.right,
													ChanR.bottom,
													& ChannelSeparatorPen,
													& SelectedChannelSeparatorPen, ALL_CHANNELS);
					}
				}
		}
	}
	catch (CResourceException * e)
	{
		e->Delete();
	}

	delete[] ppArray;
	if (m_PlaybackCursorDrawn)
	{
		DrawPlaybackCursor(pDC, m_PlaybackCursorDrawnSamplePos, m_PlaybackCursorChannel);
	}
	BaseClass::OnDraw(pDC);
}

int CDataSection<WAVE_SAMPLE, CWaveSoapFrontView>::ReadData(WAVE_SAMPLE * pBuf, LONGLONG nOffset,
															long nCount, CWaveSoapFrontView * pSource)
{
	CWaveSoapFrontDoc * pDoc = pSource->GetDocument();
	if (NULL == pDoc
		|| ! pDoc->m_WavFile.IsOpen())
	{
		return 0;
	}
	long ToZero = 0;
	if (nOffset < 0)
	{
		ToZero = nCount;
		if (ToZero > -nOffset)
		{
			ToZero = long(-nOffset);
		}
		memset(pBuf, 0, ToZero * sizeof *pBuf);
		nOffset = 0;
		nCount -= ToZero;
		pBuf += ToZero;
	}

	long Read = 0;
	if (0 != nCount)
	{
		Read = pDoc->m_WavFile.ReadAt(pBuf, nCount * sizeof (WAVE_SAMPLE),
									pDoc->m_WavFile.GetDataChunk()->dwDataOffset + nOffset * sizeof (WAVE_SAMPLE));
		if (-1 == Read)
		{
			return 0;
		}
	}
	return ToZero + Read / sizeof (WAVE_SAMPLE);
}

LONGLONG CDataSection<WAVE_SAMPLE, CWaveSoapFrontView>::GetSourceCount(CWaveSoapFrontView * pSource)
{
	CWaveSoapFrontDoc * pDoc = pSource->GetDocument();
	if (NULL == pDoc
		|| ! pDoc->m_WavFile.IsOpen())
	{
		return 0;
	}
	return pDoc->m_WavFile.GetDataChunk()->cksize / sizeof (WAVE_SAMPLE);
}

void CWaveSoapFrontView::AdjustNewOrigin(double & NewOrgX, double & /*NewOrgY*/)
{
	// make sure the screen is aligned by a multiple of m_HorizontalScale
	NewOrgX -= fmod(NewOrgX, m_HorizontalScale);
}

void CWaveSoapFrontView::AdjustNewScale(double OldScaleX, double OldScaleY,
										double & NewScaleX, double & NewScaleY)
{
	//NewScaleY = OldScaleY;  // vertical scale never changes

	m_HorizontalScale = int(1. / NewScaleX);
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

	if (TRACE_DRAWING) TRACE("Old scale X=%g, New scale X=%g, Old scale Y=%g, New scale Y=%g\n",
							OldScaleX, NewScaleX, OldScaleY, NewScaleY);
}

BOOL CWaveSoapFrontView::PlaybackCursorVisible()
{
	int pos = WorldToWindowXfloor(m_PlaybackCursorDrawnSamplePos);

	CRect r;
	GetClientRect(r);

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

void CWaveSoapFrontView::DrawPlaybackCursor(CDC * pDC, SAMPLE_INDEX Sample, CHANNEL_MASK Channel)
{
	CDC * pDrawDC = pDC;
	if ( ! IsWindowVisible())
	{
		return;
	}
	int pos = WorldToWindowXfloor(Sample);

	CRect r;
	GetClientRect(r);

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

	try
	{
		CPushDcMapMode mode(pDrawDC, MM_TEXT);
		CPushDcRop2 rop2(pDrawDC, R2_XORPEN);

		CPen pen(PS_SOLID, 0, 0xFFFFFF);

		CGdiObjectSaveT<CPen> OldPen(pDrawDC, pDrawDC->SelectObject( & pen));
		// looks like the display driver can delay the drawing with
		// WHITE_PEN, it is causing jerkiness in the playback cursor
		//CGdiObject * pOldPen = pDrawDC->SelectStockObject(WHITE_PEN);
		if (0 == (SPEAKER_FRONT_RIGHT & Channel))
		{
			r.bottom /= 2;
		}
		else if (0 == (SPEAKER_FRONT_LEFT & Channel))
		{
			r.top = r.bottom / 2;
		}

		pDrawDC->MoveTo(CPoint(pos, r.top));
		pDrawDC->LineTo(CPoint(pos, r.bottom));
		//TRACE("Cursor drawn  at %d, time=%d\n", pos, timeGetTime());
	}
	catch (CResourceException * e)
	{
		e->Delete();
	}

	if (NULL == pDC)
	{
		ReleaseDC(pDrawDC);
	}
	//GdiFlush();
	Sleep(0);
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
	BaseClass::AssertValid();
}

void CWaveSoapFrontView::Dump(CDumpContext& dc) const
{
	BaseClass::Dump(dc);
}

CWaveSoapFrontDoc* CWaveSoapFrontView::GetDocument() const// non-debug version is inline
{
	ASSERT(m_pDocument->IsKindOf(RUNTIME_CLASS(CWaveSoapFrontDoc)));
	return static_cast<ThisDoc*>(m_pDocument);
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CWaveSoapFrontView message handlers

void CWaveSoapFrontView::OnInitialUpdate()
{
	if (0) TRACE("OnInitialUpdate style = %08X\n", GetStyle());

	BaseClass::OnInitialUpdate();
	UpdateScrollbars();
	if (0) TRACE("OnInitialUpdate final style = %08X\n", GetStyle());
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
	if (m_VerticalScale > 1.)
	{
		m_VerticalScale	*= sqrt(0.5);
		if (m_VerticalScale < 1.01)    // compensate any error
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
DWORD CWaveSoapFrontView::ClientHitTest(CPoint p) const
{
	ThisDoc * pDoc = GetDocument();
	DWORD result = 0;

	CRect r;
	GetClientRect(r);

	if ( ! r.PtInRect(p))
	{
		return VSHT_NONCLIENT;
	}

	//int Separator = WorldToWindowY(0.);
	int ChannelUnderCursor = GetChannelFromPoint(p.y);

	if (ChannelUnderCursor >= 0
		&& ChannelUnderCursor < pDoc->WaveChannels())
	{
		result |= ChannelUnderCursor;

		if (0 != (pDoc->m_SelectedChannel & (1 << ChannelUnderCursor))
			&& pDoc->m_SelectionStart <= pDoc->m_SelectionEnd)
		{
			int SelBegin = WorldToWindowXfloor(pDoc->m_SelectionStart);
			int SelEnd = WorldToWindowXfloor(pDoc->m_SelectionEnd);

			if (pDoc->m_SelectionEnd != pDoc->m_SelectionStart
				&& SelEnd == SelBegin)
			{
				SelEnd++;
			}
			int BorderWidth = GetSystemMetrics(SM_CXEDGE);
			// check whether the cursor is on the selection boundary
			// TODO: separate left edge and right edge for narrow selection

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
	}

	int DataEnd = WorldToWindowXceil(pDoc->WaveFileSamples());

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

		int AutoscrollWidth = GetSystemMetrics(SM_CXVSCROLL);
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

	return BaseClass::OnSetCursor(pWnd, nHitTest, message);
}

void CWaveSoapFrontView::OnKillFocus(CWnd* pNewWnd)
{
	DestroyCaret();
	BaseClass::OnKillFocus(pNewWnd);

}

void CWaveSoapFrontView::OnSetFocus(CWnd* pOldWnd)
{
	BaseClass::OnSetFocus(pOldWnd);
	CreateAndShowCaret();
}

void CWaveSoapFrontView::CreateAndShowCaret()
{
	// create caret
	if (this != GetFocus())
	{
		return;
	}
	ThisDoc * pDoc = GetDocument();
	if (pDoc->m_PlayingSound && ! m_NewSelectionMade)
	{
		DestroyCaret();
		return;
	}

	CRect r;
	GetClientRect(r);

	CPoint p(WorldToWindowXfloor(pDoc->m_CaretPosition), r.top);

	if (TRACE_CARET) TRACE("Client rect height=%d, caret position=%d\n", r.Height(), p.x);

	if (pDoc->m_WavFile.AllChannels(pDoc->m_SelectedChannel))
	{
		CreateSolidCaret(1, r.Height());
	}
	else
	{
		CreateSolidCaret(1, r.Height() / 2);

		if (SPEAKER_FRONT_RIGHT & pDoc->m_SelectedChannel)
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
	CThisApp * pApp = GetApp();
	ThisDoc * pDoc = GetDocument();
	RemoveSelectionRect();
	CBrush backBrush(pApp->m_WaveBackground);

	CRect r;
	GetClientRect(r);

	CRect gr = r;
	int SelBegin = WorldToWindowXfloor(pDoc->m_SelectionStart);
	int SelEnd = WorldToWindowXfloor(pDoc->m_SelectionEnd);
	int FileEnd = WorldToWindowXceil(pDoc->WaveFileSamples());

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
			0x55, 0,
		};
		try {
			bmp.CreateBitmap(8, 8, 1, 1, pattern + 2 * (FileEnd & 1));
			CBrush GrayBrush( & bmp);
			r.right = FileEnd;
			gr.left = FileEnd;
			pDC->FillRect(gr, & GrayBrush);
		}
		catch (CResourceException * e)
		{
			TRACE("CResourceException\n");
			e->Delete();
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

		if (pDoc->m_WavFile.AllChannels(pDoc->m_SelectedChannel))
		{
			pDC->FillRect(r, & SelectedBackBrush);
		}
		else
		{
			// only one channel is selected
			int Separator = WorldToWindowY(0.);
			CRect r1 = r;
			CRect r2 = r;

			if (SPEAKER_FRONT_LEFT & pDoc->m_SelectedChannel)
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
	ThisDoc * pDoc = GetDocument();
	DWORD nHit = ClientHitTest(point);
	if (nHit & VSHT_NONCLIENT)
	{
		return;
	}
	SAMPLE_INDEX nSampleUnderMouse = int(WindowToWorldX(point.x));
	SAMPLE_INDEX SelectionStart = pDoc->m_SelectionStart;
	SAMPLE_INDEX SelectionEnd = pDoc->m_SelectionEnd;

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

		CHANNEL_MASK nChan = ALL_CHANNELS;
		if (nHit & VSHT_LEFT_CHAN)
		{
			nChan = SPEAKER_FRONT_LEFT;
		}
		else if (nHit & VSHT_RIGHT_CHAN)
		{
			nChan = SPEAKER_FRONT_RIGHT;
		}

		pDoc->SetSelection(SelectionStart, SelectionEnd, nChan, nSampleUnderMouse);
		OnSetCursor(this, HTCLIENT, WM_LBUTTONDOWN);
	}
	else
	{
		BaseClass::OnLButtonDown(nFlags, point);
	}
}

void CWaveSoapFrontView::OnLButtonUp(UINT nFlags, CPoint point)
{
	ThisDoc * pDoc = GetDocument();

	if (pDoc->m_TimeSelectionMode)
	{
		if ( ! bIsTrackingSelection
			&&  nKeyPressed == WM_LBUTTONDOWN)
		{
			// mouse hasn't moved after click
			if (GetApp()->m_bSnapMouseSelectionToMax
				// the whole area wasn't selected
				&& pDoc->m_SelectionStart == pDoc->m_SelectionEnd)
			{
				SAMPLE_INDEX nBegin = SAMPLE_INDEX(WindowToWorldX(point.x));
				SAMPLE_INDEX nEnd = SAMPLE_INDEX(WindowToWorldX(point.x + 1));

				pDoc->SetSelection(nBegin, nEnd, pDoc->m_SelectedChannel, nBegin,
									SetSelection_SnapToMaximum
									| SetSelection_MakeCaretVisible);
			}
		}
		ReleaseCapture();
		bIsTrackingSelection = FALSE;
		nKeyPressed = 0;

		CView::OnLButtonUp(nFlags, point);
	}
	else
	{
		BaseClass::OnLButtonUp(nFlags, point);
	}
}

BOOL CWaveSoapFrontView::OnMouseWheel(UINT nFlags, short zDelta, CPoint /*pt*/)
{
	// without control or shift:
	// just scrolls 1/20th of the window width
	m_WheelAccumulator += zDelta;
	if (0 == (nFlags & (MK_CONTROL | MK_SHIFT)))
	{
		// accumulate the delta until WHEEL_DELTA is reached
		if (m_WheelAccumulator >= 0)
		{
			while (m_WheelAccumulator >= WHEEL_DELTA)
			{
				m_WheelAccumulator -= WHEEL_DELTA;
				OnScroll(MAKEWORD(SB_LINEUP, -1), 0);
			}
		}
		else
		{
			while (m_WheelAccumulator <= - WHEEL_DELTA)
			{
				m_WheelAccumulator += WHEEL_DELTA;
				OnScroll(MAKEWORD(SB_LINEDOWN, -1), 0);
			}
		}
	}
	else if (nFlags & MK_CONTROL)
	{
		// TODO: use pt.x as zoom center
		if (m_WheelAccumulator >= 0)
		{
			while (m_WheelAccumulator >= WHEEL_DELTA)
			{
				m_WheelAccumulator -= WHEEL_DELTA;
				if (m_HorizontalScale > 1)
				{
					OnViewZoominHor2();
				}
			}
		}
		else
		{
			while (m_WheelAccumulator <= - WHEEL_DELTA)
			{
				m_WheelAccumulator += WHEEL_DELTA;
				OnViewZoomOutHor2();
			}
		}
	}
	return TRUE;
}

void CWaveSoapFrontView::OnMouseMove(UINT nFlags, CPoint point)
{
	// point is in client coordinates
	ThisDoc * pDoc = GetDocument();
	DWORD nHit = ClientHitTest(point);

	CRect r;
	GetClientRect(r);

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

	SAMPLE_INDEX nSampleUnderMouse = SAMPLE_INDEX(WindowToWorldX(point.x));
	if (nSampleUnderMouse < 0)
	{
		nSampleUnderMouse = 0;
	}
	if (nSampleUnderMouse > pDoc->WaveFileSamples())
	{
		nSampleUnderMouse = pDoc->WaveFileSamples();
	}

	SAMPLE_INDEX SelectionStart = pDoc->m_SelectionStart;
	SAMPLE_INDEX SelectionEnd = pDoc->m_SelectionEnd;

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

			if (nHit & (VSHT_LEFT_AUTOSCROLL | VSHT_RIGHT_AUTOSCROLL))
			{
				if (! m_bAutoscrollTimerStarted)
				{
					m_bAutoscrollTimerStarted = true;
					m_TimerID = SetTimer(DWORD(this) + sizeof *this, 50, NULL);
					if (TRACE_CARET) TRACE("Timer %X started\n", m_TimerID);
				}
			}
			else if (m_bAutoscrollTimerStarted)
			{
				m_bAutoscrollTimerStarted = false;
				KillTimer(m_TimerID);
				m_TimerID = NULL;
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

			int nChan = ALL_CHANNELS;
			if (nHit & (VSHT_NOWAVE | VSHT_NONCLIENT))
			{
				// don't change the channel
				nChan = pDoc->m_SelectedChannel;
			}
			else if (nHit & VSHT_LEFT_CHAN)
			{
				nChan = SPEAKER_FRONT_LEFT;
			}
			else if (nHit & VSHT_RIGHT_CHAN)
			{
				nChan = SPEAKER_FRONT_RIGHT;
			}
			pDoc->SetSelection(SelectionStart, SelectionEnd, nChan, nSampleUnderMouse);
		}
	}
	else
	{

		BaseClass::OnMouseMove(nFlags, point);
	}
}

void CWaveSoapFrontView::OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint)
{
	ThisDoc * pDoc = GetDocument();

	if (lHint == ThisDoc::UpdateSelectionChanged
		&& NULL != pHint)
	{
		m_NewSelectionMade = true;

		CSelectionUpdateInfo * pInfo =
			dynamic_cast<CSelectionUpdateInfo *>(pHint);
		if (NULL == pInfo)
		{
			BaseClass::OnUpdate(pSender, lHint, pHint);
			return;
		}

		CRect r;
		// change for foreground frame only
		CFrameWnd * pFrameWnd = GetParentFrame();
		if (NULL != pFrameWnd
			&& pFrameWnd == pFrameWnd->GetWindow(GW_HWNDFIRST))
		{
			if (pInfo->Flags & SetSelection_MakeCaretVisible)
			{
				MovePointIntoView(pInfo->CaretPos);
			}
			else if (pInfo->Flags & SetSelection_MoveCaretToCenter)
			{
				MovePointIntoView(pInfo->CaretPos, TRUE);
			}
		}

		if (pInfo->Flags & SetSelection_MakeFileVisible)
		{
			SAMPLE_INDEX LastSample = pDoc->WaveFileSamples();
			if (WindowToWorldX(0) > LastSample)
			{
				MovePointIntoView(LastSample, TRUE);
			}
		}

		GetClientRect(r);
		int Separator = WorldToWindowY(0.);

		// calculate new selection boundaries
		int SelBegin = WorldToWindowXfloor(pInfo->SelBegin);
		int SelEnd = WorldToWindowXfloor(pInfo->SelEnd);

		if (pInfo->SelEnd != pInfo->SelBegin
			&& SelEnd == SelBegin)
		{
			SelEnd++;
		}

		int SelTop = r.top;
		int SelBottom = r.bottom;
		if (pDoc->WaveChannels() > 1)
		{
			if (0 == (pInfo->SelChannel & SPEAKER_FRONT_RIGHT))
			{
				SelBottom = Separator;
			}
			else if (0 == (pInfo->SelChannel & SPEAKER_FRONT_LEFT))
			{
				SelTop = Separator;
			}
		}

		// calculate old selection boundaries
		int OldSelTop = r.top;
		int OldSelBottom = r.bottom;

		if (pDoc->WaveChannels() > 1)
		{
			if (0 == (pInfo->OldSelChannel & SPEAKER_FRONT_RIGHT))
			{
				OldSelBottom = Separator;
			}
			else if (0 == (pInfo->OldSelChannel & SPEAKER_FRONT_LEFT))
			{
				OldSelTop = Separator;
			}
		}

		int OldSelBegin = WorldToWindowXfloor(pInfo->OldSelBegin);
		int OldSelEnd = WorldToWindowXfloor(pInfo->OldSelEnd);

		if (pInfo->OldSelEnd != pInfo->OldSelBegin
			&& OldSelEnd == OldSelBegin)
		{
			OldSelEnd++;
		}

		// build rectangles with selection boundaries
		CRect r1(SelBegin, SelTop, SelEnd, SelBottom);
		CRect r2(OldSelBegin, OldSelTop, OldSelEnd, OldSelBottom);
		// invalidate the regions with changed selection
		if (pInfo->OldSelChannel == pInfo->SelChannel)
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
	else if (lHint == ThisDoc::UpdateSoundChanged
			&& NULL != pHint)
	{
		CSoundUpdateInfo * pInfo = static_cast<CSoundUpdateInfo *> (pHint);

		CRect r;
		GetClientRect(r);

		CRect r1;
		if (TRACE_UPDATE) TRACE("OnUpdate Sound Changed from %d to %d, length=%d\n",
								pInfo->m_Begin, pInfo->m_End, pInfo->m_NewLength);

		r1.top = r.top;
		r1.bottom = r.bottom;
		if (pInfo->m_NewLength != -1)
		{
			m_WaveBuffer.Invalidate(); // invalidate the data in draw buffer
			// length changed, set new extents and caret position
			if (r.Width() > 100
				&& NUMBER_OF_SAMPLES((r.Width() - 100) * m_HorizontalScale / 2) > pInfo->m_NewLength)
			{
				SetExtents(0, pInfo->m_NewLength, 0, 0);
			}
			UpdateMaxHorExtents(pInfo->m_NewLength);
			Invalidate();
		}
		else
		{
			// TODO: invalidate only if in the range
			m_WaveBuffer.Invalidate(); // invalidate the data in draw buffer
		}

		// calculate update boundaries
		r1.left = WorldToWindowXfloor(pInfo->m_Begin);
		r1.right = WorldToWindowXceil(pInfo->m_End) + 2;

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
			if (TRACE_UPDATE) TRACE("OnUpdate: SoundChanged: Invalidating from %d to %d\n", r1.left, r1.right);
			InvalidateRect(& r1, TRUE);
		}
	}
	else if (lHint == ThisDoc::UpdatePlaybackPositionChanged
			&& NULL != pHint)
	{
		CSoundUpdateInfo * pInfo = static_cast<CSoundUpdateInfo *> (pHint);

		UpdatePlaybackCursor(pInfo->m_PlaybackPosition, pInfo->m_PlaybackChannel);
	}
	else if (lHint == ThisDoc::UpdateSampleRateChanged
			&& NULL == pHint)
	{
		// don't do anything
	}
	else if (lHint == ThisDoc::UpdateWholeFileChanged
			&& NULL == pHint
			)
	{
		// recalculate the extents
		UpdateMaxHorExtents(GetDocument()->WaveFileSamples());
		UpdateVertExtents();
	}
	else
	{
		BaseClass::OnUpdate(pSender, lHint, pHint);
	}
}

void CWaveSoapFrontView::InvalidateRect( LPCRECT lpRect, BOOL bErase)
{
	HideCaret();
	BaseClass::InvalidateRect(lpRect, bErase);
	ShowCaret();
}

POINT CWaveSoapFrontView::GetZoomCenter()
{
	ThisDoc * pDoc = GetDocument();
	int caret = WorldToWindowXfloor(pDoc->m_CaretPosition);
	int SelBegin = WorldToWindowXfloor(pDoc->m_SelectionStart);
	int SelEnd = WorldToWindowXfloor(pDoc->m_SelectionEnd);
	//int CenterY = WorldToWindowY(0);

	CRect r;
	GetClientRect(r);

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
	// process cursor control commands
	ThisDoc * pDoc = GetDocument();

	SAMPLE_INDEX nCaret = pDoc->m_CaretPosition;
	SAMPLE_INDEX nSelBegin = pDoc->m_SelectionStart;
	SAMPLE_INDEX nSelEnd = pDoc->m_SelectionEnd;
	CHANNEL_MASK nChan = pDoc->m_SelectedChannel;

	BOOL KeepSelection = (0 != (0x8000 & GetKeyState(VK_SHIFT)));
	BOOL CtrlPressed = (0 != (0x8000 & GetKeyState(VK_CONTROL)));
	BOOL MakeCaretVisible = TRUE;

	int nCaretMove = m_HorizontalScale;

	// ctrl+arrow moves by 32 pixels
	if (CtrlPressed)
	{
		nCaretMove *= 32;
	}

	NUMBER_OF_SAMPLES nTotalSamples = pDoc->WaveFileSamples();

	CRect r;
	GetClientRect(r);

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
		if (CtrlPressed)
		{
			nCaret = 0;
		}
		else if (nSelBegin < nSelEnd)
		{
			KeepSelection = TRUE;
			nCaret = nSelBegin;
		}
		else if (nSelBegin > nSelEnd)
		{
			KeepSelection = TRUE;
			nCaret = nSelEnd;
		}
		else
		{
			nCaret = SAMPLE_INDEX(WindowToWorldX(r.left + 1)); // cursor to the left boundary + 1
		}

		break;
	case VK_END:
		// move to the end of file or selection
		if (CtrlPressed)
		{
			nCaret = nTotalSamples;
		}
		else if (nSelBegin < nSelEnd)
		{
			KeepSelection = TRUE;
			nCaret = nSelEnd;
		}
		else if (nSelBegin > nSelEnd)
		{
			KeepSelection = TRUE;
			nCaret = nSelBegin;
		}
		else
		{
			nCaret = SAMPLE_INDEX(WindowToWorldX(r.right - 2)); // cursor to the right boundary + 1
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
		if (pDoc->m_WavFile.AllChannels(nChan))
		{
			nChan = SPEAKER_FRONT_LEFT;
		}
		else
		{
			if (pDoc->WaveChannels() > 1)
			{
				nChan <<= 1;
				if (0 == (nChan & pDoc->m_WavFile.ChannelsMask()))
				{
					nChan = ALL_CHANNELS;
				}
			}
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

void CWaveSoapFrontView::MovePointIntoView(SAMPLE_INDEX nCaret, BOOL bCenter)
{
	CRect r;
	GetClientRect(r);

	int nDesiredPos = WorldToWindowXfloor(nCaret);
	double scroll;
	int AutoscrollWidth = GetSystemMetrics(SM_CXVSCROLL);

	if (bCenter)
	{
		scroll = (nDesiredPos - r.right / 2) * m_HorizontalScale;
	}
	else if (nDesiredPos < r.left + AutoscrollWidth)
	{
		scroll = (nDesiredPos - (r.left + AutoscrollWidth)) * m_HorizontalScale;
	}
	else if (nDesiredPos >= r.right - AutoscrollWidth)
	{
		scroll = (nDesiredPos - (r.right - AutoscrollWidth)) * m_HorizontalScale;
	}
	else
	{
		return;
	}
	if (TRACE_SCROLL) TRACE("MovePointIntoView: DesiredPos=%d, left=%d, right=%d, scroll=%d\n",
							nDesiredPos, r.left, r.right, scroll);
	ScrollBy(scroll, 0, TRUE);
	CreateAndShowCaret();
}

void CWaveSoapFrontView::UpdateCaretPosition()
{
	CreateAndShowCaret();
}

void CWaveSoapFrontView::OnActivateView(BOOL bActivate, CView* pActivateView, CView* pDeactiveView)
{
	ThisDoc * pDoc = GetDocument();
	if (bActivate
		&& pActivateView == this)
	{
		GetApp()->OnActivateDocument(pDoc, TRUE);
	}
	if (pDeactiveView == this && ! bActivate)
	{
		GetApp()->OnActivateDocument(pDoc, FALSE);
	}
	BaseClass::OnActivateView(bActivate, pActivateView, pDeactiveView);
}


int CWaveSoapFrontView::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (BaseClass::OnCreate(lpCreateStruct) == -1)
		return -1;
	KeepAspectRatio(FALSE);
	KeepScaleOnResizeX(TRUE);
	KeepScaleOnResizeY(FALSE);
	KeepOrgOnResizeX(TRUE);
	KeepOrgOnResizeY(FALSE);

	CRect r;
	GetClientRect(r);

	SetExtents(0., double(r.Width()) * m_HorizontalScale, 0., 0.);

	UpdateMaxHorExtents(GetDocument()->WaveFileSamples());
	UpdateVertExtents();

	ShowScrollBar(SB_VERT, FALSE);
	ShowScrollBar(SB_HORZ, FALSE);
	return 0;
}

BOOL CWaveSoapFrontView::MasterScrollBy(double dx, double dy, BOOL bDoScroll)
{
	if (dx != 0.)
	{
		if (TRACE_SCROLL) TRACE("before MasterScrollBy: dOrgX=%f, dx=%f\n", dOrgX, dx);
		BaseClass::MasterScrollBy(dx, 0, bDoScroll);
		// make sure the new position will be on the multiple of m_HorizontalScale
		ASSERT(0 == SAMPLE_INDEX(dOrgX) % m_HorizontalScale);
	}
	if (dy != 0.)
	{
		// check for the limits
		// ndy is in pixels

		CRect r;
		GetChannelRect(0, r);

		int ndy = -fround(dy * GetYScaleDev());

		double offset = m_WaveOffsetY - 65536. * ndy / (r.Height() * m_VerticalScale);
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

		int y1 = WaveCalculate
				(m_WaveOffsetY, m_VerticalScale, r.top, r.bottom)(0);
		int y2 = WaveCalculate
				(offset, m_VerticalScale, r.top, r.bottom)(0);

		ndy = y2 - y1;

		if (0 != ndy)
		{
			if (TRACE_SCROLL) TRACE("New offset = %g, MaxOffset=%g, VerticalScale = %g\n",
									offset, MaxOffset, m_VerticalScale);
			// calculate how much zero line would move

			m_WaveOffsetY = offset;
			NotifySlaveViews(WAVE_OFFSET_CHANGED);
			// change to scroll
			if (NoScroll)
			{
				Invalidate();
			}
			else
			{
				int nChannels = GetDocument()->WaveChannels();

				HidePlaybackCursor();
				for (int ch = 0; ch < nChannels; ch++)
				{
					CRect cr, ir;
					GetChannelRect(ch, cr);
#if 1
					ScrollWindowEx(0, ndy, & cr, & cr, NULL, & ir,
									SW_INVALIDATE);
					InvalidateRect( & ir);
#else
					ScrollWindowEx(0, ndy, NULL, NULL, NULL, NULL,
									SW_INVALIDATE | SW_ERASE);
#endif
				}
				ShowPlaybackCursor();
			}
		}
	}
	return TRUE;
}

void CWaveSoapFrontView::OnSize(UINT nType, int cx, int cy)
{
	// set m_HorizontalScale to stretch the file in the view
	NUMBER_OF_SAMPLES nSamples = GetDocument()->WaveFileSamples();
	if (nSamples != 0
		&& (cx - 100) * m_HorizontalScale / 2 > nSamples)
	{
		SetExtents(0, nSamples, 0, 0);
		Invalidate();
	}
	BaseClass::OnSize(nType, cx, cy);

	UpdateVertExtents();
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
	MovePointIntoView(GetDocument()->m_CaretPosition, TRUE);
}

void CWaveSoapFrontView::OnUpdateViewZoominHorFull(CCmdUI* pCmdUI)
{
	pCmdUI->Enable(m_HorizontalScale > 1);
}

// the function scrolls the real image, and modifies dOrgX, dOrgY.
BOOL CWaveSoapFrontView::OnScrollBy(CSize sizeScroll, BOOL bDoScroll)
{
	if (bDoScroll)
	{
		HidePlaybackCursor();
	}

	BOOL bRet = BaseClass::OnScrollBy(sizeScroll, bDoScroll);
	if (bDoScroll)
	{
		ShowPlaybackCursor();
	}
	return bRet;
}

void CWaveSoapFrontView::UpdatePlaybackCursor(SAMPLE_INDEX sample, CHANNEL_MASK channel)
{
	if (0 == m_PlaybackCursorChannel)
	{
		// first call after playback start
		m_PlaybackCursorDrawnSamplePos = sample;
		m_NewSelectionMade = false; // to hide the caret
	}

	int pos = WorldToWindowXfloor(sample);
	int OldPos = WorldToWindowXfloor(m_PlaybackCursorDrawnSamplePos);

	if (pos == OldPos
		&& channel == m_PlaybackCursorChannel)
	{
		m_PlaybackCursorDrawnSamplePos = sample;
		return; // no need to change
	}

	HidePlaybackCursor();

	m_PlaybackCursorDrawnSamplePos = sample;
	m_PlaybackCursorChannel = channel;

	CreateAndShowCaret();
	if (0 == channel)
	{
		// not playing now
		return;
	}

	CRect r;
	GetClientRect(r);

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
		double dScroll = m_HorizontalScale * (pos - NewPos);
		if (dOrgX + dExtX + dScroll < dMaxRight)
		{
			ScrollBy(m_HorizontalScale * (pos - NewPos), 0, TRUE);
		}
	}
	ShowPlaybackCursor();
	//GdiFlush();
}

void CWaveSoapFrontView::UpdateMaxHorExtents(NUMBER_OF_SAMPLES Length)
{
	CRect r;
	GetClientRect(r);

	long nRightMaxExtent = long(Length - Length % m_HorizontalScale + 100. * m_HorizontalScale);
	long MinWidth = r.Width() * m_HorizontalScale;
	long nRightExtent = 0;

	if (nRightMaxExtent < MinWidth)
	{
		nRightMaxExtent = MinWidth;
		nRightExtent = nRightMaxExtent;
	}

	// if the file fits into the view, adjust the horizontal scale
	// if the previous view l
	SetMaxExtents(0, nRightMaxExtent, 0, 0);
	SetExtents(0., nRightExtent, 0, 0);
	UpdateScrollbars();
}

void CWaveSoapFrontView::UpdateVertExtents()
{
	CRect r;
	GetClientRect(r);

	if (0 == r.Height())
	{
		return;
	}
	int nLowExtent = -32768 - 32768 / r.Height();
	int nHighExtent = 32767;
	if (GetDocument()->WaveChannels() > 1)
	{
		nLowExtent = -0x10000 - 0x20000 / r.Height();
		nHighExtent = 0x10000;
	}
	SetMaxExtents(0, 0, nLowExtent, nHighExtent);
	SetExtents(0., 0., nLowExtent, nHighExtent);
	//ShowScrollBar(SB_VERT, FALSE);
}

void CWaveSoapFrontView::OnMasterChangeOrgExt(double left, double width,
											double top, double height, DWORD flag)
{
	int OldHorScale = m_HorizontalScale;

	BaseClass::OnMasterChangeOrgExt(left, width,
									top, height, flag);

	if (OldHorScale != m_HorizontalScale)
	{
		UpdateMaxHorExtents(GetDocument()->WaveFileSamples());
	}
}

UINT CWaveSoapFrontView::GetPopupMenuID(CPoint point)
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
		return IDR_MENU_WAVE_VIEW;
	}
}

void CWaveSoapFrontView::OnRButtonDown(UINT nFlags, CPoint point)
{
	CView::OnRButtonDown(nFlags, point);
}

void CWaveSoapFrontView::OnRButtonUp(UINT nFlags, CPoint point)
{
	CView::OnRButtonUp(nFlags, point);
}

void CWaveSoapFrontView::OnUpdateViewZoomSelection(CCmdUI* pCmdUI)
{
	ThisDoc * pDoc = GetDocument();
	pCmdUI->Enable(pDoc->m_SelectionEnd - pDoc->m_SelectionStart > 32);
}

void CWaveSoapFrontView::OnViewZoomSelection()
{
	ThisDoc * pDoc = GetDocument();
	SetExtents(pDoc->m_SelectionStart, pDoc->m_SelectionEnd, 0, 0);
}

void CWaveSoapFrontView::NotifySlaveViews(DWORD flag)
{
	if (flag & (CHANGE_HOR_EXTENTS | CHANGE_MAX_HOR_EXTENTS))
	{
		CWnd * pOutlineWnd = GetParent()->GetDlgItem(CWaveMDIChildClient::OutlineViewID);
		CWaveOutlineView * pOutlineView = dynamic_cast<CWaveOutlineView *>(pOutlineWnd);
		if (NULL != pOutlineView)
		{
			double left, right, top, bottom;
			GetExtents(left, right, bottom, top);
			pOutlineView->NotifyViewExtents(SAMPLE_INDEX(left), SAMPLE_INDEX(right));
		}
	}
	BaseClass::NotifySlaveViews(flag);
}

void CWaveSoapFrontView::OnUpdateIndicatorScale(CCmdUI* pCmdUI)
{
	CString s;
	s.Format(_T("1:%d"), int(m_HorizontalScale));
	SetStatusString(pCmdUI, s);
}

void CWaveSoapFrontView::SetHorizontalScale(int HorScale)
{
	Zoom(double(m_HorizontalScale) / HorScale, 1., CPoint(INT_MAX, INT_MAX));
	MovePointIntoView(GetDocument()->m_CaretPosition, TRUE);
}

void CWaveSoapFrontView::OnUpdateViewHorScale1(CCmdUI* pCmdUI)
{
	pCmdUI->SetRadio(1 == m_HorizontalScale);
}

void CWaveSoapFrontView::OnViewHorScale1()
{
	SetHorizontalScale(1);
}

void CWaveSoapFrontView::OnUpdateViewHorScale2(CCmdUI* pCmdUI)
{
	pCmdUI->SetRadio(2 == m_HorizontalScale);
}

void CWaveSoapFrontView::OnViewHorScale2()
{
	SetHorizontalScale(2);
}

void CWaveSoapFrontView::OnUpdateViewHorScale4(CCmdUI* pCmdUI)
{
	pCmdUI->SetRadio(4 == m_HorizontalScale);
}

void CWaveSoapFrontView::OnViewHorScale4()
{
	SetHorizontalScale(4);
}

void CWaveSoapFrontView::OnUpdateViewHorScale8(CCmdUI* pCmdUI)
{
	pCmdUI->SetRadio(8 == m_HorizontalScale);
}

void CWaveSoapFrontView::OnViewHorScale8()
{
	SetHorizontalScale(8);
}

void CWaveSoapFrontView::OnUpdateViewHorScale16(CCmdUI* pCmdUI)
{
	pCmdUI->SetRadio(16 == m_HorizontalScale);
}

void CWaveSoapFrontView::OnViewHorScale16()
{
	SetHorizontalScale(16);
}

void CWaveSoapFrontView::OnUpdateViewHorScale32(CCmdUI* pCmdUI)
{
	pCmdUI->SetRadio(32 == m_HorizontalScale);
}

void CWaveSoapFrontView::OnViewHorScale32()
{
	SetHorizontalScale(32);
}

void CWaveSoapFrontView::OnUpdateViewHorScale64(CCmdUI* pCmdUI)
{
	pCmdUI->SetRadio(64 == m_HorizontalScale);
}

void CWaveSoapFrontView::OnViewHorScale64()
{
	SetHorizontalScale(64);
}

void CWaveSoapFrontView::OnUpdateViewHorScale128(CCmdUI* pCmdUI)
{
	pCmdUI->SetRadio(128 == m_HorizontalScale);
}

void CWaveSoapFrontView::OnViewHorScale128()
{
	SetHorizontalScale(128);
}

void CWaveSoapFrontView::OnUpdateViewHorScale256(CCmdUI* pCmdUI)
{
	pCmdUI->SetRadio(256 == m_HorizontalScale);
}

void CWaveSoapFrontView::OnViewHorScale256()
{
	SetHorizontalScale(256);
}

void CWaveSoapFrontView::OnUpdateViewHorScale512(CCmdUI* pCmdUI)
{
	pCmdUI->SetRadio(512 == m_HorizontalScale);
}

void CWaveSoapFrontView::OnViewHorScale512()
{
	SetHorizontalScale(512);
}

void CWaveSoapFrontView::OnUpdateViewHorScale1024(CCmdUI* pCmdUI)
{
	pCmdUI->SetRadio(1024 == m_HorizontalScale);
}

void CWaveSoapFrontView::OnViewHorScale1024()
{
	SetHorizontalScale(1024);
}

void CWaveSoapFrontView::OnUpdateViewHorScale2048(CCmdUI* pCmdUI)
{
	pCmdUI->SetRadio(2048 == m_HorizontalScale);
}

void CWaveSoapFrontView::OnViewHorScale2048()
{
	SetHorizontalScale(2048);
}

void CWaveSoapFrontView::OnUpdateViewHorScale4096(CCmdUI* pCmdUI)
{
	pCmdUI->SetRadio(4096 == m_HorizontalScale);
}

void CWaveSoapFrontView::OnViewHorScale4096()
{
	SetHorizontalScale(4096);
}

void CWaveSoapFrontView::OnUpdateViewHorScale8192(CCmdUI* pCmdUI)
{
	pCmdUI->SetRadio(8192 == m_HorizontalScale);
}

void CWaveSoapFrontView::OnViewHorScale8192()
{
	SetHorizontalScale(8192);
}

void CWaveSoapFrontView::OnTimer(UINT nIDEvent)
{
	// get mouse position and hit code
	if (m_bAutoscrollTimerStarted
		&& nIDEvent == m_TimerID)
	{
		CPoint p;
		GetCursorPos( & p);
		ScreenToClient( & p);
		double scroll;
		DWORD nHit = ClientHitTest(p);
		if (nHit & ( VSHT_RIGHT_AUTOSCROLL | VSHT_LEFT_AUTOSCROLL))
		{
			//TRACE("OnTimer: VSHT_RIGHT_AUTOSCROLL\n");
			scroll = -m_HorizontalScale;
			int nDistance;
			if (nHit & VSHT_RIGHT_AUTOSCROLL)
			{
				CRect r;
				GetClientRect(r);

				nDistance = p.x - r.right + GetSystemMetrics(SM_CXVSCROLL) - 1;
				scroll = m_HorizontalScale;
			}
			else
			{
				nDistance = GetSystemMetrics(SM_CXVSCROLL) - p.x - 1;
			}

			if (TRACE_SCROLL) TRACE("nDistance = %d\n", nDistance);
			if (nDistance > 14)
			{
				nDistance = 14;
			}
			if (nDistance > 0)
			{
				scroll *= 1 << nDistance;
			}
			ScrollBy(scroll, 0, TRUE);

			CreateAndShowCaret();
			UINT flags = 0;
			if (0x8000 & GetKeyState(VK_CONTROL))
			{
				flags |= MK_CONTROL;
			}
			if (0x8000 & GetKeyState(VK_SHIFT))
			{
				flags |= MK_SHIFT;
			}
			if (0x8000 & GetKeyState(VK_LBUTTON))
			{
				flags |= MK_LBUTTON;
			}
			if (0x8000 & GetKeyState(VK_RBUTTON))
			{
				flags |= MK_RBUTTON;
			}
			if (0x8000 & GetKeyState(VK_MBUTTON))
			{
				flags |= MK_MBUTTON;
			}
			OnMouseMove(flags, p);
			return;
		}
		else
		{
			m_bAutoscrollTimerStarted = false;
			KillTimer(m_TimerID);
			m_TimerID = NULL;
		}
	}
	else
	{
		if (TRACE_SCROLL) TRACE("Timer ID=%X\n", nIDEvent);
	}

	BaseClass::OnTimer(nIDEvent);
}

void CWaveSoapFrontView::OnCaptureChanged(CWnd *pWnd)
{
	if (pWnd != this
		&& m_bAutoscrollTimerStarted)
	{
		if (TRACE_SCROLL) TRACE("Killing timer in CWaveSoapFrontView::OnCaptureChanged\n");
		m_bAutoscrollTimerStarted = false;
		KillTimer(m_TimerID);
		m_TimerID = NULL;
	}
	BaseClass::OnCaptureChanged(pWnd);
}
