#if !defined(AFX_FILTERDIALOG_H__E4CA69D8_E2BD_4C89_AB70_2F3F8C35FF3B__INCLUDED_)
#define AFX_FILTERDIALOG_H__E4CA69D8_E2BD_4C89_AB70_2F3F8C35FF3B__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// FilterDialog.h : header file
//
#include "FilterMath.h"
enum { MaxFilterOrder = 16, };
enum
{
	HpfNotchIndex = 0,
	HpfStopbandIndex,
	HpfPassbandIndex,
	NotchBeginIndex,
	NotchEndIndex,
	LpfPassbandIndex,
	LpfStopbandIndex,
	MaxFilterFrequencies
};

class Filter
{
public:
	Filter();
	void RebuildFilters();
	void ResetBands();

	// frequency is in radians
	complex<float> CalculateResponse(double Frequency);
	void CalculateCoefficients(double Gain1, double Frequency1,
								double Gain2, double Frequency2);
	BOOL CreateLowpassElliptic(double PassFreq, double PassLoss,
								double StopFreq, double StopLoss);
	BOOL CreateHighpassElliptic(double PassFreq, double PassLoss,
								double StopFreq, double StopLoss);

	// the coefficients are: 3 numerator's coeffs and 3 denominator's coeffs

	// frequencies are in radians
	double m_Frequencies[MaxFilterFrequencies];
	double m_Gain[MaxFilterFrequencies];

	BOOL	m_bLowPass;
	int     m_nLpfOrder;    // low pass filter order
	double m_LpfCoeffs[MaxFilterOrder][6];

	BOOL	m_bHighPass;
	int     m_nHpfOrder;    // high pass filter order
	double m_HpfCoeffs[MaxFilterOrder][6];

	BOOL    m_bNotchFilter;
	int     m_nNotchOrder;
	double m_NotchCoeffs[MaxFilterOrder][6];

	BOOL	m_bZeroPhase;
};

class CFilterGraphWnd : public CWnd, public Filter
{
	// Construction
public:
	CFilterGraphWnd();

	// Attributes
public:

	void SetPointGain(int nPoint, double Gain);
	void SetPointFrequency(int nPoint, double Frequency);
	void SetPointFrequencyHz(int nPoint, double Frequency)
	{
		SetPointFrequency(nPoint, Frequency / m_SamplingRate * (2. * M_PI));
	}
	void SetCurrentPointGain(double Gain)
	{
		SetPointGain(m_PointWithFocus, Gain);
	}
	void SetCurrentPointFrequency(double Frequency)
	{
		SetPointFrequency(m_PointWithFocus, Frequency);
	}
	void SetCurrentPointFrequencyHz(double Frequency)
	{
		SetPointFrequencyHz(m_PointWithFocus, Frequency);
	}
	double GetCurrentPointGain()
	{
		return m_Gain[m_PointWithFocus];
	}
	double GetCurrentPointFrequency()
	{
		return m_Frequencies[m_PointWithFocus];
	}
	double GetPointFrequencyHz(int nPoint)
	{
		return m_SamplingRate * m_Frequencies[nPoint] / (2. * M_PI);
	}
	double GetCurrentPointFrequencyHz()
	{
		return m_SamplingRate * m_Frequencies[m_PointWithFocus] / (2. * M_PI);
	}
	void SetCurrentPointGainDb(double GainDb)
	{
		SetPointGain(m_PointWithFocus, pow(10., GainDb / 20.));
	}
	double GetCurrentPointGainDb()
	{
		return 20. * log10(m_Gain[m_PointWithFocus]);
	}
	void SetFocusPoint(int nPoint);

	int GetHitCode(POINT point);

	// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CEqualizerGraphWnd)
protected:
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	//}}AFX_VIRTUAL

// Implementation
	void DrawDotCaret(bool state = true);
//    void ShowDotCaret(bool state = true);
public:
	virtual ~CFilterGraphWnd();
	bool m_bMouseCaptured;
	bool m_bButtonPressed;
	bool m_bGotFocus;
	bool m_DotCaretIsOn;

	int m_PointWithFocus;
	double m_SamplingRate;
	// Generated message map functions
protected:
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
	afx_msg void OnTimer(UINT nIDEvent);
	afx_msg void OnLButtonDblClk(UINT nFlags, CPoint point);
	//}}AFX_MSG
	afx_msg void OnNcPaint(UINT wParam);
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////
// CFilterDialog dialog

class CFilterDialog : public CDialog
{
// Construction
public:
	CFilterDialog(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CFilterDialog)
	enum { IDD = IDD_DIALOG_FILTER };
	CStatic	m_SelectionStatic;
	BOOL	m_bUndo;
	//}}AFX_DATA
	CNumEdit m_EditGain;
	CNumEdit m_EditFrequency;

	MINMAXINFO m_mmxi;
	BOOL	m_bLockChannels;
	long m_Start;
	long m_End;
	long m_CaretPosition;
	long m_FileLength;
	int m_Chan;
	int m_TimeFormat;
	const WAVEFORMATEX * m_pWf;
	int m_DlgWidth;
	int m_DlgHeight;

	CFilterGraphWnd m_wGraph;

	CApplicationProfile m_Profile;
// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CFilterDialog)
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	CSize m_PrevSize;
	void UpdateSelectionStatic();
	void OnMetricsChange();

	void OnNotifyGraph( NMHDR * pNotifyStruct, LRESULT * result );
	void OnKillfocusEditBandGain();
	// Generated message map functions
	//{{AFX_MSG(CFilterDialog)
	afx_msg void OnButtonLoad();
	afx_msg void OnButtonResetBands();
	afx_msg void OnButtonSaveAs();
	afx_msg void OnButtonSelection();
	afx_msg void OnCheckZeroPhase();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnSizing(UINT fwSide, LPRECT pRect);
	afx_msg void OnGetMinMaxInfo(MINMAXINFO FAR* lpMMI);
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	afx_msg UINT OnNcHitTest(CPoint point);
	afx_msg void OnCheckLowpass();
	afx_msg void OnCheckHighpass();
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	afx_msg void OnLButtonDblClk(UINT nFlags, CPoint point);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_FILTERDIALOG_H__E4CA69D8_E2BD_4C89_AB70_2F3F8C35FF3B__INCLUDED_)
