// Copyright Alexander Grigoriev, 1997-2002, All Rights Reserved
// FilterDialog.cpp : implementation file
//

#include "stdafx.h"
#include "WaveSoapFront.h"
#include "FilterDialog.h"
#include "OperationDialogs.h"
#include "FileDialogWithHistory.h"
#include "GdiObjectSave.h"
#include "DialogWithSelection.inl"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CFilterDialog dialog


CFilterDialog::CFilterDialog(SAMPLE_INDEX Start,
							SAMPLE_INDEX End,
							SAMPLE_INDEX CaretPosition,
							CHANNEL_MASK Channels,
							CWaveFile & WaveFile,
							int TimeFormat,
							BOOL bLockChannels,
							BOOL	bUndoEnabled,
							CWnd* pParent /*=NULL*/)
	: BaseClass(Start, End, CaretPosition, Channels, WaveFile, TimeFormat,
				IDD, pParent),
	m_wGraph(m_Profile, WaveFile.SampleRate())
{
	m_bLockChannels = bLockChannels;
	m_bUndo = bUndoEnabled;
	//{{AFX_DATA_INIT(CFilterDialog)
	//}}AFX_DATA_INIT

	static ResizableDlgItem const ResizeItems[] =
	{
		{IDC_STATIC1, MoveDown},
		{IDC_EDIT_BAND_GAIN, MoveDown},
		{IDC_STATIC2, MoveDown},
		{IDC_EDIT_FREQUENCY, MoveDown},
		{IDC_STATIC4, MoveDown},
		{IDC_CHECK_LOWPASS, MoveDown},
		{IDC_CHECK_STOPBAND, MoveDown},
		{IDC_CHECK_HIGHPASS, MoveDown},
		{IDC_CHECK_ZERO_PHASE, MoveDown},
		{IDC_CHECK_UNDO, MoveDown},
		{IDC_STATIC_SELECTION, MoveDown},
		{IDC_BUTTON_SELECTION, MoveDown},

		{IDC_BUTTON_RESET_BANDS, MoveRight | MoveDown},
		{IDC_BUTTON_SAVE_AS, MoveRight | MoveDown},
		{IDC_BUTTON_LOAD, MoveRight | MoveDown},
		{IDOK, MoveRight | MoveDown},
		{IDCANCEL, MoveRight | MoveDown},
		{IDHELP, MoveRight | MoveDown},

		{AFX_IDW_PANE_FIRST, ExpandRight | ExpandDown},
	};

	m_pResizeItems = ResizeItems;
	m_pResizeItemsCount = countof(ResizeItems);

	m_Profile.AddItem(_T("Settings"), _T("FilterDlgWidth"), m_DlgWidth, 0, 0, 4096);
	m_Profile.AddItem(_T("Settings"), _T("FilterDlgHeight"), m_DlgHeight, 0, 0, 4096);


	m_EditGain.SetPrecision(2);
	m_EditFrequency.SetPrecision(1);
}


void CFilterDialog::DoDataExchange(CDataExchange* pDX)
{
	BaseClass::DoDataExchange(pDX);

	DDX_Control(pDX, IDC_EDIT_BAND_GAIN, m_EditGain);
	DDX_Control(pDX, IDC_EDIT_FREQUENCY, m_EditFrequency);
	//{{AFX_DATA_MAP(CFilterDialog)
	DDX_Check(pDX, IDC_CHECK_UNDO, m_bUndo);
	//}}AFX_DATA_MAP
	DDX_Check(pDX, IDC_CHECK_ZERO_PHASE, m_wGraph.m_bZeroPhase);
	DDX_Check(pDX, IDC_CHECK_LOWPASS, m_wGraph.m_bLowPass);
	DDX_Check(pDX, IDC_CHECK_HIGHPASS, m_wGraph.m_bHighPass);
	DDX_Check(pDX, IDC_CHECK_STOPBAND, m_wGraph.m_bNotchFilter);

	if (pDX->m_bSaveAndValidate)
	{
		m_Profile.UnloadAll();
	}
}


BEGIN_MESSAGE_MAP(CFilterDialog, BaseClass)
	//{{AFX_MSG_MAP(CFilterDialog)
	ON_BN_CLICKED(IDC_BUTTON_LOAD, OnButtonLoad)
	ON_BN_CLICKED(IDC_BUTTON_RESET_BANDS, OnButtonResetBands)
	ON_BN_CLICKED(IDC_BUTTON_SAVE_AS, OnButtonSaveAs)
	ON_BN_CLICKED(IDC_CHECK_ZERO_PHASE, OnCheckZeroPhase)
	ON_BN_CLICKED(IDC_CHECK_LOWPASS, OnCheckLowpass)
	ON_BN_CLICKED(IDC_CHECK_HIGHPASS, OnCheckHighpass)
	ON_EN_KILLFOCUS(IDC_EDIT_FREQUENCY, OnKillfocusEditFrequency)
	ON_BN_CLICKED(IDC_CHECK_STOPBAND, OnCheckStopband)
	//}}AFX_MSG_MAP
	ON_NOTIFY(NM_RETURN, AFX_IDW_PANE_FIRST, OnNotifyGraph)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CFilterDialog message handlers

void CFilterDialog::OnCheckLowpass()
{
	if (IsDlgButtonChecked(IDC_CHECK_LOWPASS))
	{
		m_wGraph.m_bLowPass = TRUE;
	}
	else
	{
		m_wGraph.m_bLowPass = FALSE;
		if (m_wGraph.m_PointWithFocus >= LpfPassbandIndex)
		{
			m_wGraph.m_PointWithFocus = HpfPassbandIndex;
			if ( ! m_wGraph.m_bHighPass)
			{
				m_wGraph.m_PointWithFocus = NotchBeginIndex;
			}
		}
	}
	m_wGraph.RebuildFilters();
	m_wGraph.Invalidate();
}

void CFilterDialog::OnCheckHighpass()
{
	if (IsDlgButtonChecked(IDC_CHECK_HIGHPASS))
	{
		m_wGraph.m_bHighPass = TRUE;
	}
	else
	{
		m_wGraph.m_bHighPass = FALSE;
		if (m_wGraph.m_PointWithFocus <= HpfPassbandIndex)
		{
			m_wGraph.m_PointWithFocus = LpfPassbandIndex;
			if ( ! m_wGraph.m_bLowPass)
			{
				m_wGraph.m_PointWithFocus = NotchBeginIndex;
			}
		}
	}
	m_wGraph.RebuildFilters();
	m_wGraph.Invalidate();
}

void CFilterDialog::OnCheckStopband()
{
	if (IsDlgButtonChecked(IDC_CHECK_STOPBAND))
	{
		m_wGraph.m_bNotchFilter = TRUE;
	}
	else
	{
		m_wGraph.m_bNotchFilter = FALSE;
		if (m_wGraph.m_PointWithFocus == NotchBeginIndex
			|| m_wGraph.m_PointWithFocus == NotchZeroIndex)
		{
			m_wGraph.m_PointWithFocus = LpfPassbandIndex;
			if ( ! m_wGraph.m_bLowPass)
			{
				m_wGraph.m_PointWithFocus = HpfPassbandIndex;
			}
		}
	}
	m_wGraph.RebuildFilters();
	m_wGraph.Invalidate();
}

BOOL CFilterDialog::OnInitDialog()
{
	CRect r;
	CWnd * pTemplateWnd = GetDlgItem(IDC_STATIC_RESPONSE_TEMPLATE);
	pTemplateWnd->GetWindowRect( & r);
	ScreenToClient( & r);
	m_wGraph.Create(NULL, _T(""), WS_CHILD | WS_VISIBLE
					| WS_CLIPSIBLINGS | WS_CLIPCHILDREN | WS_TABSTOP,
					r, this, AFX_IDW_PANE_FIRST);
	m_wGraph.SetWindowPos(pTemplateWnd, 0, 0, 0, 0,
						SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOSIZE);
	pTemplateWnd->DestroyWindow();

	BaseClass::OnInitDialog();

	m_EditGain.SetData(m_wGraph.GetCurrentPointGainDb());
	m_EditFrequency.SetData(m_wGraph.GetCurrentPointFrequencyHz());
	// init MINMAXINFO
	m_wGraph.RebuildFilters();

	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

CFilterGraphWnd::CFilterGraphWnd()
{
	m_bMouseCaptured = false;
	m_bButtonPressed = false;
	m_bGotFocus = false;
	m_PointWithFocus = 0;

	m_Gain[HpfPassbandIndex] = 0.9;
	m_Gain[HpfStopbandIndex] = 0.001;
	m_Frequencies[HpfPassbandIndex] = M_PI / 250.;
	m_Frequencies[HpfStopbandIndex] = M_PI / 500.;

	m_Gain[NotchBeginIndex] = 0.9;
	m_Gain[NotchZeroIndex] = 0.0001;
	m_Frequencies[NotchBeginIndex] = M_PI * 0.48;
	m_Frequencies[NotchZeroIndex] = M_PI * 0.52;

	m_Gain[LpfPassbandIndex] = 0.9;
	m_Gain[LpfStopbandIndex] = 0.001;
	m_Frequencies[LpfPassbandIndex] = M_PI * 0.8;
	m_Frequencies[LpfStopbandIndex] = M_PI * 0.9;
}

CFilterGraphWnd::CFilterGraphWnd(CApplicationProfile & Profile, int SampleRate)
	: m_SamplingRate(SampleRate)
	, m_bMouseCaptured(false)
	, m_bButtonPressed(false)
	, m_bGotFocus(false)
{
	m_PointWithFocus = 0;

	Profile.AddBoolItem(_T("Filter"), _T("ZeroPhase"), m_bZeroPhase, FALSE);
	Profile.AddBoolItem(_T("Filter"), _T("LowPassFilter"), m_bLowPass, TRUE);
	Profile.AddBoolItem(_T("Filter"), _T("HighPassFilter"), m_bHighPass, TRUE);
	Profile.AddBoolItem(_T("Filter"), _T("NotchFilter"), m_bNotchFilter, FALSE);

	m_Gain[HpfPassbandIndex] = 0.9;
	m_Gain[HpfStopbandIndex] = 0.001;
	m_Gain[NotchBeginIndex] = 0.9;
	m_Gain[NotchZeroIndex] = 0.0001;
	m_Gain[LpfPassbandIndex] = 0.9;
	m_Gain[LpfStopbandIndex] = 0.001;
	m_Frequencies[HpfPassbandIndex] = M_PI / 250.;
	m_Frequencies[HpfStopbandIndex] = M_PI / 500.;

	m_Frequencies[NotchBeginIndex] = M_PI * 0.48;
	m_Frequencies[NotchZeroIndex] = M_PI * 0.52;

	m_Frequencies[LpfPassbandIndex] = M_PI * 0.8;
	m_Frequencies[LpfStopbandIndex] = M_PI * 0.9;

	for (int n = 0; n < countof (m_Frequencies); n++)
	{
		CString s;

		s.Format(_T("Gain%d"), n + 1);
		Profile.AddItem(_T("Filter"), s, m_Gain[n], m_Gain[n], 0.00003, 1.);

		s.Format(_T("Frequency%d"), n + 1);
		Profile.AddItem(_T("Filter"), s, m_Frequencies[n], m_Frequencies[n], 0.00314, 3.14);
	}
	// check for correct frequencies and gain
	if (m_Gain[HpfPassbandIndex] <= m_Gain[HpfStopbandIndex])
	{
		m_Gain[HpfPassbandIndex] = 0.9;
		m_Gain[HpfStopbandIndex] = 0.001;
	}
	if (m_Frequencies[HpfPassbandIndex] <= m_Frequencies[HpfStopbandIndex])
	{
		m_Frequencies[HpfPassbandIndex] = M_PI / 250.;
		m_Frequencies[HpfStopbandIndex] = M_PI / 500.;
	}
	// check for correct frequencies and gain
	if (m_Gain[LpfPassbandIndex] <= m_Gain[LpfStopbandIndex])
	{
		m_Gain[LpfPassbandIndex] = 0.9;
		m_Gain[LpfStopbandIndex] = 0.001;
	}
	if (m_Frequencies[LpfPassbandIndex] >= m_Frequencies[LpfStopbandIndex])
	{
		m_Frequencies[LpfPassbandIndex] = M_PI * 0.8;
		m_Frequencies[LpfStopbandIndex] = M_PI * 0.9;
	}
	// check for correct frequencies and gain
	if (m_Gain[NotchBeginIndex] <= m_Gain[NotchZeroIndex])
	{
		m_Gain[NotchBeginIndex] = 0.9;
		m_Gain[NotchZeroIndex] = 0.001;
	}
	if (m_Frequencies[NotchBeginIndex] >= m_Frequencies[NotchZeroIndex])
	{
		m_Frequencies[NotchBeginIndex] = M_PI * 0.5;
		m_Frequencies[NotchZeroIndex] = M_PI * 0.6;
	}

	if ( ! m_bHighPass)
	{
		m_PointWithFocus = NotchBeginIndex;
		if ( ! m_bNotchFilter)
		{
			m_PointWithFocus = LpfPassbandIndex;
			if ( ! m_bLowPass)
			{
				m_PointWithFocus = HpfPassbandIndex;
			}
		}
	}
}

CFilterGraphWnd::~CFilterGraphWnd()
{
}

Filter::Filter()
	: m_bLowPass(FALSE),
	m_nLpfOrder(0),
	m_bHighPass(FALSE),
	m_nHpfOrder(0),
	m_bNotchFilter(FALSE),
	m_nNotchOrder(0),
	m_bZeroPhase(FALSE)
{
	ResetBands();
}

void Filter::ResetBands()
{
	int i;
	for (i = 0; i < MaxFilterFrequencies; i++)
	{
		m_Frequencies[i] = 1.;
		m_Gain[i] = 1.;
	}

	for (i = 0; i < MaxFilterOrder; i++)
	{
		m_LpfCoeffs[i][0] = 0.;
		m_LpfCoeffs[i][1] = 0.;
		m_LpfCoeffs[i][2] = 0.;
		m_LpfCoeffs[i][3] = 1.;
		m_LpfCoeffs[i][4] = 0.;
		m_LpfCoeffs[i][5] = 0.;

		m_HpfCoeffs[i][0] = 0.;
		m_HpfCoeffs[i][1] = 0.;
		m_HpfCoeffs[i][2] = 0.;
		m_HpfCoeffs[i][3] = 1.;
		m_HpfCoeffs[i][4] = 0.;
		m_HpfCoeffs[i][5] = 0.;

		m_NotchCoeffs[i][0] = 0.;
		m_NotchCoeffs[i][1] = 0.;
		m_NotchCoeffs[i][2] = 0.;
		m_NotchCoeffs[i][3] = 1.;
		m_NotchCoeffs[i][4] = 0.;
		m_NotchCoeffs[i][5] = 0.;
	}
}

typedef void (CWnd::*NcPaintFunc)(UINT);
BEGIN_MESSAGE_MAP(CFilterGraphWnd, CWnd)
	//{{AFX_MSG_MAP(CFilterGraphWnd)
	ON_WM_PAINT()
	ON_WM_MOUSEMOVE()
	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONUP()
	ON_WM_ERASEBKGND()
	ON_WM_NCCALCSIZE()
	ON_WM_CAPTURECHANGED()
	ON_WM_MOUSEACTIVATE()
	ON_WM_KILLFOCUS()
	ON_WM_SETFOCUS()
	ON_WM_SETCURSOR()
	ON_WM_KEYDOWN()
	ON_WM_GETDLGCODE()
	ON_WM_TIMER()
	ON_WM_LBUTTONDBLCLK()
	//}}AFX_MSG_MAP
{ WM_NCPAINT, 0, 0, 0, AfxSig_vw,
	(AFX_PMSG)(AFX_PMSGW)
	static_cast< NcPaintFunc >(OnNcPaint)
},
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CFilterGraphWnd message handlers

void CFilterGraphWnd::OnPaint()
{
	CPaintDC dc(this); // device context for painting

	CRect cr;
	CRect ur;

	GetClientRect( & cr);
	dc.GetClipBox( & ur);

	CPushDcMapMode mode(dc, MM_TEXT);
	CGdiObjectSave OldPenDc(dc, dc.SelectStockObject(BLACK_PEN));

	ur.left -= 2;
	if (ur.left < 0)
	{
		ur.left = 0;
	}
	ur.right ++;
	if (ur.right > cr.right)
	{
		ur.right = cr.right;
	}
	for (int x = ur.left; x < ur.right; x++)
	{
		double f;
		f = M_PI * pow(1000., (x + 1. - cr.Width()) / cr.Width());
		double gain = abs(CalculateResponse(f));
		if (gain < 0.0000001)
		{
			gain = 0.0000001;
		}
		int y = GainToPosY(gain, cr.Height());
		if (x == ur.left)
		{
			dc.MoveTo(x, y);
		}
		else
		{
			dc.LineTo(x, y);
		}
	}
	int dx = GetSystemMetrics(SM_CXSIZEFRAME);
	int dy = GetSystemMetrics(SM_CYSIZEFRAME);
	// ATI drivers can't draw small circles, use intermediate memory bitmap
	CBitmap bmp;
	// bitmap width and height
	int w = dx * 2 + 4;
	int h = dy * 2 + 4;
	bmp.CreateBitmap(2 * w, h, 1, 1, NULL);

	CDC cdc;
	cdc.CreateCompatibleDC( & dc);

	CGdiObjectSaveT<CBitmap> OldBitmap(cdc, cdc.SelectObject( & bmp));

	CGdiObjectSave OldBrush(cdc, cdc.SelectStockObject(WHITE_BRUSH));
	CGdiObjectSave OldCdcPen(cdc, cdc.SelectStockObject(WHITE_PEN));

	cdc.Rectangle(0, 0, 2 * w, h);
	cdc.SelectStockObject(BLACK_PEN);

	cdc.Ellipse(0, 0, 2 * dx, 2 * dy);

	cdc.SelectStockObject(BLACK_BRUSH);
	cdc.Ellipse(w, 0, w + 2 * dx, 2 * dy);

	CGdiObjectSave OldBrushDc(dc, dc.SelectStockObject(NULL_BRUSH));

	dc.SetBkColor(0xFFFFFF);
	dc.SetTextColor(0x000000);

	for (int i = 0; i < MaxFilterFrequencies; i++)
	{
		// draw circles around the reference points
		int x = int((1. + log10(m_Frequencies[i] / M_PI) / 3.) * cr.Width() - 1);
		int y = GainToPosY(m_Gain[i], cr.Height());

		int SrcOffset = 0;
		if (m_DotCaretIsOn && i == m_PointWithFocus)
		{
			SrcOffset = w;
		}
		if (i <= HpfPassbandIndex)
		{
			if ( ! m_bHighPass)
			{
				continue;
			}
		}
		else if (i >= LpfPassbandIndex)
		{
			if ( ! m_bLowPass)
			{
				continue;
			}
		}
		else
		{
			if ( ! m_bNotchFilter)
			{
				continue;
			}
		}
		dc.BitBlt(x - dx, y - dy, w, h, & cdc, SrcOffset, 0, SRCAND);
	}
}

void CFilterGraphWnd::OnMouseMove(UINT nFlags, CPoint point)
{
	CWnd::OnMouseMove(nFlags, point);
	// if a left button pressed, track mouse movement
	if (m_bButtonPressed)
	{
		if ( ! m_bMouseCaptured)
		{
			SetCapture();
			m_bMouseCaptured = true;
		}
		CRect cr;
		GetClientRect( & cr);
		if (point.y < cr.top)
		{
			point.y = cr.top;
		}
		if (point.y >= cr.bottom)
		{
			point.y = cr.bottom - 1;
		}
		if (point.x < 0)
		{
			point.x = 0;
		}
		if (point.x >= cr.right)
		{
			point.x = cr.right - 1;
		}

		SetCurrentPointGainDb(PosYToGainDb(point.y, cr.Height()));
		if (GetFilterPointPixel(m_PointWithFocus) != point.x)
		{
			SetFilterPointPixel(m_PointWithFocus, point.x, nFlags & MK_CONTROL);
			RebuildFilters();
			Invalidate();
		}
		NotifyParentDlg();
	}
}

void CFilterGraphWnd::NotifyParentDlg()
{
	NMHDR nmhdr;
	nmhdr.hwndFrom = m_hWnd;
	nmhdr.code = NM_RETURN;
	nmhdr.idFrom = GetDlgCtrlID();
	GetParent()->SendMessage(WM_NOTIFY, nmhdr.idFrom, (LPARAM) & nmhdr);
}

void CFilterGraphWnd::SetFocusPoint(int nPoint)
{
	if (nPoint != m_PointWithFocus)
	{
		DrawDotCaret(false);

		m_PointWithFocus = nPoint;
	}
	DrawDotCaret(true);
}

void CFilterGraphWnd::OnLButtonDown(UINT nFlags, CPoint point)
{

	CWnd::OnLButtonDown(nFlags, point);
	int nHit = GetHitCode(point);
	if (nHit < 0)
	{
		return;
	}
	m_bButtonPressed = true;
	DrawDotCaret(true);
	SetFocusPoint(nHit);
	NotifyParentDlg();
}

void CFilterGraphWnd::OnLButtonUp(UINT nFlags, CPoint point)
{
	CWnd::OnLButtonUp(nFlags, point);
	if (m_bMouseCaptured)
	{
		ReleaseCapture();
		m_bMouseCaptured = false;
	}
	if (m_bButtonPressed)
	{
		m_bButtonPressed = false;
		DrawDotCaret(false);
	}
}

BOOL CFilterGraphWnd::PreCreateWindow(CREATESTRUCT& cs)
{
	cs.lpszClass = AfxRegisterWndClass(CS_VREDRAW | CS_HREDRAW | CS_DBLCLKS, NULL,
										NULL, NULL);

	return CWnd::PreCreateWindow(cs);
}

// frequency is in radians
complex<float> Filter::CalculateResponse(double Frequency)
{
	complex<double> Numerator;
	complex<double> Denominator;
	complex<double> Result(1., 0.);

	complex<double> z(cos(Frequency), -sin(Frequency));
	complex<double> z2(cos(Frequency * 2), -sin(Frequency * 2));

	if (m_bLowPass)
	{
		complex<double> LpfResult(0., 0.);
		for (int i = 0; i < m_nLpfOrder; i++)
		{
			LpfResult += (m_LpfCoeffs[i][0] + z * m_LpfCoeffs[i][1]
							+ z2 * m_LpfCoeffs[i][2])
						/ (m_LpfCoeffs[i][3] + z * m_LpfCoeffs[i][4]
							+ z2 * m_LpfCoeffs[i][5]);
		}
		Result *= LpfResult;
	}
	if (m_bHighPass)
	{
		complex<double> HpfResult(0., 0.);
		for (int i = 0; i < m_nHpfOrder; i++)
		{
			HpfResult += (m_HpfCoeffs[i][0] + z * m_HpfCoeffs[i][1]
							+ z2 * m_HpfCoeffs[i][2])
						/ (m_HpfCoeffs[i][3] + z * m_HpfCoeffs[i][4]
							+ z2 * m_HpfCoeffs[i][5]);
		}
		Result *= HpfResult;
	}
	if (m_bNotchFilter)
	{
		complex<double> NotchResult(1., 0.);
		for (int i = 0; i < m_nNotchOrder; i++)
		{
			NotchResult *= (m_NotchCoeffs[i][0] + z * m_NotchCoeffs[i][1]
								+ z2 * m_NotchCoeffs[i][2])
							/ (m_NotchCoeffs[i][3] + z * m_NotchCoeffs[i][4]
								+ z2 * m_NotchCoeffs[i][5]);
		}
		Result *= NotchResult;
	}

	if (m_bZeroPhase)
	{
		// filter is applied twice
		Result *= conj(Result);
	}
	return complex<float>(Result);
}

void CFilterGraphWnd::SetPointGainDb(int nPoint, double GainDb)
{
	if (GainDb > 0.)
	{
		GainDb = 0.;
	}
	switch (nPoint)
	{
	case HpfStopbandIndex:
		if (GainDb > -10.)
		{
			GainDb = -10.;
		}
		break;
	case HpfPassbandIndex:
		if (GainDb < -6.)
		{
			GainDb = -6.;
		}
		break;
	case NotchBeginIndex:
		GainDb = -3.;
		break;
	case NotchZeroIndex:
		GainDb = -85.;
		break;
	case LpfPassbandIndex:
		if (GainDb < -6.)
		{
			GainDb = -6.;
		}
		break;
	case LpfStopbandIndex:
		if (GainDb > -10.)
		{
			GainDb = -10.;
		}
		break;
	}
	double Gain = pow(10., GainDb / 20.);
	if (m_Gain[nPoint] == Gain)
	{
		return;
	}
	m_Gain[nPoint] = Gain;
	RebuildFilters();
	Invalidate();
}

void CFilterGraphWnd::SetPointFrequency(int nPoint, double Frequency)
{
	if (m_Frequencies[nPoint] == Frequency)
	{
		return;
	}
	m_Frequencies[nPoint] = Frequency;
	RebuildFilters();
	Invalidate();
}

BOOL CFilterGraphWnd::OnEraseBkgnd(CDC* pDC)
{
	// white brush
	CGdiObjectSave OldBrush(pDC, pDC->SelectStockObject(WHITE_BRUSH));
	CRect cr;
	GetClientRect( & cr);
	if (0) TRACE("CFilterGraphWnd::OnEraseBkgnd, cr=%d, %d, %d, %d\n",
				cr.left, cr.right, cr.top, cr.bottom);
	int ClientHeight = cr.Height();
	pDC->PatBlt(cr.left, cr.top, cr.Width(), ClientHeight, PATCOPY);

	// have to use PatBlt to draw alternate lines
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
		pDC->SelectObject( & GrayBrush);

		// draw zero level line at 0 dB
		pDC->PatBlt(cr.left, GainDbToPosY(0., ClientHeight), cr.Width(), 1, PATINVERT);
		// draw frequency lines
		for (int i = 0; i < 20; i++)
		{
			int x = cr.Width() * (i * 2 + 1) / 40;
			pDC->PatBlt(x, cr.top, 1, cr.bottom - cr.top, PATINVERT);
		}
	}
	catch (CResourceException * e)
	{
		TRACE("CResourceException\n");
		e->Delete();
	}
	CPen DotPen(PS_DOT, 0, COLORREF(0));

	CGdiObjectSaveT<CPen> OldPen(pDC, pDC->SelectObject( & DotPen));

	for (double d = -20.; d >= -80.; d -= 20.)
	{
		int y = GainDbToPosY(d, ClientHeight);
		pDC->MoveTo(cr.left, y);
		pDC->LineTo(cr.right, y);
	}

	return TRUE;
}

void CFilterGraphWnd::OnNcCalcSize(BOOL bCalcValidRects, NCCALCSIZE_PARAMS * lpncsp)
{
	CWindowDC wDC(GetDesktopWindow());
	CGdiObjectSave Old(wDC, wDC.SelectStockObject(ANSI_VAR_FONT));

	int ncWidth = 3 + wDC.GetTextExtent(_T(" -20 dB"), 7).cx;
	lpncsp->rgrc[0].left += ncWidth;

	int ncHeight = 2 + GetSystemMetrics(SM_CYMENUSIZE);
	lpncsp->rgrc[0].bottom -= ncHeight /* + GetSystemMetrics(SM_CXSIZEFRAME) */;

	lpncsp->rgrc[0].right -= GetSystemMetrics(SM_CXSIZEFRAME);
	lpncsp->rgrc[0].top += GetSystemMetrics(SM_CYSIZEFRAME);
}

void CFilterGraphWnd::OnNcPaint(UINT wParam)
{
	if (0) TRACE("CFilterGraphWnd::OnNcPaint, hrgn=%X\n", wParam);
	// copy region, because it will be deleted
	CRect wr;
	GetWindowRect( & wr);
	try
	{
		CWindowDC wDC(this);

		if (wParam != 1)
		{
			CRgn rgn;
			rgn.CreateRectRgn(0, 0, 1, 1);
			rgn.CopyRgn(CRgn::FromHandle((HRGN)wParam));
			rgn.OffsetRgn( -wr.left, -wr.top);
			wDC.SelectClipRgn( & rgn, RGN_AND);
		}

		NCCALCSIZE_PARAMS ncp;
		wr.right = wr.Width();
		wr.left = 0;
		wr.bottom = wr.Height();
		wr.top = 0;
		ncp.rgrc[0] = wr;
		OnNcCalcSize(FALSE, & ncp);
		//SendMessage(WM_NCCALCSIZE, FALSE, (LPARAM) & ncp);
		wDC.ExcludeClipRect( & ncp.rgrc[0]);

		// Paint into this DC
		CBrush BkBrush;
		CWnd * pParentDlg = GetParent();
		BkBrush.Attach(GetSysColorBrush(COLOR_BTNFACE));
		//BkBrush.Attach((HBRUSH) pParentDlg->SendMessage(WM_CTLCOLORDLG, (WPARAM)(pParentDlg->m_hWnd), LPARAM(wDC.m_hDC)));

		CGdiObjectSave OldPen(wDC, wDC.SelectStockObject(BLACK_PEN));
		CGdiObjectSaveT<CBrush> OldBrush(wDC, wDC.SelectObject( & BkBrush));
		// fill the NC area
		wDC.Rectangle( & wr);
		// Draw rectangle
		wDC.SelectStockObject(HOLLOW_BRUSH);
		BkBrush.Detach();

		wDC.Rectangle(ncp.rgrc[0].left - 1, ncp.rgrc[0].top - 1,
					ncp.rgrc[0].right + 1, ncp.rgrc[0].bottom + 1);

		CGdiObjectSave OldFont(wDC, wDC.SelectStockObject(ANSI_VAR_FONT));
		// draw frequencies
		wDC.SetTextAlign(TA_TOP | TA_CENTER);
		wDC.SetTextColor(0x000000);   // black
		wDC.SetBkMode(TRANSPARENT);

		int PrevX = ncp.rgrc[0].left;

		for (int i = 0; i < 20; i ++)
		{
			int x = ncp.rgrc[0].left +
					(ncp.rgrc[0].right - ncp.rgrc[0].left) * (i * 2 + 1) / 40;
			CString s;
			double f;
			f =  0.5 * m_SamplingRate * pow(1000.,
											(x + 1. - ncp.rgrc[0].right) /
											(ncp.rgrc[0].right - ncp.rgrc[0].left));
			int TextWidth;
			if (f >= 1000.)
			{
				// 12.1k
				// 8.9k
				s.Format(_T("%.1fk"), f / 1000.);
				if (f >= 10000.)
				{
					TextWidth = wDC.GetTextExtent(_T("20.0k "), 6).cx;
				}
				else
				{
					TextWidth = wDC.GetTextExtent(_T("0.0k "), 5).cx;
				}
			}
			else if (f >= 100.)
			{
				// 550 (round to 10)
				s.Format(_T("%d"), 10 * ((int(f) + 5) / 10));
				TextWidth = wDC.GetTextExtent(_T("999 "), 4).cx;
			}
			else
			{
				// 55
				s.Format(_T("%d"), int(f));
				TextWidth = wDC.GetTextExtent(_T("99 "), 3).cx;
			}
			if (x - TextWidth / 2 >= PrevX)
			{
				wDC.TextOut(x, ncp.rgrc[0].bottom - 1, s);
				PrevX = x + TextWidth / 2;
			}
		}

		wDC.SetTextAlign(TA_BOTTOM | TA_RIGHT);
		int TextOrigin = wDC.GetTextExtent(_T("9"), 1).cy / 2 + ncp.rgrc[0].top;
		int ClientHeight = ncp.rgrc[0].bottom - ncp.rgrc[0].top;

		// from 0 to -80 dB, with 20 dB step
		for (int d = 0; d >= -80; d -= 20)
		{
			CString s;
			s.Format(_T("%d dB"), d);
			wDC.TextOut(ncp.rgrc[0].left - 1,
						TextOrigin + GainDbToPosY(d, ClientHeight),
						s);
		}
	}
	catch (CResourceException * e)
	{
		e->Delete();
	}
	CWnd::OnNcPaint();
}

void CFilterGraphWnd::OnCaptureChanged(CWnd *pWnd)
{
	m_bMouseCaptured = false;
	m_bButtonPressed = false;
	CWnd::OnCaptureChanged(pWnd);
}

int CFilterGraphWnd::OnMouseActivate(CWnd* pDesktopWnd, UINT nHitTest, UINT message)
{
	int nResult = CWnd::OnMouseActivate(pDesktopWnd, nHitTest, message);
	if (nResult == MA_NOACTIVATE || nResult == MA_NOACTIVATEANDEAT)
		return nResult;   // parent does not want to activate
	// grab focus
	SetFocus();
	return MA_NOACTIVATE;
}

void CFilterGraphWnd::OnKillFocus(CWnd* pNewWnd)
{
	CWnd::OnKillFocus(pNewWnd);

	if (m_bGotFocus)
	{
		m_bGotFocus = false;
		DrawDotCaret(true);
	}
}

void CFilterGraphWnd::OnSetFocus(CWnd* pOldWnd)
{
	CWnd::OnSetFocus(pOldWnd);

	m_bGotFocus = true;
	DrawDotCaret(true);

}

BOOL CFilterGraphWnd::OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message)
{
	POINT p;
	GetCursorPos( & p);
	ScreenToClient( & p);
	if (HTCLIENT == nHitTest
		&& GetHitCode(p) >= 0)
	{
		SetCursor(AfxGetApp()->LoadStandardCursor(IDC_SIZEALL));
		return TRUE;
	}
	SetCursor(AfxGetApp()->LoadStandardCursor(IDC_ARROW));
	return TRUE;
}

void CFilterDialog::OnButtonResetBands()
{
	m_wGraph.ResetBands();
	//m_Gain.SetData(m_wGraph.GetCurrentBandGainDb());
}

void CFilterDialog::OnButtonLoad()
{
	CString FileName;
	CString Filter;
	Filter.LoadString(IDS_FILTER_FILE_FILTER);

	CString Title;
	Title.LoadString(IDS_FILTER_LOAD_TITLE);

	CFileDialogWithHistory dlg(TRUE,
								_T("Fltr"), NULL,
								OFN_HIDEREADONLY
								| OFN_EXPLORER | OFN_NONETWORKBUTTON | OFN_FILEMUSTEXIST,
								Filter);
	dlg.m_ofn.lpstrTitle = Title;

	if (IDOK != dlg.DoModal())
	{
		return;
	}
	FileName = dlg.GetPathName();
	m_Profile.ImportSection(_T("Filter"), FileName);
	//m_wGraph.SetNumberOfBands(m_nBands);
	//m_Gain.SetData(m_wGraph.GetCurrentBandGainDb());
}

void CFilterDialog::OnButtonSaveAs()
{
	CString FileName;
	CString Filter;
	Filter.LoadString(IDS_FILTER_FILE_FILTER);

	CString Title;
	Title.LoadString(IDS_FILTER_SAVE_TITLE);

	CFileDialogWithHistory dlg(FALSE,
								_T("Fltr"), NULL,
								OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT
								| OFN_EXPLORER | OFN_NONETWORKBUTTON | OFN_PATHMUSTEXIST,
								Filter);
	dlg.m_ofn.lpstrTitle = Title;

	if (IDOK != dlg.DoModal())
	{
		return;
	}
	FileName = dlg.GetPathName();
	m_Profile.ExportSection(_T("Filter"), FileName);
}

int CFilterGraphWnd::GetFilterPointPixel(int FilterPoint)
{
	CRect cr;
	GetClientRect( & cr);
	if (FilterPoint == LeftmostPoint)
	{
		return cr.left;
	}
	else if (FilterPoint == RightmostPoint)
	{
		return cr.right;
	}
	return int((1.00001 + log10(m_Frequencies[FilterPoint] / M_PI) / 3.) * cr.Width() - 1);
}

void CFilterGraphWnd::SetFilterPointPixel(int FilterPoint, int PointPixel, BOOL MoveBothSides)
{
	CRect cr;
	GetClientRect( & cr);
	if (MoveBothSides)
	{
		int BeginPoint, EndPoint;
		switch (FilterPoint)
		{
		case HpfStopbandIndex:
		case HpfPassbandIndex:
			BeginPoint = HpfStopbandIndex;
			EndPoint = HpfPassbandIndex;
			break;
		case NotchBeginIndex:
		case NotchZeroIndex:
			BeginPoint = NotchBeginIndex;
			EndPoint = NotchZeroIndex;
			break;
		case LpfPassbandIndex:
		case LpfStopbandIndex:
			BeginPoint = LpfPassbandIndex;
			EndPoint = LpfStopbandIndex;
			break;
		default:
			return;
		}

		int OffsetX = PointPixel - GetFilterPointPixel(FilterPoint);
		int BeginX = OffsetX + GetFilterPointPixel(BeginPoint);
		int EndX = OffsetX + GetFilterPointPixel(EndPoint);
		if (BeginX <= cr.left
			|| EndX >= cr.right)
		{
			return;
		}

		m_Frequencies[BeginPoint] = M_PI * pow(10., ((BeginX + 1.) / cr.Width() - 1.) * 3);
		m_Frequencies[EndPoint] = M_PI * pow(10., ((EndX + 1.) / cr.Width() - 1.) * 3);
	}
	else
	{
		int BeginPoint, EndPoint;
		switch (FilterPoint)
		{
		case HpfStopbandIndex:
			BeginPoint = LeftmostPoint;
			EndPoint = HpfPassbandIndex;
			break;
		case HpfPassbandIndex:
			BeginPoint = HpfStopbandIndex;
			EndPoint = RightmostPoint;
			break;
		case NotchBeginIndex:
			BeginPoint = LeftmostPoint;
			EndPoint = NotchZeroIndex;
			break;
		case NotchZeroIndex:
			BeginPoint = NotchBeginIndex;
			EndPoint = RightmostPoint;
			break;
		case LpfPassbandIndex:
			BeginPoint = LeftmostPoint;
			EndPoint = LpfStopbandIndex;
			break;
		case LpfStopbandIndex:
			BeginPoint = LpfPassbandIndex;
			EndPoint = RightmostPoint;
			break;
		default:
			return;
		}
		int BeginX = GetFilterPointPixel(BeginPoint);
		int EndX = GetFilterPointPixel(EndPoint);
		if (PointPixel <= BeginX
			|| PointPixel >= EndX)
		{
			return;
		}
		m_Frequencies[FilterPoint] = M_PI * pow(10., ((PointPixel + 1.) / cr.Width() - 1.) * 3);
	}
}

void CFilterGraphWnd::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	CWnd::OnKeyDown(nChar, nRepCnt, nFlags);
	if (! m_bGotFocus)
	{
		return;
	}
	double GainDb;
	BOOL ShiftPressed = (0 != (0x8000 & GetKeyState(VK_SHIFT)));
	BOOL CtrlPressed = (0 != (0x8000 & GetKeyState(VK_CONTROL)));
	switch(nChar)
	{
	case VK_UP:
		// 1 dB up
		GainDb = 1 + floor(20. * log10(m_Gain[m_PointWithFocus] * 1.00001));
		SetCurrentPointGainDb(GainDb);
		DrawDotCaret(true);
		NotifyParentDlg();
		return;
		break;
	case VK_DOWN:
		// 1 dB down
		GainDb = ceil(20. * log10(m_Gain[m_PointWithFocus] / 1.00001)) - 1.;
		SetCurrentPointGainDb(GainDb);
		DrawDotCaret(true);
		NotifyParentDlg();
		return;
		break;
	case VK_LEFT:

		if (CtrlPressed || ShiftPressed)
		{
			// Ctrl+Left - shift the whole transition band
			// Shift+Left - shift only the focused point

			SetFilterPointPixel(m_PointWithFocus,
								GetFilterPointPixel(m_PointWithFocus) - 1, CtrlPressed);
			RebuildFilters();
			Invalidate();
			NotifyParentDlg();
		}
		else
		{
			// focus to the prev band
			// find next visible point
			int NewFocusPoint = m_PointWithFocus;
			switch (m_PointWithFocus)
			{
			case HpfStopbandIndex:
				// nothing
				break;
			case HpfPassbandIndex:
				NewFocusPoint = HpfStopbandIndex;
				break;
			case NotchBeginIndex:
				if (m_bHighPass)
				{
					NewFocusPoint = HpfPassbandIndex;
				}
				break;
			case NotchZeroIndex:
				NewFocusPoint = NotchBeginIndex;
				break;
			case LpfPassbandIndex:
				if (m_bNotchFilter)
				{
					NewFocusPoint = NotchZeroIndex;
				}
				else if (m_bHighPass)
				{
					NewFocusPoint = HpfPassbandIndex;
				}
				break;
			case LpfStopbandIndex:
				NewFocusPoint = LpfPassbandIndex;
				break;
			}
			if (m_PointWithFocus != NewFocusPoint)
			{
				SetFocusPoint(NewFocusPoint);
				NotifyParentDlg();
			}
		}
		break;
	case VK_RIGHT:
		if (CtrlPressed || ShiftPressed)
		{
			// Ctrl+Right - shift the whole transition band
			// Shift+Right - shift only the focused point

			SetFilterPointPixel(m_PointWithFocus,
								GetFilterPointPixel(m_PointWithFocus) + 1, CtrlPressed);
			RebuildFilters();
			Invalidate();
			NotifyParentDlg();
		}
		else
		{
			// focus to the next band
			// find next visible point
			int NewFocusPoint = m_PointWithFocus;
			switch (m_PointWithFocus)
			{
			case HpfStopbandIndex:
				NewFocusPoint = HpfPassbandIndex;
				break;
			case HpfPassbandIndex:
				if (m_bNotchFilter)
				{
					NewFocusPoint = NotchBeginIndex;
				}
				else if (m_bLowPass)
				{
					NewFocusPoint = LpfPassbandIndex;
				}
				break;
			case NotchBeginIndex:
				NewFocusPoint = NotchZeroIndex;
				break;
			case NotchZeroIndex:
				if (m_bLowPass)
				{
					NewFocusPoint = LpfPassbandIndex;
				}
				break;
			case LpfPassbandIndex:
				NewFocusPoint = LpfStopbandIndex;
				break;
			case LpfStopbandIndex:
				// nothing
				break;
			}
			if (m_PointWithFocus != NewFocusPoint)
			{
				SetFocusPoint(NewFocusPoint);
				NotifyParentDlg();
			}
		}
		break;
	}
}

UINT CFilterGraphWnd::OnGetDlgCode()
{
	return DLGC_WANTARROWS;
}

int CFilterGraphWnd::GetHitCode(POINT point)
{
	CRect cr;
	GetClientRect( & cr);
	if ( ! cr.PtInRect(point))
	{
		return -0x100;
	}
	int dx = GetSystemMetrics(SM_CXSIZEFRAME);
	int dy = GetSystemMetrics(SM_CYSIZEFRAME);
	for (int i = 0; i < MaxFilterFrequencies; i++)
	{
		// find if the mouse gets into a focus point
		int x = int((1. + log10(m_Frequencies[i] / M_PI) / 3.) * cr.Width() - 1);
		// full range: 5 to -85 db
		int y = GainToPosY(m_Gain[i], cr.Height());
		CRect r(x - dx, y - dy, x + dx, y + dy);
		CRect r1(x - dx, cr.top, x + dx, cr.bottom);

		if (i <= HpfPassbandIndex)
		{
			if ( ! m_bHighPass)
			{
				continue;
			}
		}
		else if (i >= LpfPassbandIndex)
		{
			if ( ! m_bLowPass)
			{
				continue;
			}
		}
		else
		{
			if ( ! m_bNotchFilter)
			{
				continue;
			}
		}

		// if drag handle hit, return zero-based band index
		if (r.PtInRect(point))
		{
			return i;
		}
		// if vertical band hit, return one complement of zero-based band index
		if (r1.PtInRect(point))
		{
			return ~i;
		}
	}
	return -0x200;
}

void CFilterGraphWnd::OnTimer(UINT nIDEvent)
{
	// redraw blinking dot
	if (m_bGotFocus && ! m_bButtonPressed)
	{
		DrawDotCaret( ! m_DotCaretIsOn);
	}
}

void CFilterGraphWnd::InvalidateGraphPoint(double Frequency, double Gain)
{
	CRect cr;
	GetClientRect( & cr);

	int x = int((1. + log10(Frequency / M_PI) / 3.) * cr.Width() - 1);
	int y = GainToPosY(Gain, cr.Height());

	int dx = GetSystemMetrics(SM_CXSIZEFRAME);
	int dy = GetSystemMetrics(SM_CYSIZEFRAME);
	CRect r(x - dx, y - dy, x + dx, y + dy);
	InvalidateRect( & r);
}

void CFilterGraphWnd::DrawDotCaret(bool state)
{
	if (m_DotCaretIsOn != state)
	{
		m_DotCaretIsOn = state;
		InvalidateGraphPoint(m_Frequencies[m_PointWithFocus], m_Gain[m_PointWithFocus]);
	}
	if (m_bGotFocus && ! m_bButtonPressed)
	{
		SetTimer(1, GetCaretBlinkTime(), NULL);
	}
}

void CFilterDialog::OnOK()
{
	int FocusId = GetFocus()->GetDlgCtrlID();
	if (IDC_EDIT_BAND_GAIN == FocusId)
	{
		// read the gain
		double GainDb;
		if (m_EditGain.GetData(NULL, GainDb, NULL, NULL, -90., -0.1))
		{
			m_wGraph.SetCurrentPointGainDb(GainDb);
			// set focus to the graph
			m_wGraph.SetFocus();
		}
		return;
	}
	else if (IDC_EDIT_FREQUENCY == FocusId)
	{
		// read the freqyqncy
		double Frequency;
		if (m_EditFrequency.GetData(NULL, Frequency, NULL, NULL, 1, m_wGraph.m_SamplingRate))
		{
			m_wGraph.SetCurrentPointFrequencyHz(Frequency);
			// set focus to the graph
			m_wGraph.SetFocus();
		}
		return;
	}

	CResizableDialog::OnOK();
}

void CFilterDialog::OnNotifyGraph( NMHDR * pNotifyStruct, LRESULT * result )
{
	m_EditGain.SetData(m_wGraph.GetCurrentPointGainDb());
	m_EditFrequency.SetData(m_wGraph.GetCurrentPointFrequencyHz());
}

void CFilterDialog::OnKillfocusEditBandGain()
{
	// read the gain
	double GainDb;
	if (m_EditGain.GetData(NULL, GainDb, NULL, NULL, -20., 20.))
	{
		m_wGraph.SetCurrentPointGainDb(GainDb);
	}
}

void CFilterGraphWnd::OnLButtonDblClk(UINT nFlags, CPoint point)
{
	// if click on the band, set the gain
	CWnd::OnLButtonDblClk(nFlags, point);
	int nHit = GetHitCode(point);
	if (nHit < int(~MaxFilterFrequencies))
	{
		return;
	}
	if (nHit < 0)
	{
		nHit = ~ nHit;
	}
	m_bButtonPressed = true;
	DrawDotCaret(true);
	SetFocusPoint(nHit);

	CRect cr;
	GetClientRect( & cr);
	if (point.y < cr.top)
	{
		point.y = cr.top;
	}
	if (point.y >= cr.bottom)
	{
		point.y = cr.bottom - 1;
	}
	SetCurrentPointGainDb(PosYToGainDb(point.y, cr.Height()));
	NotifyParentDlg();
}

void CFilterDialog::OnCheckZeroPhase()
{
	if (IsDlgButtonChecked(IDC_CHECK_ZERO_PHASE))
	{
		m_wGraph.m_bZeroPhase = TRUE;
	}
	else
	{
		m_wGraph.m_bZeroPhase = FALSE;
	}
	// force filter recalculation
	//m_wGraph.SetNumberOfBands(m_nBands);
	m_wGraph.RebuildFilters();
	m_wGraph.Invalidate();
}

void Filter::RebuildFilters()
{
	if (m_bLowPass)
	{
		CreateLowpassElliptic(m_Frequencies[LpfPassbandIndex],
							m_Gain[LpfPassbandIndex],
							m_Frequencies[LpfStopbandIndex],
							m_Gain[LpfStopbandIndex]);
	}
	if (m_bHighPass)
	{
		CreateHighpassElliptic(m_Frequencies[HpfPassbandIndex],
								m_Gain[HpfPassbandIndex],
								m_Frequencies[HpfStopbandIndex],
								m_Gain[HpfStopbandIndex]);
	}
	if (m_bNotchFilter)
	{
		// two zeros at unity circle, two poles
		double f0 = m_Frequencies[NotchZeroIndex];
		double Width = fabs(f0 - m_Frequencies[NotchBeginIndex]);
		if (m_bZeroPhase)
		{
			Width /= 2.;
		}
		m_nNotchOrder = 1;
		double RotC = cos(f0);
		double pole = 1. - Width;
		m_NotchCoeffs[0][0] = 1;
		m_NotchCoeffs[0][1] = -2 * RotC;
		m_NotchCoeffs[0][2] = 1;
		m_NotchCoeffs[0][3] = 1;
		m_NotchCoeffs[0][4] = -2 * pole * RotC;
		m_NotchCoeffs[0][5] = pole * pole;
	}
}

BOOL Filter::CreateLowpassElliptic(double PassFreq, double PassLoss,
									double StopFreq, double StopLoss)
{
	double OmegaPass = 2. * tan(PassFreq / 2.);
	double OmegaStop = 2. * tan(StopFreq / 2.);
	if (m_bZeroPhase)
	{
		PassLoss = sqrt(PassLoss);
		StopLoss = sqrt(StopLoss);
	}
	POLY_ROOTS zeros;
	POLY_ROOTS poles;
	COMPLEX NormCoeff;

	EllipticPolesZeros(OmegaPass, OmegaStop, StopLoss,
						PassLoss, 1, zeros, poles, NormCoeff);

	CArray<polyRatio *, polyRatio *> * pDecomposed
		= polyRatio(poly(zeros, NormCoeff), poly(poles)).Decompose(2, & poles);

	m_nLpfOrder = pDecomposed->GetSize();

	for (int i = 0; i < pDecomposed->GetSize(); i++)
	{
#if 1 && defined(_DEBUG)
		pDecomposed->GetAt(i)->Dump();
#endif
		polyRatio prBil;
		BilinearTransform( *pDecomposed->GetAt(i), prBil, 1.);
		ASSERT(prBil.numer().order() == 2 || prBil.numer().order() == 1);
		ASSERT(prBil.denom().order() == 2 || prBil.denom().order() == 1);

		m_LpfCoeffs[i][0] = prBil.numer()[0].real();
		m_LpfCoeffs[i][1] = prBil.numer()[1].real();
		if (prBil.numer().order() > 1)
		{
			m_LpfCoeffs[i][2] = prBil.numer()[2].real();
		}
		else
		{
			m_LpfCoeffs[i][2] = 0.;
		}
		m_LpfCoeffs[i][3] = prBil.denom()[0].real();
		m_LpfCoeffs[i][4] = prBil.denom()[1].real();

		if (prBil.numer().order() > 1)
		{
			m_LpfCoeffs[i][5] = prBil.denom()[2].real();
		}
		else
		{
			m_LpfCoeffs[i][5] = 0.;
		}

		delete pDecomposed->GetAt(i);
	}
	delete pDecomposed;
	return TRUE;
}

BOOL Filter::CreateHighpassElliptic(double PassFreq, double PassLoss,
									double StopFreq, double StopLoss)
{
	double OmegaPass = 2. / tan(PassFreq / 2.);
	double OmegaStop = 2. / tan(StopFreq / 2.);
	if (m_bZeroPhase)
	{
		PassLoss = sqrt(PassLoss);
		StopLoss = sqrt(StopLoss);
	}
	POLY_ROOTS zeros;
	POLY_ROOTS poles;
	COMPLEX NormCoeff;

	EllipticPolesZeros(OmegaPass, OmegaStop, StopLoss,
						PassLoss, 1, zeros, poles, NormCoeff);

	CArray<polyRatio *, polyRatio *> * pDecomposed
		= polyRatio(poly(zeros, NormCoeff), poly(poles)).Decompose(2, & poles);

	m_nHpfOrder = pDecomposed->GetSize();

	for (int i = 0; i < pDecomposed->GetSize(); i++)
	{
#if 1 && defined(_DEBUG)
		pDecomposed->GetAt(i)->Dump();
#endif
		polyRatio prBil;
		BilinearTransform( *pDecomposed->GetAt(i), prBil, 1.);
		ASSERT(prBil.numer().order() == 2 || prBil.numer().order() == 1);
		ASSERT(prBil.denom().order() == 2 || prBil.denom().order() == 1);

		m_HpfCoeffs[i][0] = prBil.numer()[0].real();
		m_HpfCoeffs[i][1] = -prBil.numer()[1].real();
		if (prBil.numer().order() > 1)
		{
			m_HpfCoeffs[i][2] = prBil.numer()[2].real();
		}
		else
		{
			m_HpfCoeffs[i][2] = 0.;
		}
		m_HpfCoeffs[i][3] = prBil.denom()[0].real();
		m_HpfCoeffs[i][4] = -prBil.denom()[1].real();

		if (prBil.numer().order() > 1)
		{
			m_HpfCoeffs[i][5] = prBil.denom()[2].real();
		}
		else
		{
			m_HpfCoeffs[i][5] = 0.;
		}

		delete pDecomposed->GetAt(i);
	}
	delete pDecomposed;
	return TRUE;
}



void CFilterDialog::OnKillfocusEditFrequency()
{
	// read the freqyqncy
	double Frequency;
	if (m_EditFrequency.GetData(NULL, Frequency, NULL, NULL, 1, m_wGraph.m_SamplingRate))
	{
		m_wGraph.SetCurrentPointFrequencyHz(Frequency);
	}
}

