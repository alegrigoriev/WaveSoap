#if !defined(AFX_RAWFILEPARAMETERSDLG_H__B0281B13_1E04_4C49_B08D_20F9BFDCFDFF__INCLUDED_)
#define AFX_RAWFILEPARAMETERSDLG_H__B0281B13_1E04_4C49_B08D_20F9BFDCFDFF__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// RawFileParametersDlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CRawFileParametersDlg dialog

class CRawFileParametersDlg : public CDialog
{
// Construction
public:
	CRawFileParametersDlg(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CRawFileParametersDlg)
	enum { IDD = IDD_DIALOG_RAW_FILE_PARAMETERS };
	// NOTE: the ClassWizard will add data members here
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CRawFileParametersDlg)
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CRawFileParametersDlg)
	// NOTE: the ClassWizard will add member functions here
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_RAWFILEPARAMETERSDLG_H__B0281B13_1E04_4C49_B08D_20F9BFDCFDFF__INCLUDED_)
