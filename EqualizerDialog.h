#if !defined(AFX_EQUALIZERDIALOG_H__D9EA5D94_8BAC_4E78_A268_6D4AD2625D48__INCLUDED_)
#define AFX_EQUALIZERDIALOG_H__D9EA5D94_8BAC_4E78_A268_6D4AD2625D48__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// EqualizerDialog.h : header file
//
#include <complex>
#include "NumEdit.h"
/////////////////////////////////////////////////////////////////////////////
// CEqualizerGraphWnd window

class CEqualizerGraphWnd : public CWnd
{
	// Construction
public:
	CEqualizerGraphWnd();

	// Attributes
public:
	enum { MaxNumberOfBands = 20, };
	void SetNumberOfBands(int NumBands);
	complex<float> CalculateResponse(double Frequency);
	void SetBandGain(int nBand, double Gain);
	void RebuildBandFilters();
	void ResetBands();
	void SetFocusBand(int nBand);
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
public:
	virtual ~CEqualizerGraphWnd();
	bool m_bMouseCaptured;
	bool m_bButtonPressed;
	bool m_bGotFocus;

	int m_NumOfBands;    // 2-MaxNumberOfBands
	int m_BandWithFocus;
	double m_BandGain[MaxNumberOfBands];   // target gain in the band
	double m_UsedBandGain[MaxNumberOfBands];   // gain in the band used to calculate coefficients
	double m_BandWidth;
	double m_BandFrequencies[MaxNumberOfBands];
	// the coefficients are: 3 numerator's coeffs and 3 denominator's coeffs
	double m_BandCoefficients[MaxNumberOfBands][6];
	// Generated message map functions
protected:
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
	//}}AFX_MSG
	afx_msg void OnNcPaint(UINT wParam);
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////
// CEqualizerDialog dialog

class CEqualizerDialog : public CDialog
{
// Construction
public:
	CEqualizerDialog(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CEqualizerDialog)
	enum { IDD = IDD_DIALOG_SIMPLE_EQUALIZER };
	CNumEdit	m_BandGain;
	CSpinButtonCtrl	m_SpinBands;
	CStatic	m_SelectionStatic;
	BOOL	m_bUndo;
	int		m_bMultiBandEqualizer;
	int 	m_nBands;
	//}}AFX_DATA
	MINMAXINFO m_mmxi;
	BOOL	m_bLockChannels;
	long m_Start;
	long m_End;
	long m_CaretPosition;
	long m_FileLength;
	int m_Chan;
	int m_TimeFormat;
	const WAVEFORMATEX * m_pWf;
	CApplicationProfile m_Profile;

	CEqualizerGraphWnd m_wGraph;

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CEqualizerDialog)
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	void UpdateSelectionStatic();
	void OnMetricsChange();
	// Generated message map functions
	//{{AFX_MSG(CEqualizerDialog)
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	afx_msg UINT OnNcHitTest(CPoint point);
	afx_msg void OnSizing(UINT fwSide, LPRECT pRect);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnGetMinMaxInfo(MINMAXINFO FAR* lpMMI);
	virtual BOOL OnInitDialog();
	afx_msg void OnButtonSelection();
	afx_msg void OnChangeEditBands();
	afx_msg void OnButtonLoad();
	afx_msg void OnButtonResetBands();
	afx_msg void OnButtonSaveAs();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////
//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_EQUALIZERDIALOG_H__D9EA5D94_8BAC_4E78_A268_6D4AD2625D48__INCLUDED_)
