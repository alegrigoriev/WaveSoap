// WaveFile.cpp
#include <afx.h>
#include "WaveFile.h"

CMmioFile::CMmioFile(HANDLE hFile)
	: m_hFile(hFile), m_hmmio(NULL),
	m_dwSize(0),
	m_pReadBuffer(NULL)
{
}

CMmioFile::CMmioFile( LPCTSTR lpszFileName, UINT nOpenFlags )
	: m_hFile(NULL), m_hmmio(NULL),
	m_dwSize(0),
	m_pReadBuffer(NULL)
{
	Open(lpszFileName, nOpenFlags);
}

CMmioFile::~CMmioFile()
{
	Close();
	delete m_pReadBuffer;
	m_pReadBuffer = 0;
}

static DWORD GetSectorSize(LPCTSTR szFilename)
{
	CString curr_dir;
	TCHAR c;
	DWORD size = GetCurrentDirectory(0, & c);
	if (0 == size)
	{
		return 0x1000;
	}
	LPTSTR pBuf = curr_dir.GetBuffer(size);
	if (NULL == pBuf)
	{
		return 0x1000;
	}

	if (GetCurrentDirectory(size, pBuf) > size)
	{
		return 0x1000;
	}
	curr_dir.ReleaseBuffer();

	CString file_dir;
	LPTSTR pFilenamePart;
	size = GetFullPathName(szFilename, 0, &c, & pFilenamePart);
	if (0 == size)
	{
		return 0x1000;
	}

	pBuf = file_dir.GetBuffer(size);
	if (NULL == pBuf)
	{
		return 0x1000;
	}

	if (GetFullPathName(szFilename, size, pBuf, & pFilenamePart) > size)
	{
		return 0x1000;
	}

	*pFilenamePart = 0;
	file_dir.ReleaseBuffer();
	if (0 != file_dir.CompareNoCase(curr_dir))
	{
		SetCurrentDirectory(file_dir);
	}

	DWORD sector_size = 0;
	DWORD sectors_per_cluster;
	DWORD free_clusters;
	DWORD total_clusters;
	if (! GetDiskFreeSpace(NULL, & sectors_per_cluster,
							& sector_size, & free_clusters, & total_clusters)
		|| 0 == sector_size)
	{
		sector_size = 0x1000;
	}

	if (0 != file_dir.CompareNoCase(curr_dir))
	{
		SetCurrentDirectory(curr_dir);
	}
	return sector_size;
}

BOOL CMmioFile::Open( LPCTSTR szFileName, UINT nOpenFlags)
{
	Close();

	m_hFile = CreateFile(szFileName, GENERIC_READ, FILE_SHARE_READ,
						NULL, OPEN_EXISTING, FILE_FLAG_NO_BUFFERING | FILE_FLAG_SEQUENTIAL_SCAN, NULL);

	m_BufFileOffset = 0xFFFFFFFF;
	m_SectorSize = GetSectorSize(szFileName);

	if(INVALID_HANDLE_VALUE == m_hFile)
	{
		m_hFile = NULL;
		return FALSE;
	}

	DWORD dwSizeHigh = 0;
	m_dwSize = GetFileSize(m_hFile, & dwSizeHigh);

	if (dwSizeHigh != 0 ||
		(0xFFFFFFFF == m_dwSize && GetLastError() != NO_ERROR))
	{
		Close();
		return FALSE;
	}

	MMIOINFO mmii;
	memset( & mmii, 0, sizeof mmii);
	mmii.fccIOProc = 0;
	mmii.pIOProc = BufferedIOProc;

	mmii.adwInfo[0] = (DWORD)this;

	m_hmmio = mmioOpen(NULL, & mmii,
						MMIO_READ | MMIO_ALLOCBUF);

	if (NULL == m_hmmio)
	{
		Close();
		return FALSE;
	}

	return TRUE;
}


LRESULT PASCAL CMmioFile::BufferedIOProc(LPSTR lpmmioinfo, UINT wMsg,
										LPARAM lParam1, LPARAM lParam2)
{
	LPMMIOINFO pmmi = (LPMMIOINFO) lpmmioinfo;
	CMmioFile * pFile = (CMmioFile *) pmmi->adwInfo[0];

	switch(wMsg)
	{
	case MMIOM_OPEN:
		return MMSYSERR_NOERROR;
		break;
	case MMIOM_CLOSE:
		return MMSYSERR_NOERROR;
		break;
	case MMIOM_READ:
	{
		pFile->SeekBufferedRead(pmmi->lDiskOffset);
		DWORD cbRead = pFile->BufferedRead((LPVOID) lParam1, lParam2);
		if (-1 == cbRead)
			return -1;
		pmmi->lDiskOffset += cbRead;
		return cbRead;
	}
		break;
	case MMIOM_WRITE:
		return -1;
		break;
	case MMIOM_SEEK:
		switch (lParam2)
		{
		case SEEK_CUR:
			pmmi->lDiskOffset += lParam1;
			return pmmi->lDiskOffset;
			break;
		case SEEK_END:
			pmmi->lDiskOffset = pFile->m_dwSize + lParam1;
			return pmmi->lDiskOffset;
			break;
		case SEEK_SET:
			pmmi->lDiskOffset = lParam1;
			return pmmi->lDiskOffset;
			break;
		default:
			return -1;
		}
		break;
	default:
		return -1;
	}
}

size_t CMmioFile::BufferedRead(void * pBuf, size_t size)
{
	char * buf = (char *) pBuf;
	size_t size_read = 0;
	if (NULL == m_pReadBuffer)
	{
		m_pReadBuffer = (char *) VirtualAlloc(NULL, ReadBufferSize, MEM_COMMIT, PAGE_READWRITE);
		if (NULL == m_pReadBuffer)
			return -1;
		m_BufFileOffset = 0xFFFFFFFF;
	}

	while (size != 0)
	{
		if (m_CurrFileOffset < m_BufFileOffset
			|| m_CurrFileOffset >= m_BufFileOffset + ReadBufferSize)
		{
			m_BufFileOffset = m_CurrFileOffset & -(int)m_SectorSize;
			DWORD cbRead = 0;
			if (0xFFFFFFFF == SetFilePointer(m_hFile, m_BufFileOffset, NULL, FILE_BEGIN)
				|| -1 == (cbRead = Read(m_pReadBuffer, ReadBufferSize)))
				return -1;
			if (0 == cbRead)
			{
				break;
			}
		}

		char * src = m_pReadBuffer + (m_CurrFileOffset - m_BufFileOffset);
		size_t BytesToCopy = ReadBufferSize - (m_CurrFileOffset - m_BufFileOffset);

		if (BytesToCopy > size) BytesToCopy = size;

		memcpy(buf, src, BytesToCopy);

		m_CurrFileOffset += BytesToCopy;
		buf += BytesToCopy;
		size -= BytesToCopy;
		size_read += BytesToCopy;
	}
	return size_read;
}

void CMmioFile::Close( )
{
	if (m_hmmio != NULL)
	{
		mmioClose(m_hmmio, 0);
		m_hmmio = NULL;
	}
	if (m_hFile != NULL)
	{
		CloseHandle(m_hFile);
		m_hFile = NULL;
	}
}


//DWORD CAviFile::Seek(DWORD position)
	//{
	//return SetFilePointer(m_hFile, position, NULL, FILE_BEGIN);
	//}
