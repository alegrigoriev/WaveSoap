#if !defined(AFX_OPERATIONDIALOGS_H__F0C2FDC2_64C6_11D4_9ADD_00C0F0583C4B__INCLUDED_)
#define AFX_OPERATIONDIALOGS_H__F0C2FDC2_64C6_11D4_9ADD_00C0F0583C4B__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// OperationDialogs.h : header file
//
#include "NumEdit.h"

/////////////////////////////////////////////////////////////////////////////
// CCopyChannelsSelectDlg dialog

class CCopyChannelsSelectDlg : public CDialog
{
// Construction
public:
	CCopyChannelsSelectDlg(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CCopyChannelsSelectDlg)
	enum { IDD = IDD_DIALOG_COPY_CHANNELS_SELECT };
	int		m_ChannelToCopy;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CCopyChannelsSelectDlg)
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CCopyChannelsSelectDlg)
	// NOTE: the ClassWizard will add member functions here
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};
/////////////////////////////////////////////////////////////////////////////
// CPasteModeDialog dialog

class CPasteModeDialog : public CDialog
{
// Construction
public:
	CPasteModeDialog(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CPasteModeDialog)
	enum { IDD = IDD_DIALOG_PASTE_MODE_SELECT };
	int		m_PasteMode;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CPasteModeDialog)
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CPasteModeDialog)
	// NOTE: the ClassWizard will add member functions here
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};
/////////////////////////////////////////////////////////////////////////////
// CVolumeChangeDialog dialog

class CVolumeChangeDialog : public CDialog
{
// Construction
public:
	CVolumeChangeDialog(CWnd* pParent = NULL);   // standard constructor
	void SetTemplate(UINT id)
	{
		m_lpszTemplateName = MAKEINTRESOURCE(id);
	}

// Dialog Data
	//{{AFX_DATA(CVolumeChangeDialog)
	enum { IDD = IDD_DIALOG_VOLUME_CHANGE };
	CSliderCtrl	m_SliderVolumeRight;
	CSliderCtrl	m_SliderVolumeLeft;
	CStatic	m_SelectionStatic;
	CNumEdit	m_eVolumeRight;
	CNumEdit	m_eVolumeLeft;
	BOOL	m_bUndo;
	BOOL	m_bLockChannels;
	int		m_DbPercent;
	//}}AFX_DATA

	double m_dVolumeLeftDb;
	double m_dVolumeRightDb;
	double m_dVolumeLeftPercent;
	double m_dVolumeRightPercent;
	long m_Start;
	long m_End;
	int m_Chan;
	int m_TimeFormat;
	const WAVEFORMATEX * m_pWf;
// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CVolumeChangeDialog)
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	void UpdateVolumeData(CDataExchange* pDX, BOOL InPercents);
	void CVolumeChangeDialog::UpdateSelectionStatic();
	// Generated message map functions
	//{{AFX_MSG(CVolumeChangeDialog)
	afx_msg void OnChecklockChannels();
	afx_msg void OnButtonSelection();
	afx_msg void OnSelchangeCombodbPercent();
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////
// CSelectionDialog dialog

class CSelectionDialog : public CDialog
{
// Construction
public:
	CSelectionDialog(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CSelectionDialog)
	enum { IDD = IDD_SELECTION_DIALOG };
	// NOTE: the ClassWizard will add data members here
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CSelectionDialog)
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CSelectionDialog)
	// NOTE: the ClassWizard will add member functions here
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};
//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_OPERATIONDIALOGS_H__F0C2FDC2_64C6_11D4_9ADD_00C0F0583C4B__INCLUDED_)
