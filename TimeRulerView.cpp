// TimeRulerView.cpp : implementation file
//

#include "stdafx.h"
#include "WaveSoapFront.h"
#include "TimeRulerView.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CTimeRulerView

IMPLEMENT_DYNCREATE(CTimeRulerView, CHorizontalRuler)

CTimeRulerView::CTimeRulerView()
{
}

CTimeRulerView::~CTimeRulerView()
{
}


BEGIN_MESSAGE_MAP(CTimeRulerView, CHorizontalRuler)
	//{{AFX_MSG_MAP(CTimeRulerView)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CTimeRulerView drawing

void CTimeRulerView::OnDraw(CDC* pDC)
{
	CDocument* pDoc = GetDocument();

	// background is erased by gray brush.
	// draw horizontal line with ticks and numbers
	CGdiObject * pOldFont = (CFont *) pDC->SelectStockObject(ANSI_VAR_FONT);
	CPen DarkGrayPen(PS_SOLID, 0, 0x808080);
	CRect cr;
	GetClientRect( & cr);

	// calculate position string length
	int nLength = pDC->GetTextExtent("0000000000", 10).cx;
	// calculate how much samples can be between the numbers
	unsigned nSamples = 1.5 * nLength / GetXScaleDev();
	if (nSamples > INT_MAX / 10)
	{
		nSamples = INT_MAX / 10;
	}
	// find the closest bigger 1,2,5 * 10^x
	int dist, nTickDist;
	for (unsigned k = 1; k <= INT_MAX / 10; k *= 10)
	{
		dist = k;
		nTickDist = k;
		if (dist >= nSamples)
		{
			if (dist > 10)
			{
				nTickDist = dist / 10;
			}
			break;
		}
		dist = k * 2;
		if (dist >= nSamples)
		{
			break;
		}
		dist = k * 5;
		if (dist >= nSamples)
		{
			break;
		}
	}

	int nFirstSample = WindowToWorldX(cr.left);
	// round it
	nFirstSample -= nFirstSample % dist;
	pDC->SetTextAlign(TA_BOTTOM | TA_LEFT);
	pDC->SetTextColor(0x000000);   // black
	pDC->SetBkMode(TRANSPARENT);

	CGdiObject * pOldPen = pDC->SelectStockObject(BLACK_PEN);
	pDC->MoveTo(cr.left, cr.bottom - 5);
	pDC->LineTo(cr.right, cr.bottom - 5);

	pDC->SelectStockObject(WHITE_PEN);
	pDC->MoveTo(cr.left, cr.bottom - 4);
	pDC->LineTo(cr.right, cr.bottom - 4);

	pDC->SelectObject( & DarkGrayPen);
	pDC->MoveTo(cr.left, cr.bottom - 6);
	pDC->LineTo(cr.right, cr.bottom - 6);

	for(;; nFirstSample += nTickDist)
	{
		int x = WorldToWindowX(nFirstSample);
		if (x < cr.left)
		{
			continue;
		}
		if (x > cr.right)
		{
			break;
		}
		pDC->SelectStockObject(BLACK_PEN);
		pDC->MoveTo(x - 1, cr.bottom - 6);

		if (0 == nFirstSample % dist)
		{
			// draw bigger tick (6 pixels high) and the number
			pDC->LineTo(x - 1, cr.bottom - 12);
			char s[16];
			ltoa(nFirstSample, s, 10);

			pDC->TextOut(x + 2, cr.bottom - 9, s, strlen(s));

			pDC->SelectStockObject(WHITE_PEN);
			pDC->MoveTo(x, cr.bottom - 6);
			pDC->LineTo(x, cr.bottom - 12);
		}
		else
		{
			// draw small tick (2 pixels high)
			pDC->LineTo(x - 1, cr.bottom - 8);

			pDC->SelectStockObject(WHITE_PEN);
			pDC->MoveTo(x, cr.bottom - 6);
			pDC->LineTo(x, cr.bottom - 8);
		}
	}

	pDC->SelectObject(pOldPen);
	pDC->SelectObject(pOldFont);
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
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CTimeRulerView message handlers

