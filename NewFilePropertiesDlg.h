// Copyright Alexander Grigoriev, 1997-2002, All Rights Reserved
#if !defined(AFX_NEWFILEPROPERTIESDLG_H__306F663B_DA8B_4A53_9573_334F54B5D3CF__INCLUDED_)
#define AFX_NEWFILEPROPERTIESDLG_H__306F663B_DA8B_4A53_9573_334F54B5D3CF__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// NewFilePropertiesDlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CNewFilePropertiesDlg dialog
#include "TimeEdit.h"

class CNewFilePropertiesDlg : public CDialog
{
// Construction
public:
	CNewFilePropertiesDlg(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CNewFilePropertiesDlg)
	enum { IDD = IDD_DIALOG_NEW_FILE_PARAMETERS };
	CTimeEdit	m_eLength;
	CTimeSpinCtrl	m_SpinLength;
	BOOL	m_bShowOnlyWhenShift;
	int		m_MonoStereo;
	int m_nSamplingRate;
	//}}AFX_DATA
	NUMBER_OF_SAMPLES m_Length;

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CNewFilePropertiesDlg)
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CNewFilePropertiesDlg)
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_NEWFILEPROPERTIESDLG_H__306F663B_DA8B_4A53_9573_334F54B5D3CF__INCLUDED_)
