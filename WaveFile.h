// WaveFile.h
#ifndef WAVE_FILE__H__
#define WAVE_FILE__H__
/*
Class CWaveFile serves as a WAV file input/output
helper.
*/
#ifndef __AFX_H__
 #include <afx.h>
#endif
/*
  Base class supports:
  Ascend and descend to a chunk
  Automatic descend to RIFF chunk
  Data read/write

*/
#include <mmsystem.h>
class CMmioFile
{
public:
	// construction
	CMmioFile(HANDLE hFile = NULL);
	CMmioFile( LPCTSTR lpszFileName, UINT nOpenFlags );
	virtual BOOL Open( LPCTSTR lpszFileName, UINT nOpenFlags);
	virtual void Close( );

	// read/write:
	LONG Read( void* lpBuf, LONG nCount )
	{
		return mmioRead(m_hmmio, (HPSTR) lpBuf, nCount);
	}

	LONG Write( const void* lpBuf, LONG nCount )
	{
		return mmioWrite(m_hmmio, (const char*)lpBuf, nCount);
	}
	void Flush(UINT flag = 0)   // possible value: MMIO_EMPTYBUF
	{
		mmioFlush(m_hmmio, 0);
	}

	MMRESULT Ascend(LPMMCKINFO lpck)
	{
		return mmioAscend(m_hmmio, lpck, 0);
	}

	MMRESULT Descend(LPMMCKINFO lpck,LPMMCKINFO lpckParent, UINT uFlags = 0)
	{
		return mmioDescend(m_hmmio, lpck, lpckParent, uFlags);
	}

	MMRESULT FindChunk(LPMMCKINFO lpck,LPMMCKINFO lpckParent)
	{
		return Descend(lpck, lpckParent, MMIO_FINDCHUNK);
	}
	MMRESULT FindList(LPMMCKINFO lpck,LPMMCKINFO lpckParent)
	{
		return Descend(lpck, lpckParent, MMIO_FINDLIST);
	}
	MMRESULT FindRiff(LPMMCKINFO lpck,LPMMCKINFO lpckParent)
	{
		return Descend(lpck, lpckParent, MMIO_FINDRIFF);
	}

	MMRESULT Advance(LPMMIOINFO mmio, UINT uFlags)
	{
		return mmioAdvance(m_hmmio, mmio, uFlags);
	}

	MMRESULT CreateChunk(LPMMCKINFO lpck, UINT wFlags)
	{
		lpck->cksize = 0;
		return mmioCreateChunk(m_hmmio, lpck, wFlags);
	}
	MMRESULT CreateList(LPMMCKINFO lpck)
	{
		return CreateChunk(lpck, MMIO_CREATELIST);
	}
	MMRESULT CreateRiff(LPMMCKINFO lpck)
	{
		return CreateChunk(lpck, MMIO_CREATERIFF);
	}

	MMRESULT GetInfo(LPMMIOINFO lpmmioinfo, UINT wFlags = 0)
	{
		return mmioGetInfo(m_hmmio, lpmmioinfo, wFlags);
	}

	virtual ~CMmioFile();

	HANDLE m_hFile;
	HMMIO m_hmmio;
	enum { ReadBufferSize = 0x10000};   // 64k
	char * m_pReadBuffer;
	DWORD m_BufFileOffset;
	DWORD m_CurrFileOffset;
	DWORD m_SectorSize;
	DWORD m_dwSize;
	MMCKINFO m_riffck;  // RIFF chunk info
private:
	size_t BufferedRead(void * pBuf, size_t size);
	void SeekBufferedRead(DWORD position)
	{
		m_CurrFileOffset = position;
	}

	static LRESULT PASCAL BufferedIOProc(LPSTR lpmmioinfo, UINT wMsg,
										LPARAM lParam1, LPARAM lParam2);

};
class CWaveFile : public CMmioFile
{
};

#endif //#ifndef WAVE_FILE__H__
