// Copyright Alexander Grigoriev, 1997-2002, All Rights Reserved
#pragma once
// EqualizerDialog.h : header file
//
#include "NumEdit.h"
#include "ResizableDialog.h"
#include "UiUpdatedDlg.h"
#include "DialogWithSelection.h"
#include "resource.h"       // main symbols

/////////////////////////////////////////////////////////////////////////////
// CEqualizerGraphWnd window
enum { MaxNumberOfEqualizerBands = 20, };

/////////////////////////////////////////////////////////////////////////////
// CEqualizerDialog dialog

class CEqualizerDialog : public CDialogWithSelectionT<CResizableDialog>
{
	typedef CDialogWithSelectionT<CResizableDialog> BaseClass;
// Construction
public:
	CEqualizerDialog(SAMPLE_INDEX Start,
					SAMPLE_INDEX End,
					SAMPLE_INDEX CaretPosition,
					CHANNEL_MASK Channels,
					CWaveFile & WaveFile,
					int m_TimeFormat,
					BOOL bLockChannels,
					BOOL	bUndoEnabled,
					CWnd* pParent = NULL);   // standard constructor

	~CEqualizerDialog();

	void GetBandCoefficients(double BandCoefficients[MaxNumberOfEqualizerBands][6]) const;

	int GetNumberOfBands() const
	{
		return m_nBands;
	}
	BOOL IsZeroPhase() const;

protected:
	CApplicationProfile m_Profile;
// Dialog Data
	//{{AFX_DATA(CEqualizerDialog)
	enum { IDD = IDD_DIALOG_SIMPLE_EQUALIZER };
	CEdit	m_eEditBands;
	CNumEdit	m_eBandTransfer;
	CSpinButtonCtrl	m_SpinBands;
	int		m_bMultiBandEqualizer;
	int 	m_nBands;
	//}}AFX_DATA
	class CEqualizerGraphWnd * m_pEqualizerGraph;

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CEqualizerDialog)
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual void OnOK();
	//}}AFX_VIRTUAL

// Implementation
protected:
	afx_msg void OnNotifyGraph( NMHDR * pNotifyStruct, LRESULT * result );

	// Generated message map functions
	//{{AFX_MSG(CEqualizerDialog)
	virtual BOOL OnInitDialog();
	afx_msg void OnChangeEditBands();
	afx_msg void OnButtonLoad();
	afx_msg void OnButtonResetBands();
	afx_msg void OnButtonSaveAs();
	afx_msg void OnKillfocusEditBandGain();
	afx_msg void OnRadioEqualizerType();
	afx_msg void OnRadio2();
	afx_msg void OnCheckZeroPhase();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////
//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

