// OperationDialogs2.cpp : implementation file
//

#include "stdafx.h"
#include "WaveSoapFront.h"
#include "OperationDialogs2.h"
#include <dbt.h>
#include "FolderDialog.h"
#include "FileDialogWithHistory.h"
#include <afxpriv.h>
#include "WaveSoapFileDialogs.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CInsertSilenceDialog dialog


CInsertSilenceDialog::CInsertSilenceDialog(CWnd* pParent /*=NULL*/)
	: CDialog(CInsertSilenceDialog::IDD, pParent)
{
	m_pWf = NULL;
	//{{AFX_DATA_INIT(CInsertSilenceDialog)
	m_TimeFormatIndex = -1;
	//}}AFX_DATA_INIT
	m_nChannel = -1;
	m_Length = 0;
	m_Start = 0;
}


void CInsertSilenceDialog::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CInsertSilenceDialog)
	DDX_Control(pDX, IDC_SPIN_START, m_SpinStart);
	DDX_Control(pDX, IDC_SPIN_LENGTH, m_SpinLength);
	DDX_Control(pDX, IDC_EDIT_LENGTH, m_eLength);
	DDX_Control(pDX, IDC_COMBO_START, m_eStart);
	DDX_CBIndex(pDX, IDC_COMBO_TIME_FORMAT, m_TimeFormatIndex);
	//}}AFX_DATA_MAP
	if (m_pWf->nChannels > 1)
	{
		DDX_Radio(pDX, IDC_RADIO_CHANNEL, m_nChannel);
	}
	m_eLength.ExchangeData(pDX, m_Length);
	m_eStart.ExchangeData(pDX, m_Start);
}


BEGIN_MESSAGE_MAP(CInsertSilenceDialog, CDialog)
	//{{AFX_MSG_MAP(CInsertSilenceDialog)
	ON_CBN_SELCHANGE(IDC_COMBO_TIME_FORMAT, OnSelchangeComboTimeFormat)
	ON_EN_KILLFOCUS(IDC_EDIT_LENGTH, OnKillfocusEditLength)
	ON_CBN_KILLFOCUS(IDC_COMBO_START, OnKillfocusEditStart)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CInsertSilenceDialog message handlers

void CInsertSilenceDialog::OnSelchangeComboTimeFormat()
{
	int sel = ((CComboBox *)GetDlgItem(IDC_COMBO_TIME_FORMAT))->GetCurSel();
	int Format;
	switch (sel)
	{
	case 0:
		Format = SampleToString_Sample;
		break;
	case 1:
		Format = SampleToString_HhMmSs | TimeToHhMmSs_NeedsMs | TimeToHhMmSs_NeedsHhMm;
		break;
	case 2:
	default:
		Format = SampleToString_Seconds | TimeToHhMmSs_NeedsMs;
		break;
	}
	if (Format == m_TimeFormat)
	{
		return;
	}
	m_TimeFormat = Format;
	m_Length = m_eLength.GetTimeSample();
	m_eLength.SetTimeFormat(Format);
	m_eLength.SetTimeSample(m_Length);
	m_Start = m_eStart.GetTimeSample();
	m_eStart.SetTimeFormat(Format);
	m_eStart.SetTimeSample(m_Start);
}


BOOL CInsertSilenceDialog::OnInitDialog()
{
	m_eLength.SetTimeFormat(m_TimeFormat);
	m_eStart.SetTimeFormat(m_TimeFormat);
	switch (m_TimeFormat & SampleToString_Mask)
	{
	case SampleToString_Sample:
		m_TimeFormatIndex = 0;
		break;
	case SampleToString_HhMmSs:
		m_TimeFormatIndex = 1;
		break;
	case SampleToString_Seconds: default:
		m_TimeFormatIndex = 2;
		break;
	}
	if (NULL != m_pWf)
	{
		m_eLength.SetSamplingRate(m_pWf->nSamplesPerSec);
		m_eStart.SetSamplingRate(m_pWf->nSamplesPerSec);
	}
	CDialog::OnInitDialog();

	m_eStart.AddPosition(IDS_BEGIN_OF_SAMPLE, 0);
	if (m_CaretPosition != 0
		&& m_CaretPosition != m_FileLength)
	{
		m_eStart.AddPosition(IDS_CURSOR, m_CaretPosition);
	}
	m_eStart.AddPosition(IDS_END_OF_SAMPLE, m_FileLength);
	((CComboBox*) & m_eStart)->SetExtendedUI(TRUE);
	// TODO: add markers

	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

void CInsertSilenceDialog::OnKillfocusEditLength()
{
	m_Length = m_eLength.GetTimeSample();
	m_eLength.SetTimeSample(m_Length);
}

void CInsertSilenceDialog::OnKillfocusEditStart()
{
	m_Start = m_eStart.GetTimeSample();
	m_eStart.SetTimeSample(m_Start);
}

/////////////////////////////////////////////////////////////////////////////
// CSilenceOptionDialog dialog


CSilenceOptionDialog::CSilenceOptionDialog(CWnd* pParent /*=NULL*/)
	: CDialog(CSilenceOptionDialog::IDD, pParent)
{
	//{{AFX_DATA_INIT(CSilenceOptionDialog)
	// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


void CSilenceOptionDialog::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CSilenceOptionDialog)
	// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CSilenceOptionDialog, CDialog)
	//{{AFX_MSG_MAP(CSilenceOptionDialog)
	ON_BN_CLICKED(IDC_BUTTON_SILENCE, OnButtonSilence)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CSilenceOptionDialog message handlers

void CSilenceOptionDialog::OnButtonSilence()
{
	EndDialog(IDC_BUTTON_SILENCE);
}
/////////////////////////////////////////////////////////////////
CWmpNotInstalleedWarningDlg::CWmpNotInstalleedWarningDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CWmpNotInstalleedWarningDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CWmpNotInstalleedWarningDlg)
	m_DontShowAnymore = FALSE;
	//}}AFX_DATA_INIT
}


void CWmpNotInstalleedWarningDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CWmpNotInstalleedWarningDlg)
	DDX_Check(pDX, IDC_CHECK_DONT_SHOW_THIS, m_DontShowAnymore);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CWmpNotInstalleedWarningDlg, CDialog)
	//{{AFX_MSG_MAP(CWmpNotInstalleedWarningDlg)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////
CCdGrabbingDialog::CCdGrabbingDialog(CWnd* pParent /*=NULL*/)
	: CResizableDialog(CCdGrabbingDialog::IDD, pParent)
{
	//{{AFX_DATA_INIT(CCdGrabbingDialog)
	m_RadioAssignAttributes = -1;
	m_RadioOpenInEditor = 0;
	m_sSaveFolder = _T("");
	m_sAlbum = _T("");
	m_sArtist = _T("");
	m_RadioFileFormat = -1;
	//}}AFX_DATA_INIT
	m_RadioAssignAttributes = 0;
	m_RadioOpenInEditor = 0;
	m_DiskID = -1;
	m_bNeedUpdateControls = TRUE;
	m_MaxReadSpeed = 0;
	m_CurrentReadSpeed = 0;
	m_DiskReady = CdMediaStateUnknown;
	m_bPlayingAudio = FALSE;
	m_FileTypeFlags = 0;

	m_Profile.AddItem(_T("CdRead"), _T("BaseDirectory"), m_sSaveFolder);
	m_Profile.AddItem(_T("CdRead"), _T("FileType"), m_FileTypeFlags, 0,
					0, SaveFile_WmaFile);
	m_FileTypeFlags &= ~SaveFile_NonWavFile;

	switch (m_FileTypeFlags)
	{
	case OpenDocumentMp3File:
		m_RadioFileFormat = 2;
		break;
	case OpenDocumentWmaFile:
		m_RadioFileFormat = 1;
		break;
	default:
		m_RadioFileFormat = 0;
		break;
	}

	m_Profile.AddItem(_T("CdRead"), _T("Speed"), m_SelectedReadSpeed, 64000000,
					176400, 0x10000000);
	m_Profile.AddItem(_T("CdRead"), _T("DriveLetter"),
					m_PreviousDriveLetter, 'Z', 'A', 'Z');

	m_Profile.AddBoolItem(_T("CdRead"),
						_T("AssignToAllOrSelected"), m_RadioAssignAttributes, FALSE);
	m_Profile.AddBoolItem(_T("CdRead"),
						_T("EditFiles"), m_RadioOpenInEditor, FALSE);

	static ResizableDlgItem const ResizeItems[] =
	{
		{IDC_LIST_TRACKS, ExpandRight | ExpandDown},

		{IDC_RADIO_ASSIGN_ATTRIBUTES, MoveDown},
		{IDC_RADIO_ASSIGN_SELECTED_TRACK, MoveDown},

		{IDC_STATIC_ALBUM, MoveDown},
		{IDC_COMBO_ALBUM, MoveDown | ExpandRight},

		{IDC_STATIC_ARTIST, MoveDown},
		{IDC_COMBO_ARTIST, MoveDown | ExpandRight},

		{IDC_STATIC_STORE_FOLDER, MoveDown},
		{IDC_COMBO_FOLDER, MoveDown | ExpandRight},
		{IDC_BUTTON_BROWSE_SAVE_FOLDER, MoveRight | MoveDown},

		{IDC_STATIC_SAVE_AS, MoveDown},
		{IDC_RADIO_WAV_FORMAT, MoveDown},
		{IDC_RADIO_WMA_FORMAT, MoveDown},
		{IDC_RADIO_MP3_FORMAT, MoveDown},
		{IDC_STATIC_BITRATE, MoveDown},
		{IDC_COMBO_BITRATE, MoveDown},

		{IDC_RADIO_STORE_IMMEDIATELY, MoveDown},
		{IDC_RADIO_LOAD_FOR_EDITING, MoveDown},

		{IDC_BUTTON_PLAY, MoveRight | MoveDown},
		{IDC_BUTTON_STOP, MoveRight | MoveDown},
		{IDC_BUTTON_EJECT, MoveRight | MoveDown},
		{IDC_BUTTON_CDDB, MoveRight | MoveDown},

		{IDHELP, MoveRight | MoveDown},
		{IDOK, MoveRight | MoveDown},
		{IDCANCEL, MoveRight | MoveDown},

	};

	m_pResizeItems = ResizeItems;
	m_pResizeItemsCount = sizeof ResizeItems / sizeof ResizeItems[0];
	// TODO: restore last format used
	m_Wf.InitCdAudioFormat();
	m_Acm.m_Wf.InitCdAudioFormat();
}

CCdGrabbingDialog::~CCdGrabbingDialog()
{
}

void CCdGrabbingDialog::DoDataExchange(CDataExchange* pDX)
{
	CResizableDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CCdGrabbingDialog)
	DDX_Control(pDX, IDC_BUTTON_EJECT, m_EjectButton);
	DDX_Control(pDX, IDC_COMBO_BITRATE, m_ComboBitrate);
	DDX_Control(pDX, IDC_BUTTON_STOP, m_StopButton);
	DDX_Control(pDX, IDC_BUTTON_PLAY, m_PlayButton);
	DDX_Control(pDX, IDC_COMBO_ARTIST, m_eArtist);
	DDX_Control(pDX, IDC_COMBO_ALBUM, m_eAlbum);
	DDX_Control(pDX, IDC_COMBO_FOLDER, m_eSaveFolder);
	DDX_Control(pDX, IDC_COMBO_SPEED, m_SpeedCombo);
	DDX_Control(pDX, IDC_COMBO_DRIVES, m_DrivesCombo);
	DDX_Control(pDX, IDC_LIST_TRACKS, m_lbTracks);
	DDX_Radio(pDX, IDC_RADIO_ASSIGN_ATTRIBUTES, m_RadioAssignAttributes);
	DDX_Radio(pDX, IDC_RADIO_STORE_IMMEDIATELY, m_RadioOpenInEditor);
	DDX_Text(pDX, IDC_COMBO_FOLDER, m_sSaveFolder);
	DDX_Text(pDX, IDC_COMBO_ALBUM, m_sAlbum);
	DDX_Text(pDX, IDC_COMBO_ARTIST, m_sArtist);
	DDX_Radio(pDX, IDC_RADIO_WAV_FORMAT, m_RadioFileFormat);
	//}}AFX_DATA_MAP

	if (pDX->m_bSaveAndValidate)
	{
		unsigned sel = m_ComboBitrate.GetCurSel();
		if (sel < m_Acm.m_Formats.size())
		{
			m_Wf = m_Acm.m_Formats[sel].Wf;
		}
		else
		{
			m_RadioFileFormat = 0;
			m_Wf.InitCdAudioFormat();
		}
		// save album and track names to CDPLAYER.INI
		// get selected tracks
		// check for valid file names
		// get base folder
		// check if the folder exists
		m_sSaveFolder.TrimLeft();
		m_sSaveFolder.TrimRight();

		switch (m_RadioFileFormat)
		{
		case 2:
			m_FileTypeFlags = OpenDocumentMp3File;
			break;
		case 1:
			m_FileTypeFlags = OpenDocumentWmaFile;
			break;
		default:
			m_FileTypeFlags = 0;
			break;
		}

		if ( ! VerifyCreateDirectory(m_sSaveFolder))
		{
			pDX->PrepareEditCtrl(IDC_COMBO_FOLDER);
			pDX->Fail();
		}
		// create valid file names, ask for file replace
		if (! m_sSaveFolder.IsEmpty())
		{
			TCHAR c = m_sSaveFolder[m_sSaveFolder.GetLength() - 1];
			if (c != '\\'
				&& c != '/')
			{
				m_sSaveFolder += '\\';
			}
			AddStringToHistory(m_sSaveFolder, m_FolderHistory,
								sizeof m_FolderHistory / sizeof m_FolderHistory[0], false);
		}

		if (! m_sAlbum.IsEmpty())
		{
			AddStringToHistory(m_sAlbum, m_AlbumHistory,
								sizeof m_AlbumHistory / sizeof m_AlbumHistory[0], true);
		}

		if (! m_sArtist.IsEmpty())
		{
			AddStringToHistory(m_sArtist, m_ArtistHistory,
								sizeof m_ArtistHistory / sizeof m_ArtistHistory[0], true);
		}

		for (int t = 0; t < m_Tracks.size(); t++)
		{
			if ( ! m_Tracks[t].Checked)
			{
				continue;
			}
			CString Name = m_Tracks[t].Track;
			LPTSTR pName = Name.GetBuffer(0);
			while (*pName != 0)
			{
				char c= *pName;
				if ('\\' == c
					|| '"' == c
					|| '?' == c
					|| '*' == c
					|| ':' == c
					|| ';' == c
					|| ',' == c
					|| '#' == c
					|| '&' == c
					|| '%' == c)
				{
					*pName = '_';
				}

				pName++;
			}
			Name.ReleaseBuffer();

			switch (m_FileTypeFlags & OpenDocumentNonWavFile)
			{
			case OpenDocumentMp3File:
				Name += ".mp3";
				break;
			case OpenDocumentWmaFile:
				Name += ".wma";
				break;
			default:
				Name += ".wav";
				break;
			}
			m_Tracks[t].TrackFileName = m_sSaveFolder + Name;

			// check for existing file, ask for replacement!
			SetLastError(0);
			HANDLE hFile = CreateFile(m_Tracks[t].TrackFileName,
									GENERIC_READ | GENERIC_WRITE | DELETE, 0, NULL, OPEN_EXISTING, 0, NULL);
			if (NULL == hFile || INVALID_HANDLE_VALUE == hFile)
			{
				DWORD error = GetLastError();
				int id = IDS_UNKNOWN_FILE_CREATION_ERROR;
				switch (error)
				{
				case ERROR_FILE_NOT_FOUND:
					// see if we can create a new file
					hFile = CreateFile(m_Tracks[t].TrackFileName,
										GENERIC_READ | GENERIC_WRITE | DELETE,
										0, NULL, CREATE_NEW,
										FILE_FLAG_DELETE_ON_CLOSE, NULL);
					if (NULL == hFile || INVALID_HANDLE_VALUE == hFile)
					{
						id = IDS_OVERWRITE_ACCESS_DENIED;
						break;
					}
					else
					{
						CloseHandle(hFile);
					}
					continue;
				case ERROR_ACCESS_DENIED:
				case ERROR_FILE_READ_ONLY:
					id = IDS_OVERWRITE_ACCESS_DENIED;
					break;
				case ERROR_SHARING_VIOLATION:
					id = IDS_OVERWRITE_SHARING_VIOLATION;
					break;
				}
				CString s;
				s.Format(id, LPCTSTR(m_Tracks[t].TrackFileName));
				AfxMessageBox(s, MB_OK | MB_ICONEXCLAMATION);
				pDX->PrepareCtrl(IDC_LIST_TRACKS);
				pDX->Fail();
			}
			else
			{
				CloseHandle(hFile);
				// file already exists
				CString s;
				s.Format(IDS_REPLACEYESNO, LPCTSTR(m_Tracks[t].TrackFileName));
				if (IDYES != AfxMessageBox(s, MB_YESNO | MB_ICONEXCLAMATION))
				{
					pDX->PrepareCtrl(IDC_LIST_TRACKS);
					pDX->Fail();
				}
			}
		}
		m_PreviousDriveLetter = m_DriveLetterSelected;
		m_Profile.UnloadAll();
	}
}

BEGIN_MESSAGE_MAP(CCdGrabbingDialog, CResizableDialog)
	//{{AFX_MSG_MAP(CCdGrabbingDialog)
	ON_WM_SIZE()
	ON_WM_TIMER()
	ON_BN_CLICKED(IDC_BUTTON_MORE, OnButtonMore)
	ON_CBN_SELCHANGE(IDC_COMBO_DRIVES, OnSelchangeComboDrives)
	ON_WM_DESTROY()
	ON_BN_CLICKED(IDC_BUTTON_CDDB, OnButtonCddb)
	ON_CBN_EDITCHANGE(IDC_COMBO_ALBUM, OnChangeEditAlbum)
	ON_CBN_EDITCHANGE(IDC_COMBO_ARTIST, OnChangeEditArtist)
	ON_NOTIFY(NM_CLICK, IDC_LIST_TRACKS, OnClickListTracks)
	ON_NOTIFY(LVN_BEGINLABELEDIT, IDC_LIST_TRACKS, OnBeginlabeleditListTracks)
	ON_NOTIFY(LVN_ENDLABELEDIT, IDC_LIST_TRACKS, OnEndlabeleditListTracks)
	ON_CBN_EDITCHANGE(IDC_COMBO_FOLDER, OnChangeEditFolderOrFile)
	ON_BN_CLICKED(IDC_BUTTON_PLAY, OnButtonPlay)
	ON_BN_CLICKED(IDC_BUTTON_STOP, OnButtonStop)
	ON_CBN_SELCHANGE(IDC_COMBO_SPEED, OnSelchangeComboSpeed)
	ON_NOTIFY(LVN_ITEMCHANGED, IDC_LIST_TRACKS, OnItemchangedListTracks)
	ON_BN_CLICKED(IDC_BUTTON_BROWSE_SAVE_FOLDER, OnButtonBrowseSaveFolder)
	ON_WM_CONTEXTMENU()
	ON_NOTIFY(NM_DBLCLK, IDC_LIST_TRACKS, OnDblclkListTracks)
	ON_BN_CLICKED(IDC_RADIO_WMA_FORMAT, OnRadioWmaFormat)
	ON_BN_CLICKED(IDC_RADIO_MP3_FORMAT, OnRadioMp3Format)
	ON_BN_CLICKED(IDC_RADIO_WAV_FORMAT, OnRadioWavFormat)
	ON_BN_CLICKED(IDC_BUTTON_EJECT, OnButtonEject)
	ON_WM_SYSCOLORCHANGE()
	//}}AFX_MSG_MAP
	ON_WM_DEVICECHANGE()
	ON_COMMAND(IDC_BUTTON_PLAY, OnButtonPlay)
	ON_COMMAND(IDC_BUTTON_STOP, OnButtonStop)
	ON_MESSAGE(WM_KICKIDLE, OnKickIdle)
	ON_UPDATE_COMMAND_UI(IDOK, OnUpdateOk)
	ON_UPDATE_COMMAND_UI(IDC_BUTTON_PLAY, OnUpdatePlay)
	ON_UPDATE_COMMAND_UI(IDC_BUTTON_STOP, OnUpdateStop)
	ON_UPDATE_COMMAND_UI(IDC_BUTTON_EJECT, OnUpdateEject)
	ON_UPDATE_COMMAND_UI(IDC_COMBO_BITRATE, OnUpdateComboBitrate)

END_MESSAGE_MAP()

void CCdGrabbingDialog::FillDriveList(TCHAR SelectDrive)
{
	m_NumberOfDrives = m_CdDrive.FindCdDrives(m_CDDrives);;
	m_DriveLetterSelected = 0;
	m_CDDriveSelected = 0;
	m_DiskReady = CdMediaStateUnknown;

	m_DrivesCombo.ResetContent();

	if (0 != m_NumberOfDrives)
	{
		m_DrivesCombo.EnableWindow(TRUE);
		for (int drive = 0; drive < m_NumberOfDrives; drive++)
		{
			CString s;
			if (SelectDrive == m_CDDrives[drive])
			{
				m_CDDriveSelected = drive;
			}
			s.Format("%c:", m_CDDrives[drive]);

			m_DrivesCombo.AddString(s);
		}
		m_DrivesCombo.SetCurSel(m_CDDriveSelected);
		m_DriveLetterSelected = m_CDDrives[m_CDDriveSelected];
	}
	else
	{
		m_DrivesCombo.EnableWindow(FALSE);
	}

}

LRESULT CCdGrabbingDialog::OnKickIdle(WPARAM, LPARAM)
{
	if (m_bNeedUpdateControls)
	{
		UpdateDialogControls(this, FALSE);
	}
	m_bNeedUpdateControls = FALSE;
	return 0;
}

BOOL CCdGrabbingDialog::OpenDrive(TCHAR letter)
{
	m_DiskReady = CdMediaStateUnknown;
	if ( ! m_CdDrive.Open(letter))
	{
		return FALSE;
	}
	m_CdDrive.DisableMediaChangeDetection();
	return TRUE;
}

void CCdGrabbingDialog::FillTrackList(TCHAR letter)
{
	memzero(m_toc);
	m_Tracks.clear();
	CString s;
	if (0 == m_NumberOfDrives)
	{
		if (CdMediaStateNoDrives == m_DiskReady)
		{
			return;
		}
		m_lbTracks.DeleteAllItems();
		s.LoadString(IDS_NO_CD_DRIVES);
		m_lbTracks.InsertItem(0, s);

		m_DiskReady = CdMediaStateNoDrives;
		return;
	}

	if ( ! OpenDrive(letter))
	{
		TRACE("Couldn't open CD,error=%d\n",GetLastError());
		m_lbTracks.DeleteAllItems();
		s.Format(IDS_CD_DRIVE_INACCESSIBLE, letter);
		m_lbTracks.InsertItem(0, s);
		return;
	}

	ReloadTrackList();
	InitReadSpeedCombobox();
}

void CCdGrabbingDialog::ReloadTrackList(CdMediaChangeState NewMediaState)
{
	BOOL res = FALSE;
	if (m_CdDrive.IsDriveBusy(m_DriveLetterSelected))
	{
		if (CdMediaStateBusy == m_DiskReady)
		{
			return;
		}
		memzero(m_toc);
		m_Tracks.clear();
		m_lbTracks.DeleteAllItems();
		CString s;
		s.Format(IDS_CD_DRIVE_BUSY, m_DriveLetterSelected);
		m_lbTracks.InsertItem(0, s);

		m_DiskReady = CdMediaStateBusy;
		return;
	}

	if (NewMediaState != CdMediaStateNotReady)
	{
		m_CdDrive.StopAudioPlay();
		//m_CdDrive.ReadSessions( & m_toc);
		res = m_CdDrive.ReadToc( & m_toc);
	}

	if ( ! res)
	{
		if (CdMediaStateNotReady == m_DiskReady)
		{
			return;
		}
		if (CdMediaStateReady == m_DiskReady)
		{
			m_CdDrive.SetTrayOut(true);
		}
		m_lbTracks.DeleteAllItems();
		CString s;
		s.Format(IDS_NO_DISK_IN_CD_DRIVE, m_DriveLetterSelected);
		m_lbTracks.InsertItem(0, s);
		m_Tracks.clear();

		memzero(m_toc);
		m_DiskReady = CdMediaStateNotReady;
		m_bNeedUpdateControls = TRUE;
		return;
	}

	m_bNeedUpdateControls = TRUE;
	// reset media change count:
	m_CdDrive.CheckForMediaChange();

	m_DiskReady = CdMediaStateReady;
	m_lbTracks.DeleteAllItems();

	// Get disk ID
	m_DiskID = m_CdDrive.GetDiskID();
	// TODO: find artist and album info in cdplayer.ini

	CApplicationProfile CdPlayerIni;
	CdPlayerIni.m_pszProfileName = _T("cdplayer.ini");

	CString SectionName;
	SectionName.Format("%X", m_DiskID);

	m_sAlbum = CdPlayerIni.GetProfileString(SectionName, _T("title"), "");
	m_eAlbum.SetWindowText(m_sAlbum);

	m_sArtist = CdPlayerIni.GetProfileString(SectionName, _T("artist"), "");
	m_eArtist.SetWindowText(m_sArtist);

	int NumTracks = CdPlayerIni.GetProfileInt(SectionName, _T("numtracks"), 0);

	m_Tracks.resize(m_toc.LastTrack - m_toc.FirstTrack + 1);

	int tr;
	vector<CdTrackInfo>::iterator pTrack;
	UINT FocusedState = LVIS_FOCUSED | LVIS_SELECTED;
	for (tr = 0, pTrack = m_Tracks.begin()
		; tr <= m_toc.LastTrack - m_toc.FirstTrack; tr++, pTrack++)
	{
		CdAddressMSF NextTrackBegin;
		// track descriptor after last track contains address of the disk end
		NextTrackBegin.Minute = m_toc.TrackData[tr + 1].Address[1];
		NextTrackBegin.Second = m_toc.TrackData[tr + 1].Address[2];
		NextTrackBegin.Frame = m_toc.TrackData[tr + 1].Address[3];

		CString s;

		UINT State = 0, StateMask = 0;
		if (m_toc.TrackData[tr].Control & 0xC)
		{
			pTrack->Track.LoadString(IDS_DATA_TRACK);
			pTrack->IsAudio = false;
			pTrack->Checked = false;
		}
		else
		{
			s.Format(IDS_TRACK_NUM_FORMAT, m_toc.TrackData[tr].TrackNumber);
			TCHAR buf[10];
			_stprintf(buf, "%d", tr);
			pTrack->Track = CdPlayerIni.GetProfileString(SectionName, buf, s);

			pTrack->Album = m_sAlbum;
			pTrack->Artist = m_sArtist;
			pTrack->IsAudio = true;
			State = FocusedState | INDEXTOSTATEIMAGEMASK(2);    // checked
			FocusedState = 0;
			StateMask = LVIS_STATEIMAGEMASK | LVIS_FOCUSED | LVIS_SELECTED;
			pTrack->Checked = TRUE;
		}
		pTrack->TrackBegin.Minute = m_toc.TrackData[tr].Address[1];
		pTrack->TrackBegin.Second = m_toc.TrackData[tr].Address[2];
		pTrack->TrackBegin.Frame = m_toc.TrackData[tr].Address[3];
		pTrack->NumSectors =
			LONG(NextTrackBegin) - LONG(pTrack->TrackBegin);

		LVITEM item1 = {
			LVIF_TEXT | LVIF_STATE,
			tr,
			0,
			State,
			StateMask,
			LPTSTR(LPCTSTR(pTrack->Track)),
			0, 0, 0, 0};
		m_lbTracks.InsertItem( & item1);

		s.Format("%d:%02d",
				(pTrack->NumSectors + 37) / (75 * 60),
				(pTrack->NumSectors + 37) / 75 % 60);

		m_lbTracks.SetItemText(tr, 1, s);

		TRACE("Track %d, addr: %d:%02d.%02d, Adr:%X, Control: %X\n",
			m_toc.TrackData[tr].TrackNumber,
			m_toc.TrackData[tr].Address[1],
			m_toc.TrackData[tr].Address[2],
			m_toc.TrackData[tr].Address[3],
			m_toc.TrackData[tr].Adr,
			m_toc.TrackData[tr].Control);
	}
}

void CCdGrabbingDialog::CreateImageList()
{
	CBitmap CheckboxesBmp;
	CBitmap CheckBmp;
	BITMAP info;
	CDC dc1, dc2;
	CImageList ImgList;

	CheckboxesBmp.LoadOEMBitmap(OBM_CHECKBOXES);
	CheckboxesBmp.GetBitmap( & info);
	// 3 rows, 4 in a row
	// first row: unchecked checkbutton, checked checkbutton
	int cbWidth = info.bmWidth / 4;
	int cbHeight = info.bmHeight / 3;
	CheckBmp.CreateBitmap(cbWidth, cbHeight,
						info.bmPlanes, info.bmBitsPixel, NULL);

	dc1.CreateCompatibleDC(NULL);
	dc2.CreateCompatibleDC(NULL);

	CGdiObject * OldBmp1 = dc1.SelectObject( & CheckboxesBmp);

	ImgList.Create(cbWidth, cbHeight, ILC_COLOR, 2, 1);

	CGdiObject * OldBmp2 = dc2.SelectObject( & CheckBmp);
	dc2.BitBlt(0, 0, cbWidth, cbHeight, & dc1, 0, 0, SRCCOPY);
	dc2.SelectObject(OldBmp2);

	char buf[1024];
	CheckBmp.GetBitmapBits(1024, buf);

	ImgList.Add( & CheckBmp, (CBitmap *) NULL);

	dc2.SelectObject( & CheckBmp);
	dc2.BitBlt(0, 0, cbWidth, cbHeight, & dc1, cbWidth, 0, SRCCOPY);
	dc2.SelectObject(OldBmp2);
	ImgList.Add( & CheckBmp, (CBitmap *) NULL);

	CImageList * pOldList = m_lbTracks.SetImageList( & ImgList, LVSIL_STATE);
	ImgList.Detach();
	if (NULL != pOldList)
	{
		pOldList->DeleteImageList();
	}

	dc1.SelectObject(OldBmp1);
}

BOOL CCdGrabbingDialog::OnInitDialog()
{
	CResizableDialog::OnInitDialog();

	HICON hIcon = AfxGetApp()->LoadIcon(IDI_ICON_CD);
	SetIcon(hIcon, TRUE);			// Set big icon
	SetIcon(hIcon, FALSE);		// Set small icon

	m_BmpPlay.LoadBitmap(IDB_BITMAP_PLAY);
	m_PlayButton.SetBitmap(m_BmpPlay);

	m_BmpStop.LoadBitmap(IDB_BITMAP_STOP);
	m_StopButton.SetBitmap(m_BmpStop);

	m_BmpEject.LoadBitmap(IDB_BITMAP_EJECT);
	m_EjectButton.SetBitmap(m_BmpEject);

	CreateImageList();
	CRect cr;
	GetClientRect( & cr);

	// init MINMAXINFO

	m_lbTracks.GetClientRect( & cr);

	CString s;
	s.LoadString(IDS_TRACK_NAME);
	m_lbTracks.InsertColumn(0, s, LVCFMT_LEFT, cr.Width() - 20, 0);

	s.LoadString(IDS_LENGTH);
	m_lbTracks.InsertColumn(1, s, LVCFMT_LEFT, -1, 1);
	m_lbTracks.InsertColumn(2, " ", LVCFMT_RIGHT, 100, 1);
	m_lbTracks.SetColumnWidth(1, LVSCW_AUTOSIZE_USEHEADER);

	m_lbTracks.SetColumnWidth(0,
							cr.Width() - m_lbTracks.GetColumnWidth(1)
							- GetSystemMetrics(SM_CXVSCROLL) - 2);

	m_lbTracks.DeleteColumn(2);

	FillDriveList(m_PreviousDriveLetter);
	FillTrackList(m_DriveLetterSelected);

	int i;
	for (i = 0; i < sizeof m_FolderHistory / sizeof m_FolderHistory[0]; i++)
	{
		CString s;
		s.Format("SaveFolder%d", i);
		m_Profile.AddItem(_T("CdRead"), s, m_FolderHistory[i]);
		m_FolderHistory[i].TrimLeft();
		m_FolderHistory[i].TrimRight();
		if ( ! m_FolderHistory[i].IsEmpty())
		{
			m_eSaveFolder.AddString(m_FolderHistory[i]);
		}
	}

	for (i = 0; i < sizeof m_AlbumHistory / sizeof m_AlbumHistory[0]; i++)
	{
		CString s;
		s.Format("Album%d", i);
		m_Profile.AddItem(_T("CdRead"), s, m_AlbumHistory[i]);
		m_AlbumHistory[i].TrimLeft();
		m_AlbumHistory[i].TrimRight();
		if ( ! m_AlbumHistory[i].IsEmpty())
		{
			m_eAlbum.AddString(m_AlbumHistory[i]);
		}
	}

	for (i = 0; i < sizeof m_ArtistHistory / sizeof m_ArtistHistory[0]; i++)
	{
		CString s;
		s.Format("Artist%d", i);
		m_Profile.AddItem(_T("CdRead"), s, m_ArtistHistory[i]);
		m_ArtistHistory[i].TrimLeft();
		m_ArtistHistory[i].TrimRight();
		if ( ! m_ArtistHistory[i].IsEmpty())
		{
			m_eArtist.AddString(m_ArtistHistory[i]);
		}
	}

	FillFormatCombo();
	SetTimer(1, 200, NULL);

	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

void CCdGrabbingDialog::OnSize(UINT nType, int cx, int cy)
{
	CSize PrevSize = m_PrevSize;
	CResizableDialog::OnSize(nType, cx, cy);

	// invalidate an area which is (after resizing) occupied by size grip
	int dx = m_PrevSize.cx - PrevSize.cx;
	if (PrevSize.cx < 0 || 0 == dx)
	{
		return;
	}

	m_lbTracks.SetColumnWidth(0, m_lbTracks.GetColumnWidth(0) + dx);

}

/////////////////////////////////////////////////////////////////////////////
// CReopenDialog dialog


CReopenDialog::CReopenDialog(CWnd* pParent /*=NULL*/)
	: CDialog(CReopenDialog::IDD, pParent)
{
	//{{AFX_DATA_INIT(CReopenDialog)
	m_Prompt = _T("");
	//}}AFX_DATA_INIT
}


void CReopenDialog::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CReopenDialog)
	DDX_Text(pDX, IDC_STATIC_PROMPT, m_Prompt);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CReopenDialog, CDialog)
	//{{AFX_MSG_MAP(CReopenDialog)
	ON_BN_CLICKED(IDNO, OnNo)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CReopenDialog message handlers

void CReopenDialog::OnNo()
{
	EndDialog(IDNO);
}

int CInsertSilenceDialog::DoModal()
{
	if (m_pWf->nChannels < 2)
	{
		m_lpszTemplateName = MAKEINTRESOURCE(IDD_DIALOG_INSERT_SILENCE_MONO);
	}
	return CDialog::DoModal();
}

void CCdGrabbingDialog::OnTimer(UINT nIDEvent)
{
	// check CD status
	CheckForDiskChanged();
	if (m_bPlayingAudio)
	{
		FillPlaybackBuffers();
	}
}

void CCdGrabbingDialog::OnButtonMore()
{
	// TODO: Add your control notification handler code here
}

void CCdGrabbingDialog::CheckForDiskChanged()
{
	CdMediaChangeState CdChange= m_CdDrive.CheckForMediaChange();
	switch (CdChange)
	{
	case CdMediaStateNotReady:
		break;
	case CdMediaStateReady:
		if (CdMediaStateReady == m_DiskReady)
		{
			return;
		}
		break;
	case CdMediaStateDiskChanged:
		break;
	}
	ReloadTrackList(CdChange);
	InitReadSpeedCombobox();
}

void CCdGrabbingDialog::OnSelchangeComboDrives()
{
	int sel = m_DrivesCombo.GetCurSel();
	if (sel != m_CDDriveSelected)
	{
		m_CDDriveSelected = sel;
		m_DriveLetterSelected = m_CDDrives[sel];
		FillTrackList(m_DriveLetterSelected);
	}
}

LRESULT CCdGrabbingDialog::OnDeviceChange(UINT event, DWORD data)
{
	DEV_BROADCAST_HDR * pdbh = (DEV_BROADCAST_HDR *) data;;
	switch (event)
	{
	case DBT_DEVICEARRIVAL:
		if (DBT_DEVTYP_VOLUME == pdbh->dbch_devicetype)
		{
			CheckForDrivesChanged();
		}
		break;
	case DBT_DEVICEREMOVEPENDING:
		if (DBT_DEVTYP_VOLUME == pdbh->dbch_devicetype)
		{
			CheckForDrivesChanged();
			// TODO: close the device and open after timeout
		}
		break;
	case DBT_DEVICEREMOVECOMPLETE:
		// check if CD drive is removed
		CheckForDrivesChanged();
		break;
	}
	return TRUE;
}


void CCdGrabbingDialog::OnDestroy()
{
	KillTimer(1);
	StopCdPlayback();
	CResizableDialog::OnDestroy();

}

void CCdGrabbingDialog::CheckForDrivesChanged()
{
	TCHAR PrevLetter = m_DriveLetterSelected;
	FillDriveList(PrevLetter);
	if (PrevLetter != m_DriveLetterSelected)
	{
		FillTrackList(m_DriveLetterSelected);
	}
	else
	{
		CheckForDiskChanged();
	}
}


void CCdGrabbingDialog::OnButtonBrowseSaveFolder()
{
	// save folder name consists of base folder, album name folder,
	// save file name consists of base folder, album name filename,
	// if album filename not entered, it is created from CD label

	// browse for folder
	m_eSaveFolder.GetWindowText(m_sSaveFolder);
	CFolderDialog dlg("Save Files To Folder",
					m_sSaveFolder, TRUE);
	if (IDOK == dlg.DoModal())
	{
		m_sSaveFolder = dlg.GetFolderPath();
		// TODO: check permissiong in callback
		m_eSaveFolder.SetWindowText(m_sSaveFolder);
		m_bNeedUpdateControls = TRUE;
		// TODO: check if the folder exists and create if necessary
	}
}

void CCdGrabbingDialog::OnButtonCddb()
{
	// TODO: Add your control notification handler code here

}

void CCdGrabbingDialog::OnChangeEditAlbum()
{
	// TODO: If this is a RICHEDIT control, the control will not
	// send this notification unless you override the CDialog::OnInitDialog()
	// function and call CRichEditCtrl().SetEventMask()
	// with the ENM_CHANGE flag ORed into the mask.

	// TODO: Add your control notification handler code here

}

void CCdGrabbingDialog::OnChangeEditArtist()
{
	// TODO: If this is a RICHEDIT control, the control will not
	// send this notification unless you override the CDialog::OnInitDialog()
	// function and call CRichEditCtrl().SetEventMask()
	// with the ENM_CHANGE flag ORed into the mask.

	// TODO: Add your control notification handler code here

}

static int const CdSpeeds[] =
{
	1,
	2,
	4,
	8,
	12,
	16,
	24,
	32,
	40,
	48,
	64,
};
void CCdGrabbingDialog::InitReadSpeedCombobox()
{
	m_MaxReadSpeed = 0;
	if (m_DiskReady != CdMediaStateReady
		|| ! m_CdDrive.GetMaxReadSpeed( & m_MaxReadSpeed, & m_CurrentReadSpeed))
	{
		if (m_SpeedCombo.IsWindowEnabled())
		{
			m_SpeedCombo.ResetContent();
			m_SpeedCombo.AddString("Default");  // TODO: LoadString
			m_SpeedCombo.SetCurSel(0);
			m_SpeedCombo.EnableWindow(FALSE);
		}
		return;
	}
	m_SpeedCombo.ResetContent();
	m_SpeedCombo.EnableWindow(TRUE);
	// round MaxReadSpeed to nearest multiple of 176400
	m_MaxReadSpeed += 44100 * 4 / 2;
	m_MaxReadSpeed -= m_MaxReadSpeed % (44100 * 4);

	for (int i = 0; i < sizeof CdSpeeds / sizeof CdSpeeds[0]; i++)
	{
		if (m_MaxReadSpeed < CdSpeeds[i] * 176400)
		{
			break;
		}
		CString s;
		s.Format("%dx", CdSpeeds[i]);
		m_SpeedCombo.AddString(s);
		if (m_SelectedReadSpeed == CdSpeeds[i] * 176400)
		{
			m_SpeedCombo.SetCurSel(i);
		}
	}
	if (i == sizeof CdSpeeds / sizeof CdSpeeds[0]
		|| m_MaxReadSpeed > CdSpeeds[i] * 176400)
	{
		m_SpeedCombo.AddString("Default");  // TODO: LoadString
		m_SpeedCombo.SetCurSel(i);
	}
	else if (LB_ERR == m_SpeedCombo.GetCurSel())
	{
		m_SpeedCombo.SetCurSel(i - 1);
	}
}

void CCdGrabbingDialog::OnClickListTracks(NMHDR* pNMHDR, LRESULT* pResult)
{
	*pResult = 0;
	NMLISTVIEW * pnmlv = (NMLISTVIEW *) pNMHDR;
	LVHITTESTINFO hti;
	hti.pt = pnmlv->ptAction;
	m_lbTracks.HitTest( & hti);
	TRACE("NM_CLICK Hittest=%X, item=%d\n", hti.flags, hti.iItem);
	if (-1 == hti.iItem
		|| hti.iItem >= m_Tracks.size()
		|| 0 == (LVHT_ONITEMSTATEICON & hti.flags)
		|| ! m_Tracks[hti.iItem].IsAudio)
	{
		return;
	}

	LVITEM lvi;
	lvi.iItem = hti.iItem;
	lvi.iSubItem = 0;
	lvi.mask = LVIF_STATE;
	lvi.stateMask = LVIS_STATEIMAGEMASK;

	if (m_Tracks[hti.iItem].Checked)
	{
		// uncheck
		lvi.state = INDEXTOSTATEIMAGEMASK(1);
		m_Tracks[hti.iItem].Checked = false;
	}
	else
	{
		lvi.state = INDEXTOSTATEIMAGEMASK(2);
		m_Tracks[hti.iItem].Checked = true;
	}
	m_lbTracks.SetItem( & lvi);
	m_bNeedUpdateControls = TRUE;
}

void CCdGrabbingDialog::OnDblclkListTracks(NMHDR* pNMHDR, LRESULT* pResult)
{
	*pResult = 0;

	NMLISTVIEW * pnmlv = (NMLISTVIEW *) pNMHDR;
	LVHITTESTINFO hti;
	hti.pt = pnmlv->ptAction;
	m_lbTracks.HitTest( & hti);
	TRACE("NM_CLICK Hittest=%X, item=%d\n", hti.flags, hti.iItem);
	if (-1 == hti.iItem
		|| hti.iItem >= m_Tracks.size()
		|| 0 == (LVHT_ONITEMSTATEICON & hti.flags)
		|| ! m_Tracks[hti.iItem].IsAudio)
	{
		return;
	}

	// check/uncheck all iteps
	LVITEM lvi;
	lvi.iSubItem = 0;
	lvi.mask = LVIF_STATE;
	lvi.stateMask = LVIS_STATEIMAGEMASK;

	bool Checked = m_Tracks[hti.iItem].Checked;
	if (Checked)
	{
		// uncheck
		lvi.state = INDEXTOSTATEIMAGEMASK(2);
	}
	else
	{
		lvi.state = INDEXTOSTATEIMAGEMASK(1);
	}

	for (lvi.iItem = 0; lvi.iItem < m_Tracks.size(); lvi.iItem++)
	{
		if ( ! m_Tracks[lvi.iItem].IsAudio
			|| Checked == m_Tracks[lvi.iItem].Checked)
		{
			continue;
		}
		m_Tracks[lvi.iItem].Checked = Checked;
		m_lbTracks.SetItem( & lvi);
		m_bNeedUpdateControls = TRUE;
	}
}

void CCdGrabbingDialog::OnBeginlabeleditListTracks(NMHDR* pNMHDR, LRESULT* pResult)
{
	NMLVDISPINFO* pDispInfo = (NMLVDISPINFO*)pNMHDR;
	// if the track is not audio track, return TRUE
	TRACE("OnBeginlabeleditListTracks %d\n", pDispInfo->item.iItem);
	if (pDispInfo->item.iItem >= m_Tracks.size()
		|| ! m_Tracks[pDispInfo->item.iItem].IsAudio)
	{
		*pResult = TRUE;
	}
	else
	{
		*pResult = 0;
	}
}

BOOL CCdGrabbingDialog::PreTranslateMessage(MSG* pMsg)
{
	if (pMsg->message == WM_KEYDOWN
		&& pMsg->hwnd != NULL
		&& pMsg->hwnd == m_lbTracks.m_hWnd)
	{
		if (pMsg->wParam == VK_F2)
		{
			// find a selected item
			int nSelItem = -1;
			while (-1 != (nSelItem = m_lbTracks.GetNextItem(nSelItem, LVNI_SELECTED)))
			{
				if (nSelItem < m_Tracks.size()
					&& m_Tracks[nSelItem].IsAudio)
				{
					// unselect all items
					int nItem = -1;
					while (-1 != (nItem = m_lbTracks.GetNextItem(nItem, LVNI_SELECTED)))
					{
						if (nItem != nSelItem)
						{
							m_lbTracks.SetItemState(nItem, LVIS_SELECTED, 0);
						}
					}
					m_lbTracks.SetItemState(nSelItem, LVIS_FOCUSED, LVIS_FOCUSED);
					TRACE("m_lbTracks.EditLabel(%d)\n", nSelItem);
					m_lbTracks.EditLabel(nSelItem);

					break;
				}
			}
			return TRUE;
		}
		else if (pMsg->wParam == VK_SPACE)
		{
			// toggle check
			LVITEM lvi;
			lvi.iItem = m_lbTracks.GetNextItem(-1, LVNI_FOCUSED);
			lvi.iSubItem = 0;
			lvi.mask = LVIF_STATE;
			lvi.stateMask = LVIS_STATEIMAGEMASK;

			if (m_Tracks[lvi.iItem].IsAudio)
			{
				if (m_Tracks[lvi.iItem].Checked)
				{
					// uncheck
					lvi.state = INDEXTOSTATEIMAGEMASK(1);
					m_Tracks[lvi.iItem].Checked = false;
				}
				else
				{
					lvi.state = INDEXTOSTATEIMAGEMASK(2);
					m_Tracks[lvi.iItem].Checked = true;
				}
				m_lbTracks.SetItem( & lvi);
				m_bNeedUpdateControls = TRUE;
			}
			return TRUE;
		}
	}

	return CResizableDialog::PreTranslateMessage(pMsg);
}

void CCdGrabbingDialog::OnUpdateOk(CCmdUI* pCmdUI)
{
	BOOL bEnable = FALSE;
	for (int i = 0; i < m_Tracks.size(); i++)
	{
		if (m_Tracks[i].Checked)
		{
			bEnable = TRUE;
			break;
		}
	}
	bEnable = bEnable && m_eSaveFolder.GetWindowTextLength() != 0;

	pCmdUI->Enable(bEnable);
}

void CCdGrabbingDialog::OnEndlabeleditListTracks(NMHDR* pNMHDR, LRESULT* pResult)
{
	LV_DISPINFO* pDispInfo = (LV_DISPINFO*)pNMHDR;

	if (pDispInfo->item.pszText != NULL)
	{
		// check that the value is not blank and is not the same as other track name
		CString s(pDispInfo->item.pszText);
		s.TrimLeft();
		s.TrimRight();
		if (s.IsEmpty()
			|| pDispInfo->item.iItem >= m_Tracks.size()
			|| ! m_Tracks[pDispInfo->item.iItem].IsAudio)
		{
			*pResult = FALSE;
			return;
		}
		for (int t = 0; t < m_Tracks.size(); t++)
		{
			if (t != pDispInfo->item.iItem
				&& m_Tracks[t].IsAudio
				&& 0 == s.CompareNoCase(m_Tracks[t].Track))
			{
				CString m;
				m.Format(IDS_TRACK_NAME_ALREADY_EXISTS, s);
				AfxMessageBox(m);
				*pResult = FALSE;
				return;
			}
		}
		strcpy(pDispInfo->item.pszText, s);
		m_Tracks[pDispInfo->item.iItem].Track = s;
	}
	*pResult = TRUE;
}

void CCdGrabbingDialog::OnChangeEditFolderOrFile()
{
	m_bNeedUpdateControls = TRUE;
}

void CCdGrabbingDialog::OnButtonPlay()
{
	if (m_bPlayingAudio)
	{
		StopCdPlayback();
		return;
	}
	int tr = m_lbTracks.GetNextItem(-1, LVNI_SELECTED);
	if (-1 == tr)
	{
		tr = m_lbTracks.GetNextItem(-1, LVNI_FOCUSED);
	}
	if (-1 == tr)
	{
		return;
	}
	StartCdPlayback(tr);
}

void CCdGrabbingDialog::StartCdPlayback(unsigned track)
{
	if (m_CdDrive.IsDriveBusy()
		|| track >= m_Tracks.size()
		|| ! m_Tracks[track].IsAudio)
	{
		return;
	}
	m_PlaybackAddress = m_Tracks[track].TrackBegin;
	m_PlaybackSectors = m_Tracks[track].NumSectors;
	m_PlayingTrack = track;

	WAVEFORMATEX wfx;
	wfx.cbSize = 0;
	wfx.nChannels = 2;
	wfx.nSamplesPerSec = 44100;
	wfx.wBitsPerSample = 16;
	wfx.wFormatTag = WAVE_FORMAT_PCM;
	wfx.nBlockAlign = 4;
	wfx.nAvgBytesPerSec = 44100 * 4;

	MMRESULT mmres = m_WaveOut.Open(GetApp()->m_DefaultPlaybackDevice,
									& wfx, 0);
	if (MMSYSERR_NOERROR != mmres)
	{
		return;
	}
	if (FALSE == m_WaveOut.AllocateBuffers(0x10000 - 0x10000 % CDDASectorSize, 4))
	{
		m_WaveOut.Close();
		return;
	}
	m_bPlayingAudio = TRUE;
	m_CdDrive.SetReadSpeed(176400 * 4);
	FillPlaybackBuffers();

	m_bNeedUpdateControls = TRUE;
}

void CCdGrabbingDialog::OnButtonStop()
{
	StopCdPlayback();
}

void CCdGrabbingDialog::OnUpdatePlay(CCmdUI* pCmdUI)
{
	BOOL bEnable = FALSE;
	for (int t = 0; t < m_Tracks.size(); t++)
	{
		if (m_Tracks[t].IsAudio)
		{
			bEnable = TRUE;
			break;
		}
	}

	pCmdUI->Enable(bEnable);
	pCmdUI->SetCheck(m_bPlayingAudio);

}

BOOL CCdGrabbingDialog::FillPlaybackBuffers()
{
	// get sound buffers, fill from CD
	char * pBuffer;
	size_t BufSize;
	int hBuffer;
	while (0 != m_PlaybackSectors
			&& (hBuffer = m_WaveOut.GetBuffer( & pBuffer, & BufSize, FALSE)) > 0)
	{
		int NumSectors = BufSize / CDDASectorSize;
		if (NumSectors > m_PlaybackSectors)
		{
			NumSectors = m_PlaybackSectors;
		}
		if ( ! m_CdDrive.ReadCdData(pBuffer, m_PlaybackAddress, NumSectors))
		{
			m_WaveOut.ReturnBuffer(hBuffer);
			StopCdPlayback();
			return FALSE;
		}

		m_PlaybackAddress = m_PlaybackAddress + NumSectors;
		m_PlaybackSectors -= NumSectors;
		m_WaveOut.Play(hBuffer, NumSectors * CDDASectorSize);
	}

	if (0 == m_PlaybackSectors)
	{
		// wait until all buffers are returned
		if (m_WaveOut.WaitForQueueEmpty(0))
		{
			StopCdPlayback();
			return FALSE;
		}
	}
	return TRUE;
}

void CCdGrabbingDialog::StopCdPlayback()
{
	if (m_bPlayingAudio)
	{
		m_WaveOut.Reset();
		m_WaveOut.WaitForQueueEmpty(1000);
		m_WaveOut.DeallocateBuffers();
		m_WaveOut.Close();

		m_bNeedUpdateControls = TRUE;
	}
	m_bPlayingAudio = FALSE;
}

void CCdGrabbingDialog::OnUpdateStop(CCmdUI* pCmdUI)
{
	pCmdUI->Enable(m_bPlayingAudio);
}

void CCdGrabbingDialog::OnSelchangeComboSpeed()
{
	m_SelectedReadSpeed = 176400 * CdSpeeds[m_SpeedCombo.GetCurSel()];
}

void CCdGrabbingDialog::OnItemchangedListTracks(NMHDR* pNMHDR, LRESULT* pResult)
{
	NM_LISTVIEW* pNMListView = (NM_LISTVIEW*)pNMHDR;
	if (m_bPlayingAudio
		&& pNMListView->iItem != m_PlayingTrack)
	{
		// if another item becomes selected, change playback to that item
		if (pNMListView->uNewState
			& ~ pNMListView->uOldState
			& LVIS_SELECTED)
		{
			StopCdPlayback();
			StartCdPlayback(pNMListView->iItem);
		}
	}
	*pResult = 0;
}

void CCdGrabbingDialog::OnCancel()
{
	if (! m_CdDrive.IsDriveBusy())
	{
		m_CdDrive.SetReadSpeed(m_CurrentReadSpeed);
		m_CdDrive.StopDrive();
	}

	CResizableDialog::OnCancel();
}

void CCdGrabbingDialog::OnContextMenu(CWnd* pWnd, CPoint point)
{
	CResizableDialog::OnContextMenu(pWnd, point);
	return;
	CRect cr;
	m_lbTracks.GetClientRect( & cr);
	m_lbTracks.ClientToScreen( & cr);
	if ( ! cr.PtInRect(point))
	{
		return;
	}
	CMenu menu;
	menu.LoadMenu(IDR_MENU_CDGRAB);
	CMenu* pPopup = menu.GetSubMenu(0);
	if(pPopup != NULL)
	{
		pPopup->TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON,
								point.x, point.y,
								this);
	}
}


void CCdGrabbingDialog::OnRadioWmaFormat()
{
	// TODO: Add your control notification handler code here
	// check if WMA encoder is available
	m_bNeedUpdateControls = TRUE;
	if (GetApp()->CanOpenWindowsMedia())
	{
		m_RadioFileFormat = 1;
		FillFormatCombo();

		return;
	}
	int id = AfxMessageBox(IDS_WMA_ENCODER_NOT_AVILABLE, MB_OK | MB_ICONEXCLAMATION | MB_HELP);
	if (IDHELP == id)
	{
		// TODO
	}
	CheckRadioButton(IDC_RADIO_WAV_FORMAT, IDC_RADIO_WAV_FORMAT, IDC_RADIO_MP3_FORMAT);
	m_RadioFileFormat = 0;
	FillFormatCombo();
}

void CCdGrabbingDialog::OnRadioMp3Format()
{
	m_bNeedUpdateControls = TRUE;
	m_RadioFileFormat = 2;

	FillFormatCombo();
}

void CCdGrabbingDialog::OnRadioWavFormat()
{
	// TODO: Add your control notification handler code here
	m_bNeedUpdateControls = TRUE;
	m_RadioFileFormat = 0;
	FillFormatCombo();
}
void CCdGrabbingDialog::OnUpdateComboBitrate(CCmdUI* pCmdUI)
{
	pCmdUI->Enable(0 != m_RadioFileFormat);
}

void CCdGrabbingDialog::OnUpdateEject(CCmdUI* pCmdUI)
{
	BOOL Enable;
	// the button enabled:
	// always for tray-loadable drives
	// if a disk is loaded to slot-load drive
	if (m_CdDrive.IsDriveBusy()
		|| ! m_CdDrive.EjectSupported())
	{
		Enable = FALSE;
	}
	else if (m_CdDrive.IsSlotType())
	{
		Enable = m_DiskReady == CdMediaStateReady;
	}
	else if (m_CdDrive.IsTrayType())
	{
		if (m_CdDrive.IsTrayOpen())
		{
			Enable = m_CdDrive.CanLoadMedia();
		}
		else
		{
			//
			Enable = m_CdDrive.CanEjectMedia();
		}
	}
}


void CCdGrabbingDialog::OnButtonEject()
{
	if (m_CdDrive.IsDriveBusy())
	{
		return;
	}
	if (m_CdDrive.IsTrayOpen())
	{
		m_CdDrive.LoadMedia();
	}
	else
	{
		m_CdDrive.EjectMedia();
	}
}

void CCdGrabbingDialog::FillFormatCombo()
{
	m_ComboBitrate.ResetContent();
	switch (m_RadioFileFormat)
	{
	case 0:
		// WAV
		m_Acm.m_FormatTags.resize(1);
		m_Acm.m_FormatTags[0].Tag.Tag = WAVE_FORMAT_PCM;
		m_Acm.m_FormatTags[0].Name = _T("PCM");
		m_Acm.m_Formats.resize(1);
		m_Acm.m_Formats[0].Name = m_Acm.GetFormatName(NULL, m_Acm.m_Wf);
		m_Acm.m_Formats[0].Wf = m_Acm.m_Wf;
		m_Acm.m_Formats[0].TagIndex = 0;
		break;
	case 1:
		// WMA
		m_Acm.FillWmaFormatTags();
		m_Acm.FillMultiFormatArray(0, m_Acm.m_FormatTags.size() - 1,
									WaveFormatMatchCnannels | WaveFormatMatchSampleRate);
		break;
	case 2:
		m_Acm.FillMp3EncoderTags(WaveFormatMatchCnannels | WaveFormatMatchSampleRate);
		m_Acm.FillMultiFormatArray(0, m_Acm.m_FormatTags.size() - 1,
									WaveFormatMatchCnannels | WaveFormatMatchSampleRate);
		break;
	}
	for (int i = 0; i < m_Acm.m_Formats.size(); i++)
	{
		m_ComboBitrate.AddString(m_Acm.m_Formats[i].Name
								+ _T(" - ") + m_Acm.m_FormatTags[m_Acm.m_Formats[i].TagIndex].Name);
	}
	m_ComboBitrate.SetCurSel(0);
}


void CCdGrabbingDialog::OnSysColorChange()
{
	CResizableDialog::OnSysColorChange();

	CreateImageList();
}
