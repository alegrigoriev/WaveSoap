// WaveFile.h
#ifndef WAVE_FILE__H__
#define WAVE_FILE__H__
#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
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
};

class CMmioFile
{
	class CSimpleCriticalSection
	{
		CRITICAL_SECTION m_cs;
	public:
		CSimpleCriticalSection()
		{
			InitializeCriticalSection( & m_cs);
		}
		~CSimpleCriticalSection()
		{
			DeleteCriticalSection( & m_cs);
		}
		void Lock()
		{
			EnterCriticalSection( & m_cs);
		}
		void Unlock()
		{
			LeaveCriticalSection( & m_cs);
		}
	};
	class CSimpleCriticalSectionLock
	{
		CSimpleCriticalSection & m_cs;
	public:
		CSimpleCriticalSectionLock(CSimpleCriticalSection & cs)
			: m_cs(cs)
		{
			cs.Lock();
		}
		~CSimpleCriticalSectionLock()
		{
			m_cs.Unlock();
		}
	};
public:
	// construction
	CMmioFile();
	virtual BOOL Open( LPCTSTR lpszFileName, UINT nOpenFlags);
	virtual void Close( );
	CMmioFile & operator =(CMmioFile &);


	BOOL GetFileInformationByHandle(LPBY_HANDLE_FILE_INFORMATION lpFileInformation)
	{
		return m_File.GetFileInformationByHandle(lpFileInformation);
	}

	// read/write:
	LONG Read( void* lpBuf, LONG nCount )
	{
		CSimpleCriticalSectionLock lock(m_cs);
		return mmioRead(m_hmmio, (HPSTR) lpBuf, nCount);
	}

	// read data at the specified position
	// current position won't change after this function
	LONG ReadAt(void * lpBuf, LONG nCount, LONG Position);

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

	CDirectFile m_File;
	HMMIO m_hmmio;
	FOURCC m_RiffckType;
	CSimpleCriticalSection m_cs;
	void * GetCommonData() const
	{
		char * tmp = (char *)m_File.GetCommonData();
		if (NULL == tmp)
		{
			return NULL;
		}
		return tmp + sizeof (MMCKINFO);
	}
	size_t GetCommonDataSize() const
	{
		size_t tmp = m_File.GetCommonDataSize();
		if (tmp >= sizeof (MMCKINFO))
		{
			return tmp - sizeof (MMCKINFO);
		}
		else
		{
			return 0;
		}
	}

	void * AllocateCommonData(size_t size)
	{
		char * tmp = (char *)m_File.AllocateCommonData(size + sizeof (MMCKINFO));
		if (NULL == tmp)
		{
			return NULL;
		}
		return tmp + sizeof (MMCKINFO);
	}

	LPMMCKINFO GetRiffChunk() const
	{
		return (LPMMCKINFO)m_File.GetCommonData();
	}

	BOOL LoadRiffChunk();

	BOOL IsOpen() const
	{
		return m_hmmio != NULL;
	}
	DWORD GetFileID() const
	{
		return m_File.GetFileID();
	}

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
	CreateWaveFileTempDir = 0x00100000,
	CreateWaveFileDontCopyInfo = 0x00200000,
	CreateWaveFilePcmFormat = 0x00400000,
	CreateWaveFileTemp = 0x00800000,
	CreateWaveFileAttachTemplateAsSource = 0x01000000,
};

class CWaveFile : public CMmioFile
{
public:
	CWaveFile();
	~CWaveFile();
	BOOL CreateWaveFile(CWaveFile * pTemplateFile, int Channel, int Samples, DWORD flags, LPCTSTR FileName);
#if 0
	virtual BOOL Open( LPCTSTR lpszFileName, UINT nOpenFlags);
#endif
	virtual void Close( );
	int SampleSize() const
	{
		WAVEFORMATEX * pWf = GetWaveFormat();
		if (NULL == pWf
			|| 0 == pWf->nChannels
			|| 0 == pWf->wBitsPerSample)
		{
			return 0;
		}
		return pWf->nChannels * pWf->wBitsPerSample / 8;
	}
	int Channels() const
	{
		WAVEFORMATEX * pWf = GetWaveFormat();
		if (NULL == pWf)
		{
			return 0;
		}
		return pWf->nChannels;
	}
	LONG NumberOfSamples() const
	{
		WAVEFORMATEX * pWf = GetWaveFormat();
		LPMMCKINFO pDatack = GetDataChunk();
		if (NULL == pWf
			|| NULL == pDatack
			|| 0 == pWf->nChannels
			|| 0 == pWf->wBitsPerSample)
		{
			return 0;
		}
		return pDatack->cksize / (pWf->nChannels * pWf->wBitsPerSample / 8);
	}
	CWaveFile & operator =(CWaveFile &);

	//MMCKINFO m_datack;

	BOOL LoadWaveformat();
	BOOL FindData();

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
		return (LPMMCKINFO) GetCommonData();
	}
	WAVEFORMATEX * GetWaveFormat() const
	{
		char * tmp = (char *)GetCommonData();
		if (tmp)
		{
			return (WAVEFORMATEX *)(tmp + sizeof (MMCKINFO));
		}
		else
		{
			return NULL;
		}
	}
	//WAVEFORMATEX * m_pWf;
private:
	// wrong type of constructor
	CWaveFile(const CWaveFile &)
	{
		ASSERT(FALSE);
	}
};

#endif //#ifndef WAVE_FILE__H__
