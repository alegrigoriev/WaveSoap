// Copyright Alexander Grigoriev, 1997-2002, All Rights Reserved
// RawFileParametersDlg.cpp : implementation file
//

#include "stdafx.h"
#include "WaveSoapFront.h"
#include "RawFileParametersDlg.h"
#include "ApplicationParameters.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CRawFileParametersDlg dialog

CRawFileParametersDlg::CRawFileParametersDlg(ULONGLONG Length,
											CWnd* pParent /*=NULL*/)
	: BaseClass(IDD, pParent)
	, m_SourceFileSize(Length)
{
	FileParameters * pParams = PersistentFileParameters::GetData();
	m_WaveFormat = pParams->RawFileFormat;

	m_HeaderLength = pParams->RawFileHeaderLength;
	if (Length < 4)
	{
		m_HeaderLength = 0;
	}
	else if (m_HeaderLength > Length - 4)
	{
		m_HeaderLength = Length - 4;
	}

	m_TrailerLength = DWORD(std::min<ULONGLONG>(pParams->RawFileTrailerLength, Length - 4 - m_HeaderLength));

	m_bStereo = (2 == m_WaveFormat.nChannels);
	m_bBits16 = (16 == m_WaveFormat.wBitsPerSample);

	m_bMsbFirst = pParams->RawFileBigEnded;
	m_SamplingRate = m_WaveFormat.nSamplesPerSec;

	if (WAVE_FORMAT_PCM == m_WaveFormat.wFormatTag)
	{
		m_Compression = 0;
	}
	else if (WAVE_FORMAT_ALAW == m_WaveFormat.wFormatTag)
	{
		m_Compression = 1;
	}
	else if (WAVE_FORMAT_MULAW == m_WaveFormat.wFormatTag)
	{
		m_Compression = 2;
	}
}

static void AFXAPI FailMinMaxWithFormat(CDataExchange* pDX,
										long minVal, long maxVal, LPCTSTR lpszFormat, UINT nIDPrompt)
	// error string must have '%1' and '%2' strings for min and max values
	// wsprintf formatting uses long values (format should be '%ld' or '%lu')
{
	ASSERT(lpszFormat != NULL);

	if (!pDX->m_bSaveAndValidate)
	{
		TRACE0("Warning: initial dialog data is out of range.\n");
		return;     // don't stop now
	}
	TCHAR szMin[32];
	TCHAR szMax[32];
	wsprintf(szMin, lpszFormat, minVal, minVal);
	wsprintf(szMax, lpszFormat, maxVal, maxVal);
	CString prompt;
	AfxFormatString2(prompt, nIDPrompt, szMin, szMax);
	AfxMessageBox(prompt, MB_ICONEXCLAMATION, nIDPrompt);
	prompt.Empty(); // exception prep
	pDX->Fail();
}

void AFXAPI DDV_MinMaxDWordHex(CDataExchange* pDX, DWORD value, DWORD minVal, DWORD maxVal)
{
	ASSERT(minVal <= maxVal);
	if (value < minVal || value > maxVal)
		FailMinMaxWithFormat(pDX, (long)minVal, (long)maxVal, _T("%lu (0x%08X)"),
							AFX_IDP_PARSE_INT_RANGE);
}

void AFXAPI DDX_TextHex(CDataExchange* pDX, int nIDC, DWORD& value)
{
	CWnd * pWnd = CWnd::FromHandle(pDX->PrepareEditCtrl(nIDC));
	TCHAR szT[32] = {0};
	if (pDX->m_bSaveAndValidate)
	{
		// the following works for %d, %u, %ld, %lu
		pWnd->GetWindowText(szT, 31);
		if (!_stscanf_s(szT, _T("%li"), &value))
		{
			AfxMessageBox(AFX_IDP_PARSE_UINT);
			pDX->Fail();        // throws exception
		}
	}
	else
	{
		wsprintf(szT, _T("%li"), value);
		// does not support floating point numbers - see dlgfloat.cpp
		pWnd->SetWindowText(szT);
	}
}

void CRawFileParametersDlg::DoDataExchange(CDataExchange* pDX)
{
	BaseClass::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CRawFileParametersDlg)
	DDX_Control(pDX, IDC_COMBO_SAMPLING_RATE, m_cbSamplingRate);
	DDX_TextHex(pDX, IDC_EDIT_HEADER_LENGTH, m_HeaderLength);
	DDX_TextHex(pDX, IDC_EDIT_TRAILER_LENGTH, m_TrailerLength);
	DDX_Radio(pDX, IDC_RADIO_MONO, m_bStereo);
	DDX_Radio(pDX, IDC_RADIO_8BITS, m_bBits16);
	DDX_Radio(pDX, IDC_RADIO_COMPRESSION_NONE, m_Compression);
	DDX_Radio(pDX, IDC_RADIO_LSB_FIRST, m_bMsbFirst);
	//}}AFX_DATA_MAP
	DWORD MaxHeaderLength = 0xFFFFFFFC;
	if (m_SourceFileSize < 4)
	{
		MaxHeaderLength = 0;
	}
	else if (m_SourceFileSize < 0x100000000i64)
	{
		MaxHeaderLength = DWORD(m_SourceFileSize) - 4;
	}

	DDV_MinMaxDWordHex(pDX, m_HeaderLength, 0, MaxHeaderLength);
	DDV_MinMaxDWordHex(pDX, m_TrailerLength, 0, MaxHeaderLength - m_HeaderLength);
	DDX_Text(pDX, IDC_COMBO_SAMPLING_RATE, m_SamplingRate);

	if (pDX->m_bSaveAndValidate)
	{
		FileParameters * pParams = PersistentFileParameters::GetData();

		pParams->RawFileHeaderLength = m_HeaderLength;

		pParams->RawFileTrailerLength = m_TrailerLength;

		if (m_bStereo)
		{
			m_WaveFormat.nChannels = 2;
		}
		else
		{
			m_WaveFormat.nChannels = 1;
		}

		if (m_bBits16)
		{
			m_WaveFormat.wBitsPerSample = 16;
		}
		else
		{
			m_WaveFormat.wBitsPerSample = 8;
		}

		pParams->RawFileBigEnded = m_bMsbFirst;

		if (m_bBits16)
		{
			m_WaveFormat.wFormatTag = WAVE_FORMAT_PCM;
		}
		else switch(m_Compression)
		{
		case 0:
		default:
			m_WaveFormat.wFormatTag = WAVE_FORMAT_PCM;
			break;
		case 1:
			m_WaveFormat.wFormatTag = WAVE_FORMAT_ALAW;
			break;
		case 2:
			m_WaveFormat.wFormatTag = WAVE_FORMAT_MULAW;
			break;
		}

		m_WaveFormat.nBlockAlign = m_WaveFormat.wBitsPerSample * m_WaveFormat.nChannels / 8;
		m_WaveFormat.nAvgBytesPerSec = m_WaveFormat.nBlockAlign * m_WaveFormat.nSamplesPerSec;

		pParams->RawFileFormat = m_WaveFormat;
	}
}


BEGIN_MESSAGE_MAP(CRawFileParametersDlg, BaseClass)
	//{{AFX_MSG_MAP(CRawFileParametersDlg)
	ON_BN_CLICKED(IDC_RADIO_8BITS, OnClicked8bits)
	ON_BN_CLICKED(IDC_RADIO_16BITS, OnClicked16bits)
	ON_UPDATE_COMMAND_UI(IDC_STATIC_COMPRESSION, OnUpdate8bitsOnly)
	ON_UPDATE_COMMAND_UI(IDC_RADIO_COMPRESSION_NONE, OnUpdate8bitsOnly)
	ON_UPDATE_COMMAND_UI(IDC_RADIO_COMPRESSION_ALAW, OnUpdate8bitsOnly)
	ON_UPDATE_COMMAND_UI(IDC_RADIO_COMPRESSION_ULAW, OnUpdate8bitsOnly)

	ON_UPDATE_COMMAND_UI(IDC_STATIC_BYTE_ORDER, OnUpdate16bitsOnly)
	ON_UPDATE_COMMAND_UI(IDC_RADIO_LSB_FIRST, OnUpdate16bitsOnly)
	ON_UPDATE_COMMAND_UI(IDC_RADIO_MSB_FIRST, OnUpdate16bitsOnly)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CRawFileParametersDlg message handlers

void CRawFileParametersDlg::OnClicked8bits()
{
	m_bBits16 = 0;
	NeedUpdateControls();
}

void CRawFileParametersDlg::OnClicked16bits()
{
	m_bBits16 = 1;
	NeedUpdateControls();
}

void CRawFileParametersDlg::OnUpdate16bitsOnly(CCmdUI * pCmdUI)
{
	pCmdUI->Enable(m_bBits16);
}

void CRawFileParametersDlg::OnUpdate8bitsOnly(CCmdUI * pCmdUI)
{
	pCmdUI->Enable( ! m_bBits16);
}

/////////////////////////////////////////////////////////////////////////////
// CSaveRawFileDlg dialog


CSaveRawFileDlg::CSaveRawFileDlg(CWnd* pParent /*=NULL*/)
	: CDialog(IDD, pParent)
{
	FileParameters * pParams = PersistentFileParameters::GetData();
	m_WaveFormat = pParams->RawFileFormat;

	m_b16Bits = (16 == m_WaveFormat.wBitsPerSample);

	m_bMsbFirst = pParams->RawFileBigEnded;

	if (WAVE_FORMAT_PCM == m_WaveFormat.wFormatTag)
	{
		m_Compression = 0;
	}
	else if (WAVE_FORMAT_ALAW == m_WaveFormat.wFormatTag)
	{
		m_Compression = 1;
	}
	else if (WAVE_FORMAT_MULAW == m_WaveFormat.wFormatTag)
	{
		m_Compression = 2;
	}
}


void CSaveRawFileDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CSaveRawFileDlg)
	DDX_Radio(pDX, IDC_RADIO_8BITS, m_b16Bits);
	DDX_Radio(pDX, IDC_RADIO_COMPRESSION_NONE, m_Compression);
	DDX_Radio(pDX, IDC_RADIO_LSB_FIRST, m_bMsbFirst);
	//}}AFX_DATA_MAP
	if (pDX->m_bSaveAndValidate)
	{
		FileParameters * pParams = PersistentFileParameters::GetData();

		if (m_b16Bits)
		{
			m_WaveFormat.wBitsPerSample = 16;
		}
		else
		{
			m_WaveFormat.wBitsPerSample = 8;
		}

		pParams->RawFileBigEnded = m_bMsbFirst;

		if (m_b16Bits)
		{
			m_WaveFormat.wFormatTag = WAVE_FORMAT_PCM;
		}
		else switch(m_Compression)
		{
		case 0:
		default:
			m_WaveFormat.wFormatTag = WAVE_FORMAT_PCM;
		case 1:
			m_WaveFormat.wFormatTag = WAVE_FORMAT_ALAW;
		case 2:
			m_WaveFormat.wFormatTag = WAVE_FORMAT_MULAW;
		}

		m_WaveFormat.nBlockAlign = m_WaveFormat.wBitsPerSample * m_WaveFormat.nChannels / 8;
		m_WaveFormat.nAvgBytesPerSec = m_WaveFormat.nBlockAlign * m_WaveFormat.nSamplesPerSec;

		pParams->RawFileFormat = m_WaveFormat;
	}
}


BEGIN_MESSAGE_MAP(CSaveRawFileDlg, CDialog)
	//{{AFX_MSG_MAP(CSaveRawFileDlg)
		// NOTE: the ClassWizard will add message map macros here
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CSaveRawFileDlg message handlers
