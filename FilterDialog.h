// Copyright Alexander Grigoriev, 1997-2002, All Rights Reserved
#if !defined(AFX_FILTERDIALOG_H__E4CA69D8_E2BD_4C89_AB70_2F3F8C35FF3B__INCLUDED_)
#define AFX_FILTERDIALOG_H__E4CA69D8_E2BD_4C89_AB70_2F3F8C35FF3B__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// FilterDialog.h : header file
//
#include "ResizableDialog.h"
#include "DialogWithSelection.h"
#include "NumEdit.h"
#include "resource.h"       // main symbols
#include <complex>

enum { MaxFilterOrder = 16, };
enum
{
	HpfStopbandIndex,
	HpfPassbandIndex,
	NotchBeginIndex,
	NotchZeroIndex,
	LpfPassbandIndex,
	LpfStopbandIndex,
	MaxFilterFrequencies,
	LeftmostPoint = -1,
	RightmostPoint = MaxFilterFrequencies + 1,

	HighpassFilter = HpfStopbandIndex,
	LowpassFilter = LpfPassbandIndex,
	NotchFilter = NotchBeginIndex,
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
		return m_bLowPass;
	}
	BOOL HighPassEnabled() const
	{
		return m_bHighPass;
	}
	BOOL NotchEnabled() const
	{
		return m_bNotchFilter;
	}

	int GetLowpassFilterOrder() const;

	int GetHighpassFilterOrder() const;

	int GetNotchFilterOrder() const;

	void GetLpfCoefficients(double Coeffs[MaxFilterOrder][6]);

	void GetHpfCoefficients(double Coeffs[MaxFilterOrder][6]);

	void GetNotchCoefficients(double Coeffs[MaxFilterOrder][6]);

	void EnableLowPass(bool Enable = true);
	void EnableHighPass(bool Enable = true);
	void EnableNotch(bool Enable = true);
	void SetZeroPhase(bool ZeroPhase = true);

	// frequency is in radians
	std::complex<float> CalculateResponse(double Frequency);
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
	void SetZeroPhase(bool ZeroPhase = true);

	void SetPointGainDb(int nPoint, double Gain);
	void SetPointFrequency(int nPoint, double Frequency);
	void SetPointFrequencyHz(int nPoint, double Frequency);
	void SetCurrentPointFrequency(double Frequency);
	void SetCurrentPointFrequencyHz(double Frequency);
	double GetCurrentPointGain() const;
	double GetCurrentPointFrequency() const;
	double GetPointGainDb(unsigned nPoint) const;
	double GetPointFrequencyHz(unsigned nPoint) const;
	double GetCurrentPointFrequencyHz() const;
	void SetCurrentPointGainDb(double GainDb);
	double GetCurrentPointGainDb() const;
	double GetSamplingRate() const
	{
		return m_SamplingRate;
	}

	int GetCurrentFilter() const;

	int GetCurrentFilterPassbandIndex() const;
	int GetCurrentFilterStopbandIndex() const;

	void SetCurrentFilterPassbandFrequency(double Frequency);
	void SetCurrentFilterPassbandLossDb(double Loss);
	void SetCurrentFilterStopbandFrequency(double Frequency);
	void SetCurrentFilterStopbandLossDb(double Loss);

	double GetCurrentFilterPassbandFrequency() const;
	double GetCurrentFilterPassbandLossDb() const;
	double GetCurrentFilterStopbandFrequency() const;
	double GetCurrentFilterStopbandLossDb() const;

	void SetFocusPoint(unsigned nPoint);

	int GetHitCode(POINT point);

	// get pixel where the point is drawn (x coordinate in client area)
	int GetFilterPointPixel(int FilterPoint);
	void SetFilterPointPixel(int FilterPoint, int PointPixel, BOOL MoveBoth);
	void InvalidateGraphPoint(double Frequency, double Gain);
	static double PosYToGainDb(int y, int height)
	{
		//return 20. * (0.25 - y * 4.5 / height);
		return 5. - y * 90. / height;
	}
	static int GainToPosY(double Gain, int height)
	{
		// full range: 5 to -85 db
		return int((0.25 - log10(Gain)) * height / 4.5);
	}
	static int GainDbToPosY(double GainDb, int height)
	{
		// full range: 5 to -85 db
		return int((5. - GainDb) * height / 90.);
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
	afx_msg void OnTimer(UINT nIDEvent);
	afx_msg void OnLButtonDblClk(UINT nFlags, CPoint point);
	//}}AFX_MSG
	afx_msg void OnNcPaint(UINT wParam);
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////
// CFilterDialog dialog

class CFilterDialog : public CDialogWithSelectionT<CResizableDialog>
{
	typedef CDialogWithSelectionT<CResizableDialog> BaseClass;
// Construction
public:
	CFilterDialog(SAMPLE_INDEX Start,
				SAMPLE_INDEX End,
				SAMPLE_INDEX CaretPosition,
				CHANNEL_MASK Channels,
				CWaveFile & WaveFile,
				int m_TimeFormat,
				BOOL bLockChannels,
				BOOL	bUndoEnabled,
				CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CFilterDialog)
	enum { IDD = IDD_DIALOG_FILTER };
	//}}AFX_DATA
	BOOL IsZeroPhase() const
	{
		return m_wGraph.IsZeroPhase();
	}

	int GetLowpassFilterOrder() const
	{
		return m_wGraph.GetLowpassFilterOrder();
	}

	int GetHighpassFilterOrder() const
	{
		return m_wGraph.GetHighpassFilterOrder();
	}

	int GetNotchFilterOrder() const
	{
		return m_wGraph.GetNotchFilterOrder();
	}

	void GetLpfCoefficients(double Coeffs[MaxFilterOrder][6])
	{
		m_wGraph.GetLpfCoefficients(Coeffs);
	}

	void GetHpfCoefficients(double Coeffs[MaxFilterOrder][6])
	{
		m_wGraph.GetHpfCoefficients(Coeffs);
	}

	void GetNotchCoefficients(double Coeffs[MaxFilterOrder][6])
	{
		m_wGraph.GetNotchCoefficients(Coeffs);
	}

protected:
// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CFilterDialog)
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual void OnOK();
	virtual BOOL OnInitDialog();
	//}}AFX_VIRTUAL

// Implementation

	CApplicationProfile m_Profile;  // goes before m_wGraph

	CNumEdit m_EditPassLoss;
	CNumEdit m_EditPassFrequency;

	CNumEdit m_EditStopLoss;
	CNumEdit m_EditStopFrequency;

	CFilterGraphWnd m_wGraph;

	void OnNotifyGraph(NMHDR * pNotifyStruct, LRESULT * result );

	void OnKillfocusEditBandGain();
	// Generated message map functions
	//{{AFX_MSG(CFilterDialog)
	afx_msg void OnButtonLoad();
	afx_msg void OnButtonResetBands();
	afx_msg void OnButtonSaveAs();
	afx_msg void OnCheckZeroPhase();
	afx_msg void OnCheckLowpass();
	afx_msg void OnCheckHighpass();
	afx_msg void OnKillfocusEditPassbandFrequency();
	afx_msg void OnKillfocusEditStopbandFrequency();
	afx_msg void OnKillfocusEditPassbandLoss();
	afx_msg void OnKillfocusEditStopbandLoss();
	afx_msg void OnUpdateEditPassbandLoss(CCmdUI * pCmdUI);
	afx_msg void OnUpdateEditStopbandLoss(CCmdUI * pCmdUI);

	afx_msg void OnCheckStopband();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_FILTERDIALOG_H__E4CA69D8_E2BD_4C89_AB70_2F3F8C35FF3B__INCLUDED_)
