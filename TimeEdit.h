#if !defined(AFX_TIMEEDIT_H__8CFD06E0_6A44_11D4_9ADD_00C0F0583C4B__INCLUDED_)
#define AFX_TIMEEDIT_H__8CFD06E0_6A44_11D4_9ADD_00C0F0583C4B__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// TimeEdit.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CTimeEdit window

class CTimeEdit : public CEdit
{
// Construction
public:
	CTimeEdit();

// Attributes
public:
	void ExchangeData(CDataExchange* pDX, long & sample);
	long GetTimeSample(LPCTSTR str = NULL);
// Operations
public:
	void SetTimeFormat(int format);
	void SetTimeSample(long sample);
	void SetSamplingRate(long nSamplesPerSec)
	{
		m_nSamplesPerSec = nSamplesPerSec;
	}
	void UpdateEditControl();
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
	long m_Sample;
	CString m_OriginalString;
	//{{AFX_MSG(CTimeEdit)
	// NOTE - the ClassWizard will add and remove member functions here.
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_TIMEEDIT_H__8CFD06E0_6A44_11D4_9ADD_00C0F0583C4B__INCLUDED_)
