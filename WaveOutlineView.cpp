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
{
}

CWaveOutlineView::~CWaveOutlineView()
{
}


BEGIN_MESSAGE_MAP(CWaveOutlineView, CView)
	//{{AFX_MSG_MAP(CWaveOutlineView)
	ON_WM_MOUSEACTIVATE()
	ON_WM_ERASEBKGND()
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
	CRect ur;
	if (pDC->IsKindOf(RUNTIME_CLASS(CPaintDC)))
	{
		ur = ((CPaintDC*)pDC)->m_ps.rcPaint;
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

	WavePeak * pPeaks = new WavePeak[width];
	if (NULL == pPeaks)
	{
		return;
	}

	int channels = pDoc->WaveChannels();
	WavePeak * pDocPeaks = pDoc->m_pPeaks;
	if (NULL == pDocPeaks)
	{
		delete[] pPeaks;
		return;
	}
	int i;
	int PeakMax = -0x8000;
	int PeakMin = 0x7FFF;
	int PrevIdx = ur.left * pDoc->m_WavePeakSize / width;;
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

	CPen WaveformPen;
	CWaveSoapFrontApp * pApp = GetApp();
	WaveformPen.CreatePen(PS_SOLID, 1, pApp->m_WaveColor);
	CGdiObject * pOldPen = pDC->SelectObject( & WaveformPen);

	PeakMax = abs(PeakMax);
	PeakMin = abs(PeakMin);
	if (PeakMax < PeakMin)
	{
		PeakMax = PeakMin;
	}
	for (i = ur.left; i < ur.right; i++)
	{
		if (pPeaks[i].low < pPeaks[i].high)
		{
			int y1 = (PeakMax - pPeaks[i].low) * (cr.bottom - 1)/ (PeakMax+PeakMax);
			int y2 = (PeakMax - pPeaks[i].high) * (cr.bottom - 1)/ (PeakMax+PeakMax);
			pDC->MoveTo(i, y1);
			pDC->LineTo(i, y2 - 1);
		}
	}
	pDC->SelectStockObject(BLACK_PEN);
	// draw lower border line
	pDC->MoveTo(cr.left, cr.bottom - 1);
	pDC->LineTo(cr.right, cr.bottom - 1);
	// draw cursor
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
	if (lHint == CWaveSoapFrontDoc::UpdateSelectionChanged
		&& NULL != pHint)
	{
		CSelectionUpdateInfo * pInfo = (CSelectionUpdateInfo *) pHint;
		CWaveSoapFrontDoc * pDoc = GetDocument();
		CRect r;

		GetClientRect( & r);

		// calculate new selection boundaries
		long nSamples = pDoc->WaveFileSamples();
		int SelBegin = MulDiv(pDoc->m_SelectionStart, r.Width(), nSamples);
		int SelEnd = MulDiv(pDoc->m_SelectionEnd, r.Width(), nSamples);
		if (pDoc->m_SelectionEnd != pDoc->m_SelectionStart
			&& SelEnd == SelBegin)
		{
			SelEnd++;
		}

		// calculate old selection boundaries
		int OldSelBegin = MulDiv(pInfo->SelBegin, r.Width(), nSamples);
		int OldSelEnd = MulDiv(pInfo->SelEnd, r.Width(), nSamples);
		if (pInfo->SelEnd != pInfo->SelBegin
			&& OldSelEnd == OldSelBegin)
		{
			OldSelEnd++;
		}

		// build rectangles with selection boundaries
		CRect r1(SelBegin, r.top, SelEnd, r.bottom);
		CRect r2(OldSelBegin, r.top, OldSelEnd, r.bottom);
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
		//CreateAndShowCaret();
	}
	else if (lHint == CWaveSoapFrontDoc::UpdateSoundChanged
			&& NULL != pHint)
	{
		CSoundUpdateInfo * pInfo = (CSoundUpdateInfo *) pHint;
		CWaveSoapFrontDoc * pDoc = GetDocument();
		CRect r;
		GetClientRect( & r);
		CRect r1;

		r1.top = r.top;
		r1.bottom = r.bottom;
		if (pInfo->Length != -1)
		{
			Invalidate();
			return;
		}

		// calculate update boundaries
		long nSamples = pDoc->WaveFileSamples();
		r1.left = MulDiv(pInfo->Begin, r.Width(), nSamples);
		r1.right = 1 + MulDiv(pInfo->End, r.Width(), nSamples);

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
			TRACE("CWaveOutlineView: invalidate %d...%d\n", r1.left, r1.right);
			InvalidateRect(& r1, TRUE);
		}
	}
	else if (lHint == CWaveSoapFrontDoc::UpdatePlaybackPositionChanged
			&& NULL != pHint)
	{
		CSoundUpdateInfo * pInfo = (CSoundUpdateInfo *) pHint;
		//UpdatePlaybackCursor(pInfo->Begin, pInfo->End);
	}
	else
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

BOOL CWaveOutlineView::OnEraseBkgnd(CDC* pDC)
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
