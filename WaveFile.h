// Copyright Alexander Grigoriev, 1997-2002, All Rights Reserved
// WaveFile.h
#ifndef WAVE_FILE__H__
#define WAVE_FILE__H__
#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
#include <vector>
/*
Class CWaveFile serves as a WAV file input/output
helper.
*/
#ifndef __AFX_H__
 #include <afx.h>
#endif
#include <Afxmt.h>
/*
  Base class supports:
  Ascend and descend to a chunk
  Automatic descend to RIFF chunk
  Data read/write

*/
#include <mmsystem.h>
#include "DirectFile.h"
#include "WaveSupport.h"
#include <atlbase.h>
#include <atlpath.h>

typedef long SAMPLE_INDEX;
typedef long NUMBER_OF_SAMPLES;
typedef int CHANNEL_MASK;
typedef int NUMBER_OF_CHANNELS;
typedef DWORD SAMPLE_POSITION;
typedef DWORD WAV_FILE_SIZE;
typedef DWORD MEDIA_FILE_SIZE;  // to be expanded to 64 bits
typedef unsigned PEAK_INDEX;
typedef __int16 WAVE_SAMPLE;

enum
{
	MmioFileOpenExisting = CDirectFile::OpenExisting,
	MmioFileOpenCreateNew = CDirectFile::CreateNew,
	MmioFileOpenCreateAlways = CDirectFile::CreateAlways,
	MmioFileOpenReadOnly = CDirectFile::OpenReadOnly,
	// if couldn't be opened for writing, try read-only
	MmioFileAllowReadOnlyFallback = CDirectFile::OpenAllowReadOnlyFallback,
	MmioFileOpenDeleteAfterClose = CDirectFile::OpenDeleteAfterClose,
	MmioFileOpenDontCreateRiff = 0x80000000,
	MmioFileOpenDontLoadRiff = MmioFileOpenDontCreateRiff,
	MmioFileMemoryFile = CDirectFile::CreateMemoryFile,
};

class CMmioFile : public CDirectFile
{
	typedef CDirectFile BaseClass;
public:
	// construction
	CMmioFile();
	virtual BOOL Open(LPCTSTR lpszFileName, UINT nOpenFlags);
	virtual void Close();
	CMmioFile & operator =(CMmioFile &);

	// read/write:
	LONG Read( void* lpBuf, LONG nCount )
	{
		CSimpleCriticalSectionLock lock(m_cs);
		return mmioRead(m_hmmio, (HPSTR) lpBuf, nCount);
	}

	LONG Write( const void* lpBuf, LONG nCount )
	{
		CSimpleCriticalSectionLock lock(m_cs);
		return mmioWrite(m_hmmio, (const char*)lpBuf, nCount);
	}
	LONG Seek(LONG lOffset, int iOrigin = SEEK_SET)
	{
		CSimpleCriticalSectionLock lock(m_cs);
		return mmioSeek(m_hmmio, lOffset, iOrigin);
	}

	void Flush(UINT flag = 0)   // possible value: MMIO_EMPTYBUF
	{
		CSimpleCriticalSectionLock lock(m_cs);
		mmioFlush(m_hmmio, 0);
	}

	BOOL Ascend(MMCKINFO & ck)
	{
		CSimpleCriticalSectionLock lock(m_cs);
		return MMSYSERR_NOERROR == mmioAscend(m_hmmio, & ck, 0);
	}

	BOOL Descend(MMCKINFO & ck, LPMMCKINFO lpckParent, UINT uFlags = 0)
	{
		CSimpleCriticalSectionLock lock(m_cs);
		return MMSYSERR_NOERROR == mmioDescend(m_hmmio, & ck, lpckParent, uFlags);
	}

	BOOL FindChunk(MMCKINFO & ck, LPMMCKINFO lpckParent)
	{
		return Descend(ck, lpckParent, MMIO_FINDCHUNK);
	}
	BOOL FindList(MMCKINFO & ck, LPMMCKINFO lpckParent)
	{
		return Descend(ck, lpckParent, MMIO_FINDLIST);
	}
	BOOL FindRiff()
	{
		return Descend( * GetRiffChunk(), NULL, MMIO_FINDRIFF);
	}

	BOOL Advance(LPMMIOINFO mmio, UINT uFlags)
	{
		CSimpleCriticalSectionLock lock(m_cs);
		return MMSYSERR_NOERROR == mmioAdvance(m_hmmio, mmio, uFlags);
	}

	BOOL CreateChunk(MMCKINFO & ck, UINT wFlags)
	{
		CSimpleCriticalSectionLock lock(m_cs);
		ck.cksize = 0;
		return MMSYSERR_NOERROR == mmioCreateChunk(m_hmmio, & ck, wFlags);
	}
	BOOL CreateList(MMCKINFO & ck)
	{
		return CreateChunk(ck, MMIO_CREATELIST);
	}
	BOOL CreateRiff(MMCKINFO & ck)
	{
		return CreateChunk(ck, MMIO_CREATERIFF);
	}

	BOOL GetInfo(MMIOINFO & mmioinfo, UINT wFlags = 0)
	{
		CSimpleCriticalSectionLock lock(m_cs);
		return MMSYSERR_NOERROR == mmioGetInfo(m_hmmio, & mmioinfo, wFlags);
	}

	virtual ~CMmioFile();

	HMMIO m_hmmio;
	FOURCC m_RiffckType;
	CSimpleCriticalSection m_cs;

	struct InstanceDataMm : public BaseClass::InstanceData
	{
	private:
		typedef BaseClass::InstanceData BaseClass;
	public:
		MMCKINFO riffck;
		HMMIO m_hmmio;
		InstanceDataMm()
		{
			m_hmmio = NULL;
			memzero(riffck);
			m_size = sizeof *this;
		}
		InstanceDataMm & operator =(InstanceData const & src)
		{
			if (src.m_size >= sizeof *this)
			{
				InstanceDataMm const* src1 =
					static_cast<InstanceDataMm const*>( & src);
				riffck = src1->riffck;
				m_hmmio = src1->m_hmmio;
			}
			BaseClass::operator=(src);
			return *this;
		}
		InstanceDataMm & operator =(InstanceDataMm const & src)
		{
			return operator =(static_cast<InstanceData const &>(src));
		}
		virtual void MoveDataTo(InstanceData * dst)
		{
			InstanceDataMm * dst1 = static_cast<InstanceDataMm *>(dst);
			dst1->operator=(*this);
		}
	};

	InstanceDataMm * GetInstanceData() const
	{
		return static_cast<InstanceDataMm *>(CDirectFile::GetInstanceData());
	}
	LPMMCKINFO GetRiffChunk() const
	{
		return & GetInstanceData()->riffck;
	}

	BOOL LoadRiffChunk();
	BOOL ReadChunkString(ULONG Length, CString & String);

	BOOL IsOpen() const
	{
		return m_hmmio != NULL;
	}

	BOOL CommitChanges();

private:
	// wrong type of constructor
	CMmioFile(const CMmioFile &)
	{
		ASSERT(FALSE);
	}

	static LRESULT PASCAL BufferedIOProc(LPSTR lpmmioinfo, UINT wMsg,
										LPARAM lParam1, LPARAM lParam2);

};

enum {
	CreateWaveFileDeleteAfterClose = MmioFileOpenDeleteAfterClose,
	CreateWaveFileDontInitStructure = MmioFileOpenDontCreateRiff,
	CreateWaveFileAllowMemoryFile = MmioFileMemoryFile,
	CreateWaveFileTempDir = 0x00100000,
	CreateWaveFileDontCopyInfo = 0x00200000,
	CreateWaveFilePcmFormat = 0x00400000,
	CreateWaveFileTemp = 0x00800000, // create temporary name
	CreateWaveFileAttachTemplateAsSource = 0x01000000,
	CreateWaveFileSizeSpecified = 0x02000000,
	CreateWaveFileCreateFact = 0x04000000,
};

struct WaveMarker
{
	CString Name;
	CString Comment;
	ULONG CueId;
	ULONG StartSample;
	FOURCC fccRgn;  // 'rgn '
	ULONG LengthSamples;  // >1 if it is region
	bool operator <(WaveMarker const & op)
	{
		return StartSample < op.StartSample;
	}
};

struct WavePlaylistItem
{
	unsigned MarkerIndex;
	DWORD Length;   // samples
	DWORD NumLoops;
};

struct CuePointChunkItem
{
	DWORD NameId;   // unique ID
	DWORD SamplePosition; // ordinal sample position in the file
	FOURCC fccChunk;    // 'data'
	DWORD dwChunkStart;
	DWORD dwBlockStart;
	DWORD dwSampleOffset;
};

struct PlaylistSegment
{
	DWORD NameId;   // CuePoint ID
	DWORD Length;   // in samples
	DWORD Loops;    // number of loops
};
#pragma pack(push, 1)

struct LtxtChunk  // in LIST adtl
{
	DWORD NameId;   // CuePoint ID
	DWORD SampleLength; // length in samples. For cue point - 0 or 1
	DWORD Purpose;  //'rgn '
	WORD Country;
	WORD Language;
	WORD Dialect;
	WORD Codepage;
	// then goes zero-terminated text
	//UCHAR text[1];
};

#pragma pack(pop)

struct WavePeak
{
	WAVE_SAMPLE low;
	WAVE_SAMPLE high;
	WavePeak(WAVE_SAMPLE Low, WAVE_SAMPLE High)
		: low(Low), high(High) {}
	WavePeak() {}
};
class CWavePeaks
{

public:
	CWavePeaks(unsigned granularity);
	~CWavePeaks();
	unsigned GetGranularity() const
	{
		return m_PeakDataGranularity;
	}
	void SetPeakData(unsigned index, WAVE_SAMPLE low, WAVE_SAMPLE high)
	{
		ASSERT(index < m_WavePeakSize);
		m_pPeaks[index].low = low;
		m_pPeaks[index].high = high;
	}

	WAVE_SAMPLE GetPeakDataLow(PEAK_INDEX index) const
	{
		ASSERT(index < m_WavePeakSize);
		return m_pPeaks[index].low;
	}
	WAVE_SAMPLE GetPeakDataHigh(PEAK_INDEX index) const
	{
		ASSERT(index < m_WavePeakSize);
		return m_pPeaks[index].high;
	}

	WavePeak * AllocatePeakData(NUMBER_OF_SAMPLES NewNumberOfSamples, NUMBER_OF_CHANNELS NumberOfChannels = 1);
	WavePeak * GetPeakArray()
	{
		return m_pPeaks;
	}
	WavePeak const * GetPeakArray() const
	{
		return m_pPeaks;
	}
	WavePeak GetPeakMinMax(PEAK_INDEX from, PEAK_INDEX to, NUMBER_OF_CHANNELS stride = 1);

	unsigned GetPeaksSize() const
	{
		return m_WavePeakSize;
	}
	CSimpleCriticalSection & GetLock()
	{
		return m_PeakLock;
	}

	void SetPeaks(PEAK_INDEX from, PEAK_INDEX to, NUMBER_OF_CHANNELS stride, WavePeak value);
	CWavePeaks & operator =(CWavePeaks const & src);
protected:
	WavePeak * m_pPeaks;
	size_t m_WavePeakSize;
	size_t m_AllocatedWavePeakSize;
	unsigned m_PeakDataGranularity;
	CSimpleCriticalSection m_PeakLock;
};

class CWaveFile : public CMmioFile
{
	typedef CMmioFile BaseClass;
public:
	CWaveFile();
	~CWaveFile();
	BOOL CreateWaveFile(CWaveFile * pTemplateFile, WAVEFORMATEX * pTemplateFormat,
						NUMBER_OF_CHANNELS Channels, WAV_FILE_SIZE SizeOrSamples, DWORD flags, LPCTSTR FileName);

	virtual BOOL Open(LPCTSTR lpszFileName, UINT nOpenFlags);
	virtual void Close();
	int SampleSize() const;
	BOOL SetSourceFile(CWaveFile * const pOriginalFile);

	void RescanPeaks(SAMPLE_INDEX begin, SAMPLE_INDEX end);
	BOOL AllocatePeakData(NUMBER_OF_SAMPLES NewNumberOfSamples);
	WavePeak GetPeakMinMax(PEAK_INDEX from, PEAK_INDEX to, NUMBER_OF_CHANNELS stride = 1);
	unsigned GetPeaksSize() const;
	unsigned GetPeakGranularity() const;

	SAMPLE_POSITION SampleToPosition(SAMPLE_INDEX sample) const;
	SAMPLE_INDEX PositionToSample(SAMPLE_POSITION position) const;

	int CalculatePeakInfoSize() const
	{
		unsigned Granularity = GetPeakGranularity();
		return (NumberOfSamples() + Granularity - 1) / Granularity * Channels();
	}
	void SetPeakData(PEAK_INDEX index, WAVE_SAMPLE low, WAVE_SAMPLE high);
	BOOL LoadPeaksForCompressedFile(CWaveFile & OriginalWaveFile, ULONG NumberOfSamples);

	BOOL CheckAndLoadPeakFile();
	void SavePeakInfo(CWaveFile & SavedWaveFile);

	CSimpleCriticalSection & GetPeakLock()
	{
		return GetInstanceData()->m_PeakData.GetLock();
	}

	struct InstanceDataWav : BaseClass::InstanceDataMm
	{
	private:
		typedef CMmioFile::InstanceDataMm BaseInstanceClass;
	public:
		MMCKINFO datack;
		MMCKINFO fmtck;
		MMCKINFO factck;
		CWaveFormat wf;
		CWavePeaks m_PeakData;
		// TODO: use Unicode?
		CString Author;         // WM/Author
		CString DisplayTitle;   // WM/Title
		CString Album;          // WM/AlbumTitle
		CString Copyright;      // ICOP
		CString RecordingEngineer;  // IENG

		CString Title;
		CString OriginalArtist; // IART
		CString Date;       // WM/Year
		CString Genre;      // IGNR, WM/Genre
		CString Comment;    // ICMT
		CString Subject;   // ISBJ
		CString Keywords;  // IKEY
		CString Medium;  // IMED
		CString Source; // ISRC
		CString Digitizer; // ITCH
		CString DigitizationSource; // ISRF

		std::vector<WaveMarker> Markers;
		std::vector<WavePlaylistItem> Playlist;
		bool InfoChanged;

		InstanceDataWav()
			: m_PeakData(512)
		{
			memzero(datack);
			memzero(fmtck);
			memzero(factck);
			m_size = sizeof *this;
			InfoChanged = false;
		}
		// move all data to a derived (bigger) type
		InstanceDataWav & operator =(InstanceDataWav const & src)
		{
			datack = src.datack;
			fmtck = src.fmtck;
			factck = src.factck;
			wf = src.wf;

			Album = src.Album;
			Author = src.Author;
			Date = src.Date;
			Genre = src.Genre;
			Comment = src.Comment;
			Title = src.Title;
			DisplayTitle = src.DisplayTitle;

			Markers = src.Markers;

			Playlist = src.Playlist;
			m_PeakData = src.m_PeakData;

			BaseInstanceClass::operator =(src);
			return *this;
		}
		virtual void MoveDataTo(InstanceData * dst)
		{
			InstanceDataWav * dst1 = static_cast<InstanceDataWav *>(dst);
			*dst1 = *this;
		}
		void ResetMetadata()
		{
			Author.Empty();
			DisplayTitle.Empty();
			Album.Empty();
			Copyright.Empty();
			RecordingEngineer.Empty();

			Title.Empty();
			Date.Empty();
			Genre.Empty();
			Comment.Empty();
			Subject.Empty();
			Keywords.Empty();
			Medium.Empty();
			Source.Empty();
			Digitizer.Empty();
			DigitizationSource.Empty();

			Markers.clear();
			Playlist.clear();
			InfoChanged = false;

		}
	};
	InstanceDataWav * GetInstanceData() const
	{
		return static_cast<InstanceDataWav *>(CDirectFile::GetInstanceData());
	}
	CWavePeaks * GetWavePeaks() const
	{
		InstanceDataWav * pInstData = GetInstanceData();
		if (NULL == pInstData)
		{
			return NULL;
		}
		return & pInstData->m_PeakData;
	}
	void SetPeaks(PEAK_INDEX from, PEAK_INDEX to, NUMBER_OF_CHANNELS stride, WavePeak value);

	NUMBER_OF_CHANNELS Channels() const
	{
		WAVEFORMATEX * pWf = GetWaveFormat();
		if (NULL == pWf)
		{
			return 1;
		}
		return pWf->nChannels;
	}
	LONG NumberOfSamples() const;
	CWaveFile & operator =(CWaveFile &);

	WAVEFORMATEX * AllocateWaveformat(size_t FormatSize = sizeof (WAVEFORMATEX))
	{
		return AllocateInstanceData<InstanceDataWav>()->wf.Allocate(FormatSize - sizeof (WAVEFORMATEX));
	}

	BOOL LoadWaveformat();
	BOOL FindData();
	BOOL LoadMetadata();
	BOOL LoadListMetadata(MMCKINFO & chunk);
	BOOL ReadCueSheet(MMCKINFO & chunk);
	BOOL ReadPlaylist(MMCKINFO & chunk);

	WaveMarker * GetCueItem(DWORD CueId);

	unsigned SampleRate() const
	{
		WAVEFORMATEX * pWf = GetWaveFormat();
		if (pWf)
		{
			return pWf->nSamplesPerSec;
		}
		else
		{
			return 0;
		}
	}

	LPMMCKINFO GetDataChunk() const
	{
		return & GetInstanceData()->datack;
	}

	WAVEFORMATEX * GetWaveFormat() const;
	// save all changes in wave format and data chunk size
	BOOL CommitChanges();
	MMCKINFO * GetFmtChunk() const;
	MMCKINFO * GetFactChunk() const;

	DWORD m_FactSamples;
private:
	CPath MakePeakFileName(LPCTSTR FileName);
	// wrong type of constructor
	CWaveFile(const CWaveFile &)
	{
		ASSERT(FALSE);
	}
};

#endif //#ifndef WAVE_FILE__H__
