#include "StdAfx.h"
#include "ApplicationParameters.h"
#include "ApplicationProfile.h"

PersistentUndoRedo::PersistentUndoRedo()
{
	m_UndoEnabled = TRUE;
	m_RedoEnabled = TRUE;
	m_LimitUndoSize = FALSE;
	m_LimitRedoSize = FALSE;
	m_LimitUndoDepth = FALSE;
	m_LimitRedoDepth = FALSE;
	m_UndoSizeLimit = 1000;
	m_UndoDepthLimit = 100;
	m_RedoSizeLimit = 1000;
	m_RedoDepthLimit = 100;
	m_RememberSelectionInUndo = TRUE;
}

UndoRedoParameters * PersistentUndoRedo::GetData()
{
	static PersistentUndoRedo Params;
	return & Params;
}

void PersistentUndoRedo::LoadData(CApplicationProfile & Profile)
{
	UndoRedoParameters * pParams = GetData();

	Profile.AddBoolItem(_T("Settings"), _T("UndoEnabled"), pParams->m_UndoEnabled, TRUE);
	Profile.AddBoolItem(_T("Settings"), _T("RedoEnabled"), pParams->m_RedoEnabled, TRUE);
	Profile.AddBoolItem(_T("Settings"), _T("EnableUndoLimit"), pParams->m_LimitUndoSize, FALSE);
	Profile.AddBoolItem(_T("Settings"), _T("EnableRedoLimit"), pParams->m_LimitRedoSize, FALSE);
	Profile.AddBoolItem(_T("Settings"), _T("EnableUndoDepthLimit"), pParams->m_LimitUndoDepth, FALSE);
	Profile.AddBoolItem(_T("Settings"), _T("EnableRedoDepthLimit"), pParams->m_LimitRedoDepth, FALSE);
	Profile.AddBoolItem(_T("Settings"), _T("RememberSelectionInUndo"), pParams->m_RememberSelectionInUndo, TRUE);
	Profile.AddItem(_T("Settings"), _T("MaxUndoDepth"), pParams->m_UndoDepthLimit, 100, 1, 1000);
	Profile.AddItem(_T("Settings"), _T("MaxRedoDepth"), pParams->m_RedoDepthLimit, 100, 1, 1000);
	Profile.AddItem(_T("Settings"), _T("UndoSizeLimit"), pParams->m_UndoSizeLimit, 2048u,
					1u, 4095u);
	Profile.AddItem(_T("Settings"), _T("RedoSizeLimit"), pParams->m_RedoSizeLimit, 2048u,
					1u, 4095u);

}

void PersistentUndoRedo::SaveData(const UndoRedoParameters * pParams)
{
	*GetData() = *pParams;
}

PersistentFileParameters::PersistentFileParameters()
{
	RawFileBigEnded = FALSE;
	RawFileFormat.cbSize = 0;
	RawFileFormat.nChannels = 1;
	RawFileFormat.wFormatTag = WAVE_FORMAT_PCM;
	RawFileFormat.nSamplesPerSec = 44100;
	RawFileFormat.wBitsPerSample = 16;

	RawFileFormat.nBlockAlign = RawFileFormat.wBitsPerSample * RawFileFormat.nChannels / 8;
	RawFileFormat.nAvgBytesPerSec = RawFileFormat.nBlockAlign * RawFileFormat.nSamplesPerSec;
}

FileParameters * PersistentFileParameters::GetData()
{
	static PersistentFileParameters Params;
	return & Params;
}

void PersistentFileParameters::SaveData(const FileParameters * pParams)
{
	*GetData() = *pParams;
}

void PersistentFileParameters::LoadData(CApplicationProfile & Profile)
{
	FileParameters * pParams = GetData();

	Profile.AddBoolItem(_T("Settings"), _T("RawFileBigEnded"), pParams->RawFileBigEnded, FALSE);
	Profile.AddItem(_T("Settings"), _T("RawFileSampleRate"),
					pParams->RawFileFormat.nSamplesPerSec, 44100, 1, 1000000);
	Profile.AddItem(_T("Settings"), _T("RawFileChannels"),
					pParams->RawFileFormat.nChannels, 1, 1, 2);

	Profile.AddItem(_T("Settings"), _T("RawFileFormatTag"),
					pParams->RawFileFormat.wFormatTag, WAVE_FORMAT_PCM, 0, 0xFFFF);

	Profile.AddItem(_T("Settings"), _T("RawFileBitsPerSample"),
					pParams->RawFileFormat.wBitsPerSample, 16, 8, 16);

	Profile.AddItem(_T("Settings"), _T("RawFileHeaderLength"),
					pParams->RawFileHeaderLength, 0, 0, 0xFFFFFFFF);

	Profile.AddItem(_T("Settings"), _T("RawFileTrailerLength"),
					pParams->RawFileTrailerLength, 0, 0, 0xFFFFFFFF);

	if (8 != pParams->RawFileFormat.wBitsPerSample
		&& 16 != pParams->RawFileFormat.wBitsPerSample)
	{
		pParams->RawFileFormat.wBitsPerSample = 16;
	}

	if (WAVE_FORMAT_PCM != pParams->RawFileFormat.wFormatTag
		&& WAVE_FORMAT_ALAW != pParams->RawFileFormat.wFormatTag
		&& WAVE_FORMAT_MULAW != pParams->RawFileFormat.wFormatTag)
	{
		pParams->RawFileFormat.wFormatTag = WAVE_FORMAT_PCM;
	}
	else if (WAVE_FORMAT_PCM != pParams->RawFileFormat.wFormatTag)
	{
		pParams->RawFileFormat.wBitsPerSample = 8;
	}
	pParams->RawFileFormat.nBlockAlign = pParams->RawFileFormat.wBitsPerSample * pParams->RawFileFormat.nChannels / 8;
	pParams->RawFileFormat.nAvgBytesPerSec = pParams->RawFileFormat.nBlockAlign * pParams->RawFileFormat.nSamplesPerSec;
}

