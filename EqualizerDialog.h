#if !defined(AFX_EQUALIZERDIALOG_H__D9EA5D94_8BAC_4E78_A268_6D4AD2625D48__INCLUDED_)
#define AFX_EQUALIZERDIALOG_H__D9EA5D94_8BAC_4E78_A268_6D4AD2625D48__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// EqualizerDialog.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CEqualizerDialog dialog

class CEqualizerDialog : public CDialog
{
// Construction
public:
	CEqualizerDialog(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CEqualizerDialog)
	enum { IDD = IDD_DIALOG_SIMPLE_EQUALIZER };
	// NOTE: the ClassWizard will add data members here
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CEqualizerDialog)
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CEqualizerDialog)
	// NOTE: the ClassWizard will add member functions here
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_EQUALIZERDIALOG_H__D9EA5D94_8BAC_4E78_A268_6D4AD2625D48__INCLUDED_)
