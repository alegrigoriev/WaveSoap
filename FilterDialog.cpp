// FilterDialog.cpp : implementation file
//

#include "stdafx.h"
#include "WaveSoapFront.h"
#include "FilterDialog.h"
#include "OperationDialogs.h"
#include "FileDialogWithHistory.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CFilterDialog dialog


CFilterDialog::CFilterDialog(CWnd* pParent /*=NULL*/)
	: CDialog(CFilterDialog::IDD, pParent)
{
	//{{AFX_DATA_INIT(CFilterDialog)
	m_bUndo = FALSE;
	//}}AFX_DATA_INIT
	m_PrevSize.cx = -1;
	m_PrevSize.cy = -1;
	memset(& m_mmxi, 0, sizeof m_mmxi);

	m_Profile.AddItem("Settings", "FilterDlgWidth", m_DlgWidth, 0, 0, 4096);
	m_Profile.AddItem("Settings", "FilterDlgHeight", m_DlgHeight, 0, 0, 4096);

	m_Profile.AddBoolItem("Filter", "ZeroPhase", m_wGraph.m_bZeroPhase, FALSE);
	m_Profile.AddBoolItem("Filter", "LowPassFilter", m_wGraph.m_bLowPass, TRUE);
	m_Profile.AddBoolItem("Filter", "HighPassFilter", m_wGraph.m_bHighPass, TRUE);
	m_Profile.AddBoolItem("Filter", "NotchFilter", m_wGraph.m_bNotchFilter, FALSE);

	for (int n = 0; n < MaxFilterFrequencies; n++)
	{
		CString s;
		s.Format("Gain%d", n + 1);
		m_Profile.AddItem("Filter", s, m_wGraph.m_Gain[n], m_wGraph.m_Gain[n], 0.00003, 1.);
		s.Format("Frequency%d", n + 1);
		m_Profile.AddItem("Filter", s, m_wGraph.m_Frequencies[n], m_wGraph.m_Frequencies[n], 0.00314, 3.14);
	}
	// check for correct frequencies and gain
	if (m_wGraph.m_Gain[HpfPassbandIndex] <= m_wGraph.m_Gain[HpfStopbandIndex])
	{
		m_wGraph.m_Gain[HpfPassbandIndex] = 0.9;
		m_wGraph.m_Gain[HpfStopbandIndex] = 0.001;
	}
	if (m_wGraph.m_Frequencies[HpfPassbandIndex] <= m_wGraph.m_Frequencies[HpfStopbandIndex])
	{
		m_wGraph.m_Frequencies[HpfPassbandIndex] = M_PI / 250.;
		m_wGraph.m_Frequencies[HpfStopbandIndex] = M_PI / 500.;
	}
	// check for correct frequencies and gain
	if (m_wGraph.m_Gain[LpfPassbandIndex] <= m_wGraph.m_Gain[LpfStopbandIndex])
	{
		m_wGraph.m_Gain[LpfPassbandIndex] = 0.9;
		m_wGraph.m_Gain[LpfStopbandIndex] = 0.001;
	}
	if (m_wGraph.m_Frequencies[LpfPassbandIndex] >= m_wGraph.m_Frequencies[LpfStopbandIndex])
	{
		m_wGraph.m_Frequencies[LpfPassbandIndex] = M_PI * 0.8;
		m_wGraph.m_Frequencies[LpfStopbandIndex] = M_PI * 0.9;
	}
}


void CFilterDialog::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_EDIT_BAND_GAIN, m_EditGain);
	DDX_Control(pDX, IDC_EDIT_FREQUENCY, m_EditFrequency);
	//{{AFX_DATA_MAP(CFilterDialog)
	DDX_Control(pDX, IDC_STATIC_SELECTION, m_SelectionStatic);
	DDX_Check(pDX, IDC_CHECK_UNDO, m_bUndo);
	//}}AFX_DATA_MAP
	DDX_Check(pDX, IDC_CHECK_ZERO_PHASE, m_wGraph.m_bZeroPhase);
	DDX_Check(pDX, IDC_CHECK_LOWPASS, m_wGraph.m_bLowPass);
	DDX_Check(pDX, IDC_CHECK_HIGHPASS, m_wGraph.m_bHighPass);

	if (pDX->m_bSaveAndValidate)
	{
		m_Profile.UnloadAll();
	}
}


BEGIN_MESSAGE_MAP(CFilterDialog, CDialog)
	//{{AFX_MSG_MAP(CFilterDialog)
	ON_BN_CLICKED(IDC_BUTTON_LOAD, OnButtonLoad)
	ON_BN_CLICKED(IDC_BUTTON_RESET_BANDS, OnButtonResetBands)
	ON_BN_CLICKED(IDC_BUTTON_SAVE_AS, OnButtonSaveAs)
	ON_BN_CLICKED(IDC_BUTTON_SELECTION, OnButtonSelection)
	ON_BN_CLICKED(IDC_CHECK_ZERO_PHASE, OnCheckZeroPhase)
	ON_WM_SIZE()
	ON_WM_SIZING()
	ON_WM_GETMINMAXINFO()
	ON_WM_ERASEBKGND()
	ON_WM_NCHITTEST()
	ON_BN_CLICKED(IDC_CHECK_LOWPASS, OnCheckLowpass)
	ON_BN_CLICKED(IDC_CHECK_HIGHPASS, OnCheckHighpass)
	ON_EN_KILLFOCUS(IDC_EDIT_FREQUENCY, OnKillfocusEditFrequency)
	//}}AFX_MSG_MAP
	ON_NOTIFY(NM_RETURN, AFX_IDW_PANE_FIRST, OnNotifyGraph)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CFilterDialog message handlers

void CFilterDialog::OnMetricsChange()
{
	// Initialize MINMAXINFO
	CRect r;
	SystemParametersInfo(SPI_GETWORKAREA, 0, & r, 0);
	m_mmxi.ptMaxSize.x = r.Width();
	m_mmxi.ptMaxTrackSize.x = m_mmxi.ptMaxSize.x;
	m_mmxi.ptMaxSize.y = r.Height();
	m_mmxi.ptMaxTrackSize.y = m_mmxi.ptMaxSize.y;
	m_mmxi.ptMaxPosition.x = r.left;
	m_mmxi.ptMaxPosition.y = r.top;
	GetWindowRect(& r);
	m_mmxi.ptMinTrackSize.x = r.Width();
	m_mmxi.ptMinTrackSize.y = r.Height();
}

void CFilterDialog::OnSize(UINT nType, int cx, int cy)
{
	CDialog::OnSize(nType, cx, cy);
	if (m_PrevSize.cx > 0)
	{
		// resize graph control
		if (NULL != m_wGraph.m_hWnd)
		{
			CRect r;
			m_wGraph.GetWindowRect( & r);
			m_wGraph.SetWindowPos(NULL, 0, 0, r.Width() + cx - m_PrevSize.cx,
								r.Height() + cy - m_PrevSize.cy,
								SWP_DRAWFRAME | SWP_NOACTIVATE
								| SWP_NOMOVE | SWP_NOZORDER | SWP_NOOWNERZORDER);
		}

		// move all controls
		static int const MoveDown[] =
		{
			IDC_STATIC1, IDC_EDIT_BAND_GAIN, IDC_STATIC2,
			IDC_EDIT_FREQUENCY, IDC_STATIC4,
			IDC_CHECK_LOWPASS, IDC_CHECK_STOPBAND, IDC_CHECK_HIGHPASS,
			IDC_CHECK_ZERO_PHASE, IDC_CHECK_UNDO,
			IDC_STATIC_SELECTION, IDC_BUTTON_SELECTION,
		};
		for (int i = 0; i < sizeof MoveDown / sizeof MoveDown[0]; i++)
		{
			CWnd * pWnd = GetDlgItem(MoveDown[i]);
			if (pWnd)
			{
				CRect r;
				pWnd->GetWindowRect( & r);
				ScreenToClient( & r);
				pWnd->SetWindowPos(NULL, r.left, r.top + cy - m_PrevSize.cy,
									0, 0, SWP_NOCOPYBITS | SWP_NOACTIVATE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOOWNERZORDER);
			}
		}
		static int const MoveRightDown[] =
		{
			IDC_BUTTON_RESET_BANDS, IDC_BUTTON_SAVE_AS,
			IDC_BUTTON_LOAD, IDOK, IDCANCEL, IDHELP,
		};
		for (i = 0; i < sizeof MoveRightDown / sizeof MoveRightDown[0]; i++)
		{
			CWnd * pWnd = GetDlgItem(MoveRightDown[i]);
			if (pWnd)
			{
				CRect r;
				pWnd->GetWindowRect( & r);
				ScreenToClient( & r);
				pWnd->SetWindowPos(NULL, r.left + cx - m_PrevSize.cx,
									r.top + cy - m_PrevSize.cy,
									0, 0, SWP_NOCOPYBITS | SWP_NOACTIVATE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOOWNERZORDER);
			}
		}
	}

	m_PrevSize.cx = cx;
	m_PrevSize.cy = cy;

	// invalidate an area which is (after resizing)
	// occupied by size grip
	int size = GetSystemMetrics(SM_CXVSCROLL);
	CRect r(cx - size, cy - size, cx, cy);
	InvalidateRect( & r, TRUE);
}

void CFilterDialog::OnSizing(UINT fwSide, LPRECT pRect)
{
	CDialog::OnSizing(fwSide, pRect);

	// invalidate an area currently (before resizing)
	// occupied by size grip
	CRect r;
	GetClientRect( & r);
	int size = GetSystemMetrics(SM_CXVSCROLL);
	r.left = r.right - size;
	r.top = r.bottom - size;
	InvalidateRect( & r, FALSE);
}

void CFilterDialog::OnGetMinMaxInfo(MINMAXINFO FAR* lpMMI)
{
	if (m_mmxi.ptMaxSize.x != 0)
	{
		*lpMMI = m_mmxi;
	}
	else
	{
		CDialog::OnGetMinMaxInfo(lpMMI);
	}
}

BOOL CFilterDialog::OnEraseBkgnd(CDC* pDC)
{
	if (CDialog::OnEraseBkgnd(pDC))
	{
		// draw size grip
		CRect r;
		GetClientRect( & r);
		int size = GetSystemMetrics(SM_CXVSCROLL);
		r.left = r.right - size;
		r.top = r.bottom - size;
		pDC->DrawFrameControl( & r, DFC_SCROLL, DFCS_SCROLLSIZEGRIP);
		return TRUE;
	}
	else
	{
		return FALSE;
	}
}

UINT CFilterDialog::OnNcHitTest(CPoint point)
{
	// return HTBOTTOMRIGHT for sizegrip area
	CRect r;
	GetClientRect( & r);
	int size = GetSystemMetrics(SM_CXVSCROLL);
	r.left = r.right - size;
	r.top = r.bottom - size;
	ScreenToClient( & point);

	if (r.PtInRect(point))
	{
		return HTBOTTOMRIGHT;
	}
	else
		return CDialog::OnNcHitTest(point);
}

void CFilterDialog::OnCheckLowpass()
{
	if (IsDlgButtonChecked(IDC_CHECK_LOWPASS))
	{
		m_wGraph.m_bLowPass = TRUE;
	}
	else
	{
		m_wGraph.m_bLowPass = FALSE;
	}
	// force filter recalculation
	//m_wGraph.SetNumberOfBands(m_nBands);
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
	}
	// force filter recalculation
	//m_wGraph.SetNumberOfBands(m_nBands);
	m_wGraph.RebuildFilters();
	m_wGraph.Invalidate();
}

BOOL CFilterDialog::OnInitDialog()
{
	m_wGraph.m_SamplingRate = m_pWf->nSamplesPerSec;
	CRect r;
	CWnd * pTemplateWnd = GetDlgItem(IDC_STATIC_RESPONSE_TEMPLATE);
	pTemplateWnd->GetWindowRect( & r);
	ScreenToClient( & r);
	m_wGraph.Create(NULL, "", WS_CHILD | WS_VISIBLE
					| WS_CLIPSIBLINGS | WS_CLIPCHILDREN | WS_TABSTOP,
					r, this, AFX_IDW_PANE_FIRST);
	m_wGraph.SetWindowPos(pTemplateWnd, 0, 0, 0, 0,
						SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOSIZE);
	pTemplateWnd->DestroyWindow();

	CDialog::OnInitDialog();

	m_EditGain.SetData(m_wGraph.GetCurrentPointGainDb());
	m_EditFrequency.SetData(m_wGraph.GetCurrentPointFrequencyHz());
	// init MINMAXINFO
	OnMetricsChange();
	UpdateSelectionStatic();
	m_wGraph.RebuildFilters();

	// set dialog size
	if (m_DlgWidth < m_mmxi.ptMinTrackSize.x)
	{
		m_DlgWidth = m_mmxi.ptMinTrackSize.x;
	}
	if (m_DlgWidth > m_mmxi.ptMaxTrackSize.x)
	{
		m_DlgWidth = m_mmxi.ptMaxTrackSize.x;
	}
	if (m_DlgHeight < m_mmxi.ptMinTrackSize.y)
	{
		m_DlgHeight = m_mmxi.ptMinTrackSize.y;
	}
	if (m_DlgHeight > m_mmxi.ptMaxTrackSize.y)
	{
		m_DlgHeight = m_mmxi.ptMaxTrackSize.y;
	}
	SetWindowPos(NULL, 0, 0, m_DlgWidth, m_DlgHeight,
				SWP_NOACTIVATE | SWP_NOOWNERZORDER | SWP_NOZORDER | SWP_NOMOVE);

	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}
void CFilterDialog::UpdateSelectionStatic()
{
	m_SelectionStatic.SetWindowText(GetSelectionText(m_Start, m_End, m_Chan,
													m_pWf->nChannels, m_bLockChannels,
													m_pWf->nSamplesPerSec, m_TimeFormat));
}


void CFilterDialog::OnButtonSelection()
{
	CSelectionDialog dlg;
	dlg.m_Start = m_Start;
	dlg.m_End = m_End;
	dlg.m_CaretPosition = m_CaretPosition;
	dlg.m_Length = m_End - m_Start;
	dlg.m_FileLength = m_FileLength;
	dlg.m_Chan = m_Chan + 1;
	dlg.m_pWf = m_pWf;
	dlg.m_TimeFormat = m_TimeFormat;

	if (IDOK != dlg.DoModal())
	{
		return;
	}
	m_Start = dlg.m_Start;
	m_End = dlg.m_End;
	m_Chan = dlg.m_Chan - 1;
	UpdateSelectionStatic();
}

CFilterGraphWnd::CFilterGraphWnd()
{
	m_bMouseCaptured = false;
	m_bButtonPressed = false;
	m_bGotFocus = false;
	m_PointWithFocus = 0;

	m_Gain[HpfNotchIndex] = 0.000003;
	m_Gain[HpfPassbandIndex] = 0.9;
	m_Gain[HpfStopbandIndex] = 0.001;
	m_Frequencies[HpfNotchIndex] = M_PI / 2000.;
	m_Frequencies[HpfPassbandIndex] = M_PI / 250.;
	m_Frequencies[HpfStopbandIndex] = M_PI / 500.;

	m_Gain[NotchBeginIndex] = 0.9;
	m_Gain[NotchEndIndex] = 0.9;
	m_Frequencies[NotchBeginIndex] = M_PI * 0.48;
	m_Frequencies[NotchEndIndex] = M_PI * 0.52;

	m_Gain[LpfPassbandIndex] = 0.9;
	m_Gain[LpfStopbandIndex] = 0.001;
	m_Frequencies[LpfPassbandIndex] = M_PI * 0.8;
	m_Frequencies[LpfStopbandIndex] = M_PI * 0.9;
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
	if (0) TRACE("CFilterGraphWnd::OnPaint\n");
	CPaintDC dc(this); // device context for painting

	CRect cr;
	CRect ur;
	GetClientRect( & cr);
	dc.GetClipBox( & ur);
	dc.SetMapMode(MM_TEXT);
	CGdiObject * pOldPen = dc.SelectStockObject(BLACK_PEN);
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
		int y = (0.25 - log10(gain)) * cr.Height() / 4.5;
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
	CBitmap * pOldBitmap = cdc.SelectObject( & bmp);

	CGdiObject * pOldBrush = cdc.SelectStockObject(WHITE_BRUSH);
	CGdiObject * pOldCdcPen = cdc.SelectStockObject(WHITE_PEN);
	cdc.Rectangle(0, 0, 2 * w, h);
	cdc.SelectStockObject(BLACK_PEN);

	cdc.Ellipse(0, 0, 2 * dx, 2 * dy);

	cdc.SelectStockObject(BLACK_BRUSH);
	cdc.Ellipse(w, 0, w + 2 * dx, 2 * dy);

	cdc.SelectObject(pOldCdcPen);
	cdc.SelectObject(pOldBrush);

	pOldBrush = dc.SelectStockObject(NULL_BRUSH);
	dc.SetBkColor(0xFFFFFF);
	dc.SetTextColor(0x000000);

	for (int i = 0; i < MaxFilterFrequencies; i++)
	{
		// draw circles around the reference points
		int x = (1. + log10(m_Frequencies[i] / M_PI) / 3.) * cr.Width() - 1;
		// full range: 5 to -85 db
		int y = (0.25 - log10(m_Gain[i])) * cr.Height() / 4.5;
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
	cdc.SelectObject(pOldBitmap);
	dc.SelectObject(pOldBrush);
	dc.SelectObject(pOldPen);
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
		// full range: 5 to -85 db
		double gainDb = 5. - point.y * 85. / cr.Height();

		SetPointGainDb(m_PointWithFocus, gainDb);
		// TODO SetPointFrequency(GetCurrentPointFrequencyHz());
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
	}

	if (m_bZeroPhase)
	{
		// filter is applied twice
		Result *= conj(Result);
	}
	return Result;
}

void CFilterGraphWnd::SetPointGainDb(int nPoint, double GainDb)
{
	if (GainDb > 0.)
	{
		GainDb = 0.;
	}
	switch (nPoint)
	{
	case HpfNotchIndex:
		return;
		break;
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
	case NotchEndIndex:
		GainDb = -3.;
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
	CGdiObject * pOldBrush = pDC->SelectStockObject(WHITE_BRUSH);
	CRect cr;
	GetClientRect( & cr);
	if (0) TRACE("CFilterGraphWnd::OnEraseBkgnd, cr=%d, %d, %d, %d\n",
				cr.left, cr.right, cr.top, cr.bottom);

	pDC->PatBlt(cr.left, cr.top, cr.Width(), cr.Height(), PATCOPY);

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

		// draw zero level line
		pDC->PatBlt(cr.left, (cr.top + cr.bottom) / 2, cr.Width(), 1, PATINVERT);
		// draw frequency lines
		for (int i = 0; i < 20; i++)
		{
			int x = cr.Width() * (i * 2 + 1) / 40;
			pDC->PatBlt(x, cr.top, 1, cr.bottom - cr.top, PATINVERT);
		}
		pDC->SelectObject(pOldBrush);
	}
	catch (CResourceException)
	{
		TRACE("CResourceException\n");
	}
	CPen DotPen(PS_DOT, 0, COLORREF(0));
	CPen * pOldPen = pDC->SelectObject( & DotPen);
	// TODO: change
	pDC->MoveTo(cr.left, cr.bottom / 4);
	pDC->LineTo(cr.right, cr.bottom / 4);

	pDC->MoveTo(cr.left, cr.bottom - cr.bottom / 4);
	pDC->LineTo(cr.right, cr.bottom - cr.bottom / 4);

	pDC->SelectObject(pOldPen);

	pDC->SelectObject(pOldBrush);
	return TRUE;
}

void CFilterGraphWnd::OnNcCalcSize(BOOL bCalcValidRects, NCCALCSIZE_PARAMS * lpncsp)
{
	int ncHeight = 2 + GetSystemMetrics(SM_CYMENUSIZE);
	int ncWidth;
	CWnd * pW = GetDesktopWindow();
	CDC * pDC = pW->GetWindowDC();
	CGdiObject * pOld = pDC->SelectStockObject(ANSI_VAR_FONT);
	ncWidth = 3 + pDC->GetTextExtent("-20 dB", 6).cx;

	pDC->SelectObject(pOld);
	pW->ReleaseDC(pDC);
	lpncsp->rgrc[0].left += ncWidth;
	lpncsp->rgrc[0].right -= GetSystemMetrics(SM_CXSIZEFRAME);
	lpncsp->rgrc[0].top += GetSystemMetrics(SM_CYSIZEFRAME);
	lpncsp->rgrc[0].bottom -= ncHeight /* + GetSystemMetrics(SM_CXSIZEFRAME) */;

}

void CFilterGraphWnd::OnNcPaint(UINT wParam)
{
	if (0) TRACE("CFilterGraphWnd::OnNcPaint, hrgn=%X\n", wParam);
	// copy region, because it will be deleted
	CRect wr;
	GetWindowRect( & wr);

	CDC * pDC = GetWindowDC();
	if (NULL == pDC)
	{
		return;
	}
	if (wParam != 1)
	{
		CRgn rgn;
		rgn.CreateRectRgn(0, 0, 1, 1);
		rgn.CopyRgn(CRgn::FromHandle((HRGN)wParam));
		rgn.OffsetRgn( -wr.left, -wr.top);
		pDC->SelectClipRgn( & rgn, RGN_AND);
	}

	NCCALCSIZE_PARAMS ncp;
	wr.right = wr.Width();
	wr.left = 0;
	wr.bottom = wr.Height();
	wr.top = 0;
	ncp.rgrc[0] = wr;
	OnNcCalcSize(FALSE, & ncp);
	//SendMessage(WM_NCCALCSIZE, FALSE, (LPARAM) & ncp);
	pDC->ExcludeClipRect( & ncp.rgrc[0]);

	// Paint into this DC
	CBrush BkBrush;
	CWnd * pParentDlg = GetParent();
	BkBrush.Attach(GetSysColorBrush(COLOR_BTNFACE));
	//BkBrush.Attach((HBRUSH) pParentDlg->SendMessage(WM_CTLCOLORDLG, (WPARAM)(pParentDlg->m_hWnd), LPARAM(pDC->m_hDC)));

	CGdiObject * pOldPen = pDC->SelectStockObject(BLACK_PEN);
	CGdiObject * pOldBrush = pDC->SelectObject( & BkBrush);
	// fill the NC area
	pDC->Rectangle( & wr);
	// Draw rectangle
	pDC->SelectStockObject(HOLLOW_BRUSH);
	BkBrush.Detach();

	pDC->Rectangle(ncp.rgrc[0].left - 1, ncp.rgrc[0].top - 1,
					ncp.rgrc[0].right + 1, ncp.rgrc[0].bottom + 1);
	CGdiObject * pOldFont = pDC->SelectStockObject(ANSI_VAR_FONT);
	// draw frequencies
	pDC->SetTextAlign(TA_TOP | TA_CENTER);
	pDC->SetTextColor(0x000000);   // black
	pDC->SetBkMode(TRANSPARENT);

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
			s.Format("%.1fk", f / 1000.);
			if (f >= 10000.)
			{
				TextWidth = pDC->GetTextExtent("20.0k ", 7).cx;
			}
			else
			{
				TextWidth = pDC->GetTextExtent("0.0k ", 7).cx;
			}
		}
		else if (f >= 100.)
		{
			// 550 (round to 10)
			s.Format("%d", 10 * ((int(f) + 5) / 10));
			TextWidth = pDC->GetTextExtent("999 ", 7).cx;
		}
		else
		{
			// 55
			s.Format("%d", int(f));
			TextWidth = pDC->GetTextExtent("99 ", 7).cx;
		}
		if (x - TextWidth / 2 >= PrevX)
		{
			pDC->TextOut(x, ncp.rgrc[0].bottom - 1, s);
			PrevX = x + TextWidth / 2;
		}
	}

	pDC->SetTextAlign(TA_BOTTOM | TA_RIGHT);
	int TextHeight = pDC->GetTextExtent("9", 1).cy;

	pDC->TextOut(ncp.rgrc[0].left - 1,
				ncp.rgrc[0].top + TextHeight,
				"+5 dB", 5);

	pDC->TextOut(ncp.rgrc[0].left - 1,
				ncp.rgrc[0].top + (ncp.rgrc[0].bottom - ncp.rgrc[0].top) / 4 + TextHeight / 2,
				"-20 dB", 5);

	pDC->TextOut(ncp.rgrc[0].left - 1,
				ncp.rgrc[0].top + (ncp.rgrc[0].bottom - ncp.rgrc[0].top) / 2 + TextHeight / 2,
				"-40 dB", 4);

	pDC->TextOut(ncp.rgrc[0].left - 1,
				ncp.rgrc[0].bottom - (ncp.rgrc[0].bottom - ncp.rgrc[0].top) / 4 + TextHeight / 2,
				"-60 dB", 6);

	pDC->TextOut(ncp.rgrc[0].left - 1,
				ncp.rgrc[0].bottom + TextHeight / 2,
				"-80 dB", 6);

	pDC->SelectObject(pOldFont);
	pDC->SelectObject(pOldBrush);
	pDC->SelectObject(pOldPen);
	ReleaseDC(pDC);
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
		SetCursor(AfxGetApp()->LoadStandardCursor(IDC_SIZENS));
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
								"Fltr", NULL,
								OFN_HIDEREADONLY
								| OFN_EXPLORER | OFN_NONETWORKBUTTON | OFN_FILEMUSTEXIST,
								Filter);
	dlg.m_ofn.lpstrTitle = Title;

	if (IDOK != dlg.DoModal())
	{
		return;
	}
	FileName = dlg.GetPathName();
	m_Profile.ImportSection("Filter", FileName);
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
								"Fltr", NULL,
								OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT
								| OFN_EXPLORER | OFN_NONETWORKBUTTON | OFN_PATHMUSTEXIST,
								Filter);
	dlg.m_ofn.lpstrTitle = Title;

	if (IDOK != dlg.DoModal())
	{
		return;
	}
	FileName = dlg.GetPathName();
	m_Profile.ExportSection("Filter", FileName);
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
		if (GainDb > 0.)
		{
			GainDb = 0.;
		}
		switch (m_PointWithFocus)
		{
		case HpfNotchIndex:
			GainDb = -90.;
			break;
		case HpfStopbandIndex:
			if (GainDb > -10.)
			{
				GainDb = -10.;
			}
			break;
		case HpfPassbandIndex:
			break;
		case NotchBeginIndex:
			GainDb = -3.;
			break;
		case NotchEndIndex:
			GainDb = -3.;
			break;
		case LpfPassbandIndex:
			break;
		case LpfStopbandIndex:
			if (GainDb > -10.)
			{
				GainDb = -10.;
			}
			break;
		}
		SetPointGainDb(m_PointWithFocus, GainDb);
		DrawDotCaret(true);
		NotifyParentDlg();
		return;
		break;
	case VK_DOWN:
		// 1 dB down
		GainDb = ceil(20. * log10(m_Gain[m_PointWithFocus] / 1.00001)) - 1.;
		if (GainDb < -90.)
		{
			GainDb = -90.;
		}
		switch (m_PointWithFocus)
		{
		case HpfNotchIndex:
			GainDb = -90.;
			break;
		case HpfStopbandIndex:
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
		case NotchEndIndex:
			GainDb = -3.;
			break;
		case LpfPassbandIndex:
			if (GainDb < -6.)
			{
				GainDb = -6.;
			}
			break;
		case LpfStopbandIndex:
			break;
		}
		SetPointGainDb(m_PointWithFocus, GainDb);
		DrawDotCaret(true);
		NotifyParentDlg();
		return;
		break;
	case VK_LEFT:
		// Shift+Left - shift focused point
		// between left and right points must be at least 1 pixel

		// focus to the prev band
		if (CtrlPressed)
		{
			// Ctrl+Left - shift the whole transition band
			switch (m_PointWithFocus)
			{
			case HpfNotchIndex:
				break;
			case HpfStopbandIndex:
				break;
			case HpfPassbandIndex:
				break;
			case NotchBeginIndex:
				break;
			case NotchEndIndex:
				break;
			case LpfPassbandIndex:
				break;
			case LpfStopbandIndex:
				break;
			}
		}
		else if (ShiftPressed)
		{
			switch (m_PointWithFocus)
			{
			case HpfNotchIndex:
				break;
			case HpfStopbandIndex:
				break;
			case HpfPassbandIndex:
				break;
			case NotchBeginIndex:
				break;
			case NotchEndIndex:
				break;
			case LpfPassbandIndex:
				break;
			case LpfStopbandIndex:
				break;
			}
		}
		else
		{
			// find next visible point
			int NewFocusPoint = m_PointWithFocus;
			switch (m_PointWithFocus)
			{
			case HpfNotchIndex:
				// nothing
				return;
				break;
			case HpfStopbandIndex:
				NewFocusPoint = HpfNotchIndex;
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
			case NotchEndIndex:
				NewFocusPoint = NotchBeginIndex;
				break;
			case LpfPassbandIndex:
				if (m_bNotchFilter)
				{
					NewFocusPoint = NotchEndIndex;
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
		// focus to the next band
	{
		// find next visible point
		int NewFocusPoint = m_PointWithFocus;
		switch (m_PointWithFocus)
		{
		case HpfNotchIndex:
			NewFocusPoint = HpfStopbandIndex;
			break;
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
			NewFocusPoint = NotchEndIndex;
			break;
		case NotchEndIndex:
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
	int dx = GetSystemMetrics(SM_CXDRAG);
	int dy = GetSystemMetrics(SM_CYDRAG);
	for (int i = 0; i < MaxFilterFrequencies; i++)
	{
		// find if the mouse gets into a focus point
		int x = (1. + log10(m_Frequencies[i] / M_PI) / 3.) * cr.Width() - 1;
		// full range: 5 to -85 db
		int y = (0.25 - log10(m_Gain[i])) * cr.Height() / 4.5;
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

void CFilterGraphWnd::DrawDotCaret(bool state)
{
	if (m_DotCaretIsOn != state)
	{
		m_DotCaretIsOn = state;
		CRect cr;
		GetClientRect( & cr);

		int x = (1. + log10(m_Frequencies[m_PointWithFocus] / M_PI) / 3.) * cr.Width() - 1;
		// full range: 5 to -85 db
		int y = (0.25 - log10(m_Gain[m_PointWithFocus])) * cr.Height() / 4.5;

		int dx = GetSystemMetrics(SM_CXDRAG);
		int dy = GetSystemMetrics(SM_CYDRAG);
		CRect r(x - dx, y - dy, x + dx, y + dy);
		InvalidateRect( & r);
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

	CRect r;
	GetWindowRect( & r);
	m_DlgWidth = r.Width();
	m_DlgHeight = r.Height();

	CDialog::OnOK();
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
	double gainDb = 5. - point.y * 85. / cr.Height();
	SetPointGainDb(m_PointWithFocus, gainDb);
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
