#if !defined(AFX_WAVESOAPSHEET_H__5799DAAE_2CB1_11D2_BE02_444553540000__INCLUDED_)
#define AFX_WAVESOAPSHEET_H__5799DAAE_2CB1_11D2_BE02_444553540000__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// WaveSoapSheet.h : header file
//
#include "MainPage.h"
#include "DeclickPage.h"
#include "NoiseReductionPage.h"

/////////////////////////////////////////////////////////////////////////////
// CWaveSoapSheet

class CWaveSoapSheet : public CPropertySheet
{
	DECLARE_DYNAMIC(CWaveSoapSheet)

// Construction
public:
	CWaveSoapSheet(UINT nIDCaption, CWnd* pParentWnd = NULL, UINT iSelectPage = 0);
	CWaveSoapSheet(LPCTSTR pszCaption, CWnd* pParentWnd = NULL, UINT iSelectPage = 0);

// Attributes
public:

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CWaveSoapSheet)
public:
	virtual BOOL OnInitDialog();
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CWaveSoapSheet();
	CMainPage m_MainPage;
	CDeclickPage m_DeclickPage;
	CNoiseReductionPage m_NoisePage;

	void LoadValuesFromRegistry();
	void StoreValuesToRegistry();
	// Generated message map functions
protected:
	//{{AFX_MSG(CWaveSoapSheet)
	// NOTE - the ClassWizard will add and remove member functions here.
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_WAVESOAPSHEET_H__5799DAAE_2CB1_11D2_BE02_444553540000__INCLUDED_)
