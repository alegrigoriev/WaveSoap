// Copyright Alexander Grigoriev, 1997-2002, All Rights Reserved
// OperationDialogs.cpp : implementation file
//

#include "stdafx.h"
#include "WaveSoapFront.h"
#include "resource.h"
#include "OperationDialogs.h"
#include "MainFrm.h"
#include "SaveExpressionDialog.h"
#include <afxpriv.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


CDialogWithSelection::CDialogWithSelection(SAMPLE_INDEX Start,
											SAMPLE_INDEX End, SAMPLE_INDEX CaretPos,
											CHANNEL_MASK Channel,
											CWaveFile & File, int TimeFormat,
											UINT TemplateID,
											CWnd* pParent)
	: BaseClass(TemplateID, pParent)
	, m_Start(Start)
	, m_End(End)
	, m_CaretPosition(CaretPos)
	, m_Chan(Channel)
	, m_WaveFile(File)
	, m_TimeFormat(TimeFormat)
	, m_bUndo(FALSE)
	, m_bLockChannels(FALSE)
{
}

BEGIN_MESSAGE_MAP(CDialogWithSelection, BaseClass)
	//{{AFX_MSG_MAP(CDialogWithSelection)
	ON_BN_CLICKED(IDC_CHECKLOCK_CHANNELS, OnChecklockChannels)
	ON_BN_CLICKED(IDC_BUTTON_SELECTION, OnButtonSelection)
	ON_UPDATE_COMMAND_UI(IDC_STATIC_SELECTION, OnUpdateSelectionStatic)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

void CDialogWithSelection::OnButtonSelection()
{
	CSelectionDialog dlg(m_Start, m_End, m_CaretPosition, m_Chan + 1,
						m_WaveFile.NumberOfSamples(), m_WaveFile.GetWaveFormat(), m_TimeFormat);

	if (IDOK != dlg.DoModal())
	{
		return;
	}

	m_Start = dlg.GetStart();
	m_End = dlg.GetEnd();
	m_Chan = dlg.GetChannel() - 1;

	NeedUpdateControls();
}

void CDialogWithSelection::OnChecklockChannels()
{
	m_bLockChannels = IsDlgButtonChecked(IDC_CHECKLOCK_CHANNELS);
	NeedUpdateControls();
}

void CDialogWithSelection::OnUpdateSelectionStatic(CCmdUI * pCmdUI)
{
	pCmdUI->SetText(GetSelectionText(m_Start, m_End, m_Chan,
									m_WaveFile.Channels(), m_bLockChannels,
									m_WaveFile.SampleRate(), m_TimeFormat));
}

void CDialogWithSelection::DoDataExchange(CDataExchange* pDX)
{
	BaseClass::DoDataExchange(pDX);
	// initialize slider control dirst, then edit control
	//{{AFX_DATA_MAP(CDialogWithSelection)
	//}}AFX_DATA_MAP
	// get data first, then get new selection of the combobox
	if ( ! pDX->m_bSaveAndValidate)
	{
		NeedUpdateControls();
	}
}

/////////////////////////////////////////////////////////////////////////////
// CCopyChannelsSelectDlg dialog


CCopyChannelsSelectDlg::CCopyChannelsSelectDlg(CHANNEL_MASK Channels, CWnd* pParent /*=NULL*/)
	: BaseClass(CCopyChannelsSelectDlg::IDD, pParent)
	, m_ChannelToCopy(Channels + 1)
{
	//{{AFX_DATA_INIT(CCopyChannelsSelectDlg)
	//}}AFX_DATA_INIT
}

void CCopyChannelsSelectDlg::DoDataExchange(CDataExchange* pDX)
{
	BaseClass::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CCopyChannelsSelectDlg)
	DDX_Radio(pDX, IDC_RADIO_CHANNEL_BOTH, m_ChannelToCopy);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CCopyChannelsSelectDlg, BaseClass)
//{{AFX_MSG_MAP(CCopyChannelsSelectDlg)
// NOTE: the ClassWizard will add message map macros here
//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CCopyChannelsSelectDlg message handlers
/////////////////////////////////////////////////////////////////////////////
// CPasteModeDialog dialog


CPasteModeDialog::CPasteModeDialog(int PasteMode, CWnd* pParent /*=NULL*/)
	: BaseClass(CPasteModeDialog::IDD, pParent),
	m_PasteMode(PasteMode)
{
	//{{AFX_DATA_INIT(CPasteModeDialog)
	//}}AFX_DATA_INIT
}


void CPasteModeDialog::DoDataExchange(CDataExchange* pDX)
{
	BaseClass::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CPasteModeDialog)
	DDX_Radio(pDX, IDC_RADIO_SELECT, m_PasteMode);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CPasteModeDialog, BaseClass)
//{{AFX_MSG_MAP(CPasteModeDialog)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CPasteModeDialog message handlers

/////////////////////////////////////////////////////////////////////////////
// CVolumeChangeDialog dialog


CVolumeChangeDialog::CVolumeChangeDialog(SAMPLE_INDEX begin, SAMPLE_INDEX end, SAMPLE_INDEX caret,
										CHANNEL_MASK Channels,
										CWaveFile & File,
										BOOL ChannelsLocked, BOOL UndoEnabled,
										int TimeFormat,
										CWnd* pParent /*=NULL*/)
	: BaseClass(begin, end, caret, Channels, File, TimeFormat,
				IDD, pParent)
	, m_dVolumeLeftDb(0.)
	, m_dVolumeRightDb(0.)
	, m_dVolumeLeftPercent(100.)
	, m_dVolumeRightPercent(100.)
{
	m_bUndo = UndoEnabled;
	m_bLockChannels = ChannelsLocked;

	//{{AFX_DATA_INIT(CVolumeChangeDialog)
	//}}AFX_DATA_INIT

	if (1 == m_WaveFile.Channels())
	{
		m_lpszTemplateName = MAKEINTRESOURCE(IDD_DIALOG_VOLUME_CHANGE_MONO);
	}

	// m_DbPercent: 0 = Decibel, 1 - Percent
	m_Profile.AddItem(_T("Settings"), _T("VolumeDialogDbPercents"), m_DbPercent, 0, 0, 1);
	m_Profile.AddItem(_T("Settings"), _T("VolumeLeftDb"), m_dVolumeLeftDb, 0., -40., 40.);
	m_Profile.AddItem(_T("Settings"), _T("VolumeLeftPercent"), m_dVolumeLeftPercent, 100., 1., 10000.);
	m_Profile.AddItem(_T("Settings"), _T("VolumeRightDb"), m_dVolumeRightDb, 0., -40., 40.);
	m_Profile.AddItem(_T("Settings"), _T("VolumeRightPercent"), m_dVolumeRightPercent, 100., 1., 10000.);
}

double CVolumeChangeDialog::GetLeftVolume()
{
	if (0 == m_DbPercent)   // dBs
	{
		return pow(10., m_dVolumeLeftDb / 20.);
	}
	else // percents
	{
		return 0.01 * m_dVolumeLeftPercent;
	}
}

double CVolumeChangeDialog::GetRightVolume()
{
	if (0 == m_DbPercent)   // dBs
	{
		if (m_bLockChannels || m_WaveFile.Channels() == 1)
		{
			return pow(10., m_dVolumeLeftDb / 20.);
		}
		else
		{
			return pow(10., m_dVolumeRightDb / 20.);
		}
	}
	else // percents
	{
		if (m_bLockChannels || m_WaveFile.Channels() == 1)
		{
			return 0.01 * m_dVolumeLeftPercent;
		}
		else
		{
			return 0.01 * m_dVolumeRightPercent;
		}
	}

}

void CVolumeChangeDialog::UpdateVolumeData(CDataExchange* pDX, BOOL InPercents)
{
	if (0 == InPercents)
	{
		// decibels
		m_eVolumeLeft.ExchangeData(pDX, m_dVolumeLeftDb,
									_T("Volume change"), _T("dB"), -40., 40.);
		if (m_WaveFile.Channels() > 1)
		{
			m_eVolumeRight.ExchangeData(pDX, m_dVolumeRightDb,
										_T("Volume change"), _T("dB"), -40., 40.);
		}
	}
	else
	{
		// percents
		m_eVolumeLeft.ExchangeData(pDX, m_dVolumeLeftPercent,
									_T("Volume change"), _T("%"), 1., 10000.);
		if (m_WaveFile.Channels() > 1)
		{
			m_eVolumeRight.ExchangeData(pDX, m_dVolumeRightPercent,
										_T("Volume change"), _T("%"), 1., 10000.);
		}
	}
}

void CVolumeChangeDialog::DoDataExchange(CDataExchange* pDX)
{
	BaseClass::DoDataExchange(pDX);
	// initialize slider control dirst, then edit control
	//{{AFX_DATA_MAP(CVolumeChangeDialog)
	DDX_Control(pDX, IDC_SLIDER_VOLUME_LEFT, m_SliderVolumeLeft);
	DDX_Control(pDX, IDC_EDIT_VOLUME_LEFT, m_eVolumeLeft);
	DDX_Check(pDX, IDC_CHECK_UNDO, m_bUndo);
	//}}AFX_DATA_MAP
	// get data first, then get new selection of the combobox
	if (m_WaveFile.Channels() > 1)
	{
		DDX_Check(pDX, IDC_CHECKLOCK_CHANNELS, m_bLockChannels);
		DDX_Control(pDX, IDC_SLIDER_VOLUME_RIGHT, m_SliderVolumeRight);
		DDX_Control(pDX, IDC_EDIT_VOLUME_RIGHT, m_eVolumeRight);
	}

	UpdateVolumeData(pDX, m_DbPercent);
	DDX_CBIndex(pDX, IDC_COMBODB_PERCENT, m_DbPercent);
	if ( ! pDX->m_bSaveAndValidate)
	{
		NeedUpdateControls();
	}
	else
	{
		m_Profile.FlushAll();
	}
}


BEGIN_MESSAGE_MAP(CVolumeChangeDialog, BaseClass)
//{{AFX_MSG_MAP(CVolumeChangeDialog)
	ON_CBN_SELCHANGE(IDC_COMBODB_PERCENT, OnSelchangeCombodbPercent)
	ON_WM_HSCROLL()
	ON_EN_KILLFOCUS(IDC_EDIT_VOLUME_LEFT, OnKillfocusEditVolumeLeft)
	ON_EN_KILLFOCUS(IDC_EDIT_VOLUME_RIGHT, OnKillfocusEditVolumeRight)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CVolumeChangeDialog message handlers

void CVolumeChangeDialog::UpdateEnables()
{
	BOOL EnableLeft = m_bLockChannels || 1 != m_Chan;
	BOOL EnableRight = ! m_bLockChannels && 0 != m_Chan;

	m_eVolumeLeft.EnableWindow(EnableLeft);
	m_SliderVolumeLeft.EnableWindow(EnableLeft);
	GetDlgItem(IDC_STATIC_LEFT_CHANNEL)->EnableWindow(EnableLeft);

	m_eVolumeRight.EnableWindow(EnableRight);
	m_SliderVolumeRight.EnableWindow(EnableRight);
	GetDlgItem(IDC_STATIC_RIGHT_CHANNEL)->EnableWindow(EnableRight);
}

void CVolumeChangeDialog::OnSelchangeCombodbPercent()
{
	int sel = ((CComboBox *)GetDlgItem(IDC_COMBODB_PERCENT))->GetCurSel();
	TRACE("OnSelchangeCombodbPercent() sel=%d\n", sel);
	if (sel == m_DbPercent)
	{
		return;
	}
	UpdateData();
	if (0 == sel)
	{
		// get percent values and convert to dB values
		if (m_dVolumeLeftPercent > 0)
		{
			m_dVolumeLeftDb = 20 * log10(0.01 * m_dVolumeLeftPercent);
			if (m_dVolumeLeftDb < -40.)
			{
				m_dVolumeLeftDb = -40.;
			}
			if (m_dVolumeLeftDb > 40.)
			{
				m_dVolumeLeftDb = 40.;
			}
		}
		else
		{
			m_dVolumeLeftDb = 0;
		}

		if (m_dVolumeRightPercent > 0)
		{
			m_dVolumeRightDb = 20 * log10(m_dVolumeRightPercent / 100.);
			if (m_dVolumeRightDb < -40.)
			{
				m_dVolumeRightDb = -40.;
			}
			else if (m_dVolumeRightDb > 40.)
			{
				m_dVolumeRightDb = 40.;
			}
		}
		else
		{
			m_dVolumeRightDb = 0;
		}
	}
	else
	{
		m_dVolumeLeftPercent = 100. * pow(10., m_dVolumeLeftDb / 20.);
		if (m_dVolumeLeftPercent < 1.)
		{
			m_dVolumeLeftPercent = 1.;
		}
		else if (m_dVolumeLeftPercent > 10000.)
		{
			m_dVolumeLeftPercent = 10000.;
		}
		m_dVolumeRightPercent = 100. * pow(10., m_dVolumeRightDb / 20.);
		if (m_dVolumeRightPercent < 1.)
		{
			m_dVolumeRightPercent = 1.;
		}
		else if (m_dVolumeRightPercent > 10000.)
		{
			m_dVolumeRightPercent = 10000.;
		}
	}
	UpdateData(FALSE);
}

/////////////////////////////////////////////////////////////////////////////
// CSelectionDialog dialog


CSelectionDialog::CSelectionDialog(SAMPLE_INDEX Start, SAMPLE_INDEX End,
									SAMPLE_INDEX CaretPos, CHANNEL_MASK Channel,
									NUMBER_OF_SAMPLES TotalSamples,
									const WAVEFORMATEX * pWf, int TimeFormat,
									CWnd* pParent /*=NULL*/)
	: BaseClass(CSelectionDialog::IDD, pParent)
	, m_Chan(Channel)
	, m_Start(Start)
	, m_End(End)
	, m_CaretPosition(CaretPos)
	, m_Length(End - Start)
	, m_FileLength(TotalSamples)
	, m_pWf(pWf)
	, m_TimeFormat(TimeFormat)
	, m_eStart(TimeFormat)
	, m_eEnd(TimeFormat)
	, m_eLength(TimeFormat)

{
	if (m_pWf->nChannels < 2)
	{
		m_lpszTemplateName = MAKEINTRESOURCE(IDD_SELECTION_DIALOG_MONO);
	}
	//{{AFX_DATA_INIT(CSelectionDialog)
	m_TimeFormatIndex = 0;
	m_SelectionNumber = 0;
	//}}AFX_DATA_INIT
	switch (TimeFormat & SampleToString_Mask)
	{
	case SampleToString_Sample:
		m_TimeFormatIndex = 0;
		break;
	case SampleToString_HhMmSs:
		m_TimeFormatIndex = 1;
		break;
	case SampleToString_Seconds:
	default:
		m_TimeFormatIndex = 2;
		break;
	}

	if (NULL != pWf)
	{
		m_eStart.SetSamplingRate(pWf->nSamplesPerSec);
		m_eEnd.SetSamplingRate(pWf->nSamplesPerSec);
		m_eLength.SetSamplingRate(pWf->nSamplesPerSec);
	}
}


void CSelectionDialog::DoDataExchange(CDataExchange* pDX)
{
	BaseClass::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CSelectionDialog)
	DDX_Control(pDX, IDC_COMBO_SELECTION, m_SelectionCombo);
	DDX_Control(pDX, IDC_SPIN_START, m_SpinStart);
	DDX_Control(pDX, IDC_SPIN_LENGTH, m_SpinLength);
	DDX_Control(pDX, IDC_SPIN_END, m_SpinEnd);
	DDX_Control(pDX, IDC_EDIT_LENGTH, m_eLength);
	DDX_Control(pDX, IDC_COMBO_START, m_eStart);
	DDX_Control(pDX, IDC_COMBO_END, m_eEnd);
	DDX_CBIndex(pDX, IDC_COMBO_TIME_FORMAT, m_TimeFormatIndex);
	DDX_CBIndex(pDX, IDC_COMBO_SELECTION, m_SelectionNumber);
	//}}AFX_DATA_MAP
	if (m_pWf->nChannels >= 2)
	{
		DDX_Radio(pDX, IDC_RADIO_CHANNEL, m_Chan);
	}
	m_eStart.ExchangeData(pDX, m_Start);
	m_eEnd.ExchangeData(pDX, m_End);
	m_eLength.ExchangeData(pDX, m_Length);
}


BEGIN_MESSAGE_MAP(CSelectionDialog, BaseClass)
//{{AFX_MSG_MAP(CSelectionDialog)
	ON_CBN_SELCHANGE(IDC_COMBO_TIME_FORMAT, OnSelchangeComboTimeFormat)
	ON_CBN_KILLFOCUS(IDC_COMBO_END, OnKillfocusEditEnd)
	ON_EN_KILLFOCUS(IDC_EDIT_LENGTH, OnKillfocusEditLength)
	ON_CBN_KILLFOCUS(IDC_COMBO_START, OnKillfocusEditStart)
	ON_CBN_SELCHANGE(IDC_COMBO_SELECTION, OnSelchangeComboSelection)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CSelectionDialog message handlers

BOOL CVolumeChangeDialog::OnInitDialog()
{
	BaseClass::OnInitDialog();

	m_SliderVolumeLeft.SetRange(-400, 400);
	m_SliderVolumeLeft.SetTicFreq(100);
	m_SliderVolumeLeft.SetLineSize(10);
	m_SliderVolumeLeft.SetPageSize(50);

	if (0 == m_DbPercent)
	{
		// decibel
		m_SliderVolumeLeft.SetPos(int(m_dVolumeLeftDb * 10.));
	}
	else
	{
		m_SliderVolumeLeft.SetPos(int(200. * log10(m_dVolumeLeftPercent / 100.)));
	}

	if (m_WaveFile.Channels() > 1)
	{
		m_SliderVolumeRight.SetRange(-400, 400);
		m_SliderVolumeRight.SetTicFreq(100);
		m_SliderVolumeRight.SetLineSize(10);
		m_SliderVolumeRight.SetPageSize(50);
		if (0 == m_DbPercent)
		{
			// decibel
			m_SliderVolumeRight.SetPos(int(m_dVolumeLeftDb * 10.));
		}
		else
		{
			m_SliderVolumeRight.SetPos(int(200. * log10(m_dVolumeLeftPercent / 100.)));
		}
		UpdateEnables();
	}
	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

void CVolumeChangeDialog::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
	BaseClass::OnHScroll(nSBCode, nPos, pScrollBar);
	CSliderCtrl * pSlider = dynamic_cast<CSliderCtrl *>(pScrollBar);
	if (NULL != pSlider)
	{
		int pos = pSlider->GetPos();
		CString s;
		if (0 == m_DbPercent)
		{
			s.Format(_T("%.2f"), pos / 10.);
		}
		else
		{
			s.Format(_T("%.0f"), 100. * pow(10., pos / 200.));
		}
		int id = pSlider->GetDlgCtrlID();
		if (IDC_SLIDER_VOLUME_LEFT == id)
		{
			SetDlgItemText(IDC_EDIT_VOLUME_LEFT, s);
			if (2 == m_WaveFile.Channels()
				&& m_bLockChannels)
			{
				SetDlgItemText(IDC_EDIT_VOLUME_RIGHT, s);
				m_SliderVolumeRight.SetPos(pos);
			}
		}
		else if (IDC_SLIDER_VOLUME_RIGHT == id)
		{
			SetDlgItemText(IDC_EDIT_VOLUME_RIGHT, s);
		}
	}
}

void CVolumeChangeDialog::OnKillfocusEditVolumeLeft()
{
	double num;
	CString s;
	m_eVolumeLeft.GetWindowText(s);
	int SliderPos = 0;
	if (! s.IsEmpty()
		&& m_eVolumeLeft.SimpleFloatParse(s, num))
	{
		if (0 == m_DbPercent)
		{
			// decibel
			if (num < -40. || num > 40.)
			{
				return;
			}
			SliderPos = int(num * 10.);
		}
		else
		{
			// percent
			if (num < 1.
				|| num > 10000.)
			{
				return;
			}
			SliderPos = int(200. * log10(num / 100.));
		}
		m_SliderVolumeLeft.SetPos(SliderPos);
	}
}

void CVolumeChangeDialog::OnKillfocusEditVolumeRight()
{
	double num;
	CString s;
	m_eVolumeRight.GetWindowText(s);
	int SliderPos = 0;
	if (! s.IsEmpty()
		&& m_eVolumeRight.SimpleFloatParse(s, num))
	{
		if (0 == m_DbPercent)
		{
			// decibel
			if (num < -40. || num > 40.)
			{
				return;
			}
			SliderPos = int(num * 10.);
		}
		else
		{
			// percent
			if (num < 1.
				|| num > 10000.)
			{
				return;
			}
			SliderPos = int(200. * log10(num / 100.));
		}
		m_SliderVolumeRight.SetPos(SliderPos);
	}
}

BOOL CSelectionDialog::OnInitDialog()
{
	BaseClass::OnInitDialog();

	m_eStart.AddPosition(IDS_BEGIN_OF_SAMPLE, 0);
	m_eEnd.AddPosition(IDS_BEGIN_OF_SAMPLE, 0);

	// TODO: add markers
	((CComboBox*) & m_eStart)->SetExtendedUI(TRUE);
	((CComboBox*) & m_eEnd)->SetExtendedUI(TRUE);

	if ((0 != m_Start || m_FileLength != m_End)
		&& (0 != m_Start || m_CaretPosition != m_End)
		&& (m_CaretPosition != m_Start || m_FileLength != m_End))
	{
		AddSelection(IDS_CURRENT_SELECTION, m_Start, m_End);
	}

	AddSelection(IDS_ALL_SAMPLE_DATA, 0, m_FileLength);

	if (0 != m_CaretPosition
		&& m_FileLength != m_CaretPosition)
	{
		AddSelection(IDS_FROM_BEGIN_TO_CURSOR, 0, m_CaretPosition);

		AddSelection(IDS_FROM_CURSOR_TO_END, m_CaretPosition, m_FileLength);

		m_eStart.AddPosition(IDS_CURSOR, m_CaretPosition);
		m_eEnd.AddPosition(IDS_CURSOR, m_CaretPosition);
	}

	m_eStart.AddPosition(IDS_END_OF_SAMPLE, m_FileLength);
	m_eEnd.AddPosition(IDS_END_OF_SAMPLE, m_FileLength);

	m_SelectionCombo.SetCurSel(FindSelection(m_Start, m_End));
	// TODO: add regions
	return TRUE;  // return TRUE unless you set the focus to a control
}

void CSelectionDialog::OnSelchangeComboTimeFormat()
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

	m_Start = m_eStart.GetTimeSample();
	m_eStart.SetTimeFormat(Format);
	m_eStart.SetTimeSample(m_Start);

	m_End = m_eEnd.GetTimeSample();
	m_eEnd.SetTimeFormat(Format);
	m_eEnd.SetTimeSample(m_End);

	m_Length = m_End - m_Start;
	m_eLength.SetTimeFormat(Format);
	m_eLength.SetTimeSample(m_Length);
}

void CSelectionDialog::OnKillfocusEditEnd()
{
	m_End = m_eEnd.GetTimeSample();
	m_eEnd.SetTimeSample(m_End);
	m_Length = m_End - m_Start;

	if (m_Length < 0) m_Length = 0;
	m_eLength.SetTimeSample(m_Length);
}

void CSelectionDialog::OnKillfocusEditLength()
{
	m_Length = m_eLength.GetTimeSample();
	m_eLength.SetTimeSample(m_Length);
	m_End = m_Start + m_Length;
	m_eEnd.SetTimeSample(m_End);
}

void CSelectionDialog::OnKillfocusEditStart()
{
	m_Start = m_eStart.GetTimeSample();
	m_eStart.SetTimeSample(m_Start);
	m_Length = m_End - m_Start;
	if (m_Length < 0) m_Length = 0;
	m_eLength.SetTimeSample(m_Length);
}

void CSelectionDialog::OnSelchangeComboSelection()
{
	unsigned sel = m_SelectionCombo.GetCurSel();

	if (sel < m_Selections.size())
	{
		m_Start = m_Selections[sel].begin;
		m_eStart.SetTimeSample(m_Start);
		m_End = m_Selections[sel].end;

		m_eEnd.SetTimeSample(m_End);
		m_Length = m_End - m_Start;

		if (m_Length < 0) m_Length = 0;
		m_eLength.SetTimeSample(m_Length);
	}
}

void CSelectionDialog::AddSelection(LPCTSTR Name, SAMPLE_INDEX begin, SAMPLE_INDEX end)
{
	m_SelectionCombo.AddString(Name);
	Selection s = {begin, end};
	m_Selections.push_back(s);
}

void CSelectionDialog::AddSelection(UINT id, SAMPLE_INDEX begin, SAMPLE_INDEX end)
{
	CString s;
	s.LoadString(id);
	AddSelection(s, begin, end);
}

int CSelectionDialog::FindSelection(SAMPLE_INDEX begin, SAMPLE_INDEX end)
{
	for (unsigned i = 0; i < m_Selections.size(); i++)
	{
		if (m_Selections[i].begin == begin
			&& m_Selections[i].end == end)
		{
			return i;
		}
	}
	return -1;
}

/////////////////////////////////////////////////////////////////////////////
// CGotoDialog dialog


CGotoDialog::CGotoDialog(SAMPLE_INDEX Position,
						NUMBER_OF_SAMPLES FileLength,
						const WAVEFORMATEX * pWf,
						int TimeFormat, CWnd* pParent /*=NULL*/)
	: BaseClass(CGotoDialog::IDD, pParent),
	m_Position(Position),
	m_FileLength(FileLength),
	m_TimeFormat(TimeFormat),
	m_eStart(TimeFormat)
{
	//{{AFX_DATA_INIT(CGotoDialog)
	m_TimeFormatIndex = -1;
	//}}AFX_DATA_INIT
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
	if (NULL != pWf)
	{
		m_eStart.SetSamplingRate(pWf->nSamplesPerSec);
	}
}


void CGotoDialog::DoDataExchange(CDataExchange* pDX)
{
	BaseClass::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CGotoDialog)
	DDX_Control(pDX, IDC_SPIN_START, m_StartSpin);
	DDX_Control(pDX, IDC_COMBO_START, m_eStart);
	DDX_CBIndex(pDX, IDC_COMBO_TIME_FORMAT, m_TimeFormatIndex);
	//}}AFX_DATA_MAP
	m_eStart.ExchangeData(pDX, m_Position);
}


BEGIN_MESSAGE_MAP(CGotoDialog, BaseClass)
	//{{AFX_MSG_MAP(CGotoDialog)
	ON_CBN_SELCHANGE(IDC_COMBO_TIME_FORMAT, OnSelchangeComboTimeFormat)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CGotoDialog message handlers
/////////////////////////////////////////////////////////////////////////////
// CDcOffsetDialog dialog


CDcOffsetDialog::CDcOffsetDialog(SAMPLE_INDEX begin, SAMPLE_INDEX end, SAMPLE_INDEX caret,
								CHANNEL_MASK Channels,
								CWaveFile & File,
								BOOL ChannelsLocked, BOOL UndoEnabled,
								int TimeFormat,
								CWnd* pParent /*=NULL*/)
	: BaseClass(begin, end, caret, Channels, File, TimeFormat,
				IDD, pParent)
{
	//{{AFX_DATA_INIT(CDcOffsetDialog)
	m_b5SecondsDC = FALSE;
	m_nDcOffset = 0;
	m_DcSelectMode = -1;
	//}}AFX_DATA_INIT
	// DC offset parameters:
	m_Profile.AddBoolItem(_T("Settings"), _T("5SecondsDC"), m_b5SecondsDC, TRUE);
	m_Profile.AddItem(_T("Settings"), _T("DcOffsetSelectMode"), m_DcSelectMode, 0, 0, 1);
	m_Profile.AddItem(_T("Settings"), _T("DcOffset"), m_nDcOffset, 0, -32767, 32767);

}


void CDcOffsetDialog::DoDataExchange(CDataExchange* pDX)
{
	BaseClass::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CDcOffsetDialog)
	DDX_Control(pDX, IDC_SPIN1, m_OffsetSpin);
	DDX_Check(pDX, IDC_CHECK_5SECONDS, m_b5SecondsDC);
	DDX_Check(pDX, IDC_CHECK_UNDO, m_bUndo);
	DDX_Text(pDX, IDC_EDIT_DC_OFFSET, m_nDcOffset);
	DDX_Radio(pDX, IDC_RADIO_DC_SELECT, m_DcSelectMode);
	//}}AFX_DATA_MAP
	if (0 == m_DcSelectMode)
	{
		GetDlgItem(IDC_EDIT_DC_OFFSET)->EnableWindow(FALSE);
		//GetDlgItem(IDC_EDIT_DC_OFFSET)->EnableWindow(FALSE);
	}
	if (pDX->m_bSaveAndValidate)
	{
		m_Profile.FlushAll();
	}
	else
	{
		NeedUpdateControls();
	}
}

BEGIN_MESSAGE_MAP(CDcOffsetDialog, BaseClass)
	//{{AFX_MSG_MAP(CDcOffsetDialog)
	ON_BN_CLICKED(IDC_RADIO_DC_SELECT, OnRadioDcSelect)
	ON_BN_CLICKED(IDC_RADIO2, OnRadioAdjustSelectEdit)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CDcOffsetDialog message handlers

void CDcOffsetDialog::OnRadioDcSelect()
{
	GetDlgItem(IDC_EDIT_DC_OFFSET)->EnableWindow(FALSE);
}

void CDcOffsetDialog::OnRadioAdjustSelectEdit()
{
	GetDlgItem(IDC_EDIT_DC_OFFSET)->EnableWindow(TRUE);
}
/////////////////////////////////////////////////////////////////////////////
// CStatisticsDialog dialog


CStatisticsDialog::CStatisticsDialog(CWnd* pParent /*=NULL*/)
	: BaseClass(CStatisticsDialog::IDD, pParent)
{
	//{{AFX_DATA_INIT(CStatisticsDialog)
	// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


void CStatisticsDialog::DoDataExchange(CDataExchange* pDX)
{
	BaseClass::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CStatisticsDialog)
	DDX_Control(pDX, IDC_STATIC_FILE_NAME, m_FileName);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CStatisticsDialog, BaseClass)
	//{{AFX_MSG_MAP(CStatisticsDialog)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CStatisticsDialog message handlers

BOOL CStatisticsDialog::OnInitDialog()
{
	BaseClass::OnInitDialog();

	CString s;

	s.Format(_T("File: %s"), LPCTSTR(m_sFilename));
	m_FileName.SetWindowText(s);

	int nSampleSize = m_pContext->m_DstFile.SampleSize();
	long nSamples =
		(m_pContext->m_DstPos - m_pContext->m_DstStart)
		/ nSampleSize;
	if (0 == nSamples)
	{
		nSamples = 1;
	}
	CString format, formatRight;
	if (m_pContext->m_DstFile.Channels() > 1)
	{
		format.LoadString(IDS_STATISTICS_FORMAT_LEFT);
		formatRight.LoadString(IDS_STATISTICS_FORMAT_RIGHT);
	}
	else
	{
		format.LoadString(IDS_STATISTICS_FORMAT);
	}

	CString AtCursorDb;
	CString MinDb;
	CString MaxDb;
	CString RmsDb;
	CString DcDb;

	if (m_ValueAtCursorLeft != 0)
	{
		AtCursorDb.Format(_T("%.2f"), 20. * log10(abs(m_ValueAtCursorLeft) / 32768.));
	}
	else
	{
		AtCursorDb = _T("-Inf.");
	}
	if (m_pContext->m_MinLeft != 0)
	{
		MinDb.Format(_T("%.2f"), 20. * log10(abs(m_pContext->m_MinLeft) / 32768.));
	}
	else
	{
		MinDb = _T("-Inf.");
	}
	if (m_pContext->m_MinLeft != 0)
	{
		MaxDb.Format(_T("%.2f"), 20. * log10(abs(m_pContext->m_MaxLeft) / 32768.));
	}
	else
	{
		MaxDb = _T("-Inf.");
	}
	if (m_pContext->m_EnergyLeft != 0)
	{
		RmsDb.Format(_T("%.2f"),
					10. * log10(fabs(double(m_pContext->m_EnergyLeft)) / (nSamples * 1073741824.)));
	}
	else
	{
		RmsDb = _T("-Inf.");
	}
	if (m_pContext->m_SumLeft / nSamples != 0)
	{
		DcDb.Format(_T("%.2f"),
					20. * log10(fabs(double(m_pContext->m_SumLeft) / nSamples) / 32768.));
	}
	else
	{
		DcDb = _T("-Inf.");
	}

	_stprintf(s.GetBuffer(1024), format,
			//%s (%s)\r\n"
			LPCTSTR(SampleToString(m_CaretPosition, m_SamplesPerSec,
									SampleToString_HhMmSs | TimeToHhMmSs_NeedsMs | TimeToHhMmSs_NeedsHhMm)),
			LPCTSTR(SampleToString(m_CaretPosition, m_SamplesPerSec, SampleToString_Sample)),

			//"%s (%.2f dB; %.2f%%)\r\n"
			LPCTSTR(LtoaCS(m_ValueAtCursorLeft)), LPCTSTR(AtCursorDb),
			m_ValueAtCursorLeft / 327.68,

			//"%s (%s)\r\n"
			LPCTSTR(SampleToString(m_pContext->m_PosMinLeft / nSampleSize, m_SamplesPerSec,
									SampleToString_HhMmSs | TimeToHhMmSs_NeedsMs | TimeToHhMmSs_NeedsHhMm)),
			LPCTSTR(SampleToString(m_pContext->m_PosMinLeft / nSampleSize, m_SamplesPerSec, SampleToString_Sample)),

			//"%s (%.2f dB; %.2f%%)\r\n"
			LPCTSTR(LtoaCS(m_pContext->m_MinLeft)), LPCTSTR(MinDb),
			m_pContext->m_MinLeft / 327.68,

			//"%s (%s)\r\n"
			LPCTSTR(SampleToString(m_pContext->m_PosMaxLeft / nSampleSize, m_SamplesPerSec,
									SampleToString_HhMmSs | TimeToHhMmSs_NeedsMs | TimeToHhMmSs_NeedsHhMm)),
			LPCTSTR(SampleToString(m_pContext->m_PosMaxLeft / nSampleSize, m_SamplesPerSec, SampleToString_Sample)),

			//"%s (%.2f dB; %.2f%%)\r\n"
			LPCTSTR(LtoaCS(m_pContext->m_MaxLeft)),
			LPCTSTR(MaxDb), m_pContext->m_MaxLeft / 327.68,

			//"%.2f dB (%.2f%%)\r\n"
			// RMS
			LPCTSTR(RmsDb),
			100. * sqrt(fabs(double(m_pContext->m_EnergyLeft)) / (nSamples * 1073741824.)),
			//"%s (%.2f dB; %.2f%%)\r\n"
			LPCTSTR(LtoaCS(long(m_pContext->m_SumLeft / nSamples))),
			LPCTSTR(DcDb), (m_pContext->m_SumLeft / nSamples) / 327.68,
			//"%.2f Hz\r\n\r\n"
			// zero crossing
			m_pContext->m_ZeroCrossingLeft / double(nSamples) * m_SamplesPerSec,
			// %08X
			m_pContext->m_CRC32Left,
			m_pContext->m_Checksum
			);
	s.ReleaseBuffer();
	SetDlgItemText(IDC_EDIT_LEFT, s);

	// right channel
	if (m_pContext->m_DstFile.Channels() > 1)
	{
		if (m_ValueAtCursorRight != 0)
		{
			AtCursorDb.Format(_T("%.2f"), 20. * log10(abs(m_ValueAtCursorRight) / 32768.));
		}
		else
		{
			AtCursorDb = _T("-Inf.");
		}
		if (m_pContext->m_MinRight != 0)
		{
			MinDb.Format(_T("%.2f"), 20. * log10(fabs(double(m_pContext->m_MaxRight)) / 32768.));
		}
		else
		{
			MinDb = _T("-Inf.");
		}
		if (m_pContext->m_MinRight != 0)
		{
			MaxDb.Format(_T("%.2f"), 20. * log10(fabs(double(m_pContext->m_MaxRight)) / 32768.));
		}
		else
		{
			MaxDb = _T("-Inf.");
		}
		if (m_pContext->m_EnergyRight != 0)
		{
			RmsDb.Format(_T("%.2f"),
						10. * log10(fabs(double(m_pContext->m_EnergyRight)) / (nSamples * 1073741824.)));
		}
		else
		{
			RmsDb = _T("-Inf.");
		}
		if (m_pContext->m_SumRight / nSamples != 0)
		{
			DcDb.Format(_T("%.2f"),
						20. * log10(fabs(double(m_pContext->m_SumRight) / nSamples) / 32768.));
		}
		else
		{
			DcDb = _T("-Inf.");
		}

		_stprintf(s.GetBuffer(1024), formatRight,
				//%s (%s)\r\n"
				LPCTSTR(SampleToString(m_CaretPosition, m_SamplesPerSec,
										SampleToString_HhMmSs | TimeToHhMmSs_NeedsMs | TimeToHhMmSs_NeedsHhMm)),
				LPCTSTR(SampleToString(m_CaretPosition, m_SamplesPerSec, SampleToString_Sample)),

				//"%s (%.2f dB; %.2f%%)\r\n"
				LPCTSTR(LtoaCS(m_ValueAtCursorRight)), LPCTSTR(AtCursorDb),
				m_ValueAtCursorRight / 327.68,

				//"%s (%s)\r\n"
				LPCTSTR(SampleToString(m_pContext->m_PosMinRight / nSampleSize, m_SamplesPerSec,
										SampleToString_HhMmSs | TimeToHhMmSs_NeedsMs | TimeToHhMmSs_NeedsHhMm)),
				LPCTSTR(SampleToString(m_pContext->m_PosMinRight / nSampleSize, m_SamplesPerSec, SampleToString_Sample)),

				//"%s (%.2f dB; %.2f%%)\r\n"
				LPCTSTR(LtoaCS(m_pContext->m_MinRight)), LPCTSTR(MinDb),
				m_pContext->m_MinRight / 327.68,

				//"%s (%s)\r\n"
				LPCTSTR(SampleToString(m_pContext->m_PosMaxRight / nSampleSize, m_SamplesPerSec,
										SampleToString_HhMmSs | TimeToHhMmSs_NeedsMs | TimeToHhMmSs_NeedsHhMm)),
				LPCTSTR(SampleToString(m_pContext->m_PosMaxRight / nSampleSize, m_SamplesPerSec, SampleToString_Sample)),

				//"%s (%.2f dB; %.2f%%)\r\n"
				LPCTSTR(LtoaCS(m_pContext->m_MaxRight)),
				LPCTSTR(MaxDb), m_pContext->m_MaxRight / 327.68,

				//"%.2f dB (%.2f%%)\r\n"
				// RMS
				LPCTSTR(RmsDb),
				100. * sqrt(fabs(double(m_pContext->m_EnergyRight)) / (nSamples * 1073741824.)),
				//"%s (%.2f dB; %.2f%%)\r\n"
				LPCTSTR(LtoaCS(long(double(m_pContext->m_SumRight) / nSamples))),
				LPCTSTR(DcDb), (double(m_pContext->m_SumRight) / nSamples) / 327.68,
				//"%.2f Hz"
				// zero crossing
				m_pContext->m_ZeroCrossingRight / double(nSamples) * m_SamplesPerSec,
				// %08X\r\n
				m_pContext->m_CRC32Right,
				// %08X
				m_pContext->m_CRC32Common
				);

		s.ReleaseBuffer();
		SetDlgItemText(IDC_EDIT_RIGHT, s);
	}
	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}
/////////////////////////////////////////////////////////////////////////////
// CNormalizeSoundDialog dialog


CNormalizeSoundDialog::CNormalizeSoundDialog(CWnd* pParent /*=NULL*/)
	: BaseClass(CNormalizeSoundDialog::IDD, pParent)
{
	//{{AFX_DATA_INIT(CNormalizeSoundDialog)
	m_bLockChannels = FALSE;
	m_bUndo = FALSE;
	m_DbPercent = -1;
	//}}AFX_DATA_INIT
	m_Profile.AddItem(_T("Settings"), _T("NormalizeDialogDbPercents"), m_DbPercent, 0, 0, 1);
	m_Profile.AddItem(_T("Settings"), _T("NormalizeLevelDb"), m_dLevelDb, -6., -40., 0.);
	m_Profile.AddItem(_T("Settings"), _T("NormalizeLevelPercent"), m_dLevelPercent, 50., 1., 100.);
}


void CNormalizeSoundDialog::DoDataExchange(CDataExchange* pDX)
{
	BaseClass::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CNormalizeSoundDialog)
	DDX_Control(pDX, IDC_STATIC_SELECTION, m_SelectionStatic);
	DDX_Control(pDX, IDC_SLIDER_LEVEL, m_SliderLevel);
	DDX_Control(pDX, IDC_EDIT_LEVEL, m_eLevel);
	DDX_Check(pDX, IDC_CHECK_LOCK_CHANNELS, m_bLockChannels);
	DDX_Check(pDX, IDC_CHECK_UNDO, m_bUndo);
	//}}AFX_DATA_MAP
	if (0 == m_DbPercent)
	{
		// decibels
		m_eLevel.ExchangeData(pDX, m_dLevelDb,
							_T("Target level"), _T("dB"), -40., 0.);
	}
	else
	{
		// percents
		m_eLevel.ExchangeData(pDX, m_dLevelPercent,
							_T("Target level"), _T("%"), 1., 100.);
	}
	DDX_CBIndex(pDX, IDC_COMBODB_PERCENT, m_DbPercent);
	if ( ! pDX->m_bSaveAndValidate)
	{
		UpdateSelectionStatic();
	}
	else
	{
		m_Profile.FlushAll();
	}
}


BEGIN_MESSAGE_MAP(CNormalizeSoundDialog, BaseClass)
	//{{AFX_MSG_MAP(CNormalizeSoundDialog)
	ON_EN_KILLFOCUS(IDC_EDIT_LEVEL, OnKillfocusEditLevel)
	ON_BN_CLICKED(IDC_BUTTON_SELECTION, OnButtonSelection)
	ON_CBN_SELCHANGE(IDC_COMBODB_PERCENT, OnSelchangeCombodbPercent)
	ON_WM_HSCROLL()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CNormalizeSoundDialog message handlers

void CNormalizeSoundDialog::OnKillfocusEditLevel()
{
	double num;
	CString s;
	m_eLevel.GetWindowText(s);
	int SliderPos = 0;
	if (! s.IsEmpty()
		&& m_eLevel.SimpleFloatParse(s, num))
	{
		if (0 == m_DbPercent)
		{
			// decibel
			if (num < -40. || num > 0.)
			{
				return;
			}
			SliderPos = int(num * 10.);
		}
		else
		{
			// percent
			if (num < 1.
				|| num > 100.)
			{
				return;
			}
			SliderPos = int(200. * log10(num / 100.));
		}
		m_SliderLevel.SetPos(SliderPos);
	}
}

void CNormalizeSoundDialog::OnButtonSelection()
{
	CSelectionDialog dlg(m_Start, m_End, m_CaretPosition, m_Chan + 1, m_FileLength, m_pWf, m_TimeFormat);

	if (IDOK != dlg.DoModal())
	{
		return;
	}
	m_Start = dlg.GetStart();
	m_End = dlg.GetEnd();
	m_Chan = dlg.GetChannel() - 1;

	UpdateSelectionStatic();
}

void CNormalizeSoundDialog::OnSelchangeCombodbPercent()
{
	int sel = ((CComboBox *)GetDlgItem(IDC_COMBODB_PERCENT))->GetCurSel();
	TRACE("OnSelchangeCombodbPercent() sel=%d\n", sel);
	if (sel == m_DbPercent)
	{
		return;
	}
	UpdateData();
	if (0 == sel)
	{
		// get percent values and convert to dB values
		if (m_dLevelPercent > 0)
		{
			m_dLevelDb = 20 * log10(0.01 * m_dLevelPercent);
			if (m_dLevelDb < -40.)
			{
				m_dLevelDb = -40.;
			}
			if (m_dLevelDb > 0.)
			{
				m_dLevelDb = 0.;
			}
		}
		else
		{
			m_dLevelDb = 0;
		}
	}
	else
	{
		m_dLevelPercent = 100. * pow(10., m_dLevelDb / 20.);
		if (m_dLevelPercent < 1.)
		{
			m_dLevelPercent = 1.;
		}
		else if (m_dLevelPercent > 10000.)
		{
			m_dLevelPercent = 10000.;
		}
	}
	UpdateData(FALSE);
}

void CNormalizeSoundDialog::UpdateSelectionStatic()
{
	m_SelectionStatic.SetWindowText(GetSelectionText(m_Start, m_End, m_Chan,
													m_pWf->nChannels, m_bLockChannels,
													m_pWf->nSamplesPerSec, m_TimeFormat));
}


BOOL CNormalizeSoundDialog::OnInitDialog()
{
	BaseClass::OnInitDialog();
	m_SliderLevel.SetRange(-400, 0);
	m_SliderLevel.SetTicFreq(100);
	m_SliderLevel.SetLineSize(10);
	m_SliderLevel.SetPageSize(50);
	if (0 == m_DbPercent)
	{
		// decibel
		m_SliderLevel.SetPos(int(m_dLevelDb * 10.));
	}
	else
	{
		m_SliderLevel.SetPos(int(200. * log10(m_dLevelPercent / 100.)));
	}
	if (1 == m_pWf->nChannels)
	{
		GetDlgItem(IDC_CHECK_LOCK_CHANNELS)->ShowWindow(SW_HIDE);
	}

	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

void CNormalizeSoundDialog::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
	BaseClass::OnHScroll(nSBCode, nPos, pScrollBar);
	CSliderCtrl * pSlider = dynamic_cast<CSliderCtrl *>(pScrollBar);
	if (NULL != pSlider)
	{
		int pos = pSlider->GetPos();
		CString s;
		if (0 == m_DbPercent)
		{
			s.Format(_T("%.2f"), pos / 10.);
		}
		else
		{
			s.Format(_T("%.0f"), 100. * pow(10., pos / 200.));
		}
		SetDlgItemText(IDC_EDIT_LEVEL, s);
	}
}

void CGotoDialog::OnSelchangeComboTimeFormat()
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
	m_Position = m_eStart.GetTimeSample();
	m_eStart.SetTimeFormat(Format);
	m_eStart.SetTimeSample(m_Position);
}

BOOL CGotoDialog::OnInitDialog()
{
	BaseClass::OnInitDialog();

	m_eStart.AddPosition(IDS_BEGIN_OF_SAMPLE, 0);
	m_eStart.AddPosition(IDS_END_OF_SAMPLE, m_FileLength);
	((CComboBox*) & m_eStart)->SetExtendedUI(TRUE);
	// TODO: add markers
	return TRUE;  // return TRUE unless you set the focus to a control
}
/////////////////////////////////////////////////////////////////////////////
// CResampleDialog dialog


CResampleDialog::CResampleDialog(CWnd* pParent /*=NULL*/)
	: BaseClass(CResampleDialog::IDD, pParent)
{
	//{{AFX_DATA_INIT(CResampleDialog)
	m_bChangeRateOnly = FALSE;
	m_bUndo = FALSE;
	m_bChangeSamplingRate = -1;
	m_NewSampleRate = 0;
	//}}AFX_DATA_INIT
	m_bCanOnlyChangeSamplerate = false;
	m_Profile.AddItem(_T("Settings"), _T("ResampleChangeRateOnly"), m_bChangeRateOnly, 0, 0, 1);
	m_Profile.AddItem(_T("Settings"), _T("ResampleChangeSamplingRate"), m_bChangeSamplingRate, 1, 0, 1);
	m_Profile.AddItem(_T("Settings"), _T("ResampleTempoChange"), m_TempoChange, 100., 25., 400.);
	m_Profile.AddItem(_T("Settings"), _T("ResampleNewSampleRate"), m_NewSampleRate, 44100, 11025, 176400);
}


void CResampleDialog::DoDataExchange(CDataExchange* pDX)
{
	BaseClass::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CResampleDialog)
	DDX_Control(pDX, IDC_SLIDER_TEMPO, m_SliderTempo);
	DDX_Control(pDX, IDC_SLIDER_RATE, m_SliderRate);
	DDX_Control(pDX, IDC_EDIT_TEMPO, m_EditTempo);
	DDX_Check(pDX, IDC_CHECK_CHANGE_RATE_ONLY, m_bChangeRateOnly);
	DDX_Check(pDX, IDC_CHECK_UNDO, m_bUndo);
	DDX_Radio(pDX, IDC_RADIO_CHANGE_PITCH, m_bChangeSamplingRate);
	DDX_Text(pDX, IDC_EDIT_RATE, m_NewSampleRate);
	//}}AFX_DATA_MAP
	DDV_MinMaxUInt(pDX, m_NewSampleRate, m_OldSampleRate / 4, m_OldSampleRate * 4);
	m_EditTempo.ExchangeData(pDX, m_TempoChange,
							_T("Tempo/pitch change"), _T("%"), 25., 400.);
	if (pDX->m_bSaveAndValidate)
	{
		if (m_bCanOnlyChangeSamplerate)
		{
			m_Profile.RevertItemToInitial(_T("Settings"), _T("ResampleChangeRateOnly"));
		}

		m_Profile.FlushAll();
	}
}


BEGIN_MESSAGE_MAP(CResampleDialog, BaseClass)
	//{{AFX_MSG_MAP(CResampleDialog)
	ON_BN_CLICKED(IDC_RADIO_CHANGE_RATE, OnRadioChangeRate)
	ON_BN_CLICKED(IDC_RADIO_CHANGE_PITCH, OnRadioChangeTempo)
	ON_WM_HSCROLL()
	ON_EN_KILLFOCUS(IDC_EDIT_RATE, OnKillfocusEditRate)
	ON_EN_KILLFOCUS(IDC_EDIT_TEMPO, OnKillfocusEditTempo)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CResampleDialog message handlers

void CResampleDialog::OnRadioChangeRate()
{
	m_bChangeSamplingRate = 1;
	m_SliderRate.EnableWindow(TRUE);
	GetDlgItem(IDC_EDIT_RATE)->EnableWindow(TRUE);
	m_SliderTempo.EnableWindow(FALSE);
	m_EditTempo.EnableWindow(FALSE);
}

void CResampleDialog::OnRadioChangeTempo()
{
	m_bChangeSamplingRate = 0;
	m_SliderTempo.EnableWindow(TRUE);
	m_EditTempo.EnableWindow(TRUE);
	m_SliderRate.EnableWindow(FALSE);
	GetDlgItem(IDC_EDIT_RATE)->EnableWindow(FALSE);
}

void CResampleDialog::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
	BaseClass::OnHScroll(nSBCode, nPos, pScrollBar);
	int nId = pScrollBar->GetDlgCtrlID();
	if (IDC_SLIDER_TEMPO == nId
		&& m_SliderTempo.m_hWnd != NULL)
	{
		int pos = m_SliderTempo.GetPos();
		CString s;
		s.Format(_T("%d"), pos);
		SetDlgItemText(IDC_EDIT_TEMPO, s);
	}
	else if (IDC_SLIDER_RATE == nId
			&& m_SliderRate.m_hWnd != NULL)
	{
		int pos = m_SliderRate.GetPos();
		CString s;
		s.Format(_T("%d"), pos);
		SetDlgItemText(IDC_EDIT_RATE, s);
	}
}

void CResampleDialog::OnKillfocusEditRate()
{
	CString s;
	GetDlgItemText(IDC_EDIT_RATE, s);
	int SliderPos = 0;
	if (! s.IsEmpty())
	{
		BOOL NoErr;
		int val = GetDlgItemInt(IDC_EDIT_RATE, & NoErr, FALSE);
		if ( ! NoErr || val < m_OldSampleRate / 4 || val > m_OldSampleRate * 4)
		{
			return;
		}
		SliderPos = val;
		m_SliderRate.SetPos(SliderPos);
	}

}

void CResampleDialog::OnKillfocusEditTempo()
{
	double num;
	CString s;
	m_EditTempo.GetWindowText(s);
	int SliderPos = 0;
	if (! s.IsEmpty()
		&& m_EditTempo.SimpleFloatParse(s, num))
	{
		if (num < 25. || num > 400.)
		{
			return;
		}
		SliderPos = int(num);
		m_SliderTempo.SetPos(SliderPos);
	}
}


BOOL CResampleDialog::OnInitDialog()
{
	if (m_bCanOnlyChangeSamplerate)
	{
		m_bChangeRateOnly = true;
	}
	BaseClass::OnInitDialog();

	m_SliderTempo.SetRange(25, 400);
	m_SliderTempo.SetTicFreq(25);
	m_SliderTempo.SetLineSize(5);
	m_SliderTempo.SetPageSize(25);
	m_SliderTempo.SetPos(100);

	m_SliderRate.SetRange(m_OldSampleRate / 4, m_OldSampleRate * 4);
	m_SliderRate.SetTicFreq(m_OldSampleRate / 4);
	m_SliderRate.SetLineSize(m_OldSampleRate / 20);
	m_SliderRate.SetPageSize(m_OldSampleRate / 4);
	m_SliderRate.SetPos(m_OldSampleRate);

	if (m_bChangeSamplingRate)
	{
		OnRadioChangeRate();
	}
	else
	{
		OnRadioChangeTempo();
	}
	if (m_bCanOnlyChangeSamplerate)
	{
		GetDlgItem(IDC_CHECK_CHANGE_RATE_ONLY)->EnableWindow(FALSE);
	}
	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

int CStatisticsDialog::DoModal()
{
	if (NULL != m_pContext
		&& m_pContext->m_DstFile.Channels() == 1)
	{
		m_lpszTemplateName = MAKEINTRESOURCE(IDD_DIALOG_STATISTICS_MONO);
	}
	return BaseClass::DoModal();
}
/////////////////////////////////////////////////////////////////////////////
// CLowFrequencySuppressDialog dialog


CLowFrequencySuppressDialog::CLowFrequencySuppressDialog(CWnd* pParent /*=NULL*/)
	: BaseClass(CLowFrequencySuppressDialog::IDD, pParent)
{
	//{{AFX_DATA_INIT(CLowFrequencySuppressDialog)
	m_DifferentialModeSuppress = FALSE;
	m_LowFrequencySuppress = FALSE;
	m_bUndo = FALSE;
	//}}AFX_DATA_INIT
	m_Profile.AddItem(_T("Settings"), _T("SuppressDifferentialRange"), m_dDiffNoiseRange, 200., 1., 1000.);
	m_Profile.AddItem(_T("Settings"), _T("SuppressLowFreqRange"), m_dLfNoiseRange, 20., 1., 1000.);
	m_Profile.AddBoolItem(_T("Settings"), _T("SuppressDifferential"), m_DifferentialModeSuppress, TRUE);
	m_Profile.AddBoolItem(_T("Settings"), _T("SuppressLowFrequency"), m_LowFrequencySuppress, TRUE);

}


void CLowFrequencySuppressDialog::DoDataExchange(CDataExchange* pDX)
{
	BaseClass::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CLowFrequencySuppressDialog)
	DDX_Control(pDX, IDC_STATIC_SELECTION, m_SelectionStatic);
	DDX_Control(pDX, IDC_EDIT_LF_NOISE_RANGE, m_eLfNoiseRange);
	DDX_Control(pDX, IDC_EDIT_DIFF_NOISE_RANGE, m_eDiffNoiseRange);
	DDX_Check(pDX, IDC_CHECK_DIFFERENTIAL_MODE_SUPPRESS, m_DifferentialModeSuppress);
	DDX_Check(pDX, IDC_CHECK_LOW_FREQUENCY, m_LowFrequencySuppress);
	DDX_Check(pDX, IDC_CHECK_UNDO, m_bUndo);
	//}}AFX_DATA_MAP
	m_eLfNoiseRange.ExchangeData(pDX, m_dLfNoiseRange,
								_T("Low frequency suppression range"), _T("Hz"), 1., 1000.);
	m_eDiffNoiseRange.ExchangeData(pDX, m_dDiffNoiseRange,
									_T("Differential static suppression range"), _T("Hz"), 1., 1000.);

	if (pDX->m_bSaveAndValidate)
	{
		m_Profile.FlushAll();
	}
}


BEGIN_MESSAGE_MAP(CLowFrequencySuppressDialog, BaseClass)
	//{{AFX_MSG_MAP(CLowFrequencySuppressDialog)
	ON_BN_CLICKED(IDC_BUTTON_SELECTION, OnButtonSelection)
	ON_BN_CLICKED(IDC_CHECK_DIFFERENTIAL_MODE_SUPPRESS, OnCheckDifferentialModeSuppress)
	ON_BN_CLICKED(IDC_CHECK_LOW_FREQUENCY, OnCheckLowFrequency)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CLowFrequencySuppressDialog message handlers

void CLowFrequencySuppressDialog::OnButtonSelection()
{
	CSelectionDialog dlg(m_Start, m_End, m_CaretPosition, m_Chan + 1, m_FileLength, m_pWf, m_TimeFormat);

	if (IDOK != dlg.DoModal())
	{
		return;
	}
	m_Start = dlg.GetStart();
	m_End = dlg.GetEnd();
	m_Chan = dlg.GetChannel() - 1;

	UpdateSelectionStatic();
}

void CLowFrequencySuppressDialog::OnCheckDifferentialModeSuppress()
{
	m_DifferentialModeSuppress = IsDlgButtonChecked(IDC_CHECK_DIFFERENTIAL_MODE_SUPPRESS);
	m_eDiffNoiseRange.EnableWindow(m_DifferentialModeSuppress);
	if ( ! m_DifferentialModeSuppress)
	{
		// check second button
		if (m_LowFrequencySuppress)
		{
			return;
		}
		CButton * pCheck = (CButton *)GetDlgItem(IDC_CHECK_LOW_FREQUENCY);
		if (pCheck)
		{
			pCheck->SetCheck(1);
			OnCheckLowFrequency();
		}
	}
}

void CLowFrequencySuppressDialog::OnCheckLowFrequency()
{
	m_LowFrequencySuppress = IsDlgButtonChecked(IDC_CHECK_LOW_FREQUENCY);
	m_eLfNoiseRange.EnableWindow(m_LowFrequencySuppress);
	if ( ! m_LowFrequencySuppress)
	{
		// check second button
		if (m_DifferentialModeSuppress)
		{
			return;
		}
		CButton * pCheck = (CButton *)GetDlgItem(IDC_CHECK_DIFFERENTIAL_MODE_SUPPRESS);
		if (pCheck)
		{
			pCheck->SetCheck(1);
			OnCheckDifferentialModeSuppress();
		}
	}
}

void CLowFrequencySuppressDialog::UpdateSelectionStatic()
{
	m_SelectionStatic.SetWindowText(GetSelectionText(m_Start, m_End, m_Chan,
													m_pWf->nChannels, m_bLockChannels,
													m_pWf->nSamplesPerSec, m_TimeFormat));
}


BOOL CLowFrequencySuppressDialog::OnInitDialog()
{
	BaseClass::OnInitDialog();

	if (! m_LowFrequencySuppress
		&& ! m_DifferentialModeSuppress)
	{
		m_LowFrequencySuppress = TRUE;
		m_DifferentialModeSuppress = TRUE;
	}
	m_eLfNoiseRange.EnableWindow(m_LowFrequencySuppress);
	m_eDiffNoiseRange.EnableWindow(m_DifferentialModeSuppress);
	UpdateSelectionStatic();
	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}
/////////////////////////////////////////////////////////////////////////////
// CExpressionEvaluationDialog dialog


CExpressionEvaluationDialog::CExpressionEvaluationDialog(CWnd* pParent /*=NULL*/)
	: BaseClass(CExpressionEvaluationDialog::IDD, pParent),
	m_ExpressionGroupSelected(0),
	m_ExpressionSelected(0),
	m_ExpressionTabSelected(0)
{
	//{{AFX_DATA_INIT(CExpressionEvaluationDialog)
	m_bUndo = FALSE;
	//}}AFX_DATA_INIT
	m_bNeedUpdateControls = false;
}


void CExpressionEvaluationDialog::DoDataExchange(CDataExchange* pDX)
{
	BaseClass::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CExpressionEvaluationDialog)
	DDX_Control(pDX, IDC_TAB_TOKENS, m_TabTokens);
	DDX_Control(pDX, IDC_EDIT_EXPRESSION, m_eExpression);
	DDX_Control(pDX, IDC_STATIC_SELECTION, m_SelectionStatic);
	DDX_Check(pDX, IDC_CHECK_UNDO, m_bUndo);
	DDX_Text(pDX, IDC_EDIT_EXPRESSION, m_sExpression);
	//}}AFX_DATA_MAP
	if (pDX->m_bSaveAndValidate)
	{
		m_Profile.FlushAll();
	}

}


BEGIN_MESSAGE_MAP(CExpressionEvaluationDialog, BaseClass)
	//{{AFX_MSG_MAP(CExpressionEvaluationDialog)
	ON_BN_CLICKED(IDC_BUTTON_SELECTION, OnButtonSelection)
	ON_NOTIFY(TCN_SELCHANGE, IDC_TAB_TOKENS, OnSelchangeTabTokens)
	ON_BN_CLICKED(IDC_BUTTON_SAVEAS, OnButtonSaveExpressionAs)
	ON_EN_CHANGE(IDC_EDIT_EXPRESSION, OnChangeEditExpression)
	//}}AFX_MSG_MAP
#if 0
	ON_COMMAND_EX(IDC_BUTTON_SIN, OnButtonText)
	ON_COMMAND_EX(IDC_BUTTON_COS, OnButtonText)
	ON_COMMAND_EX(IDC_BUTTON_TAN, OnButtonText)
	ON_COMMAND_EX(IDC_BUTTON_SINH, OnButtonText)
	ON_COMMAND_EX(IDC_BUTTON_COSH, OnButtonText)
	ON_COMMAND_EX(IDC_BUTTON_TANH, OnButtonText)
	ON_COMMAND_EX(IDC_BUTTON_EXP, OnButtonText)
	ON_COMMAND_EX(IDC_BUTTON_LN, OnButtonText)
	ON_COMMAND_EX(IDC_BUTTON_EXP10, OnButtonText)
	ON_COMMAND_EX(IDC_BUTTON_LOG10, OnButtonText)
	ON_COMMAND_EX(IDC_BUTTON_SQRT, OnButtonText)
	ON_COMMAND_EX(IDC_BUTTON_ABS, OnButtonText)
	ON_COMMAND_EX(IDC_BUTTON_INT, OnButtonText)
	ON_COMMAND_EX(IDC_BUTTON_POW, OnButtonText)
	ON_COMMAND_EX(IDC_BUTTON_NOISE, OnButtonText)
	ON_COMMAND_EX(IDC_BUTTON_PI, OnButtonText)
	ON_COMMAND_EX(IDC_BUTTON_T, OnButtonText)
	ON_COMMAND_EX(IDC_BUTTON_LC_T, OnButtonText)
	ON_COMMAND_EX(IDC_BUTTON_DT, OnButtonText)
	ON_COMMAND_EX(IDC_BUTTON_LC_DT, OnButtonText)
	ON_COMMAND_EX(IDC_BUTTON_DN, OnButtonText)
	ON_COMMAND_EX(IDC_BUTTON_LC_DN, OnButtonText)
	ON_COMMAND_EX(IDC_BUTTON_F, OnButtonText)
	ON_COMMAND_EX(IDC_BUTTON_LC_F, OnButtonText)
	ON_COMMAND_EX(IDC_BUTTON_WAVE, OnButtonText)
	ON_COMMAND_EX(IDC_BUTTON_PLUS, OnButtonText)
	ON_COMMAND_EX(IDC_BUTTON_MINUS, OnButtonText)
	ON_COMMAND_EX(IDC_BUTTON_MULTIPLY, OnButtonText)
	ON_COMMAND_EX(IDC_BUTTON_DIVIDE, OnButtonText)
	ON_COMMAND_EX(IDC_BUTTON_MODULE, OnButtonText)
	ON_COMMAND_EX(IDC_BUTTON_AND, OnButtonText)
	ON_COMMAND_EX(IDC_BUTTON_OR, OnButtonText)
	ON_COMMAND_EX(IDC_BUTTON_XOR, OnButtonText)
	ON_COMMAND_EX(IDC_BUTTON_INVERSE, OnButtonText)
	//ON_COMMAND_EX(IDC_BUTTON_, OnButtonText)
#else
	ON_COMMAND_EX_RANGE(IDC_BUTTON_SIN, IDC_BUTTON_INVERSE, OnButtonText)
#endif
	ON_UPDATE_COMMAND_UI(IDOK, OnUpdateOk)
	ON_UPDATE_COMMAND_UI(IDC_BUTTON_SAVEAS, OnUpdateSaveAs)
	ON_MESSAGE(WM_KICKIDLE, OnKickIdle)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CExpressionEvaluationDialog message handlers
BOOL CExpressionEvaluationDialog::OnButtonText(UINT id)
{
	TRACE("CExpressionEvaluationDialog::OnButtonText(%X)\n", id);
	CString s;
	if (s.LoadString(id))
	{
		CString Substr;
		AfxExtractSubString(Substr, s, 0, '\n');
		CEdit * pEdit = (CEdit *) GetDlgItem(IDC_EDIT_EXPRESSION);
		if (NULL != pEdit)
		{
			pEdit->ReplaceSel(Substr, TRUE);
			pEdit->SetFocus();
			return TRUE;
		}
	}
	return FALSE;
}

void CExpressionEvaluationDialog::OnButtonSelection()
{
	CSelectionDialog dlg(m_Start, m_End, m_CaretPosition, m_Chan + 1, m_FileLength, m_pWf, m_TimeFormat);

	if (IDOK != dlg.DoModal())
	{
		return;
	}
	m_Start = dlg.GetStart();
	m_End = dlg.GetEnd();
	m_Chan = dlg.GetChannel() - 1;
	UpdateSelectionStatic();
}

void CExpressionEvaluationDialog::UpdateSelectionStatic()
{
	m_SelectionStatic.SetWindowText(GetSelectionText(m_Start, m_End, m_Chan,
													m_pWf->nChannels, m_bLockChannels,
													m_pWf->nSamplesPerSec, m_TimeFormat));
}


void CExpressionEvaluationDialog::OnOK()
{
	if (!UpdateData(TRUE)
		|| ! m_OperandsTabDlg.UpdateData(TRUE)
		|| ! m_SavedExprTabDlg.UpdateData(TRUE))
	{
		TRACE("UpdateData failed during dialog termination.\n");
		// the UpdateData routine will set focus to correct item
		return;
	}
	if (NULL != m_pContext)
	{
		//CString expr;
		//m_eExpression.GetWindowText(expr);
		LPCTSTR str = m_sExpression;
		LPCTSTR str1 = str;
		if ( ! m_pContext->SetExpression( & str))
		{
			AfxMessageBox(m_pContext->m_ErrorString);
			int pos = str - str1;
			m_eExpression.SetFocus();
			m_eExpression.SetSel(pos, pos, FALSE);
			return;
		}
	}
	EndDialog(IDOK);
}
/////////////////////////////////////////////////////////////////////////////
// CDeclickDialog dialog


CDeclickDialog::CDeclickDialog(CWnd* pParent /*=NULL*/)
	: BaseClass(CDeclickDialog::IDD, pParent)
{
	//{{AFX_DATA_INIT(CDeclickDialog)
	m_ClickLogFilename = _T("");
	m_MaxClickLength = 24;
	m_MinClickAmplitude = 200;
	m_bLogClicks = FALSE;
	m_bLogClicksOnly = FALSE;
	m_bImportClicks = FALSE;
	m_ClickImportFilename = _T("");
	m_bUndo = FALSE;
	//}}AFX_DATA_INIT
	m_dAttackRate = .06;
	m_dClickToNoise = 5.;
	m_dEnvelopDecayRate = 0.02;
}

CDeclickDialog::~CDeclickDialog()
{
	GetApp()->Profile.RemoveSection(_T("Declicker"));
}

void CDeclickDialog::DoDataExchange(CDataExchange* pDX)
{
	BaseClass::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CDeclickDialog)
	DDX_Control(pDX, IDC_STATIC_SELECTION, m_SelectionStatic);
	DDX_Control(pDX, IDC_EDIT_DECAY_RATE, m_EnvelopDecayRate);
	DDX_Control(pDX, IDC_EDIT_CLICK_TO_NOISE, m_ClickToNoise);
	DDX_Control(pDX, IDC_EDIT_ATTACK_RATE, m_AttackRate);
	DDX_Text(pDX, IDC_EDIT_CLICK_LOG_FILENAME, m_ClickLogFilename);
	DDX_Text(pDX, IDC_EDIT_MAX_CLICK_LENGTH, m_MaxClickLength);
	DDV_MinMaxInt(pDX, m_MaxClickLength, 6, 64);
	DDX_Text(pDX, IDC_EDIT_MIN_CLICK_AMPLITUDE, m_MinClickAmplitude);
	DDV_MinMaxInt(pDX, m_MinClickAmplitude, 50, 5000);
	DDX_Check(pDX, IDC_CHECK_LOG_CLICKS, m_bLogClicks);
	DDX_Check(pDX, IDC_CHECK_LOG_CLICKS_ONLY, m_bLogClicksOnly);
	DDX_Check(pDX, IDC_CHECK_IMPORT_CLICKS, m_bImportClicks);
	DDX_Text(pDX, IDC_EDIT_CLICK_IMPORT_FILENAME, m_ClickImportFilename);
	DDX_Check(pDX, IDC_CHECK_UNDO, m_bUndo);
	//}}AFX_DATA_MAP
	m_AttackRate.ExchangeData(pDX, m_dAttackRate,
							_T("Attack rate"), _T(""), 0.01, 0.9);
	m_ClickToNoise.ExchangeData(pDX, m_dClickToNoise,
								_T("Click to noise rate"), _T(""), 1., 10.);
	m_EnvelopDecayRate.ExchangeData(pDX, m_dEnvelopDecayRate,
									_T("Envelop decay rate"), _T(""), 0.01, 0.99);
}


BEGIN_MESSAGE_MAP(CDeclickDialog, BaseClass)
	//{{AFX_MSG_MAP(CDeclickDialog)
	ON_BN_CLICKED(IDC_CHECK_LOG_CLICKS, OnCheckLogClicks)
	ON_BN_CLICKED(IDC_CHECK_IMPORT_CLICKS, OnCheckImportClicks)
	ON_BN_CLICKED(IDC_CLICK_LOG_BROWSE_BUTTON, OnClickLogBrowseButton)
	ON_BN_CLICKED(IDC_CLICK_IMPORT_BROWSE_BUTTON, OnClickImportBrowseButton)
	ON_BN_CLICKED(IDC_BUTTON_MORE_SETTINGS, OnButtonMoreSettings)
	ON_BN_CLICKED(IDC_BUTTON_SELECTION, OnButtonSelection)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CDeclickDialog message handlers
void CDeclickDialog::OnCheckLogClicks()
{
	BOOL Enable = IsDlgButtonChecked(IDC_CHECK_LOG_CLICKS);
	GetDlgItem(IDC_EDIT_CLICK_LOG_FILENAME)->EnableWindow(Enable);
	GetDlgItem(IDC_CLICK_LOG_BROWSE_BUTTON)->EnableWindow(Enable);
	GetDlgItem(IDC_CHECK_LOG_CLICKS_ONLY)->EnableWindow(Enable);
}

void CDeclickDialog::OnCheckImportClicks()
{
	BOOL Enable = IsDlgButtonChecked(IDC_CHECK_IMPORT_CLICKS);
	GetDlgItem(IDC_EDIT_CLICK_IMPORT_FILENAME)->EnableWindow(Enable);
	GetDlgItem(IDC_CLICK_IMPORT_BROWSE_BUTTON)->EnableWindow(Enable);
}

void CDeclickDialog::OnClickLogBrowseButton()
{
	CString filter(_T("Text files (*.txt)|*.txt|All Files (*.*)|*.*||"));

	GetDlgItem(IDC_EDIT_CLICK_LOG_FILENAME)->GetWindowText(m_ClickLogFilename);

	CFileDialog fdlg(TRUE, _T("txt"), m_ClickLogFilename,
					OFN_EXPLORER
					//| OFN_FILEMUSTEXIST
					| OFN_HIDEREADONLY,
					filter);
	fdlg.m_ofn.lpstrInitialDir = _T(".");

	if (fdlg.DoModal() == IDOK)
	{
		m_ClickLogFilename = fdlg.GetPathName();
		GetDlgItem(IDC_EDIT_CLICK_LOG_FILENAME)->SetWindowText(m_ClickLogFilename);
	}

}

void CDeclickDialog::OnClickImportBrowseButton()
{
	CString filter(_T("Text files (*.txt)|*.txt|All Files (*.*)|*.*||"));

	GetDlgItem(IDC_EDIT_CLICK_IMPORT_FILENAME)->GetWindowText(m_ClickImportFilename);

	CFileDialog fdlg(TRUE, _T("txt"), m_ClickImportFilename,
					OFN_EXPLORER
					| OFN_FILEMUSTEXIST
					| OFN_HIDEREADONLY,
					filter);
	fdlg.m_ofn.lpstrInitialDir = _T(".");

	if (fdlg.DoModal() == IDOK)
	{
		m_ClickImportFilename = fdlg.GetPathName();
		GetDlgItem(IDC_EDIT_CLICK_IMPORT_FILENAME)->SetWindowText(m_ClickImportFilename);
	}

}
void CDeclickDialog::OnButtonMoreSettings()
{
	// TODO: Add your control notification handler code here
	//CMoreDeclickDialog dlg;
	// set the data to dlg
	//if (IDOK == dlg.DoModal())
	{
		// return the data from dlg
	}
}

void CDeclickDialog::LoadValuesFromRegistry()
{
	CThisApp * pApp = GetApp();
	pApp->Profile.AddBoolItem(_T("Declicker"), _T("LogClicks"), m_bLogClicks, FALSE);
	pApp->Profile.AddBoolItem(_T("Declicker"), _T("ImportClicks"), m_bImportClicks, FALSE);
	pApp->Profile.AddBoolItem(_T("Declicker"), _T("LogClicksOnly"), m_bLogClicksOnly, FALSE);
	pApp->Profile.AddItem(_T("Declicker"), _T("ClickLogFilename"), m_ClickLogFilename, _T(""));
	pApp->Profile.AddItem(_T("Declicker"), _T("ClickImportFilename"), m_ClickImportFilename, _T(""));
	pApp->Profile.AddItem(_T("Declicker"), _T("MaxClickLength"), m_MaxClickLength, 32, 6, 64);
	pApp->Profile.AddItem(_T("Declicker"), _T("MinClickAmplitude"), m_MinClickAmplitude, 200, 50, 5000);
	pApp->Profile.AddItem(_T("Declicker"), _T("AttackRate"), m_dAttackRate, 0.5, 0.001, 0.99);
	pApp->Profile.AddItem(_T("Declicker"), _T("DecayRate"), m_dEnvelopDecayRate, 0.01, 0.001, 0.99);
	pApp->Profile.AddItem(_T("Declicker"), _T("ClickToNoise"), m_dClickToNoise, 4., 1.5, 20);

}

BOOL CDeclickDialog::OnInitDialog()
{
	LoadValuesFromRegistry();
	BaseClass::OnInitDialog();

	GetDlgItem(IDC_EDIT_CLICK_LOG_FILENAME)->EnableWindow(m_bLogClicks);
	GetDlgItem(IDC_CLICK_LOG_BROWSE_BUTTON)->EnableWindow(m_bLogClicks);
	GetDlgItem(IDC_CHECK_LOG_CLICKS_ONLY)->EnableWindow(m_bLogClicks);

	GetDlgItem(IDC_EDIT_CLICK_IMPORT_FILENAME)->EnableWindow(m_bImportClicks);
	GetDlgItem(IDC_CLICK_IMPORT_BROWSE_BUTTON)->EnableWindow(m_bImportClicks);

	UpdateSelectionStatic();
	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

void CDeclickDialog::OnButtonSelection()
{
	CSelectionDialog dlg(m_Start, m_End, m_CaretPosition, m_Chan + 1, m_FileLength, m_pWf, m_TimeFormat);

	if (IDOK != dlg.DoModal())
	{
		return;
	}
	m_Start = dlg.GetStart();
	m_End = dlg.GetEnd();
	m_Chan = dlg.GetChannel() - 1;
	UpdateSelectionStatic();
}

void CDeclickDialog::UpdateSelectionStatic()
{
	m_SelectionStatic.SetWindowText(GetSelectionText(m_Start, m_End, m_Chan,
													m_pWf->nChannels, m_bLockChannels,
													m_pWf->nSamplesPerSec, m_TimeFormat));
}


void CDeclickDialog::OnOK()
{
	if (!UpdateData(TRUE))
	{
		TRACE0("UpdateData failed during dialog termination.\n");
		// the UpdateData routine will set focus to correct item
		return;
	}
	GetApp()->Profile.FlushSection(_T("Declicker"));
	EndDialog(IDOK);
}

void CDeclickDialog::SetDeclickData(CClickRemoval * pCr)
{
	pCr->m_MeanPowerDecayRate = (float)m_dEnvelopDecayRate;
	pCr->m_PowerToDeriv3RatioThreshold = float(m_dClickToNoise * m_dClickToNoise);
	pCr->m_MeanPowerAttackRate = float(m_dAttackRate);
	pCr->m_nMaxClickLength = m_MaxClickLength;
	pCr->m_MinDeriv3Threshold = float(m_MinClickAmplitude * m_MinClickAmplitude);
	if (m_bImportClicks)
	{
		pCr->SetClickSourceFile(m_ClickImportFilename);
	}
	if (m_bLogClicks)
	{
		pCr->SetClickLogFile(m_ClickLogFilename);
	}
	pCr->m_PassTrough = m_bLogClicksOnly;
}
/////////////////////////////////////////////////////////////////////////////
// CNoiseReductionDialog dialog


CNoiseReductionDialog::CNoiseReductionDialog(CWnd* pParent /*=NULL*/)
	: BaseClass(CNoiseReductionDialog::IDD, pParent)
{
	//{{AFX_DATA_INIT(CNoiseReductionDialog)
	m_nFftOrderExp = -1;
	m_bUndo = -1;
	//}}AFX_DATA_INIT
	m_dTransientThreshold = 2;
	m_dNoiseReduction = 10.;
	m_dNoiseCriterion = 0.25;
	m_dNoiseThresholdLow = -70.;
	m_dNoiseThresholdHigh = -70.;
	m_dLowerFrequency = 4000.;
	m_FftOrder = 128;
	m_dNoiseReductionAggressivness = 1.;
	m_NearMaskingCoeff = 1.;
}

CNoiseReductionDialog::~CNoiseReductionDialog()
{
	CThisApp * pApp = GetApp();
	pApp->Profile.RemoveSection(_T("NoiseReduction"));
}

void CNoiseReductionDialog::DoDataExchange(CDataExchange* pDX)
{
	BaseClass::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CNoiseReductionDialog)
	DDX_Control(pDX, IDC_STATIC_SELECTION, m_SelectionStatic);
	DDX_Control(pDX, IDC_EDIT_TONE_PREFERENCE, m_eToneOverNoisePreference);
	DDX_Control(pDX, IDC_EDIT_AGGRESSIVNESS, m_EditAggressivness);
	DDX_Control(pDX, IDC_EDIT_NOISE_REDUCTION, m_eNoiseReduction);
	DDX_Control(pDX, IDC_EDIT_NOISE_CRITERION, m_eNoiseCriterion);
	DDX_Control(pDX, IDC_EDIT_NOISE_AREA_THRESHOLD_HIGH, m_eNoiseThresholdHigh);
	DDX_Control(pDX, IDC_EDIT_NOISE_AREA_THRESHOLD_LOW, m_eNoiseThresholdLow);
	DDX_Control(pDX, IDC_EDIT_LOWER_FREQUENCY, m_eLowerFrequency);
	DDX_CBIndex(pDX, IDC_COMBO_FFT_ORDER, m_nFftOrderExp);
	DDX_Check(pDX, IDC_CHECK_UNDO, m_bUndo);
	//}}AFX_DATA_MAP
	m_FftOrder = 256 << m_nFftOrderExp;
#if 0
	m_eTransientThreshold.ExchangeData(pDX, m_dTransientThreshold,
										"Transient threshold", "", 0.3, 2);
#endif
	m_eNoiseReduction.ExchangeData(pDX, m_dNoiseReduction,
									_T("Noise reduction"), _T("dB"), 0., 100.);
	m_eNoiseCriterion.ExchangeData(pDX, m_dNoiseCriterion,
									_T("Noise/continuous criterion"), _T(""), 0.0, 1.);
	m_eNoiseThresholdHigh.ExchangeData(pDX, m_dNoiseThresholdHigh,
										_T("Noise floor for noise in higher frequencies"), _T("dB"), -100., -10.);
	m_eNoiseThresholdLow.ExchangeData(pDX, m_dNoiseThresholdLow,
									_T("Noise floor for noise in lower frequencies"), _T("dB"), -100., -10.);
	m_EditAggressivness.ExchangeData(pDX, m_dNoiseReductionAggressivness,
									_T("Noise suppression aggressiveness"), _T(""), 0.1, 3.);
	m_eToneOverNoisePreference.ExchangeData(pDX, m_dToneOverNoisePreference,
											_T("Tone over noise preference"), _T("dB"), 0., 20.);
	m_eLowerFrequency.ExchangeData(pDX, m_dLowerFrequency,
									_T("Frequency"), _T("Hz"), 100., 48000.);
}


BEGIN_MESSAGE_MAP(CNoiseReductionDialog, BaseClass)
	//{{AFX_MSG_MAP(CNoiseReductionDialog)
	ON_BN_CLICKED(IDC_BUTTON_MORE, OnButtonMore)
	ON_BN_CLICKED(IDC_BUTTON_SELECTION, OnButtonSelection)
	ON_BN_CLICKED(IDC_BUTTON_SET_THRESHOLD, OnButtonSetThreshold)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CNoiseReductionDialog message handlers

/////////////////////////////////////////////////////////////////////////////
// CNoiseReductionPage message handlers
void CNoiseReductionDialog::LoadValuesFromRegistry()
{
	CThisApp * pApp = GetApp();
	//m_bPhaseFilter = (FALSE != pApp->GetProfileInt(_T("NoiseReduction"), _T("PhaseFilter"), FALSE));

	pApp->Profile.AddItem(_T("NoiseReduction"), _T("FftOrder"), m_FftOrder, 2048, 256, 16384);

	if (m_FftOrder >= 16384)
	{
		m_nFftOrderExp = 6;
	}
	else if (m_FftOrder >= 8192)
	{
		m_nFftOrderExp = 5;
	}
	else if (m_FftOrder >= 4096)
	{
		m_nFftOrderExp = 4;
	}
	else if (m_FftOrder >= 2048)
	{
		m_nFftOrderExp = 3;
	}
	else if (m_FftOrder >= 1024)
	{
		m_nFftOrderExp = 2;
	}
	else if (m_FftOrder >= 512)
	{
		m_nFftOrderExp = 1;
	}
	else
	{
		m_nFftOrderExp = 0;
	}

	pApp->Profile.AddItem(_T("NoiseReduction"), _T("TransientThreshold"), m_dTransientThreshold, 1., 0.3, 2);
	pApp->Profile.AddItem(_T("NoiseReduction"), _T("NoiseReduction"), m_dNoiseReduction, 10., 0., 100.);
	pApp->Profile.AddItem(_T("NoiseReduction"), _T("NoiseCriterion"), m_dNoiseCriterion, 0.25, 0., 1.);
	pApp->Profile.AddItem(_T("NoiseReduction"), _T("NoiseThresholdLow"), m_dNoiseThresholdLow, -70., -100., -10.);
	pApp->Profile.AddItem(_T("NoiseReduction"), _T("NoiseThresholdHigh"), m_dNoiseThresholdHigh, -65., -100., -10.);
	pApp->Profile.AddItem(_T("NoiseReduction"), _T("LowerFrequency"), m_dLowerFrequency, 1000., 100., 48000.);
	pApp->Profile.AddItem(_T("NoiseReduction"), _T("ToneOverNoisePreference"), m_dToneOverNoisePreference, 10., 0., 20.);
	pApp->Profile.AddItem(_T("NoiseReduction"), _T("NearMaskingDecayDistanceHigh"), m_NearMaskingDecayDistanceHigh, 500., 1., 2000.);
	pApp->Profile.AddItem(_T("NoiseReduction"), _T("NearMaskingDecayDistanceLow"), m_NearMaskingDecayDistanceLow, 30., 1., 2000.);
	pApp->Profile.AddItem(_T("NoiseReduction"), _T("NearMaskingDecayTimeHigh"), m_NearMaskingDecayTimeHigh, 40., 1., 1000.);
	pApp->Profile.AddItem(_T("NoiseReduction"), _T("NearMaskingDecayTimeLow"), m_NearMaskingDecayTimeLow, 100., 1., 1000.);
	pApp->Profile.AddItem(_T("NoiseReduction"), _T("NearMaskingCoeff"), m_NearMaskingCoeff, 1., 0., 1.);
	pApp->Profile.AddItem(_T("NoiseReduction"), _T("Aggressivness"), m_dNoiseReductionAggressivness, 1., 0.1, 3.);
}

void CNoiseReductionDialog::StoreValuesToRegistry()
{
	CThisApp * pApp = GetApp();
	m_FftOrder = 256 << m_nFftOrderExp;
	pApp->Profile.FlushSection(_T("NoiseReduction"));
}

#define DB_TO_NEPER 0.115129254
#define M_PI        3.14159265358979323846
#define M_PI_2      1.57079632679489661923
#define M_PI_4      0.785398163397448309616

void CNoiseReductionDialog::SetNoiseReductionData(CNoiseReduction * pNr)
{
	//pNr->m_bApplyPhaseFilter = m_bPhaseFilter;
	pNr->m_MinFrequencyToProcess = float(m_dLowerFrequency);
	pNr->m_ThresholdOfTransient = float(m_dTransientThreshold);
	pNr->m_FreqThresholdOfNoiselike = float(M_PI_2 * M_PI_2 * m_dNoiseCriterion * m_dNoiseCriterion);
	pNr->m_MaxNoiseSuppression = float(DB_TO_NEPER * m_dNoiseReduction);
	pNr->m_LevelThresholdForNoiseLow = float(DB_TO_NEPER * (m_dNoiseThresholdLow +111.));
	pNr->m_LevelThresholdForNoiseHigh = float(DB_TO_NEPER * (m_dNoiseThresholdHigh +111.));
	pNr->m_ToneOverNoisePreference = float(DB_TO_NEPER * m_dToneOverNoisePreference);
	pNr->m_NoiseReductionRatio = 0.5 * m_dNoiseReductionAggressivness;

	pNr->m_NearMaskingDecayDistanceLow = float(m_NearMaskingDecayDistanceLow);
	pNr->m_NearMaskingDecayDistanceHigh = float(m_NearMaskingDecayDistanceHigh);

	pNr->m_NearMaskingDecayTimeLow = float(m_NearMaskingDecayTimeLow);
	pNr->m_NearMaskingDecayTimeHigh = float(m_NearMaskingDecayTimeHigh);
	pNr->m_NearMaskingCoeff = float(m_NearMaskingCoeff);
}

/////////////////////////////////////////////////////////////////////////////
// CMoreNoiseDialog dialog


CMoreNoiseDialog::CMoreNoiseDialog(CWnd* pParent /*=NULL*/)
	: BaseClass(CMoreNoiseDialog::IDD, pParent)
{
	//{{AFX_DATA_INIT(CMoreNoiseDialog)
	// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


void CMoreNoiseDialog::DoDataExchange(CDataExchange* pDX)
{
	BaseClass::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CMoreNoiseDialog)
	DDX_Control(pDX, IDC_EDIT_NEAR_MASKING_COEFF, m_eNearMaskingCoeff);
	DDX_Control(pDX, IDC_EDIT_FAR_MASKING_COEFF, m_eFarMaskingCoeff);
	DDX_Control(pDX, IDC_EDIT_MASKING_TIME_LOW, m_eNearMaskingTimeLow);
	DDX_Control(pDX, IDC_EDIT_MASKING_TIME_HIGH, m_eNearMaskingTimeHigh);
	DDX_Control(pDX, IDC_EDIT_NEAR_MASKING_DISTANCE_LOW, m_eNearMaskingDistanceLow);
	DDX_Control(pDX, IDC_EDIT_NEAR_MASKING_DISTANCE_HIGH, m_eNearMaskingDistanceHigh);
	//}}AFX_DATA_MAP
	m_eNearMaskingDistanceHigh.ExchangeData(pDX, m_NearMaskingDecayDistanceHigh,
											_T("Near masking distance in higher frequencies"), _T("Hz"), 1., 2000.);
	m_eNearMaskingDistanceLow.ExchangeData(pDX, m_NearMaskingDecayDistanceLow,
											_T("Near masking distance in lower frequencies"), _T("Hz"), 1., 2000.);
	m_eNearMaskingTimeHigh.ExchangeData(pDX, m_NearMaskingDecayTimeHigh,
										_T("Near masking time in higher frequencies"), _T("ms"), 1., 1000.);
	m_eNearMaskingTimeLow.ExchangeData(pDX, m_NearMaskingDecayTimeLow,
										_T("Near masking time in lower frequencies"), _T("ms"), 1., 1000.);
	m_eNearMaskingCoeff.ExchangeData(pDX, m_NearMaskingCoeff,
									_T("Near masking coefficient"), _T(""), 0., 1.);
}


BEGIN_MESSAGE_MAP(CMoreNoiseDialog, BaseClass)
	//{{AFX_MSG_MAP(CMoreNoiseDialog)
		// NOTE: the ClassWizard will add message map macros here
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CMoreNoiseDialog message handlers

void CNoiseReductionDialog::OnButtonMore()
{
	CMoreNoiseDialog dlg;
	// set the data to dlg
	dlg.m_NearMaskingDecayDistanceLow = m_NearMaskingDecayDistanceLow;
	dlg.m_NearMaskingDecayDistanceHigh = m_NearMaskingDecayDistanceHigh;

	dlg.m_NearMaskingDecayTimeLow = m_NearMaskingDecayTimeLow;
	dlg.m_NearMaskingDecayTimeHigh = m_NearMaskingDecayTimeHigh;
	dlg.m_NearMaskingCoeff = m_NearMaskingCoeff;
	if (IDOK == dlg.DoModal())
	{
		// return the data from dlg
		m_NearMaskingDecayDistanceLow = dlg.m_NearMaskingDecayDistanceLow;
		m_NearMaskingDecayDistanceHigh = dlg.m_NearMaskingDecayDistanceHigh;

		m_NearMaskingDecayTimeLow = dlg.m_NearMaskingDecayTimeLow;
		m_NearMaskingDecayTimeHigh = dlg.m_NearMaskingDecayTimeHigh;
		m_NearMaskingCoeff = dlg.m_NearMaskingCoeff;
	}
}

void CNoiseReductionDialog::OnButtonSelection()
{
	CSelectionDialog dlg(m_Start, m_End, m_CaretPosition, m_Chan + 1, m_FileLength, m_pWf, m_TimeFormat);

	if (IDOK != dlg.DoModal())
	{
		return;
	}
	m_Start = dlg.GetStart();
	m_End = dlg.GetEnd();
	m_Chan = dlg.GetChannel() - 1;
	UpdateSelectionStatic();
}

void CNoiseReductionDialog::UpdateSelectionStatic()
{
	m_SelectionStatic.SetWindowText(GetSelectionText(m_Start, m_End, m_Chan,
													m_pWf->nChannels, m_bLockChannels,
													m_pWf->nSamplesPerSec, m_TimeFormat));
}


BOOL CNoiseReductionDialog::OnInitDialog()
{
	LoadValuesFromRegistry();
	BaseClass::OnInitDialog();

	UpdateSelectionStatic();

	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

void CNoiseReductionDialog::OnOK()
{
	if (!UpdateData(TRUE))
	{
		TRACE0("UpdateData failed during dialog termination.\n");
		// the UpdateData routine will set focus to correct item
		return;
	}
	GetApp()->Profile.FlushSection(_T("NoiseReduction"));
	EndDialog(IDOK);
}

void CNoiseReductionDialog::OnButtonSetThreshold()
{
	if (!UpdateData(TRUE))
	{
		TRACE0("UpdateData failed during dialog termination.\n");
		// the UpdateData routine will set focus to correct item
		return;
	}
	GetApp()->Profile.FlushSection(_T("NoiseReduction"));
	EndDialog(IDC_BUTTON_SET_THRESHOLD);
}

void CExpressionEvaluationDialog::OnSelchangeTabTokens(NMHDR* pNMHDR, LRESULT* pResult)
{
	ShowHideTabDialogs();

	*pResult = 0;
}

void CExpressionEvaluationDialog::ShowHideTabDialogs()
{
	int sel = m_TabTokens.GetCurSel();

	m_FunctionsTabDlg.EnableWindow(0 == sel);
	m_FunctionsTabDlg.ShowWindow((0 == sel) ? SW_SHOWNA : SW_HIDE);

	m_OperandsTabDlg.EnableWindow(1 == sel);
	m_OperandsTabDlg.ShowWindow((1 == sel) ? SW_SHOWNA : SW_HIDE);

	m_SavedExprTabDlg.EnableWindow(2 == sel);
	m_SavedExprTabDlg.ShowWindow((2 == sel) ? SW_SHOWNA : SW_HIDE);

	m_ExpressionTabSelected = sel;
}

BOOL CExpressionEvaluationDialog::OnInitDialog()
{

	m_Profile.AddItem(_T("Settings\\Expressions"), _T("ExpressionGroupSelected"),
					m_SavedExprTabDlg.m_ExpressionGroupSelected, 0, 0, 100);
	m_Profile.AddItem(_T("Settings\\Expressions"), _T("ExpressionSelected"),
					m_SavedExprTabDlg.m_ExpressionSelected, 0, 0, 1000);
	m_Profile.AddItem(_T("Settings\\Expressions"), _T("ExpressionTabSelected"),
					m_ExpressionTabSelected, 0, 0, 2);
	m_Profile.AddItem(_T("Settings\\Expressions"), _T("FrequencyArgument"),
					m_OperandsTabDlg.m_dFrequency, 500., 0., 1000000.);
	m_Profile.AddItem(_T("Settings\\Expressions"), _T("FrequencyArgument1"),
					m_OperandsTabDlg.m_dFrequency1, 500., 0., 1000000.);
	m_Profile.AddItem(_T("Settings\\Expressions"), _T("FrequencyArgument2"),
					m_OperandsTabDlg.m_dFrequency2, 500., 0., 1000000.);
	m_Profile.AddItem(_T("Settings\\Expressions"), _T("FrequencyArgument3"),
					m_OperandsTabDlg.m_dFrequency3, 500., 0., 1000000.);
	m_Profile.AddItem(_T("Settings\\Expressions"), _T("EvaluateExpression"), m_sExpression);

	m_FunctionsTabDlg.Create(IDD_FUNCTIONS_AND_OPERATORS_TAB, this);
	m_OperandsTabDlg.Create(IDD_OPERANDS_TAB, this);
	m_SavedExprTabDlg.Create(IDD_SAVED_EXPRESSIONS_TAB, this);

	BaseClass::OnInitDialog();

	m_OperandsTabDlg.UpdateData(FALSE);
	m_SavedExprTabDlg.UpdateData(FALSE);

	CRect r;
	CWnd * pWnd = GetDlgItem(IDC_STATIC_TAB_INTERIOR);
	if (pWnd)
	{
		pWnd->GetWindowRect( & r);
		ScreenToClient( & r);
		m_OperandsTabDlg.MoveWindow( & r, FALSE);
		m_OperandsTabDlg.EnableToolTips();

		m_FunctionsTabDlg.MoveWindow( & r, FALSE);
		m_FunctionsTabDlg.EnableToolTips();

		m_SavedExprTabDlg.MoveWindow( & r, FALSE);
		m_SavedExprTabDlg.EnableToolTips();
	}

	m_TabTokens.InsertItem(0, _T("Functions And Operators"));
	m_TabTokens.InsertItem(1, _T("Operands"));
	m_TabTokens.InsertItem(2, _T("Saved Expressions"));

	m_TabTokens.SetCurSel(m_ExpressionTabSelected);
	ShowHideTabDialogs();

	UpdateSelectionStatic();
	m_bNeedUpdateControls = TRUE;

	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}


void CExpressionEvaluationDialog::OnButtonSaveExpressionAs()
{
	CString s;
	m_eExpression.GetWindowText(s);
	m_SavedExprTabDlg.SaveExpressionAs(s);
}

LRESULT CExpressionEvaluationDialog::OnKickIdle(WPARAM, LPARAM)
{
	if (m_bNeedUpdateControls)
	{
		UpdateDialogControls(this, FALSE);
	}
	m_SavedExprTabDlg.UpdateDialogControls( & m_SavedExprTabDlg, FALSE);
	m_bNeedUpdateControls = FALSE;
	return 0;
}

void CExpressionEvaluationDialog::OnUpdateOk(CCmdUI* pCmdUI)
{
	pCmdUI->Enable(m_eExpression.GetWindowTextLength() != 0);
}

void CExpressionEvaluationDialog::OnUpdateSaveAs(CCmdUI* pCmdUI)
{
	pCmdUI->Enable(m_eExpression.GetWindowTextLength() != 0);
}


void CExpressionEvaluationDialog::OnChangeEditExpression()
{
	m_bNeedUpdateControls = TRUE;
}

BOOL CDcOffsetDialog::OnInitDialog()
{
	BaseClass::OnInitDialog();

	m_OffsetSpin.SetRange32(-0x8000, 0x7FFF);
	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}
