// OperationDialogs2.cpp : implementation file
//

#include "stdafx.h"
#include "WaveSoapFront.h"
#include "OperationDialogs2.h"

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
	m_nChannel = -1;
	//}}AFX_DATA_INIT
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
	DDX_Control(pDX, IDC_EDIT_START, m_eStart);
	DDX_Radio(pDX, IDC_RADIO_CHANNEL, m_nChannel);
	DDX_CBIndex(pDX, IDC_COMBO_TIME_FORMAT, m_TimeFormatIndex);
	//}}AFX_DATA_MAP
	m_eLength.ExchangeData(pDX, m_Length);
	m_eStart.ExchangeData(pDX, m_Start);
}


BEGIN_MESSAGE_MAP(CInsertSilenceDialog, CDialog)
	//{{AFX_MSG_MAP(CInsertSilenceDialog)
	ON_CBN_SELCHANGE(IDC_COMBO_TIME_FORMAT, OnSelchangeComboTimeFormat)
	ON_EN_KILLFOCUS(IDC_EDIT_LENGTH, OnKillfocusEditLength)
	ON_EN_KILLFOCUS(IDC_EDIT_START, OnKillfocusEditStart)
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
	// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


void CCdGrabbingDialog::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CCdGrabbingDialog)
	DDX_Control(pDX, IDC_COMBO_DRIVES, m_DrivesCombo);
	DDX_Control(pDX, IDC_LIST_TRACKS, m_lbTracks);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CCdGrabbingDialog, CDialog)
	//{{AFX_MSG_MAP(CCdGrabbingDialog)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

void CCdGrabbingDialog::FillDriveList()
{
	m_NumberOfDrives = 0;
	m_CDDriveSelected = 0;
	for (int letter = 'A'; letter <= 'Z'; letter++)
	{
		CString s;
		s.Format("%c:", letter);
		if (DRIVE_CDROM == GetDriveType(s))
		{
			m_CDDrives[m_NumberOfDrives] = letter;
			m_NumberOfDrives++;
			m_DrivesCombo.AddString(s);
		}
	}
}

void CCdGrabbingDialog::FillTrackList(TCHAR letter)
{
	CString path;
	path.Format("\\\\.\\%c:", letter);
	HANDLE hCD = CreateFile(path,
							GENERIC_READ,
							FILE_SHARE_READ | FILE_SHARE_WRITE,
							NULL,
							OPEN_EXISTING,
							FILE_ATTRIBUTE_NORMAL,
							NULL);
	if (INVALID_HANDLE_VALUE == hCD || NULL == hCD)
	{
		TRACE("Couldn't open CD,error=%d\n",GetLastError());
		m_lbTracks.InsertItem(0, "Invalid drive letter");
		return;
	}
	DWORD dwReturned;
	BOOL res = DeviceIoControl(hCD, IOCTL_CDROM_READ_TOC,
								NULL, 0,
								& m_toc, sizeof m_toc,
								& dwReturned,
								NULL);
	TRACE("Get TOC IoControl returned %x, bytes: %d, First track %d, last track: %d, Length:%02X%02X\n",
		res, dwReturned, m_toc.FirstTrack, m_toc.LastTrack, m_toc.Length[1], m_toc.Length[0]);

	if ( ! res)
	{
		m_lbTracks.InsertItem(0, "No disk in the drive");
		CloseHandle(hCD);
		return;
	}
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
		TRACE("Track %d, addr: %d:%02d.%02d, Adr:%X, Control: %X\n",
			m_toc.TrackData[tr].TrackNumber,
			m_toc.TrackData[tr].Address[1],
			m_toc.TrackData[tr].Address[2],
			m_toc.TrackData[tr].Address[3],
			m_toc.TrackData[tr].Adr,
			m_toc.TrackData[tr].Control);
	}
	CloseHandle(hCD);
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

	m_lbTracks.SetImageList( & ImgList, LVSIL_STATE);
	ImgList.Detach();

	dc1.SelectObject(OldBmp1);
}

BOOL CCdGrabbingDialog::OnInitDialog()
{
	CDialog::OnInitDialog();
	CreateImageList();
	CRect cr;

	GetClientRect( & cr);
	m_PreviousSize.cx = cr.Width();
	m_PreviousSize.cy = cr.Height();
	// init MINMAXINFO
	OnMetricsChange();

	m_lbTracks.GetClientRect( & cr);
	int width = cr.Width() - 2 * GetSystemMetrics(SM_CXVSCROLL);
	int nLengthColumnWidth = m_lbTracks.GetStringWidth(" 00:00 ");
	if (nLengthColumnWidth >  width / 2)
	{
		nLengthColumnWidth = width / 2;
	}
	m_lbTracks.InsertColumn(0, "Track name", LVCFMT_LEFT, width - nLengthColumnWidth, 0);
	m_lbTracks.DeleteColumn(1);
	m_lbTracks.InsertColumn(1, "Length", LVCFMT_LEFT, nLengthColumnWidth, 1);
	FillDriveList();
	FillTrackList(m_CDDrives[m_CDDriveSelected]);

	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

void CCdGrabbingDialog::OnSize(UINT nType, int cx, int cy)
{
	CDialog::OnSize(nType, cx, cy);

	// invalidate an area which is (after resizing) occupied by size grip
	int size = GetSystemMetrics(SM_CXVSCROLL);
	CRect r(cx - size, cy - size, cx, cy);
	InvalidateRect( & r, TRUE);

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
		IDC_CHECK_SINGLE_FILE, IDOK, IDCANCEL,  // move X, Y
		IDC_STATIC_SPEED, IDC_COMBO_SPEED,  // move Y only
	};
	const int NoMoveYItems = 1; // from IDC_ADD_FILE through IDC_AVI_PROPERTIES
	const int MoveXItems = 4;

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

		::GetWindowRect(hWnd, r);
		ScreenToClient(r);
		if (i < MoveXItems)
		{
			r.left += dx;
			r.right += dx;
		}

		if (i >= NoMoveYItems)
		{
			r.top += dy;
			r.bottom += dy;
		}

		hdwp = ::DeferWindowPos(hdwp, hWnd, NULL, r.left, r.top,
								r.Width(), r.Height(),
								SWP_NOZORDER | SWP_NOOWNERZORDER// | SWP_NOACTIVATE | SWP_NOSENDCHANGING
								);
		TRACE("DeferWindowPos hwnd=%x dw=%d dy=%d x=%d, y=%d returned %X\n",
			hWnd, dx, dy, r.left, r.top, hdwp);
		if (NULL == hdwp)
		{
			break;
		}
	}

	if (NULL != hdwp)
	{
		m_lbTracks.GetWindowRect(r);
		ScreenToClient(r);
		hdwp = ::DeferWindowPos(hdwp, m_lbTracks.GetSafeHwnd(), NULL, r.left, r.top, r.Width() + dx, r.Height() + dy,
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
		// NOTE: the ClassWizard will add message map macros here
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CReopenDialog message handlers
