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

IMPLEMENT_DYNCREATE(CWaveSoapFrontView, CWaveSoapViewBase);
IMPLEMENT_DYNAMIC(CWaveSoapViewBase, CView);

BEGIN_MESSAGE_MAP(CWaveSoapFrontView, BaseClass)
	//{{AFX_MSG_MAP(CWaveSoapFrontView)
	ON_WM_ERASEBKGND()
	ON_WM_CREATE()
	ON_UPDATE_COMMAND_UI(ID_VIEW_ZOOMINVERT, OnUpdateViewZoomInVert)
	ON_COMMAND(ID_VIEW_ZOOMINVERT, OnViewZoomInVert)
	ON_UPDATE_COMMAND_UI(ID_VIEW_ZOOMOUTVERT, OnUpdateViewZoomOutVert)
	ON_COMMAND(ID_VIEW_ZOOMOUTVERT, OnViewZoomOutVert)
	ON_UPDATE_COMMAND_UI(ID_VIEW_ZOOMINHOR2, OnUpdateViewZoominhor2)
	ON_COMMAND(ID_VIEW_ZOOMINHOR2, OnViewZoominHor2)
	ON_COMMAND(ID_VIEW_ZOOMOUTHOR2, OnViewZoomOutHor2)
	ON_COMMAND(ID_VIEW_ZOOMVERT_NORMAL, OnViewZoomvertNormal)
	ON_UPDATE_COMMAND_UI(ID_VIEW_ZOOMVERT_NORMAL, OnUpdateViewZoomvertNormal)
	ON_COMMAND(ID_VIEW_ZOOMPREVIOUS, OnViewZoomprevious)
	ON_COMMAND(ID_VIEW_ZOOMIN_HOR_FULL, OnViewZoominHorFull)
	ON_UPDATE_COMMAND_UI(ID_VIEW_ZOOMIN_HOR_FULL, OnUpdateViewZoominHorFull)
	ON_UPDATE_COMMAND_UI(ID_VIEW_ZOOM_SELECTION, OnUpdateViewZoomSelection)
	ON_COMMAND(ID_VIEW_ZOOM_SELECTION, OnViewZoomSelection)
	ON_UPDATE_COMMAND_UI(ID_INDICATOR_SCALE, OnUpdateIndicatorScale)
	ON_UPDATE_COMMAND_UI_RANGE(ID_VIEW_HOR_SCALE_1, ID_VIEW_HOR_SCALE_8192, OnUpdateViewHorScale)
	ON_COMMAND_RANGE(ID_VIEW_HOR_SCALE_1, ID_VIEW_HOR_SCALE_8192, OnViewHorScale)
	ON_COMMAND(ID_VIEW_ZOOMVERT_NORMAL, OnViewZoomvertNormal)
	ON_UPDATE_COMMAND_UI(ID_VIEW_ZOOMVERT_NORMAL, OnUpdateViewZoomvertNormal)
	ON_COMMAND(ID_VIEW_ZOOMIN_HOR_FULL, OnViewZoominHorFull)
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
	ON_COMMAND_RANGE(ID_VIEW_MINIMIZE_0, ID_VIEW_MINIMIZE_31, &CWaveSoapFrontView::OnViewMinimize0)
	ON_COMMAND_RANGE(ID_VIEW_MAXIMIZE_0, ID_VIEW_MAXIMIZE_31, &CWaveSoapFrontView::OnViewMaximize0)
	//ON_UPDATE_COMMAND_UI(ID_VIEW_MINIMIZE_0, &CWaveSoapFrontView::OnUpdateViewMinimize0)
END_MESSAGE_MAP()

BEGIN_MESSAGE_MAP(CWaveSoapViewBase, BaseClass)
	//{{AFX_MSG_MAP(CWaveSoapViewBase)
	ON_WM_SETCURSOR()
	ON_WM_KILLFOCUS()
	ON_WM_SETFOCUS()
	ON_WM_ERASEBKGND()
	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONUP()
	ON_WM_RBUTTONDOWN()
	ON_WM_RBUTTONUP()
	ON_WM_MOUSEWHEEL()
	ON_WM_MOUSEMOVE()
	ON_WM_KEYDOWN()
	ON_WM_CREATE()
	ON_WM_SIZE()
	ON_WM_TIMER()
	ON_WM_CAPTURECHANGED()
	ON_WM_LBUTTONDBLCLK()
	//ON_WM_CONTEXTMENU()
	ON_WM_DESTROY()
	//}}AFX_MSG_MAP
	ON_MESSAGE(UWM_NOTIFY_VIEWS, &CWaveSoapViewBase::OnUwmNotifyViews)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CWaveSoapFrontView construction/destruction

CWaveSoapViewBase::CWaveSoapViewBase()
	: m_HorizontalScale(2048),
	m_PrevHorizontalScale(-1),
	m_PlaybackCursorChannel(0),
	m_PlaybackCursorDrawn(false),
	m_NewSelectionMade(false),
	m_AutoscrollTimerID(0),
	m_PlaybackCursorDrawnSamplePos(0),
	m_WheelAccumulator(0),
	m_FirstSampleInView(0.),
	bIsTrackingSelection(false),
	nKeyPressed(0)
	, m_HasFocus(false)
	, m_CaretShown(false)
	, m_CaretCreated(false)
	, m_CurrentCaretChannels(0)
{
	memzero(m_Heights);
	memzero(m_InvalidAreaTop);
	memzero(m_InvalidAreaBottom);

	TRACE("CWaveSoapFrontView::CWaveSoapFrontView()\n");
}

CWaveSoapFrontView::CWaveSoapFrontView()
	: m_VerticalScale(1.)
	, m_VerticalScaleIndex(0)
	, m_WaveOffsetY(0.)
	, m_MaxAmplitudeRange(1.)
{
}

CWaveSoapFrontView::~CWaveSoapFrontView()
{
}

BOOL CWaveSoapViewBase::PreCreateWindow(CREATESTRUCT& cs)
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
													int SelectionLeft, int SelectionRight, int Y,
													CPen * NormalPen, CPen * SelectedPen,
													CHANNEL_MASK Channel, LPCRECT ClipRect)
{
	if (ClipRect->left >= ClipRect->right)
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

	// draw selection if Channel==ALL or all selected, or
	// this channel is selected
	if (0 == (Channel & pDoc->m_SelectedChannel))
	{
		// don't draw selection
		SelectionLeft = ClipRect->right;
		SelectionRight = ClipRect->right;
	}

	if (pDoc->m_SelectionEnd != pDoc->m_SelectionStart
		&& SelectionRight == SelectionLeft)
	{
		SelectionRight++;
	}
	if (SelectionLeft < ClipRect->left)
	{
		SelectionLeft = ClipRect->left;
	}
	if (SelectionRight > ClipRect->right)
	{
		SelectionRight = ClipRect->right;
	}
	pDC->MoveTo(ClipRect->left, Y);
	if (SelectionLeft >= ClipRect->right
		|| SelectionRight < ClipRect->left
		|| SelectionRight == SelectionLeft)
	{
		// no selection visible
		pDC->SelectObject(NormalPen);
		pDC->LineTo(ClipRect->right, Y);
	}
	else
	{
		if (SelectionLeft > ClipRect->left)
		{
			pDC->SelectObject(NormalPen);
			pDC->LineTo(SelectionLeft, Y);
		}

		pDC->SelectObject(SelectedPen);
		pDC->LineTo(SelectionRight, Y);

		if (SelectionRight < ClipRect->right)
		{
			pDC->SelectObject(NormalPen);
			pDC->LineTo(ClipRect->right, Y);
		}
	}
}

// this is the rectangle for scaling purposes
void CWaveSoapViewBase::GetChannelRect(int Channel, RECT * pR) const
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

	pR->top = m_Heights.ch[Channel].top;
	pR->bottom = m_Heights.ch[Channel].bottom;
}

// this is the rectangle for clipping purposes
void CWaveSoapViewBase::GetChannelClipRect(int Channel, RECT * pR) const
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

	pR->top = m_Heights.ch[Channel].clip_top;
	pR->bottom = m_Heights.ch[Channel].clip_bottom;
}

int CWaveSoapFrontView::SampleValueToY(double value, int ch) const
{
	ASSERT(ch < GetDocument()->WaveChannels());

	return WaveCalculate(m_WaveOffsetY, m_VerticalScale, m_Heights.ch[ch].clip_top, m_Heights.ch[ch].clip_bottom)(value);
}

int CWaveSoapViewBase::GetChannelFromPoint(int y) const
{
	ThisDoc * pDoc = GetDocument();
	int nChannels = pDoc->WaveChannels();

	CRect r;
	GetClientRect(r);

	if (y < 0)
	{
		return -1;
	}

	for (int i = 0; i < nChannels; i++)
	{
		if (y <= m_Heights.ch[i].clip_bottom)
		{
			return i;
		}
	}
	return nChannels;
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

	memzero(m_InvalidAreaTop);
	memzero(m_InvalidAreaBottom);

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

		double FirstFractionalSample = WindowXtoSample(cr.left);

		if (FirstFractionalSample < 0.)
		{
			FirstFractionalSample = 0.;
			cr.left = SampleToX(0);
		}

		// number of sample that corresponds to the cr.left position
		SAMPLE_INDEX NumOfFirstSample = (SAMPLE_INDEX)floor(FirstFractionalSample);
		double SamplesPerPoint = m_HorizontalScale;

		// create an array of points

		NUMBER_OF_CHANNELS nChannels = pDoc->WaveChannels();
		int nNumberOfPoints = cr.right - cr.left;

		int SelBegin = SampleToX(pDoc->m_SelectionStart);
		int SelEnd = SampleToX(pDoc->m_SelectionEnd);

		if (pDoc->m_SelectionEnd != pDoc->m_SelectionStart
			&& SelEnd == SelBegin)
		{
			// make sure the selection is drawn as at least one pixel
			SelEnd++;
		}

		if (nNumberOfPoints > 0)
		{
			CWaveFile::InstanceDataWav * pInst = pDoc->m_WavFile.GetInstanceData();

			//this bitmap is used to draw a dashed vertical line
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

			ATL::CHeapPtr<POINT[2]> ppArray;
			ppArray.Allocate(nNumberOfPoints);

			for (int ch = 0; ch < nChannels; ch++)
			{
				if ( ! ppArray)
				{
					break;
				}
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

				if (ClipR.top >= cr.bottom)
				{
					continue;
				}

				bool const ChannelMinimized = m_Heights.ch[ch].minimized;

				WaveCalculate WaveToY(ChannelMinimized ? 0. : m_WaveOffsetY,
									ChannelMinimized ? 1. : m_VerticalScale,
									ChanR.top, ChanR.bottom);

				if (TRACE_DRAWING) TRACE("V Scale=%f, m_WaveOffsetY=%f, top = %d, bottom = %d, height=%d, W2Y(0)=%d, W2Y(32767)=%d, W2Y(-32768)=%d\n",
										m_VerticalScale, m_WaveOffsetY, ChanR.top, ChanR.bottom,
										ChanR.Height(), (int)WaveToY(0), (int)WaveToY(32767), (int)WaveToY(-32768));
				// Y = wave * m_VerticalScale + m_WaveOffsetY * m_VerticalScale
				//     + (ChanR.bottom + ChanR.top) / 2

				DrawHorizontalWithSelection(pDC, SelBegin, SelEnd,
											WaveToY(0),     // zero line
											& ZeroLinePen,
											& SelectedZeroLinePen, 1 << ch, ClipR);

				if ( ! ChannelMinimized)
				{
					DrawHorizontalWithSelection(pDC, SelBegin, SelEnd,
												WaveToY(16384),         // 6 dB line
												& SixDBLinePen,
												& SelectedSixDBLinePen, 1 << ch, ClipR);

					DrawHorizontalWithSelection(pDC, SelBegin, SelEnd,
												WaveToY(-16384),       // low 6dB line
												& SixDBLinePen,
												& SelectedSixDBLinePen, 1 << ch, ClipR);
				}
				int i;
				unsigned PeakDataGranularity = pDoc->m_WavFile.GetPeakGranularity();
				if (SamplesPerPoint >= PeakDataGranularity)
				{
					CSimpleCriticalSectionLock lock(pDoc->m_WavFile.GetPeakLock());
					// use peak data for drawing
					DWORD PeakSamplesPerPoint = DWORD(SamplesPerPoint) / PeakDataGranularity * nChannels;

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

						ppArray[i][0].x = i + cr.left;
						ppArray[i][0].y = WaveToY(peak.low);
						ppArray[i][1].x = i + cr.left;
						ppArray[i][1].y = WaveToY(peak.high);
					}
				}
				else
				{
					// use wave data for drawing
					float * pWaveSamples = NULL;
					// GetData argument must be aligned to full sample boundary
					int LastSampleRequired = (int)floor(FirstFractionalSample + nNumberOfPoints * SamplesPerPoint);
					int nCountSamples = m_WaveBuffer.GetData( & pWaveSamples,
															NumOfFirstSample * nChannels,
															(LastSampleRequired - NumOfFirstSample + 1) * nChannels, this);
					if (0 && nCountSamples < (LastSampleRequired - NumOfFirstSample + 1) * nChannels)
					{
						TRACE("OnDraw: nCountSamples=%d, LastSampleRequired=%d, NumOfFirstSample=%d, (LastSampleRequired - NumOfFirstSample) * nChannels=%d\n",
							nCountSamples, LastSampleRequired, NumOfFirstSample, (LastSampleRequired - NumOfFirstSample) * nChannels);
					}

					for (i = 0; i < nNumberOfPoints; i++)
					{
						WAVE_PEAK low = SHORT_MAX;
						WAVE_PEAK high = SHORT_MIN;
						int nSample = ch + nChannels * int(FirstFractionalSample - NumOfFirstSample + i * SamplesPerPoint);

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

						ppArray[i][0].x = i + cr.left;
						ppArray[i][0].y = WaveToY(low);
						ppArray[i][1].x = i + cr.left;
						ppArray[i][1].y = WaveToY(high);
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
					DrawHorizontalWithSelection(pDC, SelBegin, SelEnd,
												ClipR.bottom,
												& ChannelSeparatorPen,
												& SelectedChannelSeparatorPen, ALL_CHANNELS,
												CRect(ClipR.left, ClipR.top, ClipR.right, ClipR.bottom + 1));   // now include the separator line into it
				}

				// Raster OP: Destination AND brush pattern
				DWORD const DstAndBrushRop = 0x00A000C9;
				DWORD const DstOrBrushRop = 0x00AF0229;

				DashBrush.UnrealizeObject();

				// the brush origin is reset for each channel to allow for scroll
				pDC->SetBrushOrg(0, WaveToY(-32768) % DashLength);

				CGdiObjectSaveT<CBrush> OldBrush(pDC, pDC->SelectObject( & DashBrush));
				CGdiObjectSave OldFont(pDC, pDC->SelectStockObject(ANSI_VAR_FONT));

				pDC->SetTextColor(0xFFFFFF ^ pApp->m_WaveBackground);
				pDC->SetBkColor(pApp->m_WaveBackground);
				pDC->SetBkMode(OPAQUE);

				for (ConstCuePointVectorIterator ii = pInst->m_CuePoints.begin();
					ii != pInst->m_CuePoints.end(); ii++)
				{
					long x = SampleToX(ii->dwSampleOffset);
					WaveRegionMarker const * pMarker = pInst->GetRegionMarker(ii->CuePointID);

					// draw text
					if (0 == ch
						&& x < cr.right)
					{
						LPCTSTR txt = pInst->GetCueText(ii->CuePointID);
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

						// draw marker
						pDC->PatBlt(x, ClipR.top, 1, ClipR.bottom - ClipR.top, BrushRop);
					}

					if (pMarker != NULL
						&& pMarker->SampleLength != 0)
					{
						x = SampleToX(ii->dwSampleOffset + pMarker->SampleLength);

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
							pDC->PatBlt(x, ClipR.top, 1, ClipR.bottom - ClipR.top, BrushRop);
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

	if (m_PlaybackCursorDrawn)
	{
		DrawPlaybackCursor(pDC, m_PlaybackCursorDrawnSamplePos, m_PlaybackCursorChannel);
	}
}

int CDataSection<WAVE_SAMPLE, CWaveSoapViewBase>::ReadData(WAVE_SAMPLE * pBuf, LONGLONG nOffset,
															long nCount, CWaveSoapViewBase * pSource)
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

int CDataSection<float, CWaveSoapViewBase>::ReadData(float * pBuf, LONGLONG nOffset,
													long nCount, CWaveSoapViewBase * pSource)
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
		ASSERT(Read == nCount / pDoc->m_WavFile.Channels());

		if (0 == Read)
		{
			return 0;
		}
	}
	return ToZero + Read * pDoc->m_WavFile.Channels();
}

LONGLONG CDataSection<WAVE_SAMPLE, CWaveSoapViewBase>::GetSourceCount(CWaveSoapViewBase * pSource)
{
	CWaveSoapFrontDoc * pDoc = pSource->GetDocument();
	if (NULL == pDoc
		|| ! pDoc->m_WavFile.IsOpen())
	{
		return 0;
	}
	return pDoc->m_WavFile.NumberOfSamples() * pDoc->m_WavFile.Channels();
}

LONGLONG CDataSection<float, CWaveSoapViewBase>::GetSourceCount(CWaveSoapViewBase * pSource)
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

bool CWaveSoapViewBase::PlaybackCursorVisible() const
{
	int pos = SampleToX(m_PlaybackCursorDrawnSamplePos);

	CRect cr;
	GetClientRect(cr);

	return pos >= cr.left && pos < cr.right;
}

void CWaveSoapViewBase::CreateCursorBitmap(CBitmap & bmp, CHANNEL_MASK Channel)
{
	bmp.DeleteObject();
	CRect cr;
	GetClientRect(cr);

	ThisDoc * pDoc = GetDocument();
	if (NULL == pDoc
		|| ! pDoc->m_WavFile.IsOpen())
	{
		return;
	}
	WORD * data = new WORD[cr.Height()];
	memset(data, 0, cr.bottom * sizeof *data);

	for (int ch = 0; ch < pDoc->m_WavFile.Channels(); ch++)
	{
		if (0 == (Channel & (1 << ch)))
		{
			continue;
		}
		for (int y = m_Heights.ch[ch].clip_top; y < m_Heights.ch[ch].clip_bottom && y < cr.bottom; y++)
		{
			data[y] = 0xFFFF;
		}
	}
	// TODO: thicker cursor for high-res displays
	bmp.CreateBitmap(1, cr.bottom, 1, 1, data);
	delete[] data;
}

void CWaveSoapViewBase::DrawPlaybackCursor(CDC * pDC, SAMPLE_INDEX Sample, CHANNEL_MASK Channel)
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
#if 0
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
#else
		if ( ! HGDIOBJ(m_PlaybackCursorBitmap))
		{
			CreateCursorBitmap(m_PlaybackCursorBitmap, Channel);
		}

		CDC SrcDc;
		SrcDc.CreateCompatibleDC(pDrawDC);
		CGdiObjectSaveT<CBitmap> OldBitmap(SrcDc, SrcDc.SelectObject( & m_PlaybackCursorBitmap));

		pDrawDC->BitBlt(pos, r.top, 1, r.bottom - r.top, & SrcDc, 0, 0, SRCINVERT);
#endif
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

void CWaveSoapViewBase::ShowPlaybackCursor(CDC * pDC)
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

void CWaveSoapViewBase::HidePlaybackCursor(CDC * pDC)
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

CWaveSoapFrontDoc* CWaveSoapViewBase::GetDocument() const// non-debug version is inline
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
	BaseClass::OnInitialUpdate();

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
	data.HorizontalScroll.HorizontalScale = m_HorizontalScale;

	NotifySiblingViews(HorizontalExtentChanged, &data);
	NotifySiblingViews(HorizontalScaleChanged, &m_HorizontalScale);

	RecalculateChannelHeight(r.Height());

}

void CWaveSoapFrontView::OnUpdateViewZoominhor2(CCmdUI* pCmdUI)
{
	pCmdUI->Enable(m_HorizontalScale > 1./16.);
}

void CWaveSoapFrontView::OnViewZoomInVert()
{
	SetVerticalScaleIndex(m_VerticalScaleIndex + 1);
}

void CWaveSoapFrontView::SetVerticalScaleIndex(int NewVerticalScaleIndex)
{
	NotifySiblingViews(VerticalScaleIndexChanged, &NewVerticalScaleIndex);
}

void CWaveSoapFrontView::OnViewZoomOutVert()
{
	SetVerticalScaleIndex(m_VerticalScaleIndex - 1);
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
	if (m_HorizontalScale > 1./16.)
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
		if (m_Heights.ch[ChannelUnderCursor].minimized)
		{
			result |= VSHT_CHANNEL_MINIMIZED;
		}

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
		if (pDoc->WaveChannels() == 2)
		{
			if (ChannelUnderCursor == 0 && p.y < SampleValueToY(0., ChannelUnderCursor))
			{
				result |= VSHT_LEFT_CHAN;
			}
			else if (ChannelUnderCursor == 1 && p.y > SampleValueToY(0., ChannelUnderCursor))
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

BOOL CWaveSoapViewBase::OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message)
{
	if (pWnd == this
		&& HTCLIENT == nHitTest)
	{
		CWinApp *app = AfxGetApp();
		CPoint p;

		GetCursorPos( & p);
		ScreenToClient( & p);

		DWORD ht = ClientHitTest(p);

		if ((ht & (VSHT_SEL_BOUNDARY_L | VSHT_SEL_BOUNDARY_R))
			|| WM_LBUTTONDOWN == nKeyPressed)
		{
			SetCursor(app->LoadStandardCursor(IDC_SIZEWE));
			return TRUE;
		}

		if (ht & (VSHT_SELECTION | VSHT_NOWAVE))
		{
			SetCursor(app->LoadStandardCursor(IDC_ARROW));
			return TRUE;
		}

		if (ht & VSHT_LEFT_CHAN)
		{
			SetCursor(app->LoadCursor(IDC_CURSOR_BEAM_LEFT));
			return TRUE;
		}

		if (ht & VSHT_RIGHT_CHAN)
		{
			SetCursor(app->LoadCursor(IDC_CURSOR_BEAM_RIGHT));
			return TRUE;
		}

		if (ht & VSHT_BCKGND)
		{
			SetCursor(app->LoadCursor(IDC_CURSOR_BEAM));
			return TRUE;
		}
	}

	return BaseClass::OnSetCursor(pWnd, nHitTest, message);
}

void CWaveSoapViewBase::OnKillFocus(CWnd* pNewWnd)
{
	m_HasFocus = false;
	if (m_CaretCreated)
	{
		DestroyCaret();
	}
	m_CaretCreated = false;
	m_CaretShown = false;
	BaseClass::OnKillFocus(pNewWnd);

}

void CWaveSoapViewBase::OnSetFocus(CWnd* pOldWnd)
{
	m_HasFocus = true;
	BaseClass::OnSetFocus(pOldWnd);
	CreateAndShowCaret(true);
}

void CWaveSoapViewBase::CreateAndShowCaret(bool ForceCreateCaret)
{
	if (ForceCreateCaret)
	{
		if (m_CaretShown)
		{
			// before the bitmap can be deleted, need to hide the caret, otherwise it cannot be erased from the window
			HideCaret();
			m_CaretShown = false;
		}
		m_CursorBitmap.DeleteObject();
	}
	// create caret
	if ( ! m_HasFocus)
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
		if (m_CaretShown)
		{
			HideCaret();
			m_CaretShown = false;
		}
		return;
	}

	CRect r;
	GetClientRect(r);

	CPoint p(SampleToX(pDoc->m_CaretPosition), r.top);

	if (p.x >= 0 && p.x < r.right)
	{
		if ( ! m_CaretCreated || ! HGDIOBJ(m_CursorBitmap))
		{
			if (m_CaretShown)
			{
				// before the bitmap can be deleted, need to hide the caret, otherwise it cannot be erased from the window
				HideCaret();
				m_CaretShown = false;
			}

			CreateCursorBitmap(m_CursorBitmap, pDoc->m_SelectedChannel);
			CreateCaret(& m_CursorBitmap);
			m_CaretCreated = true;
			m_CaretShown = false;
		}
		SetCaretPos(p);
		if ( ! m_CaretShown)
		{
			ShowCaret();
			m_CaretShown = true;
		}
	}
	else if (m_CaretShown)
	{
		HideCaret();
		m_CaretShown = false;
	}
}

BOOL CWaveSoapFrontView::OnEraseBkgnd(CDC* pDC)
{
	CThisApp * pApp = GetApp();
	ThisDoc * pDoc = GetDocument();
// FIXME    RemoveSelectionRect();

	CRect ClipRect;
	pDC->GetClipBox(ClipRect);
	if (0) TRACE("EraseBkgnd: ClipBox.left = %d, right=%d\n", ClipRect.left, ClipRect.right);

	CRect cr;
	GetClientRect(cr);

	int FileEnd = SampleToXceil(pDoc->WaveFileSamples());

	try
	{
		CBrush backBrush(pApp->m_WaveBackground);
		CBrush SelectedBackBrush(pApp->m_SelectedWaveBackground);

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
					0x5555,
				};

				// alternate the pattern to adjust for add/even scrolling, to avoid discontinuity
				bmp.CreateBitmap(8, 8, 1, 1, pattern + FileEnd % 2);

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

void CWaveSoapViewBase::OnLButtonDown(UINT nFlags, CPoint point)
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

void CWaveSoapViewBase::OnLButtonUp(UINT nFlags, CPoint point)
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

	}
	BaseClass::OnLButtonUp(nFlags, point);
}

BOOL CWaveSoapViewBase::OnMouseWheel(UINT nFlags, short zDelta, CPoint /*pt*/)
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
				GetParent()->SendMessage(WM_HSCROLL, SB_LINEUP, 0);
			}
		}
		else
		{
			while (m_WheelAccumulator <= - WHEEL_DELTA)
			{
				m_WheelAccumulator += WHEEL_DELTA;
				GetParent()->SendMessage(WM_HSCROLL, SB_LINEDOWN, 0);
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
				AfxGetMainWnd()->SendMessage(WM_COMMAND, ID_VIEW_ZOOMINHOR2);
			}
		}
		else
		{
			while (m_WheelAccumulator <= - WHEEL_DELTA)
			{
				m_WheelAccumulator += WHEEL_DELTA;
				AfxGetMainWnd()->SendMessage(WM_COMMAND, ID_VIEW_ZOOMOUTHOR2);
			}
		}
	}
	return TRUE;
}

void CWaveSoapViewBase::OnMouseMove(UINT nFlags, CPoint point)
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
		CreateAndShowCaret(pInfo->OldSelChannel != pInfo->SelChannel);
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
	else if (lHint == ThisDoc::UpdateSampleRateChanged
			&& NULL == pHint)
	{
		// don't do anything
	}
	else
	{
		if (lHint == ThisDoc::UpdateWholeFileChanged)
		{
			// recalculate limits in case PCM type changed to/from float
			NotifySiblingViews(VerticalScaleIndexChanged, &m_VerticalScaleIndex);
		}
		BaseClass::OnUpdate(pSender, lHint, pHint);
	}
}

void CWaveSoapViewBase::OnUpdate(CView* /*pSender*/, LPARAM lHint, CObject* pHint)
{
	if (lHint == ThisDoc::UpdatePlaybackPositionChanged
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
		CRect cr;
		GetClientRect(cr);

		// recalculate the extents
		UpdateHorizontalExtents(GetDocument()->WaveFileSamples(), cr.Width());
		RecalculateChannelHeight(cr.Height());
		Invalidate();

		CreateAndShowCaret(true);   // create new bitmap
	}
	else if (lHint == ThisDoc::UpdateMarkerRegionChanged
			&& NULL != pHint)
	{
		MarkerRegionUpdateInfo * pInfo = static_cast<MarkerRegionUpdateInfo *> (pHint);
		ASSERT(NULL != pInfo);

		InvalidateMarkerRegion( & pInfo->info);
	}
}

void CWaveSoapViewBase::InvalidateMarkerRegion(WAVEREGIONINFO const * pInfo)
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

POINT CWaveSoapViewBase::GetZoomCenter()
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

void CWaveSoapViewBase::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
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
	long nPage = long((r.Width()  / 2) * m_HorizontalScale);
	if (CtrlPressed)
	{
		// make it 7/8 of the window width
		nPage = long((r.Width() * 7 / 8) * m_HorizontalScale);
	}

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

void CWaveSoapViewBase::MovePointIntoView(SAMPLE_INDEX nCaret, BOOL bCenter)
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
}

void CWaveSoapViewBase::UpdateCaretPosition()
{
	CreateAndShowCaret();
}

void CWaveSoapViewBase::OnActivateView(BOOL bActivate, CView* pActivateView, CView* pDeactiveView)
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

// scroll_offset > 0 - image moves to the right, first pixel in view decremented
// scroll_offset < 0 - image moves to the left, first pixel in view incremented
void CWaveSoapViewBase::HorizontalScrollByPixels(int pixels)
{
	HorizontalScrollBy(-pixels * m_HorizontalScale);
}

void CWaveSoapViewBase::HorizontalScrollBy(double samples)
{
	SetFirstSampleInView(m_FirstSampleInView + samples);
}

void CWaveSoapViewBase::SetFirstSampleInView(double sample)
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
	// only this window handles HScroll for all windows in the child frame. Any updates are broadcast to siblings
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
		double TotalSamplesInExtent = TotalSamples + ReservedPixels * m_HorizontalScale;
		double NewFirstSampleInView = TotalSamplesInExtent * double(scrollinfo.nTrackPos - scrollinfo.nMin) / (scrollinfo.nMax - scrollinfo.nMin);
		if (0) TRACE("SB_THUMBTRACK pos=%d, trackpos=%d, new first sample=%f\n", nPos, scrollinfo.nTrackPos, NewFirstSampleInView);

		SetFirstSampleInView(NewFirstSampleInView);
		break;
	}
	default:
		break;
	}
	// not calling base class handler
}

void CWaveSoapViewBase::RecalculateChannelHeight(int cy)
{
	ThisDoc * pDoc = GetDocument();
	// recalculate channel extents
	NUMBER_OF_CHANNELS nChannels = pDoc->WaveChannels();
	int i;
	int cyhscroll = GetSystemMetrics(SM_CYHSCROLL);
	int MinimizedChannelHeight = 2 * cyhscroll;
	int NumberOfMinimizedChannels = 0;
	int ChannelHeight;

	m_Heights.NumChannels = nChannels;

	for (i = 0; i < nChannels; i++)
	{
		if (m_Heights.ch[i].minimized)
		{
			NumberOfMinimizedChannels++;
		}
	}
	// all channels cannot be minimized
	ASSERT(NumberOfMinimizedChannels < nChannels);
	// Normal height of minimized channels is 2*SM_CYHSCROLL.
	// minimum height of non-minimized channels is SM_CYHSCROLL.
	// Normal height of non-minimized channels is >2*SM_CYHSCROLL.
	// minimum height of non-minimized channels is 2*SM_CYHSCROLL.
	// When the window height cannot accomodate all of them, they are pushed out
	// channels can be different height to accomodate for fractions?
	if (cy + 1 >= nChannels * 2 * cyhscroll)
	{
		ChannelHeight = (cy + 1 - NumberOfMinimizedChannels * MinimizedChannelHeight) / (nChannels - NumberOfMinimizedChannels);
	}
	else if (cy + 1 >= NumberOfMinimizedChannels * cyhscroll + (nChannels - NumberOfMinimizedChannels) * cyhscroll * 2)
	{
		ChannelHeight = cyhscroll * 2;
		MinimizedChannelHeight = (cy + 1 - (nChannels - NumberOfMinimizedChannels) * ChannelHeight) / NumberOfMinimizedChannels;
	}
	else
	{
		ChannelHeight = cyhscroll * 2;
		MinimizedChannelHeight = cyhscroll;
	}

	int ExtraPixels = cy + 1 - MinimizedChannelHeight * NumberOfMinimizedChannels - (nChannels - NumberOfMinimizedChannels) * ChannelHeight;
	if (ExtraPixels < 0)
	{
		ExtraPixels = 0;
	}

	int y = 0;
	m_Heights.NumChannels = nChannels;
	m_Heights.NominalChannelHeight = ChannelHeight - 1; // one pixel for the separator line
	for (i = 0; i < nChannels; i++)
	{
		m_Heights.ch[i].top = y;
		m_Heights.ch[i].clip_top = y;
		int OddPixel = (i + 1) * ExtraPixels / nChannels - i * ExtraPixels / nChannels;

		if (m_Heights.ch[i].minimized)
		{
			m_Heights.ch[i].bottom = y + MinimizedChannelHeight - 1;
			y += MinimizedChannelHeight + OddPixel;
		}
		else
		{
			m_Heights.ch[i].bottom = y + ChannelHeight - 1;
			y += ChannelHeight + OddPixel;
		}
		m_Heights.ch[i].clip_bottom = y - 1;    // not including the separator line
	}
	NotifySiblingViews(ChannelHeightsChanged, & m_Heights);
}

void CWaveSoapViewBase::OnSize(UINT nType, int cx, int cy)
{
	UpdateHorizontalExtents(WaveFileSamples(), cx);
	RecalculateChannelHeight(cy);

	m_PlaybackCursorBitmap.DeleteObject();

	BaseClass::OnSize(nType, cx, cy);
	CreateAndShowCaret(true);   // force new caret
}

void CWaveSoapFrontView::OnViewZoomvertNormal()
{
	SetVerticalScaleIndex(0);
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

void CWaveSoapViewBase::UpdatePlaybackCursor(SAMPLE_INDEX sample, CHANNEL_MASK channel)
{
	if (0 == m_PlaybackCursorChannel)
	{
		// first call after playback start, and the last call after playback end
		m_PlaybackCursorBitmap.DeleteObject();      // make sure to create a new one
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

	UpdateCaretPosition();
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

void CWaveSoapViewBase::UpdateHorizontalExtents(NUMBER_OF_SAMPLES Length, int WindowWidth)
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
	data.HorizontalScroll.HorizontalScale = m_HorizontalScale;

	NotifySiblingViews(HorizontalExtentChanged, &data);
}

void CWaveSoapViewBase::OnRButtonDown(UINT nFlags, CPoint point)
{
	// point is in client coordinates
	nKeyPressed = WM_RBUTTONDOWN;

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

void CWaveSoapViewBase::OnRButtonUp(UINT nFlags, CPoint point)
{
	nKeyPressed = 0;
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

void CWaveSoapViewBase::OnTimer(UINT_PTR nIDEvent)
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

void CWaveSoapViewBase::OnCaptureChanged(CWnd *pWnd)
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

void CWaveSoapViewBase::OnLButtonDblClk(UINT nFlags, CPoint point)
{
	BaseClass::OnLButtonDblClk(nFlags, point);

	GetDocument()->SelectBetweenMarkers(SAMPLE_INDEX(WindowXtoSample(point.x)));
}

void CWaveSoapViewBase::AdjustCaretVisibility(SAMPLE_INDEX CaretPos, SAMPLE_INDEX OldCaretPos,
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
void CWaveSoapViewBase::SetHorizontalScale(double NewHorizontalScale, int ZoomCenter)
{
	m_PrevHorizontalScale = m_HorizontalScale;
	NotifySiblingViews(HorizontalScaleChanged, &NewHorizontalScale);
	MovePointIntoView(GetDocument()->m_CaretPosition, TRUE);
}

int CWaveSoapViewBase::SampleToXceil(SAMPLE_INDEX sample) const
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

int CWaveSoapViewBase::SampleToX(SAMPLE_INDEX sample) const
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

double CWaveSoapViewBase::WindowXtoSample(int x) const
{
	return x * m_HorizontalScale + m_FirstSampleInView;
}

afx_msg LRESULT CWaveSoapFrontView::OnUwmNotifyViews(WPARAM wParam, LPARAM lParam)
{
	CRect cr;
	GetClientRect(cr);

	switch (wParam)
	{
	case ChannelHeightsChanged:
		Invalidate();
		CreateAndShowCaret(true);
		break;

	case HorizontalScrollPixels:
		HorizontalScrollByPixels(*(int*)lParam);
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
			data.HorizontalScroll.HorizontalScale = m_HorizontalScale;

			NotifySiblingViews(HorizontalExtentChanged, &data);
		}
		break;
	case VerticalScaleAndOffsetChanged:
	{
		NotifyViewsData * data = (NotifyViewsData *)lParam;
		if (m_VerticalScale != data->Amplitude.NewScale
			|| data->Amplitude.NewOffset != m_WaveOffsetY
			|| data->Amplitude.MaxRange != m_MaxAmplitudeRange)
		{
			m_WaveOffsetY = data->Amplitude.NewOffset;
			m_VerticalScale = data->Amplitude.NewScale;
			m_MaxAmplitudeRange = data->Amplitude.MaxRange;
			Invalidate();
		}
	}
		break;

	case VerticalScaleIndexChanged:
		// lParam points to double new scale
	{
		NotifyViewsData data = { 0 };
		static const double VerticalScaleIndexTable[] =
		{
			0.316227766,	// step is sqrt(sqrt(sqrt(10))), for float data only
			0.421696503,
			0.562341325,
			0.749894209,
			1.,
			1.41421356,
			2,
			2.82842712,
			4,
			5.65685424,
			8,
			11.3137084,
			16,
			22.6274169,
			32,
			45.2548339,
			64,
			90.5096679,
			128,
			181.019335,
			256,
			362.038671,
			512,
			724.077343,
			1024,
		};

		m_VerticalScaleIndex = *(int*)lParam;
		double MaxRange = 1.;
		if (GetDocument()->m_WavFile.IsOpen()
			&& GetDocument()->m_WavFile.GetWaveFormat().GetSampleType() == SampleTypeFloat32)
		{
			MaxRange = 3.16227766016;

			if (m_VerticalScaleIndex < -4)
			{
				m_VerticalScaleIndex = -4;
			}
		}
		else if (m_VerticalScaleIndex < 0)
		{
			m_VerticalScaleIndex = 0;
		}
		if (m_VerticalScaleIndex+4 >= int(countof (VerticalScaleIndexTable)))
		{
			m_VerticalScaleIndex = countof (VerticalScaleIndexTable)-5;
		}

		data.Amplitude.NewScale = VerticalScaleIndexTable[m_VerticalScaleIndex +4];
		data.Amplitude.MaxRange = MaxRange;
		// check for the proper offset, correct if necessary
		// correct the offset, if necessary
		// find max and min offset for this scale
		data.Amplitude.NewOffset = WaveCalculate(m_WaveOffsetY, data.Amplitude.NewScale, 0, m_Heights.NominalChannelHeight)
									.AdjustOffset(m_WaveOffsetY, -MaxRange, MaxRange);

		NotifySiblingViews(VerticalScaleAndOffsetChanged, &data);
	}
		break;
	case AmplitudeScrollTo:
		// lParam points to double offset
		// check for the proper offset, correct if necessary
	{
		double offset = *(double*) lParam;

		offset = WaveCalculate(m_WaveOffsetY, m_VerticalScale, 0, m_Heights.NominalChannelHeight)
				.AdjustOffset(offset, -m_MaxAmplitudeRange, m_MaxAmplitudeRange);

		NotifySiblingViews(AmplitudeOffsetChanged, & offset);
	}
		break;
	case AmplitudeOffsetChanged:
		// lParam points to double new vertical offset
		SetNewAmplitudeOffset(*(double*) lParam);
		break;

	case ShowChannelPopupMenu:
	{
		NotifyViewsData * data = (NotifyViewsData *) lParam;

		CPoint client(data->PopupMenu.p);
		ScreenToClient(&client);
		client.x = 0;

		DWORD HitTest = ClientHitTest(client);

		UINT uID = data->PopupMenu.NormalMenuId;

		if ((HitTest & VSHT_CHANNEL_MINIMIZED)
			&& data->PopupMenu.MinimizedMenuId != 0)
		{
			uID = data->PopupMenu.MinimizedMenuId;
		}

		CMenu menu;
		if (uID == 0 || !menu.LoadMenu(uID))
		{
			break;
		}

		CMenu* pPopup = menu.GetSubMenu(0);
		// modify "Minimize" menu item (or remove altogether)
		if (pPopup == NULL)
		{
			break;
		}

		if (HitTest & VSHT_CHANNEL_MINIMIZED)
		{
			MENUITEMINFO info = { sizeof info, MIIM_ID };
			info.wID = ID_VIEW_MAXIMIZE_0 + (HitTest & VSHT_CHANNEL_MASK);

			pPopup->SetMenuItemInfoW(ID_VIEW_MAXIMIZE_0, &info, FALSE);
		}
		else if (GetDocument()->WaveChannels() >= 2)
		{
			MENUITEMINFO info = { sizeof info, MIIM_ID };
			info.wID = ID_VIEW_MINIMIZE_0 + (HitTest & VSHT_CHANNEL_MASK);

			pPopup->SetMenuItemInfoW(ID_VIEW_MINIMIZE_0, &info, FALSE);
		}
		else
		{
			// delete the "minimize" and the last separator before it
			pPopup->RemoveMenu(ID_VIEW_MINIMIZE_0, MF_BYCOMMAND);
			int count = pPopup->GetMenuItemCount();
			MENUITEMINFO info = { sizeof info, MIIM_TYPE };
			if (count > 2)
			{
				pPopup->GetMenuItemInfoW(count - 1, &info, TRUE);
				if (info.fType & MFT_SEPARATOR)
				{
					pPopup->RemoveMenu(count - 1, MF_BYPOSITION);
				}
			}
		}

		int Command = pPopup->TrackPopupMenu(
											TPM_LEFTALIGN | TPM_RIGHTBUTTON | TPM_RETURNCMD,
											data->PopupMenu.p.x, data->PopupMenu.p.y,
											AfxGetMainWnd()); // use main window for cmds

		if (0 != Command)
		{
			AfxGetMainWnd()->SendMessage(WM_COMMAND, Command & 0xFFFF, 0);
		}
		break;
	}
	default:
		return BaseClass::OnUwmNotifyViews(wParam, lParam);
	}
	return 0;
}

void CWaveSoapViewBase::InvalidateMarkerLabels(int dy)
{
	bool PlaybackCursorHidden = false;

	CWaveSoapFrontDoc * pDoc = GetDocument();

	CRect cr;
	GetClientRect(cr);

	// invalidate marker labels
	CWindowDC dc(this);
	CGdiObjectSave OldFont(dc, dc.SelectStockObject(ANSI_VAR_FONT));

	CWaveFile::InstanceDataWav * pInst = pDoc->m_WavFile.GetInstanceData();

	for (ConstCuePointVectorIterator i = pInst->m_CuePoints.begin();
		i < pInst->m_CuePoints.end(); i++)
	{
		long x = SampleToX(i->dwSampleOffset);

		// invalidate text
		if (x >= cr.right)
		{
			continue;
		}

		LPCTSTR txt = pInst->GetCueText(i->CuePointID);
		if (NULL == txt)
		{
			continue;
		}

		CRect ir;
		CPoint TextSize(dc.GetTextExtent(txt, (int)_tcslen(txt)));

		ir.left = x;
		ir.top = cr.top;
		ir.bottom = cr.top + TextSize.y;
		ir.right = x + TextSize.x;

		// in the wave area, invalidate old and new position,
		// only if vertical scroll was done.
		// In the background area, invalidate only if horizontal scroll was done
		if (x + TextSize.x > cr.left
			&& dy != 0)
		{
			InvalidateRect(ir);
			if (dy > 0
				&& dy < cr.bottom - cr.top)
			{
				// scroll down
				// invalidate old position
				InvalidateRect(CRect(x, cr.top + dy, x + TextSize.x, cr.top + TextSize.y + dy));
			}
		}
	}

	if (PlaybackCursorHidden)
	{
		ShowPlaybackCursor();
	}
}

void CWaveSoapFrontView::SetNewAmplitudeOffset(double offset)
{
	// scroll channels rectangles
	CWaveSoapFrontDoc * pDoc = GetDocument();
	int nChannels = pDoc->WaveChannels();
	CRect cr;
	GetClientRect(cr);
	int const FileEnd = SampleToXceil(pDoc->WaveFileSamples());
	if (cr.right > FileEnd)
	{
		cr.right = FileEnd;
	}

	// offset of the zero line down
	for (int ch = 0; ch < nChannels; ch++)
	{
		if (m_Heights.ch[ch].minimized)
		{
			continue;
		}

		long OldOffsetPixels = WaveCalculate(m_WaveOffsetY, m_VerticalScale,  m_Heights.ch[ch].top, m_Heights.ch[ch].bottom)(0.);
		long NewOffsetPixels = WaveCalculate(offset, m_VerticalScale,  m_Heights.ch[ch].top, m_Heights.ch[ch].bottom)(0.);

		int ToScroll = NewOffsetPixels - OldOffsetPixels;       // >0 - down, <0 - up

		CRect ClipRect(cr.left, m_Heights.ch[ch].clip_top, cr.right, m_Heights.ch[ch].clip_bottom);
		CRect ScrollRect(ClipRect);
		ScrollRect.top += m_InvalidAreaTop[ch];
		ScrollRect.bottom -= m_InvalidAreaBottom[ch];

		if (ch == 0)
		{
			InvalidateMarkerLabels(ToScroll);
		}

		if (ToScroll > 0)
		{
			// down
			m_InvalidAreaTop[ch] += ToScroll;
			ScrollRect.bottom -= ToScroll;
			if (ScrollRect.Height() <= 0)
			{
				InvalidateRect(ClipRect);
				continue;
			}
			CRect ToInvalidate(cr.left, ScrollRect.top, cr.right, ScrollRect.top + ToScroll);

			ScrollWindow(0, ToScroll, ScrollRect, ClipRect);
			InvalidateRect(ToInvalidate);
		}
		else if (ToScroll < 0)
		{
			// up
			m_InvalidAreaBottom[ch] -= ToScroll;
			ScrollRect.top -= ToScroll;
			if (ScrollRect.Height() <= 0)
			{
				InvalidateRect(ClipRect);
				continue;
			}
			CRect ToInvalidate(cr.left, ScrollRect.top, cr.right, ScrollRect.top + ToScroll);

			ScrollWindow(0, ToScroll, ScrollRect, ClipRect);
			InvalidateRect(ToInvalidate);
		}
		else
		{
			continue;
		}
	}

	m_WaveOffsetY = offset;
}

void CWaveSoapFrontView::OnContextMenu(CWnd* /*pWnd*/, CPoint point)
{
	// make sure window is active
	GetParentFrame()->ActivateFrame();

	NotifyViewsData notify = { 0 };
	notify.PopupMenu.p = point;
	ScreenToClient(&point);
	DWORD hit = ClientHitTest(point);
	if (hit & VSHT_SELECTION)
	{
		notify.PopupMenu.NormalMenuId = IDR_MENU_WAVE_VIEW_SELECTION;
		notify.PopupMenu.MinimizedMenuId = IDR_MENU_WAVE_VIEW_SELECTION_MINIMIZED;
	}
	else
	{
		notify.PopupMenu.NormalMenuId = IDR_MENU_WAVE_VIEW;
		notify.PopupMenu.MinimizedMenuId = IDR_MENU_WAVE_VIEW_MINIMIZED;
	}

	NotifySiblingViews(ShowChannelPopupMenu, &notify);

}

afx_msg LRESULT CWaveSoapViewBase::OnUwmNotifyViews(WPARAM wParam, LPARAM lParam)
{
	CRect cr;
	GetClientRect(cr);

	switch (wParam)
	{
	case ChannelHeightsChanged:

		break;

	case HorizontalExtentChanged:
	{
		NotifyViewsData *data = (NotifyViewsData *) lParam;
		m_FirstSampleInView = data->HorizontalScroll.FirstSampleInView;
		UpdateCaretPosition();
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
				Invalidate();
			}
			UpdateCaretPosition();
		}
		else if (offset_pixels < 0.)    // shift image to the right
		{
			if (-offset_pixels < cr.Width())
			{
				ScrollWindow(-int(offset_pixels), 0, cr, cr);
			}
			else
			{
				Invalidate();
			}
			UpdateCaretPosition();
		}
		else
		{
			// do nothing
		}
		break;

	}
	return 0;
}


void CWaveSoapFrontView::OnViewMinimize0(UINT id)
{
	int Channel = id - ID_VIEW_MINIMIZE_0;
	int nChannels = GetDocument()->WaveChannels();

	if (Channel >= nChannels)
	{
		return;
	}

	// check if there is any non-minimized channel
	int i;
	for (i = 0; i < nChannels; i++)
	{
		if (i != Channel && ! m_Heights.ch[i].minimized)
		{
			break;
		}
	}

	m_Heights.ch[Channel].minimized = true;
	if (i >= nChannels)
	{
		m_Heights.ch[(Channel + 1) % nChannels].minimized = false;
	}

	CRect cr;
	GetClientRect(cr);
	RecalculateChannelHeight(cr.Height());
}

void CWaveSoapFrontView::OnViewMaximize0(UINT id)
{
	int Channel = id - ID_VIEW_MAXIMIZE_0;
	int nChannels = GetDocument()->WaveChannels();

	if (Channel >= nChannels)
	{
		return;
	}

	m_Heights.ch[Channel].minimized = false;

	CRect cr;
	GetClientRect(cr);
	RecalculateChannelHeight(cr.Height());
}

WaveCalculate::WaveCalculate(double offset, double scale, int top, int bottom)
{
	// adjusted for display scale, the full range shall always take whole number of pixels
	m_ViewHeight = bottom - top;
	m_Height = int(m_ViewHeight * scale);

	if (m_Height % 2)
	{
		// the zero wave line is always in the middle of pixel. The full sweep of 65536 will fit to Height, add 1 for rounding errors
		m_Scale = 32768. * m_Height / 65537.;
	}
	else
	{
		m_Scale = (m_Height - 1) / 2.;
	}

	m_Offset = long((bottom + top) / 2 + offset * m_Scale);
}

double WaveCalculate::AdjustOffset(double offset, double MinRange, double MaxRange)
{
	// The sample range is from -1. to + 1. float
	// limit offset to make the full range fit to the view
	if (offset < 0.)
	{
		double MinOffset = MinRange + double(m_ViewHeight) / m_Height;
		if (offset < MinOffset)
		{
			offset = MinOffset;
		}
	}
	else
	{
		double MaxOffset = MaxRange - double(m_ViewHeight) / m_Height;
		if (offset > MaxOffset)
		{
			offset = MaxOffset;
		}
	}
	return offset;
}

