#pragma once

struct UndoRedoParameters
{
	BOOL m_UndoEnabled;
	BOOL m_RedoEnabled;
	BOOL m_LimitUndoSize;
	BOOL m_LimitRedoSize;
	BOOL m_LimitUndoDepth;
	BOOL m_LimitRedoDepth;
	UINT m_UndoSizeLimit; // in megabytes
	UINT m_UndoDepthLimit;
	UINT m_RedoSizeLimit; // in megabytes
	UINT m_RedoDepthLimit;
	BOOL m_RememberSelectionInUndo;
};

struct FileParameters
{
};

struct SoundDeviceParameters
{
};

// singleton
class PersistentUndoRedo : protected UndoRedoParameters
{
	PersistentUndoRedo();  // private
public:
	static UndoRedoParameters * GetData();
	static void SaveData(const UndoRedoParameters * pParams);
	static void LoadData(class CApplicationProfile & Profile);
};
