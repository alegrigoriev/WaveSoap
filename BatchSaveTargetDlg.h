#if !defined(AFX_BATCHSAVETARGETDLG_H__CFACB615_6CD0_466E_BF66_D0C867C94CCE__INCLUDED_)
#define AFX_BATCHSAVETARGETDLG_H__CFACB615_6CD0_466E_BF66_D0C867C94CCE__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// BatchSaveTargetDlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CBatchSaveTargetDlg dialog

class CBatchSaveTargetDlg : public CDialog
{
// Construction
public:
	CBatchSaveTargetDlg(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CBatchSaveTargetDlg)
	enum { IDD = IDD_DIALOG_BATCH_SAVE_TARGET };
	// NOTE: the ClassWizard will add data members here
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CBatchSaveTargetDlg)
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CBatchSaveTargetDlg)
	// NOTE: the ClassWizard will add member functions here
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_BATCHSAVETARGETDLG_H__CFACB615_6CD0_466E_BF66_D0C867C94CCE__INCLUDED_)
