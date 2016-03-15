// Copyright Alexander Grigoriev, 1997-2002, All Rights Reserved
// WaveOutlineView.cpp : implementation file
//

#include "stdafx.h"
#include "WaveSoapFront.h"
#include "WaveOutlineView.h"
#include "GdiObjectSave.h"
#include "WaveSoapFrontDoc.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CWaveOutlineView

IMPLEMENT_DYNCREATE(CWaveOutlineView, CView)

CWaveOutlineView::CWaveOutlineView()
	: m_PlaybackCursorPosition(-1),
	m_LeftViewBoundary(-1),
	m_LastMaxAmplitude(0),
	bIsTrackingSelection(false),
	nKeyPressed(0),
	m_RightViewBoundary(-1)
{
}

CWaveOutlineView::~CWaveOutlineView()
{
}


BEGIN_MESSAGE_MAP(CWaveOutlineView, CView)
	//{{AFX_MSG_MAP(CWaveOutlineView)
	ON_WM_MOUSEACTIVATE()
	ON_WM_ERASEBKGND()
	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONUP()
	ON_WM_MOUSEMOVE()
	ON_WM_CAPTURECHANGED()
	ON_WM_SETCURSOR()
	ON_WM_LBUTTONDBLCLK()
	ON_MESSAGE(UWM_NOTIFY_VIEWS, &CWaveOutlineView::OnUwmNotifyViews)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CWaveOutlineView drawing

void CWaveOutlineView::OnDraw(CDC* pDC)
{
	CWaveSoapFrontDoc* pDoc = GetDocument();
	// the whole file is here
	CRect cr;
	GetClientRect( & cr);
	CRect ur(cr);
	if (pDC->IsKindOf(RUNTIME_CLASS(CPaintDC)))
	{
		CPaintDC* pPaintDC = (CPaintDC*)pDC;
		if (pPaintDC->m_ps.fErase)
		{
			EraseBkgnd(pPaintDC);
		}
		ur = pPaintDC->m_ps.rcPaint;
	}

	if (ur.right > cr.right)
	{
		ur.right = cr.right;
	}
	if (ur.left < 0)
	{
		ur.left = 0;
	}

	unsigned width = cr.Width();
	unsigned update_width = ur.Width();

	if (update_width == 0)
	{
		return;
	}

	if (! pDoc->m_WavFile.IsOpen())
	{
		return;
	}
	NUMBER_OF_SAMPLES nSamples = pDoc->WaveFileSamples();

	ATL::CHeapPtr<WavePeak> pPeaks;
	if (!pPeaks.Allocate(width))
	{
		return;
	}

	NUMBER_OF_CHANNELS channels = pDoc->WaveChannels();

	CThisApp * pApp = GetApp();

	CPushDcPalette OldPalette(pDC, NULL);

	if (pDC->GetDeviceCaps(RASTERCAPS) & RC_PALETTE)
	{
		OldPalette.PushPalette(pApp->GetPalette(), FALSE);
	}

	CPen BlackPen;
	BlackPen.CreatePen(PS_SOLID, 0, DWORD(0x000000u));

	CGdiObjectSaveT<CPen> OldPen(pDC, pDC->SelectObject( & BlackPen));
	// draw lower border line
	pDC->MoveTo(cr.left, cr.bottom - 1);
	pDC->LineTo(cr.right, cr.bottom - 1);

	unsigned TotalPeaks = pDoc->m_WavFile.GetPeaksSize();
	int i;
	if (0 == nSamples)
	{
		for (i = ur.left; i < ur.right; i++)
		{
			pPeaks[i].low = 0;
			pPeaks[i].high = 0;
		}
	}
	else if (TotalPeaks / channels >= width)
	{
		CSimpleCriticalSectionLock lock(pDoc->m_WavFile.GetPeakLock());

		int PrevIdx = ur.left * TotalPeaks / width;
		for (i = ur.left; i < ur.right; i++)
		{
			int NewIdx = (i + 1) * TotalPeaks / width;

			pPeaks[i] = pDoc->m_WavFile.GetPeakMinMax(PrevIdx, NewIdx);

			PrevIdx = NewIdx;
		}
	}
	else
	{
		// use data from the file
		int SampleSize = pDoc->WaveSampleSize();
		NUMBER_OF_SAMPLES BufSamples = 4096 / SampleSize;
		if (BufSamples > nSamples)
		{
			BufSamples = nSamples;
		}

		ATL::CHeapPtr<float> pBuf;

		if (! pBuf.Allocate(BufSamples * channels))
		{
			return;
		}

		SAMPLE_INDEX LeftSampleInBuffer = 0, RightSampleInBuffer = 0;
		float * pCurSample = pBuf;
		int CurrSample = 0;
		for (i = ur.left; i < ur.right; )
		{
			int PrevSample = MulDiv(i, nSamples, width);
			int NextSample = MulDiv(i + 1, nSamples, width);

			// read some more samples to the buffer
			if (CurrSample >= RightSampleInBuffer)
			{
				LeftSampleInBuffer = CurrSample;
				long ToRead = nSamples - LeftSampleInBuffer;
				if (ToRead > BufSamples)
				{
					ToRead = BufSamples;
				}
				if (ToRead != pDoc->m_WavFile.ReadSamples(ALL_CHANNELS,
						pDoc->m_WavFile.SampleToPosition(LeftSampleInBuffer), ToRead, pBuf, SampleTypeFloat32))
				{
					return;
				}
				RightSampleInBuffer = LeftSampleInBuffer + ToRead;
				pCurSample = pBuf;
			}

			if (CurrSample <= PrevSample)
			{
				CurrSample = PrevSample;

				pPeaks[i].high = pCurSample[0];
				pPeaks[i].low = pCurSample[0];
				for (int ch = 1; ch < channels; ch++)
				{
					if (pPeaks[i].high < pCurSample[ch])
					{
						pPeaks[i].high = pCurSample[ch];
					}
					if (pPeaks[i].low > pCurSample[ch])
					{
						pPeaks[i].low = pCurSample[ch];
					}
				}
				if (CurrSample == NextSample)
				{
					i++;
					// next column will use the same sample value
					continue;
				}
			}

			pCurSample += channels;
			CurrSample++;

			if (CurrSample == NextSample)
			{
				i++;
				continue;
			}

			while (CurrSample < RightSampleInBuffer)
			{
				for (int ch = 0; ch < channels; ch++)
				{
					if (pPeaks[i].high < pCurSample[ch])
					{
						pPeaks[i].high = pCurSample[ch];
					}
					if (pPeaks[i].low > pCurSample[ch])
					{
						pPeaks[i].low = pCurSample[ch];
					}
				}

				pCurSample += channels;
				CurrSample++;

				if (CurrSample == NextSample)
				{
					i++;
					break;
				}
			}
		}
	}

	WAVE_PEAK PeakMax = -1.;
	WAVE_PEAK PeakMin = 1.;
	if (0 == nSamples)
	{
		PeakMax = 1;
		PeakMin = -1.;
	}
	else
	{
		WavePeak Peak = pDoc->m_WavFile.GetPeakMinMax(0, TotalPeaks);
		PeakMax = Peak.high;
		PeakMin = Peak.low;
	}

	if (PeakMax == PeakMin
		&& fabs(PeakMax) < 1.)
	{
		PeakMax = 1.;
		PeakMin = 1.;
	}

	PeakMax = std::abs(PeakMax);
	PeakMin = std::abs(PeakMin);
	if (PeakMax < PeakMin)
	{
		PeakMax = PeakMin;
	}
	m_LastMaxAmplitude = PeakMax;

	if (0 == PeakMax)
	{
		PeakMax = 1;
	}

	CPen WaveformPen;
	WaveformPen.CreatePen(PS_SOLID, 0, pApp->m_WaveColor);
	pDC->SelectObject( & WaveformPen);

	for (i = ur.left; i < ur.right; i++)
	{
		if (pPeaks[i].low <= pPeaks[i].high)
		{
			int y1 = int((PeakMax - pPeaks[i].low) * (cr.bottom - 1) / (PeakMax+PeakMax));
			int y2 = int((PeakMax - pPeaks[i].high) * (cr.bottom - 1) / (PeakMax+PeakMax));
			pDC->MoveTo(i, y1);
			pDC->LineTo(i, y2 - 1);
		}
	}

	if (0 != nSamples)
	{
		pDC->SelectObject( & BlackPen);
		// draw window boundary
		int nLeftPos = MulDiv(m_LeftViewBoundary, cr.right, nSamples);
		int nRightPos = MulDiv(m_RightViewBoundary, cr.right, nSamples);
		pDC->MoveTo(nLeftPos, 0);
		pDC->LineTo(nLeftPos, cr.bottom - 2);
		pDC->LineTo(nRightPos, cr.bottom - 2);
		pDC->LineTo(nRightPos, 0);
		pDC->LineTo(nLeftPos, 0);

		// draw playback cursor
		if (-1 != m_PlaybackCursorPosition)
		{
			int nCursorPos = MulDiv(m_PlaybackCursorPosition, cr.right, nSamples);
			pDC->MoveTo(nCursorPos, 0);
			pDC->LineTo(nCursorPos, cr.bottom - 1);
		}
	}
	// draw markers
	SAMPLE_INDEX_Vector markers;
	pDoc->m_WavFile.GetSortedMarkers(markers, FALSE);
	CPen DotPen(PS_DOT, 1, RGB(255, 255, 255));

	pDC->SelectObject(& DotPen);
	pDC->SetROP2(R2_XORPEN);
	pDC->SetBkColor(RGB(0, 0, 0));

	long prev_x = 0x7FFFFFFF;

	for (SAMPLE_INDEX_Vector::const_iterator ii = markers.begin();
		ii != markers.end(); ii++)
	{
		long x = MulDiv( *ii, cr.right, nSamples);

		if (x != prev_x
			&& x >= ur.left
			&& x < ur.right)
		{
			// draw marker
			pDC->MoveTo(x, cr.top);
			pDC->LineTo(x, cr.bottom);
			prev_x = x;
		}
	}

}

/////////////////////////////////////////////////////////////////////////////
// CWaveOutlineView diagnostics

#ifdef _DEBUG
void CWaveOutlineView::AssertValid() const
{
	CView::AssertValid();
}

void CWaveOutlineView::Dump(CDumpContext& dc) const
{
	CView::Dump(dc);
}

CWaveSoapFrontDoc* CWaveOutlineView::GetDocument() const// non-debug version is inline
{
	ASSERT(m_pDocument->IsKindOf(RUNTIME_CLASS(CWaveSoapFrontDoc)));
	return (CWaveSoapFrontDoc*)m_pDocument;
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CWaveOutlineView message handlers

void CWaveOutlineView::OnInitialUpdate()
{
	BaseClass::OnInitialUpdate();

	EnableToolTips();
}

void CWaveOutlineView::OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint)
{
	CWaveSoapFrontDoc * pDoc = GetDocument();
	NUMBER_OF_SAMPLES nSamples = pDoc->WaveFileSamples();
	CRect cr;
	GetClientRect( & cr);
	int width = cr.Width();

	if (lHint == CWaveSoapFrontDoc::UpdateSelectionChanged
		&& NULL != pHint)
	{
		CSelectionUpdateInfo * pInfo =
			dynamic_cast<CSelectionUpdateInfo *>(pHint);
		if (NULL == pInfo)
		{
			BaseClass::OnUpdate(pSender, lHint, pHint);
			return;
		}

		// calculate new selection boundaries
		int SelBegin = MulDiv(pInfo->SelBegin, width, nSamples);
		int SelEnd = MulDiv(pInfo->SelEnd, width, nSamples);
		if (pInfo->SelEnd != pInfo->SelBegin
			&& SelEnd == SelBegin)
		{
			SelEnd++;
		}

		// calculate old selection boundaries
		int OldSelBegin = MulDiv(pInfo->OldSelBegin, width, nSamples);
		int OldSelEnd = MulDiv(pInfo->OldSelEnd, width, nSamples);

		if (pInfo->OldSelEnd != pInfo->OldSelBegin
			&& OldSelEnd == OldSelBegin)
		{
			OldSelEnd++;
		}

		// build rectangles with selection boundaries
		CRect r1(SelBegin, cr.top, SelEnd, cr.bottom);
		CRect r2(OldSelBegin, cr.top, OldSelEnd, cr.bottom);
		// invalidate the regions with changed selection
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
			InvalidateRect(& r1, FALSE);
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
			InvalidateRect(& r2, FALSE);
		}
		//CreateAndShowCaret();
	}
	else if (lHint == CWaveSoapFrontDoc::UpdateSoundChanged
			&& NULL != pHint)
	{
		CSoundUpdateInfo * pInfo = static_cast<CSoundUpdateInfo *>(pHint);
		CRect r1;

		if (pInfo->m_NewLength != -1)
		{
			Invalidate();
			return;
		}

		unsigned TotalPeaks = pDoc->m_WavFile.GetPeaksSize();

		if (0 == TotalPeaks)
		{
			Invalidate();
			return;
		}
		WAVE_PEAK PeakMax = SHORT_MIN;
		WAVE_PEAK PeakMin = SHORT_MAX;
		WavePeak Peak = pDoc->m_WavFile.GetPeakMinMax(0, TotalPeaks);

		PeakMax = abs(Peak.high);
		PeakMin = abs(Peak.low);

		if (PeakMax < PeakMin)
		{
			PeakMax = PeakMin;
		}
		if (m_LastMaxAmplitude != PeakMax)
		{
			Invalidate(FALSE);
			return;
		}

		r1.top = cr.top;
		r1.bottom = cr.bottom;
		// calculate update boundaries
		r1.left = MulDiv(pInfo->m_Begin, width, nSamples) - 1;
		r1.right = 1 + MulDiv(pInfo->m_End, width, nSamples);

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

			InvalidateRect(& r1, FALSE);
		}
	}
	else if (lHint == CWaveSoapFrontDoc::UpdatePlaybackPositionChanged
			&& NULL != pHint)
	{
		CPlaybackUpdateInfo * pInfo = static_cast<CPlaybackUpdateInfo *>(pHint);
		int OldPosition = MulDiv(m_PlaybackCursorPosition, width, nSamples);
		int NewPosition = MulDiv(pInfo->PlaybackPosition(), width, nSamples);
		// trigger redraw of
		if (NewPosition != OldPosition)
		{
			CRect r;
			r.top = cr.top;
			r.bottom = cr.bottom;

			if (-1 != m_PlaybackCursorPosition)
			{
				r.left = OldPosition;
				r.right = OldPosition + 1;
				InvalidateRect( & r, FALSE);
			}
			if (-1 != pInfo->PlaybackPosition())
			{
				r.left = NewPosition;
				r.right = NewPosition + 1;
				InvalidateRect( & r, FALSE);
			}
		}

		m_PlaybackCursorPosition = pInfo->PlaybackPosition();
	}
	else if (lHint == CWaveSoapFrontDoc::UpdateMarkerRegionChanged
			&& NULL != pHint)
	{
		WAVEREGIONINFO * pInfo = & static_cast<MarkerRegionUpdateInfo *> (pHint)->info;

		CRect r;

		long x = MulDiv(pInfo->Sample, width, nSamples);

		if (0 != (pInfo->Flags & (pInfo->ChangeSample | pInfo->Delete))
			&& x < cr.right && x >= cr.left)
		{
			// invalidate region begin marker
			r.left = x;
			r.right = x + 1;
			r.top = cr.top;
			r.bottom = cr.bottom;

			InvalidateRect(r, FALSE);
		}

		if (0 != pInfo->Length
			&& 0 != (pInfo->Flags
				& (pInfo->ChangeSample | pInfo->ChangeLength | pInfo->Delete)))
		{
			// invalidate end marker
			x = MulDiv(pInfo->Sample + pInfo->Length, width, nSamples);

			if (x < cr.right && x >= cr.left)
			{
				r.left = x;
				r.right = x + 1;
				r.top = cr.top;
				r.bottom = cr.bottom;

				InvalidateRect(r, FALSE);
			}
		}
	}
	else if (0 == lHint
			|| pDoc->UpdateWholeFileChanged == lHint)
	{
		CView::OnUpdate(pSender, lHint, pHint);
	}
}

BOOL CWaveOutlineView::PreCreateWindow(CREATESTRUCT& cs)
{
	cs.lpszClass = AfxRegisterWndClass(CS_HREDRAW | CS_DBLCLKS, NULL,
										NULL, NULL);

	return CView::PreCreateWindow(cs);
}

int CWaveOutlineView::OnMouseActivate(CWnd* pDesktopWnd, UINT nHitTest, UINT message)
{
	// bypass CView function
	return CWnd::OnMouseActivate(pDesktopWnd, nHitTest, message);
}

BOOL CWaveOutlineView::OnEraseBkgnd(CDC* /*pDC*/)
{
	return FALSE;
}

BOOL CWaveOutlineView::EraseBkgnd(CDC* pDC)
{
	CWaveSoapFrontDoc * pDoc = GetDocument();
	CBrush backBrush(GetSysColor(COLOR_MENU));
	CRect r;
	GetClientRect( & r);

	NUMBER_OF_SAMPLES nSamples = pDoc->WaveFileSamples();
	long SelBegin = MulDiv(pDoc->m_SelectionStart, r.Width(), nSamples);
	long SelEnd = MulDiv(pDoc->m_SelectionEnd, r.Width(), nSamples);

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
		CBrush SelectedBackBrush(GetSysColor(COLOR_GRAYTEXT));
		pDC->FillRect(r, & SelectedBackBrush);
	}
	return TRUE;
}

void CWaveOutlineView::NotifyViewExtents(SAMPLE_INDEX left, SAMPLE_INDEX right)
{
	CWaveSoapFrontDoc * pDoc = GetDocument();
	NUMBER_OF_SAMPLES nSamples = pDoc->WaveFileSamples();

	if (0 == nSamples)
	{
		return;
	}

	CRect cr;
	GetClientRect( & cr);
	CRect r;
	r.top = cr.top;
	r.bottom = cr.bottom;

	int OldLPosition = MulDiv(m_LeftViewBoundary, cr.Width(), nSamples);
	int NewLPosition = MulDiv(left, cr.Width(), nSamples);

	if (NewLPosition != OldLPosition)
	{
		if (-1 != m_LeftViewBoundary)
		{
			r.left = OldLPosition;
			r.right = OldLPosition + 1;
			InvalidateRect( & r, FALSE);
		}
		if (-1 != left)
		{
			r.left = NewLPosition;
			r.right = NewLPosition + 1;
			InvalidateRect( & r, FALSE);
		}
	}

	int OldRPosition = MulDiv(m_RightViewBoundary, cr.Width(), nSamples);
	int NewRPosition = MulDiv(right, cr.Width(), nSamples);

	if (NewRPosition != OldRPosition)
	{
		if (-1 != m_RightViewBoundary)
		{
			r.left = OldRPosition;
			r.right = OldRPosition + 1;
			InvalidateRect( & r, FALSE);
		}
		if (-1 != right)
		{
			r.left = NewRPosition;
			r.right = NewRPosition + 1;
			InvalidateRect( & r, FALSE);
		}
	}
	if (NewLPosition != OldLPosition
		|| NewRPosition != OldRPosition)
	{
		if (-1 != m_RightViewBoundary
			&& NewRPosition < OldRPosition)
		{
			NewRPosition = OldRPosition;
		}
		if (-1 != m_LeftViewBoundary
			&& NewLPosition > OldLPosition)
		{
			NewLPosition = OldLPosition;
		}
		r.left = NewLPosition;
		r.right = NewRPosition;
		r.top = 0;
		r.bottom = 1;
		InvalidateRect( & r, FALSE);
		r.top = cr.bottom - 2;
		r.bottom = cr.bottom - 1;
		InvalidateRect( & r, FALSE);
	}
	m_RightViewBoundary = right;
	m_LeftViewBoundary = left;
}

void CWaveOutlineView::OnLButtonDown(UINT nFlags, CPoint point)
{
	// set cursor and view to the clicked position
	CWaveSoapFrontDoc * pDoc = GetDocument();
	NUMBER_OF_SAMPLES nSamples = pDoc->WaveFileSamples();
	CRect cr;
	GetClientRect( & cr);
	int width = cr.Width();
	if (0 == width)
	{
		return;
	}

	SAMPLE_INDEX nSampleUnderMouse = MulDiv(point.x, nSamples, width);
	SAMPLE_INDEX SelectionStart = pDoc->m_SelectionStart;
	SAMPLE_INDEX SelectionEnd = pDoc->m_SelectionEnd;

	nKeyPressed = WM_LBUTTONDOWN;
	if ((nFlags & MK_SHIFT)
		//|| (nHit & (VSHT_SEL_BOUNDARY_L | VSHT_SEL_BOUNDARY_R))
		)
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
		pDoc->SetSelection(SelectionStart, SelectionEnd,
							ALL_CHANNELS, nSampleUnderMouse,
							SetSelection_MakeCaretVisible | SetSelection_Autoscroll);
	}
	else if (0 != nSamples)
	{
		unsigned PeaksSamples = pDoc->m_WavFile.GetPeaksSize() / pDoc->WaveChannels();
		unsigned Granularity = pDoc->m_WavFile.GetPeakGranularity();
		// round to peak info granularity

		SAMPLE_INDEX nBegin = Granularity * MulDiv(point.x, PeaksSamples, width);
		SAMPLE_INDEX nEnd = Granularity * MulDiv(point.x + 1, PeaksSamples, width);

		pDoc->SetSelection(nBegin, nEnd, ALL_CHANNELS, nBegin,
							SetSelection_SnapToMaximum | SetSelection_MakeCaretVisible | SetSelection_Autoscroll);
	}

}

void CWaveOutlineView::OnLButtonUp(UINT /*nFlags*/, CPoint point)
{
	CWaveSoapFrontDoc * pDoc = GetDocument();
	NUMBER_OF_SAMPLES nSamples = pDoc->WaveFileSamples();

	CRect cr;
	GetClientRect( & cr);

	SAMPLE_INDEX nBegin = 0;
	SAMPLE_INDEX nEnd = 0;

	int width = cr.Width();
	if (0 != width && 0 != nSamples)
	{
		// round to peak info granularity
		unsigned PeaksSamples = pDoc->m_WavFile.GetPeaksSize() / pDoc->WaveChannels();
		unsigned Granularity = pDoc->m_WavFile.GetPeakGranularity();
		// round to peak info granularity
		nBegin = Granularity * MulDiv(point.x, PeaksSamples, width);
		nEnd = Granularity * MulDiv(point.x + 1, PeaksSamples, width);
	}
	else
	{
		nSamples = 1;
	}

	if ( ! bIsTrackingSelection
		&&  nKeyPressed == WM_LBUTTONDOWN)
	{
		// mouse hasn't moved after click
		if (GetApp()->m_bSnapMouseSelectionToMax
			// the whole area wasn't selected
			&& pDoc->m_SelectionStart == pDoc->m_SelectionEnd)
		{
			// see is there is a marker

			SAMPLE_INDEX_Vector markers;
			pDoc->m_WavFile.GetSortedMarkers(markers, FALSE);

			unsigned Flags = SetSelection_SnapToMaximum | SetSelection_MakeCaretVisible | SetSelection_Autoscroll;
			for (SAMPLE_INDEX_Vector::const_iterator i = markers.begin(); i != markers.end(); i++)
			{
				long x = MulDiv( *i, cr.right, nSamples);

				if (x == point.x)
				{
					// goto marker
					nBegin = *i;
					nEnd = nBegin;
					Flags = SetSelection_KeepCaretVisible;
					break;
				}
			}

			pDoc->SetSelection(nBegin, nEnd, ALL_CHANNELS, nBegin, Flags);
		}
	}

	nKeyPressed = 0;
	if (bIsTrackingSelection)
	{
		ReleaseCapture();
		bIsTrackingSelection = FALSE;
	}
}

void CWaveOutlineView::OnMouseMove(UINT nFlags, CPoint point)
{
	// point is in client coordinates
	CWaveSoapFrontDoc * pDoc = GetDocument();
	CRect cr;
	GetClientRect( & cr);
	int width = cr.Width();

	NUMBER_OF_SAMPLES nSamples = pDoc->WaveFileSamples();
	if (width <= 0 || 0 == nSamples)
	{
		CView::OnMouseMove(nFlags, point);
		return;
	}

	// round to peak info granularity
	unsigned PeaksSamples = pDoc->m_WavFile.GetPeaksSize() / pDoc->WaveChannels();
	unsigned Granularity = pDoc->m_WavFile.GetPeakGranularity();
	// round to peak info granularity
	SAMPLE_INDEX nBegin = Granularity * MulDiv(point.x, PeaksSamples, width);
	SAMPLE_INDEX nEnd = Granularity * MulDiv(point.x + 1, PeaksSamples, width);

	if (nBegin < 0)
	{
		nBegin = 0;
	}
	if (nBegin > nSamples)
	{
		nBegin = nSamples;
	}

	if (nEnd < 0)
	{
		nEnd = 0;
	}
	if (nEnd > nSamples)
	{
		nEnd = nSamples;
	}


	SAMPLE_INDEX SelectionStart = pDoc->m_SelectionStart;
	SAMPLE_INDEX SelectionEnd = pDoc->m_SelectionEnd;

	CView::OnMouseMove(nFlags, point);
	if (nKeyPressed != 0)
	{
		if ( ! bIsTrackingSelection)
		{
			if (pDoc->m_CaretPosition >= nBegin
				&& pDoc->m_CaretPosition < nEnd)
			{
				// mouse didn't move outside this column
				return;
			}
			bIsTrackingSelection = TRUE;
			SetCapture();
		}

		// tracked side (where the caret is) is moved,
		// other side stays
		SAMPLE_INDEX nSampleUnderMouse;
		if (SelectionStart == pDoc->m_CaretPosition)
		{
			SelectionStart = nBegin;
			nSampleUnderMouse = nBegin;
		}
		else if (SelectionEnd == pDoc->m_CaretPosition)
		{
			SelectionEnd = nEnd;
			nSampleUnderMouse = nEnd;
		}
		else if (nBegin <
				(double(SelectionStart) + SelectionEnd) / 2)
		{
			SelectionStart = nBegin;
			nSampleUnderMouse = nBegin;
		}
		else
		{
			SelectionEnd = nEnd;
			nSampleUnderMouse = nEnd;
		}

		pDoc->SetSelection(SelectionStart, SelectionEnd,
							ALL_CHANNELS, nSampleUnderMouse, SetSelection_MakeCaretVisible | SetSelection_Autoscroll);
	}
}

void CWaveOutlineView::OnCaptureChanged(CWnd *pWnd)
{
	if (pWnd != this)
	{
		bIsTrackingSelection = FALSE;
		nKeyPressed = 0;
	}

	CView::OnCaptureChanged(pWnd);
}

BOOL CWaveOutlineView::OnSetCursor(CWnd* /*pWnd*/,
									UINT /*nHitTest*/, UINT /*message*/)
{
	// TODO: set different cursor, depending on hit test
	SetCursor(AfxGetApp()->LoadStandardCursor(IDC_ARROW));
	return TRUE;
}

void CWaveOutlineView::OnLButtonDblClk(UINT nFlags, CPoint point)
{
	CView::OnLButtonDblClk(nFlags, point);

	CRect cr;
	GetClientRect(cr);

	if (cr.Width() != 0)
	{
		GetDocument()->SelectBetweenMarkers(SAMPLE_INDEX(
														MulDiv(point.x, GetDocument()->WaveFileSamples(), cr.Width())));
	}
}

BOOL CWaveOutlineView::OnNotify(WPARAM wParam, LPARAM lParam, LRESULT* pResult)
{
	NMHDR * pNMHDR = (NMHDR *) lParam;
	if (pNMHDR->code == TTN_NEEDTEXTA || pNMHDR->code == TTN_NEEDTEXTW)
	{
		OnToolTipText((UINT)wParam, pNMHDR, pResult);
		return 0;
	}
	return BaseClass::OnNotify(wParam, lParam, pResult);
}

INT_PTR CWaveOutlineView::OnToolHitTest(CPoint point, TOOLINFO* pTI) const
{
	RECT r;
	unsigned const hit = HitTest(point, & r);
	if (HitTestNone == hit)
	{
		return -1;
	}

	if (pTI != NULL && pTI->cbSize >= offsetof(TOOLINFO, lParam))
	{
		pTI->hwnd = m_hWnd;
		pTI->uId = hit;
		pTI->rect = r;
		pTI->lpszText = LPSTR_TEXTCALLBACK;
	}

	return hit;
}

void CWaveOutlineView::OnToolTipText(UINT /*id*/, NMHDR* pNMHDR, LRESULT* pResult)
{
	ASSERT(pNMHDR->code == TTN_NEEDTEXTA || pNMHDR->code == TTN_NEEDTEXTW);

	// need to handle both ANSI and UNICODE versions of the message
	TOOLTIPTEXTA* pTTTA = (TOOLTIPTEXTA*)pNMHDR;
	TOOLTIPTEXTW* pTTTW = (TOOLTIPTEXTW*)pNMHDR;

	CWaveFile::InstanceDataWav * pInst = GetDocument()->m_WavFile.GetInstanceData();

	LPCTSTR strTipText = pInst->GetCueTextByIndex(pNMHDR->idFrom & HitTestCueIndexMask);
	if (NULL != strTipText)
	{
#ifndef _UNICODE
		if (pNMHDR->code == TTN_NEEDTEXTA)
		{
			lstrcpyn(pTTTA->szText, strTipText, countof(pTTTA->szText));
		}
		else
		{
			_mbstowcsz(pTTTW->szText, strTipText, countof(pTTTW->szText));
		}
#else
		if (pNMHDR->code == TTN_NEEDTEXTA)
		{
			_wcstombsz(pTTTA->szText, strTipText, countof(pTTTA->szText));
		}
		else
		{
			lstrcpyn(pTTTW->szText, strTipText, countof(pTTTW->szText));
		}
#endif
		*pResult = 0;
	}

	// bring the tooltip window above other popup windows
	::SetWindowPos(pNMHDR->hwndFrom, HWND_TOP, 0, 0, 0, 0,
					SWP_NOACTIVATE|SWP_NOSIZE|SWP_NOMOVE|SWP_NOOWNERZORDER);
}

// the function accepts point coordinates relative to the client area
// it puts the marker rectangle into optional *pHitRect, and offset from the marker origin to *OffsetX
unsigned CWaveOutlineView::HitTest(POINT p, RECT * pHitRect/*, int * OffsetX*/) const
{
	unsigned result = HitTestNone;

	CWaveSoapFrontDoc * pDoc = GetDocument();
	NUMBER_OF_SAMPLES const NumberOfSamples = pDoc->WaveFileSamples();

	CRect cr;
	GetClientRect(cr);

	int ClientWidth = cr.Width();
	if (0 == ClientWidth
		|| 0 == NumberOfSamples)
	{
		return result;
	}

	CWaveFile::InstanceDataWav * pInst = pDoc->m_WavFile.GetInstanceData();
	if (NULL == pInst)
	{
		return 0;
	}

	int n;
	CuePointVectorIterator i;

	for (n = 0, i = pInst->m_CuePoints.begin();
		i != pInst->m_CuePoints.end(); i++, n++)
	{
		long x = MulDiv(i->dwSampleOffset, ClientWidth, NumberOfSamples);
		WaveRegionMarker * pMarker = pInst->GetRegionMarker(i->CuePointID);

		if (NULL == pMarker
			|| 0 == pMarker->SampleLength)
		{
			if (p.x == x)
			{
				// marker
				result = HitTestMarker | n;

				break;
			}
		}
		else
		{
			if (p.x == x)
			{
				// mark of the region begin

				result = HitTestRegionBegin | n;

				break;
			}

			x = MulDiv(i->dwSampleOffset + pMarker->SampleLength, ClientWidth, NumberOfSamples);

			if (p.x == x)
			{
				// mark of the region end
				result = HitTestRegionEnd | n;

				break;
			}
		}
	}

	if (result != HitTestNone
		&& NULL != pHitRect)
	{
		pHitRect->left = p.x;
		pHitRect->right = p.x + 1;
		pHitRect->top = cr.top;
		pHitRect->bottom = cr.bottom;
	}

	return result;
}

afx_msg LRESULT CWaveOutlineView::OnUwmNotifyViews(WPARAM wParam, LPARAM lParam)
{
	CRect cr;
	GetClientRect(cr);
	NotifyViewsData *data = (NotifyViewsData *)lParam;

	switch (wParam)
	{
	case HorizontalOriginChanged:
	case HorizontalExtentChanged:
		NotifyViewExtents(data->HorizontalScroll.FirstSampleInView,
						data->HorizontalScroll.FirstSampleInView + data->HorizontalScroll.TotalSamplesInView - 1);
		break;
	}
	return 0;
}

