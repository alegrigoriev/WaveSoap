// Copyright Alexander Grigoriev, 1997-2002, All Rights Reserved
// NewFilePropertiesDlg.cpp : implementation file
//

#include "stdafx.h"
#include "WaveSoapFront.h"
#include "NewFilePropertiesDlg.h"
#include "TimeToStr.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CNewFilePropertiesDlg dialog


CNewFilePropertiesDlg::CNewFilePropertiesDlg(long SamplingRate,
											NUMBER_OF_CHANNELS nChannels,
											NUMBER_OF_SAMPLES Length,
											WaveSampleType SampleType,
											bool WhenShiftOnly,
											CWnd* pParent /*=NULL*/)
	: BaseClass(IDD, pParent)
	, m_Length(Length)
	, m_nSamplingRate(SamplingRate)
	, m_bShowOnlyWhenShift(WhenShiftOnly)
	, m_NumberOfChannels(nChannels)
	, m_eLength(SampleToString_HhMmSs | TimeToHhMmSs_NeedsHhMm)
{
	//{{AFX_DATA_INIT(CNewFilePropertiesDlg)
	//}}AFX_DATA_INIT
	switch (SampleType)
	{
	case SampleType16bit:
	default:
		m_SampleType = 0;
		break;
	case SampleType32bit:
		m_SampleType = 1;
		break;
	case SampleTypeFloat32:
		m_SampleType = 2;
		break;
	}
	if (m_NumberOfChannels > 32)
	{
		m_NumberOfChannels = 32;
	}
	if (m_NumberOfChannels < 1)
	{
		m_NumberOfChannels = 1;
	}
}


void CNewFilePropertiesDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CNewFilePropertiesDlg)
	DDX_Control(pDX, IDC_SPIN_LENGTH, m_SpinLength);
	DDX_Control(pDX, IDC_EDIT_LENGTH, m_eLength);
	DDX_Text(pDX, IDC_COMBO_SAMPLING_RATE, m_nSamplingRate);
	DDV_MinMaxInt(pDX, m_nSamplingRate, 1, 1000000);
	DDX_Text(pDX, IDC_COMBO_NUMBER_OF_CHANNELS, m_NumberOfChannels);
	DDV_MinMaxInt(pDX, m_NumberOfChannels, 1, 32);
	DDX_CBIndex(pDX, IDC_COMBO_SAMPLE_TYPE, m_SampleType);
	DDX_Check(pDX, IDC_CHECK_SHOW_ONLY_WHEN_SHIFT, m_bShowOnlyWhenShift);
	//}}AFX_DATA_MAP

	m_eLength.ExchangeData(pDX, m_Length);
	if (pDX->m_bSaveAndValidate)
	{
		int SampleSize = 2;
		if (m_SampleType != 0)
		{
			SampleSize = 4;
		}
		WAV_FILE_SIZE MaxFileLength = 0x7FFFFFFFu - 0x100000u;
		if (GetApp()->m_bAllow4GbWavFile)
		{
			MaxFileLength = 0xFFFFFFFFu - 0x100000u;
		}

		WAV_FILE_SIZE MaxLength = MaxFileLength / (m_NumberOfChannels * m_nSamplingRate * SampleSize);

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

