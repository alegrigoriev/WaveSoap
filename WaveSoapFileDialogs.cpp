// Copyright Alexander Grigoriev, 1997-2002, All Rights Reserved
#include "StdAfx.h"
#include "WaveSoapFront.h"
#include "WaveSoapFrontDoc.h"
#include "WaveSoapFileDialogs.h"
#include "ShelLink.h"
#include "OperationDialogs2.h"
#include <Dlgs.h>
#include "BladeMP3EncDLL.h"
#include "CoInitHelper.h"
#include "WmaFile.h"

BOOL AFXAPI AfxComparePath(LPCTSTR lpszPath1, LPCTSTR lpszPath2);

CWaveSoapFileOpenDialog::CWaveSoapFileOpenDialog(BOOL bOpenFileDialog, // TRUE for FileOpen, FALSE for FileSaveAs
												LPCTSTR lpszDefExt,
												LPCTSTR lpszFileName,
												DWORD dwFlags,
												LPCTSTR lpszFilter,
												CWnd* pParentWnd)
	: BaseClass(bOpenFileDialog, lpszDefExt, lpszFileName, dwFlags,
				lpszFilter, pParentWnd),
	m_bReadOnly(false),
	m_MinWmaFilter(0),
	m_MaxWmaFilter(0),
	m_PrevFilter(~0u),
	m_bDirectMode(false)
{
#if _WIN32_WINNT < _WIN32_WINNT_WIN6
	m_ofn.lpTemplateName = MAKEINTRESOURCE(IDD_DIALOG_OPEN_TEMPLATE_V5);
	static ResizableDlgItem const ItemsV5[] =
	{
		{ IDC_STATIC_GROUPBOX, ExpandRight},
		{ IDC_COMBO_RECENT, ExpandRight},
		{ IDC_EDIT_FILE_COMMENTS, ExpandRight},
		{ IDC_BUTTON_MORE, MoveRight},
		{ IDHELP, MoveRight},
	};
	m_pResizeItems = ItemsV5;
	m_ResizeItemsCount = countof(ItemsV5);
#else
	CString ReadOnlyButton(MAKEINTRESOURCE(IDS_RADIO_OPEN_AS_READ_ONLY));
	AddCheckButton(IDC_CHECK_READONLY, ReadOnlyButton, m_bReadOnly);
	CString DirectButton(MAKEINTRESOURCE(IDS_RADIO_OPEN_AS_READ_ONLY));
	AddCheckButton(IDC_CHECK_DIRECT, DirectButton, m_bDirectMode);
#endif
}

BEGIN_MESSAGE_MAP(CWaveSoapFileOpenDialog, BaseClass)
	//{{AFX_MSG_MAP(CWaveSoapFileOpenDialog)
	ON_BN_CLICKED(IDC_CHECK_READONLY, OnCheckReadOnly)
	ON_BN_CLICKED(IDC_CHECK_DIRECT, OnCheckDirectMode)
	ON_WM_SIZE()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

void CWaveSoapFileOpenDialog::ShowWmaFileInfo(CDirectFile & File)
{
	if ( ! GetApp()->CanOpenWindowsMedia())
	{
		ClearFileInfoDisplay();
		return;
	}

	CoInitHelper CoInit(COINIT_APARTMENTTHREADED);

	CWmaDecoderSync WmaFile;
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
#if _WIN32_WINNT < _WIN32_WINNT_WIN6
		if (WmaFile.GetSrcFormat().FormatTag() == WAVE_FORMAT_MPEGLAYER3)
		{
			SetDlgItemText(IDC_STATIC_FILE_TYPE, LoadCString(IDS_MP3_FILE_TYPE));
			SetDlgItemText(IDC_STATIC_FILE_FORMAT, LoadCString(IDS_MP3_FILE_FORMAT));
		}
		else
		{
			SetDlgItemText(IDC_STATIC_FILE_TYPE, LoadCString(IDS_WMA_FILE_TYPE));
			SetDlgItemText(IDC_STATIC_FILE_FORMAT, LoadCString(IDS_WMA_FILE_FORMAT));
		}
#endif
		// length
		CString s;
		s.Format(_T("%s (%s)"),
				LPCTSTR(TimeToHhMmSs(MulDiv(WmaFile.GetTotalSamples(), 1000,
											WmaFile.GetSrcFormat().SampleRate()))),
				LPCTSTR(LtoaCS(WmaFile.GetTotalSamples())));
		SetDlgItemText(IDC_STATIC_FILE_LENGTH, s);
		// num of channels, bitrate, sampling rate

		CString ch;
		if (1 == WmaFile.GetSrcFormat().NumChannels())
		{
			ch.LoadString(IDS_MONO);
		}
		else
		{
			ch.LoadString(IDS_STEREO);
		}

		s.Format(IDS_MP3_FORMAT_STR, LPCTSTR(LtoaCS(WmaFile.GetBitRate())),
				LPCTSTR(LtoaCS(WmaFile.GetSrcFormat().SampleRate())),
				LPCTSTR(ch));
		SetDlgItemText(IDC_STATIC_ATTRIBUTES, s);
	}
	else
	{
		ClearFileInfoDisplay();
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
	return BaseClass::OnFileNameOK();
}

void CWaveSoapFileOpenDialog::OnInitDone()
{
	BaseClass::OnInitDone();
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
	if (GetFileDlg()->SendMessage(CDM_GETFILEPATH, (WPARAM)m_ofn.nMaxFile,
								(LPARAM)pBuf) < 0)
	{
		ClearFileInfoDisplay();
		return;
	}
	TRACE(_T("CWaveSoapFileOpenDialog::OnFileNameChange=%s\n"), LPCTSTR(sName));
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
		m_WaveFile.LoadMetadata();
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

			SetDlgItemText(IDC_STATIC_FILE_TYPE, LoadCString(IDS_WAV_FILE_TYPE));

			CString s;
			CString s2;

			// get format name
			SetDlgItemText(IDC_STATIC_ATTRIBUTES,
							CAudioCompressionManager::GetFormatName(NULL, pWf));

			s2.Format(_T("%s (%s)"),
					LPCTSTR(TimeToHhMmSs(MulDiv(nSamples, 1000, nSamplingRate))),
					LPCTSTR(LtoaCS(nSamples)));
			SetDlgItemText(IDC_STATIC_FILE_LENGTH, s2);

			ACMFORMATTAGDETAILS aft;
			memzero(aft);
			aft.cbStruct = sizeof aft;
			aft.dwFormatTag = pWf->wFormatTag;

			if (MMSYSERR_NOERROR == acmFormatTagDetails(NULL, & aft,
														ACM_FORMATTAGDETAILSF_FORMATTAG))
			{
				SetDlgItemText(IDC_STATIC_FILE_FORMAT, aft.szFormatTag);
			}
			else
			{
				SetDlgItemText(IDC_STATIC_FILE_FORMAT, LoadCString(IDS_UNKNOWN_FILE_TYPE_FORMAT));
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
	SetDlgItemText(IDC_STATIC_FILE_TYPE, LoadCString(IDS_UNKNOWN_FILE_TYPE_FORMAT));
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

////////////////////////////////////////////////////////
////////////////////////
CFileSaveUiSupport::CFileSaveUiSupport(CWaveFormat const & Wf)
	: m_Wf(Wf)
	, m_Acm(Wf)
	, m_SelectedTag(Wf)
	, m_SelectedFormat(~0U)
	, m_SelectedMp3Encoder(0)
	, m_SelectedMp3Bitrate(LameEncBitrate128 * 1000)
	, m_SelectedWmaBitrate(128000)
	, m_bCompatibleFormatsOnly(TRUE)
	, m_FileType(SaveFile_NonWavFile)
	, m_SelectedRawFormat(RawSoundFilePcm16Lsb)
	, m_NumOfFileTypes(0)
{
	FileParameters * pRawParams = PersistentFileParameters::GetData();
	switch (pRawParams->RawFileFormat.wFormatTag)
	{
	case WAVE_FORMAT_PCM:
	default:
		if (8 == pRawParams->RawFileFormat.wBitsPerSample)
		{
			m_SelectedRawFormat = RawSoundFilePcm8;
		}
		else if (pRawParams->RawFileBigEnded)
		{
			m_SelectedRawFormat = RawSoundFilePcm16Msb;
		}
		else
		{
			m_SelectedRawFormat = RawSoundFilePcm16Lsb;
		}
		break;

	case WAVE_FORMAT_ALAW:
		m_SelectedRawFormat = RawSoundFileALaw8;
		break;

	case WAVE_FORMAT_MULAW:
		m_SelectedRawFormat = RawSoundFileULaw8;
		break;
	}

	m_Profile.AddItem(_T("Settings"), _T("MP3Bitrate"), m_SelectedMp3Bitrate, 128000, 0, 320000);
	m_Profile.AddItem(_T("Settings"), _T("WmaBitrate"), m_SelectedWmaBitrate, 128000, 0, 160000);
	m_Profile.AddItem(_T("Settings"), _T("MP3Encoder"), m_SelectedMp3Encoder, 0, 0, 4);
	m_Profile.AddItem(_T("Settings"), _T("FormatTag"), m_SelectedTag, m_SelectedTag);
	m_Profile.AddBoolItem(_T("Settings"), _T("ShowCompatibleFormatsOnly"), m_bCompatibleFormatsOnly, TRUE);
}

/////////////////////////////////////////////////////////////////////////
//////// CWaveSoapFileSaveDialog
CWaveSoapFileSaveDialog::CWaveSoapFileSaveDialog(BOOL bOpenFileDialog, // TRUE for FileOpen, FALSE for FileSaveAs
												CWaveFormat const & Wf,
												CWaveSoapFrontDoc * pDoc,
												LPCTSTR lpszDefExt,
												LPCTSTR lpszFileName,
												DWORD dwFlags,
												LPCTSTR lpszFilter,
												CWnd* pParentWnd)

	: BaseClass(bOpenFileDialog, lpszDefExt, lpszFileName, dwFlags,
				lpszFilter, pParentWnd)
	, CFileSaveUiSupport(Wf)
	, m_pDocument(pDoc)
{
	m_ofn.lpTemplateName = MAKEINTRESOURCE(IDD_DIALOG_SAVE_TEMPLATE_V5);
#if _WIN32_WINNT < _WIN32_WINNT_WIN6
	static ResizableDlgItem const ItemsV5[] =
	{
		{ IDC_COMBO_RECENT, ExpandRight},
		{ IDC_COMBO_FORMAT, ExpandRight},
		{ IDC_COMBO_ATTRIBUTES, ExpandRight},
		{ IDC_COMBO_TITLE, ExpandRight},
		{ IDC_COMBO_ARTIST, ExpandRight},
		{ IDC_COMBO_ALBUM, ExpandRight},
		{ IDC_CHECK_COMPATIBLE_FORMATS, MoveRight},
		{ IDC_STATIC_COMMENTS, MoveRight},
		{ IDC_EDIT_COMMENT, MoveRight},
		{ IDHELP, MoveRight},
	};
	m_pResizeItems = ItemsV5;
	m_ResizeItemsCount = countof(ItemsV5);
#endif
}

CWaveSoapFileSaveDialog::~CWaveSoapFileSaveDialog()
{
}

BEGIN_MESSAGE_MAP(CWaveSoapFileSaveDialog, BaseClass)
	//{{AFX_MSG_MAP(CWaveSoapFileSaveDialog)
	ON_BN_CLICKED(IDC_CHECK_COMPATIBLE_FORMATS, OnCompatibleFormatsClicked)
	ON_CBN_SELCHANGE(IDC_COMBO_FORMAT, OnComboFormatsChange)
	ON_CBN_SELCHANGE(IDC_COMBO_ATTRIBUTES, OnComboAttributesChange)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

BOOL CWaveSoapFileSaveDialog::OnFileNameOK()
{
	// if the file type is different from the selected type,
	// set the type and let the user select the parameters
	CString Name = GetPathName();
	unsigned type = GetFileTypeForName(Name);

	if (type != m_FileType)
	{
		SetFileType(type);
		return 1;   // don't close
	}
	BaseClass::OnFileNameOK();
	// save format selection
	m_SelectedFormat = m_AttributesCombo.GetCurSel();

	CFileSaveUiSupport::m_Profile.FlushAll();

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

int CFileSaveUiSupport::GetFileTypeFlags() const
{
	if (SaveFile_RawFile == m_FileType
		&& m_SelectedRawFormat == RawSoundFilePcm16Msb)
	{
		return SaveFile_RawFile | SaveRawFileMsbFirst;
	}
	return m_FileType;
}

WAVEFORMATEX * CFileSaveUiSupport::GetWaveFormat()
{
	if (SaveFile_RawFile == m_FileType)
	{
		WORD FormatTag = WAVE_FORMAT_PCM;
		WORD BitsPerSample = 16;
		switch (m_SelectedRawFormat)
		{
		case RawSoundFilePcm8:
			BitsPerSample = 8;
			break;
		case RawSoundFileALaw8:
			FormatTag = WAVE_FORMAT_ALAW;
			BitsPerSample = 8;
			break;
		case RawSoundFileULaw8:
			FormatTag = WAVE_FORMAT_MULAW;
			BitsPerSample = 8;
			break;

		case RawSoundFilePcm16Msb:
		case RawSoundFilePcm16Lsb:
		default:
			break;
		}
		m_Wf.InitFormat(FormatTag, m_Wf.SampleRate(), m_Wf.NumChannels(), BitsPerSample);
		return m_Wf;
	}
	// all other formats
	if (m_SelectedFormat >= m_Acm.m_Formats.size())
	{
		return NULL;
	}
	return m_Acm.m_Formats[m_SelectedFormat].Wf;
}

WaveFormatTagEx const CFileSaveUiSupport::ExcludeFormats[] =
{
	WAVE_FORMAT_MPEGLAYER3,
	WAVE_FORMAT_MSAUDIO1,
	WAVE_FORMAT_MSAUDIO1+1,
	WORD(0),
};

void CFileSaveUiSupport::FillFormatTagCombo(WaveFormatTagEx const ListOfTags[], int NumTags, DWORD Flags)
{
	if (m_bCompatibleFormatsOnly)
	{
		Flags |= WaveFormatMatchCompatibleFormats;
	}

	if (NULL == ListOfTags)
	{
		NumTags = 0;
	}
	else if (-1 == NumTags)
	{
		// tags list is zero-terminated
		for (NumTags = 0; ListOfTags[NumTags].Tag != 0; NumTags++)
		{
		}
	}

	m_Acm.FillFormatTagArray(m_Wf, ListOfTags, NumTags, Flags);

	if (m_FormatTagCombo.m_hWnd == NULL)
	{
		return;
	}

	m_FormatTagCombo.ResetContent();

	int sel = -1;
	for (unsigned i = 0; i < m_Acm.m_FormatTags.size(); i++)
	{
		if (m_Acm.m_FormatTags[i].Tag == m_SelectedTag)
		{
			sel = i;
		}
		m_FormatTagCombo.AddString(m_Acm.m_FormatTags[i].Name);
	}

	if (-1 == sel
		&& m_Acm.m_FormatTags.size() > 0)
	{
		sel = 0;
		m_SelectedTag = m_Acm.m_FormatTags[0].Tag;
	}
	m_FormatTagCombo.SetCurSel(sel);
}

int CFileSaveUiSupport::FillFormatCombo(unsigned SelFormat, int Flags)
{
	if (m_AttributesCombo.m_hWnd == NULL)
	{
		return 0;
	}

	m_AttributesCombo.ResetContent();

	if ( ! m_Acm.FillFormatArray(SelFormat, Flags))
	{
		return 0;
	}

	return m_Acm.FillFormatsCombo( & m_AttributesCombo, m_Wf,
									m_SelectedTag, m_SelectedBitrate);
}

void CFileSaveUiSupport::OnCompatibleFormatsClicked()
{
	switch (m_FileType)
	{
	case SaveFile_WavFile:
		FillFormatTagCombo(ExcludeFormats, -1, WaveFormatExcludeFormats);
		m_SelectedFormat = FillFormatCombo(m_FormatTagCombo.GetCurSel());
		break;

	case SaveFile_Mp3File:
		m_SelectedFormat = FillFormatCombo(m_FormatTagCombo.GetCurSel());
		break;

	case SaveFile_WmaFile:
		m_SelectedFormat = FillFormatCombo(0);
		break;
	}
}

void CFileSaveUiSupport::OnComboAttributesChange()
{
	unsigned sel = m_AttributesCombo.GetCurSel();

	switch (m_FileType)
	{
	case SaveFile_WavFile:
		// WAV file
		m_SelectedFormat = sel;
		break;
	case SaveFile_Mp3File:
		// MP3 file
		if (sel < m_Acm.m_Formats.size())
		{
			m_SelectedMp3Bitrate = m_Acm.m_Formats[sel].Wf.BytesPerSec() * 8;
			m_SelectedBitrate = m_SelectedMp3Bitrate;
		}
		break;
	case SaveFile_WmaFile:
		// WMA file
		if (sel < m_Acm.m_Formats.size())
		{
			m_SelectedWmaBitrate = m_Acm.m_Formats[sel].Wf.BytesPerSec() * 8;
			m_SelectedBitrate = m_SelectedWmaBitrate;
		}
		break;
	case SaveFile_RawFile:
		// RAW file
		break;
		//case SaveFile_AviFile:
		// RAW file
		break;
	}
}

void CFileSaveUiSupport::OnComboFormatsChange()
{
	int sel = m_FormatTagCombo.GetCurSel();
	switch (m_FileType)
	{
	case SaveFile_WavFile:
		// WAV file
		m_SelectedTag = m_Acm.m_FormatTags[sel].Tag;
		m_SelectedFormat = FillFormatCombo(sel);
		break;
	case SaveFile_Mp3File:
		// MP3 file
		m_SelectedTag = m_Acm.m_FormatTags[sel].Tag;
		m_SelectedMp3Encoder = sel;
		m_SelectedFormat = FillFormatCombo(sel);
		break;
	case SaveFile_WmaFile:
		// WMA file
		// never called!
		//FillFormatCombo(sel, MatchNumChannels | MatchSamplingRate);
		break;
	case SaveFile_RawFile:
		// RAW file
		m_SelectedRawFormat = sel;
		break;
		//case SoundFileAvi:
		// RAW file
		break;
	}
}

void CFileSaveUiSupport::FillFileTypes(CDocManager * pDocManager)
{
	// do for all doc template

	unsigned nFilterIndex = 0;

	for (POSITION pos = pDocManager->GetFirstDocTemplatePosition();
		nFilterIndex < countof (m_DefExt) && pos != NULL; )
	{
		CDocTemplate* pTemplate = pDocManager->GetNextDocTemplate(pos);

		CWaveSoapDocTemplate * pWaveTemplate =
			dynamic_cast<CWaveSoapDocTemplate *>(pTemplate);

		if (NULL != pWaveTemplate)
		{
			if (DocumentFlagOpenOnly & pWaveTemplate->GetDocumentTypeFlags())
			{
				continue;
			}

			m_TemplateFlags[nFilterIndex] = pWaveTemplate->GetDocumentTypeFlags();
		}
		else
		{
			m_TemplateFlags[nFilterIndex] = 0;
		}

		pTemplate->GetDocString(m_DefExt[nFilterIndex], CDocTemplate::filterExt);
		pTemplate->GetDocString(m_FileTypeStrings[nFilterIndex], CDocTemplate::filterName);

		nFilterIndex++;
	}

	m_NumOfFileTypes = nFilterIndex;
}

void CWaveSoapFileSaveDialog::AddAllTypeFilters(CDocManager * pDocManager)
{
	// do for all doc template

	unsigned nFilterIndex = 0;

	for (POSITION pos = pDocManager->GetFirstDocTemplatePosition();
		nFilterIndex < countof (m_DefExt) && pos != NULL; )
	{
		CDocTemplate* pTemplate = pDocManager->GetNextDocTemplate(pos);

		CWaveSoapDocTemplate * pWaveTemplate =
			dynamic_cast<CWaveSoapDocTemplate *>(pTemplate);

		if (NULL != pWaveTemplate)
		{
			if (DocumentFlagOpenOnly & pWaveTemplate->GetDocumentTypeFlags())
			{
				continue;
			}

			m_TemplateFlags[nFilterIndex] = pWaveTemplate->GetDocumentTypeFlags();
		}
		else
		{
			m_TemplateFlags[nFilterIndex] = 0;
		}

		// BUGBUG: What If the file was opened through "All WMA files" template?
		CString * pDefaultExtCString = NULL;
		if (pTemplate == m_pDocument->GetDocTemplate())
		{
			m_ofn.nFilterIndex = nFilterIndex + 1;
			pDefaultExtCString = & m_strDefaultExt;
		}

		_AfxAppendFilterSuffix(m_strFilter, m_ofn, pTemplate,
								pDefaultExtCString);

		pTemplate->GetDocString(m_DefExt[nFilterIndex], CDocTemplate::filterExt);

		nFilterIndex++;
	}

	m_NumOfFileTypes = nFilterIndex;
	// append the "*.*" all files filter
	CString allFilter;
	VERIFY(allFilter.LoadString(AFX_IDS_ALLFILTER));
	m_strFilter += allFilter;
	m_strFilter += (TCHAR)'\0';   // next string please
	m_strFilter += _T("*.*");
	m_strFilter += (TCHAR)'\0';   // last string

	m_ofn.nMaxCustFilter++;
	m_ofn.lpstrFilter = m_strFilter;

}

void CWaveSoapFileSaveDialog::OnCompatibleFormatsClicked()
{
	m_bCompatibleFormatsOnly = ((CButton*)GetDlgItem(IDC_CHECK_COMPATIBLE_FORMATS))->GetCheck();
	CFileSaveUiSupport::OnCompatibleFormatsClicked();
}

void CWaveSoapFileSaveDialog::OnComboFormatsChange()
{
	CFileSaveUiSupport::OnComboFormatsChange();
}

void CWaveSoapFileSaveDialog::OnComboAttributesChange()
{
	CFileSaveUiSupport::OnComboAttributesChange();
}

void CWaveSoapFileSaveDialog::OnInitDone()
{
	//fill format combo box.

	if (m_FormatTagCombo.m_hWnd == NULL)
	{
		m_FormatTagCombo.SubclassDlgItem(IDC_COMBO_FORMAT, this);
	}
	if (m_AttributesCombo.m_hWnd == NULL)
	{
		m_AttributesCombo.SubclassDlgItem(IDC_COMBO_ATTRIBUTES, this);
	}

	if (m_SelectedTag.Tag == WAVE_FORMAT_MSAUDIO1
		|| m_SelectedTag.Tag == WAVE_FORMAT_MSAUDIO1 + 1)
	{
		m_SelectedWmaBitrate = m_Wf.BytesPerSec() * 8;
		m_SelectedBitrate = m_SelectedWmaBitrate;
	}
	else if (m_SelectedTag.Tag == WAVE_FORMAT_MPEGLAYER3
			|| m_SelectedTag == BladeMp3Encoder::GetTag())
	{
		m_SelectedMp3Bitrate = m_Wf.BytesPerSec() * 8;
		m_SelectedBitrate = m_SelectedMp3Bitrate;
	}
	else
	{
		m_SelectedBitrate = 128000;
	}

	CheckDlgButton(IDC_CHECK_COMPATIBLE_FORMATS, m_bCompatibleFormatsOnly);

	if (m_ofn.nFilterIndex <= m_NumOfFileTypes
		&& 0 != m_ofn.nFilterIndex)
	{
		SetFileType(m_TemplateFlags[m_ofn.nFilterIndex - 1] & (SaveFile_NonWavFile & ~ SaveRawFileMsbFirst));
	}
	else
	{
		unsigned type = GetFileTypeForName(m_ofn.lpstrFile);

		for (unsigned i = 0; i < m_NumOfFileTypes; i++)
		{
			if (m_TemplateFlags[i] == type)
			{
				m_ofn.nFilterIndex = i + 1;
				break;
			}
		}
		SetFileType(type);
	}

	BaseClass::OnInitDone();
}

void CWaveSoapFileSaveDialog::OnTypeChange()
{
	TRACE("Current type = %d\n", m_ofn.nFilterIndex);
	// get file name
	CString name;
	// set new default extension
	GetFileDlg()->SendMessage(CDM_SETDEFEXT, 0, LPARAM(LPCTSTR(m_DefExt[m_ofn.nFilterIndex]) + 1));

	CWnd * pTmp = GetFileDlg()->GetDlgItem(edt1);
	if (NULL == pTmp)
	{
		// new style dialog
		pTmp = GetFileDlg()->GetDlgItem(cmb13);
	}
	if (NULL != pTmp)
	{
		pTmp->SetFocus();
		pTmp->GetWindowText(name);
	}
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
				name.Delete(idx, name.GetLength() - idx);
				name += m_DefExt[m_ofn.nFilterIndex - 1];
			}

			GetFileDlg()->SendMessage(CDM_SETCONTROLTEXT, edt1, LPARAM(LPCTSTR(name)));
		}
	}

	if (m_ofn.nFilterIndex <= m_NumOfFileTypes
		&& 0 != m_ofn.nFilterIndex)
	{
		SetFileType(m_TemplateFlags[m_ofn.nFilterIndex - 1] & (SaveFile_NonWavFile & ~ SaveRawFileMsbFirst));
	}
	else
	{
		m_ofn.nFilterIndex = 1;
		SetFileType(SaveFile_WavFile);
	}
}

void CWaveSoapFileSaveDialog::OnFileNameChange()
{
	// if the file type is different from the selected type,
	// set the type and let the user select the parameters
	CString Name = GetPathName();
	unsigned type = GetFileTypeForName(Name);
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

void CWaveSoapFileSaveDialog::SetFileType(ULONG nType)
{
	if (m_FileType == nType)
	{
		return;
	}
	// Wav, Mp3, wma, raw...
	CString s;
	m_FileType = nType;

	switch (nType)
	{
	default:
	case SaveFile_WavFile:
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

		FillFormatTagCombo(ExcludeFormats, -1, WaveFormatExcludeFormats);

		m_SelectedFormat = FillFormatCombo(m_FormatTagCombo.GetCurSel());
		break;  // go on

	case SaveFile_Mp3File:
		// MP3 file
		// replace Format with Encoder: (LAME, Fraunhofer
		s.LoadString(IDS_ENCODER);
		SetDlgItemText(IDC_STATIC_FORMAT, s);
		ShowDlgItem(IDC_STATIC_FORMAT, SW_SHOWNOACTIVATE);
		ShowDlgItem(IDC_COMBO_FORMAT, SW_SHOWNOACTIVATE);
		ShowDlgItem(IDC_COMBO_ATTRIBUTES, SW_SHOWNOACTIVATE);
		ShowDlgItem(IDC_STATIC_ATTRIBUTES, SW_SHOWNOACTIVATE);
		ShowDlgItem(IDC_CHECK_COMPATIBLE_FORMATS, SW_SHOWNOACTIVATE);
		ShowDlgItem(IDC_STATIC_COMMENTS, SW_HIDE);
		ShowDlgItem(IDC_EDIT_COMMENT, SW_HIDE);
		// Hide Comments, show Artist, Genre, Title
		//
		// set tag to MP3
		// fill formats combo (bitrate, etc)
		FillMp3EncoderCombo();
		break;

	case SaveFile_WmaFile:
		// WMA file
		if ( ! GetApp()->CanOpenWindowsMedia())
		{
			int id = AfxMessageBox(IDS_WMA_ENCODER_NOT_AVILABLE, MB_OK | MB_ICONEXCLAMATION | MB_HELP);
			if (IDHELP == id)
			{
				// TODO: show help
			}
			SetFileType(SaveFile_WavFile);
			break;
		}

		// remove Format: combo
		ShowDlgItem(IDC_STATIC_FORMAT, SW_HIDE);
		ShowDlgItem(IDC_COMBO_FORMAT, SW_HIDE);
		ShowDlgItem(IDC_COMBO_ATTRIBUTES, SW_SHOWNOACTIVATE);
		ShowDlgItem(IDC_STATIC_ATTRIBUTES, SW_SHOWNOACTIVATE);
		ShowDlgItem(IDC_CHECK_COMPATIBLE_FORMATS, SW_SHOWNOACTIVATE);
		ShowDlgItem(IDC_STATIC_COMMENTS, SW_HIDE);
		ShowDlgItem(IDC_EDIT_COMMENT, SW_HIDE);
		// fill profiles combo
		FillWmaFormatCombo();
		break;
		//case SaveFile_AviFile:
		// AVI file
		break;
	case SaveFile_RawFile:
		// RAW file
		// show formats: 16 bits LSB first, 16 bits MSB first, a-law, u-law, 8 bits, ascii hex, ascii decimal
		// hide attributes
		s.LoadString(IDS_FORMAT);
		SetDlgItemText(IDC_STATIC_FORMAT, s);

		FillRawFormatsCombo();

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

unsigned CFileSaveUiSupport::GetFileTypeForExt(LPCTSTR lpExt)
{
	unsigned type = SaveFile_RawFile;    // if not found

	if (NULL != lpExt && 0 != *lpExt)
	{
		CString s(lpExt);
		s.TrimRight();
		for (unsigned i = 0; i < m_NumOfFileTypes; i++)
		{
			if (0 == m_DefExt[i].CompareNoCase(lpExt))
			{
				type = m_TemplateFlags[i];
				break;
			}
		}
	}
	return type;
}

unsigned CFileSaveUiSupport::GetFileTypeForName(LPCTSTR FileName)
{
	LPCTSTR ext = _tcsrchr(FileName, '.');
	if (NULL == ext)
	{
		return m_FileType;
	}
	return GetFileTypeForExt(ext);
}

void CFileSaveUiSupport::FillLameEncoderFormats()
{
	m_Acm.FillLameEncoderFormats();
	m_AttributesCombo.ResetContent();
	unsigned sel = 0;

	for (unsigned i = 0; i < m_Acm.m_Formats.size(); i++)
	{
		m_AttributesCombo.AddString(m_Acm.m_Formats[i].Name);
		if (int(m_Acm.m_Formats[i].Wf.BytesPerSec() / 125) == m_SelectedMp3Bitrate)
		{
			sel = i;
		}
	}
	m_AttributesCombo.SetCurSel(sel);
}

void CFileSaveUiSupport::FillRawFormatsCombo()
{
	CString s;
	m_FormatTagCombo.ResetContent();

	s.LoadString(IDS_RAW_16BIT_LSB);
	m_FormatTagCombo.InsertString(RawSoundFilePcm16Lsb, s);

	s.LoadString(IDS_RAW_16BIT_MSB);
	m_FormatTagCombo.InsertString(RawSoundFilePcm16Msb, s);

	s.LoadString(IDS_RAW_8BITS_PCM);
	m_FormatTagCombo.InsertString(RawSoundFilePcm8, s);

	s.LoadString(IDS_RAW_8BITS_ALAW);
	m_FormatTagCombo.InsertString(RawSoundFileALaw8, s);

	s.LoadString(IDS_RAW_8BITS_ULAW);
	m_FormatTagCombo.InsertString(RawSoundFileULaw8, s);

	m_FormatTagCombo.SetCurSel(m_SelectedRawFormat);

}

void CFileSaveUiSupport::FillWmaFormatCombo()
{
	m_Acm.FillWmaFormatTags();
	if (m_Acm.m_FormatTags.empty())
	{
		return;
	}

	m_SelectedTag = m_Acm.m_FormatTags[0].Tag;
	m_SelectedFormat = FillFormatCombo(0);
}

void CFileSaveUiSupport::FillMp3EncoderCombo()
{
	m_FormatTagCombo.ResetContent();
	m_Acm.FillMp3EncoderTags(m_bCompatibleFormatsOnly ?
								WaveFormatMatchCompatibleFormats
							: WaveFormatMatchFormatTag);

	if (m_Acm.m_FormatTags.empty())
	{
		return;
	}

	for (unsigned i = 0; i < m_Acm.m_FormatTags.size(); i++)
	{
		m_FormatTagCombo.AddString(m_Acm.m_FormatTags[i].Name);
	}

	if (m_SelectedMp3Encoder >= m_Acm.m_FormatTags.size())
	{
		m_SelectedMp3Encoder = 0;
	}
	m_FormatTagCombo.SetCurSel(m_SelectedMp3Encoder);
	m_SelectedTag = m_Acm.m_FormatTags[m_SelectedMp3Encoder].Tag;
	FillFormatCombo(m_SelectedMp3Encoder);
}
