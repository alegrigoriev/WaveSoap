// Copyright Alexander Grigoriev, 1997-2002, All Rights Reserved
#pragma once
// OperationDialogs2.h : header file
//
#include "TimeEdit.h"
#include "TimeToStr.h"
#include "UiUpdatedDlg.h"
#include "ResizableDialog.h"
#include "DialogWithSelection.h"
#include "OperationDialogs.h"
#include "resource.h"

#include "CdDrive.h"
#include <vector>
/////////////////////////////////////////////////////////////////////////////
// CInsertSilenceDialog dialog

class CInsertSilenceDialog : public CDialog
{
	typedef CDialog BaseClass;
// Construction
public:
	CInsertSilenceDialog(SAMPLE_INDEX Start,
						NUMBER_OF_SAMPLES Length,
						CHANNEL_MASK	Channel,
						CWaveFile & WaveFile,
						int TimeFormat,
						CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CInsertSilenceDialog)
	enum { IDD = IDD_DIALOG_INSERT_SILENCE };
	CTimeSpinCtrl	m_SpinStart;
	CTimeSpinCtrl	m_SpinLength;
	CTimeEdit	m_eLength;
	CFileTimesCombo	m_eStart;
	int		m_TimeFormatIndex;
	//}}AFX_DATA
	CHANNEL_MASK GetChannel() const
	{
		switch (m_nChannel)
		{
		default:
			return ALL_CHANNELS;
			break;
		case 1:
			return SPEAKER_FRONT_LEFT;
			break;
		case 2:
			return SPEAKER_FRONT_RIGHT;
			break;
		}
	}
	SAMPLE_INDEX GetStart() const
	{
		return m_Start;
	}

	NUMBER_OF_SAMPLES GetLength() const
	{
		return m_Length;
	}

protected:
	NUMBER_OF_SAMPLES    m_Length;
	SAMPLE_INDEX    m_Start;
	int     m_nChannel;
	int	    m_TimeFormat;
	SAMPLE_INDEX    m_CaretPosition;
	CWaveFile & m_WaveFile;

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
	afx_msg void OnBuddyChangeSpinStart(NMHDR * pNmHdr, LRESULT * pResult);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////
// CSilenceOptionDialog dialog

class CSilenceOptionDialog : public CDialog
{
	typedef CDialog BaseClass;
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
	typedef CDialog BaseClass;
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
#pragma pack(push, 8)
class CCdGrabbingDialog : public CResizableDialog
{
	typedef CResizableDialog BaseClass;
// Construction
public:
	CCdGrabbingDialog(CWnd* pParent = NULL);   // standard constructor
	~CCdGrabbingDialog();

// Dialog Data
	//{{AFX_DATA(CCdGrabbingDialog)
	enum { IDD = IDD_DIALOG_CD_GRABBING };
	CButton	m_EjectButton;
	CComboBox	m_ComboBitrate;
	CButton	m_StopButton;
	CButton	m_PlayButton;
	CComboBox	m_eArtist;
	CComboBox	m_eAlbum;
	CComboBox	m_eSaveFolder;
	CComboBox	m_SpeedCombo;
	CComboBox	m_DrivesCombo;
	CListCtrl	m_lbTracks;
	int		m_RadioAssignAttributes;
	int		m_RadioOpenInEditor;
	CString	m_sSaveFolder;
	CString	m_sAlbum;
	CString	m_sArtist;
	int		m_RadioFileFormat;
	//}}AFX_DATA

	class ICdDrive *m_pCdDrive;

	TCHAR m_CDDrives['Z' - 'A' + 1];
	TCHAR m_DriveLetterSelected;
	int m_NumberOfDrives;
	int m_CDDriveSelected;
	USHORT m_PreviousDriveLetter;

	typedef std::vector<CdTrackInfo> CdTrackInfoVector;
	CdTrackInfoVector m_Tracks;
	DWORD m_DiskID;

	// Format to save the files:
	CWaveFormat m_Wf;
	DWORD m_FileTypeFlags;
	CString m_Mp3EncoderName;

	// speed is in bytes/s, rounded to nearest multiple of 176400
	int m_MaxReadSpeed;
	int m_CurrentReadSpeed;
	int m_SelectedReadSpeed;

	CdMediaChangeState m_DiskReady;

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CCdGrabbingDialog)
public:
	virtual BOOL PreTranslateMessage(MSG* pMsg);
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	CApplicationProfile m_Profile;

	CStringHistory m_AlbumHistory;
	CStringHistory m_ArtistHistory;
	CStringHistory m_FolderHistory;

	BOOL m_bNeedUpdateControls;
	UINT_PTR m_TimerId;

	CAudioCompressionManager m_Acm;

	ULONG m_EncodedDataRate;
	ULONG m_FormatIndex;

	void FillFormatCombo();
	// Playback support:
	BOOL m_bPlayingAudio;
	int m_PlayingTrack;
	CWaveOut m_WaveOut;
	CdAddressMSF m_PlaybackAddress;
	LONG m_PlaybackSectors;
	BOOL FillPlaybackBuffers();

	void StopCdPlayback();
	void StartCdPlayback(unsigned track);


	void FillTrackList(TCHAR letter);
	void ReloadTrackList(CdMediaChangeState NewMediaState = CdMediaStateDiskChanged);
	void InitReadSpeedCombobox();

	BOOL OpenDrive(TCHAR letter);

	void FillDriveList(TCHAR SelectDrive);
	void CreateImageList();
	void CheckForDiskChanged();
	void CheckForDrivesChanged();

	CBitmap m_BmpPlay;
	CBitmap m_BmpStop;
	CBitmap m_BmpEject;
	CBitmap m_BmpLoad;

	// Generated message map functions
	LRESULT OnKickIdle(WPARAM, LPARAM);
	afx_msg void OnUpdateOk(CCmdUI* pCmdUI);
	afx_msg void OnUpdatePlay(CCmdUI* pCmdUI);
	afx_msg void OnUpdateStop(CCmdUI* pCmdUI);
	afx_msg void OnUpdateEject(CCmdUI* pCmdUI);
	afx_msg void OnUpdateComboBitrate(CCmdUI* pCmdUI);

	//{{AFX_MSG(CCdGrabbingDialog)
	afx_msg void OnSize(UINT nType, int cx, int cy);
	virtual BOOL OnInitDialog();
	afx_msg void OnTimer(UINT_PTR nIDEvent);
	afx_msg void OnSelchangeComboDrives();
	afx_msg void OnDestroy();
	afx_msg void OnButtonCddb();
	afx_msg void OnChangeEditAlbum();
	afx_msg void OnChangeEditArtist();
	afx_msg void OnClickListTracks(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnBeginlabeleditListTracks(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnEndlabeleditListTracks(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnChangeEditFolderOrFile();
	afx_msg void OnButtonPlay();
	afx_msg void OnButtonStop();
	afx_msg void OnSelchangeComboSpeed();
	afx_msg void OnItemchangedListTracks(NMHDR* pNMHDR, LRESULT* pResult);
	virtual void OnCancel();
	afx_msg void OnButtonBrowseSaveFolder();
	afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
	afx_msg void OnDblclkListTracks(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnRadioWmaFormat();
	afx_msg void OnRadioMp3Format();
	afx_msg void OnRadioWavFormat();
	afx_msg void OnButtonEject();
	afx_msg void OnSysColorChange();
	//}}AFX_MSG
	//void OnMetricsChange();
	afx_msg BOOL OnDeviceChange(UINT, DWORD_PTR);
	DECLARE_MESSAGE_MAP()
};
#pragma pack(pop)
/////////////////////////////////////////////////////////////////////////////
// CReopenDialog dialog

class CReopenDialog : public CDialog
{
	typedef CDialog BaseClass;
// Construction
public:
	CReopenDialog(UINT FormatId, LPCTSTR Name, CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CReopenDialog)
	enum { IDD = IDD_DIALOG_REOPEN_DIRECT };
	CString	m_Prompt;
	//}}AFX_DATA

	INT_PTR DoModalPopDocument(CDocument * pDoc);
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

struct WAVEREGIONINFO;
class CMarkerRegionDialog : public CUiUpdatedDlg, public CSelectionUiSupport
{
	typedef CUiUpdatedDlg BaseClass;
public:
	CMarkerRegionDialog(struct WAVEREGIONINFO * pRegionInfo, SAMPLE_INDEX CaretPos,
						CWaveFile & WaveFile, int TimeFormat,
						CWnd* pParent = NULL);   // standard constructor
	// Dialog Data
	//{{AFX_DATA(CMarkerRegionDialog)
	enum { IDD = IDD_REGION_DIALOG };
	CString m_sName;
	//}}AFX_DATA

	struct WAVEREGIONINFO * m_pRegionData;

	// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CMarkerRegionDialog)
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	//}}AFX_VIRTUAL

	// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CMarkerRegionDialog)
	afx_msg void OnUpdateEditLength(CCmdUI * pCmdUI);
	afx_msg void OnClickedMarkerRegion();
	afx_msg void OnClickedDelete();
	afx_msg void OnUpdateDelete(CCmdUI * pCmdUI);
	//}}AFX_MSG
	// CSelectionSupport handlers:
	afx_msg void OnSelchangeComboTimeFormat();
	afx_msg void OnKillfocusEditEnd();
	afx_msg void OnKillfocusEditLength();
	afx_msg void OnKillfocusEditStart();
	afx_msg void OnBuddyChangeSpinEnd(NMHDR * pNmHdr, LRESULT * pResult);
	afx_msg void OnBuddyChangeSpinLength(NMHDR * pNmHdr, LRESULT * pResult);
	afx_msg void OnBuddyChangeSpinStart(NMHDR * pNmHdr, LRESULT * pResult);
	afx_msg void OnSelchangeComboSelection();

	afx_msg void OnSelchangeComboStart();
	afx_msg void OnSelchangeComboEnd();
	afx_msg void OnDeferredSelchangeComboStart();
	afx_msg void OnDeferredSelchangeComboEnd();

	DECLARE_MESSAGE_MAP()
public:
	// 0 - marker, 1 - region
	int m_bMarkerOrRegion;
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

