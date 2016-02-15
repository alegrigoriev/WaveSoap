// Copyright Alexander Grigoriev, 1997-2002, All Rights Reserved
// FilterDialog.cpp : implementation file
//

#include "stdafx.h"
#define _USE_MATH_DEFINES   // for M_PI definition
#include <math.h>
#include "WaveSoapFront.h"
#include "FilterDialog.h"
#include "OperationDialogs.h"
#include "FileDialogWithHistory.h"
#include "GdiObjectSave.h"
#include "DialogWithSelection.inl"
#include "FilterMath.h"
#include "OperationContext2.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

enum
{
	HpfStopbandIndex,
	HpfPassbandIndex,
	NotchBeginIndex,
	NotchZeroIndex,
	LpfPassbandIndex,
	LpfStopbandIndex,
	HilbertPassIndex,
	MaxFilterFrequencies,
	LeftmostPoint = -1,
	RightmostPoint = MaxFilterFrequencies + 1,

	HighpassFilterIndex = HpfStopbandIndex,
	LowpassFilterIndex = LpfPassbandIndex,
	NotchFilterIndex = NotchBeginIndex,
	HilbertFilterIndex = HilbertPassIndex,
};

//////////////////////////////////////////////////////////////////
//////////////// Filter
class LowpassFilter
{
public:
	LowpassFilter()
		: m_Order(0),
		m_ZeroPhase(FALSE)
	{
		Reset();
	}
	void Rebuild();
	void Reset();
	int GetFilterOrder() const
	{
		return m_Order;
	}
	void SetZeroPhase(bool ZeroPhase)
	{
		m_ZeroPhase = ZeroPhase;
	}
	std::complex<double> CalculateResponse(std::complex<double> z) const;
	BOOL CreateElliptic(double PassFreq, double PassLoss,
						double StopFreq, double StopLoss);
	void GetCoefficients(double Coeffs[MaxFilterOrder][6]) const;

	bool	m_ZeroPhase;
	int     m_Order;    // low pass filter order
	double  m_Coeffs[MaxFilterOrder][6];
};

class HighpassFilter
{
public:
	HighpassFilter()
		: m_Order(0),
		m_ZeroPhase(FALSE)
	{
		Reset();
	}

	void Rebuild();
	void Reset();
	int GetFilterOrder() const
	{
		return m_Order;
	}
	void SetZeroPhase(bool ZeroPhase)
	{
		m_ZeroPhase = ZeroPhase;
	}
	std::complex<double> CalculateResponse(std::complex<double> z) const;
	BOOL CreateElliptic(double PassFreq, double PassLoss,
						double StopFreq, double StopLoss);
	void GetCoefficients(double Coeffs[MaxFilterOrder][6]) const;

	BOOL	m_ZeroPhase;
	int     m_Order;    // low pass filter order
	double  m_Coeffs[MaxFilterOrder][6];
};

class NotchFilter
{
public:
	NotchFilter()
		: m_Order(0),
		m_ZeroPhase(FALSE)
	{
		Reset();
	}

	void Rebuild();
	void Reset();
	int GetFilterOrder() const
	{
		return m_Order;
	}
	void SetZeroPhase(bool ZeroPhase)
	{
		m_ZeroPhase = ZeroPhase;
	}
	std::complex<double> CalculateResponse(std::complex<double> z) const;
	void Create(double PassFreq, double StopFreq);
	void GetCoefficients(double Coeffs[MaxFilterOrder][6]) const;

	bool	m_ZeroPhase;
	int     m_Order;    // low pass filter order
	double  m_Coeffs[MaxFilterOrder][6];
};

class HilbertTransformFilter
{
public:
	HilbertTransformFilter()
		: m_Order(0)
	{
		Reset();
	}

	void Rebuild();
	void Reset();
	int GetFilterOrder() const
	{
		return m_Order;
	}

	std::complex<double> CalculateResponse(std::complex<double> /*z*/) const
	{
		return 1.;
	}
	BOOL CreateElliptic(double PassFreq, double PassLoss,
						double StopFreq, double StopLoss);
	void GetCoefficients(double Coeffs[MaxFilterOrder][6]) const;

	int     m_Order;    // low pass filter order
	double  m_Coeffs[MaxFilterOrder][6];
};

class Filter
{
public:
	Filter();
	void RebuildFilters();
	void ResetBands();

	BOOL IsZeroPhase() const
	{
		return m_bZeroPhase;
	}

	BOOL LowPassEnabled() const
	{
		return m_LowPassEnabled && ! m_HilbertEnabled;
	}

	BOOL HighPassEnabled() const
	{
		return m_HighPassEnabled && ! m_HilbertEnabled;
	}

	BOOL NotchEnabled() const
	{
		return m_NotchEnabled && ! m_HilbertEnabled;
	}

	BOOL HilbertTransformEnabled() const
	{
		return m_HilbertEnabled;
	}

	int GetLowpassFilterOrder() const;

	int GetHighpassFilterOrder() const;

	int GetNotchFilterOrder() const;

	int GetHilbertFilterOrder() const
	{
		return m_HilbertTransform.GetFilterOrder();
	}

	void GetLpfCoefficients(double Coeffs[MaxFilterOrder][6]) const
	{
		m_LowpassFilter.GetCoefficients(Coeffs);
	}

	void GetHpfCoefficients(double Coeffs[MaxFilterOrder][6]) const
	{
		m_HighpassFilter.GetCoefficients(Coeffs);
	}

	void GetNotchCoefficients(double Coeffs[MaxFilterOrder][6]) const
	{
		m_NotchFilter.GetCoefficients(Coeffs);
	}

	void GetHilbertCoefficients(double Coeffs[MaxFilterOrder][6]) const
	{
		m_HilbertTransform.GetCoefficients(Coeffs);
	}

	void EnableLowPass(bool Enable = true);
	void EnableHighPass(bool Enable = true);
	void EnableNotch(bool Enable = true);
	void EnableHilbert(bool Enable = true);
	void SetZeroPhase(bool ZeroPhase = true);

	// frequency is in radians
	std::complex<float> CalculateResponse(double Frequency) const;
	void CalculateCoefficients(double Gain1, double Frequency1,
								double Gain2, double Frequency2);
	BOOL CreateLowpassElliptic(double PassFreq, double PassLoss,
								double StopFreq, double StopLoss);
	BOOL CreateHighpassElliptic(double PassFreq, double PassLoss,
								double StopFreq, double StopLoss);

	// the coefficients are: 3 numerator's coeffs and 3 denominator's coeffs
protected:
	// frequencies are in radians/s
	double m_Frequencies[MaxFilterFrequencies];
	double m_Transfer[MaxFilterFrequencies];
	BOOL        m_LowPassEnabled;
	BOOL        m_HighPassEnabled;
	BOOL        m_NotchEnabled;
	BOOL        m_HilbertEnabled;

	LowpassFilter       m_LowpassFilter;
	HighpassFilter      m_HighpassFilter;
	NotchFilter         m_NotchFilter;
	HilbertTransformFilter   m_HilbertTransform;

	BOOL	m_bZeroPhase;
};

Filter::Filter()
	: m_LowPassEnabled(FALSE),
	m_HighPassEnabled(FALSE),
	m_NotchEnabled(FALSE),
	m_HilbertEnabled(FALSE),
	m_bZeroPhase(FALSE)
{
	ResetBands();
}

void Filter::ResetBands()
{
	for (int i = 0; i < MaxFilterFrequencies; i++)
	{
		m_Frequencies[i] = 1.;
		m_Transfer[i] = 1.;
	}

	m_LowpassFilter.Reset();
	m_HighpassFilter.Reset();
	m_NotchFilter.Reset();
	m_HilbertTransform.Reset();
}

void LowpassFilter::Reset()
{
	for (int i = 0; i < MaxFilterOrder; i++)
	{
		m_Coeffs[i][0] = 0.;
		m_Coeffs[i][1] = 0.;
		m_Coeffs[i][2] = 0.;
		m_Coeffs[i][3] = 1.;
		m_Coeffs[i][4] = 0.;
		m_Coeffs[i][5] = 0.;
	}
}

void HighpassFilter::Reset()
{
	for (int i = 0; i < MaxFilterOrder; i++)
	{
		m_Coeffs[i][0] = 0.;
		m_Coeffs[i][1] = 0.;
		m_Coeffs[i][2] = 0.;
		m_Coeffs[i][3] = 1.;
		m_Coeffs[i][4] = 0.;
		m_Coeffs[i][5] = 0.;
	}
}

void NotchFilter::Reset()
{
	for (int i = 0; i < MaxFilterOrder; i++)
	{
		m_Coeffs[i][0] = 0.;
		m_Coeffs[i][1] = 0.;
		m_Coeffs[i][2] = 0.;
		m_Coeffs[i][3] = 1.;
		m_Coeffs[i][4] = 0.;
		m_Coeffs[i][5] = 0.;
	}
}

void HilbertTransformFilter::Reset()
{
	for (int i = 0; i < MaxFilterOrder; i++)
	{
		m_Coeffs[i][0] = 0.;
		m_Coeffs[i][1] = 0.;
		m_Coeffs[i][2] = 0.;
		m_Coeffs[i][3] = 1.;
		m_Coeffs[i][4] = 0.;
		m_Coeffs[i][5] = 0.;
	}
}

int Filter::GetLowpassFilterOrder() const
{
	if (LowPassEnabled())
	{
		return m_HighpassFilter.GetFilterOrder();
	}
	else
	{
		return 0;
	}
}

int Filter::GetHighpassFilterOrder() const
{
	if (HighPassEnabled())
	{
		return m_HighpassFilter.GetFilterOrder();
	}
	else
	{
		return 0;
	}
}

int Filter::GetNotchFilterOrder() const
{
	if (NotchEnabled())
	{
		return m_HighpassFilter.GetFilterOrder();
	}
	else
	{
		return 0;
	}
}

void LowpassFilter::GetCoefficients(double Coeffs[MaxFilterOrder][6]) const
{
	for (int i = 0; i < MaxFilterOrder; i++)
	{
		for (int j = 0; j < 6; j++)
		{
			Coeffs[i][j] = m_Coeffs[i][j];
		}
	}
}

void HighpassFilter::GetCoefficients(double Coeffs[MaxFilterOrder][6]) const
{
	for (int i = 0; i < MaxFilterOrder; i++)
	{
		for (int j = 0; j < 6; j++)
		{
			Coeffs[i][j] = m_Coeffs[i][j];
		}
	}
}

void NotchFilter::GetCoefficients(double Coeffs[MaxFilterOrder][6]) const
{
	for (int i = 0; i < MaxFilterOrder; i++)
	{
		for (int j = 0; j < 6; j++)
		{
			Coeffs[i][j] = m_Coeffs[i][j];
		}
	}
}

void HilbertTransformFilter::GetCoefficients(double Coeffs[MaxFilterOrder][6]) const
{
	for (int i = 0; i < MaxFilterOrder; i++)
	{
		for (int j = 0; j < 6; j++)
		{
			Coeffs[i][j] = m_Coeffs[i][j];
		}
	}
}

void Filter::EnableLowPass(bool Enable)
{
	m_LowPassEnabled = Enable;
	RebuildFilters();
}

void Filter::EnableHighPass(bool Enable)
{
	m_HighPassEnabled = Enable;
	RebuildFilters();
}

void Filter::EnableNotch(bool Enable)
{
	m_NotchEnabled = Enable;
	RebuildFilters();
}

void Filter::EnableHilbert(bool Enable)
{
	m_HilbertEnabled = Enable;
	RebuildFilters();
}

void Filter::SetZeroPhase(bool ZeroPhase)
{
	m_bZeroPhase = ZeroPhase;
	m_LowpassFilter.SetZeroPhase(ZeroPhase);
	m_HighpassFilter.SetZeroPhase(ZeroPhase);
	m_NotchFilter.SetZeroPhase(ZeroPhase);
	RebuildFilters();
}

using std::complex;
// frequency is in radians
complex<double> LowpassFilter::CalculateResponse(complex<double> z) const
{
	complex<double> Result(0., 0.);
	for (int i = 0; i < m_Order; i++)
	{
		Result += (m_Coeffs[i][0] + z * (m_Coeffs[i][1] + z * m_Coeffs[i][2]))
				/ (m_Coeffs[i][3] + z * (m_Coeffs[i][4] + z * m_Coeffs[i][5]));
	}

	if (m_ZeroPhase)
	{
		// filter is applied twice
		return Result * conj(Result);
	}
	else
	{
		return Result;
	}
}

complex<double> NotchFilter::CalculateResponse(complex<double> z) const
{
	complex<double> Result(0., 0.);
	for (int i = 0; i < m_Order; i++)
	{
		Result += (m_Coeffs[i][0] + z * (m_Coeffs[i][1] + z * m_Coeffs[i][2]))
				/ (m_Coeffs[i][3] + z * (m_Coeffs[i][4] + z * m_Coeffs[i][5]));
	}

	if (m_ZeroPhase)
	{
		// filter is applied twice
		return Result * conj(Result);
	}
	else
	{
		return Result;
	}
}

complex<double> HighpassFilter::CalculateResponse(complex<double> z) const
{
	complex<double> Result(0., 0.);
	for (int i = 0; i < m_Order; i++)
	{
		Result += (m_Coeffs[i][0] + z * (m_Coeffs[i][1] + z * m_Coeffs[i][2]))
				/ (m_Coeffs[i][3] + z * (m_Coeffs[i][4] + z * m_Coeffs[i][5]));
	}

	if (m_ZeroPhase)
	{
		// filter is applied twice
		return Result * conj(Result);
	}
	else
	{
		return Result;
	}
}

complex<float> Filter::CalculateResponse(double Frequency) const
{
	complex<double> Result(1., 0.);

	complex<double> z(cos(Frequency), -sin(Frequency));

	if (LowPassEnabled())
	{
		Result *= m_LowpassFilter.CalculateResponse(z);
	}

	if (HighPassEnabled())
	{
		Result *= m_HighpassFilter.CalculateResponse(z);
	}

	if (NotchEnabled())
	{
		Result *= m_NotchFilter.CalculateResponse(z);
	}

	if (HilbertTransformEnabled())
	{
		Result *= m_HilbertTransform.CalculateResponse(z);
	}

	return complex<float>(Result);
}

void Filter::RebuildFilters()
{
	if (LowPassEnabled())
	{
		m_LowpassFilter.CreateElliptic(m_Frequencies[LpfPassbandIndex],
										m_Transfer[LpfPassbandIndex],
										m_Frequencies[LpfStopbandIndex],
										m_Transfer[LpfStopbandIndex]);
	}

	if (HighPassEnabled())
	{
		m_HighpassFilter.CreateElliptic(m_Frequencies[HpfPassbandIndex],
										m_Transfer[HpfPassbandIndex],
										m_Frequencies[HpfStopbandIndex],
										m_Transfer[HpfStopbandIndex]);
	}

	if (NotchEnabled())
	{
		m_NotchFilter.Create(m_Frequencies[NotchBeginIndex], m_Frequencies[NotchZeroIndex]);
	}
}

void NotchFilter::Create(double PassFreq, double StopFreq)
{
	// two zeros at unity circle, two poles
	double Width = fabs(StopFreq - PassFreq);
	if (m_ZeroPhase)
	{
		Width /= 2.;
	}
	m_Order = 1;
	double RotC = cos(StopFreq);
	double pole = 1. - Width;

	m_Coeffs[0][0] = 1;
	m_Coeffs[0][1] = -2 * RotC;
	m_Coeffs[0][2] = 1;
	m_Coeffs[0][3] = 1;
	m_Coeffs[0][4] = -2 * pole * RotC;
	m_Coeffs[0][5] = pole * pole;
}

BOOL LowpassFilter::CreateElliptic(double PassFreq, double PassLoss,
									double StopFreq, double StopLoss)
{
	double OmegaPass = 2. * tan(PassFreq / 2.);
	double OmegaStop = 2. * tan(StopFreq / 2.);
	if (m_ZeroPhase)
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

	m_Order = (int)pDecomposed->GetSize();

	for (int i = 0; i < pDecomposed->GetSize(); i++)
	{
#if 0 && defined(_DEBUG)
		pDecomposed->GetAt(i)->Dump();
#endif
		polyRatio prBil;
		BilinearTransform( *pDecomposed->GetAt(i), prBil, 1.);
		ASSERT(prBil.numer().order() == 2 || prBil.numer().order() == 1);
		ASSERT(prBil.denom().order() == 2 || prBil.denom().order() == 1);

		m_Coeffs[i][0] = prBil.numer()[0].real();
		m_Coeffs[i][1] = prBil.numer()[1].real();
		if (prBil.numer().order() > 1)
		{
			m_Coeffs[i][2] = prBil.numer()[2].real();
		}
		else
		{
			m_Coeffs[i][2] = 0.;
		}
		m_Coeffs[i][3] = prBil.denom()[0].real();
		m_Coeffs[i][4] = prBil.denom()[1].real();

		if (prBil.numer().order() > 1)
		{
			m_Coeffs[i][5] = prBil.denom()[2].real();
		}
		else
		{
			m_Coeffs[i][5] = 0.;
		}

		delete pDecomposed->GetAt(i);
	}
	delete pDecomposed;
	return TRUE;
}

BOOL HighpassFilter::CreateElliptic(double PassFreq, double PassLoss,
									double StopFreq, double StopLoss)
{
	double OmegaPass = 2. / tan(PassFreq / 2.);
	double OmegaStop = 2. / tan(StopFreq / 2.);
	if (m_ZeroPhase)
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

	m_Order = (int)pDecomposed->GetSize();

	for (int i = 0; i < pDecomposed->GetSize(); i++)
	{
#if 0 && defined(_DEBUG)
		pDecomposed->GetAt(i)->Dump();
#endif
		polyRatio prBil;
		BilinearTransform( *pDecomposed->GetAt(i), prBil, 1.);
		ASSERT(prBil.numer().order() == 2 || prBil.numer().order() == 1);
		ASSERT(prBil.denom().order() == 2 || prBil.denom().order() == 1);

		m_Coeffs[i][0] = prBil.numer()[0].real();
		m_Coeffs[i][1] = -prBil.numer()[1].real();

		if (prBil.numer().order() > 1)
		{
			m_Coeffs[i][2] = prBil.numer()[2].real();
		}
		else
		{
			m_Coeffs[i][2] = 0.;
		}
		m_Coeffs[i][3] = prBil.denom()[0].real();
		m_Coeffs[i][4] = -prBil.denom()[1].real();

		if (prBil.numer().order() > 1)
		{
			m_Coeffs[i][5] = prBil.denom()[2].real();
		}
		else
		{
			m_Coeffs[i][5] = 0.;
		}

		delete pDecomposed->GetAt(i);
	}
	delete pDecomposed;
	return TRUE;
}

////////////////////////////////////////////////////////////////////////////////
///////// CFilterGraphWnd
class CFilterGraphWnd : public CWnd, public Filter
{
	typedef CWnd BaseClass;
	// Construction
public:
	CFilterGraphWnd(CApplicationProfile & Profile, int SampleRate);
	virtual ~CFilterGraphWnd();

	// Attributes

	void ValidateFilterSettings();
	void ResetToInitial();
	void RebuildFilters();

	void EnableLowPass(bool Enable = true);
	void EnableHighPass(bool Enable = true);
	void EnableNotch(bool Enable = true);
	void EnableHilbert(bool Enable = true);
	void SetZeroPhase(bool ZeroPhase = true);

	void SetPointTransferDb(int nPoint, double Transfer);
	void SetPointFrequency(int nPoint, double Frequency);
	void SetPointFrequencyHz(int nPoint, double Frequency);
	double GetCurrentPointFrequencyHz() const;
	double GetPointFrequencyHz(unsigned nPoint) const;

	void SetCurrentPointFrequency(double Frequency)
	{
		SetPointFrequency(m_PointWithFocus, Frequency);
	}

	void SetCurrentPointFrequencyHz(double Frequency)
	{
		SetPointFrequencyHz(m_PointWithFocus, Frequency);
	}

	double GetCurrentPointGain() const
	{
		return m_Transfer[m_PointWithFocus];
	}

	double GetCurrentPointFrequency() const
	{
		return m_Frequencies[m_PointWithFocus];
	}

	double GetPointTransferDb(unsigned nPoint) const
	{
		return 20. * log10(m_Transfer[nPoint]);
	}

	void SetCurrentPointTransferDb(double TransferDb)
	{
		SetPointTransferDb(m_PointWithFocus, TransferDb);
	}

	double GetSamplingRate() const
	{
		return m_SamplingRate;
	}

	int GetCurrentFilter() const;

	int GetCurrentFilterPassbandIndex() const;
	int GetCurrentFilterStopbandIndex() const;

	void SetCurrentFilterPassbandFrequency(double Frequency);
	void SetCurrentFilterPassbandTransferDb(double TransferDb);
	void SetCurrentFilterStopbandFrequency(double Frequency);
	void SetCurrentFilterStopbandTransferDb(double TransferDb);

	double GetCurrentFilterPassbandFrequency() const;
	double GetCurrentFilterPassbandTransferDb() const;
	double GetCurrentFilterStopbandFrequency() const;
	double GetCurrentFilterStopbandTransferDb() const;

	void SetFocusPoint(unsigned nPoint);

	int GetHitCode(POINT point);

	// get pixel where the point is drawn (x coordinate in client area)
	int GetFilterPointPixel(int FilterPoint);
	void SetFilterPointPixel(int FilterPoint, int PointPixel, BOOL MoveBoth);
	void InvalidateGraphPoint(double Frequency, double Transfer);

	static double PosYToTransferDb(int y, int height)
	{
		//return 20. * (0.25 - y * 4.5 / height);
		return 5. - y * 90. / height;
	}
	static int TransferToPosY(double Transfer, int height)
	{
		// full range: 5 to -85 db
		return int((0.25 - log10(Transfer)) * height / 4.5);
	}
	static int TransferDbToPosY(double TransferDb, int height)
	{
		// full range: 5 to -85 db
		return int((5. - TransferDb) * height / 90.);
	}
	// Operations
public:
	void DoDataExchange(CDataExchange* pDX);

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CEqualizerGraphWnd)
protected:
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	//}}AFX_VIRTUAL

// Implementation
	void DrawDotCaret(bool state = true);
//    void ShowDotCaret(bool state = true);

protected:
	bool m_bMouseCaptured;
	bool m_bButtonPressed;
	bool m_bGotFocus;
	bool m_DotCaretIsOn;

	unsigned m_PointWithFocus;
	double m_SamplingRate;

	CApplicationProfile & m_Profile;
	// Generated message map functions
	void NotifyParentDlg();
	//{{AFX_MSG(CEqualizerGraphWnd)
	afx_msg void OnPaint();
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	afx_msg void OnNcCalcSize(BOOL bCalcValidRects, NCCALCSIZE_PARAMS FAR* lpncsp);
	afx_msg void OnCaptureChanged(CWnd *pWnd);
	afx_msg int OnMouseActivate(CWnd* pDesktopWnd, UINT nHitTest, UINT message);
	afx_msg void OnKillFocus(CWnd* pNewWnd);
	afx_msg void OnSetFocus(CWnd* pOldWnd);
	afx_msg BOOL OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message);
	afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg UINT OnGetDlgCode();
	afx_msg void OnTimer(UINT_PTR nIDEvent);
	afx_msg void OnLButtonDblClk(UINT nFlags, CPoint point);
	//}}AFX_MSG
	afx_msg void OnNcPaint(WPARAM wParam, LPARAM);
	DECLARE_MESSAGE_MAP()
};

CFilterGraphWnd::CFilterGraphWnd(CApplicationProfile & Profile, int SampleRate)
	: m_SamplingRate(SampleRate)
	, m_bMouseCaptured(false)
	, m_bButtonPressed(false)
	, m_bGotFocus(false)
	, m_Profile(Profile)
{
	m_PointWithFocus = 0;

	Profile.AddBoolItem(_T("Filter"), _T("ZeroPhase"), m_bZeroPhase, FALSE);

	m_Transfer[HpfPassbandIndex] = 0.9;
	m_Transfer[HpfStopbandIndex] = 0.001;

	m_Transfer[NotchBeginIndex] = pow(10., -3. / 20.);
	m_Transfer[NotchZeroIndex] = 0.0001;

	m_Transfer[LpfPassbandIndex] = 0.9;
	m_Transfer[LpfStopbandIndex] = 0.001;

	m_Frequencies[HpfPassbandIndex] = M_PI / 250.;
	m_Frequencies[HpfStopbandIndex] = M_PI / 500.;

	m_Frequencies[NotchBeginIndex] = M_PI * 0.048;
	m_Frequencies[NotchZeroIndex] = M_PI * 0.052;

	m_Frequencies[LpfPassbandIndex] = M_PI * 0.8;
	m_Frequencies[LpfStopbandIndex] = M_PI * 0.9;

	Profile.AddBoolItem(_T("Filter"), _T("LowPassEnable"), m_LowPassEnabled, TRUE);
	Profile.AddItem(_T("Filter"), _T("LowPass_PassLoss"), m_Transfer[LpfPassbandIndex], m_Transfer[LpfPassbandIndex], 0.00003, 1.);
	Profile.AddItem(_T("Filter"), _T("LowPass_PassFrequency"), m_Frequencies[LpfPassbandIndex], m_Frequencies[LpfPassbandIndex], 0.00157, 3.14);
	Profile.AddItem(_T("Filter"), _T("LowPass_StopLoss"), m_Transfer[LpfStopbandIndex], m_Transfer[LpfStopbandIndex], 0.00003, 1.);
	Profile.AddItem(_T("Filter"), _T("LowPass_StopFrequency"), m_Frequencies[LpfStopbandIndex], m_Frequencies[LpfStopbandIndex], 0.00157, 3.14);

	Profile.AddBoolItem(_T("Filter"), _T("HighPassEnable"), m_HighPassEnabled, TRUE);
	Profile.AddItem(_T("Filter"), _T("HighPass_PassLoss"), m_Transfer[HpfPassbandIndex], m_Transfer[HpfPassbandIndex], 0.00003, 1.);
	Profile.AddItem(_T("Filter"), _T("HighPass_PassFrequency"), m_Frequencies[HpfPassbandIndex], m_Frequencies[HpfPassbandIndex], 0.00157, 3.14);
	Profile.AddItem(_T("Filter"), _T("HighPass_StopLoss"), m_Transfer[HpfStopbandIndex], m_Transfer[HpfStopbandIndex], 0.00003, 1.);
	Profile.AddItem(_T("Filter"), _T("HighPass_StopFrequency"), m_Frequencies[HpfStopbandIndex], m_Frequencies[HpfStopbandIndex], 0.00157, 3.14);

	Profile.AddBoolItem(_T("Filter"), _T("NotchEnable"), m_NotchEnabled, FALSE);
	Profile.AddItem(_T("Filter"), _T("Notch_PassFrequency"), m_Frequencies[NotchBeginIndex], m_Frequencies[NotchBeginIndex], 0.00157, 3.14);
	Profile.AddItem(_T("Filter"), _T("Notch_StopFrequency"), m_Frequencies[NotchZeroIndex], m_Frequencies[NotchZeroIndex], 0.00157, 3.14);

	Profile.AddBoolItem(_T("Filter"), _T("HilbertTransform"), m_HilbertEnabled, FALSE);
	Profile.AddItem(_T("Filter"), _T("Hilbert_PassFrequency"), m_Frequencies[HilbertPassIndex], m_Frequencies[HilbertPassIndex], 0.00157, 3.14);

	m_LowpassFilter.SetZeroPhase(m_bZeroPhase != 0);
	m_HighpassFilter.SetZeroPhase(m_bZeroPhase != 0);
	m_NotchFilter.SetZeroPhase(m_bZeroPhase != 0);

	ValidateFilterSettings();
}

CFilterGraphWnd::~CFilterGraphWnd()
{
	m_Profile.RemoveSection(_T("Filter"));
}

void CFilterGraphWnd::SetPointFrequencyHz(int nPoint, double Frequency)
{
	SetPointFrequency(nPoint, Frequency / m_SamplingRate * (2. * M_PI));
}

double CFilterGraphWnd::GetPointFrequencyHz(unsigned nPoint) const
{
	return m_SamplingRate * m_Frequencies[nPoint] / (2. * M_PI);
}

double CFilterGraphWnd::GetCurrentPointFrequencyHz() const
{
	return m_SamplingRate * m_Frequencies[m_PointWithFocus] / (2. * M_PI);
}

void CFilterGraphWnd::DoDataExchange(CDataExchange* pDX)
{
	DDX_Check(pDX, IDC_CHECK_ZERO_PHASE, m_bZeroPhase);
	DDX_Check(pDX, IDC_CHECK_LOWPASS, m_LowPassEnabled);
	DDX_Check(pDX, IDC_CHECK_HIGHPASS, m_HighPassEnabled);
	DDX_Check(pDX, IDC_CHECK_STOPBAND, m_NotchEnabled);
	DDX_Check(pDX, IDC_CHECK_HILBERT_TRANSFORM, m_HilbertEnabled);
}

void CFilterGraphWnd::ResetToInitial()
{
	m_Profile.ResetSectionToDefault(_T("Filter"));
	ValidateFilterSettings();
}

void CFilterGraphWnd::ValidateFilterSettings()
{
	// check for correct frequencies and gain
	if (m_Transfer[HpfPassbandIndex] <= m_Transfer[HpfStopbandIndex])
	{
		m_Transfer[HpfPassbandIndex] = 0.9;
		m_Transfer[HpfStopbandIndex] = 0.001;
	}
	if (m_Frequencies[HpfPassbandIndex] <= m_Frequencies[HpfStopbandIndex])
	{
		m_Frequencies[HpfPassbandIndex] = M_PI / 250.;
		m_Frequencies[HpfStopbandIndex] = M_PI / 500.;
	}
	// check for correct frequencies and gain
	if (m_Transfer[LpfPassbandIndex] <= m_Transfer[LpfStopbandIndex])
	{
		m_Transfer[LpfPassbandIndex] = 0.9;
		m_Transfer[LpfStopbandIndex] = 0.001;
	}
	if (m_Frequencies[LpfPassbandIndex] >= m_Frequencies[LpfStopbandIndex])
	{
		m_Frequencies[LpfPassbandIndex] = M_PI * 0.8;
		m_Frequencies[LpfStopbandIndex] = M_PI * 0.9;
	}
	// check for correct frequencies and gain
	if (m_Transfer[NotchBeginIndex] <= m_Transfer[NotchZeroIndex])
	{
		m_Transfer[NotchBeginIndex] = 0.7071067811;
		m_Transfer[NotchZeroIndex] = 0.0001;
	}
	if (m_Frequencies[NotchBeginIndex] >= m_Frequencies[NotchZeroIndex])
	{
		m_Frequencies[NotchBeginIndex] = M_PI * 0.048;
		m_Frequencies[NotchZeroIndex] = M_PI * 0.052;
	}

	if ( ! HighPassEnabled()
		&& (m_PointWithFocus == HpfStopbandIndex
			|| m_PointWithFocus == HpfPassbandIndex))
	{
		m_PointWithFocus = NotchBeginIndex;
		if ( ! NotchEnabled())
		{
			m_PointWithFocus = LpfPassbandIndex;
			if ( ! LowPassEnabled())
			{
				m_PointWithFocus = HpfPassbandIndex;
			}
		}
	}
	else if ( ! LowPassEnabled()
			&& (m_PointWithFocus == LpfStopbandIndex
				|| m_PointWithFocus == LpfPassbandIndex))
	{
		m_PointWithFocus = HpfStopbandIndex;
		if ( ! HighPassEnabled())
		{
			m_PointWithFocus = NotchBeginIndex;
			if ( ! NotchEnabled())
			{
				m_PointWithFocus = HpfPassbandIndex;
			}
		}
	}
	else if ( ! NotchEnabled()
			&& (m_PointWithFocus == NotchBeginIndex
				|| m_PointWithFocus == NotchZeroIndex))
	{
		m_PointWithFocus = LpfStopbandIndex;
		if ( ! LowPassEnabled())
		{
			m_PointWithFocus = HpfPassbandIndex;
		}
	}
}

int CFilterGraphWnd::GetCurrentFilter() const
{
	switch (m_PointWithFocus)
	{
	case HpfStopbandIndex:
	case HpfPassbandIndex:
	default:
		return HighpassFilterIndex;
		break;

	case NotchBeginIndex:
	case NotchZeroIndex:
		return NotchFilterIndex;
		break;

	case LpfPassbandIndex:
	case LpfStopbandIndex:
		return LowpassFilterIndex;
		break;
	}
}

int CFilterGraphWnd::GetCurrentFilterPassbandIndex() const
{
	switch (m_PointWithFocus)
	{
	case HpfStopbandIndex:
	case HpfPassbandIndex:
	default:
		return HpfPassbandIndex;
		break;

	case NotchBeginIndex:
	case NotchZeroIndex:
		return NotchBeginIndex;
		break;

	case LpfPassbandIndex:
	case LpfStopbandIndex:
		return LpfPassbandIndex;
		break;
	}
}

int CFilterGraphWnd::GetCurrentFilterStopbandIndex() const
{
	switch (m_PointWithFocus)
	{
	case HpfStopbandIndex:
	case HpfPassbandIndex:
	default:
		return HpfStopbandIndex;
		break;

	case NotchBeginIndex:
	case NotchZeroIndex:
		return NotchZeroIndex;
		break;

	case LpfPassbandIndex:
	case LpfStopbandIndex:
		return LpfStopbandIndex;
		break;
	}
}

void CFilterGraphWnd::SetCurrentFilterPassbandFrequency(double Frequency)
{
	SetPointFrequencyHz(GetCurrentFilterPassbandIndex(), Frequency);
}

void CFilterGraphWnd::SetCurrentFilterPassbandTransferDb(double TransferDb)
{
	SetPointTransferDb(GetCurrentFilterPassbandIndex(), TransferDb);
}

void CFilterGraphWnd::SetCurrentFilterStopbandFrequency(double Frequency)
{
	SetPointFrequencyHz(GetCurrentFilterStopbandIndex(), Frequency);
}

void CFilterGraphWnd::SetCurrentFilterStopbandTransferDb(double TransferDb)
{
	SetPointTransferDb(GetCurrentFilterStopbandIndex(), TransferDb);
}

double CFilterGraphWnd::GetCurrentFilterPassbandFrequency() const
{
	return GetPointFrequencyHz(GetCurrentFilterPassbandIndex());
}

double CFilterGraphWnd::GetCurrentFilterPassbandTransferDb() const
{
	return GetPointTransferDb(GetCurrentFilterPassbandIndex());
}

double CFilterGraphWnd::GetCurrentFilterStopbandFrequency() const
{
	return GetPointFrequencyHz(GetCurrentFilterStopbandIndex());
}

double CFilterGraphWnd::GetCurrentFilterStopbandTransferDb() const
{
	return GetPointTransferDb(GetCurrentFilterStopbandIndex());
}

typedef void (CWnd::*NcPaintFunc)(WPARAM wParam, LPARAM);
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
{ WM_NCPAINT, 0, 0, 0, AfxSig_v_w_l,
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
		int y = TransferToPosY(gain, cr.Height());
		if (x == ur.left)
		{
			dc.MoveTo(x, y);
		}
		else
		{
			dc.LineTo(x, y);
		}
	}
	int dx = std::min(GetSystemMetrics(SM_CXSIZEFRAME), GetSystemMetrics(SM_CXDOUBLECLK));
	int dy = std::min(GetSystemMetrics(SM_CYSIZEFRAME), GetSystemMetrics(SM_CYDOUBLECLK));
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

	for (unsigned i = 0; i < MaxFilterFrequencies; i++)
	{
		// draw circles around the reference points
		int x = int((1. + log10(m_Frequencies[i] / M_PI) / 3.) * cr.Width() - 1);
		int y = TransferToPosY(m_Transfer[i], cr.Height());

		int SrcOffset = 0;
		if (m_DotCaretIsOn && i == m_PointWithFocus)
		{
			SrcOffset = w;
		}
		if (i <= HpfPassbandIndex)
		{
			if ( ! HighPassEnabled())
			{
				continue;
			}
		}
		else if (i >= LpfPassbandIndex)
		{
			if ( ! LowPassEnabled())
			{
				continue;
			}
		}
		else
		{
			if ( ! NotchEnabled())
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

		SetCurrentPointTransferDb(PosYToTransferDb(point.y, cr.Height()));
		if (GetFilterPointPixel(m_PointWithFocus) != point.x)
		{
			SetFilterPointPixel(m_PointWithFocus, point.x, nFlags & MK_CONTROL);
			RebuildFilters();
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

void CFilterGraphWnd::SetFocusPoint(unsigned nPoint)
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

void CFilterGraphWnd::EnableLowPass(bool Enable)
{
	if ( ! Enable)
	{
		if (m_PointWithFocus >= LpfPassbandIndex)
		{
			m_PointWithFocus = HpfPassbandIndex;
			if ( ! HighPassEnabled())
			{
				m_PointWithFocus = NotchBeginIndex;
			}
		}
	}

	Filter::EnableLowPass(Enable);
	Invalidate();
}

void CFilterGraphWnd::EnableHighPass(bool Enable)
{
	if ( ! Enable)
	{
		if (m_PointWithFocus <= HpfPassbandIndex)
		{
			m_PointWithFocus = LpfPassbandIndex;
			if ( ! LowPassEnabled())
			{
				m_PointWithFocus = NotchBeginIndex;
			}
		}
	}

	Filter::EnableHighPass(Enable);
	Invalidate();
}

void CFilterGraphWnd::EnableNotch(bool Enable)
{
	if ( ! Enable)
	{
		if (m_PointWithFocus == NotchBeginIndex
			|| m_PointWithFocus == NotchZeroIndex)
		{
			m_PointWithFocus = LpfPassbandIndex;
			if ( ! LowPassEnabled())
			{
				m_PointWithFocus = HpfPassbandIndex;
			}
		}
	}

	Filter::EnableNotch(Enable);
	Invalidate();
}

void CFilterGraphWnd::EnableHilbert(bool Enable)
{
	if ( ! Enable)
	{
		if (m_PointWithFocus == HilbertPassIndex)
		{
			if (LowPassEnabled())
			{
				m_PointWithFocus = LpfPassbandIndex;
			}
			else if (HighPassEnabled())
			{
				m_PointWithFocus = HpfPassbandIndex;
			}
		}
	}

	Filter::EnableHilbert(Enable);
	Invalidate();
}

void CFilterGraphWnd::SetZeroPhase(bool ZeroPhase)
{
	Filter::SetZeroPhase(ZeroPhase);
	Invalidate();
}

void CFilterGraphWnd::SetPointTransferDb(int nPoint, double TransferDb)
{
	if (TransferDb > 0.)
	{
		TransferDb = 0.;
	}
	switch (nPoint)
	{
	case HpfStopbandIndex:
		if (TransferDb > -10.)
		{
			TransferDb = -10.;
		}
		break;
	case HpfPassbandIndex:
		if (TransferDb < -6.)
		{
			TransferDb = -6.;
		}
		break;
	case NotchBeginIndex:
		TransferDb = -3.;
		break;
	case NotchZeroIndex:
		TransferDb = -85.;
		break;
	case LpfPassbandIndex:
		if (TransferDb < -6.)
		{
			TransferDb = -6.;
		}
		break;
	case LpfStopbandIndex:
		if (TransferDb > -10.)
		{
			TransferDb = -10.;
		}
		break;
	}
	double Transfer = pow(10., TransferDb / 20.);
	if (m_Transfer[nPoint] == Transfer)
	{
		return;
	}
	m_Transfer[nPoint] = Transfer;
	RebuildFilters();
}

void CFilterGraphWnd::SetPointFrequency(int nPoint, double Frequency)
{
	if (m_Frequencies[nPoint] == Frequency)
	{
		return;
	}
	m_Frequencies[nPoint] = Frequency;
	RebuildFilters();
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
		pDC->PatBlt(cr.left, TransferDbToPosY(0., ClientHeight), cr.Width(), 1, PATINVERT);
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
		int y = TransferDbToPosY(d, ClientHeight);
		pDC->MoveTo(cr.left, y);
		pDC->LineTo(cr.right, y);
	}

	return TRUE;
}

void CFilterGraphWnd::OnNcCalcSize(BOOL /*bCalcValidRects*/,
									NCCALCSIZE_PARAMS * lpncsp)
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

void CFilterGraphWnd::OnNcPaint(WPARAM wParam, LPARAM)
{
	if (0) TRACE("CFilterGraphWnd::OnNcPaint, hrgn=%P\n", wParam);
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
		//CWnd * pParentDlg = GetParent();
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
						TextOrigin + TransferDbToPosY(d, ClientHeight),
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

BOOL CFilterGraphWnd::OnSetCursor(CWnd* /*pWnd*/, UINT nHitTest, UINT /*message*/)
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

	double TransferDb;
	BOOL const ShiftPressed = (0 != (0x8000 & GetKeyState(VK_SHIFT)));
	BOOL const CtrlPressed = (0 != (0x8000 & GetKeyState(VK_CONTROL)));

	switch(nChar)
	{
	case VK_UP:
		// 1 dB up
		TransferDb = 1 + floor(20. * log10(m_Transfer[m_PointWithFocus] * 1.00001));
		SetCurrentPointTransferDb(TransferDb);

		DrawDotCaret(true);
		NotifyParentDlg();
		return;
		break;

	case VK_DOWN:
		// 1 dB down
		TransferDb = ceil(20. * log10(m_Transfer[m_PointWithFocus] / 1.00001)) - 1.;
		SetCurrentPointTransferDb(TransferDb);

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
			NotifyParentDlg();
		}
		else
		{
			// focus to the prev band
			// find next visible point
			unsigned NewFocusPoint = m_PointWithFocus;
			switch (m_PointWithFocus)
			{
			case HpfStopbandIndex:
				// nothing
				break;
			case HpfPassbandIndex:
				NewFocusPoint = HpfStopbandIndex;
				break;
			case NotchBeginIndex:
				if (HighPassEnabled())
				{
					NewFocusPoint = HpfPassbandIndex;
				}
				break;
			case NotchZeroIndex:
				NewFocusPoint = NotchBeginIndex;
				break;
			case LpfPassbandIndex:
				if (NotchEnabled())
				{
					NewFocusPoint = NotchZeroIndex;
				}
				else if (HighPassEnabled())
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
			NotifyParentDlg();
		}
		else
		{
			// focus to the next band
			// find next visible point
			unsigned NewFocusPoint = m_PointWithFocus;
			switch (m_PointWithFocus)
			{
			case HpfStopbandIndex:
				NewFocusPoint = HpfPassbandIndex;
				break;
			case HpfPassbandIndex:
				if (NotchEnabled())
				{
					NewFocusPoint = NotchBeginIndex;
				}
				else if (LowPassEnabled())
				{
					NewFocusPoint = LpfPassbandIndex;
				}
				break;
			case NotchBeginIndex:
				NewFocusPoint = NotchZeroIndex;
				break;
			case NotchZeroIndex:
				if (LowPassEnabled())
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
	int dx = std::min(GetSystemMetrics(SM_CXSIZEFRAME), GetSystemMetrics(SM_CXDOUBLECLK));
	int dy = std::min(GetSystemMetrics(SM_CYSIZEFRAME), GetSystemMetrics(SM_CYDOUBLECLK));

	for (int i = 0; i < MaxFilterFrequencies; i++)
	{
		// find if the mouse gets into a focus point
		int x = int((1. + log10(m_Frequencies[i] / M_PI) / 3.) * cr.Width() - 1);
		// full range: 5 to -85 db
		int y = TransferToPosY(m_Transfer[i], cr.Height());
		CRect r(x - dx, y - dy, x + dx, y + dy);
		CRect r1(x - dx, cr.top, x + dx, cr.bottom);

		if (i <= HpfPassbandIndex)
		{
			if ( ! HighPassEnabled())
			{
				continue;
			}
		}
		else if (i >= LpfPassbandIndex)
		{
			if ( ! LowPassEnabled())
			{
				continue;
			}
		}
		else
		{
			if ( ! NotchEnabled())
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

void CFilterGraphWnd::OnTimer(UINT_PTR nIDEvent)
{
	// redraw blinking dot
	if (m_bGotFocus && ! m_bButtonPressed)
	{
		DrawDotCaret( ! m_DotCaretIsOn);
	}

	BaseClass::OnTimer(nIDEvent);
}

void CFilterGraphWnd::InvalidateGraphPoint(double Frequency, double Transfer)
{
	CRect cr;
	GetClientRect( & cr);

	int x = int((1. + log10(Frequency / M_PI) / 3.) * cr.Width() - 1);
	int y = TransferToPosY(Transfer, cr.Height());

	int dx = std::min(GetSystemMetrics(SM_CXSIZEFRAME), GetSystemMetrics(SM_CXDOUBLECLK));
	int dy = std::min(GetSystemMetrics(SM_CYSIZEFRAME), GetSystemMetrics(SM_CYDOUBLECLK));

	CRect r(x - dx, y - dy, x + dx, y + dy);
	InvalidateRect( & r);
}

void CFilterGraphWnd::DrawDotCaret(bool state)
{
	if (m_DotCaretIsOn != state)
	{
		m_DotCaretIsOn = state;
		InvalidateGraphPoint(m_Frequencies[m_PointWithFocus], m_Transfer[m_PointWithFocus]);
	}

	if (m_bGotFocus && ! m_bButtonPressed)
	{
		SetTimer(1, GetCaretBlinkTime(), NULL);
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
	SetCurrentPointTransferDb(PosYToTransferDb(point.y, cr.Height()));
	NotifyParentDlg();
}

void CFilterGraphWnd::RebuildFilters()
{
	Filter::RebuildFilters();
	Invalidate();
}

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
	m_pGraphWnd(NULL)
{
	m_bLockChannels = bLockChannels;
	m_bUndo = bUndoEnabled;
	//{{AFX_DATA_INIT(CFilterDialog)
	//}}AFX_DATA_INIT
	m_pGraphWnd = new CFilterGraphWnd(m_Profile, WaveFile.SampleRate());

	static ResizableDlgItem const ResizeItems[] =
	{
		{IDC_STATIC1, MoveDown},
		{IDC_EDIT_FILTER_PASSBAND_LOSS, MoveDown},
		{IDC_STATIC2, MoveDown},
		{IDC_EDIT_FILTER_PASS_FREQUENCY, MoveDown},
		{IDC_STATIC4, MoveDown},

		{IDC_STATIC3, MoveDown},
		{IDC_EDIT_FILTER_STOPBAND_LOSS, MoveDown},
		{IDC_STATIC9, MoveDown},
		{IDC_EDIT_FILTER_STOP_FREQUENCY, MoveDown},
		{IDC_STATIC10, MoveDown},

		{IDC_CHECK_LOWPASS, MoveDown},
		{IDC_CHECK_STOPBAND, MoveDown},
		{IDC_CHECK_HIGHPASS, MoveDown},
		{IDC_CHECK_HILBERT_TRANSFORM, MoveDown},
		{IDC_CHECK_ZERO_PHASE, MoveDown},
		{IDC_CHECK_UNDO, MoveDown},
		{IDC_STATIC_SELECTION, MoveDown},
		{IDC_BUTTON_SELECTION, MoveRight | MoveDown},

		{IDC_BUTTON_RESET_BANDS, MoveRight | MoveDown},
		{IDC_BUTTON_SAVE_AS, MoveRight | MoveDown},
		{IDC_BUTTON_LOAD, MoveRight | MoveDown},
		{IDOK, CenterHorizontally | MoveDown},
		{IDCANCEL, CenterHorizontally | MoveDown},
		{IDHELP, MoveRight | MoveDown},

		{AFX_IDW_PANE_FIRST, ExpandRight | ExpandDown},
	};

	SetResizeableItems(ResizeItems, countof(ResizeItems));

	m_Profile.AddItem(_T("Settings"), _T("FilterDlgWidth"), m_DlgWidth, 0, 0, 4096);
	m_Profile.AddItem(_T("Settings"), _T("FilterDlgHeight"), m_DlgHeight, 0, 0, 4096);

	m_EditPassLoss.SetPrecision(2);
	m_EditPassFrequency.SetPrecision(1);

	m_EditStopLoss.SetPrecision(2);
	m_EditStopFrequency.SetPrecision(1);
}

CFilterDialog::~CFilterDialog()
{
	delete m_pGraphWnd;
}

void CFilterDialog::DoDataExchange(CDataExchange* pDX)
{
	BaseClass::DoDataExchange(pDX);

	DDX_Control(pDX, IDC_EDIT_FILTER_PASSBAND_LOSS, m_EditPassLoss);
	DDX_Control(pDX, IDC_EDIT_FILTER_PASS_FREQUENCY, m_EditPassFrequency);
	DDX_Control(pDX, IDC_EDIT_FILTER_STOPBAND_LOSS, m_EditStopLoss);
	DDX_Control(pDX, IDC_EDIT_FILTER_STOP_FREQUENCY, m_EditStopFrequency);
	//{{AFX_DATA_MAP(CFilterDialog)
	DDX_Check(pDX, IDC_CHECK_UNDO, m_bUndo);
	//}}AFX_DATA_MAP
	m_pGraphWnd->DoDataExchange(pDX);

	if ( ! pDX->m_bSaveAndValidate)
	{
		UpdateEditBoxes();

		m_pGraphWnd->RebuildFilters();
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
	ON_BN_CLICKED(IDC_CHECK_STOPBAND, OnCheckStopband)
	ON_BN_CLICKED(IDC_CHECK_HILBERT_TRANSFORM, OnCheckHilbertTransform)
	ON_EN_KILLFOCUS(IDC_EDIT_FILTER_PASSBAND_LOSS, OnKillfocusEditPassbandLoss)
	ON_EN_KILLFOCUS(IDC_EDIT_FILTER_PASS_FREQUENCY, OnKillfocusEditPassbandFrequency)
	ON_EN_KILLFOCUS(IDC_EDIT_FILTER_STOPBAND_LOSS, OnKillfocusEditStopbandLoss)
	ON_EN_KILLFOCUS(IDC_EDIT_FILTER_STOP_FREQUENCY, OnKillfocusEditStopbandFrequency)
	ON_UPDATE_COMMAND_UI(IDC_EDIT_FILTER_PASSBAND_LOSS, OnUpdateEditPassbandLoss)
	ON_UPDATE_COMMAND_UI(IDC_EDIT_FILTER_STOPBAND_LOSS, OnUpdateEditStopbandLoss)
	//}}AFX_MSG_MAP
	ON_NOTIFY(NM_RETURN, AFX_IDW_PANE_FIRST, OnNotifyGraph)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CFilterDialog message handlers

void CFilterDialog::OnCheckLowpass()
{
	m_pGraphWnd->EnableLowPass(1 == IsDlgButtonChecked(IDC_CHECK_LOWPASS));
}

void CFilterDialog::OnCheckHighpass()
{
	m_pGraphWnd->EnableHighPass(1 == IsDlgButtonChecked(IDC_CHECK_HIGHPASS));
}

void CFilterDialog::OnCheckStopband()
{
	m_pGraphWnd->EnableNotch(1 == IsDlgButtonChecked(IDC_CHECK_STOPBAND));
}

void CFilterDialog::OnCheckHilbertTransform()
{
	if (1 == IsDlgButtonChecked(IDC_CHECK_HILBERT_TRANSFORM))
	{
		//m_pGraphWnd->EnableLowPass(FALSE);
		//m_pGraphWnd->EnableHighPass(FALSE);
		//m_pGraphWnd->EnableNotch(FALSE);
		m_pGraphWnd->EnableHilbert(TRUE);
		EnableDlgItem(IDC_CHECK_LOWPASS, FALSE);
		EnableDlgItem(IDC_CHECK_HIGHPASS, FALSE);
		EnableDlgItem(IDC_CHECK_STOPBAND, FALSE);
	}
	else
	{
		//m_pGraphWnd->EnableLowPass(1 == IsDlgButtonChecked(IDC_CHECK_LOWPASS));
		//m_pGraphWnd->EnableHighPass(1 == IsDlgButtonChecked(IDC_CHECK_HIGHPASS));
		//m_pGraphWnd->EnableNotch(1 == IsDlgButtonChecked(IDC_CHECK_STOPBAND));
		EnableDlgItem(IDC_CHECK_LOWPASS, TRUE);
		EnableDlgItem(IDC_CHECK_HIGHPASS, TRUE);
		EnableDlgItem(IDC_CHECK_STOPBAND, TRUE);
		m_pGraphWnd->EnableHilbert(FALSE);
	}
}

BOOL CFilterDialog::OnInitDialog()
{
	CRect r;
	// get the placeholder
	CWnd * pTemplateWnd = GetDlgItem(IDC_STATIC_RESPONSE_TEMPLATE);
	if (NULL == pTemplateWnd)
	{
		EndDialog(IDCANCEL);
		return TRUE;
	}

	pTemplateWnd->GetWindowRect( & r);

	ScreenToClient( & r);
	// create graph window the same size as placeholder
	m_pGraphWnd->Create(NULL, _T(""), WS_CHILD | WS_VISIBLE
						| WS_CLIPSIBLINGS | WS_CLIPCHILDREN | WS_TABSTOP,
						r, this, AFX_IDW_PANE_FIRST);
	m_pGraphWnd->SetWindowPos(pTemplateWnd, 0, 0, 0, 0,
							SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOSIZE);

	pTemplateWnd->DestroyWindow();

	BaseClass::OnInitDialog();
	SetBigAndSmallIcons(IDI_ICON_FILTER_DIALOG);

	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

void CFilterDialog::OnButtonResetBands()
{
	m_pGraphWnd->ResetToInitial();

	CheckDlgButton(IDC_CHECK_ZERO_PHASE, m_pGraphWnd->IsZeroPhase());
	CheckDlgButton(IDC_CHECK_LOWPASS, m_pGraphWnd->LowPassEnabled());
	CheckDlgButton(IDC_CHECK_HIGHPASS, m_pGraphWnd->HighPassEnabled());
	CheckDlgButton(IDC_CHECK_STOPBAND, m_pGraphWnd->NotchEnabled());

	UpdateEditBoxes();

	m_pGraphWnd->RebuildFilters();
}

void CFilterDialog::OnButtonLoad()
{
	UpdateData(TRUE);

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

	m_pGraphWnd->ValidateFilterSettings();

	UpdateData(FALSE);
}

void CFilterDialog::OnButtonSaveAs()
{
	if (!UpdateData(TRUE))
	{
		TRACE("UpdateData failed.\n");
		// the UpdateData routine will set focus to correct item
		return;
	}

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

void CFilterDialog::OnOK()
{
	int FocusId = GetFocus()->GetDlgCtrlID();

	if (IDC_EDIT_FILTER_PASSBAND_LOSS == FocusId)
	{
		// read the gain
		double TransferDb;
		if (m_EditPassLoss.GetData(NULL, TransferDb, NULL, NULL, -20., 0.))
		{
			m_pGraphWnd->SetCurrentFilterPassbandTransferDb(TransferDb);
			// set focus to the graph
			m_pGraphWnd->SetFocus();
		}
		return;
	}
	else if (IDC_EDIT_FILTER_PASS_FREQUENCY == FocusId)
	{
		// read the freqyqncy
		double Frequency;

		if (m_EditPassFrequency.GetData(NULL, Frequency, NULL, NULL, 1, m_pGraphWnd->GetSamplingRate()))
		{
			m_pGraphWnd->SetCurrentFilterPassbandFrequency(Frequency);
			// set focus to the graph
			m_pGraphWnd->SetFocus();
		}
		return;
	}
	else if (IDC_EDIT_FILTER_STOPBAND_LOSS == FocusId)
	{
		// read the gain
		double TransferDb;
		if (m_EditStopLoss.GetData(NULL, TransferDb, NULL, NULL, -120., 0.))
		{
			m_pGraphWnd->SetCurrentFilterStopbandTransferDb(TransferDb);
			// set focus to the graph
			m_pGraphWnd->SetFocus();
		}
		return;
	}
	else if (IDC_EDIT_FILTER_STOP_FREQUENCY == FocusId)
	{
		// read the freqyqncy
		double Frequency;

		if (m_EditPassFrequency.GetData(NULL, Frequency, NULL, NULL, 1, m_pGraphWnd->GetSamplingRate()))
		{
			m_pGraphWnd->SetCurrentFilterStopbandFrequency(Frequency);
			// set focus to the graph
			m_pGraphWnd->SetFocus();
		}
		return;
	}

	BaseClass::OnOK();
	m_Profile.FlushAll();
}

void CFilterDialog::OnNotifyGraph(NMHDR * /*pNotifyStruct*/,
								LRESULT * /*result*/)
{
	UpdateEditBoxes();
}

void CFilterDialog::OnCheckZeroPhase()
{
	m_pGraphWnd->SetZeroPhase(1 == IsDlgButtonChecked(IDC_CHECK_ZERO_PHASE));
}

void CFilterDialog::OnKillfocusEditPassbandFrequency()
{
	// read the frequency
	double Frequency;
	if (m_EditPassFrequency.GetData(NULL, Frequency, NULL, NULL, 1, m_pGraphWnd->GetSamplingRate()))
	{
		m_pGraphWnd->SetCurrentFilterPassbandFrequency(Frequency);
	}
}

void CFilterDialog::OnKillfocusEditPassbandLoss()
{
	// read the loss
	double TransferDb;
	if (m_EditPassLoss.GetData(NULL, TransferDb, NULL, NULL, -20., 0.))
	{
		m_pGraphWnd->SetCurrentFilterPassbandTransferDb(TransferDb);
	}
}

void CFilterDialog::OnKillfocusEditStopbandFrequency()
{
	// read the frequency
	double Frequency;
	if (m_EditStopFrequency.GetData(NULL, Frequency, NULL, NULL, 1, m_pGraphWnd->GetSamplingRate()))
	{
		m_pGraphWnd->SetCurrentFilterStopbandFrequency(Frequency);
	}
}

void CFilterDialog::OnKillfocusEditStopbandLoss()
{
	// read the loss
	double TransferDb;
	if (m_EditStopLoss.GetData(NULL, TransferDb, NULL, NULL, -120., 0.))
	{
		m_pGraphWnd->SetCurrentFilterStopbandTransferDb(TransferDb);
	}
}

void CFilterDialog::OnUpdateEditPassbandLoss(CCmdUI * pCmdUI)
{
	pCmdUI->Enable(m_pGraphWnd->GetCurrentFilter() != NotchFilterIndex);
}

void CFilterDialog::OnUpdateEditStopbandLoss(CCmdUI * pCmdUI)
{
	pCmdUI->Enable(m_pGraphWnd->GetCurrentFilter() != NotchFilterIndex);
}

void CFilterDialog::GetFilterCoefficients(FilterCoefficients * coeffs) const
{
	coeffs->m_bZeroPhase = m_pGraphWnd->IsZeroPhase();
	coeffs->m_nLpfOrder = m_pGraphWnd->GetLowpassFilterOrder();
	coeffs->m_nHpfOrder = m_pGraphWnd->GetHighpassFilterOrder();
	coeffs->m_nNotchOrder = m_pGraphWnd->GetNotchFilterOrder();

	m_pGraphWnd->GetLpfCoefficients(coeffs->m_LpfCoeffs);
	m_pGraphWnd->GetHpfCoefficients(coeffs->m_HpfCoeffs);
	m_pGraphWnd->GetNotchCoefficients(coeffs->m_NotchCoeffs);
}

void CFilterDialog::UpdateEditBoxes()
{
	m_EditPassLoss.SetData(m_pGraphWnd->GetCurrentFilterPassbandTransferDb());
	m_EditPassFrequency.SetData(m_pGraphWnd->GetCurrentFilterPassbandFrequency());
	m_EditStopLoss.SetData(m_pGraphWnd->GetCurrentFilterStopbandTransferDb());
	m_EditStopFrequency.SetData(m_pGraphWnd->GetCurrentFilterStopbandFrequency());

	NeedUpdateControls();
}

