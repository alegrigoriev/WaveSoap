// Copyright Alexander Grigoriev, 1997-2002, All Rights Reserved
// DialogWithSelection.inl: function definition for CDialogWithSelectionT
//
//////////////////////////////////////////////////////////////////////
template<typename B>
void CDialogWithSelectionT<B>::OnButtonSelection()
{
	CSelectionDialog dlg(m_Start, m_End, m_CaretPosition, m_Chan,
						m_WaveFile, m_TimeFormat);

	if (IDOK != dlg.DoModal())
	{
		return;
	}

	m_Start = dlg.GetStart();
	m_End = dlg.GetEnd();
	m_Chan = dlg.GetChannel();

	NeedUpdateControls();
}

template<typename B>
CDialogWithSelectionT<B>::CDialogWithSelectionT(SAMPLE_INDEX Start,
												SAMPLE_INDEX End, SAMPLE_INDEX CaretPos,
												CHANNEL_MASK Channel,
												CWaveFile & File, int TimeFormat,
												UINT TemplateID,
												CWnd* pParent)   // standard constructor
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
