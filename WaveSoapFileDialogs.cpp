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

	CoInitializeEx(NULL, COINIT_MULTITHREADED );
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
		if (NULL != WmaFile.m_pSrcWf)
		{
			if (WmaFile.m_pSrcWf->wFormatTag == WAVE_FORMAT_MPEGLAYER3)
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
												WmaFile.m_pSrcWf->nSamplesPerSec))),
					LPCTSTR(LtoaCS(WmaFile.m_CurrentSamples)));
			SetDlgItemText(IDC_STATIC_FILE_LENGTH, s);
			// num of channels, bitrate, sampling rate
			s.Format("%s bps, %s Hz, %s", LPCTSTR(LtoaCS(WmaFile.m_Bitrate)),
					LPCTSTR(LtoaCS(WmaFile.m_pSrcWf->nSamplesPerSec)),
					1 == WmaFile.m_pSrcWf->nSamplesPerSec ? _T("Mono") : _T("Stereo"));
			SetDlgItemText(IDC_STATIC_ATTRIBUTES, s);
		}
	}
	else
	{
		ClearFileInfoDisplay();
	}
	CoUninitialize();
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
			memset (& afd, 0, sizeof afd);
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
			memset (& aft, 0, sizeof aft);
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
	if (m_pDocument->m_OriginalWavFile.IsOpen()
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
		if (m_SelectedFormat >= m_Formats.GetSize()
			|| m_SelectedFormat < 0)
		{
			return NULL;
		}
		return m_Formats[m_SelectedFormat].pWf;
		break;
	}
}

// if can convert to any format of the tag, add the tag to the table
BOOL _stdcall CWaveSoapFileSaveDialog::FormatTestEnumCallback(
															HACMDRIVERID hadid, LPACMFORMATDETAILS pafd,
															DWORD dwInstance, DWORD fdwSupport)
{
	CWaveSoapFileSaveDialog * pDlg = (CWaveSoapFileSaveDialog *) dwInstance;
	TRACE("FormatTestEnumCallback: format=%s, tag=%d, \n", pafd->szFormat, pafd->dwFormatTag);
	if (pDlg->m_CurrentEnumeratedTag == pafd->dwFormatTag)
	{
		int nIndex = pDlg->m_FormatTags.GetSize();
		pDlg->m_FormatTags.SetSize(nIndex+1);
		pDlg->m_FormatTags[nIndex].SetData(pafd->dwFormatTag, pDlg->m_FormatTagName, hadid);
		return FALSE;   // no more enumeration required
	}
	else
	{
		return TRUE;    // enumerate more
	}
}

struct FormatTagEnumStruct
{
	int const * pListOfTags;
	int NumTags;
	CWaveSoapFileSaveDialog * pDlg;
};

BOOL _stdcall CWaveSoapFileSaveDialog::FormatTagEnumCallback(
															HACMDRIVERID hadid, LPACMFORMATTAGDETAILS paftd,
															DWORD dwInstance, DWORD fdwSupport)
{
	FormatTagEnumStruct * pfts = (FormatTagEnumStruct *) dwInstance;
	CWaveSoapFileSaveDialog * pDlg = pfts->pDlg;

	if (pfts->NumTags != 0
		&& pfts->pListOfTags != NULL)
	{
		for (int i = 0; i < pfts->NumTags; i++)
		{
			if (paftd->dwFormatTag == pfts->pListOfTags[i])
			{
				break;
			}
		}
		if (i == pfts->NumTags)
		{
			// skip this tag
			return TRUE;
		}
	}

	WAVEFORMATEX * pwfx = (WAVEFORMATEX *)new char[0xFFFF];
	ACMFORMATDETAILS afd;
	TRACE("FormatTagEnum: name=%s, driverID=%x, tag=%d, formats=%d, max size=%d\n", paftd->szFormatTag,
		hadid, paftd->dwFormatTag, paftd->cStandardFormats, paftd->cbFormatSize);
	pwfx->cbSize = 0;
	pwfx->wFormatTag = WAVE_FORMAT_PCM;
	pwfx->nChannels = pDlg->m_pWf->nChannels;
	pwfx->nSamplesPerSec = pDlg->m_pWf->nSamplesPerSec;
	pwfx->wBitsPerSample = 16;
	pwfx->nBlockAlign = pDlg->m_pWf->nChannels * 2;
	pwfx->nAvgBytesPerSec = pwfx->nSamplesPerSec * pwfx->nBlockAlign;
	memset( & afd, 0, sizeof afd);
	afd.cbStruct = sizeof afd;
	afd.cbwfx = 0xFFFF;
	afd.dwFormatTag = paftd->dwFormatTag;
	afd.pwfx = pwfx;
	pDlg->m_CurrentEnumeratedTag = afd.dwFormatTag;

	HACMDRIVER had = NULL;
	if (MMSYSERR_NOERROR == acmDriverOpen(&had, hadid, 0))
	{
		DWORD flags = 0;
		pDlg->m_FormatTagName = paftd->szFormatTag;
		//pDlg->m_FormatTagName += " ";
		if (paftd->dwFormatTag != WAVE_FORMAT_PCM
			&& pDlg->m_bCompatibleFormatsOnly)
		{
			flags = ACM_FORMATENUMF_CONVERT;
		}
		int res = acmFormatEnum(had, & afd, FormatTestEnumCallback, DWORD(pDlg), flags);
		TRACE("acmFormatEnum returned %x\n", res);
		acmDriverClose(had, 0);
	}
	delete [] (char*)pwfx;
	return TRUE;
}

void CWaveSoapFileSaveDialog::FillFormatTagArray(int const ListOfTags[], int NumTags)
{

	m_FormatTags.RemoveAll();
	ACMFORMATTAGDETAILS atd;
	FormatTagEnumStruct fts;
	fts.pDlg = this;
	fts.NumTags = NumTags;
	fts.pListOfTags = ListOfTags;

	// enum all formats
	memset( & atd, 0, sizeof atd);
	atd.cbStruct = sizeof atd;
	atd.dwFormatTag = WAVE_FORMAT_UNKNOWN;
	acmFormatTagEnum(NULL, & atd, FormatTagEnumCallback, DWORD( & fts), 0);

}

void CWaveSoapFileSaveDialog::FillFormatTagCombo(int const ListOfTags[], int NumTags)
{
	FillFormatTagArray(ListOfTags, NumTags);

	if (m_FormatCombo.m_hWnd == NULL
		&& ! m_FormatCombo.SubclassDlgItem(IDC_COMBO_FORMAT, this))
	{
		return;
	}
	m_FormatCombo.ResetContent();

	int sel = -1;
	for (int i = 0; i < m_FormatTags.GetSize(); i++)
	{
		if (m_FormatTags[i].dwTag == m_SelectedTag)
		{
			sel = i;
		}
		m_FormatCombo.AddString(m_FormatTags[i].Name);
	}

	if (-1 == sel
		&& m_FormatTags.GetSize() > 0)
	{
		sel = 0;
		m_SelectedTag = m_FormatTags[0].dwTag;
	}
	m_FormatCombo.SetCurSel(sel);
}

struct FormatEnumCallbackStruct
{
	CWaveSoapFileSaveDialog * pDlg;
	int Flags;
};
// enumerates all formats for the tag
BOOL _stdcall CWaveSoapFileSaveDialog::FormatEnumCallback(
														HACMDRIVERID hadid, LPACMFORMATDETAILS pafd,
														DWORD dwInstance, DWORD fdwSupport)
{
	FormatEnumCallbackStruct * pfcs = (FormatEnumCallbackStruct *) dwInstance;

	CWaveSoapFileSaveDialog * pDlg = pfcs->pDlg;
	TRACE("FormatEnum: format=%s, tag=%d\n", pafd->szFormat, pafd->dwFormatTag);
#ifdef _DEBUG
	if (WAVE_FORMAT_MPEGLAYER3 == pafd->dwFormatTag)
	{
		MPEGLAYER3WAVEFORMAT * pwf = (MPEGLAYER3WAVEFORMAT *) pafd->pwfx;
		TRACE("\"%s\", cbSize = %d, wID=%X, fdwFlags=%X, nBlockSize=%d, nFramesPerBlock=%d, nCodecDelay=%d\n",
			pafd->szFormat, pafd->pwfx->cbSize, pwf->wID, pwf->fdwFlags,
			pwf->nBlockSize, pwf->nFramesPerBlock, pwf->nCodecDelay);
	}
#endif
	if (pDlg->m_CurrentEnumeratedTag == pafd->dwFormatTag)
	{
		if (pfcs->Flags & MatchCompatibleFormats)
		{
			if (pDlg->m_CurrentEnumeratedTag != WAVE_FORMAT_PCM
				|| (pafd->pwfx->nSamplesPerSec == pDlg->m_pWf->nSamplesPerSec
					&& pafd->pwfx->nChannels == pDlg->m_pWf->nChannels
					&& (pafd->pwfx->wBitsPerSample == pDlg->m_pWf->wBitsPerSample
						|| 16 == pafd->pwfx->wBitsPerSample)))
			{
				int nIndex = pDlg->m_Formats.GetSize();
				pDlg->m_Formats.SetSize(nIndex+1);
				pDlg->m_Formats[nIndex].SetData(pafd->pwfx, pafd->szFormat);
			}
		}
		else
		{
			// check which flags are specified
			if ((0 == (pfcs->Flags & MatchNumChannels)
					|| pafd->pwfx->nChannels == pDlg->m_pWf->nChannels)
				&& (0 == (pfcs->Flags & MatchBitsPerSample)
					|| pafd->pwfx->wBitsPerSample == pDlg->m_pWf->wBitsPerSample)
				&& (0 == (pfcs->Flags & MatchSamplingRate)
					|| pafd->pwfx->nSamplesPerSec == pDlg->m_pWf->nSamplesPerSec))
			{
#ifdef _DEBUG
				if (WAVE_FORMAT_MSAUDIO1 == pafd->dwFormatTag
					|| WAVE_FORMAT_MSAUDIO1+1 == pafd->dwFormatTag)
				{
					WAVEFORMATEX * pWfx = pafd->pwfx;
					DWORD * pExt = (DWORD *) (pWfx + 1);
					TRACE("Format: %d, BytesPerSec: %d, BlockAlign: %d, cbSize: %d\n",
						pWfx->wFormatTag, pWfx->nAvgBytesPerSec,
						pWfx->nBlockAlign, pWfx->cbSize);
					TRACE("FormatExtension: %08X, %08X, %08X\n", pExt[0], pExt[1], pExt[2]);
				}
#endif
				int nIndex = pDlg->m_Formats.GetSize();
				pDlg->m_Formats.SetSize(nIndex+1);
				pDlg->m_Formats[nIndex].SetData(pafd->pwfx, pafd->szFormat);
			}
		}
	}
	return TRUE;
}

void CWaveSoapFileSaveDialog::FillFormatArray(int SelFormat, int Flags)
{
	m_Formats.RemoveAll();
	if (-1 == SelFormat || SelFormat >= m_FormatTags.GetSize())
	{
		return;
	}

	DWORD dwFormatTag = m_FormatTags[SelFormat].dwTag;
	WAVEFORMATEX * pwfx = (WAVEFORMATEX *)new char[0xFFFF];
	ACMFORMATDETAILS afd;

	pwfx->cbSize = 0;
	pwfx->wFormatTag = WAVE_FORMAT_PCM;
	pwfx->nChannels = m_pWf->nChannels;
	pwfx->nSamplesPerSec = m_pWf->nSamplesPerSec;
	pwfx->wBitsPerSample = 16;
	pwfx->nBlockAlign = m_pWf->nChannels * 2;
	pwfx->nAvgBytesPerSec = pwfx->nSamplesPerSec * pwfx->nBlockAlign;
	memset( & afd, 0, sizeof afd);
	afd.cbStruct = sizeof afd;
	afd.cbwfx = 0xFFFF;
	afd.dwFormatTag = dwFormatTag;
	afd.pwfx = pwfx;
	m_CurrentEnumeratedTag = dwFormatTag;

	HACMDRIVER had = NULL;
	if (MMSYSERR_NOERROR == acmDriverOpen(&had, m_FormatTags[SelFormat].m_hadid, 0))
	{
		DWORD flags = 0;
		if (dwFormatTag != WAVE_FORMAT_PCM
			&& (Flags & MatchCompatibleFormats))
		{
			flags = ACM_FORMATENUMF_CONVERT;
		}
		FormatEnumCallbackStruct fcs;
		fcs.pDlg = this;
		fcs.Flags = Flags;
		int res = acmFormatEnum(had, & afd, FormatEnumCallback, DWORD( & fcs), flags);
		TRACE("acmFormatEnum returned %x\n", res);
		acmDriverClose(had, 0);
	}
	delete [] (char*)pwfx;
}

void CWaveSoapFileSaveDialog::FillFormatCombo(int SelFormat, int Flags)
{
	FillFormatArray(SelFormat, Flags);

	if (m_AttributesCombo.m_hWnd == NULL
		&& ! m_AttributesCombo.SubclassDlgItem(IDC_COMBO_ATTRIBUTES, this))
	{
		return;
	}

	m_AttributesCombo.ResetContent();
	DWORD dwFormatTag = m_FormatTags[SelFormat].dwTag;
	int i;

	for (i = 0; i < m_Formats.GetSize(); i++)
	{
		m_AttributesCombo.AddString(m_Formats[i].Name);
	}

	int sel = -1;
	int BestMatch = 0;
	for (i = 0; i < m_Formats.GetSize(); i++)
	{
		WAVEFORMATEX * pwf = m_Formats[i].pWf;
		if (dwFormatTag == m_pWf->wFormatTag)
		{
			// exact match found
			if (0 == memcmp(pwf, m_pWf, m_pWf->cbSize + sizeof (WAVEFORMATEX)))
			{
				sel = i;
				break;
			}
		}
		// select the best match
		// Sample rate must match, then number of channels might match
		// If original format is non PCM and the queried format is the same format
		int match = 0;
		if (pwf->nSamplesPerSec == m_pWf->nSamplesPerSec)
		{
			match += 8;
		}
		if (pwf->nChannels == m_pWf->nChannels)
		{
			match += 4;
		}
		if (pwf->nAvgBytesPerSec == m_pWf->nAvgBytesPerSec)
		{
			match += 1;
		}

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
				|| pwf->wBitsPerSample >= m_Formats[sel].pWf->wBitsPerSample)
			{
				sel = i;
			}
		}
		else
		{
			if (-1 == sel
				|| pwf->nAvgBytesPerSec >= m_Formats[sel].pWf->nAvgBytesPerSec)
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
					m_pWf->nSamplesPerSec,
					m_pWf->nSamplesPerSec * ch * 2,  // avg bytes per sec
					ch * 2,  // block align
					16, // wBitsPerSample
					0
				};
				ACMFORMATDETAILS afd;
				memset (& afd, 0, sizeof afd);
				afd.cbStruct = sizeof afd;
				afd.pwfx = & wf;
				afd.cbwfx = sizeof (WAVEFORMATEX);
				afd.dwFormatTag = WAVE_FORMAT_PCM;
				acmFormatDetails(NULL, & afd, ACM_FORMATDETAILSF_FORMAT);
				m_Formats.InsertAt(0, SaveFormat());
				m_Formats[0].SetData( & wf, afd.szFormat);

				m_AttributesCombo.InsertString(0, afd.szFormat);
			}
			if (m_pWf->nChannels == m_Formats[0].pWf->nChannels)
			{
				sel = 0;
			}
			else
			{
				sel = 1;
			}
		}
	}
	m_SelectedFormat = sel;
	m_AttributesCombo.SetCurSel(sel);
}

void CWaveSoapFileSaveDialog::OnCompatibleFormatsClicked()
{
	m_bCompatibleFormatsOnly = ((CButton*)GetDlgItem(IDC_CHECK_COMPATIBLE_FORMATS))->GetCheck();
	if (SoundFileWav == m_FileType)
	{
		FillFormatTagCombo();
		FillFormatCombo(m_FormatCombo.GetCurSel());
	}
}

void CWaveSoapFileSaveDialog::OnComboFormatsChange()
{
	int sel = m_FormatCombo.GetCurSel();
	switch (m_FileType)
	{
	case SoundFileWav:
		// WAV file
		m_SelectedTag = m_FormatTags[sel].dwTag;
		FillFormatCombo(sel);
		break;
	case SoundFileMp3:
		// MP3 file
		FillMp3FormatArray();
		break;
	case SoundFileWma:
		// WMA file
		FillFormatCombo(sel,
						MatchNumChannels | MatchSamplingRate);
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
	m_SelectedTag = m_pWf->wFormatTag;
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
	return;
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

		FillFormatTagCombo();
		FillFormatCombo(m_FormatCombo.GetCurSel());
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
		FillMp3EncoderArray();
		break;
	case SoundFileWma:
		// WMA file
		// remove Format: combo
		ShowDlgItem(IDC_STATIC_FORMAT, SW_SHOWNOACTIVATE);
		ShowDlgItem(IDC_COMBO_FORMAT, SW_SHOWNOACTIVATE);
		ShowDlgItem(IDC_COMBO_ATTRIBUTES, SW_SHOWNOACTIVATE);
		ShowDlgItem(IDC_STATIC_ATTRIBUTES, SW_SHOWNOACTIVATE);
		ShowDlgItem(IDC_CHECK_COMPATIBLE_FORMATS, SW_HIDE);
		ShowDlgItem(IDC_STATIC_COMMENTS, SW_HIDE);
		ShowDlgItem(IDC_EDIT_COMMENT, SW_HIDE);
		// fill profiles combo
		FillWmaFormatArray();
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
	if (1 == m_pWf->nChannels)
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

void CWaveSoapFileSaveDialog::FillWmaFormatArray()
{
	// if WmaOnly, only tags 352 and 353 are used
	// WAVE_FORMAT_MSAUDIO1 and WAVE_FORMAT_MSAUDIO1+1
	int formats[2] = { WAVE_FORMAT_MSAUDIO1, WAVE_FORMAT_MSAUDIO1 + 1};
	// enum VMA V1 and V2 formats
	// V1 format: 352
	// V2 format: 353
	// filter the formats by tag, sampling rate, number of channels
	// fill format tag array with V1 and V2 formats
	FillFormatTagCombo(formats, 2);
	FillFormatCombo(m_FormatCombo.GetCurSel(), MatchNumChannels | MatchSamplingRate);
}

void CWaveSoapFileSaveDialog::FillMp3FormatArray()
{
	if (0 == m_NumOfMp3Encoders)
	{
		return;
	}
	int EncoderType = m_Mp3Encoders[m_FormatCombo.GetCurSel()];
	BOOL bSaveCompFormat;
	DWORD SaveSelectedTag;
	int nSel;
	switch (EncoderType)
	{
	case Mp3EncoderLameencoder:
		FillLameEncoderFormats();
		break;

	case Mp3EncoderAcm:
		//FillFormatTagArray();
		SaveSelectedTag = m_SelectedTag;
		m_SelectedTag = WAVE_FORMAT_MPEGLAYER3;
		bSaveCompFormat = m_bCompatibleFormatsOnly;
		m_bCompatibleFormatsOnly = true;
		// Find MP3 encode index
		for (nSel = 0; nSel < m_FormatTags.GetSize(); nSel++)
		{
			if (WAVE_FORMAT_MPEGLAYER3 == m_FormatTags[nSel].dwTag)
			{
				FillFormatCombo(nSel);
				break;
			}
		}
		m_bCompatibleFormatsOnly = bSaveCompFormat;
		m_SelectedTag = SaveSelectedTag;
		break;
	case Mp3EncoderBlade:
		break;
	}
}

void CWaveSoapFileSaveDialog::FillMp3EncoderArray()
{
	// check if LAME encoder is available
	m_FormatCombo.ResetContent();
	m_NumOfMp3Encoders = 0;

	BladeMp3Encoder Mp3Enc;
	if (Mp3Enc.Open())
	{
		BE_VERSION ver;
		memset( & ver, 0, sizeof ver);
		Mp3Enc.GetVersion( & ver);

		SYSTEMTIME time;
		memset( & time, 0, sizeof time);
		time.wYear = ver.wYear;
		time.wDay = ver.byDay;
		time.wMonth = ver.byMonth;

		int const TimeBufSize = 256;
		TCHAR str[TimeBufSize] = {0};

		GetDateFormat(LOCALE_USER_DEFAULT, DATE_SHORTDATE, & time, NULL, str, TimeBufSize - 1);

		CString s;
		s.Format("LameEnc DLL Version %d.%02d, (%s) Engine %d.%02d",
				ver.byDLLMajorVersion, ver.byDLLMinorVersion,
				str, ver.byMajorVersion, ver.byMinorVersion);
		Mp3Enc.Close();
		m_FormatCombo.AddString(s);
		m_Mp3Encoders[m_NumOfMp3Encoders] = Mp3EncoderLameencoder;
		m_NumOfMp3Encoders++;
	}

	// check if MP3 ACM encoder presents
	int Mp3Tag = WAVE_FORMAT_MPEGLAYER3;
	FillFormatTagArray( & Mp3Tag, 1);
	for (int i = 0; i < m_FormatTags.GetSize(); i++)
	{
		if (WAVE_FORMAT_MPEGLAYER3 == m_FormatTags[i].dwTag)
		{
			m_FormatCombo.AddString(m_FormatTags[i].Name);
			m_Mp3Encoders[m_NumOfMp3Encoders] = Mp3EncoderAcm;
			m_NumOfMp3Encoders++;
			break;
		}
	}
	m_FormatCombo.SetCurSel(m_SelectedMp3Encoder);
	FillMp3FormatArray();
}
