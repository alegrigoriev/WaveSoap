// Copyright Alexander Grigoriev, 1997-2002, All Rights Reserved
#if !defined(AFX_RAWFILEPARAMETERSDLG_H__B0281B13_1E04_4C49_B08D_20F9BFDCFDFF__INCLUDED_)
#define AFX_RAWFILEPARAMETERSDLG_H__B0281B13_1E04_4C49_B08D_20F9BFDCFDFF__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// RawFileParametersDlg.h : header file
//
#include "ApplicationProfile.h"
#include "UiUpdatedDlg.h"

/////////////////////////////////////////////////////////////////////////////
// CRawFileParametersDlg dialog
struct RawFileParams
{
	union
	{
		DWORD m_dwParams;
		struct
		{
			DWORD		m_bStereo :1;
			DWORD		m_bBits16 :1;
			DWORD		m_Compression :2;
			DWORD		m_bMsbFirst :1;

		} m_Params;
	};
};

class CRawFileParametersDlg : public CUiUpdatedDlg, protected RawFileParams
{
	typedef CUiUpdatedDlg BaseClass;
// Construction
public:
	CRawFileParametersDlg(LONGLONG Length, CWnd* pParent = NULL);   // standard constructor

	DWORD HeaderLength() const
	{
		return m_HeaderLength;
	}

	DWORD TrailerLength() const
	{
		return m_TrailerLength;
	}

	BOOL MsbFirst() const
	{
		return m_bMsbFirst;
	}

	NUMBER_OF_CHANNELS NumberOfChannels() const
	{
		if (m_bStereo)
		{
			return 2;
		}
		else
		{
			return 1;
		}
	}

	WORD NumberOfBits() const
	{
		return WORD(8 + 8 * (0 != m_bBits16));
	}

	unsigned long SamplingRate() const
	{
		return m_SamplingRate;
	}

	WORD GetFormatTag() const
	{
		if (m_bBits16)
		{
			return WAVE_FORMAT_PCM;
		}
		switch(m_Compression)
		{
		case 0:
		default:
			return WAVE_FORMAT_PCM;
		case 1:
			return WAVE_FORMAT_ALAW;
		case 2:
			return WAVE_FORMAT_MULAW;
		}
	}

// Dialog Data
	//{{AFX_DATA(CRawFileParametersDlg)
	enum { IDD = IDD_DIALOG_RAW_FILE_PARAMETERS };
protected:
	CComboBox	m_cbSamplingRate;
	DWORD	m_HeaderLength;
	DWORD	m_TrailerLength;
	int		m_bStereo;
	int		m_bBits16;
	int		m_Compression;
	int		m_bMsbFirst;
	//}}AFX_DATA
	unsigned long    m_SamplingRate;
	LONGLONG  m_SourceFileSize;

	CApplicationProfile m_Profile;
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
	afx_msg void OnClicked8bits();
	afx_msg void OnClicked16bits();
	//}}AFX_MSG
	afx_msg void OnUpdate16bitsOnly(CCmdUI * pCmdUI);
	afx_msg void OnUpdate8bitsOnly(CCmdUI * pCmdUI);
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////
// CSaveRawFileDlg dialog

class CSaveRawFileDlg : public CDialog, RawFileParams
{
// Construction
public:
	CSaveRawFileDlg(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CSaveRawFileDlg)
	enum { IDD = IDD_DIALOG_SAVE_RAW_FILE };
	int		m_b16Bits;
	int		m_Compression;
	int		m_bMsbFirst;
	//}}AFX_DATA

	CApplicationProfile m_Profile;

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CSaveRawFileDlg)
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CSaveRawFileDlg)
	// NOTE: the ClassWizard will add member functions here
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_RAWFILEPARAMETERSDLG_H__B0281B13_1E04_4C49_B08D_20F9BFDCFDFF__INCLUDED_)
