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

void PersistentUndoRedo::LoadData(class CApplicationProfile & Profile)
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

