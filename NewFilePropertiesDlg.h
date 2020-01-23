// Copyright Alexander Grigoriev, 1997-2002, All Rights Reserved
#pragma once
// NewFilePropertiesDlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CNewFilePropertiesDlg dialog
#include "TimeEdit.h"
#include "resource.h"       // main symbols
#include "WaveSupport.h"

class CNewFilePropertiesDlg : public CDialog
{
	typedef CDialog BaseClass;
// Construction
public:
	CNewFilePropertiesDlg(long SamplingRate,
						NUMBER_OF_CHANNELS nChannels,
						long LengthSeconds,
						WaveSampleType SampleType,
						bool WhenShiftOnly,
						CWnd* pParent = NULL);   // standard constructor

	long GetLengthSeconds() const
	{
		return m_Length;
	}
	bool ShowWhenShiftOnly() const
	{
		return m_bShowOnlyWhenShift != 0;
	}
	long GetSamplingRate() const
	{
		return m_nSamplingRate;
	}
	WORD NumberOfChannels() const
	{
		return m_NumberOfChannels;
	}
	WaveSampleType SampleType() const
	{
		switch (m_SampleType)
		{
		case 0:
		default:
			return SampleType16bit;
		case 1:
			return SampleType32bit;
		case 2:
			return SampleTypeFloat32;
		}
	}
protected:
// Dialog Data
	//{{AFX_DATA(CNewFilePropertiesDlg)
	enum { IDD = IDD_DIALOG_NEW_FILE_PARAMETERS };
	CTimeEdit	m_eLength;
	CTimeSpinCtrl	m_SpinLength;
	BOOL	m_bShowOnlyWhenShift;
	short	m_NumberOfChannels;
	int m_nSamplingRate;
	int m_SampleType;
	//}}AFX_DATA
	NUMBER_OF_SAMPLES m_Length;

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CNewFilePropertiesDlg)
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CNewFilePropertiesDlg)
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

