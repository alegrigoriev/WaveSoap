// Copyright Alexander Grigoriev, 1997-2002, All Rights Reserved
#pragma once
// CustomSampleRateDlg.h : header file
//
#include "resource.h"       // main symbols

/////////////////////////////////////////////////////////////////////////////
// CCustomSampleRateDlg dialog

class CCustomSampleRateDlg : public CDialog
{
// Construction
public:
	CCustomSampleRateDlg(long SampleRate, CWnd* pParent = NULL);   // standard constructor

	long GetSampleRate() const
	{
		return m_SampleRate;
	}
protected:
// Dialog Data
	//{{AFX_DATA(CCustomSampleRateDlg)
	enum { IDD = IDD_DIALOG_CUSTOM_SAMPLERATE };
	int		m_SampleRate;
	//}}AFX_DATA

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CCustomSampleRateDlg)
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CCustomSampleRateDlg)
	// NOTE: the ClassWizard will add member functions here
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

