// Copyright Alexander Grigoriev, 1997-2002, All Rights Reserved
// DialogWithSelection.inl: class declaration for CDialogWithSelectionT
//
//////////////////////////////////////////////////////////////////////
#pragma once
#include "MessageMapT.h"
/////////////////////////////////////////////////////////////////////////////
// CDialogWithSelection dialog

template<typename B = CUiUpdatedDlg>
class CDialogWithSelectionT : public B
{
	// B should be derived from CUiUpdatedDlgT<T>
	typedef B BaseClass;
// Construction
public:
	CDialogWithSelectionT(SAMPLE_INDEX Start,
						SAMPLE_INDEX End, SAMPLE_INDEX CaretPos,
						CHANNEL_MASK Channel,
						CWaveFile & File, int TimeFormat,
						UINT TemplateID,
						CWnd* pParent = NULL);   // standard constructor

// Dialog Data

	SAMPLE_INDEX GetStart() const
	{
		return m_Start;
	}
	SAMPLE_INDEX GetEnd() const
	{
		return m_End;
	}
	BOOL UndoEnabled() const
	{
		return m_bUndo;
	}

	BOOL ChannelsLocked() const
	{
		return m_bLockChannels;
	}

	CHANNEL_MASK GetChannel() const
	{
		if (m_bLockChannels)
		{
			return ALL_CHANNELS;
		}
		return m_Chan;
	}

protected:
	CHANNEL_MASK m_Chan;
	int		m_TimeFormat;
	SAMPLE_INDEX m_Start;
	SAMPLE_INDEX m_End;
	SAMPLE_INDEX m_CaretPosition;
	CWaveFile & m_WaveFile;

	BOOL	m_bUndo;
	BOOL	m_bLockChannels;

	struct Selection
	{
		SAMPLE_INDEX begin;
		SAMPLE_INDEX end;
	};

	std::vector<Selection> m_Selections;

	void AddSelection(LPCTSTR Name, SAMPLE_INDEX begin, SAMPLE_INDEX end);
	void AddSelection(UINT id, SAMPLE_INDEX begin, SAMPLE_INDEX end);
	int FindSelection(SAMPLE_INDEX begin, SAMPLE_INDEX end);
// Overrides

// Implementation
protected:

	// Generated message map functions
	afx_msg void OnButtonSelection();

	afx_msg void OnChecklockChannels()
	{
		m_bLockChannels = IsDlgButtonChecked(IDC_CHECKLOCK_CHANNELS);
		NeedUpdateControls();
	}

	afx_msg void OnUpdateSelectionStatic(CCmdUI * pCmdUI)
	{
		pCmdUI->SetText(GetSelectionText(m_Start, m_End, m_Chan,
										m_WaveFile.Channels(), m_bLockChannels,
										m_WaveFile.SampleRate(), m_TimeFormat));
	}

	DECLARE_MESSAGE_MAP()
};

BEGIN_MESSAGE_MAP_T(CDialogWithSelectionT, BaseClass)
	//{{AFX_MSG_MAP(CDialogWithSelectionT)
	ON_BN_CLICKED(IDC_CHECKLOCK_CHANNELS, OnChecklockChannels)
	ON_BN_CLICKED(IDC_BUTTON_SELECTION, OnButtonSelection)
	ON_UPDATE_COMMAND_UI(IDC_STATIC_SELECTION, OnUpdateSelectionStatic)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

typedef CDialogWithSelectionT<> CDialogWithSelection;
