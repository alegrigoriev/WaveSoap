#if !defined(AFX_OPERATIONDIALOGS2_H__908708EA_C03D_430C_A6BA_E92753C9E84F__INCLUDED_)
#define AFX_OPERATIONDIALOGS2_H__908708EA_C03D_430C_A6BA_E92753C9E84F__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// OperationDialogs2.h : header file
//
#include "TimeEdit.h"

#include "CdDrive.h"
#include <vector>
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
	CTimeSpinCtrl	m_SpinStart;
	CTimeSpinCtrl	m_SpinLength;
	CTimeEdit	m_eLength;
	CTimeEditCombo	m_eStart;
	int		m_TimeFormatIndex;
	//}}AFX_DATA

	int		m_nChannel;
	int		m_TimeFormat;
	long    m_Length;
	long    m_Start;
	long    m_FileLength;
	long    m_CaretPosition;
	WAVEFORMATEX * m_pWf;

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CInsertSilenceDialog)
public:
	virtual int DoModal();
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
	enum { IDD = IDD_DIALOG_NO_WINDOWS_MEDIA_TYPE };
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

struct CdTrackInfo
{
	CString Artist;
	CString Album;
	CString Track;
	bool Checked;
	bool IsAudio;
	CdAddressMSF TrackBegin;
	LONG NumSectors;
	CdTrackInfo()
	{
		Checked = FALSE;
		NumSectors = 0;
		TrackBegin.reserved = 0;
		TrackBegin.Minute = 0;
		TrackBegin.Second = 0;
		TrackBegin.Frame = 0;
	}
};

class CCdGrabbingDialog : public CDialog
{
// Construction
public:
	CCdGrabbingDialog(CWnd* pParent = NULL);   // standard constructor
	~CCdGrabbingDialog();

// Dialog Data
	//{{AFX_DATA(CCdGrabbingDialog)
	enum { IDD = IDD_DIALOG_CD_GRABBING };
	CStatic	m_StaticFormat;
	CComboBox	m_SpeedCombo;
	CComboBox	m_DrivesCombo;
	CListCtrl	m_lbTracks;
	int		m_RadioAssignAttributes;
	int		m_RadioStoreImmediately;
	int		m_RadioStoreMultiple;
	//}}AFX_DATA

	CCdDrive m_CdDrive;

	TCHAR m_CDDrives['Z' - 'A' + 1];
	int m_NumberOfDrives;
	int m_CDDriveSelected;
	TCHAR m_DriveLetterSelected;

	CDROM_TOC m_toc;
	vector<CdTrackInfo> m_Tracks;

	DWORD m_DiskID;

	// speed is in bytes/s, rounded to nearest multiple of 176400
	int m_MaxReadSpeed;
	int m_SelectedReadSpeed;
	enum DiskState
	{
		DiskStateUnknown,
		DiskStateReady,
		DiskStateNotReady,
		DiskStateNoCdDrive,
	} m_DiskReady;

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CCdGrabbingDialog)
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	BOOL m_bNeedUpdateControls;
	CSize m_PreviousSize;
	MINMAXINFO m_mmxi;
	void FillTrackList(TCHAR letter);
	void ReloadTrackList();
	void InitReadSpeedCombobox();

	BOOL OpenDrive(TCHAR letter);

	void FillDriveList(TCHAR SelectDrive);
	void CreateImageList();
	void CheckForDiskChanged();
	void CheckForDrivesChanged();

	// Generated message map functions
	LRESULT OnKickIdle(WPARAM, LPARAM);
	//{{AFX_MSG(CCdGrabbingDialog)
	virtual BOOL OnInitDialog();
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnSizing(UINT fwSide, LPRECT pRect);
	afx_msg UINT OnNcHitTest(CPoint point);
	afx_msg void OnGetMinMaxInfo(MINMAXINFO FAR* lpMMI);
	afx_msg void OnTimer(UINT nIDEvent);
	afx_msg void OnButtonMore();
	afx_msg void OnSelchangeComboDrives();
	afx_msg void OnDestroy();
	afx_msg void OnButtonBrowseSaveFolder();
	afx_msg void OnButtonCddb();
	afx_msg void OnButtonDeselectAll();
	afx_msg void OnButtonSelectAll();
	afx_msg void OnButtonSetFormat();
	afx_msg void OnChangeEditAlbum();
	afx_msg void OnChangeEditArtist();
	afx_msg void OnRadioStoreMultipleFiles();
	afx_msg void OnRadioStoreSingleFile();
	afx_msg void OnClickListTracks(NMHDR* pNMHDR, LRESULT* pResult);
	//}}AFX_MSG
	void OnMetricsChange();
	afx_msg LRESULT OnDeviceChange(UINT, DWORD);
	DECLARE_MESSAGE_MAP()
};
/////////////////////////////////////////////////////////////////////////////
// CReopenDialog dialog

class CReopenDialog : public CDialog
{
// Construction
public:
	CReopenDialog(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CReopenDialog)
	enum { IDD = IDD_DIALOG_REOPEN_DIRECT };
	CString	m_Prompt;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CReopenDialog)
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CReopenDialog)
	afx_msg void OnNo();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};
//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_OPERATIONDIALOGS2_H__908708EA_C03D_430C_A6BA_E92753C9E84F__INCLUDED_)
