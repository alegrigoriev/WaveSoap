// DirectFile.cpp: implementation of the CDirectFile class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "DirectFile.h"
#include <process.h>

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif
#pragma function(memcpy)

inline static DWORD InterlockedOr(DWORD * dst, DWORD op2)
{
	DWORD tmp;
	do {
		tmp = *dst;
	} while (tmp != DWORD(InterlockedCompareExchange((void **) dst, (void *) (tmp | op2), (void *) tmp)));
	return tmp | op2;
}
inline static DWORD InterlockedAnd(DWORD * dst, DWORD op2)
{
	DWORD tmp;
	do {
		tmp = *dst;
	} while (tmp != DWORD(InterlockedCompareExchange((void **) dst, (void *) (tmp & op2), (void *) tmp)));
	return tmp & op2;
}
//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CDirectFile::CDirectFile()
	: m_FilePointer(0),
//m_FileLength(0),
	m_pFile(NULL)
{

}

CDirectFile::~CDirectFile()
{

}

BOOL CDirectFile::Open(LPCTSTR szName, DWORD flags)
{
	Close(0);
	// only read only is currently supported
	ASSERT(flags == OpenReadOnly);
	ASSERT(NULL != CDirectFileCache::SingleInstance);
	CDirectFileCache::File * pFile = CDirectFileCache::SingleInstance->Open(szName, flags);
	if (NULL == pFile)
	{
		return FALSE;
	}
	m_pFile = pFile;
	m_FilePointer = 0;
	return TRUE;
}

CDirectFile::CDirectFileCache::File * CDirectFile::CDirectFileCache::Open
	(LPCTSTR szName, DWORD flags)
{
	// find if the file with this name is in the list.
	// if it is, add a reference
	CString FullName;
	char * pName = FullName.GetBuffer(512);

	LPTSTR pFilePart = NULL;
	GetFullPathName(szName, 511, pName, & pFilePart);
	FullName.ReleaseBuffer();
	TRACE("Full name: %s\n", (LPCTSTR) FullName);

	BY_HANDLE_FILE_INFORMATION info;
	// open the file to query information
	HANDLE hf = CreateFile(FullName,
							0,
							FILE_SHARE_WRITE,
							NULL, // lpSecurity
							OPEN_EXISTING,
							FILE_ATTRIBUTE_NORMAL,
							NULL);
	if (hf != INVALID_HANDLE_VALUE)
	{
		if (::GetFileInformationByHandle(hf, & info))
		{
			TRACE("Volume ID: %08X, File index: %08X%08X\n",
				info.dwVolumeSerialNumber, info.nFileIndexHigh,
				info.nFileIndexLow);
			// find a File struct with the same parameters
		}
		CloseHandle(hf);
	}
	// otherwise construct a new File structure, open the file
	// and put it to the list.
	DWORD access = GENERIC_READ;
	DWORD ShareMode = FILE_SHARE_READ;
	DWORD Disposition = OPEN_EXISTING;
	if (0 == (flags & OpenReadOnly))
	{
		if (flags & OpenDirect)
		{
			ShareMode = 0;
			access = GENERIC_WRITE | GENERIC_READ;
		}
		else if (flags & CreateNew)
		{
			ShareMode = 0;
			access = GENERIC_WRITE | GENERIC_READ;
			Disposition = CREATE_NEW;
		}
	}

	File * pFile = new File(FullName);
	if (NULL == pFile)
	{
		return NULL;
	}

	hf = CreateFile(FullName,
					access,
					ShareMode,
					NULL, // lpSecurity
					Disposition,
					FILE_FLAG_NO_BUFFERING | FILE_ATTRIBUTE_NORMAL,
					NULL);
	if (INVALID_HANDLE_VALUE == hf)
	{
		delete pFile;
		return NULL;
	}
	pFile->hFile = hf;
	pFile->pPrev = NULL;
	pFile->pNext = m_pFileList;
	if (m_pFileList)
	{
		m_pFileList->pPrev = pFile;
	}
	m_pFileList = pFile;

	return pFile;
}

BOOL CDirectFile::CDirectFileCache::Close(File * pFile, DWORD flags)
{
	// dereference File structure.
	if (InterlockedDecrement( & pFile->RefCount) > 0)
	{
		return TRUE;
	}
	// If the use count is 0, flush all the buffers,
	{
		CSimpleCriticalSectionLock lock(pFile->m_ListLock);
		while (NULL != pFile->BuffersList)
		{
			BufferHeader * pBuf = pFile->BuffersList;
			ASSERT(0 == pBuf->LockCount);
			// something strange: buffer not released
			if (pBuf->LockCount != 0)
			{
				return FALSE;
			}
			if (pBuf->DirtyMask)
			{
				FlushDirtyBuffers(pBuf);
			}
			ASSERT(pBuf == pFile->BuffersList);
			ASSERT(0 == pBuf->DirtyMask);
			pFile->BuffersList = pBuf->pNext;
			if (NULL != pFile->BuffersList)
			{
				ASSERT(pFile->BuffersList->pPrev == pBuf);
				pFile->BuffersList->pPrev = NULL;
			}
			CSimpleCriticalSectionLock lock1(m_cs);
			pBuf->pNext = m_pFreeBuffers;
			if (NULL != m_pFreeBuffers)
			{
				m_pFreeBuffers->pPrev = pBuf;
			}
			pBuf->pPrev = NULL;
			m_pFreeBuffers = pBuf;
			pBuf->pFile = NULL;
		}
	}
	// close the handle and delete the structure
	CSimpleCriticalSectionLock lock(m_cs);
	// remove the structure from the list
	if (NULL != pFile->pNext)
	{
		pFile->pNext->pPrev = pFile->pPrev;
	}
	if (NULL != pFile->pPrev)
	{
		pFile->pPrev->pNext = pFile->pNext;
	}
	else
	{
		m_pFileList = pFile->pNext;
	}
	CloseHandle(pFile->hFile);
	delete pFile;
	return TRUE;
}

BOOL CDirectFile::Close(DWORD flags)
{
	if (NULL == m_pFile)
	{
		return FALSE;
	}
	ASSERT(NULL != CDirectFileCache::SingleInstance);
	CDirectFileCache::SingleInstance->Close(m_pFile, flags);
	m_pFile = NULL;
	return TRUE;
}

// read data ('count' bytes) from the current position to *buf
long CDirectFile::Read(void *buf, long count)
{
	long TotalRead = 0;
	char * pDstBuf = (char *)buf;
	while(count > 0)
	{
		void * pSrcBuf;
		long nRead = GetDataBuffer( & pSrcBuf, count, m_FilePointer, 0);
		if (0 == nRead)
		{
			break;
		}
		memmove(pDstBuf, pSrcBuf, nRead);
		ReturnDataBuffer(pSrcBuf, nRead, 0);
		TotalRead += nRead;
		pDstBuf += nRead;
		m_FilePointer += nRead;
		count -= nRead;
	}
	return TotalRead;
}

// read data ('count' bytes) from the given 'position' to *buf
long CDirectFile::ReadAt(void *buf, long count, LONGLONG Position)
{
	TRACE("ReadAt 0x%x, 0x%x bytes..", long(Position), count);
	long TotalRead = 0;
	char * pDstBuf = (char *)buf;
	while(count > 0)
	{
		void * pSrcBuf;
		long nRead = GetDataBuffer( & pSrcBuf, count, Position, GetBufferAndPrefetchNext);
		if (0 == nRead)
		{
			break;
		}
		memmove(pDstBuf, pSrcBuf, nRead);
		ReturnDataBuffer(pSrcBuf, nRead, 0);
		Position += nRead;
		TotalRead += nRead;
		pDstBuf += nRead;
		count -= nRead;
	}
	TRACE("return\n");
	return TotalRead;
}

// write data ('count' bytes) from the current position to *buf
long CDirectFile::Write(const void *pBuf, long count)
{
	long TotalWritten = 0;
	const char * pSrcBuf = (const char *)pBuf;
	while(count > 0)
	{
		void * pDstBuf;
		long nWritten = GetDataBuffer( & pDstBuf, count, m_FilePointer, GetBufferWriteOnly);
		if (0 == nWritten)
		{
			break;
		}
		memmove(pDstBuf, pSrcBuf, nWritten);
		ReturnDataBuffer(pDstBuf, nWritten, ReturnBufferDirty);
		TotalWritten += nWritten;
		pSrcBuf += nWritten;
		m_FilePointer += nWritten;
		count -= nWritten;
	}
	return TotalWritten;
}

CDirectFile::CDirectFileCache *
	CDirectFile::CDirectFileCache::SingleInstance = NULL;

CDirectFile::CDirectFileCache::CDirectFileCache(size_t CacheSize)
	: m_hThread(NULL),
	m_hEvent(NULL),
	m_pHeaders(NULL),
	m_pBuffersArray(NULL),
	m_pFileList(NULL),
	m_MRU_Count(1),
	m_Flags(0),
	m_NumberOfBuffers(0),
	m_bRunThread(FALSE),
	m_pPrefetchFile(0),
	m_PrefetchPosition(0),
	m_PrefetchLength(0),
	m_MinPrefetchMRU(0),
	m_pFreeBuffers(NULL)
{
	if (SingleInstance != NULL)
	{
		return;
	}
	m_hEvent = CreateEvent(NULL, NULL, FALSE, NULL);
	// round up to 64K
	if (0 == CacheSize)
	{
		MEMORYSTATUS st;
		GlobalMemoryStatus( & st);
		CacheSize = st.dwTotalPhys / 16;
	}
	TRACE("Direct file CacheSize = %d MB\n", CacheSize / 0x100000);
	CacheSize = (CacheSize + 0xFFFF) & ~0xFFFF;
	m_pBuffersArray = VirtualAlloc(NULL, CacheSize, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
	m_NumberOfBuffers = CacheSize / 0x10000;
	m_pHeaders = new BufferHeader[m_NumberOfBuffers];
	BufferHeader * Next = NULL;
	for (int i = 0; i < m_NumberOfBuffers; i++)
	{
		m_pHeaders[i].pPrev = NULL;
		m_pHeaders[i].pNext = Next;
		if (Next)
		{
			Next->pPrev = & m_pHeaders[i];
		}
		Next = & m_pHeaders[i];
		m_pHeaders[i].DirtyMask = 0;
		m_pHeaders[i].ReadMask = 0;
		m_pHeaders[i].Flags = 0;
		m_pHeaders[i].LockCount = 0;
		m_pHeaders[i].MRU_Count = 0;
		m_pHeaders[i].pBuf = i * 0x10000 + (char *) m_pBuffersArray;
		m_pHeaders[i].pFile = NULL;
		m_pHeaders[i].Position = 0;
	}
	m_pFreeBuffers = Next;  // address of the last descriptor

	unsigned ThreadID = NULL;
	m_bRunThread = TRUE;
	m_hThread = (HANDLE) _beginthreadex(NULL, 0x10000, ThreadProc,
										this, CREATE_SUSPENDED, & ThreadID);
	if (m_hThread != NULL)
	{
		if (m_hEvent != NULL
			&& m_pBuffersArray != NULL
			&& m_pHeaders != NULL)
		{
			SingleInstance = this;
			ResumeThread(m_hThread);
		}
		else
		{
			TerminateThread(m_hThread, 0);
			CloseHandle(m_hThread);
			m_hThread = NULL;
		}
	}
}

CDirectFile::CDirectFileCache::~CDirectFileCache()
{
	// stop the thread
	if (NULL != m_hThread)
	{
		m_bRunThread = FALSE;
		SetEvent(m_hEvent);
		if (WAIT_TIMEOUT == WaitForSingleObject(m_hThread, 5000))
		{
			TerminateThread(m_hThread, 0);
		}
		CloseHandle(m_hThread);
		m_hThread = NULL;
	}

	// close the files
	// free the buffers
	if (NULL != m_hEvent)
	{
		CloseHandle(m_hEvent);
		m_hEvent = NULL;
	}
	if (NULL != m_pBuffersArray)
	{
		VirtualFree(m_pBuffersArray, 0, MEM_DECOMMIT | MEM_RELEASE);
		m_pBuffersArray = NULL;
		m_NumberOfBuffers = 0;
	}
	if (NULL != m_pHeaders)
	{
		delete[] m_pHeaders;
		m_pHeaders = NULL;
	}

	if (this == SingleInstance)
	{
		SingleInstance = NULL;
	}
}

long CDirectFile::CDirectFileCache::GetDataBuffer(CDirectFile * pDirectFile,
	void * * ppBuf, LONGLONG length, LONGLONG position, DWORD flags, unsigned MaxMRU)
{
	File * pFile = pDirectFile->m_pFile;
	ASSERT(pFile);
	if (NULL == pFile)
	{
		return 0;
	}
	long BytesRequested = 0;
	int OffsetInBuffer = 0;

	LONGLONG BufferPosition = 0;
	LONGLONG PrefetchPosition = 0;
	if (length > 0)
	{
		BufferPosition = position & ~(__int64)0xFFFF;
		OffsetInBuffer = int(position) & 0xFFFF;
		if (OffsetInBuffer + length > 0x10000)
		{
			BytesRequested = 0x10000 - OffsetInBuffer;
		}
		else
		{
			BytesRequested = long(length);
		}
	}
	else if (length < 0)
	{
		OffsetInBuffer = 0x10000 - ((-long(position)) & 0xFFFF);
		LONGLONG tmp = ((position + 0x7FF) & ~(__int64)0x7FF) - (position + length);
		if (OffsetInBuffer + length < 0)    // length < 0
		{
			BytesRequested = long(position) & 0xFFFF;
		}
		else
		{
			BytesRequested = -long(length);
		}
		BufferPosition = (position - BytesRequested) & ~(__int64)0xFFFF;
		OffsetInBuffer = (long(position) - BytesRequested) & 0xFFFF;
	}
	else
	{
		*ppBuf = NULL;
		return 0;
	}

	DWORD RequestedMask = (0xFFFFFFFFu << (OffsetInBuffer >> 11));
	if (OffsetInBuffer + BytesRequested <= 0xF800)
	{
		RequestedMask &= ~(0xFFFFFFFFu << ((OffsetInBuffer + BytesRequested + 0x7FF) >> 11));
	}
	DWORD MaskToRead = RequestedMask;
	if (flags & CDirectFile::GetBufferWriteOnly)
	{
		MaskToRead = ~RequestedMask;
		if (OffsetInBuffer & 0x7FF)
		{
			MaskToRead |= (1L << (OffsetInBuffer >> 11));
		}
		if ((OffsetInBuffer + BytesRequested) & 0x7FF)
		{
			MaskToRead |= (1L << ((OffsetInBuffer + BytesRequested) >> 11));
		}
	}

	BufferHeader * pBuf = NULL;
	BufferHeader * pFreeBuf = NULL;

	{
		CSimpleCriticalSectionLock lock(CDirectFileCache::SingleInstance->m_cs);
		m_MRU_Count++;
		if (0 == m_MRU_Count)
		{
			// change it to 0x80000000, subtract 0x80000000 from all counters
			for (int i = 0; i < m_NumberOfBuffers; i++)
			{
				if (m_pHeaders[i].MRU_Count > 0x80000000u)
				{
					m_pHeaders[i].MRU_Count -= 0x80000000u;
				}
				else
				{
					m_pHeaders[i].MRU_Count = 0;
				}
			}
			m_MRU_Count = 0x80000000u;
		}
	}
	{
		CSimpleCriticalSectionLock lock(pFile->m_ListLock);
		pBuf = pFile->BuffersList;
		// find if the buffer is already available
		while (NULL != pBuf)
		{
			ASSERT(pBuf->pFile == pFile);
			if (pBuf->Position == BufferPosition)
			{
				InterlockedIncrement( & pBuf->LockCount);
				pBuf->MRU_Count = m_MRU_Count;
				break;
			}
			pBuf = pBuf->pNext;
		}
	}
	if (NULL == pBuf)
	{
		pFreeBuf = GetFreeBuffer(MaxMRU);
		CSimpleCriticalSectionLock lock(pFile->m_ListLock);
		// make sure the buffer with the same position was not added in the meanwhile
		pBuf = pFile->BuffersList;
		while (NULL != pBuf)
		{
			ASSERT(pBuf->pFile == pFile);
			if (pBuf->Position == BufferPosition)
			{
				InterlockedIncrement( & pBuf->LockCount);
				pBuf->MRU_Count = m_MRU_Count;
				break;
			}
			pBuf = pBuf->pNext;
		}
		if (NULL == pBuf)
		{
			pBuf = pFreeBuf;
			pFreeBuf = NULL;
			pBuf->pPrev = NULL;
			pBuf->pNext = pFile->BuffersList;
			if (NULL != pBuf->pNext)
			{
				pBuf->pNext->pPrev = pBuf;
			}

			pFile->BuffersList = pBuf;
			pBuf->LockCount = 1;
			pBuf->MRU_Count = m_MRU_Count;
			pBuf->ReadMask = 0;
			pBuf->DirtyMask = 0;
			pBuf->pFile = pFile;
			pBuf->Position = BufferPosition;
		}
	}
	// since we already have the required buffer,
	// return the free buffer back
	if (NULL != pFreeBuf)
	{
		CSimpleCriticalSectionLock lock(m_cs);
		pFreeBuf->pNext = m_pFreeBuffers;
		pFreeBuf->pPrev = NULL;
		if (NULL != pFreeBuf->pNext)
		{
			pFreeBuf->pNext->pPrev = pFreeBuf;
		}
		m_pFreeBuffers = pFreeBuf;
	}

	if (MaskToRead != (pBuf->ReadMask & MaskToRead))
	{
		ReadDataBuffer(pBuf, MaskToRead);
	}
	// start prefetch for the rest of the data, if GetBufferWriteOnly is not set
	// return number of bytes available

	if (length >= 0)
	{
		*ppBuf = OffsetInBuffer + (char *)pBuf->pBuf;
		if (0 == (flags & (GetBufferNoPrefetch | GetBufferWriteOnly))
			&& (BytesRequested < length || (flags & GetBufferAndPrefetchNext)))
		{
			if ((flags & GetBufferAndPrefetchNext)
				&& length < BytesRequested)
			{
				// prefetch the next buffer
				length = (position & ~(LONGLONG) 0xFFFF) + 0x20000 - position;
			}
			{
				CSimpleCriticalSectionLock lock(CDirectFileCache::SingleInstance->m_cs);
				CDirectFileCache::SingleInstance->m_pPrefetchFile = pDirectFile;
				CDirectFileCache::SingleInstance->m_PrefetchPosition = position + BytesRequested;
				CDirectFileCache::SingleInstance->m_PrefetchLength = length - BytesRequested;
				CDirectFileCache::SingleInstance->m_MinPrefetchMRU = m_MRU_Count;
			}
			SetEvent(CDirectFileCache::SingleInstance->m_hEvent);
		}
		return BytesRequested;
	}
	else
	{
		* ppBuf = OffsetInBuffer + BytesRequested + (char *)pBuf->pBuf;
		if (0 == (flags & (GetBufferNoPrefetch | GetBufferWriteOnly))
			&& (BytesRequested < -length || (flags & GetBufferAndPrefetchNext)))
		{
			if ((flags & GetBufferAndPrefetchNext)
				&& -length < BytesRequested)
			{
				// prefetch the next buffer
				length = (position | 0xFFFF) + 1 - 0x20000 - position;
			}
			{
				CSimpleCriticalSectionLock lock(CDirectFileCache::SingleInstance->m_cs);
				CDirectFileCache::SingleInstance->m_pPrefetchFile = pDirectFile;
				CDirectFileCache::SingleInstance->m_PrefetchPosition = position - BytesRequested;
				CDirectFileCache::SingleInstance->m_PrefetchLength = length + BytesRequested;
				CDirectFileCache::SingleInstance->m_MinPrefetchMRU = m_MRU_Count;
			}
			SetEvent(CDirectFileCache::SingleInstance->m_hEvent);
		}
		return -BytesRequested;
	}
}

void CDirectFile::CDirectFileCache::ReturnDataBuffer(CDirectFile * pDirectFile,
													void * pBuffer, long count, DWORD flags)
{
	if (NULL == pBuffer || 0 == count)
	{
		return;
	}
	File * pFile = pDirectFile->m_pFile;
	// find the buffer descriptor
	DWORD dwBuffer = DWORD(pBuffer);
	if (count < 0)
	{
		dwBuffer += count;
		count = -count;
	}
	int BufferNumber = (dwBuffer - DWORD(m_pBuffersArray)) / 0x10000;
	BufferHeader * pBuf = & m_pHeaders[BufferNumber];
	ASSERT(BufferNumber >= 0 && BufferNumber < m_NumberOfBuffers);
	ASSERT(pBuf->pFile == pFile);
	ASSERT(pBuf->LockCount > 0);
	ASSERT((DWORD)pBuf->pBuf == (dwBuffer & ~0xFFFF));
	// mark the buffer dirty, if necessary
	int OffsetInBuffer = dwBuffer & 0xFFFF;

	ASSERT(OffsetInBuffer + count <= 0x10000);

	{
		CSimpleCriticalSectionLock lock(pFile->m_ListLock);
		if (flags & CDirectFile::ReturnBufferDirty)
		{
			DWORD RequestedMask = (0xFFFFFFFFu << (OffsetInBuffer >> 11));
			if (OffsetInBuffer + count <= 0xF800)
			{
				RequestedMask &= ~(0xFFFFFFFFu << ((OffsetInBuffer + count + 0x7FF) >> 11));
			}
			InterlockedOr( & pBuf->DirtyMask, RequestedMask);
			InterlockedOr( & pBuf->ReadMask, RequestedMask);
		}
		if (flags & CDirectFile::ReturnBufferDiscard)
		{
			// reduce buffer priority
			if (pBuf->MRU_Count > 0x100)
			{
				pBuf->MRU_Count -= 0x100;
			}
			else
			{
				pBuf->MRU_Count = 0;
			}
		}
	}
	// decrement lock count
	InterlockedDecrement( & pBuf->LockCount);
	// set a request for writing
	if (flags & CDirectFile::ReturnBufferDirty)
	{
	}
}

CDirectFile::CDirectFileCache::BufferHeader
	* CDirectFile::CDirectFileCache::GetFreeBuffer(unsigned MaxMRU)
{
	while (1) {
		CSimpleCriticalSectionLock lock(m_cs);
		BufferHeader * pBuf = NULL;
		// try to find an empty buffer
		if (NULL != m_pFreeBuffers)
		{
			pBuf = m_pFreeBuffers;
			m_pFreeBuffers = pBuf->pNext;
			if (m_pFreeBuffers != NULL)
			{
				m_pFreeBuffers->pPrev = NULL;
			}
			pBuf->ReadMask = 0;
			pBuf->DirtyMask = 0;
			return pBuf;
		}
		// find an unlocked buffer with oldest MRU stamp
		if (0 == MaxMRU)
		{
			MaxMRU = m_MRU_Count;
		}
		unsigned OldestMRU = MaxMRU;
		unsigned OldestDirtyMRU = MaxMRU;
		BufferHeader * pDirtyBuf = NULL;
		for (int i = 0; i < m_NumberOfBuffers; i++)
		{
			if (0 == m_pHeaders[i].LockCount)
			{
				if (0 != m_pHeaders[i].DirtyMask)
				{
					if (m_pHeaders[i].MRU_Count < OldestDirtyMRU)
					{
						OldestDirtyMRU = m_pHeaders[i].MRU_Count;
						pDirtyBuf = & m_pHeaders[i];
					}
				}
				else
				{
					if (m_pHeaders[i].MRU_Count < OldestMRU)
					{
						OldestMRU = m_pHeaders[i].MRU_Count;
						pBuf = & m_pHeaders[i];
					}
				}
			}
		}
		if (pBuf != NULL)
		{
			File * pFile = pBuf->pFile;
			CSimpleCriticalSectionLock(pFile->m_ListLock);
			// check if the buffer changed
			if (0 != pBuf->LockCount
				|| 0 != pBuf->DirtyMask
				|| pBuf->MRU_Count != OldestMRU)
			{
				continue;   // try again
			}
			// get the buffer from the list
			if (pBuf->pNext)
			{
				pBuf->pNext->pPrev = pBuf->pPrev;
			}
			if (pBuf->pPrev)
			{
				pBuf->pPrev->pNext = pBuf->pNext;
			}
			else
			{
				ASSERT(pFile->BuffersList == pBuf);
				pFile->BuffersList = pBuf->pNext;
			}
			pBuf->ReadMask = 0;
			pBuf->DirtyMask = 0;
			return pBuf;
		}
		if (pDirtyBuf != NULL)
		{
			// only "dirty" buffer available
			FlushDirtyBuffers(pBuf);
			continue;
		}
	}
	return NULL;
}

// read the data by 32-bit MaskToRead (each bit corresponds to 2K)
// to the buffer defined by BufferHeader *pBuf
void CDirectFile::CDirectFileCache::ReadDataBuffer(BufferHeader * pBuf, DWORD MaskToRead)
{
	// check if the required data is already in the buffer
	if ((pBuf->ReadMask & MaskToRead) == MaskToRead)
	{
		return;
	}
	CSimpleCriticalSectionLock lock(pBuf->pFile->m_FileLock);
	MaskToRead &= ~ pBuf->ReadMask;
	// the operation in progress might change the mask
	if (0 == MaskToRead)
	{
		return;
	}
	// read the required data
	File * pFile = pBuf->pFile;
	LONGLONG StartFilePtr = pBuf->Position;
	char * buf = (char *) pBuf->pBuf;
	DWORD mask = MaskToRead;
	while (mask != 0)
	{
		size_t ToRead;
		if (mask == 0xFFFFFFFF)
		{
			mask = 0;
			ToRead = 0x10000;
		}
		else
		{
			while (0 == (mask & 1))
			{
				mask >>= 1;
				StartFilePtr += 0x800;
				buf += 0x800;
			}
			ToRead = 0;
			while (mask & 1)
			{
				mask >>= 1;
				ToRead += 0x800;
			}
		}
		if (StartFilePtr != pFile->FilePointer)
		{
			LONG FilePtrH = LONG(StartFilePtr >> 32);
			SetFilePointer(pFile->hFile, (LONG)StartFilePtr, & FilePtrH, FILE_BEGIN);
			pFile->FilePointer = StartFilePtr;
		}
		DWORD BytesRead = 0;
		ReadFile(pFile->hFile, buf, ToRead, & BytesRead, NULL);
		buf += ToRead;
		StartFilePtr += ToRead;
		pFile->FilePointer += ToRead;
	}
	InterlockedOr(& pBuf->ReadMask, MaskToRead);
	InterlockedAnd(& pBuf->DirtyMask, ~MaskToRead);
}

void CDirectFile::CDirectFileCache::FlushDirtyBuffers(BufferHeader * pBuf)
{
	// flush all unlocked dirty buffers in sequence with pBuf;
}

unsigned CDirectFile::CDirectFileCache::_ThreadProc()
{
	while (m_bRunThread)
	{
		do {
			LONGLONG PrefetchPosition;
			LONGLONG PrefetchLength;
			unsigned PrefetchMRU;
			CDirectFile * pPrefetchFile;
			if (NULL == m_pPrefetchFile)
			{
				break;
			}
			{
				CSimpleCriticalSectionLock lock(m_cs);
				pPrefetchFile = m_pPrefetchFile;
				PrefetchPosition = m_PrefetchPosition;
				PrefetchLength = m_PrefetchLength;
				PrefetchMRU = m_MinPrefetchMRU;
			}
			if (NULL == pPrefetchFile
				|| 0 == PrefetchLength)
			{
				break;
			}
			void * pBuf = NULL;
			long ReadLength = GetDataBuffer(pPrefetchFile, & pBuf, PrefetchLength,
											PrefetchPosition, GetBufferNoPrefetch, PrefetchMRU);
			if (pBuf)
			{
				ReturnDataBuffer(pPrefetchFile, pBuf, ReadLength, 0);
			}
			{
				CSimpleCriticalSectionLock lock(m_cs);
				if (m_pPrefetchFile != pPrefetchFile
					|| PrefetchPosition != m_PrefetchPosition
					|| PrefetchLength != m_PrefetchLength)
				{
					// prefetch assignment changed
					continue;
				}

				m_PrefetchLength -= ReadLength;
				if (0 != ReadLength
					&& 0 != m_PrefetchLength)
				{
					m_PrefetchPosition += ReadLength;
					continue;
				}
				else
				{
					m_pPrefetchFile = NULL;
					m_PrefetchLength = 0;
					break;
				}
			}
		} while(1);
		WaitForSingleObject(m_hEvent, 2000);
	}
	return 0;
}
