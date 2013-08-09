// Copyright Alexander Grigoriev, 1997-2002, All Rights Reserved
#if !defined(AFX_FILTERDIALOG_H__E4CA69D8_E2BD_4C89_AB70_2F3F8C35FF3B__INCLUDED_)
#define AFX_FILTERDIALOG_H__E4CA69D8_E2BD_4C89_AB70_2F3F8C35FF3B__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// FilterDialog.h : header file
//
#include "ResizableDialog.h"
#include "DialogWithSelection.h"
#include "NumEdit.h"
#include "resource.h"       // main symbols
#include <complex>

/////////////////////////////////////////////////////////////////////////////
// CFilterDialog dialog
enum { MaxFilterOrder = 16, };

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
	BOOL IsZeroPhase() const;

	int GetLowpassFilterOrder() const;

	int GetHighpassFilterOrder() const;

	int GetNotchFilterOrder() const;

	void GetLpfCoefficients(double Coeffs[MaxFilterOrder][6]) const;

	void GetHpfCoefficients(double Coeffs[MaxFilterOrder][6]) const;

	void GetNotchCoefficients(double Coeffs[MaxFilterOrder][6]) const;

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

#endif // !defined(AFX_FILTERDIALOG_H__E4CA69D8_E2BD_4C89_AB70_2F3F8C35FF3B__INCLUDED_)
