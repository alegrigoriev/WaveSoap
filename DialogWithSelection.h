#pragma once

#include "UiUpdatedDlg.h"

/////////////////////////////////////////////////////////////////////////////
// CDialogWithSelection dialog

class CDialogWithSelection : public CUiUpdatedDlg
{
	typedef CUiUpdatedDlg BaseClass;
// Construction
public:
	CDialogWithSelection(SAMPLE_INDEX Start,
						SAMPLE_INDEX End, SAMPLE_INDEX CaretPos,
						CHANNEL_MASK Channel,
						CWaveFile & File, int TimeFormat,
						UINT TemplateID,
						CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CDialogWithSelection)
	//}}AFX_DATA

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
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CDialogWithSelection)
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CDialogWithSelection)
	afx_msg void OnButtonSelection();
	afx_msg void OnChecklockChannels();
	afx_msg void OnUpdateSelectionStatic(CCmdUI * pCmdUI);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

