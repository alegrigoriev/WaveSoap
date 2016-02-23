// Copyright Alexander Grigoriev, 1997-2002, All Rights Reserved
// PreferencesPropertySheet.cpp : implementation file
//

#include "stdafx.h"
#include "WaveSoapFront.h"
#include "PreferencesPropertySheet.h"
#include "FolderDialog.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CPreferencesPropertySheet

IMPLEMENT_DYNAMIC(CPreferencesPropertySheet, CPropertySheet)

CPreferencesPropertySheet::CPreferencesPropertySheet(UINT nIDCaption, CWnd* pParentWnd, UINT iSelectPage)
	:CPropertySheet(nIDCaption, pParentWnd, iSelectPage)
	, m_PageSelected(iSelectPage)
	, m_UndoPage(PersistentUndoRedo::GetData())
{
	m_psh.dwFlags |= PSH_NOAPPLYNOW;
	AddPage( & m_FilePage);
	AddPage( & m_UndoPage);
	AddPage( & m_SoundPage);
	AddPage( & m_ViewPage);
}

CPreferencesPropertySheet::CPreferencesPropertySheet(LPCTSTR pszCaption, CWnd* pParentWnd, UINT iSelectPage)
	:CPropertySheet(pszCaption, pParentWnd, iSelectPage)
	, m_PageSelected(iSelectPage)
	, m_UndoPage(PersistentUndoRedo::GetData())
{
	m_psh.dwFlags |= PSH_NOAPPLYNOW;
	AddPage( & m_FilePage);
	AddPage( & m_UndoPage);
	AddPage( & m_SoundPage);
	AddPage( & m_ViewPage);
}

CPreferencesPropertySheet::~CPreferencesPropertySheet()
{
}

BEGIN_MESSAGE_MAP(CPreferencesPropertySheet, CPropertySheet)
	//{{AFX_MSG_MAP(CPreferencesPropertySheet)
		// NOTE - the ClassWizard will add and remove mapping macros here.
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CPreferencesPropertySheet message handlers
/////////////////////////////////////////////////////////////////////////////

BOOL CPreferencesPropertySheet::OnWndMsg(UINT message, WPARAM wParam, LPARAM lParam, LRESULT* pResult)
{
	if (WM_COMMAND == message
		&& BN_CLICKED == HIWORD(wParam)
		&& IDOK == LOWORD(wParam))
	{
		m_PageSelected = GetActiveIndex();
	}
	return CPropertySheet::OnWndMsg(message, wParam, lParam, pResult);
}

// CFilePreferencesPage property page

IMPLEMENT_DYNCREATE(CFilePreferencesPage, CPropertyPage)

CFilePreferencesPage::CFilePreferencesPage()
	: BaseClass(CFilePreferencesPage::IDD)
{
	//{{AFX_DATA_INIT(CFilePreferencesPage)
	m_sTempFileLocation = _T("");
	m_MaxMemoryFileSize = 0;
	m_DefaultFileOpenMode = -1;
	m_bEnable4GbWavFile = -1;
	m_MaxFileCache = 0;
	m_FileTextEncoding = 0;
	//}}AFX_DATA_INIT
}

CFilePreferencesPage::~CFilePreferencesPage()
{
}

void CFilePreferencesPage::DoDataExchange(CDataExchange* pDX)
{
	BaseClass::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CFilePreferencesPage)
	DDX_Control(pDX, IDC_EDIT_TEMP_FILE_LOCATION, m_eTempFileLocation);
	DDX_Text(pDX, IDC_EDIT_TEMP_FILE_LOCATION, m_sTempFileLocation);
	DDX_Text(pDX, IDC_EDIT_TEMP_MEMORY_FILE_LIMIT, m_MaxMemoryFileSize);
	DDV_MinMaxUInt(pDX, m_MaxMemoryFileSize, 1, 16*1024);
	DDX_Radio(pDX, IDC_RADIO_OPEN_FILE_MODE, m_DefaultFileOpenMode);
	DDX_Radio(pDX, IDC_RADIO_WAV_SIZE, m_bEnable4GbWavFile);
	DDX_Text(pDX, IDC_EDIT_MAX_FILE_CACHE, m_MaxFileCache);
	DDV_MinMaxUInt(pDX, m_MaxFileCache, 1, 1024);
	DDX_Radio(pDX, IDC_RADIO_TEXT_ANSI, m_FileTextEncoding);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CFilePreferencesPage, BaseClass)
	//{{AFX_MSG_MAP(CFilePreferencesPage)
	ON_BN_CLICKED(IDC_BUTTON_BROWSE_TEMP_FILE_LOCATION, OnButtonBrowseTempFileLocation)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CFilePreferencesPage message handlers
/////////////////////////////////////////////////////////////////////////////

void CFilePreferencesPage::OnButtonBrowseTempFileLocation()
{
	m_eTempFileLocation.GetWindowText(m_sTempFileLocation);
	CFolderDialog dlg(IDS_TEMP_FOLDER_DIALOG_TITLE,
					m_sTempFileLocation,
					TRUE);

	if (IDOK == dlg.DoModal())
	{
		m_sTempFileLocation = dlg.GetFolderPath();
		// TODO: check permissiong in callback
		m_eTempFileLocation.SetWindowText(m_sTempFileLocation);
	}
}

// CSoundPreferencesPage property page

IMPLEMENT_DYNCREATE(CSoundPreferencesPage, CPropertyPage)

CSoundPreferencesPage::CSoundPreferencesPage()
	: BaseClass(CSoundPreferencesPage::IDD)
{
	//{{AFX_DATA_INIT(CSoundPreferencesPage)
	m_PlaybackDevice = WAVE_MAPPER;
	m_RecordingDevice = WAVE_MAPPER;
	m_NumPlaybackBuffers = 0;
	m_NumRecordingBuffers = 0;
	m_PlaybackBufferSize = 0;
	m_RecordingBufferSize = 0;
	//}}AFX_DATA_INIT
}

CSoundPreferencesPage::~CSoundPreferencesPage()
{
}

void CSoundPreferencesPage::DoDataExchange(CDataExchange* pDX)
{
	BaseClass::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CSoundPreferencesPage)
	DDX_Control(pDX, IDC_SPIN_NUM_RECORDING_BUFFERS, m_SpinRecordingBufs);
	DDX_Control(pDX, IDC_SPIN_NUM_PLAYBACK_BUFFERS, m_SpinPlaybackBufs);
	DDX_Control(pDX, IDC_COMBO_RECORDING_DEVICE, m_RecordingDeviceCombo);
	DDX_Control(pDX, IDC_COMBO_PLAYBACK_DEVICE, m_PlaybackDeviceCombo);
	DDX_Text(pDX, IDC_EDIT_NUM_PLAYBACK_BUFFERS, m_NumPlaybackBuffers);
	DDV_MinMaxUInt(pDX, m_NumPlaybackBuffers, 2, 32);
	DDX_Text(pDX, IDC_EDIT_NUM_RECORDING_BUFFERS, m_NumRecordingBuffers);
	DDV_MinMaxUInt(pDX, m_NumRecordingBuffers, 2, 32);
	DDX_Text(pDX, IDC_EDIT_PLAYBACK_BUF_SIZE, m_PlaybackBufferSize);
	DDV_MinMaxUInt(pDX, m_PlaybackBufferSize, 4, 256);
	DDX_Text(pDX, IDC_EDIT_RECORDING_BUF_SIZE, m_RecordingBufferSize);
	DDV_MinMaxUInt(pDX, m_RecordingBufferSize, 4, 256);
	//}}AFX_DATA_MAP

	if ( ! pDX->m_bSaveAndValidate)
	{
		// fill playback devices combo
		int const NumOutDevs = CWaveOut::GetNumDevs();

		if (m_PlaybackDevice >= NumOutDevs)
		{
			m_PlaybackDevice = NumOutDevs;
		}

		for (int i = WAVE_MAPPER; i < NumOutDevs; i++)
		{
			WAVEOUTCAPS woc;
			if (MMSYSERR_NOERROR != CWaveOut::GetDevCaps(i, & woc))
			{
				break;
			}

			m_PlaybackDeviceCombo.AddString(woc.szPname);
		}

		int const NumInDevs = CWaveIn::GetNumDevs();

		if (m_RecordingDevice >= NumInDevs)
		{
			m_RecordingDevice = NumInDevs;
		}

		for (int i = WAVE_MAPPER; i < NumInDevs; i++)
		{
			WAVEINCAPS wic;
			if (MMSYSERR_NOERROR != CWaveIn::GetDevCaps(i, & wic))
			{
				break;
			}

			m_RecordingDeviceCombo.AddString(wic.szPname);
		}
	}
	// now can set the combos selection
	DDX_CBIndex(pDX, IDC_COMBO_PLAYBACK_DEVICE, m_PlaybackDevice);
	DDX_CBIndex(pDX, IDC_COMBO_RECORDING_DEVICE, m_RecordingDevice);
}


BEGIN_MESSAGE_MAP(CSoundPreferencesPage, BaseClass)
	//{{AFX_MSG_MAP(CSoundPreferencesPage)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CSoundPreferencesPage message handlers
/////////////////////////////////////////////////////////////////////////////
BOOL CSoundPreferencesPage::OnInitDialog()
{
	BaseClass::OnInitDialog();

	m_SpinPlaybackBufs.SetRange(2, 32);
	m_SpinRecordingBufs.SetRange(2, 32);
	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}
/////////////////////////////////////////////////////////////////////////////
// CViewPreferencesPage property page

IMPLEMENT_DYNCREATE(CViewPreferencesPage, CPropertyPage)

CViewPreferencesPage::CViewPreferencesPage() : BaseClass(CViewPreferencesPage::IDD)
{
	//{{AFX_DATA_INIT(CViewPreferencesPage)
	m_bSnapMouseSelection = FALSE;
	//}}AFX_DATA_INIT
}

CViewPreferencesPage::~CViewPreferencesPage()
{
}

void CViewPreferencesPage::DoDataExchange(CDataExchange* pDX)
{
	BaseClass::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CViewPreferencesPage)
	DDX_Check(pDX, IDC_CHECK_SNAP_MOUSE_SELECTION, m_bSnapMouseSelection);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CViewPreferencesPage, BaseClass)
	//{{AFX_MSG_MAP(CViewPreferencesPage)
		// NOTE: the ClassWizard will add message map macros here
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CViewPreferencesPage message handlers

// CUndoPropertyPage dialog

CUndoPropertyPage::CUndoPropertyPage(UndoRedoParameters const * pParams)
	: BaseClass(IDD, UINT(NULL))
	, UndoRedoParameters(*pParams)
{
}

CUndoPropertyPage::~CUndoPropertyPage()
{
}

void CUndoPropertyPage::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	DDX_Check(pDX, IDC_CHECK_ENABLE_UNDO, m_UndoEnabled);

	DDX_Check(pDX, IDC_CHECK_LIMIT_UNDO_SIZE, m_LimitUndoSize);
	DDX_Text(pDX, IDC_EDIT_UNDO_SIZE_LIMIT, m_UndoSizeLimit);
	DDV_MinMaxUInt(pDX, m_UndoSizeLimit, 1, 4096);

	DDX_Check(pDX, IDC_CHECK_LIMIT_UNDO_DEPTH, m_LimitUndoDepth);
	DDX_Text(pDX, IDC_EDIT_UNDO_DEPTH_LIMIT, m_UndoDepthLimit);
	DDV_MinMaxUInt(pDX, m_UndoDepthLimit, 1, 1000);
	DDX_Control(pDX, IDC_SPIN_UNDO_DEPTH, m_SpinUndoLimit);

	DDX_Check(pDX, IDC_CHECK_ENABLE_REDO, m_RedoEnabled);

	DDX_Check(pDX, IDC_CHECK_LIMIT_REDO_SIZE, m_LimitRedoSize);
	DDX_Text(pDX, IDC_EDIT_REDO_SIZE_LIMIT, m_RedoSizeLimit);
	DDV_MinMaxUInt(pDX, m_RedoSizeLimit, 1, 4096);

	DDX_Check(pDX, IDC_CHECK_LIMIT_REDO_DEPTH, m_LimitRedoDepth);
	DDX_Text(pDX, IDC_EDIT_REDO_DEPTH_LIMIT, m_RedoDepthLimit);
	DDV_MinMaxUInt(pDX, m_RedoDepthLimit, 1, 1000);
	DDX_Control(pDX, IDC_SPIN_REDO_DEPTH, m_SpinRedoLimit);

	DDX_Check(pDX, IDC_CHECK_REMEMBER_SELECTION_IN_UNDO, m_RememberSelectionInUndo);
}

BOOL CUndoPropertyPage::OnInitDialog()
{
	BaseClass::OnInitDialog();

	m_SpinUndoLimit.SetRange(1, 1000);
	m_SpinRedoLimit.SetRange(1, 1000);
	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}


BEGIN_MESSAGE_MAP(CUndoPropertyPage, BaseClass)
	ON_BN_CLICKED(IDC_CHECK_ENABLE_REDO, OnBnClickedCheckEnableRedo)
	ON_BN_CLICKED(IDC_CHECK_ENABLE_UNDO, OnBnClickedCheckEnableUndo)

	ON_BN_CLICKED(IDC_CHECK_LIMIT_UNDO_DEPTH, OnBnClickedCheckLimitUndoDepth)
	ON_BN_CLICKED(IDC_CHECK_LIMIT_UNDO_SIZE, OnBnClickedCheckLimitUndoSize)
	ON_UPDATE_COMMAND_UI(IDC_CHECK_LIMIT_UNDO_DEPTH, OnUpdateCheckLimitUndoDepth)
	ON_UPDATE_COMMAND_UI(IDC_CHECK_LIMIT_UNDO_SIZE, OnUpdateCheckLimitUndoSize)
	ON_UPDATE_COMMAND_UI(IDC_EDIT_UNDO_SIZE_LIMIT, OnUpdateEditLimitUndoSize)
	ON_UPDATE_COMMAND_UI(IDC_EDIT_UNDO_DEPTH_LIMIT, OnUpdateEditLimitUndoDepth)
	ON_UPDATE_COMMAND_UI(IDC_STATIC_UNDO_MB, OnUpdateEditLimitUndoSize)
	ON_UPDATE_COMMAND_UI(IDC_SPIN_UNDO_DEPTH, OnUpdateEditLimitUndoDepth)

	ON_BN_CLICKED(IDC_CHECK_LIMIT_REDO_DEPTH, OnBnClickedCheckLimitRedoDepth)
	ON_BN_CLICKED(IDC_CHECK_LIMIT_REDO_SIZE, OnBnClickedCheckLimitRedoSize)
	ON_UPDATE_COMMAND_UI(IDC_CHECK_LIMIT_REDO_DEPTH, OnUpdateCheckLimitRedoDepth)
	ON_UPDATE_COMMAND_UI(IDC_CHECK_LIMIT_REDO_SIZE, OnUpdateCheckLimitRedoSize)
	ON_UPDATE_COMMAND_UI(IDC_EDIT_REDO_SIZE_LIMIT, OnUpdateEditLimitRedoSize)
	ON_UPDATE_COMMAND_UI(IDC_EDIT_REDO_DEPTH_LIMIT, OnUpdateEditLimitRedoDepth)
	ON_UPDATE_COMMAND_UI(IDC_STATIC_REDO_MB, OnUpdateEditLimitRedoSize)
	ON_UPDATE_COMMAND_UI(IDC_SPIN_REDO_DEPTH, OnUpdateEditLimitRedoDepth)
END_MESSAGE_MAP()

// CUndoPropertyPage message handlers
void CUndoPropertyPage::OnBnClickedCheckEnableRedo()
{
	m_RedoEnabled = IsDlgButtonChecked(IDC_CHECK_ENABLE_REDO);
	NeedUpdateControls();
}

void CUndoPropertyPage::OnBnClickedCheckEnableUndo()
{
	m_UndoEnabled = IsDlgButtonChecked(IDC_CHECK_ENABLE_UNDO);
	NeedUpdateControls();
}

void CUndoPropertyPage::OnBnClickedCheckLimitRedoDepth()
{
	m_LimitRedoDepth = IsDlgButtonChecked(IDC_CHECK_LIMIT_REDO_DEPTH);
	NeedUpdateControls();
}

void CUndoPropertyPage::OnBnClickedCheckLimitRedoSize()
{
	m_LimitRedoSize = IsDlgButtonChecked(IDC_CHECK_LIMIT_REDO_SIZE);
	NeedUpdateControls();
}

void CUndoPropertyPage::OnBnClickedCheckLimitUndoDepth()
{
	m_LimitUndoDepth = IsDlgButtonChecked(IDC_CHECK_LIMIT_UNDO_DEPTH);
	NeedUpdateControls();
}

void CUndoPropertyPage::OnBnClickedCheckLimitUndoSize()
{
	m_LimitUndoSize = IsDlgButtonChecked(IDC_CHECK_LIMIT_UNDO_SIZE);
	NeedUpdateControls();
}

void CUndoPropertyPage::OnUpdateCheckLimitRedoDepth(CCmdUI * pCmdUI)
{
	pCmdUI->Enable(m_RedoEnabled);
}
void CUndoPropertyPage::OnUpdateCheckLimitRedoSize(CCmdUI * pCmdUI)
{
	pCmdUI->Enable(m_RedoEnabled);
}
void CUndoPropertyPage::OnUpdateCheckLimitUndoDepth(CCmdUI * pCmdUI)
{
	pCmdUI->Enable(m_UndoEnabled);
}
void CUndoPropertyPage::OnUpdateCheckLimitUndoSize(CCmdUI * pCmdUI)
{
	pCmdUI->Enable(m_UndoEnabled);
}
void CUndoPropertyPage::OnUpdateEditLimitRedoDepth(CCmdUI * pCmdUI)
{
	pCmdUI->Enable(m_RedoEnabled && m_LimitRedoDepth);
}

void CUndoPropertyPage::OnUpdateEditLimitRedoSize(CCmdUI * pCmdUI)
{
	pCmdUI->Enable(m_RedoEnabled && m_LimitRedoSize);
}

void CUndoPropertyPage::OnUpdateEditLimitUndoDepth(CCmdUI * pCmdUI)
{
	pCmdUI->Enable(m_UndoEnabled && m_LimitUndoDepth);
}

void CUndoPropertyPage::OnUpdateEditLimitUndoSize(CCmdUI * pCmdUI)
{
	pCmdUI->Enable(m_UndoEnabled && m_LimitUndoSize);
}

