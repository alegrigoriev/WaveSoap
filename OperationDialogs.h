#if !defined(AFX_OPERATIONDIALOGS_H__F0C2FDC2_64C6_11D4_9ADD_00C0F0583C4B__INCLUDED_)
#define AFX_OPERATIONDIALOGS_H__F0C2FDC2_64C6_11D4_9ADD_00C0F0583C4B__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// OperationDialogs.h : header file
//
#include "NumEdit.h"
#include "TimeEdit.h"

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
	long m_FileLength;
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
	void UpdateSelectionStatic();
	void UpdateEnables();
	// Generated message map functions
	//{{AFX_MSG(CVolumeChangeDialog)
	afx_msg void OnChecklockChannels();
	afx_msg void OnButtonSelection();
	afx_msg void OnSelchangeCombodbPercent();
	virtual BOOL OnInitDialog();
	afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	afx_msg void OnKillfocusEditVolumeLeft();
	afx_msg void OnKillfocusEditVolumeRight();
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
	CTimeEdit	m_eLength;
	CTimeEdit	m_eStart;
	CTimeEdit	m_eEnd;
	int		m_Chan;
	int		m_TimeFormatIndex;
	int		m_SelectionNumber;
	//}}AFX_DATA
	int		m_TimeFormat;
	long m_Start;
	long m_End;
	long m_Length;
	long m_FileLength;
	const WAVEFORMATEX * m_pWf;

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
	virtual BOOL OnInitDialog();
	afx_msg void OnSelchangeComboTimeFormat();
	afx_msg void OnKillfocusEditEnd();
	afx_msg void OnKillfocusEditLength();
	afx_msg void OnKillfocusEditStart();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};
/////////////////////////////////////////////////////////////////////////////
// CGotoDialog dialog

class CGotoDialog : public CDialog
{
// Construction
public:
	CGotoDialog(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CGotoDialog)
	enum { IDD = IDD_DIALOG_GOTO };
	CTimeEdit	m_eStart;
	int		m_TimeFormatIndex;
	//}}AFX_DATA
	int m_TimeFormat;
	long m_Position;
	long m_FileLength;
	const WAVEFORMATEX * m_pWf;
// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CGotoDialog)
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CGotoDialog)
	afx_msg void OnSelchangeComboTimeFormat();
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};
/////////////////////////////////////////////////////////////////////////////
// CDcOffsetDialog dialog

class CDcOffsetDialog : public CDialog
{
// Construction
public:
	CDcOffsetDialog(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CDcOffsetDialog)
	enum { IDD = IDD_DIALOG_DC_OFFSET };
	BOOL	m_b5SecondsDC;
	BOOL	m_bUndo;
	int		m_nDcOffset;
	int		m_DcSelectMode;
	//}}AFX_DATA

	long m_Start;
	long m_End;
	long m_FileLength;
	int m_Chan;
	int m_TimeFormat;
	const WAVEFORMATEX * m_pWf;
// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CDcOffsetDialog)
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	void UpdateSelectionStatic();

	// Generated message map functions
	//{{AFX_MSG(CDcOffsetDialog)
	afx_msg void OnButtonSelection();
	afx_msg void OnRadioDcSelect();
	afx_msg void OnRadioAdjustSelectEdit();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};
/////////////////////////////////////////////////////////////////////////////
// CStatisticsDialog dialog

class CStatisticsDialog : public CDialog
{
// Construction
public:
	CStatisticsDialog(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CStatisticsDialog)
	enum { IDD = IDD_DIALOG_STATISTICS };
	// NOTE: the ClassWizard will add data members here
	//}}AFX_DATA

	CStatisticsContext * m_pContext;
	long m_SamplesPerSec;
	long m_Cursor;
	int m_ValueAtCursorLeft;
	int m_ValueAtCursorRight;
// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CStatisticsDialog)
public:
	virtual int DoModal();
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CStatisticsDialog)
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};
/////////////////////////////////////////////////////////////////////////////
// CNormalizeSoundDialog dialog

class CNormalizeSoundDialog : public CDialog
{
// Construction
public:
	CNormalizeSoundDialog(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CNormalizeSoundDialog)
	enum { IDD = IDD_DIALOG_NORMALIZE };
	CStatic	m_SelectionStatic;
	CSliderCtrl	m_SliderLevel;
	CNumEdit	m_eLevel;
	BOOL	m_bLockChannels;
	BOOL	m_bUndo;
	int		m_DbPercent;
	//}}AFX_DATA

	double m_dLevelDb;
	double m_dLevelPercent;

	long m_Start;
	long m_End;
	long m_FileLength;
	int m_Chan;
	int m_TimeFormat;
	const WAVEFORMATEX * m_pWf;
// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CNormalizeSoundDialog)
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	void UpdateSelectionStatic();

	// Generated message map functions
	//{{AFX_MSG(CNormalizeSoundDialog)
	afx_msg void OnKillfocusEditLevel();
	afx_msg void OnButtonSelection();
	afx_msg void OnSelchangeCombodbPercent();
	virtual BOOL OnInitDialog();
	afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};
/////////////////////////////////////////////////////////////////////////////
// CResampleDialog dialog

class CResampleDialog : public CDialog
{
// Construction
public:
	CResampleDialog(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CResampleDialog)
	enum { IDD = IDD_DIALOG_RESAMPLE };
	CSliderCtrl	m_SliderTempo;
	CSliderCtrl	m_SliderRate;
	CNumEdit	m_EditTempo;
	BOOL	m_bChangeRateOnly;
	BOOL	m_bUndo;
	int		m_bChangeSamplingRate;
	UINT	m_ResampleSamplingRate;
	//}}AFX_DATA

	double m_TempoChange;
	int m_NewSampleRate;
	int m_OldSampleRate;
// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CResampleDialog)
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CResampleDialog)
	afx_msg void OnRadioChangeRate();
	afx_msg void OnRadioChangeTempo();
	afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	afx_msg void OnKillfocusEditRate();
	afx_msg void OnKillfocusEditTempo();
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};
//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_OPERATIONDIALOGS_H__F0C2FDC2_64C6_11D4_9ADD_00C0F0583C4B__INCLUDED_)
