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
	m_RadioStoreImmediately = -1;
	m_RadioStoreMultiple = -1;
	m_sSaveFolderOrFile = _T("");
	m_sAlbum = _T("");
	m_sArtist = _T("");
	//}}AFX_DATA_INIT
	m_RadioAssignAttributes = 0;
	m_RadioStoreImmediately = 0;
	m_RadioStoreMultiple = 0;
	m_DiskID = -1;
	m_pWfx = NULL;
	m_bNeedUpdateControls = TRUE;
	m_MaxReadSpeed = 0;
	m_DiskReady = DiskStateUnknown;
	m_bPlayingAudio = FALSE;

	m_Profile.AddItem(_T("CdRead"), _T("BaseDirectory"), m_sSaveFolderOrFile);
	m_Profile.AddItem(_T("CdRead"), _T("Speed"), m_SelectedReadSpeed, 64000000,
					176400, 0x10000000);
	m_Profile.AddItem(_T("CdRead"), _T("DriveLetter"),
					m_PreviousDriveLetter, 'Z', 'A', 'Z');
	m_Profile.AddItem(_T("CdRead"), _T("Speed"), m_SelectedReadSpeed, 64000000,
					176400, 0x10000000);

	m_Profile.AddBoolItem(_T("CdRead"),
						_T("AssignToAllOrSelected"), m_RadioAssignAttributes, FALSE);
	m_Profile.AddBoolItem(_T("CdRead"),
						_T("EditFiles"), m_RadioStoreImmediately, FALSE);
	m_Profile.AddBoolItem(_T("CdRead"),
						_T("StoreSingleFile"), m_RadioStoreMultiple, FALSE);

	static ResizableDlgItem const ResizeItems[] =
	{
		{IDC_BUTTON_SELECT_ALL, MoveRight},
		{IDC_BUTTON_DESELECT_ALL, MoveRight},

		{IDC_RADIO_ASSIGN_ATTRIBUTES, MoveDown},
		{IDC_RADIO_ASSIGN_SELECTED_TRACK, MoveDown},
		{IDC_STATIC_ALBUM, MoveDown},
		{IDC_EDIT_ALBUM, MoveDown},
		{IDC_STATIC_ARTIST, MoveDown},
		{IDC_EDIT_ARTIST, MoveDown},
		{IDC_RADIO_STORE_MULTIPLE_FILES, MoveDown},
		{IDC_RADIO_STORE_SINGLE_FILE, MoveDown},
		{IDC_EDIT_FOLDER_OR_FILE, MoveDown},
		{IDC_BUTTON_BROWSE_SAVE_FOLDER, MoveDown},
		{IDC_RADIO_STORE_IMMEDIATELY, MoveDown},
		{IDC_RADIO_LOAD_FOR_EDITING, MoveDown},
		{IDC_STATIC_FORMAT, MoveDown},
		{IDC_BUTTON_SET_FORMAT, MoveDown},

		{IDC_BUTTON_CDDB, MoveRight | MoveDown},
		{IDOK, MoveRight | MoveDown},
		{IDCANCEL, MoveRight | MoveDown},

		{IDC_LIST_TRACKS, ExpandRight | ExpandDown},
	};

	m_pResizeItems = ResizeItems;
	m_pResizeItemsCount = sizeof ResizeItems / sizeof ResizeItems[0];

}

CCdGrabbingDialog::~CCdGrabbingDialog()
{
}

void CCdGrabbingDialog::DoDataExchange(CDataExchange* pDX)
{
	CResizableDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CCdGrabbingDialog)
	DDX_Control(pDX, IDC_BUTTON_STOP, m_StopButton);
	DDX_Control(pDX, IDC_BUTTON_PLAY, m_PlayButton);
	DDX_Control(pDX, IDC_EDIT_ARTIST, m_eArtist);
	DDX_Control(pDX, IDC_EDIT_ALBUM, m_eAlbum);
	DDX_Control(pDX, IDC_EDIT_FOLDER_OR_FILE, m_eSaveFolderOrFile);
	DDX_Control(pDX, IDC_STATIC_FORMAT, m_StaticFormat);
	DDX_Control(pDX, IDC_COMBO_SPEED, m_SpeedCombo);
	DDX_Control(pDX, IDC_COMBO_DRIVES, m_DrivesCombo);
	DDX_Control(pDX, IDC_LIST_TRACKS, m_lbTracks);
	DDX_Radio(pDX, IDC_RADIO_ASSIGN_ATTRIBUTES, m_RadioAssignAttributes);
	DDX_Radio(pDX, IDC_RADIO_STORE_IMMEDIATELY, m_RadioStoreImmediately);
	DDX_Radio(pDX, IDC_RADIO_STORE_MULTIPLE_FILES, m_RadioStoreMultiple);
	DDX_Text(pDX, IDC_EDIT_FOLDER_OR_FILE, m_sSaveFolderOrFile);
	DDX_Text(pDX, IDC_EDIT_ALBUM, m_sAlbum);
	DDX_Text(pDX, IDC_EDIT_ARTIST, m_sArtist);
	//}}AFX_DATA_MAP

	if (pDX->m_bSaveAndValidate)
	{
		// save album and track names to CDPLAYER.INI
		// get selected tracks
		// check for valid file names
		// get base folder
		// check if the folder exists
		if (0 == m_RadioStoreMultiple)
		{
			m_sSaveFolderOrFile.TrimLeft();
			m_sSaveFolderOrFile.TrimRight();

			if ( ! VerifyCreateDirectory(m_sSaveFolderOrFile))
			{
				pDX->PrepareEditCtrl(IDC_EDIT_FOLDER_OR_FILE);
				pDX->Fail();
			}
			// create valid file names, ask for file replace
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
				m_Tracks[t].TrackFileName = m_sSaveFolderOrFile;
				if (! m_sSaveFolderOrFile.IsEmpty())
				{
					TCHAR c = m_sSaveFolderOrFile[m_sSaveFolderOrFile.GetLength() - 1];
					if (c != '\\'
						&& c != '/')
					{
						m_Tracks[t].TrackFileName += '\\';
					}
				}
				Name.ReleaseBuffer();
				m_Tracks[t].TrackFileName += Name;
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
	ON_BN_CLICKED(IDC_BUTTON_BROWSE_SAVE_FOLDER, OnButtonBrowseSaveFolder)
	ON_BN_CLICKED(IDC_BUTTON_CDDB, OnButtonCddb)
	ON_BN_CLICKED(IDC_BUTTON_DESELECT_ALL, OnButtonDeselectAll)
	ON_BN_CLICKED(IDC_BUTTON_SELECT_ALL, OnButtonSelectAll)
	ON_BN_CLICKED(IDC_BUTTON_SET_FORMAT, OnButtonSetFormat)
	ON_EN_CHANGE(IDC_EDIT_ALBUM, OnChangeEditAlbum)
	ON_EN_CHANGE(IDC_EDIT_ARTIST, OnChangeEditArtist)
	ON_BN_CLICKED(IDC_RADIO_STORE_MULTIPLE_FILES, OnRadioStoreMultipleFiles)
	ON_BN_CLICKED(IDC_RADIO_STORE_SINGLE_FILE, OnRadioStoreSingleFile)
	ON_NOTIFY(NM_CLICK, IDC_LIST_TRACKS, OnClickListTracks)
	ON_NOTIFY(LVN_BEGINLABELEDIT, IDC_LIST_TRACKS, OnBeginlabeleditListTracks)
	ON_NOTIFY(LVN_ENDLABELEDIT, IDC_LIST_TRACKS, OnEndlabeleditListTracks)
	ON_EN_CHANGE(IDC_EDIT_FOLDER_OR_FILE, OnChangeEditFolderOrFile)
	ON_BN_CLICKED(IDC_BUTTON_PLAY, OnButtonPlay)
	ON_BN_CLICKED(IDC_BUTTON_STOP, OnButtonStop)
	//}}AFX_MSG_MAP
	ON_WM_DEVICECHANGE()
	ON_MESSAGE(WM_KICKIDLE, OnKickIdle)
	ON_UPDATE_COMMAND_UI(IDOK, OnUpdateOk)
	ON_UPDATE_COMMAND_UI(IDC_BUTTON_SELECT_ALL, OnUpdateSelectAll)
	ON_UPDATE_COMMAND_UI(IDC_BUTTON_DESELECT_ALL, OnUpdateDeselectAll)
	ON_UPDATE_COMMAND_UI(IDC_BUTTON_PLAY, OnUpdatePlay)
	ON_UPDATE_COMMAND_UI(IDC_BUTTON_STOP, OnUpdateStop)

END_MESSAGE_MAP()

void CCdGrabbingDialog::FillDriveList(TCHAR SelectDrive)
{
	m_NumberOfDrives = m_CdDrive.FindCdDrives(m_CDDrives);;
	m_DriveLetterSelected = 0;
	m_CDDriveSelected = 0;
	m_DiskReady = DiskStateUnknown;

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
	m_DiskReady = DiskStateUnknown;
	if ( ! m_CdDrive.Open(letter))
	{
		return FALSE;
	}
	m_CdDrive.DisableMediaChangeDetection();
	return TRUE;
}

void CCdGrabbingDialog::FillTrackList(TCHAR letter)
{
	if (0 == m_NumberOfDrives)
	{
		if (DiskStateNoCdDrive == m_DiskReady)
		{
			return;
		}
		m_lbTracks.DeleteAllItems();
		m_lbTracks.InsertItem(0, "CD drives not found");
		m_Tracks.clear();

		memzero(m_toc);
		m_DiskReady = DiskStateNoCdDrive;
		return;
	}
	if ( ! OpenDrive(letter))
	{
		TRACE("Couldn't open CD,error=%d\n",GetLastError());
		m_lbTracks.DeleteAllItems();
		m_lbTracks.InsertItem(0, "Invalid drive letter");
		return;
	}
	ReloadTrackList();
	InitReadSpeedCombobox();
}

void CCdGrabbingDialog::ReloadTrackList()
{
	m_CdDrive.StopAudioPlay();
	m_CdDrive.ReadSessions( & m_toc);
	BOOL res = m_CdDrive.ReadToc( & m_toc);

	if ( ! res)
	{
		if (DiskStateNotReady == m_DiskReady)
		{
			return;
		}
		m_lbTracks.DeleteAllItems();
		CString s;
		s.Format(IDS_NO_DISK_IN_CD_DRIVE, m_DriveLetterSelected);
		m_lbTracks.InsertItem(0, s);
		m_Tracks.clear();

		memzero(m_toc);
		m_DiskReady = DiskStateNotReady;
		m_bNeedUpdateControls = TRUE;
		return;
	}

	m_bNeedUpdateControls = TRUE;
	// reset media change count:
	m_CdDrive.CheckForMediaChange();

	m_DiskReady = DiskStateReady;
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
	CheckBmp.CreateBitmap(cbWidth, cbHeight, 1, 1, NULL);

	dc1.CreateCompatibleDC(NULL);
	dc2.CreateCompatibleDC(NULL);

	CGdiObject * OldBmp1 = dc1.SelectObject( & CheckboxesBmp);

	ImgList.Create(cbWidth, cbHeight, ILC_COLOR, 2, 1);

	CGdiObject * OldBmp2 = dc2.SelectObject( & CheckBmp);
	dc2.BitBlt(0, 0, cbWidth, cbHeight, & dc1, 0, 0, SRCCOPY);
	dc2.SelectObject(OldBmp2);
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
		ReloadTrackList();
		InitReadSpeedCombobox();
		break;
	case CdMediaStateSameMedia:
		return;
		break;
	case CdMediaStateChanged:
		ReloadTrackList();
		InitReadSpeedCombobox();
		break;
	}
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

	if (0 == m_RadioStoreMultiple)
	{
		// browse for folder
		m_eSaveFolderOrFile.GetWindowText(m_sSaveFolderOrFile);
		CFolderDialog dlg("Save Files To Folder",
						m_sSaveFolderOrFile, TRUE);
		if (IDOK == dlg.DoModal())
		{
			m_sSaveFolderOrFile = dlg.GetFolderPath();
			// TODO: check permissiong in callback
			m_eSaveFolderOrFile.SetWindowText(m_sSaveFolderOrFile);
			// TODO: check if the folder exists and create if necessary
		}
	}
	else
	{
		// browse for file
		CString FileName;
		CString Filter;
		Filter.LoadString(IDS_PLAYLIST_FILE_FILTER);

		CString Title;
		Title.LoadString(IDS_PLAYLIST_SAVE_TITLE);

		m_eSaveFolderOrFile.GetWindowText(m_sSaveFolderOrFile);
		if (m_sSaveFolderOrFile.IsEmpty())
		{
			//m_sSaveFolderOrFile = _T("album.m3u");
		}

		CWaveSoapFileSaveDialog dlg(FALSE,
									"wav", m_sSaveFolderOrFile,
									OFN_HIDEREADONLY
									| OFN_OVERWRITEPROMPT
									| OFN_ENABLESIZING
									| OFN_EXPLORER
									| OFN_ENABLETEMPLATE
									//| OFN_NONETWORKBUTTON
									| OFN_PATHMUSTEXIST,
									Filter);
		dlg.m_ofn.lpstrTitle = Title;

		dlg.m_pDocument = NULL;
		//dlg.m_ofn.lpstrInitialDir = m_eSaveFolderOrFile;

		WAVEFORMATEX * pWf = (WAVEFORMATEX*) new char[0xFFFF]; // max size
		if (NULL == pWf)
		{
			NotEnoughMemoryMessageBox();
			return;
		}
		// sample rate and number of channels might change from the original file
		// new format may not be quite valid for some convertors!!
		pWf->nSamplesPerSec = 44100;
		pWf->nChannels = 2;
		pWf->cbSize = 0;
		pWf->nAvgBytesPerSec = 44100 *4;
		pWf->nBlockAlign = 4;
		pWf->wBitsPerSample = 16;
		pWf->wFormatTag = WAVE_FORMAT_PCM;

		dlg.m_pWf = pWf;

		if (IDOK != dlg.DoModal())
		{
			delete[] (char*)pWf;
			return;
		}

		m_sSaveFolderOrFile = dlg.GetPathName();
		m_eSaveFolderOrFile.SetWindowText(m_sSaveFolderOrFile);
		delete[] (char*)pWf;
	}
}

void CCdGrabbingDialog::OnButtonCddb()
{
	// TODO: Add your control notification handler code here

}

void CCdGrabbingDialog::OnButtonDeselectAll()
{
	for (int tr = 0; tr < m_Tracks.size(); tr++)
	{
		if (m_Tracks[tr].IsAudio)
		{
			if (m_Tracks[tr].Checked)
			{
				m_bNeedUpdateControls = TRUE;
			}
			m_lbTracks.SetCheck(tr, FALSE);
			m_Tracks[tr].Checked = false;
		}
	}
}

void CCdGrabbingDialog::OnButtonSelectAll()
{
	for (int tr = 0; tr < m_Tracks.size(); tr++)
	{
		if (m_Tracks[tr].IsAudio)
		{
			if ( ! m_Tracks[tr].Checked)
			{
				m_bNeedUpdateControls = TRUE;
			}
			m_lbTracks.SetCheck(tr, TRUE);
			m_Tracks[tr].Checked = true;
		}
	}
}

void CCdGrabbingDialog::OnButtonSetFormat()
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

void CCdGrabbingDialog::OnRadioStoreMultipleFiles()
{
	// change edit box to the folder string
	m_RadioStoreMultiple = 0;
}

void CCdGrabbingDialog::OnRadioStoreSingleFile()
{
	m_RadioStoreMultiple = 1;

}

void CCdGrabbingDialog::InitReadSpeedCombobox()
{
	m_MaxReadSpeed = 0;
	if (m_DiskReady != DiskStateReady
		|| ! m_CdDrive.GetMaxReadSpeed( & m_MaxReadSpeed))
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

	static int const Speeds[] =
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
	for (int i = 0; i < sizeof Speeds / sizeof Speeds[0]; i++)
	{
		if (m_MaxReadSpeed < Speeds[i] * 176400)
		{
			break;
		}
		CString s;
		s.Format("%dx", Speeds[i]);
		m_SpeedCombo.AddString(s);
		if (m_SelectedReadSpeed == Speeds[i] * 176400)
		{
			m_SpeedCombo.SetCurSel(i);
		}
	}
	if (i == sizeof Speeds / sizeof Speeds[0]
		|| m_MaxReadSpeed > Speeds[i] * 176400)
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
	if (pMsg->message == WM_KEYDOWN &&
		pMsg->wParam == VK_F2
		&& pMsg->hwnd != NULL
		&& pMsg->hwnd == m_lbTracks.m_hWnd)
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
	bEnable = bEnable && m_eSaveFolderOrFile.GetWindowTextLength() != 0;

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
		if (s.IsEmpty())
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
	}
	*pResult = TRUE;
}

void CCdGrabbingDialog::OnChangeEditFolderOrFile()
{
	m_bNeedUpdateControls = TRUE;
}

void CCdGrabbingDialog::OnButtonPlay()
{
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
	m_bNeedUpdateControls = TRUE;
	if ( ! FillPlaybackBuffers())
	{
		StopCdPlayback();
	}
}

void CCdGrabbingDialog::OnButtonStop()
{
	if (m_bPlayingAudio)
	{
		m_WaveOut.Reset();
	}
	m_bPlayingAudio = FALSE;
	m_bNeedUpdateControls = TRUE;

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
	return TRUE;
}

void CCdGrabbingDialog::StopCdPlayback()
{
}

void CCdGrabbingDialog::OnUpdateStop(CCmdUI* pCmdUI)
{
	pCmdUI->Enable(m_bPlayingAudio);
}

void CCdGrabbingDialog::OnUpdateSelectAll(CCmdUI* pCmdUI)
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
}

void CCdGrabbingDialog::OnUpdateDeselectAll(CCmdUI* pCmdUI)
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
}
