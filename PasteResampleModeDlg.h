// Copyright Alexander Grigoriev, 1997-2002, All Rights Reserved
#if !defined(AFX_PASTERESAMPLEMODEDLG_H__0C1F094D_7C33_4483_9E8D_2A95CC109AD2__INCLUDED_)
#define AFX_PASTERESAMPLEMODEDLG_H__0C1F094D_7C33_4483_9E8D_2A95CC109AD2__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// PasteResampleModeDlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CPasteResampleModeDlg dialog

class CPasteResampleModeDlg : public CDialog
{
// Construction
public:
	CPasteResampleModeDlg(long SourceSampleRate,
						long TargetSampleRate,
						int LastResampleMode,
						CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CPasteResampleModeDlg)
	enum { IDD = IDD_DIALOG_PASTE_RESAMPLE_MODE };
	CStatic	m_Static;
	//}}AFX_DATA
	int GetSelectedResampleMode() const
	{
		return m_ModeSelect;
	}
// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CPasteResampleModeDlg)
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	int		m_ModeSelect;
	int m_SrcSampleRate;
	int m_TargetSampleRate;
	// Generated message map functions
	//{{AFX_MSG(CPasteResampleModeDlg)
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_PASTERESAMPLEMODEDLG_H__0C1F094D_7C33_4483_9E8D_2A95CC109AD2__INCLUDED_)
