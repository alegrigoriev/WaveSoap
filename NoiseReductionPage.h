#if !defined(AFX_NOISEREDUCTIONPAGE_H__3BBD6E62_2F5D_11D2_BE02_444553540000__INCLUDED_)
#define AFX_NOISEREDUCTIONPAGE_H__3BBD6E62_2F5D_11D2_BE02_444553540000__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// NoiseReductionPage.h : header file
//
#include <NumEdit.h>
/////////////////////////////////////////////////////////////////////////////
// CNoiseReductionPage dialog

class CNoiseReductionPage : public CPropertyPage
{
	DECLARE_DYNCREATE(CNoiseReductionPage)

// Construction
public:
	CNoiseReductionPage();
	~CNoiseReductionPage();

// Dialog Data
	//{{AFX_DATA(CNoiseReductionPage)
	enum { IDD = IDD_PROPPAGE_NOISEREDUCT };
	CNumEdit	m_eTransientThreshold;
	CNumEdit	m_eNoiseReduction;
	CNumEdit	m_eNoiseCriterion;
	CNumEdit	m_eNoiseThreshold;
	CNumEdit	m_eLowerFrequency;
	CNumEdit	m_eContinuousThreshold;
	BOOL	m_bPhaseFilter;
	int		m_nFftOrderExp;
	//}}AFX_DATA

	double	m_dTransientThreshold;
	double	m_dNoiseReduction;
	double	m_dNoiseCriterion;
	double	m_dNoiseThreshold;
	double	m_dLowerFrequency;
	double	m_dContinuousThreshold;

	void LoadValuesFromRegistry();
	void StoreValuesToRegistry();
	void SetWaveprocData(CNoiseReduction * pNr);

// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CNoiseReductionPage)
public:
	virtual void OnOK();
	virtual BOOL OnSetActive();
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CNoiseReductionPage)
	// NOTE: the ClassWizard will add member functions here
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_NOISEREDUCTIONPAGE_H__3BBD6E62_2F5D_11D2_BE02_444553540000__INCLUDED_)
