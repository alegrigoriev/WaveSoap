// EqualizerDialog.cpp : implementation file
//

#include "stdafx.h"
#include "WaveSoapFront.h"
#include "EqualizerDialog.h"
#include "OperationDialogs.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define M_PI        3.14159265358979323846
/////////////////////////////////////////////////////////////////////////////
// CEqualizerDialog dialog


CEqualizerDialog::CEqualizerDialog(CWnd* pParent /*=NULL*/)
	: CDialog(CEqualizerDialog::IDD, pParent)
{
	m_PrevSize.cx = -1;
	m_PrevSize.cy = -1;
	//{{AFX_DATA_INIT(CEqualizerDialog)
	m_bUndo = FALSE;
	m_bMultiBandEqualizer = -1;
	m_nBands = 0;
	//}}AFX_DATA_INIT
	memset(& m_mmxi, 0, sizeof m_mmxi);
	m_Profile.AddItem("Equalizer", "NumberOfBands", m_nBands,
					10, 3, MaxNumberOfEqualizerBands);
	m_Profile.AddBoolItem("Equalizer", "MultiBandEqualizer", m_bMultiBandEqualizer, TRUE);
	for (int n = 0; n < MaxNumberOfEqualizerBands; n++)
	{
		CString s;
		s.Format("Gain%d", n + 1);
		m_Profile.AddItem("Equalizer", s, m_wGraph.m_BandGain[n], 1., 0.1, 10.);
	}
}


void CEqualizerDialog::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CEqualizerDialog)
	DDX_Control(pDX, IDC_EDIT_BAND_GAIN, m_BandGain);
	DDX_Control(pDX, IDC_SPIN_BANDS, m_SpinBands);
	DDX_Control(pDX, IDC_STATIC_SELECTION, m_SelectionStatic);
	DDX_Check(pDX, IDC_CHECK_UNDO, m_bUndo);
	DDX_Radio(pDX, IDC_RADIO_EQUALIZER_TYPE, m_bMultiBandEqualizer);
	DDX_Text(pDX, IDC_EDIT_BANDS, m_nBands);
	//}}AFX_DATA_MAP
	DDV_MinMaxUInt(pDX, m_nBands, 3, MaxNumberOfEqualizerBands);
	if (pDX->m_bSaveAndValidate)
	{
		m_Profile.UnloadAll();
	}
}

BEGIN_MESSAGE_MAP(CEqualizerDialog, CDialog)
	//{{AFX_MSG_MAP(CEqualizerDialog)
	ON_WM_ERASEBKGND()
	ON_WM_NCHITTEST()
	ON_WM_SIZING()
	ON_WM_SIZE()
	ON_WM_GETMINMAXINFO()
	ON_BN_CLICKED(IDC_BUTTON_SELECTION, OnButtonSelection)
	ON_EN_CHANGE(IDC_EDIT_BANDS, OnChangeEditBands)
	ON_BN_CLICKED(IDC_BUTTON_LOAD, OnButtonLoad)
	ON_BN_CLICKED(IDC_BUTTON_RESET_BANDS, OnButtonResetBands)
	ON_BN_CLICKED(IDC_BUTTON_SAVE_AS, OnButtonSaveAs)
	ON_EN_KILLFOCUS(IDC_EDIT_BAND_GAIN, OnKillfocusEditBandGain)
	//}}AFX_MSG_MAP
	ON_NOTIFY(NM_RETURN, AFX_IDW_PANE_FIRST, OnNotifyGraph)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CEqualizerDialog message handlers

BOOL CEqualizerDialog::OnEraseBkgnd(CDC* pDC)
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

UINT CEqualizerDialog::OnNcHitTest(CPoint point)
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

void CEqualizerDialog::OnSizing(UINT fwSide, LPRECT pRect)
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

void CEqualizerDialog::OnSize(UINT nType, int cx, int cy)
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
			IDC_RADIO_EQUALIZER_TYPE, IDC_RADIO2, IDC_EDIT_BANDS,
			IDC_SPIN_BANDS, IDC_STATIC3, IDC_CHECK_UNDO,
			IDC_STATIC_SELECTION,
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
			IDC_BUTTON_LOAD, IDC_BUTTON_SELECTION, IDOK, IDCANCEL, IDHELP,
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

void CEqualizerDialog::OnGetMinMaxInfo(MINMAXINFO FAR* lpMMI)
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

void CEqualizerDialog::OnMetricsChange()
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


BOOL CEqualizerDialog::OnInitDialog()
{
	m_wGraph.SetNumberOfBands(m_nBands);
	m_wGraph.m_SamplingRate = m_pWf->nSamplesPerSec;
	CRect r;
	CWnd * pTemplateWnd = GetDlgItem(IDC_STATIC_RESPONSE_TEMPLATE);
	pTemplateWnd->GetWindowRect( & r);
	ScreenToClient( & r);
	m_wGraph.Create(NULL, "", WS_CHILD | WS_VISIBLE
					| WS_CLIPSIBLINGS | WS_TABSTOP,
					r, this, AFX_IDW_PANE_FIRST);
	m_wGraph.SetWindowPos(pTemplateWnd, 0, 0, 0, 0,
						SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOSIZE);
	pTemplateWnd->DestroyWindow();

	CDialog::OnInitDialog();

	m_BandGain.SetData(m_wGraph.GetCurrentBandGainDb());
	// init MINMAXINFO
	OnMetricsChange();
	UpdateSelectionStatic();

	m_SpinBands.SetRange(3, MaxNumberOfEqualizerBands);

	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

void CEqualizerDialog::UpdateSelectionStatic()
{
	m_SelectionStatic.SetWindowText(GetSelectionText(m_Start, m_End, m_Chan,
													m_pWf->nChannels, m_bLockChannels,
													m_pWf->nSamplesPerSec, m_TimeFormat));
}


void CEqualizerDialog::OnButtonSelection()
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
/////////////////////////////////////////////////////////////////////////////
// CEqualizerGraphWnd

CEqualizerGraphWnd::CEqualizerGraphWnd()
{
	m_bMouseCaptured = false;
	m_bButtonPressed = false;
	m_bGotFocus = false;
	m_BandWithFocus = 0;
}

CEqualizerGraphWnd::~CEqualizerGraphWnd()
{
}

Equalizer::Equalizer()
{
	m_NumOfBands = MaxNumberOfEqualizerBands;
	ResetBands();
}

void Equalizer::ResetBands()
{
	for (int i = 0; i < MaxNumberOfEqualizerBands; i++)
	{
		m_BandGain[i] = 1;
		m_BandCoefficients[i][0] = 1.;
		m_BandCoefficients[i][1] = 0.;
		m_BandCoefficients[i][2] = 0.;
		m_BandCoefficients[i][3] = 1.;
		m_BandCoefficients[i][4] = 0.;
		m_BandCoefficients[i][5] = 0.;
		m_BandFrequencies[i] = M_PI / 500 * pow(500., i * ( 1. / (MaxNumberOfEqualizerBands - 0.5)));
	}
}

typedef void (CWnd::*NcPaintFunc)(UINT);
BEGIN_MESSAGE_MAP(CEqualizerGraphWnd, CWnd)
	//{{AFX_MSG_MAP(CEqualizerGraphWnd)
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
	//}}AFX_MSG_MAP
{ WM_NCPAINT, 0, 0, 0, AfxSig_vw,
	(AFX_PMSG)(AFX_PMSGW)
	static_cast< NcPaintFunc >(OnNcPaint)
},
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CEqualizerGraphWnd message handlers

void CEqualizerGraphWnd::OnPaint()
{
	CPaintDC dc(this); // device context for painting

	CRect cr;
	CRect ur;
	GetClientRect( & cr);
	dc.GetClipBox( & ur);
	dc.SetMapMode(MM_TEXT);
	CGdiObject * pOldPen = dc.SelectStockObject(BLACK_PEN);
	CGdiObject * pOldBrush = dc.SelectStockObject(NULL_BRUSH);

	double coeff = M_PI / pow(500., m_NumOfBands / (m_NumOfBands - 0.5));
	for (int x = ur.left - 1; x < ur.right; x++)
	{
		double f = coeff * pow(500., m_NumOfBands * (x + 1) /((m_NumOfBands - 0.5) * cr.Width()));
		double gain = abs(CalculateResponse(f));
		int y = (1 - log10(gain)) * cr.Height() / 2;
		if (x == ur.left - 1)
		{
			dc.MoveTo(x, y);
		}
		else
		{
			dc.LineTo(x, y);
		}
	}
	int dx = GetSystemMetrics(SM_CXDRAG);
	int dy = GetSystemMetrics(SM_CYDRAG);

	for (int i = 0; i < m_NumOfBands; i++)
	{
		// draw circles around the reference points
		int x = cr.Width() * (i * 2 + 1) / (2 * m_NumOfBands);
		int y = (1 - log10(m_BandGain[i])) * cr.Height() / 2;
		CRect r(x - dx, y - dy, x + dx, y + dy);

		if (m_DotCaretIsOn && i == m_BandWithFocus)
		{
			dc.SelectStockObject(BLACK_BRUSH);
		}
		else
		{
			dc.SelectStockObject(NULL_BRUSH);
		}
		dc.Ellipse(& r);
	}
	dc.SelectObject(pOldBrush);
	dc.SelectObject(pOldPen);
}

void CEqualizerGraphWnd::OnMouseMove(UINT nFlags, CPoint point)
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
		double gain = pow(10., (cr.Height() - point.y) * 2. / cr.Height() - 1.);
		SetBandGain(m_BandWithFocus, gain);
		NotifyParentDlg();
	}
}

void CEqualizerGraphWnd::NotifyParentDlg()
{
	NMHDR nmhdr;
	nmhdr.hwndFrom = m_hWnd;
	nmhdr.code = NM_RETURN;
	nmhdr.idFrom = GetDlgCtrlID();
	GetParent()->SendMessage(WM_NOTIFY, nmhdr.idFrom, (LPARAM) & nmhdr);
}

void CEqualizerGraphWnd::SetFocusBand(int nBand)
{
	if (nBand != m_BandWithFocus)
	{
		DrawDotCaret(false);

		m_BandWithFocus = nBand;
	}
	DrawDotCaret(true);
}

void CEqualizerGraphWnd::OnLButtonDown(UINT nFlags, CPoint point)
{

	CWnd::OnLButtonDown(nFlags, point);
	int nHit = GetHitCode(point);
	if (nHit < 0)
	{
		return;
	}
	m_bButtonPressed = true;
	DrawDotCaret(true);
	SetFocusBand(nHit);
	NotifyParentDlg();
}

void CEqualizerGraphWnd::OnLButtonUp(UINT nFlags, CPoint point)
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

BOOL CEqualizerGraphWnd::PreCreateWindow(CREATESTRUCT& cs)
{
	cs.lpszClass = AfxRegisterWndClass(CS_VREDRAW | CS_HREDRAW | CS_DBLCLKS, NULL,
										NULL, NULL);

	return CWnd::PreCreateWindow(cs);
}

// frequency is in radians
complex<float> Equalizer::CalculateResponse(double Frequency)
{
	complex<double> Numerator(1., 0.);
	complex<double> Denominator(1., 0.);

	complex<double> z(cos(Frequency), -sin(Frequency));
	if (m_NumOfBands > 2)
	{
		complex<double> z2(cos(Frequency * 2), -sin(Frequency * 2));

		for (int i = 0; i < m_NumOfBands; i++)
		{
			Numerator *= m_BandCoefficients[i][0] + z * m_BandCoefficients[i][1]
						+ z2 * m_BandCoefficients[i][2];
			Denominator *= m_BandCoefficients[i][3] + z * m_BandCoefficients[i][4]
							+ z2 * m_BandCoefficients[i][5];
		}
		return Numerator / Denominator;
	}
	else
	{
		return (m_BandCoefficients[0][0] + z * m_BandCoefficients[0][1]) *
			(m_BandCoefficients[1][0] + z * m_BandCoefficients[1][1])
			/ ((m_BandCoefficients[0][3] + z * m_BandCoefficients[0][4]) *
				(m_BandCoefficients[1][3] + z * m_BandCoefficients[1][4]));
	}
}

void CalculateCoefficients(double Gain, double Frequency, double Width, double Coeffs[6])
{
	// given the pole/zero quality, calculate them
	// on frequency at F - f/Q, gain should get down to sqrt(gain)
	// first calculate for pole and zero on X axis, then turn them to the required frequency
	if (1. == Gain)
	{
		Coeffs[0] = 1.;
		Coeffs[1] = 0.;
		Coeffs[2] = 0.;
		Coeffs[3] = 1.;
		Coeffs[4] = 0.;
		Coeffs[5] = 0.;
	}
	else
	{
		double Gain2 = sqrt(Gain);
		double df = Frequency * (1. - 1. / Width);
		if (0) TRACE("SetBandGain G=%f, freq=%f, df=%f\n",
					Gain, Frequency, df);

		complex<double> Z1(cos(df), sin(df));

		// use Z-transform
		double s_pole = -df * sqrt((1.- Gain2*Gain2)/(Gain2*Gain2 - Gain*Gain));
		double s_zero = -s_pole * Gain;
		double z_pole = exp(s_pole);
		double z_zero = exp(s_zero);
		if (0) TRACE("s_pole = %f, s_zero = %f, z_pole = %f, z_zero = %f\n",
					s_pole, s_zero, z_pole, z_zero);
		// rotate the poles to the necessary position and normalize
		double NormCoeff = (1. + z_pole) / (1. + z_zero);
		NormCoeff *= NormCoeff;

		double cos_f = cos(Frequency);

		Coeffs[0] = NormCoeff;
		Coeffs[1] = -2. * NormCoeff * z_zero * cos_f;
		Coeffs[2] = NormCoeff * z_zero * z_zero;
		Coeffs[3] = 1.;
		Coeffs[4] = -2. * z_pole * cos_f;
		Coeffs[5] = z_pole * z_pole;
		if (0) TRACE("Coeffs= %f, %f, %f; %f, %f, %f\n",
					Coeffs[0],
					Coeffs[1],
					Coeffs[2],
					Coeffs[3],
					Coeffs[4],
					Coeffs[5]);
	}
}
void CEqualizerGraphWnd::SetBandGain(int nBand, double Gain)
{
	if (m_BandGain[nBand] == Gain)
	{
		return;
	}
	m_BandGain[nBand] = Gain;
	RebuildBandFilters();
	Invalidate();
}

void Equalizer::RebuildBandFilters()
{
	// compensate band interference.
	// Use Newton approximation.
	// calculate derivatives, solve system of equations,
	// to find the necessary band coefficients
	// Make frequency responce error down to 0.1 dB
	int i;
	// initial approximation
	for (i = 0; i < m_NumOfBands; i++)
	{
		m_UsedBandGain[i] = m_BandGain[i];
	}
	for (int iter = 0; iter < 10; iter++)
	{
		// reference coefficients
		for (i = 0; i < m_NumOfBands; i++)
		{
			CalculateCoefficients(m_UsedBandGain[i],
								m_BandFrequencies[i], m_BandWidth, m_BandCoefficients[i]);
		}
		double Gain1[MaxNumberOfEqualizerBands];   // real gain for m_UsedBandGain
		double MaxError = 1.;
		// calculate reference response and max response error
		for (i = 0; i < m_NumOfBands; i++)
		{
			Gain1[i] = abs(CalculateResponse(m_BandFrequencies[i]));
			double Error = Gain1[i] / m_BandGain[i];
			if (Error < 1.) Error = 1./Error;
			if (Error > MaxError)
			{
				MaxError = Error;
			}
		}
		if (MaxError < 1.0116)  // .1 dB
		{
			break;
		}
		double M[MaxNumberOfEqualizerBands][MaxNumberOfEqualizerBands + 1];
		// calculate all derivatives
		for (i = 0; i < m_NumOfBands; i++)
		{
			CalculateCoefficients(m_UsedBandGain[i] * 1.06, // 0.5dB
								m_BandFrequencies[i], m_BandWidth, m_BandCoefficients[i]);

			for (int j = 0; j < m_NumOfBands; j++)
			{
				M[j][i] = log(abs(CalculateResponse(m_BandFrequencies[j])) / Gain1[j])
						/ 0.05827; // log(1.06)
				if (0) TRACE("Deriv[%d][%d]=%f\n", j, i, M[j][i]);
			}
			// return back
			CalculateCoefficients(m_UsedBandGain[i],
								m_BandFrequencies[i], m_BandWidth, m_BandCoefficients[i]);
			M[i][m_NumOfBands] = log(abs(CalculateResponse(m_BandFrequencies[i])) / m_BandGain[i]);
			if (0) TRACE("Error[%d]=%f\n", i, M[i][m_NumOfBands]);
		}
		// solve system of equations using Gauss reduce
		for (i = 0; i < m_NumOfBands - 1; i++)
		{
			for (int j = i + 1; j < m_NumOfBands; j++)
			{
				double d = M[j][i] / M[i][i];
				for (int k = i + 1; k < m_NumOfBands + 1; k++)
				{
					M[j][k] -= M[i][k] * d;
				}
#ifdef _DEBUG
				M[j][i] = 0.;
#endif
			}
		}
#ifdef _DEBUG
		for (i = 0; i < m_NumOfBands; i++)
		{
			for (int j = 0; j < m_NumOfBands + 1; j++)
			{
				if (0) TRACE("Reduced M[%d][%d]=%f\n", i, j, M[i][j]);
			}
		}
#endif
		// matrix reduced to triangular, calculate solution vector
		for (i = m_NumOfBands - 1; i >= 0; i--)
		{
			M[i][m_NumOfBands] /= M[i][i];
			for (int j = 0; j < i; j++)
			{
				M[j][m_NumOfBands] -= M[j][i] * M[i][m_NumOfBands];
			}
		}
		// the result is in M[*][m_NumOfBands + 1]
		for (i = 0; i < m_NumOfBands; i++)
		{
			if (0) TRACE("Correction at band %d=%f\n", i, M[i][m_NumOfBands]);
			m_UsedBandGain[i] *= exp(-M[i][m_NumOfBands]);
		}
	}

}

void Equalizer::SetNumberOfBands(int NumBands)
{
	m_NumOfBands = NumBands;
	TRACE("CEqualizerGraphWnd::SetNumberOfBands n=%d\n", NumBands);
	for (int i = 0; i < NumBands; i++)
	{
		m_BandFrequencies[i] = M_PI / 500 * pow(500., i * ( 1. / (NumBands - 0.5)));
		TRACE("Band frequency[%d] = %f\n", i, m_BandFrequencies[i] * 22050 / M_PI);
	}
	m_BandWidth = pow(500., 1. / (NumBands - 0.5));
	TRACE("Quality factor = %f\n", m_BandWidth);
	RebuildBandFilters();
}

void CEqualizerGraphWnd::SetNumberOfBands(int NumBands)
{
	Equalizer::SetNumberOfBands(NumBands);
	if (m_BandWithFocus > m_NumOfBands - 1)
	{
		m_BandWithFocus = m_NumOfBands - 1;
	}
	if (NULL != m_hWnd)
	{
		RedrawWindow(NULL, NULL, RDW_FRAME | RDW_INVALIDATE | RDW_ERASENOW);
	}
}

BOOL CEqualizerGraphWnd::OnEraseBkgnd(CDC* pDC)
{
	// white brush
	CGdiObject * pOldBrush = pDC->SelectStockObject(WHITE_BRUSH);
	CRect cr;
	pDC->GetClipBox( & cr);

	pDC->PatBlt(cr.left, cr.top, cr.Width(), cr.Height(), PATCOPY);

	GetClientRect( & cr);
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
		for (int i = 0; i < m_NumOfBands; i++)
		{
			int x = cr.Width() * (i * 2 + 1) / (2 * m_NumOfBands);
			pDC->PatBlt(x, cr.top, 1, cr.bottom - cr.top, PATINVERT);
		}
	}
	catch (CResourceException)
	{
		TRACE("CResourceException\n");
	}
	CPen DotPen(PS_DOT, 0, COLORREF(0));
	CPen * pOldPen = pDC->SelectObject( & DotPen);

	pDC->MoveTo(cr.left, cr.bottom / 4);
	pDC->LineTo(cr.right, cr.bottom / 4);

	pDC->MoveTo(cr.left, cr.bottom - cr.bottom / 4);
	pDC->LineTo(cr.right, cr.bottom - cr.bottom / 4);

	pDC->SelectObject(pOldPen);

	pDC->SelectObject(pOldBrush);
	return TRUE;
}

void CEqualizerGraphWnd::OnNcCalcSize(BOOL bCalcValidRects, NCCALCSIZE_PARAMS * lpncsp)
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

void CEqualizerGraphWnd::OnNcPaint(UINT wParam)
{
	CDC * pDC = GetDCEx(CRgn::FromHandle((HRGN)wParam),
						// DCX_CACHE is necessary!
						DCX_CACHE | DCX_WINDOW | DCX_INTERSECTRGN);
	if (NULL == pDC)
	{
		return;
	}
	CRect wr;
	GetWindowRect( & wr);
	NCCALCSIZE_PARAMS ncp;
	wr.right = wr.Width();
	wr.left = 0;
	wr.bottom = wr.Height();
	wr.top = 0;
	ncp.rgrc[0] = wr;
	//OnNcCalcSize(FALSE, & ncp);
	SendMessage(WM_NCCALCSIZE, FALSE, (LPARAM) & ncp);

	// Paint into this DC
	CBrush BkBrush;
	CWnd * pParentDlg = GetParent();
	BkBrush.Attach((HBRUSH) pParentDlg->SendMessage(WM_CTLCOLORDLG, (WPARAM)(pParentDlg->m_hWnd), LPARAM(pDC->m_hDC)));
	CGdiObject * pOldPen = pDC->SelectStockObject(BLACK_PEN);
	CGdiObject * pOldBrush = pDC->SelectObject( & BkBrush);
	// fill the NC area
	pDC->Rectangle( & wr);
	// Draw rectangle
	pDC->SelectStockObject(NULL_BRUSH);
	pDC->Rectangle(ncp.rgrc[0].left - 1, ncp.rgrc[0].top - 1,
					ncp.rgrc[0].right + 1, ncp.rgrc[0].bottom + 1);
	CGdiObject * pOldFont = pDC->SelectStockObject(ANSI_VAR_FONT);
	// draw frequencies
	pDC->SetTextAlign(TA_TOP | TA_CENTER);
	pDC->SetTextColor(0x000000);   // black
	pDC->SetBkMode(TRANSPARENT);

	int PrevX = ncp.rgrc[0].left;

	for (int i = 0; i < m_NumOfBands; i ++)
	{
		int x = ncp.rgrc[0].left +
				(ncp.rgrc[0].right - ncp.rgrc[0].left) * (i * 2 + 1) / (2 * m_NumOfBands);
		CString s;
		double f = m_BandFrequencies[i] / M_PI * 0.5 * m_SamplingRate;
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
				"20 dB", 5);

	pDC->TextOut(ncp.rgrc[0].left - 1,
				ncp.rgrc[0].top + (ncp.rgrc[0].bottom - ncp.rgrc[0].top) / 4 + TextHeight / 2,
				"10 dB", 5);

	pDC->TextOut(ncp.rgrc[0].left - 1,
				ncp.rgrc[0].top + (ncp.rgrc[0].bottom - ncp.rgrc[0].top) / 2 + TextHeight / 2,
				"0 dB", 4);

	pDC->TextOut(ncp.rgrc[0].left - 1,
				ncp.rgrc[0].bottom - (ncp.rgrc[0].bottom - ncp.rgrc[0].top) / 4 + TextHeight / 2,
				"-10 dB", 6);

	pDC->TextOut(ncp.rgrc[0].left - 1,
				ncp.rgrc[0].bottom + TextHeight / 2,
				"-20 dB", 6);

	pDC->SelectObject(pOldFont);
	pDC->SelectObject(pOldBrush);
	pDC->SelectObject(pOldPen);
	ReleaseDC(pDC);
}

void CEqualizerGraphWnd::OnCaptureChanged(CWnd *pWnd)
{
	m_bMouseCaptured = false;
	m_bButtonPressed = false;
	CWnd::OnCaptureChanged(pWnd);
}

int CEqualizerGraphWnd::OnMouseActivate(CWnd* pDesktopWnd, UINT nHitTest, UINT message)
{
	int nResult = CWnd::OnMouseActivate(pDesktopWnd, nHitTest, message);
	if (nResult == MA_NOACTIVATE || nResult == MA_NOACTIVATEANDEAT)
		return nResult;   // parent does not want to activate
	// grab focus
	SetFocus();
	return MA_NOACTIVATE;
}

void CEqualizerGraphWnd::OnKillFocus(CWnd* pNewWnd)
{
	CWnd::OnKillFocus(pNewWnd);

	if (m_bGotFocus)
	{
		m_bGotFocus = false;
		DrawDotCaret(true);
	}
}

void CEqualizerGraphWnd::OnSetFocus(CWnd* pOldWnd)
{
	CWnd::OnSetFocus(pOldWnd);

	m_bGotFocus = true;
	DrawDotCaret(true);

}

BOOL CEqualizerGraphWnd::OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message)
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

void CEqualizerDialog::OnChangeEditBands()
{
	BOOL nTrans = FALSE;
	int nBands = GetDlgItemInt(IDC_EDIT_BANDS, & nTrans, FALSE);
	if (nTrans && nBands > 1 && nBands <= MaxNumberOfEqualizerBands)
	{
		m_nBands = nBands;
		m_wGraph.SetNumberOfBands(nBands);
	}
}

void CEqualizerDialog::OnButtonResetBands()
{
	m_wGraph.ResetBands();
	m_wGraph.SetNumberOfBands(m_nBands);
}

void CEqualizerDialog::OnButtonLoad()
{
	// TODO: Add your control notification handler code here

}

void CEqualizerDialog::OnButtonSaveAs()
{
	// TODO: Add your control notification handler code here

}

void CEqualizerGraphWnd::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	if (! m_bGotFocus)
	{
		return;
	}
	double GainDb = floor(0.5 + 20. * log10(m_BandGain[m_BandWithFocus]));
	switch(nChar)
	{
	case VK_UP:
		// 1 dB up
		GainDb += 1;
		if (GainDb > 20.)
		{
			GainDb = 20.;
		}
		SetBandGain(m_BandWithFocus, pow(10., GainDb / 20.));
		DrawDotCaret(true);
		NotifyParentDlg();
		return;
		break;
	case VK_DOWN:
		// 1 dB down
		GainDb -= 1;
		if (GainDb < -20.)
		{
			GainDb = -20.;
		}
		SetBandGain(m_BandWithFocus, pow(10., GainDb / 20.));
		DrawDotCaret(true);
		NotifyParentDlg();
		return;
		break;
	case VK_LEFT:
		// focus to the prev band
		if (m_BandWithFocus > 0)
		{
			SetFocusBand(m_BandWithFocus - 1);
		}
		NotifyParentDlg();
		break;
	case VK_RIGHT:
		// focus to the next band
		if (m_BandWithFocus < m_NumOfBands - 1)
		{
			SetFocusBand(m_BandWithFocus + 1);
		}
		NotifyParentDlg();
		break;
	}
}

UINT CEqualizerGraphWnd::OnGetDlgCode()
{
	return DLGC_WANTARROWS;
}

int CEqualizerGraphWnd::GetHitCode(POINT point)
{
	CRect cr;
	GetClientRect( & cr);
	if ( ! cr.PtInRect(point))
	{
		return -2;
	}
	int dx = GetSystemMetrics(SM_CXDRAG);
	int dy = GetSystemMetrics(SM_CYDRAG);
	for (int i = 0; i < m_NumOfBands; i++)
	{
		// find if the mouse gets into a focus point
		int x = cr.Width() * (i * 2 + 1) / (2 * m_NumOfBands);
		int y = (1 - log10(m_BandGain[i])) * cr.Height() / 2;
		CRect r(x - dx, y - dy, x + dx, y + dy);

		if (r.PtInRect(point))
		{
			return i;
		}
	}
	return -1;
}

void CEqualizerGraphWnd::OnTimer(UINT nIDEvent)
{
	// redraw blinking dot
	if (m_bGotFocus && ! m_bButtonPressed)
	{
		DrawDotCaret( ! m_DotCaretIsOn);
	}
}

void CEqualizerGraphWnd::DrawDotCaret(bool state)
{
	if (m_DotCaretIsOn != state)
	{
		m_DotCaretIsOn = state;
		CRect cr;
		GetClientRect( & cr);
		int x = cr.Width() * (m_BandWithFocus * 2 + 1) / (2 * m_NumOfBands);
		int y = (1 - log10(m_BandGain[m_BandWithFocus])) * cr.Height() / 2;
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

void CEqualizerDialog::OnOK()
{
	int FocusId = GetFocus()->GetDlgCtrlID();
	if (IDC_EDIT_BAND_GAIN == FocusId)
	{
		// read the gain
		double GainDb;
		if (m_BandGain.GetData(NULL, GainDb, NULL, NULL, -20., 20.))
		{
			m_wGraph.SetCurrentBandGainDb(GainDb);
			// set focus to the graph
			m_wGraph.SetFocus();
		}
		return;
	}
	CDialog::OnOK();
}

void CEqualizerDialog::OnNotifyGraph( NMHDR * pNotifyStruct, LRESULT * result )
{
	m_BandGain.SetData(m_wGraph.GetCurrentBandGainDb());
}

void CEqualizerDialog::OnKillfocusEditBandGain()
{
	// read the gain
	double GainDb;
	if (m_BandGain.GetData(NULL, GainDb, NULL, NULL, -20., 20.))
	{
		m_wGraph.SetCurrentBandGainDb(GainDb);
	}
}
