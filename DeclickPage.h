#if !defined(AFX_DECLICKPAGE_H__3D7ED702_2CF8_11D2_BE02_444553540000__INCLUDED_)
#define AFX_DECLICKPAGE_H__3D7ED702_2CF8_11D2_BE02_444553540000__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// DeclickPage.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CDeclickPage dialog
#include "NumEdit.h"

class CDeclickPage : public CPropertyPage
{
	DECLARE_DYNCREATE(CDeclickPage)

// Construction
public:
	CDeclickPage();
	~CDeclickPage();

// Dialog Data
	//{{AFX_DATA(CDeclickPage)
	enum { IDD = IDD_PROPPAGE_DECLICK };
	CNumEdit	m_EnvelopDecayRate;
	CNumEdit	m_ClickToNoise;
	CNumEdit	m_AttackRate;
	CString	m_ClickLogFilename;
	int		m_MaxClickLength;
	int		m_MinClickAmplitude;
	BOOL	m_bLogClicks;
	BOOL	m_bLogClicksOnly;
	BOOL	m_bImportClicks;
	CString	m_ClickImportFilename;
	//}}AFX_DATA
	double m_dAttackRate;
	double m_dClickToNoise;
	double m_dEnvelopDecayRate;

	void LoadValuesFromRegistry();
	void StoreValuesToRegistry();

	void SetWaveprocData(CClickRemoval * pCr);

	// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CDeclickPage)
public:
	virtual void OnOK();
	virtual BOOL OnSetActive();
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CDeclickPage)
	afx_msg void OnCheckLogClicks();
	afx_msg void OnCheckImportClicks();
	virtual BOOL OnInitDialog();
	afx_msg void OnClickLogBrowseButton();
	afx_msg void OnClickImportBrowseButton();
	afx_msg void OnButtonMoreSettings();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};

/////////////////////////////////////////////////////////////////////////////
// CMoreDeclickDialog dialog

class CMoreDeclickDialog : public CDialog
{
// Construction
public:
	CMoreDeclickDialog(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CMoreDeclickDialog)
	enum { IDD = IDD_DIALOG_MORE_DECLICK };
	// NOTE: the ClassWizard will add data members here
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CMoreDeclickDialog)
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CMoreDeclickDialog)
	// NOTE: the ClassWizard will add member functions here
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};
//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DECLICKPAGE_H__3D7ED702_2CF8_11D2_BE02_444553540000__INCLUDED_)
