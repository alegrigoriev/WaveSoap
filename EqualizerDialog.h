#if !defined(AFX_EQUALIZERDIALOG_H__D9EA5D94_8BAC_4E78_A268_6D4AD2625D48__INCLUDED_)
#define AFX_EQUALIZERDIALOG_H__D9EA5D94_8BAC_4E78_A268_6D4AD2625D48__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// EqualizerDialog.h : header file
//

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
	CStatic	m_SelectionStatic;
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
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_EQUALIZERDIALOG_H__D9EA5D94_8BAC_4E78_A268_6D4AD2625D48__INCLUDED_)
