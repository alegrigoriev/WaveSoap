#include "StdAfx.h"
#include "WaveSoapFront.h"
#include "WaveSoapFileDialogs.h"
#include "ShelLink.h"
#include "OperationDialogs2.h"
#include <Dlgs.h>
#include "BladeMP3EncDLL.h"

BOOL AFXAPI AfxComparePath(LPCTSTR lpszPath1, LPCTSTR lpszPath2);

BEGIN_MESSAGE_MAP(CWaveSoapFileOpenDialog, CFileDialogWithHistory)
	//{{AFX_MSG_MAP(CWaveSoapFileOpenDialog)
	ON_BN_CLICKED(IDC_CHECK_READONLY, OnCheckReadOnly)
	ON_BN_CLICKED(IDC_CHECK_DIRECT, OnCheckDirectMode)
	ON_WM_SIZE()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

static Mp3Bitrates[] = { 64, 96, 128, 160, 192, 256, 320 };

void CWaveSoapFileOpenDialog::ShowWmaFileInfo(CDirectFile & File)
{
	if ( ! GetApp()->CanOpenWindowsMedia())
	{
		ClearFileInfoDisplay();
		return;
	}

	HRESULT CoInitResult = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
	TRACE("CWaveSoapFileOpenDialog::ShowWmaFileInfo CoInitializeEx=%x\n", CoInitResult);
	CWmaDecoder WmaFile;
	if (WmaFile.Init()
		&& SUCCEEDED(WmaFile.Open(File)))
	{
		// can't open direct and readonly
		CWnd * pWnd = GetDlgItem(IDC_CHECK_READONLY);
		if (pWnd)
		{
			pWnd->EnableWindow(FALSE);
		}
		pWnd = GetDlgItem(IDC_CHECK_DIRECT);
		if (pWnd)
		{
			pWnd->EnableWindow(FALSE);
		}
		if (WmaFile.m_SrcWf.FormatTag() == WAVE_FORMAT_MPEGLAYER3)
		{
			SetDlgItemText(IDC_STATIC_FILE_TYPE, _T("MP3 Audio File"));
			SetDlgItemText(IDC_STATIC_FILE_FORMAT, _T("MPEG Layer III"));
		}
		else
		{
			SetDlgItemText(IDC_STATIC_FILE_TYPE, _T("Windows Media File"));
			SetDlgItemText(IDC_STATIC_FILE_FORMAT, _T("Windows Media Audio"));
		}
		// length
		CString s;
		s.Format(_T("%s (%s)"),
				LPCTSTR(TimeToHhMmSs(MulDiv(WmaFile.m_CurrentSamples, 1000,
											WmaFile.m_SrcWf.SampleRate()))),
				LPCTSTR(LtoaCS(WmaFile.m_CurrentSamples)));
		SetDlgItemText(IDC_STATIC_FILE_LENGTH, s);
		// num of channels, bitrate, sampling rate
		s.Format("%s bps, %s Hz, %s", LPCTSTR(LtoaCS(WmaFile.m_Bitrate)),
				LPCTSTR(LtoaCS(WmaFile.m_SrcWf.SampleRate())),
				1 == WmaFile.m_SrcWf.NumChannels() ? _T("Mono") : _T("Stereo"));
		SetDlgItemText(IDC_STATIC_ATTRIBUTES, s);
	}
	else
	{
		ClearFileInfoDisplay();
	}

	if (SUCCEEDED(CoInitResult))
	{
		CoUninitialize();
	}
}

void CWaveSoapFileOpenDialog::OnCheckReadOnly()
{
	CButton * pRO = (CButton *)GetDlgItem(IDC_CHECK_READONLY);
	CWnd * pDirect = GetDlgItem(IDC_CHECK_DIRECT);
	if (NULL != pRO)
	{
		m_bReadOnly = (0 != pRO->GetCheck());
		if (NULL != pDirect)
		{
			pDirect->EnableWindow( ! m_bReadOnly);
		}
	}
}

void CWaveSoapFileOpenDialog::OnCheckDirectMode()
{
	CButton * pDirect = (CButton *)GetDlgItem(IDC_CHECK_DIRECT);
	if (NULL != pDirect)
	{
		m_bDirectMode = (0 != pDirect->GetCheck());
	}
}

BOOL CWaveSoapFileOpenDialog::OnFileNameOK()
{

	m_WaveFile.Close();
	return CFileDialogWithHistory::OnFileNameOK();
}

void CWaveSoapFileOpenDialog::OnInitDone()
{
	CFileDialogWithHistory::OnInitDone();
	ClearFileInfoDisplay();

	CButton * pRO = (CButton *)GetDlgItem(IDC_CHECK_READONLY);
	CButton * pDirect = (CButton *)GetDlgItem(IDC_CHECK_DIRECT);
	if (NULL != pRO)
	{
		pRO->SetCheck(m_bReadOnly);
	}
	if (NULL != pDirect)
	{
		pDirect->SetCheck(m_bDirectMode);
		pDirect->EnableWindow( ! m_bReadOnly);
	}
}

void CWaveSoapFileOpenDialog::OnFileNameChange()
{
	// get the file info
	CString sName;
	TCHAR * pBuf = sName.GetBuffer(m_ofn.nMaxFile);
	if (NULL == pBuf)
	{
		ClearFileInfoDisplay();
		return;
	}
	if (GetParent()->SendMessage(CDM_GETFILEPATH, (WPARAM)m_ofn.nMaxFile,
								(LPARAM)pBuf) < 0)
	{
		ClearFileInfoDisplay();
		return;
	}
	TRACE("CWaveSoapFileOpenDialog::OnFileNameChange=%s\n", LPCTSTR(sName));
	// if one file selected, its name will be in the buffer.
	// If multiple files selected, the buffer will contain directory name,
	// then file name surrounded with double quotes,
	// then other filenames surrounded with quotes and delimited with space
	TCHAR * tmp = pBuf;
	while (*tmp != 0)
	{
		if ('"' == *tmp)
		{
			// multiple selection
			ClearFileInfoDisplay();
			return;
		}
		tmp++;
	}
	// try to open the file for reading (it can be .LNK, though!)
	CString FileName = ResolveIfShellLink(pBuf);
	sName.ReleaseBuffer();
	// get file name extension
	if (FileName.IsEmpty()
		|| FALSE == m_WaveFile.Open(FileName,
									MmioFileOpenExisting
									| MmioFileOpenReadOnly
									| MmioFileOpenDontLoadRiff))
	{
		ClearFileInfoDisplay();
		return;
	}
	// read first bytes of the file to check for WAVE RIFF
	DWORD WaveHeader[3];    // WaveHeader[0] = 'FFIR', [2]='EVAW'
	if (sizeof WaveHeader == m_WaveFile.ReadAt(WaveHeader, sizeof WaveHeader, 0))
	{
		if (WaveHeader[0] != 'FFIR'
			|| WaveHeader[2] != 'EVAW')
		{
			ShowWmaFileInfo(m_WaveFile);
			return;
		}
	}
	else
	{
		ClearFileInfoDisplay();
		return;
	}
	if (m_WaveFile.LoadRiffChunk()
		&& m_WaveFile.LoadWaveformat()
		&& m_WaveFile.FindData())
	{
		WAVEFORMATEX * pWf = m_WaveFile.GetWaveFormat();
		if (pWf != NULL)
		{
			nChannels = pWf->nChannels;
			nBitsPerSample = pWf->wBitsPerSample;
			if (16 == nBitsPerSample
				&& WAVE_FORMAT_PCM == pWf->wFormatTag
				&& (nChannels == 1 || nChannels == 2))
			{
				// can open direct and readonly
				CWnd * pWnd = GetDlgItem(IDC_CHECK_READONLY);
				if (pWnd)
				{
					pWnd->EnableWindow(TRUE);
				}
				pWnd = GetDlgItem(IDC_CHECK_DIRECT);
				if (pWnd)
				{
					pWnd->EnableWindow(TRUE);
				}
			}
			else
			{
				// can't open direct and readonly
				CWnd * pWnd = GetDlgItem(IDC_CHECK_READONLY);
				if (pWnd)
				{
					pWnd->EnableWindow(FALSE);
				}
				pWnd = GetDlgItem(IDC_CHECK_DIRECT);
				if (pWnd)
				{
					pWnd->EnableWindow(FALSE);
				}
			}
			nSamplingRate = pWf->nSamplesPerSec;
			if (m_WaveFile.m_FactSamples != -1
				&& (pWf->wFormatTag != WAVE_FORMAT_PCM
					|| (pWf->wBitsPerSample != 16 && pWf->wBitsPerSample != 8)))
			{
				nSamples = m_WaveFile.m_FactSamples;
			}
			else
			{
				nSamples = m_WaveFile.NumberOfSamples();
			}
			SetDlgItemText(IDC_STATIC_FILE_TYPE, _T("Microsoft RIFF Wave"));
			CString s;
			CString s2;

			// get format name
			ACMFORMATDETAILS afd;
			memzero(afd);

			afd.cbStruct = sizeof afd;
			afd.pwfx = pWf;
			afd.cbwfx = sizeof (WAVEFORMATEX) + pWf->cbSize;
			afd.dwFormatTag = pWf->wFormatTag;

			if (MMSYSERR_NOERROR == acmFormatDetails(NULL, & afd,
													ACM_FORMATDETAILSF_FORMAT))
			{
				SetDlgItemText(IDC_STATIC_ATTRIBUTES, afd.szFormat);
			}
			else
			{
				SetDlgItemText(IDC_STATIC_ATTRIBUTES, _T("Unknown"));
			}

			s2.Format(_T("%s (%s)"),
					LPCTSTR(TimeToHhMmSs(MulDiv(nSamples, 1000, nSamplingRate))),
					LPCTSTR(LtoaCS(nSamples)));
			SetDlgItemText(IDC_STATIC_FILE_LENGTH, s2);

			ACMFORMATTAGDETAILS aft;
			memzero(aft);
			aft.cbStruct = sizeof aft;
			aft.dwFormatTag = afd.dwFormatTag;

			if (MMSYSERR_NOERROR == acmFormatTagDetails(NULL, & aft,
														ACM_FORMATTAGDETAILSF_FORMATTAG))
			{
				SetDlgItemText(IDC_STATIC_FILE_FORMAT, aft.szFormatTag);
			}
			else
			{
				SetDlgItemText(IDC_STATIC_FILE_FORMAT, _T("Unknown"));
			}
		}
		else
		{
			ClearFileInfoDisplay();
		}
	}
	else
	{
		ClearFileInfoDisplay();
	}
	m_WaveFile.Close(); // don't want to keep it in use
	//POSITION pos = Get
}

void CWaveSoapFileOpenDialog::ClearFileInfoDisplay()
{
	SetDlgItemText(IDC_STATIC_FILE_TYPE, _T("Unknown"));
	SetDlgItemText(IDC_STATIC_FILE_FORMAT, _T(""));
	SetDlgItemText(IDC_STATIC_FILE_LENGTH, _T(""));
	SetDlgItemText(IDC_STATIC_ATTRIBUTES, _T(""));
	SetDlgItemText(IDC_EDIT_FILE_COMMENTS, _T(""));
}

void CWaveSoapFileOpenDialog::OnTypeChange()
{
	// display warning if can't open MP3 and WMA
	TRACE("Filter changed to %d\n", m_ofn.nFilterIndex);
	CThisApp * pApp = GetApp();
	// the function is called twice in a row
	if (m_PrevFilter == m_ofn.nFilterIndex)
	{
		return;
	}
	m_PrevFilter = m_ofn.nFilterIndex;
	if (m_ofn.nFilterIndex >= m_MinWmaFilter + 1
		&& m_ofn.nFilterIndex < m_MaxWmaFilter + 1
		&& ! pApp->CanOpenWindowsMedia()
		&& ! pApp->m_DontShowMediaPlayerWarning
		)
	{
		CWmpNotInstalleedWarningDlg dlg;
		dlg.m_DontShowAnymore = pApp->m_DontShowMediaPlayerWarning;
		dlg.DoModal();
		pApp->m_DontShowMediaPlayerWarning = dlg.m_DontShowAnymore;
	}
}

void CWaveSoapFileOpenDialog::OnSize(UINT nType, int cx, int cy)
{
	CFileDialogWithHistory::OnSize(nType, cx, cy);
	// move dialog items
}

BEGIN_MESSAGE_MAP(CWaveSoapFileSaveDialog, CFileDialogWithHistory)
	//{{AFX_MSG_MAP(CWaveSoapFileSaveDialog)
	ON_BN_CLICKED(IDC_CHECK_COMPATIBLE_FORMATS, OnCompatibleFormatsClicked)
	ON_CBN_SELCHANGE(IDC_COMBO_FORMAT, OnComboFormatsChange)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

BOOL CWaveSoapFileSaveDialog::OnFileNameOK()
{
	// if the file type is different from the selected type,
	// set the type and let the user select the parameters
	CString Name = GetPathName();
	int type = GetFileTypeForName(Name);
	if (type != m_FileType)
	{
		SetFileType(type);
		return 1;   // don't close
	}
	CFileDialogWithHistory::OnFileNameOK();
	// save format selection
	if (m_FileType == SoundFileWav
		|| m_FileType == SoundFileWma)
	{
		m_SelectedFormat = m_AttributesCombo.GetCurSel();
	}
	else if (m_FileType == SoundFileMp3)
	{
		m_SelectedLameMp3Bitrate = Mp3Bitrates[m_AttributesCombo.GetCurSel()];
	}

	return 0;   // OK to close dialog
}

UINT CWaveSoapFileSaveDialog::OnShareViolation( LPCTSTR lpszPathName)
{
	// if the name is the same as current document name, just return
	// if it's different, display warning
	if (NULL != m_pDocument
		&& m_pDocument->m_OriginalWavFile.IsOpen()
		&& AfxComparePath(m_pDocument->m_OriginalWavFile.GetName(), lpszPathName))
	{
		return OFN_SHAREFALLTHROUGH;
	}
	return OFN_SHAREWARN;
}

WAVEFORMATEX * CWaveSoapFileSaveDialog::GetWaveFormat()
{
	switch (m_FileType)
	{
	case SoundFileRaw:
		return NULL;
		break;

	case SoundFileMp3:
		if (m_Mp3Encoders[m_SelectedMp3Encoder] != Mp3EncoderAcm)
		{
			return NULL;
		}
	default:
	case SoundFileWma:
	case SoundFileWav:
		if (m_SelectedFormat >= m_Acm.m_Formats.size()
			|| m_SelectedFormat < 0)
		{
			return NULL;
		}
		return m_Acm.m_Formats[m_SelectedFormat].Wf;
		break;
	}
}
static WaveFormatTagEx const ExcludeFormats[] =
{
	{WAVE_FORMAT_MPEGLAYER3},
	{WAVE_FORMAT_MSAUDIO1},
	{WAVE_FORMAT_MSAUDIO1+1},
};

void CWaveSoapFileSaveDialog::FillFormatTagCombo(WaveFormatTagEx const ListOfTags[], int NumTags, DWORD Flags)
{
	if (m_bCompatibleFormatsOnly)
	{
		Flags |= WaveFormatMatchCompatibleFormats;
	}
	m_Acm.FillFormatTagArray(m_Wf, ListOfTags, NumTags, Flags);

	if (m_FormatCombo.m_hWnd == NULL
		&& ! m_FormatCombo.SubclassDlgItem(IDC_COMBO_FORMAT, this))
	{
		return;
	}
	m_FormatCombo.ResetContent();

	int sel = -1;
	for (int i = 0; i < m_Acm.m_FormatTags.size(); i++)
	{
		if (m_Acm.m_FormatTags[i].Tag == m_SelectedTag)
		{
			sel = i;
		}
		m_FormatCombo.AddString(m_Acm.m_FormatTags[i].Name);
	}

	if (-1 == sel
		&& m_Acm.m_FormatTags.size() > 0)
	{
		sel = 0;
		m_SelectedTag = m_Acm.m_FormatTags[0].Tag;
	}
	m_FormatCombo.SetCurSel(sel);
}

int CWaveSoapFileSaveDialog::FillFormatCombo(int SelFormat, int Flags)
{
	if (m_AttributesCombo.m_hWnd == NULL
		&& ! m_AttributesCombo.SubclassDlgItem(IDC_COMBO_ATTRIBUTES, this))
	{
		return 0;
	}

	m_AttributesCombo.ResetContent();
	if (SelFormat >= m_Acm.m_FormatTags.size())
	{
		return 0;
	}
	m_AttributesCombo.LockWindowUpdate();

	m_Acm.FillFormatArray(SelFormat, Flags);

	DWORD dwFormatTag = m_Acm.m_FormatTags[SelFormat].Tag.Tag;
	int i;

	for (i = 0; i < m_Acm.m_Formats.size(); i++)
	{
		m_AttributesCombo.AddString(m_Acm.m_Formats[i].Name);
	}

	int sel = -1;
	int BestMatch = 0;
	for (i = 0; i < m_Acm.m_Formats.size(); i++)
	{
		WAVEFORMATEX * pwf = m_Acm.m_Formats[i].Wf;
		int match = m_Wf.MatchFormat(pwf);
		if (WaveFormatExactMatch & match)
		{
			// exact match found
			sel = i;
			break;
		}
		// select the best match
		// Sample rate must match, then number of channels might match
		// If original format is non PCM and the queried format is the same format

		if (match > BestMatch)
		{
			BestMatch = match;
			sel = i;
			continue;
		}
		if (0 == match
			|| match < BestMatch)
		{
			continue;
		}
		if (dwFormatTag == WAVE_FORMAT_PCM)
		{
			if (-1 == sel
				|| pwf->wBitsPerSample >= m_Acm.m_Formats[sel].Wf.BitsPerSample())
			{
				sel = i;
			}
		}
		else
		{
			if (-1 == sel
				|| pwf->nAvgBytesPerSec >= m_Acm.m_Formats[sel].Wf.BytesPerSec())
			{
				sel = i;
			}
		}
	}

	if (-1 == sel)
	{
		if (dwFormatTag != WAVE_FORMAT_PCM)
		{
			sel = 0;
		}
		else
		{
			// sample rate is non standard
			// add two formats to the list for mono and stereo
			for (int ch = 2; ch >= 1; ch--)
			{
				WAVEFORMATEX wf =
				{
					WAVE_FORMAT_PCM,
					ch,
					m_Wf.SampleRate(),
					m_Wf.SampleRate() * ch * 2,  // avg bytes per sec
					ch * 2,  // block align
					16, // wBitsPerSample
					0
				};

				ACMFORMATDETAILS afd;
				memzero(afd);
				afd.cbStruct = sizeof afd;
				afd.pwfx = & wf;
				afd.cbwfx = sizeof (WAVEFORMATEX);
				afd.dwFormatTag = WAVE_FORMAT_PCM;

				acmFormatDetails(NULL, & afd, ACM_FORMATDETAILSF_FORMAT);
				m_Acm.m_Formats.insert(m_Acm.m_Formats.begin());
				m_Acm.m_Formats[0].Wf = & wf;
				m_Acm.m_Formats[0].Name = afd.szFormat;

				m_AttributesCombo.InsertString(0, afd.szFormat);
			}
			if (m_Wf.NumChannels() == m_Acm.m_Formats[0].Wf.NumChannels())
			{
				sel = 0;
			}
			else
			{
				sel = 1;
			}
		}
	}
	m_AttributesCombo.SetCurSel(sel);
	m_AttributesCombo.UnlockWindowUpdate();
	return sel;
}

void CWaveSoapFileSaveDialog::OnCompatibleFormatsClicked()
{
	m_bCompatibleFormatsOnly = ((CButton*)GetDlgItem(IDC_CHECK_COMPATIBLE_FORMATS))->GetCheck();
	if (SoundFileWav == m_FileType)
	{
		FillFormatTagCombo(ExcludeFormats,
							sizeof ExcludeFormats / sizeof ExcludeFormats[0], WaveFormatExcludeFormats);
		m_SelectedFormat = FillFormatCombo(m_FormatCombo.GetCurSel());
	}
}

void CWaveSoapFileSaveDialog::OnComboFormatsChange()
{
	int sel = m_FormatCombo.GetCurSel();
	switch (m_FileType)
	{
	case SoundFileWav:
		// WAV file
		m_SelectedTag = m_Acm.m_FormatTags[sel].Tag;
		m_SelectedFormat = FillFormatCombo(sel);
		break;
	case SoundFileMp3:
		// MP3 file
		FillMp3FormatCombo();
		break;
	case SoundFileWma:
		// WMA file
		// never called!
		//FillFormatCombo(sel, MatchNumChannels | MatchSamplingRate);
		break;
	case SoundFileRaw:
		// RAW file
		m_SelectedRawFormat = sel;
		break;
		//case SoundFileAvi:
		// RAW file
		break;
	}
}

int CWaveSoapFileSaveDialog::GetLameEncBitrate() const
{
	switch (m_SelectedLameMp3Bitrate)
	{
	case LameEncBitrate64:
		return 64000;
		break;
	case LameEncBitrate96:
		return 96000;
		break;
	default:
	case LameEncBitrate128:
		return 128000;
		break;
	case LameEncBitrate160:
		return 160000;
		break;
	case LameEncBitrate192:
		return 192000;
		break;
	case LameEncBitrate256:
		return 256000;
		break;
	case LameEncBitrate320:
		return 320000;
		break;
	}
}

void CWaveSoapFileSaveDialog::OnInitDone()
{
	//fill format combo box.
	m_SelectedTag = m_Wf;
	m_Acm.m_Wf = m_Wf;
	((CButton*)GetDlgItem(IDC_CHECK_COMPATIBLE_FORMATS))->SetCheck(m_bCompatibleFormatsOnly);

	SetFileType(m_ofn.nFilterIndex);

	CFileDialogWithHistory::OnInitDone();
}

void CWaveSoapFileSaveDialog::OnTypeChange()
{
	TRACE("Current type = %d\n", m_ofn.nFilterIndex);
	// get file name
	CString name;
	// set new default extension
	CWnd * pParent = GetParent();
	pParent->SendMessage(CDM_SETDEFEXT, 0, LPARAM(LPCTSTR(m_DefExt[m_ofn.nFilterIndex])));
	CWnd * pTmp = pParent->GetDlgItem(edt1);
	if (NULL == pTmp)
	{
		// new style dialog
		pTmp = pParent->GetDlgItem(cmb13);
	}
	if (NULL != pTmp)
	{
		pTmp->SetFocus();
	}
	pTmp->GetWindowText(name);
	// get the extension
	if ( ! name.IsEmpty())
	{
		int idx = name.ReverseFind('.');
		// replace the extension
		if (idx != -1)
		{
			// need to replace
			if (idx >= name.GetLength() - 4)
			{
				name.Delete(idx + 1, name.GetLength() - idx - 1);
				name += m_DefExt[m_ofn.nFilterIndex];
			}
			pParent->SendMessage(CDM_SETCONTROLTEXT, edt1, LPARAM(LPCTSTR(name)));
		}
	}
	SetFileType(m_ofn.nFilterIndex);
}

void CWaveSoapFileSaveDialog::OnFileNameChange()
{
	// if the file type is different from the selected type,
	// set the type and let the user select the parameters
	CString Name = GetPathName();
	int type = GetFileTypeForName(Name);
	if (type != m_FileType)
	{
		SetFileType(type);
	}
}

void CWaveSoapFileSaveDialog::ShowDlgItem(UINT nID, int nCmdShow)
{
	CWnd * pWnd = GetDlgItem(nID);
	if (pWnd)
	{
		pWnd->ShowWindow(nCmdShow);
	}
}

void CWaveSoapFileSaveDialog::SetFileType(int nType)
{
	// Wav, Mp3, wma, raw...
	CString s;
	m_FileType = nType;
	switch (nType)
	{
	default:
	case SoundFileWav:
		// WAV file
		// show Comments, fill formats combo box
		s.LoadString(IDS_FORMAT);
		SetDlgItemText(IDC_STATIC_FORMAT, s);
		ShowDlgItem(IDC_STATIC_FORMAT, SW_SHOWNOACTIVATE);
		ShowDlgItem(IDC_COMBO_FORMAT, SW_SHOWNOACTIVATE);
		ShowDlgItem(IDC_COMBO_ATTRIBUTES, SW_SHOWNOACTIVATE);
		ShowDlgItem(IDC_CHECK_COMPATIBLE_FORMATS, SW_SHOWNOACTIVATE);
		ShowDlgItem(IDC_STATIC_ATTRIBUTES, SW_SHOWNOACTIVATE);
		ShowDlgItem(IDC_STATIC_COMMENTS, SW_SHOWNOACTIVATE);
		ShowDlgItem(IDC_EDIT_COMMENT, SW_SHOWNOACTIVATE);

		FillFormatTagCombo(ExcludeFormats,
							sizeof ExcludeFormats / sizeof ExcludeFormats[0],
							WaveFormatExcludeFormats);
		m_SelectedFormat = FillFormatCombo(m_FormatCombo.GetCurSel());
		break;  // go on
	case SoundFileMp3:
		// MP3 file
		// replace Format with Encoder: (LAME, Fraunhofer
		s.LoadString(IDS_ENCODER);
		SetDlgItemText(IDC_STATIC_FORMAT, s);
		ShowDlgItem(IDC_STATIC_FORMAT, SW_SHOWNOACTIVATE);
		ShowDlgItem(IDC_COMBO_FORMAT, SW_SHOWNOACTIVATE);
		ShowDlgItem(IDC_COMBO_ATTRIBUTES, SW_SHOWNOACTIVATE);
		ShowDlgItem(IDC_STATIC_ATTRIBUTES, SW_SHOWNOACTIVATE);
		ShowDlgItem(IDC_CHECK_COMPATIBLE_FORMATS, SW_HIDE);
		ShowDlgItem(IDC_STATIC_COMMENTS, SW_HIDE);
		ShowDlgItem(IDC_EDIT_COMMENT, SW_HIDE);
		// Hide Comments, show Artist, Genre, Title
		//
		// set tag to MP3
		// fill formats combo (bitrate, etc)
		FillMp3EncoderCombo();
		break;
	case SoundFileWma:
		// WMA file
		// remove Format: combo
		ShowDlgItem(IDC_STATIC_FORMAT, SW_HIDE);
		ShowDlgItem(IDC_COMBO_FORMAT, SW_HIDE);
		ShowDlgItem(IDC_COMBO_ATTRIBUTES, SW_SHOWNOACTIVATE);
		ShowDlgItem(IDC_STATIC_ATTRIBUTES, SW_SHOWNOACTIVATE);
		ShowDlgItem(IDC_CHECK_COMPATIBLE_FORMATS, SW_HIDE);
		ShowDlgItem(IDC_STATIC_COMMENTS, SW_HIDE);
		ShowDlgItem(IDC_EDIT_COMMENT, SW_HIDE);
		// fill profiles combo
		FillWmaFormatCombo();
		break;
		//case SoundFileAvi:
		// AVI file
		break;
	case SoundFileRaw:
		// RAW file
		// show formats: 16 bits LSB first, 16 bits MSB first, a-law, u-law, 8 bits, ascii hex, ascii decimal
		// hide attributes
		s.LoadString(IDS_FORMAT);
		SetDlgItemText(IDC_STATIC_FORMAT, s);

		m_FormatCombo.ResetContent();
		s.LoadString(IDS_RAW_16BIT_LSB);
		m_FormatCombo.InsertString(RawSoundFilePcm16Lsb, s);
		s.LoadString(IDS_RAW_16BIT_MSB);
		m_FormatCombo.InsertString(RawSoundFilePcm16Msb, s);
		s.LoadString(IDS_RAW_8BITS_PCM);
		m_FormatCombo.InsertString(RawSoundFilePcm8, s);
		s.LoadString(IDS_RAW_8BITS_ALAW);
		m_FormatCombo.InsertString(RawSoundFileALaw8, s);
		s.LoadString(IDS_RAW_8BITS_ULAW);
		m_FormatCombo.InsertString(RawSoundFileULaw8, s);

		m_FormatCombo.SetCurSel(0);

		ShowDlgItem(IDC_STATIC_FORMAT, SW_SHOWNOACTIVATE);
		ShowDlgItem(IDC_COMBO_FORMAT, SW_SHOWNOACTIVATE);
		ShowDlgItem(IDC_COMBO_ATTRIBUTES, SW_HIDE);
		ShowDlgItem(IDC_CHECK_COMPATIBLE_FORMATS, SW_HIDE);
		ShowDlgItem(IDC_STATIC_ATTRIBUTES, SW_HIDE);
		ShowDlgItem(IDC_STATIC_COMMENTS, SW_HIDE);
		ShowDlgItem(IDC_EDIT_COMMENT, SW_HIDE);

		break;
	}
}

void CWaveSoapFileSaveDialog::SetFileType(LPCTSTR lpExt)
{
	SetFileType(GetFileTypeForExt(lpExt));
}

int CWaveSoapFileSaveDialog::GetFileTypeForExt(LPCTSTR lpExt)
{
	if (NULL != lpExt && '.' == *lpExt)
	{
		lpExt++;
	}
	int type = SoundFileRaw;
	if (NULL != lpExt && 0 != *lpExt)
	{
		CString s(lpExt);
		s.TrimRight();
		for (int i = SoundFileTypeMin; i <= SoundFileTypeMax; i++)
		{
			if (0 == m_DefExt[i].CompareNoCase(lpExt))
			{
				type = i;
				break;
			}
		}
	}
	return type;
}

int CWaveSoapFileSaveDialog::GetFileTypeForName(LPCTSTR FileName)
{
	LPCTSTR ext = _tcsrchr(FileName, '.');
	if (NULL == ext)
	{
		return m_FileType;
	}
	return GetFileTypeForExt(ext + 1);
}

void CWaveSoapFileSaveDialog::FillLameEncoderFormats()
{
	CString ms;
	if (1 == m_Wf.NumChannels())
	{
		ms.LoadString(IDS_MONO);
	}
	else
	{
		ms.LoadString(IDS_STEREO);
	}

	CString f;
	f.LoadString(IDS_LAMEENC_FORMAT);

	m_AttributesCombo.ResetContent();
	for (int i = 0; i < sizeof Mp3Bitrates / sizeof Mp3Bitrates[0]; i++)
	{
		CString s;
		s.Format(f, Mp3Bitrates[i], LPCTSTR(ms));
		m_AttributesCombo.AddString(s);
		if (Mp3Bitrates[i] == m_SelectedLameMp3Bitrate)
		{
			m_AttributesCombo.SetCurSel(i);
		}
	}
}

void CWaveSoapFileSaveDialog::FillWmaFormatCombo()
{
	m_Acm.FillWmaFormatTags();
	m_SelectedFormat = FillFormatCombo(0, WaveFormatMatchCnannels | WaveFormatMatchSampleRate);
}

void CWaveSoapFileSaveDialog::FillMp3FormatCombo()
{
	unsigned sel = m_FormatCombo.GetCurSel();
	if (sel >= m_Acm.m_FormatTags.size())
	{
		return;
	}

	WaveFormatTagEx SelectedTag = m_Acm.m_FormatTags[sel].Tag;

	if (WAVE_FORMAT_MPEGLAYER3 == SelectedTag.Tag)
	{
		FillFormatCombo(sel, WaveFormatMatchCompatibleFormats);
	}
	else if (BladeMp3Encoder::GetTag() == SelectedTag)
	{
		FillLameEncoderFormats();
	}
}

void CWaveSoapFileSaveDialog::FillMp3EncoderCombo()
{
	// check if LAME encoder is available
	m_FormatCombo.ResetContent();
	m_Acm.FillMp3EncoderTags();

	if (m_Acm.m_FormatTags.empty())
	{
		return;
	}

	for (int i = 0; i < m_Acm.m_FormatTags.size(); i++)
	{
		m_FormatCombo.AddString(m_Acm.m_FormatTags[i].Name);
	}

	if (m_SelectedMp3Encoder >= m_Acm.m_FormatTags.size())
	{
		m_SelectedMp3Encoder = 0;
	}
	m_FormatCombo.SetCurSel(m_SelectedMp3Encoder);
	FillMp3FormatCombo();
}
