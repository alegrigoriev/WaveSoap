// OperationDialogs2.cpp : implementation file
//

#include "stdafx.h"
#include "WaveSoapFront.h"
#include "OperationDialogs2.h"
#include <dbt.h>
#include <afxpriv.h>

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
	: CDialog(CCdGrabbingDialog::IDD, pParent)
{
	//{{AFX_DATA_INIT(CCdGrabbingDialog)
	m_RadioAssignAttributes = -1;
	m_RadioStoreImmediately = -1;
	m_RadioStoreMultiple = -1;
	//}}AFX_DATA_INIT
	m_RadioAssignAttributes = 0;
	m_RadioStoreImmediately = 0;
	m_RadioStoreMultiple = 0;
	m_DiskID = -1;
	m_PreviousSize.cx = -1;
	m_PreviousSize.cy = -1;
	memzero(m_mmxi);
	m_bNeedUpdateControls = TRUE;

}

CCdGrabbingDialog::~CCdGrabbingDialog()
{
}

void CCdGrabbingDialog::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CCdGrabbingDialog)
	DDX_Control(pDX, IDC_STATIC_FORMAT, m_StaticFormat);
	DDX_Control(pDX, IDC_COMBO_SPEED, m_SpeedCombo);
	DDX_Control(pDX, IDC_COMBO_DRIVES, m_DrivesCombo);
	DDX_Control(pDX, IDC_LIST_TRACKS, m_lbTracks);
	DDX_Radio(pDX, IDC_RADIO_ASSIGN_ATTRIBUTES, m_RadioAssignAttributes);
	DDX_Radio(pDX, IDC_RADIO_STORE_IMMEDIATELY, m_RadioStoreImmediately);
	DDX_Radio(pDX, IDC_RADIO_STORE_MULTIPLE_FILES, m_RadioStoreMultiple);
	//}}AFX_DATA_MAP

	if (pDX->m_bSaveAndValidate)
	{
		// get selected tracks
	}
}

BEGIN_MESSAGE_MAP(CCdGrabbingDialog, CDialog)
	//{{AFX_MSG_MAP(CCdGrabbingDialog)
	ON_WM_ERASEBKGND()
	ON_WM_SIZE()
	ON_WM_SIZING()
	ON_WM_NCHITTEST()
	ON_WM_GETMINMAXINFO()
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
	//}}AFX_MSG_MAP
	ON_WM_DEVICECHANGE()
	ON_MESSAGE(WM_KICKIDLE, OnKickIdle)
END_MESSAGE_MAP()

void CCdGrabbingDialog::FillDriveList(TCHAR SelectDrive)
{
	m_NumberOfDrives = m_CdDrive.FindCdDrives(m_CDDrives);;
	m_DriveLetterSelected = 0;
	m_CDDriveSelected = 0;

	m_DrivesCombo.ResetContent();
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

	if (0 != m_NumberOfDrives)
	{
		m_DrivesCombo.SetCurSel(m_CDDriveSelected);
		m_DriveLetterSelected = m_CDDrives[m_CDDriveSelected];
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
	if ( ! m_CdDrive.Open(letter))
	{
		return FALSE;
	}

	m_CdDrive.DisableMediaChangeDetection();
	return TRUE;
}

void CCdGrabbingDialog::FillTrackList(TCHAR letter)
{
	if ( ! OpenDrive(letter))
	{
		TRACE("Couldn't open CD,error=%d\n",GetLastError());
		m_lbTracks.DeleteAllItems();
		m_lbTracks.InsertItem(0, "Invalid drive letter");
		return;
	}
	ReloadTrackList();
}

void CCdGrabbingDialog::ReloadTrackList()
{
	m_lbTracks.DeleteAllItems();
	BOOL res = m_CdDrive.ReadToc( & m_toc);

	m_bNeedUpdateControls = TRUE;
	if ( ! res)
	{
		m_lbTracks.InsertItem(0, "No disk in the drive");
		memzero(m_toc);
		m_bDiskReady = FALSE;
		return;
	}
	m_bDiskReady = TRUE;
	// Get disk ID
	m_DiskID = m_CdDrive.GetDiskID();

	for (int tr = 0; tr <= m_toc.LastTrack - m_toc.FirstTrack; tr++)
	{
		CString s;
		s.Format("Track %d", m_toc.TrackData[tr].TrackNumber);
		LVITEM item = {
			LVIF_TEXT | LVIF_STATE,
			tr,
			0,
			INDEXTOSTATEIMAGEMASK(2),   // checked
			LVIS_STATEIMAGEMASK,
			LPTSTR(LPCTSTR(s)),
			0, 0, 0, 0};
		m_lbTracks.InsertItem( & item);
		m_lbTracks.SetCheck(tr, TRUE);
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
	CDialog::OnInitDialog();

	m_lbTracks.SetExtendedStyle(LVS_EX_CHECKBOXES);

	CreateImageList();
	CRect cr;
	GetClientRect( & cr);

	m_PreviousSize.cx = cr.Width();
	m_PreviousSize.cy = cr.Height();
	// init MINMAXINFO
	OnMetricsChange();

	m_lbTracks.GetClientRect( & cr);
	int width = cr.Width() - 2 * GetSystemMetrics(SM_CXVSCROLL);
	int nLengthColumnWidth = m_lbTracks.GetStringWidth(" 00:00  ");
	if (nLengthColumnWidth >  width / 2)
	{
		nLengthColumnWidth = width / 2;
	}
	m_lbTracks.InsertColumn(0, "Track name", LVCFMT_LEFT, width - nLengthColumnWidth, 0);
	m_lbTracks.DeleteColumn(1);
	m_lbTracks.InsertColumn(1, "Length", LVCFMT_LEFT, nLengthColumnWidth, 1);

	FillDriveList(0);
	if (m_NumberOfDrives > 0)
	{
		FillTrackList(m_DriveLetterSelected);
	}

	SetTimer(1, 200, NULL);
	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

void CCdGrabbingDialog::OnSize(UINT nType, int cx, int cy)
{
	CDialog::OnSize(nType, cx, cy);

	// invalidate an area which is (after resizing) occupied by size grip
	if (m_PreviousSize.cx < 0)
	{
		m_PreviousSize.cx = cx;
		m_PreviousSize.cy = cy;
		return;
	}

	// reposition controls

	int dx = cx - m_PreviousSize.cx;
	int dy = cy - m_PreviousSize.cy;
	m_PreviousSize.cx = cx;
	m_PreviousSize.cy = cy;

	if (dx == cx      // previous size was unknown
		|| 0 == dx && 0 == dy)
	{
		TRACE("Nothing to do in OnSize\n");
		return;
	}

	static UINT const Controls[] =
	{
		IDC_BUTTON_CDDB,    // move X only
		IDC_CHECK_SINGLE_FILE, IDOK, IDCANCEL, IDC_BUTTON_MORE, // move X, Y
		IDC_STATIC_SPEED, IDC_COMBO_SPEED,  // move Y only
	};
	const int NoMoveYItems = 1; // from IDC_ADD_FILE through IDC_AVI_PROPERTIES
	const int MoveXItems = 5;

	int i;
	HDWP hdwp = ::BeginDeferWindowPos(sizeof Controls/ sizeof Controls[0] + 2);

	if (NULL == hdwp)
	{
		m_PreviousSize.cx -= dx;
		m_PreviousSize.cy -= dy;
		return;
	}

	HWND hWnd;
	for (i = 0; i < sizeof Controls/ sizeof Controls[0]; i++)
	{
		hWnd = ::GetDlgItem(GetSafeHwnd(), Controls[i]);
		if (NULL == hWnd) continue;

		CRect cr;
		::GetWindowRect(hWnd, cr);
		ScreenToClient(cr);
		if (i < MoveXItems)
		{
			cr.left += dx;
			cr.right += dx;
		}

		if (i >= NoMoveYItems)
		{
			cr.top += dy;
			cr.bottom += dy;
		}

		hdwp = ::DeferWindowPos(hdwp, hWnd, NULL, cr.left, cr.top,
								cr.Width(), cr.Height(),
								SWP_NOZORDER | SWP_NOOWNERZORDER// | SWP_NOACTIVATE | SWP_NOSENDCHANGING
								);
		TRACE("DeferWindowPos hwnd=%x dw=%d dy=%d x=%d, y=%d returned %X\n",
			hWnd, dx, dy, cr.left, cr.top, hdwp);
		if (NULL == hdwp)
		{
			break;
		}
	}

	if (NULL != hdwp
		&& m_lbTracks.m_hWnd != NULL)
	{
		CRect cr;
		m_lbTracks.GetWindowRect(cr);
		ScreenToClient(cr);
		hdwp = ::DeferWindowPos(hdwp, m_lbTracks.GetSafeHwnd(), NULL,
								cr.left, cr.top, cr.Width() + dx, cr.Height() + dy,
								SWP_NOZORDER | SWP_NOOWNERZORDER//|SWP_NOACTIVATE | SWP_NOSENDCHANGING
								);
		m_lbTracks.SetColumnWidth(0, m_lbTracks.GetColumnWidth(0) + dx);

		TRACE("Calling ::EndDeferWindowPos(hdwp)\n");
		::EndDeferWindowPos(hdwp);
	}
	else
	{
		m_PreviousSize.cx -= dx;
		m_PreviousSize.cy -= dy;
	}
	// invalidate an area which is (after resizing)
	// occupied by size grip
	int size = GetSystemMetrics(SM_CXVSCROLL);
	CRect r(cx - size, cy - size, cx, cy);
	InvalidateRect( & r, TRUE);
}

void CCdGrabbingDialog::OnSizing(UINT fwSide, LPRECT pRect)
{
	CDialog::OnSizing(fwSide, pRect);

	// invalidate an area currently (before resizing) occupied by size grip
	CRect r;
	GetClientRect( & r);
	int size = GetSystemMetrics(SM_CXVSCROLL);
	r.left = r.right - size;
	r.top = r.bottom - size;
	InvalidateRect( & r, FALSE);
}

UINT CCdGrabbingDialog::OnNcHitTest(CPoint point)
{
	// return HTBOTTOMRIGHT for sizegrip area
	CRect r;
	GetClientRect( & r);
	int size = GetSystemMetrics(SM_CXVSCROLL);
	r.left = r.right - size;
	r.top = r.bottom - size;
	CPoint p(point);
	ScreenToClient( & p);

	if (r.PtInRect(p))
	{
		return HTBOTTOMRIGHT;
	}
	else
		return CDialog::OnNcHitTest(point);
}

void CCdGrabbingDialog::OnGetMinMaxInfo(MINMAXINFO FAR* lpMMI)
{
	if (m_mmxi.ptMaxSize.x != 0)
	{
		*lpMMI = m_mmxi;
	}
	else
	{
		CDialog::OnGetMinMaxInfo(lpMMI);
	}
}

void CCdGrabbingDialog::OnMetricsChange()
{
	// Initialize MINMAXINFO
	CRect r;
	SystemParametersInfo(SPI_GETWORKAREA, 0, & r, 0);
	m_mmxi.ptMaxSize.x = r.Width();
	m_mmxi.ptMaxTrackSize.x = m_mmxi.ptMaxSize.x;
	m_mmxi.ptMaxSize.y = r.Height();
	m_mmxi.ptMaxTrackSize.y = m_mmxi.ptMaxSize.y;
	m_mmxi.ptMaxPosition.x = r.left;
	m_mmxi.ptMaxPosition.y = r.top;
	GetWindowRect(& r);
	m_mmxi.ptMinTrackSize.x = r.Width();
	m_mmxi.ptMinTrackSize.y = r.Height();
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

BOOL CCdGrabbingDialog::OnEraseBkgnd(CDC* pDC)
{
	if (CDialog::OnEraseBkgnd(pDC))
	{
		// draw size grip
		CRect r;
		GetClientRect( & r);
		int size = GetSystemMetrics(SM_CXVSCROLL);
		r.left = r.right - size;
		r.top = r.bottom - size;
		pDC->DrawFrameControl( & r, DFC_SCROLL, DFCS_SCROLLSIZEGRIP);
		return TRUE;
	}
	else
	{
		return FALSE;
	}
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
		break;
	case CdMediaStateSameMedia:
		return;
		break;
	case CdMediaStateChanged:
		ReloadTrackList();
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
	CDialog::OnDestroy();

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
	// TODO: Add your control notification handler code here

}

void CCdGrabbingDialog::OnButtonCddb()
{
	// TODO: Add your control notification handler code here

}

void CCdGrabbingDialog::OnButtonDeselectAll()
{
	if (0 == m_toc.Length[0]
		&& 0 == m_toc.Length[1])
	{
		return;
	}
	for (int tr = 0; tr <= m_toc.LastTrack - m_toc.FirstTrack; tr++)
	{
		m_lbTracks.SetCheck(tr, FALSE);
	}
	m_bNeedUpdateControls = TRUE;
}

void CCdGrabbingDialog::OnButtonSelectAll()
{
	if (0 == m_toc.Length[0]
		&& 0 == m_toc.Length[1])
	{
		return;
	}
	for (int tr = 0; tr <= m_toc.LastTrack - m_toc.FirstTrack; tr++)
	{
		m_lbTracks.SetCheck(tr, TRUE);
	}
	m_bNeedUpdateControls = TRUE;
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
