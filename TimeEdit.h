// Copyright Alexander Grigoriev, 1997-2002, All Rights Reserved
#if !defined(AFX_TIMEEDIT_H__8CFD06E0_6A44_11D4_9ADD_00C0F0583C4B__INCLUDED_)
#define AFX_TIMEEDIT_H__8CFD06E0_6A44_11D4_9ADD_00C0F0583C4B__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// TimeEdit.h : header file
//
#include <vector>
/////////////////////////////////////////////////////////////////////////////
// CTimeEdit window

class CTimeEdit : public CEdit
{
	// Construction
public:
	CTimeEdit(int format = 0);

	// Attributes
public:
	void ExchangeData(CDataExchange* pDX, SAMPLE_INDEX & sample);
	SAMPLE_INDEX GetTimeSample();
	// Operations
public:
	void SetTimeFormat(int format)
	{
		m_TimeFormat = format;
		//UpdateEditControl();
	}
	void SetTimeSample(SAMPLE_INDEX sample);
	void SetSamplingRate(long nSamplesPerSec)
	{
		m_nSamplesPerSec = nSamplesPerSec;
	}
	void UpdateEditControl();

	SAMPLE_INDEX ChangeTimeFormat(int format);
	// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CTimeEdit)
	//}}AFX_VIRTUAL

	// Implementation
public:
	virtual ~CTimeEdit();

	// Generated message map functions
protected:
	long m_nSamplesPerSec;
	int m_TimeFormat;
	SAMPLE_INDEX m_Sample;
	CString m_OriginalString;
	//{{AFX_MSG(CTimeEdit)
	afx_msg void OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
// CTimeSpinCtrl window

class CTimeSpinCtrl : public CSpinButtonCtrl
{
	// Construction
public:
	CTimeSpinCtrl();

	// Attributes
public:

	// Operations
public:

	// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CTimeSpinCtrl)
	//}}AFX_VIRTUAL

	// Implementation
public:
	virtual ~CTimeSpinCtrl();

	// Generated message map functions
protected:
	//{{AFX_MSG(CTimeSpinCtrl)
	afx_msg void OnDeltapos(NMHDR* pNMHDR, LRESULT* pResult);
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
};

// CTimeEditCombo privides combobox with several time choices
class CTimeEditCombo : public CTimeEdit
{
	// Construction
public:
	CTimeEditCombo(int format = 0)
		: CTimeEdit(format)
	{
	}

	// Attributes
public:
	CComboBox & GetComboBox()
	{
		return *static_cast<CComboBox *>(static_cast<CWnd *>(this));
	}

	std::vector<SAMPLE_INDEX> m_Positions;
	// Operations
	void AddPosition(LPCTSTR name, SAMPLE_INDEX Sample);
	void AddPosition(UINT id, SAMPLE_INDEX Sample);
public:
	// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CTimeEditCombo)
	//}}AFX_VIRTUAL

	// Implementation
public:
	virtual ~CTimeEditCombo();

	// Generated message map functions
protected:
	afx_msg void OnReflectComboSelectionChanged();
	afx_msg void OnComboSelectionChanged();
	//{{AFX_MSG(CTimeEditCombo)

	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////
// CFileTimesCombo control

class CFileTimesCombo : public CTimeEditCombo
{
	typedef CTimeEditCombo BaseClass;

public:
	CFileTimesCombo(SAMPLE_INDEX caret,
					CWaveFile & WaveFile, int TimeFormat);

	void FillFileTimes();

protected:
	CWaveFile & m_WaveFile;
	SAMPLE_INDEX const m_CaretPosition;
};

/////////////////////////////////////////////////////////////////////////////
//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_TIMEEDIT_H__8CFD06E0_6A44_11D4_9ADD_00C0F0583C4B__INCLUDED_)
