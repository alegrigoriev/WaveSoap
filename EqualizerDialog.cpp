// Copyright Alexander Grigoriev, 1997-2002, All Rights Reserved
// EqualizerDialog.cpp : implementation file
//

#include "stdafx.h"
#include "WaveSoapFront.h"
#include "EqualizerDialog.h"
#include "OperationDialogs.h"
#include "FileDialogWithHistory.h"
#include "GdiObjectSave.h"
#include "DialogWithSelection.inl"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define M_PI        3.14159265358979323846
/////////////////////////////////////////////////////////////////////////////
// CEqualizerDialog dialog


CEqualizerDialog::CEqualizerDialog(SAMPLE_INDEX Start,
									SAMPLE_INDEX End,
									SAMPLE_INDEX CaretPosition,
									CHANNEL_MASK Channels,
									CWaveFile & WaveFile,
									int TimeFormat,
									BOOL bLockChannels,
									BOOL	bUndoEnabled,
									CWnd* pParent /*=NULL*/)
	: BaseClass(Start, End, CaretPosition, Channels, WaveFile, TimeFormat,
				IDD, pParent)
{
	m_bLockChannels = bLockChannels;
	m_bUndo = bUndoEnabled;
	//{{AFX_DATA_INIT(CEqualizerDialog)
	m_bMultiBandEqualizer = -1;
	m_nBands = 0;
	//}}AFX_DATA_INIT

	m_Profile.AddItem(_T("Equalizer"), _T("NumberOfBands"), m_nBands,
					10, 3, MaxNumberOfEqualizerBands);
	m_Profile.AddItem(_T("Settings"), _T("EqualizerDlgWidth"), m_DlgWidth, 0, 0, 4096);
	m_Profile.AddItem(_T("Settings"), _T("EqualizerDlgHeight"), m_DlgHeight, 0, 0, 4096);
	m_Profile.AddBoolItem(_T("Equalizer"), _T("MultiBandEqualizer"), m_bMultiBandEqualizer, TRUE);
	m_Profile.AddBoolItem(_T("Equalizer"), _T("ZeroPhase"), m_wGraph.m_bZeroPhase, FALSE);
	for (int n = 0; n < MaxNumberOfEqualizerBands; n++)
	{
		CString s;
		s.Format(_T("Gain%d"), n + 1);
		m_Profile.AddItem(_T("Equalizer"), s, m_wGraph.m_BandGain[n], 1., 0.1, 10.);
	}
	m_BandGain.SetPrecision(2);

	static ResizableDlgItem const ResizeItems[] =
	{
		{IDC_STATIC1, MoveDown},
		{IDC_EDIT_BAND_GAIN, MoveDown},
		{IDC_STATIC2, MoveDown},
		{IDC_CHECK_ZERO_PHASE, MoveDown},
		{IDC_RADIO_EQUALIZER_TYPE, MoveDown},
		{IDC_RADIO2, MoveDown},
		{IDC_EDIT_BANDS, MoveDown},
		{IDC_SPIN_BANDS, MoveDown},
		{IDC_STATIC3, MoveDown},
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

	SetResizeableItems(ResizeItems, countof(ResizeItems));

	m_wGraph.SetNumberOfBands(m_nBands);
	m_wGraph.m_SamplingRate = WaveFile.SampleRate();
}


void CEqualizerDialog::DoDataExchange(CDataExchange* pDX)
{
	BaseClass::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CEqualizerDialog)
	DDX_Control(pDX, IDC_EDIT_BANDS, m_eEditBands);
	DDX_Control(pDX, IDC_EDIT_BAND_GAIN, m_BandGain);
	DDX_Control(pDX, IDC_SPIN_BANDS, m_SpinBands);
	DDX_Check(pDX, IDC_CHECK_UNDO, m_bUndo);
	DDX_Radio(pDX, IDC_RADIO_EQUALIZER_TYPE, m_bMultiBandEqualizer);
	DDX_Text(pDX, IDC_EDIT_BANDS, m_nBands);
	//}}AFX_DATA_MAP
	DDX_Check(pDX, IDC_CHECK_ZERO_PHASE, m_wGraph.m_bZeroPhase);
	DDV_MinMaxUInt(pDX, m_nBands, 3, MaxNumberOfEqualizerBands);
}

BEGIN_MESSAGE_MAP(CEqualizerDialog, BaseClass)
	//{{AFX_MSG_MAP(CEqualizerDialog)
	ON_EN_CHANGE(IDC_EDIT_BANDS, OnChangeEditBands)
	ON_BN_CLICKED(IDC_BUTTON_LOAD, OnButtonLoad)
	ON_BN_CLICKED(IDC_BUTTON_RESET_BANDS, OnButtonResetBands)
	ON_BN_CLICKED(IDC_BUTTON_SAVE_AS, OnButtonSaveAs)
	ON_EN_KILLFOCUS(IDC_EDIT_BAND_GAIN, OnKillfocusEditBandGain)
	ON_BN_CLICKED(IDC_RADIO_EQUALIZER_TYPE, OnRadioEqualizerType)
	ON_BN_CLICKED(IDC_RADIO2, OnRadio2)
	ON_BN_CLICKED(IDC_CHECK_ZERO_PHASE, OnCheckZeroPhase)
	//}}AFX_MSG_MAP
	ON_NOTIFY(NM_RETURN, AFX_IDW_PANE_FIRST, OnNotifyGraph)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CEqualizerDialog message handlers

BOOL CEqualizerDialog::OnInitDialog()
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

	m_BandGain.SetData(m_wGraph.GetCurrentBandGainDb());
	// init MINMAXINFO

	m_SpinBands.SetRange(3, MaxNumberOfEqualizerBands);
	if (m_bMultiBandEqualizer)
	{
		OnRadio2();
	}
	else
	{
		OnRadioEqualizerType();
	}

	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

/////////////////////////////////////////////////////////////////////////////
// CEqualizerGraphWnd

CEqualizerGraphWnd::CEqualizerGraphWnd()
{
	m_bMouseCaptured = false;
	m_bButtonPressed = false;
	m_bGotFocus = false;
	m_MultiBandEqualizer = true;
	m_BandWithFocus = 0;
}

CEqualizerGraphWnd::~CEqualizerGraphWnd()
{
}

Equalizer::Equalizer()
{
	m_NumOfFilters = MaxNumberOfEqualizerBands;
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
	ON_WM_LBUTTONDBLCLK()
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
	if (0) TRACE("CEqualizerGraphWnd::OnPaint\n");
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
	double coeff = M_PI / pow(500., m_NumOfBands / (m_NumOfBands - 0.5));
	for (int x = ur.left; x < ur.right; x++)
	{
		double f;
		if (m_NumOfBands > 2)
		{
			f = coeff * pow(500., m_NumOfBands * (x + 1) /((m_NumOfBands - 0.5) * cr.Width()));
		}
		else
		{
			f = M_PI * pow(500., (x + 1. - cr.Width()) / cr.Width());
		}
		double gain = abs(CalculateResponse(f));
		int y = int((1 - log10(gain)) * cr.Height() / 2);
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

	CGdiObjectSaveT<CBitmap> OldBitmap( & cdc, cdc.SelectObject( & bmp));

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

	for (int i = 0; i < m_NumOfBands; i++)
	{
		// draw circles around the reference points
		int x;
		if (m_NumOfBands <= 2)
		{
			x = cr.Width() * (i * 38 + 1) / 40;
		}
		else
		{
			x = cr.Width() * (i * 2 + 1) / (2 * m_NumOfBands);
		}
		int y = int((1 - log10(m_BandGain[i])) * cr.Height() / 2);
		int SrcOffset = 0;
		if (m_DotCaretIsOn && i == m_BandWithFocus)
		{
			SrcOffset = w;
		}
		dc.BitBlt(x - dx, y - dy, w, h, & cdc, SrcOffset, 0, SRCAND);
	}

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
using std::complex;

complex<float> Equalizer::CalculateResponse(double Frequency)
{
	complex<double> Numerator(1., 0.);
	complex<double> Denominator(1., 0.);

	complex<double> z(cos(Frequency), -sin(Frequency));
	complex<double> z2(cos(Frequency * 2), -sin(Frequency * 2));

	for (int i = 0; i < m_NumOfFilters; i++)
	{
		Numerator *= m_BandCoefficients[i][0] + z * m_BandCoefficients[i][1]
					+ z2 * m_BandCoefficients[i][2];
		Denominator *= m_BandCoefficients[i][3] + z * m_BandCoefficients[i][4]
						+ z2 * m_BandCoefficients[i][5];
	}
	Numerator /= Denominator;
	if (m_bZeroPhase)
	{
		// filter is applied twice
		Numerator *= conj(Numerator);
	}
	return complex<float>(Numerator);
}

void Equalizer::CalculateCoefficients(double Gain, double Frequency, double Width, double Coeffs[6])
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
		if (m_bZeroPhase)
		{
			Gain = sqrt(Gain);
		}
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

void CalculateSimpleEqCoefficients(double f1, double f2,
									double Gain1, double Gain2, double Coeffs[6])
{
	// f1 - frequency with gain 1,
	// f2 - frequency with gain 2,
	// F1 < f2
	// Gain - ><1
	// for cutoff frequency F, the pole is (1 - sin(F))/cos(F). if (cos(f) == 0,
	// the pole is 0.
	double sin1 = sin(f1);
	double cos1 = cos(f1);
	double pole1;
	if (cos1 != 0)
	{
		pole1 = (1 - sin1) / cos1;
	}
	else
	{
		pole1 = 0;
	}
	double sin2 = sin(f2);
	double cos2 = cos(f2);
	double pole2;
	if (cos2 != 0)
	{
		pole2 = (1 - sin2) / cos2;
	}
	else
	{
		pole2 = 0;
	}
	TRACE("Target F1=%f, gain1 = %f, pole=%f\n"
		"Target F2=%f, gain2 = %f, pole=%f\n",
		f1, Gain1, pole1,
		f2, Gain2, pole2);
	Coeffs[3] = 1.;
	Coeffs[4] = -(pole1 + pole2);
	Coeffs[5] = pole1 * pole2;
	// gain1 corresponds to gain on Z==1
	// norm it
	Gain1 *= (1. - pole1) / 2;
	// gain2 corresponds to gain on Z==-1
	// norm it
	Gain2 *= (1. + pole2) / 2;
	Coeffs[0] = Gain1 + Gain2;
	Coeffs[1] = Gain1 - Gain2 - Gain1 * pole2 - Gain2 * pole1;
	Coeffs[2] = Gain2 * pole1 - Gain1 * pole2;
#ifdef _DEBUG
	{
		double d2 = Coeffs[1] * Coeffs[1] - 4. * Coeffs[0] * Coeffs[2];
		if (d2 >= 0)
		{
			double d = sqrt(d2);
			double r1 = (-Coeffs[1] + d) / (2. * Coeffs[0]);
			double r2 = (-Coeffs[1] - d) / (2. * Coeffs[0]);
			TRACE("Real zeros: %f, %f\n", r1, r2);
		}
		else
		{
			double d = sqrt(-d2) / (2. * Coeffs[0]);
			double r1 = -Coeffs[1] / (2. * Coeffs[0]);
			TRACE("Complex zeros: %f +- j*%f\n", r1, d);
		}
	}
#endif

	TRACE("Calculated coefficients:\n"
		"%f, %f, %f,\n"
		"%f, %f, %f\n",
		Coeffs[0], Coeffs[1], Coeffs[2],
		Coeffs[3], Coeffs[4], Coeffs[5]);
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
	int i;
	// initial approximation
	for (i = 0; i < m_NumOfFilters; i++)
	{
		m_UsedBandGain[i] = m_BandGain[i];
	}
	if (2 == m_NumOfFilters)
	{
		// simple equalizer
		double ratio;
		double gain = m_BandGain[0];
		ASSERT(gain > 0.);
		if (gain < 1)
		{
			ratio = sqrt(2 * gain);
			gain = gain;
		}
		else
		{
			ratio = sqrt(2 / gain);
			gain = gain;
		}
		// f2 = 500
		if (1. == m_BandGain[0])
		{
			m_BandCoefficients[0][0] = 1.;
			m_BandCoefficients[0][1] = 0.;
			m_BandCoefficients[0][2] = 0.;
			m_BandCoefficients[0][3] = 1.;
			m_BandCoefficients[0][4] = 0.;
			m_BandCoefficients[0][5] = 0.;
		}
		else
		{
			CalculateSimpleEqCoefficients(0.07 * ratio, 0.07 / ratio, gain, 1., m_BandCoefficients[0]);
		}
		gain = m_BandGain[1];
		ASSERT(gain > 0.);
		if (gain > 1)
		{
			ratio = 0.5 * gain;
			gain = -gain;
		}
		else
		{
			ratio = 0.707 / gain;
			gain = -gain;
		}
		// f1= 2000
		if (1. == m_BandGain[1])
		{
			m_BandCoefficients[1][0] = 1.;
			m_BandCoefficients[1][1] = 0.;
			m_BandCoefficients[1][2] = 0.;
			m_BandCoefficients[1][3] = 1.;
			m_BandCoefficients[1][4] = 0.;
			m_BandCoefficients[1][5] = 0.;
		}
		else
		{
			CalculateSimpleEqCoefficients(0.4, 0.4 * ratio, 1., gain, m_BandCoefficients[1]);
		}
		return;
	}

	// compensate band interference.
	// Use Newton approximation.
	// calculate derivatives, solve system of equations,
	// to find the necessary band coefficients
	// Make frequency responce error down to 0.1 dB
	for (int iter = 0; iter < 10; iter++)
	{
		// reference coefficients
		for (i = 0; i < m_NumOfFilters; i++)
		{
			CalculateCoefficients(m_UsedBandGain[i],
								m_BandFrequencies[i], m_BandWidth, m_BandCoefficients[i]);
		}
		double Gain1[MaxNumberOfEqualizerBands];   // real gain for m_UsedBandGain
		double MaxError = 1.;
		// calculate reference response and max response error
		for (i = 0; i < m_NumOfFilters; i++)
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
		for (i = 0; i < m_NumOfFilters; i++)
		{
			CalculateCoefficients(m_UsedBandGain[i] * 1.06, // 0.5dB
								m_BandFrequencies[i], m_BandWidth, m_BandCoefficients[i]);

			for (int j = 0; j < m_NumOfFilters; j++)
			{
				M[j][i] = log(abs(CalculateResponse(m_BandFrequencies[j])) / Gain1[j])
						/ 0.05827; // log(1.06)
				if (0) TRACE("Deriv[%d][%d]=%f\n", j, i, M[j][i]);
			}
			// return back
			CalculateCoefficients(m_UsedBandGain[i],
								m_BandFrequencies[i], m_BandWidth, m_BandCoefficients[i]);
			M[i][m_NumOfFilters] = log(abs(CalculateResponse(m_BandFrequencies[i])) / m_BandGain[i]);
			if (0) TRACE("Error[%d]=%f\n", i, M[i][m_NumOfFilters]);
		}
		// solve system of equations using Gauss reduce
		for (i = 0; i < m_NumOfFilters - 1; i++)
		{
			for (int j = i + 1; j < m_NumOfFilters; j++)
			{
				double d = M[j][i] / M[i][i];
				for (int k = i + 1; k < m_NumOfFilters + 1; k++)
				{
					M[j][k] -= M[i][k] * d;
				}
#ifdef _DEBUG
				M[j][i] = 0.;
#endif
			}
		}
#ifdef _DEBUG
		for (i = 0; i < m_NumOfFilters; i++)
		{
			for (int j = 0; j < m_NumOfFilters + 1; j++)
			{
				if (0) TRACE("Reduced M[%d][%d]=%f\n", i, j, M[i][j]);
			}
		}
#endif
		// matrix reduced to triangular, calculate solution vector
		for (i = m_NumOfFilters - 1; i >= 0; i--)
		{
			M[i][m_NumOfFilters] /= M[i][i];
			for (int j = 0; j < i; j++)
			{
				M[j][m_NumOfFilters] -= M[j][i] * M[i][m_NumOfFilters];
			}
		}
		// the result is in M[*][m_NumOfBands + 1]
		for (i = 0; i < m_NumOfFilters; i++)
		{
			if (0) TRACE("Correction at band %d=%f\n", i, M[i][m_NumOfFilters]);
			m_UsedBandGain[i] *= exp(-M[i][m_NumOfFilters]);
		}
	}

}

void Equalizer::SetNumberOfBands(int NumBands)
{
	m_NumOfFilters = NumBands;
	TRACE("CEqualizerGraphWnd::SetNumberOfBands n=%d\n", NumBands);
	for (int i = 0; i < NumBands; i++)
	{
		m_BandFrequencies[i] = M_PI / 500 * pow(500., i * ( 1. / (NumBands - 0.5)));
		TRACE("Band frequency[%d] = %f\n", i, m_BandFrequencies[i] * 22050 / M_PI);
	}
	if (m_bZeroPhase)
	{
		m_BandWidth = pow(500., .8 / (NumBands - 0.5));
	}
	else
	{
		m_BandWidth = pow(500., 1. / (NumBands - 0.5));
	}
	TRACE("Quality factor = %f\n", m_BandWidth);
	RebuildBandFilters();
}

void CEqualizerGraphWnd::SetNumberOfBands(int NumBands)
{
	if (NumBands == 2)
	{
		// simple equalizer
		Equalizer::SetNumberOfBands(3);
		SetBandGain(2, 1.);
		m_BandFrequencies[0] = 0.0073685;//M_PI / 500 * pow(500., 0.5 / 19.5);
		m_BandFrequencies[1] = 2.6788322; //M_PI / 500 * pow(500., 19. / 19.5);
		m_BandFrequencies[2] = 0.1413716; //M_PI * 0.045;
		m_MultiBandEqualizer = false;
		m_BandWidth = 20.;
		RebuildBandFilters();
	}
	else
	{
		Equalizer::SetNumberOfBands(NumBands);
		m_MultiBandEqualizer = true;
	}
	m_NumOfBands = NumBands;
	if (m_BandWithFocus > m_NumOfBands - 1)
	{
		m_BandWithFocus = m_NumOfBands - 1;
	}
	if (NULL != m_hWnd)
	{
		RedrawWindow(NULL, NULL, RDW_FRAME | RDW_ERASE | RDW_INVALIDATE | RDW_ERASENOW);
	}
}

BOOL CEqualizerGraphWnd::OnEraseBkgnd(CDC* pDC)
{
	// white brush
	CGdiObjectSave OldBrush(pDC, pDC->SelectStockObject(WHITE_BRUSH));

	CRect cr;
	GetClientRect( & cr);
	if (0) TRACE("CEqualizerGraphWnd::OnEraseBkgnd, cr=%d, %d, %d, %d\n",
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
		int nNumOfFrequencies = m_NumOfBands;
		if (nNumOfFrequencies <= 2)
		{
			nNumOfFrequencies = 20;
		}
		for (int i = 0; i < nNumOfFrequencies; i++)
		{
			int x = cr.Width() * (i * 2 + 1) / (2 * nNumOfFrequencies);
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

	pDC->MoveTo(cr.left, cr.bottom / 4);
	pDC->LineTo(cr.right, cr.bottom / 4);

	pDC->MoveTo(cr.left, cr.bottom - cr.bottom / 4);
	pDC->LineTo(cr.right, cr.bottom - cr.bottom / 4);

	return TRUE;
}

void CEqualizerGraphWnd::OnNcCalcSize(BOOL /*bCalcValidRects*/,
									NCCALCSIZE_PARAMS * lpncsp)
{
	int ncHeight = 2 + GetSystemMetrics(SM_CYMENUSIZE);
	int ncWidth;

	CWindowDC wDC(GetDesktopWindow());
	CGdiObjectSave Old(wDC, wDC.SelectStockObject(ANSI_VAR_FONT));

	ncWidth = 3 + wDC.GetTextExtent(_T("-20 dB"), 6).cx;

	lpncsp->rgrc[0].left += ncWidth;
	lpncsp->rgrc[0].right -= GetSystemMetrics(SM_CXSIZEFRAME);
	lpncsp->rgrc[0].top += GetSystemMetrics(SM_CYSIZEFRAME);
	lpncsp->rgrc[0].bottom -= ncHeight /* + GetSystemMetrics(SM_CXSIZEFRAME) */;

}

void CEqualizerGraphWnd::OnNcPaint(UINT wParam)
{
	if (0) TRACE("CEqualizerGraphWnd::OnNcPaint, hrgn=%X\n", wParam);
	// copy region, because it will be deleted
	try
	{
		CRect wr;
		GetWindowRect( & wr);

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

		//CWnd * pParentDlg = GetParent();
		BkBrush.Attach(GetSysColorBrush(COLOR_BTNFACE));
		//BkBrush.Attach((HBRUSH) pParentDlg->SendMessage(WM_CTLCOLORDLG, (WPARAM)(pParentDlg->m_hWnd), LPARAM(wDC.m_hDC)));

		CGdiObjectSave OldPen(wDC, wDC.SelectStockObject(BLACK_PEN));
		CGdiObjectSave OldBrush(wDC, wDC.SelectObject( & BkBrush));

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

		int nNumOfFrequencies = m_NumOfBands;
		if (nNumOfFrequencies <= 2)
		{
			nNumOfFrequencies = 20;
		}
		for (int i = 0; i < nNumOfFrequencies; i ++)
		{
			int x = ncp.rgrc[0].left +
					(ncp.rgrc[0].right - ncp.rgrc[0].left) * (i * 2 + 1) / (2 * nNumOfFrequencies);
			CString s;
			double f;
			if (m_NumOfBands > 2)
			{
				f = m_BandFrequencies[i] / M_PI * 0.5 * m_SamplingRate;
			}
			else
			{
				f =  0.5 * m_SamplingRate * pow(500.,
												(x + 1. - ncp.rgrc[0].right) /
												(ncp.rgrc[0].right - ncp.rgrc[0].left));
			}
			int TextWidth;
			if (f >= 1000.)
			{
				// 12.1k
				// 8.9k
				s.Format(_T("%.1fk"), f / 1000.);
				if (f >= 10000.)
				{
					TextWidth = wDC.GetTextExtent(_T("20.0k "), 7).cx;
				}
				else
				{
					TextWidth = wDC.GetTextExtent(_T("0.0k "), 7).cx;
				}
			}
			else if (f >= 100.)
			{
				// 550 (round to 10)
				s.Format(_T("%d"), 10 * ((int(f) + 5) / 10));
				TextWidth = wDC.GetTextExtent(_T("999 "), 7).cx;
			}
			else
			{
				// 55
				s.Format(_T("%d"), int(f));
				TextWidth = wDC.GetTextExtent(_T("99 "), 7).cx;
			}
			if (x - TextWidth / 2 >= PrevX)
			{
				wDC.TextOut(x, ncp.rgrc[0].bottom - 1, s);
				PrevX = x + TextWidth / 2;
			}
		}

		wDC.SetTextAlign(TA_BOTTOM | TA_RIGHT);
		int TextHeight = wDC.GetTextExtent(_T("9"), 1).cy;

		wDC.TextOut(ncp.rgrc[0].left - 1,
					ncp.rgrc[0].top + TextHeight,
					_T("20 dB"), 5);

		wDC.TextOut(ncp.rgrc[0].left - 1,
					ncp.rgrc[0].top + (ncp.rgrc[0].bottom - ncp.rgrc[0].top) / 4 + TextHeight / 2,
					_T("10 dB"), 5);

		wDC.TextOut(ncp.rgrc[0].left - 1,
					ncp.rgrc[0].top + (ncp.rgrc[0].bottom - ncp.rgrc[0].top) / 2 + TextHeight / 2,
					_T("0 dB"), 4);

		wDC.TextOut(ncp.rgrc[0].left - 1,
					ncp.rgrc[0].bottom - (ncp.rgrc[0].bottom - ncp.rgrc[0].top) / 4 + TextHeight / 2,
					_T("-10 dB"), 6);

		wDC.TextOut(ncp.rgrc[0].left - 1,
					ncp.rgrc[0].bottom + TextHeight / 2,
					_T("-20 dB"), 6);
	}
	catch (CResourceException * e)
	{
		e->Delete();
	}
	CWnd::OnNcPaint();
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

BOOL CEqualizerGraphWnd::OnSetCursor(CWnd* /*pWnd*/, UINT nHitTest, UINT /*message*/)
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
	if (nTrans && nBands > 2 && nBands <= MaxNumberOfEqualizerBands)
	{
		m_nBands = nBands;
		m_wGraph.SetNumberOfBands(nBands);
	}
}

void CEqualizerDialog::OnButtonResetBands()
{
	m_wGraph.ResetBands();
	if (m_bMultiBandEqualizer)
	{
		m_wGraph.SetNumberOfBands(m_nBands);
	}
	else
	{
		m_wGraph.SetNumberOfBands(2);
	}
	m_BandGain.SetData(m_wGraph.GetCurrentBandGainDb());
}

void CEqualizerDialog::OnButtonLoad()
{
	UpdateData(TRUE);

	CString FileName;
	CString Filter;
	Filter.LoadString(IDS_EQUALIZER_FILE_FILTER);

	CString Title;
	Title.LoadString(IDS_EQUALIZER_LOAD_TITLE);

	CFileDialogWithHistory dlg(TRUE,
								_T("Eqlz"), NULL,
								OFN_HIDEREADONLY
								| OFN_EXPLORER | OFN_NONETWORKBUTTON | OFN_FILEMUSTEXIST,
								Filter);
	dlg.m_ofn.lpstrTitle = Title;

	if (IDOK != dlg.DoModal())
	{
		return;
	}
	FileName = dlg.GetPathName();
	m_Profile.ImportSection(_T("Equalizer"), FileName);

	UpdateData(FALSE);

	m_wGraph.SetNumberOfBands(m_nBands);
	m_BandGain.SetData(m_wGraph.GetCurrentBandGainDb());
}

void CEqualizerDialog::OnButtonSaveAs()
{
	if (!UpdateData(TRUE))
	{
		TRACE("UpdateData failed.\n");
		// the UpdateData routine will set focus to correct item
		return;
	}

	CString FileName;
	CString Filter;
	Filter.LoadString(IDS_EQUALIZER_FILE_FILTER);

	CString Title;
	Title.LoadString(IDS_EQUALIZER_SAVE_TITLE);

	CFileDialogWithHistory dlg(FALSE,
								_T("Eqlz"), NULL,
								OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT
								| OFN_EXPLORER | OFN_NONETWORKBUTTON | OFN_PATHMUSTEXIST,
								Filter);
	dlg.m_ofn.lpstrTitle = Title;

	if (IDOK != dlg.DoModal())
	{
		return;
	}
	FileName = dlg.GetPathName();
	m_Profile.ExportSection(_T("Equalizer"), FileName);
}

void CEqualizerGraphWnd::OnKeyDown(UINT nChar, UINT /*nRepCnt*/, UINT /*nFlags*/)
{
	if (! m_bGotFocus)
	{
		return;
	}
	double GainDb;
	switch(nChar)
	{
	case VK_UP:
		// 1 dB up
		GainDb = 1 + floor(20. * log10(m_BandGain[m_BandWithFocus] * 1.00001));
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
		GainDb = ceil(20. * log10(m_BandGain[m_BandWithFocus] / 1.00001)) - 1.;
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
		return -0x100;
	}
	int dx = GetSystemMetrics(SM_CXSIZEFRAME);
	int dy = GetSystemMetrics(SM_CYSIZEFRAME);
	for (int i = 0; i < m_NumOfBands; i++)
	{
		// find if the mouse gets into a focus point
		int x;
		if (m_NumOfBands <= 2)
		{
			x = cr.Width() * (i * 38 + 1) / 40;
		}
		else
		{
			x = cr.Width() * (i * 2 + 1) / (2 * m_NumOfBands);
		}
		int y = int((1 - log10(m_BandGain[i])) * cr.Height() / 2);
		CRect r(x - dx, y - dy, x + dx, y + dy);
		CRect r1(x - dx, cr.top, x + dx, cr.bottom);

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

void CEqualizerGraphWnd::OnTimer(UINT nIDEvent)
{
	// redraw blinking dot
	if (m_bGotFocus && ! m_bButtonPressed)
	{
		DrawDotCaret( ! m_DotCaretIsOn);
	}

	BaseClass::OnTimer(nIDEvent);
}

void CEqualizerGraphWnd::DrawDotCaret(bool state)
{
	if (m_DotCaretIsOn != state)
	{
		m_DotCaretIsOn = state;
		CRect cr;
		GetClientRect( & cr);
		int x;
		if (m_NumOfBands <= 2)
		{
			x = cr.Width() * (m_BandWithFocus * 38 + 1) / 40;
		}
		else
		{
			x = cr.Width() * (m_BandWithFocus * 2 + 1) / (2 * m_NumOfBands);
		}
		int y = int((1 - log10(m_BandGain[m_BandWithFocus])) * cr.Height() / 2);
		int dx = GetSystemMetrics(SM_CXSIZEFRAME);
		int dy = GetSystemMetrics(SM_CYSIZEFRAME);
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

	BaseClass::OnOK();
	m_Profile.FlushAll();
}

void CEqualizerDialog::OnNotifyGraph( NMHDR * /*pNotifyStruct*/, LRESULT * /*result*/ )
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


void CEqualizerDialog::OnRadioEqualizerType()
{
	m_bMultiBandEqualizer = 0;
	m_eEditBands.EnableWindow(FALSE);
	m_SpinBands.EnableWindow(FALSE);
	m_wGraph.SetNumberOfBands(2);
	m_BandGain.SetData(m_wGraph.GetCurrentBandGainDb());
}

void CEqualizerDialog::OnRadio2()
{
	m_bMultiBandEqualizer = 1;
	m_eEditBands.EnableWindow(TRUE);
	m_SpinBands.EnableWindow(TRUE);
	m_wGraph.SetNumberOfBands(m_nBands);
	m_BandGain.SetData(m_wGraph.GetCurrentBandGainDb());
}

void CEqualizerGraphWnd::OnLButtonDblClk(UINT nFlags, CPoint point)
{
	// if click on the band, set the gain
	CWnd::OnLButtonDblClk(nFlags, point);
	int nHit = GetHitCode(point);
	if (nHit < int(~MaxNumberOfEqualizerBands))
	{
		return;
	}
	if (nHit < 0)
	{
		nHit = ~ nHit;
	}
	m_bButtonPressed = true;
	DrawDotCaret(true);
	SetFocusBand(nHit);

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

void CEqualizerDialog::OnCheckZeroPhase()
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
	m_wGraph.SetNumberOfBands(m_nBands);
}

