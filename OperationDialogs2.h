#if !defined(AFX_OPERATIONDIALOGS2_H__908708EA_C03D_430C_A6BA_E92753C9E84F__INCLUDED_)
#define AFX_OPERATIONDIALOGS2_H__908708EA_C03D_430C_A6BA_E92753C9E84F__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// OperationDialogs2.h : header file
//
#include "TimeEdit.h"

#include "CdDrive.h"
/////////////////////////////////////////////////////////////////////////////
// CInsertSilenceDialog dialog

class CInsertSilenceDialog : public CDialog
{
// Construction
public:
	CInsertSilenceDialog(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CInsertSilenceDialog)
	enum { IDD = IDD_DIALOG_INSERT_SILENCE };
	CSpinButtonCtrl	m_SpinStart;
	CTimeSpinCtrl	m_SpinLength;
	CTimeEdit	m_eLength;
	CTimeEdit	m_eStart;
	int		m_nChannel;
	int		m_TimeFormatIndex;
	//}}AFX_DATA

	int		m_TimeFormat;
	long    m_Length;
	long    m_Start;
	WAVEFORMATEX * m_pWf;

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CInsertSilenceDialog)
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CInsertSilenceDialog)
	afx_msg void OnSelchangeComboTimeFormat();
	virtual BOOL OnInitDialog();
	afx_msg void OnKillfocusEditStart();
	afx_msg void OnKillfocusEditLength();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////
// CSilenceOptionDialog dialog

class CSilenceOptionDialog : public CDialog
{
// Construction
public:
	CSilenceOptionDialog(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CSilenceOptionDialog)
	enum { IDD = IDD_DIALOG_MUTE_OR_SILENCE };
	// NOTE: the ClassWizard will add data members here
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CSilenceOptionDialog)
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CSilenceOptionDialog)
	afx_msg void OnButtonSilence();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};
/////////////////////////////////////////////////////////////////////////////
// CWmpNotInstalleedWarningDlg dialog

class CWmpNotInstalleedWarningDlg : public CDialog
{
// Construction
public:
	CWmpNotInstalleedWarningDlg(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CWmpNotInstalleedWarningDlg)
	enum { IDD = IDD_DIALOG_NO_WINDOWS_MEDIA };
	BOOL	m_DontShowAnymore;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CWmpNotInstalleedWarningDlg)
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CWmpNotInstalleedWarningDlg)
	// NOTE: the ClassWizard will add member functions here
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};
/////////////////////////////////////////////////////////////////////////////
// CCdGrabbingDialog dialog

class CCdGrabbingDialog : public CDialog
{
// Construction
public:
	CCdGrabbingDialog(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CCdGrabbingDialog)
	enum { IDD = IDD_DIALOG_CD_GRABBING };
	CListCtrl	m_lbTracks;
	CComboBox	m_DrivesCombo;
	//}}AFX_DATA
	TCHAR m_CDDrives['Z' - 'A' + 1];
	int m_NumberOfDrives;
	int m_CDDriveSelected;
	CDROM_TOC m_toc;

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CCdGrabbingDialog)
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	CSize m_PreviousSize;
	MINMAXINFO m_mmxi;
	void FillTrackList(TCHAR letter);
	void FillDriveList();
	void CreateImageList();

	// Generated message map functions
	//{{AFX_MSG(CCdGrabbingDialog)
	virtual BOOL OnInitDialog();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnSizing(UINT fwSide, LPRECT pRect);
	afx_msg UINT OnNcHitTest(CPoint point);
	afx_msg void OnGetMinMaxInfo(MINMAXINFO FAR* lpMMI);
	//}}AFX_MSG
	void OnMetricsChange();
	DECLARE_MESSAGE_MAP()
};
//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_OPERATIONDIALOGS2_H__908708EA_C03D_430C_A6BA_E92753C9E84F__INCLUDED_)
