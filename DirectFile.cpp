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
	ASSERT(NULL != CDirectFileCache::GetInstance());
	File * pFile = CDirectFileCache::GetInstance()->Open(szName, flags);
	if (NULL == pFile)
	{
		return FALSE;
	}
	m_pFile = pFile;
	m_FilePointer = 0;
	return TRUE;
}

BOOL CDirectFile::Attach(CDirectFile * const pOriginalFile)
{
	if (NULL == pOriginalFile->m_pFile)
	{
		return FALSE;
	}

	Close(0);
	pOriginalFile->m_pFile->RefCount++;
	m_pFile = pOriginalFile->m_pFile;
	m_FilePointer = 0;
	return TRUE;
}

CDirectFile::File * CDirectFile::CDirectFileCache::Open
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

	HANDLE hf;
	BY_HANDLE_FILE_INFORMATION info;
	// open the file to query information
	hf = CreateFile(FullName,
					0,
					FILE_SHARE_WRITE | FILE_SHARE_READ,
					NULL, // lpSecurity
					OPEN_EXISTING,
					FILE_ATTRIBUTE_NORMAL,
					NULL);
	if (hf != INVALID_HANDLE_VALUE)
	{
		if (FILE_TYPE_DISK != GetFileType(hf))
		{
			CloseHandle(hf);
			SetLastError(ERROR_INVALID_FUNCTION);
			return NULL;
		}
		if (::GetFileInformationByHandle(hf, & info))
		{
			CloseHandle(hf);
			TRACE("Volume ID: %08X, File index: %08X%08X\n",
				info.dwVolumeSerialNumber, info.nFileIndexHigh,
				info.nFileIndexLow);
			// find a File struct with the same parameters
			File * pFile = m_pFileList;
			while (NULL != pFile)
			{
				if (info.nFileIndexHigh == pFile->m_FileInfo.nFileIndexHigh
					&& info.nFileIndexLow == pFile->m_FileInfo.nFileIndexLow)
				{
					TRACE("File is in the list\n");
					// file can be reopened only for reading
					if (0 == (flags & OpenReadOnly))
					{
						SetLastError(ERROR_SHARING_VIOLATION);
					}

					SetLastError(ERROR_ALREADY_EXISTS);
					pFile->RefCount++;
					return pFile;
				}
				pFile = pFile->pNext;
			}
		}
		else
		{
			CloseHandle(hf);
		}
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
		else if (flags & CreateAlways)
		{
			ShareMode = 0;
			access = GENERIC_WRITE | GENERIC_READ;
			Disposition = CREATE_ALWAYS;
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
		SetLastError(ERROR_NOT_ENOUGH_MEMORY);
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

	if (FILE_TYPE_DISK != GetFileType(hf))
	{
		SetLastError(ERROR_INVALID_FUNCTION);
		CloseHandle(hf);
		delete pFile;
		return NULL;
	}

	pFile->hFile = hf;
	::GetFileInformationByHandle(hf, & pFile->m_FileInfo);
	pFile->FileLength = pFile->m_FileInfo.nFileSizeLow
						| (ULONGLONG(pFile->m_FileInfo.nFileSizeHigh << 32));
	pFile->RealFileLength = pFile->FileLength;

	int WrittenMaskLength = 512 + int((pFile->FileLength + 0x7FFFF) >> 19);
	pFile->m_pWrittenMask = new char[WrittenMaskLength];
	if (NULL == pFile->m_pWrittenMask)
	{
		SetLastError(ERROR_NOT_ENOUGH_MEMORY);
		CloseHandle(hf);
		delete pFile;
		return NULL;
	}

	pFile->WrittenMaskSize = WrittenMaskLength;
	memset(pFile->m_pWrittenMask, 0, WrittenMaskLength);
	memset(pFile->m_pWrittenMask, 0xFF, pFile->FileLength >> 19);
	pFile->m_pWrittenMask[pFile->FileLength >> 19] =
		0xFF >> (8 - ((pFile->FileLength >> 16) & 7));

	pFile->pPrev = NULL;
	pFile->pNext = m_pFileList;
	if (m_pFileList)
	{
		m_pFileList->pPrev = pFile;
	}
	m_pFileList = pFile;
	if ((flags & OpenDeleteAfterClose)
		&& (flags & (CreateNew | CreateAlways)))
	{
		pFile->Flags |= FileFlagsDeleteAfterClose;
	}
	if (flags & OpenReadOnly)
	{
		pFile->Flags |= FileFlagsReadOnly;
	}
	SetLastError(ERROR_SUCCESS);
	return pFile;
}

BOOL CDirectFile::File::Close(DWORD flags)
{
	// dereference File structure.
	if (InterlockedDecrement( & RefCount) > 0)
	{
		return TRUE;
	}
	// If the use count is 0, copy all remaining data
	// from the source file or init the rest and flush all the buffers,
	if (0 == (Flags & FileFlagsDeleteAfterClose))
	{
		InitializeTheRestOfFile();
	}
	CDirectFileCache * pCache = GetCache();
	{
		CSimpleCriticalSectionLock lock(m_ListLock);
		while (NULL != BuffersListHead)
		{
			BufferHeader * pBuf = BuffersListHead;
			ASSERT(0 == pBuf->LockCount);
			// something strange: buffer not released
			if (pBuf->LockCount != 0)
			{
				return FALSE;
			}
			if (pBuf->DirtyMask)
			{
				pBuf->FlushDirtyBuffers();
			}
			ASSERT(pBuf == BuffersListHead);
			ASSERT(0 == pBuf->DirtyMask);

			BuffersCount--;
			BuffersListHead = pBuf->pNext;
			if (NULL != BuffersListHead)
			{
				ASSERT(BuffersListHead->pPrev == pBuf);
				BuffersListHead->pPrev = NULL;
			}
			else
			{
				BuffersListTail = NULL;
			}
			CSimpleCriticalSectionLock lock1(pCache->m_cs);
			pBuf->pNext = pCache->m_pFreeBuffers;
			if (NULL != pCache->m_pFreeBuffers)
			{
				pCache->m_pFreeBuffers->pPrev = pBuf;
			}
			pBuf->pPrev = NULL;
			pCache->m_pFreeBuffers = pBuf;
			pBuf->pFile = NULL;
			// remove the buffer also from MRU list
			if (pBuf->pMruPrev)
			{
				pBuf->pMruPrev->pMruNext = pBuf->pMruNext;
			}
			else
			{
				pCache->m_pMruListHead = pBuf->pMruNext;
			}
			if (pBuf->pMruNext)
			{
				pBuf->pMruNext->pMruPrev = pBuf->pMruPrev;
			}
			else
			{
				pCache->m_pMruListTail = pBuf->pMruPrev;
			}
		}
	}
	// close the handle and delete the structure
	{
		CSimpleCriticalSectionLock lock(pCache->m_cs);
		// remove the structure from the list
		if (NULL != pNext)
		{
			pNext->pPrev = pPrev;
		}
		if (NULL != pPrev)
		{
			pPrev->pNext = pNext;
		}
		else
		{
			pCache->m_pFileList = pNext;
		}
	}
	if (Flags & FileFlagsDeleteAfterClose)
	{
		CloseHandle(hFile);
		DeleteFile(sName);
	}
	else
	{
		CloseHandle(hFile);
		//{
		//SetFileLength(FileLength);
		//}
		// if the file is open for writing, open it in normal mode,
		// set length, and close. When the file is open in direct mode,
		// we cannot set arbitrary file length
		if (0 == (Flags & FileFlagsReadOnly))
		{
			if (FileLength != RealFileLength)
			{
				HANDLE hf = CreateFile(sName,
										GENERIC_WRITE,
										0,
										NULL, // lpSecurity
										OPEN_EXISTING,
										FILE_ATTRIBUTE_NORMAL,
										NULL);
				long MoveHigh = FileLength >> 32;
				SetLastError(ERROR_SUCCESS);
				if (hf != INVALID_HANDLE_VALUE)
				{
					SetFilePointer(hf, long(FileLength), & MoveHigh, FILE_BEGIN);
					if (GetLastError() == ERROR_SUCCESS)
					{
						SetEndOfFile(hf);
					}
					CloseHandle(hf);
				}
			}
		}
	}
	if (NULL != m_pWrittenMask)
	{
		delete[] m_pWrittenMask;
	}

	delete this;
	return TRUE;
}

BOOL CDirectFile::Close(DWORD flags)
{
	if (NULL == m_pFile)
	{
		return FALSE;
	}
	m_pFile->Close(flags);
	m_pFile = NULL;
	return TRUE;
}

// read data ('count' bytes) from the current position to *buf
long CDirectFile::Read(void *buf, long count)
{
	long TotalRead = 0;
	char * pDstBuf = (char *)buf;
	ASSERT(m_pFile);
	if (NULL == m_pFile)
	{
		return 0;
	}

	if (m_FilePointer >= m_pFile->FileLength)
	{
		// beyond end of file
		return 0;
	}
	if (count > 0 && count + m_FilePointer > m_pFile->FileLength)
	{
		count = m_pFile->FileLength - m_FilePointer;
	}

	while(count > 0)
	{
		void * pSrcBuf;
		long nRead = GetDataBuffer( & pSrcBuf, count, m_FilePointer, GetBufferAndPrefetchNext);
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
	//TRACE("ReadAt 0x%x, 0x%x bytes..", long(Position), count);
	long TotalRead = 0;
	char * pDstBuf = (char *)buf;
	ASSERT(m_pFile);
	if (NULL == m_pFile)
	{
		return 0;
	}

	if (Position >= m_pFile->FileLength)
	{
		// beyond end of file
		return 0;
	}
	if (count > 0 && count + Position > m_pFile->FileLength)
	{
		count = m_pFile->FileLength - Position;
	}

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
	//TRACE("return\n");
	return TotalRead;
}

// write data ('count' bytes) from the current position to *buf
long CDirectFile::Write(const void *pBuf, long count)
{
	long TotalWritten = 0;
	const char * pSrcBuf = (const char *)pBuf;
	ASSERT(m_pFile);
	if (NULL == m_pFile)
	{
		return 0;
	}

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
	if (m_FilePointer > m_pFile->FileLength)
	{
		m_pFile->FileLength = m_FilePointer;
	}
	return TotalWritten;
}

// write data ('count' bytes) at the given 'position' from *buf
long CDirectFile::WriteAt(const void *buf, long count, LONGLONG Position)
{
	//TRACE("WriteAt 0x%x, 0x%x bytes..", long(Position), count);
	long TotalWritten = 0;
	char * pSrcBuf = (char *)buf;
	ASSERT(m_pFile);
	if (NULL == m_pFile)
	{
		return 0;
	}
	while(count > 0)
	{
		void * pDstBuf;
		long nWritten = GetDataBuffer( & pDstBuf, count, Position, GetBufferWriteOnly);
		if (0 == nWritten)
		{
			break;
		}
		memmove(pDstBuf, pSrcBuf, nWritten);
		ReturnDataBuffer(pDstBuf, nWritten, ReturnBufferDirty);
		Position += nWritten;
		TotalWritten += nWritten;
		pSrcBuf += nWritten;
		count -= nWritten;
	}
	if (Position > m_pFile->FileLength)
	{
		m_pFile->FileLength = Position;
	}
	//TRACE("return\n");
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
	m_pPrefetchFile(NULL),
	m_PrefetchPosition(0),
	m_PrefetchLength(0),
	m_MinPrefetchMRU(0),
	m_pMruListHead(NULL),
	m_pMruListTail(NULL),
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
		m_pHeaders[i].PositionKey = 0;
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
			SetThreadPriority(m_hThread, THREAD_PRIORITY_ABOVE_NORMAL);
			ResumeThread(m_hThread);
		}
		else
		{
			TerminateThread(m_hThread, -1);
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
			TerminateThread(m_hThread, -1);
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

	if (this == GetInstance())
	{
		SingleInstance = NULL;
	}
}

// Insert a buffer to the list with the specified key
// m_FileLock should be locked around this function call
void CDirectFile::File::InsertBuffer(BufferHeader * pBuf)
{
	unsigned long const key = pBuf->PositionKey;
	BuffersCount++;
	if (NULL == BuffersListHead)
	{
		// insert to the empty list
		pBuf->pPrev = NULL;
		pBuf->pNext = NULL;
		BuffersListHead = pBuf;
		BuffersListTail = pBuf;
		return;
	}
	else if (BuffersListHead->PositionKey > key)
	{
		// insert to the head
		pBuf->pPrev = NULL;
		pBuf->pNext = BuffersListHead;
		BuffersListHead->pPrev = pBuf;
		BuffersListHead = pBuf;
		return;
	}
	else if (BuffersListTail->PositionKey < key)
	{
		// insert to the tail
		pBuf->pNext = NULL;
		pBuf->pPrev = BuffersListTail;
		BuffersListTail->pNext = pBuf;
		BuffersListTail = pBuf;
		return;
	}
	// find which side may be closer to the key
	BufferHeader * pBufAfter;
	if (key - BuffersListHead->PositionKey > BuffersListTail->PositionKey - key)
	{
		// search from tail
		pBufAfter = BuffersListTail;
		while (pBufAfter->PositionKey > key)
		{
			pBufAfter = pBufAfter->pPrev;
			ASSERT(pBufAfter != NULL);
		}
		// pBufAfter->PositionKey < pBuf->PositionKey
	}
	else
	{
		// search from head
		pBufAfter = BuffersListHead;
		while (pBufAfter->PositionKey < key)
		{
			pBufAfter = pBufAfter->pNext;
			ASSERT(pBufAfter != NULL);
		}
		// pBufAfter->PositionKey > pBuf->PositionKey
		// return by one position
		pBufAfter = pBufAfter->pPrev;
		ASSERT(pBufAfter != NULL);
	}
	// pBufAfter->PositionKey < pBuf->PositionKey
	pBuf->pNext = pBufAfter->pNext;
	pBuf->pPrev = pBufAfter;
	pBufAfter->pNext = pBuf;
	ASSERT(pBuf->pNext != NULL);
	pBuf->pNext->pPrev = pBuf;
}

// find a buffer in the list with the specified key
// m_FileLock should be locked around this function call
CDirectFile::BufferHeader * CDirectFile::File::FindBuffer(unsigned long key) const
{
	// the buffers are ordered: BuffersListHead has lowest key, BuffersListTail has the highest key
	BufferHeader * pBuf;
#ifdef _DEBUG
	ValidateList();
#endif
	if (NULL == BuffersListHead
		|| BuffersListHead->PositionKey > key
		|| BuffersListTail->PositionKey < key)
	{
		return NULL;
	}
	// find which side may be closer to the key
	if (key - BuffersListHead->PositionKey > BuffersListTail->PositionKey - key)
	{
		// search from tail
		pBuf = BuffersListTail;
		while (pBuf != NULL && pBuf->PositionKey >= key)
		{
			if (pBuf->PositionKey == key)
			{
				return pBuf;
			}
			pBuf = pBuf->pPrev;
		}
	}
	else
	{
		// search from head
		pBuf = BuffersListHead;
		while (pBuf != NULL && pBuf->PositionKey <= key)
		{
			if (pBuf->PositionKey == key)
			{
				return pBuf;
			}
			pBuf = pBuf->pNext;
		}
	}
	return NULL;    // buffer not found
}

long CDirectFile::CDirectFileCache::GetDataBuffer(File * pFile,
	void * * ppBuf, LONGLONG length, LONGLONG position, DWORD flags, unsigned MaxMRU)
{
	ASSERT(pFile);
	ASSERT(ppBuf);
	if (NULL == pFile)
	{
		return 0;
	}
	long BytesRequested = 0;
	long BytesReturned;
	int OffsetInBuffer = 0;

	unsigned long BufferPosition;
	if (length > 0)
	{
		BufferPosition = long(position >> 16);
		OffsetInBuffer = int(position) & 0xFFFF;
		if (OffsetInBuffer + length > 0x10000)
		{
			BytesRequested = 0x10000 - OffsetInBuffer;
			BytesReturned = BytesRequested;
		}
		else
		{
			BytesRequested = long(length);
			BytesReturned = BytesRequested;
			if (flags & GetBufferAndPrefetchNext)
			{
				// read the rest of the buffer
				BytesRequested = 0x10000 - OffsetInBuffer;
			}
		}
	}
	else if (length < 0)
	{
		OffsetInBuffer = 0x10000 - ((-long(position)) & 0xFFFF);
		LONGLONG tmp = ((position + 0x7FF) & ~(__int64)0x7FF) - (position + length);
		if (OffsetInBuffer + length < 0)    // length < 0
		{
			BytesRequested = long(position) & 0xFFFF;
			BytesReturned = BytesRequested;
		}
		else
		{
			BytesRequested = -long(length);
			BytesReturned = BytesRequested;
			if (flags & GetBufferAndPrefetchNext)
			{
				// read the rest of the buffer
				BytesRequested = long(position) & 0xFFFF;
			}
		}
		BufferPosition = long((position - BytesRequested) >> 16);
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
	if (flags & GetBufferWriteOnly)
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
		if (0 == InterlockedIncrement( (PLONG) & m_MRU_Count))
		{
			CSimpleCriticalSectionLock lock(CDirectFileCache::GetInstance()->m_cs);
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
		pBuf = pFile->FindBuffer(BufferPosition);
		if (NULL != pBuf)
		{
			InterlockedIncrement( & pBuf->LockCount);
			pBuf->MRU_Count = m_MRU_Count;
		}
	}

	if (NULL == pBuf)
	{
		// the buffer is not found, get another free one
		pFreeBuf = GetFreeBuffer(MaxMRU);
		if (NULL == pFreeBuf)
		{
			// unable to find an available  buffer
			*ppBuf = NULL;
			return 0;
		}
		CSimpleCriticalSectionLock lock(pFile->m_ListLock);
		// make sure the buffer with the same position was not added
		// while we might flush the buffers
		pBuf = pFile->FindBuffer(BufferPosition);

		if (NULL != pBuf)
		{
			InterlockedIncrement( & pBuf->LockCount);
		}
		else
		{
			pBuf = pFreeBuf;
			pFreeBuf = NULL;
			pBuf->pMruPrev = NULL;
			pBuf->pMruNext = NULL;

			pBuf->LockCount = 1;
			pBuf->ReadMask = 0;
			pBuf->DirtyMask = 0;
			pBuf->pFile = pFile;
			pBuf->PositionKey = BufferPosition;
			// insert the buffer to the list for file,
			// to the correct position
			pFile->InsertBuffer(pBuf);
		}
		pBuf->MRU_Count = m_MRU_Count;
	}
	{
		// extract the buffer from MRU list
		CSimpleCriticalSectionLock lock(m_cs);
		// if they are equal (0), the buffer is not in the list
		if (pBuf->pMruPrev != pBuf->pMruNext)
		{
			if (pBuf->pMruPrev)
			{
				pBuf->pMruPrev->pMruNext = pBuf->pMruNext;
			}
			else
			{
				m_pMruListHead = pBuf->pMruNext;
			}

			if (pBuf->pMruNext)
			{
				pBuf->pMruNext->pMruPrev = pBuf->pMruPrev;
			}
			else
			{
				m_pMruListTail = pBuf->pMruPrev;
			}
		}
		// since we already have the required buffer,
		// return the free buffer back
		if (NULL != pFreeBuf)
		{
			pFreeBuf->pNext = m_pFreeBuffers;
			pFreeBuf->pPrev = NULL;
			if (NULL != pFreeBuf->pNext)
			{
				pFreeBuf->pNext->pPrev = pFreeBuf;
			}
			m_pFreeBuffers = pFreeBuf;
		}
		// insert the buffer to the list head
		pBuf->pMruNext = m_pMruListHead;
		pBuf->pMruPrev = NULL;
		if (NULL != m_pMruListHead)
		{
			m_pMruListHead->pMruPrev = pBuf;
		}
		else
		{
			// the list was empty
			m_pMruListTail = pBuf;
		}
		m_pMruListHead = pBuf;
	}

	if (MaskToRead != (pBuf->ReadMask & MaskToRead)
		|| (flags & GetBufferWriteOnly))
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
			if (flags & GetBufferAndPrefetchNext)
			{
				if (length <= BytesRequested)
				{
					// prefetch the next buffer
					length = (long(position) & ~0xFFFFL) + 0x20000 - long(position);
				}
				else
				{
					// round the length to make it prefetch the whole buffer
					length += 0xFFFF & -(long(position) + long(length));
				}
			}
			RequestPrefetch(pFile, position + BytesRequested, length - BytesRequested, m_MRU_Count);
		}
		return BytesReturned;
	}
	else
	{
		* ppBuf = OffsetInBuffer + BytesRequested + (char *)pBuf->pBuf;
		if (0 == (flags & (GetBufferNoPrefetch | GetBufferWriteOnly))
			&& (BytesRequested < -length || (flags & GetBufferAndPrefetchNext)))
		{
			if (flags & GetBufferAndPrefetchNext)
			{
				if (-length <= BytesRequested)
				{
					// prefetch the next buffer
					length = (position | 0xFFFF) + 1 - 0x20000 - position;
				}
				else
				{
					// round the length to make it prefetch the whole buffer
					length -= 0xFFFF & (long(position) - length);
				}
			}
			RequestPrefetch(pFile, position - BytesRequested, length + BytesRequested, m_MRU_Count);
		}
		return -BytesReturned;
	}
}

void CDirectFile::CDirectFileCache::RequestPrefetch(File * pFile,
													LONGLONG PrefetchPosition,
													LONGLONG PrefetchLength, unsigned MaxMRU)
{
	if (0) TRACE("RequestPrefetch: 0x%X bytes at 0x%X\n", long(PrefetchLength),
				long(PrefetchPosition));
	{
		CSimpleCriticalSectionLock lock(m_cs);
		m_pPrefetchFile = pFile;
		m_PrefetchPosition = PrefetchPosition;
		m_PrefetchLength = PrefetchLength;
		m_MinPrefetchMRU = MaxMRU;
	}
	SetEvent(m_hEvent);
}

void CDirectFile::CDirectFileCache::ReturnDataBuffer(File * pFile,
													void * pBuffer, long count, DWORD flags)
{
	if (NULL == pBuffer || 0 == count)
	{
		return;
	}
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
			DWORD OldDirtyMask = pBuf->DirtyMask;
			InterlockedOr( & pBuf->DirtyMask, RequestedMask);
			InterlockedOr( & pBuf->ReadMask, RequestedMask);
			if (0 == OldDirtyMask && 0 != RequestedMask)
			{
				InterlockedIncrement((PLONG) & pFile->DirtyBuffersCount);
			}
		}
	}
	if (flags & CDirectFile::ReturnBufferDiscard)
	{
		// move the buffer to MRU tail
		// extract the buffer from MRU list
		CSimpleCriticalSectionLock lock(m_cs);
		// if they are equal (0), the buffer is not in the list
		if (pBuf->pMruPrev != pBuf->pMruNext)
		{
			if (pBuf->pMruPrev)
			{
				pBuf->pMruPrev->pMruNext = pBuf->pMruNext;
			}
			else
			{
				m_pMruListHead = pBuf->pMruNext;
			}

			if (pBuf->pMruNext)
			{
				pBuf->pMruNext->pMruPrev = pBuf->pMruPrev;
			}
			else
			{
				m_pMruListTail = pBuf->pMruPrev;
			}
		}
		// insert the buffer to the list tail
		// to do: insert the buffer before the first with MRU_Count==1
		pBuf->MRU_Count = 1;
		BufferHeader * pBufAfter = m_pMruListTail;
		while (NULL != pBufAfter
				&& 1 == pBufAfter->MRU_Count)
		{
			pBufAfter = pBufAfter->pMruPrev;
		}

		pBuf->pMruPrev = pBufAfter;
		if (NULL != pBufAfter)
		{
			pBuf->pMruNext = pBufAfter->pMruNext;
			pBufAfter->pMruNext = pBuf;
		}
		else
		{
			pBuf->pMruNext = m_pMruListHead;
			m_pMruListHead = pBuf;
		}

		if (NULL != pBuf->pMruNext)
		{
			pBuf->pMruNext->pMruPrev = pBuf;
		}
		else
		{
			m_pMruListTail = pBuf;
		}
	}
	// decrement lock count
	InterlockedDecrement( & pBuf->LockCount);
	// set a request for writing
	if (flags & CDirectFile::ReturnBufferDirty)
	{
	}
}

CDirectFile::BufferHeader
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

		pBuf = m_pMruListTail;
		while (NULL != pBuf
				&& pBuf->MRU_Count < MaxMRU
				&& (pBuf->LockCount > 0
					|| 0 != pBuf->DirtyMask))
		{
			pBuf = pBuf->pPrev;
		}

		// unlocked buffer not found or buffer was too recent
		if (NULL == pBuf || pBuf->MRU_Count >= MaxMRU)
		{
			// try to find a dirty buffer
			pBuf = m_pMruListTail;
			while (NULL != pBuf
					&& pBuf->MRU_Count < MaxMRU
					&& pBuf->LockCount > 0)
			{
				pBuf = pBuf->pPrev;
			}
			if (pBuf != NULL
				&& 0 == pBuf->LockCount
				&& pBuf->MRU_Count < MaxMRU)
			{
				// only "dirty" buffer available
				pBuf->FlushDirtyBuffers();
				// try the loop again
				continue;
			}
			return NULL;  // unable to find a buffer
		}


		File * pFile = pBuf->pFile;
		CSimpleCriticalSectionLock lock1(pFile->m_ListLock);
		// check if the buffer changed
		if (0 != pBuf->LockCount
			|| 0 != pBuf->DirtyMask)
		{
			continue;   // try again
		}

		// get the buffer from the MRU list
		if (pBuf->pMruNext)
		{
			pBuf->pMruNext->pMruPrev = pBuf->pMruPrev;
		}
		else
		{
			m_pMruListTail = pBuf->pMruPrev;
		}

		if (pBuf->pMruPrev)
		{
			pBuf->pMruPrev->pMruNext = pBuf->pMruNext;
		}
		else
		{
			m_pMruListHead = pBuf->pMruNext;
		}
		// get the buffer from the file list
		if (pBuf->pNext)
		{
			pBuf->pNext->pPrev = pBuf->pPrev;
		}
		else
		{
			ASSERT(pFile->BuffersListTail == pBuf);
			pFile->BuffersListTail = pBuf->pPrev;
		}

		if (pBuf->pPrev)
		{
			pBuf->pPrev->pNext = pBuf->pNext;
		}
		else
		{
			ASSERT(pFile->BuffersListHead == pBuf);
			pFile->BuffersListHead = pBuf->pNext;
		}
		pFile->BuffersCount--;
		ASSERT(0 == pBuf->DirtyMask);
		pBuf->ReadMask = 0;
		pBuf->DirtyMask = 0;
		pBuf->pFile = NULL;
		return pBuf;
	}
	return NULL;
}

// read the data by 32-bit MaskToRead (each bit corresponds to 2K)
// to the buffer defined by BufferHeader *pBuf
void CDirectFile::CDirectFileCache::ReadDataBuffer(BufferHeader * pBuf, DWORD MaskToRead)
{
	// check if the required data is already in the buffer
	if (0 != MaskToRead
		&& (pBuf->ReadMask & MaskToRead) == MaskToRead)
	{
		return;
	}
	CSimpleCriticalSectionLock lock(pBuf->pFile->m_FileLock);
	// the operation in progress might change the mask
	if (0 != MaskToRead
		&& (pBuf->ReadMask & MaskToRead) == MaskToRead)
	{
		return;
	}
	MaskToRead &= ~ pBuf->ReadMask;
	// read the required data
	File * pFile = pBuf->pFile;
	LONGLONG StartFilePtr = ULONGLONG(pBuf->PositionKey) << 16;
	char * buf = (char *) pBuf->pBuf;
	// check if the buffer is in the initialized part of the file
	if (0 == (pFile->Flags & FileFlagsReadOnly))
	{
		int WrittenMaskOffset = pBuf->PositionKey >> 3;
		if (WrittenMaskOffset >= pFile->WrittenMaskSize)
		{
			int NewWrittenMaskSize = WrittenMaskOffset + 512;   // 256 more megs
			char * NewWrittenMask = new char[NewWrittenMaskSize];
			if (NULL == NewWrittenMask)
			{
				memset(pBuf->pBuf, 0, 0x10000);
				pBuf->ReadMask = 0xFFFFFFFF;
				return;
			}

			memset(NewWrittenMask, 0, NewWrittenMaskSize);
			memcpy(NewWrittenMask, pFile->m_pWrittenMask, pFile->WrittenMaskSize);
			delete[] pFile->m_pWrittenMask;
			pFile->m_pWrittenMask = NewWrittenMask;
			pFile->WrittenMaskSize = NewWrittenMaskSize;
		}
		ASSERT(pFile->m_pWrittenMask);
		if (0 == (pFile->m_pWrittenMask[WrittenMaskOffset]
				& (1 << (pBuf->PositionKey & 7))))
		{
			pFile->m_pWrittenMask[WrittenMaskOffset] |= 1 << (pBuf->PositionKey & 7);
			memset(pBuf->pBuf, 0, 0x10000);
			pBuf->ReadMask = 0xFFFFFFFF;
			if (0 == pBuf->DirtyMask)
			{
				InterlockedIncrement((PLONG) & pFile->DirtyBuffersCount);
			}
			pBuf->DirtyMask = 0xFFFFFFFF;
			return;
		}
	}

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
		DWORD BytesRead = 0;

		if (0) TRACE("Stored file pointer: %X, actual: %X\n",
					long(pFile->FilePointer), SetFilePointer(pFile->hFile, 0, NULL, FILE_CURRENT));

		if (StartFilePtr != pFile->FilePointer)
		{
			LONG FilePtrH = LONG(StartFilePtr >> 32);
			SetFilePointer(pFile->hFile, (LONG)StartFilePtr, & FilePtrH, FILE_BEGIN);
			pFile->FilePointer = StartFilePtr;
		}
		ReadFile(pFile->hFile, buf, ToRead, & BytesRead, NULL);
		pFile->FilePointer += BytesRead;
		buf += BytesRead;
		StartFilePtr += BytesRead;
		ToRead -= BytesRead;
		if (ToRead)
		{
			memset(buf, 0, ToRead);
		}
		buf += ToRead;
		StartFilePtr += ToRead;
	}
	InterlockedOr(& pBuf->ReadMask, MaskToRead);
	DWORD OldDirtyMask = pBuf->DirtyMask;
	InterlockedAnd(& pBuf->DirtyMask, ~MaskToRead);
	if (0 != OldDirtyMask && 0 == pBuf->DirtyMask)
	{
		InterlockedDecrement((PLONG) & pFile->DirtyBuffersCount);
	}
}

void CDirectFile::BufferHeader::FlushDirtyBuffers()
{
	// flush all unlocked dirty buffers in sequence with pBuf;
	BufferHeader * pDirtyBuf = this;
	BufferHeader * pBuf = pPrev;
	unsigned key = PositionKey;
	ASSERT(NULL != pDirtyBuf);
	ASSERT(pFile);

	CSimpleCriticalSectionLock lock(pFile->m_ListLock);

	while (pBuf != NULL)
	{
		if (key - pBuf->PositionKey <= 2)
		{
			if (0 != pBuf->DirtyMask)
			{
				key = pBuf->PositionKey;
				pDirtyBuf = pBuf;
			}
		}
		else
		{
			break;
		}
		pBuf = pBuf->pPrev;
	}
	pBuf = pDirtyBuf;
	key = pBuf->PositionKey;
	do {
		if (pBuf->DirtyMask)
		{
			CSimpleCriticalSectionLock lock(pFile->m_FileLock);
			LONGLONG StartFilePtr = ULONGLONG(pBuf->PositionKey) << 16;
			unsigned char * buf = (unsigned char *) pBuf->pBuf;
			int WrittenMaskOffset = pBuf->PositionKey >> 3;

			ASSERT(WrittenMaskOffset < pFile->WrittenMaskSize);
			ASSERT(pFile->m_pWrittenMask);

			DWORD mask = InterlockedExchange( (PLONG)& pBuf->DirtyMask, 0);
			if (mask) InterlockedDecrement((PLONG) & pFile->DirtyBuffersCount);
			while (mask != 0)
			{
				size_t ToWrite;
				if (mask == 0xFFFFFFFF)
				{
					mask = 0;
					ToWrite = 0x10000;
				}
				else
				{
					while (0 == (mask & 1))
					{
						mask >>= 1;
						StartFilePtr += 0x800;
						buf += 0x800;
					}
					ToWrite = 0;
					while (mask & 1)
					{
						mask >>= 1;
						ToWrite += 0x800;
					}
				}
				DWORD BytesWritten = 0;
				TRACE("Stored file pointer: %X, actual: %X\n",
					long(pFile->FilePointer), SetFilePointer(pFile->hFile, 0, NULL, FILE_CURRENT));

				if (StartFilePtr != pFile->FilePointer)
				{
					LONG FilePtrH = LONG(StartFilePtr >> 32);
					SetFilePointer(pFile->hFile, (LONG)StartFilePtr, & FilePtrH, FILE_BEGIN);
					pFile->FilePointer = StartFilePtr;
				}
				if (0) TRACE("Writing h=%X, pos=%X, len=%X, "
							"data=%02X %02X %02X %02X %02X %02X %02X %02X\n", pFile->hFile,
							long(StartFilePtr), ToWrite,
							buf[0], buf[1], buf[2], buf[3], buf[4],
							buf[5], buf[6], buf[7]);
				WriteFile(pFile->hFile, buf, ToWrite, & BytesWritten, NULL);
				pFile->FilePointer += BytesWritten;
				buf += ToWrite;
				StartFilePtr += ToWrite;
				if (StartFilePtr > pFile->RealFileLength)
				{
					pFile->RealFileLength = StartFilePtr;
				}
			}
		}
		pBuf = pBuf->pNext;
	}
	while (NULL != pBuf
			&& pBuf->PositionKey - key <= 2);
}

#ifdef _DEBUG
#define VL_ASSERT(expr) if ( ! (expr)) \
	{ TRACE("FALSE ==(" #expr ")\n"); \
	__asm int 3 \
		}
void CDirectFile::File::ValidateList() const
{
	CSimpleCriticalSectionLock lock(m_ListLock);
	BufferHeader const * pBuf = BuffersListHead;
	VL_ASSERT((NULL == BuffersListHead) == (NULL == BuffersListTail));
	int BufCount = 0;
	int DirtyBufCount = 0;
	while (pBuf)
	{
		VL_ASSERT(pBuf->pFile == this);
		if (pBuf == BuffersListHead)
		{
			VL_ASSERT(pBuf->pPrev == 0);
		}
		else
		{
			VL_ASSERT(pBuf->pPrev != 0 && pBuf->pPrev->pNext == pBuf);
			VL_ASSERT(pBuf->pPrev->PositionKey < pBuf->PositionKey);
		}
		if (pBuf == BuffersListTail)
		{
			VL_ASSERT(pBuf->pNext == 0);
		}
		else
		{
			VL_ASSERT(pBuf->pNext != 0);
		}
		BufCount++;
		if (pBuf->DirtyMask)
		{
			DirtyBufCount++;
		}
		pBuf = pBuf->pNext;
	}
	VL_ASSERT(BufCount == BuffersCount && DirtyBufCount == DirtyBuffersCount);
}
#endif

unsigned CDirectFile::CDirectFileCache::_ThreadProc()
{
	while (m_bRunThread)
	{
		do {
			LONGLONG PrefetchPosition;
			LONGLONG PrefetchLength;
			unsigned PrefetchMRU;
			File * pPrefetchFile;
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
			//TRACE("Prefetched buffer at 0x%X\n", long(PrefetchPosition));
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

BOOL CDirectFile::File::InitializeTheRestOfFile()
{
	if (NULL == m_pWrittenMask || (Flags & FileFlagsReadOnly))
	{
		return false;
	}
	// find uninitialized buffer
	unsigned i;
	signed long * pMask = (signed long *) m_pWrittenMask;
	for (i = 0; i < unsigned((FileLength + 0xFFFF) >> (16 + 3 + 2)); i++, pMask++)
	{
		long mask = pMask[0];
		LONGLONG FileOffset = LONGLONG(i) << 21;
		while (mask != 0xFFFFFFFF)
		{
			// read and release all uninitialized buffers
			if (0 == (mask & 1))
			{
				void * ptr;
				LONG count = GetDataBuffer( & ptr, 0x10000, FileOffset, 0);
				if (count)
				{
					ReturnDataBuffer(ptr, 0x10000, ReturnBufferDiscard);
				}
			}
			FileOffset += 0x10000;
			mask = (mask >> 1) | 0x80000000;
		}
	}
	// initialize last buffers
	for (i = unsigned((FileLength + 0xFFFF) >> 16) & ~0x1F; i <
		unsigned((FileLength + 0xFFFF) >> 16); i++)
	{
		if (0 == (m_pWrittenMask[i / 8] & (1 << (i & 7))))
		{
			void * ptr;
			LONG count = GetDataBuffer( & ptr, 0x10000, LONGLONG(i) << 16, 0);
			if (count)
			{
				ReturnDataBuffer(ptr, 0x10000, ReturnBufferDiscard);
			}
		}
	}
	Flush();
	return TRUE;
}

BOOL CDirectFile::File::SetFileLength(LONGLONG NewLength)
{
	// if the file becomes shorter, discard all buffers after the trunk point
#if 0
	void * p;
	GetDataBuffer( & p, 1, NewLength - 1, 0);
	ReturnDataBuffer(p, 1, 0);
#endif
	if (NewLength < FileLength)
	{
		Flush();
	}
	else if (NewLength > FileLength)
	{
	}
	if (NewLength == RealFileLength)
	{
		return TRUE;
	}
	// TODO:
	CSimpleCriticalSectionLock lock(m_FileLock);
	FileLength = NewLength;
	// set the length to the multiple of 2K
	NewLength = (NewLength + 0x7FF) & ~0x7FF;
	LONG MoveHigh = LONG(NewLength>> 32);
	DWORD SizeLow, SizeHigh;
	SetLastError(0);
	FilePointer = -1i64;  // invalidate file pointer
	if ((0xFFFFFFFF != SetFilePointer(hFile, LONG(NewLength), & MoveHigh, FILE_BEGIN)
			|| GetLastError() == NO_ERROR)
		&& SetEndOfFile(hFile)
		&& FlushFileBuffers(hFile))
	{
		SizeLow = ::GetFileSize(hFile, & SizeHigh);
		if (GetLastError() == NO_ERROR)
		{
			RealFileLength = SizeLow | (LONGLONG(SizeHigh) << 32);
			return TRUE;
		}
		else
		{
			return FALSE;
		}
	}
	else
	{
		SizeLow = ::GetFileSize(hFile, & SizeHigh);
		if (GetLastError() == NO_ERROR)
		{
			RealFileLength = SizeLow | (LONGLONG(SizeHigh) << 32);
		}
		return FALSE;
	}

}

BOOL CDirectFile::File::Flush()
{
	// flush all the buffers
	while(1)
	{
		CSimpleCriticalSectionLock lock(m_FileLock);
		BufferHeader * pBuf = BuffersListHead;
		while (1)
		{
			if (NULL == pBuf)
			{
				return TRUE;
			}
			if (0 != pBuf->DirtyMask)
			{
				if (0 != pBuf->LockCount)
				{
					return FALSE;
				}
				pBuf->FlushDirtyBuffers();
				break;
			}
			pBuf = pBuf->pNext;
		}
	}
}
