// Copyright Alexander Grigoriev, 1997-2002, All Rights Reserved
// TimeRulerView.cpp : implementation file
//

#include "stdafx.h"
#include "WaveSoapFront.h"
#include "TimeRulerView.h"
#include "WaveSoapFrontView.h"
#include "GdiObjectSave.h"
#include "TimeToStr.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

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
// CTimeRulerView

IMPLEMENT_DYNCREATE(CTimeRulerView, CHorizontalRuler)

CTimeRulerView::CTimeRulerView()
	: m_CurrentDisplayMode(ShowHhMmSs)
{
}

CTimeRulerView::~CTimeRulerView()
{
}


BEGIN_MESSAGE_MAP(CTimeRulerView, CHorizontalRuler)
	//{{AFX_MSG_MAP(CTimeRulerView)
	ON_COMMAND(IDC_VIEW_RULER_HHMMSS, OnViewRulerHhmmss)
	ON_UPDATE_COMMAND_UI(IDC_VIEW_RULER_HHMMSS, OnUpdateViewRulerHhmmss)
	ON_COMMAND(IDC_VIEW_RULER_SAMPLES, OnViewRulerSamples)
	ON_UPDATE_COMMAND_UI(IDC_VIEW_RULER_SAMPLES, OnUpdateViewRulerSamples)
	ON_COMMAND(IDC_VIEW_RULER_SECONDS, OnViewRulerSeconds)
	ON_UPDATE_COMMAND_UI(IDC_VIEW_RULER_SECONDS, OnUpdateViewRulerSeconds)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CTimeRulerView drawing

void CTimeRulerView::OnDraw(CDC* pDC)
{
	CWaveSoapFrontDoc* pDoc = GetDocument();
	if (! pDoc->m_WavFile.IsOpen())
	{
		return;
	}
	// background is erased by gray brush.
	// draw horizontal line with ticks and numbers
	CGdiObjectSave OldFont(pDC, pDC->SelectStockObject(ANSI_VAR_FONT));

	CPen DarkGrayPen(PS_SOLID, 0, 0x808080);
	CRect cr;
	GetClientRect( & cr);

	pDC->SetTextAlign(TA_BOTTOM | TA_LEFT);
	pDC->SetTextColor(0x000000);   // black
	pDC->SetBkMode(TRANSPARENT);

	CGdiObjectSave OldPen(pDC, pDC->SelectStockObject(BLACK_PEN));

	pDC->MoveTo(cr.left, cr.bottom - 5);
	pDC->LineTo(cr.right, cr.bottom - 5);

	pDC->SelectStockObject(WHITE_PEN);
	pDC->MoveTo(cr.left, cr.bottom - 4);
	pDC->LineTo(cr.right, cr.bottom - 4);

	pDC->SelectObject( & DarkGrayPen);
	pDC->MoveTo(cr.left, cr.bottom - 6);
	pDC->LineTo(cr.right, cr.bottom - 6);

	TCHAR const DecimalPoint = GetApp()->m_DecimalPoint;
	float const SampleRate = float(pDoc->WaveSampleRate());
	int nTickCount;
	double DistTime;
	double nFirstTime;
	int nLength;

	CString s;

	switch (m_CurrentDisplayMode)
	{
	case ShowSamples:
	{
		// calculate position string length

		nLength = pDC->GetTextExtent(_T("0,000,000,000"), 13).cx;

		NUMBER_OF_SAMPLES nSamples = NUMBER_OF_SAMPLES(1.5 * nLength / GetXScaleDev());
		// calculate how much samples can be between the numbers
		if (nSamples > INT_MAX / 10)
		{
			nSamples = INT_MAX / 10;
		}

		int dist = 1;
		unsigned nTickDist = 1;
		// find the closest bigger 1,2,5 * 10^x
		nTickCount = 1;

		for (unsigned k = 1; k <= INT_MAX / 10; k *= 10)
		{
			dist = k;
			nTickDist = k;

			if (dist >= nSamples)
			{
				if (dist > 10)
				{
					nTickDist = dist / 10;
					nTickCount = 10;
				}
				break;
			}
			dist = k * 2;
			if (dist >= nSamples)
			{
				nTickCount = 2;
				break;
			}
			dist = k * 5;
			if (dist >= nSamples)
			{
				nTickCount = 2;
				break;
			}
		}

		DistTime = dist / SampleRate;
	}
		break;
	case ShowHhMmSs:
	{
		// calculate position string length

		nLength = pDC->GetTextExtent(_T("00:00:00.0000"), 13).cx;

		DistTime = 1.5 * nLength / GetXScaleDev() / SampleRate;
		// select distance between ticks
		double multiplier = 1.;
		double divisor = 1.;

		if (DistTime >= 1.)
		{
			if (DistTime > 3600.)
			{
				multiplier = 3600.;
				DistTime = ceil(DistTime / 3600.);
			}
			else if (DistTime > 60.)
			{
				multiplier = 60.;
				DistTime = ceil(DistTime / 60.);
			}
			else
			{
				DistTime = ceil(DistTime);
			}
			// find the closest bigger 1,2,5, 10, 20, 30
			if (DistTime <= 1.)
			{
				DistTime = 1.;
				nTickCount = 10;
			}
			else if (DistTime <= 2.)
			{
				DistTime = 2.;
				nTickCount = 2;
			}
			else if (DistTime <= 5.)
			{
				DistTime = 5.;
				nTickCount = 5;
			}
			else if (DistTime <= 10.)
			{
				DistTime = 10.;
				nTickCount = 10;
			}
			else if (DistTime <= 20.)
			{
				DistTime = 20.;
				nTickCount = 2;
			}
			else if (DistTime <= 30.)
			{
				DistTime = 30.;
				nTickCount = 3;
			}
			else
			{
				DistTime = 60.;
				nTickCount = 6;
			}
		}
		else    // DistTime < 1
		{
			divisor = 1000.;
			DistTime = ceil(DistTime * 1000.);
			// find the closest bigger 1,2,5 * 10^x
			for (; ; multiplier *= 10., DistTime = ceil(DistTime / 10.))
			{
				if (DistTime <= 1.)
				{
					DistTime = 1.;
					nTickCount = 10;
					break;
				}
				else if (DistTime <= 2.)
				{
					DistTime = 2.;
					nTickCount = 2;
					break;
				}
				else if (DistTime <= 5.)
				{
					DistTime = 5.;
					nTickCount = 5;
					break;
				}
			}
		}

		DistTime = DistTime * multiplier / divisor;
	}
		break;

	case ShowSeconds:
	{
		// calculate position string length

		nLength = pDC->GetTextExtent(_T("00,000.0000"), 14).cx;

		DistTime = 1.5 * nLength / GetXScaleDev() / SampleRate;
		// select distance between ticks
		double multiplier = 1.;
		double divisor = 1.;

		if (DistTime >= 1.)
		{
			DistTime = ceil(DistTime);
		}
		else
		{
			divisor = 1000.;
			DistTime = ceil(DistTime * 1000.);
		}

		// find the closest bigger 1,2,5 * 10^x
		for (; ; multiplier *= 10., DistTime = ceil(DistTime / 10.))
		{
			if (DistTime <= 1.)
			{
				DistTime = 1.;
				nTickCount = 10;
				break;
			}
			else if (DistTime <= 2.)
			{
				DistTime = 2.;
				nTickCount = 2;
				break;
			}
			else if (DistTime <= 5.)
			{
				DistTime = 5.;
				nTickCount = 5;
				break;
			}
		}

		DistTime = DistTime * multiplier / divisor;
	}
		break;

	default:
		return;
	}

	nFirstTime = floor(WindowToWorldX(cr.left - nLength) / (SampleRate * DistTime))
				* DistTime;

	if (nFirstTime < 0)
	{
		nFirstTime = 0;
	}

	CString s1;

	for(int nTick = 0; ; nTick++)
	{
		double time = nFirstTime + DistTime
					* nTick / double(nTickCount);

		double sample = fround(time * SampleRate);

		if (sample > pDoc->WaveFileSamples())
		{
			break;
		}

		int x = WorldToWindowX(sample);

		if (x > cr.right)
		{
			break;
		}

		if (0 == nTick % nTickCount)
		{
			// draw bigger tick (6 pixels high) and the number
			pDC->SelectStockObject(WHITE_PEN);
			pDC->MoveTo(x + 1, cr.bottom - 6);
			pDC->LineTo(x + 1, cr.bottom - 12);

			pDC->SelectStockObject(BLACK_PEN);
			pDC->MoveTo(x, cr.bottom - 6);
			pDC->LineTo(x, cr.bottom - 12);

			switch (m_CurrentDisplayMode)
			{
			case ShowSamples:
			{
				s = LtoaCS(SAMPLE_INDEX(sample));
			}
				break;
			case ShowHhMmSs:
			{
				int flags = TimeToHhMmSs_NeedsHhMm;
				if (DistTime < 1.)
				{
					flags = TimeToHhMmSs_NeedsHhMm | TimeToHhMmSs_NeedsMs;
				}
				s = TimeToHhMmSs(unsigned((time + 0.0005) * 1000), flags);
			}
				break;
			case ShowSeconds:
			{
				time += 0.0005;
				int ss = int(time);
				time -= ss;
				int ms = int(time * 1000.);

				s1 = LtoaCS(ss);

				if (DistTime < 1.)
				{
					s.Format(_T("%s%c%03d"), s1, DecimalPoint, ms);
				}
				else
				{
					s.Format(_T("%s%c0"), s1, DecimalPoint);
				}
			}
			}
			pDC->TextOut(x + 2, cr.bottom - 9, s);

		}
		else
		{
			// draw small tick (2 pixels high)
			pDC->SelectStockObject(BLACK_PEN);
			pDC->MoveTo(x, cr.bottom - 6);
			pDC->LineTo(x, cr.bottom - 8);

			pDC->SelectStockObject(WHITE_PEN);
			pDC->MoveTo(x + 1, cr.bottom - 6);
			pDC->LineTo(x + 1, cr.bottom - 8);
		}
	}
}

/////////////////////////////////////////////////////////////////////////////
// CTimeRulerView diagnostics

#ifdef _DEBUG
void CTimeRulerView::AssertValid() const
{
	CScaledScrollView::AssertValid();
}

void CTimeRulerView::Dump(CDumpContext& dc) const
{
	CScaledScrollView::Dump(dc);
}

CWaveSoapFrontDoc* CTimeRulerView::GetDocument() // non-debug version is inline
{
	ASSERT(m_pDocument->IsKindOf(RUNTIME_CLASS(CWaveSoapFrontDoc)));
	return (CWaveSoapFrontDoc*)m_pDocument;
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CTimeRulerView message handlers


void CTimeRulerView::OnViewRulerHhmmss()
{
	m_CurrentDisplayMode = ShowHhMmSs;
	Invalidate();
}

void CTimeRulerView::OnUpdateViewRulerHhmmss(CCmdUI* pCmdUI)
{
	pCmdUI->SetRadio(m_CurrentDisplayMode == ShowHhMmSs);
}

void CTimeRulerView::OnViewRulerSamples()
{
	m_CurrentDisplayMode = ShowSamples;
	Invalidate();
}

void CTimeRulerView::OnUpdateViewRulerSamples(CCmdUI* pCmdUI)
{
	pCmdUI->SetRadio(m_CurrentDisplayMode == ShowSamples);
}

void CTimeRulerView::OnViewRulerSeconds()
{
	m_CurrentDisplayMode = ShowSeconds;
	Invalidate();
}

void CTimeRulerView::OnUpdateViewRulerSeconds(CCmdUI* pCmdUI)
{
	pCmdUI->SetRadio(m_CurrentDisplayMode == ShowSeconds);
}

void CTimeRulerView::OnUpdate( CView* /*pSender*/, LPARAM lHint, CObject* pHint )
{
	CSoundUpdateInfo * pInfo = dynamic_cast<CSoundUpdateInfo *>(pHint);
	if (lHint == CWaveSoapFrontDoc::UpdateSampleRateChanged
		|| (lHint == CWaveSoapFrontDoc::UpdateSoundChanged
			&& pInfo != NULL && pInfo->m_NewLength != -1))
	{
		// either unknown notification or length changed
		Invalidate();
	}
}
