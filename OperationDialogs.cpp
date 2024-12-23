// Copyright Alexander Grigoriev, 1997-2002, All Rights Reserved
// OperationDialogs.cpp : implementation file
//

#include "stdafx.h"
#define _USE_MATH_DEFINES   // for M_PI definition
#include "WaveSoapFront.h"
#include "resource.h"
#include "OperationContext.h"
#include "OperationContext2.h"
#include "OperationDialogs.h"
#include "MainFrm.h"
#include "SaveExpressionDialog.h"
#include "DialogWithSelection.inl"
#include "FileDialogWithHistory.h"

#include <math.h>

#define DB_TO_NEPER (0.1 * M_LN10)

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CCopyChannelsSelectDlg dialog


CCopyChannelsSelectDlg::CCopyChannelsSelectDlg(CHANNEL_MASK Channels, CWnd* pParent /*=NULL*/)
	: BaseClass(IDD, pParent)
	, m_ChannelToCopy(-1)
{
	//{{AFX_DATA_INIT(CCopyChannelsSelectDlg)
	//}}AFX_DATA_INIT
	CHANNEL_MASK const AllChannels = 3; //WaveFile.ChannelsMask();

	if ((Channels & AllChannels) == AllChannels)
	{
		m_ChannelToCopy = 0;
	}
	else if (Channels & 1)
	{
		m_ChannelToCopy = 1;
	}
	else if (Channels & 2)
	{
		m_ChannelToCopy = 2;
	}
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

	m_eVolumeLeft.SetPrecision(2);
	m_eVolumeRight.SetPrecision(2);
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
									IDS_INPUT_NAME_VOLUME_CHANGE, IDS_DECIBEL, -40., 40.);
		if (m_WaveFile.Channels() > 1)
		{
			m_eVolumeRight.ExchangeData(pDX, m_dVolumeRightDb,
										IDS_INPUT_NAME_VOLUME_CHANGE, IDS_DECIBEL, -40., 40.);
		}
	}
	else
	{
		// percents
		m_eVolumeLeft.ExchangeData(pDX, m_dVolumeLeftPercent,
									IDS_INPUT_NAME_VOLUME_CHANGE, IDS_PERCENT_CHAR, 1., 10000.);
		if (m_WaveFile.Channels() > 1)
		{
			m_eVolumeRight.ExchangeData(pDX, m_dVolumeRightPercent,
										IDS_INPUT_NAME_VOLUME_CHANGE, IDS_PERCENT_CHAR, 1., 10000.);
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
		m_Profile.UnloadAll();
	}
}


BEGIN_MESSAGE_MAP(CVolumeChangeDialog, BaseClass)
//{{AFX_MSG_MAP(CVolumeChangeDialog)
	ON_CBN_SELCHANGE(IDC_COMBODB_PERCENT, OnSelchangeCombodbPercent)
	ON_WM_HSCROLL()
	ON_EN_KILLFOCUS(IDC_EDIT_VOLUME_LEFT, OnKillfocusEditVolumeLeft)
	ON_EN_KILLFOCUS(IDC_EDIT_VOLUME_RIGHT, OnKillfocusEditVolumeRight)

	ON_BN_CLICKED(IDC_CHECKLOCK_CHANNELS, OnBnClickedLockChannels)
	ON_UPDATE_COMMAND_UI(IDC_CHECKLOCK_CHANNELS, OnUpdateLockChannels)

	ON_UPDATE_COMMAND_UI(IDC_EDIT_VOLUME_LEFT, OnUpdateVolumeLeft)
	ON_UPDATE_COMMAND_UI(IDC_STATIC_LEFT_CHANNEL, OnUpdateVolumeLeft)
	ON_UPDATE_COMMAND_UI(IDC_SLIDER_VOLUME_LEFT, OnUpdateVolumeLeft)

	ON_UPDATE_COMMAND_UI(IDC_EDIT_VOLUME_RIGHT, OnUpdateVolumeRight)
	ON_UPDATE_COMMAND_UI(IDC_STATIC_RIGHT_CHANNEL, OnUpdateVolumeRight)
	ON_UPDATE_COMMAND_UI(IDC_SLIDER_VOLUME_RIGHT, OnUpdateVolumeRight)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CVolumeChangeDialog message handlers

void CVolumeChangeDialog::OnUpdateVolumeLeft(CCmdUI * pCmdUI)
{
	// Enable if left channel is selected
	pCmdUI->Enable(0 != (m_Chan & SPEAKER_FRONT_LEFT));
}

void CVolumeChangeDialog::OnUpdateVolumeRight(CCmdUI * pCmdUI)
{
	// Enable if right channel is selected and NOT channels locked together
	pCmdUI->Enable((m_Chan & SPEAKER_FRONT_RIGHT)
					&& ! (IsDlgButtonChecked(IDC_CHECKLOCK_CHANNELS)
						&& (SPEAKER_FRONT_RIGHT | SPEAKER_FRONT_LEFT) == (m_Chan & (SPEAKER_FRONT_RIGHT | SPEAKER_FRONT_LEFT))));
}

void CVolumeChangeDialog::OnUpdateLockChannels(CCmdUI * pCmdUI)
{
	// enable only if both channels are selected
	pCmdUI->Enable((SPEAKER_FRONT_RIGHT | SPEAKER_FRONT_LEFT) == (m_Chan & (SPEAKER_FRONT_RIGHT | SPEAKER_FRONT_LEFT)));
}

void CVolumeChangeDialog::OnBnClickedLockChannels()
{
	NeedUpdateControls();
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
		NeedUpdateControls();
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

/////////////////////////////////////////////////////////////////////////////
// CSelectionUiSupport dialog
CSelectionUiSupport::CSelectionUiSupport(SAMPLE_INDEX Start, SAMPLE_INDEX End, SAMPLE_INDEX CaretPos,
										CHANNEL_MASK Channel,
										CWaveFile & WaveFile, int TimeFormat,
										BOOL bChannelsLocked,
										BOOL bAllowFileExtension)   // standard constructor
	: m_Chan(-1)
	, m_Start(0)
	, m_End(0)
	, m_CaretPosition(CaretPos)
	, m_Length(0)
	, m_TimeFormat(TimeFormat)
	, m_eStart(CaretPos, WaveFile, TimeFormat)
	, m_eEnd(CaretPos, WaveFile, TimeFormat)
	, m_eLength(TimeFormat)
	, m_WaveFile(WaveFile)
	, m_bChannelsLocked(bChannelsLocked)
	, m_bAllowFileExtension(bAllowFileExtension)
{
	if (Start <= End)
	{
		m_Start = Start;
		m_End = End;
	}
	else
	{
		m_Start = End;
		m_End = Start;
	}

	m_Length = m_End - m_Start;

	m_TimeFormatIndex = 0;
	m_SelectionNumber = 0;
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
	case SampleToString_HhMmSsFf:
		m_TimeFormatIndex = 3;
		break;
	}

	m_eLength.SetSamplingRate(WaveFile.SampleRate());

	if (WaveFile.Channels() < 2)
	{
		m_Chan = 0;
	}
	else
	{
		if (m_bChannelsLocked || WaveFile.AllChannels(Channel))
		{
			m_Chan = 0;
		}
		else if (Channel & SPEAKER_FRONT_LEFT)
		{
			m_Chan = 1;
		}
		else if (Channel & SPEAKER_FRONT_RIGHT)
		{
			m_Chan = 2;
		}
	}
}

void CSelectionUiSupport::SetSelection(SAMPLE_INDEX Start, SAMPLE_INDEX End, CHANNEL_MASK Channel)
{
	m_Start = Start;
	m_End = End;
	m_Length = End - Start;

	AdjustSelection(Start, End, End - Start);

	if (m_bChannelsLocked || m_WaveFile.AllChannels(Channel))
	{
		m_Chan = 0;
	}
	else if (Channel & SPEAKER_FRONT_LEFT)
	{
		m_Chan = 1;
	}
	else if (Channel & SPEAKER_FRONT_RIGHT)
	{
		m_Chan = 2;
	}
	// TODO: set radio button
}

void CSelectionUiSupport::InitSelectionUi()
{
	m_eStart.FillFileTimes();
	m_eEnd.FillFileTimes();
	m_eStart.GetComboBox().SetExtendedUI(TRUE);
	m_eEnd.GetComboBox().SetExtendedUI(TRUE);

	NUMBER_OF_SAMPLES FileLength = m_WaveFile.NumberOfSamples();

	if ((0 != m_Start || FileLength != m_End)
		&& (0 != m_Start || m_CaretPosition != m_End)
		&& (m_CaretPosition != m_Start || FileLength != m_End))
	{
		AddSelection(IDS_CURRENT_SELECTION, m_Start, m_End);
	}

	AddSelection(IDS_ALL_SAMPLE_DATA, 0, FileLength);

	if (0 != m_CaretPosition
		&& FileLength != m_CaretPosition)
	{
		AddSelection(IDS_FROM_BEGIN_TO_CURSOR, 0, m_CaretPosition);

		AddSelection(IDS_FROM_CURSOR_TO_END, m_CaretPosition, FileLength);

	}

	CString s;

	WaveFileSegmentVector Segments;
	m_WaveFile.GetSortedFileSegments(Segments, true);

	for (WaveFileSegmentVector::iterator i = Segments.begin();
		i != Segments.end(); i++)
	{
		if (i->Begin > 0
			&& i->Begin < i->End
			&& i->Begin < FileLength
			&& i->End <= FileLength)
		{
			AddSelection(i->Name, i->Begin, i->End);
		}
	}

	UpdateComboSelection();
}

void CSelectionUiSupport::DoDataExchange(CDataExchange* pDX, CWnd * pWnd)
{
	if (NULL != pWnd->GetDlgItem(IDC_COMBO_SELECTION))
	{
		DDX_Control(pDX, IDC_COMBO_SELECTION, m_SelectionCombo);
		DDX_CBIndex(pDX, IDC_COMBO_SELECTION, m_SelectionNumber);
	}
	//{{AFX_DATA_MAP(CSelectionDialog)
	DDX_Control(pDX, IDC_SPIN_START, m_SpinStart);
	DDX_Control(pDX, IDC_SPIN_LENGTH, m_SpinLength);
	DDX_Control(pDX, IDC_SPIN_END, m_SpinEnd);
	DDX_Control(pDX, IDC_EDIT_LENGTH, m_eLength);
	DDX_Control(pDX, IDC_COMBO_START, m_eStart);
	DDX_Control(pDX, IDC_COMBO_END, m_eEnd);

	DDX_Control(pDX, IDC_COMBO_TIME_FORMAT, m_TimeFormatCombo);
	DDX_CBIndex(pDX, IDC_COMBO_TIME_FORMAT, m_TimeFormatIndex);
	//}}AFX_DATA_MAP
	if (NULL != pWnd->GetDlgItem(IDC_RADIO_CHANNEL)
		&& m_WaveFile.Channels() >= 2)
	{
		DDX_Radio(pDX, IDC_RADIO_CHANNEL, m_Chan);
	}

	m_eStart.ExchangeData(pDX, m_Start);
	m_eEnd.ExchangeData(pDX, m_End);
	m_eLength.ExchangeData(pDX, m_Length);

}

void CSelectionUiSupport::OnSelchangeComboTimeFormat()
{
	int sel = m_TimeFormatCombo.GetCurSel();
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
	case 3:
		Format = SampleToString_HhMmSsFf | TimeToHhMmSs_NeedsHhMm;
		break;
	}
	if (Format == m_TimeFormat)
	{
		return;
	}
	m_TimeFormat = Format;

	m_Start = m_eStart.ChangeTimeFormat(Format);

	m_End = m_eEnd.ChangeTimeFormat(Format);

	m_Length = m_End - m_Start;

	m_eLength.ChangeTimeFormat(Format);
	m_eLength.SetTimeSample(m_Length);
}

void CSelectionUiSupport::UpdateAllSelections()
{
	AdjustSelection(m_eStart.UpdateTimeSample(), m_eEnd.UpdateTimeSample(), m_eLength.UpdateTimeSample());
}

void CSelectionUiSupport::AdjustSelection(SAMPLE_INDEX Start, SAMPLE_INDEX End,
										NUMBER_OF_SAMPLES Length)
{
	NUMBER_OF_SAMPLES const FileLength = m_WaveFile.NumberOfSamples();

	if (m_bAllowFileExtension)
	{
		// selection can be extended beyond current file length
		// Start should be in the file length, though
		if (Start > FileLength)
		{
			Start = FileLength;
		}

		if (Start != m_Start)
		{
			// if start is greater than end, set end to start+Length
			if (Start > End)
			{
				End = Start + Length;
			}
		}
		else if (End != m_End)
		{
			if (End < Start)
			{
				if (End < Length)
				{
					Length = End;
				}

				Start = End - Length;
			}
		}
		else
		{
			// length changed, adjust end
			End = Start + Length;
		}

	}
	else
	{
		// force the values into the file length

		if (Start > FileLength)
		{
			Start = FileLength;
		}

		if (Start != m_Start)
		{
			// if start is greater than end, set end to start+Length
			if (Start > End)
			{
				End = Start + Length;
			}
		}
		else if (End != m_End)
		{
			if (End < Start)
			{
				if (End < Length)
				{
					Length = End;
				}

				Start = End - Length;
			}
		}
		else
		{
			// length changed, adjust end
			End = Start + Length;
		}

		if (End > FileLength)
		{
			End = FileLength;
		}
	}

	m_Start = Start;
	m_eStart.SetTimeSample(m_Start);

	m_End = End;
	m_eEnd.SetTimeSample(m_End);

	m_Length = End - Start;
	m_eLength.SetTimeSample(m_Length);
}

void CSelectionUiSupport::UpdateComboSelection()
{
	if (NULL != m_SelectionCombo.m_hWnd)
	{
		m_SelectionCombo.SetCurSel(FindSelection(m_Start, m_End));
	}
}

void CSelectionUiSupport::OnBuddyChangeSpinEnd(NMHDR * /*pNmHdr*/, LRESULT * pResult)
{
	OnKillfocusEditEnd();
	*pResult = 0;
}

void CSelectionUiSupport::OnKillfocusEditEnd()
{
	AdjustSelection(m_Start, m_eEnd.UpdateTimeSample(), m_Length);

	UpdateComboSelection();
}

void CSelectionUiSupport::OnBuddyChangeSpinLength(NMHDR * /*pNmHdr*/, LRESULT * pResult)
{
	OnKillfocusEditLength();
	*pResult = 0;
}

void CSelectionUiSupport::OnKillfocusEditLength()
{
	AdjustSelection(m_Start, m_End, m_eLength.UpdateTimeSample());

	UpdateComboSelection();
}

void CSelectionUiSupport::OnBuddyChangeSpinStart(NMHDR * /*pNmHdr*/, LRESULT * pResult)
{
	OnKillfocusEditStart();
	*pResult = 0;
}

void CSelectionUiSupport::OnKillfocusEditStart()
{
	AdjustSelection(m_eStart.UpdateTimeSample(), m_End, m_Length);

	UpdateComboSelection();
}

void CSelectionUiSupport::OnSelchangeComboSelection()
{
	unsigned sel = m_SelectionCombo.GetCurSel();

	if (sel < m_Selections.size())
	{
		m_Start = m_Selections[sel].begin;
		m_eStart.SetTimeSample(m_Start);

		m_End = m_Selections[sel].end;
		m_eEnd.SetTimeSample(m_End);

		ASSERT(m_Start <= m_End);

		m_Length = m_End - m_Start;

		if (m_Length < 0)
		{
			m_Length = 0;
		}
		m_eLength.SetTimeSample(m_Length);
	}
}

void CSelectionUiSupport::AddSelection(LPCTSTR Name, SAMPLE_INDEX begin, SAMPLE_INDEX end)
{
	if (NULL == m_SelectionCombo.m_hWnd)
	{
		return;
	}
	m_SelectionCombo.AddString(Name);
	Selection s = {begin, end};
	m_Selections.push_back(s);
}

void CSelectionUiSupport::AddSelection(UINT id, SAMPLE_INDEX begin, SAMPLE_INDEX end)
{
	CString s;
	s.LoadString(id);
	AddSelection(s, begin, end);
}

int CSelectionUiSupport::FindSelection(SAMPLE_INDEX begin, SAMPLE_INDEX end)
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

///////////////////////////////////////////////////////////////
// CSelectionDialog dialog
CSelectionDialog::CSelectionDialog(SAMPLE_INDEX Start, SAMPLE_INDEX End,
									SAMPLE_INDEX CaretPos, CHANNEL_MASK Channel,
									CWaveFile & WaveFile,
									int TimeFormat,
									BOOL bChannelsLocked,
									BOOL bAllowFileExtension,
									UINT id,
									CWnd* pParent /*=NULL*/)
	: BaseClass(id, pParent)
	, CSelectionUiSupport(Start, End, CaretPos,
						Channel, WaveFile, TimeFormat, bChannelsLocked, bAllowFileExtension)
{
	if (WaveFile.Channels() < 2
		&& m_lpszTemplateName == MAKEINTRESOURCE(IDD_SELECTION_DIALOG))
	{
		m_lpszTemplateName = MAKEINTRESOURCE(IDD_SELECTION_DIALOG_MONO);
	}
}


void CSelectionDialog::DoDataExchange(CDataExchange* pDX)
{
	BaseClass::DoDataExchange(pDX);
	CSelectionUiSupport::DoDataExchange(pDX, this);
}

void CSelectionDialog::OnOK()
{
	AdjustSelection(m_eStart.GetTimeSample(), m_eEnd.GetTimeSample(), m_eLength.GetTimeSample());
	BaseClass::OnOK();
}

BEGIN_MESSAGE_MAP(CSelectionDialog, BaseClass)
//{{AFX_MSG_MAP(CSelectionDialog)
	ON_CBN_SELCHANGE(IDC_COMBO_TIME_FORMAT, OnSelchangeComboTimeFormat)
	ON_CBN_KILLFOCUS(IDC_COMBO_END, OnKillfocusEditEnd)
	ON_NOTIFY(CTimeSpinCtrl::TSC_BUDDY_CHANGE, IDC_SPIN_END, OnBuddyChangeSpinEnd)
	ON_EN_KILLFOCUS(IDC_EDIT_LENGTH, OnKillfocusEditLength)
	ON_NOTIFY(CTimeSpinCtrl::TSC_BUDDY_CHANGE, IDC_SPIN_LENGTH, OnBuddyChangeSpinLength)
	ON_CBN_KILLFOCUS(IDC_COMBO_START, OnKillfocusEditStart)
	ON_NOTIFY(CTimeSpinCtrl::TSC_BUDDY_CHANGE, IDC_SPIN_START, OnBuddyChangeSpinStart)
	ON_CBN_SELCHANGE(IDC_COMBO_SELECTION, OnSelchangeComboSelection)

	ON_CBN_SELCHANGE(IDC_COMBO_END, OnSelchangeComboEnd)
	ON_CBN_SELCHANGE(IDC_COMBO_START, OnSelchangeComboStart)

	ON_CONTROL(20, IDC_COMBO_START, OnDeferredSelchangeComboStart)
	ON_CONTROL(20, IDC_COMBO_END, OnDeferredSelchangeComboEnd)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CSelectionDialog message handlers

BOOL CSelectionDialog::OnInitDialog()
{
	BaseClass::OnInitDialog();

	InitSelectionUi();
	if (m_bChannelsLocked)
	{
		EnableDlgItem(IDC_RADIO_CHANNEL, FALSE);
		EnableDlgItem(IDC_RADIO1, FALSE);
		EnableDlgItem(IDC_RADIO2, FALSE);
	}
	return TRUE;  // return TRUE unless you set the focus to a control
}

void CSelectionDialog::OnSelchangeComboTimeFormat()
{
	CSelectionUiSupport::OnSelchangeComboTimeFormat();
}

void CSelectionDialog::OnBuddyChangeSpinEnd(NMHDR * pNmHdr, LRESULT * pResult)
{
	CSelectionUiSupport::OnBuddyChangeSpinEnd(pNmHdr, pResult);
}

void CSelectionDialog::OnKillfocusEditEnd()
{
	CSelectionUiSupport::OnKillfocusEditEnd();
}

void CSelectionDialog::OnBuddyChangeSpinLength(NMHDR * pNmHdr, LRESULT * pResult)
{
	CSelectionUiSupport::OnBuddyChangeSpinLength(pNmHdr, pResult);
}

void CSelectionDialog::OnKillfocusEditLength()
{
	CSelectionUiSupport::OnKillfocusEditLength();
}

void CSelectionDialog::OnBuddyChangeSpinStart(NMHDR * pNmHdr, LRESULT * pResult)
{
	CSelectionUiSupport::OnBuddyChangeSpinStart(pNmHdr, pResult);
}

void CSelectionDialog::OnKillfocusEditStart()
{
	CSelectionUiSupport::OnKillfocusEditStart();
}

void CSelectionDialog::OnSelchangeComboSelection()
{
	CSelectionUiSupport::OnSelchangeComboSelection();
}

void CSelectionDialog::OnSelchangeComboStart()
{
	PostMessage(WM_COMMAND, MAKEWPARAM(IDC_COMBO_START, 20), LPARAM(m_eStart.m_hWnd));
}

void CSelectionDialog::OnSelchangeComboEnd()
{
	PostMessage(WM_COMMAND, MAKEWPARAM(IDC_COMBO_END, 20), LPARAM(m_eStart.m_hWnd));
}

void CSelectionDialog::OnDeferredSelchangeComboStart()
{
	CSelectionUiSupport::OnKillfocusEditStart();
}

void CSelectionDialog::OnDeferredSelchangeComboEnd()
{
	CSelectionUiSupport::OnKillfocusEditEnd();
}

/////////////////////////////////////////////////////////////////////////////
// CGotoDialog dialog

CGotoDialog::CGotoDialog(SAMPLE_INDEX Position,
						CWaveFile & WaveFile,
						int TimeFormat, CWnd* pParent /*=NULL*/)
	: BaseClass(IDD, pParent),
	m_Position(Position),
	m_TimeFormat(TimeFormat),
	m_eStart(Position, WaveFile, TimeFormat)
	, m_WaveFile(WaveFile)
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
	case SampleToString_HhMmSsFf:
		m_TimeFormatIndex = 3;
		break;
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
	ON_CBN_KILLFOCUS(IDC_COMBO_START, OnKillfocusEditStart)
	ON_NOTIFY(CTimeSpinCtrl::TSC_BUDDY_CHANGE, IDC_SPIN_START, OnBuddyChangeSpinStart)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CGotoDialog message handlers
/////////////////////////////////////////////////////////////////////////////
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
	case 3:
		Format = SampleToString_HhMmSsFf | TimeToHhMmSs_NeedsHhMm;
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

	m_eStart.FillFileTimes();

	m_eStart.GetComboBox().SetExtendedUI(TRUE);

	return TRUE;  // return TRUE unless you set the focus to a control
}

void CGotoDialog::OnKillfocusEditStart()
{
	m_Position = m_eStart.GetTimeSample();
	if (m_Position > m_WaveFile.NumberOfSamples())
	{
		m_Position = m_WaveFile.NumberOfSamples();
	}

	m_eStart.SetTimeSample(m_Position);
}

void CGotoDialog::OnBuddyChangeSpinStart(NMHDR * /*pNmHdr*/, LRESULT * /*pResult*/)
{
	OnKillfocusEditStart();
}

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
	m_bLockChannels = ChannelsLocked;
	m_bUndo = UndoEnabled;
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
	DDV_MinMaxInt(pDX, m_nDcOffset, -0x8000, 0x7FFF);
	DDX_Radio(pDX, IDC_RADIO_DC_SELECT, m_DcSelectMode);
	//}}AFX_DATA_MAP
	if (pDX->m_bSaveAndValidate)
	{
		m_Profile.UnloadAll();
	}
}

BEGIN_MESSAGE_MAP(CDcOffsetDialog, BaseClass)
	//{{AFX_MSG_MAP(CDcOffsetDialog)
	ON_BN_CLICKED(IDC_RADIO_DC_SELECT, OnRadioDcChange)
	ON_BN_CLICKED(IDC_RADIO_DC_EDIT, OnRadioDcChange)
	ON_UPDATE_COMMAND_UI(IDC_CHECK_5SECONDS, OnUpdate5SecondsDcCompute)
	ON_UPDATE_COMMAND_UI(IDC_EDIT_DC_OFFSET, OnUpdateDcOffsetEdit)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CDcOffsetDialog message handlers

void CDcOffsetDialog::OnUpdate5SecondsDcCompute(CCmdUI * pCmdUI)
{
	// compare with TRUE (1) to avoid Undefined state (2), too
	pCmdUI->Enable(TRUE == IsDlgButtonChecked(IDC_RADIO_DC_SELECT));
}

void CDcOffsetDialog::OnUpdateDcOffsetEdit(CCmdUI * pCmdUI)
{
	// compare with TRUE (1) to avoid Undefined state (2), too
	pCmdUI->Enable(TRUE == IsDlgButtonChecked(IDC_RADIO_DC_EDIT));
}

void CDcOffsetDialog::OnRadioDcChange()
{
	NeedUpdateControls();
}

BOOL CDcOffsetDialog::OnInitDialog()
{
	BaseClass::OnInitDialog();

	m_OffsetSpin.SetRange32(-0x8000, 0x7FFF);
	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}
/////////////////////////////////////////////////////////////////////////////
// CStatisticsDialog dialog


CStatisticsDialog::CStatisticsDialog(class CStatisticsContext * pContext,
									CWaveFile & WaveFile, SAMPLE_INDEX CaretPosition,
									LPCTSTR FileName,
									CWnd* pParent /*=NULL*/)
	: BaseClass(IDD, pParent)
	, m_pContext(pContext)
	, m_CaretPosition(CaretPosition)
	, m_SamplesPerSec(WaveFile.SampleRate())
	, m_sFilename(FileName)
{
	// FIXME: handle 32 bit formats
	// read sample value at cursor
	memzero(m_ValueAtCursor);
	memzero(m_ValueAtCursor32);
	memzero(m_fValueAtCursor);

	if (m_CaretPosition < WaveFile.NumberOfSamples())
	{
		void * buf;
		WaveSampleType type = WaveFile.GetSampleType();
		switch (type)
		{
		case SampleType16bit:
			buf = &m_ValueAtCursor;
			break;
		case SampleType32bit:
			buf = &m_ValueAtCursor32;
			break;
		case SampleTypeFloat32:
			buf = &m_fValueAtCursor;
			break;
		default:
			ASSERT(type == SampleType16bit || type == SampleType32bit || type == SampleTypeFloat32);
			return;
		}
		WaveFile.ReadSamples(ALL_CHANNELS, WaveFile.SampleToPosition(m_CaretPosition), 1, buf, type);
	}
	//{{AFX_DATA_INIT(CStatisticsDialog)
	// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


void CStatisticsDialog::DoDataExchange(CDataExchange* pDX)
{
	BaseClass::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CStatisticsDialog)
	DDX_Control(pDX, IDC_STATIC_FILE_NAME, m_FileName);
	DDX_Control(pDX, IDC_EDIT, m_EditBox);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CStatisticsDialog, BaseClass)
	//{{AFX_MSG_MAP(CStatisticsDialog)
	//}}AFX_MSG_MAP
	ON_BN_CLICKED(IDC_BUTTON_GOTO_MAX, OnBnClickedButtonGotoMax)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CStatisticsDialog message handlers

BOOL CStatisticsDialog::OnInitDialog()
{
	BaseClass::OnInitDialog();

	CString s;
	s.Preallocate(1023);

	s.Format(IDS_STAT_DIALOG_FILE_NAME_FORMAT, LPCTSTR(m_sFilename));
	m_FileName.SetWindowText(s);

	int nSampleSize = m_pContext->m_SrcFile.SampleSize();

	NUMBER_OF_SAMPLES nSamples = NUMBER_OF_SAMPLES((m_pContext->m_SrcPos - m_pContext->m_SrcStart) / nSampleSize);

	if (0 == nSamples)
	{
		nSamples = 1;
	}
	CString format_channel;
	format_channel.LoadStringW(IDS_STATISTICS_FORMAT_LINE_CHANNEL);
	CString format1;
	format1.LoadStringW(IDS_STATISTICS_FORMAT_LINE1);
	CString format2;
	format2.LoadStringW(IDS_STATISTICS_FORMAT_LINE2);
	CString format3;
	format3.LoadStringW(IDS_STATISTICS_FORMAT_LINE3);
	CString format4;
	format4.LoadStringW(IDS_STATISTICS_FORMAT_LINE4);
	CString format5;
	format5.LoadStringW(IDS_STATISTICS_FORMAT_LINE5);
	CString format6;
	format6.LoadStringW(IDS_STATISTICS_FORMAT_LINE6);
	CString format7;
	format7.LoadStringW(IDS_STATISTICS_FORMAT_LINE7);
	CString format8;
	format8.LoadStringW(IDS_STATISTICS_FORMAT_LINE8);
	CString format9;
	format9.LoadStringW(IDS_STATISTICS_FORMAT_LINE9);
	CString format10;
	format10.LoadStringW(IDS_STATISTICS_FORMAT_LINE10);
	CString format11;
	format11.LoadStringW(IDS_STATISTICS_FORMAT_LINE11);

	CString text;
	text.Preallocate(4096);

	WaveSampleType SampleType = m_pContext->m_SrcFile.GetSampleType();

	for (int ch = 0; ch < m_pContext->m_SrcFile.Channels(); ch++)
	{
		CString AtCursorDb;
		CString MinDb;
		CString MaxDb;
		CString RmsDb;
		CString DcDb;
		CString AtCursor;
		CString Min;
		CString Max;
		CString Rms;
		CString Dc;
		double ValueAtCursorPercent = 0.;
		double MinPercent = 0.;
		double MaxPercent = 0.;
		double DcPercent = 0.;
		double RmsPercent = 0.;

		CStatisticsContext::ChannelStats const * stats = &m_pContext->m_Proc.m_Stats[ch];

		if (SampleType == SampleType16bit)
		{
			if (m_ValueAtCursor[ch] != 0)
			{
				AtCursorDb.Format(_T("%.2f"), 20. * log10(fabs(m_ValueAtCursor[ch]) / 32768.));
			}
			else
			{
				AtCursorDb = _T("-Inf.");
			}
			AtCursor = LtoaCS(m_ValueAtCursor[ch]);
			ValueAtCursorPercent = m_ValueAtCursor[ch] / 327.68;

			if (stats->m_Min != 0)
			{
				MinDb.Format(_T("%.2f"), 20. * log10(fabs(stats->m_Min) / 32768.));
			}
			else
			{
				MinDb = _T("-Inf.");
			}
			Min = LtoaCS(long(stats->m_Min));
			MinPercent = stats->m_Min / 327.68;

			if (stats->m_Max != 0)
			{
				MaxDb.Format(_T("%.2f"), 20. * log10(fabs(stats->m_Max) / 32768.));
			}
			else
			{
				MaxDb = _T("-Inf.");
			}
			Max = LtoaCS(long(stats->m_Max));
			MaxPercent = stats->m_Max / 327.68;

			if (stats->m_Energy != 0)
			{
				RmsDb.Format(_T("%.2f"),
							10. * log10(stats->m_Energy / (nSamples * 1073741824.)));
			}
			else
			{
				RmsDb = _T("-Inf.");
			}
			Rms = LtoaCS(long(sqrt(stats->m_Energy / nSamples)));
			RmsPercent = 100. * sqrt(stats->m_Energy / (nSamples * 1073741824.));

			if (stats->m_Sum / nSamples != 0)
			{
				DcDb.Format(_T("%.2f"),
							20. * log10(fabs(stats->m_Sum) / (nSamples * 32768.)));
			}
			else
			{
				DcDb = _T("-Inf.");
			}
			Dc = LtoaCS(long(stats->m_Sum / nSamples));
			DcPercent = (stats->m_Sum / nSamples) / 327.68;
		}
		else if (SampleType == SampleType32bit)
		{
			if (m_ValueAtCursor32[ch] != 0)
			{
				AtCursorDb.Format(_T("%.2f"), 20. * log10(fabs(m_ValueAtCursor32[ch]) / (32768.*65536.)));
			}
			else
			{
				AtCursorDb = _T("-Inf.");
			}
			AtCursor = LtoaCS(m_ValueAtCursor32[ch]);
			ValueAtCursorPercent = m_ValueAtCursor32[ch] / (327.68*65536.);

			if (stats->m_Min != 0)
			{
				MinDb.Format(_T("%.2f"), 20. * log10(fabs(stats->m_Min) / (32768.*65536.)));
			}
			else
			{
				MinDb = _T("-Inf.");
			}
			Min = LtoaCS(long(stats->m_Min));
			MinPercent = stats->m_Min / (327.68*65536.);

			if (stats->m_Max != 0)
			{
				MaxDb.Format(_T("%.2f"), 20. * log10(fabs(stats->m_Max) / (32768.*65536.)));
			}
			else
			{
				MaxDb = _T("-Inf.");
			}
			Max = LtoaCS(long(stats->m_Max));
			MaxPercent = stats->m_Max / (327.68*65536.);

			if (stats->m_Energy != 0)
			{
				RmsDb.Format(_T("%.2f"),
							10. * log10(stats->m_Energy / (nSamples * (32768.*65536.*32768.*65536.))));
			}
			else
			{
				RmsDb = _T("-Inf.");
			}
			Rms = LtoaCS(long(sqrt(stats->m_Energy / nSamples)));
			RmsPercent = sqrt(stats->m_Energy / (nSamples * (32.768*65536.*32.768*65536.)));

			if (stats->m_Sum / nSamples != 0)
			{
				DcDb.Format(_T("%.2f"),
							20. * log10(fabs(stats->m_Sum) / (nSamples * (32768.*65536.))));
			}
			else
			{
				DcDb = _T("-Inf.");
			}
			Dc = LtoaCS(long(stats->m_Sum / nSamples));
			DcPercent = (stats->m_Sum / nSamples) / (327.68*65536.);
		}
		else if(SampleType == SampleTypeFloat32)
		{
			if (m_fValueAtCursor[ch] != 0)
			{
				AtCursorDb.Format(_T("%.2f"), 20. * log10(fabs(m_fValueAtCursor[ch])));
			}
			else
			{
				AtCursorDb = _T("-Inf.");
			}
			AtCursor.Format(_T("%.6f"), m_fValueAtCursor[ch]);
			ValueAtCursorPercent = m_fValueAtCursor[ch] / 100.;

			if (stats->m_Min != 0)
			{
				MinDb.Format(_T("%.2f"), 20. * log10(fabs(stats->m_Min)));
			}
			else
			{
				MinDb = _T("-Inf.");
			}
			Min.Format(_T("%.6f"), stats->m_Min);
			MinPercent = stats->m_Min / 100.;

			if (stats->m_Max != 0)
			{
				MaxDb.Format(_T("%.2f"), 20. * log10(fabs(stats->m_Max)));
			}
			else
			{
				MaxDb = _T("-Inf.");
			}
			Max.Format(_T("%.6f"), stats->m_Max);
			MaxPercent = stats->m_Max / 100.;

			if (stats->m_Energy != 0)
			{
				RmsDb.Format(_T("%.2f"),
							10. * log10(stats->m_Energy / nSamples));
			}
			else
			{
				RmsDb = _T("-Inf.");
			}
			Rms.Format(_T("%.6f"), sqrt(stats->m_Energy / nSamples));
			RmsPercent = 100. * sqrt(stats->m_Energy / nSamples);

			if (stats->m_Sum / nSamples != 0)
			{
				DcDb.Format(_T("%.2f"),
							20. * log10(fabs(stats->m_Sum) / nSamples));
			}
			else
			{
				DcDb = _T("-Inf.");
			}
			Dc.Format(_T("%.6f"), stats->m_Sum / nSamples);
			DcPercent = (stats->m_Sum / nSamples) / 100.;
		}

		if (m_pContext->m_SrcFile.Channels() > 1)
		{
			//Channel: %d\r\n
			s.Format(format_channel, ch);
			text += s;
		}
		//Cursor position:\t%s (%s)\r\n
		s.Format(format1, LPCTSTR(SampleToString(m_CaretPosition, m_SamplesPerSec,
												SampleToString_HhMmSs | TimeToHhMmSs_NeedsMs | TimeToHhMmSs_NeedsHhMm)),
				LPCTSTR(SampleToString(m_CaretPosition, m_SamplesPerSec, SampleToString_Sample)));
		text += s;

		// Sample value at cursor:\t%s (%s dB; %.2f%%)\r\n\r\n
		s.Format(format2, LPCTSTR(AtCursor), LPCTSTR(AtCursorDb), ValueAtCursorPercent);
		text += s;

		// Max negative peak position:\t%s (%s)\r\n
		SAMPLE_INDEX MinPosSample = stats->m_PosMin;
		s.Format(format3, LPCTSTR(SampleToString(MinPosSample, m_SamplesPerSec,
												SampleToString_HhMmSs | TimeToHhMmSs_NeedsMs | TimeToHhMmSs_NeedsHhMm)),
				LPCTSTR(SampleToString(MinPosSample, m_SamplesPerSec, SampleToString_Sample)));
		text += s;

		// Max negative peak value:\t%s (%s dB; %.2f%%)\r\n\r\n
		s.Format(format4, LPCTSTR(Min), LPCTSTR(MinDb), MinPercent);
		text += s;

		// Max positive peak position:\t%s (%s)\r\n
		SAMPLE_INDEX MaxPosSample = stats->m_PosMax;
		s.Format(format5, LPCTSTR(SampleToString(MaxPosSample, m_SamplesPerSec,
												SampleToString_HhMmSs | TimeToHhMmSs_NeedsMs | TimeToHhMmSs_NeedsHhMm)),
				LPCTSTR(SampleToString(MaxPosSample, m_SamplesPerSec, SampleToString_Sample)));
		text += s;

		// Max positive peak value:\t%s dB (%.2f%%)\r\n\r\n
		s.Format(format6, LPCTSTR(Max), LPCTSTR(MaxDb), MaxPercent);
		text += s;

		// RMS power:\t%s (%s dB; %.2f%%)\r\n
		s.Format(format7, LPCTSTR(Rms), LPCTSTR(RmsDb), RmsPercent);
		text += s;

		// Average value (DC offset):%s (%s dB; %.2f%%)\r\n
		s.Format(format8, LPCTSTR(Dc), LPCTSTR(DcDb), DcPercent);
		text += s;

		// Zero crossings:\t%.2f Hz\r\n\r\n
		s.Format(format9, stats->m_ZeroCrossing / double(nSamples) * m_SamplesPerSec);
		text += s;

		if (SampleType != SampleTypeFloat32)
		{
			// CRC:\t%08X\r\n
			s.Format(format10, stats->m_CRC32);
			text += s;
		}
		text += L"\r\n\r\n";

	}

	if (SampleType == SampleType16bit)
	{
		// Checksum:%08X\r\n
		s.Format(format11, m_pContext->m_Proc.m_Checksum);
		text += s;
	}

	INT TabStops[2] = { 90, 110 };
	m_EditBox.SetTabStops(2, TabStops);
	m_EditBox.ReplaceSel(text);
	m_EditBox.SetSel(0, 0);
	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

void CStatisticsDialog::OnBnClickedButtonGotoMax()
{
	EndDialog(IDC_BUTTON_GOTO_MAX);
}

SAMPLE_INDEX CStatisticsDialog::GetMaxSamplePosition(unsigned * pChannel) const
{
	if (NULL != m_pContext)
	{
		return m_pContext->GetMaxSamplePosition(pChannel);
	}
	return 0;
}

/////////////////////////////////////////////////////////////////////////////
// CNormalizeSoundDialog dialog


CNormalizeSoundDialog::CNormalizeSoundDialog(SAMPLE_INDEX begin,
											SAMPLE_INDEX end, SAMPLE_INDEX caret,
											CHANNEL_MASK Channels,
											CWaveFile & File,
											BOOL ChannelsLocked, BOOL UndoEnabled,
											int TimeFormat,
											CWnd* pParent /*=NULL*/)
	: BaseClass(begin, end, caret, Channels, File, TimeFormat,
				IDD, pParent)
{
	m_bLockChannels = ChannelsLocked;
	m_bUndo = UndoEnabled;
	//{{AFX_DATA_INIT(CNormalizeSoundDialog)
	m_DbPercent = -1;
	//}}AFX_DATA_INIT
	m_Profile.AddItem(_T("Settings"), _T("NormalizeDialogDbPercents"), m_DbPercent, 0, 0, 1);
	m_Profile.AddItem(_T("Settings"), _T("NormalizeLevelDb"), m_dLevelDb, -6., -40., 0.);
	m_Profile.AddItem(_T("Settings"), _T("NormalizeLevelPercent"), m_dLevelPercent, 50., 1., 100.);

	m_eLevel.SetPrecision(2);
}


void CNormalizeSoundDialog::DoDataExchange(CDataExchange* pDX)
{
	BaseClass::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CNormalizeSoundDialog)
	DDX_Control(pDX, IDC_SLIDER_LEVEL, m_SliderLevel);
	DDX_Control(pDX, IDC_EDIT_LEVEL, m_eLevel);
	DDX_Check(pDX, IDC_CHECK_LOCK_CHANNELS, m_bLockChannels);
	DDX_Check(pDX, IDC_CHECK_UNDO, m_bUndo);
	//}}AFX_DATA_MAP
	if (0 == m_DbPercent)
	{
		// decibels
		m_eLevel.ExchangeData(pDX, m_dLevelDb,
							IDS_INPUT_NAME_TARGET_LEVEL, IDS_DECIBEL, -40., 0.);
	}
	else
	{
		// percents
		m_eLevel.ExchangeData(pDX, m_dLevelPercent,
							IDS_INPUT_NAME_TARGET_LEVEL, IDS_PERCENT_CHAR, 1., 100.);
	}
	DDX_CBIndex(pDX, IDC_COMBODB_PERCENT, m_DbPercent);
	if ( ! pDX->m_bSaveAndValidate)
	{
		NeedUpdateControls();
	}
	else
	{
		m_Profile.UnloadAll();
	}
}


BEGIN_MESSAGE_MAP(CNormalizeSoundDialog, BaseClass)
	//{{AFX_MSG_MAP(CNormalizeSoundDialog)
	ON_EN_KILLFOCUS(IDC_EDIT_LEVEL, OnKillfocusEditLevel)
	ON_CBN_SELCHANGE(IDC_COMBODB_PERCENT, OnSelchangeCombodbPercent)
	ON_WM_HSCROLL()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

double CNormalizeSoundDialog::GetLimitLevel() const
{
	if (0 == m_DbPercent)   // dBs
	{
		return pow(10., m_dLevelDb / 20.);
	}
	else // percents
	{
		return 0.01 * m_dLevelPercent;
	}
}

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
	if (1 == m_WaveFile.Channels())
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

/////////////////////////////////////////////////////////////////////////////
// CResampleDialog dialog


CResampleDialog::CResampleDialog(BOOL bUndoEnabled,
								unsigned long OldSampleRate,
								bool CanOnlyChangeSampleRate,
								CWnd* pParent /*=NULL*/)
	: BaseClass(CResampleDialog::IDD, pParent)
	, m_bUndo(bUndoEnabled)
	, m_bCanOnlyChangeSamplerate(CanOnlyChangeSampleRate)
	, m_OldSampleRate(OldSampleRate)
{
	//{{AFX_DATA_INIT(CResampleDialog)
	m_bChangeRateOnly = FALSE;
	m_bChangeSamplingRate = -1;
	m_NewSampleRate = 0;
	//}}AFX_DATA_INIT

	m_Profile.AddItem(_T("Settings"), _T("ResampleChangeRateOnly"), m_bChangeRateOnly, 0, 0, 1);
	m_Profile.AddItem(_T("Settings"), _T("ResampleChangeSamplingRate"), m_bChangeSamplingRate, 1, 0, 1);
	m_Profile.AddItem(_T("Settings"), _T("ResampleTempoChange"), m_TempoChange, 100., 25., 400.);
	m_Profile.AddItem(_T("Settings"), _T("ResampleNewSampleRate"), m_NewSampleRate, 44100, 150, 1000000);

	m_EditTempo.SetPrecision(3);
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
	DDV_MinMaxUInt(pDX, m_NewSampleRate, m_OldSampleRate / 6, m_OldSampleRate * 6);

	m_EditTempo.ExchangeData(pDX, m_TempoChange,
							IDS_INPUT_NAME_TEMPO_CHANGE, IDS_PERCENT_CHAR, 25., 400.);

	if (pDX->m_bSaveAndValidate)
	{
		if (m_bCanOnlyChangeSamplerate)
		{
			m_Profile.RevertItemToInitial(_T("Settings"), _T("ResampleChangeRateOnly"));
		}

		m_Profile.UnloadAll();
	}
}


BEGIN_MESSAGE_MAP(CResampleDialog, BaseClass)
	//{{AFX_MSG_MAP(CResampleDialog)
	ON_BN_CLICKED(IDC_RADIO_CHANGE_RATE, OnRadioChangeRate)
	ON_BN_CLICKED(IDC_RADIO_CHANGE_PITCH, OnRadioChangeTempo)
	ON_WM_HSCROLL()
	ON_EN_KILLFOCUS(IDC_EDIT_RATE, OnKillfocusEditRate)
	ON_EN_KILLFOCUS(IDC_EDIT_TEMPO, OnKillfocusEditTempo)

	ON_UPDATE_COMMAND_UI(IDC_EDIT_RATE, OnUpdateRateControls)
	ON_UPDATE_COMMAND_UI(IDC_SLIDER_RATE, OnUpdateRateControls)
	ON_UPDATE_COMMAND_UI(IDC_EDIT_TEMPO, OnUpdateTempoControls)
	ON_UPDATE_COMMAND_UI(IDC_SLIDER_TEMPO, OnUpdateTempoControls)
	ON_UPDATE_COMMAND_UI(IDC_RADIO_CHANGE_PITCH, OnUpdateRadioTempo)
	ON_UPDATE_COMMAND_UI(IDC_CHECK_CHANGE_RATE_ONLY, OnUpdateRadioTempo)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CResampleDialog message handlers

void CResampleDialog::OnRadioChangeRate()
{
	m_bChangeSamplingRate = 1;
	NeedUpdateControls();
}

void CResampleDialog::OnRadioChangeTempo()
{
	m_bChangeSamplingRate = 0;
	NeedUpdateControls();
}

void CResampleDialog::OnUpdateRateControls(CCmdUI * pCmdUI)
{
	pCmdUI->Enable(m_bCanOnlyChangeSamplerate || m_bChangeSamplingRate);
}

void CResampleDialog::OnUpdateTempoControls(CCmdUI * pCmdUI)
{
	pCmdUI->Enable(! m_bCanOnlyChangeSamplerate && ! m_bChangeSamplingRate);
}

void CResampleDialog::OnUpdateRadioTempo(CCmdUI * pCmdUI)
{
	pCmdUI->Enable(! m_bCanOnlyChangeSamplerate);
}

void CResampleDialog::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
	BaseClass::OnHScroll(nSBCode, nPos, pScrollBar);
	int nId = pScrollBar->GetDlgCtrlID();
	if (IDC_SLIDER_TEMPO == nId
		&& m_SliderTempo.m_hWnd != NULL)
	{
		SetDlgItemInt(IDC_EDIT_TEMPO, m_SliderTempo.GetPos());
	}
	else if (IDC_SLIDER_RATE == nId
			&& m_SliderRate.m_hWnd != NULL)
	{
		SetDlgItemInt(IDC_EDIT_RATE, m_SliderRate.GetPos());
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
		UINT val = GetDlgItemInt(IDC_EDIT_RATE, & NoErr, FALSE);

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

	ASSERT(0 != m_OldSampleRate);
	ASSERT(0 != m_NewSampleRate);

	m_SliderTempo.SetRange(25, 400);
	m_SliderTempo.SetTicFreq(25);
	m_SliderTempo.SetLineSize(5);
	m_SliderTempo.SetPageSize(25);
	m_SliderTempo.SetPos(MulDiv(100, m_NewSampleRate, m_OldSampleRate));

	m_SliderRate.SetRange(m_OldSampleRate / 4, m_OldSampleRate * 4);
	m_SliderRate.SetTicFreq(m_OldSampleRate / 4);
	m_SliderRate.SetLineSize(m_OldSampleRate / 20);
	m_SliderRate.SetPageSize(m_OldSampleRate / 4);
	m_SliderRate.SetPos(m_NewSampleRate);

	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

/////////////////////////////////////////////////////////////////////////////
// CLowFrequencySuppressDialog dialog


CLowFrequencySuppressDialog::CLowFrequencySuppressDialog(
														SAMPLE_INDEX begin, SAMPLE_INDEX end, SAMPLE_INDEX caret,
														CHANNEL_MASK Channels,
														CWaveFile & File,
														BOOL ChannelsLocked, BOOL UndoEnabled,
														int TimeFormat,
														CWnd* pParent /*=NULL*/)
	: BaseClass(begin, end, caret, Channels, File,
				TimeFormat, IDD, pParent)
	, m_DifferentialModeSuppress(TRUE)
	, m_LowFrequencySuppress(TRUE)
	, m_dLfNoiseRange(20.)
	, m_dDiffNoiseRange(200.)
{
	m_bUndo = UndoEnabled;
	m_bLockChannels = ChannelsLocked;
	//{{AFX_DATA_INIT(CLowFrequencySuppressDialog)
	//}}AFX_DATA_INIT
	m_Profile.AddItem(_T("Settings"), _T("SuppressDifferentialRange"), m_dDiffNoiseRange, 200., 1., 1000.);
	m_Profile.AddItem(_T("Settings"), _T("SuppressLowFreqRange"), m_dLfNoiseRange, 20., 1., 1000.);
	m_Profile.AddBoolItem(_T("Settings"), _T("SuppressDifferential"), m_DifferentialModeSuppress, TRUE);
	m_Profile.AddBoolItem(_T("Settings"), _T("SuppressLowFrequency"), m_LowFrequencySuppress, TRUE);

	m_eLfNoiseRange.SetPrecision(2);
	m_eDiffNoiseRange.SetPrecision(2);
}


void CLowFrequencySuppressDialog::DoDataExchange(CDataExchange* pDX)
{
	BaseClass::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CLowFrequencySuppressDialog)
	DDX_Control(pDX, IDC_EDIT_LF_NOISE_RANGE, m_eLfNoiseRange);
	DDX_Control(pDX, IDC_EDIT_DIFF_NOISE_RANGE, m_eDiffNoiseRange);
	DDX_Check(pDX, IDC_CHECK_DIFFERENTIAL_MODE_SUPPRESS, m_DifferentialModeSuppress);
	DDX_Check(pDX, IDC_CHECK_LOW_FREQUENCY, m_LowFrequencySuppress);
	//}}AFX_DATA_MAP
	DDX_Check(pDX, IDC_CHECK_UNDO, m_bUndo);

	m_eLfNoiseRange.ExchangeData(pDX, m_dLfNoiseRange,
								IDS_INPUT_NAME_LF_SUPPRESSION_RANGE, IDS_HERTZ, 1., 1000.);
	m_eDiffNoiseRange.ExchangeData(pDX, m_dDiffNoiseRange,
									IDS_INPUT_NAME_DIFFERENTIAL_SUPPRESSION_RANGE, IDS_HERTZ, 1., 1000.);

	if (pDX->m_bSaveAndValidate)
	{
		m_Profile.UnloadAll();
	}
}


BEGIN_MESSAGE_MAP(CLowFrequencySuppressDialog, BaseClass)
	//{{AFX_MSG_MAP(CLowFrequencySuppressDialog)
	ON_BN_CLICKED(IDC_CHECK_DIFFERENTIAL_MODE_SUPPRESS, OnCheckDifferentialModeSuppress)
	ON_BN_CLICKED(IDC_CHECK_LOW_FREQUENCY, OnCheckLowFrequency)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CLowFrequencySuppressDialog message handlers

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

	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}
/////////////////////////////////////////////////////////////////////////////
// CDeclickDialog dialog
CDeclickDialog::CDeclickDialog(SAMPLE_INDEX begin, SAMPLE_INDEX end, SAMPLE_INDEX caret,
								CHANNEL_MASK Channels,
								CWaveFile & File,
								BOOL ChannelsLocked, BOOL UndoEnabled,
								int TimeFormat,
								CWnd* pParent /*=NULL*/)
	: BaseClass(begin, end, caret, Channels, File,
				TimeFormat, IDD, pParent)
{
	m_bUndo = UndoEnabled;
	m_bLockChannels = ChannelsLocked;
	//{{AFX_DATA_INIT(CDeclickDialog)
	m_ClickLogFilename = _T("");
	m_nMaxClickLength = 24;
	m_MinClickAmplitude = 200;
	m_bLogClicks = FALSE;
	m_bLogClicksOnly = FALSE;
	m_bImportClicks = FALSE;
	m_ClickImportFilename = _T("");
	m_bDontAutodetectClicks = FALSE;
	//}}AFX_DATA_INIT
	m_MeanPowerAttackRate = .06;
	m_dClickToNoise = 4.;
	m_MeanPowerDecayRate = 0.02;

	LoadValuesFromRegistry();

	m_ClickToNoise.SetPrecision(1);
	m_AttackRate.SetPrecision(2);
	m_EnvelopDecayRate.SetPrecision(2);
}

void CDeclickDialog::DoDataExchange(CDataExchange* pDX)
{
	BaseClass::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CDeclickDialog)
	DDX_Control(pDX, IDC_EDIT_DECAY_RATE, m_EnvelopDecayRate);
	DDX_Control(pDX, IDC_EDIT_CLICK_TO_NOISE, m_ClickToNoise);
	DDX_Control(pDX, IDC_EDIT_ATTACK_RATE, m_AttackRate);

	DDX_Text(pDX, IDC_EDIT_MAX_CLICK_LENGTH, m_nMaxClickLength);
	DDV_MinMaxInt(pDX, m_nMaxClickLength, 6, 64);
	DDX_Text(pDX, IDC_EDIT_MIN_CLICK_AMPLITUDE, m_MinClickAmplitude);
	DDV_MinMaxInt(pDX, m_MinClickAmplitude, 50, 5000);
	DDX_Check(pDX, IDC_CHECK_UNDO, m_bUndo);

	//}}AFX_DATA_MAP
	m_AttackRate.ExchangeData(pDX, m_MeanPowerAttackRate,
							IDS_INPUT_NAME_ATTACK_RATE, 0, 0.001, 0.99);
	m_ClickToNoise.ExchangeData(pDX, m_dClickToNoise,
								IDS_INPUT_NAME_CLICK_TO_NOISE_RATE, 0, 1.5, 20.);
	m_EnvelopDecayRate.ExchangeData(pDX, m_MeanPowerDecayRate,
									IDS_INPUT_NAME_ENVELOP_DECAY_RATE, 0, 0.001, 0.99);

	if ( ! pDX->m_bSaveAndValidate)
	{
		SetClicksImportString();
	}
}


BEGIN_MESSAGE_MAP(CDeclickDialog, BaseClass)
	//{{AFX_MSG_MAP(CDeclickDialog)
	ON_BN_CLICKED(IDC_BUTTON_MORE_SETTINGS, OnButtonMoreSettings)

	ON_BN_CLICKED(IDC_BUTTON_SAVE, OnButtonSaveSettings)
	ON_BN_CLICKED(IDC_BUTTON_LOAD, OnButtonLoadSettings)
	ON_BN_CLICKED(IDC_BUTTON_RESET_DEFAULT, OnButtonSetDefaults)
	ON_BN_CLICKED(IDC_BUTTON_REVERT_INITIAL, OnButtonRevert)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CDeclickDialog message handlers
void CDeclickDialog::OnButtonMoreSettings()
{
	CMoreDeclickDialog dlg( *this);
	// set the data to dlg
	if (IDOK == dlg.DoModal())
	{
		SetClicksImportString();
	}
}

void CDeclickDialog::LoadValuesFromRegistry()
{
	Profile.AddBoolItem(_T("Declicker"), _T("LogClicks"), m_bLogClicks, FALSE);
	Profile.AddBoolItem(_T("Declicker"), _T("ImportClicks"), m_bImportClicks, FALSE);
	Profile.AddBoolItem(_T("Declicker"), _T("LogClicksOnly"), m_bLogClicksOnly, FALSE);
	Profile.AddBoolItem(_T("Declicker"), _T("ImportClicksOnly"), m_bDontAutodetectClicks, FALSE);

	Profile.AddItem(_T("Declicker"), _T("ClickLogFilename"), m_ClickLogFilename, _T(""));
	Profile.AddItem(_T("Declicker"), _T("ClickImportFilename"), m_ClickImportFilename, _T(""));
	Profile.AddItem(_T("Declicker"), _T("MaxClickLength"), m_nMaxClickLength, 32, 6, 64);
	Profile.AddItem(_T("Declicker"), _T("MinClickAmplitude"), m_MinClickAmplitude, 200, 50, 5000);
	Profile.AddItem(_T("Declicker"), _T("AttackRate"), m_MeanPowerAttackRate, 0.5, 0.001, 0.99);
	Profile.AddItem(_T("Declicker"), _T("DecayRate"), m_MeanPowerDecayRate, 0.01, 0.001, 0.99);
	Profile.AddItem(_T("Declicker"), _T("ClickToNoise"), m_dClickToNoise, 4., 1.5, 20);

}

void CDeclickDialog::OnOK()
{
	if (!UpdateData(TRUE))
	{
		TRACE("UpdateData failed during dialog termination.\n");
		// the UpdateData routine will set focus to correct item
		return;
	}
	Profile.FlushAll();
	EndDialog(IDOK);
}

void CDeclickDialog::GetDeclickParameters(DeclickParameters * pCr)
{
	*pCr = *this;
	pCr->m_PowerToDeriv3RatioThreshold = m_dClickToNoise * m_dClickToNoise;
	pCr->m_MinDeriv3Threshold = m_MinClickAmplitude * m_MinClickAmplitude;
}

void CDeclickDialog::OnButtonSaveSettings()
{
	if (!UpdateData(TRUE))
	{
		TRACE("UpdateData failed.\n");
		// the UpdateData routine will set focus to correct item
		return;
	}

	CString FileName;
	CString Filter;
	Filter.LoadString(IDS_FILTER_FILE_CLICK_REMOVAL);

	CString Title;
	Title.LoadString(IDS_CLICK_REMOVAL_SAVE_TITLE);

	CFileDialogWithHistory dlg(FALSE,
								_T("CrIni"), NULL,
								OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT
								| OFN_EXPLORER | OFN_NONETWORKBUTTON | OFN_PATHMUSTEXIST,
								Filter);
	dlg.m_ofn.lpstrTitle = Title;

	if (IDOK != dlg.DoModal())
	{
		return;
	}

	FileName = dlg.GetPathName();
	Profile.ExportSection(_T("Declicker"), FileName);
}

void CDeclickDialog::OnButtonLoadSettings()
{
	UpdateData(TRUE);

	CString FileName;
	CString Filter;
	Filter.LoadString(IDS_FILTER_FILE_CLICK_REMOVAL);

	CString Title;
	Title.LoadString(IDS_CLICK_REMOVAL_LOAD_TITLE);

	CFileDialogWithHistory dlg(TRUE,
								_T("CrIni"), NULL,
								OFN_HIDEREADONLY
								| OFN_EXPLORER | OFN_NONETWORKBUTTON | OFN_FILEMUSTEXIST,
								Filter);
	dlg.m_ofn.lpstrTitle = Title;

	if (IDOK != dlg.DoModal())
	{
		return;
	}
	FileName = dlg.GetPathName();
	Profile.ImportSection(_T("Declicker"), FileName);

	UpdateData(FALSE);
}

void CDeclickDialog::OnButtonSetDefaults()
{
	UpdateData(TRUE);
	Profile.ResetAllToDefault();
	UpdateData(FALSE);
}

void CDeclickDialog::OnButtonRevert()
{
	UpdateData(TRUE);
	Profile.RevertAllToInitial();
	UpdateData(FALSE);
}

void CDeclickDialog::SetClicksImportString()
{
	CString s;

	if ((m_bLogClicks
			&& ! m_ClickLogFilename.IsEmpty())

		|| (m_bImportClicks
			&& ! m_ClickImportFilename.IsEmpty()))
	{
		s.LoadString(IDS_MORE_CLICK_SETTINGS_IN_EFFECT);
	}
	SetDlgItemText(IDC_STATIC_MORE_IN_EFFECT, s);
}
/////////////////////////////////////////////////////////////////////////////
// CMoreDeclickDialog dialog


CMoreDeclickDialog::CMoreDeclickDialog(DeclickParameters & Dp, CWnd* pParent /*=NULL*/)
	: BaseClass(IDD, pParent)
	, m_Dp(Dp)
{
	//{{AFX_DATA_INIT(CMoreDeclickDialog)
	//}}AFX_DATA_INIT
}

void CMoreDeclickDialog::DoDataExchange(CDataExchange* pDX)
{
	BaseClass::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CMoreDeclickDialog)
	DDX_Control(pDX, IDC_CHECK_LOG_CLICKS, m_LogClicksCheck);
	DDX_Control(pDX, IDC_CHECK_IMPORT_CLICKS, m_ImportClicksCheck);

	DDX_Control(pDX, IDC_EDIT_CLICK_LOG_FILENAME, m_eLogFilename);
	DDX_Control(pDX, IDC_EDIT_CLICK_IMPORT_FILENAME, m_eImportFilename);

	DDX_Text(pDX, IDC_EDIT_CLICK_LOG_FILENAME, m_Dp.m_ClickLogFilename);
	DDX_Check(pDX, IDC_CHECK_LOG_CLICKS, m_Dp.m_bLogClicks);
	DDX_Check(pDX, IDC_CHECK_LOG_CLICKS_ONLY, m_Dp.m_bLogClicksOnly);
	DDX_Check(pDX, IDC_CHECK_IMPORT_CLICKS, m_Dp.m_bImportClicks);
	DDX_Text(pDX, IDC_EDIT_CLICK_IMPORT_FILENAME, m_Dp.m_ClickImportFilename);
	DDX_Check(pDX, IDC_CHECK_IMPORTED_CLICKS_ONLY, m_Dp.m_bDontAutodetectClicks);

	//}}AFX_DATA_MAP

}


BEGIN_MESSAGE_MAP(CMoreDeclickDialog, BaseClass)
	//{{AFX_MSG_MAP(CMoreDeclickDialog)
	ON_BN_CLICKED(IDC_CHECK_LOG_CLICKS, OnCheckLogClicks)
	ON_BN_CLICKED(IDC_CHECK_IMPORT_CLICKS, OnCheckImportClicks)
	ON_BN_CLICKED(IDC_CLICK_LOG_BROWSE_BUTTON, OnClickLogBrowseButton)
	ON_BN_CLICKED(IDC_CLICK_IMPORT_BROWSE_BUTTON, OnClickImportBrowseButton)

	ON_UPDATE_COMMAND_UI(IDC_EDIT_CLICK_LOG_FILENAME, OnUpdateLogClicks)
	ON_UPDATE_COMMAND_UI(IDC_CLICK_LOG_BROWSE_BUTTON, OnUpdateLogClicks)
	ON_UPDATE_COMMAND_UI(IDC_CHECK_LOG_CLICKS_ONLY, OnUpdateLogClicks)

	ON_UPDATE_COMMAND_UI(IDC_EDIT_CLICK_IMPORT_FILENAME, OnUpdateImportClicks)
	ON_UPDATE_COMMAND_UI(IDC_CLICK_IMPORT_BROWSE_BUTTON, OnUpdateImportClicks)
	ON_UPDATE_COMMAND_UI(IDC_CHECK_IMPORTED_CLICKS_ONLY, OnUpdateImportClicks)

	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CMoreDeclickDialog message handlers
void CMoreDeclickDialog::OnCheckLogClicks()
{
	NeedUpdateControls();
}

void CMoreDeclickDialog::OnCheckImportClicks()
{
	NeedUpdateControls();
}

void CMoreDeclickDialog::OnClickLogBrowseButton()
{
	CString filter(MAKEINTRESOURCE(IDS_FILE_FILTER_TXT));

	m_eLogFilename.GetWindowText(m_Dp.m_ClickLogFilename);

	CFileDialog fdlg(TRUE, _T("txt"), m_Dp.m_ClickLogFilename,
					OFN_EXPLORER
					//| OFN_FILEMUSTEXIST
					| OFN_HIDEREADONLY,
					filter);
	fdlg.m_ofn.lpstrInitialDir = _T(".");

	if (fdlg.DoModal() == IDOK)
	{
		m_Dp.m_ClickLogFilename = fdlg.GetPathName();
		m_eLogFilename.SetWindowText(m_Dp.m_ClickLogFilename);
	}

}

void CMoreDeclickDialog::OnClickImportBrowseButton()
{
	CString filter(MAKEINTRESOURCE(IDS_FILE_FILTER_TXT));

	m_eImportFilename.GetWindowText(m_Dp.m_ClickImportFilename);

	CFileDialog fdlg(TRUE, _T("txt"), m_Dp.m_ClickImportFilename,
					OFN_EXPLORER
					| OFN_FILEMUSTEXIST
					| OFN_HIDEREADONLY,
					filter);
	fdlg.m_ofn.lpstrInitialDir = _T(".");

	if (fdlg.DoModal() == IDOK)
	{
		m_Dp.m_ClickImportFilename = fdlg.GetPathName();
		m_eImportFilename.SetWindowText(m_Dp.m_ClickImportFilename);
	}
}

void CMoreDeclickDialog::OnUpdateLogClicks(CCmdUI * pCmdUI)
{
	pCmdUI->Enable(m_LogClicksCheck.GetCheck());
}

void CMoreDeclickDialog::OnUpdateImportClicks(CCmdUI * pCmdUI)
{
	pCmdUI->Enable(m_ImportClicksCheck.GetCheck());
}

/////////////////////////////////////////////////////////////////////////////
// CNoiseReductionDialog dialog


CNoiseReductionDialog::CNoiseReductionDialog(SAMPLE_INDEX begin, SAMPLE_INDEX end, SAMPLE_INDEX caret,
											CHANNEL_MASK Channels,
											CWaveFile & File,
											BOOL ChannelsLocked, BOOL UndoEnabled,
											int TimeFormat,
											CWnd* pParent /*=NULL*/)
	: BaseClass(begin, end, caret, Channels, File,
				TimeFormat, IDD, pParent)
{
	m_bUndo = UndoEnabled;
	m_bLockChannels = ChannelsLocked;
	//{{AFX_DATA_INIT(CNoiseReductionDialog)
	m_nFftOrderExp = -1;
	//}}AFX_DATA_INIT
	m_dTransientThreshold = 10.;
	m_dNoiseReduction = 10.;
	m_dNoiseCriterion = 0.25;
	m_dNoiseThresholdLow = -70.;
	m_dNoiseThresholdHigh = -70.;
	m_dLowerFrequency = 4000.;
	m_FftOrder = 256;
	m_dNoiseReductionAggressiveness = 1.;
	m_FarMaskingLevelDb = -40.;

	m_eLowerFrequency.SetPrecision(1);
	m_eNoiseThresholdHigh.SetPrecision(1);
	m_eNoiseThresholdLow.SetPrecision(1);
	m_EditAggressiveness.SetPrecision(2);
	m_eNoiseReduction.SetPrecision(2);

	LoadValuesFromRegistry();
}

void CNoiseReductionDialog::DoDataExchange(CDataExchange* pDX)
{
	BaseClass::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CNoiseReductionDialog)
	DDX_CBIndex(pDX, IDC_COMBO_FFT_ORDER, m_nFftOrderExp);
	DDX_Control(pDX, IDC_EDIT_LOWER_FREQUENCY, m_eLowerFrequency);
	DDX_Control(pDX, IDC_EDIT_NOISE_AREA_THRESHOLD_HIGH, m_eNoiseThresholdHigh);
	DDX_Control(pDX, IDC_EDIT_NOISE_AREA_THRESHOLD_LOW, m_eNoiseThresholdLow);
	DDX_Control(pDX, IDC_EDIT_AGGRESSIVNESS, m_EditAggressiveness);
	DDX_Control(pDX, IDC_EDIT_NOISE_REDUCTION, m_eNoiseReduction);
	DDX_Check(pDX, IDC_CHECK_UNDO, m_bUndo);
	//}}AFX_DATA_MAP
	m_eLowerFrequency.ExchangeData(pDX, m_dLowerFrequency,
									IDS_INPUT_NAME_FREQUENCY, IDS_HERTZ, 100., 48000.);
	m_eNoiseThresholdHigh.ExchangeData(pDX, m_dNoiseThresholdHigh,
										IDS_INPUT_NAME_NOISE_FLOOR_HIGH, IDS_DECIBEL, -120., -10.);
	m_eNoiseThresholdLow.ExchangeData(pDX, m_dNoiseThresholdLow,
									IDS_INPUT_NAME_NOISE_FLOOR_LOW, IDS_DECIBEL, -120., -10.);
	m_EditAggressiveness.ExchangeData(pDX, m_dNoiseReductionAggressiveness,
									IDS_INPUT_NAME_NOISE_SUPPRESSION_AGGR, 0, 0.1, 3.);
	m_eNoiseReduction.ExchangeData(pDX, m_dNoiseReduction,
									IDS_INPUT_NAME_NOISE_REDUCTION, IDS_DECIBEL, 0., 40.);

	if (pDX->m_bSaveAndValidate)
	{
		m_FftOrder = 256 << m_nFftOrderExp;
	}
}


BEGIN_MESSAGE_MAP(CNoiseReductionDialog, BaseClass)
	//{{AFX_MSG_MAP(CNoiseReductionDialog)
	ON_BN_CLICKED(IDC_BUTTON_MORE_SETTINGS, OnButtonMore)
	ON_BN_CLICKED(IDC_BUTTON_SET_THRESHOLD, OnButtonSetThreshold)
	ON_BN_CLICKED(IDC_BUTTON_SAVE, OnButtonSaveSettings)
	ON_BN_CLICKED(IDC_BUTTON_LOAD, OnButtonLoadSettings)
	ON_BN_CLICKED(IDC_BUTTON_RESET_DEFAULT, OnButtonSetDefaults)
	ON_BN_CLICKED(IDC_BUTTON_REVERT_INITIAL, OnButtonRevert)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CNoiseReductionDialog message handlers

/////////////////////////////////////////////////////////////////////////////
// CNoiseReductionPage message handlers
void CNoiseReductionDialog::LoadValuesFromRegistry()
{
	//m_bPhaseFilter = (FALSE != pApp->GetProfileInt(_T("NoiseReduction"), _T("PhaseFilter"), FALSE));

	Profile.AddItem(_T("NoiseReduction"), _T("FftOrder"), m_FftOrder, 2048, 256, 16384);

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

	Profile.AddItem(_T("NoiseReduction"), _T("TransientThreshold"), m_dTransientThreshold, 10., 2., 30.);
	Profile.AddItem(_T("NoiseReduction"), _T("NoiseReduction"), m_dNoiseReduction, 10., 0., 100.);
	Profile.AddItem(_T("NoiseReduction"), _T("NoiseCriterion"), m_dNoiseCriterion, 0.25, 0., 1.);
	Profile.AddItem(_T("NoiseReduction"), _T("NoiseThresholdLow"), m_dNoiseThresholdLow, -70., -120., -10.);
	Profile.AddItem(_T("NoiseReduction"), _T("NoiseThresholdHigh"), m_dNoiseThresholdHigh, -65., -120., -10.);
	Profile.AddItem(_T("NoiseReduction"), _T("LowerFrequency"), m_dLowerFrequency, 1000., 100., 48000.);
	Profile.AddItem(_T("NoiseReduction"), _T("ToneOverNoisePreference"), m_dToneOverNoisePreference, 10., 0., 20.);
	Profile.AddItem(_T("NoiseReduction"), _T("NearMaskingDecayDistanceHigh"), m_NearMaskingDecayDistanceHigh, 1000., 1., 5000.);
	Profile.AddItem(_T("NoiseReduction"), _T("NearMaskingDecayDistanceLow"), m_NearMaskingDecayDistanceLow, 100., 1., 1000.);
	Profile.AddItem(_T("NoiseReduction"), _T("NearMaskingDecayTimeHigh"), m_NearMaskingDecayTimeHigh, 40., 1., 1000.);
	Profile.AddItem(_T("NoiseReduction"), _T("NearMaskingDecayTimeLow"), m_NearMaskingDecayTimeLow, 100., 1., 1000.);
	Profile.AddItem(_T("NoiseReduction"), _T("FarMaskingLevel"), m_FarMaskingLevelDb, -40., -100., -10.);
	Profile.AddItem(_T("NoiseReduction"), _T("Aggressiveness"), m_dNoiseReductionAggressiveness, 1., 0.1, 3.);
}


void CNoiseReductionDialog::GetNoiseReductionData(NoiseReductionParameters * pNr)
{
	//pNr->m_bApplyPhaseFilter = m_bPhaseFilter;
	pNr->m_MinFrequencyToProcess = float(m_dLowerFrequency);
	pNr->m_ThresholdOfTransientAttack = float(pow(10., m_dTransientThreshold / 10.));
	pNr->m_ThresholdOfTransientDecay = float(pow(0.1, m_dTransientThreshold / 10.));
	pNr->m_FreqThresholdOfNoiselike = float(M_PI_2 * M_PI_2 * m_dNoiseCriterion * m_dNoiseCriterion);
	pNr->m_MaxNoiseSuppression = float(DB_TO_NEPER * m_dNoiseReduction);
	pNr->m_LevelThresholdForNoiseLow = float(DB_TO_NEPER * (m_dNoiseThresholdLow));
	pNr->m_LevelThresholdForNoiseHigh = float(DB_TO_NEPER * (m_dNoiseThresholdHigh));
	pNr->m_ToneOverNoisePreference = float(DB_TO_NEPER * m_dToneOverNoisePreference);
	pNr->m_NoiseReductionRatio = 0.5 * m_dNoiseReductionAggressiveness;

	pNr->m_NearMaskingDecayDistanceLow = float(m_NearMaskingDecayDistanceLow);
	pNr->m_NearMaskingDecayDistanceHigh = float(m_NearMaskingDecayDistanceHigh);

	pNr->m_NearMaskingDecayTimeLow = float(m_NearMaskingDecayTimeLow);
	pNr->m_NearMaskingDecayTimeHigh = float(m_NearMaskingDecayTimeHigh);
	pNr->m_FarMaskingLevelDb = float(m_FarMaskingLevelDb);
}

void CNoiseReductionDialog::OnOK()
{
	if (!UpdateData(TRUE))
	{
		TRACE(L"UpdateData failed during dialog termination.\n");
		// the UpdateData routine will set focus to correct item
		return;
	}
	Profile.FlushAll();
	EndDialog(IDOK);
}
/////////////////////////////////////////////////////////////////////////////
// CMoreNoiseDialog dialog


CMoreNoiseDialog::CMoreNoiseDialog(CNoiseReductionDialog * pParentDlg, CWnd* pParent /*=NULL*/)
	: BaseClass(CMoreNoiseDialog::IDD, pParent)
	, m_pParentDlg(pParentDlg)
{
	//{{AFX_DATA_INIT(CMoreNoiseDialog)
	// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
	m_eFarMaskingLevel.SetPrecision(2);

	m_eNearMaskingTimeLow.SetPrecision(1);
	m_eNearMaskingTimeHigh.SetPrecision(1);

	m_eNearMaskingDistanceLow.SetPrecision(1);
	m_eNearMaskingDistanceHigh.SetPrecision(1);

	m_eToneOverNoisePreference.SetPrecision(1);
	m_eNoiseCriterion.SetPrecision(4);
	m_eTransientThreshold.SetPrecision(1);
}


void CMoreNoiseDialog::DoDataExchange(CDataExchange* pDX)
{
	BaseClass::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CMoreNoiseDialog)
	DDX_Control(pDX, IDC_EDIT_FAR_MASKING_LEVEL, m_eFarMaskingLevel);
	DDX_Control(pDX, IDC_EDIT_MASKING_TIME_LOW, m_eNearMaskingTimeLow);
	DDX_Control(pDX, IDC_EDIT_MASKING_TIME_HIGH, m_eNearMaskingTimeHigh);
	DDX_Control(pDX, IDC_EDIT_NEAR_MASKING_DISTANCE_LOW, m_eNearMaskingDistanceLow);
	DDX_Control(pDX, IDC_EDIT_NEAR_MASKING_DISTANCE_HIGH, m_eNearMaskingDistanceHigh);
	DDX_Control(pDX, IDC_EDIT_TONE_PREFERENCE, m_eToneOverNoisePreference);
	DDX_Control(pDX, IDC_EDIT_NOISE_CRITERION, m_eNoiseCriterion);
	DDX_Control(pDX, IDC_EDIT_TRANSIENT_THRESHOLD, m_eTransientThreshold);
	//}}AFX_DATA_MAP
	m_eNearMaskingDistanceHigh.ExchangeData(pDX, m_pParentDlg->m_NearMaskingDecayDistanceHigh,
											IDS_INPUT_NAME_NEAR_MASK_DISTANCE_HIGH, IDS_HERTZ, 1., 5000.);
	m_eNearMaskingDistanceLow.ExchangeData(pDX, m_pParentDlg->m_NearMaskingDecayDistanceLow,
											IDS_INPUT_NAME_NEAR_MASK_DISTANCE_LOW, IDS_HERTZ, 1., 1000.);

	m_eNearMaskingTimeHigh.ExchangeData(pDX, m_pParentDlg->m_NearMaskingDecayTimeHigh,
										IDS_INPUT_NAME_NEAR_MASK_TIME_HIGH, IDS_MILISECOND, 1., 1000.);
	m_eNearMaskingTimeLow.ExchangeData(pDX, m_pParentDlg->m_NearMaskingDecayTimeLow,
										IDS_INPUT_NAME_NEAR_MASK_TIME_LOW, IDS_MILISECOND, 1., 1000.);

	m_eFarMaskingLevel.ExchangeData(pDX, m_pParentDlg->m_FarMaskingLevelDb,
									IDS_INPUT_NAME_FAR_MASK_LEVEL, 0, -100., -10.);

	m_eTransientThreshold.ExchangeData(pDX, m_pParentDlg->m_dTransientThreshold,
										NULL, IDS_DECIBEL, 2., 30.);

	m_eNoiseCriterion.ExchangeData(pDX, m_pParentDlg->m_dNoiseCriterion,
									IDS_INPUT_NAME_NOISE_VS_CONTINUOUS, 0, 0.0, 1.);

	m_eToneOverNoisePreference.ExchangeData(pDX, m_pParentDlg->m_dToneOverNoisePreference,
											IDS_INPUT_NAME_TONE_OVER_NOISE, IDS_DECIBEL, 0., 20.);
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
	CMoreNoiseDialog dlg(this);
	// set the data to dlg
	if (IDOK == dlg.DoModal())
	{
		// the data from dlg is returned through the parent dialog pointer
	}
}

void CNoiseReductionDialog::OnButtonSetThreshold()
{
	if (!UpdateData(TRUE))
	{
		TRACE("UpdateData failed during dialog termination.\n");
		// the UpdateData routine will set focus to correct item
		return;
	}

	Profile.FlushAll();
	EndDialog(IDC_BUTTON_SET_THRESHOLD);
}

void CNoiseReductionDialog::OnButtonSaveSettings()
{
	if (!UpdateData(TRUE))
	{
		TRACE("UpdateData failed.\n");
		// the UpdateData routine will set focus to correct item
		return;
	}

	CString FileName;
	CString Filter;
	Filter.LoadString(IDS_FILTER_FILE_NOISE_REDUCTION);

	CString Title;
	Title.LoadString(IDS_NOISE_REDUCTION_SAVE_TITLE);

	CFileDialogWithHistory dlg(FALSE,
								_T("NrIni"), NULL,
								OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT
								| OFN_EXPLORER | OFN_NONETWORKBUTTON | OFN_PATHMUSTEXIST,
								Filter);
	dlg.m_ofn.lpstrTitle = Title;

	if (IDOK != dlg.DoModal())
	{
		return;
	}

	FileName = dlg.GetPathName();
	Profile.ExportSection(_T("NoiseReduction"), FileName);
}

void CNoiseReductionDialog::OnButtonLoadSettings()
{
	UpdateData(TRUE);

	CString FileName;
	CString Filter;
	Filter.LoadString(IDS_FILTER_FILE_NOISE_REDUCTION);

	CString Title;
	Title.LoadString(IDS_NOISE_REDUCTION_LOAD_TITLE);

	CFileDialogWithHistory dlg(TRUE,
								_T("NrIni"), NULL,
								OFN_HIDEREADONLY
								| OFN_EXPLORER | OFN_NONETWORKBUTTON | OFN_FILEMUSTEXIST,
								Filter);
	dlg.m_ofn.lpstrTitle = Title;

	if (IDOK != dlg.DoModal())
	{
		return;
	}
	FileName = dlg.GetPathName();
	Profile.ImportSection(_T("NoiseReduction"), FileName);

	UpdateData(FALSE);
}

void CNoiseReductionDialog::OnButtonSetDefaults()
{
	UpdateData(TRUE);
	Profile.ResetAllToDefault();
	UpdateData(FALSE);
}

void CNoiseReductionDialog::OnButtonRevert()
{
	UpdateData(TRUE);
	Profile.RevertAllToInitial();
	UpdateData(FALSE);
}

///////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
// CExpressionEvaluationDialog dialog


CExpressionEvaluationDialog::CExpressionEvaluationDialog(SAMPLE_INDEX begin,
														SAMPLE_INDEX end, SAMPLE_INDEX caret,
														CHANNEL_MASK Channels,
														CWaveFile & File,
														BOOL ChannelsLocked, BOOL UndoEnabled,
														int TimeFormat,
														CExpressionEvaluationProc * pProc,
														CWnd* pParent /*=NULL*/)
	: BaseClass(begin, end, caret, Channels, File,
				TimeFormat,
				IDD, pParent, TRUE),
	m_ExpressionGroupSelected(0),
	m_ExpressionSelected(0),
	m_ExpressionTabSelected(0),
	m_pProc(pProc)
{
	m_bUndo = UndoEnabled;
	m_bLockChannels = ChannelsLocked;
	//{{AFX_DATA_INIT(CExpressionEvaluationDialog)
	//}}AFX_DATA_INIT
}

CExpressionEvaluationDialog::~CExpressionEvaluationDialog()
{
}

void CExpressionEvaluationDialog::DoDataExchange(CDataExchange* pDX)
{
	BaseClass::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CExpressionEvaluationDialog)
	DDX_Control(pDX, IDC_TAB_TOKENS, m_TabTokens);
	DDX_Control(pDX, IDC_EDIT_EXPRESSION, m_eExpression);
	DDX_Check(pDX, IDC_CHECK_UNDO, m_bUndo);
	DDX_Text(pDX, IDC_EDIT_EXPRESSION, m_sExpression);
	//}}AFX_DATA_MAP
	if (pDX->m_bSaveAndValidate)
	{
		m_Profile.UnloadAll();
	}
}


BEGIN_MESSAGE_MAP(CExpressionEvaluationDialog, BaseClass)
	//{{AFX_MSG_MAP(CExpressionEvaluationDialog)
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

		m_eExpression.ReplaceSel(Substr, TRUE);
		m_eExpression.SetFocus();
		return TRUE;
	}
	return FALSE;
}

void CExpressionEvaluationDialog::OnOK()
{
	if (! m_OperandsTabDlg.UpdateData(TRUE)
		|| ! m_SavedExprTabDlg.UpdateData(TRUE)
		|| !UpdateData(TRUE))
	{
		TRACE("UpdateData failed during dialog termination.\n");
		// the UpdateData routine will set focus to correct item
		return;
	}

	LPCTSTR str = m_sExpression;
	LPCTSTR str1 = str;
	if ( ! m_pProc->SetExpression( & str))
	{
		AfxMessageBox(m_pProc->m_ErrorString);
		int pos = int(str - str1);
		m_eExpression.SetFocus();
		m_eExpression.SetSel(pos, pos, FALSE);
		return;
	}

	EndDialog(IDOK);
}

void CExpressionEvaluationDialog::OnSelchangeTabTokens(NMHDR* /*pNMHDR*/, LRESULT* pResult)
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

	m_TabTokens.InsertItem(0, LoadCString(IDS_TAB_TITLE_FUNCTIONS_AND_OPS));
	m_TabTokens.InsertItem(1, LoadCString(IDS_TAB_TITLE_OPERANDS));
	m_TabTokens.InsertItem(2, LoadCString(IDS_TAB_TITLE_SAVED_EXPRESSIONS));

	m_TabTokens.SetCurSel(m_ExpressionTabSelected);
	ShowHideTabDialogs();

	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}


void CExpressionEvaluationDialog::OnButtonSaveExpressionAs()
{
	CString s;
	m_eExpression.GetWindowText(s);
	m_SavedExprTabDlg.SaveExpressionAs(s);
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
	NeedUpdateControls();
}

LRESULT CExpressionEvaluationDialog::OnKickIdle(WPARAM w, LPARAM l)
{
	m_SavedExprTabDlg.UpdateDialogControls( & m_SavedExprTabDlg, FALSE);
	return BaseClass::OnKickIdle(w, l);
}

CExpressionEvaluationProc * CExpressionEvaluationDialog::GetExpression()
{
	m_pProc->m_dFrequencyArgument = m_OperandsTabDlg.m_dFrequency;
	m_pProc->m_dFrequencyArgument1 = m_OperandsTabDlg.m_dFrequency1;
	m_pProc->m_dFrequencyArgument2 = m_OperandsTabDlg.m_dFrequency2;
	m_pProc->m_dFrequencyArgument3 = m_OperandsTabDlg.m_dFrequency3;

	return m_pProc.release();
}

