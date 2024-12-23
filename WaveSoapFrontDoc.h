// Copyright Alexander Grigoriev, 1997-2002, All Rights Reserved
// WaveSoapFrontDoc.h : interface of the CWaveSoapFrontDoc class
//
/////////////////////////////////////////////////////////////////////////////
#pragma once
#include "WmaFile.h"
#include "KListEntry.h"
#include "KInterlocked.h"
#include "ApplicationParameters.h"

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
					SAMPLE_INDEX Begin, SAMPLE_INDEX End, NUMBER_OF_SAMPLES NewLength)
		: m_UpdateCode(UpdateCode)
		, m_NewLength(NewLength)
		, m_FileID(FileID)
	{
		if (Begin <= End)
		{
			m_Begin = Begin;
			m_End = End;
		}
		else
		{
			m_Begin = End;
			m_End = Begin;
		}
	}

	ULONG_PTR m_FileID;
	int m_UpdateCode;

	SAMPLE_INDEX m_Begin;

	SAMPLE_INDEX m_End;
	NUMBER_OF_SAMPLES m_NewLength;
};

class CPlaybackUpdateInfo : public CSoundUpdateInfo
{
public:
	CPlaybackUpdateInfo(int UpdateCode, ULONG_PTR FileID,
						SAMPLE_INDEX PlaybackPosition, SAMPLE_INDEX PlaybackEnd, CHANNEL_MASK PlaybackChannel)
		: CSoundUpdateInfo(UpdateCode, FileID, 0, 0, -1)
		, m_PlaybackChannel(PlaybackChannel)
	{
		m_Begin = PlaybackPosition;
		m_End = PlaybackEnd;
	}

	SAMPLE_INDEX PlaybackPosition() const
	{
		return m_Begin;
	}

	SAMPLE_INDEX PlaybackEnd() const
	{
		return m_End;
	}

	CHANNEL_MASK PlaybackChannel() const
	{
		return m_PlaybackChannel;
	}

	CHANNEL_MASK m_PlaybackChannel;
};

struct MarkerRegionUpdateInfo : public CObject
{
	WAVEREGIONINFO info;
};

struct NoiseThresholdUpdateInfo : public CObject
{
	struct NoiseReductionParameters * pNoiseReductionParameters;
	int FftOrder;
	long SampleRate;
};

// Active document have highest priority for disk-intensive operations.
// while it is executing a disk-intensive command,
// such operations with non-active documents are suspended.
// To implement it, App object starts a thread for such operations.
//
enum {
	UpdateSoundDontRescanPeaks = 1,
	UpdateSoundSamplingRateChanged = 2,

	// variants:
	// 1 - make caret unconditionally visible, only scroll to bring it to the view.
	// If minimum scroll would discard all the currently visible area, move the cursor to the center.
	// When bringing the caret from outside the visible area, keep caret within the selection margin from the view edge.
	// Page up/down should not change the caret position in the window.
	//
	SetSelection_MakeCaretVisible = 1,		// move caret within margins of the active view
	SetSelection_MoveCaretToCenter = 2,		// move caret into center of the active view
	SetSelection_KeepCaretVisible = SetSelection_MakeCaretVisible | SetSelection_MoveCaretToCenter,
	SetSelection_SnapToMaximum = 4,
	// set caret to the maximum of the view column granularity (for example, when clicked on the view or outline)
	// make sure the end of file is in the view (that not all the view is beyond EOF); change origin if necessary.
	// it's used when the file is replaced because of sample rate change
	SetSelection_MakeFileVisible = 8,
	SetSelection_KeepCaretPosition = 0x10,
	SetSelection_DontAdjustView = 0x20,	// Don't shift view
	SetSelection_Autoscroll = 0x40,		// move caret within margins of the active view

	SaveFile_SameName = 4,
	SaveFile_CloseAfterSave = 8,
	SaveFile_SaveCopy = 0x10,
	SaveFile_DontPromptReopen = 0x20,
	SaveFile_DontCopyMetadata = 0x40,
	SaveFile_SameFolder = 0x80,
	SaveFile_SavePartial = 0x100,
	SaveFile_SameFormat = 0x200,
	SaveFile_DontPromptChannelReduction = 0x400,
	SaveFile_WavFile = 0,
	SaveFile_Mp3File = OpenDocumentMp3File,  // 0x1000
	SaveFile_WmaFile = OpenDocumentWmaFile, //0x2000
	SaveFile_RawFile = OpenDocumentRawFile, //0x4000
	//SaveFile_AviFile = OpenDocumentAviFile, //0x8000
	SaveRawFileMsbFirst = OpenRawFileMsbFirst,   //0x10000
	SaveFile_NonWavFile = OpenDocumentNonWavFile,

};

class CWaveSoapFrontDoc : public CDocument, protected UndoRedoParameters
{
	typedef CDocument BaseClass;
protected: // create from serialization only
	CWaveSoapFrontDoc();
	DECLARE_DYNCREATE(CWaveSoapFrontDoc)

// Attributes
public:
	BOOL UndoEnabled() const
	{
		return m_UndoEnabled;
	}
	BOOL RedoEnabled() const
	{
		return m_RedoEnabled;
	}
	int GetUndoDepth() const;
	int GetRedoDepth() const;
	ULONGLONG GetUndoSize() const;
	ULONGLONG GetRedoSize() const;

	UndoRedoParameters const * GetUndoParameters() const
	{
		return this;
	}

	BOOL ChannelsLocked() const
	{
		return m_bChannelsLocked;
	}

	NUMBER_OF_SAMPLES WaveFileSamples() const;
	LPMMCKINFO WaveDataChunk() const;
	CWaveFormat const & WaveFormat() const;
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

	BOOL OnSaveDocument(LPCTSTR lpszPathName, DWORD flags, WAVEFORMATEX const * pWf);

	BOOL OnSaveFileOrPart(class COperationContext ** ppOp, int flags, LPCTSTR FullTargetName, WAVEFORMATEX const * pWf,
						SAMPLE_INDEX Begin = 0, SAMPLE_INDEX End = LAST_SAMPLE, CHANNEL_MASK ChannelsToCopy = 0);

	BOOL OnSaveDirectFile();
	BOOL OnSaveBufferedPcmFile(class COperationContext ** ppOp, int flags, LPCTSTR FullTargetName);
	BOOL OnSaveBufferedPcmFileCopy(class COperationContext ** ppOp, int flags, LPCTSTR FullTargetName);

	BOOL OnSaveConvertedFile(class COperationContext ** ppOp, int flags, LPCTSTR FullTargetName, WAVEFORMATEX const * pWf,
							SAMPLE_INDEX Begin, SAMPLE_INDEX End, CHANNEL_MASK ChannelsToCopy);
	BOOL OnSaveMp3File(class COperationContext ** ppOp, int flags, LPCTSTR FullTargetName, WAVEFORMATEX const * pWf,
						SAMPLE_INDEX Begin, SAMPLE_INDEX End, CHANNEL_MASK ChannelsToCopy);
	BOOL OnSaveWmaFile(class COperationContext ** ppOp, int flags, LPCTSTR FullTargetName, WAVEFORMATEX const * pWf,
						SAMPLE_INDEX Begin, SAMPLE_INDEX End, CHANNEL_MASK ChannelsToCopy);
	BOOL OnSaveRawFile(class COperationContext ** ppOp, int flags, LPCTSTR FullTargetName, WAVEFORMATEX const * pWf,
						SAMPLE_INDEX Begin, SAMPLE_INDEX End, CHANNEL_MASK ChannelsToCopy);

	// flags OpenDocumentReadOnly - ReadOnly, 2 - DirectMode
	virtual BOOL OnOpenDocument(LPCTSTR lpszPathName, int flags);
// Overrides
	virtual BOOL OnNewDocument(NewFileParameters * pParams);
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CWaveSoapFrontDoc)
public:
	virtual BOOL OnCmdMsg(UINT nID, int nCode, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo);
	virtual void SetPathName(LPCTSTR lpszPathName, BOOL bAddToMRU = TRUE);
protected:
	virtual BOOL SaveModified();
	//}}AFX_VIRTUAL
	virtual void OnIdle();
	virtual BOOL DoSave(LPCTSTR lpszPathName, BOOL bReplace = TRUE);
	BOOL OpenWmaFileDocument(LPCTSTR lpszPathName);
	BOOL OpenDShowFileDocument(LPCTSTR lpszPathName);
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

	// this is waveformat for the original file (OriginalWavFile).
	// OriginalWavFile may not carry valid waveformat, if it's raw or other non-WAV
	// m_OriginalWaveFormat is set to PCM format after decompression of WMA file.
	CWaveFormat m_OriginalWaveFormat;

	int m_ModificationSequenceNumber;

	//WAVEFORMATEX WavFileFormat;

	SAMPLE_INDEX m_CaretPosition;		// from m_SelectionStart to m_SelectionEnd
	SAMPLE_INDEX m_SelectionStart;		// <= m_SelectionEnd
	SAMPLE_INDEX m_SelectionEnd;		// >= m_SelectionEnd

	CHANNEL_MASK m_SelectedChannel;

	CHANNEL_MASK GetSelectedChannel() const;

	void SetSelection(SAMPLE_INDEX begin, SAMPLE_INDEX end, CHANNEL_MASK channel,
					SAMPLE_INDEX caret, int flags = 0);		// begin and end can be not in order
	void SelectBetweenMarkers(SAMPLE_INDEX Origin);
	void GotoNextMarker();
	void GotoPrevMarker();
	SAMPLE_INDEX GetNextMarker() const;
	SAMPLE_INDEX GetPrevMarker() const;

	void SoundChanged(ULONG_PTR FileID, SAMPLE_INDEX begin, SAMPLE_INDEX end,
					NUMBER_OF_SAMPLES length = -1, DWORD flags = 0);
	void FileChanged(CWaveFile & File, SAMPLE_POSITION begin,
					SAMPLE_POSITION end, NUMBER_OF_SAMPLES length = -1, DWORD flags = 0);

	void PlaybackPositionNotify(SAMPLE_INDEX position, CHANNEL_MASK channel, SAMPLE_INDEX playback_end);
	BOOL ProcessUpdatePlaybackPosition();

	enum {
		UpdateSelectionChanged = 1,
		UpdateSelectionModeChanged,
		UpdateSoundChanged,
		UpdateSampleRateChanged,
		UpdateWholeFileChanged,
		UpdatePlaybackPositionChanged,
		UpdateMarkerRegionChanged,
		UpdateNoiseThresholdChanged,
	};

// Implementation
public:
	virtual ~CWaveSoapFrontDoc();
	void UpdateDocumentTitle();

	void SetWaveFormat(WAVEFORMATEX const * pWf)
	{
		m_WavFile.SetWaveFormat(pWf);
	}
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
	void CWaveSoapFrontDoc::QueuePlaybackUpdate(ULONG_PTR FileID,
		SAMPLE_INDEX PlaybackPosition, CHANNEL_MASK PlaybackChannel, SAMPLE_INDEX playback_end);

	enum {
		QueueSoundUpdateMerge = 1,  // merge update range
		QueueSoundUpdateReplace = 2, // replace existing item
	};
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

	LONG_volatile m_OperationInProgress;
	long volatile m_StopOperation;
	bool m_OperationNonCritical;
	bool m_PlayingSound;
	bool m_bInOnIdle;
	class CSoundPlayContext *m_pSoundPlayContext;

	bool m_bChannelsLocked;
	CHANNEL_MASK m_PrevChannelToCopy;
	PASTE_MODE m_DefaultPasteMode;
	PASTE_RESAMPLE_MODE m_PasteResampleMode;

	BOOL m_UseFadeInOut;
	int m_FadeInOutLengthMs;
	int m_FadeInEnvelope;
	int m_FadeOutEnvelope;

	CString m_CurrentStatusString;
	CSimpleCriticalSection m_StatusStringLock;
	void GetCurrentStatusString(CString & str);
	void SetCurrentStatusString(const CString & str);

	bool m_bReadOnly;
	bool m_bDirectMode;
	bool m_bClosing;    // tells OnSaveDocument that it is called in close context
	bool m_bClosePending;   // Save is in progress, request close afterwards
	bool m_bCloseThisDocumentNow;   // CDocTemplate should close it in OnIdle

	ListHead<COperationContext> m_UndoList;
	ListHead<COperationContext> m_RedoList;
	LockedListHead<COperationContext> m_RetiredList;
	LockedListHead<CSoundUpdateInfo> m_UpdateList;
	FILETIME m_ThreadUserTime;

protected:
	CContextWorkerThread m_Thread;

// Generated message map functions
public:
	virtual BOOL DoFileSave();
	void OnActivateDocument(BOOL bActivate);

	void QueueOperation(COperationContext * pContext);
	void KickDocumentThread();

	void ExecuteOperation(COperationContext * pContext, BOOL SetModify = FALSE, int UndoCreated = -1, int KeepPreviousUndo = -1);
	void SignalStopOperation();

	void UpdateFrameTitles()
	{
		UpdateFrameCounts();
	}
	void PostFileSave(CWaveFile & DstFile,
					LPCTSTR NewName, BOOL SameName);

	BOOL PostCommitFileSave(int flags, LPCTSTR FullTargetName);
	BOOL DoPaste(SAMPLE_INDEX Start, SAMPLE_INDEX End, CHANNEL_MASK Channel,
				ULONG PasteFlags = 0, LPCTSTR FileName = NULL);

	enum {
		PasteFlagSetNewSelection = 1,
		PasteFlagReplaceSelectionQuiet = 2,
	};

	BOOL ChangeWaveMarker(WAVEREGIONINFO * pInfo);
	void UpdateAllMarkers();

	void BeginMarkerChange(unsigned ChangeFlags);   // create undo, increment change count
	void EndMarkerChange(BOOL bCommitChanges = TRUE);

	void ClearUndo()
	{
		OnEditClearUndo();
	}
	void ClearRedo()
	{
		OnEditClearRedo();
	}
	bool CanClearUndo() const
	{
		return !m_UndoList.IsEmpty();
	}
	bool CanClearRedo() const
	{
		return !m_RedoList.IsEmpty();
	}
protected:
	// save the selected area to the permanent or temporary file
	BOOL CanCloseFrame(CFrameWnd* pFrameArg);
	void DoCopy(SAMPLE_INDEX Start, SAMPLE_INDEX End, CHANNEL_MASK Channel, LPCTSTR FileName);
	void DoCut(SAMPLE_INDEX Start, SAMPLE_INDEX End, CHANNEL_MASK Channel);
	void DoDelete(SAMPLE_INDEX Start, SAMPLE_INDEX End, CHANNEL_MASK Channel);

	void DoFadeInOut(BOOL FadeOut);

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

	afx_msg void OnViewStatusHhmmss();
	afx_msg void OnViewStatusSamples();
	afx_msg void OnViewStatusSeconds();
	afx_msg void OnViewStatusHhmmssFf();

	afx_msg void OnUpdateViewStatusHhmmss(CCmdUI* pCmdUI);
	afx_msg void OnUpdateViewStatusSamples(CCmdUI* pCmdUI);
	afx_msg void OnUpdateViewStatusSeconds(CCmdUI* pCmdUI);
	afx_msg void OnUpdateViewStatusHhmmssFf(CCmdUI* pCmdUI);

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
	afx_msg void OnProcessReverse();
	afx_msg void OnUpdateProcessReverse(CCmdUI *pCmdUI);
	afx_msg void OnEditWaveMarker();
	afx_msg void OnUpdateEditWaveMarker(CCmdUI *pCmdUI);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

public:
	afx_msg void OnSaveSaveselectionas();
	afx_msg void OnUpdateSaveSaveselectionas(CCmdUI *pCmdUI);
	afx_msg void OnSaveSplitToFiles();
	afx_msg void OnUpdateSaveSplitToFiles(CCmdUI *pCmdUI);
	afx_msg void OnEditMoreUndoRedo();
	afx_msg void OnUpdateEditMoreUndoRedo(CCmdUI *pCmdUI);
	afx_msg void OnEditFadeOut();
	afx_msg void OnEditFadeIn();
	afx_msg void OnUpdateEditFadeIn(CCmdUI *pCmdUI);
	afx_msg void OnUpdateEditFadeOut(CCmdUI *pCmdUI);
	afx_msg void OnProcessGilbertTransform();
	afx_msg void OnUpdateProcessGilbertTransform(CCmdUI *pCmdUI);
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

