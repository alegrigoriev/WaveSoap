// Copyright Alexander Grigoriev, 1997-2002, All Rights Reserved
#pragma once
// FilterDialog.h : header file
//
#include "ResizableDialog.h"
#include "DialogWithSelection.h"
#include "NumEdit.h"
#include "resource.h"       // main symbols
#include <complex>

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

	~CFilterDialog();
// Dialog Data
	//{{AFX_DATA(CFilterDialog)
	enum { IDD = IDD_DIALOG_FILTER };
	//}}AFX_DATA

	void GetFilterCoefficients(struct FilterCoefficients * coeffs) const;

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

	class CFilterGraphWnd * m_pGraphWnd;

	void OnNotifyGraph(NMHDR * pNotifyStruct, LRESULT * result );
	void UpdateEditBoxes();

	// Generated message map functions
	//{{AFX_MSG(CFilterDialog)
	afx_msg void OnButtonLoad();
	afx_msg void OnButtonResetBands();
	afx_msg void OnButtonSaveAs();
	afx_msg void OnCheckZeroPhase();
	afx_msg void OnCheckLowpass();
	afx_msg void OnCheckHighpass();
	afx_msg void OnCheckHilbertTransform();
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

