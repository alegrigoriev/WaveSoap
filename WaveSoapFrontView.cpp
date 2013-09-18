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

IMPLEMENT_DYNCREATE(CWaveSoapFrontView, CView);

BEGIN_MESSAGE_MAP(CWaveSoapFrontView, BaseClass)
	//{{AFX_MSG_MAP(CWaveSoapFrontView)
	ON_UPDATE_COMMAND_UI(ID_VIEW_ZOOMINHOR, OnUpdateViewZoominhor)
	ON_UPDATE_COMMAND_UI(ID_VIEW_ZOOMINHOR2, OnUpdateViewZoominhor2)
	ON_COMMAND(ID_VIEW_ZOOMINHOR2, OnViewZoominHor2)
	ON_COMMAND(ID_VIEW_ZOOMOUTHOR2, OnViewZoomOutHor2)
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
	ON_UPDATE_COMMAND_UI_RANGE(ID_VIEW_HOR_SCALE_1, ID_VIEW_HOR_SCALE_8192, OnUpdateViewHorScale)
	ON_COMMAND_RANGE(ID_VIEW_HOR_SCALE_1, ID_VIEW_HOR_SCALE_8192, OnViewHorScale)
	ON_WM_TIMER()
	ON_WM_CAPTURECHANGED()
	ON_WM_LBUTTONDBLCLK()
	ON_COMMAND(ID_VIEW_ZOOMPREVIOUS, OnViewZoomprevious)
	ON_WM_CONTEXTMENU()
	ON_WM_HSCROLL()
	ON_WM_VSCROLL()
	ON_WM_DESTROY()
	//}}AFX_MSG_MAP
	// Standard printing commands
	//ON_COMMAND(ID_FILE_PRINT, OnFilePrint)
	//ON_COMMAND(ID_FILE_PRINT_DIRECT, OnFilePrint)
	//ON_COMMAND(ID_FILE_PRINT_PREVIEW, OnFilePrintPreview)
	ON_MESSAGE(UWM_NOTIFY_VIEWS, &CWaveSoapFrontView::OnUwmNotifyViews)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CWaveSoapFrontView construction/destruction

CWaveSoapFrontView::CWaveSoapFrontView()
	: m_HorizontalScale(2048),
	m_PrevHorizontalScale(-1),
	m_VerticalScale(1.),
	m_WaveOffsetY(0.),
	m_PlaybackCursorChannel(0),
	m_PlaybackCursorDrawn(false),
	m_NewSelectionMade(false),
	m_AutoscrollTimerID(0),
	m_PlaybackCursorDrawnSamplePos(0),
	m_WheelAccumulator(0),
	m_FirstSampleInView(0.),
	bIsTrackingSelection(false),
	nKeyPressed(0)
{
	TRACE("CWaveSoapFrontView::CWaveSoapFrontView()\n");
}

CWaveSoapFrontView::~CWaveSoapFrontView()
{
}

BOOL CWaveSoapFrontView::PreCreateWindow(CREATESTRUCT& cs)
{
	// Modify the Window class or styles here by modifying
	//  the CREATESTRUCT cs
	cs.lpszClass = AfxRegisterWndClass(CS_VREDRAW | CS_DBLCLKS, NULL, NULL, NULL);
	//TRACE("CWaveSoapFrontView::PreCreateWindow(CREATESTRUCT)\n");
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
													CHANNEL_MASK Channel, LPCRECT ClipRect)
{
	if (left >= right)
	{
		return;
	}

	if (ClipRect != NULL)
	{
		if (Y < ClipRect->top)
		{
			return;
		}
		if (Y >= ClipRect->bottom)
		{
			return;
		}
	}
	ThisDoc * pDoc = GetDocument();
	// find positions of the selection start and and
	// and check whether the selected area is visible
	//double XScaleDev = GetXScaleDev();
	int SelectionLeft = SampleToX(pDoc->m_SelectionStart);
	int SelectionRight = SampleToXceil(pDoc->m_SelectionEnd);

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

// this is the rectangle for scaling purposes
void CWaveSoapFrontView::GetChannelRect(int Channel, RECT * pR) const
{
	ThisDoc * pDoc = GetDocument();
	int const nChannels = pDoc->WaveChannels();
	int const FileEnd = SampleToXceil(pDoc->WaveFileSamples());

	GetClientRect(pR);
	if (pR->right > FileEnd)
	{
		pR->right = FileEnd;
	}

	if (Channel >= nChannels)
	{
		pR->top = pR->bottom;
		pR->bottom += 2;
		return;
	}

	int h = (pR->bottom - pR->top + 1) / nChannels;
	// for all channels, the rectangle is of the same height
	pR->top = ((pR->bottom - pR->top + 1) * Channel) / nChannels;
	pR->bottom = pR->top + h - 1;
}

// this is the rectangle for clipping purposes
void CWaveSoapFrontView::GetChannelClipRect(int Channel, RECT * pR) const
{
	ThisDoc * pDoc = GetDocument();
	int const nChannels = pDoc->WaveChannels();
	int const FileEnd = SampleToXceil(pDoc->WaveFileSamples());
	GetClientRect(pR);
	if (pR->right > FileEnd)
	{
		pR->right = FileEnd;
	}

	if (Channel >= nChannels)
	{
		pR->top = pR->bottom;
		pR->bottom += 2;
		return;
	}

	int h = pR->bottom - pR->top + 1;
	pR->top = (h * Channel) / nChannels;
	pR->bottom = (h * (Channel+1)) / nChannels - 1;
}

int CWaveSoapFrontView::SampleValueToY(double value, int ch) const
{
	ThisDoc * pDoc = GetDocument();
	int const nChannels = pDoc->WaveChannels();

	CRect cr;
	GetClientRect(cr);

	if (ch >= nChannels)
	{
		return cr.bottom;
	}

	int h = cr.bottom - cr.top + 1;
	cr.top = (h * ch) / nChannels;
	cr.bottom = (h * (ch+1)) / nChannels - 1;
	int MidLine = (cr.top + cr.bottom) / 2;
	return int((value - m_WaveOffsetY) * m_VerticalScale + MidLine);
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
	CThisApp * pApp = GetApp();
	POINT (* ppArray)[2] = NULL;

	try {
		CPushDcPalette OldPalette(pDC, NULL);

		if (pDC->GetDeviceCaps(RASTERCAPS) & RC_PALETTE)
		{
			OldPalette.PushPalette(pApp->GetPalette(), FALSE);
		}

		// pen to draw the waveform
		CPen WaveformPen(PS_SOLID, 1, pApp->m_WaveColor);
		CPen SelectedWaveformPen(PS_SOLID, 1, pApp->m_SelectedWaveColor);
		// pen to draw zero level
		CPen ZeroLinePen(PS_SOLID, 1, pApp->m_ZeroLineColor);
		CPen SelectedZeroLinePen(PS_SOLID, 1, pApp->m_SelectedZeroLineColor);
		// pen do draw 6dB line
		CPen SixDBLinePen(PS_SOLID, 1, pApp->m_6dBLineColor);
		CPen SelectedSixDBLinePen(PS_SOLID, 1, pApp->m_Selected6dBLineColor);
		// pen to draw left/right channel separator
		CPen ChannelSeparatorPen(PS_SOLID, 1, pApp->m_ChannelSeparatorColor);
		CPen SelectedChannelSeparatorPen(PS_SOLID, 1, pApp->m_SelectedChannelSeparatorColor);

		CGdiObjectSaveT<CPen> OldPen(pDC, pDC->SelectObject(& ZeroLinePen));

		CRect cr;

		GetClientRect(cr);
		if (pDC->IsKindOf(RUNTIME_CLASS(CPaintDC)))
		{
			RECT r_upd = ((CPaintDC*)pDC)->m_ps.rcPaint;
			// make intersect by x coordinate
			if (cr.left < r_upd.left) cr.left = r_upd.left;
			if (cr.right > r_upd.right) cr.right = r_upd.right;
		}

		cr.left -= 2;   // make additional
		cr.right += 2;

		double left = WindowXtoSample(cr.left);
		//double right = WindowXtoSample(cr.right);
		if (left < 0.)
		{
			left = 0.;
			cr.left = SampleToX(0);
		}

		// number of sample that corresponds to the cr.left position
		SAMPLE_INDEX NumOfFirstSample = DWORD(left);
		double SamplesPerPoint = m_HorizontalScale;

		// create an array of points

		NUMBER_OF_CHANNELS nChannels = pDoc->WaveChannels();
		int nNumberOfPoints = cr.right - cr.left;

		int SelBegin = SampleToX(pDoc->m_SelectionStart);
		int SelEnd = SampleToX(pDoc->m_SelectionEnd);

		if (pDoc->m_SelectionEnd != pDoc->m_SelectionStart
			&& SelEnd == SelBegin)
		{
			SelEnd++;
		}

		if (nNumberOfPoints > 0)
		{
			CWaveFile::InstanceDataWav * pInst = pDoc->m_WavFile.GetInstanceData();

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

			ppArray = new POINT[nNumberOfPoints][2];

			if (ppArray)
				for (int ch = 0; ch < nChannels; ch++)
				{
					CRect ChanR;
					GetChannelRect(ch, ChanR);
					CRect ClipR;
					GetChannelClipRect(ch, ClipR);

					// clip the channel rectangle with 'clip rect'
					if (ChanR.left < cr.left)
					{
						ChanR.left = cr.left;
					}
					if (ChanR.right > cr.right)
					{
						ChanR.right = cr.right;
					}

					int const ClipHigh = ClipR.bottom;
					int const ClipLow = ClipR.top;

					WaveCalculate WaveToY(m_WaveOffsetY, m_VerticalScale, ChanR.top, ChanR.bottom);

					if (TRACE_DRAWING) TRACE("V Scale=%f, m_WaveOffsetY=%f, top = %d, bottom = %d, height=%d, W2Y(32767)=%d, W2Y(-32768)=%d\n",
											m_VerticalScale, m_WaveOffsetY, ChanR.top, ChanR.bottom,
											ChanR.Height(), WaveToY(32767), WaveToY(-32768));
					// Y = wave * m_VerticalScale + m_WaveOffsetY * m_VerticalScale
					//     + (ChanR.bottom + ChanR.top) / 2

					DrawHorizontalWithSelection(pDC, ChanR.left, ChanR.right,
												WaveToY(0),     // zero line
												& ZeroLinePen,
												& SelectedZeroLinePen, 1 << ch, ClipR);

					DrawHorizontalWithSelection(pDC, ChanR.left, ChanR.right,
												WaveToY(16384),         // 6 dB line
												& SixDBLinePen,
												& SelectedSixDBLinePen, 1 << ch, ClipR);

					DrawHorizontalWithSelection(pDC, ChanR.left, ChanR.right,
												WaveToY(-16384),       // low 6dB line
												& SixDBLinePen,
												& SelectedSixDBLinePen, 1 << ch);

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

							ppArray[i][0] = CPoint(i + cr.left, (int)WaveToY(peak.low));
							ppArray[i][1] = CPoint(i + cr.left, (int)WaveToY(peak.high));
						}
					}
					else
					{
						// use wave data for drawing
						float * pWaveSamples = NULL;
						// GetData argument must be aligned to full sample boundary
						int nCountSamples = m_WaveBuffer.GetData( & pWaveSamples,
																NumOfFirstSample * nChannels,
																nNumberOfPoints * SamplesPerPoint * nChannels, this);

						int nSample = ch;

						for (i = 0; i < nNumberOfPoints; i++)
						{
							WAVE_PEAK low = SHORT_MAX;
							WAVE_PEAK high = SHORT_MIN;
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

							ppArray[i][0] = CPoint(i + cr.left, (int)WaveToY(low));
							ppArray[i][1] = CPoint(i + cr.left, (int)WaveToY(high));
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
						// draw channel separator line on all view length
						DrawHorizontalWithSelection(pDC, cr.left, cr.right,
													ChanR.bottom,
													& ChannelSeparatorPen,
													& SelectedChannelSeparatorPen, ALL_CHANNELS);
					}

					DashBrush.UnrealizeObject();

					// Raster OP: Destination AND brush pattern
					DWORD const DstAndBrushRop = 0x00A000C9;
					DWORD const DstOrBrushRop = 0x00AF0229;

					pDC->SetBrushOrg(0, WaveToY(-32768) % DashLength);

					CGdiObjectSaveT<CBrush> OldBrush(pDC, pDC->SelectObject( & DashBrush));
					CGdiObjectSave OldFont(pDC, pDC->SelectStockObject(ANSI_VAR_FONT));

					pDC->SetTextColor(0xFFFFFF ^ pApp->m_WaveBackground);
					pDC->SetBkColor(pApp->m_WaveBackground);
					pDC->SetBkMode(OPAQUE);

					for (ConstCuePointVectorIterator i = pInst->m_CuePoints.begin();
						i != pInst->m_CuePoints.end(); i++)
					{
						long x = SampleToX(i->dwSampleOffset);
						WaveRegionMarker const * pMarker = pInst->GetRegionMarker(i->CuePointID);

						// draw text
						if (0 == ch
							&& x < cr.right)
						{
							LPCTSTR txt = pInst->GetCueText(i->CuePointID);
							if (NULL != txt)
							{
								int count = (int)_tcslen(txt);
								CPoint size = pDC->GetTextExtent(txt, count);
								if (x + size.x > cr.left)
								{
									pDC->DrawText(txt, count, CRect(x, ChanR.top, x + size.x, ChanR.top + size.y),
												DT_LEFT | DT_NOPREFIX | DT_SINGLELINE | DT_TOP);
								}
							}
						}

						if (x >= ChanR.left
							&& x < ChanR.right)
						{
							DWORD BrushRop = DstAndBrushRop;    // black dashes
							if (0 != (pDoc->m_SelectedChannel & (1 << ch))
								&& x >= SelBegin && x < SelEnd)
							{
								BrushRop = DstOrBrushRop; // white dashes
							}

							if (pMarker != NULL
								&& pMarker->SampleLength != 0)
							{
								// draw mark of the region begin
								pDC->PatBlt(x, ChanR.top, 1, ChanR.bottom - ChanR.top, BrushRop);
							}
							else
							{
								// draw marker
								pDC->PatBlt(x, ChanR.top, 1, ChanR.bottom - ChanR.top, BrushRop);
							}
						}

						if (pMarker != NULL
							&& pMarker->SampleLength != 0)
						{
							x = SampleToX(i->dwSampleOffset + pMarker->SampleLength);

							if (x >= ChanR.left
								&& x < ChanR.right)
							{
								DWORD BrushRop = DstAndBrushRop;
								if (0 != (pDoc->m_SelectedChannel & (1 << ch))
									&& x >= SelBegin && x < SelEnd)
								{
									BrushRop = DstOrBrushRop;
								}
								// draw mark of the region end
								pDC->PatBlt(x, ChanR.top, 1, ChanR.bottom - ChanR.top, BrushRop);
							}
						}
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
		Read = pDoc->m_WavFile.ReadSamples(ALL_CHANNELS,
					pDoc->m_WavFile.SampleToPosition((SAMPLE_INDEX)(nOffset / pDoc->m_WavFile.Channels())),
					nCount / pDoc->m_WavFile.Channels(), pBuf, SampleType16bit);
		if (0 == Read)
		{
			return 0;
		}
	}
	return ToZero + Read * pDoc->m_WavFile.Channels();
}

int CDataSection<float, CWaveSoapFrontView>::ReadData(float * pBuf, LONGLONG nOffset,
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
		Read = pDoc->m_WavFile.ReadSamples(ALL_CHANNELS,
					pDoc->m_WavFile.SampleToPosition((SAMPLE_INDEX)(nOffset / pDoc->m_WavFile.Channels())),
					nCount / pDoc->m_WavFile.Channels(), pBuf, SampleTypeFloat32);
		if (0 == Read)
		{
			return 0;
		}
	}
	return ToZero + Read * pDoc->m_WavFile.Channels();
}

LONGLONG CDataSection<WAVE_SAMPLE, CWaveSoapFrontView>::GetSourceCount(CWaveSoapFrontView * pSource)
{
	CWaveSoapFrontDoc * pDoc = pSource->GetDocument();
	if (NULL == pDoc
		|| ! pDoc->m_WavFile.IsOpen())
	{
		return 0;
	}
	return pDoc->m_WavFile.NumberOfSamples() * pDoc->m_WavFile.Channels();
}

LONGLONG CDataSection<float, CWaveSoapFrontView>::GetSourceCount(CWaveSoapFrontView * pSource)
{
	CWaveSoapFrontDoc * pDoc = pSource->GetDocument();
	if (NULL == pDoc
		|| ! pDoc->m_WavFile.IsOpen())
	{
		return 0;
	}
	return pDoc->m_WavFile.NumberOfSamples() * pDoc->m_WavFile.Channels();
}

#if 0
void CWaveSoapFrontView::AdjustNewOrigin(double & NewOrgX, double & /*NewOrgY*/)
{
	// make sure the screen is aligned by a multiple of m_HorizontalScale
	NewOrgX -= fmod(NewOrgX, m_HorizontalScale);
}

void CWaveSoapFrontView::AdjustNewScale(double OldScaleX, double OldScaleY,
										double & NewScaleX, double & NewScaleY)
{
	//NewScaleY = OldScaleY;  // vertical scale never changes
	double PreviousScaleX = m_HorizontalScale;

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

	if (PreviousScaleX != m_HorizontalScale)
	{
		m_PrevHorizontalScale = PreviousScaleX;
	}

	if (TRACE_DRAWING) TRACE("Old scale X=%g, New scale X=%g, Old scale Y=%g, New scale Y=%g\n",
							OldScaleX, NewScaleX, OldScaleY, NewScaleY);
}
#endif

bool CWaveSoapFrontView::PlaybackCursorVisible() const
{
	int pos = SampleToX(m_PlaybackCursorDrawnSamplePos);

	CRect cr;
	GetClientRect(cr);

	return pos >= cr.left && pos < cr.right;
}

void CWaveSoapFrontView::DrawPlaybackCursor(CDC * pDC, SAMPLE_INDEX Sample, CHANNEL_MASK Channel)
{
	CDC * pDrawDC = pDC;
	if ( ! IsWindowVisible())
	{
		return;
	}
	int pos = SampleToX(Sample);

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
	CRect r;
	GetClientRect(r);
	m_FirstSampleInView = 0.;
	// if the file is short, set the scale to fit
	NUMBER_OF_SAMPLES NumberOfSamples = WaveFileSamples();
	NotifyViewsData data;
	data.HorizontalScroll.FirstSampleInView = 0.;
	data.HorizontalScroll.TotalSamplesInExtent = NumberOfSamples;

	while (NumberOfSamples != 0
			&& m_HorizontalScale > 1./8.
			&& NumberOfSamples / m_HorizontalScale < r.Width() / 2)
	{
		m_HorizontalScale /= 2;
	}

	data.HorizontalScroll.FirstSampleInView = 0.;
	NotifySiblingViews(HorizontalExtentChanged, &data);
	NotifySiblingViews(HorizontalScaleChanged, &m_HorizontalScale);

	BaseClass::OnInitialUpdate();
}

void CWaveSoapFrontView::OnUpdateViewZoominhor(CCmdUI* pCmdUI)
{
	pCmdUI->Enable(m_HorizontalScale > 1./8.);
}

void CWaveSoapFrontView::OnUpdateViewZoominhor2(CCmdUI* pCmdUI)
{
	pCmdUI->Enable(m_HorizontalScale > 1./8.);
}

void CWaveSoapFrontView::OnViewZoomInVert()
{
	if (m_VerticalScale < 1024.)
	{
		SetVerticalScale(m_VerticalScale * sqrt(2.));
	}
}

void CWaveSoapFrontView::SetVerticalScale(double NewVerticalScale)
{
	// correct the offset, if necessary
	// find max and min offset for this scale
	double offset = m_WaveOffsetY;
	double MaxOffset = 32768. * (1. - 1. / NewVerticalScale);
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

	NotifySiblingViews(VerticalScaleChanged, &NewVerticalScale);
}

void CWaveSoapFrontView::OnViewZoomOutVert()
{
	if (m_VerticalScale > 1.)
	{
		double scale = m_VerticalScale * sqrt(0.5);
		if (scale < 1.01)    // compensate any error
		{
			scale = 1.;
		}
		SetVerticalScale(scale);
	}
}

void CWaveSoapFrontView::OnViewZoomOutHor2()
{
	if (m_HorizontalScale < 8192.)// fixme: depending on the file size
	{
		SetHorizontalScale(m_HorizontalScale * 2, INT_MAX);
	}
}

void CWaveSoapFrontView::OnViewZoominHor2()
{
	if (m_HorizontalScale > 1./8.)
	{
		SetHorizontalScale(m_HorizontalScale / 2., INT_MAX);
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

	int DataEnd = SampleToXceil(pDoc->WaveFileSamples());

	if (p.x < DataEnd)
	{
		if (pDoc->WaveChannels() == 2)
		{
			int ch = GetChannelFromPoint(p.y);
			if (ch == 0 && p.y < SampleValueToY(0., ch))
			{
				result |= VSHT_LEFT_CHAN;
			}
			else if (ch == 1 && p.y > SampleValueToY(0., ch))
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
	if (! pDoc->m_WavFile.IsOpen())
	{
		return;
	}

	if (pDoc->m_PlayingSound && ! m_NewSelectionMade)
	{
		DestroyCaret();
		return;
	}

	CRect r;
	GetClientRect(r);

	CPoint p(SampleToX(pDoc->m_CaretPosition), r.top);

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
// FIXME    RemoveSelectionRect();

	CBrush backBrush(pApp->m_WaveBackground);

	CRect ClipRect;
	pDC->GetClipBox(ClipRect);
	if (0) TRACE("EraseBkgnd: ClipBox.left = %d, right=%d\n", ClipRect.left, ClipRect.right);

	CRect cr;
	GetClientRect(cr);

	int FileEnd = SampleToXceil(pDoc->WaveFileSamples());

	try
	{
		if (FileEnd < cr.right)
		{
			if (FileEnd < ClipRect.right)
			{
				CBitmap bmp;
				static const WORD pattern[] =
				{
					0x5555,
					0xAAAA,
					0x5555,
					0xAAAA,
					0x5555,
					0xAAAA,
					0x5555,
					0xAAAA,
				};

				// Windows98 can only use 8x8 bitmap for a brush
				bmp.CreateBitmap(8, 8, 1, 1, pattern);

				CBrush GrayBrush( & bmp);
				CRect GrayRect = cr;
				GrayRect.left = FileEnd;
				// set background and foreground color
				pDC->FillRect(GrayRect, & GrayBrush);
			}

			cr.right = FileEnd;
		}

		for (NUMBER_OF_CHANNELS ch = 0; ch < pDoc->WaveChannels(); ch++)
		{
			int SelBegin = SampleToX(pDoc->m_SelectionStart);
			int SelEnd = SampleToX(pDoc->m_SelectionEnd);

			CRect ChanR;
			GetChannelClipRect(ch, ChanR);

			ChanR.right = std::min(cr.right, ClipRect.right);
			ChanR.left = std::max(cr.left, ClipRect.left);

			if (ChanR.left >= ChanR.right)
			{
				continue;
			}

			if (pDoc->m_SelectionEnd != pDoc->m_SelectionStart
				&& SelEnd == SelBegin)
			{
				SelEnd++;
			}

			if (SelBegin >= ChanR.right
				|| SelEnd < ChanR.left
				|| pDoc->m_SelectionStart >= pDoc->m_SelectionEnd
				|| 0 == ((1 << ch) & pDoc->m_SelectedChannel))
			{
				// erase using only one brush
				pDC->FillRect(ChanR, & backBrush);
			}
			else
			{
				if (SelBegin > ChanR.left)
				{
					CRect r1 = ChanR;
					r1.right = SelBegin;
					pDC->FillRect(r1, & backBrush);
				}
				else
				{
					SelBegin = ChanR.left;
				}

				if (SelEnd < ChanR.right)
				{
					CRect r1 = ChanR;
					r1.left = SelEnd;
					pDC->FillRect(r1, & backBrush);
					ChanR.right = SelEnd;
				}

				ChanR.left = SelBegin;
				CBrush SelectedBackBrush(pApp->m_SelectedWaveBackground);

				pDC->FillRect(ChanR, & SelectedBackBrush);
			}
		}
	}
	catch (CResourceException * e)
	{
		TRACE("CResourceException\n");
		e->Delete();
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
	SAMPLE_INDEX nSampleUnderMouse = int(WindowXtoSample(point.x));
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
				SAMPLE_INDEX nBegin = SAMPLE_INDEX(WindowXtoSample(point.x));
				SAMPLE_INDEX nEnd = SAMPLE_INDEX(WindowXtoSample(point.x + 1));

				pDoc->SetSelection(nBegin, nEnd, pDoc->m_SelectedChannel, nBegin,
									SetSelection_SnapToMaximum
									| SetSelection_MakeCaretVisible);
			}
		}
		ReleaseCapture();
		bIsTrackingSelection = false;
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

	SAMPLE_INDEX nSampleUnderMouse = SAMPLE_INDEX(WindowXtoSample(point.x));
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
			if ( ! bIsTrackingSelection)
			{
				if (pDoc->m_CaretPosition >= nSampleUnderMouse
					&& pDoc->m_CaretPosition < SAMPLE_INDEX(WindowXtoSample(point.x + 1)))
				{
					// mouse didn't move outside this column
					return;
				}
				bIsTrackingSelection = true;
				SetCapture();
			}

			if (nHit & (VSHT_LEFT_AUTOSCROLL | VSHT_RIGHT_AUTOSCROLL))
			{
				if (NULL == m_AutoscrollTimerID)
				{
					m_AutoscrollTimerID = SetTimer(UINT_PTR(this+1), 50, NULL);
					if (TRACE_CARET) TRACE("Timer %X started\n", m_AutoscrollTimerID);
				}
			}
			else if (NULL != m_AutoscrollTimerID)
			{
				KillTimer(m_AutoscrollTimerID);
				m_AutoscrollTimerID = NULL;
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

			CHANNEL_MASK nChan = ALL_CHANNELS;
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

	CRect cr;
	GetClientRect(cr);

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

		AdjustCaretVisibility(pInfo->CaretPos, pInfo->OldCaretPos, pInfo->Flags);

		// calculate new selection boundaries
		int SelBegin = SampleToX(pInfo->SelBegin);
		int SelEnd = SampleToX(pInfo->SelEnd);

		if (pInfo->SelEnd != pInfo->SelBegin
			&& SelEnd == SelBegin)
		{
			SelEnd++;
		}

		int OldSelBegin = SampleToX(pInfo->OldSelBegin);
		int OldSelEnd = SampleToX(pInfo->OldSelEnd);

		if (pInfo->OldSelEnd != pInfo->OldSelBegin
			&& OldSelEnd == OldSelBegin)
		{
			OldSelEnd++;
		}

		for (int ch = 0; ch < pDoc->WaveChannels(); ch++)
		{
			CRect ChanR;
			GetChannelClipRect(ch, ChanR);
			// build rectangles with selection boundaries

			CRect r1(SelBegin, ChanR.top, SelEnd, ChanR.bottom);
			CRect r2(OldSelBegin, ChanR.top, OldSelEnd, ChanR.bottom);

			// invalidate the regions with changed selection
			if (0 == (pInfo->SelChannel & (1 << ch)))
			{
				r1.right = r1.left;
			}

			if (0 == (pInfo->OldSelChannel & (1 << ch)))
			{
				r2.right = r2.left;
			}

			int x[4] = {r1.left, r1.right, r2.left, r2.right };

			// sort all 'x' coordinates in ascending order
			if (x[0] > x[2])
			{
				x[0] = r2.left;
				x[2] = r1.left;
			}
			if (x[1] > x[3])
			{
				x[1] = r2.right;
				x[3] = r1.right;
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

			// invalidate two rectangles
			if (r1.left != r1.right
				// limit the rectangles with the window boundaries
				&& r1.left < cr.right
				&& r1.right > cr.left)
			{
				// non-empty, in the client
				if (r1.left < cr.left)
				{
					r1.left = cr.left;
				}
				if(r1.right > cr.right)
				{
					r1.right = cr.right;
				}
				InvalidateRect(& r1);
			}

			if (r2.left != r2.right
				// limit the rectangles with the window boundaries
				&& r2.left < cr.right
				&& r2.right > cr.left)
			{
				// non-empty, in the client
				if (r2.left < cr.left)
				{
					r2.left = cr.left;
				}
				if(r2.right > cr.right)
				{
					r2.right = cr.right;
				}
				InvalidateRect(& r2);
			}
		}
		CreateAndShowCaret();
	}
	else if (lHint == ThisDoc::UpdateSoundChanged
			&& NULL != pHint)
	{
		CSoundUpdateInfo * pInfo = static_cast<CSoundUpdateInfo *> (pHint);
		ASSERT(NULL != pInfo);

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

			UpdateHorizontalExtents(pInfo->m_NewLength, r.Width());
			Invalidate();
		}
		else
		{
			// TODO: invalidate only if in the range
			m_WaveBuffer.Invalidate(); // invalidate the data in draw buffer
		}

		// calculate update boundaries
		r1.left = SampleToX(pInfo->m_Begin);
		r1.right = SampleToXceil(pInfo->m_End) + 2;

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
		ASSERT(NULL != pInfo);

		UpdatePlaybackCursor(pInfo->m_PlaybackPosition, pInfo->m_PlaybackChannel);
	}
	else if (lHint == ThisDoc::UpdateSampleRateChanged
			&& NULL == pHint)
	{
		// don't do anything
	}
	else if (lHint == ThisDoc::UpdateWholeFileChanged)
	{
		// recalculate the extents
		UpdateHorizontalExtents(GetDocument()->WaveFileSamples(), cr.Width());
		Invalidate();
	}
	else if (lHint == ThisDoc::UpdateMarkerRegionChanged
			&& NULL != pHint)
	{
		MarkerRegionUpdateInfo * pInfo = static_cast<MarkerRegionUpdateInfo *> (pHint);
		ASSERT(NULL != pInfo);

		InvalidateMarkerRegion( & pInfo->info);
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

void CWaveSoapFrontView::InvalidateMarkerRegion(WAVEREGIONINFO const * pInfo)
{
	CRect cr;
	GetClientRect(cr);
	CRect r;

	long x = SampleToX(pInfo->Sample);
	if (0 != (pInfo->Flags & (pInfo->ChangeSample | pInfo->Delete))
		&& x < cr.right && x >= cr.left)
	{
		// invalidate region begin marker
		r.left = x;
		r.right = x + 1;
		r.top = cr.top;
		r.bottom = cr.bottom;

		InvalidateRect(r);
	}

	if (x < cr.right
		&& 0 != (pInfo->Flags &
			(pInfo->ChangeSample | pInfo->Delete | pInfo->ChangeLabel
				| pInfo->ChangeComment | pInfo->ChangeLtxt)))
	{
		// invalidate text area
		LPCTSTR str = NULL;

		if (NULL != pInfo->Label
			&& 0 != pInfo->Label[0])
		{
			if (pInfo->Flags & (pInfo->ChangeSample | pInfo->ChangeLabel | pInfo->Delete))
			{
				str = pInfo->Label;
			}
		}
		else if (NULL != pInfo->Comment
				&& 0 != pInfo->Comment[0])
		{
			if (pInfo->Flags & (pInfo->ChangeSample | pInfo->ChangeComment | pInfo->Delete))
			{
				str = pInfo->Comment;
			}
		}
		else if (NULL != pInfo->Ltxt
				&& 0 != pInfo->Ltxt[0])
		{
			if (pInfo->Flags & (pInfo->ChangeSample | pInfo->ChangeLtxt | pInfo->Delete))
			{
				str = pInfo->Ltxt;
			}
		}

		if (NULL != str)
		{
			CWindowDC dc(this);
			CGdiObjectSave OldFont(dc, dc.SelectStockObject(ANSI_VAR_FONT));

			CPoint p(dc.GetTextExtent(str, (int)_tcslen(str)));

			r.left = x;
			r.top = cr.top;
			r.bottom = r.top + p.y;
			r.right = r.left + p.x;

			if (r.right > cr.left)
			{
				InvalidateRect(r);
			}
		}
	}

	if (0 != pInfo->Length
		&& 0 != (pInfo->Flags
			& (pInfo->ChangeSample | pInfo->ChangeLength | pInfo->Delete)))
	{
		// invalidate end marker
		x = SampleToX(pInfo->Sample + pInfo->Length);
		if (x < cr.right && x >= cr.left)
		{
			r.left = x;
			r.right = x + 1;
			r.top = cr.top;
			r.bottom = cr.bottom;

			InvalidateRect(r);
		}
	}
}

POINT CWaveSoapFrontView::GetZoomCenter()
{
	ThisDoc * pDoc = GetDocument();
	int caret = SampleToX(pDoc->m_CaretPosition);
	int SelBegin = SampleToX(pDoc->m_SelectionStart);
	int SelEnd = SampleToX(pDoc->m_SelectionEnd);
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
	BOOL KeepCaretVisible = FALSE;

	int nCaretMove = (int)m_HorizontalScale;
	if (nCaretMove == 0)    // m_HorizontalScale < 1
	{
		nCaretMove = 1;
	}
	NUMBER_OF_SAMPLES nTotalSamples = pDoc->WaveFileSamples();

	CRect r;
	GetClientRect(r);

	// page is one half of the window width
	double nPage = r.Width() * m_HorizontalScale / 2;
	if (CtrlPressed)
	{
		// make it 7/8 of the window width
		nPage = r.Width() * m_HorizontalScale * 7. / 8.;
	}
	// round to one pixel
	nPage -= fmod(nPage, m_HorizontalScale);

	switch (nChar)
	{
	case VK_LEFT:
		if (CtrlPressed)
		{
			nCaret = pDoc->GetPrevMarker();
			KeepCaretVisible = TRUE;
		}
		else
		{
			if (nSelBegin < nSelEnd
				&& ! KeepSelection)
			{
				// move caret to the selection start, cancel selection
				nCaret = nSelBegin;
			}
			else
			{
				// move caret 1 pixel or 1 sample to the left,
				nCaret -= nCaretMove;
			}
		}
		break;

	case VK_RIGHT:
		if (CtrlPressed)
		{
			nCaret = pDoc->GetNextMarker();
			KeepCaretVisible = TRUE;
		}
		else
		{
			if (nSelBegin < nSelEnd
				&& ! KeepSelection)
			{
				// move caret to the selection end, cancel selection
				nCaret = nSelEnd;
			}
			else
			{
				// move caret 1 to the right
				nCaret += nCaretMove;
			}
		}
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
			nCaret = SAMPLE_INDEX(WindowXtoSample(r.left + 1)); // cursor to the left boundary + 1
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
			nCaret = SAMPLE_INDEX(WindowXtoSample(r.right - 2)); // cursor to the right boundary + 1
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
		KeepSelection = TRUE;
		break;

	case VK_DOWN:
		// move the zoomed image down
		KeepSelection = TRUE;
		break;

	case VK_TAB:
		// toggle selection channel
		if (pDoc->m_WavFile.AllChannels(nChan))
		{
			nChan = 1;
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

	if (KeepCaretVisible)
	{
		AdjustCaretVisibility(nCaret, pDoc->m_CaretPosition, SetSelection_KeepCaretVisible);
	}
	else
	{
		MovePointIntoView(nCaret, MakeCaretVisible);
	}

	pDoc->SetSelection(nSelBegin, nSelEnd, nChan, nCaret);

	CView::OnKeyDown(nChar, nRepCnt, nFlags);
}

void CWaveSoapFrontView::MovePointIntoView(SAMPLE_INDEX nCaret, BOOL bCenter)
{
	CRect r;
	GetClientRect(r);

	int nDesiredPos = SampleToX(nCaret);
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
	HorizontalScrollBy(scroll);
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

	ShowScrollBar(SB_VERT, FALSE);
	ShowScrollBar(SB_HORZ, FALSE);
	return 0;
}

void CWaveSoapFrontView::HorizontalScrollBy(double samples)
{
	SetFirstSampleInView(m_FirstSampleInView + samples);
}

void CWaveSoapFrontView::SetFirstSampleInView(double sample)
{
	NUMBER_OF_SAMPLES TotalSamples = WaveFileSamples();
	CRect cr;
	GetClientRect(cr);
	double SamplesInView = cr.Width() * m_HorizontalScale;
	int ReservedPixels = cr.Width() / 10;
	if (ReservedPixels > 100)
	{
		ReservedPixels = 100;
	}
	double MaximumFirstSampleValue = TotalSamples + ReservedPixels * m_HorizontalScale - SamplesInView;
	if (MaximumFirstSampleValue < 0.)
	{
		MaximumFirstSampleValue = 0.;
	}

	if (sample > MaximumFirstSampleValue)
	{
		sample = MaximumFirstSampleValue;
	}

	if (sample < 0.)
	{
		sample = 0.;
	}
	// adjust it for integer number of pixels
	sample -= fmod(sample, m_HorizontalScale);
	// the update will reflect here
	NotifySiblingViews(HorizontalOriginChanged, &sample);
}

void CWaveSoapFrontView::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
	// calc new x position
	NUMBER_OF_SAMPLES TotalSamples = WaveFileSamples();
	CRect cr;
	GetClientRect(cr);
	double SamplesInView = cr.Width() * m_HorizontalScale;
	// at the end of the file we reserve at least 100 pixels or up to 10% of the view width after EOF

	switch (nSBCode)
	{
	case SB_LEFT:
		SetFirstSampleInView(0.);
		return;
		break;
	case SB_RIGHT:
		SetFirstSampleInView(TotalSamples);
		return;
		break;
	case SB_LINELEFT:
		HorizontalScrollBy(-SamplesInView * 0.05);
		break;
	case SB_LINERIGHT:
		HorizontalScrollBy(SamplesInView * 0.05);
		break;
	case SB_PAGELEFT:
		HorizontalScrollBy(-SamplesInView * 0.9);
		break;
	case SB_PAGERIGHT:
		HorizontalScrollBy(SamplesInView * 0.9);
		break;
		//case SB_THUMBPOSITION:    // sent when THUMBTRACK is released
	case SB_THUMBTRACK:
	{
		int ReservedPixels = cr.Width() / 10;
		if (ReservedPixels > 100)
		{
			ReservedPixels = 100;
		}
		SCROLLINFO scrollinfo = {sizeof scrollinfo};

		pScrollBar->GetScrollInfo( & scrollinfo, SIF_ALL);
		double MaximumFirstSampleValue = TotalSamples + ReservedPixels * m_HorizontalScale - SamplesInView;
		TRACE("SB_THUMBTRACK pos=%d, trackpos=%d, new first sample=%f\n", nPos, scrollinfo.nTrackPos,
			MaximumFirstSampleValue * double(scrollinfo.nTrackPos - scrollinfo.nMin) / (scrollinfo.nMax - scrollinfo.nMin));
		SetFirstSampleInView(MaximumFirstSampleValue * double(scrollinfo.nTrackPos - scrollinfo.nMin) / (scrollinfo.nMax - scrollinfo.nMin));

		break;
	}
	default:
		break;
	}

}

BOOL CWaveSoapFrontView::MasterScrollBy(double dx, double dy, BOOL bDoScroll)
{
	CWaveSoapFrontDoc * pDoc = GetDocument();

	LONG FileEnd = SampleToXceil(pDoc->WaveFileSamples());
	LONG NewFileEnd = FileEnd;

	CRect ir;
	CRect cr;
	GetClientRect(cr);

	int ndx = 0;
	int ndy = 0;
	bool PlaybackCursorHidden = false;
#if 0
	if (dy != 0.)
	{
		// check for the limits
		// ndy is in pixels

		CRect r;
		GetChannelRect(0, r);

		ndy = -fround(dy * GetYScaleDev());

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
// FIXME            NotifySlaveViews(WAVE_OFFSET_CHANGED);

			if (NoScroll)
			{
				Invalidate();
				return TRUE;
			}

			int nChannels = pDoc->WaveChannels();

			HidePlaybackCursor();
			PlaybackCursorHidden = true;

			for (int ch = 0; ch < nChannels; ch++)
			{
				CRect chr;
				GetChannelClipRect(ch, chr);
#if 1
				ScrollWindowEx(0, ndy, chr, chr, NULL, ir,
								SW_INVALIDATE);
				InvalidateRect(ir);
#else
				ScrollWindowEx(0, ndy, NULL, NULL, NULL, NULL,
								SW_INVALIDATE | SW_ERASE);
#endif
			}

		}
	}
#endif

	//  invalidate all labels of markers, if they fall to the moved area
	// make sure the new position will be on the multiple of m_HorizontalScale
	if (ndy != 0
		|| ndx != 0)
	{
		// invalidate marker labels
		CWindowDC dc(this);
		CGdiObjectSave OldFont(dc, dc.SelectStockObject(ANSI_VAR_FONT));

		CWaveFile::InstanceDataWav * pInst = pDoc->m_WavFile.GetInstanceData();

		for (CuePointVectorIterator i = pInst->m_CuePoints.begin();
			i != pInst->m_CuePoints.end(); i++)
		{
			long x = SampleToX(i->dwSampleOffset);

			// invalidate text
			if (x < cr.right)
			{
				LPCTSTR txt = pInst->GetCueText(i->CuePointID);
				if (NULL != txt)
				{
					CPoint p(dc.GetTextExtent(txt, (int)_tcslen(txt)));

					ir.left = x;
					ir.top = cr.top;
					ir.bottom = cr.top + p.y;
					ir.right = x + p.x;

					// in the wave area, invalidate old and new position,
					// only if vertical scroll was done.
					// In the background area, invalidate only if horizontal scroll was done
					if (x + p.x > cr.left)
					{
						if (ndy != 0)
						{
							InvalidateRect(ir);
							if (ndy > 0)
							{
								// scroll down
								// invalidate old position
								if (ndy < cr.bottom - cr.top)
								{
									InvalidateRect(CRect(x, cr.top + ndy, x + p.x, cr.top + p.y + ndy));
								}
							}
						}

						if (ndx != 0
							&& x + p.x > NewFileEnd)
						{
							InvalidateRect(ir, FALSE);
							if (ndx < 0)
							{
								ir.left = ir.right;
								ir.right -= ndx;
								InvalidateRect(ir, TRUE);
							}
						}
					}
				}
			}
		}
	}
	if (PlaybackCursorHidden)
	{
		ShowPlaybackCursor();
	}

	return TRUE;
}

void CWaveSoapFrontView::OnSize(UINT nType, int cx, int cy)
{
	UpdateHorizontalExtents(WaveFileSamples(), cx);

	BaseClass::OnSize(nType, cx, cy);

	CreateAndShowCaret();
}

void CWaveSoapFrontView::OnViewZoomvertNormal()
{
	SetVerticalScale(1.);
}

void CWaveSoapFrontView::OnUpdateViewZoomvertNormal(CCmdUI* pCmdUI)
{
	pCmdUI->Enable(m_VerticalScale > 1.);
}

void CWaveSoapFrontView::OnViewZoominHorFull()
{
	SetHorizontalScale(1., INT_MAX);
}

void CWaveSoapFrontView::OnUpdateViewZoominHorFull(CCmdUI* pCmdUI)
{
	pCmdUI->Enable(m_HorizontalScale > 1);
}

// the function scrolls the real image, and modifies dOrgX, dOrgY.
#if 0
BOOL CWaveSoapFrontView::OnScrollBy(CSize sizeScroll, BOOL bDoScroll, RECT const * pScrollRect, RECT const * pClipRect)
{
	if (bDoScroll)
	{
		HidePlaybackCursor();
	}

	BOOL bRet = BaseClass::OnScrollBy(sizeScroll, bDoScroll, pScrollRect, pClipRect);
	if (bDoScroll)
	{
		ShowPlaybackCursor();
	}
	return bRet;
}
#endif

void CWaveSoapFrontView::UpdatePlaybackCursor(SAMPLE_INDEX sample, CHANNEL_MASK channel)
{
	if (0 == m_PlaybackCursorChannel)
	{
		// first call after playback start
		m_PlaybackCursorDrawnSamplePos = sample;
		m_NewSelectionMade = false; // to hide the caret
	}

	int pos = SampleToX(sample);
	int OldPos = SampleToX(m_PlaybackCursorDrawnSamplePos);

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
	// FIXME: if the selection to play is completely visible, do not scroll
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
		HorizontalScrollBy(m_HorizontalScale * (pos - NewPos));
	}
	ShowPlaybackCursor();
	//GdiFlush();
}

void CWaveSoapFrontView::UpdateHorizontalExtents(NUMBER_OF_SAMPLES Length, int WindowWidth)
{
	// if the file fits into the view, adjust the horizontal scale
	// if the previous view l
	double SamplesInView = WindowWidth * m_HorizontalScale;
	int ReservedPixels = WindowWidth / 10;
	if (ReservedPixels > 100)
	{
		ReservedPixels = 100;
	}
	double EquivalentSamplesInExtent = Length + ReservedPixels * m_HorizontalScale;
	double MaximumFirstSampleValue =  EquivalentSamplesInExtent - SamplesInView;
	if (MaximumFirstSampleValue < 0.)
	{
		MaximumFirstSampleValue = 0.;
	}

	double sample = m_FirstSampleInView;

	if (sample > MaximumFirstSampleValue)
	{
		sample = MaximumFirstSampleValue;
	}

	if (sample < 0.)
	{
		sample = 0.;
	}
	// adjust it for integer number of pixels
	sample -= fmod(sample, m_HorizontalScale);

	NotifyViewsData data;
	data.HorizontalScroll.FirstSampleInView = sample;
	data.HorizontalScroll.TotalSamplesInView = SamplesInView;
	data.HorizontalScroll.TotalSamplesInExtent = EquivalentSamplesInExtent;

	NotifySiblingViews(HorizontalExtentChanged, &data);
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
	// point is in client coordinates
	CView::OnRButtonDown(nFlags, point);
	DWORD nHit = ClientHitTest(point);
	if ((nHit & VSHT_NONCLIENT)
		|| (nHit & VSHT_SELECTION))
	{
		return;
	}

	ThisDoc * pDoc = GetDocument();

	SAMPLE_INDEX nBegin = SAMPLE_INDEX(WindowXtoSample(point.x));
	SAMPLE_INDEX nEnd = SAMPLE_INDEX(WindowXtoSample(point.x + 1));

	CHANNEL_MASK nChan = ALL_CHANNELS;
	if (nHit & VSHT_LEFT_CHAN)
	{
		nChan = SPEAKER_FRONT_LEFT;
	}
	else if (nHit & VSHT_RIGHT_CHAN)
	{
		nChan = SPEAKER_FRONT_RIGHT;
	}

	pDoc->SetSelection(nBegin, nEnd, nChan, nBegin,
						SetSelection_SnapToMaximum
						| SetSelection_MakeCaretVisible);

	OnSetCursor(this, HTCLIENT, WM_RBUTTONDOWN);
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
	NUMBER_OF_SAMPLES SamplesInSelection = pDoc->m_SelectionStart - pDoc->m_SelectionEnd;

	CRect cr;
	GetClientRect(cr);
	if (SamplesInSelection < 0)
	{
		SamplesInSelection = - SamplesInSelection;
	}
	double hor_scale = 1;
	if (cr.Width() > SamplesInSelection)
	{
		while (cr.Width() > SamplesInSelection && hor_scale > 1./8.)
		{
			SamplesInSelection *= 2;
			hor_scale /= 2;
		}
	}
	else
	{
		// Width < samples)
		while (cr.Width() < SamplesInSelection)
		{
			SamplesInSelection /= 2;
			hor_scale *= 2;
		}
	}
	SetHorizontalScale(hor_scale, pDoc->m_SelectionStart + (pDoc->m_SelectionEnd - pDoc->m_SelectionStart) / 2);
}

void CWaveSoapFrontView::OnUpdateIndicatorScale(CCmdUI* pCmdUI)
{
	CString s;
	if (m_HorizontalScale >= 1)
	{
		s.Format(_T("1:%d"), int(m_HorizontalScale));
	}
	else
	{
		s.Format(_T("%d:1"), int(1. / m_HorizontalScale));
	}

	SetStatusString(pCmdUI, s);
}

void CWaveSoapFrontView::OnUpdateViewHorScale(CCmdUI* pCmdUI)
{
	C_ASSERT(ID_VIEW_HOR_SCALE_2 == ID_VIEW_HOR_SCALE_1 + 1);
	C_ASSERT(ID_VIEW_HOR_SCALE_4 == ID_VIEW_HOR_SCALE_1 + 2);
	C_ASSERT(ID_VIEW_HOR_SCALE_8 == ID_VIEW_HOR_SCALE_1 + 3);
	C_ASSERT(ID_VIEW_HOR_SCALE_16 == ID_VIEW_HOR_SCALE_1 + 4);
	C_ASSERT(ID_VIEW_HOR_SCALE_32 == ID_VIEW_HOR_SCALE_1 + 5);
	C_ASSERT(ID_VIEW_HOR_SCALE_64 == ID_VIEW_HOR_SCALE_1 + 6);
	C_ASSERT(ID_VIEW_HOR_SCALE_128 == ID_VIEW_HOR_SCALE_1 + 7);
	C_ASSERT(ID_VIEW_HOR_SCALE_256 == ID_VIEW_HOR_SCALE_1 + 8);
	C_ASSERT(ID_VIEW_HOR_SCALE_512 == ID_VIEW_HOR_SCALE_1 + 9);
	C_ASSERT(ID_VIEW_HOR_SCALE_1024 == ID_VIEW_HOR_SCALE_1 + 10);
	C_ASSERT(ID_VIEW_HOR_SCALE_2048 == ID_VIEW_HOR_SCALE_1 + 11);
	C_ASSERT(ID_VIEW_HOR_SCALE_4096 == ID_VIEW_HOR_SCALE_1 + 12);
	C_ASSERT(ID_VIEW_HOR_SCALE_8192 == ID_VIEW_HOR_SCALE_1 + 13);

	pCmdUI->SetRadio((1 << (pCmdUI->m_nID - ID_VIEW_HOR_SCALE_1)) == m_HorizontalScale);
}

void CWaveSoapFrontView::OnViewHorScale(UINT command)
{
	SetHorizontalScale(1 << (command - ID_VIEW_HOR_SCALE_1), INT_MAX);
}

void CWaveSoapFrontView::OnTimer(UINT_PTR nIDEvent)
{
	// get mouse position and hit code
	if (NULL != m_AutoscrollTimerID
		&& nIDEvent == m_AutoscrollTimerID)
	{
		CPoint p;
		GetCursorPos( & p);
		ScreenToClient( & p);
		DWORD nHit = ClientHitTest(p);

		if (nHit & ( VSHT_RIGHT_AUTOSCROLL | VSHT_LEFT_AUTOSCROLL))
		{
			//TRACE("OnTimer: VSHT_RIGHT_AUTOSCROLL\n");
			double scroll;
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
				scroll = -m_HorizontalScale;
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

			HorizontalScrollBy(scroll);

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
			KillTimer(m_AutoscrollTimerID);
			m_AutoscrollTimerID = NULL;
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
	if (pWnd != this)
	{
		if (NULL != m_AutoscrollTimerID)
		{
			if (TRACE_SCROLL) TRACE("Killing timer in CWaveSoapFrontView::OnCaptureChanged\n");
			KillTimer(m_AutoscrollTimerID);
			m_AutoscrollTimerID = NULL;
		}
		bIsTrackingSelection = FALSE;
		nKeyPressed = 0;
	}
	BaseClass::OnCaptureChanged(pWnd);
}

void CWaveSoapFrontView::OnLButtonDblClk(UINT nFlags, CPoint point)
{
	BaseClass::OnLButtonDblClk(nFlags, point);

	GetDocument()->SelectBetweenMarkers(SAMPLE_INDEX(WindowXtoSample(point.x)));
}

void CWaveSoapFrontView::AdjustCaretVisibility(SAMPLE_INDEX CaretPos, SAMPLE_INDEX OldCaretPos,
												unsigned Flags)
{
	ThisDoc * pDoc = GetDocument();
	CRect cr;
	GetClientRect(cr);
	// change for foreground frame only
	CFrameWnd * pFrameWnd = GetParentFrame();

	if (NULL != pFrameWnd
		&& pFrameWnd == pFrameWnd->GetWindow(GW_HWNDFIRST))
	{
		if (SetSelection_KeepCaretVisible ==
			(Flags & SetSelection_KeepCaretVisible))
		{
			// move caret toward center
			// if the caret was from the left side, allow it to stay on the left side,
			// but when moved to the right, keep it in center.
			// If the caret was or becomes not visible, bring it to center
			long PrevX = SampleToX(OldCaretPos);
			long NewX = SampleToX(CaretPos);
			int Center = (cr.left + cr.right) / 2;

			if (PrevX < cr.left
				|| PrevX >= cr.right
				|| NewX < cr.left
				|| NewX >= cr.right)
			{
				MovePointIntoView(CaretPos, TRUE);
			}
			else if (PrevX < Center)
			{
				if (NewX > Center)
				{
					MovePointIntoView(CaretPos, TRUE);
				}
				else
				{
					MovePointIntoView(CaretPos, FALSE);
				}
			}
			else if (PrevX > Center)
			{
				if (NewX < Center)
				{
					MovePointIntoView(CaretPos, TRUE);
				}
				else
				{
					MovePointIntoView(CaretPos, FALSE);
				}
			}
			else
			{
				// keep centered
				MovePointIntoView(CaretPos, TRUE);
			}
		}
		else if (Flags & SetSelection_MakeCaretVisible)
		{
			MovePointIntoView(CaretPos, FALSE);
		}
		else if (Flags & SetSelection_MoveCaretToCenter)
		{
			MovePointIntoView(CaretPos, TRUE);
		}
	}

	if (Flags & SetSelection_MakeFileVisible)
	{
		SAMPLE_INDEX LastSample = pDoc->WaveFileSamples();
		if (WindowXtoSample(0) > LastSample)
		{
			MovePointIntoView(LastSample, TRUE);
		}
	}
}

void CWaveSoapFrontView::OnViewZoomprevious()
{
	if (m_PrevHorizontalScale >= 1)
	{
		double zoom = m_PrevHorizontalScale;
		m_PrevHorizontalScale = m_HorizontalScale;

		SetHorizontalScale(zoom, INT_MAX);
	}
}

// if ZoomCenter is INT_MAX, use current caret position
void CWaveSoapFrontView::SetHorizontalScale(double NewHorizontalScale, int ZoomCenter)
{
	// TODO
	NotifySiblingViews(HorizontalScaleChanged, &NewHorizontalScale);
	MovePointIntoView(GetDocument()->m_CaretPosition, TRUE);
}

int CWaveSoapFrontView::SampleToXceil(SAMPLE_INDEX sample) const
{
	double pos = (sample - m_FirstSampleInView) / m_HorizontalScale;
	if (pos > INT_MAX / 2)
	{
		return INT_MAX / 2;
	}
	else if (pos < INT_MIN / 2)
	{
		return INT_MIN / 2;
	}
	else
	{
		return (int)ceil(pos);
	}
}

int CWaveSoapFrontView::SampleToX(SAMPLE_INDEX sample) const
{
	double pos = (sample - m_FirstSampleInView) / m_HorizontalScale;
	if (pos > INT_MAX / 2)
	{
		return INT_MAX / 2;
	}
	else if (pos < INT_MIN / 2)
	{
		return INT_MIN / 2;
	}
	else
	{
		return (int)floor(pos);
	}
}

SAMPLE_INDEX CWaveSoapFrontView::WindowXtoSample(int x) const
{
	return (SAMPLE_INDEX)floor(x * m_HorizontalScale + m_FirstSampleInView);
}

afx_msg LRESULT CWaveSoapFrontView::OnUwmNotifyViews(WPARAM wParam, LPARAM lParam)
{
	CRect cr;
	GetClientRect(cr);

	switch (wParam)
	{
	case ChannelHeightChanged:

		break;
	case FftBandChanged:
		break;
	case HorizontalScaleChanged:
		if (m_HorizontalScale != *(double*)lParam)
		{
			m_HorizontalScale = *(double*)lParam;
			Invalidate();
		}
		// notify the siblings of the change in extents:
		{
			NotifyViewsData data;
			data.HorizontalScroll.TotalSamplesInView = cr.Width() * m_HorizontalScale;

			NUMBER_OF_SAMPLES TotalSamples = WaveFileSamples();

			int ReservedPixels = cr.Width() / 10;
			if (ReservedPixels > 100)
			{
				ReservedPixels = 100;
			}
			data.HorizontalScroll.TotalSamplesInExtent = TotalSamples + ReservedPixels * m_HorizontalScale;
			double MaximumFirstSampleValue = data.HorizontalScroll.TotalSamplesInExtent - data.HorizontalScroll.TotalSamplesInView;

			if (MaximumFirstSampleValue < 0.)
			{
				MaximumFirstSampleValue = 0.;
			}

			data.HorizontalScroll.FirstSampleInView = m_FirstSampleInView;
			if (data.HorizontalScroll.FirstSampleInView > MaximumFirstSampleValue)
			{
				data.HorizontalScroll.FirstSampleInView = MaximumFirstSampleValue;
			}

			if (data.HorizontalScroll.FirstSampleInView < 0.)
			{
				data.HorizontalScroll.FirstSampleInView = 0.;
			}
			// adjust it for integer number of pixels
			data.HorizontalScroll.FirstSampleInView -= fmod(data.HorizontalScroll.FirstSampleInView, m_HorizontalScale);
			NotifySiblingViews(HorizontalExtentChanged, &data);
		}
		break;
	case VerticalScaleChanged:
		// lParam points to double new scale
		if (m_VerticalScale != *(double*)lParam)
		{
			m_VerticalScale = *(double*)lParam;
			Invalidate();
		}
		break;
	case AmplitudeOffsetChanged:
		// lParam points to double new vertical offset
		break;
	case FftVerticalScaleChanged:
		// lParam points to double new scale
		break;
	case FftOffsetChanged:
		break;
	case HorizontalExtentChanged:
	{
		NotifyViewsData *data = (NotifyViewsData *) lParam;
		m_FirstSampleInView = data->HorizontalScroll.FirstSampleInView;
	}
		break;
	case HorizontalOriginChanged:
		double NewFirstSample = *(double*) lParam;  // it's validated to be in the proper range and adjusted for whole pixels
		// see if we do scroll or invalidation
		double offset_pixels = (NewFirstSample - m_FirstSampleInView) / m_HorizontalScale;
		ASSERT(0. == fmod(offset_pixels, 1.));
		m_FirstSampleInView = NewFirstSample;

		if (offset_pixels > 0.)     // shift image to the left
		{
			if (offset_pixels < cr.Width())
			{
				ScrollWindow(-int(offset_pixels), 0, cr, cr);
			}
			else
			{
				Invalidate(FALSE);
			}
		}
		else if (offset_pixels < 0.)    // shift image to the right
		{
			if (-offset_pixels < cr.Width())
			{
				ScrollWindow(-int(offset_pixels), 0, cr, cr);
			}
			else
			{
				Invalidate(FALSE);
			}
		}
		else
		{
			// do nothing
		}
	}
	return 0;
}
