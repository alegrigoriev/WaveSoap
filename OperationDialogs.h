// Copyright Alexander Grigoriev, 1997-2002, All Rights Reserved
#if !defined(AFX_OPERATIONDIALOGS_H__F0C2FDC2_64C6_11D4_9ADD_00C0F0583C4B__INCLUDED_)
#define AFX_OPERATIONDIALOGS_H__F0C2FDC2_64C6_11D4_9ADD_00C0F0583C4B__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// OperationDialogs.h : header file
//
#include "NumEdit.h"
#include "TimeEdit.h"
#include "ChildDialog.h"
#include <vector>
#include "ApplicationProfile.h"
#include "UiUpdatedDlg.h"
#include "DialogWithSelection.h"

/////////////////////////////////////////////////////////////////////////////
// CCopyChannelsSelectDlg dialog

class CCopyChannelsSelectDlg : public CDialog
{
	typedef CDialog BaseClass;
	// Construction
public:
	CCopyChannelsSelectDlg(CHANNEL_MASK Channels, CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CCopyChannelsSelectDlg)
	enum { IDD = IDD_DIALOG_COPY_CHANNELS_SELECT };
	//}}AFX_DATA
	CHANNEL_MASK GetChannelToCopy() const
	{
		return m_ChannelToCopy - 1;
	}
protected:
	CHANNEL_MASK		m_ChannelToCopy;
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
	typedef CDialog BaseClass;
// Construction
public:
	CPasteModeDialog(int PasteMode, CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CPasteModeDialog)
	enum { IDD = IDD_DIALOG_PASTE_MODE_SELECT };
	//}}AFX_DATA
	int GetPasteMode() const
	{
		return m_PasteMode;
	}

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CPasteModeDialog)
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	int		m_PasteMode;

	// Generated message map functions
	//{{AFX_MSG(CPasteModeDialog)
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};
/////////////////////////////////////////////////////////////////////////////
// CVolumeChangeDialog dialog

class CVolumeChangeDialog : public CDialogWithSelection
{
	typedef CDialogWithSelection BaseClass;
// Construction
public:
	CVolumeChangeDialog(
						SAMPLE_INDEX begin, SAMPLE_INDEX end, SAMPLE_INDEX caret,
						CHANNEL_MASK Channels,
						CWaveFile & File,
						BOOL ChannelsLocked, BOOL UndoEnabled,
						int TimeFormat = SampleToString_HhMmSs | TimeToHhMmSs_NeedsHhMm | TimeToHhMmSs_NeedsMs,
						CWnd* pParent = NULL);   // standard constructor

	double GetLeftVolume();
	double GetRightVolume();

	// Dialog Data
	enum { IDD = IDD_DIALOG_VOLUME_CHANGE,
		IDD_MONO = IDD_DIALOG_VOLUME_CHANGE_MONO };
	//{{AFX_DATA(CVolumeChangeDialog)
	CSliderCtrl	m_SliderVolumeRight;
	CSliderCtrl	m_SliderVolumeLeft;
	CNumEdit	m_eVolumeRight;
	CNumEdit	m_eVolumeLeft;
	int		m_DbPercent;
	//}}AFX_DATA

	CApplicationProfile m_Profile;

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CVolumeChangeDialog)
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	void UpdateVolumeData(CDataExchange* pDX, BOOL InPercents);
	// Generated message map functions
	//{{AFX_MSG(CVolumeChangeDialog)
	afx_msg void OnSelchangeCombodbPercent();
	virtual BOOL OnInitDialog();
	afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	afx_msg void OnKillfocusEditVolumeLeft();
	afx_msg void OnKillfocusEditVolumeRight();
	void OnUpdateVolumeLeft(CCmdUI * pCmdUI);
	void OnUpdateVolumeRight(CCmdUI * pCmdUI);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

	double m_dVolumeLeftDb;
	double m_dVolumeRightDb;
	double m_dVolumeLeftPercent;
	double m_dVolumeRightPercent;
};

/////////////////////////////////////////////////////////////////////////////
// CSelectionDialog dialog

class CSelectionDialog : public CDialog
{
	typedef CDialog BaseClass;
// Construction
public:
	CSelectionDialog(SAMPLE_INDEX Start, SAMPLE_INDEX End, SAMPLE_INDEX CaretPos,
					CHANNEL_MASK Channel,
					CWaveFile & WaveFile, int TimeFormat,
					BOOL bAllowFileExtension = FALSE,
					CWnd* pParent = NULL);   // standard constructor

	SAMPLE_INDEX GetStart() const
	{
		return m_Start;
	}
	SAMPLE_INDEX GetEnd() const
	{
		return m_End;
	}
	CHANNEL_MASK GetChannel() const
	{
		return m_Chan - 1;
	}
// Dialog Data
	//{{AFX_DATA(CSelectionDialog)
	enum { IDD = IDD_SELECTION_DIALOG };
	CComboBox	m_SelectionCombo;
	CTimeSpinCtrl	m_SpinStart;
	CTimeSpinCtrl	m_SpinLength;
	CTimeSpinCtrl	m_SpinEnd;
	CTimeEdit	m_eLength;
	CFileTimesCombo	m_eStart;
	CFileTimesCombo	m_eEnd;
	int		m_TimeFormatIndex;
	int		m_SelectionNumber;
	//}}AFX_DATA
protected:
	CWaveFile & m_WaveFile;
	CHANNEL_MASK m_Chan;
	int		m_TimeFormat;
	SAMPLE_INDEX m_Start;
	SAMPLE_INDEX m_End;
	SAMPLE_INDEX m_CaretPosition;
	NUMBER_OF_SAMPLES m_Length;

	BOOL const m_bAllowFileExtension;
	void AdjustSelection();

	struct Selection
	{
		SAMPLE_INDEX begin;
		SAMPLE_INDEX end;
	};

	std::vector<Selection> m_Selections;

	void AddSelection(LPCTSTR Name, SAMPLE_INDEX begin, SAMPLE_INDEX end);
	void AddSelection(UINT id, SAMPLE_INDEX begin, SAMPLE_INDEX end);
	int FindSelection(SAMPLE_INDEX begin, SAMPLE_INDEX end);
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
	afx_msg void OnSelchangeComboSelection();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};
/////////////////////////////////////////////////////////////////////////////
// CGotoDialog dialog

class CGotoDialog : public CDialog
{
	typedef CDialog BaseClass;
// Construction
public:
	CGotoDialog(SAMPLE_INDEX Position,
				CWaveFile & WaveFile,
				int TimeFormat, CWnd* pParent = NULL);   // standard constructor

	SAMPLE_INDEX GetSelectedPosition() const
	{
		return m_Position;
	}

// Dialog Data
	//{{AFX_DATA(CGotoDialog)
	enum { IDD = IDD_DIALOG_GOTO };
	CTimeSpinCtrl	m_StartSpin;
	CFileTimesCombo	m_eStart;
	int		m_TimeFormatIndex;
	//}}AFX_DATA
	int m_TimeFormat;
// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CGotoDialog)
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	SAMPLE_INDEX m_Position;
	// Generated message map functions
	//{{AFX_MSG(CGotoDialog)
	afx_msg void OnSelchangeComboTimeFormat();
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};
/////////////////////////////////////////////////////////////////////////////
// CDcOffsetDialog dialog

class CDcOffsetDialog : public CDialogWithSelection
{
	typedef CDialogWithSelection BaseClass;
// Construction
public:
	CDcOffsetDialog(SAMPLE_INDEX begin, SAMPLE_INDEX end, SAMPLE_INDEX caret,
					CHANNEL_MASK Channels,
					CWaveFile & File,
					BOOL ChannelsLocked, BOOL UndoEnabled,
					int TimeFormat = SampleToString_HhMmSs | TimeToHhMmSs_NeedsHhMm | TimeToHhMmSs_NeedsMs,
					CWnd* pParent = NULL);   // standard constructor

	bool NeedToCalculateDcOffset() const
	{
		return 0 == m_DcSelectMode;
	}
	bool ScanOnly5Seconds() const
	{
		return 0 != m_b5SecondsDC;
	}
	int GetDcOffset() const
	{
		return m_nDcOffset;
	}
protected:
	CApplicationProfile m_Profile;
// Dialog Data
	//{{AFX_DATA(CDcOffsetDialog)
	enum { IDD = IDD_DIALOG_DC_OFFSET };
	CSpinButtonCtrl	m_OffsetSpin;
	BOOL	m_b5SecondsDC;
	int		m_nDcOffset;
	int		m_DcSelectMode;
	//}}AFX_DATA

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CDcOffsetDialog)
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CDcOffsetDialog)
	afx_msg void OnRadioDcSelect();
	afx_msg void OnRadioAdjustSelectEdit();
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};
/////////////////////////////////////////////////////////////////////////////
// CStatisticsDialog dialog

class CStatisticsDialog : public CDialog
{
	typedef CDialog BaseClass;
// Construction
public:
	CStatisticsDialog(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CStatisticsDialog)
	enum { IDD = IDD_DIALOG_STATISTICS };
	CStatic	m_FileName;
	//}}AFX_DATA

	CStatisticsContext * m_pContext;
	long m_SamplesPerSec;
	long m_CaretPosition;
	int m_ValueAtCursorLeft;
	int m_ValueAtCursorRight;
	CString m_sFilename;
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

class CNormalizeSoundDialog : public CDialogWithSelection
{
	typedef CDialogWithSelection BaseClass;
// Construction
public:
	CNormalizeSoundDialog(SAMPLE_INDEX begin, SAMPLE_INDEX end, SAMPLE_INDEX caret,
						CHANNEL_MASK Channels,
						CWaveFile & File,
						BOOL ChannelsLocked, BOOL UndoEnabled,
						int TimeFormat = SampleToString_HhMmSs | TimeToHhMmSs_NeedsHhMm | TimeToHhMmSs_NeedsMs,
						CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CNormalizeSoundDialog)
	enum { IDD = IDD_DIALOG_NORMALIZE };
	CSliderCtrl	m_SliderLevel;
	CNumEdit	m_eLevel;
	int		m_DbPercent;
	//}}AFX_DATA

	double GetLimitLevel() const;
	CApplicationProfile m_Profile;
// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CNormalizeSoundDialog)
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	double m_dLevelDb;
	double m_dLevelPercent;

	// Generated message map functions
	//{{AFX_MSG(CNormalizeSoundDialog)
	afx_msg void OnKillfocusEditLevel();
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
	typedef CDialog BaseClass;
// Construction
public:
	CResampleDialog(BOOL bUndoEnabled,
					long OldSampleRate,
					bool CanOnlyChangeSampleRate,
					CWnd* pParent = NULL);   // standard constructor

	BOOL ChangeRateOnly() const
	{
		return m_bChangeRateOnly;
	}

	BOOL ChangeSampleRate() const
	{
		return m_bChangeSamplingRate;
	}

	BOOL UndoEnabled() const
	{
		return m_bUndo;
	}
	long NewSampleRate() const
	{
		if (m_bChangeSamplingRate)
		{
			return m_NewSampleRate;
		}
		else
		{
			return long(m_OldSampleRate * m_TempoChange);
		}
	}

	double ResampleRatio() const
	{
		if (m_bChangeSamplingRate)
		{
			return double(m_NewSampleRate) / m_OldSampleRate;
		}
		else
		{
			return m_TempoChange;
		}
	}
// Dialog Data
	CApplicationProfile m_Profile;
	//{{AFX_DATA(CResampleDialog)
	enum { IDD = IDD_DIALOG_RESAMPLE };
	CSliderCtrl	m_SliderTempo;
	CSliderCtrl	m_SliderRate;
	CNumEdit	m_EditTempo;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CResampleDialog)
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	BOOL	m_bChangeRateOnly;    // don't resample, set different rate only
	int		m_bChangeSamplingRate; // set new rate for resampling. Otherwise the sound is just stretched.
	BOOL	m_bUndo;
	double m_TempoChange;
	long m_NewSampleRate;
	long m_OldSampleRate;
	bool m_bCanOnlyChangeSamplerate;    // if the file have zero length
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
/////////////////////////////////////////////////////////////////////////////
// CLowFrequencySuppressDialog dialog

class CLowFrequencySuppressDialog : public CDialogWithSelection
{
	typedef CDialogWithSelection BaseClass;
// Construction
public:
	CLowFrequencySuppressDialog(SAMPLE_INDEX begin, SAMPLE_INDEX end, SAMPLE_INDEX caret,
								CHANNEL_MASK Channels,
								CWaveFile & File,
								BOOL ChannelsLocked, BOOL UndoEnabled,
								int TimeFormat = SampleToString_HhMmSs | TimeToHhMmSs_NeedsHhMm | TimeToHhMmSs_NeedsMs,
								CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CLowFrequencySuppressDialog)
	enum { IDD = IDD_DIALOG_ULF_REDUCTION };
	CNumEdit	m_eLfNoiseRange;
	CNumEdit	m_eDiffNoiseRange;
	BOOL	m_DifferentialModeSuppress;
	BOOL	m_LowFrequencySuppress;
	//}}AFX_DATA

	double m_dLfNoiseRange;
	double m_dDiffNoiseRange;

	CApplicationProfile m_Profile;
// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CLowFrequencySuppressDialog)
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CLowFrequencySuppressDialog)
	afx_msg void OnCheckDifferentialModeSuppress();
	afx_msg void OnCheckLowFrequency();
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};
/////////////////////////////////////////////////////////////////////////////
// CExpressionEvaluationDialog dialog

class CExpressionEvaluationDialog : public CDialogWithSelection
{
	typedef CDialogWithSelection BaseClass;
// Construction
public:
	CExpressionEvaluationDialog(SAMPLE_INDEX begin, SAMPLE_INDEX end, SAMPLE_INDEX caret,
								CHANNEL_MASK Channels,
								CWaveFile & File,
								BOOL ChannelsLocked, BOOL UndoEnabled,
								int TimeFormat,
								class CExpressionEvaluationContext * pContext,
								CWnd* pParent = NULL);   // standard constructor
	~CExpressionEvaluationDialog();
	class CExpressionEvaluationContext * GetExpressionContext();

protected:
// Dialog Data
	//{{AFX_DATA(CExpressionEvaluationDialog)
	enum { IDD = IDD_DIALOG_EXPRESSION_EVALUATION };
	CTabCtrl	m_TabTokens;
	CEdit	m_eExpression;
	CString m_sExpression;
	//}}AFX_DATA

	CApplicationProfile m_Profile;
	CChildDialog m_FunctionsTabDlg;
	COperandsDialog m_OperandsTabDlg;
	CInsertExpressionDialog m_SavedExprTabDlg;

	class CExpressionEvaluationContext * m_pContext;

	int m_ExpressionGroupSelected;
	int m_ExpressionSelected;
	int m_ExpressionTabSelected;

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CExpressionEvaluationDialog)
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL
// Implementation
protected:
	void ShowHideTabDialogs();
	afx_msg BOOL OnButtonText(UINT id);
	afx_msg void OnUpdateOk(CCmdUI* pCmdUI);
	afx_msg void OnUpdateSaveAs(CCmdUI* pCmdUI);

	// Generated message map functions
	//{{AFX_MSG(CExpressionEvaluationDialog)
	virtual void OnOK();
	afx_msg void OnSelchangeTabTokens(NMHDR* pNMHDR, LRESULT* pResult);
	virtual BOOL OnInitDialog();
	afx_msg void OnButtonSaveExpressionAs();
	afx_msg void OnChangeEditExpression();
	virtual LRESULT OnKickIdle(WPARAM, LPARAM);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
	// m_Profile will be conrtructed last and destructed first
};
/////////////////////////////////////////////////////////////////////////////
// CDeclickDialog dialog

class CDeclickDialog : public CDialogWithSelection
{
	typedef CDialogWithSelection BaseClass;
// Construction
public:
	CDeclickDialog(SAMPLE_INDEX begin, SAMPLE_INDEX end, SAMPLE_INDEX caret,
					CHANNEL_MASK Channels,
					CWaveFile & File,
					BOOL ChannelsLocked, BOOL UndoEnabled,
					int TimeFormat,
					CWnd* pParent = NULL);   // standard constructor

	// Dialog Data
	CApplicationProfile Profile;
	void SetDeclickData(CClickRemoval * pCr);
protected:
	//{{AFX_DATA(CDeclickDialog)
	enum { IDD = IDD_DIALOG_DECLICKING };
	CNumEdit	m_EnvelopDecayRate;
	CNumEdit	m_ClickToNoise;
	CNumEdit	m_AttackRate;
	CString	m_ClickLogFilename;
	int		m_MaxClickLength;
	int		m_MinClickAmplitude;
	BOOL	m_bLogClicks;
	BOOL	m_bLogClicksOnly;
	BOOL	m_bImportClicks;
	CString	m_ClickImportFilename;
	//}}AFX_DATA
	double m_dAttackRate;
	double m_dClickToNoise;
	double m_dEnvelopDecayRate;

	CButton m_LogClicksCheck;
	CButton m_ImportClicksCheck;
	CEdit m_eLogFilename;
	CEdit m_eImportFilename;

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CDeclickDialog)
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CDeclickDialog)
	afx_msg void OnCheckLogClicks();
	afx_msg void OnCheckImportClicks();
	afx_msg void OnClickLogBrowseButton();
	afx_msg void OnClickImportBrowseButton();
	afx_msg void OnButtonMoreSettings();

	afx_msg void OnUpdateLogClicks(CCmdUI * pCmdUI);
	afx_msg void OnUpdateImportClicks(CCmdUI * pCmdUI);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
	void LoadValuesFromRegistry();
};
/////////////////////////////////////////////////////////////////////////////
// CNoiseReductionDialog dialog

class CNoiseReductionDialog : public CDialogWithSelection
{
	typedef CDialogWithSelection BaseClass;
// Construction
public:
	CNoiseReductionDialog(SAMPLE_INDEX begin, SAMPLE_INDEX end, SAMPLE_INDEX caret,
						CHANNEL_MASK Channels,
						CWaveFile & File,
						BOOL ChannelsLocked, BOOL UndoEnabled,
						int TimeFormat,
						CWnd* pParent = NULL);   // standard constructor

	double GetNoiseThresholdLow() const
	{
		return m_dNoiseThresholdLow;
	}

	double GetNoiseThresholdHigh() const
	{
		return m_dNoiseThresholdHigh;
	}

	double GetLowerFrequency() const
	{
		return m_dLowerFrequency;
	}
	void SetNoiseReductionData(CNoiseReduction * pNr);

protected:
// Dialog Data
	CApplicationProfile Profile;
	//{{AFX_DATA(CNoiseReductionDialog)
	enum { IDD = IDD_DIALOG_NOISE_REDUCTION };
	CNumEdit	m_eToneOverNoisePreference;
	CNumEdit	m_EditAggressivness;
	CNumEdit	m_eNoiseReduction;
	CNumEdit	m_eNoiseCriterion;
	CNumEdit	m_eNoiseThresholdHigh;
	CNumEdit	m_eNoiseThresholdLow;
	CNumEdit	m_eLowerFrequency;
	int		m_nFftOrderExp;
	//}}AFX_DATA

	double	m_dTransientThreshold;
	double	m_dNoiseReduction;
	double	m_dNoiseCriterion;
	double	m_dNoiseThresholdLow;
	double	m_dNoiseThresholdHigh;
	double	m_dLowerFrequency;
	double  m_dNoiseReductionAggressivness;
	double  m_dToneOverNoisePreference;
	double  m_NearMaskingDecayDistanceHigh;
	double  m_NearMaskingDecayDistanceLow;
	double m_NearMaskingDecayTimeLow;   // for low frequencies
	double m_NearMaskingDecayTimeHigh;   // for high frequencies
	double m_NearMaskingCoeff;
	int m_FftOrder;

	void LoadValuesFromRegistry();

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CNoiseReductionDialog)
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CNoiseReductionDialog)
	afx_msg void OnButtonMore();
	afx_msg void OnButtonSetThreshold();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};
/////////////////////////////////////////////////////////////////////////////
// CMoreNoiseDialog dialog

class CMoreNoiseDialog : public CDialog
{
	typedef CDialog BaseClass;
// Construction
public:
	CMoreNoiseDialog(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CMoreNoiseDialog)
	enum { IDD = IDD_DIALOG_MORE_NOISEREDUCTION };
	CNumEdit	m_eNearMaskingCoeff;
	CNumEdit	m_eFarMaskingCoeff;
	CNumEdit	m_eNearMaskingTimeLow;
	CNumEdit	m_eNearMaskingTimeHigh;
	CNumEdit	m_eNearMaskingDistanceLow;
	CNumEdit	m_eNearMaskingDistanceHigh;
	//}}AFX_DATA

	double  m_NearMaskingDecayDistanceHigh;
	double  m_NearMaskingDecayDistanceLow;
	double m_NearMaskingDecayTimeLow;   // for low frequencies
	double m_NearMaskingDecayTimeHigh;   // for high frequencies
	double m_NearMaskingCoeff;
// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CMoreNoiseDialog)
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CMoreNoiseDialog)
	// NOTE: the ClassWizard will add member functions here
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};
//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_OPERATIONDIALOGS_H__F0C2FDC2_64C6_11D4_9ADD_00C0F0583C4B__INCLUDED_)
