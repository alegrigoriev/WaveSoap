// WaveOutlineView.cpp : implementation file
//

#include "stdafx.h"
#include "WaveSoapFront.h"
#include "WaveOutlineView.h"

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
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CWaveOutlineView drawing

void CWaveOutlineView::OnDraw(CDC* pDC)
{
	CWaveSoapFrontDoc* pDoc = GetDocument();
	if (! pDoc->m_WavFile.IsOpen())
	{
		return;
	}
	// the whole file is here
	CRect cr;
	GetClientRect( & cr);
	CRect ur;
	if (pDC->IsKindOf(RUNTIME_CLASS(CPaintDC)))
	{
		CPaintDC* pPaintDC = (CPaintDC*)pDC;
		ur = pPaintDC->m_ps.rcPaint;
	}
	else
	{
		pDC->GetClipBox( & ur);
	}
	if (ur.right > cr.right)
	{
		ur.right = cr.right;
	}
	int width = cr.Width();
	if (width == 0)
	{
		return;
	}

	long nSamples = pDoc->WaveFileSamples();
	WavePeak * pDocPeaks = pDoc->m_pPeaks;
	if (NULL == pDocPeaks || 0 == nSamples)
	{
		return;
	}
	WavePeak * pPeaks = new WavePeak[width];
	if (NULL == pPeaks)
	{
		return;
	}

	int channels = pDoc->WaveChannels();

	int i;
	if (pDoc->m_WavePeakSize / channels >= width)
	{
		int PrevIdx = ur.left * pDoc->m_WavePeakSize / width;
		for (i = ur.left; i < ur.right; i++)
		{
			pPeaks[i].high = -0x8000;
			pPeaks[i].low = 0x7FFF;
			int NewIdx = (i + 1) * pDoc->m_WavePeakSize / width;
			for (int j = PrevIdx; j < NewIdx; j++)
			{
				if (pPeaks[i].low > pDocPeaks[j].low)
				{
					pPeaks[i].low = pDocPeaks[j].low;
				}
				if (pPeaks[i].high < pDocPeaks[j].high)
				{
					pPeaks[i].high = pDocPeaks[j].high;
				}
			}
			PrevIdx = NewIdx;
		}
	}
	else
	{
		// use data from the file
		int SampleSize = pDoc->WaveSampleSize();
		size_t BufSize = (nSamples / width + 2) * SampleSize;
		__int16 * pBuf = new __int16[BufSize / sizeof(__int16)];
		if (NULL == pBuf)
		{
			delete[] pPeaks;
			return;
		}
		int PrevIdx = MulDiv(ur.left, nSamples, width);
		size_t DataOffset = pDoc->WaveDataChunk()->dwDataOffset;

		for (i = ur.left; i < ur.right; i++)
		{
			int NewIdx = MulDiv(i + 1, nSamples, width);
			pPeaks[i].high = -0x8000;
			pPeaks[i].low = 0x7FFF;
			DWORD Offset = DataOffset + SampleSize * PrevIdx;
			size_t ToRead = (NewIdx - PrevIdx) * SampleSize;
			if (ToRead != 0)
			{
				if (ToRead > BufSize)
				{
					ToRead = BufSize;
					TRACE("Miscalculation: ToRead > BufSize\n");
				}
				ToRead = pDoc->m_WavFile.ReadAt(pBuf, ToRead, Offset);
				for (int j = 0; j < ToRead / sizeof (__int16); j++)
				{
					if (pPeaks[i].low > pBuf[j])
					{
						pPeaks[i].low = pBuf[j];
					}
					if (pPeaks[i].high < pBuf[j])
					{
						pPeaks[i].high = pBuf[j];
					}
				}

			}
			PrevIdx = NewIdx;
		}
		delete[] pBuf;
	}

	int PeakMax = -0x8000;
	int PeakMin = 0x7FFF;
	for (i = 0; i < pDoc->m_WavePeakSize; i++)
	{
		if (PeakMin > pDocPeaks[i].low)
		{
			PeakMin = pDocPeaks[i].low;
		}
		if (PeakMax < pDocPeaks[i].high)
		{
			PeakMax = pDocPeaks[i].high;
		}
	}

	PeakMax = abs(PeakMax);
	PeakMin = abs(PeakMin);
	if (PeakMax < PeakMin)
	{
		PeakMax = PeakMin;
	}
	m_LastMaxAmplitude = PeakMax;

	CPen WaveformPen;
	CWaveSoapFrontApp * pApp = GetApp();
	WaveformPen.CreatePen(PS_SOLID, 0, pApp->m_WaveColor);
	CGdiObject * pOldPen = pDC->SelectObject( & WaveformPen);

	if (pDC->IsKindOf(RUNTIME_CLASS(CPaintDC)))
	{
		CPaintDC* pPaintDC = (CPaintDC*)pDC;
		if (pPaintDC->m_ps.fErase)
		{
			EraseBkgnd(pPaintDC);
		}
	}

	if (0 == PeakMax)
	{
		PeakMax = 1;
	}
	for (i = ur.left; i < ur.right; i++)
	{
		if (pPeaks[i].low <= pPeaks[i].high)
		{
			int y1 = (PeakMax - pPeaks[i].low) * (cr.bottom - 1)/ (PeakMax+PeakMax);
			int y2 = (PeakMax - pPeaks[i].high) * (cr.bottom - 1)/ (PeakMax+PeakMax);
			pDC->MoveTo(i, y1);
			pDC->LineTo(i, y2 - 1);
		}
	}
	CPen BlackPen;
	BlackPen.CreatePen(PS_SOLID, 0, DWORD(0x000000u));
	pDC->SelectObject( & BlackPen);
	// draw lower border line
	pDC->MoveTo(cr.left, cr.bottom - 1);
	pDC->LineTo(cr.right, cr.bottom - 1);
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
	pDC->SelectObject(pOldPen);
	delete[] pPeaks;
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

CWaveSoapFrontDoc* CWaveOutlineView::GetDocument() // non-debug version is inline
{
	ASSERT(m_pDocument->IsKindOf(RUNTIME_CLASS(CWaveSoapFrontDoc)));
	return (CWaveSoapFrontDoc*)m_pDocument;
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CWaveOutlineView message handlers

void CWaveOutlineView::OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint)
{
	CWaveSoapFrontDoc * pDoc = GetDocument();
	long nSamples = pDoc->WaveFileSamples();
	CRect cr;
	GetClientRect( & cr);

	if (lHint == CWaveSoapFrontDoc::UpdateSelectionChanged
		&& NULL != pHint)
	{
		CSelectionUpdateInfo * pInfo = (CSelectionUpdateInfo *) pHint;

		// calculate new selection boundaries
		int SelBegin = MulDiv(pDoc->m_SelectionStart, cr.Width(), nSamples);
		int SelEnd = MulDiv(pDoc->m_SelectionEnd, cr.Width(), nSamples);
		if (pDoc->m_SelectionEnd != pDoc->m_SelectionStart
			&& SelEnd == SelBegin)
		{
			SelEnd++;
		}

		// calculate old selection boundaries
		int OldSelBegin = MulDiv(pInfo->SelBegin, cr.Width(), nSamples);
		int OldSelEnd = MulDiv(pInfo->SelEnd, cr.Width(), nSamples);
		if (pInfo->SelEnd != pInfo->SelBegin
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
		CSoundUpdateInfo * pInfo = (CSoundUpdateInfo *) pHint;
		CRect r1;

		if (pInfo->Length != -1)
		{
			Invalidate();
			return;
		}

		int PeakMax = -0x8000;
		int PeakMin = 0x7FFF;
		WavePeak * pDocPeaks = pDoc->m_pPeaks;
		if (NULL == pDocPeaks)
		{
			Invalidate();
			return;
		}
		for (int i = 0; i < pDoc->m_WavePeakSize; i++)
		{
			if (PeakMin > pDocPeaks[i].low)
			{
				PeakMin = pDocPeaks[i].low;
			}
			if (PeakMax < pDocPeaks[i].high)
			{
				PeakMax = pDocPeaks[i].high;
			}
		}

		PeakMax = abs(PeakMax);
		PeakMin = abs(PeakMin);
		if (PeakMax < PeakMin)
		{
			PeakMax = PeakMin;
		}
		if (m_LastMaxAmplitude != PeakMax)
		{
			Invalidate();
			return;
		}

		r1.top = cr.top;
		r1.bottom = cr.bottom;
		// calculate update boundaries
		r1.left = MulDiv(pInfo->Begin, cr.Width(), nSamples) - 1;
		r1.right = 1 + MulDiv(pInfo->End, cr.Width(), nSamples);

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
//            TRACE("CWaveOutlineView: invalidate %d...%d\n", r1.left, r1.right);
			InvalidateRect(& r1, FALSE);
		}
	}
	else if (lHint == CWaveSoapFrontDoc::UpdatePlaybackPositionChanged
			&& NULL != pHint)
	{
		CSoundUpdateInfo * pInfo = (CSoundUpdateInfo *) pHint;
		int OldPosition = MulDiv(m_PlaybackCursorPosition, cr.Width(), nSamples);
		int NewPosition = MulDiv(pInfo->Begin, cr.Width(), nSamples);
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
			if (-1 != pInfo->Begin)
			{
				r.left = NewPosition;
				r.right = NewPosition + 1;
				InvalidateRect( & r, FALSE);
			}
		}
		m_PlaybackCursorPosition = pInfo->Begin;
	}
	else
	{
		CView::OnUpdate(pSender, lHint, pHint);
	}
}

BOOL CWaveOutlineView::PreCreateWindow(CREATESTRUCT& cs)
{
	cs.lpszClass = AfxRegisterWndClass(CS_HREDRAW | CS_DBLCLKS, AfxGetApp()->LoadStandardCursor(IDC_ARROW),
										NULL, NULL);

	return CView::PreCreateWindow(cs);
}

int CWaveOutlineView::OnMouseActivate(CWnd* pDesktopWnd, UINT nHitTest, UINT message)
{
	// bypass CView function
	return CWnd::OnMouseActivate(pDesktopWnd, nHitTest, message);
}

BOOL CWaveOutlineView::OnEraseBkgnd(CDC* pDC)
{
	return FALSE;
}

BOOL CWaveOutlineView::EraseBkgnd(CDC* pDC)
{
	CWaveSoapFrontApp * pApp = (CWaveSoapFrontApp *) AfxGetApp();
	CWaveSoapFrontDoc * pDoc = GetDocument();
	CBrush backBrush(GetSysColor(COLOR_MENU));
	CRect r;
	GetClientRect( & r);

	long nSamples = pDoc->WaveFileSamples();
	int SelBegin = MulDiv(pDoc->m_SelectionStart, r.Width(), nSamples);
	int SelEnd = MulDiv(pDoc->m_SelectionEnd, r.Width(), nSamples);

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

void CWaveOutlineView::NotifyViewExtents(long left, long right)
{
	CWaveSoapFrontDoc * pDoc = GetDocument();
	long nSamples = pDoc->WaveFileSamples();
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
	long nSamples = pDoc->WaveFileSamples();
	CRect cr;
	GetClientRect( & cr);
	if (0 == cr.Width())
	{
		return;
	}
	int nSampleUnderMouse = MulDiv(point.x, nSamples, cr.Width());
	pDoc->SetSelection(nSampleUnderMouse, nSampleUnderMouse,
						pDoc->m_SelectedChannel, nSampleUnderMouse, SetSelection_MoveCaretToCenter);

	//CView::OnLButtonDown(nFlags, point);
}

void CWaveOutlineView::OnLButtonUp(UINT nFlags, CPoint point)
{
	// TODO: Add your message handler code here and/or call default

	CView::OnLButtonUp(nFlags, point);
}

void CWaveOutlineView::OnMouseMove(UINT nFlags, CPoint point)
{
	// TODO: Add your message handler code here and/or call default

	CView::OnMouseMove(nFlags, point);
}
