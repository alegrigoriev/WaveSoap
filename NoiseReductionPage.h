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
	CNumEdit	m_eToneOverNoisePreference;
	CNumEdit	m_EditAggressivness;
	CNumEdit	m_eNoiseReduction;
	CNumEdit	m_eNoiseCriterion;
	CNumEdit	m_eNoiseThresholdHigh;
	CNumEdit	m_eNoiseThresholdLow;
	CNumEdit	m_eLowerFrequency;
	int		m_nFftOrderExp;
	//}}AFX_DATA

	double	m_dTransientThreshold;
	double	m_dNoiseReduction;
	double	m_dNoiseCriterion;
	double	m_dNoiseThresholdLow;
	double	m_dNoiseThresholdHigh;
	double	m_dLowerFrequency;
	double  m_dNoiseReductionAggressivness;
	double  m_dToneOverNoisePreference;
	double  m_NearMaskingDecayDistanceHigh;
	double  m_NearMaskingDecayDistanceLow;
	double m_NearMaskingDecayTimeLow;   // for low frequencies
	double m_NearMaskingDecayTimeHigh;   // for high frequencies
	double m_NearMaskingCoeff;
	int m_FftOrder;

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
	afx_msg void OnButtonMore();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};

/////////////////////////////////////////////////////////////////////////////
// CMoreNoiseDialog dialog

class CMoreNoiseDialog : public CDialog
{
// Construction
public:
	CMoreNoiseDialog(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CMoreNoiseDialog)
	enum { IDD = IDD_DIALOG_MORE_NOISEREDUCTION };
	CNumEdit	m_eNearMaskingCoeff;
	CNumEdit	m_eFarMaskingCoeff;
	CNumEdit	m_eNearMaskingTimeLow;
	CNumEdit	m_eNearMaskingTimeHigh;
	CNumEdit	m_eNearMaskingDistanceLow;
	CNumEdit	m_eNearMaskingDistanceHigh;
	//}}AFX_DATA

	double  m_NearMaskingDecayDistanceHigh;
	double  m_NearMaskingDecayDistanceLow;
	double m_NearMaskingDecayTimeLow;   // for low frequencies
	double m_NearMaskingDecayTimeHigh;   // for high frequencies
	double m_NearMaskingCoeff;
// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CMoreNoiseDialog)
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CMoreNoiseDialog)
	// NOTE: the ClassWizard will add member functions here
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};
//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_NOISEREDUCTIONPAGE_H__3BBD6E62_2F5D_11D2_BE02_444553540000__INCLUDED_)
