// Copyright Alexander Grigoriev, 1997-2002, All Rights Reserved
// WaveSoapFrontDoc.h : interface of the CWaveSoapFrontDoc class
//
/////////////////////////////////////////////////////////////////////////////

#if !defined(AFX_WAVESOAPFRONTDOC_H__FFA16C4C_2FA7_11D4_9ADD_00C0F0583C4B__INCLUDED_)
#define AFX_WAVESOAPFRONTDOC_H__FFA16C4C_2FA7_11D4_9ADD_00C0F0583C4B__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
#include "WmaFile.h"
#include "KListEntry.h"
#include "KInterlocked.h"
typedef int PASTE_MODE;
typedef int PASTE_RESAMPLE_MODE;

class CSelectionUpdateInfo : public CObject
{
public:
	CSelectionUpdateInfo()
		: CaretPos(0)
		, SelBegin(0)
		, SelEnd(0)
		, SelChannel(0)
		, OldCaretPos(0)
		, OldSelBegin(0)
		, OldSelEnd(0)
		, OldSelChannel(0)
		, Flags(0)
	{}
	~CSelectionUpdateInfo() {}
	SAMPLE_INDEX CaretPos;
	SAMPLE_INDEX SelBegin;
	SAMPLE_INDEX SelEnd;
	CHANNEL_MASK SelChannel;
	SAMPLE_INDEX OldCaretPos;
	SAMPLE_INDEX OldSelBegin;
	SAMPLE_INDEX OldSelEnd;
	CHANNEL_MASK OldSelChannel;
	int Flags;
};

class CSoundUpdateInfo : public ListItem<CSoundUpdateInfo>, public CObject
{
public:
	CSoundUpdateInfo(int UpdateCode, ULONG_PTR FileID,
					SAMPLE_INDEX Begin, SAMPLE_INDEX End, long NewLength)
		: m_UpdateCode(UpdateCode)
		, m_Begin(Begin)
		, m_End(End)
		, m_NewLength(NewLength)
		, m_FileID(FileID)
	{
	}
	ULONG_PTR m_FileID;
	int m_UpdateCode;
	SAMPLE_INDEX m_Begin;
	SAMPLE_INDEX m_End;
	NUMBER_OF_SAMPLES m_NewLength;
};

// Active document have highest priority for disk-intensive operations.
// while it is executing a disk-intensive command,
// such operations with non-active documents are suspended.
// To implement it, App object starts a thread for such operations.
//
enum {
	UpdateSoundDontRescanPeaks = 1,
	UpdateSoundSamplingRateChanged = 2,
	SetSelection_MakeCaretVisible = 1,
	SetSelection_MoveCaretToCenter = 2,
	SetSelection_SnapToMaximum = 4,
	SetSelection_MakeFileVisible = 8,   // make sure the file is in the
	SaveFile_SameName = 4,
	SaveFile_CloseAfterSave = 8,
	SaveFile_SaveCopy = 0x10,
	SaveFile_Mp3File = OpenDocumentMp3File,
	SaveFile_WmaFile = OpenDocumentWmaFile,
	SaveFile_RawFile = OpenDocumentRawFile,
	SaveFile_AviFile = OpenDocumentAviFile,
	SaveRawFileMsbFirst = OpenRawFileMsbFirst,
	SaveFile_NonWavFile = OpenDocumentNonWavFile,
};

class CWaveSoapFrontDoc : public CDocument
{
	typedef CDocument BaseClass;
protected: // create from serialization only
	CWaveSoapFrontDoc();
	DECLARE_DYNCREATE(CWaveSoapFrontDoc)

// Attributes
public:
	BOOL UndoEnabled() const
	{
		return m_bUndoEnabled;
	}
	BOOL RedoEnabled() const
	{
		return m_bRedoEnabled;
	}
	BOOL ChannelsLocked() const
	{
		return m_bChannelsLocked;
	}

	NUMBER_OF_SAMPLES WaveFileSamples() const;
	LPMMCKINFO WaveDataChunk() const;
	LPWAVEFORMATEX WaveFormat() const;
	NUMBER_OF_CHANNELS WaveChannels() const;

	unsigned int WaveSampleRate() const;
	int WaveSampleSize() const;
	ULONG_PTR WaveFileID() const;

	virtual BOOL IsModified();
	bool IsReadOnly() const
	{
		return m_bReadOnly;
	}
	bool IsBusy() const
	{
		return m_OperationInProgress != 0;
	}

	bool CanReadSelection() const
	{
		return CanStartOperation(1, true, false);
	}
	bool CanModifySelection() const
	{
		return CanStartOperation(1, true, true);
	}

	bool CanReadFile() const
	{
		return CanStartOperation(1, false, false);
	}

	bool CanPlayFile() const;

	bool CanStartOperation(NUMBER_OF_SAMPLES SamplesNecessary, bool SelectionNecessary, bool Modify) const;
	bool CanModifyFile() const
	{
		return CanStartOperation(1, false, true);
	}

	bool CanWriteFile() const
	{
		return CanStartOperation(0, false, true);
	}
// Operations
public:
	void IncrementModified(BOOL DeleteRedo = TRUE, int KeepPreviousUndo = -1);
	void DecrementModified();   // called at UNDO
	virtual void SetModifiedFlag(BOOL bModified = TRUE, int KeepPreviousUndo = -1);
	void AddUndoRedo(class CUndoRedoContext * pUndo);
	void EnableUndo(BOOL bEnable = TRUE);
	void EnableRedo(BOOL bEnable = TRUE);
	void DeletePermanentUndoRedo();
	//BOOL InitUndoRedo(class CUndoRedoContext * pContext, BOOL IsRedo);

	BOOL OnSaveDocument(LPCTSTR lpszPathName, DWORD flags, WAVEFORMATEX * pWf);
	BOOL OnSaveDirectFile();
	BOOL OnSaveBufferedPcmFile(int flags, LPCTSTR FullTargetName);
	BOOL OnSaveBufferedPcmFileCopy(int flags, LPCTSTR FullTargetName);
	BOOL OnSaveConvertedFile(int flags, LPCTSTR FullTargetName, WAVEFORMATEX * pWf);
	BOOL OnSaveMp3File(int flags, LPCTSTR FullTargetName, WAVEFORMATEX * pWf);
	BOOL OnSaveWmaFile(int flags, LPCTSTR FullTargetName, WAVEFORMATEX * pWf);
	BOOL OnSaveRawFile(int flags, LPCTSTR FullTargetName, WAVEFORMATEX * pWf);

	// flags OpenDocumentReadOnly - ReadOnly, 2 - DirectMode
	virtual BOOL OnOpenDocument(LPCTSTR lpszPathName, int flags);
// Overrides
	virtual BOOL OnNewDocument(NewFileParameters * pParams);
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CWaveSoapFrontDoc)
public:
	virtual void Serialize(CArchive& ar);
	virtual BOOL OnCmdMsg(UINT nID, int nCode, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo);
	virtual void SetPathName(LPCTSTR lpszPathName, BOOL bAddToMRU = TRUE);
protected:
	virtual BOOL SaveModified();
	//}}AFX_VIRTUAL
	virtual void OnIdle();
	virtual BOOL DoSave(LPCTSTR lpszPathName, BOOL bReplace = TRUE);
	BOOL OpenWmaFileDocument(LPCTSTR lpszPathName);
	BOOL OpenRawFileDocument(LPCTSTR lpszPathName);
	BOOL OpenMp3FileDocument(LPCTSTR lpszPathName)
	{
		return OpenWmaFileDocument(lpszPathName);
	}
	BOOL OpenAviFileDocument(LPCTSTR lpszPathName);
	BOOL OpenNonWavFileDocument(LPCTSTR lpszPathName, int flags);
public:
	CString m_szWaveFilename;
	CString szWaveTitle;
	int m_FileTypeFlags; // OpenFileWav, ...
	// wave file currently containing the data (it could be the temp file)
	CWaveFile m_WavFile;
	// original file with the name which was open
	// for new file, it isn't open
	CWaveFile m_OriginalWavFile;
	CWaveFormat m_OriginalWaveFormat;

	int m_ModificationSequenceNumber;

	//WAVEFORMATEX WavFileFormat;

	SAMPLE_INDEX m_CaretPosition;
	SAMPLE_INDEX m_SelectionStart;
	SAMPLE_INDEX m_SelectionEnd;

	CHANNEL_MASK m_SelectedChannel;
	bool m_TimeSelectionMode;

	CHANNEL_MASK GetSelectedChannel() const;

	void SetSelection(SAMPLE_INDEX begin, SAMPLE_INDEX end, CHANNEL_MASK channel,
					SAMPLE_INDEX caret, int flags = 0);

	void SoundChanged(ULONG_PTR FileID, SAMPLE_INDEX begin, SAMPLE_INDEX end,
					NUMBER_OF_SAMPLES length = -1, DWORD flags = 0);
	void FileChanged(CWaveFile & File, SAMPLE_POSITION begin,
					SAMPLE_POSITION end, NUMBER_OF_SAMPLES length = -1, DWORD flags = 0);

	void PlaybackPositionNotify(SAMPLE_INDEX position, CHANNEL_MASK channel);
	BOOL ProcessUpdatePlaybackPosition();

	enum {UpdateSelectionChanged = 1,
		UpdateSelectionModeChanged,
		UpdateSoundChanged,
		UpdateSampleRateChanged,
		UpdateWholeFileChanged,
		UpdatePlaybackPositionChanged,
	};

// Implementation
public:
	virtual ~CWaveSoapFrontDoc();
	void UpdateDocumentTitle();

	void BuildPeakInfo(BOOL bSavePeakFile);
	void GetSoundMinMax(WavePeak & Left, WavePeak & Right,
						SAMPLE_INDEX begin, SAMPLE_INDEX end);
	BOOL SetThreadPriority(int priority)
	{
		return m_Thread.SetThreadPriority(priority);
	}

	BOOL OpenWaveFile(CWaveFile & WaveFile, LPCTSTR szName, DWORD flags);
	void QueueSoundUpdate(int UpdateCode, ULONG_PTR FileID,
						SAMPLE_INDEX Begin, SAMPLE_INDEX End, NUMBER_OF_SAMPLES NewLength, int flags = 0);
	enum {
		QueueSoundUpdateMerge = 1,  // merge update range
		QueueSoundUpdateReplace = 2, // replace existing item
	};
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

	LONG_volatile m_OperationInProgress;
	bool volatile m_StopOperation;
	bool m_OperationNonCritical;
	bool m_PlayingSound;
	bool m_bInOnIdle;
	bool m_bUndoEnabled;
	bool m_bRedoEnabled;
	bool m_bChannelsLocked;
	CHANNEL_MASK m_PrevChannelToCopy;
	PASTE_MODE m_DefaultPasteMode;
	PASTE_RESAMPLE_MODE m_PasteResampleMode;

	CString m_CurrentStatusString;
	CSimpleCriticalSection m_StatusStringLock;
	void GetCurrentStatusString(CString & str);
	void SetCurrentStatusString(const CString & str);

	bool m_bReadOnly;
	bool m_bDirectMode;
	bool m_bClosing;    // tells OnSaveDocument that it is called in close context
	bool m_bClosePending;   // Save is in progress, request close afterwards
	bool m_bCloseThisDocumentNow;   // CDocTemplate should close it in OnIdle

	LockedListHead<COperationContext> m_OpList;
	ListHead<COperationContext> m_UndoList;
	ListHead<COperationContext> m_RedoList;
	LockedListHead<COperationContext> m_RetiredList;
	LockedListHead<CSoundUpdateInfo> m_UpdateList;

protected:
	HANDLE m_hThreadEvent;
	bool volatile m_bRunThread;
	CWinThread m_Thread;
	UINT _ThreadProc();
	static UINT AFX_CDECL ThreadProc(LPVOID arg)
	{
		CWaveSoapFrontDoc * pDoc = DYNAMIC_DOWNCAST(CWaveSoapFrontDoc, (CObject *)arg);
		if (NULL == pDoc)
		{
			return ~0U;
		}
		return pDoc->_ThreadProc();
	}


// Generated message map functions
public:
	virtual BOOL DoFileSave();
	void OnActivateDocument(BOOL bActivate);

	void QueueOperation(COperationContext * pContext);
	void UpdateFrameTitles()
	{
		UpdateFrameCounts();
	}
	void PostFileSave(CWaveFile & DstFile,
					LPCTSTR NewName, BOOL SameName);

	BOOL PostCommitFileSave(int flags, LPCTSTR FullTargetName);
	BOOL DoPaste(SAMPLE_INDEX Start, SAMPLE_INDEX End, CHANNEL_MASK Channel, LPCTSTR FileName);

protected:
	// save the selected area to the permanent or temporary file
	BOOL CanCloseFrame(CFrameWnd* pFrameArg);
	void DoCopy(SAMPLE_INDEX Start, SAMPLE_INDEX End, CHANNEL_MASK Channel, LPCTSTR FileName);
	void DoCut(SAMPLE_INDEX Start, SAMPLE_INDEX End, CHANNEL_MASK Channel);
	void DoDelete(SAMPLE_INDEX Start, SAMPLE_INDEX End, CHANNEL_MASK Channel);
	void DeleteUndo();
	void DeleteRedo();

	void OnUpdateSampleRate(CCmdUI* pCmdUI, unsigned SampleRate);
	void SetSampleRate(unsigned SampleRate);
	void ChangeChannels(NUMBER_OF_CHANNELS nChannels);

	//{{AFX_MSG(CWaveSoapFrontDoc)
	afx_msg void OnEditCopy();
	afx_msg void OnUpdateEditCopy(CCmdUI* pCmdUI);
	afx_msg void OnEditCut();
	afx_msg void OnUpdateEditCut(CCmdUI* pCmdUI);
	afx_msg void OnUpdateEditPaste(CCmdUI* pCmdUI);
	afx_msg void OnEditUndo();
	afx_msg void OnUpdateEditUndo(CCmdUI* pCmdUI);
	afx_msg void OnEditStop();
	afx_msg void OnUpdateEditStop(CCmdUI* pCmdUI);
	afx_msg void OnUpdateFileSave(CCmdUI* pCmdUI);
	afx_msg void OnEditSelectAll();
	afx_msg void OnEditSelection();
	afx_msg void OnUpdateEditSelection(CCmdUI* pCmdUI);
	afx_msg void OnUpdateSoundPlay(CCmdUI* pCmdUI);
	afx_msg void OnSoundPlay();
	afx_msg void OnUpdateSoundStop(CCmdUI* pCmdUI);
	afx_msg void OnSoundStop();
	afx_msg void OnStopAll();
	afx_msg void OnSoundPause();
	afx_msg void OnPlayAndStop();
	afx_msg void OnUpdateSoundPause(CCmdUI* pCmdUI);
	afx_msg void OnUpdateIndicatorFileSize(CCmdUI* pCmdUI);
	afx_msg void OnUpdateIndicatorCurrentPos(CCmdUI* pCmdUI);
	afx_msg void OnUpdateIndicatorSelectionLength(CCmdUI* pCmdUI);
	afx_msg void OnUpdateIndicatorSampleRate(CCmdUI* pCmdUI);
	afx_msg void OnUpdateIndicatorSampleSize(CCmdUI* pCmdUI);
	afx_msg void OnUpdateIndicatorChannels(CCmdUI* pCmdUI);
	afx_msg void OnEditDelete();
	afx_msg void OnUpdateEditDelete(CCmdUI* pCmdUI);
	afx_msg void OnEditPaste();
	afx_msg void OnEditChannelsLock();
	afx_msg void OnUpdateEditChannelsLock(CCmdUI* pCmdUI);
	afx_msg void OnEditRedo();
	afx_msg void OnUpdateEditRedo(CCmdUI* pCmdUI);
	afx_msg void OnSample16bit();
	afx_msg void OnUpdateSample16bit(CCmdUI* pCmdUI);
	afx_msg void OnSample8bit();
	afx_msg void OnUpdateSample8bit(CCmdUI* pCmdUI);
	afx_msg void OnSamplerate11025();
	afx_msg void OnUpdateSamplerate11025(CCmdUI* pCmdUI);
	afx_msg void OnSamplerate16000();
	afx_msg void OnUpdateSamplerate16000(CCmdUI* pCmdUI);
	afx_msg void OnSamplerate22050();
	afx_msg void OnUpdateSamplerate22050(CCmdUI* pCmdUI);
	afx_msg void OnSamplerate32000();
	afx_msg void OnUpdateSamplerate32000(CCmdUI* pCmdUI);
	afx_msg void OnSamplerate44100();
	afx_msg void OnUpdateSamplerate44100(CCmdUI* pCmdUI);
	afx_msg void OnSamplerate48k();
	afx_msg void OnUpdateSamplerate48k(CCmdUI* pCmdUI);
	afx_msg void OnSamplerate7200();
	afx_msg void OnUpdateSamplerate7200(CCmdUI* pCmdUI);
	afx_msg void OnSamplerate8000();
	afx_msg void OnUpdateSamplerate8000(CCmdUI* pCmdUI);
	afx_msg void OnSamplerate96k();
	afx_msg void OnUpdateSamplerate96k(CCmdUI* pCmdUI);
	afx_msg void OnChannelsMono();
	afx_msg void OnUpdateChannelsMono(CCmdUI* pCmdUI);
	afx_msg void OnChannelsStereo();
	afx_msg void OnUpdateChannelsStereo(CCmdUI* pCmdUI);
	afx_msg void OnProcessChangevolume();
	afx_msg void OnUpdateProcessChangevolume(CCmdUI* pCmdUI);
	afx_msg void OnViewStatusHhmmss();
	afx_msg void OnViewStatusSamples();
	afx_msg void OnViewStatusSeconds();
	afx_msg void OnUpdateProcessDcoffset(CCmdUI* pCmdUI);
	afx_msg void OnProcessDcoffset();
	afx_msg void OnUpdateProcessInsertsilence(CCmdUI* pCmdUI);
	afx_msg void OnProcessInsertsilence();
	afx_msg void OnUpdateProcessMute(CCmdUI* pCmdUI);
	afx_msg void OnProcessMute();
	afx_msg void OnUpdateProcessNormalize(CCmdUI* pCmdUI);
	afx_msg void OnProcessNormalize();
	afx_msg void OnUpdateProcessResample(CCmdUI* pCmdUI);
	afx_msg void OnProcessResample();
	afx_msg void OnFileStatistics();
	afx_msg void OnUpdateFileStatistics(CCmdUI* pCmdUI);
	afx_msg void OnUpdateEditGoto(CCmdUI* pCmdUI);
	afx_msg void OnEditGoto();
	afx_msg void OnUpdateProcessInvert(CCmdUI* pCmdUI);
	afx_msg void OnProcessInvert();
	afx_msg void OnUpdateViewRescanPeaks(CCmdUI* pCmdUI);
	afx_msg void OnViewRescanPeaks();
	afx_msg void OnUpdateProcessSynthesisExpressionEvaluation(CCmdUI* pCmdUI);
	afx_msg void OnProcessSynthesisExpressionEvaluation();
	afx_msg void OnUpdateViewStatusHhmmss(CCmdUI* pCmdUI);
	afx_msg void OnUpdateViewStatusSamples(CCmdUI* pCmdUI);
	afx_msg void OnUpdateViewStatusSeconds(CCmdUI* pCmdUI);
	afx_msg void OnUpdateFileSaveAs(CCmdUI* pCmdUI);
	afx_msg void OnUpdateFileSaveCopyAs(CCmdUI* pCmdUI);
	afx_msg void OnFileSaveCopyAs();
	afx_msg void OnUpdateToolsInterpolate(CCmdUI* pCmdUI);
	afx_msg void OnToolsInterpolate();
	afx_msg void OnUpdateProcessDoUlf(CCmdUI* pCmdUI);
	afx_msg void OnProcessDoUlf();
	afx_msg void OnUpdateProcessDoDeclicking(CCmdUI* pCmdUI);
	afx_msg void OnProcessDoDeclicking();
	afx_msg void OnUpdateProcessNoiseReduction(CCmdUI* pCmdUI);
	afx_msg void OnProcessNoiseReduction();
	afx_msg void OnFileClose();
	afx_msg void OnProcessChannels();
	afx_msg void OnUpdateProcessChannels(CCmdUI* pCmdUI);
	afx_msg void OnSamplerateCustom();
	afx_msg void OnUpdateSamplerateCustom(CCmdUI* pCmdUI);
	afx_msg void OnEditEnableUndo();
	afx_msg void OnUpdateEditEnableUndo(CCmdUI* pCmdUI);
	afx_msg void OnEditEnableRedo();
	afx_msg void OnUpdateEditEnableRedo(CCmdUI* pCmdUI);
	afx_msg void OnEditClearUndo();
	afx_msg void OnUpdateEditClearUndo(CCmdUI* pCmdUI);
	afx_msg void OnEditClearRedo();
	afx_msg void OnUpdateEditClearRedo(CCmdUI* pCmdUI);
	afx_msg void OnProcessEqualizer();
	afx_msg void OnUpdateProcessEqualizer(CCmdUI* pCmdUI);
	afx_msg void OnProcessSwapchannels();
	afx_msg void OnUpdateProcessSwapchannels(CCmdUI* pCmdUI);
	afx_msg void OnProcessFilter();
	afx_msg void OnUpdateProcessFilter(CCmdUI* pCmdUI);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};

#pragma pack(push, 1)
struct MP3_IDTAG
{
	char Tag[3];    // "TAG"
	char Title[30];
	char Artist[30];
	char Album[30];
	char Year[4];
	union {
		char CommentV0[30];
		struct {
			char CommentV1[28];
			char Track[2];
		};
	};
	char Genre;
};
#pragma pack(pop)

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_WAVESOAPFRONTDOC_H__FFA16C4C_2FA7_11D4_9ADD_00C0F0583C4B__INCLUDED_)
