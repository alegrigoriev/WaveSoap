#if !defined(AFX_CUSTOMSAMPLERATEDLG_H__570E57BE_09B1_4DE2_8E1D_2A259DFDE28B__INCLUDED_)
#define AFX_CUSTOMSAMPLERATEDLG_H__570E57BE_09B1_4DE2_8E1D_2A259DFDE28B__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// CustomSampleRateDlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CCustomSampleRateDlg dialog

class CCustomSampleRateDlg : public CDialog
{
// Construction
public:
	CCustomSampleRateDlg(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CCustomSampleRateDlg)
	enum { IDD = IDD_DIALOG_CUSTOM_SAMPLERATE };
	// NOTE: the ClassWizard will add data members here
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

#endif // !defined(AFX_CUSTOMSAMPLERATEDLG_H__570E57BE_09B1_4DE2_8E1D_2A259DFDE28B__INCLUDED_)
