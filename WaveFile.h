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
	CMmioFile( LPCTSTR lpszFileName, UINT nOpenFlags );
	virtual BOOL Open( LPCTSTR lpszFileName, UINT nOpenFlags);
	virtual void Close( );
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
		return Descend(m_riffck, NULL, MMIO_FINDRIFF);
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
	enum { ReadBufferSize = 0x10000};   // 64k
	//char * m_pReadBuffer;
	//DWORD m_BufFileOffset;
	//ORD m_CurrFileOffset;
	//DWORD m_SectorSize;
	DWORD m_dwSize;
	MMCKINFO m_riffck;  // RIFF chunk info
	CSimpleCriticalSection m_cs;
private:
#if 0
	size_t BufferedRead(void * pBuf, size_t size);
	LONG FileRead(void * pBuf, size_t size);
	void SeekBufferedRead(DWORD position)
	{
		m_CurrFileOffset = position;
	}
#endif

	static LRESULT PASCAL BufferedIOProc(LPSTR lpmmioinfo, UINT wMsg,
										LPARAM lParam1, LPARAM lParam2);

};
class CWaveFile : public CMmioFile
{
public:
	CWaveFile();
	CWaveFile( LPCTSTR lpszFileName, UINT nOpenFlags );
	~CWaveFile();
#if 0
	virtual BOOL Open( LPCTSTR lpszFileName, UINT nOpenFlags);
	virtual void Close( );
#endif
	MMCKINFO m_datack;

	BOOL LoadWaveformat();
	BOOL FindData();
	unsigned SampleRate() const;
	int Channels() const;

	WAVEFORMATEX * m_pWf;
};

#endif //#ifndef WAVE_FILE__H__
