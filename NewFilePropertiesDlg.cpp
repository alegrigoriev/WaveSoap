// Copyright Alexander Grigoriev, 1997-2002, All Rights Reserved
// NewFilePropertiesDlg.cpp : implementation file
//

#include "stdafx.h"
#include "WaveSoapFront.h"
#include "NewFilePropertiesDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CNewFilePropertiesDlg dialog


CNewFilePropertiesDlg::CNewFilePropertiesDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CNewFilePropertiesDlg::IDD, pParent),
	m_eLength(SampleToString_HhMmSs | TimeToHhMmSs_NeedsHhMm)
{
	//{{AFX_DATA_INIT(CNewFilePropertiesDlg)
	m_bShowOnlyWhenShift = FALSE;
	m_MonoStereo = -1;
	m_nSamplingRate = -1;
	//}}AFX_DATA_INIT
}


void CNewFilePropertiesDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CNewFilePropertiesDlg)
	DDX_Control(pDX, IDC_SPIN_LENGTH, m_SpinLength);
	DDX_Text(pDX, IDC_COMBO_SAMPLING_RATE, m_nSamplingRate);
	DDV_MinMaxInt(pDX, m_nSamplingRate, 1, 1000000);
	DDX_Check(pDX, IDC_CHECK_SHOW_ONLY_WHEN_SHIFT, m_bShowOnlyWhenShift);
	DDX_Radio(pDX, IDC_RADIO_MONO, m_MonoStereo);
	DDX_Control(pDX, IDC_EDIT_LENGTH, m_eLength);
	//}}AFX_DATA_MAP
	m_eLength.ExchangeData(pDX, m_Length);
	if (pDX->m_bSaveAndValidate)
	{
		int channels = 1;
		if (m_MonoStereo)
		{
			channels = 2;
		}
		WAV_FILE_SIZE MaxFileLength = 0x7FFFFFFFu - 0x100000u;
		if (GetApp()->m_bAllow4GbWavFile)
		{
			MaxFileLength = 0xFFFFFFFFu - 0x100000u;
		}

		NUMBER_OF_SAMPLES MaxLength = MaxFileLength / (channels * m_nSamplingRate * 2);

		if (m_Length < 0
			|| m_Length > MaxLength)
		{
			CString s;
			s.Format(IDS_WRONG_NEW_FILE_LENGTH, MaxLength);
			AfxMessageBox(s);
			pDX->Fail();
		}
	}
}


BEGIN_MESSAGE_MAP(CNewFilePropertiesDlg, CDialog)
	//{{AFX_MSG_MAP(CNewFilePropertiesDlg)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CNewFilePropertiesDlg message handlers

